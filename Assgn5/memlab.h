#include <iostream>
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
    uint32_t next;        // pointer to the next stable_entry
};
struct s_table
{
    int head_idx;
    int tail_idx;
    int cur_size;
    int mx_size;
    s_table_entry *arr;

public:
    void s_table_init(int, s_table_entry *);                             // constructs
    void insert(uint32_t addr, uint32_t unit_size, uint32_t total_size); // inserts at the tail of the list
    void remove(uint32_t idx);                                           // removes entry at idx
};
struct stack_entry
{
    int scope_number; // scope number of the stack entry
    int redirect;     // pointer to the stable_entry
    int to_be_freed;  // tells us if the entry has to be freed
};
// Linked list of stable_entries to make the symbol table
// create Stack out of stack_entry
struct stack
{
    stack_entry *arr;                    // array implementation of stack
    int top;                             // index of top in arr
    int max_size;                        // max size of the stack
    void stack_init(int, stack_entry *); // constructor
    void push(int, int);                 // pushes an entry onto the stack
};
stack *GLOBAL_STACK;
s_table *SYMBOL_TABLE;
int big_memory_sz;
int *BIG_MEMORY = NULL;                                        // Pointer to the start of the BIG_MEMORY, int for enforcing word allignment
int *BOOKKEEP_MEMORY = NULL;                                   // Pointer to the memory segment used for bookkeeping data structures
void CreateMemory(int);                                        // A function to create a memory segment using malloc
s_table_entry *CreateVar(DATATYPE);                            // Returns the symbol table entry. Using this function you can create a variable. These variables will reside in the memory created by createMem
s_table_entry *CreateArray(DATATYPE, int);                     // Returns the symbol table entry. Using this function you can create an array of the above types. These variables reside in the memory created by createMem.
void AssignVar(int, void *);                                   // Pass the symbol table entry. Assign values to variables. Have a light type-checking, boolean variable cannot hold an int etc
void AssignArray(int, void *);                                 // Pass the symbol table entry. Assign values to array or array elements. Have a light typechecking, your boolean variable cannot hold an int etc
void freeElem(int);                                            // Mark the element to be freed by the garbage collector
void freeMem();                                                // Free the memory segment created by createMem // Extra
void startScope();                                             // Needs to be called by the programmer to indicate the start of a new scope
void endScope();                                               // Needs to be called by the programmer to indicate the end of a scope
pthread_mutex_t symbol_table_mutex, stack_mutex, memory_mutex; // Locks for synchronisation
const int bookkeeping_memory_size = 1e8;
const int max_stack_size = 1e5; // also max size of symbol table
struct GarbageCollector
{
    // #ifdef NO_GC
    // #endif
    void gc_init();       // sets up the thread for garbage collection
    void gc_run();        // runs the garbage collector, periodically wakes up and sees if called by endScope, or if anything marked for deletion by freeElem
    void gc_run_inner();  // does the actual sweep and deletion of memory, called by gc_run
    int compact_once();   // compacts the memory space once, removes the first gap it finds in the memory, returns 0 if nothing to compact, 1 if compacted
    void compact_total(); // compacts the memory space until it is compacted
};
GarbageCollector *GC;

int CreatePartitionMainMemory(int size)
{
    // Format:
    // Header: size (31 bits), free (1 bit)
    // Data: size (nearest mult of 4)
    // Footer: size (31 bits), free (1 bit)
    // source: https://web2.qatar.cmu.edu/~msakr/15213-f09/lectures/class19.pdf
    // returns idx of location of data in the memory 
    int *p = BIG_MEMORY;
    int newsize = (((size + 3) >> 2) << 2);
    newsize += 2 * sizeof(int);
    while ((p < BIG_MEMORY + big_memory_sz) && ((*p & 1) || ((*p << 1) < newsize)))
        p = p + (*p >> 1);
    if (p == BIG_MEMORY + big_memory_sz)
    {
        return -1;
    }
    // found a block with size >= size wanted
    int oldsize = *p << 1;               // old size of the block
    int words = newsize >> 2;            // number of 4 byte blocks we need
    *p = (words << 1) | 1;               // set the header of the new block, first 31 bits: words, last bit: 1 (in use)
    *(p + words - 1) = (words << 1) | 1; // footer: same as above
    if (newsize < oldsize)               // If some blocks are left
    {
        *(p + words) = (oldsize - newsize) >> 1;              // header of the new block, last bit 0 as free
        *(p + (oldsize >> 2) - 1) = (oldsize - newsize) >> 1; // footer of the new block, last bit is 0 as free
    }
    return (p - BIG_MEMORY);
}