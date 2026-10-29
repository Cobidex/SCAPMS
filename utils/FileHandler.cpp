#include "utils/FileHandler.h"

#include <fstream>

#include "model/EmergencyPatient.h"
#include "model/FollowUpPatient.h"
#include "model/GeneralPatient.h"
#include "utils/Exceptions.h"
#include "utils/TextUtils.h"

namespace scapms::utils {

using model::Appointment;
using model::PatientRecord;

namespace {

const char* const kPatientHeader = "category,id,name,age,contact,extra1,extra2";
const char* const kAppointmentHeader = "id,patientId,date,time,reason,status";

std::string lineError(int lineNumber, const std::string& message)
{
    return "Line " + std::to_string(lineNumber) + ": " + message;
}

// Reads every non-blank line, stripping the trailing '\r' left by Windows
// editors. The first line must be the expected header.
std::vector<std::pair<int, std::string>> readDataLines(const std::string& path, const std::string& header)
{
    std::ifstream in(path);
    if (!in) {
        throw FileException("Could not open file: " + path);
    }
    std::vector<std::pair<int, std::string>> lines;
    std::string line;
    int lineNumber = 0;
    bool sawHeader = false;
    while (std::getline(in, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (trim(line).empty()) {
            continue;
        }
        if (!sawHeader) {
            if (toLower(trim(line)) != toLower(header)) {
                throw FileException(lineError(lineNumber, "expected header '" + header + "'"));
            }
            sawHeader = true;
            continue;
        }
        lines.emplace_back(lineNumber, line);
    }
    return lines;
}

std::vector<std::string> splitFields(const std::string& line, std::size_t expected, int lineNumber)
{
    std::vector<std::string> fields = split(line, ',');
    if (fields.size() != expected) {
        throw FileException(lineError(lineNumber, "expected " + std::to_string(expected) + " columns but found " +
                                                      std::to_string(fields.size())));
    }
    for (std::string& f : fields) {
        f = trim(f);
    }
    return fields;
}

int parseNumber(const std::string& text, const std::string& what, int lineNumber)
{
    if (!isDigits(text) || text.size() > 3) {
        throw FileException(lineError(lineNumber, "invalid " + what + " '" + text + "'"));
    }
    return std::stoi(text);
}

// Factory: the category column decides which subclass to build.
std::unique_ptr<PatientRecord> makePatient(const std::vector<std::string>& f, int lineNumber)
{
    const std::string& category = f[0];
    const int age = parseNumber(f[3], "age", lineNumber);
    if (category == "General") {
        return std::make_unique<model::GeneralPatient>(f[1], f[2], age, f[4], f[5]);
    }
    if (category == "Emergency") {
        const int severity = parseNumber(f[5], "severity", lineNumber);
        return std::make_unique<model::EmergencyPatient>(f[1], f[2], age, f[4], severity, f[6]);
    }
    if (category == "FollowUp" || category == "Follow-Up") {
        return std::make_unique<model::FollowUpPatient>(f[1], f[2], age, f[4], f[5], f[6]);
    }
    throw FileException(lineError(lineNumber, "unknown category '" + category + "'"));
}

std::string csvCategory(const PatientRecord& p)
{
    const std::string category = p.getCategory();
    return category == "Follow-Up" ? "FollowUp" : category;
}

} // namespace

void FileHandler::savePatients(const std::string& path, const std::vector<PatientRecord*>& patients)
{
    std::ofstream out(path);
    if (!out) {
        throw FileException("Could not write file: " + path);
    }
    out << kPatientHeader << "\n";
    for (const PatientRecord* p : patients) {
        out << csvCategory(*p) << "," << p->getId() << "," << p->getName() << "," << p->getAge() << ","
            << p->getContact();
        for (const std::string& field : p->toCsvFields()) {
            out << "," << field;
        }
        out << "\n";
    }
    if (!out) {
        throw FileException("Could not write file: " + path);
    }
}

std::vector<std::unique_ptr<PatientRecord>> FileHandler::loadPatients(const std::string& path)
{
    std::vector<std::unique_ptr<PatientRecord>> patients;
    for (const auto& [lineNumber, line] : readDataLines(path, kPatientHeader)) {
        const std::vector<std::string> fields = splitFields(line, 7, lineNumber);
        std::unique_ptr<PatientRecord> patient;
        try {
            patient = makePatient(fields, lineNumber);
        } catch (const ValidationException& e) {
            throw FileException(lineError(lineNumber, e.what()));
        }
        for (const auto& existing : patients) {
            if (existing->getId() == patient->getId()) {
                throw FileException(lineError(lineNumber, "duplicate patient ID " + patient->getId()));
            }
        }
        patients.push_back(std::move(patient));
    }
    return patients;
}

void FileHandler::saveAppointments(const std::string& path, const std::vector<Appointment>& appointments)
{
    std::ofstream out(path);
    if (!out) {
        throw FileException("Could not write file: " + path);
    }
    out << kAppointmentHeader << "\n";
    for (const Appointment& a : appointments) {
        out << a.getId() << "," << a.getPatientId() << "," << a.getDate() << "," << a.getTime() << ","
            << a.getReason() << "," << Appointment::statusToString(a.getStatus()) << "\n";
    }
    if (!out) {
        throw FileException("Could not write file: " + path);
    }
}

std::vector<Appointment> FileHandler::loadAppointments(const std::string& path)
{
    std::vector<Appointment> appointments;
    for (const auto& [lineNumber, line] : readDataLines(path, kAppointmentHeader)) {
        const std::vector<std::string> f = splitFields(line, 6, lineNumber);
        try {
            Appointment a(f[0], f[1], f[2], f[3], f[4], Appointment::statusFromString(f[5]));
            for (const Appointment& existing : appointments) {
                if (existing.getId() == a.getId()) {
                    throw FileException(lineError(lineNumber, "duplicate appointment ID " + a.getId()));
                }
            }
            appointments.push_back(a);
        } catch (const ValidationException& e) {
            throw FileException(lineError(lineNumber, e.what()));
        }
    }
    return appointments;
}

} // namespace scapms::utils
