#ifndef MIPS32_TREE_H
#define MIPS32_TREE_H

#include <stdint.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <list>
#include <string>
#include <sstream>
#include "mips32_sim.h"
#include "util.h"
#include "mempool.h"

using namespace std;

#define UNUSED(x) ((void)(x))

void reportError(const char *format, ...);

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

/* Instruction node kinds */
#define MCMD_Show        200
#define MCMD_Set         201
#define MCMD_Exec        202
#define MCMD_Stop        203
#define MINST_1ARG       300
#define MINST_2ARG       301
#define MINST_3ARG       302
#define MINST_TAGGED     303

typedef list<uint32_t> ConstantList;

class MIPS32Sim;

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

/* Argument base class */
class MArgument: public MNode {
protected:
    MArgument() {}

public:
    virtual string toString() = 0;
};

/* Node defitions for arguments */
class MArgRegister: public MArgument
{
public:
    MArgRegister(string regName) {
        this->regName = regName;
    }

    int getKind() { return MARG_REGISTER; }
    string toString() { return regName; }
public:
    string regName;
};

class MArgMemRef: public MArgument
{
public:
    MArgMemRef(int sizeDirective, int offset, int count, string regName) {
        this->sizeDirective = sizeDirective;
        this->offset = offset;
        this->count = count;
        this->regName = regName;
    }

    int getKind() { return MARG_MEMREF; }
    string toString() { return "MemRef"; }
public:
    int sizeDirective;
    int offset;
    int count;
    string regName;
};

class MArgIdentifier: public MArgument
{
public:
    MArgIdentifier(string name) {
        this->name = name;
    }

    int getKind() { return MARG_IDENTIFIER; }
    string toString() { return name; }
public:
    string name;
};

class MArgConstant: public MArgument
{
public:
    MArgConstant(uint32_t value) {
        this->value = value;
    }

    int getKind() { return MARG_IMMEDIATE; }

    string toString() {
        stringstream ss;

        ss << value;

        return ss.str();
    }
public:
    uint32_t value;
};

/* Node definition for x86 instructions and simulator commands */
class MInstruction: public MNode {
protected:
    MInstruction() {}
    
public:
    virtual int getArgumentCount() = 0;
    virtual int resolveArguments(uint32_t values[]) = 0;

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
    int resolveArguments(uint32_t values[]) { return 0; }

    bool exec(MIPS32Sim *sim);

public:
    MArgument *arg;
    PrintFormat dataFormat;
};

class MCmd_Set: public MInstruction {
public:
    MCmd_Set(string name, MArgument *arg, ConstantList &lvalue): MInstruction() {
        this->name = name;
        this->arg = arg;
        this->lvalue = lvalue;
    }

    string toString() { return "set"; }
    int getKind() { return MCMD_Set; }
    int getArgumentCount() { return 2; }
    int resolveArguments(uint32_t values[]) { return 0; }

    bool exec(MIPS32Sim *sim);

public:
    MArgument *arg;
    ConstantList lvalue;
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
    int resolveArguments(uint32_t values[]) { return 0; }

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
    int resolveArguments(uint32_t values[]) { return 0; }

    bool exec(MIPS32Sim *sim) {}
};

class MInstTagged: public MInstruction {
public:
    MInstTagged(string tag, MInstruction *inst): MInstruction() {
        this->name = "nop";
        this->tag = tag;
        this->inst = inst;
    }

    string toString() { return name; }
    int getKind() { return MINST_TAGGED; }
    int getArgumentCount() { return 0; }
    int resolveArguments(uint32_t values[]) { return 0; }
    
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
    int resolveArguments(uint32_t values[]);
    
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
    int resolveArguments(uint32_t values[]);
    
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
    int resolveArguments(uint32_t values[]);
    
public:
    MArgument *arg1;
    MArgument *arg2;
    MArgument *arg3;
};
#endif // MIPS32_TREE_H
