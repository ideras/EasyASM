  mov eax, 0x10000000
  #set byte [eax] = [48,49,50,48,48,0]
  push 6
  push eax
  push 0x10000008
  call strncpy
  add esp, 12
  #show byte [0x10000008][6]
  #stop

strncpy:
  push ebp
  mov ebp, esp
  sub esp, 4
  mov eax, [ebp+8]
  mov dword [ebp-4], eax

while_1:
  cmp dword [ebp+16], 0
  jle end_while_1
  mov eax, dword [ebp+12]
  cmp byte [eax], 0
  je end_while_1
  mov eax, dword [ebp-4]
  inc dword [ebp-4]
  mov ebx, [ebp+12]
  mov bl, byte [ebx]
  inc dword [ebp+12]
  mov byte [eax], bl
  dec dword [ebp+16]
  jmp while_1

end_while_1:
  cmp dword [ebp+16], 0
  jle end_while_2
  mov eax, dword [ebp-4]
  inc dword [ebp-4]
  mov byte [eax], 0
  dec dword [ebp+16]
  jmp end_while_1

end_while_2:
  mov eax, dword [ebp+8]
  mov esp, ebp
  pop ebp
  ret