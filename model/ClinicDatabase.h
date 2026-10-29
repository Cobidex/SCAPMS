#pragma once

#include <memory>
#include <string>
#include <vector>

#include "model/Appointment.h"
#include "model/PatientRecord.h"

namespace scapms::model {

// Owns every patient and appointment (composition): destroying the
// database destroys them too. Everyone else gets non-owning pointers.
class ClinicDatabase {
public:
    void addPatient(std::unique_ptr<PatientRecord> patient);
    std::unique_ptr<PatientRecord> removePatient(const std::string& id);
    void restorePatient(std::unique_ptr<PatientRecord> patient);
    // Swaps in a record with the same ID and returns the old one.
    std::unique_ptr<PatientRecord> replacePatient(std::unique_ptr<PatientRecord> patient);

    PatientRecord* findPatient(const std::string& id) const;
    std::vector<PatientRecord*> getPatients() const;
    std::size_t patientCount() const { return patients_.size(); }

    void addAppointment(const Appointment& appointment);
    Appointment removeAppointment(const std::string& id);
    Appointment* findAppointment(const std::string& id);
    const Appointment* findAppointment(const std::string& id) const;
    const std::vector<Appointment>& getAppointments() const { return appointments_; }
    std::vector<Appointment> appointmentsForPatient(const std::string& patientId) const;

    void replaceAll(std::vector<std::unique_ptr<PatientRecord>> patients, std::vector<Appointment> appointments);
    void clear();

private:
    int indexOfPatient(const std::string& id) const;
    int indexOfAppointment(const std::string& id) const;

    std::vector<std::unique_ptr<PatientRecord>> patients_;
    std::vector<Appointment> appointments_;
};

} // namespace scapms::model
