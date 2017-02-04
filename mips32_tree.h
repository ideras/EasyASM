#ifndef MIPS32_TREE_H
#define MIPS32_TREE_H

#include <stdint.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <list>
#include <string>
#include <sstream>
#include <bitset>
#include "mips32_sim.h"
#include "util.h"
#include "mempool.h"

using namespace std;

#define UNUSED(x) ((void)(x))

void reportRuntimeError(const char *format, ...);

/* Mips32 Size Directives */
#define MSD_None    0
#define MSD_Byte    8
#define MSD_HWord   16
#define MSD_Word    32

/* Argument kinds */
#define MARGUMENT        100
#define MARG_REGISTER    101
#define MARG_IMMEDIATE   102
#define MARG_IDENTIFIER  103
#define MARG_MEMREF      104
#define MARG_HIGH_HWORD  105
#define MARG_LOW_HWORD   106
#define MARG_PHY_ADDR    107
#define MARG_EXTFUNC_NAME   108

/* Instruction node kinds */
#define MCMD_Show        200
#define MCMD_Set         201
#define MCMD_Exec        202
#define MCMD_Stop        203
#define MINST_1ARG       300
#define MINST_2ARG       301
#define MINST_3ARG       302
#define MINST_TAGGED     303

/* Address Expression kinds */
#define MADDR_EXPR             400
#define MADDR_EXPR_REG         401
#define MADDR_EXPR_CONST       402
#define MADDR_EXPR_IDENT       403
#define MADDR_EXPR_BASE_OFFSET 404

class MIPS32Sim;
class MReference;
class MArgument;

enum NumberBaseFormat { 
    NBF_SignedDecimal,
    NBF_UnsignedDecimal,
    NBF_Hexadecimal,
    NBF_Octal,
    NBF_Binary,
    NBF_Ascii
};

typedef list<MArgument *> MArgumentList;

/* AST node definitions start here. */
class MNode {
public:
    static void *operator new(std::size_t sz);
    static void operator delete(void* ptrb);

    virtual string toString() = 0;
    virtual int getKind() = 0;

    bool isA(int kind) { return (getKind() == kind); }

public:
    int line; //Source line
};

typedef list<MNode *>   MNodeList;

/* Address expressions */
class MAddrExpr: public MNode {
protected:
    MAddrExpr() {}

public:
    virtual bool eval(MIPS32Sim *sim, uint32_t &value) = 0;
    virtual string toString() = 0;
};

class MAddrExprConstant: public MAddrExpr {
public:
    MAddrExprConstant(MArgument *argValue) {
        this->argValue = argValue;
    }
    
    int getKind() { return MADDR_EXPR_CONST; }
    bool eval(MIPS32Sim *sim, uint32_t &value);
    
    string toString();
    
    MArgument *argValue;
};

class MAddrExprIdentifier: public MAddrExpr {
public:
    MAddrExprIdentifier(string ident) {
        this->ident = ident;
    }
    
    int getKind() { return MADDR_EXPR_IDENT; }
    bool eval(MIPS32Sim *sim, uint32_t &value);    
    string toString();
    
    string ident;
};

class MAddrExprRegister: public MAddrExpr {
public:
    MAddrExprRegister(uint32_t value) {
        this->regIndex = value;
    }
    
    int getKind() { return MADDR_EXPR_REG; }
    bool eval(MIPS32Sim *sim, uint32_t &value);
    string toString();
    
    uint32_t regIndex;
};

class MAddrExprBaseOffset: public MAddrExpr {
public:
    MAddrExprBaseOffset(MArgument *argOffset, uint32_t baseRegIndex) {
        this->argOffset = argOffset;
        this->baseRegIndex = baseRegIndex;
    }
    
    int getKind() { return MADDR_EXPR_BASE_OFFSET; }
    bool eval(MIPS32Sim *sim, uint32_t &value);
    string toString();

    MArgument *argOffset;
    uint32_t baseRegIndex;
};

/* Argument nodes base class */
class MArgument: public MNode {
protected:
    MArgument() {}

public:
    virtual bool getReference(MIPS32Sim *sim, MReference &ref) = 0;
    virtual string toString() = 0;
};

/* Node defitions for arguments */
class MArgRegister: public MArgument
{
public:
    MArgRegister(string regName, unsigned regIndex) {
        this->regIndex = regIndex;
        this->regName = regName;
    }

    int getKind() { return MARG_REGISTER; }
    string toString() { return regName; }
    bool getReference(MIPS32Sim *sim, MReference &ref);

public:
    unsigned regIndex;
    string regName;
};

class MArgMemRef: public MArgument
{
public:
    MArgMemRef(int sizeDirective, MArgument *arg1, MArgument *arg2, MArgument *argCount) {
        this->sizeDirective = sizeDirective;
        this->arg1 = arg1;
        this->arg2 = arg2;
        this->argCount = argCount;
    }

    int getKind() { return MARG_MEMREF; }
    string toString() { return "MemRef"; }    
    bool getReference(MIPS32Sim *sim, MReference &ref);

public:
    int sizeDirective;
    MArgument *arg1;
    MArgument *arg2;
    MArgument *argCount;
};

class MArgIdentifier: public MArgument
{
public:
    MArgIdentifier(string name) {
        this->name = name;
    }

    int getKind() { return MARG_IDENTIFIER; }
    string toString() { return name; }
    bool getReference(MIPS32Sim *sim, MReference &ref);
    
public:
    string name;
};

class MArgConstant: public MArgument
{
public:
    MArgConstant(NumberBaseFormat numberBase, uint32_t value) {
        this->numberBase = numberBase;
        this->value = value;
    }
    MArgConstant(uint32_t value) {
        this->numberBase = NBF_UnsignedDecimal;
        this->value = value;
    }

    int getKind() { return MARG_IMMEDIATE; }

    string toString() {
        stringstream ss;

        switch (numberBase) {
            case NBF_Hexadecimal: ss << "0x" << hex << value << dec; break;
            case NBF_SignedDecimal: ss << ((int32_t)value); break;
            case NBF_UnsignedDecimal: ss << value; break;
            case NBF_Binary: {
					ss << "0b";
				    for (uint32_t mask = (1 << 31); mask > 0; mask >>= 1) {
						ss << (((value & mask) == 0) ? '0' : '1');
					}                
                break;
            }
            case NBF_Octal: ss << value; break;
            case NBF_Ascii: ss << (char)value;
            default:
                ss << value;
        }

        return ss.str();
    }
    
    bool getReference(MIPS32Sim *sim, MReference &ref);

public:
    NumberBaseFormat numberBase;
    uint32_t value;
};

class MArgHighHalfWord: public MArgument
{
public:
    MArgHighHalfWord(MArgument *arg) {
        this->arg = arg;
    }

    int getKind() { return MARG_HIGH_HWORD; }
    string toString() { return "#hihw(" + arg->toString() + ")"; }
    bool getReference(MIPS32Sim *sim, MReference &ref);
    
public:
    MArgument *arg;
};

class MArgLowHalfWord: public MArgument
{
public:
    MArgLowHalfWord(MArgument *arg) {
        this->arg = arg;
    }

    int getKind() { return MARG_LOW_HWORD; }
    string toString() { return "#lohw(" + arg->toString() + ")"; }
    bool getReference(MIPS32Sim *sim, MReference &ref);

public:
    MArgument *arg;
};

class MArgPhyAddress: public MArgument
{
public:
    MArgPhyAddress(MAddrExpr *expr) {
        this->expr = expr;
    }

    int getKind() { return MARG_PHY_ADDR; }
    string toString() { return "#paddr(" + expr->toString() + ")"; }
    bool getReference(MIPS32Sim *sim, MReference &ref);

public:
    MAddrExpr *expr;
};

class MArgExternalFuntionId: public MArgument
{
public:
    MArgExternalFuntionId(string name1, string name2) {
        this->name1 = name1;
        this->name2 = name2;
    }

    int getKind() { return MARG_EXTFUNC_NAME; }
    string toString() { return "@" + name1 + "." + name2; }
    bool getReference(MIPS32Sim *sim, MReference &ref);
    
public:
    string name1;
    string name2;
};

/* Node definition for x86 instructions and simulator commands */
class MInstruction: public MNode {
protected:
    MInstruction() {}
    
public:
    virtual int getArgumentCount() = 0;
    virtual bool resolveArguments(MIPS32Sim *sim, uint32_t values[]) { return false; }

    string name;
};

class MCmd_Show: public MInstruction {
public:
    MCmd_Show(string name, MArgument *arg, PrintFormat dataFormat): MInstruction() {
        this->name = name;
        this->arg = arg;
        this->dataFormat = dataFormat;
    }

    string toString() { return "show"; }
    int getKind() { return MCMD_Show; }
    int getArgumentCount() { return 2; }

    bool exec(MIPS32Sim *sim);

public:
    MArgument *arg;
    PrintFormat dataFormat;
};

class MCmd_Set: public MInstruction {
public:
    MCmd_Set(string name, MArgument *arg, MArgumentList &lvalue): MInstruction() {
        this->name = name;
        this->arg = arg;
        this->lvalue = lvalue;
    }

    string toString() { return "set"; }
    int getKind() { return MCMD_Set; }
    int getArgumentCount() { return 2; }

    bool exec(MIPS32Sim *sim);

public:
    MArgument *arg;
    MArgumentList lvalue;
};

class MCmd_Exec: public MInstruction {
public:
    MCmd_Exec(string name, string file_path): MInstruction() {
        this->name = name;
        this->file_path = file_path;
    }

    string toString() { return "#exec"; }
    int getKind() { return MCMD_Exec; }
    int getArgumentCount() { return 1; }

    bool exec(MIPS32Sim *sim);

public:
    string file_path;
};

class MCmd_Stop: public MInstruction {
public:
    MCmd_Stop() {}

    string toString() { return "#stop"; }
    int getKind() { return MCMD_Stop; }
    int getArgumentCount() { return 0; }

    bool exec(MIPS32Sim *sim) {}
};

class MInstTagged: public MInstruction {
public:
    MInstTagged(string tag, MInstruction *inst): MInstruction() {
        this->name = "nop";
        this->tag = tag;
        this->inst = inst;
    }

    string toString() { return  tag + string(": ") + ((inst != NULL)? inst->toString() : string("")); }
    int getKind() { return MINST_TAGGED; }
    int getArgumentCount() { return 0; }
    
public:
    string tag;   
    MInstruction *inst;
};

class MInst_1Arg: public MInstruction {
public:
    MInst_1Arg(string name, MArgument *arg): MInstruction() {
        this->name = name;
        this->arg1 = arg;
    }

    string toString() { return name; }
    int getKind() { return MINST_1ARG; }
    int getArgumentCount() { return 1; }
    bool resolveArguments(MIPS32Sim *sim, uint32_t values[]);
    
public:
    MArgument *arg1;
};

class MInst_2Arg: public MInstruction {
public:
    MInst_2Arg(string name, MArgument *arg1, MArgument *arg2): MInstruction() {
        this->name = name;
        this->arg1 = arg1;
        this->arg2 = arg2;
    }

    string toString() { return name; }
    int getKind() { return MINST_2ARG; }
    int getArgumentCount() { return 2; }
    bool resolveArguments(MIPS32Sim *sim, uint32_t values[]);
    
public:
    MArgument *arg1;
    MArgument *arg2;
};

class MInst_3Arg: public MInstruction {
public:
    MInst_3Arg(string name, MArgument *arg1, MArgument *arg2, MArgument *arg3): MInstruction() {
        this->name = name;
        this->arg1 = arg1;
        this->arg2 = arg2;
        this->arg3 = arg3;
    }

    string toString() { return name; }
    int getKind() { return MINST_3ARG; }
    int getArgumentCount() { return 3; }
    bool resolveArguments(MIPS32Sim *sim, uint32_t values[]);
    
public:
    MArgument *arg1;
    MArgument *arg2;
    MArgument *arg3;
};
#endif // MIPS32_TREE_H
