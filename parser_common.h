/* 
 * File:   lexer_common.h
 * Author: ideras
 *
 * Created on August 11, 2016, 3:42 PM
 */

#ifndef PARSER_COMMON_H
#define PARSER_COMMON_H

#include <stdint.h>
#include <string>
#include <list>
#include "mempool.h"

using namespace std;

class MemPool;

extern MemPool *tk_pool;

#define MAX_TOKEN_LENGTH    256

struct TokenInfo {
    string tokenLexeme;
    uint32_t intValue;
    int line;

    static void *operator new(std::size_t sz);
    static void operator delete(void* ptrb);

    TokenInfo() {
	tokenLexeme = "";
	intValue = 0;
        line = 0;
    }

    void set(string str, int line) { 
        this->tokenLexeme = str; 
        this->line = line; 
    }
};

template <typename T>
struct ParserContext {
    
    ParserContext() { 
        init();
    }
    
    ~ParserContext() {
        tokenPool.freeAll();
        parserPool.freeAll();
        instList.clear();
    }
    
    void init() {
        instList.clear();
        tokenPool.freeAll();
        parserPool.freeAll();
        error = 0;
    }
    
    list<T *> instList;
    MemPool tokenPool;
    MemPool parserPool;
    int error;
};


#endif /* PARSER_COMMON_H */

