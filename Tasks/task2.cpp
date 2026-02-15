/*
Session 1: ARRAYS & MEMORY
---- Create an Array	Declare and initialize an integer array of size 5	1/5
---- Access Elements	Print all array elements using index	1/5
---- Modify Elements	Change the value at position 2 to 99	2/5
---- Array Sum	Calculate the sum of all elements in the array	2/5
---- Find Maximum	Find the largest element in the array	3/5

Session 2: LOOPING CONSTRUCTS
--- For Loop Basics: Print numbers 1 to 10 using a for loop	1/5
--- While Loop:	Repeat until user enters 0	1/5
--- Nested Loops: Print a multiplication table (1-5)	2/5
--- Loop with Array:	Traverse an array using a loop and print each element	2/5
--- Break & Continue:	Print 1-20, skip even numbers, stop at 15	3/5

Session 3: FUNCTIONS & SCOPE
---	Function Declaration:	Define and call a function that prints "Hello World"	2/5
---	Parameters & Return:	Write a function that adds two numbers and returns the result	2/5
---	Pass by Value:	Demonstrate that the original variable doesn't change	2/5
---	Pass by Reference:	Modify the original variable using references	3/5
---	Function Overloading:	Create two functions with same name, different parameters	3/5
---	Scope Rules: Demonstrate local vs global variables	3/5

Session 4:  DEBUGGIN WORKSHOP
---- Syntax Errors: Fix missing semicolons, brackets, and typos	2/5
---- Logic Errors:	Find why the sum is always incorrect	3/5
---- Runtime Errors:	Handle division by zero and array out of bounds	3/5
---- Trace Execution:	Step through code mentally and predict the output	4/5

Session 5: MINI PROJECT LAB
---- Student Grade Calculator: Input 5 grades, calculate average, assign letter grade	4/5
---- Basic Inventory System: Store item names and quantities, search by name	4/5
---- Simple Menu Program: Create a do-while menu with at least 4 options	4/5
*/

#include <iostream>
#include <limits>
#include "datastructures.h"

using namespace std;

// ========== Initializing Variables ==========
SessionManagement sessions;
extern LearnerLinkedList learnerLL;

// Queue constants
const int MAX_QUEUE_SIZE = 50;

// Queue 2: Transition Queue (students waiting for specific activities) - local to task2
struct TransitionRequest {
    int learnerID;
    int targetSession;
    int targetActivity;
};

struct TransitionQueue {
    TransitionRequest requests[MAX_QUEUE_SIZE];
    int front;
    int rear;
    int count;
    
    TransitionQueue() {
        front = 0;
        rear = -1;
        count = 0;
        for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
            requests[i].learnerID = -1;
            requests[i].targetSession = -1;
            requests[i].targetActivity = -1;
        }
    }
};

// Global enrollment queue instance - defined here, declared as extern in datastructures.h
EnrollmentQueue enrollmentQueue;

// Local transition queue instance
TransitionQueue transitionQueue;

// Forward declarations
void viewAllSessionsAndActivities();
void changeStudentActivity();
void rollbackStudentActivity();
void viewQueues();
Learner* findLearnerByID(int id);
int getMaxActivities(int sessionID);
void clearInputBuffer();
bool isActivityOccupied(int sessionID, int activityID, int excludeLearnerID);

// Queue operations - made accessible to main.cpp
bool isEnrollmentQueueEmpty() {
    return enrollmentQueue.count == 0;
}

bool isEnrollmentQueueFull() {
    return enrollmentQueue.count >= MAX_ENROLLMENT_QUEUE_SIZE;
}

void enqueueEnrollment(int learnerID) {
    if (isEnrollmentQueueFull()) {
        cout << "Error: Enrollment queue is full!" << endl;
        return;
    }
    enrollmentQueue.rear = (enrollmentQueue.rear + 1) % MAX_ENROLLMENT_QUEUE_SIZE;
    enrollmentQueue.learnerIDs[enrollmentQueue.rear] = learnerID;
    enrollmentQueue.count++;
}

int dequeueEnrollment() {
    if (isEnrollmentQueueEmpty()) {
        return -1;
    }
    int learnerID = enrollmentQueue.learnerIDs[enrollmentQueue.front];
    enrollmentQueue.learnerIDs[enrollmentQueue.front] = -1;
    enrollmentQueue.front = (enrollmentQueue.front + 1) % MAX_ENROLLMENT_QUEUE_SIZE;
    enrollmentQueue.count--;
    return learnerID;
}

bool isTransitionQueueEmpty() {
    return transitionQueue.count == 0;
}

bool isTransitionQueueFull() {
    return transitionQueue.count >= MAX_QUEUE_SIZE;
}

void enqueueTransition(int learnerID, int targetSession, int targetActivity) {
    if (isTransitionQueueFull()) {
        cout << "Error: Transition queue is full!" << endl;
        return;
    }
    transitionQueue.rear = (transitionQueue.rear + 1) % MAX_QUEUE_SIZE;
    transitionQueue.requests[transitionQueue.rear].learnerID = learnerID;
    transitionQueue.requests[transitionQueue.rear].targetSession = targetSession;
    transitionQueue.requests[transitionQueue.rear].targetActivity = targetActivity;
    transitionQueue.count++;
}

TransitionRequest dequeueTransition() {
    TransitionRequest req;
    req.learnerID = -1;
    req.targetSession = -1;
    req.targetActivity = -1;
    
    if (isTransitionQueueEmpty()) {
        return req;
    }
    
    req = transitionQueue.requests[transitionQueue.front];
    transitionQueue.requests[transitionQueue.front].learnerID = -1;
    transitionQueue.requests[transitionQueue.front].targetSession = -1;
    transitionQueue.requests[transitionQueue.front].targetActivity = -1;
    transitionQueue.front = (transitionQueue.front + 1) % MAX_QUEUE_SIZE;
    transitionQueue.count--;
    return req;
}

bool isLearnerInEnrollmentQueue(int learnerID) {
    int idx = enrollmentQueue.front;
    for (int i = 0; i < enrollmentQueue.count; i++) {
        if (enrollmentQueue.learnerIDs[idx] == learnerID) {
            return true;
        }
        idx = (idx + 1) % MAX_ENROLLMENT_QUEUE_SIZE;
    }
    return false;
}

bool isLearnerInTransitionQueue(int learnerID, int &targetSession, int &targetActivity) {
    int idx = transitionQueue.front;
    for (int i = 0; i < transitionQueue.count; i++) {
        if (transitionQueue.requests[idx].learnerID == learnerID) {
            targetSession = transitionQueue.requests[idx].targetSession;
            targetActivity = transitionQueue.requests[idx].targetActivity;
            return true;
        }
        idx = (idx + 1) % MAX_QUEUE_SIZE;
    }
    return false;
}

void processQueues() {
    // First, process transition queue (higher priority for existing students)
    bool processedAny = true;
    int iterations = 0;
    const int MAX_ITERATIONS = transitionQueue.count + 1;
    
    while (processedAny && iterations < MAX_ITERATIONS && !isTransitionQueueEmpty()) {
        processedAny = false;
        iterations++;
        
        int idx = transitionQueue.front;
        TransitionRequest req = transitionQueue.requests[idx];
        
        if (req.learnerID != -1) {
            if (!isActivityOccupied(req.targetSession, req.targetActivity, req.learnerID)) {
                req = dequeueTransition();
                Learner* learner = findLearnerByID(req.learnerID);
                if (learner != NULL) {
                    learner->currentSessionID = req.targetSession;
                    learner->currentActivity = req.targetActivity;
                    learner->isActive = true;
                    cout << "\n>>> Auto-assigned: Student " << learner->name 
                         << " (ID: " << req.learnerID << ") to Session " 
                         << req.targetSession << ", Activity " << req.targetActivity 
                         << " from transition queue" << endl;
                    processedAny = true;
                }
            }
        }
    }
    
    // Then, process enrollment queue for Session 1, Activity 1
    while (!isEnrollmentQueueEmpty()) {
        int idx = enrollmentQueue.front;
        int learnerID = enrollmentQueue.learnerIDs[idx];
        
        if (learnerID != -1) {
            if (!isActivityOccupied(1, 1, learnerID)) {
                learnerID = dequeueEnrollment();
                Learner* learner = findLearnerByID(learnerID);
                if (learner != NULL) {
                    learner->currentSessionID = 1;
                    learner->currentActivity = 1;
                    learner->isActive = true;
                    cout << "\n>>> Auto-enrolled: Student " << learner->name 
                         << " (ID: " << learnerID << ") to Session 1, Activity 1" << endl;
                }
            } else {
                break;
            }
        }
    }
}

void displayQueues() {
    cout << "\n========== WAITING QUEUES ==========" << endl;
    
    cout << "\n--- NEW ENROLLMENT QUEUE (Session 1, Activity 1) ---" << endl;
    if (isEnrollmentQueueEmpty()) {
        cout << "  Queue is empty" << endl;
    } else {
        cout << "  Waiting students (" << enrollmentQueue.count << "): ";
        int idx = enrollmentQueue.front;
        for (int i = 0; i < enrollmentQueue.count; i++) {
            int learnerID = enrollmentQueue.learnerIDs[idx];
            Learner* l = findLearnerByID(learnerID);
            if (l != NULL) {
                cout << l->name << " (ID: " << learnerID << ")";
                if (i < enrollmentQueue.count - 1) cout << ", ";
            }
            idx = (idx + 1) % MAX_ENROLLMENT_QUEUE_SIZE;
        }
        cout << endl;
    }

    cout << "\n--- TRANSITION QUEUE ---" << endl;
    if (isTransitionQueueEmpty()) {
        cout << "  Queue is empty" << endl;
    } else {
        cout << "  Waiting students (" << transitionQueue.count << "):" << endl;
        int idx = transitionQueue.front;
        for (int i = 0; i < transitionQueue.count; i++) {
            TransitionRequest req = transitionQueue.requests[idx];
            if (req.learnerID != -1) {
                Learner* l = findLearnerByID(req.learnerID);
                if (l != NULL) {
                    cout << "    " << (i + 1) << ". " << l->name 
                         << " (ID: " << req.learnerID << ") -> Session " 
                         << req.targetSession << ", Activity " << req.targetActivity << endl;
                }
            }
            idx = (idx + 1) % MAX_QUEUE_SIZE;
        }
    }
    
    cout << "\n====================================" << endl;
}

void clearInputBuffer() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

void InitializingSessions() {
    int choice;
    
    while (true) {
        processQueues();
        
        cout << "\n===== ACTIVITY NAVIGATION MENU =====" << endl;
        cout << "1. View all sessions and activities with enrolled students" << endl;
        cout << "2. Promote Student" << endl;
        cout << "3. Rollback student to previous activity" << endl;
        cout << "4. View Waiting Queues" << endl;
        cout << "5. Return to Main Menu" << endl;
        cout << "Choice: ";
        
        if (!(cin >> choice)) {
            clearInputBuffer();
            cout << "Invalid input. Please enter a number." << endl;
            continue;
        }
        
        switch(choice) {
            case 1:
                viewAllSessionsAndActivities();
                break;
            case 2:
                changeStudentActivity();
                break;
            case 3:
                rollbackStudentActivity();
                break;
            case 4:
                viewQueues();
                break;
            case 5:
                cout << "Returning to main menu..." << endl;
                system("cls");
                return;
            default:
                cout << "Invalid choice. Please try again." << endl;
        }
    }
}

int getMaxActivities(int sessionID) {
    if (sessionID == 3) return 6;
    if (sessionID == 4) return 4;
    if (sessionID == 5) return 3;
    return 5;
}

void viewAllSessionsAndActivities() {
    cout << "\n========== ALL SESSIONS AND ACTIVITIES ==========" << endl;
    
    Learner* temp = learnerLL.getHead();
    
    cout << "\n--- STUDENTS CURRENTLY ENROLLED ---" << endl;
    if (temp == NULL) {
        cout << "No learners enrolled." << endl;
        return;
    }
    
    // Group by session and activity
    for (int sessionID = 1; sessionID <= 5; sessionID++) {
        cout << "\nSession " << sessionID << ":" << endl;
        int maxAct = getMaxActivities(sessionID);
        bool hasStudents = false;
        
        for (int activityID = 1; activityID <= maxAct; activityID++) {
            temp = learnerLL.getHead();
            bool found = false;
            while (temp != NULL) {
                if (temp->currentSessionID == sessionID && 
                    temp->currentActivity == activityID && 
                    temp->isActive) {
                    cout << "  Activity " << activityID << ": Student " << temp->name << " (ID: " << temp->id << ")" << endl;
                    found = true;
                    hasStudents = true;
                    break;
                }
                temp = temp->next;
            }
            if (!found) {
                cout << "  Activity " << activityID << ": [Empty]" << endl;
            }
        }
        
        if (!hasStudents) {
            cout << "  (No active students in this session)" << endl;
        }
    }
    
    // Show students not actively enrolled (inactive)
    temp = learnerLL.getHead();
    bool hasInactive = false;
    while (temp != NULL) {
        if (!temp->isActive) {
            // Check if they're in a queue
            int targetSession, targetActivity;
            bool inTransitionQueue = isLearnerInTransitionQueue(temp->id, targetSession, targetActivity);
            bool inEnrollmentQueue = isLearnerInEnrollmentQueue(temp->id);
            
            if (!hasInactive) {
                cout << "\n--- INACTIVE STUDENTS ---" << endl;
                hasInactive = true;
            }
            
            cout << "  Student ID: " << temp->id 
                 << " | Name: " << temp->name;
            
            if (inTransitionQueue) {
                cout << " | Status: Waiting for Session " << targetSession 
                     << ", Activity " << targetActivity << endl;
            } else if (inEnrollmentQueue) {
                cout << " | Status: Waiting for Session 1, Activity 1 (enrollment queue)" << endl;
            } else {
                cout << " | Status: Not enrolled" << endl;
            }
        }
        temp = temp->next;
    }
    
    cout << "\n================================================" << endl;
}

void viewQueues() {
    displayQueues();
}

Learner* findLearnerByID(int id) {
    Learner* temp = learnerLL.getHead();
    while (temp != NULL) {
        if (temp->id == id) {
            return temp;
        }
        temp = temp->next;
    }
    return NULL;
}

bool isActivityOccupied(int sessionID, int activityID, int excludeLearnerID) {
    Learner* temp = learnerLL.getHead();
    while (temp != NULL) {
        if (temp->isActive && 
            temp->id != excludeLearnerID &&
            temp->currentSessionID == sessionID && 
            temp->currentActivity == activityID) {
            return true;
        }
        temp = temp->next;
    }
    return false;
}

void changeStudentActivity() {
    int learnerID;
    
    cout << "\n--- PROMOTE/ENROLL STUDENT ---" << endl;
    cout << "Enter student ID: ";
    
    if (!(cin >> learnerID)) {
        clearInputBuffer();
        cout << "Invalid input. Please enter a valid student ID." << endl;
        return;
    }
    
    Learner* learner = findLearnerByID(learnerID);
    if (learner == NULL) {
        cout << "Error: Student with ID " << learnerID << " not found." << endl;
        return;
    }
    
    // Check if student is in a queue
    int targetSession, targetActivity;
    if (isLearnerInTransitionQueue(learnerID, targetSession, targetActivity)) {
        cout << "\nStudent " << learner->name << " is currently waiting in the transition queue" << endl;
        cout << "for Session " << targetSession << ", Activity " << targetActivity << "." << endl;
        cout << "They cannot be promoted while waiting in queue." << endl;
        return;
    }
    
    if (isLearnerInEnrollmentQueue(learnerID)) {
        cout << "\nStudent " << learner->name << " is currently waiting in the enrollment queue." << endl;
        cout << "They cannot be promoted while waiting in queue." << endl;
        return;
    }
    
    if (!learner->isActive) {
        cout << "Student is not currently active in any session." << endl;
        cout << "Do you want to enroll them in Session 1, Activity 1? (1=Yes, 2=Add to queue, 0=No): ";
        int enroll;
        if (!(cin >> enroll)) {
            clearInputBuffer();
            cout << "Invalid input." << endl;
            return;
        }
        if (enroll == 1) {
            if (isActivityOccupied(1, 1, learnerID)) {
                cout << "Session 1, Activity 1 is currently occupied." << endl;
                cout << "Adding student to enrollment queue..." << endl;
                enqueueEnrollment(learnerID);
                cout << "Student " << learner->name << " added to enrollment queue (position: " 
                     << enrollmentQueue.count << ")" << endl;
            } else {
                learner->isActive = true;
                learner->currentSessionID = 1;
                learner->currentActivity = 1;
                cout << "Student enrolled in Session 1, Activity 1." << endl;
            }
        } else if (enroll == 2) {
            enqueueEnrollment(learnerID);
            cout << "Student " << learner->name << " added to enrollment queue (position: " 
                 << enrollmentQueue.count << ")" << endl;
        }
        return;
    }
    
    cout << "\nCurrent Status:" << endl;
    cout << "Student: " << learner->name << endl;
    cout << "Current Session: " << learner->currentSessionID << endl;
    cout << "Current Activity: " << learner->currentActivity << endl;
    
    cout << "\nDo you want to promote this student to the next activity? (1=Yes, 0=No): ";
    
    int option;
    if (!(cin >> option)) {
        clearInputBuffer();
        cout << "Invalid input." << endl;
        return;
    }
    
    if (option == 1) {
        int currentSession = learner->currentSessionID;
        int currentActivity = learner->currentActivity;
        
        double score;
        cout << "Enter student's score (0-100): ";
        if (!(cin >> score)) {
            clearInputBuffer();
            cout << "Invalid input. Please enter a number." << endl;
            return;
        }

        if (score < 0 || score > 100) {
            cout << "Invalid score! Must be between 0 and 100." << endl;
            return;
        }
        
        if (score < 50) {
            cout << "Promotion rejected! Score " << score << "% is below 50% (Fail)." << endl;
            return;
        }
        
        int maxActivities = getMaxActivities(currentSession);
        int targetSession = currentSession;
        int targetActivity;
        
        if (currentActivity < maxActivities) {
            targetActivity = currentActivity + 1;
        } else {
            if (currentSession < 5) {
                targetSession = currentSession + 1;
                targetActivity = 1;
            } else {
                if (currentActivity >= 1 && currentActivity <= 5) {
                    learner->completedSessions[currentActivity - 1] = 1;
                }
                learner->isActive = false;
                cout << "Congratulations! Student has completed all sessions!" << endl;
                return;
            }
        }
        
        if (isActivityOccupied(targetSession, targetActivity, learnerID)) {
            cout << "Session " << targetSession << ", Activity " << targetActivity 
                 << " is currently occupied." << endl;
            cout << "Adding student to transition queue..." << endl;
            enqueueTransition(learnerID, targetSession, targetActivity);
            
            if (currentActivity >= 1 && currentActivity <= 5) {
                learner->completedSessions[currentActivity - 1] = 1;
            }
            learner->isActive = false;
            learner->currentSessionID = -1;
            learner->currentActivity = -1;
            
            cout << "Student " << learner->name << " added to transition queue." << endl;
            cout << "They will be automatically enrolled when the activity becomes free." << endl;
            return;
        }
        
        learner->currentSessionID = targetSession;
        learner->currentActivity = targetActivity;
        if (currentActivity >= 1 && currentActivity <= 5) {
            learner->completedSessions[currentActivity - 1] = 1;
        }
        
        if (targetSession != currentSession) {
            cout << "Student promoted to Session " << targetSession 
                 << ", Activity " << targetActivity 
                 << " (Auto-transitioned from Session " << currentSession << ")" << endl;
        } else {
            cout << "Student promoted to Session " << targetSession 
                 << ", Activity " << targetActivity << endl;
        }
    } else if (option == 0) {
        cout << "Promotion cancelled." << endl;
    } else {
        cout << "Invalid option." << endl;
    }
}

void rollbackStudentActivity() {
    int learnerID;
    
    cout << "\n--- ROLLBACK STUDENT ACTIVITY ---" << endl;
    cout << "Enter student ID: ";
    if (!(cin >> learnerID)) {
        clearInputBuffer();
        cout << "Invalid input. Please enter a valid student ID." << endl;
        return;
    }
    
    Learner* learner = findLearnerByID(learnerID);
    if (learner == NULL) {
        cout << "Error: Student with ID " << learnerID << " not found." << endl;
        return;
    }
    
    // Check if student is in a queue
    int targetSession, targetActivity;
    if (isLearnerInTransitionQueue(learnerID, targetSession, targetActivity)) {
        cout << "\nStudent " << learner->name << " is currently waiting in the transition queue" << endl;
        cout << "for Session " << targetSession << ", Activity " << targetActivity << "." << endl;
        cout << "They cannot be rolled back while waiting in queue." << endl;
        return;
    }
    
    if (isLearnerInEnrollmentQueue(learnerID)) {
        cout << "\nStudent " << learner->name << " is currently waiting in the enrollment queue." << endl;
        cout << "They cannot be rolled back while waiting in queue." << endl;
        return;
    }
    
    if (!learner->isActive) {
        cout << "Student is not currently active in any session." << endl;
        return;
    }
    
    int currentSession = learner->currentSessionID;
    int currentActivity = learner->currentActivity;
    
    cout << "\nCurrent Status:" << endl;
    cout << "Student: " << learner->name << endl;
    cout << "Current Session: " << currentSession << endl;
    cout << "Current Activity: " << currentActivity << endl;
    
    cout << "\nEnter target session (1-" << currentSession << "): ";
    int rollbackSession;
    if (!(cin >> rollbackSession)) {
        clearInputBuffer();
        cout << "Invalid input." << endl;
        return;
    }
    
    if (rollbackSession < 1 || rollbackSession > currentSession) {
        cout << "Error: Invalid session number." << endl;
        return;
    }
    
    cout << "Enter target activity: ";
    int rollbackActivity;
    if (!(cin >> rollbackActivity)) {
        clearInputBuffer();
        cout << "Invalid input." << endl;
        return;
    }
    
    int maxActivities = getMaxActivities(rollbackSession);
    
    if (rollbackActivity < 1 || rollbackActivity > maxActivities) {
        cout << "Error: Invalid activity number for Session " << rollbackSession << "." << endl;
        return;
    }
    
    if (rollbackSession == currentSession && rollbackActivity >= currentActivity) {
        cout << "Error: Can only rollback to previous activities." << endl;
        return;
    }
    
    if (isActivityOccupied(rollbackSession, rollbackActivity, learnerID)) {
        cout << "Session " << rollbackSession << ", Activity " << rollbackActivity 
             << " is currently occupied." << endl;
        cout << "Adding student to transition queue..." << endl;
        enqueueTransition(learnerID, rollbackSession, rollbackActivity);
        
        learner->isActive = false;
        learner->currentSessionID = -1;
        learner->currentActivity = -1;
        
        cout << "Student " << learner->name << " added to transition queue." << endl;
        cout << "They will be automatically enrolled when the activity becomes free." << endl;
        return;
    }
    
    learner->currentSessionID = rollbackSession;
    learner->currentActivity = rollbackActivity;
    
    cout << "Student successfully rolled back to Session " << rollbackSession 
         << ", Activity " << rollbackActivity << endl;
}
