#ifndef X86_SIM_H
#define X86_SIM_H

#include <stdint.h>
#include <fstream>
#include <list>
#include <vector>
#include <map>
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

enum XRefType { RT_Reg, RT_PMem, RT_Mem, RT_Const, RT_None };

class X86Sim;

struct XReference {
    XRefType type; //Register index or Memory address
    int      bitSize; //Target size in bits

    bool deref(uint32_t &value);
    bool assign(uint32_t value);
    
    uint32_t address;
    X86Sim   *sim;
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

class X86Debugger;

class X86Sim
{    
    friend class X86Debugger;
    friend class XArgPhyAddress;
    friend struct XReference;
    friend class XI_Call;
private:
    bool resolveLabels(list<XInstruction *> &linst, vector<XInstruction *> &vinst, map<string, uint32_t> &lbl_map);
    bool loadFile(istream *in, vector<XInstruction *> &instList, map<string, uint32_t> &labelMap);
    bool translateVirtualToPhysical(uint32_t vaddr, uint32_t &paddr);

public:
    X86Sim();

    XReference getLastResult() { return last_result; }
    int getSourceLine() { return runtime_context->line; }
    
    bool getLabel(string label, uint32_t &target) { 
        if (label_map != NULL) {
            if (label_map->find(label) != label_map->end()) {
                target = label_map->at(label);
                return true;
            } else {
                return false;
            }
        }
        
        return false;
    }
    
    X86Debugger *getDebugger() { return dbg; }
    bool getRegValue(int regId, uint32_t &value);
    bool setRegValue(int regId, uint32_t value);
    bool readMem(uint32_t vaddr, uint32_t &result, XBitSize bitSize);
    bool writeMem(uint32_t vaddr, uint32_t value, XBitSize bitSize);
    bool doOperation(unsigned char op, XReference &ref1, uint32_t value2);
    bool parseFile(istream *in, XParserContext &ctx);
    bool exec(istream *in);
    bool debug(string asm_file);
    void updateFlags(uint8_t op, uint8_t sign1, uint8_t sign2, uint32_t arg1, uint32_t arg2, uint32_t result, XBitSize bitSize);

    bool isFlagSet(unsigned int flags) { return (gpr[R_EFLAGS] & flags) != 0; }
    
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
    X86Debugger *dbg;
    XRtContext *runtime_context;
    map<string, uint32_t> *label_map;
    
private:
    XReference last_result;
    uint8_t *getMemPtr(uint32_t vaddr);
    bool hasEvenParity(uint8_t value);

    uint32_t stack_start_address;
    uint32_t gpr[9];
    uint32_t mem[X_GLOBAL_MEM_WORD_COUNT + X_STACK_SIZE_WORDS];
};

#endif // X86_SIM_
