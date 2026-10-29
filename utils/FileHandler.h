#pragma once

#include <memory>
#include <string>
#include <vector>

#include "model/Appointment.h"
#include "model/PatientRecord.h"

namespace scapms::utils {

// CSV persistence. Loading returns fresh containers and throws
// FileException on the first bad line, so callers can keep their current
// data when a file is broken.
class FileHandler {
public:
    static void savePatients(const std::string& path, const std::vector<model::PatientRecord*>& patients);
    static std::vector<std::unique_ptr<model::PatientRecord>> loadPatients(const std::string& path);

    static void saveAppointments(const std::string& path, const std::vector<model::Appointment>& appointments);
    static std::vector<model::Appointment> loadAppointments(const std::string& path);
};

} // namespace scapms::utils
