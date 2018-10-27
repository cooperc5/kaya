#ifndef ASL
#define ASL


#include "../h/types.h"
#include "../e/pcb.e"


static semd_PTR semdASL;
static semd_PTR semdFree;


int insertBlocked (int *semAdd, pcb_PTR p) {
	semd_PTR target = searchASL(semAdd);	
	semd_PTR newNode = allocSemd(semAdd);
	if (newNode == NULL) {
		return 1;
	}
	newNode->s_next = target->s_next;
	target->s_next = newNode;
	insertProcQ(newNode->s_procQ, p);
	return 0;
}

pcb_PTR removeBlocked (int *semAdd){
	semd_PTR target = searchASL(semAdd);	
	if (target->s_next->s_semAdd != semAdd) {
		return NULL;
	} /* node found */
	pcb_PTR removedPcb = removeProcQ(target->s_next->s_procQ);
	if (emmptyProcQ(target->s_next->s_procQ)) { /* free semd if procQ is now empty */
		semd_PTR emptySemd = target->s_next;
		target->s_next = emptySemd->s_next;
		freeSemd(emptySemd);
	}
}

pcb_PTR outBlocked (pcb_PTR p){
	semd_PTR target = searchASL(semAdd);
	if (target->s_next->s_semAdd != semAdd) { /* is the target semd there? */
		return NULL;
	} /*node found */
	pcb_PTR removedPcb = outProcQ(&a.s_procQ, p);
	if (emmptyProcQ(target->s_next->s_procQ)) { /* free semd if procQ is now empty */
		semd_PTR emptySemd = target->s_next;
		target->s_next = emptySemd->s_next;
		freeSemd(emptySemd);
	}
}

pcb_PTR headBlocked (int *semAdd){
	semd_PTR target = searchASL(semAdd);
	if (target->s_next->s_semAdd != semAdd) { /* is the target semd there? */
		return null;
	}
	return headProcQ(target->s_next->s_procQ);
}

void initASL () {
	static semd_PTR foo[20];	/* init semd free list */
	for (int i = 0; i<20; i++) {
		foo[i] = mkEmptySemd();
		freeSemd(foo[i]);
	}

	/* init asl */
	static semd_t dummy1, dummy2;  /* set up dummy nodes */
	semd_PTR dummy1 = mkEmptySemd();
	semd_PTR dummy2 = mkEmptySemd();
	dummy1->s_semAdd = 0;
	dummy2->s_semAdd = MAXINT;
	semdASL = dummy1;
	semdASL->s_next = dummy2;
}

semd_PTR mkEmptySemd() {
	return NULL;  /* is this necessary? */
}

/* search semd list method */
semd_PTR searchASL(int *semAdd) {
	/* get past head dummy node */
	semd_t current = semdFree;
	
	while (current->s_next->s_semAdd < semAdd) { /* next asl node is not equal to or higher than target semAdd */
		current = current->s_next; /* advance to next asl node */
	}
	
	return current; /* returns node just before where the target semAdd is or should be */
}

/* alloc semd method */
semd_PTR allocSemd(int *semAdd) {
	if (*(semdFree) == NULL) { /*is free list empty? */
		return NULL;
	}
	/* free list isn't empty */
	semd_PTR allocated = semdFree; 
	semdFree = semdFree->s_next;

	/* clean the semd */
	allocated->s_next = NULL;
	allocated->s_procQ = mkEmptyProcQ();
	allocated->s_semAdd = semAdd;

	return allocated;
}

/* free semd method */
void freeSemd(semd_PTR s) {
	/* empty free list case */
	if ((*semdFree) == NULL) {
		semdFree->s_next = s;
	}

	/* non-empty free list case */
	semd_t head = (*semdFree);
	semdFree->s_next = s;
	s->s_next = head;
}

/***************************************************************/

#endif
