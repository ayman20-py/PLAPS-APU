#ifndef DATASTRUCTURES_H
#define DATASTRUCTURES_H

#include <iostream> 
#include <string>
#include <ctime>
using namespace std;

// ========== GLOBAL VARIABLES ==========  
extern int nextID;

// ========== LEARNER ==========
struct Learner {
    int id;
    string name;                
    int currentSessionID;       
    int currentActivity;        
    int completedSessions[5];   
    bool isActive;             
    Learner* next;          
};

// Forward declaration
struct Activity;

// ========== SESSION ==========
struct Session {
    int id;
    string name;               
    string topic;
    int capacity;
    Activity* activities[6];
};

struct Activity {
    int id; 
    string title;
    string description;
    int difficulty;
    Learner* currentLearner;
    int waitingQueue[20];  // Queue of learner IDs waiting for this activity
    int queueFront;
        int queueRear;
    int queueCount;
    
    Activity() {
        currentLearner = NULL;
        queueFront = 0;
        queueRear = -1;
        queueCount = 0;
        for(int i = 0; i < 20; i++) {
            waitingQueue[i] = -1;
        }
    }
};

// ========== WAITING QUEUE ==========
struct WaitingQueue {
    int data[20];
    int front;
    int rear;
    int count;
};

// ========== HASH MAP NODE ==========
struct MapNode {
    int learnerID;
    int sessionID;
    MapNode* next;
};

// Example of logger output => [1] 2025-03-15 14:23 | Learner 101 | S1 Act2 | Arrays | Score: 45% | ❌ FAIL | Diff: 1/5
// timestamp | Learner id |Session index, Activity index | Session name | Score : Percentage | failed | Difficulty level
struct LoggerRecord {
    int learnerID;
    int sessionID;
    int activityID;
    string topic;
    int score;
    bool failed;
    int difficulty;
    time_t timestamp;
};


class LearnerLinkedList {
    private:
        Learner* head;
        Learner* tail;
        int totalLearners;

    public:
        LearnerLinkedList() {
            head = nullptr;
            tail = nullptr;
            totalLearners = 0;
        }


        void addLearner(Learner* newLearner) {
            if (head == NULL) {
                head = newLearner;
                tail = newLearner;
                totalLearners++;
                return;
            }

            Learner* temp = head;
            while (temp->next != NULL) {
                temp = temp -> next;
            }

            temp->next = newLearner;
            tail = newLearner;
            totalLearners++;
        }

        Learner* getHead() {
            return head;
        }

        int getCount() {
            return totalLearners;
        }


        void displayAllLearners() {
            cout << "\n=== REGISTERED LEARNERS ===\n";
            
            if (head == NULL) {
                cout << "No learners.\n";
                return;
            }
            
            Learner* temp = head;
            while (temp != NULL) {
                cout << "ID: " << temp->id 
                    << " | Name: " << temp->name;
                
                if (temp->isActive) {
                    cout << " | IN SESSION " << temp->currentSessionID;
                } else {
                    cout << " | Not in session";
                }
                cout << "\n";
                
                temp = temp->next;
            }
        }


};

class SessionManagement {
    private:
    Session* sessions[5];

    public:
    // Creating all of the sessions and activities
    SessionManagement() {

        // ========== SESSION 1: ARRAYS & MEMORY ==========
        sessions[0] = createSession(1, "Arrays & Memory", "Arrays", 5);
        
        // Add activities
        sessions[0]->activities[0] = createActivity(101, "Create an Array", 
            "Declare and initialize an integer array of size 5", 1);
        sessions[0]->activities[1] = createActivity(102, "Access Elements", 
            "Print all array elements using index", 1);
        sessions[0]->activities[2] = createActivity(103, "Modify Elements", 
            "Change the value at position 2 to 99", 2);
        sessions[0]->activities[3] = createActivity(104, "Array Sum", 
            "Calculate the sum of all elements in the array", 2);
        sessions[0]->activities[4] = createActivity(105, "Find Maximum", 
            "Find the largest element in the array", 3);
        
        // ========== SESSION 2: LOOPING CONSTRUCTS ==========
        sessions[1] = createSession(2, "Looping Constructs", "Loops", 5);
        
        sessions[1]->activities[0] = createActivity(201, "For Loop Basics", 
            "Print numbers 1 to 10 using a for loop", 1);
        sessions[1]->activities[1] = createActivity(202, "While Loop", 
            "Repeat until user enters 0", 1);
        sessions[1]->activities[2] = createActivity(203, "Nested Loops", 
            "Print a multiplication table (1-5)", 2);
        sessions[1]->activities[3] = createActivity(204, "Loop with Array", 
            "Traverse an array using a loop and print each element", 2);
        sessions[1]->activities[4] = createActivity(205, "Break & Continue", 
            "Print 1-20, skip even numbers, stop at 15", 3);
        
        // ========== SESSION 3: FUNCTIONS & SCOPE ==========
        sessions[2] = createSession(3, "Functions & Scope", "Functions", 5);
        
        sessions[2]->activities[0] = createActivity(301, "Function Declaration", 
            "Define and call a function that prints \"Hello World\"", 2);
        sessions[2]->activities[1] = createActivity(302, "Parameters & Return", 
            "Write a function that adds two numbers and returns the result", 2);
        sessions[2]->activities[2] = createActivity(303, "Pass by Value", 
            "Demonstrate that the original variable doesn't change", 2);
        sessions[2]->activities[3] = createActivity(304, "Pass by Reference", 
            "Modify the original variable using references", 3);
        sessions[2]->activities[4] = createActivity(305, "Function Overloading", 
            "Create two functions with same name, different parameters", 3);
        sessions[2]->activities[5] = createActivity(306, "Scope Rules", 
            "Demonstrate local vs global variables", 3);
        
        // ========== SESSION 4: DEBUGGING WORKSHOP ==========
        sessions[3] = createSession(4, "Debugging Workshop", "Debugging", 4);
        
        sessions[3]->activities[0] = createActivity(401, "Syntax Errors", 
            "Fix missing semicolons, brackets, and typos", 2);
        sessions[3]->activities[1] = createActivity(402, "Logic Errors", 
            "Find why the sum is always incorrect", 3);
        sessions[3]->activities[2] = createActivity(403, "Runtime Errors", 
            "Handle division by zero and array out of bounds", 3);
        sessions[3]->activities[3] = createActivity(404, "Trace Execution", 
            "Step through code mentally and predict the output", 4);
        
        // ========== SESSION 5: MINI PROJECT LAB ==========
        sessions[4] = createSession(5, "Mini Project Lab", "Integration", 3);
        
        sessions[4]->activities[0] = createActivity(501, "Student Grade Calculator", 
            "Input 5 grades, calculate average, assign letter grade", 4);
        sessions[4]->activities[1] = createActivity(502, "Basic Inventory System", 
            "Store item names and quantities, search by name", 4);
        sessions[4]->activities[2] = createActivity(503, "Simple Menu Program", 
            "Create a do-while menu with at least 4 options", 4);
    }


    Session* createSession(int id, string name, string topic, int cap){
        Session* sess = new Session;  // Allocate on heap
        sess->id = id;
        sess->name = name;
        sess->topic = topic;
        sess->capacity = cap;
        
        // Initialize activity pointers to NULL
        for (int i = 0; i < 5; i++) {
            sess->activities[i] = NULL;
        }
    
        return sess;
    }

    Activity* createActivity(int id, string title, string desc, int diff) {
        Activity* act = new Activity;
        act->id = id;
        act->title = title;
        act->description = desc;
        act->difficulty = diff;
        act->currentLearner = NULL;
        return act;
    }

    void displayTest() {
        cout << sessions[1]->name << endl;
        cout << sessions[2]->name << endl;
        cout << sessions[3]->name << endl;
        cout << sessions[4]->name << endl;
    }

};

Learner* createLearner(string name) {
    Learner* newL = new Learner;

    newL->id = nextID++;
    newL->name = name;
    newL->currentSessionID = -1;
    newL->currentActivity = -1;
    newL->isActive = false;
    newL->next = NULL;

    for(int i = 0; i < 5; i++) {
        newL->completedSessions[i] = 0;
    }
    return newL;
}

#endif // DATASTRUCTURES_H
