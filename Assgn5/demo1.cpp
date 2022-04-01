#include "memlab.h"
void runn(s_table_entry *ptr1, s_table_entry *ptr2)
{
    startScope();
    s_table_entry *i = CreateVar(DATATYPE::INT);
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
    while ((int)accessVar(i) < 50000)
    {
        AssignArray(arr, (int)accessVar(i), rand() % 2);
        AssignVar(i, (int)accessVar(i) + 1);
    }
    endScope();
}

int main()
{

    CreateMemory(250e6);
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
    // usleep(2000000);
    // print_big_memory();
    SYMBOL_TABLE->print_s_table();
    GLOBAL_STACK->StackTrace();
    freeMem();
    return 0;
}