#set dword[0x10000000] = [22, 13, 1, 69, 2042]
mov eax, GreaterThan
push eax
push 5
push 0x10000000

call SelectionSort
add esp, 12

#show dword[0x10000000] [5]

mov eax, LessThan
push eax
push 5
push 0x10000000

call SelectionSort
add esp, 12

#show dword[0x10000000] [5]

#stop

LessThan:
  xor eax, eax
  mov ebx, [esp+4]
  cmp ebx, dword [esp+8]
  setl al
  ret

GreaterThan:
  xor eax, eax
  mov ebx, [esp+4]
  cmp ebx, dword [esp+8]
  setg al
  ret

;array [ebp+8]
;number_of_elements [ebp+12]
;compare [ebp+16]
SelectionSort:
    push ebp
    mov ebp, esp
    sub esp, 16
    ;iter [ebp-4]
    ;jter [ebp-8]
    ;index [ebp-12]
    ;temp [ebp-16]

    mov dword [ebp-4], 0

for1: 
    mov ebx, dword [ebp-4] 
    cmp ebx, dword [ebp+12]
    jge endfor1
    mov edx, dword[ebp-4]
    mov dword [ebp-12], edx

    inc edx
    mov dword[ebp-8], edx

for2:
    mov ecx, dword [ebp-8]
    cmp ecx, dword [ebp+12]
    jge endfor2
    
    mov eax, dword [ebp+8]
    mov ebx, dword [ebp-12]

    push dword [eax+ebx*4]
    mov edx, dword [ebp -8]

    push dword [eax+edx*4]
    
    call dword [ebp+16]

    add esp, 8
    cmp eax, 0
    je incfor2
    mov ebx, dword[ebp-8]
    mov dword[ebp-12], ebx

incfor2:    
    inc dword[ebp-8]
    jmp for2
      
endfor2:
    mov eax, [ebp-4]
    mov ebx, [ebp+8]
    mov ecx, [ebx+eax*4]
    
    mov dword[ebp-16], ecx
    
    mov ecx, [ebp-12]
    mov edx, [ebx+ecx*4]
    
    mov dword[ebx+eax*4], edx
    
    mov eax, [ebp-16]
    mov dword[ebx+ecx*4], eax
    
    inc dword[ebp-4]
    jmp for1
    
endfor1:    
    leave
    ret
    



  

