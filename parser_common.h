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
        token_pool.freeAll();
        parser_pool.freeAll();
        input_list.clear();
    }
    
    void init() {
        input_list.clear();
        token_pool.freeAll();
        parser_pool.freeAll();
        error = 0;
    }
    
    list<T *> input_list;
    MemPool token_pool;
    MemPool parser_pool;
    int error;
};


#endif /* PARSER_COMMON_H */

