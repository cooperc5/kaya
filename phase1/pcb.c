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


/* four conditions to account for: 1) p is only pcb in procQ, 2) more than one pcb in procQ and target pcb is tail, 3) it's one of more than one and isn't the tail, 4) or it's not there */
pcb_PTR outProcQ(pcb_PTR* tp, pcb_PTR p) {
		/* when removing a pcb_t from a process queue
		pointed to by pcb_t - there aree three cases to
		consder. FIRST: the tp points to an empty pcb_t
		process queue - meaning there is no list to take
		off. SECOND: , there is only one pcb_t remaining in
		the list - therefore its tp must be updated.
		THIRD: the pcb_t is adjectent to 1 or more
		pcb_t and must be adjusted */
		if(emptyProcQ(*tp)) {
			/* there is no process queue. our work here is done */
			return NULL;
		} else {
			pcb_PTR rmvdPcb = NULL;
			/* the process queue is not empty */
			if((*tp) == p) {
				/* here, the pcb_t we are searching for
				is the tp. From here, there are the following possibilities */
				if((*tp)->p_next == (*tp)) {
					/* here, we are seacrhing for the tp and it is
					the last pcb_t on the list. We therefore adjust to this
					by making the list queue null and returning our found tp */
					rmvdPcb = (*tp);
					/* goodbye */
					(*tp) = mkEmptyProcQ();
					/* what we were looking for was the tail pointer - all done! */
					return rmvdPcb;
				} else {
					/* this takes a little more work. we are looking
					for the tp but there are pcb_t left in the list.
					adjust the adjacent pcb_t tp accordingly */
					rmvdPcb = (*tp);
					/* reallocate pointers */
					(*tp)->p_next->p_prev = (*tp)->p_next;
					(*tp)->p_prev->p_next = (*tp)->p_next;
					/* reasign the tp - someones lucky day */
					(*tp) = (*tp)->p_prev;
					return rmvdPcb;
				}
			} else {
				/* since the pcb_t we are looking for is not the tp
				of the list, we start at the head and work our way down the
				list intil we find the pcb_t we need. if we cant find it,
				return null */
				pcb_PTR currentPcb = (*tp)->p_next;
				/* keep going until we are back where we started */
				while(currentPcb != (*tp)) {
					if(currentPcb == p) {
						/* a match! we found the pcb_t */
						if(currentPcb == (*tp)->p_next) {
							/* in this case, we had to search for the pcb_t
							but if it is at the head, i.e. the tp next,
							then simply implement the removeProcQ function to
							accomplish this task and save some work */
							return removeProcQ(tp);
						} else {
							/* this is perhaps the most taxing case; here,
							we had to search for the pcb_t, and it wasnt the head.
							if we find it, we have to do some pointer rearranging */
							currentPcb->p_next->p_prev = currentPcb->p_prev;
							currentPcb->p_prev->p_next = currentPcb->p_next;
							/* jobs all done */
							return currentPcb;

						}
					} else {
						/* no match. make the current point to the next pcb_t */
						currentPcb = currentPcb->p_next;
					}
				}
				/* if this loop exits, then that means it wasnt in the queue
				to begign with. return null */
				return NULL;
			}
		}
}

pcb_PTR removeProcQ(pcb_PTR *tp) {
	pcb_PTR rmvdPcb = NULL;
	/* first, consider the case in which the process queue is
	empty */
	if(emptyProcQ(*tp)) {
		/* empty list */
		return NULL;
	}
	/* next what must be considered are the cases for having the
	tp be the only element in the list, in which
	case, its pointers must be reasigned */
	 else if(((*tp)->p_next) == (*tp)) {
		/* tp is the last remaining */
		/* get the return value - no reason to
		call p_next - it is the head */
		rmvdPcb = (*tp);
		/* asign the next to be null, since
		it was just removed */
		(*tp) = mkEmptyProcQ();
		return rmvdPcb;
	}
	/* the case where there is >1 elements in the tree;
	this cam be tricky, as poimters get reasigned; first,
	get the head of the list */
	rmvdPcb = (*tp)->p_next;
	/* here, reasign the the tp, so it is pointing
	at the next item's next item */
	(*tp)->p_next->p_next->p_prev = (*tp);
	/* reasign the pt to be the next */
	(*tp)->p_next = ((*tp)->p_next->p_next);
	return rmvdPcb;
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

int emptyChild (pcb_PTR p){
	addokbuf("\nentered and finished? emptyChild");
	return (p->p_child == NULL);
}

/* assumption: it's fine to insert new child at the head of the child list and it's fine to have child lists singly linked */
/* cases: 1) empty child list, 2) non-empty child list */
void insertChild (pcb_PTR prnt, pcb_PTR p){
	addokbuf("\nentered insertChild");

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
	addokbuf("\nentered and finished? removeChild");
	if (emptyChild(p)) {
		return NULL;
	}
	return outChild(p->p_child);
}

/* five conditions to account for: 1) p has no parent, 2) p is only child of its parent, 3) more than one pcb in child list and target p is first one, 4) it's one of more than one and isn't the first */
pcb_PTR outChild (pcb_PTR p){ 
	addokbuf("\nentered outChild");
	if (p->p_prnt == NULL) { /* case 1 */
		addokbuf("\nfinished outChild case 1");
		return NULL;
	}
	if (emptyChild(p->p_prnt)) {
		return NULL;
	}
	if (p->p_prnt->p_child == p) { /* if p is the first child, either falls into case 2 or 3 */
		if (p->p_sib == NULL) { /* case 2 - p is only child */
			p->p_prnt->p_child = NULL; /* no more children of its parent */
			addokbuf("\nfinished outChild case 2");
			return cleanPcb(&(*(p)));
		}
		/* case 3 */
		p->p_prnt->p_child = p->p_sib; /* next child is now first child */
		p->p_prnt->p_child->p_prevSib = NULL;
		addokbuf("\nfinished outChild case 3");
		return cleanPcb(&(*(p)));
	}
	/* case 4, we know p isn't first child of its parent, so either p is end of child list or it's not */
	if (p->p_sib == NULL) { /* p is last node on child list */
		p->p_prevSib->p_sib == NULL;
		addokbuf("\nfinished outChild case 4a");
		return cleanPcb(&(*(p)));
	}
	/* still case 4, p is somewhere in the middle of the child list */
	p->p_prevSib->p_sib = p->p_sib; /* adjust prev and next pointers of p's next and prev, respectively */
	p->p_sib->p_prevSib = p->p_prevSib;
	addokbuf("\nfinished outChild case 4b");
	return cleanPcb(&(*(p)));
}
