#pragma once

#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QTimer;

namespace scapms::controller {
class ClinicManager;
}

namespace scapms::gui {

// Search & Sort tab: pick the search technique and the sorting algorithm.
class SearchPanel : public QWidget {
    Q_OBJECT

public:
    explicit SearchPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

    void focusSearch();

public slots:
    void refresh();

signals:
    void patientAccessed();
    void statusMessage(const QString& text);

private slots:
    void onSearchClicked();
    void onLiveSearch();
    void onSort();

private:
    // Interactive searches show errors as dialogs; live searches show them
    // in the info label so typing is never interrupted.
    void runSearch(bool interactive);
    void showCurrentView();

    controller::ClinicManager& manager_;
    QComboBox* fieldCombo_ = nullptr;
    QComboBox* searchAlgorithmCombo_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    QPushButton* searchButton_ = nullptr;
    QComboBox* sortAlgorithmCombo_ = nullptr;
    QComboBox* sortKeyCombo_ = nullptr;
    QPushButton* sortButton_ = nullptr;
    QTableWidget* results_ = nullptr;
    QLabel* info_ = nullptr;
    QTimer* debounce_ = nullptr;
};

} // namespace scapms::gui
