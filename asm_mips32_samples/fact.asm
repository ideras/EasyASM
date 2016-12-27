
#set $a0 = 6
; lui $s0, #hihw(fact)
; ori $s0, $s0, #lohw(fact)
#set $s0 = fact
jalr $s0

#show $v0
#stop

;------------------------------------------------
; Fact - Implementacion recursiva de Factorial
;	a0 - es el parametro n
;	s0 - mantiene una copia del parametro n
;	v0 - valor de retorno
;------------------------------------------------

fact:	
	addi $sp, $sp, -12 ; Reserva 12 bytes en la pila para salvar los registros
	sw $a0, 0($sp)
	sw $s0, 4($sp)
	sw $ra, 8($sp)

        ; if (n < 2)
        slti $t0, $a0, 2
        beq $t0, $zero, notOne
	addi $v0, $zero, 1
	j fret

notOne: 
        #show $a0
        move $s0, $a0           ; Guarda el n original
	addi $a0, $a0, -1	; param = n-1
	jal fact		; calcula fact(n-1)
        #show $v0
        mult $v0, $s0
        mflo $v0

fret:	lw $a0, 0($sp) 	; restaura los valores de los registros
	lw $s0, 4($sp)
	lw $ra, 8($sp)
	addi $sp, $sp, 12
	jr $ra
