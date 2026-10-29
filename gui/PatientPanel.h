#pragma once

#include <QWidget>
#include <memory>

class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QTableWidget;

namespace scapms::controller {
class ClinicManager;
}
namespace scapms::model {
class PatientRecord;
}

namespace scapms::gui {

// Patient Records tab: register, edit and delete patients.
class PatientPanel : public QWidget {
    Q_OBJECT

public:
    explicit PatientPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

public slots:
    void refresh();

signals:
    void dataChanged();
    void patientAccessed();
    void statusMessage(const QString& text);

private slots:
    void onAdd();
    void onUpdate();
    void onDelete();
    void clearForm();
    void onIdChanged(const QString& text);
    void onSelectionChanged();

private:
    std::unique_ptr<model::PatientRecord> buildFromForm() const;
    void fillForm(const model::PatientRecord& patient);
    void setEditMode(bool editing);

    controller::ClinicManager& manager_;
    QString editingId_;

    QLineEdit* idEdit_ = nullptr;
    QLabel* idHint_ = nullptr;
    QLineEdit* nameEdit_ = nullptr;
    QLineEdit* ageEdit_ = nullptr;
    QLineEdit* contactEdit_ = nullptr;
    QComboBox* typeCombo_ = nullptr;
    QStackedWidget* typeStack_ = nullptr;
    QLineEdit* reasonEdit_ = nullptr;
    QSpinBox* severitySpin_ = nullptr;
    QLineEdit* triageEdit_ = nullptr;
    QDateEdit* previousVisitEdit_ = nullptr;
    QLineEdit* doctorEdit_ = nullptr;
    QPushButton* addButton_ = nullptr;
    QPushButton* updateButton_ = nullptr;
    QPushButton* deleteButton_ = nullptr;
    QPushButton* clearButton_ = nullptr;
    QTableWidget* table_ = nullptr;
};

} // namespace scapms::gui
