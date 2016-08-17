#include "mempool.h"
#include "parser_common.h"

MemPool *tk_pool;

void *TokenInfo::operator new(size_t sz)
{
    return tk_pool->memAlloc(sz);
}

void TokenInfo::operator delete(void *ptrb)
{
    tk_pool->memFree(ptrb);
}