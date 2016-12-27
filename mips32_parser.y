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
%type address_expr {MAddrExpr *}
%type data_format {PrintFormat}
%type opt_data_format {PrintFormat}
%type opt_size_directive {int}
%type size_directive {int}
%type constant {MArgument *}
%type set_rvalue {MArgumentList *}
%type constant_list {MArgumentList *}
%type opt_count {MArgument *}

%name Mips32Parse

%syntax_error {
    string strErr = Mips32Lexer::getTokenString(yymajor, TOKEN);
    ctx->error ++;

    if (yymajor != MTK_ERROR)
        reportError("Line %d: Syntax error. Unexpected %s\n", TOKEN->line, strErr.c_str());
    else
        reportError("Line %d: Syntax error. Invalid %s\n", TOKEN->line, strErr.c_str());
}   

%start_symbol input   

input ::= statements MTK_EOF. { /* Noting to do here */ }
        
statements ::= statements MTK_EOL statement(S). { ctx->instList.push_back((MInstruction *)S); }
statements ::= statement(S) .                   { ctx->instList.push_back((MInstruction *)S); }
        
statement(R) ::= instruction(I).  { R = I; }
statement(R) ::= command(C).      { R = C; }
statement(R)::= MTK_ID(I) MTK_COLON. { R = new MInstTagged(I->tokenLexeme, NULL); R->line = I->line; }
statement(R)::= MTK_ID(I) MTK_COLON instruction(T). 
                { R = new MInstTagged(I->tokenLexeme, (MInstruction *)T); R->line = I->line; }
statement(R)::= MTK_ID(I) MTK_COLON command(C). 
                { R = new MInstTagged(I->tokenLexeme, (MInstruction *)C); R->line = I->line; }

command(R) ::= MCKW_SHOW(I) cmd_argument(A) opt_data_format(F).  
               { R = new MCmd_Show(I->tokenLexeme, A, F); R->line = I->line; }
command(R) ::= MCKW_SET(I) cmd_argument(A) MTK_OPEQUAL set_rvalue(V).
               { R = new MCmd_Set(I->tokenLexeme, A, *V); R->line = I->line; delete V; }
command(R) ::= MCKW_EXEC(I) MSTR_LITERAL(S). 
               { R = new MCmd_Exec(I->tokenLexeme, S->tokenLexeme); R->line = I->line; }
command(R) ::= MCKW_STOP(I). 
               { R = new MCmd_Stop(); R->line = I->line; }
        
set_rvalue(R) ::= constant_arg(C). { R = new MArgumentList; R->push_back((MArgument *)C); }
set_rvalue(R) ::= MTK_LBRACKET constant_list(L) MTK_RBRACKET. { R = L; }

constant_list(R) ::= constant_list(L) MTK_COMMA constant_arg(C). { R = L; R->push_back((MArgument *)C); }
constant_list(R) ::= constant_arg(C). { R = new MArgumentList; R->push_back((MArgument *)C); }
        
cmd_argument(R) ::= argument(A).   { R = A; }
cmd_argument(R) ::= MCKW_MEM opt_size_directive(D) opt_count(T) argument(C) MTK_LPAREN argument(A) MTK_RPAREN.  
                    { R = new MArgMemRef(D, C, A, T); }
cmd_argument(R) ::= MCKW_MEM opt_size_directive(D) opt_count(T) argument(A).  
                    { R = new MArgMemRef(D, A, NULL, T); }

opt_count(R) ::= MTK_LBRACKET constant(C) MTK_RBRACKET. { R = C; }
opt_count(R) ::= . { R = new MArgConstant(1); }

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
data_format(R) ::= MCKW_ASCII.                { R = F_Ascii; }

instruction(R) ::= MTK_ID(I) argument(A1) MTK_COMMA argument(A2) MTK_COMMA argument(A3). 
                   { 
                        R = new MInst_3Arg(I->tokenLexeme, A1, A2, A3); 
			R->line = I->line; 
                    }
instruction(R) ::= MTK_ID(I) argument(A1) MTK_COMMA argument(A2).
                   { R = new MInst_2Arg(I->tokenLexeme, A1, A2); R->line = I->line; }
instruction(R) ::= MTK_ID(I) argument(A1). { R = new MInst_1Arg(I->tokenLexeme, A1); R->line = I->line; }
instruction(R) ::= MTK_ID(I) argument(A1) MTK_COMMA constant(A2) MTK_LPAREN argument(A3) MTK_RPAREN.
                   { R = new MInst_3Arg(I->tokenLexeme, A1, A3, A2); R->line = I->line; }

argument(R) ::= MTK_REGISTER(R1).   { R = new MArgRegister(R1->tokenLexeme, R1->intValue); }
argument(R) ::= constant_arg(A).    { R = (MArgument *)A; }

constant_arg(R) ::= constant(C). { R = C; }
constant_arg(R) ::= MTK_ID(I). { R = new MArgIdentifier(I->tokenLexeme); }
constant_arg(R) ::= MTK_AT MTK_ID(I1) MTK_DOT MTK_ID(I2). { R = new MArgExternalFuntionId(I1->tokenLexeme, I2->tokenLexeme); }
constant_arg(R) ::= MCKW_HIHW MTK_LPAREN constant_arg(A) MTK_RPAREN. { R = new MArgHighHalfWord((MArgument *)A); }
constant_arg(R) ::= MCKW_LOHW MTK_LPAREN constant_arg(A) MTK_RPAREN. { R = new MArgLowHalfWord((MArgument *)A); }
constant_arg(R) ::= MCKW_PADDR MTK_LPAREN address_expr(E) MTK_RPAREN. { R = new MArgPhyAddress(E); }

constant(R) ::= MTK_DEC_CONSTANT(C).  { R = new MArgConstant(NBF_UnsignedDecimal, C->intValue); }
constant(R) ::= MTK_OP_MINUS MTK_DEC_CONSTANT(C).  { R = new MArgConstant(NBF_SignedDecimal, -C->intValue); }
constant(R) ::= MTK_HEX_CONSTANT(C).  { R = new MArgConstant(NBF_Hexadecimal, C->intValue); }
constant(R) ::= MTK_BIN_CONSTANT(C).  { R = new MArgConstant(NBF_Binary, C->intValue); }
constant(R) ::= MTK_CHAR_CONSTANT(C). { R = new MArgConstant(NBF_Ascii, C->intValue); }

address_expr(R) ::= constant(C). { R = new MAddrExprConstant(C); }
address_expr(R) ::= MTK_ID(I). { R = new MAddrExprIdentifier(I->tokenLexeme); }
address_expr(R) ::= MTK_REGISTER(T) . { R = new MAddrExprRegister(T->intValue); }
address_expr(R) ::= constant(C) MTK_LPAREN MTK_REGISTER(T) MTK_RPAREN. 
                    { R = new MAddrExprBaseOffset(C, T->intValue); }