cmake_minimum_required(VERSION 3.16)
project(EngineECU LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(engine_ecu
    main.cpp
    engine_ecu.cpp
    uds_handler.cpp      # was missing from the old build file
)
target_include_directories(engine_ecu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(engine_ecu PRIVATE -Wall -Wextra)
