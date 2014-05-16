	.file	"context_switch.c"
	.text
	.align	2
	.global	kerent
	.type	kerent, %function
kerent:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0

	# 1. acquire the arguments of the request, which are in registers that might be over-written;
	# 2. acquire the lr, which is the pc of the active task;
	mov r1, lr
	# 3. change to system state;
	msr cpsr_c, #0xdf
	# 4. overwrite lr with the value from 2;
	mov lr, r1
	# 5. push the registers of the active task onto its stack;
	# stmfd sp!, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, fp, ip, lr, pc}
	# 6. acquire the sp of the active task;
	mov r2, sp
	# 7. return to svc state;
	msr cpsr_c, #0xd3
	# 8. acquire the spsr of the active task;
	mrs r3, spsr
	# 9. pop the registers of the kernel from its stack;
	ldmfd sp!, {r4, r5, r6, r7, r8, r9, r10, fp, ip, lr}
	# 10. fill in the request with its arguments; and
	# 11. put the sp and the spsr into the TD of the active task
	# str sp, [r0]
	# str r3, [r0, #4]
	# str sp, [r0, #8]

	.size	kerent, .-kerent
	.align	2
	.global	kerxit
	.type	kerxit, %function
kerxit:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 1, uses_anonymous_args = 0

	# 1. push the kernel registers onto its stack;
	stmfd sp!, {r4, r5, r6, r7, r8, r9, r10, fp, ip, lr}
	# 2. change to system state;
	msr cpsr_c, #0xdf
	# 3. get the sp, spsr and return value of the active task from its TD;
	# ldr sp, [r0]
	# ldr r1, [r0, #4]
	# ldr r2, [r0, #8]
	# 4. pop the registers of the active task from its stack;
	# ldmfd	sp!, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, fp, ip, lr}
	# 5. put the return value in r0;
	mov r0, #1
	# 6. return to svc state;
	msr cpsr_c, #0xd3
	# 7. install the spsr of the active task; and
	# msr spsr, r1
	# 8. install the pc of the active task.
	ldmfd	sp!, {pc}

	.size	kerxit, .-kerxit
	.ident	"GCC: (GNU) 4.0.2"
