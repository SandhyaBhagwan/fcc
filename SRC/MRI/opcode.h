// Operation Class and Operation Code Definitions

#ifndef INCLUDED_OPCODE_H
#define INCLUDED_OPCODE_H

/**********************************************************************/
/* Note that external op classes that are specified in the TSP        */
/* functional spec are defined in the range 1 to 127.  Internal op    */
/* classes that are internal to the TSP are defined in the range 128  */
/* to 253.  Class 254-255 are reserved for the host interface.        */
/**********************************************************************/

/**********************************************************/
/* TSP external opclass defines                           */
/**********************************************************/

/* ----------------------------------
 * VOICE class and opcode definitions
 * ----------------------------------
 *
 * The following defines decribe the opcodes for all VOICE operations.
 * These operations are used to store and retrieve voice data for
 * use by a TSP PRIM.  The opcodes from 0x01 - 0x1f are exchanged
 * between VNMS and the TSP.  The opcodes 0x20 and greater are used
 * within the TSP to process voice data internally.  Internal voice
 * operations are exchanged between the PDP, PRIM and PDSP boards.
 */
# define	CL_VOICE		0x01	/* Voice Class */
# define		OP_SENDGRP6		0x01	/* Send Group */
# define		OP_VMIDATA6		0x02	/* VMS/VMI DATA */
# define		OP_VMSDATA6		0x03	/* VMS DATA */
# define		OP_CTERMGRP		0x04	/* Conditional Terminate Grp */
# define		OP_TERMGRP		0x05	/* Terminate Group */
# define		OP_VMSREQ6		0x06	/* VMS Request */
# define		OP_LISTEN		0x07	/* VMS Listen Data */
# define		OP_GRPRSLT		0x08	/* Group Result */
# define		OP_SENDFILE		0x09	/* Send Group from File */
# define		OP_ALLOWGRP		0x0a	/* Allow Group */
# define		OP_PONG			0x0b	/* PONG (ack to PING) */
# define		OP_TSPCLRACK	0x0c	/* TSP CLEAR ACK */
# define        OP_RECOG_TRAIN6 0x0d    /* Recognize or Train Voice */
# define        OP_RECOGNIZEDV  0x0e    /* Recogized Result */
# define        OP_TRAINEDV     0x0f    /* Trained Voice */
# define        OP_DELETEWORD6  0x10    /* Delete Word from Vocab */
# define        OP_WORDDELETED  0x11    /* Word Deleted from Vocab */
# define        OP_SAMPLENOISE  0x12    /* Sample Background Noise */
# define        OP_NOISELEVEL   0x13    /* Noise Level of call */
# define		OP_PURGE_DTMF	0x14	/* Purge DTMF */
# define		OP_RETRIEVE_DTMF 0x15	/* Retieve DTMF */
# define		OP_SENDGRP		0x16	/* Send Group   - 12 byte msgnum */
# define		OP_VMIDATA		0x17	/* VMS/VMI DATA - 12 byte msgnum */
# define		OP_VMSDATA		0x18	/* VMS DATA     - 12 byte msgnum */
# define		OP_VMSREQ 		0x19	/* VMS Request  - 12 byte msgnum */
# define        OP_RECOG_TRAIN	0x1a   /* Recognize or Train Voice - 12 byte msgnum */
# define        OP_DELETEWORD	0x1b    /* Delete Word from Vocab  - 12 byte msgnum*/
# define        OP_STOP_CLEAR	0x1c    /* Suppress TSP Clear on Conditional Reinit */
#define         OP_HMS_CKTINUSEACK    0x1d    /* Shared CMM - HMS response to CircuitInUse request */
#define         OP_HMS_HOSTOFFLINEACK 0x1e    /* Shared CMM - HMS ack to offline op */
#define         OP_HMS_LINKSETINUSEACK 0x1f   /* Shared CMM - HMS response to LinksetInUse request */

# define        OP_START        0x20    /* Start Group */
# define        OP_STOP         0x21    /* Stop Group */
# define        OP_TALK         0x22    /* Talk Data */
# define        OP_TSPLIST      0x23    /* TSP Listen Data */
# define        OP_DSPREQ       0x24    /* DSP Request */
# define        OP_DSPRPL       0x25    /* DSP Response */
# define        OP_CANCEL       0x26    /* Cancel Group */
# define        OP_UNDERUN      0x27    /* Underun */
# define        OP_PAUSE        0x28    /* Pause */
# define        OP_DSPSET       0x29    /* DSP Set */
# define		OP_DSPSETR		0x2a	/* DSP Set Response */
# define        OP_DSPCNCL      0x2b    /* DSP Cancel */
# define		OP_DSPSTATUS	0x2c	/* ESP / PDSP Status */
# define		OP_STOPPROMPT	0x2d	/* Stop Prompt for voice barge-in */
# define        OP_VOCABDATA    0x2e    /* Trained VPC Vocab Data */
# define        OP_CALLUPD      0x2f    /* Start/stop call */

# define		OP_INT_PERFORM_SIG	0X30	/* Generate single or dual tone */

# define        OP_CLEARSTATE   0x40    /* Clear Voice State Machine */



/* Signaling OPs */

# define	CL_SIGNALING		0x02
# define		OP_INITIATE			0x01
# define		OP_ALERTING			0x02
# define		OP_ANSWER			0x03
# define		OP_TERMINATE		0x04
# define		OP_NEW_INITIATE		0x05
# define		OP_NEW_ALERTING		0x06
# define		OP_NEW_ANSWER		0x07
# define		OP_NEW_TERMINATE	0x08
# define		OP_NETOP			0x09
# define		OP_NETOP_ACK		0x0a
# define		OP_PERFORM_SIG		0x0b
# define		OP_PERFORM_SIG_ACK	0x0c
# define		OP_CONTINUITY		0x0d
# define		OP_TRANSFER_PORT	0x0e

/* Event OPs */

# define	CL_EVENT			0x03
# define		OP_DTMF_EVENT		0x01
# define		OP_NONDTMF_TONE		0x02
# define		OP_UNCOND_TERM		0x05
# define		OP_PING				0x0b	/* PING sent by CM */

/* RFS OPs */

# define	CL_RFS				0x04
# define		OP_OPEN				0x01
# define		OP_OPEN_ACK			0x02
# define		OP_CLOSE			0x03
# define		OP_CLOSE_ACK		0x04
# define		OP_READ				0x05
# define		OP_READ_ACK			0x06
# define		OP_WRITE			0x07
# define		OP_WRITE_ACK		0x08

/* TSP Configuration/Initialization OPs */

# define	CL_CONFIG			0x05
# define		OP_REINITIALIZE		0x01
# define		OP_TSP_READY		0x02
# define		OP_VNMS_READY		0x03
# define		OP_TSP_RESET		0x04
# define        OP_TSP_INQUIRY      0x05
# define		OP_IDENTIFY			0x06
# define		OP_UPDATE_CONFIG	0x07
# define        OP_RESET_ISA        0x08

/* Line Event OPs */

# define	CL_LINE_EVENT		0x06
# define		OP_PRIM_L_EVENT		0x02
# define		OP_PRIM_L_STATE		0x03
# define		OP_PRIM_L_STATE_REQ	0x04
# define		OP_CAMP_ON_MAINT	0x05
# define		OP_RESTORE_TO_IDLE	0x06
# define		OP_SIG_AVAIL		0x09
# define		OP_SIG_UNAVAIL		0x0a
# define		OP_TSPCLR			0x0c	/* TSP CLEAR sent by CM */
# define		OP_CKT_GRP_RESET	0x10
# define		OP_SUSP_CKT_USE		0x11
# define		OP_SUSP_CKT_USE_ACK	0x12
# define		OP_RESM_CKT_USE		0x13
# define		OP_RESM_CKT_USE_ACK	0x14
# define		OP_CKT_USE_INQUIRY	0x15
# define		OP_CKT_USE_STATUS	0x16
# define		OP_DPC_SIG_AVAIL	0x19
# define		OP_DPC_SIG_UNAVAIL	0x1a
# define		OP_SIGPATH_AVAIL	0x19
# define		OP_SIGPATH_UNAVAIL	0x1a
# define		OP_LINKSET_ENABLE	0x1b
# define		OP_LINKSET_DISABLE	0x1c
# define        OP_ROUTESET_QUERY   0x1d
/* High Density Start */
/*
Circuit Maintenance Request opcode used to
suspend/resume a HD E1/IPLink port which
does not have MRB voice resource attached.
*/
#define         OP_CKT_MAINT_RQST   0x1b
/* High Density End */

/* NAPLOG entry OP */

# define	CL_NAP_LOG			0x07

/* PRIM Signaling OPs */

# define	CL_PRIM_SIG			0x08
# define		OP_EHAIRPIN			0x01
# define		OP_CLR_EHAIRPIN		0x02
# define		OP_HAIRPIN			0x07
# define		OP_CLR_HAIRPIN		0x08
# define		OP_HAIRDATA			0xf3	/* Internal HAIRDATA op */
# define		OP_IHAIRPIN			0xf5	/* Internal HAIRPIN op */
# define		OP_CLR_IHAIRPIN		0xf6	/* Internal Clr HAIRPIN op*/

/* Signaling Maintenance OPs */

# define	CL_SIG_MAINT		0x09

/* Host Interface Management OPs */

# define	CL_HST_MGT			0x0a
# define		OP_W_UNIT_SWITCH	0x01
# define		OP_R_UNIT_SWITCH	0x02
# define		OP_TSP_HEARTBEAT	0x03
# define		OP_R_XFR_REQ		0x0a
# define		OP_R_XFR_ACK		0x0b
# define		OP_R_XFR_CMPL		0x0c
# define		OP_W_XFR_REQ		0x0d
# define		OP_W_XFR_ACK		0x0e
# define		OP_STOP_R_REQ		0x0f
# define		OP_STOP_R_ACK		0x10
# define		OP_SET_SLAVE_HIP	0x11
# define		OP_SET_SH_PARAMS	0x12
# define		OP_CUT_TRACEFILE	0x13
# define		OP_MODE_CHANGE		0x14
# define		OP_WRITE_PING		0x15

    // Internal CMM ops
# define        OP_POLL_TELESERVER  0xf0 /* poll to Teleserver */
# define        OP_ACK_TELESERVER   0xf1 /* ack from Teleserver */

# define        OP_RC_SWITCHOVER        0xf2 /* send switchover request */
# define        OP_RC_SUSPEND_SEND      0xf3 /* suspend send queue request */
# define        OP_RC_SUSPEND_SEND_ACK  0xf4 /* suspend send queue ack */
# define        OP_RC_GET_HOST_BOUND    0xf5 /* request for queued host-bound buffs */
# define        OP_RC_HOST_BOUND        0xf6 /* send one host-bound buffer */
# define        OP_RC_BUFFERS_SENT      0xf7 /* last host-bound buffer sent */
# define        OP_RC_GET_RCV_SEQ       0xf8 /* suspend rcvs and return seq nums */ 
# define        OP_RC_GET_RCV_SEQ_ACK   0xf9 /* suspend rcvs and return seq nums ack */
# define        OP_RC_RCV_COMPLETE      0xfa /* receive switch complete */ 
# define        OP_RC_GET_IO_STATUS     0xfb /* request read/write unit status */ 
# define        OP_RC_IO_STATUS         0xfc /* return read/write unit status */ 
# define        OP_RC_SET_SEND_BD_ACK   0xfd /* RM ack of request to set send MDP board */
# define        OP_IOS_IBTS_POLL        0xfe /* IOServices and IBTS poll OP to check the IP Connection */
 

/* TSP Management OPs */

# define	CL_TSP_MGT			0x0b
# define		OP_BD_HEARTBEAT		0x01
# define		OP_BD_FAILURE		0x02
# define		OP_BD_RECOVERY		0x03
# define        OP_BD_START_HB      0x04
# define        OP_BD_STOP_HB       0x05
# define        OP_ESP_ALG_MASK     0x06
# define        OP_PM_POLL          0x07
# define        OP_PM_POLL_ACK      0x08
# define        OP_REQ_SPAN_STAT    0x09
# define        OP_MRI_HMSSTART     0x0a
# define        OP_MRI_HMSSTOP      0x0b
/* HSK Start */
# define        OP_START_BOARDV     0x0c
# define        OP_STOP_BOARDV      0x0d
# define        OP_STATUS_BOARDV    0x0e
/* HSK End */

/* Host Interface NO-OP Class */

# define	CL_HST_NOOP			0x0c
# define		OP_WRITE_RESPONSE	0x02

/**********************************************************/
/* SS7 Management Messages                                */
/**********************************************************/

# define	CL_CONTROL				0x0d
# define		OP_STARTSIGNALING 		0x01
# define		OP_STOPSIGNALING 		0x02
# define		OP_CRITICAL_MODULE		0x04	/* the alias for this op is POLLRETURN */
# define        OP_FAILEDSIGNALING      0x04    /* alias for OP_CRITICAL_MODULE */
# define		OP_SUSPENDSIGNALING		0x05	
# define		OP_RESUMESIGNALING		0x06

/* Data Transmission (fax) Class */
# define	CL_DATATRANS		0x0e
# define		OP_SENDDATA6		0x01	/* Send Data Group (obsolete) */
# define		OP_VMIDATA6			0x02	/* VMS/VMI DATA (obsolete) */
# define		OP_VMSDATA6			0x03	/* VMS DATA (obsolete) */
# define		OP_CTERMGRP			0x04	/* Conditional Terminate Grp */
# define		OP_TERMGRP			0x05	/* Terminate Group */
# define		OP_VMSREQ6			0x06	/* VMS Request (obsolete) */
# define		OP_RETURNDATA		0x07	/* Return Data */
# define		OP_DATARESULT		0x08	/* Data Group Result */
# define		OP_SENDDATAFILE		0x09	/* Send Data Group from File */
# define		OP_ALLOWGRP			0x0a	/* Allow Group */
# define		OP_EXTDATAREQ		0x0b	/* External Data File Request */
# define		OP_EXTDATA			0x0c	/* External Data */
# define		OP_RECVDATA			0x0d	/* Receive Data Group */
# define        OP_SENDDATA			0x16    /* Send Data Group - 12 byte msgnum */
# define        OP_VMIDATA			0x17    /* VMS/VMI DATA    - 12 byte msgnum */
# define        OP_VMSDATA			0x18    /* VMS DATA        - 12 byte msgnum */
# define        OP_VMSREQ			0x19    /* VMS Request     - 12 byte msgnum */



/**********************************************************/
/* Recovery Management Messages                           */
/**********************************************************/

# define         CL_RECOVERY                    0x0f
# define                OP_RECOVER_COMPONENT            0x01
# define                OP_START_COMPONENT              0x02
# define                OP_HALT_COMPONENT               0x03
# define                OP_COMPONENT_STATUS             0x04
# define                OP_RECOVERY_ACK                 0x05
# define                OP_RECOVERY_ENABLED             0x06
# define                OP_BOARD_FAILURE                0x07
# define                OP_RECOVERY_SW                  0x08
# define                OP_RECOVERY_ALERT               0x09
# define                OP_BOARD_ALERT                  0x0a
# define                OP_LOAD_CODE                    0x0b
# define                OP_START_BOARD                  0x0c
# define				OP_BD_ALERT_ACK					0x0d
# define				OP_HB_START						0x0e
# define				OP_HB_STOP						0x0f
# define				OP_HB_ACK						0x10
# define				OP_DISABLE_BD					0x11
# define				OP_DISABLE_ACK					0x12
# define                max_recovery_op                 18

/**********************************************************/
/* Host Management Services Messages                      */
/**********************************************************/

# define        CL_HMS              0x10
# define            OP_HMS_START            0x01
# define            OP_HOST_OFFLINE         0x02
# define            OP_HOST_ONLINE          0x03
# define            OP_TSP_OFFLINE          0x04
# define            OP_TSP_ONLINE           0x05
# define            OP_MASTER_SW            0x06
# define            OP_HOST_SHARED          0x07
# define            OP_HOST_NONSHARED       0x08
# define            OP_TSP_SHARED           0x09
# define            OP_TSP_NONSHARED        0x0a
# define			OP_HMS_INQUIRY			0x0b
# define			OP_HMS_STATUS			0x0c
# define			OP_HMS_SIG_STATUS		0x0d
# define			OP_ESP_ONLINE			0x0e
# define			OP_ESP_OFFLINE			0x0f
# define			OP_REMOTE_TSPRDY		0x10
# define			OP_NETWORK_DELAY		0x11
# define            OP_HMS_CIRCUITINUSE     0x12
# define			OP_HMS_LINKSETINUSE		0x13
# define            OP_ROUTESET_RSP         0x14
# define            max_hms_op              20

/***********************************************************/
/* Network Service Class Messages                          */
/***********************************************************/

# define            CL_SERVICE              0x11
# define                    OP_OPEN                 0x01
# define                    OP_OPEN_ACK             0x02
# define                    OP_CLOSE                0x03
# define                    OP_CLOSE_ACK            0x04
# define                    OP_SEND                 0x05
# define                    OP_RECEIVE              0x06
# define                    OP_CLOSE_REQ            0x07

/**********************************************************/
/* APC Class Messages					                  */
/**********************************************************/
# define		CL_APC	0x12
# define			OP_SYSTEM_EVENT		0x01


/* HSK Start */
/**********************************************************/
/* HotSwap Class Messages                                 */
/**********************************************************/

# define    CL_HOT_SWAP             0x13
# define        OP_CLOSE_PORT       0x01
# define        OP_OPEN_PORT        0x02
# define        OP_CMM_STATUS       0x03
/* HSK End */

/**********************************************************/
/* First unassigned external class number                 */
/* Update this number when new class numbers are assigned */
/**********************************************************/

# define    CL_EXT_CLASS_TOP    0x14


/**********************************************************/
/* CMM internal opclass defines                           */
/**********************************************************/

#define CL_INT_OPCLASS_BOTTOM		0x80

/**********************************************************/
/* Internal OPs MRI                                       */
/**********************************************************/

# define	CL_MRI			0x80
# define		OP_ATTACH_PORT			0x01
# define		OP_DETACH_PORT			0x02
# define		OP_VNMS_HOST			0x03
# define		OP_GREETINGS_REQUEST	0x04
# define		OP_GREETINGS_RESPONSE	0x05
# define		OP_INIT_COMPLETE		0x06
# define		OP_MDP_FAILURE			0x07
# define		OP_AVAILABLE_PORTS		0x08
# define		OP_SET_SEND_BOARD		0x09 // ACK is in CL_HST_MGT class


/**********************************************************/
/* Internal OPs SS7 / H110                                */
/**********************************************************/

# define	CL_SS7_MEDIA		0x81
# define		OP_H110INFO_REQ			0x01	// SS7 --> MS
# define		OP_H110INFO_RESP		0x02	// MS --> SS7
# define		OP_H110INFO_CONF		0x03	// MS --> SS7
#define			OP_H110INFO_XMITRCVTS   0x04    // TeleServer --> SS7 (the Rcv/Xmit TS on H110 bus)
#define			OP_H110TSINFO_COMPLETE	0x05	// TeleServer --> SS7 (H110 Info transmission complete)
# define        OP_H110INFO_UPDATE      0x06    // TeleServer --> SS7 (indicate timeslot updates forthcoming)
# define        OP_H110INFO_DELETE      0x07    // TeleServer --> SS7 (delete specified H.110 link)

/**********************************************************/
/* Internal OPs ESP Client Initialization                 */
/**********************************************************/

# define	CL_DNAP_CLIENT			0x8d
# define		OP_CLIENT_READY			0x01

/**********************************************************/
/* Internal OPs ESP Server Initialization                 */
/**********************************************************/

# define	CL_DNAP_SERVER			0x8e
# define		OP_SERVER_READY			0x01

/**********************************************************/
/* First unassigned internal class number                 */
/* Update this number when new class numbers are assigned */
/**********************************************************/

# define	CL_INT_OPCLASS_TOP	0x8f
#endif
