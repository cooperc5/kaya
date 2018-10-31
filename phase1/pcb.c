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

pcb_PTR cleanPcb(pcb_PTR p) {
	
	p->p_next = NULL;  /* initialize fields */
	p->p_prev = NULL;
	p->p_prnt = NULL;
	p->p_child = NULL;
	p->p_sib = NULL;
	p->p_prevSib = NULL;
	p->p_semAdd = NULL; 
	
	return p;
}

void freePcb (pcb_PTR p){
	addokbuf("\nentered and finished? freePcb");
	pcb_PTR tmp = cleanPcb(p);
	p = tmp;
	insertProcQ(&(pcbFree_h), p);
}

pcb_PTR allocPcb (){
	addokbuf("\nentered allocPcb");

	if (emptyProcQ(pcbFree_h)) {
		return NULL;
	}

	pcb_PTR tmp = removeProcQ(&pcbFree_h);

	if (tmp != NULL) {
		tmp = cleanPcb(tmp);
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
pcb_PTR removeProcQ (pcb_PTR *tp){
	addokbuf("\nremoveProcQ started");
	if (!emptyProcQ(*tp)) {
		addokbuf("\nabout to enter outProcQ");
		return outProcQ(tp, (*tp)->p_next); /* case 2 */
	}
	return NULL; /* case 1 */
}

/* four conditions to account for: 1) p is only pcb in procQ, 2) more than one pcb in procQ and target pcb is tail, 3) it's one of more than one and isn't the tail, 4) or it's not there */
pcb_PTR outProcQ (pcb_PTR *tp, pcb_PTR p){
	addokbuf("\nentered outProcQ");

	if (emptyProcQ(*tp)) {
		addokbuf("\nline 101");
		return NULL;
	}

	if (p == NULL) {
		addokbuf("\np was null in outProcQ");
		return NULL;
	}

	addokbuf("\nline 96");
	 /* current is now head pcb of procQ */
	addokbuf("\nline 98");
	if ((*tp) == p) { /* tail pcb is p */
		addokbuf("\nline 100");
		if (((*tp)->p_next) == (*tp)) { /* case 1: p is only pcb on the procQ tp */
			pcb_PTR removedPcb = (*tp);
			addokbuf("\nline 102");
			(*tp) = mkEmptyProcQ; /* set the tp to null to indicate an empty procQ */
			addokbuf("\noutProcQ finished");
			return removedPcb;
		}
		/* condition 2 */
		addokbuf("\nline 108");
		((*tp)->p_prev->p_next) = ((*tp)->p_next); /*adjust next pointer for new tail of procQ */
		addokbuf("\nline 110");
		((*tp)->p_next->p_prev) = ((*tp)->p_prev); /* adjust prev pointer for head of procQ */
		addokbuf("\nline 112");
		(*tp) = ((*tp)->p_prev); /* adjust tp for procQ */
		addokbuf("\nfinished outProcQ 114");
		return (*tp);
	}
	/* condition 3 */
	pcb_PTR current = (*tp)->p_next;

	if (current == NULL) {
		return NULL;
	}

	addokbuf("\nline 118");
	while (current != (*tp)) { /*while current != tail pcb, i.e. the first one we checked */
		addokbuf("\nline 120");
		if (current == p) {  /* find right pcb then... */
			addokbuf("\nline 122");
			(p->p_prev->p_next) = (p->p_next); /* redo next and prev pointers for nodes adjacent to p */
			addokbuf("\nline 124");
			(p->p_next->p_prev) = (p->p_prev);
			addokbuf("\noutProcQ finished");
			return p;
		}		
		addokbuf("\nline 129");
		current = (current->p_next);
		addokbuf("\nline 131");
	}
	/* case 3: target pcb not found in procQ */
	addokbuf("\noutProcQ finished");
	return NULL;		
}

pcb_PTR headProcQ (pcb_PTR tp){
	addokbuf("\nentered headProcQ");
	if (emptyProcQ(tp)) {
		addokbuf("\nheadProcQ finished");
		return NULL;
	}
	addokbuf("\nheadProcQ finished");
	return (tp);
}

int emptyChild (pcb_PTR p){
	addokbuf("\nentered and finished? emptyChild");
	return (p->p_child == NULL);
}

/* assumption: it's fine to insert new child at the head of the child list and it's fine to have child lists singly linked */
/* cases: 1) empty child list, 2) non-empty child list */
void insertChild (pcb_PTR prnt, pcb_PTR p){
	addokbuf("\nentered insertChild");
	pcb_PTR firstChild = prnt->p_child;

	if (emptyChild(prnt)) { /* case 1 */
		prnt->p_child = p; /* set p as child or prnt */
		addokbuf("\nfinished insertChild");
		return;
	}
	/*case 2*/
	prnt->p_child = p; /* set p as new first child of prnt */
	p->p_sib = firstChild;
	firstChild->p_prevSib = p; /*adjust original first child's prev ptr */
	addokbuf("\nfinished insertChild");
}

pcb_PTR removeChild (pcb_PTR p){
	addokbuf("\nentered and finished? removeChild");
	return outChild(p->p_child);
}

/* five conditions to account for: 1) p has no parent, 2) p is only child of its parent, 3) more than one pcb in child list and target p is first one, 4) it's one of more than one and isn't the first */
pcb_PTR outChild (pcb_PTR p){ 
	addokbuf("\nentered outChild");
	if (p->p_prnt == NULL) { /* case 1 */
		addokbuf("\nfinished outChild case 1");
		return NULL;
	}
	if (p->p_prnt->p_child == p) { /* if p is the first child, either falls into case 2 or 3 */
		if (p->p_sib == NULL) { /* case 2 - p is only child */
			p->p_prnt->p_child = NULL; /* no more children of its parent */
			addokbuf("\nfinished outChild case 2");
			return p;
		}
		/* case 3 */
		p->p_prnt->p_child = p->p_sib; /* next child is now first child */
		p->p_prnt->p_child->p_prevSib = NULL;
		addokbuf("\nfinished outChild case 3");
		return p;
	}
	/* case 4, we know p isn't first child of its parent, so either p is end of child list or it's not */
	if (p->p_sib == NULL) { /* p is last node on child list */
		p->p_prevSib->p_sib == NULL;
		addokbuf("\nfinished outChild case 4a");
		return p;
	}
	/* still case 4, p is somewhere in the middle of the child list */
	p->p_prevSib->p_sib = p->p_sib; /* adjust prev and next pointers of p's next and prev, respectively */
	p->p_sib->p_prevSib = p->p_prevSib;
	addokbuf("\nfinished outChild case 4b");
	return p;
}
