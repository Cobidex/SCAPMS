// Console checks for the model, utils and controller layers (no Qt).
// Run from the project root: ./build/scapms_tests

#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "controller/ClinicManager.h"
#include "controller/SearchEngine.h"
#include "controller/SortEngine.h"
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
using controller::ClinicManager;
using controller::Compare;
using controller::SearchEngine;
using controller::SortEngine;

namespace {

int passed = 0;
int failed = 0;
int notImplemented = 0;

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

// ------------------------------------------------------ algorithm tests

// Owns a set of patients and hands out a fresh raw-pointer view.
struct Fixture {
    std::vector<std::unique_ptr<PatientRecord>> owned;

    void add(const std::string& id, const std::string& name, int age)
    {
        owned.push_back(std::make_unique<GeneralPatient>(id, name, age, "08030000000", "Check-up"));
    }

    std::vector<PatientRecord*> view() const
    {
        std::vector<PatientRecord*> v;
        for (const auto& p : owned) {
            v.push_back(p.get());
        }
        return v;
    }
};

using SortFn = void (*)(std::vector<PatientRecord*>&, const Compare&, long&);

std::string ids(const std::vector<PatientRecord*>& items)
{
    std::string out;
    for (const PatientRecord* p : items) {
        out += (out.empty() ? "" : " ") + p->getId();
    }
    return out;
}

// Sorts a copy of `input` and compares the ID order with `expected`.
void expectOrder(SortFn sortFn, const std::string& algorithm, const std::vector<PatientRecord*>& input,
                 const Compare& less, const std::string& expected, const std::string& label)
{
    std::vector<PatientRecord*> items = input;
    long comparisons = 0;
    sortFn(items, less, comparisons);
    const std::string actual = ids(items);
    check(actual == expected, algorithm + ": " + label + (actual == expected ? "" : " (got: " + actual + ")"));
}

void testSortAlgorithm(SortFn sortFn, const std::string& algorithm)
{
    const Compare byName = [](const PatientRecord* a, const PatientRecord* b) {
        return utils::toLower(a->getName()) < utils::toLower(b->getName());
    };
    const Compare byId = [](const PatientRecord* a, const PatientRecord* b) { return a->getId() < b->getId(); };
    const Compare byAge = [](const PatientRecord* a, const PatientRecord* b) { return a->getAge() < b->getAge(); };

    Fixture empty;
    expectOrder(sortFn, algorithm, empty.view(), byName, "", "empty list");

    Fixture one;
    one.add("P0001", "Ada", 30);
    expectOrder(sortFn, algorithm, one.view(), byName, "P0001", "one item");

    Fixture sorted;
    sorted.add("P0001", "Ada", 10);
    sorted.add("P0002", "Bola", 20);
    sorted.add("P0003", "Chidi", 30);
    sorted.add("P0004", "Dayo", 40);
    expectOrder(sortFn, algorithm, sorted.view(), byName, "P0001 P0002 P0003 P0004", "already sorted (name)");
    expectOrder(sortFn, algorithm, sorted.view(), byAge, "P0001 P0002 P0003 P0004", "already sorted (age)");

    Fixture reversed;
    reversed.add("P0004", "dayo", 40);
    reversed.add("P0003", "Chidi", 30);
    reversed.add("P0002", "bola", 20);
    reversed.add("P0001", "Ada", 10);
    expectOrder(sortFn, algorithm, reversed.view(), byName, "P0001 P0002 P0003 P0004",
                "reverse sorted, mixed case (name)");
    expectOrder(sortFn, algorithm, reversed.view(), byId, "P0001 P0002 P0003 P0004", "reverse sorted (ID)");
    expectOrder(sortFn, algorithm, reversed.view(), byAge, "P0001 P0002 P0003 P0004", "reverse sorted (age)");

    Fixture equal;
    equal.add("P0003", "Same", 50);
    equal.add("P0001", "Same", 50);
    equal.add("P0002", "Same", 50);
    expectOrder(sortFn, algorithm, equal.view(), byAge, "P0003 P0001 P0002", "all equal keeps order (stable)");

    Fixture duplicates;
    duplicates.add("P0005", "Okafor", 40);
    duplicates.add("P0001", "Bello", 25);
    duplicates.add("P0004", "Okafor", 25);
    duplicates.add("P0002", "Adeyemi", 60);
    duplicates.add("P0003", "Bello", 40);
    expectOrder(sortFn, algorithm, duplicates.view(), byName, "P0002 P0001 P0003 P0005 P0004",
                "duplicate names stay in input order (stable)");
    expectOrder(sortFn, algorithm, duplicates.view(), byAge, "P0001 P0004 P0005 P0003 P0002",
                "duplicate ages stay in input order (stable)");

    std::vector<PatientRecord*> items = reversed.view();
    long comparisons = 0;
    sortFn(items, byAge, comparisons);
    check(comparisons > 0, algorithm + ": counts comparisons (" + std::to_string(comparisons) + ")");
}

void testSorting()
{
    section("Sorting algorithms");
    testSortAlgorithm(&SortEngine::insertionSort, "Insertion sort");
    testSortAlgorithm(&SortEngine::mergeSort, "Merge sort");
}

void testSearching()
{
    section("Searching algorithms");
    Fixture f;
    f.add("P0001", "Ada Okafor", 34);
    f.add("P0002", "Musa Bello", 52);
    f.add("P0003", "Grace Eze", 27);
    f.add("P0004", "Chinedu Okafor", 61);
    f.add("P0005", "Adamu Sani", 19);
    const std::vector<PatientRecord*> all = f.view();

    check(SearchEngine::linearSearchById({}, "P0001") == nullptr, "linear: empty list");
    check(SearchEngine::linearSearchById(all, "P0001") == all[0], "linear: first item");
    check(SearchEngine::linearSearchById(all, "P0005") == all[4], "linear: last item");
    check(SearchEngine::linearSearchById(all, "P0099") == nullptr, "linear: missing ID");

    // Already in ID order.
    check(SearchEngine::binarySearchById({}, "P0001") == -1, "binary ID: empty list");
    check(SearchEngine::binarySearchById({all[2]}, "P0003") == 0, "binary ID: one item");
    check(SearchEngine::binarySearchById(all, "P0001") == 0, "binary ID: first item");
    check(SearchEngine::binarySearchById(all, "P0005") == 4, "binary ID: last item");
    check(SearchEngine::binarySearchById(all, "P0003") == 2, "binary ID: middle item");
    check(SearchEngine::binarySearchById(all, "P0000") == -1, "binary ID: missing (before first)");
    check(SearchEngine::binarySearchById(all, "P0099") == -1, "binary ID: missing (after last)");

    Fixture names;
    names.add("P0010", "Ada Okafor", 34);
    names.add("P0011", "Bola Ade", 40);
    names.add("P0012", "grace eze", 27);
    names.add("P0013", "Grace Eze", 50);
    names.add("P0014", "Musa Bello", 52);
    const std::vector<PatientRecord*> byName = names.view(); // sorted by lower-case name
    check(SearchEngine::binarySearchByName({}, "Ada Okafor").empty(), "binary name: empty list");
    check(ids(SearchEngine::binarySearchByName(byName, "ada okafor")) == "P0010", "binary name: first, any case");
    check(ids(SearchEngine::binarySearchByName(byName, "Musa Bello")) == "P0014", "binary name: last");
    check(ids(SearchEngine::binarySearchByName(byName, "Grace Eze")) == "P0012 P0013", "binary name: duplicates");
    check(SearchEngine::binarySearchByName(byName, "Nobody").empty(), "binary name: missing");

    std::vector<PatientRecord*> results;
    SearchEngine::recursiveNameSearch({}, "ad", 0, results);
    check(results.empty(), "recursive: empty list");
    results.clear();
    SearchEngine::recursiveNameSearch(all, "ad", 0, results);
    check(ids(results) == "P0001 P0005", "recursive: partial match 'ad'");
    results.clear();
    SearchEngine::recursiveNameSearch(all, "OKAFOR", 0, results);
    check(ids(results) == "P0001 P0004", "recursive: ignores case");
    results.clear();
    SearchEngine::recursiveNameSearch(all, "zzz", 0, results);
    check(results.empty(), "recursive: no match");

    std::vector<std::unique_ptr<PatientRecord>> mixed;
    mixed.push_back(std::make_unique<GeneralPatient>("P0001", "Ada", 30, "08030000000", "Cough"));
    mixed.push_back(std::make_unique<EmergencyPatient>("P0002", "Musa", 40, "08030000000", 3, ""));
    mixed.push_back(std::make_unique<EmergencyPatient>("P0003", "Grace", 50, "08030000000", 2, ""));
    std::vector<PatientRecord*> mixedView;
    for (const auto& p : mixed) {
        mixedView.push_back(p.get());
    }
    check(ids(SearchEngine::filterByCategory(mixedView, "emergency")) == "P0002 P0003", "category filter");
    check(SearchEngine::filterByCategory(mixedView, "Follow-Up").empty(), "category filter: no match");
}

// ----------------------------------------------------- controller tests

std::string futureDate(int daysAhead)
{
    std::time_t t = std::time(nullptr) + static_cast<std::time_t>(daysAhead) * 24 * 60 * 60;
    char buffer[16];
    std::strftime(buffer, sizeof buffer, "%Y-%m-%d", std::localtime(&t));
    return buffer;
}

void addTenPatients(ClinicManager& m)
{
    m.addPatient(std::make_unique<GeneralPatient>("P0001", "Ada Okafor", 34, "08031234567", "Persistent cough"));
    m.addPatient(std::make_unique<EmergencyPatient>("P0002", "Musa Bello", 52, "08029876543", 4, "Chest pain"));
    m.addPatient(std::make_unique<FollowUpPatient>("P0003", "Grace Eze", 27, "07011223344", "2026-10-01",
                                                   "Dr. Adeyemi"));
    m.addPatient(std::make_unique<GeneralPatient>("P0004", "Chinedu Okafor", 61, "08035550101", "Back pain"));
    m.addPatient(std::make_unique<GeneralPatient>("P0005", "Adamu Sani", 19, "08035550102", "Fever"));
    m.addPatient(std::make_unique<EmergencyPatient>("P0006", "Ngozi Umeh", 45, "08035550103", 2, "Sprained ankle"));
    m.addPatient(std::make_unique<FollowUpPatient>("P0007", "Tunde Bakare", 70, "08035550104", "2026-09-20",
                                                   "Dr. Musa"));
    m.addPatient(std::make_unique<GeneralPatient>("P0008", "Fatima Lawal", 8, "08035550105", "Vaccination"));
    m.addPatient(std::make_unique<GeneralPatient>("P0009", "Bisi Adeyemi", 34, "08035550106", "Headache"));
    m.addPatient(std::make_unique<FollowUpPatient>("P0010", "Emeka Obi", 52, "08035550107", "2026-10-10",
                                                   "Dr. Adeyemi"));
}

void testManager()
{
    section("ClinicManager");
    ClinicManager m;
    addTenPatients(m);
    check(m.getPatients().size() == 10, "10 patients added");
    expectThrow<utils::DuplicateIdException>(
        [&m] { m.addPatient(std::make_unique<GeneralPatient>("P0001", "Copy", 30, "08030000000", "X")); },
        "addPatient rejects a duplicate ID");

    for (auto algorithm : {controller::SortAlgorithm::Insertion, controller::SortAlgorithm::Merge}) {
        for (auto key : {controller::SortKey::Name, controller::SortKey::Age}) {
            controller::SortResult r = m.sort(algorithm, key);
            std::cout << "  " << r.algorithm << " by " << r.key << ": " << r.count << " records, " << r.comparisons
                      << " comparisons -> " << ids(m.getSortedView()) << "\n";
        }
    }
    check(m.getSortedView().front()->getId() == "P0008", "sorted by age, youngest first");

    controller::SearchResult linear = m.search(controller::SearchField::Id, "P0007", controller::SearchMode::Auto);
    check(ids(linear.patients) == "P0007" && linear.algorithm.find("Linear") == 0,
          "Auto ID search on unsorted data uses linear (" + linear.algorithm + ")");
    m.sort(controller::SortAlgorithm::Merge, controller::SortKey::Id);
    controller::SearchResult binary = m.search(controller::SearchField::Id, "p0007", controller::SearchMode::Auto);
    check(ids(binary.patients) == "P0007" && binary.algorithm.find("Binary") == 0,
          "Auto ID search after sorting by ID uses binary (" + binary.algorithm + ")");
    controller::SearchResult partial = m.search(controller::SearchField::Name, "ad", controller::SearchMode::Auto);
    check(ids(partial.patients) == "P0001 P0005 P0009", "recursive search for 'ad' (" + partial.algorithm + ")");
    expectThrow<utils::ValidationException>(
        [&m] { m.search(controller::SearchField::Name, "Ada Okafor", controller::SearchMode::Binary); },
        "forced binary on data not sorted by name");
    expectThrow<utils::ValidationException>(
        [&m] { m.search(controller::SearchField::Id, "XYZ", controller::SearchMode::Auto); }, "search ID 'XYZ'");
    expectThrow<utils::ValidationException>(
        [&m] { m.search(controller::SearchField::Name, "   ", controller::SearchMode::Auto); }, "empty search");

    // Appointments
    const std::string a1 = m.scheduleAppointment("P0004", futureDate(3), "09:30", "Back review");
    const std::string a2 = m.scheduleAppointment("P0004", futureDate(5), "10:00", "Physio");
    expectThrow<utils::ValidationException>(
        [&m] { m.scheduleAppointment("P0004", futureDate(3), "09:30", "Again"); }, "same patient, same slot");
    expectThrow<utils::ValidationException>(
        [&m] { m.scheduleAppointment("P0004", "2020-01-01", "09:30", "Old"); }, "appointment in the past");
    expectThrow<utils::ValidationException>(
        [&m] { m.scheduleAppointment("P0003", "2026-09-01", "09:30", "Too early"); },
        "follow-up before previous visit");
    m.cancelAppointment(a2);
    expectThrow<utils::ValidationException>([&m, &a2] { m.cancelAppointment(a2); },
                                            "cancel an already cancelled appointment");

    // Delete + undo restores the patient with their appointments
    m.touch(m.findPatient("P0004"));
    const std::size_t cachedBefore = m.getRecent().size();
    m.deletePatient("P0004");
    bool stillCached = false;
    for (const PatientRecord* p : m.getRecent()) {
        stillCached = stillCached || p->getId() == "P0004";
    }
    check(!m.patientExists("P0004") && !stillCached && m.getRecent().size() == cachedBefore - 1,
          "delete removes patient and cache entry");
    check(m.nextUndoDescription() == "Delete patient P0004 (Chinedu Okafor)", "undo description");
    m.undo();
    PatientRecord* restored = m.findPatient("P0004");
    int restoredAppointments = 0;
    for (const Appointment& a : m.getAppointments()) {
        restoredAppointments += a.getPatientId() == "P0004" ? 1 : 0;
    }
    check(restored && restoredAppointments == 2 && restored->hasAppointmentId(a1),
          "undo delete restores patient and appointments");

    // Edit + undo restores the old values
    m.editPatient(std::make_unique<GeneralPatient>("P0001", "Ada Okafor-Nwosu", 35, "08031234567", "Cough"));
    check(m.findPatient("P0001")->getName() == "Ada Okafor-Nwosu", "edit changes the name");
    m.undo();
    check(m.findPatient("P0001")->getName() == "Ada Okafor" && m.findPatient("P0001")->getAge() == 34,
          "undo edit restores the old values");

    // Remove appointment + undo
    m.removeAppointment(a1);
    check(!m.findPatient("P0004")->hasAppointmentId(a1), "remove appointment unlinks it");
    m.undo();
    check(m.findPatient("P0004")->hasAppointmentId(a1), "undo remove appointment re-links it");
    expectThrow<utils::ValidationException>([&m] { m.undo(); }, "undo with an empty stack");

    // Queue
    m.enqueue("P0002");
    m.enqueue("P0005");
    m.enqueue("P0009");
    expectThrow<utils::ValidationException>([&m] { m.enqueue("P0005"); }, "patient already queued");
    const std::string first = m.callNext().patientId;
    const std::string second = m.callNext().patientId;
    const std::string third = m.callNext().patientId;
    check(first == "P0002" && second == "P0005" && third == "P0009", "queue is first in, first out");
    expectThrow<utils::ValidationException>([&m] { m.callNext(); }, "call next on an empty queue");

    // Recently accessed cache holds at most 10, most recent first
    for (PatientRecord* p : m.getPatients()) {
        m.touch(p);
    }
    m.touch(m.findPatient("P0003"));
    std::vector<PatientRecord*> recent = m.getRecent();
    check(recent.size() == 10 && recent.front()->getId() == "P0003", "cache keeps 10, newest first");

    // Reports
    m.completeAppointment(a1);
    const std::string a3 = m.scheduleAppointment("P0002", futureDate(1), "11:00", "ECG");
    const std::string a4 = m.scheduleAppointment("P0002", futureDate(2), "11:00", "Bloods");
    m.completeAppointment(a3);
    m.completeAppointment(a4);
    m.scheduleAppointment("P0007", futureDate(4), "08:15", "Blood pressure review");

    std::cout << "  Most frequently attended:\n";
    std::vector<controller::AttendanceRow> attended = m.mostFrequentlyAttended();
    for (const auto& row : attended) {
        std::cout << "    " << row.patient->getId() << " " << row.patient->getName() << ": " << row.completed << "\n";
    }
    check(attended.size() == 2 && attended[0].patient->getId() == "P0002", "attendance report, most visits first");

    std::cout << "  Pending appointments:\n";
    for (const auto& row : m.pendingAppointments()) {
        std::cout << "    " << row.patient->getId() << " " << row.pending << " pending, next " << row.next << "\n";
    }
    check(m.pendingAppointments().size() == 1, "pending report");

    std::cout << "  Category distribution:\n";
    int counted = 0;
    for (const auto& row : m.categoryDistribution()) {
        std::cout << "    " << row.category << ": " << row.count << "\n";
        counted += row.count;
    }
    check(counted == 10, "category report counts every patient");

    controller::DailySummary daily = m.dailySummary(futureDate(1));
    std::cout << "  Daily summary " << daily.date << ": " << daily.total << " total, " << daily.completed
              << " completed, " << daily.pending << " pending, " << daily.cancelled << " cancelled\n";
    check(daily.total == 1 && daily.completed == 1, "daily summary");
}

// Runs a test group; stubs that are not written yet throw std::logic_error.
void run(void (*test)(), const std::string& name)
{
    try {
        test();
    } catch (const std::logic_error& e) {
        ++notImplemented;
        std::cout << "  [TODO] " << name << " stopped: " << e.what() << "\n";
    } catch (const std::exception& e) {
        ++failed;
        std::cout << "  [FAIL] " << name << " threw: " << e.what() << "\n";
    }
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
    run(testSorting, "Sorting tests");
    run(testSearching, "Searching tests");
    run(testManager, "ClinicManager tests");

    std::cout << "\n" << passed << " passed, " << failed << " failed";
    if (notImplemented > 0) {
        std::cout << ", " << notImplemented << " group(s) waiting on unimplemented algorithms";
    }
    std::cout << "\n";
    return failed == 0 && notImplemented == 0 ? 0 : 1;
}
