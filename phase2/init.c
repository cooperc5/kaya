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
/* semaphore list */
int semdTable[MAXSEMALLOC];


int main() {
    processCount = 0;
    softBlockedCount = 0;
    currentProcess = NULL;
    readyQueue = mkEmptyProcQ();

    static semd_t deviceTbl[]
}
