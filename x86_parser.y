%token_type {TokenInfo *}
  
%include {
#include <cstdlib>
#include <iostream>
#include <list>
#include <sstream>
#include <assert.h>
#include "x86_lexer.h"
#include "x86_tree.h"
#include "util.h"

extern XNodeList xstatements;

using namespace std;

void reportError(const char *format, ...);
}  

%default_type {XNode *}
%extra_argument {XParserContext *ctx}
%type inst {XNode *}
%type command {XNode *}
%type cmd_argument {XArgument *}
%type argument {XArgument *}
%type register {XArgument *}
%type register8 {XArgument *}
%type register16 {XArgument *}
%type register32 {XArgument *}
%type memory {XArgument *}
%type opt_argument {XArgument *}
%type call_argument {XArgument *}
%type address_ref  {XAddrExpr *}
%type address_expr {XAddrExpr *}
%type address_term {XAddrExpr *}
%type address_factor {XAddrExpr *}
%type expr_op      {int}
%type statement {XNode *}
%type data_format {PrintFormat}
%type opt_data_format {PrintFormat}
%type size_directive {int}
%type opt_size_directive {int}
%type constant {uint32_t}
%type r_value {list<int> *}
%type constant_list {list<int> *}
%type opt_count {int}
%type tag {string *}
   
%syntax_error {
    string strErr = X86Lexer::getTokenString(yymajor, TOKEN);
    ctx->error++;
    printf("Line %d: Syntax error. Unexpected %s\n", TOKEN->line, strErr.c_str());
}   

%start_symbol input   

input ::= statements XTK_EOF. { /* Noting to do here */ }

statements ::= statements XTK_EOL statement(S). { ctx->input_list.push_back((XInstruction *)S); }
statements ::= statement(S) .                   { ctx->input_list.push_back((XInstruction *)S); }

statement(R) ::= instruction(I).  { R = I; }
statement(R) ::= command(C).     { R = C; }
statement(R)::= tag(L) XTK_COLON(C). { R = new XInstTagged(*L, NULL); R->line = C->line; delete L; }
statement(R)::= tag(L) XTK_COLON(C) instruction(T). { R = new XInstTagged(*L, (XInstruction *)T); R->line = C->line; delete L; }
statement(R)::= tag(L) XTK_COLON(C) command(CMD). { R = new XInstTagged(*L, (XInstruction *)CMD); R->line = C->line; delete L; }

tag(R) ::= XTK_ID(I). { R = new string(I->tokenLexeme); }
tag(R) ::= XTK_DOT XTK_ID(I). { R = new string("." + I->tokenLexeme); }

command(R) ::= XCKW_SET(N) cmd_argument(A) XTK_OP_EQUAL r_value(V).  { R = new XCmdSet(A, *V); delete V; R->line = N->line; }
command(R) ::= XCKW_SHOW(N) cmd_argument(A) opt_data_format(F).  { R = new XCmdShow(A, F); R->line = N->line; }
command(R) ::= XCKW_EXEC(N) XSTR_LITERAL(S). { R = new XCmdExec(S->tokenLexeme); R->line = N->line; }
command(R) ::= XCKW_DEBUG(N) XSTR_LITERAL(S). { R = new XCmdDebug(S->tokenLexeme); R->line = N->line; }
command(R) ::= XCKW_STOP(N). { R = new XCmdStop(); R->line = N->line; }

r_value(R) ::= constant(C). { R = new list<int>; R->push_back(C); }
r_value(R) ::= XTK_LBRACKET constant_list(L) XTK_RBRACKET. { R = L; }

constant_list(R) ::= constant_list(L) XTK_COMMA constant(C). { R = L; R->push_back(C); }
constant_list(R) ::= constant(C). { R = new list<int>; R->push_back(C); }

cmd_argument(R) ::= register(R1). { R = R1; }
cmd_argument(R) ::= memory(M) opt_count(C).    { R = M; ((XArgMemRef *)R)->count = C; }
cmd_argument(R) ::= constant(C).  { R = new XArgConstant((uint32_t)C); }
cmd_argument(R) ::= TK_EFLAGS.    { R = new XArgRegister(BS_32, R_EFLAGS); }

opt_count(R) ::= XTK_LBRACKET constant(C) XTK_RBRACKET. { R = C; }
opt_count(R) ::= . { R = 1; }

opt_data_format(R) ::= data_format(F).    { R = F; }
opt_data_format(R) ::= . { R = F_Unspecified; }

data_format(R) ::= XCKW_HEX.                  { R = F_Hexadecimal; }
data_format(R) ::= fmt_decimal.               { R = F_SignedDecimal; }
data_format(R) ::= XCKW_SIGNED fmt_decimal.   { R = F_SignedDecimal; }
data_format(R) ::= XCKW_UNSIGNED fmt_decimal. { R = F_UnsignedDecimal; }
data_format(R) ::= XCKW_BIN.                  { R = F_Binary; }
data_format(R) ::= XCKW_OCT.                  { R = F_Octal; }
data_format(R) ::= XCKW_ASCII.                { R = F_Ascii; }

fmt_decimal ::= XCKW_DEC.
fmt_decimal ::= XKW_DEC.

instruction(R) ::= XKW_MOV(N) argument(A1) XTK_COMMA argument(A2).     { R = new XI_Mov(A1, A2); R->line = N->line; }
instruction(R) ::= XKW_MOVSX(N) argument(A1) XTK_COMMA argument(A2).   { R = new XI_Movsx(A1, A2); R->line = N->line; }
instruction(R) ::= XKW_MOVZX(N) argument(A1) XTK_COMMA argument(A2).   { R = new XI_Movzx(A1, A2); R->line = N->line; }
instruction(R) ::= XKW_PUSH(N) argument(A1).                           { R = new XI_Push(A1); R->line = N->line; }
instruction(R) ::= XKW_POP(N) argument(A1).                            { R = new XI_Pop(A1); R->line = N->line; }
instruction(R) ::= XKW_LEA(N) argument(A1) XTK_COMMA argument(A2).     { R = new XI_Lea(A1, A2); R->line = N->line; }
instruction(R) ::= XKW_ADD(N) argument(A1) XTK_COMMA argument(A2).     { R = new XI_Add(A1, A2); R->line = N->line; }
instruction(R) ::= XKW_SUB(N) argument(A1) XTK_COMMA argument(A2).     { R = new XI_Sub(A1, A2); R->line = N->line; }
instruction(R) ::= XKW_CMP(N) argument(A1) XTK_COMMA argument(A2).     { R = new XI_Cmp(A1, A2); R->line = N->line; }
instruction(R) ::= XKW_TEST(N) argument(A1) XTK_COMMA argument(A2).    { R = new XI_Test(A1, A2); R->line = N->line; }
instruction(R) ::= XKW_INC(N) argument(A1).                            { R = new XI_Inc(A1); R->line = N->line; }
instruction(R) ::= XKW_DEC(N) argument(A1).                            { R = new XI_Dec(A1); R->line = N->line; }
instruction(R) ::= XKW_IMUL(N) argument(A1).                           { R = new XI_Imul1(A1); R->line = N->line; }
instruction(R) ::= XKW_MUL(N) argument(A1).                           { R = new XI_Mul(A1); R->line = N->line; }
instruction(R) ::= XKW_IMUL(N) argument(A1) XTK_COMMA argument(A2).    { R = new XI_Imul2(A1, A2); R->line = N->line; }
instruction(R) ::= XKW_IMUL(N) argument(A1) XTK_COMMA argument(A2) XTK_COMMA argument(A3). { R = new XI_Imul3(A1, A2, A3); R->line = N->line; }
instruction(R) ::= XKW_DIV(N) argument(A1).                            { R = new XI_Div(A1); R->line = N->line; }
instruction(R) ::= XKW_IDIV(N) argument(A1).                           { R = new XI_Idiv(A1); R->line = N->line; }
instruction(R) ::= XKW_AND(N) argument(A1) XTK_COMMA argument(A2).     { R = new XI_And(A1, A2); R->line = N->line; }
instruction(R) ::= XKW_OR(N) argument(A1) XTK_COMMA argument(A2).      { R = new XI_Or(A1, A2); R->line = N->line; }
instruction(R) ::= XKW_XOR(N) argument(A1) XTK_COMMA argument(A2).     { R = new XI_Xor(A1, A2); R->line = N->line; }
instruction(R) ::= XKW_NOT(N) argument(A1).                            { R = new XI_Not(A1); R->line = N->line; }
instruction(R) ::= XKW_NEG(N) argument(A1).                            { R = new XI_Neg(A1); R->line = N->line; }
instruction(R) ::= XKW_SHL(N) argument(A1) XTK_COMMA argument(A2).     { R = new XI_Shl(A1, A2); R->line = N->line; }
instruction(R) ::= XKW_SHR(N) argument(A1) XTK_COMMA argument(A2).     { R = new XI_Shr(A1, A2); R->line = N->line; }
instruction(R) ::= XKW_JMP(N) argument(A1).                            { R = new XI_Jmp(A1); R->line = N->line; }
instruction(R) ::= XKW_JE(N) argument(A1).                             { R = new XI_Jz(A1); R->line = N->line; }
instruction(R) ::= XKW_JNE(N) argument(A1).                            { R = new XI_Jnz(A1); R->line = N->line; }
instruction(R) ::= XKW_JZ(N) argument(A1).                             { R = new XI_Jz(A1); R->line = N->line; }
instruction(R) ::= XKW_JNZ(N) argument(A1).                            { R = new XI_Jnz(A1); R->line = N->line; }
instruction(R) ::= XKW_JG(N) argument(A1).                             { R = new XI_Jg(A1); R->line = N->line; }
instruction(R) ::= XKW_JGE(N) argument(A1).                            { R = new XI_Jge(A1); R->line = N->line; }
instruction(R) ::= XKW_JL(N) argument(A1).                             { R = new XI_Jl(A1); R->line = N->line; }
instruction(R) ::= XKW_JLE(N) argument(A1).                            { R = new XI_Jle(A1); R->line = N->line; }
instruction(R) ::= XKW_JB(N) argument(A1).                             { R = new XI_Jb(A1); R->line = N->line; }
instruction(R) ::= XKW_JNAE(N) argument(A1).                           { R = new XI_Jb(A1); R->line = N->line; }
instruction(R) ::= XKW_JC(N) argument(A1).                             { R = new XI_Jb(A1); R->line = N->line; }
instruction(R) ::= XKW_JNB(N) argument(A1).                            { R = new XI_Jae(A1); R->line = N->line; }
instruction(R) ::= XKW_JAE(N) argument(A1).                            { R = new XI_Jae(A1); R->line = N->line; }
instruction(R) ::= XKW_JNC(N) argument(A1).                            { R = new XI_Jae(A1); R->line = N->line; }
instruction(R) ::= XKW_JBE(N) argument(A1).                            { R = new XI_Jbe(A1); R->line = N->line; }
instruction(R) ::= XKW_JNA(N) argument(A1).                            { R = new XI_Jbe(A1); R->line = N->line; }
instruction(R) ::= XKW_JA(N) argument(A1).                             { R = new XI_Ja(A1); R->line = N->line; }
instruction(R) ::= XKW_JNBE(N) argument(A1).                           { R = new XI_Ja(A1); R->line = N->line; }
instruction(R) ::= XKW_CALL(N) call_argument(A1).                      { R = new XI_Call(A1); R->line = N->line; }
instruction(R) ::= XKW_RET(N) opt_argument(A1).                        { R = new XI_Ret(A1); R->line = N->line; }
instruction(R) ::= XKW_SETA(N) argument(A1).    { R = new XI_Seta(A1); R->line = N->line; }
instruction(R) ::= XKW_SETAE(N) argument(A1).   { R = new XI_Setae(A1); R->line = N->line; }
instruction(R) ::= XKW_SETB(N) argument(A1).    { R = new XI_Setb(A1); R->line = N->line; }
instruction(R) ::= XKW_SETBE(N) argument(A1).   { R = new XI_Setbe(A1); R->line = N->line; }
instruction(R) ::= XKW_SETC(N) argument(A1).    { R = new XI_Setc(A1); R->line = N->line; }
instruction(R) ::= XKW_SETE(N) argument(A1).    { R = new XI_Sete(A1); R->line = N->line; }
instruction(R) ::= XKW_SETG(N) argument(A1).    { R = new XI_Setg(A1); R->line = N->line; }
instruction(R) ::= XKW_SETGE(N) argument(A1).   { R = new XI_Setge(A1); R->line = N->line; }
instruction(R) ::= XKW_SETL(N) argument(A1).    { R = new XI_Setl(A1); R->line = N->line; }
instruction(R) ::= XKW_SETLE(N) argument(A1).   { R = new XI_Setle(A1); R->line = N->line; }
instruction(R) ::= XKW_SETNA(N) argument(A1).   { R = new XI_Setna(A1); R->line = N->line; }
instruction(R) ::= XKW_SETNAE(N) argument(A1).  { R = new XI_Setnae(A1); R->line = N->line; }
instruction(R) ::= XKW_SETNB(N) argument(A1).   { R = new XI_Setnb(A1); R->line = N->line; }
instruction(R) ::= XKW_SETNBE(N) argument(A1).  { R = new XI_Setnbe(A1); R->line = N->line; }
instruction(R) ::= XKW_SETNC(N) argument(A1).   { R = new XI_Setnc(A1); R->line = N->line; }
instruction(R) ::= XKW_SETNE(N) argument(A1).   { R = new XI_Setne(A1); R->line = N->line; }
instruction(R) ::= XKW_SETNG(N) argument(A1).   { R = new XI_Setng(A1); R->line = N->line; }
instruction(R) ::= XKW_SETNGE(N) argument(A1).  { R = new XI_Setnge(A1); R->line = N->line; }
instruction(R) ::= XKW_SETNL(N) argument(A1).   { R = new XI_Setnl(A1); R->line = N->line; }
instruction(R) ::= XKW_SETNLE(N) argument(A1).  { R = new XI_Setnle(A1); R->line = N->line; }
instruction(R) ::= XKW_SETNO(N) argument(A1).   { R = new XI_Setno(A1); R->line = N->line; }
instruction(R) ::= XKW_SETNP(N) argument(A1).   { R = new XI_Setnp(A1); R->line = N->line; }
instruction(R) ::= XKW_SETNS(N) argument(A1).   { R = new XI_Setns(A1); R->line = N->line; }
instruction(R) ::= XKW_SETNZ(N) argument(A1).   { R = new XI_Setnz(A1); R->line = N->line; }
instruction(R) ::= XKW_SETO(N) argument(A1).    { R = new XI_Seto(A1); R->line = N->line; }
instruction(R) ::= XKW_SETP(N) argument(A1).    { R = new XI_Setp(A1); R->line = N->line; }
instruction(R) ::= XKW_SETPE(N) argument(A1).   { R = new XI_Setpe(A1); R->line = N->line; }
instruction(R) ::= XKW_SETPO(N) argument(A1).   { R = new XI_Setpo(A1); R->line = N->line; }
instruction(R) ::= XKW_SETS(N) argument(A1).    { R = new XI_Sets(A1); R->line = N->line; }
instruction(R) ::= XKW_SETZ(N) argument(A1).    { R = new XI_Setz(A1); R->line = N->line; }
instruction(R) ::= XKW_CDQ(N).                  { R = new XI_Cdq(); R->line = N->line; }
instruction(R) ::= XKW_LEAVE(N).                { R = new XI_Leave(); R->line = N->line; }

opt_argument(R) ::= argument(A). { R = A; }
opt_argument(R) ::=  .           { R = NULL; }

call_argument(R) ::= argument(A). { R = A; }
call_argument(R) ::= XTK_AT XTK_ID(I1) XTK_DOT XTK_ID(I2). { R = new XArgExternalFuntionName(I1->tokenLexeme, I2->tokenLexeme); }

argument(R) ::= register(R1). { R = R1; }
argument(R) ::= memory(M).    { R = M; }
argument(R) ::= constant(C).  { R = new XArgConstant((unsigned int)C); }
argument(R) ::= XTK_ID(I).        { R = new XArgIdentifier(I->tokenLexeme); }
argument(R) ::= XTK_DOT XTK_ID(I).{ R = new XArgIdentifier("." + I->tokenLexeme); }
argument(R) ::= XCKW_PADDR XTK_LPAREN address_expr(E) XTK_RPAREN. { R = new XArgPhyAddress(E); }

register(R) ::= register8(R1).     { R = R1; }
register(R) ::= register16(R1).    { R = R1; }
register(R) ::= register32(R1).    { R = R1; }

register8(R) ::= TK_AL.   { R = new XArgRegister(BS_8, R_AL); }
register8(R) ::= TK_AH.   { R = new XArgRegister(BS_8, R_AH); }
register8(R) ::= TK_BL.   { R = new XArgRegister(BS_8, R_BL); }
register8(R) ::= TK_BH.   { R = new XArgRegister(BS_8, R_BH); }
register8(R) ::= TK_CL.   { R = new XArgRegister(BS_8, R_CL); }
register8(R) ::= TK_CH.   { R = new XArgRegister(BS_8, R_CH); }
register8(R) ::= TK_DL.   { R = new XArgRegister(BS_8, R_DL); }
register8(R) ::= TK_DH.   { R = new XArgRegister(BS_8, R_DH); }

register16(R) ::= TK_AX.  { R = new XArgRegister(BS_16, R_AX); }
register16(R) ::= TK_BX.  { R = new XArgRegister(BS_16, R_BX); }
register16(R) ::= TK_CX.  { R = new XArgRegister(BS_16, R_CX); }
register16(R) ::= TK_DX.  { R = new XArgRegister(BS_16, R_DX); }

register32(R) ::= TK_EAX. { R = new XArgRegister(BS_32, R_EAX); }
register32(R) ::= TK_EBX. { R = new XArgRegister(BS_32, R_EBX); }
register32(R) ::= TK_ECX. { R = new XArgRegister(BS_32, R_ECX); }
register32(R) ::= TK_EDX. { R = new XArgRegister(BS_32, R_EDX); }
register32(R) ::= TK_ESI. { R = new XArgRegister(BS_32, R_ESI); }
register32(R) ::= TK_EDI. { R = new XArgRegister(BS_32, R_EDI); }
register32(R) ::= TK_ESP. { R = new XArgRegister(BS_32, R_ESP); }
register32(R) ::= TK_EBP. { R = new XArgRegister(BS_32, R_EBP); }

memory(R) ::= opt_size_directive(D) address_ref(A).  { R = new XArgMemRef(D, A); }

opt_size_directive(R) ::= size_directive(D).  { R = D; }
opt_size_directive(R) ::= .                   { R = SD_None; }

size_directive(R) ::= XKW_BYTE opt_kw_ptr.   { R = SD_BytePtr; }
size_directive(R) ::= XKW_WORD opt_kw_ptr.   { R = SD_WordPtr; }
size_directive(R) ::= XKW_DWORD opt_kw_ptr.  { R = SD_DWordPtr; }

opt_kw_ptr ::= XKW_PTR.
opt_kw_ptr ::= .
        
address_ref(R) ::= XTK_LBRACKET address_expr(A) XTK_RBRACKET.   { R = A; }

address_expr(R) ::= address_term(T1).  { R = T1; }
address_expr(R) ::= address_term(T1) expr_op(OP) address_term(T2).
                    { R = new XAddrExpr2Term(OP, T1, T2); }

address_expr(R) ::= address_term(T1) expr_op(OP1) address_term(T2) expr_op(OP2) address_term (T3).
                    { R = new XAddrExpr3Term(OP1, OP2, T1, T2, T3); }

address_term(R) ::= address_factor(T1) XTK_OP_MULT address_factor(T2). { R = new XAddrExprMult(T1, T2); }
address_term(R) ::= address_factor(F). { R = F; }

address_factor(R) ::= register (R1).  { R = new XAddrExprReg(R1); }
address_factor(R) ::= constant (C).   { R = new XAddrExprConst(C); }

expr_op(R) ::= XTK_OP_PLUS.  { R = XOP_PLUS; }
expr_op(R) ::= XTK_OP_MINUS. { R = XOP_MINUS; }

constant(R) ::= XTK_DEC_CONSTANT(C).  { R = C->intValue; }
constant(R) ::= XTK_OP_MINUS XTK_DEC_CONSTANT(C).  { R = -1*C->intValue; }
constant(R) ::= XTK_HEX_CONSTANT(C).  { R = C->intValue; }
constant(R) ::= XTK_BIN_CONSTANT(C).  { R = C->intValue; }
constant(R) ::= XTK_OCT_CONSTANT(C).  { R = C->intValue; }
constant(R) ::= XTK_CHAR_CONSTANT(C). { R = C->intValue; }
