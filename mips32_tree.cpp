#include <iostream>
#include <fstream>
#include "mips32_tree.h"
#include "util.h"

using namespace std;

MemPool *mparser_pool;

static inline int isShiftFunction(int opcode) {
    return ((opcode==FN_SLL) || (opcode==FN_SRL) || (opcode==FN_SRA));
}

void *MNode::operator new(size_t sz)
{
    return mparser_pool->memAlloc(sz);
}

void MNode::operator delete(void *ptrb)
{
    mparser_pool->memFree(ptrb);
}

bool MCmd_Show::exec(MIPS32Sim *sim)
{
	sim->last_result.refType = MRT_None;

    if (arg->isA(MARG_REGISTER)) {
        MArgRegister *rarg = (MArgRegister *)arg;
        uint32_t value;
        
        if (!sim->getRegisterValue(rarg->regName, value))
            return false;
        
        printf("%s = ", rarg->regName.c_str());
        printNumber(value, 32, dataFormat);
        printf("\n");
        
        return true;
    } else if (arg->isA(MARG_MEMREF)) {
        MArgMemRef *marg = (MArgMemRef *)arg;
        uint32_t base, value;
        
        if (!sim->getRegisterValue(marg->regName, base))
            return false;
        
        uint32_t address = marg->offset + base;
        int i ;
        
        for (i = 0; i < marg->count; i++) {
            switch (marg->sizeDirective) {
                case MSD_Byte: {
                    if (!sim->readByte(address, value, 0))
                        return false;

                    printf("byte [0x%X] ", address);
                    printNumber(value, 8, dataFormat);

                    address++;
                    
                    break;
                }
                case MSD_HWord: {
                    if (!sim->readHalfWord(address, value, false))
                        return false;

                    printf("half word [0x%X] ", address);
                    printNumber(value, 16, dataFormat);
                    
                    address += 2;

                    break;
                }
                case MSD_Word: {
                    if (!sim->readWord(address, value))
                        return false;

                    printf("word [0x%X] ", address);
                    printNumber(value, 32, dataFormat);

                    address += 4;
                    
                    break;
                }
            }
            
            if ( i < (marg->count - 1) )
                printf(", ");
        }
        
        printf("\n");
        return true;
    }
    
    return false;
}

bool MCmd_Set::exec(MIPS32Sim *sim)
{
	sim->last_result.refType = MRT_None;

    if (arg->isA(MARG_REGISTER)) {
        MArgRegister *rarg = (MArgRegister *)arg;
        uint32_t value = lvalue.front();
        
        if (!sim->setRegisterValue(rarg->regName, value))
            return false;
        
        return true;
    } else if (arg->isA(MARG_MEMREF)) {
        MArgMemRef *marg = (MArgMemRef *)arg;
        uint32_t base;
        
        if (!sim->getRegisterValue(marg->regName, base))
            return false;
        
        uint32_t address = marg->offset + base;
        ConstantList::iterator it = lvalue.begin();
        
        while (it != lvalue.end()) {
            uint32_t value = *it;
            
            switch (marg->sizeDirective) {
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
    
	sim->last_result.refType = MRT_None;

    in.open(file_path.c_str(), ifstream::in|ifstream::binary);

    if (!in.is_open()) {
        reportError("Cannot open file '%s'\n", file_path.c_str());
        return false;
    }

    bool result = true;
    
    if (!sim->exec(&in)) {
        result = false;
    }
        
    in.close();
    
    return result;
}

static inline int resolveRegister(string regName)
{
    int regIndex = mips32_getRegisterIndex(regName.c_str());
    if (regIndex == -1) {
        reportError("Invalid register name '%s'\n", regName.c_str());
        return -1;
    }
    return regIndex;
}

int MInst_1Arg::resolveArguments(uint32_t values[])
{
    MIPS32Function *f = getFunctionByName(name.c_str());
    int regIndex;
        
    switch (f->format) {
        case R_FORMAT: {
            if (!arg1->isA(MARG_REGISTER)) {
                reportError("Argument of instruction '%s' should be register\n", name.c_str());
                return -1;
            }
            regIndex = resolveRegister(((MArgRegister *)arg1)->regName);
            if (regIndex < 0)
                return -1;
            values[0] = regIndex;
            break;
        }
        case J_FORMAT: {
            if (!arg1->isA(MARG_IMMEDIATE)) {
                reportError("Argument of instruction '%s' should be an address\n", name.c_str());
                return -1;
            }
            values[0] = ((MArgConstant *)arg1)->value;
            break;
        }
    }
    
    return 0;
}

int MInst_2Arg::resolveArguments(uint32_t values[])
{
    MIPS32Function *f = getFunctionByName(name.c_str());
    int regIndex;
    
    switch (f->format) {
        case R_FORMAT: {
            if (!arg1->isA(MARG_REGISTER)) {
                reportError("First argument of instruction '%s' should be a register\n", name.c_str());
                return -1;
            }
            if (!arg2->isA(MARG_REGISTER)) {
                reportError("Second argument of instruction '%s' should be a register\n", name.c_str());
                return -1;
            }
            regIndex = resolveRegister(((MArgRegister *)arg1)->regName);
            if (regIndex < 0)
                return -1;
            values[0] = regIndex;
            
            regIndex = resolveRegister(((MArgRegister *)arg2)->regName);
            if (regIndex < 0)
                return -1;
            values[1] = regIndex;
            break;
        }
        case I_FORMAT: {
            if (!arg1->isA(MARG_REGISTER)) {
                reportError("Argument of instruction '%s' should be a register\n", name.c_str());
                return -1;
            }
            if (!arg2->isA(MARG_IMMEDIATE)) {
                reportError("Argument of instruction '%s' should be an immediate\n", name.c_str());
                return -1;
            }
            regIndex = resolveRegister(((MArgRegister *)arg1)->regName);
            if (regIndex < 0)
                return -1;
            values[0] = regIndex;
            values[1] = ((MArgConstant *)arg2)->value & 0xFFFF;

            break;
        }
    }
    
    return 0;
}

int MInst_3Arg::resolveArguments(uint32_t values[]) 
{
    MIPS32Function *f = getFunctionByName(name.c_str());
    int regIndex;
            
    switch (f->format) {
        case R_FORMAT: {
            if (!arg1->isA(MARG_REGISTER)) {
                reportError("First argument of instruction '%s' should be a register\n", name.c_str());
                return -1;
            }
            if (!arg2->isA(MARG_REGISTER)) {
                reportError("Second argument of instruction '%s' should be a register\n", name.c_str());
                return -1;
            }
            regIndex = resolveRegister(((MArgRegister *)arg1)->regName);
            if (regIndex < 0)
                return -1;
            values[0] = regIndex;
            
            regIndex = resolveRegister(((MArgRegister *)arg2)->regName);
            if (regIndex < 0)
                return -1;
            values[1] = regIndex;
            
            if (isShiftFunction(f->opcode)) {
                if (!arg3->isA(MARG_IMMEDIATE)) {
                    reportError("Argument of instruction '%s' should be an immediate\n", name.c_str());
                    return -1;
                }
                values[2] = ((MArgConstant *)arg3)->value & 0x1FF;
            } else {
                if (!arg3->isA(MARG_REGISTER)) {
                    reportError("Third argument of instruction '%s' should be a register\n", name.c_str());
                    return -1;
                }
                regIndex = resolveRegister(((MArgRegister *)arg3)->regName);
                if (regIndex < 0)
                    return -1;
                values[2] = regIndex;
            }
            break;
        }
        case I_FORMAT: {
            if (!arg1->isA(MARG_REGISTER)) {
                reportError("First argument of instruction '%s' should be a register\n", name.c_str());
                return -1;
            }
            if (!arg2->isA(MARG_REGISTER)) {
                reportError("Second argument of instruction '%s' should be a register\n", name.c_str());
                return -1;
            }
            if (!arg3->isA(MARG_IMMEDIATE)) {
                reportError("Argument of instruction '%s' should be an immediate\n", name.c_str());
                return -1;
            }
            regIndex = resolveRegister(((MArgRegister *)arg1)->regName);
            if (regIndex < 0)
                return -1;
            values[0] = regIndex;
            
            regIndex = resolveRegister(((MArgRegister *)arg2)->regName);
            if (regIndex < 0)
                return -1;
            values[1] = regIndex;
            values[2] = ((MArgConstant *)arg3)->value & 0xffff;
            
            break;
        }
    }
    
    return 0;
}