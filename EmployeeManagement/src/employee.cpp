#include "employee.h"
#include <iostream>

static std::string g_name;
static double g_monthlySalary = 0.0;

void setEmployee(std::string name, double monthlySalary) {
    g_name = name;
    g_monthlySalary = monthlySalary;
}

double annualSalary() {
    return g_monthlySalary * 12.0;
}

void printEmployee() {
    std::cout << "Employee Name : " << g_name << "\n";
    std::cout << "Monthly Salary: " << g_monthlySalary << "\n";
    std::cout << "Annual Salary : " << annualSalary() << "\n";
}
