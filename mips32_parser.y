%token_type {TokenInfo *}
  
%include {
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <list>
#include <string>    
#include <sstream>
#include <assert.h>
#include "mips32_lexer.h"
#include "mips32_tree.h"    
#include "util.h"

using namespace std;

void reportError(const char *format, ...);
extern int error;
}  

//%token_destructor { printf("Releasing: %p\n", $$); }
%extra_argument {MParserContext *ctx}
%default_type {MNode *}
%type instruction {MNode *}
%type command {MNode *}
%type statement {MNode *}
%type tagged_statement {MNode *}
%type cmd_argument {MArgument *}
%type argument {MArgument *}
%type data_format {PrintFormat}
%type opt_data_format {PrintFormat}
%type opt_size_directive {int}
%type size_directive {int}
%type constant {uint32_t}
%type r_value {ConstantList *}
%type constant_list {ConstantList *}
%type opt_count {int}

%name Mips32Parse

%syntax_error {
	string strErr = Mips32Lexer::getTokenString(yymajor, TOKEN);
	ctx->error ++;
    reportError("Line %d: Syntax error. Unexpected %s\n", TOKEN->line, strErr.c_str());
}   

%start_symbol input   

input ::= statements MTK_EOF. { /* Noting to do here */ }
        
statements ::= statements MTK_EOL statement(S). { ctx->input_list.push_back((MInstruction *)S); }
statements ::= statement(S) .                   { ctx->input_list.push_back((MInstruction *)S); }
        
statement(R) ::= instruction(I).  { R = I; }
statement(R) ::= command(C).      { R = C; }
statement(R)::= MTK_ID(I) MTK_COLON. { R = new MInstTagged(I->tokenLexeme, NULL); R->line = I->line; }
statement(R)::= MTK_ID(I) MTK_COLON instruction(T). 
			   { R = new MInstTagged(I->tokenLexeme, (MInstruction *)T); R->line = I->line; }
statement(R)::= MTK_ID(I) MTK_COLON command(C). 
                { R = new MInstTagged(I->tokenLexeme, (MInstruction *)C); R->line = I->line; }

command(R) ::= MCKW_SHOW(I) cmd_argument(A) opt_data_format(F).  
			   { R = new MCmd_Show(I->tokenLexeme, A, F); R->line = I->line; }
command(R) ::= MCKW_SET(I) cmd_argument(A) MTK_OPEQUAL r_value(V).
               { R = new MCmd_Set(I->tokenLexeme, A, *V); R->line = I->line; delete V; }
command(R) ::= MCKW_EXEC(I) MSTR_LITERAL(S). 
               { R = new MCmd_Exec(I->tokenLexeme, S->tokenLexeme); R->line = I->line; }
command(R) ::= MCKW_STOP(I). 
               { R = new MCmd_Stop(); R->line = I->line; }
        
r_value(R) ::= constant(C). { R = new ConstantList; R->push_back(C); }
r_value(R) ::= MTK_LBRACKET constant_list(L) MTK_RBRACKET. { R = L; }

constant_list(R) ::= constant_list(L) MTK_COMMA constant(C). { R = L; R->push_back(C); }
constant_list(R) ::= constant(C). { R = new ConstantList; R->push_back(C); }
        
cmd_argument(R) ::= MTK_REGISTER(R1).  { R = new MArgRegister(R1->tokenLexeme); }
cmd_argument(R) ::= MCKW_MEM opt_size_directive(D) opt_count(T) constant(C) MTK_OPENP MTK_REGISTER(R1) MTK_CLOSEP.  
                    { R = new MArgMemRef(D, C, T, R1->tokenLexeme); }
cmd_argument(R) ::= MCKW_MEM opt_size_directive(D) opt_count(T) MTK_REGISTER(R1).
                    { R = new MArgMemRef(D, 0, T, R1->tokenLexeme); }
cmd_argument(R) ::= MCKW_MEM opt_size_directive(D) opt_count(T) constant(C).  
                    { R = new MArgMemRef(D, C, T, "$zero"); }

opt_count(R) ::= MTK_LBRACKET constant(C) MTK_RBRACKET. { R = C; }
opt_count(R) ::= . { R = 1; }

opt_size_directive(R) ::= size_directive (D). { R = D; }
opt_size_directive(R) ::= . { R = MSD_Word; }

size_directive(R) ::= MKW_BYTE.   { R = MSD_Byte; }
size_directive(R) ::= MKW_HWORD.  { R = MSD_HWord; }
size_directive(R) ::= MKW_WORD.   { R = MSD_Word; }

opt_data_format(R) ::= data_format(F).    { R = F; }
opt_data_format(R) ::= . { R = F_Unspecified; }

data_format(R) ::= MCKW_HEX.                  { R = F_Hexadecimal; }
data_format(R) ::= MCKW_DEC.                  { R = F_SignedDecimal; }
data_format(R) ::= MCKW_SIGNED MCKW_DEC.      { R = F_SignedDecimal; }
data_format(R) ::= MCKW_UNSIGNED MCKW_DEC.    { R = F_UnsignedDecimal; }
data_format(R) ::= MCKW_BINARY.               { R = F_Binary; }

instruction(R) ::= MTK_ID(I) argument(A1) MTK_COMMA argument(A2) MTK_COMMA argument(A3). 
                   { 
				     R = new MInst_3Arg(I->tokenLexeme, A1, A2, A3); 
				     R->line = I->line; 
				   }
instruction(R) ::= MTK_ID(I) argument(A1) MTK_COMMA argument(A2).
                   { R = new MInst_2Arg(I->tokenLexeme, A1, A2); R->line = I->line; }
instruction(R) ::= MTK_ID(I) argument(A1). { R = new MInst_1Arg(I->tokenLexeme, A1); R->line = I->line; }
instruction(R) ::= MTK_ID(I) argument(A1) MTK_COMMA constant(A2) MTK_OPENP argument(A3) MTK_CLOSEP.
                   { R = new MInst_3Arg(I->tokenLexeme, A1, A3, new MArgConstant(A2)); R->line = I->line; }

argument(R) ::= MTK_REGISTER(R1).   { R = new MArgRegister(R1->tokenLexeme); }
argument(R) ::= constant(C).        { R = new MArgConstant((unsigned int)C); }
argument(R) ::= MTK_ID(I).          { R = new MArgIdentifier(I->tokenLexeme); }

constant(R) ::= MTK_DEC_CONSTANT(C).  { R = C->intValue; }
constant(R) ::= MTK_OP_MINUS MTK_DEC_CONSTANT(C).  { R = -1*C->intValue; }
constant(R) ::= MTK_HEX_CONSTANT(C).  { R = C->intValue; }
constant(R) ::= MTK_BIN_CONSTANT(C).  { R = C->intValue; }
constant(R) ::= MTK_CHAR_CONSTANT(C). { R = C->intValue; }