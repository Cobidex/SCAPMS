#include "gui/SearchPanel.h"

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

#include "controller/ClinicManager.h"
#include "gui/GuiHelpers.h"

namespace scapms::gui {

using controller::SearchField;
using controller::SearchMode;
using controller::SortAlgorithm;
using controller::SortKey;

namespace {

// Each combo item stores its enum value as item data.
template <typename Enum>
void addOption(QComboBox* combo, const QString& text, Enum value)
{
    combo->addItem(text, static_cast<int>(value));
}

template <typename Enum>
Enum selected(const QComboBox* combo)
{
    return static_cast<Enum>(combo->currentData().toInt());
}

QString explanation(SortAlgorithm algorithm)
{
    if (algorithm == SortAlgorithm::Insertion) {
        return "Insertion sort is stable and close to O(n) on nearly sorted lists, O(n²) in the worst case.";
    }
    return "Merge sort guarantees O(n log n), is stable and uses O(n) extra memory.";
}

} // namespace

SearchPanel::SearchPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager)
{
    fieldCombo_ = new QComboBox;
    addOption(fieldCombo_, "Patient ID", SearchField::Id);
    addOption(fieldCombo_, "Name", SearchField::Name);
    addOption(fieldCombo_, "Category", SearchField::Category);
    fieldCombo_->setToolTip("Field to search");
    searchAlgorithmCombo_ = new QComboBox;
    addOption(searchAlgorithmCombo_, "Auto", SearchMode::Auto);
    addOption(searchAlgorithmCombo_, "Linear", SearchMode::Linear);
    addOption(searchAlgorithmCombo_, "Binary", SearchMode::Binary);
    addOption(searchAlgorithmCombo_, "Recursive", SearchMode::Recursive);
    searchAlgorithmCombo_->setToolTip(
        "Auto picks binary search when the data is sorted by the same field, otherwise linear (ID) or "
        "recursive (name).");
    searchEdit_ = new QLineEdit;
    searchEdit_->setPlaceholderText("Type an ID, name or category…");
    searchEdit_->setToolTip("Results update as you type (Ctrl+F jumps here)");
    searchButton_ = new QPushButton("Search");
    searchButton_->setToolTip("Run the search");

    auto* searchGroup = new QGroupBox("Search");
    auto* searchGrid = new QGridLayout(searchGroup);
    searchGrid->addWidget(new QLabel("Search by:"), 0, 0);
    searchGrid->addWidget(fieldCombo_, 0, 1);
    searchGrid->addWidget(new QLabel("Algorithm:"), 0, 2);
    searchGrid->addWidget(searchAlgorithmCombo_, 0, 3);
    searchGrid->addWidget(searchEdit_, 1, 0, 1, 3);
    searchGrid->addWidget(searchButton_, 1, 3);

    sortAlgorithmCombo_ = new QComboBox;
    addOption(sortAlgorithmCombo_, "Insertion Sort", SortAlgorithm::Insertion);
    addOption(sortAlgorithmCombo_, "Merge Sort", SortAlgorithm::Merge);
    sortAlgorithmCombo_->setToolTip("Sorting algorithm to use");
    sortKeyCombo_ = new QComboBox;
    addOption(sortKeyCombo_, "Patient Name", SortKey::Name);
    addOption(sortKeyCombo_, "Patient ID", SortKey::Id);
    addOption(sortKeyCombo_, "Age", SortKey::Age);
    addOption(sortKeyCombo_, "Appointment Date", SortKey::AppointmentDate);
    sortKeyCombo_->setToolTip("Appointment Date uses each patient's next pending appointment; none sorts last.");
    sortButton_ = new QPushButton("SORT");
    sortButton_->setToolTip("Sort all patients with the chosen algorithm");

    auto* sortGroup = new QGroupBox("Sort");
    auto* sortGrid = new QGridLayout(sortGroup);
    sortGrid->addWidget(new QLabel("Sort Algorithm:"), 0, 0);
    sortGrid->addWidget(sortAlgorithmCombo_, 0, 1);
    sortGrid->addWidget(new QLabel("Sort By:"), 1, 0);
    sortGrid->addWidget(sortKeyCombo_, 1, 1);
    sortGrid->addWidget(sortButton_, 2, 0, 1, 2);

    auto* controls = new QHBoxLayout;
    controls->addWidget(searchGroup, 3);
    controls->addWidget(sortGroup, 2);

    results_ = new QTableWidget;
    setupTable(results_, patientHeaders());
    info_ = new QLabel;
    info_->setWordWrap(true);
    info_->setToolTip("Which algorithm ran, and how many comparisons it made");

    auto* root = new QVBoxLayout(this);
    root->addLayout(controls);
    root->addWidget(results_, 1);
    root->addWidget(info_);

    debounce_ = new QTimer(this);
    debounce_->setSingleShot(true);
    debounce_->setInterval(300);

    connect(searchEdit_, &QLineEdit::textChanged, debounce_, qOverload<>(&QTimer::start));
    connect(debounce_, &QTimer::timeout, this, &SearchPanel::onLiveSearch);
    connect(searchEdit_, &QLineEdit::returnPressed, this, &SearchPanel::onSearchClicked);
    connect(searchButton_, &QPushButton::clicked, this, &SearchPanel::onSearchClicked);
    connect(sortButton_, &QPushButton::clicked, this, &SearchPanel::onSort);
    connect(results_, &QTableWidget::itemSelectionChanged, this, [this] {
        if (auto* p = manager_.findPatient(selectedRowId(results_).toStdString())) {
            manager_.touch(p);
            emit patientAccessed();
        }
    });
}

void SearchPanel::focusSearch()
{
    searchEdit_->setFocus();
    searchEdit_->selectAll();
}

void SearchPanel::refresh()
{
    // Patients may have been added or deleted: drop any old results.
    showCurrentView();
}

void SearchPanel::showCurrentView()
{
    if (manager_.getSortedBy() == SortKey::None) {
        fillPatientTable(results_, manager_.getPatients());
        info_->setText("Showing all patients in registration order (not sorted).");
    } else {
        fillPatientTable(results_, manager_.getSortedView());
        info_->setText("Showing patients sorted by " + qs(controller::sortKeyName(manager_.getSortedBy())) + ".");
    }
}

void SearchPanel::onSearchClicked()
{
    debounce_->stop();
    runSearch(true);
}

void SearchPanel::onLiveSearch()
{
    if (searchEdit_->text().trimmed().isEmpty()) {
        showCurrentView();
        return;
    }
    runSearch(false);
}

void SearchPanel::runSearch(bool interactive)
{
    const auto field = selected<SearchField>(fieldCombo_);
    const auto mode = selected<SearchMode>(searchAlgorithmCombo_);
    const std::string query = searchEdit_->text().toStdString();
    controller::SearchResult result;

    const auto search = [&] { result = manager_.search(field, query, mode); };
    if (interactive) {
        if (!guarded(this, search)) {
            return;
        }
    } else {
        try {
            search();
        } catch (const std::exception& e) {
            info_->setText(qs(e.what()));
            return;
        }
    }
    fillPatientTable(results_, result.patients);
    info_->setText(QString("%1: %2 match(es).").arg(qs(result.algorithm)).arg(result.patients.size()));
    if (!result.patients.empty()) {
        emit patientAccessed();
    }
    if (interactive) {
        emit statusMessage(qs(result.algorithm));
    }
}

void SearchPanel::onSort()
{
    const auto algorithm = selected<SortAlgorithm>(sortAlgorithmCombo_);
    const auto key = selected<SortKey>(sortKeyCombo_);
    controller::SortResult result;
    if (!guarded(this, [&] { result = manager_.sort(algorithm, key); })) {
        return;
    }
    fillPatientTable(results_, manager_.getSortedView());
    info_->setText(QString("%1 by %2: %3 records, %4 comparisons. %5")
                       .arg(qs(result.algorithm), qs(result.key))
                       .arg(result.count)
                       .arg(result.comparisons)
                       .arg(explanation(algorithm)));
    emit statusMessage(qs(result.algorithm) + " by " + qs(result.key) + " done");
}

} // namespace scapms::gui
