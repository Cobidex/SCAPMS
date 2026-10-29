#include "controller/SearchEngine.h"

#include <stdexcept>

#include "utils/TextUtils.h"

namespace scapms::controller {

using model::PatientRecord;

// TODO(student): linear search by ID.
//  - Check each element from index 0 until the ID matches.
//  - Return the matching pointer, or nullptr if there is none.
//  - Do not use std::find or std::find_if.
PatientRecord* SearchEngine::linearSearchById(const std::vector<PatientRecord*>& items, const std::string& id)
{
    (void)items;
    (void)id;
    throw std::logic_error("Linear search is not implemented yet (controller/SearchEngine.cpp)");
}

// TODO(student): binary search by ID.
//  - Keep low/high bounds, compare the middle element, discard half each step.
//  - Return the index of the match, or -1.
//  - Do not use std::binary_search or std::lower_bound.
int SearchEngine::binarySearchById(const std::vector<PatientRecord*>& sortedById, const std::string& id)
{
    (void)sortedById;
    (void)id;
    throw std::logic_error("Binary search is not implemented yet (controller/SearchEngine.cpp)");
}

// TODO(student): binary search by exact name.
//  - Compare utils::toLower() of both names so case does not matter.
//  - Names can repeat: after one match, walk left and right to collect the
//    neighbours with the same name. Return them in list order.
std::vector<PatientRecord*> SearchEngine::binarySearchByName(const std::vector<PatientRecord*>& sortedByName,
                                                             const std::string& name)
{
    (void)sortedByName;
    (void)name;
    throw std::logic_error("Binary search is not implemented yet (controller/SearchEngine.cpp)");
}

// TODO(student): recursive name search.
//  - Base case: index >= items.size() -> stop.
//  - Otherwise, if the name at `index` contains `query` (ignoring case; see
//    utils::containsIgnoreCase), add it to `results`.
//  - Then call this function again with index + 1. No loops.
void SearchEngine::recursiveNameSearch(const std::vector<PatientRecord*>& items, const std::string& query,
                                       std::size_t index, std::vector<PatientRecord*>& results)
{
    (void)items;
    (void)query;
    (void)index;
    (void)results;
    throw std::logic_error("Recursive search is not implemented yet (controller/SearchEngine.cpp)");
}

// TODO(student): category filter.
//  - Linear pass; keep every patient whose getCategory() equals `category`,
//    ignoring case.
std::vector<PatientRecord*> SearchEngine::filterByCategory(const std::vector<PatientRecord*>& items,
                                                           const std::string& category)
{
    (void)items;
    (void)category;
    throw std::logic_error("Category filter is not implemented yet (controller/SearchEngine.cpp)");
}

} // namespace scapms::controller
