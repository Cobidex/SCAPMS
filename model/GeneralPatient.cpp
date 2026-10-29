#include "model/GeneralPatient.h"

#include "model/Appointment.h"
#include "utils/Exceptions.h"
#include "utils/TextUtils.h"

namespace scapms::model {

GeneralPatient::GeneralPatient(const std::string& id, const std::string& name, int age, const std::string& contact,
                               const std::string& reasonForVisit)
    : PatientRecord(id, name, age, contact)
{
    setReasonForVisit(reasonForVisit);
}

void GeneralPatient::setReasonForVisit(const std::string& reason)
{
    const std::string trimmed = utils::trim(reason);
    if (trimmed.empty()) {
        throw utils::ValidationException("Reason for visit cannot be empty");
    }
    utils::requireNoCommas(trimmed, "Reason for visit");
    reasonForVisit_ = trimmed;
}

std::string GeneralPatient::getCategory() const { return "General"; }

std::string GeneralPatient::getPriority() const { return "Normal"; }

std::string GeneralPatient::describe() const
{
    return "Reason for visit: " + reasonForVisit_;
}

std::unique_ptr<PatientRecord> GeneralPatient::clone() const
{
    return std::make_unique<GeneralPatient>(*this);
}

std::vector<std::string> GeneralPatient::toCsvFields() const
{
    return {reasonForVisit_, ""};
}

void GeneralPatient::scheduleAppointment(Appointment& appointment)
{
    if (appointment.getDate() == utils::today()) {
        throw utils::ValidationException("Same-day appointments are only available for emergency patients");
    }
    addAppointmentId(appointment.getId());
}

void GeneralPatient::cancelAppointment(const std::string& appointmentId)
{
    removeAppointmentId(appointmentId);
}

} // namespace scapms::model
