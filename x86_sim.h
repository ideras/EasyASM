#ifndef X86_SIM_H
#define X86_SIM_H

#include <stdint.h>
#include <fstream>
#include <list>
#include <vector>
#include "util.h"
#include "x86_lexer.h"

#define X_GLOBAL_MEM_WORD_COUNT 256
#define X_STACK_SIZE_WORDS      256
#define XTOTAL_MEM_WORDS            (X_GLOBAL_MEM_WORD_COUNT + XSTACK_SIZE)
#define X_VIRTUAL_GLOBAL_START_ADDR 0x10000000
#define X_VIRTUAL_GLOBAL_END_ADDR   (X_VIRTUAL_GLOBAL_START_ADDR + X_GLOBAL_MEM_WORD_COUNT - 1)
#define X_VIRTUAL_STACK_END_ADDR    0x7FFFEFFC

using namespace std;

#define BIT_SIZE(r) (r)

/* x86 Flags */
#define CF_POS  0
#define PF_POS  2
#define AF_POS  4
#define ZF_POS  6
#define SF_POS  7
#define OF_POS  11

#define CF_MASK  1
#define PF_MASK  4
#define AF_MASK  16
#define ZF_MASK  64
#define SF_MASK  128
#define OF_MASK  2048

/* Bit sizes */
#define BS_8    8
#define BS_16   16
#define BS_32   32

/* Size Directives */
#define SD_None     0
#define SD_BytePtr  8
#define SD_WordPtr  16
#define SD_DWordPtr 32

/* ALU operation */
#define XFN_AND  0x2
#define XFN_OR   0x3
#define XFN_XOR  0x4
#define XFN_NOT  0x5
#define XFN_NEG  0x6
#define XFN_SHL  0x7
#define XFN_SHR  0x8
#define XFN_TEST 0x9
#define XFN_CMP  0x81
#define XFN_ADD  0x82
#define XFN_SUB  0x83
#define XFN_IMUL 0x84
#define XFN_IDIV 0x85

#define XFN_IS_ARITH(fn) ((fn) & 0x80)

typedef unsigned char XBitSize;
typedef unsigned char XSizeDirective;

enum XRegister {R_EAX, R_EBX, R_ECX, R_EDX, R_ESI, R_EDI, R_ESP, R_EBP, R_EFLAGS,
                R_AX, R_BX, R_CX, R_DX,
                R_AL, R_AH, R_BL, R_BH, R_CL, R_CH, R_DL, R_DH};

enum XRefType { RT_Reg, RT_Mem, RT_Const, RT_None };

struct XReference {
    XRefType type; //Register index or Memory address
    int      bitSize; //Target size

    uint32_t address;
    uint32_t value;
};

class XNode;
class XInstruction;

struct XRtContext
{
    XRtContext() {
        ip = 0;
        line = 0;
        stop = false;
    }
    
    int line; // Source line for current instruction
    int ip; // Instruction pointer
    bool stop;
};

class X86Sim
{    
private:
    bool parseFile(istream *in, XParserContext &ctx);
    bool resolveLabels(list<XInstruction *> &linst, vector<XInstruction *> &vinst);
    bool translateVirtualToPhysical(uint32_t vaddr, uint32_t &paddr);

public:
    X86Sim();

    XReference getLastResult() { return last_result; }
    int getSourceLine() { return runtime_context->line; }
    bool getRegValue(int regId, uint32_t &value);
    bool setRegValue(int regId, uint32_t value);
    bool getValue(XReference &ref);
    bool setValue(XReference &ref, uint32_t value);
    bool readMem(uint32_t vaddr, uint32_t &result, XBitSize bitSize);
    bool writeMem(uint32_t vaddr, uint32_t value, XBitSize bitSize);
    bool doOperation(unsigned char op, XReference &ref1, uint32_t value2);
    bool exec(istream *in);
    void updateFlags(uint8_t op, uint8_t sign1, uint8_t sign2, uint32_t arg1, uint32_t arg2, uint32_t result, XBitSize bitSize);

    bool isFlagSet(unsigned int flags) { return (gpr[R_EFLAGS] & flags) != 0; }
    bool showRegValue(int regId, PrintFormat format);
    bool showMemValue(uint32_t vaddr, XBitSize bitSize, PrintFormat format);
    void showFlags();
    
    static inline const char *sizeDirectiveToString(XSizeDirective sd) {
        switch (sd) {
            case SD_BytePtr: return "byte";
            case SD_WordPtr: return "word";
            case SD_DWordPtr: return "dword";
            default:
                return "";
        }
    }

public:
    XRtContext *runtime_context;
    
private:
    XReference last_result;
    uint8_t *getMemPtr(uint32_t vaddr);
    bool hasEvenParity(uint8_t value);

    uint32_t stack_start_address;
    uint32_t gpr[9];
    uint32_t mem[X_GLOBAL_MEM_WORD_COUNT + X_STACK_SIZE_WORDS];
};

#endif // X86_SIM_
