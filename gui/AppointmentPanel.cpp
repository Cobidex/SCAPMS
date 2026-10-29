#include "gui/AppointmentPanel.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTime>
#include <QTimeEdit>
#include <QVBoxLayout>

#include "controller/ClinicManager.h"
#include "gui/GuiHelpers.h"

namespace scapms::gui {

using model::Appointment;

AppointmentPanel::AppointmentPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager)
{
    patientCombo_ = new QComboBox;
    patientCombo_->setToolTip("Patient to book");
    dateEdit_ = new QDateEdit(QDate::currentDate());
    dateEdit_->setCalendarPopup(true);
    dateEdit_->setDisplayFormat("yyyy-MM-dd");
    dateEdit_->setToolTip("Appointment date. Only emergency patients can be booked for today.");
    timeEdit_ = new QTimeEdit(QTime(9, 0));
    timeEdit_->setDisplayFormat("HH:mm");
    timeEdit_->setToolTip("Appointment time (24-hour)");
    reasonEdit_ = new QLineEdit;
    reasonEdit_->setPlaceholderText("Reason for the appointment");
    reasonEdit_->setToolTip("Required. Commas are not allowed.");

    auto* form = new QFormLayout;
    form->addRow("Patient:", patientCombo_);
    form->addRow("Date:", dateEdit_);
    form->addRow("Time:", timeEdit_);
    form->addRow("Reason:", reasonEdit_);

    scheduleButton_ = new QPushButton("Schedule");
    scheduleButton_->setToolTip("Book the appointment. Enabled once a reason is entered.");
    scheduleButton_->setEnabled(false);
    cancelButton_ = new QPushButton("Cancel Selected");
    cancelButton_->setToolTip("Cancel the selected pending appointment");
    completeButton_ = new QPushButton("Mark Completed");
    completeButton_->setToolTip("Mark the selected pending appointment as attended");
    auto* buttons = new QHBoxLayout;
    buttons->addWidget(scheduleButton_);
    buttons->addWidget(cancelButton_);
    buttons->addWidget(completeButton_);

    auto* formGroup = new QGroupBox("New appointment");
    auto* formLayout = new QVBoxLayout(formGroup);
    formLayout->addLayout(form);
    formLayout->addLayout(buttons);

    filterCombo_ = new QComboBox;
    filterCombo_->addItems({"All", "Pending", "Completed", "Cancelled"});
    filterCombo_->setToolTip("Show only appointments with this status");
    auto* filterRow = new QHBoxLayout;
    filterRow->addWidget(new QLabel("Show:"));
    filterRow->addWidget(filterCombo_);
    filterRow->addStretch();

    table_ = new QTableWidget;
    setupTable(table_, {"ID", "Patient", "Date", "Time", "Reason", "Status"});
    table_->setToolTip("Pending = amber, Completed = green, Cancelled = grey");

    auto* root = new QVBoxLayout(this);
    root->addWidget(formGroup);
    root->addLayout(filterRow);
    root->addWidget(table_, 1);

    connect(reasonEdit_, &QLineEdit::textChanged, this, &AppointmentPanel::onReasonChanged);
    connect(scheduleButton_, &QPushButton::clicked, this, &AppointmentPanel::onSchedule);
    connect(cancelButton_, &QPushButton::clicked, this, &AppointmentPanel::onCancel);
    connect(completeButton_, &QPushButton::clicked, this, &AppointmentPanel::onComplete);
    connect(filterCombo_, &QComboBox::currentIndexChanged, this, &AppointmentPanel::refresh);
}

void AppointmentPanel::refresh()
{
    fillPatientCombo(patientCombo_, manager_);

    const QString filter = filterCombo_->currentText();
    table_->setRowCount(0);
    for (const Appointment& a : manager_.getAppointments()) {
        const QString status = qs(Appointment::statusToString(a.getStatus()));
        if (filter != "All" && filter != status) {
            continue;
        }
        QString patient = qs(a.getPatientId());
        if (const auto* p = manager_.findPatient(a.getPatientId())) {
            patient += " - " + qs(p->getName());
        }
        const QStringList cells{qs(a.getId()), patient, qs(a.getDate()), qs(a.getTime()), qs(a.getReason()), status};
        const QColor colour = statusColour(status);
        const int row = table_->rowCount();
        table_->insertRow(row);
        for (int col = 0; col < cells.size(); ++col) {
            auto* item = new QTableWidgetItem(cells[col]);
            item->setBackground(colour);
            item->setForeground(Qt::black);
            table_->setItem(row, col, item);
        }
        table_->item(row, 0)->setData(Qt::UserRole, qs(a.getId()));
    }
    table_->resizeColumnsToContents();
}

QString AppointmentPanel::selectedAppointmentId()
{
    const QString id = selectedRowId(table_);
    if (id.isEmpty()) {
        QMessageBox::information(this, "Appointments", "Select an appointment in the table first.");
    }
    return id;
}

void AppointmentPanel::onReasonChanged(const QString& text)
{
    scheduleButton_->setEnabled(!text.isEmpty());
}

void AppointmentPanel::onSchedule()
{
    const QString patientId = comboId(patientCombo_);
    if (patientId.isEmpty()) {
        QMessageBox::information(this, "Schedule", "Register a patient first.");
        return;
    }
    std::string id;
    const bool ok = guarded(this, [this, &patientId, &id] {
        id = manager_.scheduleAppointment(patientId.toStdString(),
                                          dateEdit_->date().toString("yyyy-MM-dd").toStdString(),
                                          timeEdit_->time().toString("HH:mm").toStdString(),
                                          reasonEdit_->text().toStdString());
    });
    if (ok) {
        reasonEdit_->clear();
        emit dataChanged();
        emit statusMessage("Appointment " + qs(id) + " scheduled for " + patientId);
    }
}

void AppointmentPanel::onCancel()
{
    const QString id = selectedAppointmentId();
    if (!id.isEmpty() && guarded(this, [this, &id] { manager_.cancelAppointment(id.toStdString()); })) {
        emit dataChanged();
        emit statusMessage("Appointment " + id + " cancelled");
    }
}

void AppointmentPanel::onComplete()
{
    const QString id = selectedAppointmentId();
    if (!id.isEmpty() && guarded(this, [this, &id] { manager_.completeAppointment(id.toStdString()); })) {
        emit dataChanged();
        emit statusMessage("Appointment " + id + " marked completed");
    }
}

} // namespace scapms::gui
