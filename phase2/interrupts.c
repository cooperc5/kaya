#include "../h/const.h"
#include "../h/types.h"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/initial.e"
#include "../e/exceptions.e"
#include "../e/scheduler.e"
#include "/usr/local/include/umps2/umps/libumps.e"

static int getDeviceNumber(int lineNumber) {
    /* get the address of the device bit map. Per secion 5.2.4 of pops, the 
    physical address of the bit map is 0x1000003C. When bit i is in word j is 
    set to one then device i attached to interrupt line j + 3 */
    devregarea_PTR temp = (devregarea_PTR) RAMBASEADDR;
    unsigned int deviceBitMap = temp->interrupt_dev[(lineNumber - MAINDEVOFFSET)];
    /* start at the first device */
    unsigned int candidate = FIRST;
    int deviceNumber;
    /* for searching for the device number */
    /* search each 8 bits */
    for(deviceNumber = 0; deviceNumber < DEVPERINT; deviceNumber++) {
        if((deviceBitMap & candidate) != 0) {
            /* the candidate has been found */
            break;
        } else {
            /* find the next candidate */
            candidate = candidate << 1;
        }
    }
    /* we found the device numner */
    return deviceNumber;
}

/*
* Function: Exit Interrupt Handler
* Ensures that the current process will not be charged for time spent the 
* in the interrupt handler - baring that there is a current process. I
*/
static void exitInterruptHandler(cpu_t startTime) {
    /* do we have a current process? */
    if(currentProcess != NULL) {
        /* get the old interrupt area */
        state_PTR oldInterrupt = (memaddr)INTERRUPTOLDAREA;
        cpu_t endTime;
        /* start the clock by placing a new value in 
        the ROM supported STCK function */
        STCK(endTime);
        /* find the startedTOD */
        cpu_t elapsedTime = (endTime - startTime);
        startTOD = startTOD + elapsedTime;
        /* copy the state from the old interrupt area to the current state */
        copyState(oldInterrupt, &(currentProcess->p_s));
        /* insert the new pricess in the ready queue */
        insertProcQ(&(readyQueue), currentProcess);
    }
    /* get a new process */
    invokeScheduler();
}






static int getDeviceNumber(int lineNumber) {

    devregarea_PTR temp = (devregarea_PTR) RAMBASEADDR;
    unsigned int deviceBitMap = temp->interrupt_dev[(lineNumber - MAINDEVOFFSET)];
    unsigned int candidate = FIRST;
    int deviceNumber;

    for(deviceNumber = 0; deviceNumber < DEVPERINT; deviceNumber++) {
        if((deviceBitMap & candidate) != 0) {
            break;
        } else {
            candidate = candidate << 1;
        }
    }
    return deviceNumber;
}


static void exitInterruptHandler(cpu_t startTime) {
    if(currentProcess != NULL) {
        state_PTR oldInterrupt = (memaddr)INTERRUPTOLDAREA;
        cpu_t endTime;

        STCK(endTime);
        cpu_t elapsedTime = (endTime - startTime);
        startTOD = startTOD + elapsedTime;
        copyState(oldInterrupt, &(currentProcess->p_s));
        insertProcQ(&(readyQueue), currentProcess);
    }
    scheduler();
}

static int getLineNumber(unsigned int cause) {
    
    unsigned int lineNumbers[SEMDEVICE] = {FOURTH, FIFTH, SIXTH, SEVENTH, EIGHTH};
    
    unsigned int devices[SEMDEVICE] = {DISKINT, TAPEINT, NETWINT, PRNTINT, TERMINT};
    int i;
    
    int finding = 0;
    
    for (i = 0; i < SEMDEVICE; i++) {
        if(((cause) & (lineNumbers[i])) != 0) {
            
            finding = devices[i];
        }
    }
    
    return finding;
}


static void intervalTimerHandler(cpu_t startTime, cpu_t endTime) {
    
    LDIT(INTERVAL);
    
    int *semaphore = &(devSemdTable[CLOCK]);
    
    (*semaphore) = 0;
    
    pcb_PTR blocked = headBlocked(semaphore);
    
    while(blocked != NULL) {
        pcb_PTR p = removeBlocked(semaphore);
        STCK(endTime);
        if(p != NULL) {
            
            insertProcQ(&(readyQueue), p);
            
            cpu_t elapsedTime = (endTime - startTime);
            (p->p_time) = (p->p_time) + elapsedTime;
            softBlockedCount--;
            blocked = headBlocked(semaphore);
        }
    }
    exitInterruptHandler(startTime);
}


void interruptHandler() {
    state_PTR oldInterupt = (state_PTR) INTERRUPTOLDAREA;
    device_PTR devReg;
    unsigned int cause = (((oldInterupt->s_cause) & IM) >> IPMASK);
    cpu_t startTime;
    cpu_t endTime;
    STCK(startTime);
    int deviceNumber = 0;
    int lineNumber = 0;
    int i = 0;
    if ((cause & FIRST) != 0) {
        PANIC();
    } else if((cause & SECOND) != 0) {
        exitInterruptHandler(startTime);
    } else if((cause & THIRD) != 0) {
        intervalTimerHandler(startTime, endTime);
    } else {
        lineNumber = getLineNumber(cause);
    }
    deviceNumber = getDeviceNumber(lineNumber);
    devReg = (device_PTR) (INTDEVREG + ((lineNumber - MAINDEVOFFSET) * DEVREGSIZE * DEVPERINT) + (deviceNumber * DEVREGSIZE));
    int receive = TRUE;

    if(lineNumber == TERMINT) {
        if((devReg->t_transm_status & FULLBYTE) != READY) {
            i = DEVPERINT * (lineNumber - MAINDEVOFFSET) + deviceNumber;
            receive = FALSE;
        } else {
            i = DEVPERINT * ((lineNumber - MAINDEVOFFSET) + 1) + deviceNumber;
        }
    } else {
        i = DEVPERINT * (lineNumber - MAINDEVOFFSET) + deviceNumber;
    }
    int *semaphore = &(devSemdTable[i]);
    (*semaphore)++;
    if((*semaphore) <= 0) {
        pcb_PTR p = removeBlocked(semaphore);
        if (p != NULL) {
            if(receive && (lineNumber == TERMINT)) {
                p->p_s.s_v0 = devReg->t_recv_status;
                devReg->t_recv_command = ACK;
            } else if(!receive && (lineNumber == TERMINT)) {
                p->p_s.s_v0 = devReg->t_transm_status;
                devReg->t_transm_command = ACK;
            } else {
                p->p_s.s_v0 = devReg->d_status;
                devReg->d_command = ACK;
            }
            softBlockedCount--;
            insertProcQ(&(readyQueue), p);
        }
    }
    exitInterruptHandler(startTime);
}











