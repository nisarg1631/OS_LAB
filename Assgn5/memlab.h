#include <iostream>
enum DATATYPE
{
    INT,        // 0
    MEDIUM_INT, // 1
    CHAR,       // 2
    BOOL        // 3
};
using namespace std;
struct s_table_entry
{
    int to_be_freed;
    int unit_size;         // size of a unit, eg bool=1, int = 32, char = 9
    int total_size;        // total number of bits used in memory,
    int total_size_used; // total number of bits units used in memory, basically (total_size+31)/32)*32
    int idx_start_in_mem;  // index of the first 4byte unit in memory
    s_table_entry *next;    // pointer to the next stable_entry
};
struct DATA 
{
    bool is_array;          // is the data an array
    int size;               // size of the data, if it is an array, size is the size of the array
    DATATYPE type;          // type of the data
    s_table_entry *redirect; // pointer to the stable_entry
};
struct stack_entry 
{
    int scope_number;       // scope number of the stack entry
    s_table_entry *redirect; // pointer to the stable_entry
};
// Linked list of stable_entries to make the symbol table
struct s_table
{
    s_table_entry *head;
    s_table_entry *tail;
    int cur_size;

public:
    s_table();
    void insert_at_tail(s_table_entry *);  // inserts at the tail of the list
    void insert_after(s_table_entry *, s_table_entry *);    // inserts after the given entry
    void find_first_free_block(s_table_entry * );  // finds the first free block of given size 
};
// create Stack out of stack_entry
struct stack
{
    stack_entry * arr;  // array implementation of stack
    int top;            // index of top in arr
    int max_size;       // max size of the stack
    stack(int );        // constructor
    void push(stack_entry *); // pushes an entry onto the stack
    stack_entry pop();      // pops an entry from the stack
};