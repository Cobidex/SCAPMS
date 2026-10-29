#pragma once

#include <QWidget>

class QComboBox;
class QDateEdit;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QTimeEdit;

namespace scapms::controller {
class ClinicManager;
}

namespace scapms::gui {

// Appointments tab: schedule, cancel and complete appointments.
class AppointmentPanel : public QWidget {
    Q_OBJECT

public:
    explicit AppointmentPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

public slots:
    void refresh();

signals:
    void dataChanged();
    void statusMessage(const QString& text);

private slots:
    void onSchedule();
    void onCancel();
    void onComplete();
    void onReasonChanged(const QString& text);

private:
    QString selectedAppointmentId();

    controller::ClinicManager& manager_;
    QComboBox* patientCombo_ = nullptr;
    QDateEdit* dateEdit_ = nullptr;
    QTimeEdit* timeEdit_ = nullptr;
    QLineEdit* reasonEdit_ = nullptr;
    QPushButton* scheduleButton_ = nullptr;
    QPushButton* cancelButton_ = nullptr;
    QPushButton* completeButton_ = nullptr;
    QComboBox* filterCombo_ = nullptr;
    QTableWidget* table_ = nullptr;
};

} // namespace scapms::gui
