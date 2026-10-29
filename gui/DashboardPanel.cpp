#include "gui/DashboardPanel.h"

#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QTimer>
#include <QVBoxLayout>

#include "controller/ClinicManager.h"
#include "gui/GuiHelpers.h"
#include "utils/TextUtils.h"

namespace scapms::gui {

namespace {
// Lower the reminder/wait intervals and the threshold for demos and screenshots.
constexpr int kRefreshIntervalMs = 5 * 1000;
constexpr int kReminderIntervalMs = 60 * 1000;
constexpr int kWaitCheckIntervalMs = 60 * 1000;
constexpr int kReminderWindowMinutes = 30;
constexpr int kWaitAlertMinutes = 30;
} // namespace

DashboardPanel::DashboardPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager)
{
    auto* grid = new QGridLayout;
    totalLabel_ = addCard(grid, 0, 0, "Total patients");
    generalLabel_ = addCard(grid, 0, 1, "General");
    emergencyLabel_ = addCard(grid, 0, 2, "Emergency");
    followUpLabel_ = addCard(grid, 1, 0, "Follow-Up");
    todayLabel_ = addCard(grid, 1, 1, "Today's appointments");
    queueLabel_ = addCard(grid, 1, 2, "Queue length");

    waitAlert_ = new QLabel;
    waitAlert_->setToolTip("Turns red when someone has waited more than 30 minutes");

    recentList_ = new QListWidget;
    recentList_->setToolTip("Last 10 patients opened or found by search, newest first");
    auto* recentGroup = new QGroupBox("Recently accessed");
    auto* recentLayout = new QVBoxLayout(recentGroup);
    recentLayout->addWidget(recentList_);

    auto* root = new QVBoxLayout(this);
    root->addLayout(grid);
    root->addWidget(waitAlert_);
    root->addWidget(recentGroup, 1);

    refreshTimer_ = new QTimer(this);
    connect(refreshTimer_, &QTimer::timeout, this, &DashboardPanel::refresh);
    refreshTimer_->start(kRefreshIntervalMs);

    reminderTimer_ = new QTimer(this);
    connect(reminderTimer_, &QTimer::timeout, this, &DashboardPanel::checkReminders);
    reminderTimer_->start(kReminderIntervalMs);

    waitTimer_ = new QTimer(this);
    connect(waitTimer_, &QTimer::timeout, this, &DashboardPanel::checkWaitingTimes);
    waitTimer_->start(kWaitCheckIntervalMs);
}

QLabel* DashboardPanel::addCard(QGridLayout* grid, int row, int col, const QString& title)
{
    auto* card = new QFrame;
    card->setFrameShape(QFrame::StyledPanel);
    card->setMinimumHeight(90);
    auto* layout = new QVBoxLayout(card);
    auto* titleLabel = new QLabel(title);
    auto* value = new QLabel("0");
    QFont big = value->font();
    big.setPointSize(big.pointSize() + 14);
    big.setBold(true);
    value->setFont(big);
    layout->addWidget(titleLabel);
    layout->addWidget(value);
    grid->addWidget(card, row, col);
    return value;
}

void DashboardPanel::refresh()
{
    int general = 0;
    int emergency = 0;
    int followUp = 0;
    manager_.processAll([&](const model::PatientRecord& p) {
        const std::string category = p.getCategory();
        general += category == "General";
        emergency += category == "Emergency";
        followUp += category == "Follow-Up";
    });
    const std::string today = utils::today();
    int todays = 0;
    for (const model::Appointment& a : manager_.getAppointments()) {
        todays += a.getDate() == today && a.getStatus() != model::Appointment::Status::Cancelled;
    }

    totalLabel_->setText(QString::number(general + emergency + followUp));
    generalLabel_->setText(QString::number(general));
    emergencyLabel_->setText(QString::number(emergency));
    followUpLabel_->setText(QString::number(followUp));
    todayLabel_->setText(QString::number(todays));
    queueLabel_->setText(QString::number(manager_.queueLength()));

    recentList_->clear();
    for (const model::PatientRecord* p : manager_.getRecent()) {
        auto* item = new QListWidgetItem(qs(p->getId()) + "  " + qs(p->getName()) + "  (" + qs(p->getCategory()) +
                                             ")",
                                         recentList_);
        const QColor colour = categoryColour(qs(p->getCategory()));
        if (colour.isValid()) {
            item->setBackground(colour);
            item->setForeground(Qt::black);
        }
    }

    const int longest = manager_.longestWaitMinutes();
    waitAlert_->setText(QString("Longest wait in queue: %1 min").arg(longest));
    waitAlert_->setStyleSheet(longest > kWaitAlertMinutes ? "color: white; background: #C0392B; padding: 4px;"
                                                          : "padding: 4px;");
}

void DashboardPanel::checkReminders()
{
    const std::string today = utils::today();
    const int now = utils::minutesOfDay(utils::currentTime());
    QStringList due;
    for (const model::Appointment& a : manager_.getAppointments()) {
        if (a.getDate() != today || a.getStatus() != model::Appointment::Status::Pending) {
            continue;
        }
        const int minutesAway = utils::minutesOfDay(a.getTime()) - now;
        if (minutesAway >= 0 && minutesAway <= kReminderWindowMinutes) {
            QString text = qs(a.getPatientId());
            if (const auto* p = manager_.findPatient(a.getPatientId())) {
                text += " " + qs(p->getName());
            }
            due << text + " at " + qs(a.getTime());
        }
    }
    if (!due.isEmpty()) {
        emit statusMessage("Reminder: " + due.join("; "));
    }
}

void DashboardPanel::checkWaitingTimes()
{
    const int longest = manager_.longestWaitMinutes();
    if (longest > kWaitAlertMinutes) {
        const auto queue = manager_.queueSnapshot();
        emit statusMessage(QString("Alert: %1 has been waiting %2 min").arg(qs(queue.front().patientId)).arg(longest));
    }
    refresh();
}

} // namespace scapms::gui
