#include "bank.h"
#include <iostream>

int main() {
    double initial, dep, with;
    if (std::cin >> initial >> dep >> with) {
        setInitialBalance(initial);
        
        std::cout << "Initial Balance : " << initial << "\n";
        
        deposit(dep);
        std::cout << "Deposit         : " << dep << "\n";
        
        if (withdraw(with)) {
            std::cout << "Withdrawal      : " << with << "\n";
        } else {
            std::cout << "Withdrawal Failed : " << with << "\n";
        }
        
        std::cout << "Final Balance   : " << getBalance() << "\n";
    }
    return 0;
}
