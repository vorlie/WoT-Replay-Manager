cmake_minimum_required(VERSION 3.16)

project(WoT-Replay-Manager VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools)

set(TS_FILES WoT-Replay-Manager_en_US.ts)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    settingsdialog.cpp
    settingsdialog.h
    settingsdialog.ui
    ${TS_FILES}
)

qt_add_executable(WoT-Replay-Manager
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}

    json.hpp
    resources.qrc






)

qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

target_link_libraries(WoT-Replay-Manager PRIVATE Qt6::Widgets)

# Ensure the compiler sees generated ui_*.h files in the build dir
target_include_directories(WoT-Replay-Manager PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

set_target_properties(WoT-Replay-Manager PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS WoT-Replay-Manager
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(WoT-Replay-Manager)
