; S(-6, 0)
push 0
push -6
call S
add esp, 8
#stop

S:
  mov eax, dword [esp + 4]
  loop: 
      cmp eax, dword [esp + 8]
      jg end_loop
      #show eax signed decimal
      inc eax
      jmp loop
  end_loop:
      ret
      