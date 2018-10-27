#ifndef ASL
#define ASL


#include "../h/types.h"
#include "../h/const.h"
#include "../e/pcb.e"


static semd_PTR semdASL; /* active semaphore list head ptr */
static semd_PTR semdFree; /* semd free list head ptr */


semd_PTR mkEmptySemd() {
	return NULL;
}

/* search semd list method */
static semd_PTR searchASL(int *semAdd) {
	/* get past head dummy node */
	semd_PTR current = semdFree;
	
	while (current->s_next->s_semAdd < semAdd) { /* next asl node is not equal to or higher than target semAdd */
		current = current->s_next; /* advance to next asl node */
	}
	
	return current; /* returns node just before where the target semAdd is or should be */
}

/* alloc semd method */
static semd_PTR allocSemd(int *semAdd) {
	if (semdFree == NULL) { /*is free list empty? */
		return NULL;
	}
	/* free list isn't empty */
	semd_PTR allocated = semdFree;
	/* is the semd to be allocated the only one in the list? */
	if (semdFree) 
	semdFree = semdFree->s_next;

	allocated->s_semAdd = semAdd;

	return allocated;
}

static semd_PTR cleanSemd(semd_PTR s) {
	/* clean the semd */
	s->s_next = NULL;
	s->s_procQ = mkEmptyProcQ();

	return s;
}

/* free semd method */
static void freeSemd(semd_PTR s) {
	cleanSemd(s);

	/* empty free list case */
	if (semdFree == NULL) {
		semdFree->s_next = s;
	}

	/* non-empty free list case */
	semd_PTR head = semdFree;
	semdFree->s_next = s;
	s->s_next = head;
}

int insertBlocked (int *semAdd, pcb_PTR p) {
	semd_PTR target = searchASL(semAdd);	
	semd_PTR newNode = allocSemd(semAdd);
	if (newNode == NULL) {
		return 1;
	}
	newNode->s_next = target->s_next;
	target->s_next = newNode;
	insertProcQ(&(newNode->s_procQ), p);
	return 0;
}

pcb_PTR removeBlocked (int *semAdd){
	semd_PTR target = searchASL(semAdd);	
	if (target->s_next->s_semAdd != semAdd) { /* is the target semd there? */
		return NULL;
	} /* node found */
	pcb_PTR removedPcb = removeProcQ(&(target->s_next->s_procQ));
	if (emmptyProcQ(target->s_next->s_procQ)) { /* free semd if procQ is now empty */
		semd_PTR emptySemd = target->s_next;
		target->s_next = emptySemd->s_next;
		freeSemd(emptySemd);
	}
	return removedPcb;
}

pcb_PTR outBlocked (pcb_PTR p){
	semd_PTR target = searchASL(p->p_semAdd); /* get semd associated with pcb */
	
	pcb_PTR removedPcb = outProcQ(&(target->s_next->s_procQ), p);
	if (removedPcb == NULL) {
		return NULL; /* pcb not found */
	}
	if (emmptyProcQ(target->s_next->s_procQ)) { /* free semd if procQ is now empty */
		semd_PTR emptySemd = target->s_next;
		target->s_next = emptySemd->s_next;
		freeSemd(emptySemd);
	}
	return removedPcb;
}

pcb_PTR headBlocked (int *semAdd){
	semd_PTR target = searchASL(semAdd);
	if (target->s_next->s_semAdd != semAdd) { /* is the target semd there? */
		return NULL;
	}
	return headProcQ(target->s_next->s_procQ);
}

void initASL () {
	static semd_PTR foo[20];	/* init semd free list */
	int i;
	for (i = 0; i<20; i++) {
		foo[i] = mkEmptySemd();
		freeSemd(foo[i]);
	}

	/* init asl */
	static semd_PTR dummy1, dummy2;  /* set up dummy nodes */
	dummy1 = mkEmptySemd();
	dummy2 = mkEmptySemd();
	dummy1->s_semAdd = 0;
	dummy2->s_semAdd = MAXINT;
	semdASL = dummy1;
	semdASL->s_next = dummy2;
}



/***************************************************************/

#endif
