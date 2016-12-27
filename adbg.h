/* 
 * File:   adbg.h
 * Author: ideras
 *
 * Created on September 12, 2016, 4:41 PM
 */

#ifndef ADBG_H
#define ADBG_H

#include <string>
#include <vector>

using namespace std;

class AsmDebugger
{
public:    
    virtual void showStatus() = 0;
    virtual void addBreakpoint(int line) = 0;
    virtual void removeBreakpoint(int line) = 0;
    virtual void removeAllBreakpoints() = 0;
    virtual bool isInBreakpoint() = 0;
    virtual bool isFinished() = 0;
    virtual void setSourceLines(vector<string> sourceLines) = 0;
    virtual void start() = 0;
    virtual bool next() = 0;
    virtual bool run() = 0;
    virtual void stop() = 0;
    virtual bool doSimCommand(string cmd) = 0;
};


#endif /* ADBG_H */

