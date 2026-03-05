#include <iostream>
#include <string>
#include <limits>
#include <map>        // For hash maps
#include "../Tasks/datastructures.h"

using namespace std;

// External global variables from main.cpp
extern int nextID;
extern LearnerLinkedList learnerLL;

// ========== SESSION MANAGEMENT ==========
// Global session array with predefined sessions
Session task1Sessions[5];

// Active session slots - stores learnerID in each activity slot
// sessions have different capacities based on number of activities
int activeSlots[5][6]; // Max 6 activities per session

// ========== WAITING QUEUE (Manual Array-Based Queue) ==========
// Normal queue implementation for each session
const int MAX_WAITING_QUEUE = 20;

struct WaitingQueueStruct {
    int learnerIDs[MAX_WAITING_QUEUE];
    int front;
    int rear;
    int count;

    WaitingQueueStruct() {
        front = 0;
        rear = -1;
        count = 0;
        for (int i = 0; i < MAX_WAITING_QUEUE; i++) {
            learnerIDs[i] = -1;
        }
    }
};

WaitingQueueStruct waitingQueues[5]; // One queue per session

// ========== HASH MAPS (STL map) ==========
// Learner → Session mapping: O(1) lookup "Which session is learner 101 in?"
map<int, int> learnerSessionMap; // learnerID → sessionID

// Learner → Progress mapping: O(1) Resume functionality
struct ProgressInfo {
    int currentSessionID;
    int currentActivityID;
    int completedSessions[5];
};
map<int, ProgressInfo> learnerProgressMap; // learnerID → ProgressInfo

// ========== INITIALIZATION ==========
void initializeSessions();
void initializeActiveSlots();
void task1_restoreSessionStateFromCSV(); // Task 1 specific: Restore session state from CSV

// ========== QUEUE OPERATIONS ==========
bool isSessionFull(int sessionID);
void enqueueWaiting(int sessionID, int learnerID);
int dequeueWaiting(int sessionID);
void displayWaitingQueue(int sessionID);
int getWaitingQueueSize(int sessionID);

// ========== LEARNER OPERATIONS ==========
void registerLearner();
void joinSession();
void exitSession();
void displayStatus();
void resumeProgress();
Learner* task1_findLearnerByID(int learnerID);
bool canJoinSession(Learner* learner, int sessionID);
int findAvailableActivitySlot(int sessionID);

// ========== SESSION PROGRESSION ==========
bool hasCompletedPrerequisite(Learner* learner, int sessionID);
void markSessionComplete(Learner* learner, int sessionID);
void updateProgress(int learnerID, int sessionID, int activityID);

// ========== MAIN MENU FOR TASK 1 ==========
void Task1_Menu() {
    // Initialize everything first time
    static bool initialized = false;
    if (!initialized) {
        initializeSessions();
        initializeActiveSlots();
        task1_restoreSessionStateFromCSV(); // Task 1: Restore session state from CSV
        initialized = true;
    }

    int choice;
    while (true) {
        cout << "\n===== TASK 1: SESSION MANAGEMENT =====" << endl;
        cout << "1. Register Learner" << endl;
        cout << "2. Join Session" << endl;
        cout << "3. Exit Session" << endl;
        cout << "4. Display Status" << endl;
        cout << "5. Resume Progress" << endl;
        cout << "6. View All Sessions" << endl;
        cout << "7. View Waiting Queues" << endl;
        cout << "8. Back to Main Menu" << endl;
        cout << "Choice: ";

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a number." << endl;
            continue;
        }

        switch (choice) {
            case 1:
                registerLearner();
                break;
            case 2:
                joinSession();
                break;
            case 3:
                exitSession();
                break;
            case 4:
                displayStatus();
                break;
            case 5:
                resumeProgress();
                break;
            case 6:
                // Display all sessions
                cout << "\n===== ALL SESSIONS =====" << endl;
                for (int i = 0; i < 5; i++) {
                    cout << "Session " << task1Sessions[i].id << ": " << task1Sessions[i].name
                         << " (Capacity: " << task1Sessions[i].capacity << " activities)" << endl;
                }
                break;
            case 7:
                // View waiting queues
                cout << "\n===== WAITING QUEUES =====" << endl;
                for (int i = 0; i < 5; i++) {
                    cout << "\nSession " << (i + 1) << ": " << task1Sessions[i].name << endl;
                    cout << "Queue Size: " << waitingQueues[i].count << "/" << MAX_WAITING_QUEUE << endl;

                    if (waitingQueues[i].count > 0) {
                        // Display all learners in queue
                        cout << "Learners waiting (in order): ";
                        int position = 1;
                        int idx = waitingQueues[i].front;
                        for (int j = 0; j < waitingQueues[i].count; j++) {
                            int learnerID = waitingQueues[i].learnerIDs[idx];
                            Learner* learner = task1_findLearnerByID(learnerID);
                            if (learner != NULL) {
                                cout << "\n  " << position << ". ID: " << learnerID
                                     << " (" << learner->name << ")";
                            } else {
                                cout << "\n  " << position << ". ID: " << learnerID;
                            }
                            idx = (idx + 1) % MAX_WAITING_QUEUE;
                            position++;
                        }
                        cout << endl;
                    } else {
                        cout << "No learners waiting." << endl;
                    }
                }
                break;
            case 8:
                return; // Back to main menu
            default:
                cout << "Invalid choice. Please try again." << endl;
        }
    }
}

// ========== INITIALIZATION FUNCTIONS ==========
void initializeSessions() {
    // Session 1: Arrays & Memory (5 activities)
    task1Sessions[0].id = 1;
    task1Sessions[0].name = "Arrays & Memory";
    task1Sessions[0].topic = "Arrays";
    task1Sessions[0].capacity = 5; // 5 activities = max 5 learners

    // Session 2: Looping Constructs (5 activities)
    task1Sessions[1].id = 2;
    task1Sessions[1].name = "Looping Constructs";
    task1Sessions[1].topic = "Loops";
    task1Sessions[1].capacity = 5; // 5 activities = max 5 learners

    // Session 3: Functions & Scope (6 activities)
    task1Sessions[2].id = 3;
    task1Sessions[2].name = "Functions & Scope";
    task1Sessions[2].topic = "Functions";
    task1Sessions[2].capacity = 6; // 6 activities = max 6 learners

    // Session 4: Debugging Workshop (4 activities)
    task1Sessions[3].id = 4;
    task1Sessions[3].name = "Debugging Workshop";
    task1Sessions[3].topic = "Debugging";
    task1Sessions[3].capacity = 4; // 4 activities = max 4 learners

    // Session 5: Mini Project Lab (3 activities)
    task1Sessions[4].id = 5;
    task1Sessions[4].name = "Mini Project Lab";
    task1Sessions[4].topic = "Integration";
    task1Sessions[4].capacity = 3; // 3 activities = max 3 learners
}

void initializeActiveSlots() {
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 6; j++) {
            activeSlots[i][j] = -1; // -1 means empty activity slot
        }
    }
}

// ========== QUEUE OPERATIONS (Manual Array-Based) ==========
bool isSessionFull(int sessionID) {
    int idx = sessionID - 1;
    int occupiedSlots = 0;

    // Count how many activities have learners
    for (int i = 0; i < task1Sessions[idx].capacity; i++) {
        if (activeSlots[idx][i] != -1) {
            occupiedSlots++;
        }
    }

    return occupiedSlots >= task1Sessions[idx].capacity;
}

void enqueueWaiting(int sessionID, int learnerID) {
    int idx = sessionID - 1;

    if (waitingQueues[idx].count >= MAX_WAITING_QUEUE) {
        cout << "Waiting queue is full!" << endl;
        return;
    }

    // Check if learner is already in the queue (prevent duplicates)
    int current = waitingQueues[idx].front;
    for (int i = 0; i < waitingQueues[idx].count; i++) {
        if (waitingQueues[idx].learnerIDs[current] == learnerID) {
            cout << "Learner " << learnerID << " is already in the waiting queue!" << endl;
            return;
        }
        current = (current + 1) % MAX_WAITING_QUEUE;
    }

    // Add to rear (circular)
    waitingQueues[idx].rear = (waitingQueues[idx].rear + 1) % MAX_WAITING_QUEUE;
    waitingQueues[idx].learnerIDs[waitingQueues[idx].rear] = learnerID;
    waitingQueues[idx].count++;
}

int dequeueWaiting(int sessionID) {
    int idx = sessionID - 1;

    if (waitingQueues[idx].count == 0) {
        return -1;
    }

    int learnerID = waitingQueues[idx].learnerIDs[waitingQueues[idx].front];
    waitingQueues[idx].learnerIDs[waitingQueues[idx].front] = -1;
    waitingQueues[idx].front = (waitingQueues[idx].front + 1) % MAX_WAITING_QUEUE;
    waitingQueues[idx].count--;

    return learnerID;
}

void displayWaitingQueue(int sessionID) {
    int idx = sessionID - 1;

    if (waitingQueues[idx].count == 0) {
        cout << "No learners waiting." << endl;
        return;
    }

    cout << "Waiting Queue (" << waitingQueues[idx].count << " learners): ";
    int current = waitingQueues[idx].front;
    for (int i = 0; i < waitingQueues[idx].count; i++) {
        cout << waitingQueues[idx].learnerIDs[current] << " ";
        current = (current + 1) % MAX_WAITING_QUEUE;
    }
    cout << endl;
}

int getWaitingQueueSize(int sessionID) {
    return waitingQueues[sessionID - 1].count;
}

// ========== LEARNER OPERATIONS ==========
void registerLearner() {
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    string name;
    cout << "\nEnter learner name: ";
    getline(cin, name);

    if (name.empty()) {
        cout << "Name cannot be empty!" << endl;
        return;
    }

    Learner* newLearner = createLearner(name);
    learnerLL.addLearner(newLearner);

    // Initialize progress in hash map
    ProgressInfo progress;
    progress.currentSessionID = -1;
    progress.currentActivityID = -1;
    for (int i = 0; i < 5; i++) {
        progress.completedSessions[i] = 0;
    }
    learnerProgressMap[newLearner->id] = progress;

    cout << "✓ Learner registered successfully!" << endl;
    cout << "  ID: " << newLearner->id << endl;
    cout << "  Name: " << newLearner->name << endl;
}

// Renamed to avoid conflict with task2.cpp's findLearnerByID
Learner* task1_findLearnerByID(int learnerID) {
    Learner* temp = learnerLL.getHead();
    while (temp != NULL) {
        if (temp->id == learnerID) {
            return temp;
        }
        temp = temp->next;
    }
    return NULL;
}

bool hasCompletedPrerequisite(Learner* learner, int sessionID) {
    // Session 1 has no prerequisite
    if (sessionID == 1) return true;

    // Check progress hash map
    if (learnerProgressMap.find(learner->id) != learnerProgressMap.end()) {
        return learnerProgressMap[learner->id].completedSessions[sessionID - 2] == 1;
    }

    // Fallback to learner struct
    return learner->completedSessions[sessionID - 2] == 1;
}

bool canJoinSession(Learner* learner, int sessionID) {
    // Check if already in a session
    if (learner->isActive) {
        cout << "Learner is already in Session " << learner->currentSessionID << endl;
        return false;
    }

    // Check prerequisites
    if (!hasCompletedPrerequisite(learner, sessionID)) {
        cout << "Cannot join Session " << sessionID << ". Must complete Session "
             << (sessionID - 1) << " first." << endl;
        return false;
    }

    return true;
}

int findAvailableActivitySlot(int sessionID) {
    int idx = sessionID - 1;

    // Each activity can have only 1 or no learners
    for (int i = 0; i < task1Sessions[idx].capacity; i++) {
        if (activeSlots[idx][i] == -1) {
            return i; // Return the activity index
        }
    }

    return -1; // No available activity slot
}

void updateProgress(int learnerID, int sessionID, int activityID) {
    if (learnerProgressMap.find(learnerID) != learnerProgressMap.end()) {
        learnerProgressMap[learnerID].currentSessionID = sessionID;
        learnerProgressMap[learnerID].currentActivityID = activityID;
    }
}

void joinSession() {
    int learnerID, sessionID;

    cout << "\nEnter Learner ID: ";
    if (!(cin >> learnerID)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input!" << endl;
        return;
    }

    Learner* learner = task1_findLearnerByID(learnerID);
    if (learner == NULL) {
        cout << "Learner not found!" << endl;
        return;
    }

    cout << "\nAvailable Sessions:" << endl;
    for (int i = 0; i < 5; i++) {
        cout << (i + 1) << ". " << task1Sessions[i].name;
        cout << " (Capacity: " << task1Sessions[i].capacity << " activities)";
        if (hasCompletedPrerequisite(learner, i + 1)) {
            cout << " ✓";
        } else {
            cout << " ✗ (Locked - Complete Session " << i << " first)";
        }
        cout << endl;
    }

    cout << "\nEnter Session ID (1-5): ";
    if (!(cin >> sessionID) || sessionID < 1 || sessionID > 5) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid session ID!" << endl;
        return;
    }

    if (!canJoinSession(learner, sessionID)) {
        return;
    }

    // Check if session is full (all activities occupied)
    if (isSessionFull(sessionID)) {
        cout << "Session " << sessionID << " is full (all " << task1Sessions[sessionID-1].capacity
             << " activities occupied)." << endl;

        // Check if learner is already in the waiting queue
        int idx = sessionID - 1;
        int current = waitingQueues[idx].front;
        bool alreadyInQueue = false;
        for (int i = 0; i < waitingQueues[idx].count; i++) {
            if (waitingQueues[idx].learnerIDs[current] == learnerID) {
                alreadyInQueue = true;
                break;
            }
            current = (current + 1) % MAX_WAITING_QUEUE;
        }

        if (alreadyInQueue) {
            cout << "✗ Learner " << learnerID << " is already in the waiting queue for this session!" << endl;
            return;
        }

        if (getWaitingQueueSize(sessionID) < MAX_WAITING_QUEUE) {
            enqueueWaiting(sessionID, learnerID);
            cout << "✓ Learner " << learnerID << " added to waiting queue." << endl;
            displayWaitingQueue(sessionID);
        } else {
            cout << "✗ Waiting queue is also full!" << endl;
        }
        return;
    }

    // Find available activity slot and assign learner
    int activitySlot = findAvailableActivitySlot(sessionID);
    if (activitySlot == -1) {
        cout << "Error: No available activity slot!" << endl;
        return;
    }

    activeSlots[sessionID - 1][activitySlot] = learnerID;
    learner->isActive = true;
    learner->currentSessionID = sessionID;
    learner->currentActivity = activitySlot + 1; // Activity number (1-indexed)

    // Update hash maps
    learnerSessionMap[learnerID] = sessionID;
    updateProgress(learnerID, sessionID, activitySlot + 1);

    cout << "✓ Learner " << learnerID << " joined Session " << sessionID
         << " (" << task1Sessions[sessionID - 1].name << ")" << endl;
    cout << "  Assigned to Activity " << (activitySlot + 1) << "/"
         << task1Sessions[sessionID - 1].capacity << endl;
}

void exitSession() {
    int learnerID;

    cout << "\nEnter Learner ID: ";
    if (!(cin >> learnerID)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input!" << endl;
        return;
    }

    Learner* learner = task1_findLearnerByID(learnerID);
    if (learner == NULL) {
        cout << "Learner not found!" << endl;
        return;
    }

    if (!learner->isActive) {
        cout << "Learner is not in any session!" << endl;
        return;
    }

    int sessionID = learner->currentSessionID;
    int idx = sessionID - 1;

    // Find and free the activity slot
    for (int i = 0; i < task1Sessions[idx].capacity; i++) {
        if (activeSlots[idx][i] == learnerID) {
            activeSlots[idx][i] = -1;
            break;
        }
    }

    learner->isActive = false;
    learnerSessionMap.erase(learnerID);
    updateProgress(learnerID, -1, -1);

    cout << "✓ Learner " << learnerID << " exited Session " << sessionID << endl;

    // Check waiting queue and assign next learner
    if (waitingQueues[idx].count > 0) {
        int nextLearnerID = dequeueWaiting(sessionID);
        Learner* nextLearner = task1_findLearnerByID(nextLearnerID);

        if (nextLearner != NULL && !nextLearner->isActive) {
            int activitySlot = findAvailableActivitySlot(sessionID);
            if (activitySlot != -1) {
                activeSlots[idx][activitySlot] = nextLearnerID;
                nextLearner->isActive = true;
                nextLearner->currentSessionID = sessionID;
                nextLearner->currentActivity = activitySlot + 1;

                learnerSessionMap[nextLearnerID] = sessionID;
                updateProgress(nextLearnerID, sessionID, activitySlot + 1);

                cout << "✓ Learner " << nextLearnerID << " moved from waiting queue to Session "
                     << sessionID << " (Activity " << (activitySlot + 1) << ")" << endl;
            }
        }
    }
}

void displayStatus() {
    cout << "\n========== SYSTEM STATUS ==========" << endl;

    // Display all registered learners
    learnerLL.displayAllLearners();

    // Display active sessions
    cout << "\n=== ACTIVE SESSIONS ===" << endl;
    for (int i = 0; i < 5; i++) {
        cout << "\nSession " << task1Sessions[i].id << ": " << task1Sessions[i].name << endl;
        cout << "Total Activities: " << task1Sessions[i].capacity << " | Occupied: ";

        int activeCount = 0;
        for (int j = 0; j < task1Sessions[i].capacity; j++) {
            if (activeSlots[i][j] != -1) {
                activeCount++;
            }
        }
        cout << activeCount << " | Available: " << (task1Sessions[i].capacity - activeCount) << endl;

        // Show active learners per activity
        if (activeCount > 0) {
            cout << "Active Learners:" << endl;
            for (int j = 0; j < task1Sessions[i].capacity; j++) {
                if (activeSlots[i][j] != -1) {
                    Learner* l = task1_findLearnerByID(activeSlots[i][j]);
                    if (l != NULL) {
                        cout << "  Activity " << (j + 1) << ": Learner "
                             << l->id << " (" << l->name << ")" << endl;
                    }
                }
            }
        }

        // Show waiting queue
        if (waitingQueues[i].count > 0) {
            cout << "Waiting Queue (" << waitingQueues[i].count << " learners): ";
            int current = waitingQueues[i].front;
            for (int j = 0; j < waitingQueues[i].count; j++) {
                int learnerID = waitingQueues[i].learnerIDs[current];
                Learner* l = task1_findLearnerByID(learnerID);
                if (l != NULL) {
                    cout << learnerID << " (" << l->name << ") ";
                } else {
                    cout << learnerID << " ";
                }
                current = (current + 1) % MAX_WAITING_QUEUE;
            }
            cout << endl;
        }
    }

    // Display hash map statistics
    cout << "\n=== HASH MAP STATISTICS ===" << endl;
    cout << "Learner→Session Map entries: " << learnerSessionMap.size() << endl;
    cout << "Learner→Progress Map entries: " << learnerProgressMap.size() << endl;
}

void markSessionComplete(Learner* learner, int sessionID) {
    if (sessionID >= 1 && sessionID <= 5) {
        learner->completedSessions[sessionID - 1] = 1;

        // Also update progress hash map
        if (learnerProgressMap.find(learner->id) != learnerProgressMap.end()) {
            learnerProgressMap[learner->id].completedSessions[sessionID - 1] = 1;
        }

        cout << "✓ Session " << sessionID << " marked as complete for Learner "
             << learner->id << endl;
    }
}

void resumeProgress() {
    int learnerID;

    cout << "\nEnter Learner ID: ";
    if (!(cin >> learnerID)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input!" << endl;
        return;
    }

    Learner* learner = task1_findLearnerByID(learnerID);
    if (learner == NULL) {
        cout << "Learner not found!" << endl;
        return;
    }

    cout << "\n=== PROGRESS REPORT (from Hash Map) ===" << endl;
    cout << "Learner: " << learner->name << " (ID: " << learner->id << ")" << endl;

    // Get progress from hash map (O(1) lookup)
    if (learnerProgressMap.find(learnerID) != learnerProgressMap.end()) {
        ProgressInfo& progress = learnerProgressMap[learnerID];

        cout << "\nCompleted Sessions:" << endl;
        bool hasCompleted = false;
        for (int i = 0; i < 5; i++) {
            if (progress.completedSessions[i] == 1) {
                cout << "  ✓ Session " << (i + 1) << ": " << task1Sessions[i].name << endl;
                hasCompleted = true;
            }
        }

        if (!hasCompleted) {
            cout << "  No sessions completed yet." << endl;
        }

        if (learner->isActive) {
            cout << "\nCurrent Status:" << endl;
            cout << "  Active in Session " << progress.currentSessionID
                 << " (" << task1Sessions[progress.currentSessionID - 1].name << ")" << endl;
            cout << "  Current Activity: " << progress.currentActivityID << endl;
        } else {
            cout << "\nNot currently in any session." << endl;
        }

        // Option to mark session as complete (for testing)
        if (learner->isActive) {
            cout << "\nMark current session as complete? (y/n): ";
            char choice;
            cin >> choice;
            if (choice == 'y' || choice == 'Y') {
                int currentSessionID = learner->currentSessionID;

                markSessionComplete(learner, currentSessionID);

                // AUTO-EXIT: Automatically exit the session after marking complete
                int idx = currentSessionID - 1;

                // Find and free the activity slot
                for (int i = 0; i < task1Sessions[idx].capacity; i++) {
                    if (activeSlots[idx][i] == learner->id) {
                        activeSlots[idx][i] = -1;
                        break;
                    }
                }

                learner->isActive = false;
                learner->currentSessionID = -1;
                learner->currentActivity = -1;
                learnerSessionMap.erase(learner->id);
                updateProgress(learner->id, -1, -1);

                cout << "✓ Learner " << learner->id << " automatically exited Session "
                     << currentSessionID << endl;

                // Check waiting queue and assign next learner
                if (waitingQueues[idx].count > 0) {
                    int nextLearnerID = dequeueWaiting(currentSessionID);
                    Learner* nextLearner = task1_findLearnerByID(nextLearnerID);

                    if (nextLearner != NULL && !nextLearner->isActive) {
                        int activitySlot = findAvailableActivitySlot(currentSessionID);
                        if (activitySlot != -1) {
                            activeSlots[idx][activitySlot] = nextLearnerID;
                            nextLearner->isActive = true;
                            nextLearner->currentSessionID = currentSessionID;
                            nextLearner->currentActivity = activitySlot + 1;

                            learnerSessionMap[nextLearnerID] = currentSessionID;
                            updateProgress(nextLearnerID, currentSessionID, activitySlot + 1);

                            cout << "✓ Learner " << nextLearnerID
                                 << " moved from waiting queue to Session "
                                 << currentSessionID << " (Activity " << (activitySlot + 1) << ")" << endl;
                        }
                    }
                }
            }
        }
    } else {
        cout << "No progress data found in hash map." << endl;
    }
}

// ========== RESTORE SESSION STATE FROM CSV ==========
// This function restores the session state after loading learners from CSV
// It updates activeSlots and hash maps based on learners' current sessions
void task1_restoreSessionStateFromCSV() {
    Learner* temp = learnerLL.getHead();

    while (temp != NULL) {
        // If learner is marked as active in CSV, restore their session state
        if (temp->isActive && temp->currentSessionID > 0 && temp->currentSessionID <= 5) {
            int sessionIdx = temp->currentSessionID - 1;
            int activityIdx = temp->currentActivity - 1;

            // Restore active slot (assign learner to their activity)
            if (activityIdx >= 0 && activityIdx < task1Sessions[sessionIdx].capacity) {
                activeSlots[sessionIdx][activityIdx] = temp->id;
            }

            // Restore Learner→Session hash map
            learnerSessionMap[temp->id] = temp->currentSessionID;

            // Restore Learner→Progress hash map
            ProgressInfo progress;
            progress.currentSessionID = temp->currentSessionID;
            progress.currentActivityID = temp->currentActivity;
            for (int i = 0; i < 5; i++) {
                progress.completedSessions[i] = temp->completedSessions[i];
            }
            learnerProgressMap[temp->id] = progress;

            // Debug output (optional)
            // cout << "Restored: Learner " << temp->id << " in Session "
            //      << temp->currentSessionID << ", Activity " << temp->currentActivity << endl;
        }

        temp = temp->next;
    }
}