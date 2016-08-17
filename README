# EasyASM

EasyASM is a simple simulator for a subset of MIPS32 and x86 ISAs.  The main purpose is make easy the process
of learning the assembler language.  For this purpose EasyASM provides a simple command line interface were the
user can type assembler instructions and test them without the need of using some assembler program.  The program
provides instant feedback to user, in case of an error.  Apart from that the simulator provides some useful commands
which are described in the section commands.

## Installation

To install the program you need a C++ compiler and the source code.  Once you have both you just need to enter
the directory with the source code and type the command `make`.  This process has been tested on linux.  On 
windows it might work if you use **msys**

## Usage

Once you have the simulator compiled, you can run it by typing `./EasyASM` in the source directory.  By default
EasyASM starts in MIPS32 mode, if you want to start in x86 mode you have to include the flag `--x86`

## Supported commands

`#set **argument** = constant`

The **argument** can be a register or a memory reference

`#show **argument** [format]`

The **argument** can be a register or a memory reference.  

The **format** can be:

1. `unsigned decimal`
2. `signed decimal`
3. `hex` or `hexadecimal`
4. `binary`

The **format** is optinal, if no format is specified `unsigned decimal` will be used.

`#exec <assembler file>`
Executes the code in a assembler file.  The file path should be surrounded by ".

## Credits

EasyASM is mainly developed by Ivan de Jesus Deras (ideras at gmail dot com)

## License

GPLv2