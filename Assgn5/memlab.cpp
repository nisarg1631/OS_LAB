#include "memlab.h"
s_table::s_table()
{
    this->head = NULL;
    this->tail = NULL;
}
void s_table::insert_at_tail(s_table_entry *new_entry)
{
    if (this->head == NULL)
    {
        this->head = new_entry;
        this->tail = new_entry;
    }
    else
    {
        this->tail->next = new_entry;
        this->tail = new_entry;
    }
    this->cur_size++;
}
void s_table::insert_after(s_table_entry *new_entry, s_table_entry *prev_entry)
{
    if (prev_entry == NULL)
        return insert_at_tail(new_entry);
    new_entry->next = prev_entry->next;
    prev_entry->next = new_entry;
    this->cur_size++;
}
void s_table::find_first_free_block(s_table_entry *new_data)
{
    s_table_entry *cur = this->head;
    s_table_entry *prev = NULL;
    while (cur != NULL)
    {
        int prev_used = 0;
        if (prev != NULL)
        {
            prev_used = (prev->idx_start_in_mem * 32 + prev->total_size_used);
        }
        if (cur->idx_start_in_mem * 32 - prev_used >= new_data->total_size_used)
        {
            new_data->idx_start_in_mem = prev_used / 32;
            this->insert_after(new_data, prev);
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}
stack::stack(int mx)
{
    this->arr = new stack_entry[mx];
    this->top = -1;
    this->max_size = mx;
}
void stack::push(stack_entry *new_entry)
{
    if (this->top == this->max_size - 1)
    {
        cout << "Stack out of memory" << endl;
        exit(1);
    }
    this->top++;
    this->arr[this->top] = *new_entry;
}
stack_entry stack::pop()
{
    if (this->top == -1)
    {
        cout << "Stack underflow" << endl;
        exit(1);
    }
    return this->arr[this->top--];
}

