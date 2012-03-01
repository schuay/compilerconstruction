	.file	"asmb.c"
	.text
	.globl	asmb
	.type	asmb, @function
asmb:
.LFB0:
	.cfi_startproc
# x[]: rdi, n: rsi
	dec 	%rsi                    # n = n - 1
	js      out                     # if (n < 0) goto L1
    clc                             # clear carry
loop:
    rcrq    (%rdi,%rsi,8)           # x[n] = x[n] >> 1, save carry
	dec 	%rsi                    # n = n - 1
	jns	    loop                    # if (n >= 0) goto loop
out:
	ret
	.cfi_endproc
.LFE0:
	.size	asmb, .-asmb
	.ident	"GCC: (GNU) 4.6.2 20120120 (prerelease)"
	.section	.note.GNU-stack,"",@progbits
