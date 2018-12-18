#include "../h/const.h"
#include "../h/types.h"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/initial.e"
#include "../e/exceptions.e"
#include "../e/scheduler.e"
#include "/usr/local/include/umps2/umps/libumps.e"


void debugInterrupt(int a) {
    int i;
    i = 0;
}
HIDDEN int getLine(unsigned int cause) {
    int found = FALSE;
    int line = -1;
    while (!found && line < 7) {
        line++;
        if (CHECK_BIT(cause, line + 8)) {
            found == TRUE;
        }
    }
    return line;
}

HIDDEN int getDevice(int line) {
    devregarea_PTR bus = (devregarea_PTR) RAMBASEADDR;
    unsigned int deviceBitMap = bus->interrupt_dev[(line - MAINDEVOFFSET)];
    /* now that we have the device bit map for the appropriate line, check for the lowest order on bit */
    int found = FALSE;
    int device = -1;
    while (!found && device < 7) {
        device++;
        if (CHECK_BIT(deviceBitMap, device)) {
            found == TRUE;
        }
    }
    return device;
}

void interruptHandler() {
    state_PTR oldState = (state_PTR) INTERRUPTOLDAREA;
    unsigned int cause = oldState->s_cause;
    int line = getLine(cause);
    int device = getDevice(line);
    int devSemdIndex;

    cpu_t startTime;
    cpu_t endTime;
    STCK(startTime);

    device_PTR deviceRegister = (device_PTR) (INTDEVREG + ((line - MAINDEVOFFSET) * DEVREGSIZE * DEVPERINT) + (device * DEVREGSIZE));

    int receiving = TRUE; /* assume receiving if terminal */

    if (line == 7) {
        if((deviceRegister->t_transm_status & FULLBYTE) != READY) {
            receiving = FALSE;

            devSemdIndex = DEVPERINT * (line - MAINDEVOFFSET) + device;
        } else {
            /* receiving sem index */
            devSemdIndex = DEVPERINT * ((line - MAINDEVOFFSET) + 1) + device;
        }
    }

    if (line < 8 && line > 2) {
        
    
        if (line < 7 && line > 2) {
            devSemdIndex = DEVPERINT * (line - MAINDEVOFFSET) + device;
        }

        devSemdTable[devSemdIndex]++;

        if (devSemdTable[devSemdIndex] <= 0) {
            pcb_PTR p = removeBlocked(&(devSemdTable[devSemdIndex]));
            if(receiving && (line == 7)) {
                p->p_s.s_v0 = deviceRegister->t_recv_status;
                /* ack the receive */
                deviceRegister->t_recv_command = ACK;
            } else if(!receiving && (line == 7)) {
                p->p_s.s_v0 = deviceRegister->t_transm_status;
                /* ack the transmission */
                deviceRegister->t_transm_command = ACK;
            } else {
                p->p_s.s_v0 = deviceRegister->d_status;
                /* ack normal dev */
                deviceRegister->d_command = ACK;
            }
            
            
            insertProcQ(&(readyQueue), p);
            softBlockedCount--;
            /* state_PTR oldInt = (state_PTR) INTERRUPTOLDAREA; */
            /* if previous process was in a wait state, call the scheduler here 
            if (waitState) {      but how the hell do i figure out if it's a wait state? can't find wait bit!!!
                scheduler();
            } */
            /* LDST(oldInt); */
            /* scheduler(); */
        }
        
    }
    if (line == 0) {
        PANIC();
    }
    if (line == 1) {
        handleTime(startTime);
    }
    if (line == 2) {
        psuedoClockHandler(startTime, endTime);
    }

    handleTime(startTime);
    
}

HIDDEN void psuedoClockHandler(cpu_t startTime, cpu_t endTime) {
    LDIT(INTERVAL);

    int *sem = &(devSemdTable[CLOCK]);
    (*sem) = 0;

    pcb_PTR blocked = headBlocked(sem);
    while(blocked != NULL) {

        pcb_PTR p = removeBlocked(sem);
        STCK(endTime);

        if(p != NULL) {
            softBlockedCount--;

            cpu_t elapsedTime = (endTime - startTime);
            (p->p_time) = (p->p_time) + elapsedTime;

            insertProcQ(&(readyQueue), p);
            
            blocked = headBlocked(sem);
        }
    }
    handleTime(startTime);
}

HIDDEN void handleTime(cpu_t startTime) {
    debugInterrupt(146);
    state_PTR oldState = (state_PTR) INTERRUPTOLDAREA;

    if(currentProcess != NULL) {
        
        cpu_t endTime;
        STCK(endTime);
        cpu_t elapsedTime = (endTime - startTime);
        startTOD = startTOD + elapsedTime;

        copyState(oldState, &(currentProcess->p_s));

        insertProcQ(&(readyQueue), currentProcess);
    }
    scheduler();
}