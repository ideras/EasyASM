  mov eax, 0x10000000
  #set byte [eax] = [48, 49, 50, 48, 48, 48, 50, 0]
  push 0x10000008
  push eax
  call my_strlen
  add esp, 8
  #show dword [0x10000008]
  #stop

  my_strlen:
	  ;*len = 0
	  mov eax, [esp + 8]
	  mov [eax], 0
  for:
	  mov ebx, [esp + 4]
	  add ebx, dword [eax]
	  mov bl, byte [ebx]
	cmp bl, 0
	je end_for
	inc dword [eax]
	jmp for
end_for:
prologue:
	ret