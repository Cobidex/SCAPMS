#include "model/ClinicDatabase.h"

#include "utils/Exceptions.h"

namespace scapms::model {

using utils::DuplicateIdException;
using utils::NotFoundException;
using utils::ValidationException;

int ClinicDatabase::indexOfPatient(const std::string& id) const
{
    for (std::size_t i = 0; i < patients_.size(); ++i) {
        if (patients_[i]->getId() == id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int ClinicDatabase::indexOfAppointment(const std::string& id) const
{
    for (std::size_t i = 0; i < appointments_.size(); ++i) {
        if (appointments_[i].getId() == id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void ClinicDatabase::addPatient(std::unique_ptr<PatientRecord> patient)
{
    if (!patient) {
        throw ValidationException("No patient data");
    }
    if (indexOfPatient(patient->getId()) >= 0) {
        throw DuplicateIdException("Patient ID already exists");
    }
    patients_.push_back(std::move(patient));
}

std::unique_ptr<PatientRecord> ClinicDatabase::removePatient(const std::string& id)
{
    const int index = indexOfPatient(id);
    if (index < 0) {
        throw NotFoundException("Patient " + id + " not found");
    }
    std::unique_ptr<PatientRecord> removed = std::move(patients_[static_cast<std::size_t>(index)]);
    patients_.erase(patients_.begin() + index);
    return removed;
}

void ClinicDatabase::restorePatient(std::unique_ptr<PatientRecord> patient)
{
    addPatient(std::move(patient));
}

std::unique_ptr<PatientRecord> ClinicDatabase::replacePatient(std::unique_ptr<PatientRecord> patient)
{
    if (!patient) {
        throw ValidationException("No patient data");
    }
    const int index = indexOfPatient(patient->getId());
    if (index < 0) {
        throw NotFoundException("Patient " + patient->getId() + " not found");
    }
    std::unique_ptr<PatientRecord> old = std::move(patients_[static_cast<std::size_t>(index)]);
    patients_[static_cast<std::size_t>(index)] = std::move(patient);
    return old;
}

PatientRecord* ClinicDatabase::findPatient(const std::string& id) const
{
    const int index = indexOfPatient(id);
    return index < 0 ? nullptr : patients_[static_cast<std::size_t>(index)].get();
}

std::vector<PatientRecord*> ClinicDatabase::getPatients() const
{
    std::vector<PatientRecord*> view;
    view.reserve(patients_.size());
    for (const auto& p : patients_) {
        view.push_back(p.get());
    }
    return view;
}

void ClinicDatabase::addAppointment(const Appointment& appointment)
{
    if (indexOfAppointment(appointment.getId()) >= 0) {
        throw DuplicateIdException("Appointment ID already exists");
    }
    appointments_.push_back(appointment);
}

Appointment ClinicDatabase::removeAppointment(const std::string& id)
{
    const int index = indexOfAppointment(id);
    if (index < 0) {
        throw NotFoundException("Appointment " + id + " not found");
    }
    Appointment removed = appointments_[static_cast<std::size_t>(index)];
    appointments_.erase(appointments_.begin() + index);
    return removed;
}

Appointment* ClinicDatabase::findAppointment(const std::string& id)
{
    const int index = indexOfAppointment(id);
    return index < 0 ? nullptr : &appointments_[static_cast<std::size_t>(index)];
}

const Appointment* ClinicDatabase::findAppointment(const std::string& id) const
{
    const int index = indexOfAppointment(id);
    return index < 0 ? nullptr : &appointments_[static_cast<std::size_t>(index)];
}

std::vector<Appointment> ClinicDatabase::appointmentsForPatient(const std::string& patientId) const
{
    std::vector<Appointment> result;
    for (const Appointment& a : appointments_) {
        if (a.getPatientId() == patientId) {
            result.push_back(a);
        }
    }
    return result;
}

void ClinicDatabase::replaceAll(std::vector<std::unique_ptr<PatientRecord>> patients,
                                std::vector<Appointment> appointments)
{
    patients_ = std::move(patients);
    appointments_ = std::move(appointments);
}

void ClinicDatabase::clear()
{
    patients_.clear();
    appointments_.clear();
}

} // namespace scapms::model
