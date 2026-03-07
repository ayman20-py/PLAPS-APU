/*
 * ============================================================
 *  TASK 4: AT-RISK PRIORITY & RECOMMENDATIONS
 *  Author: Omar
 *  Description: Reads activity logs from Task 3's circular
 *  buffer, calculates risk scores for learners, ranks them
 *  using a max-heap, updates dynamically when new logs arrive,
 *  generates rule-based recommendations, and exports CSV reports.
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
#include "datastructures.h"

// External references to shared data.
extern LearnerLinkedList learnerLL;
extern ActivityLogBuffer activityLogBuffer;
Learner* findLearnerByID(int id);

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
const int RISK_SCORE_CACHE_CAPACITY = 127;
const int RECOMMENDATION_RULE_COUNT = 7;

// Sentinel values.
const int EMPTY_SLOT_SENTINEL = -1;
const int NO_HEAP_POSITION = -1;

// ============================================================
//  LEARNER STATISTICS (OPEN-ADDRESSING HASH MAP)
// ============================================================

// Stores aggregated statistics for a single learner.
struct LearnerStatistics {
    int learnerID;
    int totalScore;
    int totalAttempts;
    int failedAttempts;
    int scores[LOG_BUFFER_CAPACITY];
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

// ============================================================
//  RISK SCORE CACHE (OPEN-ADDRESSING HASH MAP)
// ============================================================

// Caches the latest computed risk profile for each learner.
struct RiskScoreCacheEntry {
    int learnerID;
    double riskScore;
    double averageScore;
    double failureRate;
    double scoreVariance;
    int inactivityDays;
    string recommendation;
    bool occupied;
    bool hasCurrentData;
};

// Open-addressing hash map for cached risk profiles.
struct RiskScoreCacheHashMap {
    RiskScoreCacheEntry entries[RISK_SCORE_CACHE_CAPACITY];
};

// ============================================================
//  RECOMMENDATION RULES (ARRAY OF STRUCTS)
// ============================================================

enum RecommendationRuleType {
    RULE_CRITICAL_RISK,
    RULE_HIGH_FAILURE,
    RULE_LOW_AVERAGE,
    RULE_HIGH_VARIANCE,
    RULE_INACTIVITY,
    RULE_MODERATE_COMBO,
    RULE_MONITOR
};

struct RecommendationRule {
    RecommendationRuleType type;
    const char* message;
};

const RecommendationRule recommendationRules[RECOMMENDATION_RULE_COUNT] = {
    {RULE_CRITICAL_RISK, "CRITICAL: Immediate intervention required."},
    {RULE_HIGH_FAILURE, "Revisit foundational topics. Consider restarting from Session 1."},
    {RULE_LOW_AVERAGE, "Schedule one-on-one tutoring session with instructor."},
    {RULE_HIGH_VARIANCE, "Performance is inconsistent. Focus on weaker activity types."},
    {RULE_INACTIVITY, "Learner is inactive. Send engagement reminder."},
    {RULE_MODERATE_COMBO, "At moderate risk. Assign supplementary practice activities."},
    {RULE_MONITOR, "Monitor closely. Review progress weekly."}
};

// Global instances of Task 4 data structures.
LearnerStatisticsHashMap learnerStatisticsMap;
RiskMaxHeap riskMaxHeap;
HeapIndexHashMap heapIndexMap;
RiskScoreCacheHashMap riskScoreCache;
bool riskEngineInitialized = false;

// ============================================================
//  COMMON HELPERS
// ============================================================

// Computes a hash index for the given key using modular arithmetic.
int computeHashIndex(int key, int capacity) {
    int hashValue = key % capacity;
    if (hashValue < 0) hashValue += capacity;
    return hashValue;
}

// Clears the aggregated values for a learner statistics entry.
void clearLearnerStatisticsData(LearnerStatistics& entry) {
    entry.totalScore = 0;
    entry.totalAttempts = 0;
    entry.failedAttempts = 0;
    entry.scoreCount = 0;
    entry.lastActivityTimestamp = 0;

    for (int i = 0; i < LOG_BUFFER_CAPACITY; i++) {
        entry.scores[i] = 0;
    }
}

// Initializes Task 4 structures once so they can react to new log data immediately.
void ensureRiskEngineInitialized() {
    if (riskEngineInitialized) {
        return;
    }

    for (int i = 0; i < STATISTICS_MAP_CAPACITY; i++) {
        learnerStatisticsMap.entries[i].learnerID = EMPTY_SLOT_SENTINEL;
        learnerStatisticsMap.entries[i].occupied = false;
        clearLearnerStatisticsData(learnerStatisticsMap.entries[i]);
    }

    riskMaxHeap.size = 0;

    for (int i = 0; i < HEAP_INDEX_MAP_CAPACITY; i++) {
        heapIndexMap.entries[i].learnerID = EMPTY_SLOT_SENTINEL;
        heapIndexMap.entries[i].heapPosition = NO_HEAP_POSITION;
        heapIndexMap.entries[i].occupied = false;
    }

    for (int i = 0; i < RISK_SCORE_CACHE_CAPACITY; i++) {
        riskScoreCache.entries[i].learnerID = EMPTY_SLOT_SENTINEL;
        riskScoreCache.entries[i].riskScore = 0.0;
        riskScoreCache.entries[i].averageScore = 0.0;
        riskScoreCache.entries[i].failureRate = 0.0;
        riskScoreCache.entries[i].scoreVariance = 0.0;
        riskScoreCache.entries[i].inactivityDays = 0;
        riskScoreCache.entries[i].recommendation = "";
        riskScoreCache.entries[i].occupied = false;
        riskScoreCache.entries[i].hasCurrentData = false;
    }

    riskEngineInitialized = true;
}

// ============================================================
//  HASH MAP OPERATIONS (LEARNER STATISTICS)
// ============================================================

// Initializes all slots in the learner statistics hash map as empty.
void initializeLearnerStatisticsHashMap() {
    ensureRiskEngineInitialized();

    for (int i = 0; i < STATISTICS_MAP_CAPACITY; i++) {
        learnerStatisticsMap.entries[i].learnerID = EMPTY_SLOT_SENTINEL;
        learnerStatisticsMap.entries[i].occupied = false;
        clearLearnerStatisticsData(learnerStatisticsMap.entries[i]);
    }
}

// Looks up a learner's statistics by ID. Returns nullptr if not found.
LearnerStatistics* lookupLearnerStatistics(int learnerID) {
    ensureRiskEngineInitialized();

    int index = computeHashIndex(learnerID, STATISTICS_MAP_CAPACITY);
    int startIndex = index;

    while (true) {
        if (!learnerStatisticsMap.entries[index].occupied) {
            return nullptr;
        }
        if (learnerStatisticsMap.entries[index].learnerID == learnerID) {
            return &learnerStatisticsMap.entries[index];
        }
        index = (index + 1) % STATISTICS_MAP_CAPACITY;
        if (index == startIndex) {
            return nullptr;
        }
    }
}

// Finds an existing learner statistics entry or creates a new one.
LearnerStatistics* findOrCreateLearnerStatistics(int learnerID) {
    ensureRiskEngineInitialized();

    int index = computeHashIndex(learnerID, STATISTICS_MAP_CAPACITY);
    int startIndex = index;

    while (true) {
        if (!learnerStatisticsMap.entries[index].occupied) {
            learnerStatisticsMap.entries[index].learnerID = learnerID;
            learnerStatisticsMap.entries[index].occupied = true;
            clearLearnerStatisticsData(learnerStatisticsMap.entries[index]);
            return &learnerStatisticsMap.entries[index];
        }
        if (learnerStatisticsMap.entries[index].learnerID == learnerID) {
            return &learnerStatisticsMap.entries[index];
        }
        index = (index + 1) % STATISTICS_MAP_CAPACITY;
        if (index == startIndex) {
            cout << "Warning: Learner statistics hash map is full." << endl;
            return nullptr;
        }
    }
}

// Appends a log record's values into a learner's aggregated statistics.
void appendRecordToLearnerStatistics(LearnerStatistics* stats, const LoggerRecord& record) {
    if (stats == nullptr) {
        return;
    }

    stats->totalScore += record.score;
    stats->totalAttempts++;
    if (record.failed) {
        stats->failedAttempts++;
    }
    if (stats->scoreCount < LOG_BUFFER_CAPACITY) {
        stats->scores[stats->scoreCount] = record.score;
        stats->scoreCount++;
    }
    if (record.timestamp > stats->lastActivityTimestamp) {
        stats->lastActivityTimestamp = record.timestamp;
    }
}

// Rebuilds one learner's statistics from the current circular buffer contents.
void rebuildLearnerStatisticsFromLogs(int learnerID) {
    ensureRiskEngineInitialized();

    LearnerStatistics* stats = findOrCreateLearnerStatistics(learnerID);
    if (stats == nullptr) {
        return;
    }

    clearLearnerStatisticsData(*stats);

    int index = activityLogBuffer.head;
    for (int i = 0; i < activityLogBuffer.count; i++) {
        const LoggerRecord& record = activityLogBuffer.records[index];
        if (record.learnerID == learnerID) {
            appendRecordToLearnerStatistics(stats, record);
        }
        index = (index + 1) % LOG_BUFFER_CAPACITY;
    }
}

// ============================================================
//  HEAP INDEX HASH MAP OPERATIONS
// ============================================================

// Initializes all slots in the heap index hash map as empty.
void initializeHeapIndexHashMap() {
    ensureRiskEngineInitialized();

    for (int i = 0; i < HEAP_INDEX_MAP_CAPACITY; i++) {
        heapIndexMap.entries[i].learnerID = EMPTY_SLOT_SENTINEL;
        heapIndexMap.entries[i].heapPosition = NO_HEAP_POSITION;
        heapIndexMap.entries[i].occupied = false;
    }
}

// Inserts or updates a learner's heap position in the index map.
void insertOrUpdateHeapIndex(int learnerID, int heapPosition) {
    int index = computeHashIndex(learnerID, HEAP_INDEX_MAP_CAPACITY);
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
        if (index == startIndex) {
            return;
        }
    }
}

// Looks up a learner's current position in the heap. Returns -1 if not found.
int lookupHeapIndex(int learnerID) {
    int index = computeHashIndex(learnerID, HEAP_INDEX_MAP_CAPACITY);
    int startIndex = index;

    while (true) {
        if (!heapIndexMap.entries[index].occupied) {
            return NO_HEAP_POSITION;
        }
        if (heapIndexMap.entries[index].learnerID == learnerID) {
            return heapIndexMap.entries[index].heapPosition;
        }
        index = (index + 1) % HEAP_INDEX_MAP_CAPACITY;
        if (index == startIndex) {
            return NO_HEAP_POSITION;
        }
    }
}

// ============================================================
//  RISK SCORE CACHE HASH MAP OPERATIONS
// ============================================================

// Initializes all slots in the risk score cache hash map as empty.
void initializeRiskScoreCacheHashMap() {
    ensureRiskEngineInitialized();

    for (int i = 0; i < RISK_SCORE_CACHE_CAPACITY; i++) {
        riskScoreCache.entries[i].learnerID = EMPTY_SLOT_SENTINEL;
        riskScoreCache.entries[i].riskScore = 0.0;
        riskScoreCache.entries[i].averageScore = 0.0;
        riskScoreCache.entries[i].failureRate = 0.0;
        riskScoreCache.entries[i].scoreVariance = 0.0;
        riskScoreCache.entries[i].inactivityDays = 0;
        riskScoreCache.entries[i].recommendation = "";
        riskScoreCache.entries[i].occupied = false;
        riskScoreCache.entries[i].hasCurrentData = false;
    }
}

// Looks up a learner's cached risk profile. Returns nullptr if unavailable.
RiskScoreCacheEntry* lookupRiskScoreCache(int learnerID) {
    ensureRiskEngineInitialized();

    int index = computeHashIndex(learnerID, RISK_SCORE_CACHE_CAPACITY);
    int startIndex = index;

    while (true) {
        if (!riskScoreCache.entries[index].occupied) {
            return nullptr;
        }
        if (riskScoreCache.entries[index].learnerID == learnerID) {
            if (riskScoreCache.entries[index].hasCurrentData) {
                return &riskScoreCache.entries[index];
            }
            return nullptr;
        }
        index = (index + 1) % RISK_SCORE_CACHE_CAPACITY;
        if (index == startIndex) {
            return nullptr;
        }
    }
}

// Inserts or updates the cached risk profile for a learner.
void insertOrUpdateRiskScoreCache(const RiskHeapEntry& entry) {
    int index = computeHashIndex(entry.learnerID, RISK_SCORE_CACHE_CAPACITY);
    int startIndex = index;

    while (true) {
        if (!riskScoreCache.entries[index].occupied ||
            riskScoreCache.entries[index].learnerID == entry.learnerID) {
            riskScoreCache.entries[index].learnerID = entry.learnerID;
            riskScoreCache.entries[index].riskScore = entry.riskScore;
            riskScoreCache.entries[index].averageScore = entry.averageScore;
            riskScoreCache.entries[index].failureRate = entry.failureRate;
            riskScoreCache.entries[index].scoreVariance = entry.scoreVariance;
            riskScoreCache.entries[index].inactivityDays = entry.inactivityDays;
            riskScoreCache.entries[index].recommendation = entry.recommendation;
            riskScoreCache.entries[index].occupied = true;
            riskScoreCache.entries[index].hasCurrentData = true;
            return;
        }
        index = (index + 1) % RISK_SCORE_CACHE_CAPACITY;
        if (index == startIndex) {
            return;
        }
    }
}

// Marks a cached learner profile as stale when they no longer appear in the log buffer.
void invalidateRiskScoreCache(int learnerID) {
    int index = computeHashIndex(learnerID, RISK_SCORE_CACHE_CAPACITY);
    int startIndex = index;

    while (true) {
        if (!riskScoreCache.entries[index].occupied) {
            return;
        }
        if (riskScoreCache.entries[index].learnerID == learnerID) {
            riskScoreCache.entries[index].hasCurrentData = false;
            riskScoreCache.entries[index].recommendation = "";
            return;
        }
        index = (index + 1) % RISK_SCORE_CACHE_CAPACITY;
        if (index == startIndex) {
            return;
        }
    }
}

// ============================================================
//  RISK SCORE CALCULATION
// ============================================================

// Calculates the average score for a learner from their statistics.
double calculateAverageScore(LearnerStatistics* stats) {
    if (stats == nullptr || stats->totalAttempts == 0) return 0.0;
    return (double)stats->totalScore / stats->totalAttempts;
}

// Calculates the failure rate (failed attempts / total attempts).
double calculateFailureRate(LearnerStatistics* stats) {
    if (stats == nullptr || stats->totalAttempts == 0) return 0.0;
    return (double)stats->failedAttempts / stats->totalAttempts;
}

// Calculates the variance of the learner's scores.
double calculateScoreVariance(LearnerStatistics* stats) {
    if (stats == nullptr || stats->scoreCount < 2) return 0.0;

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
    if (stats == nullptr || stats->lastActivityTimestamp == 0) return 30;

    time_t now = time(nullptr);
    double secondsDifference = difftime(now, stats->lastActivityTimestamp);
    return (int)(secondsDifference / 86400.0);
}

// Normalizes score variance to the range [0, 1].
double normalizeScoreVariance(double scoreVariance) {
    double varianceNormalized = scoreVariance / 2500.0;
    if (varianceNormalized > 1.0) varianceNormalized = 1.0;
    if (varianceNormalized < 0.0) varianceNormalized = 0.0;
    return varianceNormalized;
}

// Computes the composite risk score using the weighted formula.
double calculateCompositeRiskScore(LearnerStatistics* stats) {
    double averageScore = calculateAverageScore(stats);
    double failureRate = calculateFailureRate(stats);
    double scoreVariance = calculateScoreVariance(stats);
    int inactivityDays = calculateInactivityDays(stats);

    double averageScoreComponent = (100.0 - averageScore) / 100.0;
    double failureRateComponent = failureRate;
    double varianceComponent = normalizeScoreVariance(scoreVariance);
    double inactivityComponent = (double)inactivityDays / 30.0;
    if (inactivityComponent > 1.0) inactivityComponent = 1.0;

    double riskScore = WEIGHT_AVERAGE_SCORE * averageScoreComponent
                     + WEIGHT_FAILURE_RATE * failureRateComponent
                     + WEIGHT_SCORE_VARIANCE * varianceComponent
                     + WEIGHT_INACTIVITY_DAYS * inactivityComponent;

    if (riskScore < 0.0) riskScore = 0.0;
    if (riskScore > 1.0) riskScore = 1.0;

    return riskScore;
}

// ============================================================
//  RECOMMENDATION ENGINE
// ============================================================

// Checks whether a recommendation rule matches the learner's risk profile.
bool doesRecommendationRuleMatch(const RecommendationRule& rule, double riskScore,
                                 double averageScore, double failureRate,
                                 double varianceNormalized, int inactivityDays) {
    switch (rule.type) {
        case RULE_CRITICAL_RISK:
            return riskScore >= CRITICAL_RISK_THRESHOLD;
        case RULE_HIGH_FAILURE:
            return failureRate >= HIGH_FAILURE_RATE_THRESHOLD;
        case RULE_LOW_AVERAGE:
            return averageScore < LOW_AVERAGE_SCORE_THRESHOLD;
        case RULE_HIGH_VARIANCE:
            return varianceNormalized > HIGH_VARIANCE_THRESHOLD;
        case RULE_INACTIVITY:
            return inactivityDays > INACTIVITY_DAYS_THRESHOLD;
        case RULE_MODERATE_COMBO:
            return failureRate >= MODERATE_FAILURE_RATE_THRESHOLD &&
                   averageScore < MODERATE_AVERAGE_SCORE_THRESHOLD;
        case RULE_MONITOR:
            return riskScore >= MONITOR_RISK_THRESHOLD;
        default:
            return false;
    }
}

// Generates a recommendation string from the array of rule structs.
string generateRecommendation(double riskScore, double averageScore,
                              double failureRate, double varianceNormalized,
                              int inactivityDays) {
    for (int i = 0; i < RECOMMENDATION_RULE_COUNT; i++) {
        if (doesRecommendationRuleMatch(
                recommendationRules[i], riskScore, averageScore,
                failureRate, varianceNormalized, inactivityDays)) {
            return recommendationRules[i].message;
        }
    }

    return "On track. Continue current learning path.";
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
void insertIntoRiskHeap(const RiskHeapEntry& entry) {
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

// Removes one learner from the max-heap when they no longer have active risk data.
void removeLearnerFromRiskHeap(int learnerID) {
    int removeIndex = lookupHeapIndex(learnerID);
    if (removeIndex == NO_HEAP_POSITION || removeIndex >= riskMaxHeap.size) {
        return;
    }

    int lastIndex = riskMaxHeap.size - 1;
    insertOrUpdateHeapIndex(learnerID, NO_HEAP_POSITION);

    if (removeIndex == lastIndex) {
        riskMaxHeap.size--;
        return;
    }

    riskMaxHeap.entries[removeIndex] = riskMaxHeap.entries[lastIndex];
    riskMaxHeap.size--;
    insertOrUpdateHeapIndex(riskMaxHeap.entries[removeIndex].learnerID, removeIndex);

    int parentIndex = (removeIndex - 1) / 2;
    if (removeIndex > 0 &&
        riskMaxHeap.entries[removeIndex].riskScore > riskMaxHeap.entries[parentIndex].riskScore) {
        bubbleUpHeapEntry(removeIndex);
    } else {
        bubbleDownHeapEntry(removeIndex);
    }
}

// Updates a learner's full risk profile inside the heap and restores heap order.
void updateLearnerRiskInHeap(const RiskHeapEntry& updatedEntry) {
    int heapPosition = lookupHeapIndex(updatedEntry.learnerID);
    if (heapPosition == NO_HEAP_POSITION || heapPosition >= riskMaxHeap.size) {
        return;
    }

    double oldRiskScore = riskMaxHeap.entries[heapPosition].riskScore;
    riskMaxHeap.entries[heapPosition] = updatedEntry;
    insertOrUpdateHeapIndex(updatedEntry.learnerID, heapPosition);

    if (updatedEntry.riskScore > oldRiskScore) {
        bubbleUpHeapEntry(heapPosition);
    } else if (updatedEntry.riskScore < oldRiskScore) {
        bubbleDownHeapEntry(heapPosition);
    }
}

// Extracts the highest-risk entry from any heap copy.
RiskHeapEntry extractHighestRiskFromHeap(RiskMaxHeap& heap) {
    RiskHeapEntry top = heap.entries[0];
    heap.size--;

    if (heap.size > 0) {
        heap.entries[0] = heap.entries[heap.size];

        int index = 0;
        while (true) {
            int leftChild = 2 * index + 1;
            int rightChild = 2 * index + 2;
            int largest = index;

            if (leftChild < heap.size &&
                heap.entries[leftChild].riskScore > heap.entries[largest].riskScore) {
                largest = leftChild;
            }
            if (rightChild < heap.size &&
                heap.entries[rightChild].riskScore > heap.entries[largest].riskScore) {
                largest = rightChild;
            }
            if (largest != index) {
                RiskHeapEntry swap = heap.entries[index];
                heap.entries[index] = heap.entries[largest];
                heap.entries[largest] = swap;
                index = largest;
            } else {
                break;
            }
        }
    }

    return top;
}

// ============================================================
//  RISK PROFILE BUILDING AND SYNCHRONIZATION
// ============================================================

// Builds one heap entry from a learner's current aggregated statistics.
RiskHeapEntry buildRiskHeapEntryFromStatistics(LearnerStatistics* stats) {
    RiskHeapEntry entry;

    double averageScore = calculateAverageScore(stats);
    double failureRate = calculateFailureRate(stats);
    double scoreVariance = calculateScoreVariance(stats);
    int inactivityDays = calculateInactivityDays(stats);
    double riskScore = calculateCompositeRiskScore(stats);
    double varianceNormalized = normalizeScoreVariance(scoreVariance);

    entry.learnerID = stats->learnerID;
    entry.riskScore = riskScore;
    entry.averageScore = averageScore;
    entry.failureRate = failureRate;
    entry.scoreVariance = scoreVariance;
    entry.inactivityDays = inactivityDays;
    entry.recommendation = generateRecommendation(
        riskScore, averageScore, failureRate, varianceNormalized, inactivityDays
    );

    return entry;
}

// Recomputes one learner's cached profile and heap entry from the current stats map.
void refreshRiskEntryForLearner(int learnerID) {
    LearnerStatistics* stats = lookupLearnerStatistics(learnerID);
    if (stats == nullptr || stats->totalAttempts == 0) {
        removeLearnerFromRiskHeap(learnerID);
        invalidateRiskScoreCache(learnerID);
        return;
    }

    RiskHeapEntry updatedEntry = buildRiskHeapEntryFromStatistics(stats);
    insertOrUpdateRiskScoreCache(updatedEntry);

    int heapPosition = lookupHeapIndex(learnerID);
    if (heapPosition == NO_HEAP_POSITION) {
        insertIntoRiskHeap(updatedEntry);
    } else {
        updateLearnerRiskInHeap(updatedEntry);
    }
}

// Scans all records in the activity log buffer and populates the statistics hash map.
void processAllLogRecords() {
    initializeLearnerStatisticsHashMap();

    int index = activityLogBuffer.head;
    for (int i = 0; i < activityLogBuffer.count; i++) {
        const LoggerRecord& record = activityLogBuffer.records[index];
        LearnerStatistics* stats = findOrCreateLearnerStatistics(record.learnerID);
        appendRecordToLearnerStatistics(stats, record);
        index = (index + 1) % LOG_BUFFER_CAPACITY;
    }
}

// Calculates risk scores for all learners in the statistics map and builds the heap and cache.
void buildRiskHeapFromStatistics() {
    initializeRiskMaxHeap();
    initializeHeapIndexHashMap();
    initializeRiskScoreCacheHashMap();

    for (int i = 0; i < STATISTICS_MAP_CAPACITY; i++) {
        if (!learnerStatisticsMap.entries[i].occupied) {
            continue;
        }
        if (learnerStatisticsMap.entries[i].totalAttempts == 0) {
            continue;
        }

        RiskHeapEntry entry = buildRiskHeapEntryFromStatistics(&learnerStatisticsMap.entries[i]);
        insertIntoRiskHeap(entry);
        insertOrUpdateRiskScoreCache(entry);
    }
}

// Rebuilds all statistics, heap state, and cache directly from Task 3's circular buffer.
void synchronizeRiskEngineWithLogs() {
    ensureRiskEngineInitialized();
    processAllLogRecords();
    buildRiskHeapFromStatistics();
}

// Updates only the affected learners when Task 3 appends a new log record.
void notifyRiskEngineOfLogChange(const LoggerRecord* overwrittenRecord, const LoggerRecord& newRecord) {
    ensureRiskEngineInitialized();

    rebuildLearnerStatisticsFromLogs(newRecord.learnerID);
    refreshRiskEntryForLearner(newRecord.learnerID);

    if (overwrittenRecord != nullptr && overwrittenRecord->learnerID != newRecord.learnerID) {
        rebuildLearnerStatisticsFromLogs(overwrittenRecord->learnerID);
        refreshRiskEntryForLearner(overwrittenRecord->learnerID);
    }
}

// ============================================================
//  DISPLAY FUNCTIONS
// ============================================================

// Displays the top N at-risk learners without destroying the heap.
void displayTopAtRiskLearners() {
    ensureRiskEngineInitialized();

    if (riskMaxHeap.size == 0) {
        cout << "No risk data available." << endl;
        return;
    }

    int topN;
    cout << "How many top at-risk learners to display? ";
    if (!(cin >> topN)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input. Please enter a number." << endl;
        return;
    }

    if (topN <= 0) {
        cout << "Invalid number." << endl;
        return;
    }
    if (topN > riskMaxHeap.size) {
        topN = riskMaxHeap.size;
    }

    RiskMaxHeap tempHeap = riskMaxHeap;

    cout << "\n===== TOP " << topN << " AT-RISK LEARNERS =====" << endl;
    cout << "Rank | ID   | Name            | Risk Score | Avg Score | Fail Rate | Recommendation" << endl;
    cout << "-----+------+-----------------+------------+-----------+-----------+----------------------------------" << endl;

    for (int rank = 1; rank <= topN; rank++) {
        RiskHeapEntry top = extractHighestRiskFromHeap(tempHeap);
        Learner* learner = findLearnerByID(top.learnerID);
        string learnerName = (learner != nullptr) ? learner->name : "Unknown";

        cout << "  " << rank
             << "  | " << top.learnerID
             << "  | " << learnerName;
        for (int p = (int)learnerName.length(); p < 15; p++) {
            cout << " ";
        }
        cout << " | " << (int)(top.riskScore * 100) << "%"
             << "        | " << (int)top.averageScore << "%"
             << "      | " << (int)(top.failureRate * 100) << "%"
             << "      | " << top.recommendation
             << endl;
    }
}

// Displays a detailed risk profile for an individual learner.
void displayIndividualRiskProfile() {
    ensureRiskEngineInitialized();

    int targetLearnerID;
    cout << "Enter Learner ID: ";
    if (!(cin >> targetLearnerID)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input. Please enter a number." << endl;
        return;
    }

    LearnerStatistics* stats = lookupLearnerStatistics(targetLearnerID);
    if (stats == nullptr || stats->totalAttempts == 0) {
        cout << "No data found for Learner " << targetLearnerID << "." << endl;
        return;
    }

    RiskScoreCacheEntry* cachedProfile = lookupRiskScoreCache(targetLearnerID);
    if (cachedProfile == nullptr) {
        refreshRiskEntryForLearner(targetLearnerID);
        cachedProfile = lookupRiskScoreCache(targetLearnerID);
    }
    if (cachedProfile == nullptr) {
        cout << "No data found for Learner " << targetLearnerID << "." << endl;
        return;
    }

    Learner* learner = findLearnerByID(targetLearnerID);
    string learnerName = (learner != nullptr) ? learner->name : "Unknown";
    double varianceNormalized = normalizeScoreVariance(cachedProfile->scoreVariance);

    cout << "\n===== RISK PROFILE: LEARNER " << targetLearnerID << " =====" << endl;
    cout << "Name:                 " << learnerName << endl;
    cout << "Total Attempts:       " << stats->totalAttempts << endl;
    cout << "Average Score:        " << (int)cachedProfile->averageScore << "%" << endl;
    cout << "Failure Rate:         " << (int)(cachedProfile->failureRate * 100) << "%" << endl;
    cout << "Score Variance:       " << (int)cachedProfile->scoreVariance << endl;
    cout << "Days Inactive:        " << cachedProfile->inactivityDays << endl;
    cout << "Composite Risk Score: " << (int)(cachedProfile->riskScore * 100) << "%" << endl;
    cout << endl;
    cout << "Risk Score Breakdown:" << endl;
    cout << "  Avg Score Component   (30%): " << (int)(((100.0 - cachedProfile->averageScore) / 100.0) * 100) << "%" << endl;
    cout << "  Failure Rate Component(35%): " << (int)(cachedProfile->failureRate * 100) << "%" << endl;
    cout << "  Variance Component    (20%): " << (int)(varianceNormalized * 100) << "%" << endl;
    cout << "  Inactivity Component  (15%): "
         << (int)(((double)cachedProfile->inactivityDays / 30.0 > 1.0 ? 1.0 : (double)cachedProfile->inactivityDays / 30.0) * 100)
         << "%" << endl;
    cout << endl;
    cout << "Recommendation: " << cachedProfile->recommendation << endl;
}

// ============================================================
//  CSV EXPORT
// ============================================================

// Exports the at-risk report to a timestamped CSV file.
void exportAtRiskReportToCSV() {
    ensureRiskEngineInitialized();

    if (riskMaxHeap.size == 0) {
        cout << "No risk data available." << endl;
        return;
    }

    time_t now = time(nullptr);
    struct tm* timeInfo = localtime(&now);
    char filename[128];
    strftime(filename, sizeof(filename), "risk_report_%Y%m%d_%H%M%S.csv", timeInfo);

    ofstream outFile(filename);
    if (!outFile.is_open()) {
        cout << "Error: Could not open " << filename << " for writing." << endl;
        return;
    }

    outFile << "learnerID,learnerName,riskScore,averageScore,failureRate,recommendation" << endl;

    RiskMaxHeap tempHeap = riskMaxHeap;
    while (tempHeap.size > 0) {
        RiskHeapEntry top = extractHighestRiskFromHeap(tempHeap);
        Learner* learner = findLearnerByID(top.learnerID);
        string learnerName = (learner != nullptr) ? learner->name : "Unknown";

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
    synchronizeRiskEngineWithLogs();
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
                synchronizeRiskEngineWithLogs();
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
