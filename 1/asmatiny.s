	.file	"asmatiny.c"
	.text
	.globl	asma
	.type	asma, @function
asma:
.LFB0:
	.cfi_startproc
    clc                 # clear carry flag
	rcrq	8(%rdi)     # right shift x[1], value of carry flag is shifted in at left
                        # and then set to shifted out bit
	rcrq	(%rdi)      # right shift x[0] and shift in carry flag from previous shift
	ret
	.cfi_endproc
.LFE0:
	.size	asma, .-asma
	.ident	"GCC: (GNU) 4.6.2 20120120 (prerelease)"
	.section	.note.GNU-stack,"",@progbits
