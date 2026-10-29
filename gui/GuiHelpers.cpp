#include "gui/GuiHelpers.h"

#include <QComboBox>
#include <QHeaderView>
#include <QTableWidget>

#include "controller/ClinicManager.h"
#include "model/PatientRecord.h"

namespace scapms::gui {

QColor categoryColour(const QString& category)
{
    if (category == "Emergency") {
        return QColor("#F8D7DA");
    }
    if (category == "Follow-Up") {
        return QColor("#D6E9F8");
    }
    return QColor(); // General: default background
}

QColor statusColour(const QString& status)
{
    if (status == "Pending") {
        return QColor("#FFF3CD");
    }
    if (status == "Completed") {
        return QColor("#D4EDDA");
    }
    if (status == "Cancelled") {
        return QColor("#E2E3E5");
    }
    return QColor();
}

void setupTable(QTableWidget* table, const QStringList& headers)
{
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    table->setAlternatingRowColors(false);
}

QStringList patientHeaders()
{
    return {"ID", "Name", "Age", "Contact", "Category", "Priority", "Details"};
}

void addPatientRow(QTableWidget* table, const model::PatientRecord& patient)
{
    // Category, Priority and Details are virtual calls on the base reference.
    const QStringList cells{qs(patient.getId()),       qs(patient.getName()),     QString::number(patient.getAge()),
                            qs(patient.getContact()),  qs(patient.getCategory()), qs(patient.getPriority()),
                            qs(patient.describe())};
    const QColor colour = categoryColour(qs(patient.getCategory()));
    const int row = table->rowCount();
    table->insertRow(row);
    for (int col = 0; col < cells.size(); ++col) {
        auto* item = new QTableWidgetItem(cells[col]);
        if (colour.isValid()) {
            item->setBackground(colour);
            item->setForeground(Qt::black);
        }
        table->setItem(row, col, item);
    }
    table->item(row, 0)->setData(Qt::UserRole, qs(patient.getId()));
}

void fillPatientTable(QTableWidget* table, const std::vector<model::PatientRecord*>& patients)
{
    table->setRowCount(0);
    for (const model::PatientRecord* p : patients) {
        addPatientRow(table, *p);
    }
    table->resizeColumnsToContents();
    table->horizontalHeader()->setStretchLastSection(true);
}

QString selectedRowId(const QTableWidget* table)
{
    const QList<QTableWidgetItem*> selected = table->selectedItems();
    if (selected.isEmpty()) {
        return {};
    }
    const QTableWidgetItem* first = table->item(selected.first()->row(), 0);
    return first ? first->data(Qt::UserRole).toString() : QString();
}

void fillPatientCombo(QComboBox* combo, const controller::ClinicManager& manager)
{
    const QString previous = comboId(combo);
    combo->blockSignals(true);
    combo->clear();
    for (const model::PatientRecord* p : manager.getPatients()) {
        combo->addItem(qs(p->getId()) + " - " + qs(p->getName()), qs(p->getId()));
    }
    const int index = combo->findData(previous);
    combo->setCurrentIndex(index >= 0 ? index : (combo->count() > 0 ? 0 : -1));
    combo->blockSignals(false);
}

QString comboId(const QComboBox* combo)
{
    return combo->currentData().toString();
}

} // namespace scapms::gui
