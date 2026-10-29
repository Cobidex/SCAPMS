#pragma once

#include <string>

namespace scapms::model {

class Appointment;

// Interface for anything that can be booked. No data, only behaviour.
// scheduleAppointment takes a non-const reference so a subclass can adjust
// the appointment (EmergencyPatient marks urgent reasons).
class Appointable {
public:
    virtual ~Appointable() = default;
    virtual void scheduleAppointment(Appointment& appointment) = 0;
    virtual void cancelAppointment(const std::string& appointmentId) = 0;
};

} // namespace scapms::model
