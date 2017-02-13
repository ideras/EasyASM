#ifndef X86LEXER_H
#define X86LEXER_H

#include <inttypes.h>
#include <cstring>
#include <iostream>
#include <string>
#include <list>
#include "x86_parser.h"
#include "parser_common.h"
#include "mempool.h"
#include "util.h"

#define XTK_ERROR 9999

using namespace std;

class XInstruction;
typedef ParserContext<XInstruction> XParserContext;

class X86Lexer
{
public:
    X86Lexer(istream *in);
    int getNextToken();

    int getCurrentLine() { return currentLine; }
    
    void getTokenInfo(TokenInfo *ti) {
        ti->intValue = tokenInfo.intValue;
        ti->tokenLexeme = tokenInfo.tokenLexeme;
        ti->line = tokenInfo.line;
    }

    static string getTokenString(int token, TokenInfo *info);

private:
	int nextChar() { return in->get(); }
	void ungetChar(int c) { in->putback(c); }
	
private:
    istream *in;
    int currentLine;
    TokenInfo tokenInfo;
};

#endif // X86LEXER_H
