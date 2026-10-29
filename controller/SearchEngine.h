#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "model/PatientRecord.h"

namespace scapms::controller {

// Manual searching algorithms over non-owning views of the patients.
// Input validation (empty query, badly shaped ID) happens in ClinicManager
// before these are called.
class SearchEngine {
public:
    // Any order. Returns the patient with this exact ID, or nullptr.
    static model::PatientRecord* linearSearchById(const std::vector<model::PatientRecord*>& items,
                                                  const std::string& id);

    // `sortedById` must be sorted by ID (A->Z). Returns the index of the
    // match, or -1.
    static int binarySearchById(const std::vector<model::PatientRecord*>& sortedById, const std::string& id);

    // `sortedByName` must be sorted by utils::toLower(name). Returns every
    // patient whose name equals `name`, ignoring case.
    static std::vector<model::PatientRecord*> binarySearchByName(
        const std::vector<model::PatientRecord*>& sortedByName, const std::string& name);

    // Recursive partial-name search. Adds every patient from `index` onward
    // whose name contains `query` (ignoring case) to `results`.
    static void recursiveNameSearch(const std::vector<model::PatientRecord*>& items, const std::string& query,
                                    std::size_t index, std::vector<model::PatientRecord*>& results);

    // Linear filter on getCategory(), ignoring case.
    static std::vector<model::PatientRecord*> filterByCategory(const std::vector<model::PatientRecord*>& items,
                                                               const std::string& category);
};

} // namespace scapms::controller
