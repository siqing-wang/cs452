	.file	"context_switch.c"
	.text
	.align	2
	.global	kerent
	.type	kerent, %function
kerent:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0

	# 2. acquire the lr, which is the pc of the active task;
	mov r2, lr
	
	# 3. change to system state;
	msr cpsr_c, #0xdf
	
	# 4. overwrite lr with the value from 2;
	# 8. acquire the spsr of the active task;
	mov lr, r2
	mrs r1, cpsr
	
	# 5. push the registers of the active task onto its stack; r0 is return value, which will be pushed in kernel
	stmfd sp!, {r1, r2, r4, r5, r6, r7, r8, r9, r10, fp, ip, lr}

	# 1. acquire the arguments of the request, which are in registers that might be over-written;
	mov r2, r0

	# 6. acquire the sp of the active task;
	mov r3, sp
	
	# 7. return to svc state;
	msr cpsr_c, #0xd3
	
	# 9. pop the registers of the kernel from its stack;
	ldmfd sp!, {r0, r1, r4, r5, r6, r7, r8, r9, r10, fp, ip, lr}
	
	# 10. fill in the request with its arguments; and
	str r2, [r1]

	# 11. put the sp and the spsr into the TD of the active task
	str r3, [r0]

	mov pc, lr

	.size	kerent, .-kerent
	.align	2
	.global	kerxit
	.type	kerxit, %function
kerxit:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 1, uses_anonymous_args = 0

	# 1. push the kernel registers onto its stack;
	stmfd sp!, {r0, r1, r4, r5, r6, r7, r8, r9, r10, fp, ip, lr}
	
	# 2. change to system state;
	msr cpsr_c, #0xdf
	
	# 3. get the sp, spsr and return value of the active task from its TD;
	ldr sp, [r0]

	# 4. pop the registers of the active task from its stack; return value in r0, spsr in r1; code in r2
	# 5. put the return value in r0;
	ldmfd sp!, {r0, r1, r2, r4, r5, r6, r7, r8, r9, r10, fp, ip, lr}

	# stmfd sp!, {r0, r1, r2, r3, lr}
	# mov r3, r0
	# mov r0, #1
	# mov	r1, r3
	# bl	bwputr(PLT)
	# ldmfd sp!, {r0, r1, r2, r3, lr}

	# 6. return to svc state;
	msr cpsr_c, #0xd3

	# 7. install the spsr of the active task; and
	msr cpsr, r1
	
	# 8. install the pc of the active task.
	mov pc, r2

	.size	kerxit, .-kerxit
	.ident	"GCC: (GNU) 4.0.2"
