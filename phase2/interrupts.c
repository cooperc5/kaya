#include "../h/const.h"
#include "../h/types.h"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/initial.e"
#include "../e/exceptions.e"
#include "../e/scheduler.e"
#include "/usr/local/include/umps2/umps/libumps.e"









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
    state_PTR oldInterupt = (state_PTR) INTRUPTOLDAREA;
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





