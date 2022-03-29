#include "memlab.h"
void stack::stack_init(int mx, stack_entry *mem_block)
{
    this->arr = mem_block;
    this->top = -1;
    this->max_size = mx;
}
void s_table::s_table_init(int mx, s_table_entry *mem_block)
{
    this->arr = mem_block;
    this->head_idx = 0;
    this->tail_idx = mx - 1;
    for (int i = 0; i + 1 < mx; i++)
        this->arr[i].next = i + 1;
    this->cur_size = 0;
    this->mx_size = mx;
}
int s_table::insert(uint32_t addr, uint32_t unit_size, uint32_t total_size)
{
    pthread_mutex_lock(&symbol_table_mutex);

    if (this->cur_size == this->mx_size)
    {
        printf("[s_table::insert]: Symbol table is full\n");
        pthread_mutex_unlock(&symbol_table_mutex);

        return -1;
    }
    int idx = head_idx;
    arr[idx].addr_in_mem = addr;
    arr[idx].unit_size = unit_size;
    arr[idx].total_size = total_size;
    head_idx = arr[idx].next;
    arr[idx].next = -1; // end of the entries list
    this->cur_size++;
    pthread_mutex_unlock(&symbol_table_mutex);
    return idx;
}
void s_table::remove(uint32_t idx)
{
    pthread_mutex_lock(&symbol_table_mutex);
    if (idx >= this->mx_size or this->cur_size <= 0)
    {
        pthread_mutex_unlock(&symbol_table_mutex);
        return;
    }
    this->cur_size--;
    this->arr[idx].addr_in_mem = -1;
    this->arr[idx].next = -1;
    this->arr[idx].total_size = 0;
    this->arr[idx].unit_size = 0;
    this->arr[tail_idx].next = idx;
    tail_idx = idx;
    printf("[s_table::remove]: Removed variable at index %d\n", idx);
    pthread_mutex_unlock(&symbol_table_mutex);
}
void CreateMemory(int size)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&symbol_table_mutex, &attr);
    pthread_mutex_init(&stack_mutex, &attr);
    pthread_mutex_init(&memory_mutex, &attr);
    printf("[CreateMemory]: Set up mutexes\n");

    pthread_mutex_lock(&memory_mutex);
    BIG_MEMORY = (int *)malloc(((size + 3) / 4) * 4);
    big_memory_sz = ((size + 3) / 4) * 4; // nearest multiple of 4 to size
    printf("[CreateMemory]: Allocated %d bytes of data as requested\n", ((size + 3) / 4) * 4);
    BOOKKEEP_MEMORY = (int *)malloc(bookkeeping_memory_size);
    printf("[CreateMemory]: Allocated %d bytes of data for bookkeeping\n", bookkeeping_memory_size);
    pthread_mutex_unlock(&memory_mutex);

    pthread_mutex_lock(&stack_mutex);
    GLOBAL_STACK = (stack *)(BOOKKEEP_MEMORY);
    GLOBAL_STACK->stack_init(max_stack_size, (stack_entry *)(GLOBAL_STACK + 1));
    pthread_mutex_unlock(&stack_mutex);

    pthread_mutex_lock(&symbol_table_mutex);
    s_table *S_TABLE_START = (s_table *)(GLOBAL_STACK->arr + GLOBAL_STACK->max_size);
    SYMBOL_TABLE = S_TABLE_START;
    SYMBOL_TABLE->s_table_init(max_stack_size, (s_table_entry *)(SYMBOL_TABLE + 1));
    printf("[CreateMemory]: Setup Stack and Symbol Table\n");
    pthread_mutex_unlock(&symbol_table_mutex);
}
int CreatePartitionMainMemory(int size)
{
    // Format:
    // Header: size (31 bits), free (1 bit)
    // Data: size (nearest mult of 4)
    // Footer: size (31 bits), free (1 bit)
    // source: https://courses.cs.washington.edu/courses/cse351/17au/lectures/25/CSE351-L25-memalloc-II_17au.pdf
    // returns idx of location of data in the memory
    // Uses First Fit
    pthread_mutex_lock(&memory_mutex);
    int *p = BIG_MEMORY;
    int newsize = (((size + 3) >> 2) << 2);
    newsize += 2 * sizeof(int);
    while ((p < BIG_MEMORY + big_memory_sz) && ((*p & 1) || ((*p << 1) < newsize)))
        p = p + (*p >> 1);
    if (p == BIG_MEMORY + big_memory_sz)
    {
        pthread_mutex_unlock(&memory_mutex);
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
    pthread_mutex_unlock(&memory_mutex);
    return (p - BIG_MEMORY);
}
void FreePartitionMainMemory(int *ptr)
{
    *ptr = *ptr & -2;                                      // clear allocated flag
    int *next = ptr + *ptr;                                // find next block
    if (next - BIG_MEMORY < big_memory_sz && !(*next & 1)) // if next block is free
        *ptr += *next;                                     // merge with next block
    if (ptr != BIG_MEMORY)                                 // there is a block before
    {
        int *prev = ptr - *(ptr - 1); // find previous block
        if (!(*prev & 1))             // if previous block is free
            *prev += *ptr;            // merge with previous block
    }
}
s_table_entry *CreateVar(DATATYPE a)
{
    int main_memory_idx, unit_size;
    switch (a)
    {
    case INT:
        unit_size = 32;
        main_memory_idx = CreatePartitionMainMemory(32);
        break;
    case MEDIUM_INT:
        unit_size = 24;
        main_memory_idx = CreatePartitionMainMemory(24);
        break;
    case CHAR:
        unit_size = 8;
        main_memory_idx = CreatePartitionMainMemory(8);
        break;
    case BOOL:
        unit_size = 1;
        main_memory_idx = CreatePartitionMainMemory(1);
        break;
    default:
        main_memory_idx = -1;
        break;
    }
    if (main_memory_idx == -1)
    {
        printf("[CreateVar]: Failed to create variable\n");
        return NULL;
    }
    printf("[CreateVar]: Created variable of size %d at index %d\n", unit_size, main_memory_idx);
    pthread_mutex_lock(&symbol_table_mutex);
    int idx = SYMBOL_TABLE->insert(main_memory_idx, unit_size, unit_size);
    printf("[CreateVar]: Inserted variable into symbol table at index %d\n", idx);
    pthread_mutex_unlock(&symbol_table_mutex);
    return &SYMBOL_TABLE->arr[idx];
}
s_table_entry *CreateArray(DATATYPE a, int sz)
{
    int main_memory_idx, unit_size, total_size;
    switch (a)
    {
    case INT:
        unit_size = 32;
        total_size = unit_size * sz;
        main_memory_idx = CreatePartitionMainMemory(total_size);
        break;
    case MEDIUM_INT:
        unit_size = 24;
        total_size = unit_size * sz;
        main_memory_idx = CreatePartitionMainMemory(total_size);
        break;
    case CHAR:
        unit_size = 8;
        total_size = unit_size * sz;
        main_memory_idx = CreatePartitionMainMemory(total_size);
        break;
    case BOOL:
        unit_size = 1;
        total_size = unit_size * sz;
        main_memory_idx = CreatePartitionMainMemory(total_size);
        break;
    default:
        main_memory_idx = -1;
        break;
    }
    if (main_memory_idx == -1)
    {
        printf("[CreateArr]: Failed to create array\n");
        return NULL;
    }
    pthread_mutex_lock(&symbol_table_mutex);
    int idx = SYMBOL_TABLE->insert(main_memory_idx, unit_size, unit_size);
    printf("[CreateArr]: Created array of size %d, total size %d at index %d\n", unit_size, total_size, main_memory_idx);
    pthread_mutex_unlock(&symbol_table_mutex);
    return &SYMBOL_TABLE->arr[idx];
}
void AssignVar(s_table_entry *var, int val)
{
    pthread_mutex_lock(&symbol_table_mutex);
    if (var->unit_size == 32)
    {
        pthread_mutex_lock(&memory_mutex);
        *((int *)(BIG_MEMORY + var->addr_in_mem)) = val;
        pthread_mutex_unlock(&memory_mutex);
        printf("[AssignVar]: Assigned %d to variable at index %d, it looks like %d\n", val, var->addr_in_mem, *((BIG_MEMORY + var->addr_in_mem)));
    }
    else if (var->unit_size == 24)
    {
        pthread_mutex_lock(&memory_mutex);
        *((int *)(BIG_MEMORY + var->addr_in_mem)) = (val << 8) >> 8; // remove the top 8 bits for medium int
        pthread_mutex_unlock(&memory_mutex);
        printf("[AssignVar]: Assigned %d to variable at index %d, it looks like %d\n", val, var->addr_in_mem, *((BIG_MEMORY + var->addr_in_mem)));
    }
    else
    {
        printf("[AssignVar]: Trying to assign an integer to a bool, type checking failed\n");
    }
}
void AssignVar(s_table_entry *var, int val)
{

}