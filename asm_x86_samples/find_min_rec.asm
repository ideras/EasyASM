mov eax, 0x10000000
#set dword [eax] = [79, 64, 99, 29, 22, 9, 23, 56, 74, 29]
#set dword [0x10000028] = 79
push 0x10000028
push 10
push eax
call find_min
add esp, 12
#show dword [0x10000028]
#stop

find_min:
    mov eax, dword [esp + 4]
    mov eax, dword [eax]

    mov ebx, dword [esp + 12]
    cmp eax, dword [ebx]
    jge endif1
    mov dword[ebx], eax

endif1:
    cmp dword [esp + 8], 1
    je endif2
    mov edx, esp
    push ebx
    mov ecx, dword[edx + 8]
    dec ecx
    push ecx
    mov eax, dword[edx + 4]
    add eax, 4
    push eax
    call find_min
    add esp, 12
endif2:
    ret