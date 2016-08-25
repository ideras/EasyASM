  mov eax, 0x10000000
  #set byte [eax] = [48, 49, 50, 48, 48, 48, 50, 0]
  push 8
  push eax
  push 0x10000008
  call strncpy
  add esp, 12
  #show byte [0x10000008][8]
  #stop
  
strncpy:
  push ebp
  mov ebp, esp
  sub esp, 4
  
  mov eax, [ebp+8]
  mov dword [ebp-4], eax

while_1:
  cmp dword [ebp+16], 0
  jle while_2
  mov eax, [ebp+12]
  cmp byte [eax], 0
  je while_2
  
  mov ebx, [ebp-4]
  inc dword [ebp-4]
  inc dword [ebp+12]
  mov cl, [eax]
  mov byte [ebx], cl

  dec dword [ebp+16]
  jmp while_1

while_2:
    cmp dword [ebp+16], 0
    jle end
    
    mov ebx, [ebp-4]
    inc dword [ebp-4]
    mov byte [ebx], 0

    dec dword [ebp+16]
    jmp while_2

end:
    mov eax, [ebp+8]
    mov esp, ebp
    pop ebp
    ret
