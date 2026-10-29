#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include "model/PatientRecord.h"

namespace scapms::controller {

// less(a, b) returns true when a must come before b.
using Compare = std::function<bool(const model::PatientRecord*, const model::PatientRecord*)>;

// Manual sorting algorithms. Both sort a non-owning view in place, must be
// stable, and add 1 to `comparisons` for every call to less().
class SortEngine {
public:
    static void insertionSort(std::vector<model::PatientRecord*>& items, const Compare& less, long& comparisons);
    static void mergeSort(std::vector<model::PatientRecord*>& items, const Compare& less, long& comparisons);

private:
    // Sorts items[left, right) recursively.
    static void mergeSortRange(std::vector<model::PatientRecord*>& items, std::size_t left, std::size_t right,
                               const Compare& less, long& comparisons);
    // Merges the sorted halves items[left, mid) and items[mid, right).
    static void merge(std::vector<model::PatientRecord*>& items, std::size_t left, std::size_t mid,
                      std::size_t right, const Compare& less, long& comparisons);
};

} // namespace scapms::controller
