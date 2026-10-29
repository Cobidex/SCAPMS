#include "model/FollowUpPatient.h"

#include "model/Appointment.h"
#include "utils/Exceptions.h"
#include "utils/TextUtils.h"

namespace scapms::model {

FollowUpPatient::FollowUpPatient(const std::string& id, const std::string& name, int age,
                                 const std::string& contact, const std::string& previousVisitDate,
                                 const std::string& doctorName)
    : PatientRecord(id, name, age, contact)
{
    setPreviousVisitDate(previousVisitDate);
    setDoctorName(doctorName);
}

void FollowUpPatient::setPreviousVisitDate(const std::string& date)
{
    const std::string trimmed = utils::trim(date);
    if (!utils::isValidDate(trimmed)) {
        throw utils::ValidationException("Previous visit date must be in YYYY-MM-DD format");
    }
    previousVisitDate_ = trimmed;
}

void FollowUpPatient::setDoctorName(const std::string& doctor)
{
    const std::string trimmed = utils::trim(doctor);
    if (trimmed.empty()) {
        throw utils::ValidationException("Doctor name cannot be empty");
    }
    utils::requireNoCommas(trimmed, "Doctor name");
    doctorName_ = trimmed;
}

std::string FollowUpPatient::getCategory() const { return "Follow-Up"; }

std::string FollowUpPatient::getPriority() const { return "Scheduled"; }

std::string FollowUpPatient::describe() const
{
    return "Previous visit " + previousVisitDate_ + " with " + doctorName_;
}

std::unique_ptr<PatientRecord> FollowUpPatient::clone() const
{
    return std::make_unique<FollowUpPatient>(*this);
}

std::vector<std::string> FollowUpPatient::toCsvFields() const
{
    return {previousVisitDate_, doctorName_};
}

void FollowUpPatient::scheduleAppointment(Appointment& appointment)
{
    // YYYY-MM-DD strings compare in date order.
    if (appointment.getDate() <= previousVisitDate_) {
        throw utils::ValidationException("Follow-up appointment must be after the previous visit on " +
                                         previousVisitDate_);
    }
    addAppointmentId(appointment.getId());
}

void FollowUpPatient::cancelAppointment(const std::string& appointmentId)
{
    removeAppointmentId(appointmentId);
}

} // namespace scapms::model
