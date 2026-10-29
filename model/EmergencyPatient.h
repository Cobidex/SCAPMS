#pragma once

#include "model/Appointable.h"
#include "model/PatientRecord.h"

namespace scapms::model {

// Urgent case. Priority High. Can be booked for today; severity 4-5 marks
// the appointment reason as URGENT.
class EmergencyPatient : public PatientRecord, public Appointable {
public:
    EmergencyPatient(const std::string& id, const std::string& name, int age, const std::string& contact,
                     int severity, const std::string& triageNotes);

    int getSeverity() const { return severity_; }
    const std::string& getTriageNotes() const { return triageNotes_; }
    void setSeverity(int severity);
    void setTriageNotes(const std::string& notes);

    std::string getCategory() const override;
    std::string getPriority() const override;
    std::string describe() const override;
    std::unique_ptr<PatientRecord> clone() const override;
    std::vector<std::string> toCsvFields() const override;

    void scheduleAppointment(Appointment& appointment) override;
    void cancelAppointment(const std::string& appointmentId) override;

private:
    int severity_ = 1;
    std::string triageNotes_;
};

} // namespace scapms::model
