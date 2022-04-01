#include<iostream>
#include "memlab.h"
// this function was used for testing the code demo3.c
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
    freeElem(char_arr_var);
    // endScope();
    SYMBOL_TABLE->print_s_table();
    GLOBAL_STACK->StackTrace();
    // print_big_memory();
    // usleep(20000);
    // cout << "helllo\n";
    print_big_memory();
    SYMBOL_TABLE->print_s_table();
    GLOBAL_STACK->StackTrace();
    freeMem();
    return 0;
}