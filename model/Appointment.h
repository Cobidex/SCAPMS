#pragma once

#include <string>

namespace scapms::model {

class Appointment {
public:
    enum class Status { Pending, Completed, Cancelled };

    Appointment(const std::string& id, const std::string& patientId, const std::string& date,
                const std::string& time, const std::string& reason, Status status = Status::Pending);

    const std::string& getId() const { return id_; }
    const std::string& getPatientId() const { return patientId_; }
    const std::string& getDate() const { return date_; }
    const std::string& getTime() const { return time_; }
    const std::string& getReason() const { return reason_; }
    Status getStatus() const { return status_; }

    void setReason(const std::string& reason);
    void setStatus(Status status) { status_ = status; }

    static std::string statusToString(Status status);
    static Status statusFromString(const std::string& text);

private:
    std::string id_;
    std::string patientId_;
    std::string date_;
    std::string time_;
    std::string reason_;
    Status status_ = Status::Pending;
};

} // namespace scapms::model
