#include <cstdio>
#include <iostream>
#include <vector>
#include "util.h"

uint32_t signExtend(uint32_t value, int inBitSize, int outBitSize)
{
    if (SIGN_BIT(value, inBitSize) != 0) {
        uint32_t outMask = MASK_FOR(outBitSize);
        uint32_t signMask = ~MASK_FOR(inBitSize);

        return ((value | signMask) & outMask);
    } else {
        return value;
    }
}

string numberToBinaryString(uint32_t x, int bs)
{
    string result;

    result.reserve(33);

    for (uint32_t mask = (1 << (bs-1)); mask > 0; mask >>= 1) {
        result += ((x & mask) == 0) ? '0' : '1';
    }

    return result;
}

void printNumber(uint32_t value, int bitSize, PrintFormat format)
{
    static const char *hexFmt[] = {"%02X", "%04X", "%08X"};

    switch (format) {
        case F_SignedDecimal:
            printf("%d", signExtend(value, bitSize, 32));
            break;
        case F_Unspecified:
        case F_UnsignedDecimal:
            printf("%u", value);
            break;
        case F_Hexadecimal: {
            const char *fmt;
            switch (bitSize) {
                case  8: fmt = hexFmt[0]; break;
                case 16: fmt = hexFmt[1]; break;
                case 32: fmt = hexFmt[2]; break;
            }
            printf(fmt, value);
            break;
        }
        case F_Octal:
            printf("%o", value);
            break;
        case F_Binary: {
            string s = numberToBinaryString(value, bitSize);

            cout << s;
            break;
        }
        case F_Ascii: {
            printf("%c", (char)value);
            break;
        }
        default:
            break;
    }
}

bool tokenizeString(string str, vector<string> &strList)
{
    unsigned index = 0;
    string currentToken = "";
    
    strList.clear();
    
    if (str.empty()) return true;
    
    while (index < str.length()) {
        char ch = str.at(index);
        
        switch (ch) {
            case ' ':
            case '\t': {
                
                while (ch == ' ' || ch == '\t') {
                    index++;
                    if (index >= str.length()) break;
                    ch = str.at(index);
                }
                index--;
                
                if (!currentToken.empty()) {
                    strList.push_back(currentToken);
                    currentToken = "";
                }
                
                break;
            }
            case '"': {
                index++;
                currentToken = "";
                while (index < str.length() && (ch = str.at(index)) != '"') {
                    currentToken += ch;
                    index++;
                }
                
                if (ch != '"' || currentToken.empty()) {
                    cout << "Error missing a closing quote in string." << endl;
                    return false;
                }
                break;
            }
            default:
                currentToken += ch;
        }
        index++;
    }
    
    if (!currentToken.empty())
        strList.push_back(currentToken);
    
    return true;
}