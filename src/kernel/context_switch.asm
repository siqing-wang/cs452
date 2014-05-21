	.file	"context_switch.c"
	.text
	.align	2
	.global	kerent
	.type	kerent, %function
kerent:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0

	# acquire the lr, which is the pc of the active task, temporarily save to r2
	mov r2, lr

	# change to system state;
	msr cpsr_c, #0xdf

	# overwrite lr with the value from r2
	mov lr, r2

	# r1 = spsr of the active task
	mrs r1, cpsr

	# push spsr(r1), pc(r2), and other registers of the active task onto its stack
	# will push return value(r0) later when handling syscall request
	stmfd sp!, {r1, r2, r4, r5, r6, r7, r8, r9, r10, fp, ip, lr}

	# r2 = ptr to request(already saved in r0 by request_handle)
	mov r2, r0

	# r3 = sp of the active task
	mov r3, sp

	# change to svc state;
	msr cpsr_c, #0xd3

	# pop the registers of the kernel from its stack;
	# r0 = ptr to sp, r1 = ptr to request*
	ldmfd sp!, {r0, r1, r4, r5, r6, r7, r8, r9, r10, fp, ip, lr}

	# update temporarily saved request*(r2) in ptr to request(r1)
	str r2, [r1]

	# update temporarily saved sp(r3) in ptr to sp(r0)
	str r3, [r0]

	# jump back to kernel
	mov pc, lr

	.size	kerent, .-kerent
	.align	2
	.global	kerxit
	.type	kerxit, %function
kerxit:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 1, uses_anonymous_args = 0

	# push the kernel registers onto its stack;
	# we need r0 & r1 because want to update sp(r0) and request(r1)
	# leave out r2, r3 because they are scratch variables
	stmfd sp!, {r0, r1, r4, r5, r6, r7, r8, r9, r10, fp, ip, lr}

	# change to system state;
	# _c for only accessing 0-7 bits, so do not override other bits including flags
	msr cpsr_c, #0xdf

	# get the sp from r0
	# because r0 contains a ptr to sp we use ldr instead of "mov sp, r0"
	ldr sp, [r0]

	# pop return value(r0), spsr(r1), pc/code(r2) and other saved registers of the active task from its stack
	ldmfd sp!, {r0, r1, r2, r4, r5, r6, r7, r8, r9, r10, fp, ip, lr}

	# change to svc state;
	msr cpsr_c, #0xd3

	# install the spsr of the active task to cpsr
	msr cpsr, r1

	# install the pc of the active task.
	mov pc, r2

	.size	kerxit, .-kerxit
	.ident	"GCC: (GNU) 4.0.2"
