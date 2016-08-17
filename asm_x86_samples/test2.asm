; test2.asm

mov eax, 0
mov ebx, 5

loop:
    cmp eax, ebx
    je end_loop
    add eax, 1
    #show eax
    jmp loop
  
end_loop:


