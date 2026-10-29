#include "gui/AdminPanel.h"

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "controller/ClinicManager.h"
#include "gui/GuiHelpers.h"

namespace scapms::gui {

AdminPanel::AdminPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager)
{
    patientCombo_ = new QComboBox;
    patientCombo_->setToolTip("Patient to delete");
    deletePatientButton_ = new QPushButton("Delete Patient");
    deletePatientButton_->setToolTip("Delete the patient and their appointments (can be undone)");
    appointmentCombo_ = new QComboBox;
    appointmentCombo_->setToolTip("Appointment to remove from the records");
    removeAppointmentButton_ = new QPushButton("Remove Appointment");
    removeAppointmentButton_->setToolTip("Remove the appointment completely (can be undone)");

    auto* operations = new QGroupBox("Administrative operations");
    auto* form = new QFormLayout(operations);
    auto* deleteRow = new QHBoxLayout;
    deleteRow->addWidget(patientCombo_, 1);
    deleteRow->addWidget(deletePatientButton_);
    auto* removeRow = new QHBoxLayout;
    removeRow->addWidget(appointmentCombo_, 1);
    removeRow->addWidget(removeAppointmentButton_);
    form->addRow("Patient:", deleteRow);
    form->addRow("Appointment:", removeRow);

    undoButton_ = new QPushButton("Undo Last Operation");
    undoButton_->setToolTip("Reverse the most recent delete, removal or edit (Ctrl+Z)");
    undoButton_->setMinimumHeight(40);

    history_ = new QListWidget;
    history_->setToolTip("Operations performed this session");
    auto* historyGroup = new QGroupBox("History");
    auto* historyLayout = new QVBoxLayout(historyGroup);
    historyLayout->addWidget(history_);

    auto* root = new QVBoxLayout(this);
    root->addWidget(operations);
    root->addWidget(undoButton_);
    root->addWidget(historyGroup, 1);

    connect(deletePatientButton_, &QPushButton::clicked, this, &AdminPanel::onDeletePatient);
    connect(removeAppointmentButton_, &QPushButton::clicked, this, &AdminPanel::onRemoveAppointment);
    connect(undoButton_, &QPushButton::clicked, this, &AdminPanel::undoLast);
}

void AdminPanel::refresh()
{
    fillPatientCombo(patientCombo_, manager_);

    const QString previous = appointmentCombo_->currentData().toString();
    appointmentCombo_->clear();
    for (const model::Appointment& a : manager_.getAppointments()) {
        appointmentCombo_->addItem(QString("%1 - %2 %3 %4 (%5)")
                                       .arg(qs(a.getId()), qs(a.getPatientId()), qs(a.getDate()), qs(a.getTime()),
                                            qs(model::Appointment::statusToString(a.getStatus()))),
                                   qs(a.getId()));
    }
    const int index = appointmentCombo_->findData(previous);
    if (index >= 0) {
        appointmentCombo_->setCurrentIndex(index);
    }

    if (manager_.canUndo()) {
        undoButton_->setText("Undo: " + qs(manager_.nextUndoDescription()));
        undoButton_->setEnabled(true);
    } else {
        undoButton_->setText("Undo Last Operation (nothing to undo)");
        undoButton_->setEnabled(false);
    }

    history_->clear();
    for (const std::string& entry : manager_.getHistory()) {
        history_->insertItem(0, qs(entry)); // newest first
    }
}

void AdminPanel::onDeletePatient()
{
    const QString id = comboId(patientCombo_);
    if (id.isEmpty()) {
        return;
    }
    if (QMessageBox::question(this, "Delete patient", "Delete patient " + id + " and their appointments?") !=
        QMessageBox::Yes) {
        return;
    }
    if (guarded(this, [this, &id] { manager_.deletePatient(id.toStdString()); })) {
        emit dataChanged();
        emit statusMessage("Patient " + id + " deleted");
    }
}

void AdminPanel::onRemoveAppointment()
{
    const QString id = appointmentCombo_->currentData().toString();
    if (id.isEmpty()) {
        return;
    }
    if (guarded(this, [this, &id] { manager_.removeAppointment(id.toStdString()); })) {
        emit dataChanged();
        emit statusMessage("Appointment " + id + " removed");
    }
}

void AdminPanel::undoLast()
{
    std::string description;
    if (guarded(this, [this, &description] { description = manager_.undo(); })) {
        emit dataChanged();
        emit statusMessage("Undone: " + qs(description));
    }
}

} // namespace scapms::gui
