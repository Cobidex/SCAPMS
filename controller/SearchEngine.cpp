#include "controller/SearchEngine.h"

#include "utils/TextUtils.h"

namespace scapms::controller {

using model::PatientRecord;

// Checks each element from the front until the ID matches.
PatientRecord* SearchEngine::linearSearchById(const std::vector<PatientRecord*>& items, const std::string& id)
{
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (items[i]->getId() == id) {
            return items[i];
        }
    }
    return nullptr;
}

// Halves the [low, high] range each step until the ID is found or the range is empty.
int SearchEngine::binarySearchById(const std::vector<PatientRecord*>& sortedById, const std::string& id)
{
    int low = 0;
    int high = static_cast<int>(sortedById.size()) - 1;
    while (low <= high) {
        const int mid = low + (high - low) / 2;
        const std::string& midId = sortedById[static_cast<std::size_t>(mid)]->getId();
        if (midId == id) {
            return mid;
        }
        if (midId < id) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return -1;
}

// Binary search on the lower-cased name, then widens to any equal neighbours.
std::vector<PatientRecord*> SearchEngine::binarySearchByName(const std::vector<PatientRecord*>& sortedByName,
                                                             const std::string& name)
{
    std::vector<PatientRecord*> matches;
    const std::string target = utils::toLower(name);
    const auto nameAt = [&sortedByName](int i) {
        return utils::toLower(sortedByName[static_cast<std::size_t>(i)]->getName());
    };

    int low = 0;
    int high = static_cast<int>(sortedByName.size()) - 1;
    int found = -1;
    while (low <= high) {
        const int mid = low + (high - low) / 2;
        const std::string midName = nameAt(mid);
        if (midName == target) {
            found = mid;
            break;
        }
        if (midName < target) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    if (found < 0) {
        return matches;
    }

    int first = found;
    while (first > 0 && nameAt(first - 1) == target) {
        --first;
    }
    int last = found;
    while (last + 1 < static_cast<int>(sortedByName.size()) && nameAt(last + 1) == target) {
        ++last;
    }
    for (int i = first; i <= last; ++i) {
        matches.push_back(sortedByName[static_cast<std::size_t>(i)]);
    }
    return matches;
}

// Checks one item, then recurses on the rest of the list.
void SearchEngine::recursiveNameSearch(const std::vector<PatientRecord*>& items, const std::string& query,
                                       std::size_t index, std::vector<PatientRecord*>& results)
{
    if (index >= items.size()) {
        return;
    }
    if (utils::containsIgnoreCase(items[index]->getName(), query)) {
        results.push_back(items[index]);
    }
    recursiveNameSearch(items, query, index + 1, results);
}

// Keeps every patient whose category matches, ignoring case.
std::vector<PatientRecord*> SearchEngine::filterByCategory(const std::vector<PatientRecord*>& items,
                                                           const std::string& category)
{
    std::vector<PatientRecord*> matches;
    const std::string target = utils::toLower(category);
    for (PatientRecord* p : items) {
        if (utils::toLower(p->getCategory()) == target) {
            matches.push_back(p);
        }
    }
    return matches;
}

} // namespace scapms::controller
