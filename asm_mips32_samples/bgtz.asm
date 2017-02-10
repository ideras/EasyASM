#set $t0 = 10

loop:
    #show $t0
    addi $t0, $t0, -1
    bgtz $t0, loop

