#include "memlab.h"
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
        printf("[stack::push] Stack Overflow\n");
        pthread_mutex_unlock(&stack_mutex);
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
        printf("[stack::pop] stack is empty\n");
        pthread_mutex_unlock(&stack_mutex);
        return NULL;
    }
    else
    {
        this->top--;
        auto ret = &this->arr[this->top + 1];
        printf("[stack::pop] poped element \n");
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
int startScope()
{
    return CURRENT_SCOPE++;
}
void endScope(int scope = CURRENT_SCOPE)
{
    pthread_mutex_lock(&symbol_table_mutex);
    pthread_mutex_lock(&stack_mutex);
    for (int i = 0; i <= GLOBAL_STACK->top; i++)
    {
        if ((GLOBAL_STACK->arr[i].scope_tbf >> 1) == scope)
        {
            SYMBOL_TABLE->unmark(GLOBAL_STACK->arr[i].redirect - SYMBOL_TABLE->arr);
            GLOBAL_STACK->arr[i].scope_tbf |= 1;
        }
    }
    CURRENT_SCOPE--;
    pthread_mutex_unlock(&stack_mutex);
    pthread_mutex_unlock(&symbol_table_mutex);
}
void stack::StackTrace()
{
    pthread_mutex_lock(&stack_mutex);
    printf("[StackTrace]: Stack has %d entries, it looks like:\n", this->top + 1);
    for (int i = this->top; ~i; i--)
        printf("[StackTrace]: Scope Number %d \t Redirect %ld \t To be free %d\n", this->arr[i].scope_tbf >> 1, this->arr[i].redirect - SYMBOL_TABLE->arr, this->arr[i].scope_tbf & 1);
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
        this->arr[i].next = ((i + 1) << 1);
        this->arr[i].next |= 1;
    }
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
        printf("[s_table::insert]: Symbol table is full\n");
        pthread_mutex_unlock(&symbol_table_mutex);
        return -1;
    }
    int idx = head_idx;
    printf("[s_table::insert]: Inserting at index %d\n", idx);
    this->arr[idx].addr_in_mem = addr;
    this->arr[idx].unit_size = unit_size;
    this->arr[idx].total_size = total_size;
    this->arr[idx].next |= 1; // mark as allocated
    head_idx = arr[idx].next >> 1;
    this->cur_size++;
    printf("[s_table::insert]: Added symbol table entry %d, allocated at %d, new head %d\n", idx, addr, head_idx);
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
    printf("[s_table::remove]: Removed variable at index %d, marked it %d \n", idx, this->arr[idx].next & 1);
    pthread_mutex_unlock(&symbol_table_mutex);
}
void s_table::unmark(uint32_t idx)
{
    this->arr[idx].next &= -2;
}
void s_table::print_s_table()
{
    pthread_mutex_lock(&symbol_table_mutex);
    printf("[s_table::print_s_table]: Printing elements of symbol table to be deleted, cur total size %d\n", this->cur_size);
    for (int i = 0; i < this->mx_size; i++)
        if (!(this->arr[i].next & 1))
            printf("[s_table::print_s_table]: Entry %d: addr: %d, unit_size: %d, total_size: %d  %d\n", i, this->arr[i].addr_in_mem, this->arr[i].unit_size, this->arr[i].total_size, this->arr[i].next);
    pthread_mutex_unlock(&symbol_table_mutex);
}
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
            printf("[GarbageCollector::gc_run_inner]: Freeing symbol table entry %d\n", i);
            freeElem_inner(&SYMBOL_TABLE->arr[i]);
        }
    }
    pthread_mutex_unlock(&stack_mutex);
    pthread_mutex_unlock(&symbol_table_mutex);
    this->compact_total();
    // printf("done with gc_run_inner\n");
}
void GarbageCollector::compact_total()
{
    return;
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

        usleep(20000);
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

    printf("[CreateMemory]: Set up mutexes\n");

    pthread_mutex_lock(&memory_mutex);
    // BIG_MEMORY = (int *)calloc(((size + 3) / 4), sizeof(int));
    BIG_MEMORY = new int[((size + 3) / 4)]();
    big_memory_sz = ((size + 3) / 4) * 4; // nearest multiple of 4 to size
    printf("[CreateMemory]: Allocated %d bytes of data as requested\n", ((size + 3) / 4) * 4);
    // BOOKKEEP_MEMORY = (int *)calloc(bookkeeping_memory_size, sizeof(int));
    BOOKKEEP_MEMORY = new int[bookkeeping_memory_size]();
    BIG_MEMORY[0] = (big_memory_sz) << 1;
    BIG_MEMORY[big_memory_sz - 1] = (big_memory_sz) << 1;
    // printf("[CreateMemory]: Big memory header and footer %d\n", BIG_MEMORY[0]);
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

    GC = new GarbageCollector;
    GC_ACTIVE = 1;
    pthread_attr_t attr_t;
    pthread_attr_init(&attr_t);
    pthread_create(&GC->gc_thread, &attr_t, garbageCollector, NULL);
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
        printf("[CreateVar]: Failed to create variable\n");
        return NULL;
    }
    printf("[CreateVar]: Created variable of size %d at index %d\n", unit_size, main_memory_idx);
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
        printf("[CreateArr]: Failed to create array\n");
        return NULL;
    }
    // pthread_mutex_lock(&symbol_table_mutex);
    int idx = SYMBOL_TABLE->insert(main_memory_idx + 1, unit_size, total_size); // add plus one to account for header
    printf("[CreateArr]: Created array of size %d, total size %d at index %d\n", unit_size, total_size, main_memory_idx);
    // pthread_mutex_unlock(&symbol_table_mutex);
    GLOBAL_STACK->push(SYMBOL_TABLE->arr + idx);

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
    else if (var->unit_size >= 8)
    {
        pthread_mutex_lock(&memory_mutex);
        *((int *)(BIG_MEMORY + var->addr_in_mem)) = (val << (32 - var->unit_size)); // shift left to align, offset should be 0 in the word
        pthread_mutex_unlock(&memory_mutex);
        printf("[AssignVar]: Assigned %d to variable at index %d, it looks like %d\n", val, var->addr_in_mem, (*((BIG_MEMORY + var->addr_in_mem))) >> (32 - var->unit_size));
    }
    else
    {
        if (val == 0 or val == 1)
        {
            pthread_mutex_lock(&memory_mutex);
            *((int *)(BIG_MEMORY + var->addr_in_mem)) = (val << 31); // shift left to align, offset should be 0 in the word
            pthread_mutex_unlock(&memory_mutex);
            printf("[AssignVar]: Assigned %d to variable at index %d, it looks like %d\n", val, var->addr_in_mem, (uint32_t)(*((BIG_MEMORY + var->addr_in_mem))) >> 31);
        }
        else
        {
            printf("[AssignVar]: Trying to assign an integer to a bool, type checking failed\n");
        }
    }
    pthread_mutex_unlock(&symbol_table_mutex);
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
uint32_t accessVar(s_table_entry *var, int idx = 0)
{
    pthread_mutex_lock(&symbol_table_mutex);
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
    pthread_mutex_unlock(&symbol_table_mutex);
    // exit(0);
    return ret;
}
void AssignArray(s_table_entry *arr, int idx, uint32_t val)
{
    pthread_mutex_lock(&symbol_table_mutex);
    // cout<<arr->unit_size<<endl;
    if (arr->unit_size == 32)
    {
        pthread_mutex_lock(&memory_mutex);
        *((int *)(BIG_MEMORY + arr->addr_in_mem + (idx * arr->unit_size) / 32)) = val;
        pthread_mutex_unlock(&memory_mutex);
        printf("[AssignArr]: Assigned %d to array at index %d, it looks like %d\n", val, arr->addr_in_mem + (idx * arr->unit_size) / 32, *((BIG_MEMORY + arr->addr_in_mem + (idx * arr->unit_size) / 32)));
    }
    else if (arr->unit_size == 24)
    {
        pthread_mutex_lock(&memory_mutex);
        *(int *)(BIG_MEMORY + arr->addr_in_mem + idx) = val << (32 - arr->unit_size);
        pthread_mutex_unlock(&memory_mutex);
        printf("[AssignArr]: Assigned %d to array at index %d, it looks like %d\n", val, arr->addr_in_mem + idx, *(BIG_MEMORY + arr->addr_in_mem + idx));
    }
    else if (arr->unit_size == 8)
    {
        pthread_mutex_lock(&memory_mutex);
        int wordidx = (idx * arr->unit_size) / 32;
        int offset = (idx * arr->unit_size) % 32;
        *(int *)(BIG_MEMORY + arr->addr_in_mem + wordidx) |= (val << (32 - arr->unit_size - offset));
        pthread_mutex_unlock(&memory_mutex);
        printf("[AssignArr]: Assigned %d to array at index %d, it looks like %d\n", val, arr->addr_in_mem + wordidx, *(BIG_MEMORY + arr->addr_in_mem + wordidx));
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
            printf("[AssignArr]: Assigned %d to array at index %d, it looks like %d\n", val, arr->addr_in_mem + wordidx, *(BIG_MEMORY + arr->addr_in_mem + wordidx));
        }
        else
        {
            printf("[AssignArr]: Trying to assign a non bool to a bool, type checking failed\n");
        }
    }
    pthread_mutex_unlock(&symbol_table_mutex);
}
void print_big_memory()
{
    pthread_mutex_lock(&memory_mutex);
    printf("[PrintBigMemory]: Big Memory looks like\n");
    int *ptr = BIG_MEMORY;
    while (ptr - BIG_MEMORY < big_memory_sz)
    {
        if (*ptr & 1) // allocated
        {
            printf("[PrintBigMemory]: Allocated block of size %d\n", (*ptr) >> 1);
            cout << "\t Header looks like" << bitset<32>(*ptr) << "\n";
            cout << "\t";
            for (int i = 1; i + 1 < (*ptr >> 1); i++)
            {
                cout << std::bitset<32>(ptr[i]) << "\t";
            }
            cout << endl;
        }
        else
        {
            printf("[PrintBigMemory]: Unallocated block of size %d\n", (*ptr) >> 1);
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
            printf("[Assign_array_in_range]: right more than array size, aborting\n");
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
    cout << "\t[freeElem_inner]: Freeing at big memory address: " << var->addr_in_mem - 1 << endl;
    if (var->addr_in_mem and var->addr_in_mem - 1 < big_memory_sz)
    {
        FreePartitionMainMemory(BIG_MEMORY + (var->addr_in_mem - 1)); // addr is one after the header so -1
        SYMBOL_TABLE->remove(var - SYMBOL_TABLE->arr);
    }
    // print_big_memory();
}
void freeMem()
{
    if (GC_ACTIVE)
    {
        pthread_mutex_lock(&gc_active_mutex);
        GC_ACTIVE = 0;
        pthread_mutex_unlock(&gc_active_mutex);
        pthread_join(GC->gc_thread, NULL);
        // pthread_kill(GC->gc_thread, 9);
        printf("[freeMem]: GC thread joined\n");
    }
    delete[](BIG_MEMORY);
    printf("[freeMem]: Big memory freed\n");
    delete[](BOOKKEEP_MEMORY);
    printf("[freeMem]: Book keeping memory freed\n");
    pthread_mutex_destroy(&memory_mutex);
    pthread_mutex_destroy(&symbol_table_mutex);
    pthread_mutex_destroy(&stack_mutex);
    printf("[freeMem]: mutexes destroyed\n");
}
int main()
{
    // only for test, remove later
    CreateMemory(1e6);
    printf("[Main]: Symbol table at the start\n");
    SYMBOL_TABLE->print_s_table();
    auto s1 = startScope();
    GLOBAL_STACK->StackTrace();
    print_big_memory();
    if (1)
    {
        startScope();
        auto int_var = CreateVar(DATATYPE::INT);
        AssignVar(int_var, 3);
        cout << "[Main]: accessing int: " << accessVar(int_var) << endl;
        SYMBOL_TABLE->print_s_table();
        print_big_memory();
        endScope();
        GLOBAL_STACK->StackTrace();
    }
    if (1)
    {
        startScope();
        auto char_var = CreateVar(DATATYPE::CHAR);
        AssignVar(char_var, 'b');
        cout << "[Main]: accessing char: " << accessVar(char_var) << endl;
        SYMBOL_TABLE->print_s_table();
        endScope();
        GLOBAL_STACK->StackTrace();
    }
    if (1)
    {
        startScope();
        auto bool_var = CreateVar(DATATYPE::BOOL);
        AssignVar(bool_var, 1);
        cout << "[Main]: accessing bool: " << accessVar(bool_var) << endl;
        SYMBOL_TABLE->print_s_table();
        endScope();
        GLOBAL_STACK->StackTrace();
    }
    if (1)
    {
        startScope();
        auto mint_var = CreateVar(DATATYPE::MEDIUM_INT);
        AssignVar(mint_var, 42);
        cout << "[Main]: accessing med_int: " << accessVar(mint_var) << endl;
        SYMBOL_TABLE->print_s_table();
        endScope();
        GLOBAL_STACK->StackTrace();
    }
    print_big_memory();
    auto int_arr_var = CreateArray(DATATYPE::INT, 4);
    Assign_array_in_range(int_arr_var, 0, 2, -50);
    Assign_array_in_range(int_arr_var, 2, 4, -76);
    cout << "[Main]: accessing int array at 0: " << (int)accessVar(int_arr_var, 0) << endl;
    cout << "[Main]: accessing int array at 1: " << (int)accessVar(int_arr_var, 1) << endl;
    cout << "[Main]: accessing int array at 2: " << (int)accessVar(int_arr_var, 2) << endl;

    auto char_arr_var = CreateArray(DATATYPE::CHAR, 5);
    Assign_array_in_range(char_arr_var, 0, 2, 'a');
    Assign_array_in_range(char_arr_var, 2, 5, 'A');
    cout << "[Main]: accessing char array at 0: " << (char)accessVar(char_arr_var, 0) << endl;
    cout << "[Main]: accessing char array at 1: " << (char)accessVar(char_arr_var, 1) << endl;
    cout << "[Main]: accessing char array at 2: " << (char)accessVar(char_arr_var, 2) << endl;
    cout << "[Main]: accessing char array at 4: " << (char)accessVar(char_arr_var, 4) << endl;

    auto bool_arr_var = CreateArray(DATATYPE::BOOL, 33);
    Assign_array_in_range(bool_arr_var, 0, 10, 1);
    Assign_array_in_range(bool_arr_var, 17, 33, 1);
    cout << "[Main]: accessing bool array at 0: " << (bool)accessVar(bool_arr_var, 0) << endl;
    cout << "[Main]: accessing bool array at 1: " << (bool)accessVar(bool_arr_var, 1) << endl;
    cout << "[Main]: accessing bool array at 2: " << (bool)accessVar(bool_arr_var, 2) << endl;
    cout << "[Main]: accessing bool array at 32: " << (bool)accessVar(bool_arr_var, 32) << endl;

    auto mint_arr_var = CreateArray(DATATYPE::MEDIUM_INT, 5);
    Assign_array_in_range(mint_arr_var, 0, 2, -1 * 'a');
    Assign_array_in_range(mint_arr_var, 2, 5, 'A');
    cout << "[Main]: accessing mint array at 0: " << (int)accessVar(mint_arr_var, 0) << endl;
    cout << "[Main]: accessing mint array at 1: " << (int)accessVar(mint_arr_var, 1) << endl;
    cout << "[Main]: accessing mint array at 2: " << (int)accessVar(mint_arr_var, 2) << endl;
    cout << "[Main]: accessing mint array at 4: " << (int)accessVar(mint_arr_var, 4) << endl;
    GLOBAL_STACK->StackTrace();
    // printf("line 655\n");
    print_big_memory();
    // freeElem(char_arr_var);
    endScope();
    SYMBOL_TABLE->print_s_table();
    GLOBAL_STACK->StackTrace();
    // print_big_memory();
    usleep(200000);
    cout << "helllo\n";
    print_big_memory();
    SYMBOL_TABLE->print_s_table();
    GLOBAL_STACK->StackTrace();
    freeMem();
    return 0;
}