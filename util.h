#ifndef ARITH_UTIL_H
#define ARITH_UTIL_H

#include <stdint.h>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

#define MAX_VALUE_U(bs) ( ((1 << ((bs)-1))-1) | (1 << ((bs)-1)) )
#define MASK_FOR(bs)    MAX_VALUE_U(bs)

#define SIGN_BIT(X, BS) ((X) & (1 << ((BS)-1)))

#define ARITH_OVFL(RESULT, OP1, OP2, BS)    (SIGN_BIT (OP1, BS) == SIGN_BIT (OP2, BS) \
                                              && SIGN_BIT (OP1, BS) != SIGN_BIT (RESULT, BS))

template <typename T>
string convertToString(T value)
{
    std::ostringstream os ;
    os << value ;
    return os.str() ;
}

enum PrintFormat {
    F_SignedDecimal, 
    F_UnsignedDecimal, 
    F_Hexadecimal, 
    F_Octal, 
    F_Binary, 
    F_Ascii, 
    F_Unspecified
};

uint32_t signExtend(uint32_t value, int inBitSize, int outBitSize);
void printNumber(uint32_t value, int bitSize, PrintFormat format);
string numberToBinaryString(uint32_t x, int bs);
bool tokenizeString(string str, vector<string> &strList);
#endif // ARITH_UTIL_H
