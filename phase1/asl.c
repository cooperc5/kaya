#ifndef ASL
#define ASL


#include "../h/types.h"
#include "../h/const.h"
#include "../e/pcb.e"


static semd_PTR semdASL; /* active semaphore list head ptr */
static semd_PTR semdFree; /* semd free list head ptr */


semd_PTR mkEmptySemd() {
	addokbuf2("entered mkEmptySemd");
	return NULL;
}

/* search semd list method */
static semd_PTR searchASL(int *semAdd) {
	addokbuf2("entered searchASL");
	/* get past head dummy node */
	semd_PTR current = semdFree;
	
	while (current->s_next->s_semAdd < semAdd) { /* next asl node is not equal to or higher than target semAdd */
		current = current->s_next; /* advance to next asl node */
	}
	addokbuf2("finished searchASL");
	return current; /* returns node just before where the target semAdd is or should be */
}

/* alloc semd method */
static semd_PTR allocSemd(int *semAdd) {
	addokbuf2("entered allocSemd");
	if (semdFree == NULL) { /*is free list empty? */
		addokbuf2("finished allocSemd");
		return NULL;
	}
	/* free list isn't empty */
	semd_PTR allocated = semdFree;
	/* is the semd to be allocated the only one in the list? */
	if (semdFree) 
	semdFree = semdFree->s_next;

	allocated->s_semAdd = semAdd;
	addokbuf2("finished allocSemd");
	return allocated;
}

static semd_PTR cleanSemd(semd_PTR s) {
	addokbuf2("entered cleanSemd");
	/* clean the semd */
	s->s_next = NULL;
	s->s_procQ = mkEmptyProcQ();
	addokbuf2("finished cleanSemd");
	return s;
}

/* free semd method */
static void freeSemd(semd_PTR s) {
	addokbuf2("entered freeSemd");
	cleanSemd(s);

	/* empty free list case */
	if (semdFree == NULL) {
		semdFree->s_next = s;
	}

	/* non-empty free list case */
	semd_PTR head = semdFree;
	semdFree->s_next = s;
	s->s_next = head;
	addokbuf2("finished freeSemd");
}

int insertBlocked (int *semAdd, pcb_PTR p) {
	addokbuf2("entered insertBlocked");
	semd_PTR target = searchASL(semAdd);	
	semd_PTR newNode = allocSemd(semAdd);
	if (newNode == NULL) {
		addokbuf2("finished insertBlocked");
		return 1;
	}
	newNode->s_next = target->s_next;
	target->s_next = newNode;
	insertProcQ(&(newNode->s_procQ), p);
	addokbuf2("finished insertBlocked");
	return 0;
}

pcb_PTR removeBlocked (int *semAdd){
	addokbuf2("entered removeBlocked");
	semd_PTR target = searchASL(semAdd);	
	if (target->s_next->s_semAdd != semAdd) { /* is the target semd there? */
		addokbuf2("finished removeBlocked");
		return NULL;
	} /* node found */
	pcb_PTR removedPcb = removeProcQ(&(target->s_next->s_procQ));
	if (emptyProcQ(target->s_next->s_procQ)) { /* free semd if procQ is now empty */
		semd_PTR emptySemd = target->s_next;
		target->s_next = emptySemd->s_next;
		freeSemd(emptySemd);
	}
	addokbuf2("finished removeBlocked");
	return removedPcb;
}

pcb_PTR outBlocked (pcb_PTR p){
	addokbuf2("entered outBlocked");
	semd_PTR target = searchASL(p->p_semAdd); /* get semd associated with pcb */
	
	pcb_PTR removedPcb = outProcQ(&(target->s_next->s_procQ), p);
	if (removedPcb == NULL) {
		return NULL; /* pcb not found */
		addokbuf2("finished outBlocked");
	}
	if (emptyProcQ(target->s_next->s_procQ)) { /* free semd if procQ is now empty */
		semd_PTR emptySemd = target->s_next;
		target->s_next = emptySemd->s_next;
		freeSemd(emptySemd);
	}
	addokbuf2("finished outBlocked");
	return removedPcb;
}

pcb_PTR headBlocked (int *semAdd){
	addokbuf2("entered headBlocked");
	semd_PTR target = searchASL(semAdd);
	if (target->s_next->s_semAdd != semAdd) { /* is the target semd there? */
		return NULL;
		addokbuf2("finished headBlocked");
	}
	addokbuf2("finished headBlocked");
	return headProcQ(target->s_next->s_procQ);
}

void initASL () {
	addokbuf2("entered initASL");
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
	addokbuf2("finished initASL");
}



/***************************************************************/

#endif
