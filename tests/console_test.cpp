// Console checks for the model, utils and controller layers (no Qt).
// Run from the project root: ./build/scapms_tests

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "model/Appointment.h"
#include "model/ClinicDatabase.h"
#include "model/EmergencyPatient.h"
#include "model/FollowUpPatient.h"
#include "model/GeneralPatient.h"
#include "utils/Exceptions.h"
#include "utils/FileHandler.h"
#include "utils/IDGenerator.h"
#include "utils/TextUtils.h"

using namespace scapms;
using model::Appointment;
using model::ClinicDatabase;
using model::EmergencyPatient;
using model::FollowUpPatient;
using model::GeneralPatient;
using model::PatientRecord;

namespace {

int passed = 0;
int failed = 0;

void check(bool condition, const std::string& label)
{
    if (condition) {
        ++passed;
        std::cout << "  [PASS] " << label << "\n";
    } else {
        ++failed;
        std::cout << "  [FAIL] " << label << "\n";
    }
}

// Runs `fn` and checks that it throws ExceptionType.
template <typename ExceptionType, typename Fn>
void expectThrow(Fn fn, const std::string& label)
{
    try {
        fn();
        check(false, label + " (nothing thrown)");
    } catch (const ExceptionType& e) {
        check(true, label + " -> \"" + e.what() + "\"");
    } catch (const std::exception& e) {
        check(false, label + " (wrong exception: " + e.what() + ")");
    }
}

void section(const std::string& title)
{
    std::cout << "\n== " << title << " ==\n";
}

void testPolymorphism()
{
    section("Runtime polymorphism through PatientRecord*");
    ClinicDatabase db;
    db.addPatient(std::make_unique<GeneralPatient>("P0001", "Ada Okafor", 34, "08031234567", "Persistent cough"));
    db.addPatient(std::make_unique<EmergencyPatient>("P0002", "Musa Bello", 52, "08029876543", 4, "Chest pain"));
    db.addPatient(std::make_unique<FollowUpPatient>("P0003", "Grace Eze", 27, "07011223344", "2026-10-01",
                                                    "Dr. Adeyemi"));

    std::vector<std::string> categories;
    for (PatientRecord* p : db.getPatients()) {
        std::cout << "  " << p->getId() << " " << p->getName() << " | " << p->getCategory() << " | "
                  << p->getPriority() << " | " << p->describe() << "\n";
        categories.push_back(p->getCategory());
    }
    check(categories.size() == 3 && categories[0] == "General" && categories[1] == "Emergency" &&
              categories[2] == "Follow-Up",
          "each subclass reports its own category");

    std::unique_ptr<PatientRecord> copy = db.findPatient("P0002")->clone();
    copy->setName("Changed Name");
    check(db.findPatient("P0002")->getName() == "Musa Bello", "clone() is a deep copy");
}

void testValidation()
{
    section("Validation errors");
    expectThrow<utils::ValidationException>(
        [] { GeneralPatient("P0010", "Old Person", 150, "08030000000", "Check-up"); }, "age 150");
    expectThrow<utils::ValidationException>(
        [] { GeneralPatient("P0010", "   ", 30, "08030000000", "Check-up"); }, "empty name");
    expectThrow<utils::ValidationException>(
        [] { GeneralPatient("P0010", "Okafor, Ada", 30, "08030000000", "Check-up"); }, "name with comma");
    expectThrow<utils::ValidationException>(
        [] { GeneralPatient("X12", "Ada", 30, "08030000000", "Check-up"); }, "badly shaped ID");
    expectThrow<utils::ValidationException>(
        [] { GeneralPatient("P0010", "Ada", 30, "phone", "Check-up"); }, "non-numeric contact");
    expectThrow<utils::ValidationException>(
        [] { EmergencyPatient("P0010", "Ada", 30, "08030000000", 9, ""); }, "severity 9");
    expectThrow<utils::ValidationException>(
        [] { Appointment("A0001", "P0001", "2026-13-01", "09:00", "Check-up"); }, "month 13");
    expectThrow<utils::ValidationException>(
        [] { Appointment("A0001", "P0001", "2026-11-01", "25:00", "Check-up"); }, "hour 25");
    expectThrow<utils::ValidationException>(
        [] { Appointment("A0001", "P0001", "2026-11-01", "09:00", "  "); }, "empty reason");

    ClinicDatabase db;
    db.addPatient(std::make_unique<GeneralPatient>("P0001", "Ada Okafor", 34, "08031234567", "Cough"));
    expectThrow<utils::DuplicateIdException>(
        [&db] { db.addPatient(std::make_unique<GeneralPatient>("P0001", "Someone Else", 40, "08031234567", "X")); },
        "duplicate ID");
}

void testSchedulingRules()
{
    section("Subclass scheduling rules");
    FollowUpPatient follow("P0003", "Grace Eze", 27, "07011223344", "2026-10-01", "Dr. Adeyemi");
    Appointment early("A0001", "P0003", "2026-09-15", "10:00", "Review");
    expectThrow<utils::ValidationException>([&] { follow.scheduleAppointment(early); },
                                            "follow-up before previous visit");
    Appointment later("A0002", "P0003", "2026-12-15", "10:00", "Review");
    follow.scheduleAppointment(later);
    check(follow.hasAppointmentId("A0002"), "follow-up after previous visit is linked");

    EmergencyPatient urgent("P0002", "Musa Bello", 52, "08029876543", 5, "Chest pain");
    Appointment today("A0003", "P0002", utils::today(), "23:59", "ECG");
    urgent.scheduleAppointment(today);
    check(today.getReason() == "URGENT: ECG", "severity 5 marks reason URGENT");

    GeneralPatient general("P0001", "Ada Okafor", 34, "08031234567", "Cough");
    Appointment sameDay("A0004", "P0001", utils::today(), "23:59", "Check-up");
    expectThrow<utils::ValidationException>([&] { general.scheduleAppointment(sameDay); },
                                            "general patient same-day booking");

    expectThrow<utils::NotFoundException>([&] { general.cancelAppointment("A0999"); },
                                          "cancel an unlinked appointment");
}

void testDatabase()
{
    section("ClinicDatabase");
    ClinicDatabase db;
    db.addPatient(std::make_unique<GeneralPatient>("P0001", "Ada Okafor", 34, "08031234567", "Cough"));
    db.addAppointment(Appointment("A0001", "P0001", "2026-11-03", "09:30", "Check-up"));
    std::unique_ptr<PatientRecord> removed = db.removePatient("P0001");
    check(removed && db.patientCount() == 0, "removePatient hands back ownership");
    db.restorePatient(std::move(removed));
    check(db.findPatient("P0001") != nullptr, "restorePatient puts it back");
    check(db.appointmentsForPatient("P0001").size() == 1, "appointmentsForPatient");
    expectThrow<utils::NotFoundException>([&db] { db.removeAppointment("A0404"); }, "remove missing appointment");
}

std::string tempPath(const std::string& name)
{
    return (std::filesystem::temp_directory_path() / ("scapms_" + name)).string();
}

void writeFile(const std::string& path, const std::string& content)
{
    std::ofstream out(path);
    out << content;
}

void testIdGenerator()
{
    section("IDGenerator");
    utils::IDGenerator::reset();
    check(utils::IDGenerator::nextPatientId() == "P0001", "first patient ID is P0001");
    check(utils::IDGenerator::nextAppointmentId() == "A0001", "first appointment ID is A0001");
    utils::IDGenerator::syncPatients({"P0003", "P0012", "P0007"});
    check(utils::IDGenerator::peekNextPatientId() == "P0013", "syncPatients continues after the highest ID");
    check(utils::IDGenerator::nextPatientId() == "P0013", "peek does not consume an ID");
    utils::IDGenerator::reset();
}

void testFileRoundTrip()
{
    section("CSV save and load");
    ClinicDatabase db;
    db.addPatient(std::make_unique<GeneralPatient>("P0001", "Ada Okafor", 34, "08031234567", "Persistent cough"));
    db.addPatient(std::make_unique<EmergencyPatient>("P0002", "Musa Bello", 52, "08029876543", 4, "Chest pain"));
    db.addPatient(std::make_unique<FollowUpPatient>("P0003", "Grace Eze", 27, "07011223344", "2026-10-01",
                                                    "Dr. Adeyemi"));
    db.addAppointment(Appointment("A0001", "P0001", "2026-11-03", "09:30", "General check-up"));
    db.addAppointment(Appointment("A0002", "P0002", "2026-10-28", "14:00", "ECG review",
                                  Appointment::Status::Completed));

    const std::string patientsPath = tempPath("patients.csv");
    const std::string appointmentsPath = tempPath("appointments.csv");
    utils::FileHandler::savePatients(patientsPath, db.getPatients());
    utils::FileHandler::saveAppointments(appointmentsPath, db.getAppointments());

    auto patients = utils::FileHandler::loadPatients(patientsPath);
    auto appointments = utils::FileHandler::loadAppointments(appointmentsPath);
    check(patients.size() == 3 && appointments.size() == 2, "same number of records after loading");
    bool same = patients.size() == 3;
    for (std::size_t i = 0; same && i < patients.size(); ++i) {
        const PatientRecord* original = db.getPatients()[i];
        same = patients[i]->getId() == original->getId() && patients[i]->getName() == original->getName() &&
               patients[i]->getCategory() == original->getCategory() &&
               patients[i]->describe() == original->describe();
    }
    check(same, "patients match field by field, including subclass fields");
    check(appointments.size() == 2 && appointments[1].getStatus() == Appointment::Status::Completed,
          "appointment status survives the round trip");

    expectThrow<utils::FileException>([] { utils::FileHandler::loadPatients(tempPath("missing.csv")); },
                                      "missing file");

    const std::string broken = tempPath("broken.csv");
    writeFile(broken, "category,id,name,age,contact,extra1,extra2\r\n"
                      "General,P0001,Ada Okafor,34,08031234567,Cough,\r\n"
                      "General,P0002,Bad Age,abc,08031234567,Cough,\r\n");
    expectThrow<utils::FileException>([&] { utils::FileHandler::loadPatients(broken); }, "age 'abc' on line 3");

    writeFile(broken, "category,id,name,age,contact,extra1,extra2\nAlien,P0001,Ada,34,08031234567,x,\n");
    expectThrow<utils::FileException>([&] { utils::FileHandler::loadPatients(broken); }, "unknown category");

    writeFile(broken, "category,id,name,age,contact,extra1,extra2\nGeneral,P0001,Ada,34\n");
    expectThrow<utils::FileException>([&] { utils::FileHandler::loadPatients(broken); }, "too few columns");

    std::filesystem::remove(patientsPath);
    std::filesystem::remove(appointmentsPath);
    std::filesystem::remove(broken);
}

} // namespace

int main()
{
    testPolymorphism();
    testValidation();
    testSchedulingRules();
    testDatabase();
    testIdGenerator();
    testFileRoundTrip();

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
