/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

// check if has error code : http://pdos.csail.mit.edu/6.828/2012/readings/i386/s09_06.htm
// or http://pdos.csail.mit.edu/6.828/2012/readings/ia32/IA32-3A.pdf Chapter 5,
// table 5-1
TRAPHANDLER_NOEC(idt_divide, T_DIVIDE) 
TRAPHANDLER_NOEC(idt_debug, T_DEBUG) 
TRAPHANDLER_NOEC(idt_nmi, T_NMI) 
TRAPHANDLER_NOEC(idt_brkpt, T_BRKPT) 
TRAPHANDLER_NOEC(idt_oflow, T_OFLOW) 
TRAPHANDLER_NOEC(idt_bound, T_BOUND) 
TRAPHANDLER_NOEC(idt_illop, T_ILLOP) 
TRAPHANDLER_NOEC(idt_device, T_DEVICE) 
TRAPHANDLER(idt_dblflt, T_DBLFLT)
# idt_9  reserved
TRAPHANDLER(idt_tss, T_TSS)
TRAPHANDLER(idt_segnp, T_SEGNP)
TRAPHANDLER(idt_stack, T_STACK)
TRAPHANDLER(idt_gpflt, T_GPFLT)
TRAPHANDLER(idt_pgflt, T_PGFLT)
# idt_15 reserved
TRAPHANDLER_NOEC(idt_fperr, T_FPERR) 
TRAPHANDLER(idt_align, T_ALIGN) 
TRAPHANDLER_NOEC(idt_mchk, T_MCHK) 
TRAPHANDLER_NOEC(idt_simderr, T_SIMDERR) 

TRAPHANDLER_NOEC(idt_syscall, T_SYSCALL)
TRAPHANDLER(idt_default, T_DEFAULT) 

# H/W IRQ
TRAPHANDLER_NOEC(idt_irq0, IRQ_OFFSET + 0)
TRAPHANDLER_NOEC(idt_irq1, IRQ_OFFSET + 1)
TRAPHANDLER_NOEC(idt_irq2, IRQ_OFFSET + 2)
TRAPHANDLER_NOEC(idt_irq3, IRQ_OFFSET + 3)
TRAPHANDLER_NOEC(idt_irq4, IRQ_OFFSET + 4)
TRAPHANDLER_NOEC(idt_irq5, IRQ_OFFSET + 5)
TRAPHANDLER_NOEC(idt_irq6, IRQ_OFFSET + 6)
TRAPHANDLER_NOEC(idt_irq7, IRQ_OFFSET + 7)
TRAPHANDLER_NOEC(idt_irq8, IRQ_OFFSET + 8)
TRAPHANDLER_NOEC(idt_irq9, IRQ_OFFSET + 9)
TRAPHANDLER_NOEC(idt_irq10, IRQ_OFFSET + 10)
TRAPHANDLER_NOEC(idt_irq11, IRQ_OFFSET + 11)
TRAPHANDLER_NOEC(idt_irq12, IRQ_OFFSET + 12)
TRAPHANDLER_NOEC(idt_irq13, IRQ_OFFSET + 13)
TRAPHANDLER_NOEC(idt_irq14, IRQ_OFFSET + 14)
TRAPHANDLER_NOEC(idt_irq15, IRQ_OFFSET + 15)

/*
 * Lab 3: Your code here for _alltraps
 */
.global _alltraps
_alltraps :
  // Build trap frame as an argument of trap(struct Trapframe *tf)
  // in trap.c , trapno and error code are set up by TRAPHANDLER
  // and TRAPHANDLER_NOEC
  pushl %ds
  pushl %es
  pushal
  
  // Set up data and per-cpu segment , cs and ss are set up by h/w
  movw $(GD_KD), %ax
  movw %ax, %ds
  movw %ax, %es
  //movw %ax, %fs
  //movw %ax, %gs

  // Call trap(struct Trapframe *tr)
  pushl %esp
  call trap


