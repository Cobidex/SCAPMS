#include "controller/SortEngine.h"

namespace scapms::controller {

using model::PatientRecord;

// Shifts each item left past every larger item before it. Equal items are not
// passed, so the sort is stable.
void SortEngine::insertionSort(std::vector<PatientRecord*>& items, const Compare& less, long& comparisons)
{
    for (std::size_t i = 1; i < items.size(); ++i) {
        PatientRecord* current = items[i];
        std::size_t j = i;
        while (j > 0) {
            ++comparisons;
            if (!less(current, items[j - 1])) {
                break;
            }
            items[j] = items[j - 1];
            --j;
        }
        items[j] = current;
    }
}

void SortEngine::mergeSort(std::vector<PatientRecord*>& items, const Compare& less, long& comparisons)
{
    mergeSortRange(items, 0, items.size(), less, comparisons);
}

void SortEngine::mergeSortRange(std::vector<PatientRecord*>& items, std::size_t left, std::size_t right,
                                const Compare& less, long& comparisons)
{
    if (right - left <= 1) {
        return;
    }
    const std::size_t mid = left + (right - left) / 2;
    mergeSortRange(items, left, mid, less, comparisons);
    mergeSortRange(items, mid, right, less, comparisons);
    merge(items, left, mid, right, less, comparisons);
}

// Takes from the right half only when it is strictly smaller, which keeps the
// merge stable.
void SortEngine::merge(std::vector<PatientRecord*>& items, std::size_t left, std::size_t mid, std::size_t right,
                       const Compare& less, long& comparisons)
{
    std::vector<PatientRecord*> merged;
    merged.reserve(right - left);
    std::size_t i = left;
    std::size_t j = mid;
    while (i < mid && j < right) {
        ++comparisons;
        if (less(items[j], items[i])) {
            merged.push_back(items[j++]);
        } else {
            merged.push_back(items[i++]);
        }
    }
    while (i < mid) {
        merged.push_back(items[i++]);
    }
    while (j < right) {
        merged.push_back(items[j++]);
    }
    for (std::size_t k = 0; k < merged.size(); ++k) {
        items[left + k] = merged[k];
    }
}

} // namespace scapms::controller
