#pragma once

#include <memory>
#include <string>
#include <vector>

namespace scapms::model {

// Abstract base for every kind of patient. Cannot be instantiated: each
// subclass decides its own category, priority, description and CSV fields.
class PatientRecord {
public:
    PatientRecord(const std::string& id, const std::string& name, int age, const std::string& contact);
    virtual ~PatientRecord() = default;

    const std::string& getId() const { return id_; }
    const std::string& getName() const { return name_; }
    int getAge() const { return age_; }
    const std::string& getContact() const { return contact_; }

    void setName(const std::string& name);
    void setAge(int age);
    void setContact(const std::string& contact);

    const std::vector<std::string>& getAppointmentIds() const { return appointmentIds_; }
    bool hasAppointmentId(const std::string& appointmentId) const;
    void addAppointmentId(const std::string& appointmentId);
    void removeAppointmentId(const std::string& appointmentId);

    virtual std::string getCategory() const = 0;
    virtual std::string getPriority() const = 0;
    virtual std::string describe() const = 0;
    virtual std::unique_ptr<PatientRecord> clone() const = 0;
    virtual std::vector<std::string> toCsvFields() const = 0;

    // Throws ValidationException unless the ID looks like P0001.
    static void validateId(const std::string& id);

private:
    std::string id_;
    std::string name_;
    int age_ = 0;
    std::string contact_;
    std::vector<std::string> appointmentIds_;
};

} // namespace scapms::model
