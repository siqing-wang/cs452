	.file	"asm_lib.c"
	.text
	.align	2
	.global	storeRequestInR0
	.type	storeRequestInR0, %function
storeRequestInR0:
	@ args = 0, pretend = 0, frame = 4
	@ frame_needed = 1, uses_anonymous_args = 0

	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}

	# mov r0, r0

	sub	fp, ip, #4
	ldmfd	sp, {fp, sp, pc}

	.size	storeRequestInR0, .-storeRequestInR0
	.ident	"GCC: (GNU) 4.0.2"
