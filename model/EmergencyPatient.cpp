#include "model/EmergencyPatient.h"

#include "model/Appointment.h"
#include "utils/Exceptions.h"
#include "utils/TextUtils.h"

namespace scapms::model {

EmergencyPatient::EmergencyPatient(const std::string& id, const std::string& name, int age,
                                   const std::string& contact, int severity, const std::string& triageNotes)
    : PatientRecord(id, name, age, contact)
{
    setSeverity(severity);
    setTriageNotes(triageNotes);
}

void EmergencyPatient::setSeverity(int severity)
{
    if (severity < 1 || severity > 5) {
        throw utils::ValidationException("Severity must be between 1 and 5");
    }
    severity_ = severity;
}

void EmergencyPatient::setTriageNotes(const std::string& notes)
{
    const std::string trimmed = utils::trim(notes);
    utils::requireNoCommas(trimmed, "Triage notes");
    triageNotes_ = trimmed;
}

std::string EmergencyPatient::getCategory() const { return "Emergency"; }

std::string EmergencyPatient::getPriority() const { return "High Priority"; }

std::string EmergencyPatient::describe() const
{
    std::string text = "Severity " + std::to_string(severity_) + "/5";
    if (!triageNotes_.empty()) {
        text += "; triage: " + triageNotes_;
    }
    return text;
}

std::unique_ptr<PatientRecord> EmergencyPatient::clone() const
{
    return std::make_unique<EmergencyPatient>(*this);
}

std::vector<std::string> EmergencyPatient::toCsvFields() const
{
    return {std::to_string(severity_), triageNotes_};
}

void EmergencyPatient::scheduleAppointment(Appointment& appointment)
{
    const std::string& reason = appointment.getReason();
    if (severity_ >= 4 && reason.rfind("URGENT", 0) != 0) {
        appointment.setReason("URGENT: " + reason);
    }
    addAppointmentId(appointment.getId());
}

void EmergencyPatient::cancelAppointment(const std::string& appointmentId)
{
    removeAppointmentId(appointmentId);
}

} // namespace scapms::model
