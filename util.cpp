#include <cstdio>
#include <iostream>
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
        default:
            break;
    }
}
