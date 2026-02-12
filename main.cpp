#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "Tasks/datastructures.cpp"

using namespace std;

LearnerLinkedList learnerLL;

void LearnerLinkedList_Test();
void saveLearnersToCSV();
void loadLearnersFromCSV();
int getNextIDFromCSV();

int main() {
    int choice;

    // Initializing Learner Linked List from CSV
    loadLearnersFromCSV();
    nextID = getNextIDFromCSV();

    cout << "Loaded " << learnerLL.getCount() << " learners from students.csv" << endl;
    cout << "Next ID will be: " << nextID << endl;

    LearnerLinkedList_Test();


    // ========== MENU ==========
    while(true) {
        cout << "\n===== PLAPS MAIN MENU =====" << endl;
        cout << "1. Session Management (Task 1)" << endl;
        cout << "2. Activity Navigation (Task 2)" << endl;
        cout << "3. Activity Logging (Task 3)" << endl;
        cout << "4. Risk Engine (Task 4)" << endl;
        cout << "5. Save & Exit" << endl;
        cout << "Choice: ";
        
        cin >> choice;
        
        switch(choice) {
            case 5: 
                saveLearnersToCSV();
                cout << "Data saved to students.csv. Exiting..." << endl;
                return 0;
        }
    }
}

void saveLearnersToCSV() {
    ofstream file("Dataset/students.csv");
    if (!file.is_open()) {
        cout << "Error: Could not open students.csv for writing" << endl;
        return;
    }

    // Write header
    file << "id,name,currentSessionID,currentActivity,isActive,completedSessions" << endl;

    // Write all learners
    Learner* temp = learnerLL.getHead();
    while (temp != NULL) {
        file << temp->id << ","
             << temp->name << ","
             << temp->currentSessionID << ","
             << temp->currentActivity << ","
             << (temp->isActive ? 1 : 0) << ",";
        
        // Write completed sessions array
        for (int i = 0; i < 5; i++) {
            file << temp->completedSessions[i];
            if (i < 4) file << ";";
        }
        file << endl;
        
        temp = temp->next;
    }

    file.close();
}

void loadLearnersFromCSV() {
    ifstream file("Dataset/students.csv");
    if (!file.is_open()) {
        // File doesn't exist yet, that's okay
        return;
    }

    string line;
    bool isHeader = true;
    
    while (getline(file, line)) {
        if (isHeader) {
            isHeader = false;
            continue;
        }

        stringstream ss(line);
        string idStr, name, sessionIdStr, activityStr, isActiveStr, completedSessionsStr;

        getline(ss, idStr, ',');
        getline(ss, name, ',');
        getline(ss, sessionIdStr, ',');
        getline(ss, activityStr, ',');
        getline(ss, isActiveStr, ',');
        getline(ss, completedSessionsStr, ',');

        if (idStr.empty()) continue;

        Learner* newL = new Learner;
        newL->id = stoi(idStr);
        newL->name = name;
        newL->currentSessionID = stoi(sessionIdStr);
        newL->currentActivity = stoi(activityStr);
        newL->isActive = (isActiveStr == "1");
        newL->next = NULL;

        // Parse completed sessions
        stringstream csStream(completedSessionsStr);
        string session;
        int i = 0;
        while (getline(csStream, session, ';') && i < 5) {
            newL->completedSessions[i++] = stoi(session);
        }

        learnerLL.addLearner(newL);
    }

    file.close();
}

int getNextIDFromCSV() {
    ifstream file("Dataset/students.csv");
    if (!file.is_open()) {
        return 1;
    }

    string line;
    int maxID = 0;
    bool isHeader = true;

    while (getline(file, line)) {
        if (isHeader) {
            isHeader = false;
            continue;
        }

        stringstream ss(line);
        string idStr;
        getline(ss, idStr, ',');

        if (!idStr.empty()) {
            int id = stoi(idStr);
            if (id > maxID) {
                maxID = id;
            }
        }
    }

    file.close();
    return maxID + 1;
}

void LearnerLinkedList_Test() {
    // learnerLL.addLearner(createLearner("Ayman"));
    // learnerLL.addLearner(createLearner("Abdul"));

    learnerLL.displayAllLearners();
}