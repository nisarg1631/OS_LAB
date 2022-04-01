#include <iostream>
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
void fibonacci(s_table_entry *k, s_table_entry *dp)
{
    AssignArray(dp, 0, 1);
    AssignArray(dp, 1, 1);
    s_table_entry *i = CreateVar(DATATYPE::INT);
    // calc_mem_footprint();;
    AssignVar(i, 2);
    while (accessVar(i) <= accessVar(k))
    {
        AssignArray(dp, accessVar(i), accessVar(dp, accessVar(i) - 1) + accessVar(dp, accessVar(i) - 2));
        AssignVar(i, accessVar(i) + 1);
    }
}

s_table_entry *fibnacciProduct(s_table_entry *k)
{
    // calc_mem_footprint();;
    s_table_entry *dp = CreateArray(DATATYPE::INT, accessVar(k) + 1);
    startScope();
    fibonacci(k, dp);
    endScope();
    // usleep(200000);
    // calc_mem_footprint();;
    s_table_entry *product = CreateVar(DATATYPE::INT);
    AssignVar(product, 1);
    s_table_entry *j = CreateVar(DATATYPE::INT);
    // calc_mem_footprint();
    AssignVar(j, 1);
    while (accessVar(j) <= accessVar(k))
    {
        AssignVar(product, accessVar(dp, accessVar(j)) * accessVar(product));
        AssignVar(j, accessVar(j) + 1);
    }

    return product;
}

int main(int argc, char **argv)
{
    CreateMemory(20000);
    startScope();
    s_table_entry *k = CreateVar(DATATYPE::INT);
    // printPageTable();
    AssignVar(k, atoi(argv[1]));
    s_table_entry *retval = CreateVar(DATATYPE::INT);
    // calc_mem_footprint();
    startScope();
    s_table_entry *product = fibnacciProduct(k);
    AssignVar(retval, accessVar(product));
    // endScope();
    // usleep(20000);
    // calc_mem_footprint();;
    cout << "Product of first " << accessVar(k) << " fibonacci nos. = " << accessVar(retval) << "\n";
    // usleep(200000);
    freeMem();
    return 0;
}