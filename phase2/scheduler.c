#include "../h/const.h"
#include "../h/types.h"
#include "../e/initial.e"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "/usr/local/include/umps2/umps/libumps.e"

pcb_PTR runningProcess;
cpu_t startTOD;
cpu_t currentTOD;

extern void scheduler() {
	if (emptyProcQ(readyQueue)) {
		if (processCount == 0) {
			currentProcess = NULL;
        	/* do we have any job to do? */
       		if(processCount == 0) {
            /* nothing to do */
            	HALT();
			}
			return;
		}
		if (processCount > 0) {
			if (softBlockedCount == 0) {
				PANIC();
				return;
			}
			if (softBlockCount > 0) {
				/* modify status */
				setSTATUS(getSTATUS() | OFF | INTERRUPTSON | IEc | IM);
				WAIT();
				return;
			}
		}
	

	} else {
        /* use round robin for next job */
        if (currentProcess != NULL) {
            /* start the clock */
            STCK(currentTOD);
            currentProcess->p_time = currentProcess->p_time + (currentTOD - startTOD);
        }
        /* set interrupt timer */
        if(currentTOD < QUANTUM) {
            setTIMER(currentTOD);
        } else {
            /* set the quantum */
            setTIMER(QUANTUM);
        }
        /* grab a job */
        currentProcess = removeProcQ(&(readyQueue));
        STCK(startTOD);
        /* perform a context switch */
        LDST(&(currentProcess->p_s));
	}
}

