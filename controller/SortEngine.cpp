#include "controller/SortEngine.h"

#include <stdexcept>

namespace scapms::controller {

using model::PatientRecord;

// TODO(student): insertion sort.
//  - Sort `items` in place into the order defined by less().
//  - Stable: items that compare equal keep their original order.
//  - Add 1 to `comparisons` each time you call less().
//  - Do not use std::sort, std::stable_sort or any other library sort.
void SortEngine::insertionSort(std::vector<PatientRecord*>& items, const Compare& less, long& comparisons)
{
    (void)items;
    (void)less;
    (void)comparisons;
    throw std::logic_error("Insertion sort is not implemented yet (controller/SortEngine.cpp)");
}

// TODO(student): merge sort entry point.
//  - Call mergeSortRange on the whole vector.
void SortEngine::mergeSort(std::vector<PatientRecord*>& items, const Compare& less, long& comparisons)
{
    (void)items;
    (void)less;
    (void)comparisons;
    throw std::logic_error("Merge sort is not implemented yet (controller/SortEngine.cpp)");
}

// TODO(student): recursive step of merge sort.
//  - Base case: a range of 0 or 1 items is already sorted.
//  - Otherwise split at the middle, sort both halves recursively, then merge.
void SortEngine::mergeSortRange(std::vector<PatientRecord*>& items, std::size_t left, std::size_t right,
                                const Compare& less, long& comparisons)
{
    (void)items;
    (void)left;
    (void)right;
    (void)less;
    (void)comparisons;
    throw std::logic_error("Merge sort is not implemented yet (controller/SortEngine.cpp)");
}

// TODO(student): merge two sorted halves using a temporary vector.
//  - Stable: when two items are equal, take the one from the left half first.
//  - Add 1 to `comparisons` each time you call less().
void SortEngine::merge(std::vector<PatientRecord*>& items, std::size_t left, std::size_t mid, std::size_t right,
                       const Compare& less, long& comparisons)
{
    (void)items;
    (void)left;
    (void)mid;
    (void)right;
    (void)less;
    (void)comparisons;
    throw std::logic_error("Merge sort is not implemented yet (controller/SortEngine.cpp)");
}

} // namespace scapms::controller
