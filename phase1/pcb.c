#include "../h/const.h"
#include "../h/types.h"

#include "/usr/include/uarm/libuarm.h"
#include "../e/pcb.e"
#include "../e/asl.e"

#define TRUE 1
#define FALSE 0

/* PCB.C */

HIDDEN pcb_PTR pcbFree_h;

char okbuf2[2048];	
char *mp2 = okbuf2;


pcb_PTR mkEmptyProcQ (){
	addokbuf("\nentered mkEmptyProcQ");
	return NULL;
}

int emptyProcQ (pcb_PTR tp){
	addokbuf("\nentered emptyProcQ");
	return (tp == NULL);
}

void cleanPcb(pcb_PTR p) {
	p->p_next = NULL;  /* initialize fields */
	p->p_prev = NULL;
	p->p_prnt = NULL;
	p->p_child = NULL;
	p->p_sib = NULL;
	p->p_prevSib = NULL;
	p->p_semAdd = NULL; 
}

void freePcb (pcb_PTR p){
	addokbuf("\nentered and finished? freePcb");
	cleanPcb(p);
	insertProcQ(&(pcbFree_h), p);
}

pcb_PTR allocPcb (){
	addokbuf("\nentered allocPcb");

	if (emptyProcQ(pcbFree_h)) {
		return NULL;
	}

	pcb_PTR tmp = removeProcQ(&pcbFree_h);

	if (tmp != NULL) {
		cleanPcb(tmp);
		addokbuf("\nalloc cleaned");
	}

	addokbuf("\nfinished allocPcb\n");
	return tmp;
}

void initPcbs (){
	addokbuf("\nentered initPcbs");
	pcbFree_h = mkEmptyProcQ();
	static pcb_t initialPcbs[MAXPROC];	
	int i;
	for (i = 0; i < MAXPROC; i++) {
		freePcb(&(initialPcbs[i]));
	}
	addokbuf("\nfinished initPcbs");
}

/* cases: 1) empty procQ, 2) non-empty procQ */
void insertProcQ (pcb_PTR *tp, pcb_PTR p){
	addokbuf("\nentered insertProcQ");
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
	addokbuf("\ninsertProcQ finished");
}

/* 2 cases: the q is empty so return null, or 2, remove head */

/* new outProcQ */
pcb_PTR outProcQ (pcb_PTR *tp, pcb_PTR p){
	addokbuf("\nentered outProcQ");
	if (emptyProcQ(*tp)) {
		addokbuf("\nline 99");
		return NULL;
	}

	if ((*tp)->p_next == p) { /* head is p */
		addokbuf("\nline 104");
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

pcb_PTR removeProcQ (pcb_PTR *tp){
	if (emptyProcQ(*tp)) {
		addokbuf("\nline 132");
		return NULL;
	}

	/* is head only pcb in q or are there others */
	addokbuf("\nline 137");
	if ((*tp)->p_next == (*tp)) { /* head/tail is only one in q */
		addokbuf("\nline 139");
		pcb_PTR removedPcb = (*tp);
		(*tp) = mkEmptyProcQ();
		return removedPcb;
	}
	/* head isn't only pcb in q */
	addokbuf("\nline 144");
	pcb_PTR head = (*tp)->p_next;
	head->p_prev->p_next = head->p_next;
	head->p_next->p_prev = head->p_prev;
	return head;
}




pcb_PTR headProcQ (pcb_PTR tp){
	addokbuf("\nentered headProcQ");
	if (emptyProcQ(tp)) {
		addokbuf("\nheadProcQ finished");
		return NULL;
	}
	addokbuf("\nheadProcQ finished");
	return (tp->p_next);
}

pcb_PTR cleanChild(pcb_PTR p) {
	p->p_sib = NULL;
	p->p_prevSib = NULL;
	p->p_prnt = NULL;

	return p;
}

int emptyChild (pcb_PTR p){
	addokbuf("\nentered and finished? emptyChild");
	return (p->p_child == NULL);
}

/* assumption: it's fine to insert new child at the head of the child list and it's fine to have child lists singly linked */
/* cases: 1) empty child list, 2) non-empty child list */
void insertChild (pcb_PTR prnt, pcb_PTR p){
	
	if (emptyChild(prnt)) {
		prnt->p_child = p;

		p->p_prnt = prnt;
		p->p_sib = NULL;
		p->p_prevSib = NULL;

		return;
	}

	pcb_PTR firstChild = prnt->p_child;

	p->p_sib = firstChild;
	p->p_prevSib = NULL;

	firstChild->p_prevSib = p;

	prnt->p_child = p;
}


pcb_PTR removeChild (pcb_PTR p){
	
	if (emptyChild(p)) {
		return NULL;
	}

	pcb_PTR removedChild = p->p_child;

	if (removedChild->p_sib == NULL) {
		p->p_child = NULL;

		return cleanChild(removedChild); 
	}

	removedChild->p_sib->p_prevSib = NULL;
	p->p_child = removedChild->p_sib;

	return cleanChild(removedChild); 
}


/* five conditions to account for: 1) p has no parent, 2) p is only child of its parent, 3) more than one pcb in child list and target p is first one, 4) it's one of more than one and isn't the first */
pcb_PTR outChild (pcb_PTR p){ 
	if (p == NULL) {
		return NULL;
	}

	if (p->p_prnt == NULL) {
		return NULL;
	}
	addokbuf("\np has a parent line 232");

	pcb_PTR firstChild = p->p_prnt->p_child;

	if (firstChild == p) {
		addokbuf("\nfirst child is p line 237");
		return removeChild(p->p_prnt);
	}

	if (firstChild->p_sib == NULL) { /*shouldn't be able to get this to be true */
		addokbuf("\n shouldn't be able to get here line 241");
		return NULL;
	}
	/* p is not the start of the child list */
	if (p->p_sib == NULL) { /* is p at the end of the child list */
		addokbuf("\np is last child line 247");
		p->p_prevSib->p_sib = NULL;
		return cleanChild(p);
	}
	/* p should be somewhere in middle of child list */
	p->p_prevSib->p_sib = p->p_sib;
	p->p_sib->p_prevSib = p->p_prevSib;
	addokbuf("\np is a middle child line 254");
	return cleanChild(p);
}


