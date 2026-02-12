#include <iostream>
#include "Tasks/datastructures.cpp"

using namespace std;

LearnerLinkedList learnerLL;

void LearnerLinkedList_Test();

int main() {
    int choice;

    // Initializing Learner Linked List
    LearnerLinkedList_Test();


    // while(true) {

    //     cout << "\n===== PLAPS MAIN MENU =====\n";
    //     cout << "1. Session Management (Task 1)\n";
    //     cout << "2. Activity Navigation (Task 2)\n";
    //     cout << "3. Activity Logging (Task 3)\n";
    //     cout << "4. Risk Engine (Task 4)\n";
    //     cout << "5. Exit\n";
    //     cout << "Choice: ";
        
    //     cin >> choice;
        
    //     switch(choice) {
    //         case 5: return 0;
    //     }
    // }
}

void LearnerLinkedList_Test() {
    learnerLL.addLearner(createLearner("Ayman"));
    learnerLL.addLearner(createLearner("Abdul"));

    learnerLL.displayAllLearners();
}