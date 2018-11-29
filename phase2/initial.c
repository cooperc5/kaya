/* h files to include */
#include "../h/const.h"
#include "../h/types.h"
/* e files to include */
#include "../e/pcb.e"
#include "../e/asl.e"

#include "../e/initial.e"
#include "../e/interrupts.e"
#include "../e/exceptions.e"
#include "../e/scheduler.e"
#include "/usr/local/include/umps2/umps/libumps.e"

#include "../e/p2test.e"

/* globals */
/* the current process count */
int processCount;
/* the soft blocked count */
int softBlockedCount;
/* the current process */
pcb_PTR currentProcess;
/* the queue of ready processes */
pcb_PTR readyQueue;


int main() {

	devregarea_PTR bus = (devregarea_PTR) RAMBASEADDR;
    
	unsigned int RAMTOP = (bus->rambase) + (bus->ramsize);


	state_PTR syscallState;
	state_PTR programTrapState;
	state_PTR tblMgmtState;
	state_PTR interruptState;

	/******************************************** SYSCALL AREA ****************************************/
	syscallState = (state_PTR) SYSCALLNEWAREA;
    syscallState->s_status = 0;   
    syscallState->s_sp = RAMTOP;
    syscallState->s_pc = syscallState->s_t9 = (memaddr) syscallHandler; 
    /******************************************** PRGMTRAP AREA ***************************************/
    programTrapState = (state_PTR) PRGMTRAPNEWAREA;
    programTrapState->s_status = 0;   
    programTrapState->s_sp = RAMTOP;
    programTrapState->s_pc = programTrapState->s_t9 = (memaddr) programTrapHandler; 
    /******************************************** TBLMGMT AREA ****************************************/
    tblMgmtState = (state_PTR) TBLMGMTNEWAREA;
    /* privlaged ROM instruction */
    tblMgmtState->s_status = 0;   
    tblMgmtState->s_sp = RAMTOP;
    tblMgmtState->s_pc = tblMgmtState->s_t9 = (memaddr) translationLookasideBufferHandler; 
    /******************************************** INTRUPT AREA ****************************************/
    interruptState = (state_PTR) INTRUPTNEWAREA;
    interruptState->s_status = 0;   
    interruptState->s_sp = RAMTOP;
    interruptState->s_pc = interruptState->s_t9 = (memaddr) interruptHandler; 


	initPcbs();
	initASL();


    processCount = 0;
    softBlockedCount = 0;
    currentProcess = NULL;
    readyQueue = mkEmptyProcQ();


    currentProcess = allocPcb();

    currentProcess->p_s.s_sp = (RAMTOP - PAGESIZE);
    currentProcess->p_s.s_pc = (memaddr) /*insert p2test function here*/
    /* initialize the status */
    currentProcess->p_s.s_status = (ALLOFF | INTERRUPSON); /*finish this with supervisor mode/whatever)


    processCount++;

    scheduler();
}
