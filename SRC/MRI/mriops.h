#ifndef MRIOPS_H
#define MRIOPS_H

struct mrihostop
{
	UINT_8	opcode;
	UINT_8	oplength[3];
	int		cmmid;
	int		version;
	int		slotid;
	int		vnmsid;
	UINT_8	originator;
	UINT_8	action;
	UINT_8	reserved[2];
} ;

typedef struct mrihostop MRIHOSTOP;

// same structure used for OP_SET_SEND_BOARD and OP_SET_SEND_BOARD_ACK
struct mrisetsendbdop
{
	UINT_8	opcode;
	UINT_8	oplength[3];
	UINT_8	hostnumber;
	UINT_8	reqiomgr;
	UINT_8	sendiomgr;
    UINT_8  SetSendAck;
} ;

typedef struct mrisetsendbdop MRISETSENDBDOP;

// Values for originator	
#define	INITMGR		0
#define	IOMGR		1

// Values for action
#define BROADCAST	0
#define NOACTION	1

#endif