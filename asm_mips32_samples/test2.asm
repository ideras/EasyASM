#set $t0 = 0
#set $t1 = 5

loop: 
  beq $t0, $t1, end_loop
  addi $t0, $t0, 1
  #show $t0
  j loop
  
end_loop:
