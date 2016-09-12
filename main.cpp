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

const char *getRegisterName(int index);

//Report runtime errors
void reportError(const char *format, ...)
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

void debugSession()
{
    X86Debugger *dbg = xsim.getDebugger();

    char *line;
    vector<string> strList;

    dbg->start();
    while (1) {
        dbg->showStatus();
        line = readline("DBG>> ");

        if (line == NULL) continue;

        if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) {
            break;
        }

        if (!tokenizeString(line, strList)) {
            continue;
        }
        
        if (strcmp(strList[0].c_str(), "step") == 0) {
            dbg->next();
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
                int line = atoi(strList[1].c_str());
                dbg->addBreakpoint(line);
            }
        } else if (strcmp(strList[0].c_str(), "#show") == 0 ||
                   strcmp(strList[0].c_str(), "#set") == 0) {
            stringstream in;
            
            in.str(line);
            
            if (simMips32) {
                //TODO: MIPS32 debugger
            } else {
                XParserContext ctx;
                
                if (xsim.parseFile(&in, ctx)) {
                    XInstruction *inst = ctx.input_list.front();
                    XReference result;
                    
                    inst->exec(&xsim, result);
                } else {
                    cout << "Error in " << strList[0] << " command." << endl;
                }
            }  
        }
        else {
            cout << "Invalid command '" << strList[0] << "'" << endl;
        }
        add_history(line);
        free(line);

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

        if (result.refType != MRT_None) {
            uint32_t value = msim.reg[result.address];

            printf("%s = 0x%X %d %u\n", MIPS32Sim::getRegisterName(result.address), value, (int32_t)value, value);
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
                    if (xsim.debug(asmfile)) {
                        debugSession();
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
            sprintf(buffer, "%d: -> ", line_count);
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
