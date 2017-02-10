#set $t0 = -10

loop:
    #show $t0 signed decimal
    addi $t0, $t0, 1
    blez $t0, loop