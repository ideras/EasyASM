#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <map>
#include <vector>
#include "mips32_sim.h"
#include "mips32_dbg.h"
#include "mips32_lexer.h"
#include "mips32_parser.h"
#include "mips32_tree.h"
#include "mempool.h"
#include "util.h"
#include "native_lib.h"

struct MIPS32Function functions[] = {
	{"add", R_FORMAT, MKOPCODE2(0x00, 0x20), 3, 0},
	{"sll", R_FORMAT, MKOPCODE2(0x0, 0x00), 3, 0},
	{"addu", R_FORMAT, MKOPCODE2(0x00, 0x21), 3, 0},
	{"srl", R_FORMAT, MKOPCODE2(0x0, 0x02), 3, 0},
	{"and", R_FORMAT, MKOPCODE2(0x00, 0x24), 3, 0},
	{"sra", R_FORMAT, MKOPCODE2(0x0, 0x03), 3, 0},
	{"break", R_FORMAT, MKOPCODE2(0x00, 0x0D), 0, 0},
	{"sllv", R_FORMAT, MKOPCODE2(0x0, 0x04), 3, 0},
	{"div", R_FORMAT, MKOPCODE2(0x00, 0x1A), 2, 0},
	{"srlv", R_FORMAT, MKOPCODE2(0x0, 0x06), 3, 0},
	{"divu", R_FORMAT, MKOPCODE2(0x00, 0x1B), 2, 0},
	{"srav", R_FORMAT, MKOPCODE2(0x0, 0x07), 3, 0},
	{"jalr", R_FORMAT, MKOPCODE2(0x00, 0x09), 1, 0},
	{"jr", R_FORMAT, MKOPCODE2(0x0, 0x08), 1, 0},
	{"mfhi", R_FORMAT, MKOPCODE2(0x00, 0x10), 1, 0},
	{"syscall", R_FORMAT, MKOPCODE2(0x0, 0x0C), 0, 0},
	{"mflo", R_FORMAT, MKOPCODE2(0x00, 0x12), 1, 0},
	{"mthi", R_FORMAT, MKOPCODE2(0x00, 0x11), 1, 0},
	{"mtlo", R_FORMAT, MKOPCODE2(0x00, 0x13), 1, 0},
	{"mult", R_FORMAT, MKOPCODE2(0x00, 0x18), 2, 0},
	{"multu", R_FORMAT, MKOPCODE2(0x00, 0x19), 2, 0},
	{"nor", R_FORMAT, MKOPCODE2(0x00, 0x27), 3, 0},
	{"or", R_FORMAT, MKOPCODE2(0x00, 0x25), 3, 0},
	{"slt", R_FORMAT, MKOPCODE2(0x00, 0x2A), 3, 0},
	{"sltu", R_FORMAT, MKOPCODE2(0x00, 0x2B), 3, 0},
	{"sub", R_FORMAT, MKOPCODE2(0x0, 0x22), 3, 0},
	{"subu", R_FORMAT, MKOPCODE2(0x0, 0x23), 3, 0},
	{"xor", R_FORMAT, MKOPCODE2(0x0, 0x26), 3, 0},
        {"addi", I_FORMAT, MKOPCODE1(0x08), 3, 0},
        {"bltz", I_FORMAT, MKOPCODE2(0x01, 0x0), 2, 0},
        {"addiu", I_FORMAT, MKOPCODE1(0x09), 3, 0},
        {"bgez", I_FORMAT, MKOPCODE2(0x01, 0x1), 2, 0},
        {"andi", I_FORMAT, MKOPCODE1(0x0C), 3, 0},
        {"beq", I_FORMAT, MKOPCODE1(0x04), 3, 0},
        {"bne", I_FORMAT, MKOPCODE1(0x05), 3, 0},
        {"blez", I_FORMAT, MKOPCODE2(0x06, 0x0), 2, 0},
        {"bgtz", I_FORMAT, MKOPCODE2(0x07, 0x0), 2, 0},
        {"slti", I_FORMAT, MKOPCODE1(0x0A), 3, 0},
        {"lb", I_FORMAT, MKOPCODE1(0x20), 3, 1},
        {"sltiu", I_FORMAT, MKOPCODE1(0x0B), 3, 0},
        {"lbu", I_FORMAT, MKOPCODE1(0x24), 3, 1},
        {"lh", I_FORMAT, MKOPCODE1(0x21), 3, 1},
        {"ori", I_FORMAT, MKOPCODE1(0x0D), 3, 0},
        {"lhu", I_FORMAT, MKOPCODE1(0x25), 3, 1},
        {"xori", I_FORMAT, MKOPCODE1(0x0E), 3, 0},
        {"lui", I_FORMAT, MKOPCODE1(0x0F), 2, 0},
        {"lw", I_FORMAT, MKOPCODE1(0x23), 3, 1},
        {"lwc1", I_FORMAT, MKOPCODE1(0x31), 3, 1},
        {"sb", I_FORMAT, MKOPCODE1(0x28), 3, 1},
        {"sh", I_FORMAT, MKOPCODE1(0x29), 3, 1},
        {"sw", I_FORMAT, MKOPCODE1(0x2B), 3, 1},
        {"swc1", I_FORMAT, MKOPCODE1(0x39), 3, 1},
        {"j", J_FORMAT, MKOPCODE1(0x02), 1, 0},
        {"jal", J_FORMAT, MKOPCODE1(0x03), 1, 0},
        {"move", R_FORMAT, MKOPCODE2(0xFF, 0x01), 2, 0},
};

const int function_count = sizeof(functions) / sizeof(functions[0]);

extern MemPool *mpool;
extern map<uint32_t, void *> externalFuncHandles;

void reportRuntimeError(const char *format, ...);

/* Parser related functions */
void *Mips32ParseAlloc(void *(*mallocProc)(size_t));
void Mips32ParseFree(void *p, void (*freeProc)(void*));
void Mips32Parse(void *yyp, int yymajor, TokenInfo *yyminor, MParserContext *ctx);

MIPS32Sim::MIPS32Sim()
{
    for (int i = 0; i <= 31; i++)
        reg[i] = 0;

    reg[SP_INDEX] = M_VIRTUAL_STACK_END_ADDR;
    reg[GP_INDEX] = M_VIRTUAL_GLOBAL_START_ADDR;
    stack_start_address = M_VIRTUAL_STACK_END_ADDR - (M_STACK_SIZE_WORDS * 4);
    runtimeCtx = NULL;
    dbg = NULL;
}

AsmDebugger *MIPS32Sim::getDebugger()
{
    return dbg;
}

bool MIPS32Sim::translateVirtualToPhysical(uint32_t vaddr, uint32_t &paddr)
{
    if (vaddr >= M_VIRTUAL_GLOBAL_START_ADDR && vaddr <= M_VIRTUAL_GLOBAL_END_ADDR) {
        paddr = vaddr - M_VIRTUAL_GLOBAL_START_ADDR;
    } else if (vaddr >= stack_start_address && vaddr < M_VIRTUAL_STACK_END_ADDR) {
        paddr = (vaddr - stack_start_address) + (M_GLOBAL_MEM_WORD_COUNT * 4);
    } else {
        reportRuntimeError("Runtime exception: fetch address out of limit 0x%x\n", vaddr);
        return false;
    }
    
    return true;
}

bool MIPS32Sim::readWord(unsigned int vaddr, uint32_t &result)
{
    uint32_t paddr;
    
    if ((vaddr % 4) != 0) {
        reportRuntimeError("Runtime exception: fetch address not aligned on word boundary 0x%x\n", vaddr);
	    return false;
    }
    
    if (!translateVirtualToPhysical(vaddr, paddr))
        return 0;

    result = mem[paddr / 4];
    
    return true;
}

bool MIPS32Sim::readByte(unsigned int vaddr, uint32_t &result, bool sign_extend)
{
    uint32_t paddr;

    if (!translateVirtualToPhysical(vaddr, paddr))
        return false;
    
    uint32_t byteToRead = vaddr % 4;
    uint32_t byteMask;
    
    //This assumes Big Endian order
    switch (byteToRead) {
        case 0: byteMask = 0xFF000000; break;
        case 1: byteMask = 0x00FF0000; break;
        case 2: byteMask = 0x0000FF00; break;
        case 3: byteMask = 0x000000FF; break;
    }

    int shift = (3 - byteToRead) * 8;
    result = (mem[paddr / 4] & byteMask) >> shift;

    if (sign_extend && ((result & (1 << 7))!=0))
        result |= 0xFFFFFF00;

    return true;

}

bool MIPS32Sim::readHalfWord(unsigned int vaddr, uint32_t &result, bool sign_extend)
{
    uint32_t paddr;

    if (!translateVirtualToPhysical(vaddr, paddr))
        return false;
        
    uint32_t hwordToRead = vaddr - (vaddr/4)*4, shift;
    uint32_t hwordMask;

    if ((vaddr % 2) != 0) {
        reportRuntimeError("Runtime exception: fetch address not aligned on halfword boundary 0x%x\n", vaddr);
        return false;
    }

    //This assumes Big Endian order
    switch (hwordToRead) {
        case 0: hwordMask = 0xFFFF0000; shift = 16; break;
        case 2: hwordMask = 0x0000FFFF; shift = 0; break;
    }

    uint32_t word = mem[paddr / 4];
    result = (word & hwordMask) >> shift;

    if (sign_extend && (SIGN_BIT(result, 16) == 1))
        result |= 0xFFFF0000;
    
    return true;
}

bool MIPS32Sim::writeWord(unsigned int vaddr, uint32_t value)
{
    uint32_t paddr;

    if (!translateVirtualToPhysical(vaddr, paddr))
        return false;

    mem[paddr / 4] = value;
    return true;
}

bool MIPS32Sim::writeHalfWord(unsigned int vaddr, uint16_t value)
{
    uint32_t paddr;

    if (!translateVirtualToPhysical(vaddr, paddr))
        return false;
        
    int pos = vaddr - (vaddr / 4) * 4;
    int shift;
    uint32_t mask;

    switch (pos) {
        case 0: mask = 0x0000FFFF; shift = 16; break;
        case 2: mask = 0xFFFF0000; shift = 0; break;
    }

    uint32_t word = mem[paddr / 4];
    mem[paddr / 4] = (word & mask) | (((uint32_t)(value)) << shift);

    return true;
}

bool MIPS32Sim::writeByte(unsigned int vaddr, uint8_t value)
{
    uint32_t paddr;

    if (!translateVirtualToPhysical(vaddr, paddr))
        return false;
    
    int bytePos = vaddr % 4;
    int mask, shift;

    switch (bytePos) {
        case 0: mask = 0x00FFFFFF; shift = 24; break;
        case 1: mask = 0xFF00FFFF; shift = 16; break;
        case 2: mask = 0xFFFF00FF; shift = 8; break;
        case 3: mask = 0xFFFFFF00; shift = 0; break;
    }

    uint32_t word = mem[paddr / 4];

    mem[paddr / 4] = (word & mask) | (((uint32_t)(value)) << shift);

    return true;
}

static inline int isShiftFunction(int opcode) {
    return ((opcode==FN_SLL) || (opcode==FN_SRL) || (opcode==FN_SRA));
}

bool MIPS32Sim::getRegisterValue(string name, uint32_t &value)
{
    int regIndex = mips32_getRegisterIndex(name.c_str());
    
    if (regIndex < 0) {
        reportRuntimeError("Invalid register name '%s'\n", name.c_str());
        return false;
    }
    
    value = reg[regIndex];
    return true;
}

bool MIPS32Sim::setRegisterValue(string name, uint32_t value)
{
    int regIndex = mips32_getRegisterIndex(name.c_str());
    
    if (regIndex < 0) {
        reportRuntimeError("Invalid register name '%s'\n", name.c_str());
        return false;
    }
    
    reg[regIndex] = value;
    return true;
}

bool MIPS32Sim::parseFile(istream *in, MParserContext &ctx)
{
    Mips32Lexer lexer(in);
    TokenInfo *tokenInfo;
    int token;
    void* pParser = Mips32ParseAlloc (malloc);
    
    tk_pool = &(ctx.tokenPool);

    while ((token = lexer.getNextToken()) == MTK_EOL);

    while (token != MTK_EOF) {

        if (token == MTK_EOL) {
            tokenInfo = new TokenInfo;
            lexer.getTokenInfo(tokenInfo);
            tokenInfo->line--; //For better error reporting when finding unexpected new lines
            
            while ((token = lexer.getNextToken()) == MTK_EOL);
            
            if (token == MTK_EOF) break;
            
            Mips32Parse(pParser, MTK_EOL, tokenInfo, &ctx);

            tokenInfo = new TokenInfo;
            lexer.getTokenInfo(tokenInfo);
            
            Mips32Parse(pParser, token, tokenInfo, &ctx);
        } else {
            tokenInfo = new TokenInfo;
            lexer.getTokenInfo(tokenInfo);    
            Mips32Parse(pParser, token, tokenInfo, &ctx);
        }

        token = lexer.getNextToken();
    }

    tokenInfo = new TokenInfo;
    lexer.getTokenInfo(tokenInfo);
            
    Mips32Parse(pParser, MTK_EOF, tokenInfo, &ctx);
    Mips32Parse(pParser, 0, NULL, &ctx);
    Mips32ParseFree(pParser, free);

    tk_pool->freeAll();
    tk_pool = NULL;

    return (ctx.error == 0);
}

bool MIPS32Sim::getLabel(string label, uint32_t &target) 
{
    if (jumpTable != NULL) {
        if (jumpTable->find(label) != jumpTable->end()) {
            target = jumpTable->at(label);
            return true;
        } else {
            return false;
        }
    }

    return false;
}

bool MIPS32Sim::resolveLabels(list<MInstruction *> &linst, vector<MInstruction *> &vinst, map<string, uint32_t> &jmpTbl)
{
    list<MInstruction *>::iterator it = linst.begin();
    int index = 0;
    
    while (it != linst.end()) {
        MInstruction *inst = (MInstruction *)(*it);
        
        if (inst->isA(MINST_TAGGED)) {
            MInstTagged *itagged = (MInstTagged *)inst;
            
            jmpTbl[itagged->tag] = index;
            inst = itagged->inst;
        }
        
        if (inst != NULL) {
            vinst.push_back(inst);
            index++;
        }
        
        it++;
        
    }

    return true;
}

bool MIPS32Sim::loadFile(istream *in, vector<MInstruction *> &instList, map<string, uint32_t> &labelMap)
{
    MParserContext parser_ctx;
    
    if (!parseFile(in, parser_ctx)) {
        return false;
    }
    
    if (!resolveLabels(parser_ctx.instList, instList, labelMap)) {
        return false;
    }
    
    return true;
}

bool MIPS32Sim::exec(istream *in)
{
    MParserContext parse_ctx;

    mpool = &(parse_ctx.parserPool);
    if (!parseFile(in, parse_ctx)) {
        return false;
    }
    
    vector<MInstruction *> vinst;
    map<string, uint32_t> jmpTbl;
    
    if (!resolveLabels(parse_ctx.instList, vinst, jmpTbl)) {
        return false;
    }
    
    unsigned count = vinst.size();
    MRtContext *prev_ctx = runtimeCtx;
    map<string, uint32_t> *prev_jmpTbl = jumpTable;
    MRtContext ctx;
    bool result = true;

    runtimeCtx = &ctx;
    jumpTable = &jmpTbl;

    ctx.pc = 0;
    ctx.stop = false;
    lastResult.init();
    while ((ctx.pc < count) && !ctx.stop) {
        MInstruction *inst = vinst[ctx.pc];
        
        ctx.line = inst->line;
        ctx.pc++;
        if (!execInstruction(inst)) {
            result = false;
            break;
        }
    }
    
    runtimeCtx = prev_ctx;
    mpool = NULL;

    return result;
}

bool MIPS32Sim::debug(string asm_file) 
{
    if (dbg != NULL) {
        printf("The simulator is in debug mode already.\n");
        return false;
    }
    
    ifstream in;
    
    in.open(asm_file.c_str(), ifstream::in|ifstream::binary);

    if (!in.is_open()) {
        printf("Cannot open file '%s'\n", asm_file.c_str());
        return false;
    }
    
    vector<MInstruction *> instList;

    jumpTable = new map<string, uint32_t>;
    mpool = new MemPool();
    
    if (!loadFile(&in, instList, *jumpTable)) {
        delete jumpTable;
        delete mpool;
        
        return false;
    }
    
    vector<string> sourceLines;

    in.clear();
    in.seekg(0);
    
    if (in.fail()) {
        cout << "Oops. Fail." << endl;
        dbg->stop();
        return false;
    }
    
    while (!in.eof()) {
        string line;
        getline(in, line);
        sourceLines.push_back(line);
    };
    
    runtimeCtx = new MRtContext;
    
    dbg = new MIPS32Debugger(this, instList, mpool);
    dbg->setSourceLines(sourceLines);
        
    in.close();
    
    return true;
}

bool MIPS32Sim::doNativeCall(uint32_t funcAddr)
{
    uint32_t r_sp, r_v0, r_v1;
    uint32_t p_index;
    void *old_stack_ptr, *new_stack_ptr;
    HFUNC hfunc;

    r_sp = reg[SP_INDEX];

    //Push a0, a1, a2, a4
    r_sp -= 4;

    if (!writeWord(r_sp, reg[A3_INDEX]))
        return false;
    
    r_sp -= 4;
    
    if (!writeWord(r_sp, reg[A2_INDEX]))
        return false;

    r_sp -= 4;

    if (!writeWord(r_sp, reg[A1_INDEX]))
        return false;

    r_sp -= 4;

    if (!writeWord(r_sp, reg[A0_INDEX]))
        return false;
    
    
    if (!translateVirtualToPhysical(r_sp, p_index))
        return false;
    
    new_stack_ptr = (void *)(&mem[p_index / 4]);
    hfunc = externalFuncHandles[funcAddr];

#ifdef _WIN32
    __asm {
        mov old_stack_ptr, esp
        mov esp, new_stack_ptr
        call [hfunc]
        mov r_v0, eax
        mov r_v1, edx
        mov esp, old_stack_ptr
    };
#elif __linux__
#if defined(__i386__) || defined(__x86_64__)
    asm volatile ("xchg %%esp, %0\n\t"
      : "=r"(old_stack_ptr) /* output */
      : "0"(new_stack_ptr) /* input */
    );

    asm volatile ("call *%2\n" : "=a"(r_v0), "=d"(r_v1) : "r"(hfunc));
    asm volatile ("mov %0, %%esp\n\t" : : "r"(old_stack_ptr) );
#else
# error Native call not supported in this architecture.
#endif

#else
#error "Unknownk compiler"
#endif
    
    reg[V0_INDEX] = r_v0;
    reg[V1_INDEX] = r_v1;
    
    readWord(r_sp, reg[A0_INDEX]);
    r_sp += 4;
    readWord(r_sp, reg[A1_INDEX]);
    r_sp += 4;
    readWord(r_sp, reg[A2_INDEX]);
    r_sp += 4;
    readWord(r_sp, reg[A3_INDEX]);
    
    return true;
}

bool MIPS32Sim::execInstruction(MInstruction *inst)
{
    int count1, count2, argcount;
    MIPS32Function *f;
    uint32_t values[3];
    MRtContext *ctx = runtimeCtx;
    
    lastResult.init();
    
    if (inst->isA(MCMD_Show)) {
        MCmd_Show *shCmd = (MCmd_Show *)inst;
       
        return shCmd->exec(this);
    } else if (inst->isA(MCMD_Set)) {
        MCmd_Set *cmd = (MCmd_Set *)inst;
       
        return cmd->exec(this);
    } else if (inst->isA(MCMD_Exec)) {
        MCmd_Exec *cmd = (MCmd_Exec *)inst;
        
        return cmd->exec(this);
    } else if (inst->isA(MCMD_Stop)) {
        ctx->stop = true;
        return true;
    }

    f = getFunctionByName(inst->name.c_str());
    
    if (f == NULL) {
        reportRuntimeError("Invalid instruction '%s'\n", inst->name.c_str());
        return false;
    }

    count1 = f->argcount;
    count2 = (f->is_mem_access) ? (f->argcount - 1) : (f->argcount);
    argcount = inst->getArgumentCount();
    
    if ((argcount != count1) && (argcount != count2)) {
        reportRuntimeError("Invalid number of arguments to function '%s', expected %d, found %d\n",
                f->name, f->argcount, argcount);
        return false;
    }
    if (!inst->resolveArguments(this, values)) {
        return false;
    }

    int rd; //Index of destination register
    uint32_t *p0, *p1, *p2; //Maximum 3 registers arguments
    uint32_t imm; //Immediate argument value

    rd = values[0];
    switch (f->format) {
        case R_FORMAT:
        {
            switch (argcount) {
                case 3: {
                    if (isShiftFunction(f->opcode)) {
                        imm = values[2];
                        p1 = &(reg[values[1]]);
                        p0 = &(reg[values[0]]);
                    } else {
                        p2 = &(reg[values[2]]);
                        p1 = &(reg[values[1]]);
                        p0 = &(reg[values[0]]);
                    }
                    break;
                }
                case 2: {
                    p1 = &(reg[values[1]]);
                    p0 = &(reg[values[0]]);                    
                    break;
                }
                case 1: {
                    p0 = &(reg[values[0]]);
                }
            }
            break;
        }
        case I_FORMAT:
        {            
            switch (f->argcount) {
                case 3: {
                    imm = values[2];
                    p1 = &(reg[values[1]]);
                    p0 = &(reg[values[0]]);                      
                    break;
                }
                case 2: {
                    imm = values[1];
                    p0 = &(reg[values[0]]);
                    break;
                }
                
                default:
                    reportRuntimeError("%d: BUG in machine\n", __LINE__);
                    return false;
            }
            
            break;
        }
        case J_FORMAT:
        {
            imm = values[0];
            break;
        }
        default:
            reportRuntimeError("%d: BUG in machine\n", __LINE__);
            return false;
    }

    bool write_register = f->opcode != FN_BEQ && 
                          f->opcode != FN_BNE && 
                          f->opcode != FN_SW &&
                          f->opcode != FN_SH &&
                          f->opcode != FN_SB;
    
    if (rd == 0 && write_register) {
        reportRuntimeError("Register $zero cannot be used as target in instruction '%s'\n", f->name);
        return false;
    }

    switch (f->opcode) {
        case FN_ADD:    // add rd, rs, rt ; R Format
        { 
            unsigned int result = *p1 + *p2;
            if (ARITH_OVFL(result, *p1, *p2, 32)) {
                reportRuntimeError("Aritmethic overflow in 'add' operation\n");
            } else {
                *p0 = result;
            }
            break;
        }
        case FN_SLL: // sll rd,rt,sa ; R Format
            *p0 = *p1 << imm;
            break;
        case FN_ADDU: // addu rd,rs,rt ; R Format
            *p0 = *p1 + *p2;
            break;
        case FN_SRL: // srl rd,rt,sa ; R Format
            *p0 = *p1 >> imm;
            break;
        case FN_AND: // and rd,rs,rt ; R Format
            *p0 = *p1 & *p2;
            break;
        case FN_SRA: // sra rd,rt,sa ; R Format
            *p0 = *p1 >> imm;
            if (SIGN_BIT (*p0, 32) != SIGN_BIT(*p1, 32)) // == SIGN_BIT (p1, 32))
                *p0 =  signExtend(*p0, 32 - imm, 32);                    
            break;            
        case FN_BREAK: // break  ; R Format
            break;
        case FN_SLLV: // sllv rd,rt,rs ; R Format
            break;
        case FN_DIV: // div rs,rt ; R Format
            hi_lo = ((int32_t)*p0 / (int32_t)*p1);
            hi_lo &= 0x00000000FFFFFFFF;
            hi_lo = (((uint64_t) ((int32_t)*p0 % (int32_t)*p1)) << 32) | hi_lo;
            break;
        case FN_SRLV: // srlv rd,rt,rs ; R Format
            break;
        case FN_DIVU: // divu rs,rt ; R Format
            hi_lo = *p0 / *p1;
            hi_lo &= 0x00000000FFFFFFFF;
            hi_lo = ((uint64_t)(*p0 % *p1) << 32) | hi_lo;
            printf("%u\n", (*p0 % *p1)); 
            break;
        case FN_SRAV: // srav rd,rt,rs ; R Format
            break;
        case FN_JALR: // jalr rs ; R Format
            reg[RA_INDEX] = ctx->pc;
            if ((*p0 >= M_VIRTUAL_EXTFUNC_START_ADDR) && (*p0 < M_VIRTUAL_GLOBAL_START_ADDR)) {
                return doNativeCall(*p0);
            } else {
                ctx->pc = *p0;
            }
            break;
        case FN_JR: // jr rs ; R Format
            if ((*p0 >= M_VIRTUAL_EXTFUNC_START_ADDR) && (*p0 < M_VIRTUAL_GLOBAL_START_ADDR)) {
                reportRuntimeError("Jump to native functions are not valid. Use JALR if you want to call a native function.\n");
                return false;
            } else {
                ctx->pc = *p0;
            }
            break;
        case FN_MFHI: // mfhi rd ; R Format
            *p0 = (uint32_t)(hi_lo >> 32);
            break;
        case FN_SYSCALL: // syscall  ; R Format
            break;
        case FN_MFLO: // mflo rd ; R Format
            *p0 = (uint32_t)(hi_lo & 0xFFFFFFFF);
            break;
        case FN_MTHI: // mthi rs ; R Format
            break;
        case FN_MTLO: // mtlo rs ; R Format
            break;
        case FN_MULT: // mult rs, rt ; R Format
            hi_lo = (int64_t)( ((int32_t)*p0) * ((int32_t)*p1) );
            break;
        case FN_MULTU: // multu rs, rt ; R Format
            hi_lo = (uint64_t)( (*p0) * (*p1) );
            break;
        case FN_NOR: // nor rd,rs,rt ; R Format
            *p0 = ~(*p1 | *p2);
            break;
        case FN_OR: // or rd,rs,rt ; R Format
            *p0 = *p1 | *p2;
            break;
        case FN_SLT: // slt rd,rs,rt ; R Format
            *p0 = ((int32_t)*p1 < (int32_t)*p2);
            break;
        case FN_SLTU: // sltu rd,rs,rt ; R Format
            *p0 = *p1 < *p2;
            break;
        case FN_SUB: // sub rd,rs,rt ; R Format
            *p0 = (int32_t)*p1 - (int32_t)*p2;
            break;
        case FN_SUBU: // subu rd,rs,rt ; R Format
            *p0 = *p1 - *p2;
            break;
        case FN_XOR: // xor rd,rs,rt ; R Format
            *p0 = *p1 ^ *p2;
            break;
        case FN_ADDI: // addi rt,rs,immediate ; I Format
            *p0 = (int32_t)*p1 + (int32_t)((int16_t)imm);
            break;
        case FN_ADDIU: // addiu rt, rs, immediate ; I Format
            *p0 = *p1 + (uint32_t)((int16_t)imm);
            break;
        case FN_ANDI: // andi rt,rs,immediate ; I Format
            *p0 = *p1 & (imm & 0xFFFF);
            break;
        case FN_BEQ: // beq rs, rt, label ; I Format
            if (*p0 == *p1)
                ctx->pc = imm;
            break;
        case FN_BNE: // bne rs,rt ; I Format
            if (*p0 != *p1)
                ctx->pc = imm;
            break;
        case FN_BLEZ: // blez rs, label ; I Format
            if (*((int32_t *)p0) <= 0)
                ctx->pc = imm;
            break;
        case FN_BGEZ: // bgez rs, label ; I Format
            if (*((int32_t *)p0) >= 0)
                ctx->pc = imm;            
            break;
        case FN_BLTZ: // bltz rs, label ; I Format
            if (*((int32_t *)p0) < 0)
                ctx->pc = imm;
            break;            
        case FN_BGTZ: // bgtz rs, label ; I Format
            if (*((int32_t *)p0) > 0)
                ctx->pc = imm;            
            break;
        case FN_SLTI: // slti rt, rs, immediate ; I Format
            *p0 = (int32_t)*p1 < (int32_t)((int16_t)imm);
            break;
        case FN_SLTIU: // slti rt, rs, immediate ; I Format
            *p0 = (uint32_t)*p1 < (uint32_t)imm;
            break;
        case FN_LB:   // lb rt, immediate(rs) ; I Format
        {
            unsigned int vaddr = *p1 + imm;
            uint32_t result;
            if (!readByte(vaddr, result, true)) {
                reportRuntimeError("Invalid virtual address '%08X'\n", vaddr);
                return false;
            } else {
                *p0 = result;
            }
            break;
        }
        case FN_LBU:    // lbu rt, immediate(rs) ; I Format
        { 
            unsigned int vaddr = *p1 + imm;
            uint32_t result;
            
            if (!readByte(vaddr, result, false)) {
                reportRuntimeError("Invalid virtual address '%08X'\n", vaddr);
                return false;
            } else {
                *p0 = result;
            }
            break;
        }
        case FN_LH: // lh rt, immediate(rs) ; I Format
        {
            unsigned int vaddr = *p1 + imm;
            uint32_t result;
            if (!readHalfWord(vaddr, result, true)) {
                reportRuntimeError("Invalid virtual address '%08X'\n", vaddr);
                return false;
            } else {
                *p0 = result;
            }
            break;
        }
        case FN_ORI: // ori rt,rs,immediate ; I Format
            *p0 = *p1 | (imm & 0xFFFF);
            break;
        case FN_LHU: // lhu rt, immediate(rs) ; I Format
        {
            unsigned int vaddr = *p1 + imm;
            uint32_t result;
            if (!readHalfWord(vaddr, result, false)) {
                reportRuntimeError("Invalid virtual address '%08X'\n", vaddr);
                return false;
            } else {
                *p0 = result;
            }
            break;
        }
        case FN_XORI: // xori rt,rs,immediate ; I Format
            *p0 = *p1 ^ (imm & 0xFFFF);
            break;
        case FN_LUI: // lui rt,immediate ; I Format
            *p0 = (imm & 0xFFFF) << 16;
            break;
        case FN_LW: // lw rt, immediate(rs) ; I Format
        { 
            unsigned int vaddr = *p1 + imm;
            uint32_t result;
            
            if (!readWord(vaddr, result)) {
                reportRuntimeError("Invalid virtual address '%08X', try increasing the physical memory\n", vaddr);
                return false;
            } else {
                *p0 = result;
            }
            break;
        }
        case FN_LWC1: // lwc1 rt, immediate(rs) ; I Format
            break;
        case FN_SB:  // sb rt, immediate(rs) ; I Format
        { 
            int32_t vaddr = (int32_t)*p1 + (int32_t)imm;
            uint8_t value = (uint8_t) (*p0 & 0xFF);

            if (!writeByte(vaddr, value)) {
                reportRuntimeError("Invalid virtual address '%08X'\n", vaddr);
                return false;
            }
            break;
        }
        case FN_SH: // sh rt, immediate(rs) ; I Format
        { 
            int32_t vaddr = (int32_t)*p1 + (int32_t)imm;
            uint16_t value = (uint16_t) (*p0 & 0xFFFF);

            if (!writeHalfWord(vaddr, value)) {
                reportRuntimeError("Invalid virtual address '%08X'\n", vaddr);
                return false;
            }
            break;
        }
        case FN_SW:
        { // sw rt, immediate(rs) ; I Format
            unsigned int vaddr = *p1 + imm;
            
            if (!writeWord(vaddr, *p0)) {
                reportRuntimeError("Invalid virtual address '%08X'\n", vaddr);
                return false;
            }
            break;
        }
        case FN_SWC1: // swc1 rt, immediate(rs) ; I Format
            break;
        case FN_J: // j label
            if ((imm >= M_VIRTUAL_EXTFUNC_START_ADDR) && (imm < M_VIRTUAL_GLOBAL_START_ADDR)) {
                reportRuntimeError("Jump to native functions are not valid. Use JAL if you want to call a native function.\n");
                return false;
            } else {
                ctx->pc = imm;
            }
            break;
        case FN_JAL: // jal label
            reg[RA_INDEX] = ctx->pc;
            if ((imm >= M_VIRTUAL_EXTFUNC_START_ADDR) && (imm < M_VIRTUAL_GLOBAL_START_ADDR)) {
                return doNativeCall(imm);
            } else {
                ctx->pc = imm;
            }
            break;
        case FN_MOVE: // move rd, rt
            *p0 = *p1;
            break;
    }

    lastResult.setSim(this);
    lastResult.setRegIndex(rd);
    
    return true;
}

MIPS32Function *getFunctionByName(const char *name) 
{
    for (int i = 0; i < function_count; i++) {
        if (strcmp(name, functions[i].name) == 0) {
            return &functions[i];
        }
    }

    return NULL;
}

MIPS32Function *getFunctionByOpcode(unsigned int opcode) 
{
    for (int i = 0; i < function_count; i++) {
        if (functions[i].opcode == opcode) {
            return &functions[i];
        }
    }

    return NULL;
}
