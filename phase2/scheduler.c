#include "../h/const.h"
#include "../h/types.h"
#include "../e/initial.e"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "/usr/local/include/umps2/umps/libumps.e"

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
				/*enter a WAIT state*/
				return;
			}
		}
	}
}