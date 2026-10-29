#pragma once

#include <string>

#include "model/ClinicDatabase.h"

namespace scapms::controller {

// Validates and applies appointment changes. Removing an appointment is an
// admin operation and goes through ClinicManager so it can be undone.
class AppointmentController {
public:
    explicit AppointmentController(model::ClinicDatabase& db) : db_(db) {}

    // Returns the new appointment ID.
    std::string schedule(const std::string& patientId, const std::string& date, const std::string& time,
                         const std::string& reason);
    void cancel(const std::string& appointmentId);
    void complete(const std::string& appointmentId);

private:
    model::Appointment& requirePending(const std::string& appointmentId, const std::string& action);

    model::ClinicDatabase& db_;
};

} // namespace scapms::controller
