#include "gui/MainWindow.h"

#include <QAction>
#include <QCloseEvent>
#include <QFileDialog>
#include <QKeySequence>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>

#include "gui/AdminPanel.h"
#include "gui/AppointmentPanel.h"
#include "gui/DashboardPanel.h"
#include "gui/GuiHelpers.h"
#include "gui/PatientPanel.h"
#include "gui/QueuePanel.h"
#include "gui/ReportPanel.h"
#include "gui/SearchPanel.h"

namespace scapms::gui {

namespace {
const char* const kCsvFilter = "CSV files (*.csv);;All files (*)";
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("SCAPMS - Smart Clinic Appointment & Patient Management System");
    resize(1200, 760);

    dashboard_ = new DashboardPanel(manager_);
    patients_ = new PatientPanel(manager_);
    appointments_ = new AppointmentPanel(manager_);
    queue_ = new QueuePanel(manager_);
    search_ = new SearchPanel(manager_);
    reports_ = new ReportPanel(manager_);
    admin_ = new AdminPanel(manager_);

    tabs_ = new QTabWidget;
    tabs_->addTab(dashboard_, "Dashboard");
    tabs_->addTab(patients_, "Patient Records");
    tabs_->addTab(appointments_, "Appointments");
    tabs_->addTab(queue_, "Waiting Queue");
    tabs_->addTab(search_, "Search && Sort");
    tabs_->addTab(reports_, "Reports");
    tabs_->addTab(admin_, "Admin");
    setCentralWidget(tabs_);

    for (QObject* panel : std::initializer_list<QObject*>{dashboard_, patients_, appointments_, queue_, search_,
                                                          reports_, admin_}) {
        connectPanel(panel);
    }
    connect(patients_, &PatientPanel::patientAccessed, dashboard_, &DashboardPanel::refresh);
    connect(search_, &SearchPanel::patientAccessed, dashboard_, &DashboardPanel::refresh);

    countsLabel_ = new QLabel;
    statusBar()->addPermanentWidget(countsLabel_);

    createMenus();
    refreshAll();
    // Load after the window is shown so any warning appears on top of it.
    QTimer::singleShot(0, this, &MainWindow::loadStartupData);
}

void MainWindow::connectPanel(QObject* panel)
{
    // Old-style string connections let one helper wire every panel type.
    if (panel->metaObject()->indexOfSignal("dataChanged()") >= 0) {
        connect(panel, SIGNAL(dataChanged()), this, SLOT(onDataChanged()));
    }
    connect(panel, SIGNAL(statusMessage(QString)), this, SLOT(showStatus(QString)));
}

void MainWindow::createMenus()
{
    QMenu* file = menuBar()->addMenu("&File");
    QAction* newAction = file->addAction("&New", this, &MainWindow::onNew);
    newAction->setShortcut(QKeySequence::New);
    newAction->setToolTip("Start with empty records");
    QAction* loadAction = file->addAction("&Load…", this, &MainWindow::onLoad);
    loadAction->setShortcut(QKeySequence::Open);
    QAction* saveAction = file->addAction("&Save…", this, &MainWindow::onSave);
    saveAction->setShortcut(QKeySequence::Save);
    file->addSeparator();
    QAction* exitAction = file->addAction("E&xit", this, &QWidget::close);
    exitAction->setShortcut(QKeySequence::Quit);

    QMenu* edit = menuBar()->addMenu("&Edit");
    QAction* undoAction = edit->addAction("&Undo Last Operation", admin_, &AdminPanel::undoLast);
    undoAction->setShortcut(QKeySequence::Undo);

    QMenu* searchMenu = menuBar()->addMenu("&Search");
    QAction* findAction = searchMenu->addAction("&Find Patient…", this, &MainWindow::onFind);
    findAction->setShortcut(QKeySequence::Find);
}

void MainWindow::loadStartupData()
{
    try {
        manager_.load(patientsPath_.toStdString(), appointmentsPath_.toStdString());
        showStatus(QString("Loaded %1 patients and %2 appointments")
                       .arg(manager_.getPatients().size())
                       .arg(manager_.getAppointments().size()));
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Startup",
                             QString("Could not load the saved data, starting empty.\n\n%1").arg(e.what()));
    }
    dirty_ = false;
    refreshAll();
}

void MainWindow::onNew()
{
    if (QMessageBox::question(this, "New", "Clear all patients, appointments and the waiting queue?") !=
        QMessageBox::Yes) {
        return;
    }
    manager_.clearAll();
    dirty_ = false;
    refreshAll();
    showStatus("Started with empty records");
}

void MainWindow::onLoad()
{
    const QString patientsPath =
        QFileDialog::getOpenFileName(this, "Load patients file", patientsPath_, kCsvFilter);
    if (patientsPath.isEmpty()) {
        return;
    }
    const QString appointmentsPath =
        QFileDialog::getOpenFileName(this, "Load appointments file", appointmentsPath_, kCsvFilter);
    if (appointmentsPath.isEmpty()) {
        return;
    }
    // On failure the manager keeps the current data untouched.
    if (guarded(this, [&] { manager_.load(patientsPath.toStdString(), appointmentsPath.toStdString()); })) {
        patientsPath_ = patientsPath;
        appointmentsPath_ = appointmentsPath;
        dirty_ = false;
        refreshAll();
        showStatus("Loaded " + patientsPath + " and " + appointmentsPath);
    }
}

void MainWindow::onSave()
{
    const QString patientsPath =
        QFileDialog::getSaveFileName(this, "Save patients file", patientsPath_, kCsvFilter);
    if (patientsPath.isEmpty()) {
        return;
    }
    const QString appointmentsPath =
        QFileDialog::getSaveFileName(this, "Save appointments file", appointmentsPath_, kCsvFilter);
    if (appointmentsPath.isEmpty()) {
        return;
    }
    if (saveTo(patientsPath, appointmentsPath)) {
        patientsPath_ = patientsPath;
        appointmentsPath_ = appointmentsPath;
    }
}

bool MainWindow::saveTo(const QString& patientsPath, const QString& appointmentsPath)
{
    if (!guarded(this, [&] { manager_.save(patientsPath.toStdString(), appointmentsPath.toStdString()); })) {
        return false;
    }
    dirty_ = false;
    showStatus("Saved to " + patientsPath + " and " + appointmentsPath);
    return true;
}

void MainWindow::onFind()
{
    tabs_->setCurrentWidget(search_);
    search_->focusSearch();
}

void MainWindow::onDataChanged()
{
    dirty_ = true;
    refreshAll();
}

void MainWindow::refreshAll()
{
    dashboard_->refresh();
    patients_->refresh();
    appointments_->refresh();
    queue_->refresh();
    search_->refresh();
    reports_->refresh();
    admin_->refresh();
    updateCounts();
}

void MainWindow::updateCounts()
{
    countsLabel_->setText(
        QString("Patients: %1 | Queue: %2").arg(manager_.getPatients().size()).arg(manager_.queueLength()));
}

void MainWindow::showStatus(const QString& text)
{
    statusBar()->showMessage(text, 5000);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (!dirty_) {
        event->accept();
        return;
    }
    const auto answer = QMessageBox::question(this, "Exit", "Save changes before exiting?",
                                              QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Cancel) {
        event->ignore();
        return;
    }
    if (answer == QMessageBox::Save && !saveTo(patientsPath_, appointmentsPath_)) {
        event->ignore();
        return;
    }
    event->accept();
}

} // namespace scapms::gui
