#include <iostream>
#include "memlab.h"
// this function was used for testing the code demo3.c
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
int main()
{
    // only for test, remove later
    CreateMemory(250e6);
    pthread_mutex_lock(&print_mutex);

    printf("[Main]: Symbol table at the start\n");
    pthread_mutex_unlock(&print_mutex);

    SYMBOL_TABLE->print_s_table();
    startScope();
    GLOBAL_STACK->StackTrace();
    print_big_memory();
    if (1)
    {
        startScope();
        auto int_var = CreateVar(DATATYPE::INT);
        AssignVar(int_var, 3);
        pthread_mutex_lock(&print_mutex);

        cout << "[Main]: accessing int: " << accessVar(int_var) << endl;
        pthread_mutex_unlock(&print_mutex);

        SYMBOL_TABLE->print_s_table();
        // calc_mem_footprint();
        endScope();
        // usleep(20000);
        // calc_mem_footprint();
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
        // calc_mem_footprint();
        endScope();
        // usleep(20000);
        // calc_mem_footprint();
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
    auto int_arr_var = CreateArray(DATATYPE::INT, 40);
    Assign_array_in_range(int_arr_var, 0, 2, -50);
    Assign_array_in_range(int_arr_var, 2, 4, -76);
    cout << "[Main]: accessing int array at 0: " << (int)accessVar(int_arr_var, 0) << endl;
    cout << "[Main]: accessing int array at 1: " << (int)accessVar(int_arr_var, 1) << endl;
    cout << "[Main]: accessing int array at 2: " << (int)accessVar(int_arr_var, 2) << endl;
    // calc_mem_footprint();

    auto char_arr_var = CreateArray(DATATYPE::CHAR, 50);
    Assign_array_in_range(char_arr_var, 0, 2, 'a');
    Assign_array_in_range(char_arr_var, 2, 5, 'A');
    cout << "[Main]: accessing char array at 0: " << (char)accessVar(char_arr_var, 0) << endl;
    cout << "[Main]: accessing char array at 1: " << (char)accessVar(char_arr_var, 1) << endl;
    cout << "[Main]: accessing char array at 2: " << (char)accessVar(char_arr_var, 2) << endl;
    cout << "[Main]: accessing char array at 4: " << (char)accessVar(char_arr_var, 4) << endl;

    auto bool_arr_var = CreateArray(DATATYPE::BOOL, 3300);
    Assign_array_in_range(bool_arr_var, 0, 10, 1);
    Assign_array_in_range(bool_arr_var, 17, 33, 1);
    cout << "[Main]: accessing bool array at 0: " << (bool)accessVar(bool_arr_var, 0) << endl;
    cout << "[Main]: accessing bool array at 1: " << (bool)accessVar(bool_arr_var, 1) << endl;
    cout << "[Main]: accessing bool array at 2: " << (bool)accessVar(bool_arr_var, 2) << endl;
    cout << "[Main]: accessing bool array at 32: " << (bool)accessVar(bool_arr_var, 32) << endl;
    // calc_mem_footprint();

    auto mint_arr_var = CreateArray(DATATYPE::MEDIUM_INT, 50000);
    Assign_array_in_range(mint_arr_var, 0, 2, -1 * 'a');
    Assign_array_in_range(mint_arr_var, 2, 5, 'A');
    cout << "[Main]: accessing mint array at 0: " << (int)accessVar(mint_arr_var, 0) << endl;
    cout << "[Main]: accessing mint array at 1: " << (int)accessVar(mint_arr_var, 1) << endl;
    cout << "[Main]: accessing mint array at 2: " << (int)accessVar(mint_arr_var, 2) << endl;
    cout << "[Main]: accessing mint array at 4: " << (int)accessVar(mint_arr_var, 4) << endl;
    GLOBAL_STACK->StackTrace();
    // calc_mem_footprint();

    // printf("line 655\n");
    print_big_memory();
    freeElem(char_arr_var);
    // usleep(20000);
    // calc_mem_footprint();

    // endScope();
    SYMBOL_TABLE->print_s_table();
    GLOBAL_STACK->StackTrace();
    // print_big_memory();
    // usleep(20000);
    // cout << "helllo\n";
    print_big_memory();
    // calc_mem_footprint();
    SYMBOL_TABLE->print_s_table();
    GLOBAL_STACK->StackTrace();
    freeMem();
    return 0;
}