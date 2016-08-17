main:
        #set memory word 0($gp) = [55, 44, 20, 99, 3]
	move $a0, $gp
        #set $a1 = 5
	jal selectionSort
	j print

selectionSort:
	addi $sp, $sp, -4
	sw $ra, 0($sp)
	
	slti $t0, $a1, 2
	bne $t0, $zero, end_if
	addi $t0, $zero, 0
	addi $t1, $zero, 1
	
for:
	slt $t3, $t1, $a1
	beq $t3, $zero, end_for
	sll $t2, $t1, 2
	add $t2, $a0, $t2
	lw $t3, 0($t2)
	sll $t2, $t0, 2
	add $t2, $a0, $t2
	lw $t4, 0($t2)
	slt $t2, $t3, $t4
	beq $t2, $zero, add_index
	add $t0, $zero, $t1

add_index:
	addi $t1, $t1, 1
	j for

end_for:
	lw $t1, 0($a0)
	sll $t2, $t0, 2
	add $t3, $a0, $t2
	lw $t2, 0($t3)
	sw $t2, 0($a0)
	sw $t1, 0($t3)
	addi $a0, $a0, 4
	addi $a1, $a1, -1
	jal selectionSort

end_if:
	lw $ra, 0($sp)
	addi $sp, $sp, 4
	jr $ra

print:
    move $t0, $gp
    #set $t1 = 5
	addi $t2, $zero, 0

print_for:
    #show memory word 0($t0)
	
	addi $t2, $t2, 1
	addi $t0, $t0, 4
	bne $t1, $t2, print_for

print_end_for:
	#stop

	
	
