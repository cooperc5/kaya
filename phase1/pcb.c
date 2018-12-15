/*************************************************** pcb.c **************************************************************
	pcb.c encpsulates the functionality of Process Control Blocks, henceforth known as pcb_t; the pcb.c module is
	responsible for three major pcb_t functions: first, a free list of MAXPROC pcb_t are allocated - where
	MAXPROC = 20; then, pcb_t themsleves are to keep a child-parent-sibling relationships, where the siblings of
	a pcb_t are kepted in a doubly liked list that is null terminated; third, it is responsible
	for keeping process queues of pcb_t to be allocated fromon and off the free list.
	This module contributes function definitions and a few sample fucntion implementations  to the contributors put forth by
	the Kaya OS project
***************************************************** pcb.c ************************************************************/

#include "../h/const.h"
#include "../h/types.h"

#include "/usr/include/uarm/libuarm.h"
#include "../e/pcb.e"
#include "../e/asl.e"

/* PCB.C */

/* the free list of pcb_t's */
HIDDEN pcb_PTR pcbFree_h;


/*
* Function: initialize the tp of an
* empty process queue - i.e. return null
*/
pcb_PTR mkEmptyProcQ (){
	return NULL;
}

/*
* Function: returns a boolean expression
* if a tp is null - that is, a tp points
* to an empty process queue
*/
int emptyProcQ (pcb_PTR tp){
	return (tp == NULL);
}

/*
* Function: nulls out all of the fields for
* a provided pcb_t; if a null pcb_t is provided,
* the function will return null
*/
void cleanPcb(pcb_PTR p) {
	p->p_next = NULL;  /* initialize fields */
	p->p_prev = NULL;
	p->p_prnt = NULL;
	p->p_child = NULL;
	p->p_sib = NULL;
	p->p_prevSib = NULL;
	p->p_semAdd = NULL;
	oldSys = NULL;
    newSys = NULL;
    oldPgm = NULL;
    newPgm = NULL;
    oldTLB = NULL;
    newTLB = NULL;
    p_time = NULL;
    waitState = NULL; 
}

/*
* Function: pcb_t that are no longer in use
* are returned to the free list here; this simply
* requires the uses of the already written
* insertProcQ, but must be cleaned before they
* are inserted onto the list pointed to by
* insertProcQ - that is, the free list.
*/
void freePcb (pcb_PTR p){
	cleanPcb(p);

	insertProcQ(&(pcbFree_h), p); /* insert into free list */
}

/*
* Function: allocate a pcb_t from the free
* free list; remove the pcb_t so that the
* free list has n-1 pcb_t on it; if the
* free list has no remaining free pcb_t
* that is, the free list is empty,
* simply return null to indicate that there
* are no pcb_t remaining; otherwise, make the
* free list n-1 pcb_t by calling removeProcQ()
* given the free list as a pointer. Before the
* pcb_t is returned, it is cleaned so that
* can be appropriately used and a pointer to the
* returned pcb_t is provided; additionally, the
* pcb_t will be cleaned before it is returned
*/
pcb_PTR allocPcb (){
	/* return null if free list is empty */
	if (emptyProcQ(pcbFree_h)) {
		return NULL;
	}

	pcb_PTR tmp = removeProcQ(&pcbFree_h); /* remove head of free list */

	if (tmp != NULL) {
		cleanPcb(tmp); /* clean it */

		tmp->oldSys = NULL;
    	tmp->newSys = NULL;
    	tmp->oldPgm = NULL;
    	tmp->newPgm = NULL;
    	tmp->oldTLB = NULL;
    	tmp->newTLB = NULL;
	}

	return tmp;
}

/*
* Function: initializes the pcb_t
* free list for pcb_t to be allocated to
* by some predefined constant; this function
* is an init call
*/
void initPcbs (){
	pcbFree_h = mkEmptyProcQ(); /* initialize the free list */
	static pcb_t initialPcbs[MAXPROC];	
	int i;
	for (i = 0; i < MAXPROC; i++) {
		freePcb(&(initialPcbs[i])); /* add each pcb to the free list */
	}
}

/*
* Function: insert the pcb_t p into the
* process queue tp
*/
/* cases: 1) empty procQ, 2) non-empty procQ */
void insertProcQ (pcb_PTR *tp, pcb_PTR p){
	if (emptyProcQ(*tp)) { /* empty q case 1 */
		p->p_next = p;
		p->p_prev = p;
	}
	/* non-empty q */
	else { /* case 2 */
		p->p_next = (*tp)->p_next; /* set next of p */
		p->p_prev = (*tp); /* set prev of p */
		(*tp)->p_next->p_prev = p; /* set prev of head */
		(*tp)->p_next = p; /* set next of previous tail */
	}
	/* set tail pointer */
	(*tp) = p;
}


/*
* Function: remove the pcb_t pointed to by p
* from the process queue pointed to by tp;
* update the process queue's tp if necessary;
* if the desired entry is not in the indicated queue,
* return null; else, return p
*/
pcb_PTR outProcQ (pcb_PTR *tp, pcb_PTR p){
	if (emptyProcQ(*tp)) {
		return NULL;
	}

	if ((*tp)->p_next == p) { /* head is p */
		return removeProcQ(tp);
	}
	/* previous if covers the cases of p being only pcb in q and if it's the head of a q with more than 1 in the q */
	if ((*tp) == p) { /* p is tail but there are more in q */
		p->p_prev->p_next = p->p_next;
		p->p_next->p_prev = p->p_prev;
		(*tp) = p->p_prev;
		return p;
	}
	/* look for p; we know it's not tail or head and don't know if it's in the list */
	pcb_PTR current = (*tp)->p_next;

	while (current != (*tp)) {
		current = current->p_next;
		if (current == p) {
			current->p_prev->p_next = current->p_next;
			current->p_next->p_prev = current->p_prev;
			return current;
		}
	}

	return NULL;
}


/*
* Function: removes the first element from the
* processes queue whose tp is passed in as an
* argument; return null if the tp is null - meaning
* there is no list
*/
pcb_PTR removeProcQ (pcb_PTR *tp){
	if (emptyProcQ(*tp)) { /* is the procQ empty? */
		return NULL;
	}

	/* is head only pcb in q or are there others */
	if ((*tp)->p_next == (*tp)) { /* head/tail is only one in q */
		pcb_PTR removedPcb = (*tp);
		(*tp) = mkEmptyProcQ();

		return removedPcb;
	}
	/* head isn't only pcb in q */
	pcb_PTR head = (*tp)->p_next;
	head->p_prev->p_next = head->p_next;
	head->p_next->p_prev = head->p_prev;

	return head;
}


/*
* Function: returns a pointer to the head
* of a process queue signified by tp - however
* this head should not be removed; if there is no
* head of the process queue - that is, there is
* no process queue, return null
*/
pcb_PTR headProcQ (pcb_PTR tp){
	if (emptyProcQ(tp)) {
		return NULL;
	}
	return (tp->p_next);
}

pcb_PTR cleanChild(pcb_PTR p) {
	p->p_sib = NULL; /* null out fields */
	p->p_prevSib = NULL;
	p->p_prnt = NULL;

	return p;
}


/*
* Function: takes a pcb_t as an
* argument and returns a boolean expression
* as to whether or not a particular
* pcb_t has a child or not
*/
int emptyChild (pcb_PTR p){
	return (p->p_child == NULL);
}

/*
* Function: takes a parent pcb_t and
* provides a pcb_t to be linked to that
* parent
*/
/* assumption: it's fine to insert new child at the head of the child list and it's fine to have child lists singly linked */
/* cases: 1) empty child list, 2) non-empty child list */
void insertChild (pcb_PTR prnt, pcb_PTR p){
	/* does prnt have any children? */
	if (emptyChild(prnt)) { /* no so make p only child of prnt */
		prnt->p_child = p;

		p->p_prnt = prnt;
		p->p_sib = NULL;
		p->p_prevSib = NULL;

		return;
	}
	/* prnt has children, so weave in p */
	pcb_PTR firstChild = prnt->p_child;

	p->p_sib = firstChild;
	p->p_prevSib = NULL;
	p->p_prnt = prnt;

	firstChild->p_prevSib = p;

	prnt->p_child = p;
}

/*
* Function: takes a parent pcb_t and
* removes and returns the first pcb_t child -
* baring there is one; if the parent
* pcb_t, however, is null, then this
* function must return null to hanle
* that case
*/
pcb_PTR removeChild (pcb_PTR p){
	/* does p even have any children */
	if (emptyChild(p)) {
		return NULL;
	}

	pcb_PTR removedChild = p->p_child;

	if (removedChild->p_sib == NULL) { /* is p the only child? */
		p->p_child = NULL;

		return cleanChild(removedChild); 
	}
	/* remove first child and adjust pointers */
	removedChild->p_sib->p_prevSib = NULL;
	p->p_child = removedChild->p_sib;

	return cleanChild(removedChild); 
}


/*
* Function: makes the pcb_t given by p
* no longer a child of the its parent
* pcb_t, and remove it from the list;
* however, this can be any child in the list;
* if the pcb_t has no parent, simply return
* null
*/
pcb_PTR outChild (pcb_PTR p){ 
	if (p == NULL) { /* this shouldn't be true */
		return NULL;
	}

	if (p->p_prnt == NULL) { /* does p have a parent? */
		return NULL;
	}

	pcb_PTR firstChild = p->p_prnt->p_child;

	if (firstChild == p) {
		return removeChild(p->p_prnt);
	}

	if (firstChild->p_sib == NULL) { /*shouldn't be able to get this to be true */
		return NULL;
	}
	/* p is not the start of the child list */
	if (p->p_sib == NULL) { /* is p at the end of the child list */
		p->p_prevSib->p_sib = NULL;

		return cleanChild(p);
	}
	/* p should be somewhere in middle of child list */
	p->p_prevSib->p_sib = p->p_sib;
	p->p_sib->p_prevSib = p->p_prevSib;

	return cleanChild(p);
}


