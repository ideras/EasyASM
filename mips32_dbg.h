/* 
 * File:   mips32_dbg.h
 * Author: ideras
 *
 * Created on September 12, 2016, 4:52 PM
 */

#ifndef MIPS32_DBG_H
#define MIPS32_DBG_H

#include <stdint.h>
#include <fstream>
#include <list>
#include <vector>
#include <map>
#include <set>
#include "adbg.h"

using namespace std;

class MemPool;
class MInstruction;
class MIPS32Sim;

class MIPS32Debugger: public AsmDebugger
{
public:    
    MIPS32Debugger(MIPS32Sim *sim, vector<MInstruction *> instList, MemPool *mPool) {
        this->sim = sim;
        this->instList = instList;
        this->mPool = mPool;
        inBreakpoint = false;
        finished = false;
    }

    void showStatus();
    void addBreakpoint(int line) { breakpoints.insert(line); }
    void removeBreakpoint(int line) { breakpoints.erase(line); } 
    void removeAllBreakpoints() { breakpoints.clear(); }
    bool isInBreakpoint() { return inBreakpoint; }
    bool isFinished() { return finished; }
    void setSourceLines(vector<string> sourceLines) { this->sourceLines = sourceLines; }
    void start();
    bool next();
    bool run();
    void stop();
    bool doSimCommand(string cmd);
    
private:    
    bool inBreakpoint;
    bool finished;
    vector<string> sourceLines;
    vector<MInstruction *> instList;
    set<int> breakpoints;
    MIPS32Sim *sim;
    MemPool *mPool;
};

#endif /* MIPS32_DBG_H */

