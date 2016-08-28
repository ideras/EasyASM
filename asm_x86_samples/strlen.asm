  mov eax, 0x10000000
  #set byte [eax] = [48,49,50,48,48,0]
  push 0x10000008
  push eax
  call my_strlen
  add esp, 8
  #show dword [0x10000008]
  #stop
  
my_strlen:
    mov eax, dword [esp + 8]
    mov dword [eax], 0
loop_for:
    mov ebx, dword[esp + 4]
    mov ecx, [eax]
    mov bl, [ebx + ecx]
    cmp bl, 0
    je end_loop_for
    inc dword [eax]
    jmp loop_for
end_loop_for:
    ret