cmake_minimum_required(VERSION 3.20)
project(EmployeeManagement CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(employee STATIC src/employee.cpp)
target_include_directories(employee PUBLIC include)

add_executable(employee_app app/main.cpp)
target_link_libraries(employee_app PRIVATE employee)
