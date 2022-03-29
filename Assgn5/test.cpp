struct Symbol
{
    // word1 -> 31 bits for wordIdx, 1 bit for if symbol is allocated in symboltable memory
    // word2 -> 31 bits for offset, 1 bit for if symbol is in use (mark for garbage collection)
    unsigned int word1, word2;
};

struct SymbolTable
{
    unsigned int head, tail;
    Symbol symbols[MAX_SYMBOLS];
    int size;
    pthread_mutex_t mutex;
    SymbolTable();
    ~SymbolTable();
    int alloc(unsigned int wordidx, unsigned int offset);
    void free(unsigned int idx);
    inline int getWordIdx(unsigned int idx) { return symbols[idx].word1 >> 1; }
    inline int getOffset(unsigned int idx) { return symbols[idx].word2 >> 1; }
    inline void setMarked(unsigned int idx) { symbols[idx].word2 |= 1; }    // mark as in use
    inline void setUnmarked(unsigned int idx) { symbols[idx].word2 &= -2; } // mark as free
    inline void setAllocated(unsigned int idx) { symbols[idx].word1 |= 1; } // mark as allocated
    inline void setUnallocated(unsigned int idx) { symbols[idx].word1 &= -2; }
    inline bool isMarked(unsigned int idx) { return symbols[idx].word2 & 1; }
    inline bool isAllocated(unsigned int idx) { return symbols[idx].word1 & 1; }
    int *getPtr(unsigned int idx);
};

int SymbolTable::alloc(unsigned int wordidx, unsigned int offset)
{
    if (size == MAX_SYMBOLS)
    {
        return -1;
    }
    unsigned int idx = head;
    head = (symbols[head].word2 & -2) >> 1;
    symbols[idx].word1 = (wordidx << 1) | 1; // mark as allocated
    symbols[idx].word2 = (offset << 1) | 1;  // mark as in use
    size++;
    LOG("SymbolTable", _COLOR_BLUE, "Alloc symbol: %d at address: %d\n", idx, (wordidx << 2) | offset);
    return idx; // local address
}

void SymbolTable::free(unsigned int idx)
{
    unsigned int wordidx = getWordIdx(idx);
    unsigned int offset = getOffset(idx);
    if (size == MAX_SYMBOLS)
    {
        head = tail = idx;
        symbols[idx].word1 = 0;
        symbols[idx].word2 = -2; // sentinel
        size--;
        return;
    }
    symbols[tail].word2 = idx << 1;
    symbols[idx].word1 = 0;
    symbols[idx].word2 = -2; // sentinel
    tail = idx;
    size--;
    LOG("SymbolTable", _COLOR_BLUE, "Freed symbol: %d at address: %d\n", idx, (wordidx << 2) | offset);
}

void gc_run()
{
    for (int i = 0; i < MAX_SYMBOLS; i++)
    {
        PTHREAD_MUTEX_LOCK(&mem->mutex);
        PTHREAD_MUTEX_LOCK(&symTable->mutex);
        if (symTable->isAllocated(i) && !symTable->isMarked(i))
        {
            _freeElem(i);
        }
        PTHREAD_MUTEX_UNLOCK(&symTable->mutex);
        PTHREAD_MUTEX_UNLOCK(&mem->mutex);
    }
}

void handlSigUSR1(int sig)
{
    gc_run();
}

void *garbageCollector(void *)
{
    signal(SIGUSR1, handlSigUSR1);
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    while (true)
    {
        usleep(GC_PERIOD_MS * 1000);
        pthread_sigmask(SIG_BLOCK, &set, NULL);
        gc_run();
        pthread_sigmask(SIG_UNBLOCK, &set, NULL);
    }
}

void calcOffset()
{
    int *p = mem->start;
    int offset = 0;
    while (p < mem->end)
    {
        if ((*p & 1) == 0)
        {
            offset += (*p >> 1);
        }
        else
        {
            *(p + (*p >> 1) - 1) = (((p - offset) - mem->start) << 1) | 1;
        }
        p = p + (*p >> 1);
    }
}

void updateSymbolTable()
{
    for (int i = 0; i < MAX_SYMBOLS; i++)
    {
        if (symTable->isAllocated(i))
        {
            int *p = symTable->getPtr(i) - 1;
            int newWordId = *(p + (*p >> 1) - 1) >> 1;
            symTable->symbols[i].word1 = (newWordId << 1) | 1;
        }
    }
}

void compactMem()
{
    calcOffset();
    updateSymbolTable();
    int *p = mem->start;
    int *next = p + (*p >> 1);
    while (next != mem->end)
    {
        if ((*p & 1) == 0 && (*next & 1) == 1)
        {
            int word1 = *p >> 1;
            int word2 = *next >> 1;
            memcpy(p, next, word2 << 2);
            p = p + word2;
            *p = word1 << 1;
            *(p + word1 - 1) = word1 << 1;
            next = p + word1;
            if (next != mem->end && (*next & 1) == 0)
            {
                word1 = word1 + (*next >> 1);
                *p = word1 << 1;
                *(p + word1 - 1) = word1 << 1;
                next = p + word1;
            }
        }
        else
        {
            p = next;
            next = p + (*p >> 1);
        }
    }
    p = mem->start;
    while (p < mem->end)
    {
        *(p + (*p >> 1) - 1) = *p;
        p = p + (*p >> 1);
    }
}

int MemBlock::getMem(int size)
{ // size in bytes (4 bytes aligned)
    int *p = start;
    int newsize = (((size + 3) >> 2) << 2) + 8;
    while ((p < end) &&
           ((*p & 1) ||
            ((*p << 1) < newsize)))
        p = p + (*p >> 1);
    if (p == end)
    {
        return -1;
    }
    addBlock((int *)p, newsize);
    return (p - start);
}

void MemBlock::addBlock(int *ptr, int size)
{
    int oldsize = *ptr << 1; // old size in bytes
    int words = size >> 2;
    *ptr = (words << 1) | 1;
    *(ptr + words - 1) = (words << 1) | 1; // footer
    if (size < oldsize)
    {
        *(ptr + words) = (oldsize - size) >> 1;
        *(ptr + (oldsize >> 2) - 1) = (oldsize - size) >> 1;
    }
}