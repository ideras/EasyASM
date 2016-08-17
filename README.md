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

### `#set argument = constant`

The **argument** can be a register or a memory reference.  
The **constant** can be any 32 bits constant in base 2, 10 or 16.  For base use the prefix **0b** and for base 16 **0x**

#### Example MIPS32

```
#set $s0 = 0x10008000
#set memory 0($s0) = 0xdeadbeef
#set memory 0x10008000 = 0x00fe0fea
```

#### Example x86

```
#set eax = 0x10000000
#set dword [eax] = 0xdeadbeef
#set memory [eax + 4] = 0x00fe0fea
```

### `#show argument [format]`

The **argument** can be a register, constant or a memory reference.  

The **format** can be:

1. `unsigned decimal`
2. `signed decimal`
3. `hex` or `hexadecimal`
4. `binary`

#### Example MIPS32

```
#show $s0
#show $s0 hex
#show memory 0($s0) signed decimal
#show memory 0x10008000
#show -1 binary
```

#### Example x86

```
#show eax
#show dword [eax] hexadecimal
#show dword [eax + 4] signed decimal
#show -1 binary
```

The **format** is optinal, if no format is specified `unsigned decimal` will be used.

### `#exec <assembler file>`
Executes the code in a assembler file.  The file path should be surrounded by ".

#### Example MIPS32 and x86

```
#exec "factorial.asm"
```

## Credits

EasyASM is mainly developed by Ivan de Jesus Deras (ideras at gmail dot com)

## License

GPLv2