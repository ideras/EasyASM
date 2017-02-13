#include <cstring>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include "mips32_lexer.h"
#include "mips32_parser.h"
#include "mempool.h"
#include "mips32_sim.h"

using namespace std;

#define RETURN_TOKEN(tk)    \
    do {                    \
        tokenInfo.set(tkText, currentLine); \
        return tk; \
    } while (0)
	
#define APPEND_SEQUENCE(cond, sb) \
            do { \
                while ( (cond) ) {  \
                    sb += (char)ch; \
                    ch = nextChar();\
                }                   \
                ungetChar(ch); \
            } while (0)

#if defined(_MSC_VER)
#define strcasecmp _stricmp
#endif

struct MKeyword {
    const char *name;
    int tokenKind;
};

static MKeyword m_keywords[] = {
	{"hexadecimal", MCKW_HEX },
	{"hex", MCKW_HEX },
	{"decimal", MCKW_DEC },
	{"signed", MCKW_SIGNED },
	{"unsigned", MCKW_UNSIGNED },
	{"binary", MCKW_BINARY },
        {"byte", MKW_BYTE},
        {"ascii", MCKW_ASCII},
        {"hword", MKW_HWORD},
        {"word", MKW_WORD},
        {"memory", MCKW_MEM},
};
const int KWCount = sizeof(m_keywords)/sizeof(MKeyword);

static MKeyword m_commands[] = {
	{"#show", MCKW_SHOW },
	{"#set", MCKW_SET },
	{"#exec", MCKW_EXEC },
        {"#stop", MCKW_STOP },
        {"#paddr", MCKW_PADDR},
        {"#hihw", MCKW_HIHW},
        {"#lohw", MCKW_LOHW},
};

const char *reg_names[] = { "$zero", "$at", "$v0", "$v1",
                            "$a0", "$a1", "$a2", "$a3",
                            "$t0", "$t1", "$t2", "$t3",
                            "$t4", "$t5", "$t6", "$t7",
                            "$s0", "$s1", "$s2", "$s3",
                            "$s4", "$s5", "$s6", "$s7",
                            "$t8", "$t9", "$k0", "$k1",
                            "$gp", "$sp", "$fp", "$ra"
                          };

const char *reg_names1[] = { "$r0", "$r1", "$r2", "$r3",
                            "$r4", "$r5", "$r6", "$r7",
                            "$r8", "$r9", "$r10", "$r11",
                            "$r12", "$r13", "$r14", "$r15",
                            "$r16", "$r17", "$r18", "$r19",
                            "$r20", "$r21", "$r22", "$r23",
                            "$r24", "$r25", "$r26", "$r27",
                            "$r28", "$r29", "$r30", "$r31"
                          };

const int KWCmdCount = sizeof(m_commands)/sizeof(MKeyword);

static int lookUpWord(MKeyword kw[], int size, string str)
{
    for (int i=0; i<size; i++) {
        if (strcasecmp(str.c_str(), kw[i].name) == 0)
            return kw[i].tokenKind;
    }

    return MTK_ID;
}

static int _getRegisterIndex(const char *name, const char *reg_names[], int size)
{
    for (int i = 0; i<size; i++) {
        if (strcmp(name, reg_names[i]) == 0) {
            return i;
        }
    }

    return -1;
}

int mips32_getRegisterIndex(const char *rname) 
{
    int count = sizeof(reg_names) / sizeof(reg_names[0]);
	int index;
    
	index = _getRegisterIndex(rname, reg_names, count);
	if (index != -1)
		return index;

	return _getRegisterIndex(rname, reg_names1, count);
}

string mips32_getRegisterName(unsigned int regIndex)
{
    return ((regIndex<=31)? reg_names[regIndex] : "");
}

Mips32Lexer::Mips32Lexer(istream *in)
{
    this->in = in;
    currentLine = 1;
}

int Mips32Lexer::getNextToken()
{
    int ch;
    string tkText;

    tkText.reserve(MAX_TOKEN_LENGTH);
    tkText = "";

    while (1) {
        ch = nextChar();

        if (ch == ' ' || ch == '\t')
            continue;

        tkText += (char)ch;

        switch (ch) {
            case EOF: RETURN_TOKEN(MTK_EOF);
            case '(': RETURN_TOKEN(MTK_LPAREN);
            case ')': RETURN_TOKEN(MTK_RPAREN);
            case ',': RETURN_TOKEN(MTK_COMMA);
            case '-': RETURN_TOKEN(MTK_OP_MINUS);
            case '=': RETURN_TOKEN(MTK_OPEQUAL);
            case ':': RETURN_TOKEN(MTK_COLON);
            case '[': RETURN_TOKEN(MTK_LBRACKET);
            case ']': RETURN_TOKEN(MTK_RBRACKET);
            case '@': RETURN_TOKEN(MTK_AT);
            case '.': RETURN_TOKEN(MTK_DOT);
            case ';': {
                ch = nextChar();
                while (ch != '\n' && ch != EOF) {
                    ch = nextChar();
                }
                currentLine ++;
                RETURN_TOKEN(MTK_EOL);
            }
            case '\r': {
                ch = nextChar();

                if (ch != '\n')
                    ungetChar(ch);

                currentLine ++;
                RETURN_TOKEN(MTK_EOL);
            }
            case '\n': {
                currentLine ++;
                RETURN_TOKEN(MTK_EOL);
            }
            case '$': {
                tkText = "$";
                
                ch = nextChar();
                APPEND_SEQUENCE(isalnum(ch), tkText);
                tokenInfo.intValue = mips32_getRegisterIndex(tkText.c_str());
                RETURN_TOKEN(MTK_REGISTER);
            }
            case '"': {
                tkText.clear();
                ch = nextChar();
                APPEND_SEQUENCE((ch != '"') && (ch != EOF), tkText);
                ch = nextChar();
                
                if (ch == EOF) {
                    tokenInfo.set(string("unterminated string \"") + tkText + string("\""), currentLine);
                    return MTK_ERROR;
                }
                tokenInfo.set(tkText, currentLine);
                
                return MSTR_LITERAL;
            }
            case '\'': {
                tkText.clear();
                ch = nextChar();
                APPEND_SEQUENCE((ch != '\'') && (ch != EOF), tkText);
                ch = nextChar();
                
                if (ch == EOF) {
                    tokenInfo.set(string("unterminated character constant '") + tkText + string("'"), currentLine);
                    return MTK_ERROR;
                }
                if (tkText.length() != 1) {
                    tokenInfo.set("character constant '" + tkText + "'", currentLine);
                    return MTK_ERROR;
                }

                tokenInfo.set(tkText, currentLine);
                tokenInfo.intValue = (int)tkText[0];
                
                return MTK_CHAR_CONSTANT;
            }
            case '#': {
                tkText.clear();
                
                tkText += (char)ch;
                ch = nextChar();
                APPEND_SEQUENCE(isalpha(ch), tkText);
                
                tokenInfo.set(tkText, currentLine);
                
                int tokenKW = lookUpWord(m_commands, KWCmdCount, tkText);
                
                if (tokenKW != MTK_ID) 
                    return tokenKW;
                else {
                    tokenInfo.set("command name '" + tkText + "'", currentLine);
                    return MTK_ERROR;
                }
            }

            default: {
                if (isdigit(ch)) {
                    char prevCh = ch;
                    ch = nextChar();

                    if (prevCh == '0' && (ch == 'b' || ch == 'B')) {
                        tkText.clear();

                        ch = nextChar();
                        APPEND_SEQUENCE((ch=='0') || (ch=='1'), tkText);
						
                        if (tkText.empty()) {
                            tokenInfo.set("binary constant '0b'", currentLine);
                            return MTK_ERROR;
                        }

                        tokenInfo.set(tkText, currentLine);
                        tokenInfo.intValue = strtoul(tkText.c_str(), NULL, 2);

                        return MTK_BIN_CONSTANT;

                    } else if (prevCh == '0' && (ch == 'x' || ch == 'X')) {
                        tkText.clear();

                        ch = nextChar();
                        APPEND_SEQUENCE(isxdigit(ch), tkText);

                        if (tkText.empty()) {
                            tokenInfo.set("hexadecimal constant '0x'", currentLine);
                            return MTK_ERROR;
                        }
                        tokenInfo.set(tkText, currentLine);
                        tokenInfo.intValue = strtoul(tkText.c_str(), NULL, 16);

                        return MTK_HEX_CONSTANT;
                    } else if (isdigit(ch)) {
						
                        APPEND_SEQUENCE(isdigit(ch), tkText);

                        tokenInfo.set(tkText, currentLine);
                        tokenInfo.intValue = atol(tkText.c_str());

                        return MTK_DEC_CONSTANT;
                    } else {
                        ungetChar(ch);

                        tokenInfo.tokenLexeme += prevCh;
                        tokenInfo.intValue = prevCh - '0';

                        return MTK_DEC_CONSTANT;
                    }
                } else if (isalpha(ch) || ch == '_') {
                    tkText.clear();
                    APPEND_SEQUENCE( (isalnum(ch) || ch == '_') && (ch != EOF), tkText );

                    tokenInfo.set(tkText, currentLine);
                    return lookUpWord(m_keywords, KWCount, tkText);

                } else {
                    tokenInfo.set(string("symbol '") + ((char)ch) + string("'"), currentLine);
                    return MTK_ERROR;
                }
            }
        }
    }
}

string Mips32Lexer::getTokenString(int token, TokenInfo *info)
{
    string tokenName = "";

    switch (token) {
    case MTK_EOF: tokenName = "end of input"; break;
    case MTK_EOL: tokenName = "end of line"; break;
    case MTK_REGISTER: tokenName = "register"; break;
    case MTK_ERROR: tokenName = ""; break;

    case MTK_OPEQUAL: tokenName = "operator"; break;
    case MTK_COMMA:
    case MTK_LPAREN:
    case MTK_RPAREN:
    case MTK_COLON:
    case MTK_LBRACKET:
    case MTK_RBRACKET:        
        tokenName = "symbol";
        break;
    case MSTR_LITERAL:  tokenName = "string literal"; break;
    case MTK_ID: tokenName = "identifier"; break;
    case MTK_DEC_CONSTANT: tokenName = "decimal constant"; break;
    case MTK_HEX_CONSTANT: tokenName = "hexadecimal constant"; break;
    case MTK_BIN_CONSTANT: tokenName = "binary constant"; break;
    default:
        tokenName = ::convertToString(token);
    }

    string result = tokenName;

    if (info != NULL) {
        switch (token) {
            case MTK_ERROR:
                result += info->tokenLexeme;
                break;
            case MTK_EOL:
            case MTK_EOF:
                break;
            default:
                result += " '" + info->tokenLexeme + "'";
        }
    }

    return result;
}
