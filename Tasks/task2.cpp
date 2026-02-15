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
#include "datastructures.h"

using namespace std;

// ========== Initializing Variables ==========
SessionManagement sessions;



void InitializingSessions() {
    sessions.displayTest();
}