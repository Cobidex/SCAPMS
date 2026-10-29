#include "gui/QueuePanel.h"

#include <QComboBox>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <ctime>

#include "controller/ClinicManager.h"
#include "gui/GuiHelpers.h"

namespace scapms::gui {

namespace {
int minutesSince(std::time_t then)
{
    const double seconds = std::difftime(std::time(nullptr), then);
    return seconds <= 0 ? 0 : static_cast<int>(seconds / 60);
}
} // namespace

QueuePanel::QueuePanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager)
{
    patientCombo_ = new QComboBox;
    patientCombo_->setToolTip("Patient who has arrived");
    addButton_ = new QPushButton("Add to Queue");
    addButton_->setToolTip("Add the patient to the end of the waiting queue");
    auto* top = new QHBoxLayout;
    top->addWidget(new QLabel("Patient:"));
    top->addWidget(patientCombo_, 1);
    top->addWidget(addButton_);

    list_ = new QListWidget;
    list_->setToolTip("Waiting patients in arrival order. Emergency patients are shown in red.");

    callNextButton_ = new QPushButton("Call Next Patient");
    callNextButton_->setToolTip("Serve the patient at the front of the queue");
    callNextButton_->setMinimumHeight(48);
    QFont big = callNextButton_->font();
    big.setPointSize(big.pointSize() + 4);
    big.setBold(true);
    callNextButton_->setFont(big);

    nowServing_ = new QLabel("Now serving: nobody");
    QFont servingFont = nowServing_->font();
    servingFont.setPointSize(servingFont.pointSize() + 2);
    nowServing_->setFont(servingFont);
    averageWait_ = new QLabel;

    auto* root = new QVBoxLayout(this);
    root->addLayout(top);
    root->addWidget(list_, 1);
    root->addWidget(callNextButton_);
    root->addWidget(nowServing_);
    root->addWidget(averageWait_);

    // Keeps the "waiting N min" text current.
    waitClock_ = new QTimer(this);
    waitClock_->setInterval(30 * 1000);
    connect(waitClock_, &QTimer::timeout, this, &QueuePanel::refresh);
    waitClock_->start();

    connect(addButton_, &QPushButton::clicked, this, &QueuePanel::onAdd);
    connect(callNextButton_, &QPushButton::clicked, this, &QueuePanel::onCallNext);
}

void QueuePanel::refresh()
{
    fillPatientCombo(patientCombo_, manager_);
    list_->clear();
    int position = 1;
    for (const controller::QueueEntry& entry : manager_.queueSnapshot()) {
        const model::PatientRecord* p = manager_.findPatient(entry.patientId);
        QString text = QString("%1. %2").arg(position++).arg(qs(entry.patientId));
        if (p) {
            text += " " + qs(p->getName());
        }
        text += QString(" (waiting %1 min)").arg(minutesSince(entry.joinedAt));
        auto* item = new QListWidgetItem(text, list_);
        if (p && p->getCategory() == "Emergency") {
            item->setBackground(categoryColour("Emergency"));
            item->setForeground(QColor("#842029"));
            QFont bold = item->font();
            bold.setBold(true);
            item->setFont(bold);
        }
    }
    averageWait_->setText(QString("Average wait: %1 min | Longest wait: %2 min")
                              .arg(manager_.averageWaitMinutes())
                              .arg(manager_.longestWaitMinutes()));
}

void QueuePanel::onAdd()
{
    const QString id = comboId(patientCombo_);
    if (id.isEmpty()) {
        return;
    }
    if (guarded(this, [this, &id] { manager_.enqueue(id.toStdString()); })) {
        emit dataChanged();
        emit statusMessage(id + " added to the waiting queue");
    }
}

void QueuePanel::onCallNext()
{
    controller::QueueEntry entry;
    if (!guarded(this, [this, &entry] { entry = manager_.callNext(); })) {
        return;
    }
    QString text = "Now serving: " + qs(entry.patientId);
    if (const auto* p = manager_.findPatient(entry.patientId)) {
        text += " " + qs(p->getName());
    }
    text += QString(" (waited %1 min)").arg(minutesSince(entry.joinedAt));
    nowServing_->setText(text);
    emit dataChanged();
    emit statusMessage(text);
}

} // namespace scapms::gui
