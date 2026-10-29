#pragma once

#include <string>
#include <vector>

namespace scapms::utils {

// Hands out zero-padded IDs (P0001, A0001). Zero-padding keeps text order
// the same as numeric order.
class IDGenerator {
public:
    static std::string nextPatientId();
    static std::string peekNextPatientId();
    static std::string nextAppointmentId();
    static std::string peekNextAppointmentId();

    // Set each counter to the highest number among existing IDs so new IDs
    // never clash with loaded ones.
    static void syncPatients(const std::vector<std::string>& existingIds);
    static void syncAppointments(const std::vector<std::string>& existingIds);
    static void reset();

private:
    static std::string format(char prefix, int number);
    static int highestNumber(const std::vector<std::string>& ids);

    static int patientCounter_;
    static int appointmentCounter_;
};

} // namespace scapms::utils
