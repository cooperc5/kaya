#include "../h/const.h"
#include "../h/types.h"
#include "../e/initial.e"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "/usr/local/include/umps2/umps/libumps.e"

pcb_PTR runningProcess;
cpu_t startTOD;
cpu_t currentTOD;

void debugScheduler(int a) {
	int i;
	i=0;
}

extern void scheduler() {
	debugScheduler(13);
	if (emptyProcQ(readyQueue)) {
		currentProcess = NULL;
		if (processCount == 0) {
            	HALT();
		}
		if (processCount > 0) {
			if (softBlockedCount == 0) {
				PANIC();
			}
			if (softBlockedCount > 0) {
				/* modify status */
				setSTATUS(getSTATUS() | OFF | INTERRUPTSON | IEc | IM);
				WAIT();
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
        /* pop a job */
        currentProcess = removeProcQ(&(readyQueue));
        STCK(startTOD);
        
        LDST(&(currentProcess->p_s));
	}
}

