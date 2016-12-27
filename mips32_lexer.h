#ifndef MIPS32LEXER_H
#define MIPS32LEXER_H

#include <inttypes.h>
#include <cstring>
#include <iostream>
#include <string>
#include <fstream>
#include "mips32_parser.h"
#include "parser_common.h"
#include "util.h"

using namespace std;

#define MTK_ERROR 9999
#define is_opcode(t)        ((t)>=0 && (t)<=4095)

class MInstruction;
typedef ParserContext<MInstruction> MParserContext;

class Mips32Lexer
{
public:
    Mips32Lexer(istream *in);
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

int mips32_getRegisterIndex(const char *rname);
string mips32_getRegisterName(unsigned int regIndex);

#endif // MIP32LEXER_H
