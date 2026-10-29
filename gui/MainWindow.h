#pragma once

#include <QMainWindow>

#include "controller/ClinicManager.h"

class QLabel;
class QTabWidget;

namespace scapms::gui {

class AdminPanel;
class AppointmentPanel;
class DashboardPanel;
class PatientPanel;
class QueuePanel;
class ReportPanel;
class SearchPanel;

// Owns the ClinicManager and every panel. Panels never talk to each other:
// they emit dataChanged() and the window refreshes them all.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onNew();
    void onLoad();
    void onSave();
    void onFind();
    void onDataChanged();
    void refreshAll();
    void showStatus(const QString& text);
    void loadStartupData();

private:
    void createMenus();
    void connectPanel(QObject* panel);
    bool saveTo(const QString& patientsPath, const QString& appointmentsPath);
    void updateCounts();

    controller::ClinicManager manager_;
    QString patientsPath_ = "data/patients.csv";
    QString appointmentsPath_ = "data/appointments.csv";
    bool dirty_ = false;

    QTabWidget* tabs_ = nullptr;
    DashboardPanel* dashboard_ = nullptr;
    PatientPanel* patients_ = nullptr;
    AppointmentPanel* appointments_ = nullptr;
    QueuePanel* queue_ = nullptr;
    SearchPanel* search_ = nullptr;
    ReportPanel* reports_ = nullptr;
    AdminPanel* admin_ = nullptr;
    QLabel* countsLabel_ = nullptr;
};

} // namespace scapms::gui
