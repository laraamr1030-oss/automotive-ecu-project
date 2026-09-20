cmake_minimum_required(VERSION 3.20)
project(BankSystem CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(bank STATIC src/bank.cpp)
target_include_directories(bank PUBLIC include)

add_executable(bank_app app/main.cpp)
target_link_libraries(bank_app PRIVATE bank)
