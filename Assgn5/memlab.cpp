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
    this->head_idx = -1;
    this->tail_idx = -1;
    this->cur_size = 0;
    this->mx_size = mx;
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
    pthread_mutex_unlock(&symbol_table_mutex);
    printf("[CreateMemory]: Setup Stack and Symbol Table\n");
}
