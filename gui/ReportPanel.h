#pragma once

#include <QWidget>

class QComboBox;
class QDateEdit;
class QLabel;
class QPushButton;
class QTableWidget;

namespace scapms::controller {
class ClinicManager;
}

namespace scapms::gui {

// Reports tab: four summary reports with export to .csv or .txt.
class ReportPanel : public QWidget {
    Q_OBJECT

public:
    explicit ReportPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

public slots:
    void refresh();

signals:
    void statusMessage(const QString& text);

private slots:
    void onReportChanged(int index);
    void onGenerate();
    void onExport();

private:
    void showRows(const QStringList& headers, const QList<QStringList>& rows);

    controller::ClinicManager& manager_;
    QComboBox* reportCombo_ = nullptr;
    QLabel* dateLabel_ = nullptr;
    QDateEdit* dateEdit_ = nullptr;
    QPushButton* generateButton_ = nullptr;
    QPushButton* exportButton_ = nullptr;
    QTableWidget* table_ = nullptr;
    QLabel* summary_ = nullptr;
};

} // namespace scapms::gui
