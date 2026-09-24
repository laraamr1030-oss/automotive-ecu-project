cmake_minimum_required(VERSION 3.10)
project(bms_ecu)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(bms_ecu
    main.cpp
    bms_ecu.cpp
)

target_include_directories(bms_ecu PRIVATE .)
