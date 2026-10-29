#include "model/PatientRecord.h"

#include "utils/Exceptions.h"
#include "utils/TextUtils.h"

namespace scapms::model {

using utils::NotFoundException;
using utils::ValidationException;

PatientRecord::PatientRecord(const std::string& id, const std::string& name, int age, const std::string& contact)
{
    validateId(id);
    id_ = id;
    setName(name);
    setAge(age);
    setContact(contact);
}

void PatientRecord::validateId(const std::string& id)
{
    if (id.size() != 5 || id[0] != 'P' || !utils::isDigits(id.substr(1))) {
        throw ValidationException("Patient ID must look like P0001 (P followed by 4 digits)");
    }
}

void PatientRecord::setName(const std::string& name)
{
    const std::string trimmed = utils::trim(name);
    if (trimmed.empty()) {
        throw ValidationException("Name cannot be empty");
    }
    utils::requireNoCommas(trimmed, "Name");
    name_ = trimmed;
}

void PatientRecord::setAge(int age)
{
    if (age < 0 || age > 120) {
        throw ValidationException("Patient age must be between 0 and 120");
    }
    age_ = age;
}

void PatientRecord::setContact(const std::string& contact)
{
    const std::string trimmed = utils::trim(contact);
    if (trimmed.empty()) {
        throw ValidationException("Contact cannot be empty");
    }
    if (!utils::isDigits(trimmed) || trimmed.size() < 7 || trimmed.size() > 15) {
        throw ValidationException("Contact must be a phone number of 7 to 15 digits");
    }
    contact_ = trimmed;
}

bool PatientRecord::hasAppointmentId(const std::string& appointmentId) const
{
    for (const std::string& existing : appointmentIds_) {
        if (existing == appointmentId) {
            return true;
        }
    }
    return false;
}

void PatientRecord::addAppointmentId(const std::string& appointmentId)
{
    if (!hasAppointmentId(appointmentId)) {
        appointmentIds_.push_back(appointmentId);
    }
}

void PatientRecord::removeAppointmentId(const std::string& appointmentId)
{
    for (std::size_t i = 0; i < appointmentIds_.size(); ++i) {
        if (appointmentIds_[i] == appointmentId) {
            appointmentIds_.erase(appointmentIds_.begin() + static_cast<long>(i));
            return;
        }
    }
    throw NotFoundException("Appointment " + appointmentId + " is not linked to patient " + id_);
}

} // namespace scapms::model
