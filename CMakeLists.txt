cmake_minimum_required(VERSION 3.16)
project(SCAPMS VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Plain C++ core: model, controller, utils (no Qt)
add_library(scapms_core STATIC
    model/PatientRecord.cpp
    model/GeneralPatient.cpp
    model/EmergencyPatient.cpp
    model/FollowUpPatient.cpp
    model/Appointment.cpp
    model/ClinicDatabase.cpp
    controller/ClinicManager.cpp
    controller/AppointmentController.cpp
    controller/SearchEngine.cpp
    controller/SortEngine.cpp
    utils/IDGenerator.cpp
    utils/FileHandler.cpp
)
target_include_directories(scapms_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Console test program for Phases 2-4
add_executable(scapms_tests tests/console_test.cpp)
target_link_libraries(scapms_tests PRIVATE scapms_core)

# Qt GUI application (Phase 5)
qt_add_executable(SCAPMS
    main.cpp
    gui/GuiHelpers.h        gui/GuiHelpers.cpp
    gui/MainWindow.h        gui/MainWindow.cpp
    gui/DashboardPanel.h    gui/DashboardPanel.cpp
    gui/PatientPanel.h      gui/PatientPanel.cpp
    gui/AppointmentPanel.h  gui/AppointmentPanel.cpp
    gui/QueuePanel.h        gui/QueuePanel.cpp
    gui/SearchPanel.h       gui/SearchPanel.cpp
    gui/AdminPanel.h        gui/AdminPanel.cpp
    gui/ReportPanel.h       gui/ReportPanel.cpp
)
target_link_libraries(SCAPMS PRIVATE scapms_core Qt6::Widgets)
