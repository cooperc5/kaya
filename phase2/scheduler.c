#include "../h/const.h"
#include "../h/types.h"
#include "../e/initial.e"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "/usr/local/include/umps2/umps/libumps.e"

pcb_PTR runningProcess;
cpu_t startTOD;
cpu_t currentTOD;

extern void scheduler() {
	if (emptyProcQ(readyQueue)) {
		if (processCount == 0) {
			/*invoke HALT instruction*/
			return;
		}
		if (processCount > 0) {
			if (softBlockCount == 0) {
				/*invoke PANIC instruction*/
				return;
			}
			if (softBlockCount > 0) {
				/* set wait bit in state */
				/*enter a WAIT state*/
				return;
			}
		}
	}
	pcb_PTR currentProcess = removeProcQ(&readyQueue);

	if (currentProcess != NULL) {
		/* quantum stuff and something */
	}
	runningProcess = currentProcess;
	/* load a timer with the value of a quantum */
	LDST(&(currentProcess->p_s));
}