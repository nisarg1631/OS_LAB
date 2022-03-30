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
