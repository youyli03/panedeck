cmake_minimum_required(VERSION 3.21)

project(PaneDeck VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

qt_add_executable(PaneDeck WIN32
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/DashboardView.cpp
    src/DashboardView.h
    src/GroupWidget.cpp
    src/GroupWidget.h
    src/WindowEnumerator.cpp
    src/WindowEnumerator.h
    src/WindowInfo.h
    src/PathResolver.cpp
    src/PathResolver.h
    src/ActivationService.cpp
    src/ActivationService.h
    src/WinEventWatcher.cpp
    src/WinEventWatcher.h
    src/SettingsStore.cpp
    src/SettingsStore.h
)

target_include_directories(PaneDeck PRIVATE src)

target_compile_definitions(PaneDeck PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_link_libraries(PaneDeck PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ole32
    oleaut32
    shlwapi
    dwmapi
    user32
    gdi32
    shell32
)


