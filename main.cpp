#include <iostream>

using namespace std;

int main() {
    int choice;

    while(true) {

        cout << "\n===== PLAPS MAIN MENU =====\n";
        cout << "1. Session Management (Task 1)\n";
        cout << "2. Activity Navigation (Task 2)\n";
        cout << "3. Activity Logging (Task 3)\n";
        cout << "4. Risk Engine (Task 4)\n";
        cout << "5. Exit\n";
        cout << "Choice: ";
        
        cin >> choice;
        
        switch(choice) {
            case 5: return 0;
        }
    }
}