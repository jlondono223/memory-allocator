.text
.align 8
.globl getFP
getFP:
    movq %rbp, %rax 
    ret 

.text
.align 8
.globl getSP
getSP:
    movq %rsp, %rax 
    ret

.text
.align 8
.globl getRBX
getRBX:
    movq %rbx, %rax
    ret

.text
.align 8
.globl getRSI
getRSI:
    movq %rsi, %rax
    ret

.text
.align 8
.globl getRDI
getRDI:
    movq %rdi, %rax
    ret
    