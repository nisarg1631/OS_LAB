#include <iostream>
#include <stdio.h>
#include <bitset>
#include <csignal>
#include <sys/time.h>
#include <unistd.h>
using namespace std;
enum DATATYPE
{
    INT,        // 0
    MEDIUM_INT, // 1
    CHAR,       // 2
    BOOL        // 3
};
struct s_table_entry
{
    uint32_t addr_in_mem; // index in memory
    uint32_t unit_size;   // size of a unit in bits, eg bool=1, int = 32, char = 8, medium_int = 24
    uint32_t total_size;  // total number of bits used in memory
    uint32_t next;        //  31 bits idx to the next stable_entry, last bit saying if this block is to not be freed or not
    int is_free()
    {
        return this->next & 1;
    }
};
struct s_table
{
    int head_idx;
    int tail_idx;
    int cur_size;
    int mx_size;
    s_table_entry *arr;

    void s_table_init(int, s_table_entry *);                            // constructs
    int insert(uint32_t addr, uint32_t unit_size, uint32_t total_size); // inserts at the tail of the list
    void remove(uint32_t idx);                                          // removes entry at idx
    void unmark(uint32_t idx);                                          // unmakrs entry at idx
    void print_s_table();                                               // prints the symbol table
};
struct stack_entry
{
    s_table_entry *redirect; // pointer to the stable_entry
    int scope_tbf;           // first 31 bits scope number, last bit tells us if the entry has to be freed
};
// Linked list of stable_entries to make the symbol table
// create Stack out of stack_entry
struct stack
{
    stack_entry *arr;                    // array implementation of stack
    int top;                             // index of top in arr
    int max_size;                        // max size of the stack
    void stack_init(int, stack_entry *); // constructor
    void push(s_table_entry *);          // pushes an entry onto the stack
    stack_entry *pop();                  // pops an entry from the stack
    s_table_entry *top_ret();            // returns the top of the stack
    void StackTrace();                   // prints the stack
};
stack *GLOBAL_STACK;
s_table *SYMBOL_TABLE;
int big_memory_sz;
int *BIG_MEMORY = NULL;                           // Pointer to the start of the BIG_MEMORY, int for enforcing word allignment
int *BOOKKEEP_MEMORY = NULL;                      // Pointer to the memory segment used for bookkeeping data structures
void CreateMemory(int);                           // A function to create a memory segment using malloc
s_table_entry *CreateVar(DATATYPE);               // Returns the symbol table entry. Using this function you can create a variable. These variables will reside in the memory created by createMem
s_table_entry *CreateArray(DATATYPE, int);        // Returns the symbol table entry. Using this function you can create an array of the above types. These variables reside in the memory created by createMem.
void AssignVar(s_table_entry *, int);             // Pass the symbol table entry. Assign values to variables. Have a light type-checking, boolean variable cannot hold an int etc
void AssignArray(s_table_entry *, int, uint32_t); // Pass the symbol table entry. Assign values to array or array elements. Have a light typechecking, your boolean variable cannot hold an int etc
void freeElem(s_table_entry *);                   // Mark the element to be freed by the garbage collector
void freeMem();                                   // Free the memory segment created by createMem // Extra
void startScope();                                // Needs to be called by the programmer to indicate the start of a new scope
void endScope();                                  // Needs to be called by the programmer to indicate the end of a scope
void freeElem_inner(s_table_entry *var);
pthread_mutex_t symbol_table_mutex, stack_mutex, memory_mutex; // Locks for synchronisation
const int bookkeeping_memory_size = 1e8;
const int max_stack_size = 1e5; // also max size of symbol table
int CURRENT_SCOPE = 0;
int GC_ACTIVE = 0;
struct GarbageCollector
{
    // #ifdef NO_GC
    // #endif
    pthread_t gc_thread;
    int gc_stat;
    void gc_init();       // sets up the thread for garbage collection
    void gc_run_inner();  // does the actual sweep and deletion of memory, called by gc_run
    int compact_once();   // compacts the memory space once, removes the first gap it finds in the memory, returns 0 if nothing to compact, 1 if compacted
    void compact_total(); // compacts the memory space until it is compacted
};
GarbageCollector *GC;
void gc_run(int); // runs the garbage collector, periodically wakes up and sees if called by endScope, or if anything marked for deletion by freeElem

int CreatePartitionMainMemory(int size);
void FreePartitionMainMemory(int idx);
