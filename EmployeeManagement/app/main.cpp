#include "employee.h"
#include <iostream>

int main() {
    std::string name;
    double salary;
    if (std::cin >> name >> salary) {
        setEmployee(name, salary);
        printEmployee();
    }
    return 0;
}
