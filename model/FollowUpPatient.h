#pragma once

#include "model/Appointable.h"
#include "model/PatientRecord.h"

namespace scapms::model {

// Return visit. Priority Scheduled. Appointments must come after the
// previous visit.
class FollowUpPatient : public PatientRecord, public Appointable {
public:
    FollowUpPatient(const std::string& id, const std::string& name, int age, const std::string& contact,
                    const std::string& previousVisitDate, const std::string& doctorName);

    const std::string& getPreviousVisitDate() const { return previousVisitDate_; }
    const std::string& getDoctorName() const { return doctorName_; }
    void setPreviousVisitDate(const std::string& date);
    void setDoctorName(const std::string& doctor);

    std::string getCategory() const override;
    std::string getPriority() const override;
    std::string describe() const override;
    std::unique_ptr<PatientRecord> clone() const override;
    std::vector<std::string> toCsvFields() const override;

    void scheduleAppointment(Appointment& appointment) override;
    void cancelAppointment(const std::string& appointmentId) override;

private:
    std::string previousVisitDate_;
    std::string doctorName_;
};

} // namespace scapms::model
