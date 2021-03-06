#include "../h/const.h"
#include "../h/types.h"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/initial.e"
#include "../e/interrupts.e"
#include "../e/exceptions.e"
#include "../e/scheduler.e"
#include "/usr/local/include/umps2/umps/libumps.e"
#include "../e/p2test.e"

int processCount;
int softBlockedCount;
pcb_PTR currentProcess;
pcb_PTR readyQueue;
int devSemdTable[MAXDEVSEM];

void debuginitial(int a) {
    int i;
    i=0;
}
int main() {
    devregarea_PTR bus = (devregarea_PTR) RAMBASEADDR;
    
    unsigned int RAMTOP = (bus->rambase) + (bus->ramsize);


    state_PTR syscallState;
    state_PTR programTrapState;
    state_PTR tblMgmtState;
    state_PTR interruptState;

    /* syscall initialization */
    syscallState = (state_PTR) SYSCALLNEWAREA;
    syscallState->s_status = OFF;   
    syscallState->s_sp = RAMTOP;
    syscallState->s_pc = syscallState->s_t9 = (memaddr) syscallHandler; 
    /* pgmtrap initilization */
    programTrapState = (state_PTR) PRGMTRAPNEWAREA;
    programTrapState->s_status = OFF;   
    programTrapState->s_sp = RAMTOP;
    programTrapState->s_pc = programTrapState->s_t9 = (memaddr) programTrapHandler; 
    /* table management initialization */
    tblMgmtState = (state_PTR) TBLMGMTNEWAREA;
    tblMgmtState->s_status = OFF;   
    tblMgmtState->s_sp = RAMTOP;
    tblMgmtState->s_pc = tblMgmtState->s_t9 = (memaddr) translationLookasideBufferHandler; 
    /* interrupt initialization */
    interruptState = (state_PTR) INTERRUPTNEWAREA;
    interruptState->s_status = OFF;   
    interruptState->s_sp = RAMTOP;
    interruptState->s_pc = interruptState->s_t9 = (memaddr) interruptHandler; 


    initPcbs();
    initASL();

    int i;
    for (i=0; i<MAXDEVSEM; i++) {
        devSemdTable[i] = 0;
    }

    processCount = 0;
    softBlockedCount = 0;
    currentProcess = NULL;
    readyQueue = mkEmptyProcQ();


    currentProcess = allocPcb();

    currentProcess->p_s.s_sp = (RAMTOP - PAGESIZE);
    currentProcess->p_s.s_pc = currentProcess->p_s.s_t9 = (memaddr) test; /*insert p2test function here*/
    /* initialize the status */
    currentProcess->p_s.s_status = (OFF | INTERRUPTSON | IM | TE);

    insertProcQ(&(readyQueue), currentProcess);

    processCount++;

    currentProcess = NULL;

    LDIT(INTERVAL);

    scheduler();
}