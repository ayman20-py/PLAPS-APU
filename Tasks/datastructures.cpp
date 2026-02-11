#include <iostream> 
using namespace std;

// Singly Linked list
struct Student {
    int id;
    string name;
    int currentSessionID;  // -1 = not in session
    int currentActivity;
    bool isActive;
    StudentStats stats;
    Student* next;
};

struct Session {
    int id;
    string name;
    string topic;
    int capacity;
    int activeCount;
    Student* activeSlots[5];  // fixed capacity
    Activity activities[];
    // Waiting queue is GLOBAL, not per session (simpler)
};

struct Activity { 
    int id;
    string title;
    string desciption;
    Session* session;
};

struct StudentStats {
    int learnerID;
    int totalAttempts;
    int failedAttempts;
    float totalScore;
    float avgScore;
    float failureRate;
    int sessionsCompleted;
    int currentSession;
    int scoresByTopic[][2]; // topicID → [total, count]
};