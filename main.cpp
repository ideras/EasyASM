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
#include "mips32_sim.h"
#include "mips32_parser.h"

bool simMips32 = true;
MIPS32Sim msim;
X86Sim xsim;

using namespace std;

const char *getRegisterName(int index);

void reportError(const char *format, ...)
{
    va_list args;

    if (simMips32) {
		if (msim.runtime_context != NULL)
            printf("Line %d: ", msim.getSourceLine());
    } else {
        if (xsim.runtime_context != NULL)
            printf("Line %d: ", xsim.getSourceLine());
    }

    va_start(args, format);
    vprintf(format, args);
    va_end(args);
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
        msim.exec(&in);
    } else {
        xsim.exec(&in);
    }
}

int main(int argc, char *argv[])
{
    const char *prompt1 = "Inst> ";
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
    } else {
        cout << "--- EasyASM x86 mode (little endian) ----" << endl << endl;
        cout << "Global base address = 0x" << hex << X_VIRTUAL_GLOBAL_START_ADDR << dec << endl;
        cout << "Stack pointer address = 0x" << hex << X_VIRTUAL_STACK_END_ADDR << dec << endl;
        cout << "Global memory size = " << X_GLOBAL_MEM_WORD_COUNT << " words" << endl;
    }

    prompt = prompt1;
	line_count = 1;
    while (1) {
        line = readline(prompt);

        if (line != NULL && *line == 0) {
            free(line);
            line = NULL;
        }

        if (line == NULL) {
            if (lines.size() == 0) {
                continue;
            } else {
                processLines(lines);
				line_count = 1;
            }
        }

        if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) {
            break;
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
            add_history(line);

            lines.push_back(string(line));
            prompt = prompt1;
            processLines(lines);
            lines.clear();
			line_count = 1;
        }
    }

    printf("\nExiting ...\n");

    return 0;
}
