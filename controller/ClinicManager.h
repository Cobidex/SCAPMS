#pragma once

#include <ctime>
#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <stack>
#include <string>
#include <vector>

#include "controller/AppointmentController.h"
#include "model/ClinicDatabase.h"

namespace scapms::controller {

enum class SortKey { None, Name, Id, Age, AppointmentDate };
enum class SortAlgorithm { Insertion, Merge };
enum class SearchField { Id, Name, Category };
enum class SearchMode { Auto, Linear, Binary, Recursive };

std::string sortKeyName(SortKey key);
std::string sortAlgorithmName(SortAlgorithm algorithm);

struct QueueEntry {
    std::string patientId;
    std::time_t joinedAt = 0;
};

enum class UndoType { DeletePatient, RemoveAppointment, EditPatient };

struct UndoAction {
    UndoType type = UndoType::DeletePatient;
    std::string description;
    std::unique_ptr<model::PatientRecord> patient;    // removed patient, or pre-edit clone
    std::vector<model::Appointment> appointments;     // appointments removed with a patient
    std::optional<model::Appointment> appointment;    // removed appointment
};

struct SearchResult {
    std::vector<model::PatientRecord*> patients;
    std::string algorithm;
};

struct SortResult {
    std::string algorithm;
    std::string key;
    std::size_t count = 0;
    long comparisons = 0;
};

struct AttendanceRow {
    const model::PatientRecord* patient = nullptr;
    int completed = 0;
};

struct PendingRow {
    const model::PatientRecord* patient = nullptr;
    int pending = 0;
    std::string next; // "YYYY-MM-DD HH:MM"
};

struct CategoryRow {
    std::string category;
    int count = 0;
    double percent = 0.0;
};

struct DailySummary {
    std::string date;
    int total = 0;
    int completed = 0;
    int pending = 0;
    int cancelled = 0;
};

// Coordinates the database with the waiting queue, undo stack, recently
// accessed cache, search/sort engines and reports. Plain C++, no Qt.
class ClinicManager {
public:
    static constexpr int RecentCapacity = 10;

    ClinicManager();

    // Patients
    void addPatient(std::unique_ptr<model::PatientRecord> patient);
    void editPatient(std::unique_ptr<model::PatientRecord> updated);
    void deletePatient(const std::string& id);
    model::PatientRecord* findPatient(const std::string& id) const;
    bool patientExists(const std::string& id) const;
    std::vector<model::PatientRecord*> getPatients() const;
    // Visits every patient through a base-class reference (runtime polymorphism).
    void processAll(const std::function<void(const model::PatientRecord&)>& visit) const;

    // Appointments
    std::string scheduleAppointment(const std::string& patientId, const std::string& date, const std::string& time,
                                    const std::string& reason);
    void cancelAppointment(const std::string& appointmentId);
    void completeAppointment(const std::string& appointmentId);
    void removeAppointment(const std::string& appointmentId);
    const std::vector<model::Appointment>& getAppointments() const;

    // Waiting queue (first in, first out)
    void enqueue(const std::string& patientId);
    QueueEntry callNext();
    std::vector<QueueEntry> queueSnapshot() const;
    bool isQueued(const std::string& patientId) const;
    std::size_t queueLength() const { return waitingQueue_.size(); }
    int longestWaitMinutes() const;
    int averageWaitMinutes() const;

    // Undo (last in, first out)
    bool canUndo() const { return !undoStack_.empty(); }
    std::string nextUndoDescription() const;
    std::string undo();
    const std::vector<std::string>& getHistory() const { return history_; }

    // Recently accessed cache
    void touch(model::PatientRecord* patient);
    std::vector<model::PatientRecord*> getRecent() const;

    // Search and sort
    SortResult sort(SortAlgorithm algorithm, SortKey key);
    SearchResult search(SearchField field, const std::string& query, SearchMode mode);
    const std::vector<model::PatientRecord*>& getSortedView() const { return sortedView_; }
    SortKey getSortedBy() const { return sortedBy_; }

    // Reports
    std::vector<AttendanceRow> mostFrequentlyAttended(std::size_t top = 5) const;
    std::vector<PendingRow> pendingAppointments() const;
    std::vector<CategoryRow> categoryDistribution() const;
    DailySummary dailySummary(const std::string& date) const;

    // Files
    void save(const std::string& patientsPath, const std::string& appointmentsPath) const;
    void load(const std::string& patientsPath, const std::string& appointmentsPath);
    void clearAll();

private:
    void invalidateSort();
    void forgetRecent(const model::PatientRecord* patient);
    void replaceRecent(const model::PatientRecord* oldPatient, model::PatientRecord* newPatient);
    void removeFromQueue(const std::string& patientId);
    void syncPatientIds() const;
    void record(const std::string& entry);
    std::string nextPendingSlot(const std::string& patientId) const;

    model::ClinicDatabase db_;
    AppointmentController appointmentController_;
    std::queue<QueueEntry> waitingQueue_;
    std::stack<UndoAction> undoStack_;
    model::PatientRecord* recentPatients_[RecentCapacity] = {};
    int recentCount_ = 0;
    std::vector<model::PatientRecord*> sortedView_;
    SortKey sortedBy_ = SortKey::None;
    std::vector<std::string> history_;
};

} // namespace scapms::controller
