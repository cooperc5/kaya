#include "../h/const.h"
#include "../h/types.h"
#ifndef EXCEPTIONS
#define EXCEPTIONS
    extern void syscallHandler();
    extern void programTrapHandler();
    extern void translationLookasideBufferHandler();
    extern void copyState(state_PTR oldState, state_PTR destState);
#endif