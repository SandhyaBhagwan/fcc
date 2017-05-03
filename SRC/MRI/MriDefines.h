/******************   MRI Defines ***********************/


#define BR_DATA_SZ_MRI					12
#define	MMF_size						200
#define MR_MAX_ROUTING_CLASSES			255
#define MAX_QUEUE_DEPTH                 600
#define MR_MAX_BOARDS					16
#define DEAD_MDP						30
#define MR_TIMEOUT_FOREVER				-1
#define MR_TIMEOUT_NOW					0
#define	MAXHOSTS						256
#define	CL_INT_OPCLASS_TOP				0x8f  // in opcode.h??
#define	MAXROUTES						256
#define	MAXPORTS						256
#define BOARD_COUNT_ZERO                0

//#define _D_DESKTOP 1

// Destination slot ID for Broadcast messages
#define MR_SENDALLBOARDS				0xff

#pragma warning (disable : 4200)


typedef enum
{
    DUMMY,       // 0
    ASMA,        // 1
    ASAPI,       // 2 
    CASISDN,     // 3   
    DNAPA,       // 4   
    SIP,         // 5  - was DNAPB, obsolete for SigSvr, repurposed to SIP
    HMS,         // 6
    IMGR,        // 7
    IOMA,        // 8 
    IOMB,        // 9
    MRI,         // a
    MS,          // b
    MSAL,        // c
    SIGROUTER,   // d
    SS7,         // e
    VNMS,        // f  - these are mapped internally by MRI to VNMSA or VNMSB
    VNMSA,		 //	10 - VNMS Routing Class on MDPA
    VNMSB,		 // 11 - VNMS Routing Class on MDPB
    PM,          // 12
    HB,          // 13
    BMGR,        // 14
    APC,         // 15
    HMS_IN,      // 16
    HMS_OUT,     // 17
/*EoMDP Start*/
	IBTS,        // 18
/*EoMDP End*/
/* HSK Start */
    HOTSWAP,     // 19
/* HSK End */
    MAXRCLASS    // 20
} MR_ROUTING;

#define VNMSy(x)	(VNMS + x)

typedef enum 
{	
    MR_STATUS_SUCCESS = 0,								// must match mbapi.h
    MR_STATUS_NO_RESOURCES,				// 1			// must match mbapi.h        
    MR_STATUS_INVALID_HANDLE,			// 2			// must match mbapi.h
    MR_STATUS_INVALID_PORTID,			// 3			// must match mbapi.h
	MR_STATUS_PORT_ALREADY_ATTACHED,	// 4			// must match mbapi.h
	MR_STATUS_PORT_INUSE,				// 5			// must match mbapi.h
	MR_STATUS_INVALID_PORT_HANDLE,		// 6			// must match mbapi.h
	MR_STATUS_PORT_ALREADY_DETACHED,	// 7			// must match mbapi.h
	MR_STATUS_INVALID_OPEN_HANDLE,		// 8			// must match mbapi.h
	MR_STATUS_INVALID_QUEUE_HANDLE,		// 9 			// must match mbapi.h
	MR_STATUS_TIMEOUT,					// 10 (0x0a)	// must match mbapi.h
	MR_STATUS_NO_MESSAGE,				// 11 (0x0b)	// must match mbapi.h
	MR_STATUS_INVALID_MESSAGE,			// 12 (0x0c)	// must match mbapi.h
	MR_STATUS_INVALID_PARAMETER,		// 13 (0x0d)	// must match mbapi.h
	MR_STATUS_FAILURE,					// 14 (0x0e)	// must match mbapi.h
	MR_STATUS_INVALID_SIGNALTYPE,		// 15 (0x0f)	// must match mbapi.h
	MR_STATUS_INVALID_DATA_LEN,			// 16 (0x10)	// must match mbapi.h
	MR_STATUS_COULD_NOT_CREATE_MMF,		// 17 (0x11)
	MR_STATUS_NULL_SIGNATURE,			// 18 (0x12)
	MR_STATUS_INVALID_SIGNATURE,		// 19 (0x13)
	MR_STATUS_INVALID_ROUTING_VECTOR,	// 20 (0x14)
	MR_STATUS_MUTEX_FAILURE,			// 21 (0x15)
	MR_STATUS_SEMAPHORE_FAILURE,		// 22 (0x16)
	MR_STATUS_INVALID_PROCESS_FOR_THIS_PORT, // 23 (0x17)
	MR_STATUS_RV_MMF_NOT_ACCESSIBLE,	// 24 (0x18)
	MR_STATUS_INVALID_OPCODE,			// 25 (0x19)
	MR_STATUS_INVALID_OPCLASS,			// 26 (0x1a)
	MR_STATUS_INVALID_RCLASS,			// 27 (0x1b)
	MR_STATUS_QUEUE_OVERFLOW,			// 28 (0x1c)
	MR_STATUS_INVALID_DATAPTR,			// 29 (0x1d)
	MR_STATUS_INVALID_ASYNC,			// 30 (0x1e)
	MR_STATUS_INVALID_MSGTYPE,			// 31 (0x1f)
	MR_STATUS_INVALID_DESTSLOT,			// 32 (0x20)
	MR_STATUS_INVALID_MRCB,				// 33 (0x21)
	MR_STATUS_INVALID_BOARD_ID,			// 34 (0x22)
	MR_STATUS_UNEXPECTED_EXCEPTION,		// 35 (0x23)
	MR_STATUS_RESOURCE_FAILURE,			// 36 (0x24)
	MR_STATUS_INVALID_REQUEST,			// 37 (0x25)
	MR_STATUS_INVALID_BUFTYPE,			// 38 (0x26)
	MR_STATUS_BROADCAST_FAILURE,		// 39 (0x27)
	MR_STATUS_INTERBOARD_SEND_FAILURE,	// 40 (0x28)
	MR_STATUS_INVALID_SENDDESC,			// 41 (0x29)
	MR_STATUS_WAIT_FAILURE,				// 42 (0x2a)
	MR_STATUS_INVALID_GLOBALS,			// 43 (0x2b)
	MR_STATUS_MISSING_MESSAGE,			// 44 (0x2c)
	MR_STATUS_NO_EVENT_Q,				// 45 (0x2d)
	MR_STATUS_PERFMON_NOT_REGISTERED,   // 46 (0x2e)
	MR_STATUS_UNKNOWN_EXCEPTION_THROWN,  // 47 (0x2f)
	MR_STATUS_FLUSH_DRIVER_BUFFERS_FAILED	//48 (0x30)
} MR_STATUS;

typedef enum 
{	
	MR_SIGNAL_NONE,
	MR_SIGNAL_SEMAPHORE

} MR_SIGNAL;

typedef enum 
{
	MR_EVENT_INVALID,
	MR_EVENT_SEND_COMPLETE
  
} MR_EVENT_ID;

typedef void *MR_HANDLE;



typedef struct _vnmspaths {
	unsigned int slotid;
	unsigned int rclass;
} VNMSPATHS;


typedef struct _Q_STRUCT {
	ULONG			Signature;
	DWORD			ProcessId;
	//MR_RCV_QUEUE	RcvQ;
	HANDLE			hMMF;
	MR_HANDLE		hOpen;
	HANDLE			hSemaphore;
	WCHAR			SemaphoreName[MMF_size];
	HANDLE			hMutex;
	WCHAR			MutexName[MMF_size];
	DWORD			PortCount;		// number of Ports using this queue
	DWORD			qDepth;
	ULONG			head;			// messages are de-queued from start of list
	ULONG			tail;			// messages are en-queued at end of linked list 
} Q_STRUCT, *PMR_Q_STRUCT;

// Message Routing Control Block: note that there can be multiple MRCBs for the same RcvQ.
// This structure is visible across processes and thus must store the names of the MMFs.
// Some of these fields are copied from the Q_STRUCT.
typedef struct _MESSAGE_ROUTING_CONTROL_BLOCK {
	ULONG				Signature;
	HANDLE				ProcessId;
	HANDLE				OpenHandle;
	UINT_8				BoardId;
	UINT_8				PortId;
	UINT_8				RouteId;
	MR_HANDLE			hMRCB;
	HANDLE				hMRCBMMF;					// Handle from CreateFile
	WCHAR	 			SemaphoreName[MMF_size];	// Semaphore Memory Mapped File Name
	MR_HANDLE			pUserSemaphore;				// User's Pointer to Semaphore
	MR_HANDLE			pMriSemaphore;
	WCHAR				MutexName[MMF_size];
	MR_HANDLE			pUserMutex;
	HANDLE				pMriMutex;
	WCHAR				RcvQSName[MMF_size];
	PMR_Q_STRUCT		pRcvQStruct;
	MR_HANDLE			pMriRcvQ;
	BOOLEAN				Attached;
	MR_HANDLE			NextMRCB;
	MR_HANDLE			PreviousMRCB;
} MR_MRCB,	*PMR_MRCB;

// This structure is used to verify that a structure has a valid signature
typedef	struct _SIGNATURE_VERIFICATION {
	ULONG						Signature;
} SIGNATURE_VERIFICATION, *PSIG_VERIFY;

// Routing Class Info about Port Vector
typedef struct _MR_ROUTE_INFO {
	BOOLEAN			bPortInUse;
	HANDLE			pUserPortVector;
	HANDLE			pMriPortVector;
	WCHAR			PortVectorMMF[MMF_size];  // Name of Memory Mapped File
} MR_ROUTE_INFO, *PMR_ROUTE_INFO;

//The Routing Structure contains a pointer to Route info for each registered Routing Class
typedef struct _MR_ROUTING_STRUCT {
	ULONG			Signature;
	HANDLE			hMMF;					// Handle from CreateFileMapping
	ULONG			AttachedPortCount;
	UINT_8			SendingMDP[MAXHOSTS];	// Indicates VNMS Class on MDPA or MDPB
	MR_ROUTE_INFO	RouteInfo[MR_MAX_ROUTING_CLASSES];
} MR_ROUTING_STRUCT, *PMR_ROUTING_STRUCT;

typedef struct _MR_OPEN_HANDLE {
	ULONG						Signature;
	PMR_ROUTING_STRUCT			hRoutingVector;		// handle of MMMF for Routing 
	HANDLE						hRDSMutex;			// handle for the RDS Mutex
	HANDLE						hGlobMutex;			// handle for Globals Lock
	HANDLE						hFirstMRCB;			// First MRCB for this receiver
	HANDLE						ProcessId;			// Id of the Process that issued Open
	HANDLE						hPortList;			// Array tracks all attached ports
	HANDLE						hEQMMF;				// handle of MMF for Event Queue
	HANDLE						hUserEQ;			// handle of MMF mapped view for Event Queue
	WCHAR						EQName[MMF_size];	// name of MMF for Event Queue
} MR_OPEN_HANDLE, *PMR_OPEN_HANDLE;   



//MsgType of MR_MESSAGE
typedef enum  {
	MR_NONCONTIGUOUS,	
	MR_CONTIGUOUS,		
	MR_ATTACH_NOTIFICATION,
	MR_DETACH_NOTIFICATION,
	MR_ATTACHED_PORTS_NOTIFICATION,
	MR_BROADCAST,
	MR_GREETINGS_REQUEST,
	MR_GREETINGS_RESPONSE,
	MR_INITIALIZATION_COMPLETE,
	MR_BOARD_FAILURE,
	MR_SEND_BOARD_UPDATED,
	MAXMSGTYPE
} MR_MSGTYPE;


// The message structure is also defined at the driver level.
// If it changes here it must also change in appmsghdr.h.

typedef struct
{
	UINT_32				MsgLink;
	UINT_32				MsgType;
	UINT_32				DataLen;
	void				*lpvData;
	ULONG				Pid;
	UINT_8				DestSlotId;
	UINT_8				SrcSlotId;
	UINT_8				OpCode;
	UINT_8				OpClass;
	UINT_8				Vnmsid;
	UINT_8				Rclass;
	UINT_16				SendElapsed;
	UINT_16				SendResult;
    UINT_8              Flags;
    UINT_8				reserved;
	MR_HANDLE			hEqueue;
	void				(*lpvCallback)(PBYTE msg, ULONG status);
   	UINT_8		        brData[BR_DATA_SZ_MRI];
	ULONG				Signature;     
	//	UINT_8				OpData[0];

}  MR_MESSAGE, * PMR_MESSAGE;	// Length of 56 bytes

// Flag definitions
//      MRI_TRACE_FLAG : True forces mri tracing
#define MRI_TRACE_FLAG 0x01


typedef struct _MR_SLOT_INFO
{
   UINT_8	SlotId;
   bool		MDP;
   WCHAR	BoardId[12];
   bool		SwitchOverComplete;

} MR_SLOT_INFO;

typedef struct _MR_EVENT
{
	MR_EVENT_ID		EventId;
	MR_MESSAGE     *lpMessage;

} MR_EVENT;


typedef struct _MR_INFO
{
	unsigned int	Class;
	unsigned int	BoardId;
} MR_INFO;

typedef struct _MR_ROUTEINFO
{
		MR_INFO		Routes[MAXROUTES];
} MR_ROUTEINFO;

typedef struct _MR_PORTINFO
{
		MR_INFO		Ports[MAXPORTS];
} MR_PORTINFO;

//The CMM Global Structure
typedef struct _MR_CMM_GLOBALS {
	ULONG			Signature;				// GLOB
	HANDLE			hMMF;					// Handle from CreateFileMapping
	unsigned int	cmmID;
	unsigned int	masterHost;				// Master (or initializing) host ID
	unsigned int	VNMSpath;				// Represents the I/O path for sending data,
											// contains the slot ID of the read unit
	VNMSPATHS		VNMSpaths[MAXHOSTS];	// Represents the I/O paths for sending data,
											// contains the slot ID of the read units
    bool            is_HMSPresent;          // HMS Present flag.
    bool            is_HMSInstalled;        // HMS installed Flag
    bool            is_SCA;                 // Indicates that this is the SCA board
    bool            is_CMM_NonShared;       // CMM NonShared flag
    unsigned int    CMM_NonShared_Host;     // Host that put the CMM in nonshared mode. 
	unsigned int	MDPS;
	unsigned int	SCS;
	unsigned int	ActiveBoardCount;		// The number of Active MDPs and SC in CMM
	unsigned int	failedBoards;
	bool			MdpSwitchOver;			// Indicates MDP Send SwitchOver Occurring
	unsigned int	SendingMDP[MAXHOSTS];	// Slot id of MDP sending to host
	unsigned int	RequestingMDP;			// Slot id of MDP that is requesting switch
	unsigned int	HostNumber;
	MR_SLOT_INFO	ActiveBoards [MR_MAX_BOARDS]; // Used to track MDP switchover

} MR_GLOBALS, *PMR_GLOBALS;

		
// This structure is created each time an Open is done. It's purpose is to keep a list of 
// all attached port handles that a process can access without calling MapViewOfFile, 
// and thus avoid the Page Fault incurred each time this is done.
typedef struct _MR_ATTACHED_PORT_INFO {

	ULONG						Signature;
	HANDLE						hMRCB;
	HANDLE						hRcvQ;
	HANDLE						hMRCBMMF;
	HANDLE						hRcvQMMF;

} MR_ATTACHED_PORT_INFO, PMR_ATTACHED_PORT_INFO;


typedef struct _MR_PORT_LIST {
	ULONG						Signature;
	MR_ATTACHED_PORT_INFO		PortEntry[MAXRCLASS] [CL_INT_OPCLASS_TOP];
} MR_PORT_LIST, *PMR_PORT_LIST;


class MriSharedInternals
{

public:
	MriSharedInternals();
	MR_STATUS	ValidateMD (MR_MESSAGE *pMsg);
	MR_STATUS	ValidateMD (MR_MESSAGE *pMsg, bool sentMsg);
	MR_STATUS	FindMRCB (BYTE hostId, BYTE opClass, BYTE rCclass, PMR_MRCB *pMRCB, HANDLE *hMMF);
	MR_STATUS	DispatchMsg (MR_MESSAGE *pMsg, PMR_MRCB pMRCB, HANDLE hMMF);
    bool        ProcessHMS (MR_MESSAGE *pMsg);
	void		GetRcvQ ( PMR_MRCB pMRCB, HANDLE *pRcvQ );

	static void		ReleaseLock (HANDLE hLock);
	static bool		GetLock (HANDLE hLock, DWORD timeout);
	static void		VerifySignatureWithThrow (PSIG_VERIFY StructPtr, 
												ULONG signature, 
												USHORT ErrorLine);
	static void		InitGlobals(PMR_GLOBALS pGlobal);
	MR_STATUS		SwitchSendBoard (MR_MESSAGE * pMsg);

	PMR_GLOBALS		m_pMRIGlobal;	// initialized on Open()
    void            SetHMSInstalled( bool isInstalled ) { m_pMRIGlobal->is_HMSInstalled = isInstalled; }
    bool            HMSIsInstalled(){ return m_pMRIGlobal->is_HMSInstalled; }
	BOOLEAN			IsActiveSC ();
    BOOLEAN         IsSCA(){ return m_pMRIGlobal->is_SCA; }

protected:
	MR_OPEN_HANDLE	*m_hOpen;
	HANDLE		m_hRoutingVector;	// initialized on Open()
	PBYTE		m_pBufBase;			// initialized on Open()
	UINT_8		m_SrcSlotId;		// initialized on Open()
	bool		m_BufType;
	HANDLE		m_hGlobalMMF;		// initialized on Open()
	HANDLE		m_hMRIGlobal;		// initialized on Open()
	SECURITY_DESCRIPTOR m_sd;
	SECURITY_ATTRIBUTES m_sa;



};


//.interface:Message_Routing_Interface
/*********************************************************************************

 This class presents an interface for routing of messages to CMM modules both
 on and off board.

 *********************************************************************************/

class Message_Routing_Interface : public MriSharedInternals
{

public:
	Message_Routing_Interface();

	MR_STATUS				Open (MR_HANDLE *hOpenHandle);
	MR_STATUS				Close (MR_HANDLE Handle);
	MR_STATUS				AttachPort (MR_HANDLE OpenHandle,
										UINT_8 portid, 
										UINT_8 routeid,
										MR_HANDLE *PortHandle);
	MR_STATUS				DetachPort (MR_HANDLE hPort);
	MR_STATUS				CreateEqueue (MR_HANDLE hOpenHandle,
											MR_HANDLE *hEQueue);
	MR_STATUS				DestroyEqueue (MR_HANDLE hEQueue);
	MR_STATUS				WaitEqueue (MR_HANDLE hEQueue,
									  	MR_EVENT  *pEvent,
										DWORD timeout);
	MR_STATUS				ReceiveMRIMsg (MR_HANDLE hPort, 
											MR_MESSAGE **pMessage,
											DWORD timeout);
	MR_STATUS				SendMRIMsg (MR_MESSAGE *message,
											bool bBufType);
    MR_STATUS				RouteMRIMsg (MR_MESSAGE *message,
											bool bBufType);
	MR_STATUS				FreeBuf (MR_MESSAGE *message);
	MR_STATUS				FreeBuf (MR_MESSAGE *buffer, bool NOTMRIFMT);
	MR_STATUS				AllocateBuf (UINT_32 bytesRequired, 
											MR_MESSAGE **pMessage);
	MR_STATUS				GetMySlot (UINT_8 *slotinfo);
	MR_STATUS				GetSlotInfo (MR_SLOT_INFO *slotinfo);
	MR_STATUS				GetMDPBoardMask (ULONG *MDPBoardMask);
	MR_STATUS				GetSCBoardMask (ULONG *SCBoardMask);
	MR_STATUS				GetBoardMask (ULONG *BoardMask);
	UINT_8					GetHostRclass ();
	UINT_8					GetHostRclass (UINT_8 slotid);	
	MR_STATUS				GetPortInfo (UINT_8 portId, MR_PORTINFO *ports);
	MR_STATUS				GetRouteInfo (UINT_8 routeId, MR_ROUTEINFO *routes);
	MR_STATUS				SendInterBoardNotification (UINT_8 PortId,
											UINT_8 RoutingId,
											UINT_8 MsgType,
											UINT_8 DestSlot);
	MR_STATUS				SendInterBoardNotification (UINT_8	MsgType,
											UINT_8	DestSlot,
											UINT_32	DataLen,
											HANDLE	DataPtr);
	MR_STATUS				SignalEQueue (PMR_Q_STRUCT	pEventQ, MR_MESSAGE *pMsg);
	MR_STATUS				ProcessSendBoardUpdated (UINT_8	BoardId); 
	MR_STATUS				BuildSetSendReply	(MR_MESSAGE	**pMsg, PMR_GLOBALS	pGlob);

    UINT_8                  SetMRITrace(MR_MESSAGE *message)
                                    { return ( message->Flags |= MRI_TRACE_FLAG ); }
    UINT_8                  ClearMRITrace(MR_MESSAGE *message)
                                    { return ( message->Flags &= (~ MRI_TRACE_FLAG ) ); }
    BOOLEAN                 IsMRITrace(MR_MESSAGE *message)
                                    { return ( (BOOLEAN)( message->Flags & MRI_TRACE_FLAG ) ); }
	MR_STATUS				DumpHdr (MR_MESSAGE *pMsg);

	static void (MRI_SetNAPLog) ( NLogBase* n1);

private: // data
#ifdef _D_DESKTOP
#else
//IBCM Removal
//	IBCM_CommManager m_Comm;
#endif
    SECURITY_DESCRIPTOR m_sd;
	SECURITY_ATTRIBUTES m_sa;
	
private: // methods
	void			ProcessInterBoardMsg (MR_MESSAGE *pMsg);
//IBCM Removal
/*	void			PostProcessMsg (const IBCM_Target& target, 
									IBCM_BufferDescriptors& buffers,
									ULONG status);
*/
	MR_STATUS		ReturnSendStatus (MR_MESSAGE *pMsg, ULONG status);
	MR_STATUS		CopyMsg (MR_MESSAGE *pMsg, MR_MESSAGE **pDupMsg);
	void			ProcessIntraBoardMsg (MR_MESSAGE *pMsg, PMR_MRCB pMRCB, HANDLE hMMF);
	MR_STATUS		GetMessageFromReceiveQueue 
						(PMR_Q_STRUCT pRcvQStruct, PMR_MESSAGE * RcvMessage);
	MR_STATUS		ProcessMRIMsg(MR_MESSAGE *pMsg);
	unsigned int	GetVNMSRouting (unsigned int hostid);
	MR_STATUS		UpdateActiveBoardStructure 	(MR_MESSAGE *pMessage);

};

struct Mri_Status  {
	MR_STATUS	code;
	Mri_Status(MR_STATUS statuscode) { code = statuscode; }
};

	MR_STATUS MRI_Init();
//IBCM Removal
/*	void AsyncCompletion (const IBCM_Target& target,
                      IBCM_BufferDescriptors& bufferList,
                      ULONG ibcmStatus,
                      void* pContext );
*/


// To create a DLL for MRS Support, a C Style header is created for MRI since DLLs do
// not support the capability of doing a "new" on an object. With the DLL, the app calls
// the CreateMriInterface and then is able to use the API. 


// MRI_C_STYLE_HEADER.h

typedef void *MR_API_HANDLE;

MR_API_HANDLE CreateMRIInterface();

MR_STATUS		MRI_Interface_Open ( MR_API_HANDLE hMRIApi, 
							MR_HANDLE *hOpenHandle );
MR_STATUS		MRI_Interface_Close( MR_API_HANDLE hMRIApi, 
							MR_HANDLE Handle );
MR_STATUS		MRI_Interface_AttachPort( MR_API_HANDLE hMRIApi, 
							MR_HANDLE OpenHandle,
							UINT_8 portid, 
							UINT_8 routeid,
							MR_HANDLE *PortHandle );
MR_STATUS		MRI_Interface_DetachPort( MR_API_HANDLE hMRIApi, 
							MR_HANDLE hPort );
MR_STATUS		MRI_Interface_CreateEqueue( MR_API_HANDLE hMRIApi, 
								MR_HANDLE hOpenHandle,
								MR_HANDLE *hEQueue );
MR_STATUS		MRI_Interface_DestroyEqueue( MR_API_HANDLE hMRIApi, 
							MR_HANDLE hEQueue );
MR_STATUS		MRI_Interface_WaitEqueue(	MR_API_HANDLE hMRIApi, 
							MR_HANDLE hEQueue,
						  	MR_EVENT  *pEvent,
							DWORD timeout );
MR_STATUS		MRI_Interface_ReceiveMRIMsg(	MR_API_HANDLE hMRIApi, 
								MR_HANDLE hPort, 
								MR_MESSAGE **pMessage,
								DWORD timeout );
MR_STATUS		MRI_Interface_SendMRIMsg( MR_API_HANDLE hMRIApi, 
								MR_MESSAGE *message,
								BOOLEAN bBufType );
MR_STATUS		MRI_Interface_FreeBuf( MR_API_HANDLE hMRIApi, 
								MR_MESSAGE *message );
MR_STATUS		MRI_Interface_FreeBufOptions( MR_API_HANDLE hMRIApi, 
								MR_MESSAGE *buffer, bool NOTMRIFMT );
MR_STATUS		MRI_Interface_AllocateBuf( MR_API_HANDLE hMRIApi, 
								UINT_32 bytesRequired, 
								MR_MESSAGE **pMessage );
MR_STATUS		MRI_Interface_GetMySlot( MR_API_HANDLE hMRIApi, 
								UINT_8 *slotinfo );
MR_STATUS		MRI_Interface_GetSlotInfo( MR_API_HANDLE hMRIApi, 
								MR_SLOT_INFO *slotinfo );
MR_STATUS		MRI_Interface_GetMDPBoardMask( MR_API_HANDLE hMRIApi, 
								ULONG *MDPBoardMask);
UINT_8			MRI_Interface_GetHostRclass(MR_API_HANDLE hMRIApi);
UINT_8			MRI_Interface_GetHostRclassBySlot (MR_API_HANDLE hMRIApi, UINT_8 slotid);	
MR_STATUS		MRI_Interface_GetPortInfo( MR_API_HANDLE hMRIApi, 
								UINT_8 opClass, MR_PORTINFO *ports );
MR_STATUS		MRI_Interface_GetRouteInfo( MR_API_HANDLE hMRIApi, 
								UINT_8 rClass, MR_ROUTEINFO *routes );
MR_STATUS		MRI_Interface_SendInterBoardNotification( MR_API_HANDLE hMRIApi,
								UINT_8 PortId,
								UINT_8 RoutingId,
								UINT_8 MsgType,
								UINT_8 DestSlot);
MR_STATUS		MRI_Interface_SendInterBoardNotificationWithData (MR_API_HANDLE hMRIApi,
								UINT_8	MsgType,
								UINT_8	DestSlot,
								UINT_32	DataLen,
								HANDLE	DataPtr);
MR_STATUS		MRI_Interface_SignalEQueue( MR_API_HANDLE hMRIApi,
								PMR_Q_STRUCT pEventQ, MR_MESSAGE *pMsg);
MR_STATUS		MRI_Interface_ProcessSendBoardUpdated( MR_API_HANDLE hMRIApi,
								UINT_8	BoardId); 
MR_STATUS		MRI_Interface_BuildSetSendReply( MR_API_HANDLE hMRIApi,
								MR_MESSAGE	**pMsg, PMR_GLOBALS	pGlob);


int             DestroyMRIInterface(MR_API_HANDLE hMRAPI);

