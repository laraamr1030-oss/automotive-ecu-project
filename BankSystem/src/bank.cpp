#include "bank.h"

static double g_balance = 0.0;

void setInitialBalance(double amount) {
    g_balance = amount;
}

void deposit(double amount) {
    g_balance += amount;
}

bool withdraw(double amount) {
    if (amount <= g_balance) {
        g_balance -= amount;
        return true;
    }
    return false;
}

double getBalance() {
    return g_balance;
}
