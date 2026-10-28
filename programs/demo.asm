# sum from 1 to 5
addi x1 x0 5
addi x2 x0 1
loop:
beq x1 x0 done
add x2 x2 x1
addi x1 x1 -1
jal x0 loop
done:
halt
