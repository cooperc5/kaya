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
	addokbuf("\nentered insertChild");

	p=cleanChild(p);

	if (emptyChild(prnt)) { /* case 1 */
		prnt->p_child = p; /* set p as child */
		p->p_prnt = prnt;
		p->p_prevSib = NULL;
		p->p_sib = NULL;
		addokbuf("\nfinished insertChild");
		return;
	}
	/*case 2*/
	pcb_PTR firstChild = prnt->p_child;

	prnt->p_child = p; /* set p as new first child of prnt */
	p->p_sib = firstChild;
	firstChild->p_prevSib = p; /*adjust original first child's prev ptr */
	p->p_prevSib = NULL;
	p->p_prnt = prnt;
	addokbuf("\nfinished insertChild");
}


pcb_PTR removeChild (pcb_PTR p){
	addokbuf("\nentered removeChild");
	if (emptyChild(p)) {
		addokbuf("\nremoveChild emptyChild true");
		return NULL;
	}
	pcb_PTR firstChild = p->p_child;
	if (firstChild->p_sib == NULL) { /* if p is only child */
		p->p_child = NULL;
		addokbuf("\nremoveChild line 213 finished");
		return cleanChild(firstChild);
	}
	/* not only child */
	firstChaddokbuf("\nremoveChild line 213");ild->p_sib->p_prev = NULL;
	p->p_child = firstChild->p_sib;
	addokbuf("\nremoveChild line 219 finished");
	return cleanChild(firstChild);
}


/* five conditions to account for: 1) p has no parent, 2) p is only child of its parent, 3) more than one pcb in child list and target p is first one, 4) it's one of more than one and isn't the first */
pcb_PTR outChild (pcb_PTR p){ 
	addokbuf("\nentered outChild");
	if (p->p_prnt == NULL) { /* case 1 */
		addokbuf("\nfinished outChild case 1");
		return NULL;
	}
	/*if (emptyChild(p->p_prnt)) {
		addokbuf("\noutChild emptyChild true (case 2)");
		return NULL;
	}*/
	if (p->p_prnt->p_child == p) { /* if p is the first child, either falls into case 2 or 3 */
		addokbuf("\noutChild line 236 finished");
		return removeChild(p->p_prnt);
	}
	/* case 4, we know p isn't first child of its parent, so either p is end of child list or it's not */
	if (p->p_sib == NULL) { /* p is last node on child list */
		p->p_prevSib->p_sib == NULL;
		addokbuf("\nfinished outChild case 4a");
		return cleanChild(p);
	}
	/* still case 4, p is somewhere in the middle of the child list */
	p->p_prevSib->p_sib = p->p_sib; /* adjust prev and next pointers of p's next and prev, respectively */
	p->p_sib->p_prevSib = p->p_prevSib;
	addokbuf("\nfinished outChild case 4b");
	return cleanChild(p);
}


