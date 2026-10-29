#pragma once

#include <QWidget>

class QComboBox;
class QLabel;
class QListWidget;
class QPushButton;
class QTimer;

namespace scapms::controller {
class ClinicManager;
}

namespace scapms::gui {

// Waiting Queue tab: first come, first served.
class QueuePanel : public QWidget {
    Q_OBJECT

public:
    explicit QueuePanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

public slots:
    void refresh();

signals:
    void dataChanged();
    void statusMessage(const QString& text);

private slots:
    void onAdd();
    void onCallNext();

private:
    controller::ClinicManager& manager_;
    QComboBox* patientCombo_ = nullptr;
    QPushButton* addButton_ = nullptr;
    QListWidget* list_ = nullptr;
    QPushButton* callNextButton_ = nullptr;
    QLabel* nowServing_ = nullptr;
    QLabel* averageWait_ = nullptr;
    QTimer* waitClock_ = nullptr;
};

} // namespace scapms::gui
