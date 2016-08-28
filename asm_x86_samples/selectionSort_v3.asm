

LessThan:
  mov ebx, [esp+4]
  cmp ebx, dword [esp+8]
  jl less 
  mov eax, 0
  ret 
less: 
  mov eax, 1
  ret

GreaterThan:
  mov ebx, [esp+4]
  cmp ebx, dword [esp+8]
  jg greater 
  mov eax, 0
  ret 
greater: 
  mov eax, 1
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
    mov ebx, dword [ebp-8] 
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
    



  

