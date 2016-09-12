#include <cstring>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include "x86_lexer.h"

using namespace std;

#define XTK_ERROR 9999

void reportError(const char *format, ...);

#define RETURN_TOKEN(tk)    \
    do {                    \
        tokenInfo.set(tkText, currentLine); \
        return tk; \
    } while (0)
	
#define APPEND_SEQUENCE(cond, sb) \
            do { \
                while ( (cond) ) { \
                    sb += (char)ch;\
                    ch = nextChar();\
			    } \
                ungetChar(ch); \
			} while (0)	

struct XKeyword {
    const char *name;
    int tokenKind;
};

static XKeyword kw[] = {
    {"hexadecimal", XCKW_HEX },
    {"hex", XCKW_HEX },
    {"decimal", XCKW_DEC },
    {"signed", XCKW_SIGNED },
    {"unsigned", XCKW_UNSIGNED },
    {"binary", XCKW_BIN },
    {"octal", XCKW_OCT },
    {"ascii", XCKW_ASCII},
    {"mov", XKW_MOV},
    {"movzx", XKW_MOVZX},
    {"movsx", XKW_MOVSX},
    {"push", XKW_PUSH},
    {"pop", XKW_POP},
    {"lea", XKW_LEA},
    {"add", XKW_ADD},
    {"sub", XKW_SUB},
    {"cmp", XKW_CMP},
    {"test", XKW_TEST},
    {"inc", XKW_INC},
    {"dec", XKW_DEC},
    {"imul", XKW_IMUL},
    {"idiv", XKW_IDIV},
    {"mul", XKW_MUL},
    {"div", XKW_DIV},
    {"and", XKW_AND},
    {"or", XKW_OR},
    {"xor", XKW_XOR},
    {"not", XKW_NOT},
    {"neg", XKW_NEG},
    {"shl", XKW_SHL},
    {"shr", XKW_SHR},
    {"jmp", XKW_JMP},
    {"je", XKW_JE},
    {"jne", XKW_JNE},
    {"jz", XKW_JZ},
    {"jnz", XKW_JNZ},
    {"jg", XKW_JG},
    {"jge", XKW_JGE},
    {"jl", XKW_JL},
    {"jle", XKW_JLE},
    {"jb", XKW_JB},
    {"jnae", XKW_JNAE},
    {"jc", XKW_JC},
    {"jnb", XKW_JNB},
    {"jae", XKW_JAE},
    {"jnc", XKW_JNC},
    {"jbe", XKW_JBE},
    {"jna", XKW_JNA},
    {"ja", XKW_JA},
    {"jnbe", XKW_JNBE},
    {"cmp", XKW_CMP},
    {"call", XKW_CALL},
    {"ret", XKW_RET},
    {"seta", XKW_SETA},
    {"setae", XKW_SETAE},
    {"setb", XKW_SETB},
    {"setbe", XKW_SETBE},
    {"setc", XKW_SETC},
    {"sete", XKW_SETE},
    {"setg", XKW_SETG},
    {"setge", XKW_SETGE},
    {"setl", XKW_SETL},
    {"setle", XKW_SETLE},
    {"setna", XKW_SETNA},
    {"setnae", XKW_SETNAE},
    {"setnb", XKW_SETNB},
    {"setnbe", XKW_SETNBE},
    {"setnc", XKW_SETNC},
    {"setne", XKW_SETNE},
    {"setng", XKW_SETNG},
    {"setnge", XKW_SETNGE},
    {"setnl", XKW_SETNL},
    {"setnle", XKW_SETNLE},
    {"setno", XKW_SETNO},
    {"setnp", XKW_SETNP},
    {"setns", XKW_SETNS},
    {"setnz", XKW_SETNZ},
    {"seto", XKW_SETO},
    {"setp", XKW_SETP},
    {"setpe", XKW_SETPE},
    {"setpo", XKW_SETPO},
    {"sets", XKW_SETS},
    {"setz", XKW_SETZ},  
    {"cdq", XKW_CDQ},
    {"leave", XKW_LEAVE},
    {"byte", XKW_BYTE},
    {"word", XKW_WORD},
    {"dword", XKW_DWORD},
    {"ptr", XKW_PTR},
    {"eax", TK_EAX}, {"ebx", TK_EBX}, {"ecx", TK_ECX}, {"edx", TK_EDX}, {"esi", TK_ESI}, {"edi", TK_EDI}, {"esp", TK_ESP}, {"ebp", TK_EBP},
    {"eflags", TK_EFLAGS},
    {"ax", TK_AX}, {"bx", TK_BX}, {"cx", TK_CX}, {"dx", TK_DX},
    {"al", TK_AL}, {"ah", TK_AH}, {"bl", TK_BL}, {"bh", TK_BH}, {"cl", TK_CL}, {"ch", TK_CH}, {"dl", TK_DX}, {"dh", TK_DX},
};

const int KWCount = sizeof(kw)/sizeof(XKeyword);

static XKeyword x_commands[] = {
	{"show", XCKW_SHOW },
	{"set", XCKW_SET },
	{"exec", XCKW_EXEC },
        {"debug", XCKW_DEBUG},
        {"stop", XCKW_STOP },
	{"paddr", XCKW_PADDR},
};

const int KWCmdCount = sizeof(x_commands)/sizeof(XKeyword);

#if defined(_MSC_VER)
#define strcasecmp _stricmp
#endif

static int lookUpWord(XKeyword kw[], int size, string str)
{
    for (int i=0; i<size; i++) {
        if (strcasecmp(str.c_str(), kw[i].name) == 0)
            return kw[i].tokenKind;
    }

    return XTK_ID;
}

X86Lexer::X86Lexer(istream *in)
{
    this->in = in;
    currentLine = 1;
}

int X86Lexer::getNextToken()
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
            case EOF: RETURN_TOKEN(XTK_EOF);
            case '[': RETURN_TOKEN(XTK_LBRACKET);
            case ']': RETURN_TOKEN(XTK_RBRACKET);
            case '(': RETURN_TOKEN(XTK_LPAREN);
            case ')': RETURN_TOKEN(XTK_RPAREN);
            case ',': RETURN_TOKEN(XTK_COMMA);
            case ':': RETURN_TOKEN(XTK_COLON);
            case '-': RETURN_TOKEN(XTK_OP_MINUS);
            case '+': RETURN_TOKEN(XTK_OP_PLUS);
            case '*': RETURN_TOKEN(XTK_OP_MULT);
            case '=': RETURN_TOKEN(XTK_OP_EQUAL);
            case '@': RETURN_TOKEN(XTK_AT);
            case '.': RETURN_TOKEN(XTK_DOT);
            case ';': {
                ch = nextChar();
                while (ch != '\n' && ch != EOF) {
                    ch = nextChar();
                }
                currentLine ++;
                RETURN_TOKEN(XTK_EOL);
            }
            case '\r': {
                ch = nextChar();

                if (ch != '\n')
                    ungetChar(ch);

                currentLine ++;
                RETURN_TOKEN(XTK_EOL);
            }
            case '\n': {
                currentLine ++;
                RETURN_TOKEN(XTK_EOL);
            }
            case '"': {
                tkText.clear();
                ch = nextChar();
                APPEND_SEQUENCE(ch != '"', tkText);
                ch = nextChar();
                tokenInfo.set(tkText, currentLine);
                
                return XSTR_LITERAL;
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
                
                return XTK_CHAR_CONSTANT;
            }
            case '#': {
                tkText.clear();
                ch = nextChar();
                APPEND_SEQUENCE(isalpha(ch), tkText);

                tokenInfo.set(tkText, currentLine);
                return lookUpWord(x_commands, KWCmdCount, tkText);
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

                            return XTK_ERROR;
                        }

                        tokenInfo.set(tkText, currentLine);
                        tokenInfo.intValue = strtoul(tkText.c_str(), NULL, 2);

                        return XTK_BIN_CONSTANT;

                    } else if (prevCh == '0' && (ch == 'x' || ch == 'X')) {
                        tkText.clear();

                        ch = nextChar();
                        APPEND_SEQUENCE(isxdigit(ch), tkText);

                        if (tkText.empty()) {
                            reportError("Invalid hexadecimal constant detected at line %d\n", currentLine);

                            return XTK_ERROR;
                        }
                        tokenInfo.set(tkText, currentLine);
                        tokenInfo.intValue = strtoul(tkText.c_str(), NULL, 16);

                        return XTK_HEX_CONSTANT;
                    } else if (isdigit(ch)) {
						
                        APPEND_SEQUENCE(isdigit(ch), tkText);

                        tokenInfo.set(tkText, currentLine);
                        tokenInfo.intValue = atol(tkText.c_str());

                        return XTK_DEC_CONSTANT;
                    } else {
                        ungetChar(ch);

			tokenInfo.set(tkText, currentLine);
                        tokenInfo.intValue = prevCh - '0';

                        return XTK_DEC_CONSTANT;
                    }
                } else if (isalpha(ch) || ch == '_') {

                    ch = nextChar();
                    APPEND_SEQUENCE( (isalnum(ch) || ch == '_') && (ch != EOF), tkText );

                    tokenInfo.set(tkText, currentLine);
                    return lookUpWord(kw, KWCount, tkText);

                } else {
                    reportError("Invalid symbol '%X' detected at line %d\n", ch, currentLine);
                    return XTK_ERROR;
                }
            }
        }
    }
}

string X86Lexer::getTokenString(int token, TokenInfo *info)
{
    string tokenName;

    switch (token) {
    case XTK_EOF: tokenName = "end of input"; break;
    case XTK_EOL: tokenName = "end of line"; break;

    case XCKW_EXEC:
    case XCKW_SET:
    case XCKW_SHOW:
    case XCKW_PADDR:
    case XCKW_HEX:
    case XCKW_SIGNED:
    case XCKW_UNSIGNED:
    case XCKW_BIN:
    case XCKW_OCT:
    case XCKW_DEC:
    case XKW_DEC:
    case XKW_MOV:
    case XKW_MOVSX:
    case XKW_MOVZX:
    case XKW_PUSH:
    case XKW_POP:
    case XKW_LEA:
    case XKW_ADD:
    case XKW_SUB:
    case XKW_CMP:
    case XKW_TEST:
    case XKW_INC:
    case XKW_IMUL:
    case XKW_IDIV:
    case XKW_AND:
    case XKW_OR:
    case XKW_XOR:
    case XKW_NOT:
    case XKW_NEG:
    case XKW_SHL:
    case XKW_SHR:
    case XKW_JMP:
    case XKW_JE:
    case XKW_JNE:
    case XKW_JZ:
    case XKW_JNZ:
    case XKW_JG:
    case XKW_JGE:
    case XKW_JL:
    case XKW_JLE:
    case XKW_CALL:
    case XKW_RET:
    case XKW_BYTE:
    case XKW_WORD:
    case XKW_DWORD:
    case XKW_PTR:
        tokenName = "keyword";
        break;
    case TK_AL:
    case TK_AH:
    case TK_BL:
    case TK_BH:
    case TK_CL:
    case TK_CH:
    case TK_DL:
    case TK_DH:
    case TK_AX:
    case TK_BX:
    case TK_CX:
    case TK_DX:
    case TK_EAX:
    case TK_EBX:
    case TK_ECX:
    case TK_EDX:
    case TK_ESI:
    case TK_EDI:
    case TK_ESP:
    case TK_EBP:
    case TK_EFLAGS:
        tokenName = "register";
        break;
    case XTK_COMMA:
    case XTK_LPAREN:
    case XTK_RPAREN:
    case XTK_AT:
    case XTK_DOT:
    case XTK_LBRACKET:
    case XTK_RBRACKET:
        tokenName = "symbol";
        break;
    case XTK_OP_PLUS:
    case XTK_OP_MINUS:
    case XTK_OP_MULT:
        tokenName = "operator";
        break;
    case XTK_ID: tokenName = "identifier"; break;
    case XTK_DEC_CONSTANT: tokenName = "decimal constant"; break;
    case XTK_HEX_CONSTANT: tokenName = "hexadecimal constant"; break;
    case XTK_BIN_CONSTANT: tokenName = "binary constant"; break;
    case XTK_OCT_CONSTANT: tokenName = "octal constant"; break;
    default:
        tokenName = ::convertToString(token);
    }

    string result = tokenName;

    if (info != NULL)
        result += " '" + info->tokenLexeme + "'";

    return result;
}
