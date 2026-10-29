#pragma once

#include <QWidget>

class QComboBox;
class QListWidget;
class QPushButton;

namespace scapms::controller {
class ClinicManager;
}

namespace scapms::gui {

// Admin tab: delete patients, remove appointments, undo, history.
class AdminPanel : public QWidget {
    Q_OBJECT

public:
    explicit AdminPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

public slots:
    void refresh();
    void undoLast();

signals:
    void dataChanged();
    void statusMessage(const QString& text);

private slots:
    void onDeletePatient();
    void onRemoveAppointment();

private:
    controller::ClinicManager& manager_;
    QComboBox* patientCombo_ = nullptr;
    QPushButton* deletePatientButton_ = nullptr;
    QComboBox* appointmentCombo_ = nullptr;
    QPushButton* removeAppointmentButton_ = nullptr;
    QPushButton* undoButton_ = nullptr;
    QListWidget* history_ = nullptr;
};

} // namespace scapms::gui
