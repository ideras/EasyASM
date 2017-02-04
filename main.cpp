#include <cstdio>
#include <cstdlib>		/* for free() */
#include <cstring>
#include <cstdarg>
#include <editline/readline.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <string>
#include "x86_lexer.h"
#include "x86_tree.h"
#include "x86_sim.h"
#include "x86_dbg.h"
#include "mips32_sim.h"
#include "mips32_parser.h"

bool simMips32 = true;
MIPS32Sim msim;
X86Sim xsim;

using namespace std;

#if defined(_MSC_VER)
#define snprintf _snprintf
#endif

//Report runtime errors
void reportRuntimeError(const char *format, ...)
{
    va_list args;

    if (simMips32) {
        printf("Line %d: ", msim.getSourceLine());
    } else {
        printf("Line %d: ", xsim.getSourceLine());
    }

    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void reportError(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void debugSession()
{
    AsmDebugger *dbg;

    char *line;
    string last_command, curr_command;
    vector<string> strList;

    dbg = (simMips32)? msim.getDebugger() : xsim.getDebugger();
    
    last_command = "";
    dbg->start();
    while (1) {
        dbg->showStatus();
        line = readline("DBG>> ");

        if (line == NULL) continue;

        if (*line == '\0') {
            //Repeat last command
            if (last_command.empty()) continue;
         
            curr_command = last_command;
        } else {
            curr_command = line;
            
            if (curr_command.compare(last_command) != 0) {
                add_history(line);
                free(line);
            }
            
            last_command = curr_command;
        }
        
        if (!tokenizeString(curr_command, strList)) {
            continue;
        }
        
        if (strcmp(strList[0].c_str(), "step") == 0) {
            if (!dbg->next()) {
                cout << "Debug session terminated due to errors in the program." << endl;
                dbg->stop();
                return;
            }
        } else if (strcmp(strList[0].c_str(), "run") == 0) {
            dbg->run();
        } else if (strcmp(strList[0].c_str(), "stop") == 0) {
            cout << "Debug session terminated by user command.\n" << endl;
            dbg->stop();
            return;
        } else if (strcmp(strList[0].c_str(), "breakpoint") == 0) {
            if (strList.size() != 2) {
                cout << "Invalid usage in 'breakpoint' command. Usage breakpoint <line number>." << endl;
            } else {
                int lineNumber = atoi(strList[1].c_str());
                dbg->addBreakpoint(lineNumber);
                cout << "Breakpoint set at line " << lineNumber << endl;
            }
        } else if (strcmp(strList[0].c_str(), "#show") == 0 ||
                   strcmp(strList[0].c_str(), "#set") == 0) {
            dbg->doSimCommand(curr_command);
        }
        else {
            cout << "Invalid command '" << strList[0] << "'" << endl;
        }
        
        if (dbg->isFinished()) {
            cout << "Debug session finished.\n" << endl;
            dbg->stop();
            return;
        }
    }
}

void processLines(list<string> &lines)
{
    list<string>::iterator it = lines.begin();
    stringstream in;

    while (it != lines.end()) {
        in << *it << endl;

        it++;
    }

    if (simMips32) {
        if (!msim.exec(&in))
            return;

        MReference result = msim.getLastResult();

        if (result.isReg()) {
            uint32_t value;
            
            result.deref(value);

            printf("%s = 0x%X %d %u\n", mips32_getRegisterName(result.getRegIndex()).c_str(), value, (int32_t)value, value);
        }
    } else {

        if (!xsim.exec(&in))
            return;
       
        if (lines.size() == 1) {
            XReference result;
            uint32_t value, svalue;
            
            result = xsim.getLastResult();
            switch (result.type) {
                case RT_Reg: {
                    int regId = result.address;

                    xsim.getRegValue(regId, value);
                    svalue = (result.bitSize < BS_32)? signExtend(value, result.bitSize, BS_32) : value;

                    printf("%s = 0x%X %d %u\n", xreg[regId], value, (int)svalue, value);
                    break;
                }
                case RT_Mem: {
                    uint32_t value;

                    xsim.readMem(result.address, value, result.bitSize);
                    svalue = signExtend(value, result.bitSize, BS_32);

                    printf("%s [0x%X] = 0x%X %d %u\n", X86Sim::sizeDirectiveToString(result.bitSize), result.address, value, (int)svalue, value);
                    break;
                }

                default:
                    break;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    const char *prompt1 = "ASM> ";
    const char *prompt;
    char *line;
    list<string> lines;
    int line_count;
    char buffer[16];

    if (argc > 1) {
        ++argv, --argc; /* The first argument is the program name */
        if (strcmp(argv[0], "--mips32") == 0)
            simMips32 = true;
        else if (strcmp(argv[0], "--x86") == 0)
            simMips32 = false;
        else {
            cerr << "Invalid option '" << argv[0] << "'" << endl;
            exit(1);
        }
    }

    if (simMips32) {
        cout << "--- EasyASM MIPS32 mode (big endian) ----" << endl << endl;
        cout << "Global base address = 0x" << hex << M_VIRTUAL_GLOBAL_START_ADDR << dec << endl;
        cout << "Stack pointer address = 0x" << hex << M_VIRTUAL_STACK_END_ADDR << dec << endl;
        cout << "Global memory size = " << M_GLOBAL_MEM_WORD_COUNT << " words" << endl;
        cout << "Stack size         = " << M_STACK_SIZE_WORDS << " words" << endl << endl;
    } else {
        cout << "--- EasyASM x86 mode (little endian) ----" << endl << endl;
        cout << "Global base address = 0x" << hex << X_VIRTUAL_GLOBAL_START_ADDR << dec << endl;
        cout << "Stack pointer address = 0x" << hex << X_VIRTUAL_STACK_END_ADDR << dec << endl;
        cout << "Global memory size = " << X_GLOBAL_MEM_WORD_COUNT << " dwords" << endl;
        cout << "Stack size         = " << X_STACK_SIZE_WORDS << " dwords" << endl << endl;
    }

    prompt = prompt1;
    line_count = 1;
    while (1) {
        line = readline(prompt);

        if (line == NULL) continue;

        if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) {
            break;
        }
        
        if (strncmp(line, "#debug", 6) == 0) {
            vector<string> strList;
            
            add_history(line);
            
            if (tokenizeString(line, strList)) {
                if (strList.size() != 2) {
                    cout << "Invalid usage of #debug command. Usage: #debug \"<assembler file>\"." << endl;
                    continue;
                } else {
                    string asmfile = strList[1];
                    
                    if (simMips32) {
                        if (msim.debug(asmfile)) {
                            debugSession();
                        }
                    } else {
                        if (xsim.debug(asmfile)) {
                            debugSession();
                        }
                    }
                }
            }
            
            free(line);
            continue;
        }

        int len = strlen(line);
        if (line[len - 1] == ';') {
            add_history(line);

            line[len - 1] = '\0';
            lines.push_back(string(line));
            free(line);
            line_count++;
            snprintf(buffer, 16, "%d: -> ", line_count);
            prompt = buffer;
            continue;
        } else {
            if (*line != 0) {
                add_history(line);
                lines.push_back(string(line));
            }
            free(line);

            if (lines.size() > 0) {
                prompt = prompt1;
                processLines(lines);
                lines.clear();
                line_count = 1;
            }
        }
    }

    printf("\nExiting ...\n");

    return 0;
}
