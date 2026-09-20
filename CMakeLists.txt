cmake_minimum_required(VERSION 3.16)
project(AutomotiveECUSystem CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include Subdirectories
add_subdirectory(src/powertrain/engine_ecu)
add_subdirectory(src/powertrain/transmission_ecu)
add_subdirectory(src/powertrain/body_control_module)
