mov eax, 0x10000000
#set dword [eax] = [24, 15, 79, 16, 78, 52, 9, 61, 8, 57]
push 9
push 0
push eax
mov eax, quicksort
call eax
; call quicksort
add esp, 12
#show dword [0x10000000][10]
#stop

quicksort:
	push	ebp
	mov	ebp, esp
	sub	esp, 24
	mov	eax, DWORD PTR [ebp+12]
	cmp	eax, DWORD PTR [ebp+16]
	jge	.L1
	mov	eax, DWORD PTR [ebp+12]
	mov	DWORD PTR [ebp-20], eax
	mov	eax, DWORD PTR [ebp+12]
	mov	DWORD PTR [ebp-12], eax
	mov	eax, DWORD PTR [ebp+16]
	mov	DWORD PTR [ebp-16], eax
	jmp	.L3
.L10:
	jmp	.L4
.L6:
	add	DWORD PTR [ebp-12], 1
.L4:
	mov	eax, DWORD PTR [ebp-12]
	lea	edx, [0+eax*4]
	mov	eax, DWORD PTR [ebp+8]
	add	eax, edx
	mov	edx, DWORD PTR [eax]
	mov	eax, DWORD PTR [ebp-20]
	lea	ecx, [0+eax*4]
	mov	eax, DWORD PTR [ebp+8]
	add	eax, ecx
	mov	eax, DWORD PTR [eax]
	cmp	edx, eax
	jg	.L5
	mov	eax, DWORD PTR [ebp-12]
	cmp	eax, DWORD PTR [ebp+16]
	jle	.L6
.L5:
	jmp	.L7
.L9:
	sub	DWORD PTR [ebp-16], 1
.L7:
	mov	eax, DWORD PTR [ebp-16]
	lea	edx, [0+eax*4]
	mov	eax, DWORD PTR [ebp+8]
	add	eax, edx
	mov	edx, DWORD PTR [eax]
	mov	eax, DWORD PTR [ebp-20]
	lea	ecx, [0+eax*4]
	mov	eax, DWORD PTR [ebp+8]
	add	eax, ecx
	mov	eax, DWORD PTR [eax]
	cmp	edx, eax
	jle	.L8
	mov	eax, DWORD PTR [ebp-16]
	cmp	eax, DWORD PTR [ebp+12]
	jge	.L9
.L8:
	mov	eax, DWORD PTR [ebp-12]
	cmp	eax, DWORD PTR [ebp-16]
	jge	.L3
	mov	eax, DWORD PTR [ebp-12]
	lea	edx, [0+eax*4]
	mov	eax, DWORD PTR [ebp+8]
	add	eax, edx
	mov	eax, DWORD PTR [eax]
	mov	DWORD PTR [ebp-24], eax
	mov	eax, DWORD PTR [ebp-12]
	lea	edx, [0+eax*4]
	mov	eax, DWORD PTR [ebp+8]
	add	edx, eax
	mov	eax, DWORD PTR [ebp-16]
	lea	ecx, [0+eax*4]
	mov	eax, DWORD PTR [ebp+8]
	add	eax, ecx
	mov	eax, DWORD PTR [eax]
	mov	DWORD PTR [edx], eax
	mov	eax, DWORD PTR [ebp-16]
	lea	edx, [0+eax*4]
	mov	eax, DWORD PTR [ebp+8]
	add	edx, eax
	mov	eax, DWORD PTR [ebp-24]
	mov	DWORD PTR [edx], eax
.L3:
	mov	eax, DWORD PTR [ebp-12]
	cmp	eax, DWORD PTR [ebp-16]
	jl	.L10
	mov	eax, DWORD PTR [ebp-16]
	lea	edx, [0+eax*4]
	mov	eax, DWORD PTR [ebp+8]
	add	eax, edx
	mov	eax, DWORD PTR [eax]
	mov	DWORD PTR [ebp-24], eax
	mov	eax, DWORD PTR [ebp-16]
	lea	edx, [0+eax*4]
	mov	eax, DWORD PTR [ebp+8]
	add	edx, eax
	mov	eax, DWORD PTR [ebp-20]
	lea	ecx, [0+eax*4]
	mov	eax, DWORD PTR [ebp+8]
	add	eax, ecx
	mov	eax, DWORD PTR [eax]
	mov	DWORD PTR [edx], eax
	mov	eax, DWORD PTR [ebp-20]
	lea	edx, [0+eax*4]
	mov	eax, DWORD PTR [ebp+8]
	add	edx, eax
	mov	eax, DWORD PTR [ebp-24]
	mov	DWORD PTR [edx], eax
	mov	eax, DWORD PTR [ebp-16]
	sub	eax, 1
	sub	esp, 4
	push	eax
	push	DWORD PTR [ebp+12]
	push	DWORD PTR [ebp+8]
	call	quicksort
	add	esp, 16
	mov	eax, DWORD PTR [ebp-16]
	add	eax, 1
	sub	esp, 4
	push	DWORD PTR [ebp+16]
	push	eax
	push	DWORD PTR [ebp+8]
	call	quicksort
	add	esp, 16
.L1:
	mov esp, ebp
        pop ebp
	ret
