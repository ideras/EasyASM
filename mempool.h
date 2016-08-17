#ifndef XALLOCPOOL_H
#define XALLOCPOOL_H

#include <set>

using namespace  std;

class MemPool
{
public:
    MemPool();

    void *memAlloc(std::size_t size);
    void memFree(void *ptrb);
    void freeAll();
private:
    set<void *> nodes;
};

#endif // XALLOCPOOL_H
