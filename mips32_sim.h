#ifndef MIPS32_SIM_H
#define MIPS32_SIM_H

#include <stdint.h>
#include <string>
#include <fstream>
#include <vector>
#include <map>

#include "mips32_lexer.h"
#include "mips32_tree.h"
#include "adbg.h"

using namespace std;

#define MKOPCODE1(t1) ((t1 & 0x3F) << 6)
#define MKOPCODE2(t1, t2) (((t1 & 0x3F) << 6) | (t2 & 0x3F))

#define R_FORMAT	1
#define I_FORMAT	2
#define J_FORMAT	3

#define M_GLOBAL_MEM_WORD_COUNT 256
#define M_STACK_SIZE_WORDS      256
#define M_VIRTUAL_GLOBAL_START_ADDR	0x10000000
#define M_VIRTUAL_GLOBAL_END_ADDR	(M_VIRTUAL_GLOBAL_START_ADDR + M_GLOBAL_MEM_WORD_COUNT - 1)
#define M_VIRTUAL_STACK_END_ADDR	0x7FFFEFFC
#define M_VIRTUAL_EXTFUNC_START_ADDR    0x01400000

#define A_REGISTER	1
#define A_IMMEDIATE	2

//MIPS 32 Register Format functions
#define FN_ADD MKOPCODE2(0x00, 0x20)
#define FN_SLL MKOPCODE2(0x00, 0x00)
#define FN_ADDU MKOPCODE2(0x00, 0x21)
#define FN_SRL MKOPCODE2(0x0, 0x02)
#define FN_AND MKOPCODE2(0x00, 0x24)
#define FN_SRA MKOPCODE2(0x0, 0x03)
#define FN_BREAK MKOPCODE2(0x00, 0x0D)
#define FN_SLLV MKOPCODE2(0x0, 0x04)
#define FN_DIV MKOPCODE2(0x00, 0x1A)
#define FN_SRLV MKOPCODE2(0x0, 0x06)
#define FN_DIVU MKOPCODE2(0x00, 0x1B)
#define FN_SRAV MKOPCODE2(0x0, 0x07)
#define FN_JALR MKOPCODE2(0x00, 0x09)
#define FN_JR MKOPCODE2(0x0, 0x08)
#define FN_MFHI MKOPCODE2(0x00, 0x10)
#define FN_SYSCALL MKOPCODE2(0x0, 0x0C)
#define FN_MFLO MKOPCODE2(0x00, 0x12)
#define FN_MTHI MKOPCODE2(0x00, 0x11)
#define FN_MTLO MKOPCODE2(0x00, 0x13)
#define FN_MULT MKOPCODE2(0x00, 0x18)
#define FN_MULTU MKOPCODE2(0x00, 0x19)
#define FN_NOR MKOPCODE2(0x00, 0x27)
#define FN_OR MKOPCODE2(0x00, 0x25)
#define FN_SLT MKOPCODE2(0x00, 0x2A)
#define FN_SLTU MKOPCODE2(0x00, 0x2B)
#define FN_SUB MKOPCODE2(0x0, 0x22)
#define FN_SUBU MKOPCODE2(0x0, 0x23)
#define FN_XOR MKOPCODE2(0x0, 0x26)

//MIPS32 Pseudo functions
#define FN_MOVE MKOPCODE2(0xFF, 0x01)

//MIPS 32 Immediate Format functions
#define FN_ADDI MKOPCODE1(0x08)
#define FN_BLTZ MKOPCODE2(0x01, 0x0)
#define FN_ADDIU MKOPCODE1(0x09)
#define FN_BGEZ MKOPCODE2(0x01, 0x1)
#define FN_ANDI MKOPCODE1(0x0C)
#define FN_BEQ MKOPCODE1(0x04)
#define FN_BNE MKOPCODE1(0x05)
#define FN_BLEZ MKOPCODE2(0x06, 0x0)
#define FN_BGTZ MKOPCODE2(0x07, 0x0)
#define FN_SLTI MKOPCODE1(0x0A)
#define FN_LB MKOPCODE1(0x20)
#define FN_SLTIU MKOPCODE1(0x0B)
#define FN_LBU MKOPCODE1(0x24)
#define FN_LH MKOPCODE1(0x21)
#define FN_ORI MKOPCODE1(0x0D)
#define FN_LHU MKOPCODE1(0x25)
#define FN_XORI MKOPCODE1(0x0E)
#define FN_LUI MKOPCODE1(0x0F)
#define FN_LW MKOPCODE1(0x23)
#define FN_LWC1 MKOPCODE1(0x31)
#define FN_SB MKOPCODE1(0x28)
#define FN_SH MKOPCODE1(0x29)
#define FN_SW MKOPCODE1(0x2B)
#define FN_SWC1 MKOPCODE1(0x39)
#define FN_ADDI MKOPCODE1(0x08)
#define FN_BLTZ MKOPCODE2(0x01, 0x0)
#define FN_ADDIU MKOPCODE1(0x09)
#define FN_BGEZ MKOPCODE2(0x01, 0x1)
#define FN_ANDI MKOPCODE1(0x0C)
#define FN_BEQ MKOPCODE1(0x04)
#define FN_BNE MKOPCODE1(0x05)
#define FN_BLEZ MKOPCODE2(0x06, 0x0)
#define FN_BGTZ MKOPCODE2(0x07, 0x0)
#define FN_SLTI MKOPCODE1(0x0A)
#define FN_LB MKOPCODE1(0x20)
#define FN_SLTIU MKOPCODE1(0x0B)
#define FN_LBU MKOPCODE1(0x24)
#define FN_LH MKOPCODE1(0x21)
#define FN_ORI MKOPCODE1(0x0D)
#define FN_LHU MKOPCODE1(0x25)
#define FN_XORI MKOPCODE1(0x0E)
#define FN_LUI MKOPCODE1(0x0F)
#define FN_LW MKOPCODE1(0x23)
#define FN_LWC1 MKOPCODE1(0x31)
#define FN_SB MKOPCODE1(0x28)
#define FN_SH MKOPCODE1(0x29)
#define FN_SW MKOPCODE1(0x2B)
#define FN_SWC1 MKOPCODE1(0x39)
#define FN_J MKOPCODE1(0x02)
#define FN_JAL MKOPCODE1(0x03)

//Register indexes
#define ZERO_INDEX  0
#define AT_INDEX    1 
#define V0_INDEX    2
#define V1_INDEX    3
#define A0_INDEX    4
#define A1_INDEX    5
#define A2_INDEX    6
#define A3_INDEX    7
#define T0_INDEX    8
#define T1_INDEX    9
#define T2_INDEX    10
#define T3_INDEX    11
#define T4_INDEX    12
#define T5_INDEX    13
#define T6_INDEX    14
#define T7_INDEX    15
#define S0_INDEX    16
#define S1_INDEX    17
#define S2_INDEX    18
#define S3_INDEX    19
#define S4_INDEX    20
#define S5_INDEX    21
#define S6_INDEX    22
#define S7_INDEX    23
#define T8_INDEX    24
#define T9_INDEX    25
#define K0_INDEX    26
#define K1_INDEX    27
#define GP_INDEX    28
#define SP_INDEX    29
#define FP_INDEX    30
#define RA_INDEX    31

struct MIPS32Instruction;
class MNode;
class MInstruction;

struct MRtContext
{
    unsigned pc;    // Program counter	
    int line;       // Source line
    bool stop;
};

enum MRefType { MRT_Reg, MRT_Mem, MRT_Const, MRT_None };

class MIPS32Debugger;
class MIPS32Sim;

class MReference 
{
public:
    MReference(MIPS32Sim *sim) {
        this->sim = sim;
        this->mrt = MRT_None;
    }

    MReference() {
        init();
    }

    void init() {
        this->sim = NULL;
        this->mrt = MRT_None;
    }
    
    bool isNull() { return mrt == MRT_None; }
    bool isConst() { return mrt == MRT_Const; }
    bool isReg() { return mrt == MRT_Reg; }
    bool isMem() { return mrt == MRT_Mem; }
    MIPS32Sim *getSim() { return sim; }
    unsigned getRegIndex() { return u.regIndex; }
    uint32_t getConstValue() { return u.constValue; }
    int getMemWordSize() { return u.mem.wordSize; }
    uint32_t getMemVAddr() { return u.mem.vaddr; }

    void setSim(MIPS32Sim *sim) { 
        this->sim = sim;
    }
    
    void setRegIndex(unsigned regIndex) {
        mrt = MRT_Reg;
        u.regIndex = regIndex;
    }
    
    void setConstValue(uint32_t constValue) {
        mrt = MRT_Const;
        u.constValue = constValue;
    }
    
    void setMemRef(int wordSize, uint32_t vaddr) {
        mrt = MRT_Mem;
        u.mem.wordSize = wordSize;
        u.mem.vaddr = vaddr;
    }

    bool deref(uint32_t &value);
    bool assign(uint32_t &value);
    
protected:
    MRefType mrt;
    MIPS32Sim *sim;
    union {
        unsigned regIndex;
        uint32_t constValue;
        struct {
            int wordSize;
            uint32_t vaddr;
        } mem;
    } u;
};

class MIPS32Sim 
{
    friend class MIPS32Debugger;
private:
    bool loadFile(istream *in, vector<MInstruction *> &instList, map<string, uint32_t> &jmpTbl);
    bool resolveLabels(list<MInstruction *> &linst, vector<MInstruction *> &vinst, map<string, uint32_t> &jmpTbl);
    bool doNativeCall(uint32_t funcAddr);
public:
    MIPS32Sim();
    
    static const char *getRegisterName(int regIndex);

    AsmDebugger *getDebugger();
    bool translateVirtualToPhysical(uint32_t vaddr, uint32_t &paddr);
    bool readWord(unsigned int vaddr, uint32_t &result);
    bool readHalfWord(unsigned int vaddr, uint32_t &result, bool sign_extend);
    bool readByte(unsigned int vaddr, uint32_t &result, bool sign_extend);
    bool writeWord(unsigned int vaddr, uint32_t value);
    bool writeHalfWord(unsigned int vaddr, uint16_t value);
    bool writeByte(unsigned int vaddr, uint8_t value);
    bool getRegisterValue(string name, uint32_t &value);
    bool setRegisterValue(string name, uint32_t value);
    int getSourceLine() { return runtimeCtx->line; }
    bool exec(istream *in);
    bool debug(string asm_file);
    bool execInstruction(MInstruction *inst);
    MReference getLastResult() { return lastResult; }
    bool getLabel(string label, uint32_t &target);
    bool parseFile(istream *in, MParserContext &ctx);
    
    uint32_t reg[32]; //MIPS32 uses 32 registers, 32 bits each one
    uint32_t mem[M_GLOBAL_MEM_WORD_COUNT + M_STACK_SIZE_WORDS]; //Words of physical memory
    uint64_t hi_lo; //Low and High registers
    uint32_t stack_start_address;

public:
    MRtContext *runtimeCtx;
    MReference lastResult;
private:
    map<string, uint32_t> *jumpTable;
    MIPS32Debugger *dbg;
};

enum MIPS32ArgumentType { M32ARG_Register, M32ARG_Immediate };

struct MIPS32Argument {
    MIPS32ArgumentType type; //Register or Immediate
    unsigned int value;
};

struct MIPS32Function {
    const char *name;
    int format;
    unsigned int opcode;
    int argcount;
    unsigned char is_mem_access;
};

struct MIPS32Instruction {
    int opcode;
    int argcount;
    MIPS32Argument arg[3]; //At most 3 arguments
};

MIPS32Function *getFunctionByName(const char *name);
MIPS32Function *getFunctionByOpcode(unsigned int opcode);
const char *getRegisterName(int index);
int mips32_getRegisterIndex(const char *rname);

#endif
