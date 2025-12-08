/* Copyright (c) 2005-2006 Russ Cox, MIT; see COPYRIGHT */

#include "taskimpl.h"

#if defined(__APPLE__)
#if defined(__i386__)
#define NEEDX86MAKECONTEXT
#define NEEDSWAPCONTEXT
#elif defined(__x86_64__)
#define NEEDAMD64MAKECONTEXT
#define NEEDSWAPCONTEXT
#else
#define NEEDPOWERMAKECONTEXT
#define NEEDSWAPCONTEXT
#endif
#endif

#if defined(__FreeBSD__) && defined(__i386__) && __FreeBSD__ < 5
#define NEEDX86MAKECONTEXT
#define NEEDSWAPCONTEXT
#endif

#if defined(__OpenBSD__) && defined(__i386__)
#define NEEDX86MAKECONTEXT
#define NEEDSWAPCONTEXT
#endif

#if defined(__linux__) && defined(__arm__)
#define NEEDSWAPCONTEXT
#define NEEDARMMAKECONTEXT
#endif

#if defined(__linux__) && defined(__aarch64__)
#define NEEDSWAPCONTEXT
#define NEEDAARCH64MAKECONTEXT
#endif

#if defined(__linux__) && defined(__mips__)
#define	NEEDSWAPCONTEXT
#define	NEEDMIPSMAKECONTEXT
#endif

#ifdef NEEDPOWERMAKECONTEXT
void
makecontext(ucontext_t *ucp, void (*func)(void), int argc, ...)
{
	ulong *sp, *tos;
	va_list arg;

	tos = (ulong*)ucp->uc_stack.ss_sp+ucp->uc_stack.ss_size/sizeof(ulong);
	sp = tos - 16;
	ucp->mc.pc = (long)func;
	ucp->mc.sp = (long)sp;
	va_start(arg, argc);
	ucp->mc.r3 = va_arg(arg, long);
	va_end(arg);
}
#endif

#ifdef NEEDX86MAKECONTEXT
void
makecontext(ucontext_t *ucp, void (*func)(void), int argc, ...)
{
	int *sp;

	sp = (int*)ucp->uc_stack.ss_sp+ucp->uc_stack.ss_size/4;
	sp -= argc;
	sp = (void*)((uintptr_t)sp - (uintptr_t)sp%16);	/* 16-align for OS X */
	memmove(sp, &argc+1, argc*sizeof(int));

	*--sp = 0;		/* return address */
	ucp->uc_mcontext.mc_eip = (long)func;
	ucp->uc_mcontext.mc_esp = (int)sp;
}
#endif

#ifdef NEEDAMD64MAKECONTEXT
void
makecontext(ucontext_t *ucp, void (*func)(void), int argc, ...)
{
	long *sp;
	va_list va;

	memset(&ucp->uc_mcontext, 0, sizeof ucp->uc_mcontext);
	if(argc != 2)
		*(int*)0 = 0;
	va_start(va, argc);
	ucp->uc_mcontext.mc_rdi = va_arg(va, int);
	ucp->uc_mcontext.mc_rsi = va_arg(va, int);
	va_end(va);
	sp = (long*)ucp->uc_stack.ss_sp+ucp->uc_stack.ss_size/sizeof(long);
	sp -= argc;
	sp = (void*)((uintptr_t)sp - (uintptr_t)sp%16);	/* 16-align for OS X */
	*--sp = 0;	/* return address */
	ucp->uc_mcontext.mc_rip = (long)func;
	ucp->uc_mcontext.mc_rsp = (long)sp;
}
#endif

#ifdef NEEDARMMAKECONTEXT
void
makecontext(ucontext_t *uc, void (*fn)(void), int argc, ...)
{
	int i, *sp;
	va_list arg;

	sp = (int*)uc->uc_stack.ss_sp+uc->uc_stack.ss_size/4;
	va_start(arg, argc);
	for(i=0; i<4 && i<argc; i++)
		uc->uc_mcontext.gregs[i] = va_arg(arg, uint);
	va_end(arg);
	uc->uc_mcontext.gregs[13] = (uint)sp;
	uc->uc_mcontext.gregs[14] = (uint)fn;
}
#endif

#ifdef NEEDAARCH64MAKECONTEXT
void
makecontext(ucontext_t *uc, void (*fn)(void), int argc, ...)
{
    uint64_t *sp;
    va_list arg;
    int i;

    /* Calculate stack pointer - ensure 16-byte alignment */
    sp = (uint64_t*)((char*)uc->uc_stack.ss_sp + uc->uc_stack.ss_size);
    sp = (uint64_t*)((uintptr_t)sp & ~15); /* Align to 16 bytes */

    /* Reserve space for dummy frame record (FP/LR pair) */
    sp -= 2;

    va_start(arg, argc);

    /* Set up arguments in x0-x7 registers */
    for (i = 0; i < 8 && i < argc; i++) {
        uc->uc_mcontext.gregs[i] = va_arg(arg, uint64_t);
    }

    /* If we have more than 8 arguments, they go on the stack */
    if (argc > 8) {
        /* Move sp down for additional arguments */
        sp -= (argc - 8);
        uint64_t *stack_args = sp;
        for (i = 8; i < argc; i++) {
            stack_args[i - 8] = va_arg(arg, uint64_t);
        }
    }

    va_end(arg);

    /* Set up the context:
     * x30 (LR) = fn (function to call)
     * SP = aligned stack pointer
     * PC = fn (starting address)
     */
    uc->uc_mcontext.gregs[30] = (uint64_t)fn;  /* LR = function address */
    uc->uc_mcontext.sp = (uint64_t)sp;         /* Stack pointer */
    uc->uc_mcontext.pc = (uint64_t)fn;         /* Program counter */

    /* Set FP to NULL for clean stack trace */
    uc->uc_mcontext.gregs[29] = 0;             /* FP = NULL */

    /* Set x0 to 0 (like ARM32 sets r0 to 1 in GET) */
    uc->uc_mcontext.gregs[0] = 0;
}
#endif


#ifdef NEEDMIPSMAKECONTEXT
void
makecontext(ucontext_t *uc, void (*fn)(void), int argc, ...)
{
	int i, *sp;
	va_list arg;

	va_start(arg, argc);
	sp = (int*)uc->uc_stack.ss_sp+uc->uc_stack.ss_size/4;
	for(i=0; i<4 && i<argc; i++)
		uc->uc_mcontext.mc_regs[i+4] = va_arg(arg, int);
	va_end(arg);
	uc->uc_mcontext.mc_regs[29] = (int)sp;
	uc->uc_mcontext.mc_regs[31] = (int)fn;
}
#endif

#ifdef NEEDSWAPCONTEXT
int swapcontext(ucontext_t *oucp, const ucontext_t *ucp) {
    // if succeeded, getcontext returns 0
	if(getcontext(oucp) == 0)
		setcontext(ucp);
	return 0;
}
#endif
