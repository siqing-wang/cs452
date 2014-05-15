	.file	"context_switch.c"
	.text
	.align	2
	.global	kerent
	.type	kerent, %function
kerent:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	ldmfd	sp, {fp, sp, pc}
	.size	kerent, .-kerent
	.section	.rodata
	.align	2
.LC0:
	.ascii	"kerxit.c: Hello.\012\015\000"
	.align	2
.LC1:
	.ascii	"kerxit.c: Activating.\012\015\000"
	.align	2
.LC2:
	.ascii	"kerxit.c: Good-bye.\012\015\000"
	.text
	.align	2
	.global	kerxit
	.type	kerxit, %function
kerxit:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 1, uses_anonymous_args = 0

	mov	ip, sp
	stmfd	sp!, {sl, fp, ip, lr, pc}
	sub	fp, ip, #4
	sub	sp, sp, #8
	ldr	sl, .L6
.L5:
	add	sl, pc, sl
	str	r0, [fp, #-20]
	str	r1, [fp, #-24]
	mov	r0, #1
	ldr	r3, .L6+4
	add	r3, sl, r3
	mov	r1, r3
	bl	bwprintf(PLT)
	mov	r0, #1
	ldr	r3, .L6+8
	add	r3, sl, r3
	mov	r1, r3
	bl	bwprintf(PLT)

	# 1. push the kernel registers onto its stack;
	# stmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, fp, ip, lr, pc}
	# 2. change to system state;
	# msr cpsr, #223
	# 3. get the sp, spsr and return value of the active task from its TD;
	# 4. pop the registers of the active task from its stack;
	# 5. put the return value in r0;
	# 6. return to svc state;
	# msr cpsr, #211
	# 7. install the spsr of the active task; and
	# hardcode user mode 208 now
	# msr spsr, #208
	# 8. install the pc of the active task.
	
	bl	kerent(PLT)
	
	# 1. acquire the arguments of the request, which are in registers that might be over-written;
	# 2. acquire the lr, which is the pc of the active task;
	# 3. change to system state;
	# 4. overwrite lr with the value from 2;
	# 5. push the registers of the active task onto its stack;
	# 6. acquire the sp of the active task;
	# 7. return to svc state;
	# msr cpsr, #211
	# 8. acquire the spsr of the active task;
	# 9. pop the registers of the kernel from its stack;
	# ldmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, fp, ip, lr, pc}
	# 10. fill in the request with its arguments; and
	# 11. put the sp and the spsr into the TD of the active task

	mov	r0, #1
	ldr	r3, .L6+12
	add	r3, sl, r3
	mov	r1, r3
	# bl	bwprintf(PLT)
	sub	sp, fp, #16
	ldmfd	sp, {sl, fp, sp, pc}
.L7:
	.align	2
.L6:
	.word	_GLOBAL_OFFSET_TABLE_-(.L5+8)
	.word	.LC0(GOTOFF)
	.word	.LC1(GOTOFF)
	.word	.LC2(GOTOFF)
	.size	kerxit, .-kerxit
	.ident	"GCC: (GNU) 4.0.2"
