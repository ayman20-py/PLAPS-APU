
## **Assignment Specifications**

| Task   | Name                                      | Core Function                                                         |
| ------ | ----------------------------------------- | --------------------------------------------------------------------- |
| **T1** | Learner Registration & Session Management | Register learners, join/exit sessions, waiting queue, resume progress |
| **T2** | Activity Navigation & Session Flow        | Activities per session, next/back/forward, scores, send to T3         |
| **T3** | Recent Activity Logging                   | Fixed-size circular buffer, store records, filter, export             |
| **T4** | At-Risk Priority & Recommendations        | Risk scores, max-heap ranking, recommendations, export CSV            |
**FORBIDDEN** - No `<vector>`, `<list>`, `<queue>`, `<stack>`, `<priority_queue>`, `<map>`

## **Sessions & Activities**
# PLAPS - ALL SESSIONS & ACTIVITIES

| Session | Cap | Activity Name | Description | Diff |
|---------|:---:|---------------|-------------|:----:|
| **1. Arrays & Memory** | 5 | Create an Array | Declare and initialize an integer array of size 5 | 1/5 |
| | | Access Elements | Print all array elements using index | 1/5 |
| | | Modify Elements | Change the value at position 2 to 99 | 2/5 |
| | | Array Sum | Calculate the sum of all elements in the array | 2/5 |
| | | Find Maximum | Find the largest element in the array | 3/5 |
| | | | | |
| **2. Looping Constructs** | 5 | For Loop Basics | Print numbers 1 to 10 using a for loop | 1/5 |
| | | While Loop | Repeat until user enters 0 | 1/5 |
| | | Nested Loops | Print a multiplication table (1-5) | 2/5 |
| | | Loop with Array | Traverse an array using a loop and print each element | 2/5 |
| | | Break & Continue | Print 1-20, skip even numbers, stop at 15 | 3/5 |
| | | | | |
| **3. Functions & Scope** | 5 | Function Declaration | Define and call a function that prints "Hello World" | 2/5 |
| | | Parameters & Return | Write a function that adds two numbers and returns the result | 2/5 |
| | | Pass by Value | Demonstrate that the original variable doesn't change | 2/5 |
| | | Pass by Reference | Modify the original variable using references | 3/5 |
| | | Function Overloading | Create two functions with same name, different parameters | 3/5 |
| | | Scope Rules | Demonstrate local vs global variables | 3/5 |
| | | | | |
| **4. Debugging Workshop** | 4 | Syntax Errors | Fix missing semicolons, brackets, and typos | 2/5 |
| | | Logic Errors | Find why the sum is always incorrect | 3/5 |
| | | Runtime Errors | Handle division by zero and array out of bounds | 3/5 |
| | | Trace Execution | Step through code mentally and predict the output | 4/5 |
| | | | | |
| **5. Mini Project Lab** | 3 | Student Grade Calculator | Input 5 grades, calculate average, assign letter grade | 4/5 |
| | | Basic Inventory System | Store item names and quantities, search by name | 4/5 |
| | | Simple Menu Program | Create a do-while menu with at least 4 options | 4/5 |

### Rules
1 Session has up to 5 activities. But every activity can have only 1 or no learners. So, max number of learner per sessions = total number of activities in session.

## **Task 1 - Mohammed**

| Requirement             | What it means                                        |
| ----------------------- | ---------------------------------------------------- |
| **Register learner**    | Add new learner to system. Generate unique ID.       |
| **Join session**        | Learner enters session. If full → waiting queue.     |
| **Exit session**        | Learner leaves. Free slot → next from queue.         |
| **Display status**      | Show registered, active, waiting.                    |
| **Persistence**         | Save/load learners to `students.csv`.                |
| **Resume progress**     | Remember where learner left off.                     |
| **5 sessions**          | Predefined sessions with names, capacities.          |
| **Session progression** | Unlock sessions only after completing prerequisites. |
### **Data Structures**

| Component                 | DS Choice              | Why                                                 |
| ------------------------- | ---------------------- | --------------------------------------------------- |
| **Registered learners**   | **Singly Linked List** | Dynamic growth, O(1) insert, no capacity limit      |
| **Active session slots**  | **Fixed Array**        | Session capacity known, O(1) slot access            |
| **Waiting queue**         | **Circular Queue**     | O(1) enqueue/dequeue, no shifting, memory efficient |
| **Learner → Session map** | **Hash Map**           | O(1): "Which session is learner 101 in?"            |
| **Learner → Progress**    | **Hash Map**           | O(1): Resume functionality                          |
| **Sessions**              | **Array of Structs**   | Fixed 5 sessions, random access by ID               |



## **Task 2 - Ayman**
**Your job:**
- Define all activities for all 5 sessions    
- Track where EACH learner is **right now** (current activity)    
- Allow **next**, **back**, and **forward** navigation    
- Store **scores** for completed activities    
- Send records to **Task 3 (Logger)**
- Tell **Task 1** when a learner completes a session    
- Tell **Task 1** to save progress (for resume)

| Requirement                  | What it means                                                        |
| ---------------------------- | -------------------------------------------------------------------- |
| **Define activities**        | 5 sessions × up to 6 activities each. Titles, topics, difficulty.    |
| **Load session**             | When learner joins, load that session's activities.                  |
| **Show current activity**    | Display the activity the learner is on.                              |
| **Next activity**            | Move forward. Record score. Push to back stack. Clear forward stack. |
| **Previous activity (Back)** | Move backward. Push current to forward stack. Pop back stack.        |
| **Forward activity**         | Move forward through undone history.                                 |
| **Record score**             | Store score for completed activity. Mark as completed.               |
| **Send to Task 3**           | Call `logger.addRecord()` with activity result.                      |
| **Tell Task 1**              | Update learner progress + notify on session completion.              |
| **Navigation history**       | Two stacks: back + forward. Full undo/redo.                          |
| **Branching**                | Going back then taking new path = clear forward stack.               |
### **Data Structures**

| Component                | DS Choice                | Why                                                        |
| ------------------------ | ------------------------ | ---------------------------------------------------------- |
| **Activity definitions** | **2D Array**             | `Activity sessions[5][20]` — fixed, random access by index |
| **Current position**     | **Hash Map**             | `learnerID → currentActivityIndex` — O(1) lookup           |
| **Back history**         | **Stack (Linked)**       | LIFO — last visited, first to go back to                   |
| **Forward history**      | **Stack (Linked)**       | LIFO — last undone, first to redo                          |
| **Scores**               | **2D Array or Hash Map** | `learnerID → sessionID → activityIndex → score`            |
| **Session completion**   | **Boolean check**        | `currentIndex == lastIndex`                                |



## **Task 3 - Hasan**

**Your job:**
- **Receive** activity records from Task 2    
- **Store** them in a **fixed-size circular buffer**    
- **Overwrite** oldest records when full    
- **View** all logs (chronological order)    
- **Filter** logs by learner ID (fast!)    
- **Export** logs to timestamped text files    
- **Provide** logs to Task 4 for risk calculation

### **Requirements**
| Requirement           | What it means                                     |
| --------------------- | ------------------------------------------------- |
| **Fixed-size log**    | Set capacity (50). Never exceed this.             |
| **Circular buffer**   | Oldest overwritten when full. O(1) insert.        |
| **Add record**        | Receive from Task 2, store with timestamp.        |
| **View all logs**     | Display from oldest to newest.                    |
| **Filter by learner** | Show only records for specific learner ID.        |
| **Export to file**    | Write all logs to `logs_YYYYMMDD_HHMMSS.txt`.     |
| **Provide to Task 4** | Allow Task 4 to access logs for risk calculation. |
### **Data Structure**

| Component                | DS Choice                  | Why                                                  |
| ------------------------ | -------------------------- | ---------------------------------------------------- |
| **Activity log storage** | **Circular Queue (Array)** | Fixed size, O(1) insert, no shifting, auto-overwrite |
| **Learner index**        | **Hash Map + Linked List** | O(1) filter by learner — NOT O(N) scan               |
| **Export formatting**    | **String buffer**          | Efficient file writing                               |
| **Timestamp**            | `time()`                   | Standard library, unique filenames                   |



## **Task 4 - Omar**

Your Job:
- **Read logs** from Task 3 (circular buffer)    
- **Calculate risk score** for each learner (4 factors)    
- **Rank learners** by risk (highest first)    
- **Maintain priority queue** (Max-Heap)    
- **Update dynamically** when new data arrives    
- **Generate recommendations** (rule-based)
- **Export at-risk list** to CSV

### **Requirements**
|Requirement|What it means|
|---|---|
|**Read logs from Task 3**|Get all activity records or filter by learner|
|**Calculate risk score**|Formula using scores, failures, consistency, time|
|**Rank learners**|Highest risk first|
|**Display at-risk list**|Show top N learners with risk score + recommendation|
|**Priority queue**|Max-heap for O(1) access to highest risk|
|**Dynamic updates**|When new activity logged, update risk + heap|
|**Export CSV**|Timestamped file with at-risk learners|
|**Recommendations**|Specific actions based on performance patterns|
### **Data Structures**
| Component                  | DS Choice             | Why                                       |
| -------------------------- | --------------------- | ----------------------------------------- |
| **At-risk priority queue** | **Max-Heap (Array)**  | O(1) peek highest risk, O(log N) push/pop |
| **Learner → Heap index**   | **Hash Map**          | O(1) find learner in heap for updates     |
| **Learner statistics**     | **Hash Map**          | O(1) access to performance data           |
| **Recommendation rules**   | **Array of structs**  | Linear scan, priority order               |
| **Risk score cache**       | **Array or Hash Map** | Avoid recalculating unnecessarily         |
