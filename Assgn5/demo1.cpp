#include "memlab.h"
void calc_mem_footprint()
{
    pthread_mutex_lock(&memory_mutex);
    int *ptr = BIG_MEMORY;
    int size = 0;
    while (ptr - BIG_MEMORY < big_memory_sz)
    {
        if (!(*ptr & 1)) // unallocated
        {
            size += (*ptr >> 1);
        }
        ptr = ptr + (*ptr>>1);
    }
    pthread_mutex_unlock(&memory_mutex);
    cout << "[calc_mem_footprint]: Unallocated Memory Right Now: " << size << "\n";
}
void runn(s_table_entry *ptr1, s_table_entry *ptr2)
{
    startScope();
    s_table_entry *i = CreateVar(DATATYPE::INT);
    if (!i)
        printf("i is null\n");
    AssignVar(i, 0);
    s_table_entry *arr;
    if (ptr1->unit_size == 32)
        arr = CreateArray(DATATYPE::INT, 50000);
    else if (ptr1->unit_size == 24)
        arr = CreateArray(DATATYPE::MEDIUM_INT, 50000);
    else if (ptr1->unit_size == 8)
        arr = CreateArray(DATATYPE::CHAR, 50000);
    else if (ptr1->unit_size == 1)
        arr = CreateArray(DATATYPE::BOOL, 50000);
    else
        exit(1);
    // calc_mem_footprint();
    while ((int)accessVar(i) < 50000)
    {
        AssignArray(arr, (int)accessVar(i), rand() % 2);
        AssignVar(i, (int)accessVar(i) + 1);
    }
    endScope();
    // usleep(20000);
    // calc_mem_footprint();
}

int main()
{

    CreateMemory(3e8);
    startScope();
    s_table_entry *var1 = CreateVar(DATATYPE::MEDIUM_INT);
    s_table_entry *var2 = CreateVar(DATATYPE::MEDIUM_INT);
    runn(var1, var2);

    var1 = CreateVar(DATATYPE::CHAR);
    var2 = CreateVar(DATATYPE::CHAR);
    runn(var1, var2);

    var1 = CreateVar(DATATYPE::BOOL);
    var2 = CreateVar(DATATYPE::BOOL);
    runn(var1, var2);

    var1 = CreateVar(DATATYPE::INT);
    var2 = CreateVar(DATATYPE::INT);
    runn(var1, var2);

    var1 = CreateVar(DATATYPE::MEDIUM_INT);
    var2 = CreateVar(DATATYPE::MEDIUM_INT);
    runn(var1, var2);

    var1 = CreateVar(DATATYPE::CHAR);
    var2 = CreateVar(DATATYPE::CHAR);
    runn(var1, var2);

    var1 = CreateVar(DATATYPE::BOOL);
    var2 = CreateVar(DATATYPE::BOOL);
    runn(var1, var2);

    var1 = CreateVar(DATATYPE::INT);
    var2 = CreateVar(DATATYPE::INT);
    runn(var1, var2);

    var1 = CreateVar(DATATYPE::MEDIUM_INT);
    var2 = CreateVar(DATATYPE::MEDIUM_INT);
    runn(var1, var2);

    var1 = CreateVar(DATATYPE::CHAR);
    var2 = CreateVar(DATATYPE::CHAR);
    runn(var1, var2);
    // usleep(200000);
    // print_big_memory();
    SYMBOL_TABLE->print_s_table();
    GLOBAL_STACK->StackTrace();
    freeMem();
    return 0;
}