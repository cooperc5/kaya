#include "../h/const.h"
#include "../h/types.h"
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

	if (sysCallNumber >= 9 && sysCallNumber <= 255) {
		passUpOrDie( sysCallNumber, oldState);
	}

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
	else if (sysCallNumber <= 8 && sysCallNumber > 0 && !userMode) {
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
}

HIDDEN void createProcess(state_PTR callerState) {
	state_PTR newState = (state_PTR) callerState->s_a1;
	pcb_PTR newPcb = allocPcb();
	

	if (newPcb == NULL) { /* no free pcbs */
		currentProcess->p_s.s_v0 = -1;
		LDST(callerState);
	}
	processCount++;
	copyState(newState, &(newPcb->p_s));
	insertChild(currentProcess, newPcb);
	insertProcQ(&(readyQueue), newPcb);

	currentProcess->p_s.s_v0 = 0;
	

	LDST(callerState);
}

HIDDEN void terminateProcess() {
	death(currentProcess);
	
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

HIDDEN void death(pcb_PTR p) {
	while (!emptyChild(p)) {
			death(removeChild(p));
	}

	if (p == currentProcess) {
		outChild(p);
		return;
	}
    
	if (p->p_semAdd == NULL) { /* p is on readyQ */
		outProcQ(&(readyQueue), p);
		return;
	}

    if (p->p_semAdd != NULL) {
        int* sem = p->p_semAdd;
        outBlocked(p);
        /* is it blocked on a device semaphore? */
        if(sem >= &(devSemdTable[0]) && sem <= &(devSemdTable[CLOCK])) {
            softBlockedCount--;
        } else {
            /* on normal sem */
            (*sem)++;
        }
    }
}

HIDDEN void passUpOrDie(int callNumber, state_PTR old) {
    switch(callNumber) { 
        case SYSTRAP:
            if(currentProcess->newSys != NULL) { /* newSYS has been written to previously */
                copyState(old, currentProcess->oldSys);
                LDST(currentProcess->newSys);
            }
            break;
        case TLBTRAP:
            if(currentProcess->newTLB != NULL) { /* newTLB has been written to previously */
                copyState(old, currentProcess->oldTLB);
                LDST(currentProcess->newTLB);
            }
            break;
        case PROGTRAP: 
            if(currentProcess->newPgm != NULL) { /* newPGM has been written to previously */
                copyState(old, currentProcess->oldPgm);
                LDST(currentProcess->newPgm);
            break;
        }
    }
    terminateProcess(); /* new area was already written to, kill the process */
}

HIDDEN void waitForIODevice(state_PTR state) {
	int line = state->s_a1;
    int device = state->s_a2; 
    int read = (state->s_a3 == TRUE);
    int devSemIndex;
	if (line > 2 && line < 8) { /* normal devices */
        
        if (line != 7) {
        	devSemIndex = DEVPERINT * (line - MAINDEVOFFSET) + device;
        }
        else {
        	devSemIndex = DEVPERINT * (line - MAINDEVOFFSET + read) + device;
        }

        devSemdTable[devSemIndex]--;

        if (devSemdTable[devSemIndex] < 0) {
            insertBlocked(&(devSemdTable[devSemIndex]), currentProcess);
            copyState(state, &(currentProcess->p_s));
            softBlockedCount++;
            scheduler();
        }
        else {
        	LDST(state);
        }
    }
    else {
    	terminateProcess();
    }
}

HIDDEN void waitForClock(state_PTR state) {
     /* get clock sem index */
     int *sem = (int*) &(devSemdTable[CLOCK]);
     /* p the clock sem */
     (*sem)--;
     if ((*sem) < 0)
     {
         /* block the process */
         insertBlocked(sem, currentProcess);
         softBlockedCount++;
     }
     scheduler();
}

HIDDEN void getCpuTime(state_PTR state) {
        copyState(state, &(currentProcess->p_s));
        cpu_t stopTOD;
        STCK(stopTOD);
        cpu_t elapsedTime = stopTOD - startTOD;
        currentProcess->p_time = (currentProcess->p_time) + elapsedTime;
        currentProcess->p_s.s_v0 = currentProcess->p_time;
        STCK(startTOD);
        LDST(&(currentProcess->p_s));
}

HIDDEN void specifyExceptionsStateVector(state_PTR state) {
    switch(state->s_a1) {
        case TLBTRAP:
            if(currentProcess->newTLB != NULL) {
                terminateProcess();
            }

            currentProcess->oldTLB = (state_PTR) state->s_a2;
            currentProcess->newTLB = (state_PTR) state->s_a3;
            break;

        case PROGTRAP:
            if(currentProcess->newPgm != NULL) {
                terminateProcess();
            }

            currentProcess->oldPgm = (state_PTR) state->s_a2;
            currentProcess->newPgm = (state_PTR) state->s_a3;
            break;

        case SYSTRAP:
            if(currentProcess->newSys != NULL) {
                terminateProcess();
            }

            currentProcess->oldSys = (state_PTR) state->s_a2;
            currentProcess->newSys = (state_PTR) state->s_a3;
            break;

    }
    LDST(state);
}

HIDDEN void passeren(state_PTR state) {
    int *sem = (int*) state->s_a1;
    (*sem)--;
    if (*(sem) < 0) {
        cpu_t stopTOD;
        STCK(stopTOD);
        int elapsedTime = stopTOD - startTOD;
        currentProcess->p_time = currentProcess->p_time + elapsedTime;
        insertBlocked(sem, currentProcess);
        scheduler();
    }
    LDST(state);
}

HIDDEN void verhogen(state_PTR callerState) {
    int* sem = (int*) callerState->s_a1;

    (*sem)++;

    if(*(sem) <= 0) {
        pcb_PTR newProcess = removeBlocked(sem);
        if(newProcess != NULL) {
            insertProcQ(&(readyQueue), newProcess);
        }
    }
    LDST(callerState);
}

void programTrapHandler() {
    state_PTR oldState = (state_PTR) PRGMTRAPOLDAREA;
    passUpOrDie(PROGTRAP, oldState);
}

void translationLookasideBufferHandler() {
    state_PTR oldState = (state_PTR)TBLMGMTOLDAREA;
    passUpOrDie(TLBTRAP, oldState);
}



