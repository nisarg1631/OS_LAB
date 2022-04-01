#include "memlab.h"
int glob = 0;
void func1(s_table_entry *ptr1, s_table_entry *ptr2)
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
    // print_big_memory();
    endScope();
}

int main()
{

    CreateMemory(250e6);
    // Medium Int
    s_table_entry *p1 = CreateVar(DATATYPE::MEDIUM_INT);
    s_table_entry *p2 = CreateVar(DATATYPE::MEDIUM_INT);
    func1(p1, p2);

    // Char
    p1 = CreateVar(DATATYPE::CHAR);
    p2 = CreateVar(DATATYPE::CHAR);
    func1(p1, p2);

    // Boolean
    p1 = CreateVar(DATATYPE::BOOL);
    p2 = CreateVar(DATATYPE::BOOL);
    func1(p1, p2);

    // Int
    p1 = CreateVar(DATATYPE::INT);
    p2 = CreateVar(DATATYPE::INT);
    func1(p1, p2);

    // Medium Int
    p1 = CreateVar(DATATYPE::MEDIUM_INT);
    p2 = CreateVar(DATATYPE::MEDIUM_INT);
    func1(p1, p2);

    // Char
    p1 = CreateVar(DATATYPE::CHAR);
    p2 = CreateVar(DATATYPE::CHAR);
    func1(p1, p2);

    // Boolean
    p1 = CreateVar(DATATYPE::BOOL);
    p2 = CreateVar(DATATYPE::BOOL);
    func1(p1, p2);

    // Int
    p1 = CreateVar(DATATYPE::INT);
    p2 = CreateVar(DATATYPE::INT);
    func1(p1, p2);

    // Medium Int
    p1 = CreateVar(DATATYPE::MEDIUM_INT);
    p2 = CreateVar(DATATYPE::MEDIUM_INT);
    func1(p1, p2);

    // Char
    p1 = CreateVar(DATATYPE::CHAR);
    p2 = CreateVar(DATATYPE::CHAR);
    func1(p1, p2);
    // usleep(2000000);
    // print_big_memory();
    // SYMBOL_TABLE->print_s_table();
    freeMem();
    return 0;
}