#include "model/Appointment.h"

#include "model/PatientRecord.h"
#include "utils/Exceptions.h"
#include "utils/TextUtils.h"

namespace scapms::model {

using utils::ValidationException;

Appointment::Appointment(const std::string& id, const std::string& patientId, const std::string& date,
                         const std::string& time, const std::string& reason, Status status)
    : status_(status)
{
    if (id.size() != 5 || id[0] != 'A' || !utils::isDigits(id.substr(1))) {
        throw ValidationException("Appointment ID must look like A0001 (A followed by 4 digits)");
    }
    PatientRecord::validateId(patientId);
    if (!utils::isValidDate(date)) {
        throw ValidationException("Date must be a valid date in YYYY-MM-DD format");
    }
    if (!utils::isValidTime(time)) {
        throw ValidationException("Time must be a valid time in HH:MM format");
    }
    id_ = id;
    patientId_ = patientId;
    date_ = date;
    time_ = time;
    setReason(reason);
}

void Appointment::setReason(const std::string& reason)
{
    const std::string trimmed = utils::trim(reason);
    if (trimmed.empty()) {
        throw ValidationException("Reason cannot be empty");
    }
    utils::requireNoCommas(trimmed, "Reason");
    reason_ = trimmed;
}

std::string Appointment::statusToString(Status status)
{
    switch (status) {
    case Status::Pending:
        return "Pending";
    case Status::Completed:
        return "Completed";
    case Status::Cancelled:
        return "Cancelled";
    }
    return "Pending";
}

Appointment::Status Appointment::statusFromString(const std::string& text)
{
    const std::string lower = utils::toLower(utils::trim(text));
    if (lower == "pending") {
        return Status::Pending;
    }
    if (lower == "completed") {
        return Status::Completed;
    }
    if (lower == "cancelled") {
        return Status::Cancelled;
    }
    throw ValidationException("Unknown appointment status '" + text + "'");
}

} // namespace scapms::model
