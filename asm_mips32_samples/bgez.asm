#set $t0 = 10

loop:
    #show $t0
    addi $t0, $t0, -1
    bgez $t0, loop

