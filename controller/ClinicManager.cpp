#include "controller/ClinicManager.h"

#include <cctype>
#include <map>

#include "controller/SearchEngine.h"
#include "controller/SortEngine.h"
#include "utils/Exceptions.h"
#include "utils/FileHandler.h"
#include "utils/IDGenerator.h"
#include "utils/TextUtils.h"

namespace scapms::controller {

using model::Appointment;
using model::PatientRecord;
using utils::DuplicateIdException;
using utils::FileException;
using utils::NotFoundException;
using utils::ValidationException;

namespace {

// Patients with no pending appointment sort last.
const char* const kNoAppointment = "9999-99-99";

int minutesSince(std::time_t then)
{
    const double seconds = std::difftime(std::time(nullptr), then);
    return seconds <= 0 ? 0 : static_cast<int>(seconds / 60);
}

} // namespace

std::string sortKeyName(SortKey key)
{
    switch (key) {
    case SortKey::Name:
        return "Name";
    case SortKey::Id:
        return "ID";
    case SortKey::Age:
        return "Age";
    case SortKey::AppointmentDate:
        return "Appointment Date";
    case SortKey::None:
        break;
    }
    return "None";
}

std::string sortAlgorithmName(SortAlgorithm algorithm)
{
    return algorithm == SortAlgorithm::Insertion ? "Insertion Sort" : "Merge Sort";
}

ClinicManager::ClinicManager() : appointmentController_(db_) {}

// ---------------------------------------------------------------- patients

void ClinicManager::addPatient(std::unique_ptr<PatientRecord> patient)
{
    if (!patient) {
        throw ValidationException("No patient data");
    }
    if (SearchEngine::linearSearchById(db_.getPatients(), patient->getId()) != nullptr) {
        throw DuplicateIdException("Patient ID already exists");
    }
    const std::string summary = patient->getId() + " (" + patient->getName() + ")";
    db_.addPatient(std::move(patient));
    invalidateSort();
    syncPatientIds();
    record("Added patient " + summary);
}

void ClinicManager::editPatient(std::unique_ptr<PatientRecord> updated)
{
    if (!updated) {
        throw ValidationException("No patient data");
    }
    PatientRecord* current = db_.findPatient(updated->getId());
    if (!current) {
        throw NotFoundException("Patient " + updated->getId() + " not found");
    }
    if (current->getCategory() != updated->getCategory()) {
        throw ValidationException("Patient type cannot be changed");
    }
    for (const std::string& appointmentId : current->getAppointmentIds()) {
        updated->addAppointmentId(appointmentId);
    }

    UndoAction action;
    action.type = UndoType::EditPatient;
    action.description = "Edit patient " + current->getId() + " (" + current->getName() + ")";
    action.patient = current->clone();

    const std::string id = updated->getId();
    std::unique_ptr<PatientRecord> old = db_.replacePatient(std::move(updated));
    replaceRecent(old.get(), db_.findPatient(id));
    invalidateSort();
    record("Edited patient " + id);
    undoStack_.push(std::move(action));
}

void ClinicManager::deletePatient(const std::string& id)
{
    PatientRecord* patient = db_.findPatient(id);
    if (!patient) {
        throw NotFoundException("Patient " + id + " not found");
    }
    // Drop every raw pointer to the patient before ownership moves.
    forgetRecent(patient);
    invalidateSort();

    UndoAction action;
    action.type = UndoType::DeletePatient;
    action.description = "Delete patient " + id + " (" + patient->getName() + ")";
    action.appointments = db_.appointmentsForPatient(id);
    for (const Appointment& a : action.appointments) {
        db_.removeAppointment(a.getId());
    }
    action.patient = db_.removePatient(id);
    removeFromQueue(id);
    record("Deleted patient " + id + " (" + action.patient->getName() + ")");
    undoStack_.push(std::move(action));
}

PatientRecord* ClinicManager::findPatient(const std::string& id) const { return db_.findPatient(id); }

bool ClinicManager::patientExists(const std::string& id) const { return db_.findPatient(id) != nullptr; }

std::vector<PatientRecord*> ClinicManager::getPatients() const { return db_.getPatients(); }

void ClinicManager::processAll(const std::function<void(const PatientRecord&)>& visit) const
{
    for (const PatientRecord* p : db_.getPatients()) {
        visit(*p); // getCategory(), getPriority(), describe() dispatch to the subclass
    }
}

// ------------------------------------------------------------ appointments

std::string ClinicManager::scheduleAppointment(const std::string& patientId, const std::string& date,
                                               const std::string& time, const std::string& reason)
{
    const std::string id = appointmentController_.schedule(patientId, date, time, reason);
    invalidateSort();
    return id;
}

void ClinicManager::cancelAppointment(const std::string& appointmentId)
{
    appointmentController_.cancel(appointmentId);
    invalidateSort();
}

void ClinicManager::completeAppointment(const std::string& appointmentId)
{
    appointmentController_.complete(appointmentId);
    invalidateSort();
}

void ClinicManager::removeAppointment(const std::string& appointmentId)
{
    const Appointment* existing = db_.findAppointment(appointmentId);
    if (!existing) {
        throw NotFoundException("Appointment " + appointmentId + " not found");
    }
    UndoAction action;
    action.type = UndoType::RemoveAppointment;
    action.description = "Remove appointment " + appointmentId + " (" + existing->getPatientId() + ", " +
                         existing->getDate() + " " + existing->getTime() + ")";
    action.appointment = *existing;

    db_.removeAppointment(appointmentId);
    if (PatientRecord* patient = db_.findPatient(action.appointment->getPatientId())) {
        if (patient->hasAppointmentId(appointmentId)) {
            patient->removeAppointmentId(appointmentId);
        }
    }
    invalidateSort();
    record("Removed appointment " + appointmentId);
    undoStack_.push(std::move(action));
}

const std::vector<Appointment>& ClinicManager::getAppointments() const { return db_.getAppointments(); }

// ----------------------------------------------------------- waiting queue

void ClinicManager::enqueue(const std::string& patientId)
{
    if (!db_.findPatient(patientId)) {
        throw NotFoundException("Patient " + patientId + " not found");
    }
    if (isQueued(patientId)) {
        throw ValidationException("Patient " + patientId + " is already in the waiting queue");
    }
    waitingQueue_.push(QueueEntry{patientId, std::time(nullptr)});
}

QueueEntry ClinicManager::callNext()
{
    if (waitingQueue_.empty()) {
        throw ValidationException("No patients waiting");
    }
    QueueEntry next = waitingQueue_.front();
    waitingQueue_.pop();
    return next;
}

std::vector<QueueEntry> ClinicManager::queueSnapshot() const
{
    // std::queue cannot be iterated, so pop from a copy.
    std::queue<QueueEntry> copy = waitingQueue_;
    std::vector<QueueEntry> entries;
    while (!copy.empty()) {
        entries.push_back(copy.front());
        copy.pop();
    }
    return entries;
}

bool ClinicManager::isQueued(const std::string& patientId) const
{
    for (const QueueEntry& entry : queueSnapshot()) {
        if (entry.patientId == patientId) {
            return true;
        }
    }
    return false;
}

int ClinicManager::longestWaitMinutes() const
{
    // The front of a FIFO queue has waited longest.
    return waitingQueue_.empty() ? 0 : minutesSince(waitingQueue_.front().joinedAt);
}

int ClinicManager::averageWaitMinutes() const
{
    const std::vector<QueueEntry> entries = queueSnapshot();
    if (entries.empty()) {
        return 0;
    }
    int total = 0;
    for (const QueueEntry& entry : entries) {
        total += minutesSince(entry.joinedAt);
    }
    return total / static_cast<int>(entries.size());
}

void ClinicManager::removeFromQueue(const std::string& patientId)
{
    std::queue<QueueEntry> kept;
    while (!waitingQueue_.empty()) {
        if (waitingQueue_.front().patientId != patientId) {
            kept.push(waitingQueue_.front());
        }
        waitingQueue_.pop();
    }
    waitingQueue_ = std::move(kept);
}

// -------------------------------------------------------------------- undo

std::string ClinicManager::nextUndoDescription() const
{
    return undoStack_.empty() ? std::string() : undoStack_.top().description;
}

std::string ClinicManager::undo()
{
    if (undoStack_.empty()) {
        throw ValidationException("Nothing to undo");
    }
    // Check before popping so a refused undo stays on the stack.
    UndoAction& top = undoStack_.top();
    if (top.type == UndoType::DeletePatient && db_.findPatient(top.patient->getId())) {
        throw ValidationException("Cannot undo: patient ID " + top.patient->getId() + " is in use again");
    }
    if (top.type == UndoType::EditPatient && !db_.findPatient(top.patient->getId())) {
        throw ValidationException("Cannot undo: patient " + top.patient->getId() + " no longer exists");
    }

    UndoAction action = std::move(top);
    undoStack_.pop();

    switch (action.type) {
    case UndoType::DeletePatient:
        db_.restorePatient(std::move(action.patient));
        for (const Appointment& a : action.appointments) {
            db_.addAppointment(a);
        }
        break;
    case UndoType::RemoveAppointment: {
        const Appointment& a = *action.appointment;
        db_.addAppointment(a);
        PatientRecord* patient = db_.findPatient(a.getPatientId());
        if (patient && a.getStatus() != Appointment::Status::Cancelled) {
            patient->addAppointmentId(a.getId());
        }
        break;
    }
    case UndoType::EditPatient: {
        const std::string id = action.patient->getId();
        std::unique_ptr<PatientRecord> edited = db_.replacePatient(std::move(action.patient));
        replaceRecent(edited.get(), db_.findPatient(id));
        break;
    }
    }
    invalidateSort();
    record("Undo: " + action.description);
    return action.description;
}

void ClinicManager::record(const std::string& entry)
{
    history_.push_back(utils::currentTime() + "  " + entry);
}

// ------------------------------------------------------------------- cache

void ClinicManager::touch(PatientRecord* patient)
{
    if (!patient) {
        return;
    }
    int position = -1;
    for (int i = 0; i < recentCount_; ++i) {
        if (recentPatients_[i] == patient) {
            position = i;
            break;
        }
    }
    // Shift everything before `end` one slot right; the last drops off when full.
    int end = position;
    if (end < 0) {
        end = recentCount_ < RecentCapacity ? recentCount_++ : RecentCapacity - 1;
    }
    for (int i = end; i > 0; --i) {
        recentPatients_[i] = recentPatients_[i - 1];
    }
    recentPatients_[0] = patient;
}

std::vector<PatientRecord*> ClinicManager::getRecent() const
{
    return std::vector<PatientRecord*>(recentPatients_, recentPatients_ + recentCount_);
}

void ClinicManager::forgetRecent(const PatientRecord* patient)
{
    for (int i = 0; i < recentCount_; ++i) {
        if (recentPatients_[i] == patient) {
            for (int j = i; j < recentCount_ - 1; ++j) {
                recentPatients_[j] = recentPatients_[j + 1];
            }
            recentPatients_[--recentCount_] = nullptr;
            return;
        }
    }
}

void ClinicManager::replaceRecent(const PatientRecord* oldPatient, PatientRecord* newPatient)
{
    for (int i = 0; i < recentCount_; ++i) {
        if (recentPatients_[i] == oldPatient) {
            recentPatients_[i] = newPatient;
        }
    }
}

// --------------------------------------------------------- search and sort

void ClinicManager::invalidateSort()
{
    sortedView_.clear();
    sortedBy_ = SortKey::None;
}

std::string ClinicManager::nextPendingSlot(const std::string& patientId) const
{
    std::string earliest = kNoAppointment;
    for (const Appointment& a : db_.getAppointments()) {
        if (a.getPatientId() == patientId && a.getStatus() == Appointment::Status::Pending) {
            const std::string slot = a.getDate() + " " + a.getTime();
            if (slot < earliest) {
                earliest = slot;
            }
        }
    }
    return earliest;
}

SortResult ClinicManager::sort(SortAlgorithm algorithm, SortKey key)
{
    Compare less;
    std::map<std::string, std::string> nextSlot;
    switch (key) {
    case SortKey::Name:
        less = [](const PatientRecord* a, const PatientRecord* b) {
            return utils::toLower(a->getName()) < utils::toLower(b->getName());
        };
        break;
    case SortKey::Id:
        less = [](const PatientRecord* a, const PatientRecord* b) { return a->getId() < b->getId(); };
        break;
    case SortKey::Age:
        less = [](const PatientRecord* a, const PatientRecord* b) { return a->getAge() < b->getAge(); };
        break;
    case SortKey::AppointmentDate:
        for (const PatientRecord* p : db_.getPatients()) {
            nextSlot[p->getId()] = nextPendingSlot(p->getId());
        }
        less = [&nextSlot](const PatientRecord* a, const PatientRecord* b) {
            return nextSlot.at(a->getId()) < nextSlot.at(b->getId());
        };
        break;
    case SortKey::None:
        throw ValidationException("Choose a field to sort by");
    }

    std::vector<PatientRecord*> view = db_.getPatients(); // registration order
    long comparisons = 0;
    if (algorithm == SortAlgorithm::Insertion) {
        SortEngine::insertionSort(view, less, comparisons);
    } else {
        SortEngine::mergeSort(view, less, comparisons);
    }
    sortedView_ = view;
    sortedBy_ = key;
    return SortResult{sortAlgorithmName(algorithm), sortKeyName(key), view.size(), comparisons};
}

SearchResult ClinicManager::search(SearchField field, const std::string& query, SearchMode mode)
{
    std::string q = utils::trim(query);
    if (q.empty()) {
        throw ValidationException("Invalid search input: the search box is empty");
    }
    const auto notSortedBy = [](const std::string& what) {
        return ValidationException("Data is not sorted by " + what + ". Sort first or choose another algorithm.");
    };

    SearchResult result;
    switch (field) {
    case SearchField::Id: {
        q[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(q[0])));
        if (q.size() != 5 || q[0] != 'P' || !utils::isDigits(q.substr(1))) {
            throw ValidationException("Invalid search input: IDs look like P0001");
        }
        if (mode == SearchMode::Recursive) {
            throw ValidationException("Recursive search works on names. Choose Auto, Linear or Binary for IDs.");
        }
        const bool sorted = sortedBy_ == SortKey::Id;
        if (mode == SearchMode::Binary && !sorted) {
            throw notSortedBy("ID");
        }
        if (mode == SearchMode::Binary || (mode == SearchMode::Auto && sorted)) {
            const int index = SearchEngine::binarySearchById(sortedView_, q);
            if (index >= 0) {
                result.patients.push_back(sortedView_[static_cast<std::size_t>(index)]);
            }
            result.algorithm = "Binary search used (data sorted by ID)";
        } else {
            if (PatientRecord* p = SearchEngine::linearSearchById(db_.getPatients(), q)) {
                result.patients.push_back(p);
            }
            result.algorithm = "Linear search used (data not sorted by ID)";
        }
        break;
    }
    case SearchField::Name: {
        utils::requireNoCommas(q, "Invalid search input: name");
        if (mode == SearchMode::Linear) {
            throw ValidationException(
                "Linear search works on IDs and categories. Choose Auto, Binary or Recursive for names.");
        }
        const bool sorted = sortedBy_ == SortKey::Name;
        if (mode == SearchMode::Binary && !sorted) {
            throw notSortedBy("Name");
        }
        if (mode == SearchMode::Binary || (mode == SearchMode::Auto && sorted)) {
            result.patients = SearchEngine::binarySearchByName(sortedView_, q);
            result.algorithm = "Binary search used (exact name, data sorted by Name)";
        } else {
            SearchEngine::recursiveNameSearch(db_.getPatients(), q, 0, result.patients);
            result.algorithm = "Recursive search used (partial name match)";
        }
        break;
    }
    case SearchField::Category: {
        if (mode == SearchMode::Binary || mode == SearchMode::Recursive) {
            throw ValidationException("Categories are matched with a linear filter. Choose Auto or Linear.");
        }
        const std::string lower = utils::toLower(q);
        std::string category;
        if (lower == "general") {
            category = "General";
        } else if (lower == "emergency") {
            category = "Emergency";
        } else if (lower == "follow-up" || lower == "followup" || lower == "follow up") {
            category = "Follow-Up";
        } else {
            throw ValidationException("Invalid search input: category must be General, Emergency or Follow-Up");
        }
        result.patients = SearchEngine::filterByCategory(db_.getPatients(), category);
        result.algorithm = "Linear filter used (category)";
        break;
    }
    }
    if (!result.patients.empty()) {
        touch(result.patients.front());
    }
    return result;
}

// ----------------------------------------------------------------- reports

std::vector<AttendanceRow> ClinicManager::mostFrequentlyAttended(std::size_t top) const
{
    std::map<std::string, int> completed;
    for (const Appointment& a : db_.getAppointments()) {
        if (a.getStatus() == Appointment::Status::Completed) {
            ++completed[a.getPatientId()];
        }
    }
    std::vector<PatientRecord*> attended;
    for (PatientRecord* p : db_.getPatients()) {
        if (completed.count(p->getId()) > 0) {
            attended.push_back(p);
        }
    }
    // Most visits first, using our own (stable) merge sort.
    long comparisons = 0;
    SortEngine::mergeSort(
        attended,
        [&completed](const PatientRecord* a, const PatientRecord* b) {
            return completed.at(a->getId()) > completed.at(b->getId());
        },
        comparisons);

    std::vector<AttendanceRow> rows;
    for (const PatientRecord* p : attended) {
        const int count = completed.at(p->getId());
        // Keep going past `top` only while tied with the last row shown.
        if (rows.size() >= top && count < rows.back().completed) {
            break;
        }
        rows.push_back(AttendanceRow{p, count});
    }
    return rows;
}

std::vector<PendingRow> ClinicManager::pendingAppointments() const
{
    std::vector<PendingRow> rows;
    for (const PatientRecord* p : db_.getPatients()) {
        PendingRow row{p, 0, kNoAppointment};
        for (const Appointment& a : db_.getAppointments()) {
            if (a.getPatientId() == p->getId() && a.getStatus() == Appointment::Status::Pending) {
                ++row.pending;
                const std::string slot = a.getDate() + " " + a.getTime();
                if (slot < row.next) {
                    row.next = slot;
                }
            }
        }
        if (row.pending > 0) {
            rows.push_back(row);
        }
    }
    return rows;
}

std::vector<CategoryRow> ClinicManager::categoryDistribution() const
{
    std::vector<CategoryRow> rows{{"General", 0, 0.0}, {"Emergency", 0, 0.0}, {"Follow-Up", 0, 0.0}};
    int total = 0;
    processAll([&rows, &total](const PatientRecord& p) {
        const std::string category = p.getCategory(); // virtual call
        for (CategoryRow& row : rows) {
            if (row.category == category) {
                ++row.count;
            }
        }
        ++total;
    });
    for (CategoryRow& row : rows) {
        row.percent = total == 0 ? 0.0 : 100.0 * row.count / total;
    }
    return rows;
}

DailySummary ClinicManager::dailySummary(const std::string& date) const
{
    if (!utils::isValidDate(date)) {
        throw ValidationException("Date must be a valid date in YYYY-MM-DD format");
    }
    DailySummary summary;
    summary.date = date;
    for (const Appointment& a : db_.getAppointments()) {
        if (a.getDate() != date) {
            continue;
        }
        ++summary.total;
        switch (a.getStatus()) {
        case Appointment::Status::Pending:
            ++summary.pending;
            break;
        case Appointment::Status::Completed:
            ++summary.completed;
            break;
        case Appointment::Status::Cancelled:
            ++summary.cancelled;
            break;
        }
    }
    return summary;
}

// ------------------------------------------------------------------- files

void ClinicManager::save(const std::string& patientsPath, const std::string& appointmentsPath) const
{
    utils::FileHandler::savePatients(patientsPath, db_.getPatients());
    utils::FileHandler::saveAppointments(appointmentsPath, db_.getAppointments());
}

void ClinicManager::load(const std::string& patientsPath, const std::string& appointmentsPath)
{
    // Load into temporaries; live data is replaced only if both files are good.
    std::vector<std::unique_ptr<PatientRecord>> patients = utils::FileHandler::loadPatients(patientsPath);
    std::vector<Appointment> appointments = utils::FileHandler::loadAppointments(appointmentsPath);

    for (const Appointment& a : appointments) {
        PatientRecord* owner = nullptr;
        for (const auto& p : patients) {
            if (p->getId() == a.getPatientId()) {
                owner = p.get();
                break;
            }
        }
        if (!owner) {
            throw FileException(appointmentsPath + ": appointment " + a.getId() + " refers to unknown patient " +
                                a.getPatientId());
        }
        if (a.getStatus() != Appointment::Status::Cancelled) {
            owner->addAppointmentId(a.getId());
        }
    }

    clearAll();
    db_.replaceAll(std::move(patients), std::move(appointments));
    syncPatientIds();
    std::vector<std::string> appointmentIds;
    for (const Appointment& a : db_.getAppointments()) {
        appointmentIds.push_back(a.getId());
    }
    utils::IDGenerator::syncAppointments(appointmentIds);
}

void ClinicManager::clearAll()
{
    db_.clear();
    waitingQueue_ = {};
    undoStack_ = {};
    for (PatientRecord*& slot : recentPatients_) {
        slot = nullptr;
    }
    recentCount_ = 0;
    invalidateSort();
    history_.clear();
    utils::IDGenerator::reset();
}

void ClinicManager::syncPatientIds() const
{
    std::vector<std::string> ids;
    for (const PatientRecord* p : db_.getPatients()) {
        ids.push_back(p->getId());
    }
    utils::IDGenerator::syncPatients(ids);
}

} // namespace scapms::controller
