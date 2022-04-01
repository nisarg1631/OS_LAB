# Structure of Page Table
```c++
struct s_table {
	int head_idx;
	int tail_idx;
	int cur_size;
	int mx_size;
	s_table_entry *arr;
	
	// constructs
	void s_table_init(int, s_table_entry *);
	// inserts at the tail of the list
	int insert(uint32_t addr, uint32_t unit_size, uint32_t total_size); 
	// removes entry at idx
	void remove(uint32_t idx); 
	// unmakrs entry at idx
	void unmark(uint32_t idx); 
	// prints the symbol table
	void print_s_table();
};
```

- The symbol table is an array of  ```s_table_entry``` elements. However, we follow a hybrid approach where we follow a linked list pattern but in a static manner.

> Why do we use this hybrid approach?
> This method allows our insert and remove functions to run in O(1). 
> Because we have an array, we can do random access for the symbol table entries, for array elements which are unused,we chain them in a linked list, where the first 31 bits of .next field tells us where the next unuses array element is, this allows us to fully use the array and not worry about potential fragementation due to symbol table entries being deleted in the middle. Because in our approach they are added to the end of the linked list. 

- ```s_table_init```, ```remove```, ```unmark```, and ```print_s_table``` are self explanatory.
- ```insert``` will add an entry into the symbol table for an element in main memory whose address is ```addr```, and the type is derived using the ```unit_size``` parameter and the ```total_size``` it takes, which will be equal to ```unit_size``` if it is a single element. The other case is when the entry is an array in which case the sizes will differ.

The structure ```s_table_entry``` is as follows:

```C++
struct s_table_entry {
	// index in memory
	uint32_t addr_in_mem;
	// size of a unit in bits, eg bool=1, int = 32, char = 8, medium_int = 24
	uint32_t unit_size;
	// total number of bits used in memory
	uint32_t total_size;
	// 31 bits idx to the next s_table_entry, last bit saying if this block is to not be freed or not
	uint32_t next;
	
	int is_free() {
		return this->next & 1;
	}
};
```

The explanation for each of the members is clear from the code comments.

# Additional Data Structures and Functions
## Data Structures
```C++
struct stack {
	stack_entry *arr; // array implementation of stack
	int top; // index of top in arr
	int max_size; // max size of the stack
	void stack_init(int, stack_entry *); // constructor
	void push(s_table_entry *); // pushes an entry onto the stack
	stack_entry *pop(); // pops an entry from the stack
	s_table_entry *top_ret(); // returns the top of the stack
	void StackTrace(); // prints the stack
};

struct stack_entry {
	s_table_entry *redirect; // pointer to the s_table_entry
	int scope_tbf; // first 31 bits scope number, last bit tells us if the entry has to be freed
};
```

The stack is used to keep track of the scope of the variables. This is then used to remove the elements from the symbol table once their scope ends. Their is a direct pointer to the symbol table entry which facilitates for O(1) removal.

> How does scope work?
> //TODO: Explain basic working

## Functions
- ```startScope()``` - 
- ```endScope(int scope)``` - 
- ```freeElem_inner(s_table_entry *var)``` -
- ```CreatePartitionMainMemory(int size)``` -
- ```FreePartitionMainMemory(int idx)``` -
# Garbage Collection
//TODO: Add basic working of garbage collection

- ```gc_init()``` - Sets up the thread for garbage collection and periodically invokes the ```gc_run_inner()``` function to do the garbage collection.
- ```gc_run_inner()``` - Does the actual sweep and deletion of memory.
- ```gc_run(int signum)``` - Signal handler function. Can manually invoke the garbage collector by sending a signal to the thread (not used as of now but kept for functionality).

## Compact
# Usage of Locks
# Statistics

