#include <iostream>
#include <fstream>
#include <map>
#include "native_lib.h"
#include "mips32_tree.h"
#include "util.h"

using namespace std;

MemPool *mpool;

map<string, uint32_t> externalFunctions;
map<uint32_t, void *> externalFuncHandles;
uint32_t externalFuncAddr = 0x01400000;

static inline int isShiftInstruction(int opcode) {
    return ((opcode==FN_SLL) || (opcode==FN_SRL) || (opcode==FN_SRA));
}

static inline int isBranchInstruction(int opcode) {
    return ((opcode==FN_BEQ) || (opcode==FN_BNE));
}

/*** MReference methods implementation ***/

bool MReference::deref(uint32_t &value)
{
    switch (mrt) {
        case MRT_Const: {
            value = u.constValue;
            return true;
        }
        case MRT_Reg: {
            if (u.regIndex <= 31) {
                value = sim->reg[u.regIndex];
                return true;
            } else {
                return false;
            }
        }
        case MRT_Mem: {
            switch (u.mem.wordSize) {
                case  MSD_Byte: return sim->readByte(u.mem.vaddr, value, false);
                case MSD_HWord: return sim->readHalfWord(u.mem.vaddr, value, false);
                case  MSD_Word: return sim->readWord(u.mem.vaddr, value);
                default:
                    return false;
            }
        }
    }

    return false;
}

bool MReference::assign(uint32_t &value)
{
    switch (mrt) {
        case MRT_Const: return false;
        
        case MRT_Reg: {
            if (u.regIndex <= 31) {
                sim->reg[u.regIndex] = value;
                return true;
            } else {
                return false;
            }
        }

        case MRT_Mem: {
            switch (u.mem.wordSize) {
                case  MSD_Byte: return sim->writeByte(u.mem.vaddr, value & 0xFF);
                case MSD_HWord: return sim->writeHalfWord(u.mem.vaddr, value & 0xFFFF);
                case  MSD_Word: return sim->writeWord(u.mem.vaddr, value);
                default:
                    return false;
            }
        }
    }
    
    return false;
}

/*** MAddrExpr methods implementation ***/
string MAddrExprConstant::toString() 
{
    return argValue->toString();
}

string MAddrExprIdentifier::toString()
{
    return ident;
}

string MAddrExprRegister::toString()
{
    return mips32_getRegisterName(regIndex);
}

string MAddrExprBaseOffset::toString()
{
    stringstream ss;
        
    ss << argOffset->toString() << "(" << mips32_getRegisterName(baseRegIndex) << ")";
    return ss.str();
}

bool MAddrExprConstant::eval(MIPS32Sim *sim, uint32_t &value) {
    MReference aref;
    
    if (!argValue->getReference(sim, aref))
        return false;

    if (!aref.isConst()) {
        reportRuntimeError("Argument '%s' is not valid in memory address expression.\n", argValue->toString().c_str());
        return false;
    }

    value = aref.getConstValue();
    return true;
}

bool MAddrExprIdentifier::eval(MIPS32Sim *sim, uint32_t &value) { 
    return sim->getLabel(ident, value);
}

bool MAddrExprRegister::eval(MIPS32Sim *sim, uint32_t &value) {
    if (regIndex <= 31) {
        value = sim->reg[regIndex];
        return true;
    }

    return false;
}

bool MAddrExprBaseOffset::eval(MIPS32Sim *sim, uint32_t &value) {
    MReference refOffset;
    
    if (!argOffset->getReference(sim, refOffset))
        return false;

    if (!refOffset.isConst()) {
        reportRuntimeError("Expected a constant value as offset in memory address expression. Found '%s'.\n", 
                           argOffset->toString().c_str());
        return false;
    }
        
    if (baseRegIndex <= 31) {
        value = sim->reg[baseRegIndex] + refOffset.getConstValue();
        return true;
    }

    return false;
}
    
/*** MArgument methods implementation ***/
bool MArgRegister::getReference(MIPS32Sim *sim, MReference &ref) {
    ref.setSim(sim);
    ref.setRegIndex(regIndex);
    return true;
}

bool MArgMemRef::getReference(MIPS32Sim *sim, MReference &ref) {
    MReference ref1, ref2;
    
    ref.setSim(sim);
    
    if (!arg1->getReference(sim, ref1))
        return false;
    
    if (arg2 != NULL) {
        if (!arg2->getReference(sim, ref2))
            return false;
    } else {
        ref2.init();
    }
    
    uint32_t offset, regValue;
    
    if (ref2.isNull()) {
        if (ref1.isConst()) {
            offset = ref1.getConstValue();
            regValue = 0;
        } else if (ref1.isReg()) {
            offset = 0;
            if (!ref1.deref(regValue))
                return false;
        } else {
            reportRuntimeError("Invalid argument '%s' in memory address expression. Expected constant or register\n",
                               arg1->toString().c_str());
            return false;
        }
    } else {
        if (!ref1.isConst()) {
            reportRuntimeError("Invalid argument '%s' in memory address expression. Expected a constant value.\n",
                               arg1->toString().c_str());
            return false;        
        }
        if (!ref2.isReg()) {
            reportRuntimeError("Invalid argument '%s' in memory address expression. Expected a register.\n",
                               arg2->toString().c_str());
            return false;        
        }
        
        offset = ref1.getConstValue();

        if (!ref2.deref(regValue))
            return false;
    }
    
    uint32_t vaddr = regValue + offset;

    ref.setMemRef(sizeDirective, vaddr);

    return true;
}

bool MArgIdentifier::getReference(MIPS32Sim *sim, MReference &ref) {
    uint32_t target;

    if (!sim->getLabel(name, target)) {
        reportRuntimeError("Invalid label name '%s'.", name.c_str());
        return false;
    }

    ref.setSim(sim);
    ref.setConstValue(target);

    return true;
}

bool MArgConstant::getReference(MIPS32Sim *sim, MReference &ref) {
    ref.setSim(sim);
    ref.setConstValue(value);

    return true;
}
 
bool MArgHighHalfWord::getReference(MIPS32Sim *sim, MReference &ref) {
    MReference a_ref;

    if (!arg->getReference(sim, a_ref))
        return false;

    if (!a_ref.isConst()) {
        reportRuntimeError("Argument of #hihw function should be a constant value or label. Found '%s'.\n",
                            arg->toString().c_str());
        return false;
    }

    ref.setSim(sim);
    ref.setConstValue((a_ref.getConstValue() & 0xFFFF0000) >> 16);
    
    return true;
}
    
bool MArgLowHalfWord::getReference(MIPS32Sim *sim, MReference &ref) {
    MReference a_ref;

    if (!arg->getReference(sim, a_ref))
        return false;

    if (!a_ref.isConst()) {
        reportRuntimeError("Argument of #lohw function should be a constant value or label. Found '%s'.\n",
                            arg->toString().c_str());
        return false;
    }

    ref.setSim(sim);
    ref.setConstValue(a_ref.getConstValue() & 0x0000FFFF);
    
    return true;
}

bool MArgPhyAddress::getReference(MIPS32Sim *sim, MReference &ref) {
    uint32_t vaddr, paddr;

    if (!expr->eval(sim, vaddr))
        return false;

    if (!sim->translateVirtualToPhysical(vaddr, paddr))
        return false;

    ref.setSim(sim);
    ref.setConstValue((uint32_t)&sim->mem[paddr / 4]);
    
    return true;
}

bool MArgExternalFuntionId::getReference(MIPS32Sim *sim, MReference &ref)
{
    string funcName = toString();
    map<string, uint32_t>::iterator it = externalFunctions.find(funcName);
    
    ref.setSim(sim);
    
    if (it != externalFunctions.end()) {
        ref.setConstValue(it->second);
    } else {
        string libFullName = getLibFullName(name1);
        HLIB lhandle = openLibrary(libFullName.c_str());
        
        if (lhandle == NULL) {
            reportRuntimeError("Cannot open library '%s'.\n", libFullName.c_str());
            return false;
        }
                
        HFUNC hfunc = getFunctionAddr(lhandle, name2.c_str());
        
        if (hfunc == NULL) {
            reportRuntimeError("Function '%s' doesn't exist in library '%s'.\n", name2.c_str(), libFullName.c_str());
            return false;
        }
        ref.setConstValue(externalFuncAddr);
        externalFunctions[funcName] = externalFuncAddr;
        externalFuncHandles[externalFuncAddr] = hfunc;
        externalFuncAddr += 4;
    }

    return true;
}
    
/*** MNode operator new and delete overloading ***/
void *MNode::operator new(size_t sz)
{
    return mpool->memAlloc(sz);
}

void MNode::operator delete(void *ptrb)
{
    mpool->memFree(ptrb);
}

bool MCmd_Show::exec(MIPS32Sim *sim)
{
    MReference aref;
    
    sim->lastResult.init();

    if (!arg->getReference(sim, aref))
        return false;
    
    switch (arg->getKind()) {
        case MARG_REGISTER: {
            uint32_t value;
        
            if (!aref.deref(value))
                return false;

            cout << ((MArgRegister *)arg)->regName << " = ";
            printNumber(value, 32, dataFormat);
            cout << endl;

            return true;
        }
        case MARG_MEMREF: {
            MReference cref;
            MArgMemRef *marg = (MArgMemRef *)arg;
            int wordSize = aref.getMemWordSize();
            uint32_t address = aref.getMemVAddr();
            uint32_t value;
            unsigned i ;
            
            if (!marg->argCount->getReference(sim, cref))
                return false;
            
            if (!cref.isConst()) {
                reportRuntimeError("Count argument in memory reference should be a constant value. '%s' is not valid.\n",
                                    marg->argCount->toString().c_str());
                
                return false;
            }

            for (i = 0; i < cref.getConstValue(); i++) {
                switch (wordSize) {
                    case MSD_Byte: {
                        if (!sim->readByte(address, value, 0))
                            return false;

                        printf("byte (0x%X) ", address);
                        printNumber(value, 8, dataFormat);

                        address++;

                        break;
                    }
                    case MSD_HWord: {
                        if (!sim->readHalfWord(address, value, false))
                            return false;

                        printf("half word (0x%X) ", address);
                        printNumber(value, 16, dataFormat);

                        address += 2;

                        break;
                    }
                    case MSD_Word: {
                        if (!sim->readWord(address, value))
                            return false;

                        printf("word (0x%X) ", address);
                        printNumber(value, 32, dataFormat);

                        address += 4;

                        break;
                    }
                }

                cout << endl;
            } 

            return true;
        }
        case MARG_IMMEDIATE: {
            printNumber(aref.getConstValue(), 32, dataFormat);
            cout << endl;
            break;
        }
        case MARG_IDENTIFIER: {
            string ident = ((MArgIdentifier *)arg)->name;
        
            cout << "Label '" << ident << "' points to 0x" << hex << aref.getConstValue() << dec << endl;
            break;
        }
        default: {
            if (aref.isConst()) {
                cout << arg->toString() << " = ";
                printNumber(aref.getConstValue(), 32, dataFormat);
                cout << endl;
            } else {
                reportRuntimeError("Invalid argument '%s' for #show command.\n", arg->toString().c_str());
                return false;
            }
        }
    }
    
    return true;
}

bool MCmd_Set::exec(MIPS32Sim *sim)
{
    sim->lastResult.init();

    if (arg->isA(MARG_REGISTER)) {
        MArgRegister *rarg = (MArgRegister *)arg;
        MArgument *arg = lvalue.front();
        MReference aref;

        if (!arg->getReference(sim, aref))
            return false;

        if (!aref.isConst()) {
            reportRuntimeError("Invalid argument '%s' in #set command.\n", arg->toString().c_str());
            return false;
        }

        if (!sim->setRegisterValue(rarg->regName, aref.getConstValue()))
            return false;
        
        return true;
    } else if (arg->isA(MARG_MEMREF)) {
        MReference mref;
        MArgMemRef *marg = (MArgMemRef *)arg;
        
        if (!arg->getReference(sim, mref))
            return false;
        
        uint32_t address = mref.getMemVAddr();
        int wordSize = mref.getMemWordSize();
        MArgumentList::iterator it = lvalue.begin();
        
        while (it != lvalue.end()) {
            MArgument *arg = *it;
            MReference aref;
            
            if (!arg->getReference(sim, aref))
                return false;

            if (!aref.isConst()) {
                reportRuntimeError("Invalid argument '%s' in #set command.\n", arg->toString().c_str());
                return false;
            }
            
            uint32_t value = aref.getConstValue();
            
            switch (wordSize) {
                case MSD_Byte: {
                    if (!sim->writeByte(address, (uint8_t)(value & 0xFF)))
                        return false;   
                    
                    address++;
                    
                    break;
                }
                case MSD_HWord: {
                    if (!sim->writeHalfWord(address, (uint16_t)(value & 0xFFFF)))
                        return false;  
                    
                    address += 2;
                    
                    break;
                }
                case MSD_Word: {
                    if (!sim->writeWord(address, value))
                        return false;       
                    
                    address += 4;
                    
                    break;
                }
            }
            
            it++;
        }

        return true;
    }
    
    return false;
}

bool MCmd_Exec::exec(MIPS32Sim *sim)
{
    ifstream in;
    
    in.open(file_path.c_str(), ifstream::in|ifstream::binary);

    if (!in.is_open()) {
        reportRuntimeError("Cannot open file '%s'\n", file_path.c_str());
        return false;
    }

    bool result = true;
    
    if (!sim->exec(&in)) {
        result = false;
    }
    
    sim->lastResult.init();
    in.close();
    
    return result;
}

bool MInst_1Arg::resolveArguments(MIPS32Sim *sim, uint32_t values[])
{
    MIPS32Function *f = getFunctionByName(name.c_str());
    MReference a_ref;
    
    if (!arg1->getReference(sim, a_ref))
        return false;

    switch (f->format) {
        case R_FORMAT: {
            if (!a_ref.isReg()) {
                reportRuntimeError("Argument of instruction '%s' should be register. \n", name.c_str());
                return false;
            }
            values[0] = a_ref.getRegIndex();
            break;
        }
        case J_FORMAT: {
            if (!arg1->isA(MARG_IDENTIFIER) &&
                !arg1->isA(MARG_EXTFUNC_NAME)) {
                reportRuntimeError("Argument of instruction '%s' should be a label or external function name. Found '%s'\n", name.c_str(), arg1->toString().c_str());
                return false;
            }
            values[0] = a_ref.getConstValue();
            break;
        }
    }
    
    return true;
}

bool MInst_2Arg::resolveArguments(MIPS32Sim *sim, uint32_t values[])
{
    MIPS32Function *f = getFunctionByName(name.c_str());
    MReference a_ref1, a_ref2;
    
    if (!arg1->getReference(sim, a_ref1))
        return false;
    
    if (!arg2->getReference(sim, a_ref2))
        return false;
    
    switch (f->format) {
        case R_FORMAT: {
            if (!arg1->isA(MARG_REGISTER)) {
                reportRuntimeError("First argument of instruction '%s' should be a register.\n", name.c_str());
                return false;
            }
            if (!arg2->isA(MARG_REGISTER)) {
                reportRuntimeError("Second argument of instruction '%s' should be a register.\n", name.c_str());
                return false;
            }
            values[0] = a_ref1.getRegIndex();
            values[1] = a_ref2.getRegIndex();
            break;
        }
        case I_FORMAT: {
            if (!arg1->isA(MARG_REGISTER)) {
                reportRuntimeError("Argument of instruction '%s' should be a register\n", name.c_str());
                return false;
            }
            if (!a_ref2.isConst()) {
                reportRuntimeError("Argument of instruction '%s' should be a constant value.\n", name.c_str());
                return false;
            }
            values[0] = a_ref1.getRegIndex();
            values[1] = a_ref2.getConstValue() & 0x0000FFFF;
            break;
        }
    }
    
    return true;
}

bool MInst_3Arg::resolveArguments(MIPS32Sim *sim, uint32_t values[]) 
{
    MIPS32Function *f = getFunctionByName(name.c_str());
    MReference a_ref1, a_ref2, a_ref3;
    
    if (!arg1->getReference(sim, a_ref1))
        return false;
    
    if (!arg2->getReference(sim, a_ref2))
        return false;

    if (!arg3->getReference(sim, a_ref3))
        return false;

    switch (f->format) {
        case R_FORMAT: {
            if (!arg1->isA(MARG_REGISTER)) {
                reportRuntimeError("First argument of instruction '%s' should be a register\n", name.c_str());
                return false;
            }
            if (!arg2->isA(MARG_REGISTER)) {
                reportRuntimeError("Second argument of instruction '%s' should be a register\n", name.c_str());
                return false;
            }
            values[0] = a_ref1.getRegIndex();
            values[1] = a_ref2.getRegIndex();
            
            if (isShiftInstruction(f->opcode)) {
                if (!arg3->isA(MARG_IMMEDIATE)) {
                    reportRuntimeError("Argument of instruction '%s' should be an immediate\n", name.c_str());
                    return false;
                }
                values[2] = a_ref3.getConstValue() & 0x1F;
            } else {
                if (!arg3->isA(MARG_REGISTER)) {
                    reportRuntimeError("Third argument of instruction '%s' should be a register\n", name.c_str());
                    return false;
                }
                values[2] = a_ref3.getRegIndex();
            }
            break;
        }
        case I_FORMAT: {
            if (!arg1->isA(MARG_REGISTER)) {
                reportRuntimeError("First argument of instruction '%s' should be a register\n", name.c_str());
                return false;
            }
            if (!arg2->isA(MARG_REGISTER)) {
                reportRuntimeError("Second argument of instruction '%s' should be a register\n", name.c_str());
                return false;
            }
            if (isBranchInstruction(f->opcode) && arg3->isA(MARG_EXTFUNC_NAME)) {
                reportRuntimeError("Jump to native functions are not valid. Use JAL if you want to call a native function.\n");
                return false;
            }
            if (!a_ref3.isConst()) {
                reportRuntimeError("Argument of instruction '%s' should be a constant value or label\n", name.c_str());
                return false;
            }
            values[0] = a_ref1.getRegIndex();
            values[1] = a_ref2.getRegIndex();
            values[2] = a_ref3.getConstValue() & 0x0000ffff;           
            
            break;
        }
    }
    
    return true;
}