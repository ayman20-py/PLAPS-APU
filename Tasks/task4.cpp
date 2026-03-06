/*
 * ============================================================
 *  TASK 4: AT-RISK PRIORITY & RECOMMENDATIONS
 *  Author: Omar
 *  Description: Reads activity logs from Task 3's circular
 *  buffer, calculates risk scores for learners, ranks them
 *  using a max-heap, generates recommendations, and exports
 *  CSV reports.
 * ============================================================
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <limits>

// External references to shared data.
extern LearnerLinkedList learnerLL;
extern ActivityLogBuffer activityLogBuffer;
Learner* findLearnerByID(int id);
void generateMockActivityLogs();

// ============================================================
//  NAMED CONSTANTS
// ============================================================

// Risk score component weights (must sum to 1.0).
const double WEIGHT_AVERAGE_SCORE = 0.30;
const double WEIGHT_FAILURE_RATE = 0.35;
const double WEIGHT_SCORE_VARIANCE = 0.20;
const double WEIGHT_INACTIVITY_DAYS = 0.15;

// Thresholds for recommendation rules.
const double CRITICAL_RISK_THRESHOLD = 0.75;
const double HIGH_FAILURE_RATE_THRESHOLD = 0.6;
const int LOW_AVERAGE_SCORE_THRESHOLD = 40;
const double HIGH_VARIANCE_THRESHOLD = 0.7;
const int INACTIVITY_DAYS_THRESHOLD = 14;
const double MODERATE_FAILURE_RATE_THRESHOLD = 0.4;
const int MODERATE_AVERAGE_SCORE_THRESHOLD = 60;
const double MONITOR_RISK_THRESHOLD = 0.5;

// Data structure capacities.
const int STATISTICS_MAP_CAPACITY = 127;
const int RISK_HEAP_CAPACITY = 100;
const int HEAP_INDEX_MAP_CAPACITY = 127;

// Sentinel value indicating an empty hash map slot.
const int EMPTY_SLOT_SENTINEL = -1;

// ============================================================
//  LEARNER STATISTICS (OPEN-ADDRESSING HASH MAP)
// ============================================================

// Stores aggregated statistics for a single learner.
struct LearnerStatistics {
    int learnerID;
    int totalScore;
    int totalAttempts;
    int failedAttempts;
    int scores[200];
    int scoreCount;
    time_t lastActivityTimestamp;
    bool occupied;
};

// Open-addressing hash map for learner statistics.
struct LearnerStatisticsHashMap {
    LearnerStatistics entries[STATISTICS_MAP_CAPACITY];
};

// ============================================================
//  RISK MAX-HEAP
// ============================================================

// Each entry in the max-heap represents a learner and their risk score.
struct RiskHeapEntry {
    int learnerID;
    double riskScore;
    double averageScore;
    double failureRate;
    double scoreVariance;
    int inactivityDays;
    string recommendation;
};

// Array-based max-heap sorted by risk score (highest risk at root).
struct RiskMaxHeap {
    RiskHeapEntry entries[RISK_HEAP_CAPACITY];
    int size;
};

// ============================================================
//  HEAP INDEX (OPEN-ADDRESSING HASH MAP)
// ============================================================

// Maps a learner ID to their current position in the heap array.
struct HeapIndexEntry {
    int learnerID;
    int heapPosition;
    bool occupied;
};

// Open-addressing hash map for heap index lookups.
struct HeapIndexHashMap {
    HeapIndexEntry entries[HEAP_INDEX_MAP_CAPACITY];
};

// Global instances of task 4 data structures.
LearnerStatisticsHashMap learnerStatisticsMap;
RiskMaxHeap riskMaxHeap;
HeapIndexHashMap heapIndexMap;

// ============================================================
//  HASH MAP OPERATIONS (LEARNER STATISTICS)
// ============================================================

// Computes a hash index for the given key using modular arithmetic.
int computeStatisticsHashIndex(int key, int capacity) {
    int hashValue = key % capacity;
    if (hashValue < 0) hashValue += capacity;
    return hashValue;
}

// Initializes all slots in the learner statistics hash map as empty.
void initializeLearnerStatisticsHashMap() {
    for (int i = 0; i < STATISTICS_MAP_CAPACITY; i++) {
        learnerStatisticsMap.entries[i].learnerID = EMPTY_SLOT_SENTINEL;
        learnerStatisticsMap.entries[i].totalScore = 0;
        learnerStatisticsMap.entries[i].totalAttempts = 0;
        learnerStatisticsMap.entries[i].failedAttempts = 0;
        learnerStatisticsMap.entries[i].scoreCount = 0;
        learnerStatisticsMap.entries[i].lastActivityTimestamp = 0;
        learnerStatisticsMap.entries[i].occupied = false;
    }
}

// Inserts or updates a learner's statistics in the hash map.
void insertOrUpdateLearnerStatistics(int learnerID, int score, bool failed, time_t timestamp) {
    int index = computeStatisticsHashIndex(learnerID, STATISTICS_MAP_CAPACITY);
    int startIndex = index;

    // Linear probing to find the correct slot.
    while (true) {
        if (!learnerStatisticsMap.entries[index].occupied) {
            // Empty slot found. Insert new entry.
            learnerStatisticsMap.entries[index].learnerID = learnerID;
            learnerStatisticsMap.entries[index].totalScore = score;
            learnerStatisticsMap.entries[index].totalAttempts = 1;
            learnerStatisticsMap.entries[index].failedAttempts = failed ? 1 : 0;
            learnerStatisticsMap.entries[index].scores[0] = score;
            learnerStatisticsMap.entries[index].scoreCount = 1;
            learnerStatisticsMap.entries[index].lastActivityTimestamp = timestamp;
            learnerStatisticsMap.entries[index].occupied = true;
            return;
        }
        if (learnerStatisticsMap.entries[index].learnerID == learnerID) {
            // Existing entry found. Update statistics.
            learnerStatisticsMap.entries[index].totalScore += score;
            learnerStatisticsMap.entries[index].totalAttempts++;
            if (failed) {
                learnerStatisticsMap.entries[index].failedAttempts++;
            }
            int scoreIndex = learnerStatisticsMap.entries[index].scoreCount;
            if (scoreIndex < 200) {
                learnerStatisticsMap.entries[index].scores[scoreIndex] = score;
                learnerStatisticsMap.entries[index].scoreCount++;
            }
            if (timestamp > learnerStatisticsMap.entries[index].lastActivityTimestamp) {
                learnerStatisticsMap.entries[index].lastActivityTimestamp = timestamp;
            }
            return;
        }
        index = (index + 1) % STATISTICS_MAP_CAPACITY;
        if (index == startIndex) {
            // Hash map is full. This should not happen with proper sizing.
            cout << "Warning: Learner statistics hash map is full." << endl;
            return;
        }
    }
}

// Looks up a learner's statistics by ID. Returns NULL if not found.
LearnerStatistics* lookupLearnerStatistics(int learnerID) {
    int index = computeStatisticsHashIndex(learnerID, STATISTICS_MAP_CAPACITY);
    int startIndex = index;

    while (true) {
        if (!learnerStatisticsMap.entries[index].occupied) {
            return NULL;
        }
        if (learnerStatisticsMap.entries[index].learnerID == learnerID) {
            return &learnerStatisticsMap.entries[index];
        }
        index = (index + 1) % STATISTICS_MAP_CAPACITY;
        if (index == startIndex) {
            return NULL;
        }
    }
}

// ============================================================
//  HEAP INDEX HASH MAP OPERATIONS
// ============================================================

// Initializes all slots in the heap index hash map as empty.
void initializeHeapIndexHashMap() {
    for (int i = 0; i < HEAP_INDEX_MAP_CAPACITY; i++) {
        heapIndexMap.entries[i].learnerID = EMPTY_SLOT_SENTINEL;
        heapIndexMap.entries[i].heapPosition = -1;
        heapIndexMap.entries[i].occupied = false;
    }
}

// Inserts or updates a learner's heap position in the index map.
void insertOrUpdateHeapIndex(int learnerID, int heapPosition) {
    int index = computeStatisticsHashIndex(learnerID, HEAP_INDEX_MAP_CAPACITY);
    int startIndex = index;

    while (true) {
        if (!heapIndexMap.entries[index].occupied) {
            heapIndexMap.entries[index].learnerID = learnerID;
            heapIndexMap.entries[index].heapPosition = heapPosition;
            heapIndexMap.entries[index].occupied = true;
            return;
        }
        if (heapIndexMap.entries[index].learnerID == learnerID) {
            heapIndexMap.entries[index].heapPosition = heapPosition;
            return;
        }
        index = (index + 1) % HEAP_INDEX_MAP_CAPACITY;
        if (index == startIndex) return;
    }
}

// Looks up a learner's current position in the heap. Returns -1 if not found.
int lookupHeapIndex(int learnerID) {
    int index = computeStatisticsHashIndex(learnerID, HEAP_INDEX_MAP_CAPACITY);
    int startIndex = index;

    while (true) {
        if (!heapIndexMap.entries[index].occupied) {
            return -1;
        }
        if (heapIndexMap.entries[index].learnerID == learnerID) {
            return heapIndexMap.entries[index].heapPosition;
        }
        index = (index + 1) % HEAP_INDEX_MAP_CAPACITY;
        if (index == startIndex) return -1;
    }
}

// ============================================================
//  RISK SCORE CALCULATION
// ============================================================

// Calculates the average score for a learner from their statistics.
double calculateAverageScore(LearnerStatistics* stats) {
    if (stats->totalAttempts == 0) return 0.0;
    return (double)stats->totalScore / stats->totalAttempts;
}

// Calculates the failure rate (failed attempts / total attempts).
double calculateFailureRate(LearnerStatistics* stats) {
    if (stats->totalAttempts == 0) return 0.0;
    return (double)stats->failedAttempts / stats->totalAttempts;
}

// Calculates the variance of the learner's scores.
double calculateScoreVariance(LearnerStatistics* stats) {
    if (stats->scoreCount < 2) return 0.0;
    double average = calculateAverageScore(stats);
    double sumSquaredDifferences = 0.0;
    for (int i = 0; i < stats->scoreCount; i++) {
        double difference = stats->scores[i] - average;
        sumSquaredDifferences += difference * difference;
    }
    return sumSquaredDifferences / stats->scoreCount;
}

// Calculates the number of days since the learner's last activity.
int calculateInactivityDays(LearnerStatistics* stats) {
    if (stats->lastActivityTimestamp == 0) return 30;
    time_t now = time(NULL);
    double secondsDifference = difftime(now, stats->lastActivityTimestamp);
    return (int)(secondsDifference / 86400.0);
}

// Computes the composite risk score using the weighted formula.
double calculateCompositeRiskScore(LearnerStatistics* stats) {
    double averageScore = calculateAverageScore(stats);
    double failureRate = calculateFailureRate(stats);
    double scoreVariance = calculateScoreVariance(stats);
    int inactivityDays = calculateInactivityDays(stats);

    // Component 1: Low average score contributes to risk.
    double averageScoreComponent = (100.0 - averageScore) / 100.0;

    // Component 2: High failure rate contributes to risk.
    double failureRateComponent = failureRate;

    // Component 3: High variance (inconsistent performance) contributes to risk.
    double varianceComponent = scoreVariance / 2500.0;
    if (varianceComponent > 1.0) varianceComponent = 1.0;

    // Component 4: Long inactivity contributes to risk.
    double inactivityComponent = (double)inactivityDays / 30.0;
    if (inactivityComponent > 1.0) inactivityComponent = 1.0;

    double riskScore = WEIGHT_AVERAGE_SCORE * averageScoreComponent
                     + WEIGHT_FAILURE_RATE * failureRateComponent
                     + WEIGHT_SCORE_VARIANCE * varianceComponent
                     + WEIGHT_INACTIVITY_DAYS * inactivityComponent;

    // Clamp the final score to the range [0, 1].
    if (riskScore < 0.0) riskScore = 0.0;
    if (riskScore > 1.0) riskScore = 1.0;

    return riskScore;
}

// ============================================================
//  MAX-HEAP OPERATIONS
// ============================================================

// Initializes the max-heap to an empty state.
void initializeRiskMaxHeap() {
    riskMaxHeap.size = 0;
}

// Swaps two entries in the heap and updates the heap index map.
void swapHeapEntries(int indexA, int indexB) {
    RiskHeapEntry temp = riskMaxHeap.entries[indexA];
    riskMaxHeap.entries[indexA] = riskMaxHeap.entries[indexB];
    riskMaxHeap.entries[indexB] = temp;

    // Update both entries' positions in the index map.
    insertOrUpdateHeapIndex(riskMaxHeap.entries[indexA].learnerID, indexA);
    insertOrUpdateHeapIndex(riskMaxHeap.entries[indexB].learnerID, indexB);
}

// Moves an entry upward in the heap to restore the max-heap property.
void bubbleUpHeapEntry(int index) {
    while (index > 0) {
        int parentIndex = (index - 1) / 2;
        if (riskMaxHeap.entries[index].riskScore > riskMaxHeap.entries[parentIndex].riskScore) {
            swapHeapEntries(index, parentIndex);
            index = parentIndex;
        } else {
            break;
        }
    }
}

// Moves an entry downward in the heap to restore the max-heap property.
void bubbleDownHeapEntry(int index) {
    while (true) {
        int leftChild = 2 * index + 1;
        int rightChild = 2 * index + 2;
        int largest = index;

        if (leftChild < riskMaxHeap.size &&
            riskMaxHeap.entries[leftChild].riskScore > riskMaxHeap.entries[largest].riskScore) {
            largest = leftChild;
        }
        if (rightChild < riskMaxHeap.size &&
            riskMaxHeap.entries[rightChild].riskScore > riskMaxHeap.entries[largest].riskScore) {
            largest = rightChild;
        }
        if (largest != index) {
            swapHeapEntries(index, largest);
            index = largest;
        } else {
            break;
        }
    }
}

// Inserts a new entry into the max-heap.
void insertIntoRiskHeap(RiskHeapEntry entry) {
    if (riskMaxHeap.size >= RISK_HEAP_CAPACITY) {
        cout << "Warning: Risk heap is full. Cannot insert learner " << entry.learnerID << "." << endl;
        return;
    }
    int insertIndex = riskMaxHeap.size;
    riskMaxHeap.entries[insertIndex] = entry;
    riskMaxHeap.size++;
    insertOrUpdateHeapIndex(entry.learnerID, insertIndex);
    bubbleUpHeapEntry(insertIndex);
}

// Extracts the entry with the highest risk score from the heap.
RiskHeapEntry extractHighestRisk() {
    RiskHeapEntry top = riskMaxHeap.entries[0];
    riskMaxHeap.size--;
    if (riskMaxHeap.size > 0) {
        riskMaxHeap.entries[0] = riskMaxHeap.entries[riskMaxHeap.size];
        insertOrUpdateHeapIndex(riskMaxHeap.entries[0].learnerID, 0);
        bubbleDownHeapEntry(0);
    }
    return top;
}

// Updates a learner's risk score in the heap and restores heap order.
void updateLearnerRiskInHeap(int learnerID, double newRiskScore) {
    int heapPosition = lookupHeapIndex(learnerID);
    if (heapPosition < 0 || heapPosition >= riskMaxHeap.size) return;

    double oldRiskScore = riskMaxHeap.entries[heapPosition].riskScore;
    riskMaxHeap.entries[heapPosition].riskScore = newRiskScore;

    if (newRiskScore > oldRiskScore) {
        bubbleUpHeapEntry(heapPosition);
    } else {
        bubbleDownHeapEntry(heapPosition);
    }
}

// ============================================================
//  RECOMMENDATION ENGINE
// ============================================================

// Generates a recommendation string based on the learner's risk profile.
// Rules are evaluated top-to-bottom; the first matching rule wins.
string generateRecommendation(double riskScore, double averageScore,
                              double failureRate, double varianceNormalized,
                              int inactivityDays) {
    if (riskScore >= CRITICAL_RISK_THRESHOLD) {
        return "CRITICAL: Immediate intervention required.";
    }
    if (failureRate >= HIGH_FAILURE_RATE_THRESHOLD) {
        return "Revisit foundational topics. Consider restarting from Session 1.";
    }
    if (averageScore < LOW_AVERAGE_SCORE_THRESHOLD) {
        return "Schedule one-on-one tutoring session with instructor.";
    }
    if (varianceNormalized > HIGH_VARIANCE_THRESHOLD) {
        return "Performance is inconsistent. Focus on weaker activity types.";
    }
    if (inactivityDays > INACTIVITY_DAYS_THRESHOLD) {
        return "Learner is inactive. Send engagement reminder.";
    }
    if (failureRate >= MODERATE_FAILURE_RATE_THRESHOLD && averageScore < MODERATE_AVERAGE_SCORE_THRESHOLD) {
        return "At moderate risk. Assign supplementary practice activities.";
    }
    if (riskScore >= MONITOR_RISK_THRESHOLD) {
        return "Monitor closely. Review progress weekly.";
    }
    return "On track. Continue current learning path.";
}

// ============================================================
//  ORCHESTRATION
// ============================================================

// Scans all records in the activity log buffer and populates the statistics hash map.
void processAllLogRecords() {
    initializeLearnerStatisticsHashMap();

    if (activityLogBuffer.count == 0) {
        cout << "No log records to process." << endl;
        return;
    }

    int index = activityLogBuffer.head;
    for (int i = 0; i < activityLogBuffer.count; i++) {
        LoggerRecord record = activityLogBuffer.records[index];
        insertOrUpdateLearnerStatistics(
            record.learnerID,
            record.score,
            record.failed,
            record.timestamp
        );
        index = (index + 1) % LOG_BUFFER_CAPACITY;
    }
}

// Calculates risk scores for all learners in the statistics map and builds the heap.
void buildRiskHeapFromStatistics() {
    initializeRiskMaxHeap();
    initializeHeapIndexHashMap();

    for (int i = 0; i < STATISTICS_MAP_CAPACITY; i++) {
        if (!learnerStatisticsMap.entries[i].occupied) continue;

        LearnerStatistics* stats = &learnerStatisticsMap.entries[i];
        double averageScore = calculateAverageScore(stats);
        double failureRate = calculateFailureRate(stats);
        double scoreVariance = calculateScoreVariance(stats);
        int inactivityDays = calculateInactivityDays(stats);
        double riskScore = calculateCompositeRiskScore(stats);

        // Normalize variance to [0, 1] for the recommendation engine.
        double varianceNormalized = scoreVariance / 2500.0;
        if (varianceNormalized > 1.0) varianceNormalized = 1.0;

        string recommendation = generateRecommendation(
            riskScore, averageScore, failureRate, varianceNormalized, inactivityDays
        );

        RiskHeapEntry entry;
        entry.learnerID = stats->learnerID;
        entry.riskScore = riskScore;
        entry.averageScore = averageScore;
        entry.failureRate = failureRate;
        entry.scoreVariance = scoreVariance;
        entry.inactivityDays = inactivityDays;
        entry.recommendation = recommendation;

        insertIntoRiskHeap(entry);
    }
}

// ============================================================
//  DISPLAY FUNCTIONS
// ============================================================

// Displays the top N at-risk learners without destroying the heap.
void displayTopAtRiskLearners() {
    if (riskMaxHeap.size == 0) {
        cout << "No risk data available. Run 'Update Risk Scores' first." << endl;
        return;
    }

    int topN;
    cout << "How many top at-risk learners to display? ";
    cin >> topN;
    if (topN <= 0) {
        cout << "Invalid number." << endl;
        return;
    }
    if (topN > riskMaxHeap.size) topN = riskMaxHeap.size;

    // Create a temporary copy of the heap so we can extract without destroying the original.
    RiskMaxHeap tempHeap;
    for (int i = 0; i < riskMaxHeap.size; i++) {
        tempHeap.entries[i] = riskMaxHeap.entries[i];
    }
    tempHeap.size = riskMaxHeap.size;

    cout << "\n===== TOP " << topN << " AT-RISK LEARNERS =====" << endl;
    cout << "Rank | ID   | Name            | Risk Score | Avg Score | Fail Rate | Recommendation" << endl;
    cout << "-----+------+-----------------+------------+-----------+-----------+----------------------------------" << endl;

    for (int rank = 1; rank <= topN; rank++) {
        // Extract the max from the temporary heap.
        RiskHeapEntry top = tempHeap.entries[0];
        tempHeap.size--;
        if (tempHeap.size > 0) {
            tempHeap.entries[0] = tempHeap.entries[tempHeap.size];
            // Bubble down in the temp heap.
            int idx = 0;
            while (true) {
                int left = 2 * idx + 1;
                int right = 2 * idx + 2;
                int largest = idx;
                if (left < tempHeap.size && tempHeap.entries[left].riskScore > tempHeap.entries[largest].riskScore) {
                    largest = left;
                }
                if (right < tempHeap.size && tempHeap.entries[right].riskScore > tempHeap.entries[largest].riskScore) {
                    largest = right;
                }
                if (largest != idx) {
                    RiskHeapEntry swap = tempHeap.entries[idx];
                    tempHeap.entries[idx] = tempHeap.entries[largest];
                    tempHeap.entries[largest] = swap;
                    idx = largest;
                } else {
                    break;
                }
            }
        }

        // Look up the learner's name.
        Learner* learner = findLearnerByID(top.learnerID);
        string learnerName = (learner != NULL) ? learner->name : "Unknown";

        cout << "  " << rank
             << "  | " << top.learnerID
             << "  | " << learnerName;
        // Pad the name column.
        for (int p = learnerName.length(); p < 15; p++) cout << " ";
        cout << " | " << (int)(top.riskScore * 100) << "%"
             << "        | " << (int)top.averageScore << "%"
             << "      | " << (int)(top.failureRate * 100) << "%"
             << "      | " << top.recommendation
             << endl;
    }
}

// Displays a detailed risk profile for an individual learner.
void displayIndividualRiskProfile() {
    int targetLearnerID;
    cout << "Enter Learner ID: ";
    cin >> targetLearnerID;

    LearnerStatistics* stats = lookupLearnerStatistics(targetLearnerID);
    if (stats == NULL) {
        cout << "No data found for Learner " << targetLearnerID << "." << endl;
        return;
    }

    Learner* learner = findLearnerByID(targetLearnerID);
    string learnerName = (learner != NULL) ? learner->name : "Unknown";

    double averageScore = calculateAverageScore(stats);
    double failureRate = calculateFailureRate(stats);
    double scoreVariance = calculateScoreVariance(stats);
    int inactivityDays = calculateInactivityDays(stats);
    double riskScore = calculateCompositeRiskScore(stats);
    double varianceNormalized = scoreVariance / 2500.0;
    if (varianceNormalized > 1.0) varianceNormalized = 1.0;
    string recommendation = generateRecommendation(riskScore, averageScore, failureRate, varianceNormalized, inactivityDays);

    cout << "\n===== RISK PROFILE: LEARNER " << targetLearnerID << " =====" << endl;
    cout << "Name:                " << learnerName << endl;
    cout << "Total Attempts:      " << stats->totalAttempts << endl;
    cout << "Average Score:       " << (int)averageScore << "%" << endl;
    cout << "Failure Rate:        " << (int)(failureRate * 100) << "%" << endl;
    cout << "Score Variance:      " << (int)scoreVariance << endl;
    cout << "Days Inactive:       " << inactivityDays << endl;
    cout << "Composite Risk Score:" << (int)(riskScore * 100) << "%" << endl;
    cout << endl;
    cout << "Risk Score Breakdown:" << endl;
    cout << "  Avg Score Component  (30%): " << (int)(((100.0 - averageScore) / 100.0) * 100) << "%" << endl;
    cout << "  Failure Rate Component(35%): " << (int)(failureRate * 100) << "%" << endl;
    cout << "  Variance Component   (20%): " << (int)(varianceNormalized * 100) << "%" << endl;
    cout << "  Inactivity Component (15%): " << (int)(((double)inactivityDays / 30.0 > 1.0 ? 1.0 : (double)inactivityDays / 30.0) * 100) << "%" << endl;
    cout << endl;
    cout << "Recommendation: " << recommendation << endl;
}

// ============================================================
//  CSV EXPORT
// ============================================================

// Exports the at-risk report to a timestamped CSV file.
void exportAtRiskReportToCSV() {
    if (riskMaxHeap.size == 0) {
        cout << "No risk data available. Run 'Update Risk Scores' first." << endl;
        return;
    }

    // Build a timestamped filename.
    time_t now = time(NULL);
    struct tm* timeInfo = localtime(&now);
    char filename[128];
    strftime(filename, sizeof(filename), "Dataset/risk_report_%Y%m%d_%H%M%S.csv", timeInfo);

    ofstream outFile(filename);
    if (!outFile.is_open()) {
        cout << "Error: Could not open " << filename << " for writing." << endl;
        return;
    }

    // Write header row.
    outFile << "learnerID,learnerName,riskScore,averageScore,failureRate,recommendation" << endl;

    // Create a temporary copy of the heap for non-destructive extraction.
    RiskMaxHeap tempHeap;
    for (int i = 0; i < riskMaxHeap.size; i++) {
        tempHeap.entries[i] = riskMaxHeap.entries[i];
    }
    tempHeap.size = riskMaxHeap.size;

    while (tempHeap.size > 0) {
        RiskHeapEntry top = tempHeap.entries[0];
        tempHeap.size--;
        if (tempHeap.size > 0) {
            tempHeap.entries[0] = tempHeap.entries[tempHeap.size];
            int idx = 0;
            while (true) {
                int left = 2 * idx + 1;
                int right = 2 * idx + 2;
                int largest = idx;
                if (left < tempHeap.size && tempHeap.entries[left].riskScore > tempHeap.entries[largest].riskScore) {
                    largest = left;
                }
                if (right < tempHeap.size && tempHeap.entries[right].riskScore > tempHeap.entries[largest].riskScore) {
                    largest = right;
                }
                if (largest != idx) {
                    RiskHeapEntry swap = tempHeap.entries[idx];
                    tempHeap.entries[idx] = tempHeap.entries[largest];
                    tempHeap.entries[largest] = swap;
                    idx = largest;
                } else {
                    break;
                }
            }
        }

        Learner* learner = findLearnerByID(top.learnerID);
        string learnerName = (learner != NULL) ? learner->name : "Unknown";

        outFile << top.learnerID << ","
                << learnerName << ","
                << (int)(top.riskScore * 100) << "%,"
                << (int)top.averageScore << "%,"
                << (int)(top.failureRate * 100) << "%,"
                << "\"" << top.recommendation << "\""
                << endl;
    }

    outFile.close();
    cout << "Risk report exported to " << filename << " successfully." << endl;
}

// ============================================================
//  MENU AND ENTRY POINT
// ============================================================

// Main entry point for the Risk Engine module.
void initializeRiskEngine() {
    // If the activity log buffer is empty, generate mock data so we have something to analyze.
    if (activityLogBuffer.count == 0) {
        cout << "Activity log buffer is empty. Generating mock data..." << endl;
        generateMockActivityLogs();
    }

    // Process all log records and build the risk heap.
    processAllLogRecords();
    buildRiskHeapFromStatistics();
    cout << "Risk engine initialized. " << riskMaxHeap.size << " learners analyzed." << endl;

    int choice;
    while (true) {
        cout << "\n===== RISK ENGINE MENU =====" << endl;
        cout << "1. View At-Risk Learners (Top N)" << endl;
        cout << "2. View Individual Learner Risk Profile" << endl;
        cout << "3. Update Risk Scores (Recalculate All)" << endl;
        cout << "4. Export At-Risk Report to CSV" << endl;
        cout << "5. Return to Main Menu" << endl;
        cout << "Choice: ";

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a number." << endl;
            continue;
        }

        switch (choice) {
            case 1:
                displayTopAtRiskLearners();
                break;
            case 2:
                displayIndividualRiskProfile();
                break;
            case 3:
                processAllLogRecords();
                buildRiskHeapFromStatistics();
                cout << "Risk scores recalculated. " << riskMaxHeap.size << " learners analyzed." << endl;
                break;
            case 4:
                exportAtRiskReportToCSV();
                break;
            case 5:
                return;
            default:
                cout << "Invalid choice. Please try again." << endl;
        }
    }
}
