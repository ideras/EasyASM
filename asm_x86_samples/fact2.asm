  push 5
  mov eax, fact
  call eax
  add esp, 4
  #show eax
  #stop
  
fact:
  cmp dword [esp+4], 2
  jl caso_base
  mov eax, [esp+4]
  dec eax
  push eax
  call fact
  add esp, 4
  imul eax, dword [esp + 4]
  jmp epilogo
caso_base:
  mov eax, 1
epilogo:
  ret