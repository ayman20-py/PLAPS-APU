/*
 * ============================================================
 *  TASK 3: ACTIVITY LOGGING (CIRCULAR BUFFER)
 *  Author: Hasan
 *  Description: Stores activity log records in a fixed-size
 *  circular buffer. When the buffer is full, the oldest
 *  record gets overwritten automatically. Supports viewing,
 *  filtering by learner ID, exporting to file, and providing
 *  log data to Task 4 for risk analysis.
 * ============================================================
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <limits>
#include "datastructures.h"

// External references to shared data from main.cpp and datastructures.h.
extern LearnerLinkedList learnerLL;
Learner* findLearnerByID(int id);
void notifyRiskEngineOfLogChange(const LoggerRecord* overwrittenRecord, const LoggerRecord& newRecord);

// LOG_BUFFER_CAPACITY and ActivityLogBuffer are defined in datastructures.h.

// ============================================================
//  LEARNER LOG INDEX (SEPARATE-CHAINING HASH MAP)
// ============================================================

const int LEARNER_INDEX_MAP_SIZE = 53;
const int MAX_LOG_ENTRIES_PER_LEARNER = LOG_BUFFER_CAPACITY;

// Each node stores the buffer indices where a particular learner's records live.
struct LogIndexNode {
    int bufferIndices[MAX_LOG_ENTRIES_PER_LEARNER];
    int count;
    int learnerID;
    LogIndexNode* next;
};

// Separate-chaining hash map that maps learner IDs to their buffer indices.
struct LearnerLogIndexMap {
    LogIndexNode* buckets[LEARNER_INDEX_MAP_SIZE];
};

// Global instances.
ActivityLogBuffer activityLogBuffer;
LearnerLogIndexMap learnerLogIndex;
bool activityLoggerInitialized = false;

// ============================================================
//  INITIALIZATION FUNCTIONS
// ============================================================

// Resets the circular buffer to an empty state.
void initializeActivityLogBuffer() {
    activityLogBuffer.head = 0;
    activityLogBuffer.tail = 0;
    activityLogBuffer.count = 0;
}

// Clears all buckets in the learner log index hash map.
void initializeLearnerLogIndex() {
    for (int i = 0; i < LEARNER_INDEX_MAP_SIZE; i++) {
        learnerLogIndex.buckets[i] = nullptr;
    }
}

// Initializes Task 3 state once without wiping existing runtime logs later.
void ensureActivityLoggerInitialized() {
    if (activityLoggerInitialized) {
        return;
    }

    initializeActivityLogBuffer();
    initializeLearnerLogIndex();
    activityLoggerInitialized = true;
}

// ============================================================
//  HASH MAP OPERATIONS
// ============================================================

// Computes a hash index for the given learner ID.
int computeLearnerLogHash(int learnerID) {
    int hashValue = learnerID % LEARNER_INDEX_MAP_SIZE;
    if (hashValue < 0) hashValue += LEARNER_INDEX_MAP_SIZE;
    return hashValue;
}

// Finds the index node for a given learner ID, or returns nullptr if not found.
LogIndexNode* findLearnerIndexNode(int learnerID) {
    int bucket = computeLearnerLogHash(learnerID);
    LogIndexNode* current = learnerLogIndex.buckets[bucket];
    while (current != nullptr) {
        if (current->learnerID == learnerID) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

// Adds a buffer index to the learner's index node. Creates the node if needed.
void addToLearnerIndex(int learnerID, int bufferIndex) {
    LogIndexNode* node = findLearnerIndexNode(learnerID);
    if (node == nullptr) {
        // Create a new node for this learner.
        node = new LogIndexNode;
        node->learnerID = learnerID;
        node->count = 0;
        node->next = nullptr;
        for (int i = 0; i < MAX_LOG_ENTRIES_PER_LEARNER; i++) {
            node->bufferIndices[i] = -1;
        }
        // Insert at the head of the bucket chain.
        int bucket = computeLearnerLogHash(learnerID);
        node->next = learnerLogIndex.buckets[bucket];
        learnerLogIndex.buckets[bucket] = node;
    }
    if (node->count < MAX_LOG_ENTRIES_PER_LEARNER) {
        node->bufferIndices[node->count] = bufferIndex;
        node->count++;
    }
}

// Removes a specific buffer index from a learner's index node.
void removeFromLearnerIndex(int learnerID, int bufferIndex) {
    LogIndexNode* node = findLearnerIndexNode(learnerID);
    if (node == nullptr) return;
    for (int i = 0; i < node->count; i++) {
        if (node->bufferIndices[i] == bufferIndex) {
            // Shift remaining entries left to fill the gap.
            for (int j = i; j < node->count - 1; j++) {
                node->bufferIndices[j] = node->bufferIndices[j + 1];
            }
            node->count--;
            node->bufferIndices[node->count] = -1;
            return;
        }
    }
}

// ============================================================
//  CIRCULAR BUFFER OPERATIONS
// ============================================================

// Adds a new activity log record to the circular buffer.
void addActivityLogRecord(int learnerID, int sessionID, int activityID,
                          string topic, int score, bool failed, int difficulty) {
    ensureActivityLoggerInitialized();

    int insertIndex = activityLogBuffer.tail;
    LoggerRecord overwrittenRecord;
    LoggerRecord* overwrittenRecordPtr = nullptr;

    // If the buffer is full, the oldest record at head gets overwritten.
    if (activityLogBuffer.count == LOG_BUFFER_CAPACITY) {
        // Remove the old record's index entry before overwriting.
        overwrittenRecord = activityLogBuffer.records[activityLogBuffer.head];
        overwrittenRecordPtr = &overwrittenRecord;
        int oldLearnerID = overwrittenRecord.learnerID;
        removeFromLearnerIndex(oldLearnerID, activityLogBuffer.head);
        activityLogBuffer.head = (activityLogBuffer.head + 1) % LOG_BUFFER_CAPACITY;
    } else {
        activityLogBuffer.count++;
    }

    // Write the new record into the buffer.
    activityLogBuffer.records[insertIndex].learnerID = learnerID;
    activityLogBuffer.records[insertIndex].sessionID = sessionID;
    activityLogBuffer.records[insertIndex].activityID = activityID;
    activityLogBuffer.records[insertIndex].topic = topic;
    activityLogBuffer.records[insertIndex].score = score;
    activityLogBuffer.records[insertIndex].failed = failed;
    activityLogBuffer.records[insertIndex].difficulty = difficulty;
    activityLogBuffer.records[insertIndex].timestamp = time(nullptr);

    // Add the new index entry for this learner.
    addToLearnerIndex(learnerID, insertIndex);

    // Advance the tail pointer.
    activityLogBuffer.tail = (activityLogBuffer.tail + 1) % LOG_BUFFER_CAPACITY;

    notifyRiskEngineOfLogChange(overwrittenRecordPtr, activityLogBuffer.records[insertIndex]);
}

// ============================================================
//  DISPLAY AND FORMATTING
// ============================================================

// Formats a time_t timestamp into a human-readable string.
string formatTimestamp(time_t timestamp) {
    struct tm* timeInfo = localtime(&timestamp);
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", timeInfo);
    return string(buffer);
}

// Builds one formatted log line for display or export using a string buffer.
string buildLogRecordLine(const LoggerRecord& record, int displayIndex) {
    ostringstream lineBuffer;
    lineBuffer << "[" << displayIndex << "] "
               << formatTimestamp(record.timestamp)
               << " | Learner " << record.learnerID
               << " | S" << record.sessionID
               << " Act" << record.activityID
               << " | " << record.topic
               << " | Score: " << record.score << "%"
               << " | " << (record.failed ? "FAIL" : "PASS")
               << " | Diff: " << record.difficulty << "/5";
    return lineBuffer.str();
}

// Displays a single log record with a display index number.
void displaySingleLogRecord(const LoggerRecord& record, int displayIndex) {
    cout << buildLogRecordLine(record, displayIndex) << endl;
}

// Displays all log records currently in the circular buffer.
void viewAllActivityLogs() {
    ensureActivityLoggerInitialized();

    if (activityLogBuffer.count == 0) {
        cout << "No activity logs recorded yet." << endl;
        return;
    }

    cout << "\n===== ALL ACTIVITY LOGS (" << activityLogBuffer.count << " records) =====" << endl;
    int index = activityLogBuffer.head;
    for (int i = 0; i < activityLogBuffer.count; i++) {
        displaySingleLogRecord(activityLogBuffer.records[index], i + 1);
        index = (index + 1) % LOG_BUFFER_CAPACITY;
    }
}

// Filters and displays log records for a specific learner ID.
void filterLogsByLearnerID() {
    ensureActivityLoggerInitialized();

    int targetLearnerID;
    cout << "Enter Learner ID to filter: ";
    cin >> targetLearnerID;

    LogIndexNode* node = findLearnerIndexNode(targetLearnerID);
    if (node == nullptr || node->count == 0) {
        cout << "No logs found for Learner " << targetLearnerID << "." << endl;
        return;
    }

    Learner* learner = findLearnerByID(targetLearnerID);
    string learnerName = (learner != nullptr) ? learner->name : "Unknown";

    cout << "\n===== LOGS FOR LEARNER " << targetLearnerID
         << " (" << learnerName << ") - " << node->count << " records =====" << endl;

    for (int i = 0; i < node->count; i++) {
        int bufferIndex = node->bufferIndices[i];
        displaySingleLogRecord(activityLogBuffer.records[bufferIndex], i + 1);
    }
}

// ============================================================
//  EXPORT TO FILE
// ============================================================

// Builds the full export text in memory before writing it to disk.
string buildActivityLogExportText(time_t exportTimestamp) {
    ostringstream exportBuffer;

    exportBuffer << "===== ACTIVITY LOG EXPORT =====" << '\n';
    exportBuffer << "Exported at: " << formatTimestamp(exportTimestamp) << '\n';
    exportBuffer << "Total records: " << activityLogBuffer.count << '\n';
    exportBuffer << "===============================" << '\n' << '\n';

    int index = activityLogBuffer.head;
    for (int i = 0; i < activityLogBuffer.count; i++) {
        exportBuffer << buildLogRecordLine(activityLogBuffer.records[index], i + 1) << '\n';
        index = (index + 1) % LOG_BUFFER_CAPACITY;
    }

    return exportBuffer.str();
}

// Exports all activity logs to a timestamped text file.
void exportActivityLogsToFile() {
    ensureActivityLoggerInitialized();

    if (activityLogBuffer.count == 0) {
        cout << "No logs to export." << endl;
        return;
    }

    // Build a timestamped filename.
    time_t now = time(nullptr);
    struct tm* timeInfo = localtime(&now);
    char filename[128];
    strftime(filename, sizeof(filename), "logs_%Y%m%d_%H%M%S.txt", timeInfo);

    ofstream outFile(filename);
    if (!outFile.is_open()) {
        cout << "Error: Could not open " << filename << " for writing." << endl;
        return;
    }

    string exportText = buildActivityLogExportText(now);
    outFile << exportText;

    outFile.close();
    cout << "Logs exported to " << filename << " successfully." << endl;
}

// ============================================================
//  MOCK DATA GENERATION
// ============================================================

// Session topic names used for generating realistic mock data.
static const char* mockSessionTopics[] = {
    "Arrays", "Loops", "Functions", "Debugging", "Integration"
};

// Number of activities per session.
static const int mockActivityCounts[] = { 5, 5, 6, 4, 3 };

// Difficulty levels for each activity in each session.
static const int mockActivityDifficulties[][6] = {
    {1, 1, 2, 2, 3, 0},  // Session 1: Arrays & Memory.
    {1, 1, 2, 2, 3, 0},  // Session 2: Looping Constructs.
    {2, 2, 2, 3, 3, 3},  // Session 3: Functions & Scope.
    {2, 3, 3, 4, 0, 0},  // Session 4: Debugging Workshop.
    {4, 4, 4, 0, 0, 0}   // Session 5: Mini Project Lab.
};

// Generates randomized mock activity log data using registered learners.
void generateMockActivityLogs() {
    ensureActivityLoggerInitialized();

    srand((unsigned int)time(nullptr));

    Learner* current = learnerLL.getHead();
    if (current == nullptr) {
        cout << "No learners registered. Cannot generate mock logs." << endl;
        return;
    }

    // Collect all learner IDs into a fixed-size array.
    int learnerIDs[100];
    int learnerCount = 0;
    while (current != nullptr && learnerCount < 100) {
        learnerIDs[learnerCount] = current->id;
        learnerCount++;
        current = current->next;
    }

    // Generate between 30 and 50 mock log records.
    int recordCount = 30 + (rand() % 21);
    for (int i = 0; i < recordCount; i++) {
        int learnerID = learnerIDs[rand() % learnerCount];
        int sessionIndex = rand() % 5;
        int activityIndex = rand() % mockActivityCounts[sessionIndex];

        int sessionID = sessionIndex + 1;
        int activityID = (sessionID * 100) + activityIndex + 1;
        string topic = mockSessionTopics[sessionIndex];
        int difficulty = mockActivityDifficulties[sessionIndex][activityIndex];

        // Generate a score influenced by difficulty. Higher difficulty means lower average.
        int baseScore = 80 - (difficulty * 10);
        int score = baseScore + (rand() % 41) - 20;
        if (score < 0) score = 0;
        if (score > 100) score = 100;

        bool failed = (score < 50);

        addActivityLogRecord(learnerID, sessionID, activityID, topic, score, failed, difficulty);
    }

    cout << "Generated " << recordCount << " mock activity log records." << endl;
}

// Returns a pointer to the global activity log buffer.
ActivityLogBuffer* getActivityLogBuffer() {
    ensureActivityLoggerInitialized();
    return &activityLogBuffer;
}

// Returns the current number of records in the log buffer.
int getActivityLogCount() {
    ensureActivityLoggerInitialized();
    return activityLogBuffer.count;
}

// Returns the current head index of the log buffer.
int getActivityLogHead() {
    ensureActivityLoggerInitialized();
    return activityLogBuffer.head;
}

// ============================================================
//  MENU AND ENTRY POINT
// ============================================================

// Main entry point for the Activity Logger module.
void initializeActivityLogger() {
    ensureActivityLoggerInitialized();

    int choice;
    while (true) {
        cout << "\n===== ACTIVITY LOGGER MENU =====" << endl;
        cout << "1. View All Activity Logs" << endl;
        cout << "2. Filter Logs by Learner ID" << endl;
        cout << "3. Export Logs to File" << endl;
        cout << "4. Return to Main Menu" << endl;
        cout << "Choice: ";

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a number." << endl;
            continue;
        }

        switch (choice) {
            case 1:
                viewAllActivityLogs();
                break;
            case 2:
                filterLogsByLearnerID();
                break;
            case 3:
                exportActivityLogsToFile();
                break;
            case 4:
                return;
            default:
                cout << "Invalid choice. Please try again." << endl;
        }
    }
}
