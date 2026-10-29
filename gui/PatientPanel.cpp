#include "gui/PatientPanel.h"

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
#include <QSpinBox>
#include <QStackedWidget>
#include <QTableWidget>
#include <QVBoxLayout>

#include "controller/ClinicManager.h"
#include "gui/GuiHelpers.h"
#include "model/EmergencyPatient.h"
#include "model/FollowUpPatient.h"
#include "model/GeneralPatient.h"
#include "utils/IDGenerator.h"

namespace scapms::gui {

using model::EmergencyPatient;
using model::FollowUpPatient;
using model::GeneralPatient;
using model::PatientRecord;

namespace {
enum TypeIndex { General = 0, Emergency = 1, FollowUp = 2 };
const char* const kIdInUseStyle = "border: 2px solid #C0392B;";
} // namespace

PatientPanel::PatientPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager)
{
    // Basic details: aligned label/field pairs.
    idEdit_ = new QLineEdit;
    idEdit_->setToolTip("Patient ID, e.g. P0001. Pre-filled with the next free ID.");
    idHint_ = new QLabel;
    idHint_->setStyleSheet("color: #C0392B;");
    nameEdit_ = new QLineEdit;
    nameEdit_->setPlaceholderText("Full name");
    nameEdit_->setToolTip("Required. Commas are not allowed.");
    ageEdit_ = new QLineEdit;
    ageEdit_->setPlaceholderText("0-120");
    ageEdit_->setToolTip("Age must be between 0 and 120");
    contactEdit_ = new QLineEdit;
    contactEdit_->setPlaceholderText("e.g. 08031234567");
    contactEdit_->setToolTip("Phone number, 7 to 15 digits");
    typeCombo_ = new QComboBox;
    typeCombo_->addItems({"General", "Emergency", "Follow-Up"});
    typeCombo_->setToolTip("Patient type. The fields below change with the type. Locked while editing.");

    auto* form = new QFormLayout;
    form->addRow("Patient ID:", idEdit_);
    form->addRow("", idHint_);
    form->addRow("Name:", nameEdit_);
    form->addRow("Age:", ageEdit_);
    form->addRow("Contact:", contactEdit_);
    form->addRow("Type:", typeCombo_);

    // Type-specific fields: one page per patient type.
    reasonEdit_ = new QLineEdit;
    reasonEdit_->setToolTip("Why the patient is visiting");
    auto* generalPage = new QWidget;
    auto* generalForm = new QFormLayout(generalPage);
    generalForm->addRow("Reason for visit:", reasonEdit_);

    severitySpin_ = new QSpinBox;
    severitySpin_->setRange(1, 5);
    severitySpin_->setValue(3);
    severitySpin_->setToolTip("1 = minor, 5 = critical. Severity 4-5 marks appointments URGENT.");
    triageEdit_ = new QLineEdit;
    triageEdit_->setToolTip("Notes from triage (optional)");
    auto* emergencyPage = new QWidget;
    auto* emergencyForm = new QFormLayout(emergencyPage);
    emergencyForm->addRow("Severity (1-5):", severitySpin_);
    emergencyForm->addRow("Triage notes:", triageEdit_);

    previousVisitEdit_ = new QDateEdit(QDate::currentDate().addDays(-14));
    previousVisitEdit_->setCalendarPopup(true);
    previousVisitEdit_->setDisplayFormat("yyyy-MM-dd");
    previousVisitEdit_->setToolTip("Follow-up appointments must be after this date");
    doctorEdit_ = new QLineEdit;
    doctorEdit_->setToolTip("Doctor who saw the patient last time");
    auto* followUpPage = new QWidget;
    auto* followUpForm = new QFormLayout(followUpPage);
    followUpForm->addRow("Previous visit:", previousVisitEdit_);
    followUpForm->addRow("Doctor:", doctorEdit_);

    typeStack_ = new QStackedWidget;
    typeStack_->addWidget(generalPage);
    typeStack_->addWidget(emergencyPage);
    typeStack_->addWidget(followUpPage);
    auto* typeGroup = new QGroupBox("Type-specific details");
    auto* typeLayout = new QVBoxLayout(typeGroup);
    typeLayout->addWidget(typeStack_);

    addButton_ = new QPushButton("Add");
    addButton_->setToolTip("Register a new patient");
    updateButton_ = new QPushButton("Update");
    updateButton_->setToolTip("Save changes to the selected patient (can be undone with Ctrl+Z)");
    deleteButton_ = new QPushButton("Delete");
    deleteButton_->setToolTip("Deleting can be undone from the Admin tab (Ctrl+Z)");
    clearButton_ = new QPushButton("Clear");
    clearButton_->setToolTip("Clear the form and start a new registration");
    auto* buttons = new QHBoxLayout;
    buttons->addWidget(addButton_);
    buttons->addWidget(updateButton_);
    buttons->addWidget(deleteButton_);
    buttons->addWidget(clearButton_);

    auto* left = new QVBoxLayout;
    left->addLayout(form);
    left->addWidget(typeGroup);
    left->addLayout(buttons);
    left->addStretch();
    auto* leftWidget = new QWidget;
    leftWidget->setLayout(left);
    leftWidget->setFixedWidth(360);

    table_ = new QTableWidget;
    setupTable(table_, patientHeaders());
    table_->setToolTip("Click a patient to edit. Emergency rows are red, Follow-Up rows blue.");

    auto* root = new QHBoxLayout(this);
    root->addWidget(leftWidget);
    root->addWidget(table_, 1);

    connect(typeCombo_, &QComboBox::currentIndexChanged, typeStack_, &QStackedWidget::setCurrentIndex);
    connect(idEdit_, &QLineEdit::textChanged, this, &PatientPanel::onIdChanged);
    connect(table_, &QTableWidget::itemSelectionChanged, this, &PatientPanel::onSelectionChanged);
    connect(addButton_, &QPushButton::clicked, this, &PatientPanel::onAdd);
    connect(updateButton_, &QPushButton::clicked, this, &PatientPanel::onUpdate);
    connect(deleteButton_, &QPushButton::clicked, this, &PatientPanel::onDelete);
    connect(clearButton_, &QPushButton::clicked, this, &PatientPanel::clearForm);

    clearForm();
}

void PatientPanel::refresh()
{
    table_->blockSignals(true);
    table_->setRowCount(0);
    manager_.processAll([this](const PatientRecord& p) { addPatientRow(table_, p); });
    table_->resizeColumnsToContents();
    table_->blockSignals(false);

    if (!editingId_.isEmpty() && !manager_.patientExists(editingId_.toStdString())) {
        clearForm();
    } else if (editingId_.isEmpty()) {
        idEdit_->setText(qs(utils::IDGenerator::peekNextPatientId()));
    } else if (PatientRecord* p = manager_.findPatient(editingId_.toStdString())) {
        fillForm(*p); // reflects an undo of the edit being shown
    }
}

std::unique_ptr<PatientRecord> PatientPanel::buildFromForm() const
{
    const std::string id = idEdit_->text().trimmed().toStdString();
    const std::string name = nameEdit_->text().toStdString();
    const std::string contact = contactEdit_->text().toStdString();
    bool ok = false;
    const int age = ageEdit_->text().trimmed().toInt(&ok);
    if (!ok) {
        throw utils::ValidationException("Age must be a number");
    }
    switch (typeCombo_->currentIndex()) {
    case Emergency:
        return std::make_unique<EmergencyPatient>(id, name, age, contact, severitySpin_->value(),
                                                  triageEdit_->text().toStdString());
    case FollowUp:
        return std::make_unique<FollowUpPatient>(id, name, age, contact,
                                                 previousVisitEdit_->date().toString("yyyy-MM-dd").toStdString(),
                                                 doctorEdit_->text().toStdString());
    default:
        return std::make_unique<GeneralPatient>(id, name, age, contact, reasonEdit_->text().toStdString());
    }
}

void PatientPanel::onAdd()
{
    QString id;
    const bool ok = guarded(this, [this, &id] {
        std::unique_ptr<PatientRecord> patient = buildFromForm();
        id = qs(patient->getId());
        manager_.addPatient(std::move(patient));
    });
    if (ok) {
        clearForm();
        emit dataChanged();
        emit statusMessage("Patient " + id + " added");
    }
}

void PatientPanel::onUpdate()
{
    if (editingId_.isEmpty()) {
        QMessageBox::information(this, "Update", "Select a patient in the table first.");
        return;
    }
    if (guarded(this, [this] { manager_.editPatient(buildFromForm()); })) {
        emit dataChanged();
        emit statusMessage("Patient " + editingId_ + " updated");
    }
}

void PatientPanel::onDelete()
{
    if (editingId_.isEmpty()) {
        QMessageBox::information(this, "Delete", "Select a patient in the table first.");
        return;
    }
    const auto answer = QMessageBox::question(
        this, "Delete patient",
        "Delete patient " + editingId_ + " and their appointments?\nYou can undo this from the Admin tab.");
    if (answer != QMessageBox::Yes) {
        return;
    }
    const QString id = editingId_;
    if (guarded(this, [this, &id] { manager_.deletePatient(id.toStdString()); })) {
        clearForm();
        emit dataChanged();
        emit statusMessage("Patient " + id + " deleted");
    }
}

void PatientPanel::clearForm()
{
    editingId_.clear();
    table_->clearSelection();
    nameEdit_->clear();
    ageEdit_->clear();
    contactEdit_->clear();
    reasonEdit_->clear();
    severitySpin_->setValue(3);
    triageEdit_->clear();
    previousVisitEdit_->setDate(QDate::currentDate().addDays(-14));
    doctorEdit_->clear();
    setEditMode(false);
    idEdit_->setText(qs(utils::IDGenerator::peekNextPatientId()));
    nameEdit_->setFocus();
}

void PatientPanel::setEditMode(bool editing)
{
    idEdit_->setReadOnly(editing);
    typeCombo_->setEnabled(!editing);
    addButton_->setEnabled(!editing);
    updateButton_->setEnabled(editing);
    deleteButton_->setEnabled(editing);
    idEdit_->setStyleSheet("");
    idHint_->clear();
}

void PatientPanel::onIdChanged(const QString& text)
{
    if (!editingId_.isEmpty()) {
        return;
    }
    const bool inUse = manager_.patientExists(text.trimmed().toStdString());
    idEdit_->setStyleSheet(inUse ? kIdInUseStyle : "");
    idHint_->setText(inUse ? "ID already in use" : "");
}

void PatientPanel::onSelectionChanged()
{
    const QString id = selectedRowId(table_);
    if (id.isEmpty()) {
        return;
    }
    PatientRecord* patient = manager_.findPatient(id.toStdString());
    if (!patient) {
        return;
    }
    editingId_ = id;
    fillForm(*patient);
    setEditMode(true);
    manager_.touch(patient);
    emit patientAccessed();
}

void PatientPanel::fillForm(const PatientRecord& patient)
{
    idEdit_->blockSignals(true);
    idEdit_->setText(qs(patient.getId()));
    idEdit_->blockSignals(false);
    nameEdit_->setText(qs(patient.getName()));
    ageEdit_->setText(QString::number(patient.getAge()));
    contactEdit_->setText(qs(patient.getContact()));

    if (const auto* general = dynamic_cast<const GeneralPatient*>(&patient)) {
        typeCombo_->setCurrentIndex(General);
        reasonEdit_->setText(qs(general->getReasonForVisit()));
    } else if (const auto* emergency = dynamic_cast<const EmergencyPatient*>(&patient)) {
        typeCombo_->setCurrentIndex(Emergency);
        severitySpin_->setValue(emergency->getSeverity());
        triageEdit_->setText(qs(emergency->getTriageNotes()));
    } else if (const auto* followUp = dynamic_cast<const FollowUpPatient*>(&patient)) {
        typeCombo_->setCurrentIndex(FollowUp);
        previousVisitEdit_->setDate(QDate::fromString(qs(followUp->getPreviousVisitDate()), "yyyy-MM-dd"));
        doctorEdit_->setText(qs(followUp->getDoctorName()));
    }
}

} // namespace scapms::gui
