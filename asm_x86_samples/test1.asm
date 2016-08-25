; A simple test program that adds two numbers

start: 
mov eax, 10
mov ebx, 20

lbl_add1: add eax, ebx
lbl_mult1: imul eax, -10

#show eax signed decimal
