#ifndef CONSTS
#define CONSTS

/**************************************************************************** 
 *
 * This header file contains utility constants & macro definitions.
 * 
 ****************************************************************************/

/* Maximum process blocks */
#define MAXPROC  		20
#define MAXINT 2147483647
#define MAXDEVSEM 49

/* Hardware & software constants */
#define PAGESIZE		4096	/* page size in bytes */
#define WORDLEN			4		/* word size in bytes */
#define PTEMAGICNO		0x2A

#define ROMPAGESTART	0x20000000	 /* ROM Reserved Page */

/* timer, timescale, TOD-LO and other bus regs */
#define RAMBASEADDR	0x10000000
#define TODLOADDR	0x1000001C
#define INTERVALTMR	0x10000020	
#define TIMESCALEADDR	0x10000024

/* utility constants */
#define	TRUE		1
#define	FALSE		0
#define ON              1
#define OFF             0
#define HIDDEN		static
#define EOS		'\0'
#define FULLBYTE 0x000000FF
#define RESERVED 0x00000028

#define NULL ((void *)0xFFFFFFFF)

/* syscalls */
#define CREATEPROCESS 1
#define TERMINATEPROCESS 2
#define VERHOGEN 3
#define PASSEREN 4
#define SPECIFYEXCEPTIONSTATEVECTOR 5
#define GETCPUTIME 6
#define WAITFORCLOCK 7
#define WAITFORIODEVICE 8

/* important status register bits */
#define OFF 0x00000000
#define IEc 0x00000001
/*#define KUc 0x00000002
#define IEp 0x00000004
#define KUp 0x00000008
#define IEo 0x00000010
#define KUo 0x00000020 */
#define IM  0x0000FF00
/*#define BEV 0x00400000
#define VMc 0x01000000
#define VMp 0x02000000
#define VMo 0x04000000 */
#define TE  0x08000000
/* #define CU  0x10000000 */
#define INTERRUPTSON 0x00000004

/* Set quantum value to 5 miliseconds and pseudo clock interval */
#define INTERVAL 100000
#define QUANTUM 5000

/* important cause register hex codes */
#define BLANKEXCCODE 0xFFFFFF83
#define EXC_RI 0x00000028

/* processor state areas */
/* SYSYCALLS */
#define SYSCALLNEWAREA 0x200003D4
#define SYSCALLOLDAREA 0X20000348
/* Program trap */
#define PRGMTRAPNEWAREA 0x200002BC
#define PRGMTRAPOLDAREA 0x20000230
/* Table management */
#define TBLMGMTNEWAREA 0x200001A4
#define TBLMGMTOLDAREA 0x20000118
/* Interrupt handling */
#define INTERRUPTNEWAREA 0x2000008C
#define INTERRUPTOLDAREA 0x20000000

/* clock sem number */
#define CLOCK MAXDEVSEM - 1


/* vectors number and type */
#define VECTSNUM	4
#define TLBTRAP		0
#define PROGTRAP	1
#define SYSTRAP		2
#define TRAPTYPES	3

/*bit maps */
#define INTDEVREG 0x10000050

/*masks*/
#define IPMASK 8


/* device interrupts */
#define MAINDEVOFFSET 3
#define DISKINT	3
#define TAPEINT 4
#define NETWINT 5
#define PRNTINT 6
#define TERMINT	7
#define DEVINTNUM 5
#define DEVPERINT 8

#define DEVREGLEN	4	/* device register field length in bytes & regs per dev */
#define DEVREGSIZE	16 	/* device register size in bytes */

/* device register field number for non-terminal devices */
#define STATUS		0
#define COMMAND		1
#define DATA0		2
#define DATA1		3

/* device register field number for terminal devices */
#define RECVSTATUS      0
#define RECVCOMMAND     1
#define TRANSTATUS      2
#define TRANCOMMAND     3


/* device common STATUS codes */
#define UNINSTALLED	0
#define READY		1
#define BUSY		3

/* device common COMMAND codes */
#define RESET		0
#define ACK		1

#define SUCCESS 0
#define FAILURE -1

/* operations */
#define	MIN(A,B)	((A) < (B) ? A : B)
#define MAX(A,B)	((A) < (B) ? B : A)
#define	ALIGNED(A)	(((unsigned)A & 0x3) == 0)

/* Useful operations */
#define STCK(T) ((T) = ((* ((cpu_t *) TODLOADDR)) / (* ((cpu_t *) TIMESCALEADDR))))
#define LDIT(T)	((* ((cpu_t *) INTERVALTMR)) = (T) * (* ((cpu_t *) TIMESCALEADDR))) 
/* returns true if bit in position pos is on */
#define CHECK_BIT(var,pos) (((var)>>(pos)) & 1)


#endif

