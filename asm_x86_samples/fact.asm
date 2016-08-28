; Factorial implementation
; This code was generated with gcc using no optimization
  
  mov eax, 7
  push eax
  call fact
  add esp, 4
  #show eax
  #stop
  
fact:
        push    ebp
        mov     ebp, esp
        sub     esp, 8
        cmp     DWORD PTR [ebp+8], 1
        jg      L2
        mov     eax, 1
        jmp     L3
L2:
        mov     eax, DWORD PTR [ebp+8]
        sub     eax, 1
        sub     esp, 12
        push    eax
        call    fact
        add     esp, 16
        imul    eax, DWORD PTR [ebp+8]
L3:
        leave
        ret
