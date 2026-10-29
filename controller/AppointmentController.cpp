#include "controller/AppointmentController.h"

#include "model/Appointable.h"
#include "utils/Exceptions.h"
#include "utils/IDGenerator.h"
#include "utils/TextUtils.h"

namespace scapms::controller {

using model::Appointable;
using model::Appointment;
using model::PatientRecord;
using utils::NotFoundException;
using utils::ValidationException;

std::string AppointmentController::schedule(const std::string& patientId, const std::string& date,
                                            const std::string& time, const std::string& reason)
{
    PatientRecord* patient = db_.findPatient(patientId);
    if (!patient) {
        throw NotFoundException("Patient " + patientId + " not found");
    }
    // Cross-cast to the interface: every patient type is Appointable.
    auto* bookable = dynamic_cast<Appointable*>(patient);
    if (!bookable) {
        throw ValidationException("This patient type cannot take appointments");
    }
    if (!utils::isValidDate(date)) {
        throw ValidationException("Date must be a valid date in YYYY-MM-DD format");
    }
    if (!utils::isValidTime(time)) {
        throw ValidationException("Time must be a valid time in HH:MM format");
    }
    const std::string today = utils::today();
    if (date < today) {
        throw ValidationException("Appointment date cannot be in the past");
    }
    if (date == today && time < utils::currentTime()) {
        throw ValidationException("Appointment time has already passed today");
    }
    for (const Appointment& existing : db_.getAppointments()) {
        if (existing.getPatientId() == patientId && existing.getStatus() == Appointment::Status::Pending &&
            existing.getDate() == date && existing.getTime() == time) {
            throw ValidationException("Patient already has an appointment at that time");
        }
    }

    Appointment appointment(utils::IDGenerator::peekNextAppointmentId(), patientId, date, time, reason);
    bookable->scheduleAppointment(appointment); // subclass rules apply here
    utils::IDGenerator::nextAppointmentId();
    try {
        db_.addAppointment(appointment);
    } catch (...) {
        patient->removeAppointmentId(appointment.getId());
        throw;
    }
    return appointment.getId();
}

Appointment& AppointmentController::requirePending(const std::string& appointmentId, const std::string& action)
{
    Appointment* appointment = db_.findAppointment(appointmentId);
    if (!appointment) {
        throw NotFoundException("Appointment " + appointmentId + " not found");
    }
    if (appointment->getStatus() != Appointment::Status::Pending) {
        throw ValidationException("Only pending appointments can be " + action);
    }
    return *appointment;
}

void AppointmentController::cancel(const std::string& appointmentId)
{
    Appointment& appointment = requirePending(appointmentId, "cancelled");
    PatientRecord* patient = db_.findPatient(appointment.getPatientId());
    if (patient && patient->hasAppointmentId(appointmentId)) {
        if (auto* bookable = dynamic_cast<Appointable*>(patient)) {
            bookable->cancelAppointment(appointmentId);
        }
    }
    appointment.setStatus(Appointment::Status::Cancelled);
}

void AppointmentController::complete(const std::string& appointmentId)
{
    requirePending(appointmentId, "completed").setStatus(Appointment::Status::Completed);
}

} // namespace scapms::controller
