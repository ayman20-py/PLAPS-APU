#include <iostream> 
#include <string>
using namespace std;


// ========== GLOBAL VARIABLES ==========  
int nextID = 1;

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

// ========== SESSION ==========
struct Session {
    int id;
    string name;               
    string topic;
    int capacity;
    int activeCount;
    Learner* activeSlots[5];
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
                return;
            }

            Learner* temp = head;
            while (temp->next != NULL) {
                temp = temp -> next;
            }

            temp->next = newLearner;
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
class SessionManagement {

    public: 

    void initSessions() { 
        
    }
};