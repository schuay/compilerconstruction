    .text
    .globl  asmb
    .type   asmb, @function

asmb:
    .cfi_startproc              # rdi: x, rsi: n
    cmp     $0, %rsi            # note: clears carry
    jle     out                 # if (n <= 0) return
    mov     %rsi, %rcx          # i = n
loop:
    rcrq    -8(%rdi, %rcx, 8)   # rotate x[i - 1] right through carry
    loop    loop                # if (--i) != 0 goto loop
out:
    ret
    .cfi_endproc

