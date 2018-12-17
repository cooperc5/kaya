#include "../h/const.h"
#include "../h/types.h"
#include "../e/initial.e"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "/usr/local/include/umps2/umps/libumps.e"

pcb_PTR runningProcess;
cpu_t startTOD;
cpu_t currentTOD;

void invokeScheduler() {
    /* are there any ready jobs? */
    if(emptyProcQ(readyQueue)) {
        /* we have no running process */
        currentProcess = NULL;
        /* do we have any job to do? */
        if(processCount == 0) {
            /* nothing to do */
            HALT();
        }
        /* are we waiting on I/O? */
        if(processCount > 0) {
            /* do are we waiting for I/O? */
            if(softBlockedCount == 0) {
                /* not a job, not waiting for a job, 
                and are not a job itself - kernel panic */ 
                PANIC();
                /* are we waiting for I/O? */
            } else if(softBlockedCount > 0) {
                /* enable interrupts for the next job */
                setSTATUS(getSTATUS() | OFF | INTERRUPTSON | IEc | IM);
                /* wait */
                WAIT();
            }
        }
    } else {
        /* simply ready the next job using round-robbin */
        if (currentProcess != NULL) {
            /* start the clock */
            STCK(currentTOD);
            currentProcess->p_time = currentProcess->p_time + (currentTOD - startTOD);
        }
        /* generate an interrupt when timer is up */
        if(currentTOD < QUANTUM) {
            /* our current job will be less than 
            our quantum, take the shorter */
            setTIMER(currentTOD);
        } else {
            /* set the quantum */
            setTIMER(QUANTUM);
        }
        /* grab a job */
        currentProcess = removeProcQ(&(readyQueue));
        STCK(startTOD);
        /* perform a context switch */
        contextSwitch(&(currentProcess->p_s));
    }
}

