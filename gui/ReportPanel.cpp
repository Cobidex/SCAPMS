#include "gui/ReportPanel.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTextStream>
#include <QVBoxLayout>

#include "controller/ClinicManager.h"
#include "gui/GuiHelpers.h"

namespace scapms::gui {

namespace {
enum Report { MostAttended = 0, Pending = 1, Categories = 2, Daily = 3 };
}

ReportPanel::ReportPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager)
{
    reportCombo_ = new QComboBox;
    reportCombo_->addItems({"Most Frequently Attended Patients", "Patients with Pending Appointments",
                            "Patient Category Distribution", "Daily Appointment Summary"});
    reportCombo_->setToolTip("Report to generate");
    dateLabel_ = new QLabel("Date:");
    dateEdit_ = new QDateEdit(QDate::currentDate());
    dateEdit_->setCalendarPopup(true);
    dateEdit_->setDisplayFormat("yyyy-MM-dd");
    dateEdit_->setToolTip("Day to summarise");
    generateButton_ = new QPushButton("Generate");
    generateButton_->setToolTip("Build the selected report from the current data");
    exportButton_ = new QPushButton("Export Report…");
    exportButton_->setToolTip("Save the report shown below as .csv or .txt");

    auto* top = new QHBoxLayout;
    top->addWidget(reportCombo_, 1);
    top->addWidget(dateLabel_);
    top->addWidget(dateEdit_);
    top->addWidget(generateButton_);
    top->addWidget(exportButton_);

    table_ = new QTableWidget;
    setupTable(table_, {});
    summary_ = new QLabel;

    auto* root = new QVBoxLayout(this);
    root->addLayout(top);
    root->addWidget(table_, 1);
    root->addWidget(summary_);

    connect(reportCombo_, &QComboBox::currentIndexChanged, this, &ReportPanel::onReportChanged);
    connect(generateButton_, &QPushButton::clicked, this, &ReportPanel::onGenerate);
    connect(exportButton_, &QPushButton::clicked, this, &ReportPanel::onExport);
    onReportChanged(reportCombo_->currentIndex());
}

void ReportPanel::refresh()
{
    // Reports hold raw pointers into the data, so clear them when it changes.
    table_->setRowCount(0);
    table_->setColumnCount(0);
    summary_->setText("Click Generate to build the report from the current data.");
}

void ReportPanel::onReportChanged(int index)
{
    // The date picker only applies to the daily summary.
    dateLabel_->setVisible(index == Daily);
    dateEdit_->setVisible(index == Daily);
    refresh();
}

void ReportPanel::showRows(const QStringList& headers, const QList<QStringList>& rows)
{
    table_->setRowCount(0);
    table_->setColumnCount(headers.size());
    table_->setHorizontalHeaderLabels(headers);
    for (const QStringList& cells : rows) {
        const int row = table_->rowCount();
        table_->insertRow(row);
        for (int col = 0; col < cells.size(); ++col) {
            table_->setItem(row, col, new QTableWidgetItem(cells[col]));
        }
    }
    table_->resizeColumnsToContents();
}

void ReportPanel::onGenerate()
{
    guarded(this, [this] {
        QList<QStringList> rows;
        switch (reportCombo_->currentIndex()) {
        case MostAttended:
            for (const auto& r : manager_.mostFrequentlyAttended()) {
                rows.append({qs(r.patient->getId()), qs(r.patient->getName()), qs(r.patient->getCategory()),
                             QString::number(r.completed)});
            }
            showRows({"ID", "Name", "Category", "Completed appointments"}, rows);
            summary_->setText("Top patients by completed appointments (ties included).");
            break;
        case Pending:
            for (const auto& r : manager_.pendingAppointments()) {
                rows.append({qs(r.patient->getId()), qs(r.patient->getName()), QString::number(r.pending),
                             qs(r.next)});
            }
            showRows({"ID", "Name", "Pending", "Next appointment"}, rows);
            summary_->setText(QString("%1 patient(s) with pending appointments.").arg(rows.size()));
            break;
        case Categories: {
            int total = 0;
            for (const auto& r : manager_.categoryDistribution()) {
                rows.append({qs(r.category), QString::number(r.count), QString::number(r.percent, 'f', 1) + "%"});
                total += r.count;
            }
            showRows({"Category", "Patients", "Share"}, rows);
            summary_->setText(QString("%1 patient(s) in total.").arg(total));
            break;
        }
        case Daily: {
            const auto s = manager_.dailySummary(dateEdit_->date().toString("yyyy-MM-dd").toStdString());
            rows.append({"Total", QString::number(s.total)});
            rows.append({"Completed", QString::number(s.completed)});
            rows.append({"Pending", QString::number(s.pending)});
            rows.append({"Cancelled", QString::number(s.cancelled)});
            showRows({"Status", "Appointments"}, rows);
            summary_->setText("Appointments on " + qs(s.date) + ".");
            break;
        }
        }
        emit statusMessage(reportCombo_->currentText() + " generated");
    });
}

void ReportPanel::onExport()
{
    if (table_->columnCount() == 0) {
        QMessageBox::information(this, "Export report", "Generate a report first.");
        return;
    }
    const QString path = QFileDialog::getSaveFileName(this, "Export report", "report.csv",
                                                      "CSV files (*.csv);;Text files (*.txt)");
    if (path.isEmpty()) {
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "File error", "Could not write file: " + path);
        return;
    }
    const bool csv = !path.endsWith(".txt", Qt::CaseInsensitive);
    const QString separator = csv ? "," : "\t";
    QTextStream out(&file);
    if (!csv) {
        out << reportCombo_->currentText() << "\n" << summary_->text() << "\n\n";
    }
    QStringList headers;
    for (int col = 0; col < table_->columnCount(); ++col) {
        headers << table_->horizontalHeaderItem(col)->text();
    }
    out << headers.join(separator) << "\n";
    for (int row = 0; row < table_->rowCount(); ++row) {
        QStringList cells;
        for (int col = 0; col < table_->columnCount(); ++col) {
            const QTableWidgetItem* item = table_->item(row, col);
            cells << (item ? item->text() : QString());
        }
        out << cells.join(separator) << "\n";
    }
    emit statusMessage("Report exported to " + path);
}

} // namespace scapms::gui
