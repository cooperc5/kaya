/* exceptions .c */

#include "../h/const.h"
#include "../h/types.h"
/* e files to include */
#include "../e/initial.e"
#include "../e/scheduler.e"
#include "../e/interrupts.e"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "/usr/local/include/umps2/umps/libumps.e"

void syscallHandler() {
	state_PTR oldState = (state_PTR) SYSCALLOLDAREA;

	oldState->s_pc = oldState->s_pc + 4;
	
	unsigned int sysCallNumber = oldState->s_a0;
	unsigned int status = oldState->s_status;

	if (sysCallNumber > 8 && sysCallNumber >= 255 && sysCallNumber != 0) {
		passUpOrDie(); /* implement */
	}

	/* check for kernel mode before case statement */
	if (CHECK_BIT(status, 3)) { /* checks KUp bit */
		/* user mode so generate priveleged instruction program trap */
		state_PTR oldProgram = (state_PTR) PRGMTRAPOLDAREA;
		copyState(oldState, oldProgram);
		/* change ExcCode in cause register */
		unsigned int oldCause = oldProgram->s_cause;
		oldCause = (oldCause & BLANKEXCCODE);
		oldCause = (oldCause | EXC_RI);
		oldProgram->s_cause	= oldCause;

		programTrapHandler(); /*fix args later */
	}
	/* kernel mode is on, handle syscalls 1-8 */
	switch (sysCallNumber) {
        case CREATEPROCESS: /* SYSCALL 1 */
            createProcess(caller);
            break;
        case TERMINATEPROCESS: /* SYSCALL 2 */
            terminateProcess();
            break;
        case VERHOGEN: /* SYSCALL 3 */
            verhogen(caller);
            break;
        case PASSEREN: /* SYSCALL 4 */
            passeren(caller);
            break;
        case SPECIFYEXCEPTIONSTATEVECTOR: /* SYSCALL 5 */
            specifyExceptionsStateVector(caller);
            break;
        case GETCPUTIME: /* SYSCALL 6 */
            getCpuTime(caller);
            break;
        case WAITFORCLOCK: /* SYSCALL 7 */
            waitForClock(caller);
            break;
        case WAITFORIODEVICE: /* SYSCALL 8 */
            waitForIODevice(caller);
            break;
        default:
			passUpOrDie(SYSTRAP, caller);
	}
}

HIDDEN void createProcess() {
	state_t newState = currentProcess->p_s.s_a1;
	newPcb = allocPcb();
	if (newPcb == NULL) { /* no free pcbs */
		currentProcess->p_s.s_v0 = -1;
		return;
	}
	copyState(&newState, &(newPcb->p_s));
	insertChild(currentProcess, newPcb);

	currentProcess->p_s.s_v0 = 0;
	processCount++;
}

HIDDEN void terminateProcess() {
	terminateProgeny(currentProcess);
	freePcb(outProcQ(&readyQueue, currentProcess))
	processCount--;
}

HIDDEN void copyState(state_PTR oldState, state_PTR destState){
	destState->s_asid = oldState->s_asid;
	destState->s_cause = oldState->s_cause;
	destState->s_status = oldState->s_status;
	destState->s_pc = oldState->s_pc;

	int i;
	for (i = 0; i < STATEREGNUM; i++) {
		destState->s_reg[i] = oldState->s_reg[i];
	}
}

HIDDEN void terminateProgeny(pcb_PTR p) {
	while (!emptyChild(p)) {
		if (p->p_child->p_child != NULL) {
			terminateProgeny(p->p_child->p_child);
		}
		else {
			freePcb(removeChild(p));
		}
	}
}