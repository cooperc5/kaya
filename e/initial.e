#ifndef INIT
#define INIT
#include "../h/types.h"
#include "../h/const.h"


    extern pcb_PTR currentProcess;
    extern pcb_PTR readyQueue;
    extern int processCount;
    extern int softBlockedCount;
    extern cpu_t startTOD;
    extern cpu_t currentTOD;

    /* dev sem table */
    extern int devSemdTable[MAXDEVSEM];

#endif