#include "memlab.h"
stack *GLOBAL_STACK;
s_table *SYMBOL_TABLE;
GarbageCollector *GC;
int big_memory_sz;
int *BIG_MEMORY = NULL;                                                                      // Pointer to the start of the BIG_MEMORY, int for enforcing word allignment
int *BOOKKEEP_MEMORY = NULL;                                                                 // Pointer to the memory segment used for bookkeeping data structures
pthread_mutex_t symbol_table_mutex, stack_mutex, memory_mutex, gc_active_mutex, print_mutex; // Locks for synchronisation
const int bookkeeping_memory_size = 1e8;                                                     // max size of bookkeeping memory
const int max_stack_size = 1e5;                                                              // also max size of symbol table
int CURRENT_SCOPE = 0;                                                                       // current scope
int GC_ACTIVE = 0;                                                                           // 0 if garbage collector is not active, 1 if active

void stack::stack_init(int mx, stack_entry *mem_block)
{
    pthread_mutex_lock(&stack_mutex);
    this->arr = mem_block;
    this->top = -1;
    this->max_size = mx;
    pthread_mutex_unlock(&stack_mutex);
}
void stack::push(s_table_entry *redirect_ptr)
{
    pthread_mutex_lock(&stack_mutex);
    this->top++;
    if (this->top >= this->max_size)
    {
        pthread_mutex_lock(&print_mutex);
        printf("[stack::push] Stack Overflow\n");
        pthread_mutex_unlock(&print_mutex);
        pthread_mutex_unlock(&stack_mutex);
        return;
    }
    this->arr[this->top].redirect = redirect_ptr;
    this->arr[this->top].scope_tbf = CURRENT_SCOPE << 1;
    pthread_mutex_unlock(&stack_mutex);
}
stack_entry *stack::pop()
{
    pthread_mutex_lock(&stack_mutex);
    if (this->top == -1)
    {
        pthread_mutex_lock(&print_mutex);
        printf("[stack::pop] stack is empty\n");
        pthread_mutex_unlock(&print_mutex);
        pthread_mutex_unlock(&stack_mutex);
        return NULL;
    }
    else
    {
        this->top--;
        auto ret = &this->arr[this->top + 1];
        pthread_mutex_lock(&print_mutex);
        printf("[stack::pop] poped element \n");
        pthread_mutex_unlock(&print_mutex);
        pthread_mutex_unlock(&stack_mutex);
        return ret; // should be processed before the next push else race condition
    }
}
s_table_entry *stack::top_ret()
{
    pthread_mutex_lock(&stack_mutex);
    if (~this->top)
    {
        auto ret = this->arr[this->top].redirect;
        pthread_mutex_unlock(&stack_mutex);
        return ret;
    }
    pthread_mutex_unlock(&stack_mutex);
    return NULL;
}
void startScope()
{
#ifndef NO_GC
    CURRENT_SCOPE++;
#endif
}
void endScope()
{
#ifndef NO_GC
    pthread_mutex_lock(&symbol_table_mutex);
    pthread_mutex_lock(&stack_mutex);
    for (int i = 0; i <= GLOBAL_STACK->top; i++)
    {
        if ((GLOBAL_STACK->arr[i].scope_tbf >> 1) == CURRENT_SCOPE)
        {
            SYMBOL_TABLE->unmark(GLOBAL_STACK->arr[i].redirect - SYMBOL_TABLE->arr);
            GLOBAL_STACK->arr[i].scope_tbf |= 1;
        }
    }
    CURRENT_SCOPE--;
    pthread_mutex_unlock(&stack_mutex);
    pthread_mutex_unlock(&symbol_table_mutex);
#endif
}
void stack::StackTrace()
{
    pthread_mutex_lock(&stack_mutex);
    pthread_mutex_lock(&print_mutex);
    printf("[StackTrace]: Stack has %d entries, it looks like:\n", this->top + 1);
    for (int i = this->top; ~i; i--)
        printf("[StackTrace]: Scope Number %d \t Redirect %ld \t To be free %d\n", this->arr[i].scope_tbf >> 1, this->arr[i].redirect - SYMBOL_TABLE->arr, this->arr[i].scope_tbf & 1);
    pthread_mutex_unlock(&print_mutex);
    pthread_mutex_unlock(&stack_mutex);
}
void s_table::s_table_init(int mx, s_table_entry *mem_block)
{
    pthread_mutex_lock(&symbol_table_mutex);
    this->arr = mem_block;
    this->head_idx = 0;
    this->tail_idx = mx - 1;
    for (int i = 0; i + 1 < mx; i++)
    {
        this->arr[i].addr_in_mem = -1;
        this->arr[i].next = ((i + 1) << 1);
        this->arr[i].next |= 1;
    }
    this->arr[mx - 1].addr_in_mem = -1;
    this->arr[mx - 1].next = -1;
    this->cur_size = 0;
    this->mx_size = mx;
    pthread_mutex_unlock(&symbol_table_mutex);
}
int s_table::insert(uint32_t addr, uint32_t unit_size, uint32_t total_size)
{
    pthread_mutex_lock(&symbol_table_mutex);
    if (this->cur_size == this->mx_size)
    {
        pthread_mutex_lock(&print_mutex);
        printf("[s_table::insert]: Symbol table is full\n");
        pthread_mutex_unlock(&print_mutex);
        pthread_mutex_unlock(&symbol_table_mutex);
        return -1;
    }
    int idx = head_idx;
    // pthread_mutex_lock(&print_mutex);
    // printf("[s_table::insert]: Inserting at index %d\n", idx);
    // pthread_mutex_unlock(&print_mutex);
    this->arr[idx].addr_in_mem = addr;
    this->arr[idx].unit_size = unit_size;
    this->arr[idx].total_size = total_size;
    this->arr[idx].next |= 1; // mark as allocated
    head_idx = arr[idx].next >> 1;
    this->cur_size++;
    pthread_mutex_lock(&print_mutex);
    printf("[s_table::insert]: Added symbol table entry %d, allocated at %d, new head %d\n", idx, addr, head_idx);
    pthread_mutex_unlock(&print_mutex);
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
    this->arr[idx].addr_in_mem = -1;
    this->arr[idx].next = -1;
    this->arr[idx].total_size = 0;
    this->arr[idx].unit_size = 0;
    this->arr[tail_idx].next = idx << 1;
    this->arr[tail_idx].next |= 1;
    tail_idx = idx;
    this->cur_size--;
    pthread_mutex_lock(&print_mutex);
    printf("[s_table::remove]: Removed variable at index %d, marked it %d \n", idx, this->arr[idx].next & 1);
    pthread_mutex_unlock(&print_mutex);
    pthread_mutex_unlock(&symbol_table_mutex);
}
void s_table::unmark(uint32_t idx)
{
    this->arr[idx].next &= -2;
}
void s_table::print_s_table()
{
    pthread_mutex_lock(&symbol_table_mutex);
    pthread_mutex_lock(&print_mutex);
    printf("[s_table::print_s_table]: Printing elements of symbol table to be deleted, cur total size %d\n", this->cur_size);
    for (int i = 0; i < this->mx_size; i++)
        if (!(this->arr[i].next & 1))
            printf("[s_table::print_s_table]: Entry %d: addr: %d, unit_size: %d, total_size: %d\n", i, this->arr[i].addr_in_mem, this->arr[i].unit_size, this->arr[i].total_size);
    pthread_mutex_unlock(&print_mutex);
    pthread_mutex_unlock(&symbol_table_mutex);
}
#ifndef NO_GC
void GarbageCollector::gc_run_inner()
{
    pthread_mutex_lock(&symbol_table_mutex);
    pthread_mutex_lock(&stack_mutex);
    for (int i = 0; i <= GLOBAL_STACK->top; i++) // remove all other entries which are to be freed from the stack
    {
        if (GLOBAL_STACK->arr[i].scope_tbf & 1)
        {
            if (~SYMBOL_TABLE->arr[GLOBAL_STACK->arr[i].redirect - SYMBOL_TABLE->arr].addr_in_mem)
            {
                SYMBOL_TABLE->unmark(GLOBAL_STACK->arr[i].redirect - SYMBOL_TABLE->arr);
            }
            GLOBAL_STACK->top--;
            for (int j = i; j < GLOBAL_STACK->top; j++)
                GLOBAL_STACK->arr[j] = GLOBAL_STACK->arr[j + 1];
        }
    }
    for (int i = 0; i < SYMBOL_TABLE->mx_size; i++)
    {
        if (!(SYMBOL_TABLE->arr[i].next & 1))
        {
            pthread_mutex_lock(&print_mutex);
            printf("[GarbageCollector::gc_run_inner]: Freeing symbol table entry %d\n", i);
            pthread_mutex_unlock(&print_mutex);
            freeElem_inner(&SYMBOL_TABLE->arr[i]);
        }
    }
    pthread_mutex_unlock(&stack_mutex);
    pthread_mutex_unlock(&symbol_table_mutex);
    this->compact_total();
    print_big_memory();
    // printf("done with gc_run_inner\n");
}
int GarbageCollector::compact_once()
{
    pthread_mutex_lock(&symbol_table_mutex);

    pthread_mutex_lock(&memory_mutex);
    int compact_count = 0;
    // write compact once code here
    // traverse the list
    // at the first hole
    // copy the elements
    // remember the old address
    // find the entry in the symbol with the old address and update it
    int *p = BIG_MEMORY;
    int *next = p + (*p >> 1);
    while (next != BIG_MEMORY + big_memory_sz)
    {
        if ((*p & 1) == 0 && (*next & 1) == 1)
        {
            int sz1 = *p >> 1;
            int sz2 = *next >> 1;
            memmove(p, next, sz2 << 2);

            for (int j = 0; j < SYMBOL_TABLE->mx_size; j++)
            {
                if (SYMBOL_TABLE->arr[j].addr_in_mem + 1 == (next - BIG_MEMORY))
                {
                    SYMBOL_TABLE->arr[j].addr_in_mem = (p - BIG_MEMORY);
                    break;
                }
            }

            p = p + sz2;
            *p = sz1 << 1;
            *(p + sz1 - 1) = sz1 << 1;
            next = p + sz1;
            if (next < BIG_MEMORY + big_memory_sz and !(*next & 1)) // coalesce if the next block is free
            {
                sz1 = sz1 + (*next >> 1);
                *p = sz1 << 1;
                *(p + sz1 - 1) = sz1 << 1;
                next = p + sz1;
            }
            compact_count++;
            break;
        }
        else
        {
            p = next;
            next = p + (*p >> 1);
        }
    }
    if (compact_count)
    {
        pthread_mutex_lock(&print_mutex);
        printf("[GarbageCollector::compact_once]: Compacted first hole\n");
        pthread_mutex_unlock(&print_mutex);
        // print_big_memory();
    }
    pthread_mutex_unlock(&memory_mutex);
    pthread_mutex_unlock(&symbol_table_mutex);

    return compact_count;
}
void GarbageCollector::compact_total()
{
    pthread_mutex_lock(&print_mutex);
    printf("[GarbageCollector::compact_total]: Before compaction\n");
    pthread_mutex_unlock(&print_mutex);
    print_big_memory();
    while (this->compact_once())
        ;
    pthread_mutex_lock(&print_mutex);
    printf("[GarbageCollector::compact_total]: Done compacting\n");
    pthread_mutex_unlock(&print_mutex);
}
void gc_run(int signum)
{
    GC->gc_run_inner();
}
void GarbageCollector::gc_init()
{
    signal(SIGUSR1, gc_run);
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR1);
    while (true)
    {
        pthread_mutex_lock(&gc_active_mutex);
        if (!GC_ACTIVE)
        {
            pthread_mutex_unlock(&gc_active_mutex);
            break;
        }
        pthread_mutex_unlock(&gc_active_mutex);

        usleep(20 * 1000);
        pthread_sigmask(SIG_BLOCK, &sigset, NULL);
        GC->gc_run_inner();
        pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);
    }
}
void *garbageCollector(void *)
{
    GC->gc_init();
    return NULL;
}
#endif
void CreateMemory(int size)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&symbol_table_mutex, &attr);
    pthread_mutex_init(&stack_mutex, &attr);
    pthread_mutex_init(&memory_mutex, &attr);
    pthread_mutex_init(&gc_active_mutex, &attr);
    pthread_mutex_init(&print_mutex, &attr);

    pthread_mutex_lock(&print_mutex);
    printf("[CreateMemory]: Set up mutexes\n");
    pthread_mutex_unlock(&print_mutex);

    pthread_mutex_lock(&memory_mutex);
    // BIG_MEMORY = (int *)calloc(((size + 3) / 4), sizeof(int));
    BIG_MEMORY = new int[((size + 3) / 4)]();
    pthread_mutex_unlock(&memory_mutex);
    big_memory_sz = ((size + 3) / 4); // nearest multiple of 4 to size
    pthread_mutex_lock(&print_mutex);
    printf("[CreateMemory]: Allocated %d bytes of data as requested\n", ((size + 3) / 4) * 4);
    pthread_mutex_unlock(&print_mutex);

    // BOOKKEEP_MEMORY = (int *)calloc(bookkeeping_memory_size, sizeof(int));
    BOOKKEEP_MEMORY = new int[bookkeeping_memory_size]();
    BIG_MEMORY[0] = (big_memory_sz) << 1;
    BIG_MEMORY[big_memory_sz - 1] = (big_memory_sz) << 1;
    // printf("[CreateMemory]: Big memory header and footer %d\n", BIG_MEMORY[0]);

    pthread_mutex_lock(&print_mutex);
    printf("[CreateMemory]: Allocated %d bytes of data for bookkeeping\n", bookkeeping_memory_size);
    pthread_mutex_unlock(&print_mutex);

    pthread_mutex_lock(&stack_mutex);
    GLOBAL_STACK = (stack *)(BOOKKEEP_MEMORY);
    GLOBAL_STACK->stack_init(max_stack_size, (stack_entry *)(GLOBAL_STACK + 1));
    pthread_mutex_unlock(&stack_mutex);

    pthread_mutex_lock(&symbol_table_mutex);
    s_table *S_TABLE_START = (s_table *)(GLOBAL_STACK->arr + GLOBAL_STACK->max_size);
    SYMBOL_TABLE = S_TABLE_START;
    SYMBOL_TABLE->s_table_init(max_stack_size, (s_table_entry *)(SYMBOL_TABLE + 1));

    pthread_mutex_lock(&print_mutex);
    printf("[CreateMemory]: Setup Stack and Symbol Table\n");
    pthread_mutex_unlock(&print_mutex);
    pthread_mutex_unlock(&symbol_table_mutex);

#ifndef NO_GC
    GC = new GarbageCollector;
    GC_ACTIVE = 1;
    pthread_attr_t attr_t;
    pthread_attr_init(&attr_t);
    pthread_create(&GC->gc_thread, &attr_t, garbageCollector, NULL);
#endif
}
int CreatePartitionMainMemory(int size)
{
    // Format:
    // input size= bytes
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
    while ((p - BIG_MEMORY < big_memory_sz) && ((*p & 1) || ((*p << 1) < newsize)))
        p = p + (*p >> 1);
    if (p - BIG_MEMORY >= big_memory_sz)
    {
        cout << p - BIG_MEMORY << " " << big_memory_sz << endl;
        // printf("hello\n");
        pthread_mutex_unlock(&memory_mutex);
        return -1;
    }
    // found a block with size >= size wanted
    // printf("size: %d\n", newsize);
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
    pthread_mutex_lock(&memory_mutex);
    *ptr = *ptr & -2;                                      // clear allocated flag
    *(ptr + (*(ptr) >> 1) - 1) = *ptr;                     // clear allocated flag
    int *next = ptr + (*ptr >> 1);                         // find next block
    if (next - BIG_MEMORY < big_memory_sz && !(*next & 1)) // if next block is free
    {
        *ptr += *next;                   // merge with next block
        *(ptr + (*ptr >> 1) - 1) = *ptr; // update boundary tag
    }
    if (ptr != BIG_MEMORY) // there is a block before
    {
        int *prev = ptr - (*(ptr - 1) >> 1); // find previous block
        if (!(*prev & 1))                    // if previous block is free
        {
            *prev += *ptr; // merge with previous block
            *(prev + (*prev >> 1) - 1) = *prev;
        }
    }
    pthread_mutex_unlock(&memory_mutex);
}
s_table_entry *CreateVar(DATATYPE a)
{
    int main_memory_idx, unit_size;
    switch (a)
    {
    case INT:
        unit_size = 32;
        main_memory_idx = CreatePartitionMainMemory(4);
        break;
    case MEDIUM_INT:
        unit_size = 24;
        main_memory_idx = CreatePartitionMainMemory(3);
        break;
    case CHAR:
        unit_size = 8;
        main_memory_idx = CreatePartitionMainMemory(1);
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
        pthread_mutex_lock(&print_mutex);
        printf("[CreateVar]: Failed to create variable\n");
        pthread_mutex_unlock(&print_mutex);
        return NULL;
    }
    pthread_mutex_lock(&print_mutex);
    printf("[CreateVar]: Created variable of size %d at index %d\n", unit_size, main_memory_idx);
    pthread_mutex_unlock(&print_mutex);
    // pthread_mutex_lock(&symbol_table_mutex);
    int idx = SYMBOL_TABLE->insert(main_memory_idx + 1, unit_size, unit_size); // add plus one to account for header
    // printf("[CreateVar]: Inserted variable into symbol table at index %d\n", idx);
    // pthread_mutex_unlock(&symbol_table_mutex);
    GLOBAL_STACK->push(SYMBOL_TABLE->arr + idx);
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
        main_memory_idx = CreatePartitionMainMemory(total_size / 8);
        break;
    case MEDIUM_INT:
        unit_size = 24; // I am not storing medium int arrays compactly, I dont think the implementation over head of reading two blocks ( eg 24-12 12-24 ) is worth the memory savings
        total_size = 32 * sz;
        main_memory_idx = CreatePartitionMainMemory(total_size / 8);
        break;
    case CHAR:
        unit_size = 8;
        total_size = unit_size * sz;
        main_memory_idx = CreatePartitionMainMemory(total_size / 8);
        break;
    case BOOL:
        unit_size = 1;
        total_size = unit_size * sz;
        main_memory_idx = CreatePartitionMainMemory((total_size + 7) / 8);
        break;
    default:
        main_memory_idx = -1;
        break;
    }
    if (main_memory_idx == -1)
    {
        pthread_mutex_lock(&print_mutex);
        printf("[CreateArr]: Failed to create array\n");
        pthread_mutex_unlock(&print_mutex);
        return NULL;
    }
    // pthread_mutex_lock(&symbol_table_mutex);
    int idx = SYMBOL_TABLE->insert(main_memory_idx + 1, unit_size, total_size); // add plus one to account for header
    pthread_mutex_lock(&print_mutex);
    printf("[CreateArr]: Created array of size %d, total size %d at index %d\n", unit_size, total_size, main_memory_idx);
    pthread_mutex_unlock(&print_mutex);
    // pthread_mutex_unlock(&symbol_table_mutex);
    GLOBAL_STACK->push(SYMBOL_TABLE->arr + idx);

    return &SYMBOL_TABLE->arr[idx];
}
void AssignVar(s_table_entry *var, int val)
{
    if (var->unit_size == 32)
    {
        pthread_mutex_lock(&memory_mutex);
        *((int *)(BIG_MEMORY + var->addr_in_mem)) = val;
        pthread_mutex_unlock(&memory_mutex);
        // pthread_mutex_lock(&print_mutex);
        // printf("[AssignVar]: Assigned %d to variable at index %d, it looks like %d\n", val, var->addr_in_mem, *((BIG_MEMORY + var->addr_in_mem)));
        // pthread_mutex_unlock(&print_mutex);
    }
    else if (var->unit_size >= 8)
    {
        pthread_mutex_lock(&memory_mutex);
        *((int *)(BIG_MEMORY + var->addr_in_mem)) = (val << (32 - var->unit_size)); // shift left to align, offset should be 0 in the word
        pthread_mutex_unlock(&memory_mutex);
        // pthread_mutex_lock(&print_mutex);
        // printf("[AssignVar]: Assigned %d to variable at index %d, it looks like %d\n", val, var->addr_in_mem, (*((BIG_MEMORY + var->addr_in_mem))) >> (32 - var->unit_size));
        // pthread_mutex_unlock(&print_mutex);
    }
    else
    {
        if (val == 0 or val == 1)
        {
            pthread_mutex_lock(&memory_mutex);
            *((int *)(BIG_MEMORY + var->addr_in_mem)) = (val << 31); // shift left to align, offset should be 0 in the word
            pthread_mutex_unlock(&memory_mutex);
            // pthread_mutex_lock(&print_mutex);
            // printf("[AssignVar]: Assigned %d to variable at index %d, it looks like %d\n", val, var->addr_in_mem, (uint32_t)(*((BIG_MEMORY + var->addr_in_mem))) >> 31);
            // pthread_mutex_unlock(&print_mutex);
        }
        else
        {
            pthread_mutex_lock(&print_mutex);
            printf("[AssignVar]: Trying to assign an integer to a bool, type checking failed\n");
            pthread_mutex_unlock(&print_mutex);
        }
    }
}
// void AssignVar(s_table_entry *var, char val)
// {
//     pthread_mutex_lock(&symbol_table_mutex);
//     if (var->unit_size >= 8)
//     {
//         pthread_mutex_lock(&memory_mutex);
//         *((int *)(BIG_MEMORY + var->addr_in_mem)) = val;
//         pthread_mutex_unlock(&memory_mutex);
//         printf("[AssignVar]: Assigned %d to variable at index %d, it looks like %d\n", val, var->addr_in_mem, *((BIG_MEMORY + var->addr_in_mem)));
//     }
//     else
//     {
//         if ((int)val == 0 or (int) val == 1)
//         {
//             pthread_mutex_lock(&memory_mutex);
//             *((int *)(BIG_MEMORY + var->addr_in_mem)) = ((int)val << 31); // shift left to align, offset should be 0 in the word
//             pthread_mutex_unlock(&memory_mutex);
//             printf("[AssignVar]: Assigned %d to variable at index %d, it looks like %d\n", (int)val, var->addr_in_mem, *((BIG_MEMORY + var->addr_in_mem)));
//         }
//         else
//         {
//             printf("[AssignVar]: Trying to assign an character to a bool, type checking failed\n");
//         }
//     }
//     pthread_mutex_unlock(&symbol_table_mutex);
// }
uint32_t accessVar(s_table_entry *var, int idx)
{
    int correct_unit_size = var->unit_size;
    if (correct_unit_size == 24)
        correct_unit_size = 32;
    int main_idx = var->addr_in_mem + (idx * correct_unit_size) / 32;
    // cout << "main idx: " << main_idx << endl;
    int offset = (idx * correct_unit_size) % 32;
    int end_offset = (offset + correct_unit_size - 1) % 32;
    pthread_mutex_lock(&memory_mutex);
    uint32_t val = *((int *)(BIG_MEMORY + main_idx));
    pthread_mutex_unlock(&memory_mutex);
    // printf(, main_idx, val);
    // cout << "\t[AccessVar]: Accessed raw variable at index" << main_idx << " , it looks like " << std::bitset<32>(val) << "\t";
    // cout<<"...."<<offset<<" "<<end_offset<<".....\n";
    // start from offset and read till unit_size
    val = ~((1L << (31 - end_offset)) - 1) & val; // remove all bits after offset  val looks like  ......usefulf000000
    // cout<< std::bitset<32>(val)<<"\n";
    val = val << offset; // shift left to align          val looks liek useful000000000000
    // cout<< std::bitset<32>(val)<<"\n";
    int ret = val;
    ret = ret >> (32 - var->unit_size); // shift right to align         val looks like 0000000000useful
    // cout<< std::bitset<32>(val)<<"\n";
    return ret;
}
void AssignArray(s_table_entry *arr, int idx, uint32_t val)
{
    // cout<<arr->unit_size<<endl;
    if (arr->unit_size == 32)
    {
        pthread_mutex_lock(&memory_mutex);
        *((int *)(BIG_MEMORY + arr->addr_in_mem + (idx * arr->unit_size) / 32)) = val;
        pthread_mutex_unlock(&memory_mutex);
        // pthread_mutex_lock(&print_mutex);
        // printf("[AssignArr]: Assigned %d to array at index %d, it looks like %d\n", val, arr->addr_in_mem + (idx * arr->unit_size) / 32, *((BIG_MEMORY + arr->addr_in_mem + (idx * arr->unit_size) / 32)));
        // pthread_mutex_unlock(&print_mutex);
    }
    else if (arr->unit_size == 24)
    {
        pthread_mutex_lock(&memory_mutex);
        *(int *)(BIG_MEMORY + arr->addr_in_mem + idx) = val << (32 - arr->unit_size);
        pthread_mutex_unlock(&memory_mutex);
        // pthread_mutex_lock(&print_mutex);
        // printf("[AssignArr]: Assigned %d to array at index %d, it looks like %d\n", val, arr->addr_in_mem + idx, *(BIG_MEMORY + arr->addr_in_mem + idx));
        // pthread_mutex_unlock(&print_mutex);
    }
    else if (arr->unit_size == 8)
    {
        pthread_mutex_lock(&memory_mutex);
        int wordidx = (idx * arr->unit_size) / 32;
        int offset = (idx * arr->unit_size) % 32;
        *(int *)(BIG_MEMORY + arr->addr_in_mem + wordidx) |= (val << (32 - arr->unit_size - offset));
        pthread_mutex_unlock(&memory_mutex);
        // pthread_mutex_lock(&print_mutex);
        // printf("[AssignArr]: Assigned %d to array at index %d, it looks like %d\n", val, arr->addr_in_mem + wordidx, *(BIG_MEMORY + arr->addr_in_mem + wordidx));
        // pthread_mutex_unlock(&print_mutex);
    }
    else
    {
        if (val == 0 or val == 1)
        {
            pthread_mutex_lock(&memory_mutex);
            int wordidx = (idx * arr->unit_size) / 32;
            int offset = (idx * arr->unit_size) % 32;
            if (val == 1)
                *(int *)(BIG_MEMORY + arr->addr_in_mem + wordidx) |= (1 << (31 - offset));
            else
                // -1 is all 1s, 1 << (31-offset) is 1 at only offset bit, so remove that from -1 will give all 1s except offset bit
                *(int *)(BIG_MEMORY + arr->addr_in_mem + wordidx) &= ((-1) - (1 << (31 - offset)));
            pthread_mutex_unlock(&memory_mutex);
            // pthread_mutex_lock(&print_mutex);
            // printf("[AssignArr]: Assigned %d to array at index %d, it looks like %d\n", val, arr->addr_in_mem + wordidx, *(BIG_MEMORY + arr->addr_in_mem + wordidx));
            // pthread_mutex_unlock(&print_mutex);
        }
        else
        {
            pthread_mutex_lock(&print_mutex);
            printf("[AssignArr]: Trying to assign a non bool to a bool, type checking failed\n");
            pthread_mutex_unlock(&print_mutex);
        }
    }
}
void print_big_memory()
{
    pthread_mutex_lock(&memory_mutex);
    pthread_mutex_lock(&print_mutex);
    printf("[PrintBigMemory]: Big Memory looks like\n");
    pthread_mutex_unlock(&print_mutex);
    int *ptr = BIG_MEMORY;
    while (ptr - BIG_MEMORY < big_memory_sz)
    {
        if (*ptr & 1) // allocated
        {
            pthread_mutex_lock(&print_mutex);
            printf("[PrintBigMemory]: Allocated block of size %d\n", (*ptr) >> 1);
            // cout << "\t Header looks like" << bitset<32>(*ptr) << "\n";
            // cout << "\t";

            // for (int i = 1; i + 1 < (*ptr >> 1); i++)
            // {
            //     cout << std::bitset<32>(ptr[i]) << "\t";
            // }
            // cout << endl;
            pthread_mutex_unlock(&print_mutex);
        }
        else
        {
            pthread_mutex_lock(&print_mutex);

            printf("[PrintBigMemory]: Unallocated block of size %d\n", (*ptr) >> 1);
            pthread_mutex_unlock(&print_mutex);
        }
        ptr = ptr + ((*ptr) >> 1);
    }
    // printf("\n");
    pthread_mutex_unlock(&memory_mutex);
}
void Assign_array_in_range(s_table_entry *var, int begin, int end, uint32_t val)
{
    for (int i = begin; i < end; i++)
    {
        if (i >= var->total_size / var->unit_size)
        {
            pthread_mutex_lock(&print_mutex);

            printf("[Assign_array_in_range]: right more than array size, aborting\n");
            pthread_mutex_unlock(&print_mutex);
            return;
        }
        AssignArray(var, i, val);
    }
}
void freeElem(s_table_entry *var)
{
    pthread_mutex_lock(&symbol_table_mutex);
    SYMBOL_TABLE->unmark(var - SYMBOL_TABLE->arr);
    pthread_mutex_unlock(&symbol_table_mutex);
}
void freeElem_inner(s_table_entry *var)
{
    for (int i = 0; i <= GLOBAL_STACK->top; i++)
    {
        if (GLOBAL_STACK->arr[i].redirect == var)
        {
            GLOBAL_STACK->arr[i].scope_tbf |= 1;
            break;
        }
    }
    pthread_mutex_lock(&print_mutex);
    cout << "\t[freeElem_inner]: Freeing at big memory address: " << var->addr_in_mem - 1 << endl;
    pthread_mutex_unlock(&print_mutex);
    if (var->addr_in_mem and var->addr_in_mem - 1 < big_memory_sz)
    {
        FreePartitionMainMemory(BIG_MEMORY + (var->addr_in_mem - 1)); // addr is one after the header so -1
        SYMBOL_TABLE->remove(var - SYMBOL_TABLE->arr);
        // cout<<"hi2\n";
    }
    // print_big_memory();
}
void freeMem()
{
#ifndef NO_GC
    if (GC_ACTIVE)
    {
        pthread_mutex_lock(&gc_active_mutex);
        GC_ACTIVE = 0;
        pthread_mutex_unlock(&gc_active_mutex);
        pthread_join(GC->gc_thread, NULL);
        // pthread_kill(GC->gc_thread, 9);
        pthread_mutex_lock(&print_mutex);
        printf("[freeMem]: GC thread joined\n");
        pthread_mutex_unlock(&print_mutex);
    }
#endif
    delete[](BIG_MEMORY);

    pthread_mutex_lock(&print_mutex);
    printf("[freeMem]: Big memory freed\n");
    pthread_mutex_unlock(&print_mutex);
    delete[](BOOKKEEP_MEMORY);
    pthread_mutex_lock(&print_mutex);
    printf("[freeMem]: Book keeping memory freed\n");
    pthread_mutex_unlock(&print_mutex);
    pthread_mutex_destroy(&memory_mutex);
    pthread_mutex_destroy(&symbol_table_mutex);
    pthread_mutex_destroy(&stack_mutex);
    pthread_mutex_lock(&print_mutex);
    printf("[freeMem]: mutexes destroyed\n");
    pthread_mutex_unlock(&print_mutex);
}
// this function was used for testing the code demo3.c
// int main()
// {
//     // only for test, remove later
//     CreateMemory(250e6);
//     pthread_mutex_lock(&print_mutex);

//     printf("[Main]: Symbol table at the start\n");
//     pthread_mutex_unlock(&print_mutex);

//     SYMBOL_TABLE->print_s_table();
//     startScope();
//     GLOBAL_STACK->StackTrace();
//     print_big_memory();
//     if (1)
//     {
//         startScope();
//         auto int_var = CreateVar(DATATYPE::INT);
//         AssignVar(int_var, 3);
//         pthread_mutex_lock(&print_mutex);

//         cout << "[Main]: accessing int: " << accessVar(int_var) << endl;
//         pthread_mutex_unlock(&print_mutex);

//         SYMBOL_TABLE->print_s_table();
//         print_big_memory();
//         endScope();
//         GLOBAL_STACK->StackTrace();
//     }
//     if (1)
//     {
//         startScope();
//         auto char_var = CreateVar(DATATYPE::CHAR);
//         AssignVar(char_var, 'b');
//         cout << "[Main]: accessing char: " << accessVar(char_var) << endl;
//         SYMBOL_TABLE->print_s_table();
//         endScope();
//         GLOBAL_STACK->StackTrace();
//     }
//     if (1)
//     {
//         startScope();
//         auto bool_var = CreateVar(DATATYPE::BOOL);
//         AssignVar(bool_var, 1);
//         cout << "[Main]: accessing bool: " << accessVar(bool_var) << endl;
//         SYMBOL_TABLE->print_s_table();
//         endScope();
//         GLOBAL_STACK->StackTrace();
//     }
//     if (1)
//     {
//         startScope();
//         auto mint_var = CreateVar(DATATYPE::MEDIUM_INT);
//         AssignVar(mint_var, 42);
//         cout << "[Main]: accessing med_int: " << accessVar(mint_var) << endl;
//         SYMBOL_TABLE->print_s_table();
//         endScope();
//         GLOBAL_STACK->StackTrace();
//     }
//     print_big_memory();
//     auto int_arr_var = CreateArray(DATATYPE::INT, 4);
//     Assign_array_in_range(int_arr_var, 0, 2, -50);
//     Assign_array_in_range(int_arr_var, 2, 4, -76);
//     cout << "[Main]: accessing int array at 0: " << (int)accessVar(int_arr_var, 0) << endl;
//     cout << "[Main]: accessing int array at 1: " << (int)accessVar(int_arr_var, 1) << endl;
//     cout << "[Main]: accessing int array at 2: " << (int)accessVar(int_arr_var, 2) << endl;

//     auto char_arr_var = CreateArray(DATATYPE::CHAR, 5);
//     Assign_array_in_range(char_arr_var, 0, 2, 'a');
//     Assign_array_in_range(char_arr_var, 2, 5, 'A');
//     cout << "[Main]: accessing char array at 0: " << (char)accessVar(char_arr_var, 0) << endl;
//     cout << "[Main]: accessing char array at 1: " << (char)accessVar(char_arr_var, 1) << endl;
//     cout << "[Main]: accessing char array at 2: " << (char)accessVar(char_arr_var, 2) << endl;
//     cout << "[Main]: accessing char array at 4: " << (char)accessVar(char_arr_var, 4) << endl;

//     auto bool_arr_var = CreateArray(DATATYPE::BOOL, 33);
//     Assign_array_in_range(bool_arr_var, 0, 10, 1);
//     Assign_array_in_range(bool_arr_var, 17, 33, 1);
//     cout << "[Main]: accessing bool array at 0: " << (bool)accessVar(bool_arr_var, 0) << endl;
//     cout << "[Main]: accessing bool array at 1: " << (bool)accessVar(bool_arr_var, 1) << endl;
//     cout << "[Main]: accessing bool array at 2: " << (bool)accessVar(bool_arr_var, 2) << endl;
//     cout << "[Main]: accessing bool array at 32: " << (bool)accessVar(bool_arr_var, 32) << endl;

//     auto mint_arr_var = CreateArray(DATATYPE::MEDIUM_INT, 5);
//     Assign_array_in_range(mint_arr_var, 0, 2, -1 * 'a');
//     Assign_array_in_range(mint_arr_var, 2, 5, 'A');
//     cout << "[Main]: accessing mint array at 0: " << (int)accessVar(mint_arr_var, 0) << endl;
//     cout << "[Main]: accessing mint array at 1: " << (int)accessVar(mint_arr_var, 1) << endl;
//     cout << "[Main]: accessing mint array at 2: " << (int)accessVar(mint_arr_var, 2) << endl;
//     cout << "[Main]: accessing mint array at 4: " << (int)accessVar(mint_arr_var, 4) << endl;
//     GLOBAL_STACK->StackTrace();
//     // printf("line 655\n");
//     print_big_memory();
//     freeElem(char_arr_var);
//     // endScope();
//     SYMBOL_TABLE->print_s_table();
//     GLOBAL_STACK->StackTrace();
//     // print_big_memory();
//     // usleep(20000);
//     // cout << "helllo\n";
//     print_big_memory();
//     SYMBOL_TABLE->print_s_table();
//     GLOBAL_STACK->StackTrace();
//     freeMem();
//     return 0;
// }
// demo1.c
// void runn(s_table_entry *ptr1, s_table_entry *ptr2)
// {
//     startScope();
//     s_table_entry *i = CreateVar(DATATYPE::INT);
//     if (!i)
//         printf("i is null\n");
//     AssignVar(i, 0);
//     s_table_entry *arr;
//     if (ptr1->unit_size == 32)
//         arr = CreateArray(DATATYPE::INT, 50000);
//     else if (ptr1->unit_size == 24)
//         arr = CreateArray(DATATYPE::MEDIUM_INT, 50000);
//     else if (ptr1->unit_size == 8)
//         arr = CreateArray(DATATYPE::CHAR, 50000);
//     else if (ptr1->unit_size == 1)
//         arr = CreateArray(DATATYPE::BOOL, 50000);
//     else
//         exit(1);
//     while ((int)accessVar(i) < 50000)
//     {
//         AssignArray(arr, (int)accessVar(i), rand() % 2);
//         AssignVar(i, (int)accessVar(i) + 1);
//     }
//     endScope();
// }

// int main()
// {

//     CreateMemory(3e8);
//     startScope();
//     s_table_entry *var1 = CreateVar(DATATYPE::MEDIUM_INT);
//     s_table_entry *var2 = CreateVar(DATATYPE::MEDIUM_INT);
//     runn(var1, var2);

//     var1 = CreateVar(DATATYPE::CHAR);
//     var2 = CreateVar(DATATYPE::CHAR);
//     runn(var1, var2);

//     var1 = CreateVar(DATATYPE::BOOL);
//     var2 = CreateVar(DATATYPE::BOOL);
//     runn(var1, var2);

//     var1 = CreateVar(DATATYPE::INT);
//     var2 = CreateVar(DATATYPE::INT);
//     runn(var1, var2);

//     var1 = CreateVar(DATATYPE::MEDIUM_INT);
//     var2 = CreateVar(DATATYPE::MEDIUM_INT);
//     runn(var1, var2);

//     var1 = CreateVar(DATATYPE::CHAR);
//     var2 = CreateVar(DATATYPE::CHAR);
//     runn(var1, var2);

//     var1 = CreateVar(DATATYPE::BOOL);
//     var2 = CreateVar(DATATYPE::BOOL);
//     runn(var1, var2);

//     var1 = CreateVar(DATATYPE::INT);
//     var2 = CreateVar(DATATYPE::INT);
//     runn(var1, var2);

//     var1 = CreateVar(DATATYPE::MEDIUM_INT);
//     var2 = CreateVar(DATATYPE::MEDIUM_INT);
//     runn(var1, var2);

//     var1 = CreateVar(DATATYPE::CHAR);
//     var2 = CreateVar(DATATYPE::CHAR);
//     runn(var1, var2);
//     usleep(200000);
//     // print_big_memory();
//     endScope();
//     SYMBOL_TABLE->print_s_table();
//     GLOBAL_STACK->StackTrace();
//     freeMem();
//     return 0;
// }
// demo 2
// void fibonacci(s_table_entry *k, s_table_entry *dp)
// {
//     AssignArray(dp, 0, 1);
//     AssignArray(dp, 1, 1);
//     s_table_entry *i = CreateVar(DATATYPE::INT);
//     AssignVar(i, 2);
//     while (accessVar(i) <= accessVar(k))
//     {
//         AssignArray(dp, accessVar(i), accessVar(dp, accessVar(i) - 1) + accessVar(dp, accessVar(i) - 2));
//         AssignVar(i, accessVar(i) + 1);
//     }
// }

// s_table_entry *fibnacciProduct(s_table_entry *k)
// {
//     s_table_entry *dp = CreateArray(DATATYPE::INT, accessVar(k) + 1);
//     startScope();
//     fibonacci(k, dp);
//     endScope();
//     s_table_entry *product = CreateVar(DATATYPE::INT);
//     AssignVar(product, 1);
//     s_table_entry *j = CreateVar(DATATYPE::INT);
//     AssignVar(j, 1);
//     while (accessVar(j) <= accessVar(k))
//     {
//         AssignVar(product, accessVar(dp, accessVar(j)) * accessVar(product));
//         AssignVar(j, accessVar(j) + 1);
//     }

//     return product;
// }

// int main(int argc, char **argv)
// {
//     CreateMemory(20000);
//     startScope();
//     s_table_entry *k = CreateVar(DATATYPE::INT);
//     // printPageTable();
//     AssignVar(k, atoi(argv[1]));
//     s_table_entry *retval = CreateVar(DATATYPE::INT);
//     startScope();
//     s_table_entry *product = fibnacciProduct(k);
//     AssignVar(retval, accessVar(product));
//     endScope();
//     cout << "Product of first " << accessVar(k) << " fibonacci nos. = " << accessVar(retval) << "\n";
//     endScope();
//     SYMBOL_TABLE->print_s_table();
//     GLOBAL_STACK->StackTrace();
//     freeMem();
//     // return 0;
// }