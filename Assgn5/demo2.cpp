#include <iostream>
#include "memlab.h"


void fibonacci(s_table_entry *k, s_table_entry *dp)
{
    AssignArray(dp, 0, 1);
    AssignArray(dp, 1, 1);
    s_table_entry *i = CreateVar(DATATYPE::INT);
    AssignVar(i, 2);
    while (accessVar(i) <= accessVar(k))
    {
        AssignArray(dp, accessVar(i), accessVar(dp, accessVar(i) - 1) + accessVar(dp, accessVar(i) - 2));
        AssignVar(i, accessVar(i) + 1);
    }
}

s_table_entry *fibnacciProduct(s_table_entry *k)
{
    s_table_entry *dp = CreateArray(DATATYPE::INT, accessVar(k) + 1);
    startScope();
    fibonacci(k, dp);
    endScope();
    s_table_entry *product = CreateVar(DATATYPE::INT);
    AssignVar(product, 1);
    s_table_entry *j = CreateVar(DATATYPE::INT);
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
    startScope();
    s_table_entry *product = fibnacciProduct(k);
    AssignVar(retval, accessVar(product));
    cout << "Product of first " << accessVar(k) << " fibonacci nos. = " << accessVar(retval) << "\n";
    usleep(200000);
    freeMem();
    return 0;
}