	.file	"context_switch.c"
	.text
	.align	2
	.global	intent
	.type	intent, %function
intent:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0

	# Enter with IRQ mode, change to system state
	msr cpsr_c, #0xdf

	# push all scratch registers of the interrupted user task onto its own stack
	stmfd sp!, {r0, r1, r2, r3, ip}

	# change to irq state;
	msr cpsr_c, #0x92

	# r1 = spsr
	mrs r1, spsr

	# r2 = lr - 4 (Important Note: we need to jump to original instruction)
	sub r2, lr, #4

	# r3 = cpsr_irq
	mrs r3, cpsr

	# change to system state
	msr cpsr_c, #0xdf

	# store irq mode's spsr and lr on user stack
	stmfd sp!, {r1, r2}

	# r0 = 0, then Reuqest* will be (Request *)0, to differentiate b/w swi and hwi
	mov r0, #0

	# change to svc state;
	msr cpsr_c, #0xd3

	# spsr_svc = r3 of the active task
	msr spsr, r3

	# kerent
	bl kerent

	# back from kernel

	# change to system state;
	msr cpsr_c, #0xdf

	# load spsr and lr from user stack
	ldmfd sp!, {r1, r2}

	# change to irq state;
	msr cpsr_c, #0x92

	# spsr_irq = r1
	msr spsr, r1

	# lr_irq = r2
	mov lr, r2

	# change to system state;
	msr cpsr_c, #0xdf

	# pop all scratch registers of the active task from its stack
	ldmfd sp!, {r0, r1, r2, r3, ip}

	# change to irq state;
	msr cpsr_c, #0x92

	# back to state before interrupt
	# pc = lr_irq, cpsr = spsr_irq
	movs pc, lr

	.size	intent, .-intent
	.align	2
	.global	kerent
	.type	kerent, %function
kerent:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0

	# acquire the lr, which is the pc of the active task, temporarily save to r2
	mov r2, lr

	# r1 = spsr of the active task
	mrs r1, spsr

	# change to system state;
	msr cpsr_c, #0xdf

	# push request*(r0), spsr(r1), pc(r2), and other registers of the active task onto its stack
	stmfd sp!, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, fp, ip, lr}

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

	# pop request*(r0), spsr(r1), pc/code(r2) and other saved registers of the active task from its stack
	ldmfd sp!, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, fp, ip, lr}

	# change to svc state;
	msr cpsr_c, #0xd3

	# install the spsr of the active task
	msr spsr, r1

	# *!NOTE: install the pc of the active task (it will also copy spsr to cpsr atomically).
	movs pc, r2

	.size	kerxit, .-kerxit
	.ident	"GCC: (GNU) 4.0.2"
