; ================================================
; Selection sort program.
; Generated using gcc version 4.9.2 with -O3 flag
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
	push	ebp
	push	edi
	push	esi
	push	ebx
	sub	esp, 8
	cmp	DWORD PTR [esp+32], 1
	mov	ebp, DWORD PTR [esp+28]
	jle	.L1
.L14:
	mov	eax, DWORD PTR [ebp+0]
	lea	ebx, [ebp+4]
	mov	esi, ebx
	mov	DWORD PTR [esp], ebx
	mov	ebx, 1
	mov	DWORD PTR [esp+4], eax
	mov	ecx, eax
	xor	eax, eax
.L8:
	mov	edx, DWORD PTR [esi]
	lea	edi, [ebp+0+eax*4]
	cmp	edx, ecx
	jge	.L4
	mov	eax, ebx
.L4:
	cmp	edx, ecx
	jge	.L6
	mov	edi, esi
.L6:
	cmp	ecx, edx
	jle	.L7
	mov	ecx, edx
.L7:
	add	ebx, 1
	add	esi, 4
	cmp	ebx, DWORD PTR [esp+32]
	jl	.L8
	sub	DWORD PTR [esp+32], 1
	mov	eax, DWORD PTR [esp+4]
	cmp	DWORD PTR [esp+32], 1
	mov	DWORD PTR [ebp+0], ecx
	mov	ebp, DWORD PTR [esp]
	mov	DWORD PTR [edi], eax
	jne	.L14
.L1:
	add	esp, 8
	pop	ebx
	pop	esi
	pop	edi
	pop	ebp
	ret


