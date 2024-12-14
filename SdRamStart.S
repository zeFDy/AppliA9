/*
 * armboot - Startup Code for OMAP3530/ARM Cortex CPU-core
 *
 * Copyright (c) 2004	Texas Instruments <r-woodruff2@ti.com>
 *
 * Copyright (c) 2001	Marius Gröger <mag@sysgo.de>
 * Copyright (c) 2002	Alex Züpke <azu@sysgo.de>
 * Copyright (c) 2002	Gary Jennejohn <garyj@denx.de>
 * Copyright (c) 2003	Richard Woodruff <r-woodruff2@ti.com>
 * Copyright (c) 2003	Kshitij <kshitij@ti.com>
 * Copyright (c) 2006-2008 Syed Mohammed Khasim <x0khasim@ti.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */


/*************************************************************************
 *
 * Startup Code (reset vector)
 *
 * Do important init only if we don't start from memory!
 * Setup memory and board specific bits prior to relocation.
 * Relocate armboot to ram. Setup stack.
 *
 *************************************************************************/
/*
	.globl	_start
	.globl	reset
	.globl 	cpu_init_cp15
*/
	.globl 	SdRamMain
	.globl 	Cpu1CodeStart
	.globl	_start_SdRamPart
	.globl 	_mainCore1Start
	
/*
 *************************************************************************
 *
 * Vectors have their own section so linker script can map them easily
 *
 *************************************************************************
 */
	.section ".vectorsSdRam", "ax"

/*
 *************************************************************************
 *
 * Exception vectors as described in ARM reference manuals
 *
 * Uses indirect branch to allow reaching handlers anywhere in memory.
 *
 *************************************************************************
 */

	.globl	_undefined_instruction_SdRamPart
	.globl	_software_interrupt_SdRamPart
	.globl	_prefetch_abort_SdRamPart
	.globl	_data_abort_SdRamPart
	.globl	_not_used_SdRamPart
	.globl	alt_int_handler_irq
	.globl	_fiq_SdRamPart
	
_start_SdRamPart:

	b	Cpu1CodeStart
	ldr	pc, _undefined_instruction_SdRamPart
	ldr	pc, _software_interrupt_SdRamPart
	ldr	pc, _prefetch_abort_SdRamPart
	ldr	pc, _data_abort_SdRamPart
	ldr	pc, _not_used_SdRamPart
	ldr	pc, _irq_SdRamPart
	ldr	pc, _fiq_SdRamPart


/*
 *************************************************************************
 *
 * Indirect vectors table
 *
 * Symbols referenced here must be defined somewhere else
 *
 *************************************************************************
 */


_undefined_instruction_SdRamPart:	.word undefined_instruction_SdRamPart
_software_interrupt_SdRamPart:		.word software_interrupt_SdRamPart
_prefetch_abort_SdRamPart:			.word prefetch_abort_SdRamPart
_data_abort_SdRamPart:				.word data_abort_SdRamPart
_not_used_SdRamPart:				.word not_used_SdRamPart
_irq_SdRamPart:						.word irq_SdRamPart
_fiq_SdRamPart	:					.word fiq_SdRamPart
									.word 0xdeadbeef


///*
// *************************************************************************
// *
// * Interrupt handling
// *
// *************************************************************************
// */
//
///* SPL interrupt handling: just hang */
//
//

//	.align	5
undefined_instruction_SdRamPart:
software_interrupt_SdRamPart:
prefetch_abort_SdRamPart:
data_abort_SdRamPart:
not_used_SdRamPart:
fiq_SdRamPart:
l_SdRamPart:
	bl	l_SdRamPart			/* hang and never return */

/* section en 0x100 pour lancer le C de l'exec en sdram pour core0 */
	.section ".mainSdRamPart", "ax"

_mainSdRamPart:
/*
 * Set up initial C runtime environment and call board_init_f(0).
 */
	/* Set vector address in CP15 VBAR register */
	ldr	r0, =_start_SdRamPart
	mcr	p15, 0, r0, c12, c0, 0	@Set VBAR

	// Core0 SP
	//ldr	r0, =0x0001E000			// Ok DRAM
	ldr	r0, =0x3FFFF000				// Ok DRAM high - fix me !
	@ ldr	r0, =0xFFFFFF40
	@ ldr	r0, =0xFFFFFF00			// On Chip RAM
	bic	r0, r0, #7	/* 8-byte alignment for ABI compliance */
	mov	sp, r0
	mov	r9, r0
	mov	r0, #0
		
	bl	SdRamMain

	
/* ici c'est reset du CPU1 */
Cpu1CodeStart:
	// /*
	//  * disable interrupts (FIQ and IRQ), also set the cpu to SVC32 mode,
	//  * except if in HYP mode already
	//  */
	// mrs	r0, cpsr
	// and	r1, r0, #0x1f			@ mask mode bits
	// teq	r1, #0x1a				@ test for HYP mode
	// bicne	r0, r0, #0x1f		@ clear all mode bits
	// orrne	r0, r0, #0x13		@ set SVC mode
	// orr	r0, r0, #0xc0			@ disable FIQ and IRQ
	// msr	cpsr,r0
	// 
	// /*
	// * Setup vector:
	// * (OMAP4 spl TEXT_BASE is not 32 byte aligned.
	// * Continue to use ROM code vector only in OMAP4 spl)
	// */
	// 
	// /* Set V=0 in CP15 SCTLR register - for VBAR to point to vector */
	// mrc	p15, 0, r0, c1, c0, 0	@ Read CP15 SCTLR Register
	// bic	r0, #0x2000				@ CR_V = 0x2000		@ V = 0
	// mcr	p15, 0, r0, c1, c0, 0	@ Write CP15 SCTLR Register
	// 
	// /* Set vector address in CP15 VBAR register */
	// ldr	r0, =_start_SdRamPart
	// mcr	p15, 0, r0, c12, c0, 0	@Set VBAR
	// 
	// /* the mask ROM code should have PLL and others stable */
	// bl		cpu_init_cp15_SdRamPart
	// 
	// /*
	//  * Set up initial C runtime environment and call board_init_f(0).
	//  */
	//  
	// /* Set vector address in CP15 VBAR register */
	// ldr	r0, =_start_SdRamPart
	// mcr	p15, 0, r0, c12, c0, 0	@Set VBAR

	// Core1 SP
	//ldr	r0, =0xFFFFF000			// Ok On Chip RAM
	//ldr	r0, =0x0001F000			// Ok DRAM en bas
	ldr	r0, =0x40000000				// Ok haut de DRAM - fix me !
	bic	r0, r0, #7	/* 8-byte alignment for ABI compliance */
	mov	sp, r0
	mov	r9, r0
	mov	r0, #0
		
    //
    // essai de test core0/core1
	// MRC p15, 0, r1, c0, c0, 5	// recuperation du coreX - Read CPU ID Register
	// and r1, #0xFF				// mask uniquement info coreX
	// cmp r1, #1					// running on core1 ?
	// bne JumpCpu1Code				// non jump to JumpCpu1Code
	//	
	// ldr r0, ='1'
	// ldr r1, =0xFFC02000	/* UartData */ 
	// str r0, [r1, #0]
	//
	// ldr r0, LedValue		/* 1<<24 */
	// ldr r1, LedData		/* LedData port in memory */
	// str r0, [r1, #0]
	//JumpCpu1Code:
	//
	
	bl	Cpu1Code
	
	
	/*bl		SdRamMain*/

LedValue:	.word 	0x01000000
LedData:	.word 	0xFF709000

/*------------------------------------------------------------------------------*/

c_runtime_cpu_setup_SdRamPart:

	/*
	* If I-cache is enabled invalidate it
	*/

	mcr	p15, 0, r0, c7, c5, 0	@ invalidate icache
	mcr     p15, 0, r0, c7, c10, 4	@ DSB
	mcr     p15, 0, r0, c7, c5, 4	@ ISB

	bx	lr


/*************************************************************************
 *
 * cpu_init_cp15
 *
 * Setup CP15 registers (cache, MMU, TLBs). The I-cache is turned on unless
 * CONFIG_SYS_ICACHE_OFF is defined.
 *
 *************************************************************************/
cpu_init_cp15_SdRamPart:
	/*
	 * Invalidate L1 I/D
	 */
	mov	r0, #0			@ set up for MCR
	mcr	p15, 0, r0, c8, c7, 0	@ invalidate TLBs
	mcr	p15, 0, r0, c7, c5, 0	@ invalidate icache
	mcr	p15, 0, r0, c7, c5, 6	@ invalidate BP array
	mcr     p15, 0, r0, c7, c10, 4	@ DSB
	mcr     p15, 0, r0, c7, c5, 4	@ ISB

	/*
	 * disable MMU stuff and caches
	 */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #0x00002000	@ clear bits 13 (--V-)
	bic	r0, r0, #0x00000007	@ clear bits 2:0 (-CAM)
	orr	r0, r0, #0x00000002	@ set bit 1 (--A-) Align
	orr	r0, r0, #0x00000800	@ set bit 11 (Z---) BTB
	orr	r0, r0, #0x00001000	@ set bit 12 (I) I-cache
	mcr	p15, 0, r0, c1, c0, 0

	mov	r5, lr			@ Store my Caller
	mrc	p15, 0, r1, c0, c0, 0	@ r1 has Read Main ID Register (MIDR)
	mov	r3, r1, lsr #20		@ get variant field
	and	r3, r3, #0xf		@ r3 has CPU variant
	and	r4, r1, #0xf		@ r4 has CPU revision
	mov	r2, r3, lsl #4		@ shift variant field for combined value
	orr	r2, r4, r2		@ r2 has combined CPU variant + revision

	mov	pc, r5			@ back to my caller



//
// IRQ - set LED & send carac for the moment
//
irq_SdRamPart:	
		push {r0-r7, lr}

		//uint32_t periphbase = get_periphbase();
   		mrc 		p15, 4, r0, c15, c0, 0		// r0 = periphbase;
   		//alt_int_base_cpu  = periphbase + 0x0100;
		//add		r0, r0, #0x0100
		//ldr 		r0, intBaseCpu

		//uint32_t icciar 	= alt_read_word(alt_int_base_cpu + 0xC);
		//add		r0, r0, 0x000C
		ldr 		r1, [r0, #0x010C]
		str 		r1, value
		
		// check the icciar
		// normalement, c'est 0x1D ? (verify with set int)
		// -> ALT_INT_INTERRUPT_PPI_TIMER_PRIVATE = 29 = 0x1D
		// if not an IRQ from Private Timer : exit IRQ (branch to exitIrq)
		ldr 		r0, =#0x3FF
		and 		r2, r1, r0
		//ldr 		r0, =#0x01D
		cmp 		r2, #0x01D
		bne 		badIrq	
		
		//if it is an IRQ from Private Timer : clear the pending int
		//alt_gpt_int_clear_pending(ALT_GPT_CPU_PRIVATE_TMR)
		//-> alt_write_word(ALT_CPU_PRIVATE_TMR_BASE + ALT_CPU_PRIV_TMR_INT_STATUS_REG_OFFSET, ALT_CPU_PRIV_TMR_INT_STATUS);
        /* Clear interrupt status bit by writing 0x00000001 to register */
		
		ldr 	r0, CpuTimeBase			// ALT_CPU_PRIVATE_TMR_BASE
		ldr 	r1, =#0x00000001		// ALT_CPU_PRIV_TMR_INT_STATUS
		str		r1, [r0, #0x0000000C] 	// ALT_CPU_PRIV_TMR_INT_STATUS_REG_OFFSET
		

		//ldr 	r0, =#Counter
		//ldr 	r1, [r0]
		//add	r1, r1, #1
		//str 	r1, [r0]

		//cmp	r1, #1000
		//blt 	exitIrq

		//ldr 	r1, =#0
		//str	r1, [r0]

		//ldr 	r0, LedValue	/* 1<<24 */
		//ldr 	r1, LedData		/* LedData port in memory */
		//str 	r0, [r1, #0]

		//ldr 	r0, UartValue	/* 'U' */
		//ldr 	r1, UartData	/* UartData port in memory */
		//str 	r0, [r1, #0]
		
		// les IRQ passent par ici 
		//b 	exitIrq
		
	    //
		// test core0/core1
		MRC 	p15, 0, r1, c0, c0, 5	// recuperation du coreX - Read CPU ID Register
		and 	r1, #0xFF				// mask uniquement info coreX
		cmp 	r1, #1					// running on core1 ?
		
		bne 	JumpCore0Code			// non jump to JumpCore0Code
		
		ldr 	r0, =#CounterCore1
		ldr 	r1, [r0]
		add		r1, r1, #1
		str 	r1, [r0]

		cmp		r1, #1000
		blt 	exitIrq

		ldr 	r1, =#0
		str		r1, [r0]		

		ldr 	r1, =#IrqMessage
		bl 		displayMsg
		ldr 	r0, ='1'				/* envoi 1 sur RS232 pour Core1 */
		ldr 	r1, =0xFFC02000			/* UartData */ 
		str 	r0, [r1, #0]
		
		ldr 	r0, =10
		str 	r0, [r1, #0]
		ldr 	r0, =13
		str 	r0, [r1, #0]

		//bl	myIrqRoutine
		b 		exitIrq
		
	JumpCore0Code:
		
		ldr 	r0, =#CounterCore0
		ldr 	r1, [r0]
		add		r1, r1, #1
		str 	r1, [r0]

		cmp	r1, #1000
		blt 	exitIrq

		ldr 	r1, =#0
		str		r1, [r0]		

		ldr 	r1, =#IrqMessage
		bl 		displayMsg
		ldr 	r0, ='0'				/* envoi 0 sur RS232 pour Core0 */
		ldr 	r1, =0xFFC02000			/* UartData */ 
		str 	r0, [r1, #0]
			
		ldr 	r0, =10
		str 	r0, [r1, #0]
		ldr 	r0, =13
		str 	r0, [r1, #0]
		//bl	myIrqRoutine

	exitIrq:
	
		//bx 	lr
		//bl	alt_int_handler_irq
		
		
		//alt_write_word(alt_int_base_cpu + 0x10, icciar); /* icceoir */

		//ldr 	r0, intBaseCpu
		//ldr 	r1,	value
		//str	r1, [r0, #0x0010]

		mrc		p15, 4, r0, c15, c0, 0		// r0 = periphbase;
   		ldr 	r1, value
		str 	r1, [r0, #0x0110]
		

		pop		{R0-R7, LR}
		subs	PC, LR, #4

lirq_SdRamPart:
		bl	lirq_SdRamPart				/* on boucle ici (pour le moment) */


badIrq:				//mov 	r0, r1 						// icciar
					//ldr 	r0, =0xDEADBEEF
					//bl	displayHexa32
					//ldr 	r1, =#BadIrqMessage
					//bl	displayMsg
					//bl	myIrqRoutine
					b 		exitIrq	


displayMsg:			ldr 	r2, =0xFFC02000			/* UartData */ 
displayMsgLoop:		ldrb 	r0, [r1],#1
					//and 	r0, #0xFF
					cmp 	r0, #0
					beq 	displayMsgEnd
					str 	r0, [r2, #0]
					//add 	r1, r1, #1
					b 		displayMsgLoop
displayMsgEnd:		bx		lr

displayNibble:		// in R0
					add 	r0, r0, #0x30
					cmp 	r0, #0x39
					ble 	okNibbleIsNumber
					add 	r0, r0, #0x07
okNibbleIsNumber:	ldr 	r1, =0xFFC02000			/* UartData */ 
					str 	r0, [r1, #0]
displayNibbleEnd:	bx		lr

displayHexa32:		// in r0
					mov 	r2, r0

					ldr 	r0, ='0'
					ldr 	r1, =0xFFC02000			/* UartData */ 
					str 	r0, [r1, #0]

					ldr 	r0, ='x'
					ldr 	r1, =0xFFC02000			/* UartData */ 
					str 	r0, [r1, #0]
					
					mov 	r0, r2
					lsr 	r0, #24+4
					and 	r0, #0x0f
					bl 		displayNibble

					mov 	r0, r2
					lsr 	r0, #24
					and 	r0, #0x0f
					bl 		displayNibble

					mov 	r0, r2
					lsr 	r0, #16+4
					and 	r0, #0x0f
					bl 		displayNibble

					mov 	r0, r2
					lsr 	r0, #16
					and 	r0, #0x0f
					bl 		displayNibble

					mov 	r0, r2
					lsr 	r0, #8+4
					and 	r0, #0x0f
					bl 		displayNibble

					mov 	r0, r2
					lsr 	r0, #8
					and 	r0, #0x0f
					bl 		displayNibble

					mov 	r0, r2
					lsr 	r0, #0+4
					and 	r0, #0x0f
					bl 		displayNibble

					mov 	r0, r2
					//lsr 	r0, #0
					and 	r0, #0x0f
					bl 		displayNibble

displayHexa32End:	bx		lr



UartValue:		.word 	'U'
UartData:		.word	0xFFC02000
intBaseCpu:		.word 	0xFFFEC100
CpuTimeBase:	.word	0xFFFEC600
value:			.word 	0
CounterCore0:	.word 	0
CounterCore1:	.word 	0
IrqMessage:		.asciz 	"IRQ on Core #"
BadIrqMessage:	.asciz 	"Unkown IRQ !\r\n"