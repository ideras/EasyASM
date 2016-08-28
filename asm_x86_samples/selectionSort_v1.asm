; ================================================
; Selection sort program.
; Generated using gcc version 4.9.2 with -O0 flag
; ================================================

mov eax, 0x10000000
#set dword [eax] = [39, 79, 61, 57, 45, 34, 15, 27, 19, 5]
mov ebx, 10
push ebx
push eax
call selectionSort
add esp, 8

mov eax, 0x10000000
#show dword [eax][10]

#stop

selectionSort:
        push    ebp
        mov     ebp, esp
        sub     esp, 24
        cmp     DWORD PTR [ebp+12], 1
        jg      .L2
        jmp     .L1
.L2:
        mov     DWORD PTR [ebp-16], 0
        mov     DWORD PTR [ebp-12], 1
        jmp     .L4
.L6:
        mov     eax, DWORD PTR [ebp-12]
        lea     edx, [0+eax*4]
        mov     eax, DWORD PTR [ebp+8]
        add     eax, edx
        mov     edx, DWORD PTR [eax]
        mov     eax, DWORD PTR [ebp-16]
        lea     ecx, [0+eax*4]
        mov     eax, DWORD PTR [ebp+8]
        add     eax, ecx
        mov     eax, DWORD PTR [eax]
        cmp     edx, eax
        jge     .L5
        mov     eax, DWORD PTR [ebp-12]
        mov     DWORD PTR [ebp-16], eax
.L5:
        add     DWORD PTR [ebp-12], 1
.L4:
        mov     eax, DWORD PTR [ebp-12]
        cmp     eax, DWORD PTR [ebp+12]
        jl      .L6
        mov     eax, DWORD PTR [ebp+8]
        mov     eax, DWORD PTR [eax]
        mov     DWORD PTR [ebp-20], eax
        mov     eax, DWORD PTR [ebp-16]
        lea     edx, [0+eax*4]
        mov     eax, DWORD PTR [ebp+8]
        add     eax, edx
        mov     edx, DWORD PTR [eax]
        mov     eax, DWORD PTR [ebp+8]
        mov     DWORD PTR [eax], edx
        mov     eax, DWORD PTR [ebp-16]
        lea     edx, [0+eax*4]
        mov     eax, DWORD PTR [ebp+8]
        add     edx, eax
        mov     eax, DWORD PTR [ebp-20]
        mov     DWORD PTR [edx], eax
        mov     eax, DWORD PTR [ebp+12]
        lea     edx, [eax-1]
        mov     eax, DWORD PTR [ebp+8]
        add     eax, 4
        sub     esp, 8
        push    edx
        push    eax
        call    selectionSort
        add     esp, 16
.L1:
        leave
        ret
