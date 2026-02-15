#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <limits>
#include "Tasks/datastructures.h"
#include "Tasks/task2.cpp"

using namespace std;

int nextID = 1;
LearnerLinkedList learnerLL;

void LearnerLinkedList_Test();
void saveLearnersToCSV();
void loadLearnersFromCSV();
int getNextIDFromCSV();
void saveQueuesToCSV();
void loadQueuesFromCSV();

int main() {
    // Clear terminal at startup
    system("cls");
    
    int choice;

    // Initializing Learner Linked List from CSV
    loadLearnersFromCSV();
    loadQueuesFromCSV();
    nextID = getNextIDFromCSV();

    cout << "Loaded " << learnerLL.getCount() << " learners from students.csv" << endl;
    cout << "Next ID will be: " << nextID << endl;

    // ========== MENU ==========
    while(true) {
        cout << "\n===== PLAPS MAIN MENU =====" << endl;
        cout << "1. Session Management (Task 1)" << endl;
        cout << "2. Activity Navigation (Task 2)" << endl;
        cout << "3. Activity Logging (Task 3)" << endl;
        cout << "4. Risk Engine (Task 4)" << endl;
        cout << "5. Save & Exit" << endl;
        cout << "Choice: ";
        
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a number." << endl;
            continue;
        }
        system("cls");
        
        switch(choice) {
            case 2: 
                cout << "Initializing Activities" << endl;
                InitializingSessions();
                // Clear screen after returning from submenu
                system("cls");
                break;
            case 5: 
                saveLearnersToCSV();
                saveQueuesToCSV();
                cout << "Data saved. Exiting..." << endl;
                return 0;
            default:
                cout << "Invalid choice. Please try again." << endl;
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

        // Initialize completed sessions array to 0 first
        for (int i = 0; i < 5; i++) {
            newL->completedSessions[i] = 0;
        }

        // Parse completed sessions from CSV
        stringstream csStream(completedSessionsStr);
        string session;
        int i = 0;
        while (getline(csStream, session, ';') && i < 5) {
            if (!session.empty()) {
                newL->completedSessions[i] = stoi(session);
            }
            i++;
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
    string lastLine;
    bool isHeader = true;

    while (getline(file, line)) {
        if (isHeader) {
            isHeader = false;
            continue;
        }
        if (!line.empty()) {
            lastLine = line;
        }
    }

    file.close();

    if (lastLine.empty()) {
        return 1;
    }

    stringstream ss(lastLine);
    string idStr;
    getline(ss, idStr, ',');

    if (!idStr.empty()) {
        return stoi(idStr) + 1;
    }

    return 1;
}

void LearnerLinkedList_Test() {
    // learnerLL.addLearner(createLearner("Mohammed"));
    // learnerLL.addLearner(createLearner("Ayman"));
    // learnerLL.addLearner(createLearner("Hasan"));
    // learnerLL.addLearner(createLearner("Omar"));

    learnerLL.displayAllLearners();
}

// Forward declarations for queue functions (defined in task2.cpp)
extern EnrollmentQueue enrollmentQueue;
extern TransitionQueue transitionQueue;
extern bool isEnrollmentQueueFull();
extern bool isTransitionQueueFull();
extern void enqueueEnrollment(int learnerID);
extern void enqueueTransition(int learnerID, int targetSession, int targetActivity);

void saveQueuesToCSV() {
    ofstream file("Dataset/queues.csv");
    if (!file.is_open()) {
        cout << "Error: Could not open queues.csv for writing" << endl;
        return;
    }

    // Write header
    file << "queueType,data" << endl;

    // Write enrollment queue
    if (!isEnrollmentQueueEmpty()) {
        file << "enrollment,";
        int idx = enrollmentQueue.front;
        for (int i = 0; i < enrollmentQueue.count; i++) {
            file << enrollmentQueue.learnerIDs[idx];
            if (i < enrollmentQueue.count - 1) file << ";";
            idx = (idx + 1) % MAX_QUEUE_SIZE;
        }
        file << endl;
    }

    // Write transition queue
    if (!isTransitionQueueEmpty()) {
        file << "transition,";
        int idx = transitionQueue.front;
        for (int i = 0; i < transitionQueue.count; i++) {
            file << transitionQueue.requests[idx].learnerID << ":"
                 << transitionQueue.requests[idx].targetSession << ":"
                 << transitionQueue.requests[idx].targetActivity;
            if (i < transitionQueue.count - 1) file << ";";
            idx = (idx + 1) % MAX_QUEUE_SIZE;
        }
        file << endl;
    }

    file.close();
}

void loadQueuesFromCSV() {
    ifstream file("Dataset/queues.csv");
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
        string queueType, data;

        getline(ss, queueType, ',');
        getline(ss, data, ',');

        if (queueType == "enrollment" && !data.empty()) {
            stringstream dataStream(data);
            string learnerID;
            while (getline(dataStream, learnerID, ';')) {
                if (!learnerID.empty() && !isEnrollmentQueueFull()) {
                    enqueueEnrollment(stoi(learnerID));
                }
            }
        } else if (queueType == "transition" && !data.empty()) {
            stringstream dataStream(data);
            string entry;
            while (getline(dataStream, entry, ';')) {
                if (!entry.empty() && !isTransitionQueueFull()) {
                    stringstream entryStream(entry);
                    string learnerID, sessionID, activityID;
                    getline(entryStream, learnerID, ':');
                    getline(entryStream, sessionID, ':');
                    getline(entryStream, activityID, ':');
                    if (!learnerID.empty() && !sessionID.empty() && !activityID.empty()) {
                        enqueueTransition(stoi(learnerID), stoi(sessionID), stoi(activityID));
                    }
                }
            }
        }
    }

    file.close();
}