#pragma once

#include "model/Appointable.h"
#include "model/PatientRecord.h"

namespace scapms::model {

// Routine visit. Priority Normal. Same-day bookings are not allowed.
class GeneralPatient : public PatientRecord, public Appointable {
public:
    GeneralPatient(const std::string& id, const std::string& name, int age, const std::string& contact,
                   const std::string& reasonForVisit);

    const std::string& getReasonForVisit() const { return reasonForVisit_; }
    void setReasonForVisit(const std::string& reason);

    std::string getCategory() const override;
    std::string getPriority() const override;
    std::string describe() const override;
    std::unique_ptr<PatientRecord> clone() const override;
    std::vector<std::string> toCsvFields() const override;

    void scheduleAppointment(Appointment& appointment) override;
    void cancelAppointment(const std::string& appointmentId) override;

private:
    std::string reasonForVisit_;
};

} // namespace scapms::model
