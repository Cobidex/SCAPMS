# SCAPMS

Smart Clinic Appointment & Patient Management System. A Qt 6 Widgets desktop
app for registering patients, booking appointments, running a waiting queue,
searching and sorting records, undoing admin operations and producing simple
reports. Data is saved as CSV.

COS202 Programming II lab project.

## Layout

| Folder | Contents |
|---|---|
| `model/` | `PatientRecord` (abstract), `Appointable` (interface), the three patient types, `Appointment`, `ClinicDatabase` |
| `controller/` | `ClinicManager` (queue, undo stack, recent cache, reports), `AppointmentController`, `SearchEngine`, `SortEngine` |
| `utils/` | Custom exceptions, `IDGenerator`, `FileHandler` (CSV), text/date helpers |
| `gui/` | `MainWindow` and one panel per tab |
| `data/` | Sample `patients.csv` and `appointments.csv` |
| `tests/` | Console test program for the non-GUI layers |

`model/`, `controller/` and `utils/` are plain C++17 with no Qt, so the logic
can be tested without the GUI.

## Build

Needs `build-essential cmake ninja-build qt6-base-dev libgl1-mesa-dev` on Ubuntu.

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

Run from the project root so the app finds `data/`:

```bash
./build/SCAPMS
./build/scapms_tests
```

## Keyboard shortcuts

| Shortcut | Action |
|---|---|
| Ctrl+N | New (clear all records) |
| Ctrl+O | Load patients and appointments |
| Ctrl+S | Save patients and appointments |
| Ctrl+Z | Undo last administrative operation |
| Ctrl+F | Find patient (Search & Sort tab) |
| Ctrl+Q | Exit |
