#include <iostream>
#include "mips32_dbg.h"
#include "mips32_sim.h"

using namespace std;

void MIPS32Debugger::showStatus()
{
    MRtContext *ctx = sim->runtimeCtx;
    MInstruction *inst = instList[ctx->pc];
    
    cout << inst->line << ": " <<  sourceLines[inst->line - 1] << endl;
}

void MIPS32Debugger::start()
{
    sim->runtimeCtx->pc = 0;
    sim->runtimeCtx->stop = false;
}

bool MIPS32Debugger::next()
{
    MRtContext *ctx = sim->runtimeCtx;
    MInstruction *inst = instList[ctx->pc];
    unsigned count = instList.size();
        
    ctx->line = inst->line;
    ctx->pc ++;
    
    if (!sim->execInstruction(inst)) {
        return false;
    }
        
    if (ctx->stop || (ctx->pc >= count)) {
        finished = true;
    }

    return true;
}

bool MIPS32Debugger::run()
{
    MRtContext *ctx = sim->runtimeCtx;
    unsigned count = instList.size();
        
    while (1) {
        MInstruction *inst = instList[ctx->pc];
        
        if (breakpoints.find(inst->line) != breakpoints.end()) {
            if (inBreakpoint)
                inBreakpoint = false;
            else {
                cout << "Program paused due to breakpoint at line " << inst->line << endl;
                
                inBreakpoint = true;
                return false;
            }
        }
        
        ctx->line = inst->line;
        ctx->pc ++;

        if (!sim->execInstruction(inst)) {
            return false;
        }

        if (ctx->stop || (ctx->pc >= count)) {
            finished = true;
            break;
        }
    }
    
    return true;
}

void MIPS32Debugger::stop()
{
    delete mPool; //This releases all the tree nodes
    
    delete sim->runtimeCtx;
    delete sim->jumpTable;

    sim->runtimeCtx = NULL;
    sim->jumpTable = NULL;
    sim->dbg = NULL;
    
    delete this;
}

bool MIPS32Debugger::doSimCommand(string cmd)
{
    stringstream in;
            
    in.str(cmd);

    MParserContext ctx;

    if (sim->parseFile(&in, ctx)) {
        MInstruction *inst = ctx.instList.front();

        sim->execInstruction(inst);
    } else {
        cout << "Error in command." << endl;
        return false;
    }
    
    return true;
}
