#include <cstdio>
#include <sstream>
#include <map>
#include "x86_sim.h"
#include "x86_lexer.h"
#include "x86_tree.h"
#include "mempool.h"
#include "util.h"

using namespace std;

const char *xreg[] = { "eax", "ebx", "ecx", "edx", "esi", "edi", "esp", "ebp", "eflags",
                       "ax", "bx", "cx", "dx",
                       "al", "ah", "bl", "bh", "cl", "ch", "dl", "dh"
                     };

extern MemPool *xpool;

/* Parser related functions */
void *ParseAlloc(void *(*mallocProc)(size_t));
void ParseFree(void *p, void (*freeProc)(void*));
void Parse(void *yyp, int yymajor, TokenInfo *yyminor, XParserContext *ctx);

inline const char *sizeDirectiveToString(XSizeDirective sd) {
    switch (sd) {
        case SD_BytePtr: return "byte";
        case SD_WordPtr: return "word";
        case SD_DWordPtr: return "dword";
        default:
            return "";
    }
}

X86Sim::X86Sim()
{
    gpr[R_ESP] = X_VIRTUAL_STACK_END_ADDR;
    stack_start_address = X_VIRTUAL_STACK_END_ADDR - (X_STACK_SIZE_WORDS * 4);
	runtime_context = NULL;
}

bool X86Sim::getRegValue(int regId, uint32_t &value)
{
    if ((regId >= R_EAX) && (regId <= R_EFLAGS)) {
        value = gpr[regId];

        return true;
    } else if ((regId >= R_AX) && (regId <= R_DX)) {
        switch (regId) {
            case R_AX: value = (gpr[R_EAX] & 0x0000FFFF); break;
            case R_BX: value = (gpr[R_EBX] & 0x0000FFFF); break;
            case R_CX: value = (gpr[R_ECX] & 0x0000FFFF); break;
            case R_DX: value = (gpr[R_EDX] & 0x0000FFFF); break;
            default:
                return false;
        }

        return true;
    } else {
        switch (regId) {
            case R_AL: value = (gpr[R_EAX] & 0x000000FF); break;
            case R_AH: value = ((gpr[R_EAX] & 0x0000FF00) >> 8); break;
            case R_BL: value = (gpr[R_EBX] & 0x000000FF); break;
            case R_BH: value = ((gpr[R_EBX] & 0x0000FF00) >> 8); break;
            case R_CL: value = (gpr[R_ECX] & 0x000000FF); break;
            case R_CH: value = ((gpr[R_ECX] & 0x0000FF00) >> 8); break;
            case R_DL: value = (gpr[R_EDX] & 0x000000FF); break;
            case R_DH: value = ((gpr[R_EDX] & 0x0000FF00) >> 8); break;
            default:
                return false;
        }

        return true;
    }

    return false;
}

bool X86Sim::setRegValue(int regId, uint32_t value)
{
    if ((regId >= R_EAX) && (regId <= R_EFLAGS)) {
        gpr[regId] = value;
    }
    else if ((regId >= R_AX) && (regId <= R_DX)) {
        value &= 0xFFFF;
        switch (regId) {
            case R_AX: gpr[R_EAX] = (gpr[R_EAX] & 0xFFFF0000) | value; break;
            case R_BX: gpr[R_EBX] = (gpr[R_EBX] & 0xFFFF0000) | value; break;
            case R_CX: gpr[R_ECX] = (gpr[R_ECX] & 0xFFFF0000) | value; break;
            case R_DX: gpr[R_EDX] = (gpr[R_EDX] & 0xFFFF0000) | value; break;
            default:
                return false;
        }
    }
    else {
        value &= 0xFF;
        switch (regId) {
            case R_AL: gpr[R_EAX] = (gpr[R_EAX] & 0xFFFFFF00) | (value); break;
            case R_AH: gpr[R_EAX] = (gpr[R_EAX] & 0xFFFF00FF) | (value<<8); break;
            case R_BL: gpr[R_EBX] = (gpr[R_EBX] & 0xFFFFFF00) | (value); break;
            case R_BH: gpr[R_EBX] = (gpr[R_EBX] & 0xFFFF00FF) | (value<<8); break;
            case R_CL: gpr[R_ECX] = (gpr[R_ECX] & 0xFFFFFF00) | (value); break;
            case R_CH: gpr[R_ECX] = (gpr[R_ECX] & 0xFFFF00FF) | (value<<8); break;
            case R_DL: gpr[R_EDX] = (gpr[R_EDX] & 0xFFFFFF00) | (value); break;
            case R_DH: gpr[R_EDX] = (gpr[R_EDX] & 0xFFFF00FF) | (value<<8); break;
            default:
                return false;
        }
    }

    return true;
}

bool X86Sim::setValue(XReference &ref, uint32_t value)
{
    switch (ref.type) {
        case RT_Reg:
            setRegValue(ref.address, value);
            break;
        case RT_Mem:
            writeMem(ref.address, value, ref.bitSize);
            break;
        default:
            return false;
    }

    return true;
}

bool X86Sim::showRegValue(int regId, PrintFormat format)
{
    uint32_t value;

    if (!getRegValue(regId, value))
        return false;

    XBitSize bs;

    if ((regId >= R_EAX) && (regId <= R_EFLAGS)) {
        bs = BS_32;
    }
    else if ((regId >= R_AX) && (regId <= R_DX)) {
        bs = BS_16;
    }
    else if ((regId >= R_AL) && (regId <= R_DH)) {
        bs = BS_8;
    } else {
        return false;
    }

    if ((regId == R_EFLAGS) && format == F_Unspecified) {
        cout << xreg[regId] << ": ";
        showFlags();
    }
    else {
        cout << xreg[regId] << " = ";
        printNumber(value, bs, format);
    }

    cout << endl;

    return true;
}

bool X86Sim::readMem(uint32_t vaddr, uint32_t &result, XBitSize bitSize)
{
    uint8_t *pmem = getMemPtr(vaddr);

    if (pmem == NULL)
        return false;

    switch (bitSize) {
        case BS_8: {
            result = (uint32_t)(*pmem);
            break;
        }
        case BS_16: {
            result = (uint32_t)(*((uint16_t *)pmem));

            break;
        }
        case BS_32: {
            result = *((uint32_t *)pmem);
            break;
        }
        default:
            return false;
    }

    return true;
}

bool X86Sim::writeMem(uint32_t vaddr, uint32_t value, XBitSize bitSize)
{
    uint8_t *pmem = getMemPtr(vaddr);

    if (pmem == NULL)
        return false;

    switch (bitSize) {
        case BS_8: {
            *pmem = (uint8_t)(value & 0xFF);
            break;
        }
        case BS_16: {
            *((uint16_t *)pmem) = value & 0xFFFF;
            break;
        }
        case BS_32: {
            *((uint32_t *)pmem) = value;
            break;
        }
    }

    return true;
}

bool X86Sim::showMemValue(uint32_t vaddr, XBitSize bitSize, PrintFormat format)
{
    uint32_t value;

    if (!readMem(vaddr, value, bitSize))
        return false;

    printf("%s [0x%X] = ", sizeDirectiveToString(bitSize), vaddr);
    printNumber(value, bitSize, format);
    printf("\n");

    return true;
}

bool translateVirtualToPhysical(uint32_t vaddr, uint32_t &paddr);

bool X86Sim::parseFile(istream *in, XParserContext &ctx)
{
    X86Lexer lexer(in);
    TokenInfo *tokenInfo;
    int token;
    void* pParser = ParseAlloc (malloc);
    
    tk_pool = &(ctx.tk_pool);

    while ((token = lexer.getNextToken()) == XTK_EOL);

    while (token != XTK_EOF) {

        if (token == XTK_EOL) {
            tokenInfo = new TokenInfo;
            lexer.getTokenInfo(tokenInfo);
            tokenInfo->line--; //For better error reporting when finding unexpected new lines
            
            while ((token = lexer.getNextToken()) == XTK_EOL);
            
            if (token == XTK_EOF) break;
            
            Parse(pParser, XTK_EOL, tokenInfo, &ctx);

            tokenInfo = new TokenInfo;
            lexer.getTokenInfo(tokenInfo);

            Parse(pParser, token, tokenInfo, &ctx);
        } else {
            tokenInfo = new TokenInfo;
            lexer.getTokenInfo(tokenInfo);

            Parse(pParser, token, tokenInfo, &ctx);
        }

        token = lexer.getNextToken();
    }

    tokenInfo = new TokenInfo;
    lexer.getTokenInfo(tokenInfo);
    Parse(pParser, XTK_EOF, tokenInfo, &ctx);
    Parse(pParser, 0, tokenInfo, &ctx);
    ParseFree(pParser, free);

    tk_pool->freeAll();
    
    if (ctx.error != 0) {
        return false;
    }
    
    return true;
}

bool X86Sim::resolveLabels(list<XInstruction *> &linst, vector<XInstruction *> &vinst)
{
    list<XInstruction *>::iterator it = linst.begin();
    map<string, int> lbl_map;
    int index = 0;
    
    while (it != linst.end()) {
        XInstruction *inst = *it;
        
        if (inst->isA(XINST_Tagged)) {
            XInstTagged *itagged = (XInstTagged *)inst;
            
            lbl_map[itagged->tag] = index;
            inst = itagged->inst;
            
            if (inst != NULL)
                inst->line = itagged->line;
        }
        
        if (inst != NULL) {
            vinst.push_back(inst);
            index++;
        }
        
        it++;
    }
    
    vector<XInstruction *>::iterator it_inst = vinst.begin();
    
    while (it_inst != vinst.end()) {
        XInstruction *inst = *it_inst;
        
        runtime_context->line = inst->line;
        switch (inst->getKind()) {
            case XINST_Jmp:
            case XINST_Jz:
            case XINST_Jnz:
            case XINST_Jl:
            case XINST_Jg:
            case XINST_Jle:
            case XINST_Jge:
            case XINST_Jb:
            case XINST_Ja:
            case XINST_Jbe:
            case XINST_Jae:
            case XINST_Call: {
                XInst1Arg *jinst = (XInst1Arg *)inst;
                
                if (!jinst->arg->isA(XARG_IDENTIFIER)) {
                    reportError("Invalid argument '%s' for instruction '%s'\n", jinst->arg->toString().c_str(), jinst->getName());
                    return false;
                }
                
                string tag = ((XArgIdentifier *)jinst->arg)->name;
                
                if (lbl_map.find(tag) == lbl_map.end()) {
                    reportError("Invalid label '%s' in instruction '%s'\n", tag.c_str(), jinst->getName());
                    return false;
                }
                uint32_t target = lbl_map[tag];
                delete jinst->arg;
                jinst->arg = new XArgConstant(target);
                break;
            }
        }
        
        it_inst ++;
    }
    
    return true;
}
    
bool X86Sim::exec(istream *in)
{
    XRtContext rt_ctx, *old_rt_ctx;
    XParserContext parser_ctx;
    
    
    old_rt_ctx = runtime_context;
    xpool = &(parser_ctx.p_pool);

    runtime_context = NULL;
    
    if (!parseFile(in, parser_ctx)) {
        runtime_context = old_rt_ctx;
        return false;
    }
    
    vector<XInstruction *> vinst;
    
    runtime_context = &rt_ctx;
    
    if (!resolveLabels(parser_ctx.input_list, vinst)) {
        runtime_context = old_rt_ctx;
        return false;
    }
    
    bool result = true;
    int count = vinst.size();
    XReference ref;
    
    rt_ctx.ip = 0;
    rt_ctx.stop = false;
    while (1) {
        XInstruction *inst = vinst[rt_ctx.ip];
        
        rt_ctx.line = inst->line;
        rt_ctx.ip ++;
        if (!inst->exec(this, ref)) {
            result = false;
            break;
        }
        
        if (rt_ctx.stop) break;
        if (rt_ctx.ip >= count) break;
    }
    
    runtime_context = old_rt_ctx;
    
    return result;
}

void X86Sim::updateFlags(uint8_t op, uint8_t sign1, uint8_t sign2, uint32_t arg1, uint32_t arg2, uint32_t result, XBitSize bitSize)
{
    uint32_t eflags = 0;

    //Update carry flag (overflow for unsigned add, borrow for sub)
    switch (op) {
        case XFN_ADD: {
            if ((result < arg1) || (result < arg2))
                eflags |= CF_MASK;
            break;
        }
        case XFN_CMP:
        case XFN_SUB: {
            if (result > arg1)
                eflags |= CF_MASK;

            break;
        }
    }

    //Update parity flag
    if ( hasEvenParity((uint8_t)(result & 0xFF)) )
        eflags |= PF_MASK;

    //Update zero flag
    if (result == 0)
        eflags |= ZF_MASK;

    if (XFN_IS_ARITH(op)) {

        //Update sign flag
        if (SIGN_BIT(result, bitSize) != 0)
           eflags |= SF_MASK;

        //Update overflow flag
        //if (ARITH_OVFL(result, arg1, arg2, bitSize))
		if ( (sign1 == sign2) && (sign1 != SIGN_BIT (result, bitSize)) )
            eflags |= OF_MASK;
    }

    gpr[R_EFLAGS] = eflags;
}

bool X86Sim::doOperation(unsigned char op, XReference &ref1, uint32_t value2)
{
    if (!getValue(ref1))
        return false;

    uint32_t result = ref1.value;
    uint32_t value1 = ref1.value;
    uint8_t sign1, sign2;

    sign1 = SIGN_BIT(value1, ref1.bitSize) != 0;
    sign2 = SIGN_BIT(value2, ref1.bitSize) != 0;

    switch (op) {
        case XFN_TEST:
        case XFN_AND: result &= value2; break;
        case XFN_OR: result |= value2; break;
        case XFN_XOR: result ^= value2; break;
        case XFN_NOT: result = ~value1; break;
        case XFN_NEG: {
            op = XFN_SUB;
            value2 = -value1;
            value1 = 0;
            result = value2;
            break;
        }
        case XFN_ADD: result += value2; break;

        case XFN_CMP:
        case XFN_SUB: {
            result -= value2;
            sign2 = !sign2;
            break;
        }
        case XFN_IMUL: {
            result = ((int32_t)result) * ((int32_t)value2);
            break;
        }
        case XFN_IDIV: {
            result = ((int32_t)result) / ((int32_t)value2);
            break;
        }
        case XFN_SHL: {
            result <<= value2 % 32;
            break;
        }
        case XFN_SHR: {
            result >>= value2 % 32;
            break;
        }
        default:
            return false;
    }

    uint32_t mask = MASK_FOR(ref1.bitSize);
    result &= mask;

    updateFlags(op, sign1, sign2, value1, value2, result, ref1.bitSize);

    if ((op != XFN_CMP) && (op != XFN_TEST)) {

        setValue(ref1, result);
        ref1.value = result;
    }

    return true;
}

bool X86Sim::getValue(XReference &ref)
{
    switch (ref.type) {
        case RT_Reg: return getRegValue(ref.address, ref.value);
        case RT_Mem: return readMem(ref.address, ref.value, ref.bitSize);
        case RT_Const: break;
        default:
            return false;
    }

    return true;
}

void X86Sim::showFlags()
{
    uint32_t value = this->gpr[R_EFLAGS];

    cout << "(CF=" << ((value & CF_MASK) != 0) << ", "
         << "PF=" << ((value & PF_MASK) != 0) << ", "
         << "AF=" << ((value & AF_MASK) != 0) << ", "
         << "ZF=" << ((value & ZF_MASK) != 0) << ", "
         << "SF=" << ((value & SF_MASK) != 0) << ", "
         << "OF=" << ((value & OF_MASK) != 0) << ")";
}

bool X86Sim::translateVirtualToPhysical(uint32_t vaddr, uint32_t &paddr)
{
    if (vaddr >= X_VIRTUAL_GLOBAL_START_ADDR && vaddr <= X_VIRTUAL_GLOBAL_END_ADDR) {
        paddr = vaddr - X_VIRTUAL_GLOBAL_START_ADDR;
    } else if (vaddr >= stack_start_address && vaddr < X_VIRTUAL_STACK_END_ADDR) {
        paddr = (vaddr - stack_start_address) + (X_GLOBAL_MEM_WORD_COUNT * 4);
    } else {
        reportError("Runtime exception: fetch address out of limit 0x%x\n", vaddr);
        return false;
    }
    
    return true;
}

uint8_t *X86Sim::getMemPtr(uint32_t vaddr)
{
    uint32_t offset;
    
    if (!translateVirtualToPhysical(vaddr, offset)) {
        return NULL;
    }

    return (uint8_t *)(((uint8_t *)mem) + offset);
}

bool X86Sim::hasEvenParity(uint8_t value)
{
    value ^= value >> 4;
    value ^= value >> 2;
    value ^= value >> 1;

    return (~value) & 1;
}
