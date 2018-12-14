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

void debugA(int a){
	int i;
	i = 0;
}

void syscallHandler() {
	debugA(19);
	state_PTR oldState = (state_PTR) SYSCALLOLDAREA;
	debugA(21);
	oldState->s_pc = oldState->s_pc + 4;
	
	unsigned int sysCallNumber = oldState->s_a0;
	unsigned int status = oldState->s_status;
	
	int userMode = FALSE;

	if((status & KUp) != OFF) {
        /* in user mode */
        userMode = TRUE;
	}
	debugA(33);
	if (sysCallNumber <= 8 && sysCallNumber > 0 && userMode) {
		state_PTR oldProgram = (state_PTR) PRGMTRAPOLDAREA;
		copyState(oldState, oldProgram);
		/* change ExcCode in cause register */
		unsigned int oldCause = oldProgram->s_cause;
		oldCause = (oldCause & BLANKEXCCODE);
		oldCause = (oldCause | EXC_RI);
		oldProgram->s_cause	= oldCause;

		programTrapHandler();
		return;
	}
	/* kernel mode is on, handle syscalls 1-8 */
	switch (sysCallNumber) {
        case CREATEPROCESS: /* SYSCALL 1 */
            createProcess(oldState);
            break;
        case TERMINATEPROCESS: /* SYSCALL 2 */
            terminateProcess();
            break;
        case VERHOGEN: /* SYSCALL 3 */
            verhogen(oldState);
            break;
        case PASSEREN: /* SYSCALL 4 */
            passeren(oldState);
            break;
        case SPECIFYEXCEPTIONSTATEVECTOR: /* SYSCALL 5 */
            specifyExceptionsStateVector(oldState);
            break;
        case GETCPUTIME: /* SYSCALL 6 */
            getCpuTime(oldState);
            break;
        case WAITFORCLOCK: /* SYSCALL 7 */
            waitForClock(oldState);
            break;
        case WAITFORIODEVICE: /* SYSCALL 8 */
            waitForIODevice(oldState);
            break;
        default:
			passUpOrDie(SYSTRAP, oldState);
	}
}

HIDDEN void createProcess(state_PTR callerState) {
	state_PTR newState = (state_PTR) callerState->s_a1;
	pcb_PTR newPcb = allocPcb();
	if (newPcb == NULL) { /* no free pcbs */
		currentProcess->p_s.s_v0 = -1;
		LDST(callerState);
		return;
	}
	copyState(newState, &(newPcb->p_s));
	insertChild(currentProcess, newPcb);
	insertProcQ(&(readyQueue), newPcb);

	currentProcess->p_s.s_v0 = 0;
	processCount++;

	LDST(callerState);
}

HIDDEN void terminateProcess() {
	if (!emptyChild(currentProcess)) {
		terminateProgeny(currentProcess);
	}

	outChild(currentProcess);
	
	freePcb(currentProcess);
	processCount--;

	scheduler();
}

void copyState(state_PTR oldState, state_PTR destState){
	/* copy over all state fields */
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
			terminateProgeny(removeChild(p));
			processCount--;
	}
	/* n-1 processes left */
    
    /* check of the pcb_t has a semaphore address */
    if (p->p_semAdd != NULL) {
        /* get the semaphore */
        int* sem = p->p_semAdd;
        /* call outblocked on the pcb_t */
        outBlocked(p);
        /* if the semaphore greater than 0 and less than 48, then
        it is a device semapore */
        if(sem >= &(devSemdTable[0]) && sem <= &(devSemdTable[CLOCK])) {
            /* we have 1 less waiting process */
            softBlockedCount--;
        } else {
            /* not a device semaphore */
            (*sem)++;
        }
    } else if(p == currentProcess){
         /* yank the process from the parent */
         outChild(currentProcess);
    } else {
         /* not on asl nor currP so must be on ready q */
         outProcQ(&(readyQueue), p);
    }
	freePcb(p);
	processCount--;
}

HIDDEN void passUpOrDie(int callNumber, state_PTR old) {
    switch(callNumber) { 
        case SYSTRAP:
            if(currentProcess->newSys != NULL) { /* newSYS hasn't been written to previously */
                copyState(old, currentProcess->oldSys);
                LDST(currentProcess->newSys);
            }
            break;
        case TLBTRAP:
            if(currentProcess->newTLB != NULL) { /* newTLB hasn't been written to previously */
                copyState(old, currentProcess->oldTLB);
                LDST(currentProcess->newTLB);
            }
            break;
        case PROGTRAP: 
            if(currentProcess->newPgm != NULL) { /* newPGM hasn't been written to previously */
                copyState(old, currentProcess->oldPgm);
                LDST(currentProcess->newPgm);
            break;
        }
    }
    terminateProcess(); /* new area was already written to, kill the process */
}

HIDDEN void waitForIODevice(state_PTR state) {
    /* get the line from a1 */
    int line = state->s_a1;
    /* get the device number in the a2 register */
    int device = state->s_a2; 
    /* set the terminal read/write flag to be the contents of a3 */
    int read = (state->s_a3 == TRUE);
    /* is the device in the range of 3-8? if not kill the process */
    if(line > TERMINT || line < DISKINT) {
        terminateProcess();
    }
    /* find the corresponding semaphore for the device */
    int i = findSemaphore(line, device, read);
    int* sem = &(devSemdTable[i]);
    /* perform a P operation */
    (*sem)--;
    if((*sem) < 0) {
        /* block the current process */
        insertBlocked(sem, currentProcess);
        /* copy the old syscall area to the new pcb_t state_t */
        copyState(state, &(currentProcess->p_s));
        /* we have 1 more waiting process */
        softBlockedCount++;
        /* get a new process */
        scheduler();
    }
    /* if no P operation can be done, simply context switch */
    LDST(state);
}

HIDDEN int findSemaphore(int line, int device, int flag) {
    int offset;
    /* terminal read? */
    if(flag == TRUE) {
        /* index with the flag offset */
        offset = (line - NOSEM + flag); 
    } else {
        /* index no flag offset */
        offset = (line - NOSEM);
    }
    /* get the index from the offset and the deice number */
    return (DEVPERINT * offset) + device;
}

HIDDEN void waitForClock(state_PTR state) {
     /* get the semaphore index of the clock timer */
     int *sem = (int*) &(devSemdTable[CLOCK]);
     /* perform a passeren operation */
     (*sem)--;
     if ((*sem) < 0)
     {
         /* block the process */
         insertBlocked(sem, currentProcess);
         /* copy from the old syscall area into the new pcb_state */
         copyState(state, &(currentProcess->p_s));
         /* increment the number of waiting processes */
         softBlockedCount++;
     }
     scheduler();
}

HIDDEN void getCpuTime(state_PTR state) {
        /* copy the state from the old syscall into the pcb_t's state */
        copyState(state, &(currentProcess->p_s));
        /* the clock can be started by placing a new value in the 
        STCK ROM function */
        cpu_t stopTOD;
        /* start the clock  for the stop */ 
        STCK(stopTOD);
        /* get the time that has passed */
        cpu_t elapsedTime = stopTOD - startTOD;
        currentProcess->p_time = (currentProcess->p_time) + elapsedTime;
        /* store the state in the pcb_t's v0 register */
        currentProcess->p_s.s_v0 = currentProcess->p_time;
        /* start the clock for the start TOD */
        STCK(startTOD);
        LDST(&(currentProcess->p_s));
}

HIDDEN void specifyExceptionsStateVector(state_PTR state) {
    /* get the exception from the a1 register */
    switch(state->s_a1) {
        /* check if the specified exception is a translation 
        look aside buffer exception */
        case TLBTRAP:
            /* if the new tlb has already been set up,
            kill the process */
            if(currentProcess->newTLB != NULL) {
                terminateProcess();
            }
            /* store the syscall area state in the new tlb */
            currentProcess->newTLB = (state_PTR) state->s_a3;
            /* store the syscall area state in the old tlb*/
            currentProcess->oldTLB = (state_PTR) state->s_a2;
            break;
        case PROGTRAP:
            /* if the new pgm has already been set up,
            kill the process */
            if(currentProcess->newPgm != NULL) {
                terminateProcess();
            }
            /* store the syscall area state in the new pgm */
            currentProcess->newPgm = (state_PTR) state->s_a3;
            currentProcess->oldPgm = (state_PTR) state->s_a2;
            break;
        case SYSTRAP:
            /* if the new systrap has already been set up,
            kill the process */
            if(currentProcess->newSys != NULL) {
                terminateProcess();
            }
            /* store the syscall area state in the new pgm */
            currentProcess->newSys = (state_PTR) state->s_a3;
            /* store the syscall area state in the old pgm*/
            currentProcess->oldSys = (state_PTR) state->s_a2;
            break;
    }
    LDST(state);
}

HIDDEN void passeren(state_PTR state) {
    /* get the semaphore in the s_a1 */
    int *sem = (int*) state->s_a1;
    /* decrement teh semaphore */
    (*(sem))--;
    if ((*(sem)) < 0) {
        cpu_t stopTOD;
        STCK(stopTOD);
        /*Store elapsed time*/
        int elapsedTime = stopTOD - startTOD;
        /* add the elapsed time to the current process */
        currentProcess->p_time = currentProcess->p_time + elapsedTime;
        /* copy from the old syscall area to the new process's state */
        copyState(state, &(currentProcess->p_s));
        /* the process now must wait */
        insertBlocked(sem, currentProcess);
        /* get a new job */
        scheduler();
    }
    /* if the semaphore is not less than zero, do not 
    block the process, just load the new state */
    LDST(state);
}

HIDDEN void verhogen(state_PTR callerState) {
    /* the semaphore is placed in the a1 register of the 
    passed in state_t */
    int* sem = (int*) callerState->s_a1;
    /* increment the semaphore - the V operation on 
    the semaphore */
    (*(sem))++;
    /* if the synchronization semaphore description is <= 0, 
    then it will remove the process from the blocked processes 
    and place it in the ready queue - which synchronizes the processes */
    if(*(sem) <= 0) {
        /* unblock the next process */
        pcb_PTR newProcess = removeBlocked(sem);
        /* the current process is then placed in the ready 
        queue - baring its not null */
        if(newProcess != NULL) {
            /* place it in the ready queue */
            insertProcQ(&(readyQueue), newProcess);
        }
    }
    /* perform a context switch on the requested process */
    LDST(state);
}

void programTrapHandler() {
    /* get the area in memory */
    state_PTR oldState = (state_PTR) PRGMTRAPOLDAREA;
    /* pass up the process to its appropriate handler
    or kill it */
    passUpOrDie(PROGTRAP, oldState);
}

void translationLookasideBufferHandler() {
    /* get the area in memory */
    state_PTR oldState = (state_PTR)TBLMGMTOLDAREA;
    /* pass up the process to its appropriate handler
    or kill it */
    passUpOrDie(TLBTRAP, oldState);
}



