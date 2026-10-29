#pragma once

#include <QWidget>

class QGridLayout;
class QLabel;
class QListWidget;
class QTimer;

namespace scapms::controller {
class ClinicManager;
}

namespace scapms::gui {

// Dashboard tab: stat cards, recently accessed patients and three timers.
class DashboardPanel : public QWidget {
    Q_OBJECT

public:
    explicit DashboardPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

public slots:
    void refresh();

signals:
    void statusMessage(const QString& text);

private slots:
    void checkReminders();
    void checkWaitingTimes();

private:
    QLabel* addCard(QGridLayout* grid, int row, int col, const QString& title);

    controller::ClinicManager& manager_;
    QLabel* totalLabel_ = nullptr;
    QLabel* generalLabel_ = nullptr;
    QLabel* emergencyLabel_ = nullptr;
    QLabel* followUpLabel_ = nullptr;
    QLabel* todayLabel_ = nullptr;
    QLabel* queueLabel_ = nullptr;
    QLabel* waitAlert_ = nullptr;
    QListWidget* recentList_ = nullptr;
    QTimer* refreshTimer_ = nullptr;
    QTimer* reminderTimer_ = nullptr;
    QTimer* waitTimer_ = nullptr;
};

} // namespace scapms::gui
