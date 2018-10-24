#include "../h/const.h"
#include "../h/types.h"

#include "/usr/include/uarm/libuarm.h"
#include "../e/pcb.e"
#include "../e/asl.e"

#define TRUE 1
#define FALSE 0

/* PCB.C */

HIDDEN pcb_PTR pcbFree_h;


void freePcb (pcb_PTR p){
	insertProcQ(&pcbFree_h, p);
}

pcb_PTR allocPcb (){
	pcb_PTR tmp = removeProcQ(&pcbFree_h);

	/*if (tmp != NULL) {
		tmp->p_next = NULL;  /* initialize fields */
		/*tmp->p_prev = NULL;
		tmp->p_prnt = NULL;
		tmp->p_child = NULL;
		tmp->p_sib = NULL;
		tmp->p_semAdd = NULL; 
	}*/
	
	tmp->p_next = NULL;  /* initialize fields */
	tmp->p_prev = NULL;
	tmp->p_prnt = NULL;
	tmp->p_child = NULL;
	tmp->p_sib = NULL;
	tmp->p_semAdd = NULL;  

	if(tmp != NULL) return tmp;
	return NULL;
}

void initPcbs (){
	static pcb_t *foo[MAXPROC];	
	int i;
	for (i = 0; i < MAXPROC; i++) {
		foo[i] = mkEmptyProcQ();
		freePcb(foo[i]);
	}
}



pcb_PTR mkEmptyProcQ (){
	return NULL;
}

int emptyProcQ (pcb_PTR tp){
	return (tp == NULL);
}

/* cases: 1) empty procQ, 2) non-empty procQ */
void insertProcQ (pcb_PTR *tp, pcb_PTR p){
	if (emptyProcQ(*tp)) { /* empty q case 1 */
		*tp = p;
		p->p_next = p;
		p->p_prev = p;
		return;
	}
	/* non-empty q */
	else { /* case 2 */
		pcb_PTR tail = (*tp);
		p->p_next = tail->p_next; /* set next of p */
		p->p_prev = tail; /* set prev of p */
		tail->p_next = p; /* set next of previous tail */
		tail->p_next->p_prev = p; /* set prev of head */
	}
	/* set tail pointer */
	(*tp) = p;
}

pcb_PTR removeProcQ (pcb_PTR *tp){    /* might need to add just one pcb or empty procQ condition */
	return outProcQ(tp, *tp);
}

/* four conditions to account for: 1) p is only pcb in procQ, 2) more than one pcb in procQ and target pcb is first one, 3) it's one of more than one and isn't the first, 4) or it's not there */
pcb_PTR outProcQ (pcb_PTR *tp, pcb_PTR p){
	pcb_PTR firstPcb = *tp;
	if (firstPcb == p) { /* first pcb is p */
		if (p->p_next == p) { /* case 1: p is only pcb on the procQ tp */
			(*tp) = NULL; /* set the tp to null to indicate an empty procQ */
			return p;
		}
		/* condition 2 */
		p->p_prev->p_next = p->p_next; /*adjust next pointer for new tail of procQ */
		p->p_next->p_prev = p->p_prev; /* adjust prev pointer for head of procQ */
		*tp = p->p_prev; /* adjust tp for procQ */
	}
	/* condition 3 */
	pcb_PTR current = firstPcb->p_next; /* current is now head pcb of procQ */
	while (current != firstPcb) { /*while current != tail pcb, i.e. the first one we checked */
		if (current == p) {  /* find right pcb then... */
			p->p_prev->p_next = p->p_next; /* redo next and prev pointers for nodes around p */
			p->p_next->p_prev = p->p_prev;
			return p;
		}		
		current = current->p_next;
	}
	/* case 3: target pcb not found in procQ */
	return NULL;		
}

pcb_PTR headProcQ (pcb_PTR tp){
	if (emptyProcQ(tp)) return NULL;
	return (tp);
}

int emptyChild (pcb_PTR p){
	return (p->p_child == NULL);
}

void insertChild (pcb_PTR prnt, pcb_PTR p){
	insertProcQ(&prnt->p_child, p);
}

pcb_PTR removeChild (pcb_PTR p){
	return removeProcQ(&p->p_child);
}

pcb_PTR outChild (pcb_PTR p){ /* do you need to search each process block to find the one that has p as a child? */
	pcb_PTR *prnt = &(p->p_prnt);	
	return outProcQ(prnt, p);
}



