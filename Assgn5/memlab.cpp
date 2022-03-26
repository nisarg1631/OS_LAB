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
