#ifndef X86_TREE_H
#define X86_TREE_H

#include <inttypes.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <list>
#include <string>
#include <sstream>
#include "x86_sim.h"
#include "util.h"
#include "mempool.h"

using namespace std;

#define UNUSED(x) ((void)(x))

void reportError(const char *format, ...);

#define ZX_MASK 1
#define SX_MASK 2
#define WANT_XTEND(f) ((f) & (3))

/* Bit sizes */
#define BS_8    8
#define BS_16   16
#define BS_32   32

/* Size Directives */
#define SD_None     0
#define SD_BytePtr  8
#define SD_WordPtr  16
#define SD_DWordPtr 32

/* Argument kinds */
#define XARGUMENT        100
#define XARG_REGISTER    101
#define XARG_MEMREF      102
#define XARG_IDENTIFIER  103
#define XARG_CONST       104
#define XARG_PADDR       105
#define XARG_EXT_FUNC    106

/* Memory Reference kinds */
#define XADDR_EXPR_REG                  110
#define XADDR_EXPR_REG_PLUS_CONST       111
#define XADDR_EXPR_REG_MINUS_CONST      112
#define XADDR_EXPR_REG_PLUS_REG         113
#define XADDR_EXPR_REG_PLUS_CONST_REG   114
#define XADDR_EXPR_CONST                115
#define XADDR_EXPR_2TERM                116
#define XADDR_EXPR_3TERM                117
#define XADDR_EXPR_MULT                 118

#define XOP_PLUS    0
#define XOP_MINUS   1

/* X86 Instructions */
#define XINST_Mov        0
#define XINST_Movsx      1
#define XINST_Movzx      2
#define XINST_Push       3
#define XINST_Pop        4
#define XINST_Lea        5
#define XINST_Add        6
#define XINST_Sub        7
#define XINST_Imul1      8
#define XINST_Imul2      9
#define XINST_Imul3      10
#define XINST_Idiv       11
#define XINST_And        12
#define XINST_Or         13
#define XINST_Xor        14
#define XINST_Cmp        15
#define XINST_Test       16
#define XINST_Shl        17
#define XINST_Shr        18
#define XINST_Inc        19
#define XINST_Dec        20
#define XINST_Not        21
#define XINST_Neg        22
#define XINST_Jmp        23
#define XINST_Jz         24
#define XINST_Jnz        25
#define XINST_Jl         26
#define XINST_Jg         27
#define XINST_Jle        28
#define XINST_Jge        29
#define XINST_Jb         30
#define XINST_Ja         31
#define XINST_Jbe        32
#define XINST_Jae        33
#define XINST_Call       34
#define XINST_Ret        35
#define XINST_Cdq        36
#define XINST_Mul        37
#define XINST_Div        38
#define XINST_Seta       39
#define XINST_Setae      40
#define XINST_Setb       41
#define XINST_Setbe      42
#define XINST_Setc       43
#define XINST_Sete       44
#define XINST_Setg       45
#define XINST_Setge      46
#define XINST_Setl       47
#define XINST_Setle      48
#define XINST_Setna      49
#define XINST_Setnae     50
#define XINST_Setnb      51
#define XINST_Setnbe     52
#define XINST_Setnc      53
#define XINST_Setne      54
#define XINST_Setng      55
#define XINST_Setnge     56
#define XINST_Setnl      57
#define XINST_Setnle     58
#define XINST_Setno      59
#define XINST_Setnp      60
#define XINST_Setns      61
#define XINST_Setnz      62
#define XINST_Seto       63
#define XINST_Setp       64
#define XINST_Setpe      65
#define XINST_Setpo      66
#define XINST_Sets       67
#define XINST_Setz       68
#define XINST_Leave      69

#define XINST_Tagged     899
#define XCMD_Show        900
#define XCMD_Set         901
#define XCMD_Exec        902
#define XCMD_Stop        903
#define XCMD_Debug       904

extern const char *xreg[];

class XAddrExpr;
class X86Sim;

class XNode {
public:
    static void *operator new(std::size_t sz);
    static void operator delete(void* ptrb);

    virtual string toString() = 0;
    virtual int getKind() = 0;

    bool isA(int kind) { return (getKind() == kind); }
    
    int line;
};

typedef list<XNode *>   XNodeList;

/* Argument base class */
class XArgument: public XNode {
protected:
    XArgument() {}

public:
    virtual bool getReference(X86Sim *xsim, XReference &ref) = 0;
    virtual bool eval(X86Sim *xsim, int resultSize, uint8_t flags, uint32_t &result) = 0;
};

/* Memory Reference base class */
class XAddrExpr: public XNode {
protected:
    XAddrExpr() {}
public:
    virtual bool eval(X86Sim *xsim, uint32_t &result) = 0;
};

/* Node defitions for arguments */
class XArgRegister: public XArgument
{
public:
    XArgRegister(int regSize, int regId) {
        this->regSize = regSize;
        this->regId = regId;
    }

    int getKind() { return XARG_REGISTER; }

    string toString() { return string(xreg[regId]); }

    bool eval(X86Sim *xsim, int resultSize, uint8_t flags, uint32_t &result);
    bool getReference(X86Sim *xsim, XReference &ref);

public:
    uint8_t regSize, regId;
};

class XArgMemRef: public XArgument
{
public:
    XArgMemRef(int sizeDirective, XAddrExpr *expr) {
        this->sizeDirective = sizeDirective;
        this->expr = expr;
    }

    int getKind() { return XARG_MEMREF; }

    string toString() {
        stringstream ss;
        string code;

        switch (sizeDirective) {
            case SD_BytePtr: code = "byte "; break;
            case SD_WordPtr: code = "word "; break;
            case SD_DWordPtr: code = "dword "; break;
        }

        ss << code << "[" << expr->toString() << "]";

        return ss.str();
    }

    bool eval(X86Sim *xsim, int resultSize, uint8_t flags, uint32_t &result);
    bool getReference(X86Sim *xsim, XReference &ref);

public:
    int sizeDirective;
    int count; // For show command
    XAddrExpr *expr;
};

class XArgIdentifier: public XArgument
{
public:
    XArgIdentifier(string name) {
        this->name = name;
    }

    int getKind() { return XARG_IDENTIFIER; }
    string toString() { return name; }

    bool eval(X86Sim *xsim, int resultSize, uint8_t flags, uint32_t &result) {
        if (!xsim->getLabel(name, result)) {
            reportError("Invalid label '%s'\n", name.c_str());
            return false;
        }
        else 
            return true;
    }

    bool getReference(X86Sim *, XReference &) { return false; }

public:
    string name;
};

class XArgPhyAddress: public XArgument
{
public:
    XArgPhyAddress(XAddrExpr *expr) {
        this->expr = expr;
    }

    int getKind() { return XARG_PADDR; }
    string toString() { return "paddr(" + expr->toString() + ")"; }

    bool eval(X86Sim *sim, int resultSize, uint8_t flags, uint32_t &result);
    bool getReference(X86Sim *sim, XReference &ref);

public:
    XAddrExpr *expr;
};

class XArgExternalFuntionName: public XArgument
{
public:
    XArgExternalFuntionName(string libName, string funcName) {
        this->libName = libName;
        this->funcName = funcName;
    }

    int getKind() { return XARG_EXT_FUNC; }
    string toString() { return "@" + libName + "." + funcName; }

    bool eval(X86Sim *xsim, int resultSize, uint8_t flags, uint32_t &result) { return false; }
    bool getReference(X86Sim *, XReference &) { return false; }

public:
    string libName;
    string funcName;
};

class XArgConstant: public XArgument
{
public:
    XArgConstant(uint32_t value) {
        this->value = value;
    }

    int getKind() { return XARG_CONST; }

    string toString() {
        stringstream ss;

        ss << value;

        return ss.str();
    }

    bool eval(X86Sim *xsim, int resultSize, uint8_t flags, uint32_t &result);
    bool getReference(X86Sim *xsim, XReference &ref);

public:
    uint32_t value;
};

/* Node definitions for address expressions */
class XAddrExprReg: public XAddrExpr
{
public:
    XAddrExprReg(XArgument *reg) { this->reg = reg; }

    int getKind() { return XADDR_EXPR_REG; }
    string toString() { return reg->toString(); }
    bool eval(X86Sim *xsim, uint32_t &result);

public:
    XArgument *reg;
};

class XAddrExprConst: public XAddrExpr
{
public:
    XAddrExprConst(int value) { this->value = value; }

    int getKind() { return XADDR_EXPR_CONST; }
    string toString() { return ::convertToString(value); }
    bool eval(X86Sim *xsim, uint32_t &result);

public:
    int value;
};

class XAddrExpr2Term: public XAddrExpr
{
public:
    XAddrExpr2Term(int op, XAddrExpr *expr1, XAddrExpr *expr2) {
        this->op = op;
        this->expr1 = expr1;
        this->expr2 = expr2;
    }

    int getKind() { return XADDR_EXPR_2TERM; }
    string toString() { return expr1->toString() + (op==XOP_PLUS? "+":"-") + expr2->toString(); }
    bool eval(X86Sim *xsim, uint32_t &result);

public:
    int op;
    XAddrExpr *expr1;
    XAddrExpr *expr2;
};

class XAddrExpr3Term: public XAddrExpr
{
public:
    XAddrExpr3Term(int op1, int op2, XAddrExpr *expr1, XAddrExpr *expr2, XAddrExpr *expr3) {
        this->op1 = op1;
        this->op2 = op2;
        this->expr1 = expr1;
        this->expr2 = expr2;
        this->expr3 = expr3;
    }

    int getKind() { return XADDR_EXPR_3TERM; }
    string toString() { return expr1->toString() + (op1==XOP_PLUS? "+":"-") + expr2->toString() +
                                                   (op2==XOP_PLUS? "+":"-") + expr3->toString(); }
    bool eval(X86Sim *xsim, uint32_t &result);
public:
    int op1, op2;
    XAddrExpr *expr1;
    XAddrExpr *expr2;
    XAddrExpr *expr3;
};

class XAddrExprMult: public XAddrExpr
{
public:
    XAddrExprMult(XAddrExpr *expr1, XAddrExpr *expr2) {
        this->expr1 = expr1;
        this->expr2 = expr2;
    }

    int getKind() { return XADDR_EXPR_MULT; }
    string toString() { return expr1->toString() + "*" + expr2->toString(); }
    bool eval(X86Sim *xsim, uint32_t &result);

public:
    XAddrExpr *expr1;
    XAddrExpr *expr2;
};

/* Node definition for x86 instructions and simulator commands */
class XInstruction: public XNode {
protected:
    XInstruction() {}
public:
    virtual bool exec(X86Sim *sim, XReference &result) = 0;
};

class XCmdShow: public XInstruction {
public:
    XCmdShow(XArgument *arg, PrintFormat dataFormat) {
        this->arg = arg;
        this->dataFormat = dataFormat;
    }

    string toString() { return "#show"; }
    int getKind() { return XCMD_Show; }
    bool exec(X86Sim *sim, XReference &result);

public:
    XArgument *arg;
    PrintFormat dataFormat;
};

class XCmdSet: public XInstruction {
public:
    XCmdSet(XArgument *arg, list<int> &lvalue) {
        this->arg = arg;
        this->lvalue = lvalue;
    }

    string toString() { return "#set"; }
    int getKind() { return XCMD_Set; }
    bool exec(X86Sim *sim, XReference &result);

public:
    XArgument *arg;
    list<int> lvalue;
};

class XCmdExec: public XInstruction {
public:
    XCmdExec(string file_path) {
        this->file_path = file_path;
    }

    string toString() { return "#exec \"" + file_path + "\""; }
    int getKind() { return XCMD_Exec; }
    bool exec(X86Sim *sim, XReference &result);

public:
    string file_path;
};

class XCmdDebug: public XInstruction {
public:
    XCmdDebug(string file_path) {
        this->file_path = file_path;
    }

    string toString() { return "#debug \"" + file_path + "\""; }
    int getKind() { return XCMD_Debug; }
    bool exec(X86Sim *sim, XReference &result);

public:
    string file_path;
};

class XCmdStop: public XInstruction {
public:
    XCmdStop() { }

    string toString() { return "#stop"; }
    int getKind() { return XCMD_Stop; }
    bool exec(X86Sim *sim, XReference &result);
};

class XInstTagged: public XInstruction {
public:
    XInstTagged(string tag, XInstruction *inst) {
        this->tag = tag;
        this->inst = inst;
    }
    
    string toString() { return tag; }
    int getKind() { return XINST_Tagged; }
    bool exec(X86Sim *sim, XReference &result) { return false; }
    
public:
    string tag;
    XInstruction *inst;
};

class XInst1Arg: public XInstruction 
{
public:
    XInst1Arg(XArgument *arg) {
        this->arg = arg;
    }
    
    const virtual char *getName() = 0;
    
    string toString() {
        stringstream ss;
        ss << getName() << " " << arg->toString();
        return ss.str();
    }
    
public:    
    XArgument *arg;
};

class XInst2Arg: public XInstruction 
{
public:
    XInst2Arg(XArgument *arg1, XArgument *arg2) {
        this->arg1 = arg1;
        this->arg2 = arg2;
    }
    
    virtual const char *getName() = 0;
    
    string toString() {
        stringstream ss;
        ss << getName() << " " << arg1->toString() << ", " << arg2->toString();
        return ss.str();
    }
    
public:    
    XArgument *arg1;
    XArgument *arg2;
};

class XInst3Arg: public XInstruction 
{
public:
    XInst3Arg(XArgument *arg1, XArgument *arg2, XArgument *arg3) {
        this->arg1 = arg1;
        this->arg2 = arg2;
        this->arg3 = arg3;
    }
    
    virtual const char *getName() = 0;
    
    string toString() {
        stringstream ss;
        ss << getName() << " " << arg1->toString() << ", " 
                               << arg2->toString() << ", " 
                               << arg3->toString();
        return ss.str();
    }
    
public:    
    XArgument *arg1;
    XArgument *arg2;
    XArgument *arg3;
};

#define DEFINE_INSTRUCTION_3ARG(name, opcode)     \
        class XI_##opcode: public XInst3Arg {  \
        public:                                   \
            XI_##opcode(XArgument *arg1, XArgument *arg2, XArgument *arg3): \
                XInst3Arg(arg1, arg2, arg3) { } \
            const char *getName() { return name; } \
            int getKind() { return XINST_##opcode; } \
            bool exec(X86Sim *sim, XReference &result); \
        }

#define DEFINE_INSTRUCTION_2ARG(name, opcode)     \
        class XI_##opcode: public XInst2Arg {  \
        public:                                   \
            XI_##opcode(XArgument *arg1, XArgument *arg2): XInst2Arg(arg1, arg2) { } \
            const char *getName() { return name; } \
            int getKind() { return XINST_##opcode; } \
            bool exec(X86Sim *sim, XReference &result); \
        }

#define DEFINE_INSTRUCTION_1ARG(name, opcode) \
        class XI_##opcode: public XInst1Arg { \
        public:                               \
            XI_##opcode(XArgument *arg): XInst1Arg(arg) { } \
            const char *getName() { return name; } \
            int getKind() { return XINST_##opcode; } \
            bool exec(X86Sim *sim, XReference &result); \
        }

#define DEFINE_INSTRUCTION_0ARG(name, opcode)    \
        class XI_##opcode: public XInstruction { \
        public:                                  \
            XI_##opcode() { }			 \
            string toString() {           \
                return name;              \
            } \
            int getKind() { return XINST_##opcode; } \
            bool exec(X86Sim *sim, XReference &result); \
        }

#define IMPLEMENT_INSTRUCTION(opcode) \
        bool XI_##opcode::exec(X86Sim *sim, XReference &result)

DEFINE_INSTRUCTION_2ARG("mov", Mov);
DEFINE_INSTRUCTION_2ARG("movsx", Movsx);
DEFINE_INSTRUCTION_2ARG("movzx", Movzx);
DEFINE_INSTRUCTION_1ARG("push", Push);
DEFINE_INSTRUCTION_1ARG("pop", Pop);
DEFINE_INSTRUCTION_2ARG("lea", Lea);
DEFINE_INSTRUCTION_2ARG("add", Add);
DEFINE_INSTRUCTION_2ARG("sub", Sub);
DEFINE_INSTRUCTION_1ARG("imul", Imul1); //IMUL with 1 argument
DEFINE_INSTRUCTION_2ARG("imul", Imul2); //IMUL with 2 arguments
DEFINE_INSTRUCTION_3ARG("imul", Imul3); //IMUL with 3 arguments
DEFINE_INSTRUCTION_1ARG("idiv", Idiv);
DEFINE_INSTRUCTION_1ARG("mul", Mul);
DEFINE_INSTRUCTION_1ARG("div", Div);
DEFINE_INSTRUCTION_2ARG("and", And);
DEFINE_INSTRUCTION_2ARG("or", Or);
DEFINE_INSTRUCTION_2ARG("xor", Xor);
DEFINE_INSTRUCTION_2ARG("cmp", Cmp);
DEFINE_INSTRUCTION_2ARG("test", Test);
DEFINE_INSTRUCTION_2ARG("shl", Shl);
DEFINE_INSTRUCTION_2ARG("shr", Shr);
DEFINE_INSTRUCTION_1ARG("inc", Inc);
DEFINE_INSTRUCTION_1ARG("dec", Dec);
DEFINE_INSTRUCTION_1ARG("not", Not);
DEFINE_INSTRUCTION_1ARG("neg", Neg);

DEFINE_INSTRUCTION_1ARG("jmp", Jmp);
DEFINE_INSTRUCTION_1ARG("jz", Jz);
DEFINE_INSTRUCTION_1ARG("jnz", Jnz);
DEFINE_INSTRUCTION_1ARG("jl", Jl);
DEFINE_INSTRUCTION_1ARG("jg", Jg);
DEFINE_INSTRUCTION_1ARG("jle", Jle);
DEFINE_INSTRUCTION_1ARG("jge", Jge);
DEFINE_INSTRUCTION_1ARG("jb", Jb);
DEFINE_INSTRUCTION_1ARG("ja", Ja);
DEFINE_INSTRUCTION_1ARG("jbe", Jbe);
DEFINE_INSTRUCTION_1ARG("jae", Jae);
DEFINE_INSTRUCTION_1ARG("call", Call);
DEFINE_INSTRUCTION_1ARG("ret", Ret);

#define XI_Setnbe   XI_Seta
#define XI_Setnc    XI_Setae
#define XI_Setnb    XI_Setae
#define XI_Setna    XI_Setbe
#define XI_Setnae   XI_Setb
#define XI_Setc     XI_Setb
#define XI_Setnle   XI_Setg
#define XI_Setnl    XI_Setge
#define XI_Setnge   XI_Setl
#define XI_Setng    XI_Setle
#define XI_Setne    XI_Setnz
#define XI_Setpo    XI_Setnp
#define XI_Setpe    XI_Setp
#define XI_Sete     XI_Setz

DEFINE_INSTRUCTION_1ARG("seta", Seta);
DEFINE_INSTRUCTION_1ARG("setae", Setae);
DEFINE_INSTRUCTION_1ARG("setb", Setb);
DEFINE_INSTRUCTION_1ARG("setbe", Setbe);
DEFINE_INSTRUCTION_1ARG("setg", Setg);
DEFINE_INSTRUCTION_1ARG("setge", Setge);
DEFINE_INSTRUCTION_1ARG("setl", Setl);
DEFINE_INSTRUCTION_1ARG("setle", Setle);
DEFINE_INSTRUCTION_1ARG("setno", Setno);
DEFINE_INSTRUCTION_1ARG("setnp", Setnp);
DEFINE_INSTRUCTION_1ARG("setns", Setns);
DEFINE_INSTRUCTION_1ARG("setnz", Setnz);
DEFINE_INSTRUCTION_1ARG("seto", Seto);
DEFINE_INSTRUCTION_1ARG("setp", Setp);
DEFINE_INSTRUCTION_1ARG("sets", Sets);
DEFINE_INSTRUCTION_1ARG("setz", Setz);

DEFINE_INSTRUCTION_0ARG("cdq", Cdq);
DEFINE_INSTRUCTION_0ARG("leave", Leave);


#endif // X86_TREE_H
