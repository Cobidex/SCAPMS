#pragma once

#include <QColor>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <exception>
#include <string>
#include <vector>

#include "utils/Exceptions.h"

class QComboBox;
class QTableWidget;
class QWidget;

namespace scapms::model {
class PatientRecord;
}
namespace scapms::controller {
class ClinicManager;
}

namespace scapms::gui {

inline QString qs(const std::string& s) { return QString::fromStdString(s); }

QColor categoryColour(const QString& category);
QColor statusColour(const QString& status);

// Read-only, whole-row selection, last column stretched. Sorting by
// clicking headers stays off: sorting is done by our own algorithms.
void setupTable(QTableWidget* table, const QStringList& headers);

QStringList patientHeaders();
void addPatientRow(QTableWidget* table, const model::PatientRecord& patient);
void fillPatientTable(QTableWidget* table, const std::vector<model::PatientRecord*>& patients);
QString selectedRowId(const QTableWidget* table);

// Fills a combo with "P0001 - Ada Okafor" items (ID as item data) and keeps
// the current selection when it still exists.
void fillPatientCombo(QComboBox* combo, const controller::ClinicManager& manager);
QString comboId(const QComboBox* combo);

// Runs `action`; any exception becomes a dialog so the app keeps running.
template <typename Action>
bool guarded(QWidget* parent, Action&& action)
{
    try {
        action();
        return true;
    } catch (const utils::ValidationException& e) {
        QMessageBox::warning(parent, "Invalid input", e.what());
    } catch (const utils::DuplicateIdException& e) {
        QMessageBox::warning(parent, "Duplicate ID", e.what());
    } catch (const utils::NotFoundException& e) {
        QMessageBox::warning(parent, "Not found", e.what());
    } catch (const utils::FileException& e) {
        QMessageBox::critical(parent, "File error", e.what());
    } catch (const std::exception& e) {
        QMessageBox::critical(parent, "Error", e.what());
    }
    return false;
}

} // namespace scapms::gui
