#include "utils/IDGenerator.h"

#include <cstdio>

#include "utils/TextUtils.h"

namespace scapms::utils {

int IDGenerator::patientCounter_ = 0;
int IDGenerator::appointmentCounter_ = 0;

std::string IDGenerator::format(char prefix, int number)
{
    char buffer[16];
    std::snprintf(buffer, sizeof buffer, "%c%04d", prefix, number);
    return buffer;
}

int IDGenerator::highestNumber(const std::vector<std::string>& ids)
{
    int highest = 0;
    for (const std::string& id : ids) {
        if (id.size() > 1 && id.size() <= 9 && isDigits(id.substr(1))) {
            const int number = std::stoi(id.substr(1));
            if (number > highest) {
                highest = number;
            }
        }
    }
    return highest;
}

std::string IDGenerator::nextPatientId() { return format('P', ++patientCounter_); }

std::string IDGenerator::peekNextPatientId() { return format('P', patientCounter_ + 1); }

std::string IDGenerator::nextAppointmentId() { return format('A', ++appointmentCounter_); }

std::string IDGenerator::peekNextAppointmentId() { return format('A', appointmentCounter_ + 1); }

void IDGenerator::syncPatients(const std::vector<std::string>& existingIds)
{
    patientCounter_ = highestNumber(existingIds);
}

void IDGenerator::syncAppointments(const std::vector<std::string>& existingIds)
{
    appointmentCounter_ = highestNumber(existingIds);
}

void IDGenerator::reset()
{
    patientCounter_ = 0;
    appointmentCounter_ = 0;
}

} // namespace scapms::utils
