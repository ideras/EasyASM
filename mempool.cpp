#include <cstdlib>
#include <iostream>
#include "mempool.h"

using namespace std;

MemPool::MemPool()
{
    nodes.clear();
}

void *MemPool::memAlloc(size_t size)
{
    void *result = ::operator new (size);
    
    if (result != NULL) {
        nodes.insert(result);
    }

    return result;
}

void MemPool::memFree(void *ptrb)
{
    if (nodes.find(ptrb) != nodes.end()) {
        nodes.erase(ptrb);
        ::operator delete (ptrb);
    }
}

void MemPool::freeAll()
{
    set<void *>::iterator it = nodes.begin();

    while (it != nodes.end()) {
        void *ptr = *it;

        //cout << hex << "Freeing " << ptr << dec << endl;
        ::operator delete (ptr);
        it++;
    }

    nodes.clear();
}
