/*************************************************** interrupts.c ********************************************************
	Handles the interrupts that occur in the Kaya OS. When an interrupt occurs, assuming that the interrupt bit is 
    turned on (otherwise an interrupt cannot occur), the interval handler will be invoked to dermine the cause of 
    the interrupt, as well as the appropriate actions to be taken henceforth. The cause of the interrupt can either 
    be a device that requires to be acknowledged as part of umps2's handshake protocol, or for from a clock interrupt
    caused by either a quantum ending or a psuedo clock timer. For semaphore devices, i.e. a disk, tape, network, printer 
    or terminal device, causes an interupt, a V operation is performed on that device's semaphore and implements the 
    shandshake. Furthermore, for all devices, the interrupt handler will insure that running processes' will not be
    charged for time spent in the the interupt handler. 
    This module contributes function definitions and a few sample fucntion implementations to the contributors put 
    forth by the Kaya OS project.
***************************************************** interrupts.c ******************************************************/

/* h files to include */
#include "../h/const.h"
#include "../h/types.h"
/* e files to include */
#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/initial.e"
#include "../e/exceptions.e"
#include "../e/scheduler.e"
/* include the µmps2 library */
#include "/usr/local/include/umps2/umps/libumps.e"


/************************************************************************************************************************/
/******************************************** HELPER FUNCTIONS  *********************************************************/
/************************************************************************************************************************/

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
        /* check if the transmission status is recieve command */
        if((deviceRegister->t_transm_status & FULLBYTE) != READY) {
            /* get sem index for transmitting */
            devSemdIndex = DEVPERINT * (line - MAINDEVOFFSET) + device;
            /* mark the flag as false - turn off recieve */
            receiving = FALSE;
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
            state_PTR oldInt = (state_PTR) INTERRUPTOLDAREA;
            /* if previous process was in a wait state, call the scheduler here 
            if (waitState) {      but how the hell do i figure out if it's a wait state? can't find wait bit!!!
                scheduler();
            } */
            LDST(oldInt);
        }
        
    }
    if (line == 0) {
        PANIC();
    }
    if (line == 1) {
        exitInterruptHandler(startTime);
    }
    if (line == 2) {
        intervalTimerHandler(startTime, endTime);
    }

    exitInterruptHandler(startTime);
    
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
        cpu_t endTime = STCK(endTime);
        /* start the clock by placing a new value in 
        the ROM supported STCK function */
        /* find the startedTOD */
        cpu_t elapsedTime = (endTime - startTime);
        startTOD = startTOD + elapsedTime;
        /* copy the state from the old interrupt area to the current state */
        copyState(oldInterrupt, &(currentProcess->p_s));
        /* insert the new process in the ready queue */
        insertProcQ(&(readyQueue), currentProcess);
    }
    /* get a new process */
    scheduler();
}

/************************************************************************************************************************/
/******************************************** INTERVAL TIMER HANDLER*****************************************************/
/************************************************************************************************************************/

/*
* Function: Interval Timer Handler
* Handles the interval timer which serves as a pseudo-clock. If left unadjusted, 
* the pseudo-clock will grow by 1 every 100 milliseconds - which represents a clock 
* tick. When a wait for clock is requested inbetween to adjacent clock ticks, the the interval 
* timer handler will load a new 100 millisecond intervals, resets the interval timer's
* semaphore device, and performs a V operation on all of the blocked processes. Finally, 
* it exits the interrupt handler. Takes in the start time and the end time to properly insure 
* each process refelcts its true time and insures the current process does not be charged 
* for the time in the interupt handler or its various subroutines
*
*/
static void intervalTimerHandler(cpu_t startTime, cpu_t endTime) {
    /* load the interval */
    LDIT(INTERVAL);
    /* get the index of the last device in the device 
    semaphore list - which is the interval timer */
    int *sem = &(devSemdTable[CLOCK]);
    /* reset the semaphore */
    (*sem) = 0;
    /* get all of the blocked devices*/
    pcb_PTR blocked = headBlocked(sem);
    /* while there are blocked devices */
    while(blocked != NULL) {
        pcb_PTR p = removeBlocked(sem);
        STCK(endTime);
        if(p != NULL) {
            /* a process has been freed up */
            insertProcQ(&(readyQueue), p);
            /* the elapsed time is the start minus the end */
            cpu_t elapsedTime = (endTime - startTime);
            /* handle the charging of time */
            (p->p_time) = (p->p_time) + elapsedTime;
            /* one less device waiting */
            softBlockedCount--;
            /* get the next one */
            blocked = headBlocked(sem);
        }
    }
    /* exit the interrupt handler - from which this process had 
    come from */
    exitInterruptHandler(startTime);
}



