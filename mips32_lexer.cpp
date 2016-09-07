#include <cstring>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include "mips32_lexer.h"
#include "mips32_parser.h"
#include "mempool.h"

using namespace std;

#define MTK_ERROR 9999

void reportError(const char *format, ...);

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
            case '(': RETURN_TOKEN(MTK_OPENP);
            case ')': RETURN_TOKEN(MTK_CLOSEP);
            case ',': RETURN_TOKEN(MTK_COMMA);
            case '-': RETURN_TOKEN(MTK_OP_MINUS);
            case '=': RETURN_TOKEN(MTK_OPEQUAL);
            case ':': RETURN_TOKEN(MTK_COLON);
            case '[': RETURN_TOKEN(MTK_LBRACKET);
            case ']': RETURN_TOKEN(MTK_RBRACKET);
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
                RETURN_TOKEN(MTK_REGISTER);
            }
            case '"': {
                tkText.clear();
                ch = nextChar();
                APPEND_SEQUENCE(ch != '"', tkText);
                ch = nextChar();
                tokenInfo.set(tkText, currentLine);
                
                return MSTR_LITERAL;
            }
            case '\'': {
                tkText.clear();
                ch = nextChar();
                APPEND_SEQUENCE(ch != '\'', tkText);
                ch = nextChar();
                
                if (tkText.length() != 1) {
                    reportError("Invalid character constant '%s'\n", tkText.c_str());
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
                
                int tokenId = lookUpWord(m_commands, KWCmdCount, tkText);
                
                if (tokenId != -1) 
                    return tokenId;
                else {
                    reportError("Invalid command name '%s'\n", tkText.c_str());
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
                            reportError("Invalid binary constant detected at line %d\n", currentLine);

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
                            reportError("Invalid hexadecimal constant detected at line %d\n", currentLine);

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
                    reportError("Invalid symbol '%X' detected at line %d\n", ch, currentLine);
                    return MTK_ERROR;
                }
            }
        }
    }
}

string Mips32Lexer::getTokenString(int token, TokenInfo *info)
{
    string tokenName;

    switch (token) {
    case MTK_EOF: tokenName = "end of input"; break;
    case MTK_EOL: tokenName = "end of line"; break;
    case MTK_REGISTER: tokenName = "register"; break;

    case MTK_OPEQUAL: tokenName = "operator"; break;
    case MTK_COMMA:
    case MTK_OPENP:
    case MTK_CLOSEP:
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

    if ( info != NULL && (token != MTK_EOL) && (token != MTK_EOF) )
        result += " '" + info->tokenLexeme + "'";

    return result;
}
