/*************************************************** asl.c **************************************************************
	asl.c implements a semaphore list - an important OS concept; here, the asl will be seen as an integer value and
	will keep addresses of Semaphore Descriptors, henceforth known as semd_t; much like in the pcb.c, the asl will keep an
	asl free list with MAXPROC free semd_t; this class will encapsulate the functionality needed too perform operations on
	the semd_t
	This module contributes function definitions and a few sample fucntion implementations to the contributors put forth by
	the Kaya OS project
***************************************************** asl.c ************************************************************/

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

/*
* Function: searches the semd_t asl list for
* the specified semd_t address passed in as an
* argument to the function; since there are two
* dummy semd_t on the list, there are no erronous
* exit conditions
*/
static semd_PTR searchASL(int *semAdd) {
	if(semAdd == NULL) { /* if invalid semAdd is passed, set it to MAXINT */
		semAdd = (int*) MAXINT;
	}
	semd_PTR current = semdASL;

	while (current->s_next->s_semAdd < semAdd) { /* next asl node is not equal to or higher than target semAdd */
		current = current->s_next; /* advance to next asl node */
	}
	
	return current; /* returns node just before where the target semAdd is or should be */
}

/*
*	Function: allocates a semd_t from the semd_t
* free list and returns a pointer to it;
* should the send_t free list head is null,
* then there are no free semd_t to allocate
*/
static semd_PTR allocSemd(int *semAdd) {
	if (semdFree == NULL) { /*is free list empty? */
		return NULL;
	}
	/* free list isn't empty */
	semd_PTR allocated = semdFree;
	/* is the semd to be allocated the only one in the list? */
	if (semdFree->s_next == NULL) {
		semdFree = NULL;
		cleanSemd(allocated);
		allocated->s_semAdd = semAdd;
		return allocated;
	} 
	/* no not only one */
	semdFree = semdFree->s_next;

	cleanSemd(allocated);
	allocated->s_semAdd = semAdd;

	return allocated;
}

/*
* Function: nulls out all of the values of a
* semd_t so that it is clean and can be ready to
* used - since a semd_t cannot come off the
* free list with defined values
*/
static void cleanSemd(semd_PTR s) {
	/* clean the semd */
	s->s_next = NULL;
	s->s_procQ = mkEmptyProcQ();
	s->s_semAdd = NULL;
}

/*
* Function: takes a semd_t and points it onto
* the semd_t free list; if there is nothing on
* the semd_t free list, a free list is "created"
* by making the newly added semd_t next semd_t
* to be null; if its not empty
*/
static void freeSemd(semd_PTR s) {
	cleanSemd(s);

	/* empty free list case */
	if (semdFree == NULL) {
		semdFree = s;
		semdFree->s_next = NULL;
		return;
	}

	/* non-empty free list case */
	semd_PTR head = semdFree;
	semdFree = s;
	s->s_next = head;
}

/*
* Function: insert the pcb_t provided as an a
* argument to the tail of that pcb_t process
* queue at the semd_t address provided; this method
* can get tricky: if there is no semd_t descriptor,
* as in, there is it is not active because it is
* nonexistent in the asl list a new semd_t must initalized,
* an be allocated to take its place - however, if the
* free list is blocked - return true; in a successful operation
* the function returns null
*/
int insertBlocked (int *semAdd, pcb_PTR p) {
	if (p == NULL) {
		return TRUE;
	}

	semd_PTR target = searchASL(semAdd);
	if (target->s_next->s_semAdd == semAdd) { /* is semAdd already present? */
		insertProcQ(&(target->s_next->s_procQ), p); /* yes */
		p->p_semAdd = semAdd;
		return FALSE;
	} /* no */
	/* is there a node available on the free list? */
	if (semdFree == NULL) {
		return TRUE;
	}
	/* yes, so allocate it */
	semd_PTR newASLNode = allocSemd(semAdd);
	insertProcQ(&(newASLNode->s_procQ), p);
	newASLNode->s_next = target->s_next; /* weave in asl node */
	target->s_next = newASLNode;
	p->p_semAdd = semAdd;
	return FALSE;
}

/*
* Function: search the asl semd_t list for the specified
* semd_t address; in the case that it is not found, simply
* exit and return null; in the case that it is found, remove
* the HEAD pcb_t from that process queue of the found semd_t
* descriptor and return its pointer; this, too, like insertBlocked
* can be tricky in its own right: if this process queue then becomes
* null, meaning that emptyProcQ is null, then this semd_t must be
* removed and sent to the semd_t free list
*/
pcb_PTR removeBlocked (int *semAdd){
	semd_PTR target = searchASL(semAdd);	
	if (target->s_next->s_semAdd != semAdd) { /* is the target semd there? */
		return NULL;
	} /* node found */
	pcb_PTR removedPcb = removeProcQ(&(target->s_next->s_procQ));
	if (emptyProcQ(target->s_next->s_procQ)) { /* free semd if procQ is now empty */
		semd_PTR emptySemd = target->s_next;
		target->s_next = emptySemd->s_next;
		freeSemd(emptySemd);
	}

	return removedPcb;
}

/*
* Function: remove the pcb_t passed in as the argument from
* the semd_t that contains the specified pcb; if the pcb_t
* does not appear in the process queue in the associated
* semd_t, return null
*/
pcb_PTR outBlocked (pcb_PTR p){
	semd_PTR target = searchASL(p->p_semAdd); /* get semd associated with pcb */
	
	pcb_PTR removedPcb = outProcQ(&(target->s_next->s_procQ), p);
	if (removedPcb == NULL) {
		return NULL; /* pcb not found */
	}
	if (emptyProcQ(target->s_next->s_procQ)) { /* free semd if procQ is now empty */
		semd_PTR emptySemd = target->s_next;
		target->s_next = emptySemd->s_next;
		freeSemd(emptySemd);
	}
	return removedPcb;
}

/*
* Function: returns a pointer to the pcb_t
* that is at the HEAD of the pcb_t process queue
* with its associated semd_t address;
* if there is no associated semaphore descriptor or
* if the process queue associated with the
* semaphore address is empty - return null in both cases
*/
pcb_PTR headBlocked (int *semAdd){
	semd_PTR target = searchASL(semAdd);
	if (target->s_next->s_semAdd != semAdd) { /* is the target semd there? */
		return NULL;
	}
	return headProcQ(target->s_next->s_procQ);
}

/*
* Function: the first and perhaps most important
* stored procedure - the allocation of the
* active semaphore list asl of type semd_t;
* here, the semd_t free list is allocated to be
* of size MAXPROC, where MAXPROC = 20;
* IMPORTANT! this implementation of the
* semd_t free list uses 2 DUMMY nodes as to avoid
* error-prone exit conditions - which will be referenced
* further on in the documentation; thus, the semd_t
* free list will have MAXPROC + 2 dummy nodes to account
* for the space necessary to house enough semd_t on the
* free list
*/
void initASL () {
	semdASL = mkEmptySemd();
	semdFree = mkEmptySemd();
	static semd_t foo[MAXPROC + 2];	/* init semd free list */
	int i;
	for (i = 0; i<MAXPROC; i++) {
		freeSemd(&foo[i]); /* build free list */
	}
	/* init asl and put in dummy nodes */
	semdASL = &(foo[MAXPROC]);
	semdASL->s_procQ = mkEmptyProcQ();
	semdASL->s_semAdd = 0;
	semdASL->s_next = &(foo[MAXPROC + 1]);
	semdASL->s_next->s_procQ = mkEmptyProcQ();
	semdASL->s_next->s_next = NULL;
	semdASL->s_next->s_semAdd = MAXINT;
}



/***************************************************************/

#endif
