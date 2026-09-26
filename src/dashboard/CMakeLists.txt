cmake_minimum_required(VERSION 3.16)
project(dashboard LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt5/Qt6 modules
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network Svg)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network Svg)

set(SOURCES
    main.cpp
    MainWindow.cpp
    SpeedometerWidget.cpp
    CANWorker.cpp
    CheckEngineIcon.cpp
)

set(HEADERS
    MainWindow.h
    SpeedometerWidget.h
    CANWorker.h
    CheckEngineIcon.h
)

add_executable(dashboard ${SOURCES} ${HEADERS})

target_link_libraries(dashboard PRIVATE 
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Svg
)
