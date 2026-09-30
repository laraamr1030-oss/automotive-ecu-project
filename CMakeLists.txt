cmake_minimum_required(VERSION 3.16)
project(AutomotiveECUSystem CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Every executable (engine_ecu, transmission_ecu, bcm, bms_ecu, dashboard)
# lands in build/bin so run_demo.sh knows where to find them.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ECUs (each is its own process, each owns its own CAN ID(s))
add_subdirectory(src/powertrain/engine_ecu)
add_subdirectory(src/powertrain/transmission_ecu)
add_subdirectory(src/powertrain/body_control_module)
add_subdirectory(src/powertrain/bms_ecu)          # was missing from the build

# Qt dashboard: only built when Qt is installed, so the ECUs still build without it
find_package(QT NAMES Qt6 Qt5 QUIET COMPONENTS Widgets Svg)
if(QT_FOUND)
    add_subdirectory(src/dashboard)
else()
    message(WARNING "Qt5/Qt6 (Widgets, Svg) not found - dashboard will NOT be built")
endif()
