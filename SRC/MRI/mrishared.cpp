//#define _WIN32_WINNT 0x0500 // Windows NT 5 == Windows 2000
#define _WIN32_WINNT 0x0600   // Windows 2008
#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <process.h>
#include <atlbase.h>

#include <BuffMgr/BufferMgr.h>
#include <mri/mriapi.h>
#include <mri/mrishared.h>
// #include <ibcm/ibcm.h>
#include <MRI/mriops.h>
#include <mri/Queue.h>
#include <GLOB/Globals.h>
// #include <TRB/logtrace.h>
#include <NL/NetworkLookup.h>
#include <EM/nestedException.h>
#include <EM/ExcTF.h>

// Logging variables
extern	BOOLEAN			MriLogging;
extern	LOG_TRACE_DATA	MriInfo;
// extern	AS_Trace		MriTraceObj;
extern	CHAR 			*MriTraceString;
extern TR_Module MriModule;
extern TR_Level MriVerbose;
extern TR_Level MriDataPath;
extern TR_Level MriInform;
extern TR_Level MriWarning;
extern TR_Level MriSevere;
#define	AS_TRACEX		MriTraceObj.AS_TRACE

extern LOG_TRACE_DATA *AS_FillLogBuffer (LOG_TRACE_DATA *lpLogBuf, DWORD dwCount, ...);


/*
This variable is set to false after logging a warning
for missing Signaling Server attribute in CMMParam.cfg.
*/
static bool bSingleLogging = true;

// Declare MRI's own log object
NLogBase * MriLogObj;

THR_Semaphore g_MriCtlSemaphore;

//IBCM
/*extern void AsyncCompletion (const IBCM_Target& target,
                      IBCM_BufferDescriptors& bufferList,
                      ULONG ibcmStatus,
                      void* pContext );
*/

//.public:MriSharedInternals
/*================================================================================

  MriSharedInternals - these function are shared between ReceiveMonitor and the 
						Mri Lib.

  Purpose:
    Constructor.

================================================================================*/
MriSharedInternals::MriSharedInternals()
{
	m_hOpen = NULL;
	m_hRoutingVector = NULL;
	m_pBufBase = NULL;
	m_SrcSlotId = NULL;
	m_BufType = NULL;


	// Handle security issues		
	// This provides a NULL DACL which will allow access to everyone.
	if (!InitializeSecurityDescriptor( &m_sd, SECURITY_DESCRIPTOR_REVISION ))
    {
		MRI_BIN_TRACE (MriWarning, MR_STATUS_COULD_NOT_CREATE_MMF);
		throw MR_STATUS_COULD_NOT_CREATE_MMF;
    }

    if (!SetSecurityDescriptorDacl( &m_sd, TRUE, NULL, FALSE ))
    {
		MRI_BIN_TRACE (MriWarning, MR_STATUS_COULD_NOT_CREATE_MMF);
		throw MR_STATUS_COULD_NOT_CREATE_MMF;
    }

    m_sa.nLength = sizeof m_sa;
    m_sa.lpSecurityDescriptor = &m_sd;
    m_sa.bInheritHandle = FALSE;
}




/******************************************************************************************
*
*	Name:
*		GetLock
*
*	Parameters:
*		LockHandle
*		TimeOut - Time to wait for lock
*
*	Description:
*		This function only handles Mutexes - not Semaphores
*
******************************************************************************************/

bool	MriSharedInternals::GetLock (
					 HANDLE		hLock,
					 DWORD		timeout) {


	DWORD				dWaitResult;


    try
    {
        dWaitResult = WaitForSingleObject (hLock, MR_TIME_TO_WAIT);
        switch (dWaitResult) {
            
        case WAIT_OBJECT_0:
            return TRUE;
            
            // If the Lock failed for any reason then immediately exit
        case WAIT_TIMEOUT:
            MRI_BIN_TRACE (MriWarning, GetLastError());
            return FALSE;
            
        case WAIT_ABANDONED:
            MRI_BIN_TRACE (MriWarning, GetLastError());
            return FALSE;
            
        default:
            MRI_BIN_TRACE2 (MriSevere, 50523, GetLastError(), dWaitResult, 
                "MRI: GetLock Failed due to Status=%x, WaitResult=%x", 0);
            return FALSE;
            
        }
    }
    EM_ELLIPSES_CATCH
    (
        MRI_BIN_TRACE1( MriSevere, 0, GetLastError(),
            "MriSharedInternals::GetLock() Failed with unexpected exception", 0 );
        return FALSE;
    )

}  // GetLock


/******************************************************************************************
*		
*	Name:
*		ReleaseLock
*
*	Parameters:
*		LockHandle
*
*	Description:
*		This function handles Mutexes - not semaphores
******************************************************************************************/

void	MriSharedInternals::ReleaseLock (
					 HANDLE		hLock) {


	if (!ReleaseMutex (hLock)) {
		MRI_BIN_TRACE (MriWarning, GetLastError());
		}
}

#pragma optimize( "", off )

/*****************************************************************************************
*
*	Name:	
*		VerifySignatureWithThrow
*
*	Description:
*		This inline function evaluates an input structure to determine if it has a valid 
*		signature which has been specified by the caller. If the signature is valid
*		this function exits without incident. If the signature is invalid, an error is 
*		logged and the calling function's exception handler is invoked.
*
*		Note that this function presumes that the signature is at the beginning of 
*		the input structure.
*
******************************************************************************************/


inline void 

MriSharedInternals::VerifySignatureWithThrow 
						(PSIG_VERIFY StructPtr, ULONG signature, USHORT ErrorLine) {
	
	STATUS		Status;  

	try {
		if ( (StructPtr)->Signature != (signature) ) 		
		  { /* failure */ 
			Status.errorcode = MR_STATUS_INVALID_SIGNATURE;  
			MRI_BIN_TRACE (MriWarning, Status.errorcode);
			MRI_BIN_TRACE (MriWarning, ErrorLine);
			// TBD : this throw always invokes the following catch - needs to be changed
			throw Status; 
		  } /* failure */
	}

    EM_ELLIPSES_CATCH
	(
		Status.errorcode = MR_STATUS_INVALID_DATAPTR;  
		MRI_BIN_TRACE (MriWarning, Status.errorcode);
		MRI_BIN_TRACE (MriWarning, ErrorLine);
		throw Status;
	)

}  // VerifySignatureWithThrow



#pragma optimize( "", on )


/**************************************************************************************
*
*	Name:
*		SwitchSendBoard
*
*	Description:
*		This function handles the OP_SET_SEND_BOARD message. This op is broadcast
*		from IO Services to cause a switchover of host-bound traffic from one MDP to 
*		the other. MRI/RM processes this switchover by changing the SendingMDP field in 
*		the Routing Tbl. This change is then transparent to all users since they send 
*		based on the VNMS class and MRI maps it to the SendingMDP field.
*		
*
***************************************************************************************/
MR_STATUS	MriSharedInternals::SwitchSendBoard	(MR_MESSAGE		*pMsg)
{

	PMR_OPEN_HANDLE		hOpen;
	PMR_ROUTING_STRUCT	hRoutingTable;
	HANDLE				hLock;
	UINT_8				NewSender;
    UINT_8              hostId;
	STATUS				Status;

	Status.errorcode = MR_STATUS_SUCCESS;

	try 
	{

	
		mrisetsendbdop *SetSendBoardMsg;
		SetSendBoardMsg = (mrisetsendbdop *)pMsg->lpvData;
		NewSender = SetSendBoardMsg->sendiomgr;
        hostId = SetSendBoardMsg->hostnumber;

		MRI_BIN_TRACE3(MriInform, 50832, hostId, NewSender, pMsg->SrcSlotId, 
					  "SwitchSendBoard Message Received, HostId=%d, NewSender=%x, SourceSlotId=%x", 0);
		
		// update the Routing Table to indicate the new route Id.
		if (NewSender == IOMA)
		{
			// Switch over to MDPA for VNMS sends
			NewSender = VNMSA;
		}
		else
		{
			if (NewSender == IOMB)
			{
				// Switchover to MDPB for VNMS sends
				NewSender = VNMSB;
			}
			else
			{
				// Bad enumeration inthe request
				MRI_BIN_TRACE3 (MriSevere, 50541, NewSender, pMsg->SrcSlotId, hostId,
							"OP_SET_SEND_BOARD has bad Send IO Mgr of %x from Slot %x host %d", 0);
				return MR_STATUS_INVALID_REQUEST;
			}
		}

		hOpen = (PMR_OPEN_HANDLE)m_hOpen;

		// Ensure validity of Open handle and RDS pointer 
		VerifySignatureWithThrow ((PSIG_VERIFY)hOpen, OPEN_SIGNATURE, (USHORT)__LINE__);
		
		// Get the Routing Table
		hRoutingTable = hOpen->hRoutingVector;
		VerifySignatureWithThrow ((PSIG_VERIFY)hRoutingTable, RDS_SIGNATURE, (USHORT)__LINE__);

		// Lock the Routing Table
		if (!GetLock (hOpen->hRDSMutex, MR_TIME_TO_WAIT)) 
		{
			MRI_BIN_TRACE2(MriSevere, 50835, hOpen->hRDSMutex, GetLastError(), 
				 "ScHandleHotInsertion Failed To Get Lock," 
				  "Lock=%p, Status=%x", 11300); 
			return MR_STATUS_MUTEX_FAILURE;
		}
				
		// Save the Lock handle in case of throw
		hLock = hOpen->hRDSMutex;

   		PMR_ROUTING_STRUCT	pRoutingVector = (PMR_ROUTING_STRUCT)hRoutingTable;

		MRI_BIN_TRACE4 (MriWarning, 50541, 
//			, pMsg->SrcSlotId, NewSender, pRoutingVector->SendingMDP[hostId], hostId,
// CKK 06/15/2010.  Removed ',' from start of above line.  Was causing C4002 warning, too many parameters, and syntax errors
			pMsg->SrcSlotId, NewSender, pRoutingVector->SendingMDP[hostId], hostId,
			"IOMgr Switchover occuring from Board %x, NewSender=%x, CurrentSender=%x, HostId=%d", 0);

		pRoutingVector->SendingMDP[hostId] = NewSender;  // VNMSA or VNMSB

		// Unlock the Routing Table
		ReleaseLock (hLock);
		hLock = NULL;


	} // try

	EM_ELLIPSES_CATCH	
	(
		MRI_BIN_TRACE2 (MriSevere, 50541, pMsg->SrcSlotId, NewSender,
			"ProcessSwitchSendBoard Failed, RequestBoard=%x, SendBoard=%x", 0);
		return MR_STATUS_UNEXPECTED_EXCEPTION;
	)

	MRI_BIN_TRACE2 (MriVerbose, 50541, pMsg->SrcSlotId, NewSender,
		"ProcessSwitchSendBoard Completed, RequestBoard=%x, SendBoard=%x", 0);


	return Status.errorcode;
}
		

/***************************************************************************************
*
*	Name:	
*		GetRcvQ
*
*	Description:
*		This functions returns a pointer to the Receive Q for a particular MRCB. 
*		If the Receive Queue has not already been stored in the Attached_Ports array
*		then a view to the file must be mapped and the pointer saved.
*
***************************************************************************************/

void MriSharedInternals::GetRcvQ ( PMR_MRCB pMRCB, HANDLE *pRcvQ )
{
	STATUS			Status;
	UINT_8			routeId =0, portId = 0;
	PMR_PORT_LIST	pPortList;
	HANDLE			hRcvQ = NULL;
	HANDLE			hRcvQMMF = NULL;
	
	Status.errorcode = MR_STATUS_SUCCESS;

	if (!m_hOpen)
	{
		Status.errorcode = MR_STATUS_INVALID_OPEN_HANDLE;
		throw Status;
	}
		
	routeId = pMRCB->RouteId;
	portId  = pMRCB->PortId;
	pPortList = (PMR_PORT_LIST)m_hOpen->hPortList;
	VerifySignatureWithThrow ((PSIG_VERIFY)pPortList, PLIST_SIGNATURE, (USHORT)__LINE__); 
	hRcvQ = pPortList->PortEntry [routeId][portId].hRcvQ;
	if (hRcvQ)
	{
		VerifySignatureWithThrow ((PSIG_VERIFY)hRcvQ, RCVQ_SIGNATURE, (USHORT)__LINE__);
		*pRcvQ = hRcvQ;
		return;
	}


	// The handle to the receive queue was not stored so must retrieve it via the
	// Memory Mapped file name in the MRCB.

	// map a view of the message recipient's receive queue
	hRcvQMMF = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, pMRCB->RcvQSName);
	if  (hRcvQMMF == NULL)  
	{
		// No Receive Q has been found - this generally indicates that an MRI Send was
		// done specifying the wrong board id.
		Status.errorcode = MR_STATUS_INVALID_BOARD_ID;
		throw Status;
	}
	hRcvQ = MapViewOfFile(hRcvQMMF, FILE_MAP_ALL_ACCESS, NULL, NULL, NULL);
	VerifySignatureWithThrow ((PSIG_VERIFY)hRcvQ, RCVQ_SIGNATURE, (USHORT)__LINE__);

	// Save the RcvQ handle and the handle to its Memory Mapped File - the hRcvQ
	// will be used for future Dispatches andthe hRcvQMMF must be closed when the 
	// Port is detached.
	pPortList->PortEntry [routeId][portId].hRcvQ = hRcvQ;
	pPortList->PortEntry [routeId][portId].hRcvQMMF = hRcvQMMF;


	*pRcvQ = hRcvQ;


}




/*
*****************************************************************************************
*
*	Name:	
*		DispatchMSG
*
*	Description:
*		This is is worker function that posts a message on a users receive queue. It 
*		is responsible for un-realizing a pointer to an offset, mapping a view of the
*		message recipient's receive queue, locking the receive queue, en-queuing a 
*		message to the receive queue, unlocking the receive queue and signaling the 
*		waiting thread.
*
*		The name of the file mapping object for the receive queue, the handle for
*		the mutex and the handle for the semaphore are contained in the MRCB.
*
*	Parameters:
*		pMsg:	Pointer to the message (from SendMessage())
*
*		pMRCB:	Pointer to the Message Routing Control Block corresponding to the 
*				message recipient
*
*	Return Values:
*		MR_STATUS_SUCCESS
*		MR_STATUS_FAILURE
*
*****************************************************************************************
*/
MR_STATUS MriSharedInternals::DispatchMsg( MR_MESSAGE *pMsg, PMR_MRCB pMRCB, HANDLE hMMF )
{
	int				retval;
	unsigned int	MsgOffset;
	bool			unrealizeptr = false;
	PBYTE			pBufBase = 0;
	DWORD			dwWaitResult, qDepth;
	HANDLE			hMRCB = (HANDLE)pMRCB;
	HANDLE			hMutex = NULL, hSemaphore = NULL;
	PMR_Q_STRUCT	pRcvQ;
	STATUS			Status;
	UINT_8			RouteId =0, PortId = 0;

	Status.errorcode = MR_STATUS_SUCCESS;

	retval = MR_STATUS_SUCCESS;
	
	try  {
		// validate the MRCB pointer
		if (!pMRCB)  
		{
			Status.errorcode = MR_STATUS_INVALID_MRCB;
			throw Status;
		}

		RouteId = pMRCB->RouteId;
		PortId  = pMRCB->PortId;

		// does the MRI object have a base pointer?
		if (!m_pBufBase)  
		{
			if (GetBufferBase (&pBufBase))  {
				// trace error
				Status.errorcode = MR_STATUS_NO_RESOURCES;
				throw Status;
			}
			if ( pBufBase == 0 )  {
				// trace error
				Status.errorcode = MR_STATUS_NO_RESOURCES;
				throw Status;
			}
			m_pBufBase = pBufBase;
		}

		// save the operation offset in the Message Descriptor so it 
		// can be realized as a pointer on the receiving side
		pMsg->lpvData = (void *)((PBYTE)pMsg->lpvData - (PBYTE)m_pBufBase);
		unrealizeptr = true;

		//un-realize the message pointer
		MsgOffset = (PBYTE)pMsg - (PBYTE)m_pBufBase;

		// Get the pointer to the Receive Queue for this MRCB
		HANDLE		hRcvQ;
		GetRcvQ (pMRCB, &hRcvQ); 
		pRcvQ = (PMR_Q_STRUCT)hRcvQ;

		// Secure atomic access to receive queue
		hMutex = OpenMutex(MUTEX_ALL_ACCESS, NULL, pRcvQ->MutexName);
		dwWaitResult = WaitForSingleObject( hMutex, MR_TIMEOUT_FOREVER );
		switch (dwWaitResult) 
		{

			case WAIT_OBJECT_0:
			{
				//en-queue the message to the receive queue
				Queue	RcvQ(pRcvQ, m_pBufBase);		// instantiate the Receive Queue object;
				qDepth = RcvQ.Size();
				if (qDepth == (MAX_QUEUE_DEPTH/2))  
				{
					// If the queue is half full then most likely it is 
					// stuck. Log an error that it is going to overflow. 
					MRI_BIN_TRACE3 (MriInform, 50532, RouteId, PortId, pRcvQ,
						"Send Q is half full and will overflow, Route=%x, Port=%x, pRcvQ=%x", 0);
				}
				else
				{
                // CD-1004920 - jpd - 10/13/14 - change depth test. See full description in 
                //   MriSharedInternals::DispatchMsg
 					if (qDepth >= MAX_QUEUE_DEPTH)  
					{
						// If this message exceeds the receive queue depth, 
						// log error and reject send request.
						Status.errorcode = MR_STATUS_QUEUE_OVERFLOW;
						throw Status;
					}
				}

				// en-queue message on Receive queue
				retval = RcvQ.EnQ( MsgOffset );
				if ( retval != MR_STATUS_SUCCESS )  {
					// log error and reject request
					Status.errorcode = (MR_STATUS)retval;
					throw Status;
				}

				// unlock the Receive queue
				if (!ReleaseMutex(hMutex))  {
					MRI_BIN_TRACE (MriWarning, GetLastError());
				}

				hSemaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, NULL, pRcvQ->SemaphoreName);
				if (!hSemaphore) 
				{
					Status.errorcode = (MR_STATUS) GetLastError();
					throw Status;
				} 
				else 
				{
					if (!ReleaseSemaphore (hSemaphore, 1, 0))  
					{
						MRI_BIN_TRACE (MriWarning, GetLastError());
					}
				}
				break;
			}

			case WAIT_TIMEOUT:
			case WAIT_ABANDONED:
			case WAIT_FAILED:
			default:
				MRI_BIN_TRACE (MriWarning, dwWaitResult);
				Status.errorcode = MR_STATUS_WAIT_FAILURE;
				throw Status;
				break;
		}

	}	// end try


	catch (STATUS &Status) {
		MRI_BIN_TRACE1 (MriSevere, 50530, Status.errorcode, "MRI Dispatch1 Error=%x", 0);
		MRI_BIN_TRACE4 (MriSevere, 50531, pMsg, pMRCB, RouteId, 
		                PortId,"MRI: Unable To Place Msg in Q, MsgPtr=%x, PortHandle=%x,Route=%x,Port=%x", 0);
		// Return LPVData to its original setting since the dispatch failed
		if (unrealizeptr)
			pMsg->lpvData = (void *)((PBYTE)m_pBufBase + (DWORD)pMsg->lpvData);
		// unlock the Receive queue
		if (hMutex)  {
			if (!ReleaseMutex(hMutex))  {
				MRI_BIN_TRACE (MriWarning, GetLastError());
			}
		}

	}

	EM_ELLIPSES_CATCH
	(
		MRI_BIN_TRACE1 (MriSevere, 50530, Status.errorcode, "MRI Dispatch2 Error=%x", 0);
		// Return LPVData to its original setting since the dispatch failed
		if (unrealizeptr)
			pMsg->lpvData = (void *)((PBYTE)m_pBufBase + (DWORD)pMsg->lpvData);
		return MR_STATUS_UNEXPECTED_EXCEPTION;
	)


	try 
	{
		// Close the handles for the mutex and semaphore objects
		if (hMutex) {
			if (!CloseHandle (hMutex)) {
				MRI_BIN_TRACE (MriWarning, GetLastError());
			}
		}

		if (hSemaphore) {
			if (!CloseHandle (hSemaphore)) {
				MRI_BIN_TRACE (MriWarning, GetLastError());
			}
		}
	}



	catch (STATUS status) {
		MRI_BIN_TRACE1 (MriSevere, 50530, status.errorcode, "MRI Dispatch3 Error=%x", 0);
		return status.errorcode;
	}

	MRI_BIN_TRACE4 (MriDataPath, 50531, pMsg, pMRCB, RouteId, 
		PortId,"MRI: Rcvd Msg Placed in Q, MsgPtr=%x, PortHandle=%x,Route=%x,Port=%x", 0);

	MRI_BIN_TRACE1 (MriVerbose, 50531, Status.errorcode, "MRI Dispatch Status=%x", 0);
	return Status.errorcode;
}	// end DispatchMsg

/*
*****************************************************************************************
*
*	Name:	
*		FindMRCB
*
*	Description:
*		This is a private function that obtains the Message Routing Control Block (MRCB)
*		for the target port (i.e., routing class / operation class).
*
*		If a valid open handle is available then this function first looks in 
*		the Attached_Ports array to determine if the pointer
*		to the MRCB has already been stored. If it hasn't then to find the target MRCB, 
*		it is necessary to walk through the routing structures beginning with the
*		the Routing class structure. The path to the MRCB is as follows:
*
*		Routing Class vector -> Port Id vector -> MRCB.
*
*		Each structure is contained in a Memory-Mapped File. Each MMF is opened and 
*		mapped. Each structure is verifed by validating its signature embedded within the 
*		structure.
*
*		If the MRCB must be obtained via walking the Routing class structure then the 
*		pMRCB is saved in the Attached_Ports array. It is much better to use this array
*		because each MapViewOfFile causes a page fault. If the working set is not high 
*		enough it will cause a hard page fault rather than a soft page fault which results
*		in horrendous performace degradation.  
*
*	Input Parameters:
*		portId:		message class
*
*		routeId:	message route
*
*		pMRCB:	Pointer to an MRCB pointer corresponding to the specified port.
*				This pointer is returned to the caller.
*
*	Return Values:
*		MR_STATUS_SUCCESS
*		MR_STATUS_FAILURE
*
*****************************************************************************************
*/
MR_STATUS MriSharedInternals::FindMRCB 
		(BYTE hostId, BYTE portId, BYTE routeId, PMR_MRCB *pMRCB, HANDLE *hMMF)
{
	HANDLE				hRoutingMMF = NULL, hPortMMF = NULL, hMRCBMMF = NULL;
	HANDLE				hRoutingVector = NULL, hPortVector = NULL, hMRCB = NULL;
	PMR_PORT_STRUCT		pPortVector;
	PMR_ROUTING_STRUCT	pRoutingVector;
	PMR_MRCB			pTargetMRCB;
	PMR_PORT_LIST		pPortList;
	WCHAR				MMF_Name[200];
	STATUS				Status;
	bool				ValidOpenHandle = FALSE;
	
	Status.errorcode = MR_STATUS_SUCCESS;
    
    //validate the hostId for VNMS bound ops, if invalid just send it to host 1
    if (routeId == VNMS && (hostId < 1 || hostId > MAXHOSTS))
    {
        if (m_pMRIGlobal)
            hostId = (m_pMRIGlobal->masterHost == NULL) ? 1 : m_pMRIGlobal->masterHost;
        else
            hostId = 1;
	}

	// First try to get the MRCB from the Attached Ports Array 
	try {

		// Need a Valid Open Handle to access the Attached Ports Array 
		if (!m_hOpen)
		{
			MRI_BIN_TRACE3 (MriInform, 50533, portId, routeId, m_hOpen,
						"FindMRCB: No OpenHandle for this Send, PortId=%x, RouteId=%x, hOpen=%x", 0);
		}
		else  // there is an open handle
		{
			VerifySignatureWithThrow ((PSIG_VERIFY)m_hOpen, OPEN_SIGNATURE, (USHORT)__LINE__);
			ValidOpenHandle = TRUE;
		}

		if (ValidOpenHandle)
		{
			if (routeId == VNMS)
			{
				// Update the routeid to point to the MDP that is currently 
				// handling VNMS sends
				VerifySignatureWithThrow 
						((PSIG_VERIFY)m_hOpen->hRoutingVector, RDS_SIGNATURE, (USHORT)__LINE__);
                routeId = m_hOpen->hRoutingVector->SendingMDP[hostId];
			}


			pPortList = (PMR_PORT_LIST)m_hOpen->hPortList;
			VerifySignatureWithThrow ((PSIG_VERIFY)pPortList, PLIST_SIGNATURE, (USHORT)__LINE__); 
			pTargetMRCB = (PMR_MRCB)pPortList->PortEntry [routeId][portId].hMRCB;
			if (!pTargetMRCB)
			{
				MRI_BIN_TRACE4 (MriInform, 50533, pPortList, portId, routeId, m_hOpen,
						"FindMRCB: Target MRCB Null in MLIST, pPortList=%x, PortId=%x, RouteId=%x, hOpen=%x", 0);
			}
			else // Target MRCB exists
			{
				VerifySignatureWithThrow 
						((PSIG_VERIFY)pTargetMRCB, MRCB_SIGNATURE, (USHORT)__LINE__);
				// Just make doubly sure this is the correct MRCB
				if ((pTargetMRCB->RouteId == routeId) &&
					(pTargetMRCB->PortId == portId))
				{ 
					*pMRCB = pTargetMRCB;
					Status.errorcode = MR_STATUS_SUCCESS;
					MRI_BIN_TRACE4 (MriVerbose, 50533, Status.errorcode, portId, 
						routeId, pMRCB,
						"FindMRCB: MRCB found in MLIST, Status=%x, PortId=%x, RouteId=%x pMRCB=%x", 0);
					return Status.errorcode;
				}
				else
				{
					// This failed so continue into MMF code to get MRCB
					MRI_BIN_TRACE4 (MriSevere, 50533, GetLastError(), pTargetMRCB->PortId, 
						pTargetMRCB->RouteId, pMRCB,
						"Port Not Registered: MRCB Bad, Status=%x, PortId=%x, RouteId=%x pMRCB=%x", 0);
				}
			} 
		} // Valid Open Handle
		MRI_BIN_TRACE4 (MriVerbose, 50533, pPortList, portId, routeId, m_hOpen,
						"FindMRCB: MRCB NOT found in MLIST, PortLsit=%x, PortId=%x, RouteId=%x, hOpen=%x", 0);

	}// try

	EM_ELLIPSES_CATCH
	(
		MRI_BIN_TRACE4 (MriSevere, 50532, GetLastError(), portId, routeId, m_hOpen,
			"Port Not Registered: error in MLIST, Status=%x, PortId=%x, RouteId=%x, Open=%x", 0);
	)



	try {


		*pMRCB = NULL;
		*hMMF = NULL;
		if (m_hRoutingVector != NULL)  {
			pRoutingVector = (PMR_ROUTING_STRUCT)m_hRoutingVector;
			MR_VERIFY_SIGNATURE (pRoutingVector, RDS_SIGNATURE, Status.errorcode);
			if (Status.errorcode != MR_STATUS_SUCCESS) {
				m_hRoutingVector = NULL;
			}
		}
		if (m_hRoutingVector == NULL)  {
			// Get a view of MRI's base Routing Vector
			wsprintf (MMF_Name, MR_ROUTING_VECTOR_MMF);
			hRoutingMMF = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, MMF_Name);
			if  (hRoutingMMF == NULL)  {
				Status.errorcode = MR_STATUS_INVALID_ROUTING_VECTOR;
				throw Status.errorcode;
			}
			// Get the pointer to MMF
			hRoutingVector = MapViewOfFile(hRoutingMMF, FILE_MAP_ALL_ACCESS, 0, 0, 0);
			pRoutingVector = (PMR_ROUTING_STRUCT)hRoutingVector;
			VerifySignatureWithThrow ((PSIG_VERIFY)pRoutingVector, RDS_SIGNATURE, (USHORT)__LINE__);
			MRI_BIN_TRACE3 (MriInform, 50532, pRoutingVector, portId, routeId,
				"FindMRCB: Mapped Routing Vector pRoutingTbl=%x, PortId=%x, RouteId=%x", 0);
			m_hRoutingVector = hRoutingVector;
		}

		if (routeId == VNMS)
		{
			// Update the routeid to point to the MDP that is currently handling VNMS sends
			routeId = pRoutingVector->SendingMDP[hostId];
		}

		// Ensure the port is actually in use
		if ( !(pRoutingVector->RouteInfo[routeId].bPortInUse) )  {
			Status.errorcode = MR_STATUS_INVALID_PORTID;
			MRI_BIN_TRACE3 (MriWarning, 50532, Status.errorcode, portId, routeId,
				"Port Not Registered: Null PortInUse, Status=%x, PortId=%x, RouteId=%x", 0);
			throw Status;
		}	

		// Get a view of MRI's Port Vector
		wsprintf (MMF_Name, pRoutingVector->RouteInfo[routeId].PortVectorMMF);
		hPortMMF = 
			OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, 
					pRoutingVector->RouteInfo[routeId].PortVectorMMF);
		if  (hPortMMF == NULL)  {
			Status.errorcode = MR_STATUS_INVALID_PORTID;
			MRI_BIN_TRACE3 (MriWarning, 50532, GetLastError(), portId, routeId,
				"Port Not Registered: Null PortVector, Status=%x, PortId=%x, RouteId=%x", 0);
			throw Status;
		}
		hPortVector = MapViewOfFile(hPortMMF, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		pPortVector = (PMR_PORT_STRUCT)hPortVector;
		VerifySignatureWithThrow ((PSIG_VERIFY)pPortVector, PORT_SIGNATURE, (USHORT)__LINE__);

		// Get view of MRCB for routeId/portId
		if ( !(pPortVector->PortInfo[portId].pUserMRCB) )  {
			Status.errorcode = MR_STATUS_INVALID_PORTID;
			MRI_BIN_TRACE3 (MriWarning, 50532, Status.errorcode, portId, routeId,
				"Port Not Registered: No MRCB, Status=%x, PortId=%x, RouteId=%x", 0);
			throw Status;
		}

		// Get view of MRCB
		wsprintf (MMF_Name, pPortVector->PortInfo[portId].MRCB_NAME);
		hMRCBMMF = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, pPortVector->PortInfo[portId].MRCB_NAME);
		if  (hMRCBMMF == NULL)  {
			Status.errorcode = MR_STATUS_INVALID_PORTID;
			MRI_BIN_TRACE3 (MriWarning, 50532, GetLastError(), portId, routeId,
				"PortNot Registered: Can't view MRCB, Status=%x, PortId=%x, RouteId=%x", 0);
			throw Status;
		}
		hMRCB = MapViewOfFile(hMRCBMMF, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		pTargetMRCB = (PMR_MRCB)hMRCB;
		VerifySignatureWithThrow ((PSIG_VERIFY)pTargetMRCB, MRCB_SIGNATURE, (USHORT)__LINE__);

 		// An MRCB has been found
		*pMRCB = pTargetMRCB;
		*hMMF = hMRCBMMF;
		Status.errorcode = MR_STATUS_SUCCESS;

		// Need an Open Handle to access the Attached Ports Array
		if (ValidOpenHandle)
		{
			// Update the Attached_Ports Array to reflect this new port 
			pPortList = (PMR_PORT_LIST)m_hOpen->hPortList;
			VerifySignatureWithThrow ((PSIG_VERIFY)pPortList, PLIST_SIGNATURE, (USHORT)__LINE__); 
			pPortList->PortEntry [routeId][portId].Signature = PENTRY_SIGNATURE;
			pPortList->PortEntry [routeId][portId].hMRCB     = pTargetMRCB;
			pPortList->PortEntry [routeId][portId].hMRCBMMF  = hMRCBMMF;
			MRI_BIN_TRACE4 (MriInform, 50533, pPortList, pPortList->PortEntry, portId, routeId, 
						"FindMRCB: MRCB Put in MLIST, pPortList=%x, PortEntry=%x, PortId=%x, RouteId=%x", 0);
		}


		// Unmap View of Files
		if (hPortMMF) {
			if (hPortVector)  {
				if (!UnmapViewOfFile (hPortVector)) {
					MRI_BIN_TRACE (MriWarning, GetLastError());
				}
			}
			if (!CloseHandle (hPortMMF)) {
				MRI_BIN_TRACE (MriWarning, GetLastError());
			}
			hPortMMF = NULL;
		}

	}

	catch (STATUS status) {
		MRI_BIN_TRACE3 (MriWarning, 50532, status.errorcode, portId, routeId,
			"Port Not Registered: FindMRCB catch1, Status=%x, PortId=%x, RouteId=%x", 0);

		// Delete the objects associated with the Port and MRCB MMF's
		if (hPortMMF) {
			if (hPortVector)  {
				if (!UnmapViewOfFile (hPortVector)) {
				MRI_BIN_TRACE (MriWarning, GetLastError());
				}
			}
			if (!CloseHandle (hPortMMF)) {
				MRI_BIN_TRACE (MriWarning, GetLastError());
			}
		}

	} // first catch

	EM_ELLIPSES_CATCH
	(
		MRI_BIN_TRACE3 (MriSevere, 50532, Status.errorcode, portId, routeId,
			"Port Not Registered: FindMRCB catch2, Status=%x, PortId=%x, RouteId=%x", 0);
		return MR_STATUS_UNEXPECTED_EXCEPTION;
	)

	
	MRI_BIN_TRACE4 (MriVerbose, 50533, Status.errorcode, portId, routeId, pMRCB,
			"MRI: FindMRCB, Status=%x, PortId=%x, RouteId=%x pMRCB=%x", 0);

	return Status.errorcode;

}	// FindMRCB


/*
*****************************************************************************************
*
*	Name:	
*		ProcessInterBoardMsg
*
*	Description:
*		This is a worker function that processes an interboard send request. 
*
*	Parameters:
*		pMsg:	pointer to a message (i.e., the Message Descriptor)
*
*
*	Return Values:
*		None
*
*****************************************************************************************
*/
void Message_Routing_Interface::ProcessInterBoardMsg (MR_MESSAGE *pMsg)
{
	MR_STATUS		retval = MR_STATUS_SUCCESS;
	bool			foundtarget = false;
	ULONG			bytesSent = 0;

    //EoMDP

    try  {

        //EoMDP


        if ( pMsg->Rclass == MRI && pMsg->OpClass == CL_MRI )
        {
            if ( IsSCA() )
            {
                pMsg->Rclass = IOMB;
                pMsg->OpClass = CL_HST_MGT;
                pMsg->MsgType = MR_CONTIGUOUS;
            }
            else
            {
                pMsg->Rclass = IOMA;
                pMsg->OpClass = CL_HST_MGT;
                pMsg->MsgType = MR_CONTIGUOUS;
            }

            retval = SendMRIMsg( pMsg, FALSE );
            if ( retval != MR_STATUS_SUCCESS )
            {
                MRI_BIN_TRACE3 ( MriSevere, 50533, pMsg->OpCode,pMsg->DestSlotId,retval,
                    "ProcessInterBoardMsg:SendMRI message failed, OpCode=%x, SlotId=%x, Status=%x", 0 );
                if( pMsg != NULL )
                {
                    FreeBuf( pMsg );
                    pMsg = NULL;
                }
            }
            return;
        }
	}//end try
//IBCM Removal
/*        else
        {
		    if (IsMRITrace (pMsg))
		    {
			    MRI_BIN_TRACE3 (MriWarning, 50518, pMsg, pMsg->OpClass, pMsg->Rclass,
				    "MRI: TraceBitSet on InterBoard Send, pMsg=%x, OpClass=%x, Rclass=%x", 0);
			    DumpHdr(pMsg);
		    }
		    // Instantiate an interboard comm manager object.
    //		IBCM_CommManager			Comm;
		    IBCM_BufferDescriptors		mVector;

		    // Set up BufferDescriptor vector object with components of message
		    if (pMsg->lpvData == ((PBYTE)pMsg + sizeof(MR_MESSAGE)))  {
			    // Message Descriptor and operation are contained in same buffer
			    IBCM_BufferDescriptor		msg( (BYTE *)pMsg, (ULONG)(sizeof(MR_MESSAGE) + pMsg->DataLen) );

			    mVector.push_back(msg);
    //			mVector.push_back( IBCM_BufferDescriptor( (BYTE *)pMsg, (ULONG)(sizeof(MR_MESSAGE) + pMsg->DataLen) );
		    } 
		    else  {
			    // Message Descriptor and operation are contained in separate buffers
			    IBCM_BufferDescriptor		mDesc( (BYTE *)pMsg, (ULONG)(sizeof(MR_MESSAGE)) );
			    IBCM_BufferDescriptor		operation( (BYTE *)pMsg->lpvData, (ULONG)(pMsg->DataLen) );

			    mVector.push_back(mDesc);
			    mVector.push_back(operation);
		    }

		    // Get list of targets.
		    IBCM_Targets	targets;
#ifdef _D_DESKTOP
#else
		    m_Comm.GetTargets (targets);
#endif
		    IBCM_Targets::iterator iTarget;


		    // Send a message via IBCM
		    switch (pMsg->MsgType)
		    {
			    case MR_ATTACH_NOTIFICATION:
			    case MR_DETACH_NOTIFICATION:
			    case MR_BROADCAST:
#ifdef PCIEMULATOR
				    iTarget = targets.begin();
				    if (retval = (MR_STATUS) m_Comm.Send ( *iTarget, mVector ))
					    throw Mri_Status (MR_STATUS_INTERBOARD_SEND_FAILURE);
#elif _D_DESKTOP
#else
				    MRI_BIN_TRACE2 (MriVerbose, 50533, pMsg->MsgType, pMsg->OpCode,
						    "Send Broadcast Attempted, Type=%x, OpCode=%x", 0);
				    if (retval = (MR_STATUS)m_Comm.Broadcast(mVector))
				    {
					    if (retval == IBCM_INCOMPLETE) 
					    {	// *Temporary Kludge until PCI Driver can handle MDP Deaths*
						    // Once of the boards could not receive this message 
						    MRI_BIN_TRACE1 (MriSevere, 50534, pMsg->OpCode,
							    "A BroadCast Message Failed to Be Sent to Dead MDP, MsgType=%x", 0);
					    
						    if (pMsg->OpCode == OP_SET_SEND_BOARD)
						    {
							    // Broadcast incomplete because other MDP can't receive it.  
							    // Return a positive result and move on.
							    MRI_BIN_TRACE1 (MriSevere, 50534, retval,
								    "SetSendBoard Rsp Sent for Dead MDP", 0);
							    ProcessSendBoardUpdated(DEAD_MDP);
						    }
					    }
					    else
						    throw Mri_Status (MR_STATUS_BROADCAST_FAILURE);
				    }
#endif
				    break;

			    case MR_INITIALIZATION_COMPLETE:
			    case MR_GREETINGS_REQUEST:
			    case MR_GREETINGS_RESPONSE:
				    // Iterate through the targets, searching for the matching target object.
				    for (iTarget = targets.begin(); iTarget != targets.end(); iTarget++) {
#ifdef PCIEMULATOR
					    foundtarget = true;
					    break;
#else
					    if (iTarget->GetDeviceID() == (WORD)pMsg->DestSlotId)  {
						    foundtarget = true;
						    break;
					    }
#endif
				    }
				    if (foundtarget)  {
#ifdef PCIEMULATOR
					    if (retval = (MR_STATUS) m_Comm.Send ( *iTarget, mVector ))  {
						    throw Mri_Status (retval);
					    }
#elif _D_DESKTOP
#else
					    // the behavior of this send is blocking (or synchronous)
					    if (retval = (MR_STATUS) m_Comm.Send ( *iTarget, mVector ))  { 
						    throw Mri_Status (MR_STATUS_INTERBOARD_SEND_FAILURE);
					    }
					    PostProcessMsg (*iTarget, mVector, retval);
#endif
				    }
				    else  {
					    throw Mri_Status (MR_STATUS_INVALID_REQUEST);
				    }
				    break;

			    default:
				    // Iterate through the targets, searching for the matching target object.
				    for (iTarget = targets.begin(); iTarget != targets.end(); iTarget++) {
#ifdef PCIEMULATOR
					    foundtarget = true;
					    break;
#else
					    if (iTarget->GetDeviceID() == (WORD)pMsg->DestSlotId)  {
						    foundtarget = true;
						    break;
					    }
#endif
				    }
				    if (foundtarget)  {
#ifdef PCIEMULATOR
					    if (retval = (MR_STATUS) m_Comm.Send ( *iTarget, mVector ))  {
						    throw Mri_Status (retval);
					    }
    #elif _D_DESKTOP
#else
					    // All interboards sends take this path. Prior to this change 
					    // all sends were blocking. This allows send requests to give
					    // immediate response instead of waiting for the transfer to occur.
					    // To use blocking an application must now use the Equeue interface.
					    MRI_BIN_TRACE3 (MriVerbose, 50533, pMsg->MsgType, pMsg->OpCode,pMsg->DestSlotId,
						    "InterBoardSend Broadcast Attempted, Type=%x, OpCode=%x, SlotId=%x", 0);
					    if (retval = (MR_STATUS) m_Comm.Send (*iTarget, 
														    mVector, 
														    AsyncCompletion,
														    this)) { 
						    throw Mri_Status (MR_STATUS_INTERBOARD_SEND_FAILURE);
					    }
					    // the following is temporary until the Async interface is resolved ...
					    // the behavior is blocking
    //					if (retval = (MR_STATUS) m_Comm.Send ( *iTarget, mVector ))  { 
    //						throw Mri_Status (MR_STATUS_INTERBOARD_SEND_FAILURE);
    //					}
    //					PostProcessMsg (*iTarget, mVector, retval);
					    // end of temporary blocking mode
#endif
				    }
				    else  {
					    throw Mri_Status (MR_STATUS_INVALID_REQUEST);
				    }
				    break;

		    }	// end switch
        }
	}	//	end try block*/

    catch( EM_NestedException& ex )
    {
        retval = (MR_STATUS)ex.GetId();
		MRI_BIN_TRACE1 (MriSevere, 50928, ex.GetId(),
				"MRI: IBCM Failure on Interboard Send, Status=%x", 11314);
		// Pop up box for initial testing
        std::wstring msg( ex.AsString() );
        ::MessageBox( NULL, msg.c_str(), L"Unexpected Exception", MB_ICONERROR|MB_OK );
		throw retval;
    }

	catch (Mri_Status& error)  {
		MRI_BIN_TRACE2 (MriSevere, 50534, error.code, retval,
			"MRI: ProcessInterBoardMsg failure - error status %x, IBCM retval=%x", 0);
		throw;
	}
	
	MRI_BIN_TRACE1 (MriVerbose, 50535, retval,"MRI: ProcessInterBoardMsg, retval=%x", 0);

	return;

}	// end ProcessInterBoardMsg

/*
*****************************************************************************************
*
*	Name:	
*		PostProcessMessage
*
*	Description:
*		The application invokes this function to send a message to one or more CMM boards.
*		To send a message, the application creates a message descriptor,
*		initializing fills several fields.
*
*	Parameters:
*		Handle: The handle returned by mri::open()
*
*		pmriMsg: Pointer to the mri message descriptor (and message) to be sent
*
*	Return Values:
*
*****************************************************************************************
*
void
Message_Routing_Interface::PostProcessMsg( const IBCM_Target& target, 
											   IBCM_BufferDescriptors& buffers,
											   ULONG status )
{

	MR_STATUS				retval;
	ULONG					numBufferObjects, idx = 0;
	IBCM_BufferDescriptor	msgObj;
	MR_MESSAGE				*pMsg;
	PBYTE					msgVector[2] = {0,0};

	STATUS	Status;
	Status.errorcode = MR_STATUS_SUCCESS;

	try {
//		Message_Routing_Interface mri;
		// get number of buffer objects in buffer descriptor vector
		numBufferObjects = buffers.size();

		// validate the number of buffer descriptor objects returned,
		// if invalid return the buffers
		if ( (numBufferObjects > 2) || (numBufferObjects == 0) ) {
			// trace error - invalid message format
			retval = MR_STATUS_INVALID_SENDDESC;
			MRI_BIN_TRACE (MriSevere, retval);
			
			// iterate through vector and release buffers
			for (IBCM_BufferDescriptors::iterator iBuffer = buffers.begin();
				 iBuffer != buffers.end();
				 iBuffer++)
			{
				pMsg = (MR_MESSAGE *)iBuffer->GetAddr();
				retval = FreeBuf (pMsg, true);
				if (retval)  {
					// trace error - invalid buffer, can't be free'd
				MRI_BIN_TRACE (MriSevere, retval);
				}
			}
			throw Mri_Status (retval);
		}

		// iterate through the buffer descriptor vector to get the 
		// buffer pointers from each buffer descriptor object
		for (IBCM_BufferDescriptors::iterator iBuffer = buffers.begin();
			 iBuffer != buffers.end();
			 iBuffer++)  {
				msgVector[idx] = iBuffer->GetAddr();
				idx++;
		}

		// set the pointer to the Message Descriptor
		pMsg = (MR_MESSAGE *)msgVector[0];

		// validate sent message & save status in MD
		if (retval = ValidateMD(pMsg, true))  {
			MRI_BIN_TRACE (MriSevere, retval);
			throw Mri_Status (retval);
		}

		if (pMsg->hEqueue)  {
			// send async status to sender
			pMsg->SendResult = status;
			if (retval = ReturnSendStatus(pMsg, (MR_STATUS)status))  {
				MRI_BIN_TRACE (MriSevere, retval);
				FreeBuf(pMsg);
				throw Mri_Status (retval);
			}
		}
		else
		{
			FreeBuf(pMsg);
		}
	}	// end try

	catch (Mri_Status& error)  {
		MRI_BIN_TRACE1(MriSevere, 50536, error.code,
			"MRI: PostProcessMsg failed, status=%x", 0);
		throw;
	}

	EM_ELLIPSES_CATCH
	(
		MRI_BIN_TRACE1 (MriSevere, 50536, retval, 
			"MRI: PostProcessMsg failure, status=%x", 0);
		return;
	)

	
	MRI_BIN_TRACE1 (MriVerbose, 50537, retval,	
		"MRI: PostProcessMsg, status=%x", 0);

	return;
}	// end PostProcessMsg
*/
/*
*****************************************************************************************
*
*	Name:	
*		ReturnSendStatus
*
*	Description:
*		This is a worker function that returns asynchronous status after a message 
*		has transferred to another board. It invokes SignalEQueue() which signals
*		the waiting sender or invokes a call-back function.  
*
*		The message sender's Event Queue is signaled using a semaphore object and locked
*		using a mutex object. The handles to the semaphore and mutex objects are saved
*		in the  queue structure created via the CreateEqueue interface.
*
*	Parameters:
*		pMsg:	pointer to the sent message
*		status:	status regarding the message transmission
*
*
*	Return Values:
*		MR_STATUS_SUCCESS
*		MR_STATUS_FAILURE
*
*****************************************************************************************
*/
MR_STATUS	Message_Routing_Interface::ReturnSendStatus ( MR_MESSAGE *pMsg, ULONG status)
{

	typedef		void MRI_Callback (MR_MESSAGE msg, MR_STATUS sendstatus);

	try {

//CKK 06/16/2010 Added (UINT_16) cast below to eliminate warning C4244
		pMsg->SendResult = (UINT_16)status;
		if (pMsg->lpvCallback)  {
			(pMsg->lpvCallback)((PBYTE)pMsg, status);
			return MR_STATUS_SUCCESS;
		}
		if (pMsg->hEqueue)  {
			// signal the event queue with the original message
			SignalEQueue( (PMR_Q_STRUCT)pMsg->hEqueue, pMsg );
			return MR_STATUS_SUCCESS;
		}
		// return status not required, just return the buffer(s)
//		Message_Routing_Interface mri;
		if (status = FreeBuf(pMsg)) {
			// trace error - free buf error - ReturnSendStatus
			MRI_BIN_TRACE (MriWarning, status); 
			return (MR_STATUS)status;
		}
	}

	EM_ELLIPSES_CATCH
	(
		MRI_BIN_TRACE1 (MriSevere, 50538, status, 
			"MRI: ReturnSendStatus Failed, status %x", 0);
		return MR_STATUS_UNEXPECTED_EXCEPTION;
    )  

	MRI_BIN_TRACE1 (MriVerbose, 50538, status, "MRI:ReturnSendStatus=%x", 0);
	return (MR_STATUS_SUCCESS);
}	// end ReturnSendStatus

/*
*****************************************************************************************
*
*	Name:	
*		SignalEQueue
*
*	Description:
*		This is a worker function that signals the Event Queue after a message is queued
*		to either an on-board message receive queue or is sentto another board. 
*
*		The message sender's Event Queue is signaled using a semaphore object and locked
*		using a mutex object. The handles to the semaphore and mutex objects are saved
*		in the  queue structure created via the CreateEqueue interface.
*
*	Parameters:
*		hEqueue:	Handle to the user's Event Queue (this is a pointer to the queue)
*
*
*	Return Values:
*		MR_STATUS_SUCCESS
*		MR_STATUS_FAILURE
*
*****************************************************************************************
*/
MR_STATUS Message_Routing_Interface::SignalEQueue( PMR_Q_STRUCT	pEventQ, MR_MESSAGE *pMsg )
{
	PBYTE			pBufBase = 0;
	DWORD			qDepth, dwWaitResult;
	MR_STATUS		status;

	try {

		dwWaitResult = WaitForSingleObject( pEventQ->hMutex, MR_TIMEOUT_FOREVER );
		switch (dwWaitResult) {

			case WAIT_OBJECT_0:
			{
                // CD-1004920 - jpd - 10/13/14 - change depth test. Testing for > MAX_QUEUE_DEPTH allows us to EnQ the MAX_QUEUE_DEPTH + 1 message to 
                //  the queue. However, hSemaphore was created with a limit of MAX_QUEUE_DEPTH entries so when we try to increment the semaphore for 
                //  the last allowed queue entry, we get a 12a ( 298 ) system error and the semaphore count isn't incremented.
                //  If we subsequently recover from the condition that caused the queueing overflow, we are off by 1 so after the waits clear out the 
                //  count, we end up with one message on the queue that isn't signaled because the semaphore count has reached zero. This causes that 
                //  message not to be retrieved until the next message arrives - and will remain out of sync indefinitely. Each time we overflow
                //  the queues and recover, we will be one more message off. The fix is to change the depth test so that we never queue that last extra 
                //  message.

				//en-queue the message to the event queue
				Queue	EventQ(pEventQ, 0);		// instantiate the Event Queue object;
				qDepth = EventQ.Size();
				if (qDepth >= MAX_QUEUE_DEPTH)  {
					MRI_BIN_TRACE3 (MriSevere, 50538, pMsg->Rclass, 
						pMsg->OpClass, pMsg->OpCode, 
						"Event Q full: RClass=%x, OpClass=%x, OpCode=%x\n", 0);
					throw Mri_Status (MR_STATUS_QUEUE_OVERFLOW);
				}

				// en-queue message on the Event queue
				if (status = EventQ.EnQ (pMsg))  {
					// log error and reject request
					MRI_BIN_TRACE (MriWarning, status); 
					throw Mri_Status (status);
				}

				// unlock the Event queue
				if (!ReleaseMutex (pEventQ->hMutex))  { 
					MRI_BIN_TRACE (MriSevere, GetLastError());
				}

				// signal the target thread
				if (!ReleaseSemaphore(pEventQ->hSemaphore, 1, 0))  {
					MRI_BIN_TRACE (MriSevere, GetLastError()); 
				}
				break;
			}

			case WAIT_TIMEOUT:
			case WAIT_ABANDONED:
			case WAIT_FAILED:
				MRI_BIN_TRACE (MriWarning, GetLastError()); 
				throw Mri_Status (MR_STATUS_WAIT_FAILURE);
				break;

			default:
				MRI_BIN_TRACE (MriWarning, dwWaitResult); 
				throw Mri_Status (MR_STATUS_WAIT_FAILURE);
				break;

		}

	}	// end try

	catch (Mri_Status& error)  {
		MRI_BIN_TRACE1 (MriSevere, 50539, error.code, 
			"Signal Equeue failure, error status %x", 0);
		// unlock the Event queue
		if (!ReleaseMutex (pEventQ->hMutex))  { 
			MRI_BIN_TRACE (MriWarning, GetLastError());
		}
		return error.code;
	}

	EM_ELLIPSES_CATCH
	(
		MRI_BIN_TRACE1 (MriSevere, 50539, status,
			"Signal Equeue failure, error status %x", 0);
				// unlock the Event queue
		if (!ReleaseMutex (pEventQ->hMutex))  { 
			MRI_BIN_TRACE (MriWarning, GetLastError());
		}
		return MR_STATUS_UNEXPECTED_EXCEPTION;
    )  


	MRI_BIN_TRACE1 (MriVerbose, 50539, status, "MRI: SignalEqueue, status %x", 0);
	return MR_STATUS_SUCCESS;
}	// end SignalEQueue

/******************************************************************************************
*
*	Name:
*		IsActiveSC
*	
*	Description:
*		Returns True if this is the active SC board, false otherwise.
*       Will throw an exception after logging it if encountered
*           calling routine Must handle returned exception
*	
*	Parameters:
*		None.
*	
*******************************************************************************************/

BOOLEAN		MriSharedInternals::IsActiveSC ()
{


    bool bNIU = true;  // bNIU is true for Signaling Server or TMP.  

    // Local ParamMgr object and ParamErr code.
    ParamMgr sigSrvParamScanSession;
    ParamErr iErrorCode = PE_NOERROR;

    // Check if Signaling Server section is present in the param file.
    iErrorCode = sigSrvParamScanSession.GetIsSigServer( &bNIU );
    if ( PE_NOERROR == iErrorCode )
    {
        if( bNIU )
        {
	        MRI_BIN_TRACE1( MriVerbose, 50539, 0,
		        "System is running as a Signaling Server.", 0);
        }
        else
        {
    	    MRI_BIN_TRACE1( MriVerbose, 50539, 0,
		        "System is running as a Media Server.", 0);
        }
    }
    else
    {
		iErrorCode = sigSrvParamScanSession.GetIsHMPServer( &bNIU );
		if ( PE_NOERROR == iErrorCode)
		{
			MRI_BIN_TRACE1( MriVerbose, 50539, 0, 
				"MRI: System is running as Media Server.", 0 );
		}
		else
		{
			if ( bSingleLogging )
			{
				MRI_BIN_TRACE1( MriWarning, 50539, iErrorCode,
				    "MRI: Failed to get Server Type attribute in param file. ParamMgr error code=%x", 0 );
				bSingleLogging = false;
			}
		}
	}

    // Always return true for a Signaling Server or TMP as there is no passive SC.
    if ( bNIU )
    {
        return TRUE;
    }

#ifdef _D_DESKTOP
        return TRUE;        // Assume is SC for desktop mode
#else		
    try
    {
	    // GetActiveSC returns 0 if running on PassiveSC
        if (GetActiveSC(g_MriCtlSemaphore) == 0) 
	    {
		    // The Failover Library believes this is a passive SC. However, it doesn't handle
		    // Split Domains so ensure this is passive by checking for MDPs. If MDPs can
		    // be seen than this is an Active SC no matter what Failover returns.
		    return FALSE;
	    }
	    else
	    {
		    return TRUE;
	    }
    }
    EM_ELLIPSES_CATCH
    (
        // If an exception is caught, this routine cannont handle it.
        //  Note it, log it, trace it and rethrow it.
        MRI_BIN_TRACE1 (MriSevere, 50573, e.getSeNumber(),
			"Failure attempting to obtain SC Active state, exception 0x%08x", 0);
        throw;
    )
#endif
}


/*
*****************************************************************************************
*
*	Name:	
*		ValidateMD
*
*	Description:
*		This is a private function that validates the Message Descriptor.
*
*		The fields within the Message Descriptor that are validated include:
*			OpCode:		If OpCode contains a NULL value, MR_STATUS_INVALID_OPCODE is
*						returned.
*			OpClass:	If OpClass contains a NULL value or exceeds the MAX_OPCLASS 
*						value, MR_STATUS_INVALID_OPCLASS is returned.
*			Rclass:		If Rclass contains a NULL value, MR_STATUS_INVALID_RCLASS is
*						returned.
*			DataLen:	If DataLen contains a NULL value or does not match the length
*						in the message data, MR_STATUS_INVALID_DATA_LEN is returned.
*			lpvData:	If lpvData contains a NULL value, MR_STATUS_INVALID_DATAPTR is
*						returned.
*			hEqueue:	If hEqueue is set and lpvCallback is also set, MR_STATUS_INVALID_ASYNC
*						is returned.
*			lpvCallback:If lpvCallback is set and hEqueue is also set, MR_STATUS_INVALID_ASYNC
*						is returned.
*			MsgType:	If MsgType is invalid, MR_STATUS_INVALID_MSGTYPE is returned.
*			DestSlotId:	If DestSlotId is set but is invalid, MR_STATUS_INVALID_DESTSLOT is
*						returned. 
*			
*	Input Parameters:
*		pMsg:		pointer to a Message Descriptor
*
*	Return Values:
*		MR_STATUS_SUCCESS
*		MR_STATUS_FAILURE
*		MR_STATUS_INVALID_OPCODE
*		MR_STATUS_INVALID_OPCLASS
*		MR_STATUS_INVALID_RCLASS
*		MR_STATUS_INVALID_DATA_LEN
*		MR_STATUS_INVALID_DATAPTR
*		MR_STATUS_INVALID_ASYNC
*		MR_STATUS_INVALID_MSGTYPE
*		MR_STATUS_INVALID_DESTSLOT
*
*****************************************************************************************
*/
MR_STATUS MriSharedInternals::ValidateMD (MR_MESSAGE *pMsg)
{	
	
	STATUS	Status;
	Status.errorcode = MR_STATUS_SUCCESS;

	try {
		if (pMsg) {
			// validate operation code
			if (!(pMsg->OpCode))  {
				Status.errorcode = MR_STATUS_INVALID_OPCODE;
				throw Status;
			}
			//validate operation class
			if ((!(pMsg->OpClass)) || 
				((pMsg->OpClass >= CL_EXT_CLASS_TOP) && (pMsg->OpClass < CL_INT_OPCLASS_BOTTOM)) ||
				(pMsg->OpClass >= CL_INT_OPCLASS_TOP))  {
				Status.errorcode = MR_STATUS_INVALID_OPCLASS;
				throw Status;
			}
			//validate routing class
			if ((!(pMsg->Rclass)) || (pMsg->Rclass >= MAXRCLASS))  {
				Status.errorcode = MR_STATUS_INVALID_RCLASS;
				throw Status;
			}
			//validate message ptr
			if (!(pMsg->lpvData))  {
				Status.errorcode = MR_STATUS_INVALID_DATAPTR;
				throw Status;
			}
			//validate message type
			if ((pMsg->MsgType == MR_CONTIGUOUS) && 
				(pMsg->lpvData != ((PBYTE)pMsg + sizeof(MR_MESSAGE))))  {
				Status.errorcode = MR_STATUS_INVALID_MSGTYPE;
				MRI_BIN_TRACE2 (MriSevere, 50542, (DWORD)pMsg->lpvData,	 
					DWORD((PBYTE)pMsg + sizeof(MR_MESSAGE)),
					"MRI ValidateMd, Message Contiguous but lpvData=%x, Msg=%x\n", 0);
				throw Status;
			}
			if ((pMsg->MsgType == MR_NONCONTIGUOUS) && 
				(pMsg->lpvData == ((PBYTE)pMsg + sizeof(MR_MESSAGE))))  {
				Status.errorcode = MR_STATUS_INVALID_MSGTYPE;
				MRI_BIN_TRACE2 (MriSevere, 50542, (DWORD)pMsg->lpvData,	
					DWORD((PBYTE)pMsg + sizeof(MR_MESSAGE)),
					"MRI ValidateMd, Message NonContiguous but lpvData=%x, Msg=%x\n", 0);
				throw Status;
			}
			if (pMsg->MsgType >= MAXMSGTYPE)  {
				Status.errorcode = MR_STATUS_INVALID_MSGTYPE;
				throw Status;
			}
			//validate data length
			int	datalen;
			memcpy ((char *)&datalen, pMsg->lpvData, 4);
			datalen = datalen >> 8;
			if ((!(pMsg->DataLen)) || (pMsg->DataLen != datalen))  {
				Status.errorcode = MR_STATUS_INVALID_DATA_LEN;
				MRI_BIN_TRACE3 (MriSevere, 50542, Status.errorcode, datalen, pMsg->DataLen,	
					"MRI ValidateMd Status=%x, datalen=%x, MsgLen=%x\n", 0);
				throw Status;
			}
			// validate asynchronous set-up
			if ((pMsg->hEqueue) && (pMsg->lpvCallback))  {
				Status.errorcode = MR_STATUS_INVALID_ASYNC;
				throw Status;
			}
			if (pMsg->hEqueue)  {
				if ((!m_hOpen) || (!(m_hOpen->hUserEQ)))  {
					Status.errorcode = MR_STATUS_INVALID_ASYNC;
					throw Status;
				}
			}
			// validate destination slot ID
//			ULONG boardMask;
//			GetBoardMask(&boardMask);
//			if (!(1 << pMsg->DestSlotId) & boardMask)  {
//				Status.errorcode = MR_STATUS_INVALID_DESTSLOT;
//				throw Status;
//			}
			// Set MRI signature in Message Descriptor
			pMsg->Signature = MD_SIGNATURE;
		}	// end (if pMsg)
	}	// end try
	
	catch (STATUS Status)	{
		MRI_BIN_TRACE2 (MriSevere, 50541, Status.errorcode, pMsg,	
			"MRI ValidateMd Error=%x, pMsg=%x", 0);
		return Status.errorcode;
	}
	
	EM_ELLIPSES_CATCH
	(
		MRI_BIN_TRACE2 (MriSevere, 50541, Status.errorcode, pMsg,	
			"MRI ValidateMd Error=%x, pMsg=%x", 0);
		return MR_STATUS_UNKNOWN_EXCEPTION_THROWN;
	)

	MRI_BIN_TRACE2 (MriVerbose, 50542, Status.errorcode, pMsg,
		"MRI ValidateMd Status=%x, pMsg=%x", 0);
	return MR_STATUS_SUCCESS;

}	// end ValidateMD

/*
*****************************************************************************************
*
*	Name:	
*		ValidateMD
*
*	Description:
*		This is a private function that validates the Message Descriptor.
*
*		The fields within the Message Descriptor that are validated include:
*			OpCode:		If OpCode contains a NULL value, MR_STATUS_INVALID_OPCODE is
*						returned.
*			OpClass:	If OpClass contains a NULL value or exceeds the MAX_OPCLASS 
*						value, MR_STATUS_INVALID_OPCLASS is returned.
*			Rclass:		If Rclass contains a NULL value, MR_STATUS_INVALID_RCLASS is
*						returned.
*			DataLen:	If DataLen contains a NULL value or does not match the length
*						in the message data, MR_STATUS_INVALID_DATA_LEN is returned.
*			lpvData:	If lpvData contains a NULL value, MR_STATUS_INVALID_DATAPTR is
*						returned.
*			hEqueue:	If hEqueue is set and lpvCallback is also set, MR_STATUS_INVALID_ASYNC
*						is returned.
*			lpvCallback:If lpvCallback is set and hEqueue is also set, MR_STATUS_INVALID_ASYNC
*						is returned.
*			MsgType:	If MsgType is invalid, MR_STATUS_INVALID_MSGTYPE is returned.
*			DestSlotId:	If DestSlotId is set but is invalid, MR_STATUS_INVALID_DESTSLOT is
*						returned. 
*			
*	Input Parameters:
*		pMsg:		pointer to a Message Descriptor
*
*	Return Values:
*		MR_STATUS_SUCCESS
*		MR_STATUS_FAILURE
*		MR_STATUS_INVALID_OPCODE
*		MR_STATUS_INVALID_OPCLASS
*		MR_STATUS_INVALID_RCLASS
*		MR_STATUS_INVALID_DATA_LEN
*		MR_STATUS_INVALID_DATAPTR
*		MR_STATUS_INVALID_ASYNC
*		MR_STATUS_INVALID_MSGTYPE
*		MR_STATUS_INVALID_DESTSLOT
*
*****************************************************************************************
*/
MR_STATUS MriSharedInternals::ValidateMD (MR_MESSAGE *pMsg, bool sentMsg)
{	
	
	STATUS	Status;
	Status.errorcode = MR_STATUS_SUCCESS;

	try {
		if (pMsg) {
			// validate MRI signature in Message Descriptor
			if (pMsg->Signature != MD_SIGNATURE)  {
				Status.errorcode = MR_STATUS_INVALID_REQUEST;
				throw Status;
			}
			// validate the current process 
			if (GetCurrentProcessId() != pMsg->Pid)  {
				Status.errorcode = MR_STATUS_INVALID_REQUEST;
				throw Status;
			}
			// validate operation code
			if (!(pMsg->OpCode))  {
				Status.errorcode = MR_STATUS_INVALID_OPCODE;
				throw Status;
			}
			//validate operation class
			if ((!(pMsg->OpClass)) || 
				((pMsg->OpClass >= CL_EXT_CLASS_TOP) && (pMsg->OpClass < CL_INT_OPCLASS_BOTTOM)) ||
				(pMsg->OpClass >= CL_INT_OPCLASS_TOP))  {
				Status.errorcode = MR_STATUS_INVALID_OPCLASS;
				throw Status;
			}
			//validate routing class
			if ((!(pMsg->Rclass)) || (pMsg->Rclass >= MAXRCLASS))  {
				Status.errorcode = MR_STATUS_INVALID_RCLASS;
				throw Status;
			}
			//validate message ptr
			if (!(pMsg->lpvData))  {
				Status.errorcode = MR_STATUS_INVALID_DATAPTR;
				throw Status;
			}
			//validate message type
			if ((pMsg->MsgType == MR_CONTIGUOUS) && 
				(pMsg->lpvData != ((PBYTE)pMsg + sizeof(MR_MESSAGE))))  {
				Status.errorcode = MR_STATUS_INVALID_MSGTYPE;
				throw Status;
			}
			if ((pMsg->MsgType == MR_NONCONTIGUOUS) && 
				(pMsg->lpvData == ((PBYTE)pMsg + sizeof(MR_MESSAGE))))  {
				Status.errorcode = MR_STATUS_INVALID_MSGTYPE;
				throw Status;
			}
			if (pMsg->MsgType >= MAXMSGTYPE)  {
				Status.errorcode = MR_STATUS_INVALID_MSGTYPE;
				throw Status;
			}
			//validate data length
			int	datalen;
			memcpy ((char *)&datalen, pMsg->lpvData, 4);
			datalen = datalen >> 8;
			if ((!(pMsg->DataLen)) || (pMsg->DataLen != datalen))  {
				Status.errorcode = MR_STATUS_INVALID_DATA_LEN;
				throw Status;
			}
			// validate asynchronous set-up
//			if (!(pMsg->hEqueue))  {
//				Status.errorcode = MR_STATUS_INVALID_ASYNC;
//				throw Status;
//			}
		}	// end (if pMsg)
	}	// end try
	
	catch (STATUS Status)	{
		MRI_BIN_TRACE1 (MriSevere, 50541, Status.errorcode,	"MRI ValidateMd Status=%x", 0);
		return Status.errorcode;
	}
	
	EM_ELLIPSES_CATCH
	(
		MRI_BIN_TRACE1 (MriSevere, 50541, Status.errorcode,	"MRI ValidateMd Status=%x", 0);
		return MR_STATUS_UNKNOWN_EXCEPTION_THROWN;
	)


	MRI_BIN_TRACE1 (MriVerbose, 50542, Status.errorcode,	"MRI ValidateMd Status=%x", 0);
	return MR_STATUS_SUCCESS;

}	// end ValidateMD

/*****************************************************************************************
*
*	Name:	
*		ProcessHMS
*
*	Description:
*       Change routing class to forward to HMS if present and appropriate.
*		
*
*	Parameters:
*		pMsg:	Pointer to the message
*
*****************************************************************************************/
bool MriSharedInternals::ProcessHMS( MR_MESSAGE *pMsg )
{    
    PMR_GLOBALS pGlobals = m_pMRIGlobal;
    bool blocked = false;

    try
    {
        if( pGlobals->is_HMSInstalled )
        {
            if( !( IsActiveSC() )  )
            {
                if( ( pMsg->OpClass == CL_NAP_LOG ) && ( pMsg->Rclass == VNMS ) )
                {
                    pMsg->Rclass = IsSCA() ? VNMSB : VNMSA;
                    return true;
                }
            }
    
            if ( pGlobals->is_HMSPresent) 
            {
                //if the CMM is marked nonshared, ignore all ops for or from the other hosts  with 
                // the exceptions of CL_MRI ( internal ops ) and CL_HST_MGT, op >= OP_POLL_TELESERVER ( f0 ) ( internal ops ).
                //
                // Can only be marked shared again by the same host that marked it
                //  nonshared.
                if (pGlobals->is_CMM_NonShared && pMsg->Vnmsid != pGlobals->CMM_NonShared_Host)
                {
                        switch ( pMsg->OpClass )
                        {
                            case CL_CONFIG:
                                switch ( pMsg->OpCode )
                                {
                                    case OP_REINITIALIZE:
                                        // Log reinit attempt if denied
                                        MriLogging = true;
		                                MRI_BIN_TRACE1 (MriSevere, 11213, pMsg->Vnmsid,	"Reinit blocked while CMM in non-Share mode from host = %x", 11213);
                                        MriLogging = false;
                                        break;

                                    case OP_TSP_INQUIRY:
                                        // Log inquiry attempt if denied
                                        MriLogging = true;
		                                MRI_BIN_TRACE1 (MriSevere, 11214, pMsg->Vnmsid,	"CMM Inquiry blocked while CMM in non-Share mode from host = %x", 11214);
                                        MriLogging = false;
                                        break;

                                    default:
                                        break;
                                }

                                // These are all blocked
                                blocked = true;
                                break;

                            case CL_MRI:                            // let these pass through
                            case CL_NAP_LOG:
                                break;

                            case CL_HST_MGT:
                                if(  pMsg->OpCode < OP_POLL_TELESERVER ) 
                                {
                                    blocked = true;
                                }
                                // Don't block the internal ops ( <= OP_POLL_TELESERVER ) 
                                break;
  

                            default:
                               blocked = true;
                        }
                        if( blocked )
                        {
                            return false;
                        }
                }

                switch (pMsg->OpClass)
                {
                    case CL_HMS:
                        if (pMsg->OpCode == OP_TSP_SHARED)
                        {
                            pGlobals->is_CMM_NonShared = false;
                            pGlobals->CMM_NonShared_Host = 0;

                        }
                        else if (pMsg->OpCode == OP_TSP_NONSHARED)
                        {
                            pGlobals->is_CMM_NonShared = true;
                            pGlobals->CMM_NonShared_Host = pMsg->Vnmsid;
                        }
                        break;

                    case CL_VOICE:
                    case CL_DATATRANS:
                    case CL_SIGNALING:
                    case CL_EVENT:
                    case CL_CONTROL:
                    case CL_LINE_EVENT:
                    case CL_PRIM_SIG:
                        pMsg->Rclass = (pMsg->Rclass == VNMS) ? HMS_OUT : HMS_IN;
                        break;
                    case CL_TSP_MGT:
                        if ((pMsg->OpCode == OP_BD_FAILURE || pMsg->OpCode == OP_BD_RECOVERY) && 
                            pMsg->Rclass == VNMS)
                            pMsg->Rclass = HMS_OUT;
                        break;
                    case CL_NAP_LOG:
                        pMsg->Rclass = HMS_OUT;
                        break;
                }     
            }
        }
    }
    EM_ELLIPSES_CATCH
    (
        MRI_BIN_TRACE2 (MriSevere, 50574, e.getSeNumber(), pMsg,
                "Exception encountered in ProcessHMS :  0x%08x, msgaddr = 0x%08x", 0);
    )
    
    return true;
}


//.public:MriDetachHandler
/*================================================================================

  MriDetachHandler

  Purpose:
    Constructor.

================================================================================*/
MriDetachHandler::MriDetachHandler(Message_Routing_Interface *pMri)
{
	m_pMri = pMri;
}

/*================================================================================

  ~MriDetachHandler

  Purpose:
    Destructor.

================================================================================*/
MriDetachHandler::~MriDetachHandler()
{

}
/******************************************************************************************
* 
*	Name:
*		DeleteRcvQueueStructures
*
*	Description:
*
******************************************************************************************/

void	MriDetachHandler::DeleteRcvQueueStructures (PMR_MRCB	pMRCB) {

	PMR_OPEN_HANDLE		hOpen;
	MR_STATUS			status = MR_STATUS_SUCCESS;

	try {

		if (pMRCB->pUserMutex) {
			if (!CloseHandle (pMRCB->pUserMutex)) {
				MRI_BIN_TRACE (MriWarning, GetLastError());
			}
			pMRCB->pUserMutex = NULL;
		}
		
		if (pMRCB->pUserSemaphore) {
			if (!CloseHandle (pMRCB->pUserSemaphore)) {
				MRI_BIN_TRACE (MriWarning, GetLastError());
			}
			pMRCB->pUserSemaphore = NULL;
		}
		
		if (pMRCB->pRcvQStruct)  {
			if (!CloseHandle (pMRCB->pRcvQStruct->hMMF)) {
				MRI_BIN_TRACE (MriWarning, GetLastError());
			}
			if (!UnmapViewOfFile (pMRCB->pRcvQStruct)) {
				MRI_BIN_TRACE (MriWarning, GetLastError());
			}
			pMRCB->pRcvQStruct = NULL;
		}


		// Indicate that the only existing MRCB is gone
		hOpen = (PMR_OPEN_HANDLE)pMRCB->OpenHandle;
		hOpen->hFirstMRCB = NULL;
	}


	EM_ELLIPSES_CATCH
	(
		MRI_BIN_TRACE1 (MriSevere, 50543, GetLastError(), 
					"MRI DeleteRcvQueueStructures Failure=%x", 0);
		return;
	)

	MRI_BIN_TRACE1 (MriVerbose, 50543, status, "MRI DeleteRcvQueueStructures Status=%x", 0);
	return;

}	// DeleteRcvQueueStructures




/***************************************************************************************
*	
*	Name:
*		DestroyPort
*
*	Description:
*
*
****************************************************************************************/

void MriDetachHandler::DestroyPort (
			 PMR_MRCB			pMRCB,
			 PMR_OPEN_HANDLE	hOpen) {

	PMR_ROUTING_STRUCT	pRoutingStruct;
	PMR_PORT_STRUCT		pPortStruct, hPortStruct = NULL;
	UINT_8				PortId, RouteId;
	HANDLE				pTempMRCB;
	STATUS				Status;

	Status.errorcode = MR_STATUS_SUCCESS;

	try {

		// Ensure the entry in the Routing Table matches the MRCB that is to be deleted

		// Find the MRCB in the Routing Table
		pRoutingStruct = (MR_ROUTING_STRUCT *)hOpen->hRoutingVector;
		MriSharedInternals::VerifySignatureWithThrow 
			((PSIG_VERIFY)pRoutingStruct, RDS_SIGNATURE, (USHORT)__LINE__);
		RouteId = pMRCB->RouteId;
		// Get the Port Vector
		pPortStruct = 
			(PMR_PORT_STRUCT)pRoutingStruct->RouteInfo[RouteId].pUserPortVector;
		MriSharedInternals::VerifySignatureWithThrow 
			((PSIG_VERIFY)pPortStruct, PORT_SIGNATURE, (USHORT)__LINE__); 
		// Ensure the MRCB in the RDS is the same as the one that is to be deattached
		PortId = pMRCB->PortId;
		pTempMRCB = pPortStruct->PortInfo[PortId].pUserMRCB;
		if (pTempMRCB != pMRCB) {
			Status.errorcode = MR_STATUS_INVALID_HANDLE;
			throw Status;
		} else {
			// save the RDS for passing to the DeleteMRCB
			hPortStruct = pPortStruct;
		}

		// Delete the MRCB
		DeleteMRCB (pMRCB, pPortStruct);
		pRoutingStruct->AttachedPortCount--;

	}


	catch (STATUS &status) {
		MRI_BIN_TRACE2 (MriSevere, 50544, status.errorcode, pMRCB,
			"MRI DestroyPort Status=%x, hMRCB=%x", 0);
		return;
	}

	EM_ELLIPSES_CATCH
	(
		MRI_BIN_TRACE2 (MriSevere, 50544, GetLastError(), pMRCB,
			"MRI DestroyPort Status=%x, hMRCB=%x", 0);
		return;

	)

	MRI_BIN_TRACE4 (MriInform, 50544, Status, pMRCB, RouteId, PortId,
		"MRI DestroyPort Status=%x, hMRCB=%x, RouteId=%x, PortId=%x", 0);
	return;

}



/*****************************************************************************************
*
*	Name:
*		DeleteMRCB
*
*	Description:
*		This functions deletes the MRCB by Unmapping a view to it. The RDS is locked
*		down during this proces so no other process should be accessing it.
*		Also if the MRCB belongs to this board the Receive Queue is deleted 
*		if this is the last user of it.
*
*****************************************************************************************/


void MriDetachHandler::DeleteMRCB (
				 PMR_MRCB			pMRCB,
				 PMR_PORT_STRUCT	pPortStruct) {

	PMR_Q_STRUCT		pRcvStruct;
	UINT_8				PortId, RouteId;
	BOOLEAN				DeletePortStruct = TRUE;
	PMR_ROUTING_STRUCT	pRoutingStruct;
	PMR_OPEN_HANDLE		hOpen;
	PMR_MRCB			TempMRCB;
	STATUS				Status;
	
	Status.errorcode = MR_STATUS_SUCCESS;

	try {


		// Set the state in case we stumble on this structure when debugging
		pMRCB->Attached = FALSE;

		hOpen = (PMR_OPEN_HANDLE)pMRCB->OpenHandle;

		PortId  = pMRCB->PortId;
		RouteId = pMRCB->RouteId;

		// If the Port Struct exists then erase anything regarding this MRCB
		if (pPortStruct) {
			pPortStruct->PortInfo[PortId].pUserMRCB = NULL;
			ZeroMemory (pPortStruct->PortInfo[PortId].MRCB_NAME, MMF_size);
			// If this is the last MRCB for this Route then delete the Port Vector.
			// We can't leave the structure around in case the process is stopped and restarted
			// since it won't be able to view the pointer.
			for (PortId = 0; PortId < MR_MAX_PORT_CLASSES; PortId ++) {
				if (pPortStruct->PortInfo[PortId].pUserMRCB) {
					DeletePortStruct = FALSE;
					break;
				}
			}
			if (DeletePortStruct) {
				if (!CloseHandle (pPortStruct->hPortMMF)) {
					MRI_BIN_TRACE (MriWarning, GetLastError());
				}
				if (!UnmapViewOfFile (pPortStruct)) {
					MRI_BIN_TRACE (MriWarning, GetLastError());
				}
				pRoutingStruct = hOpen->hRoutingVector;
				pRoutingStruct->RouteInfo[pMRCB->RouteId].bPortInUse = FALSE;
				pRoutingStruct->RouteInfo[pMRCB->RouteId].pUserPortVector = NULL;
				ZeroMemory(pRoutingStruct->RouteInfo[pMRCB->RouteId].PortVectorMMF, MMF_size);
			} // DeletePortStruct
		}

		// Update the Open Handle to point to the next MRCB
		if (hOpen->hFirstMRCB == pMRCB) 
			hOpen->hFirstMRCB = pMRCB->NextMRCB;

		// Update the Forward and Backwards links.
		// If there is a backwards MRCB Link then update it to point to the next MRCB
		if (TempMRCB = (PMR_MRCB)pMRCB->PreviousMRCB)
			TempMRCB->NextMRCB = pMRCB->NextMRCB;
		// If there is a forward MRCB link then update it to point to the previous MRCB
		if (TempMRCB = (PMR_MRCB)pMRCB->NextMRCB) 
			TempMRCB->PreviousMRCB = pMRCB->PreviousMRCB;


		// If the MRCB belongs to this board determine if the Rcv Q should be deleted 
		UINT_8 ThisBoardId;
		m_pMri->GetMySlot(&ThisBoardId);
		if (pMRCB->BoardId == ThisBoardId) 
		{
			// Handle the Receive Queue
			pRcvStruct = pMRCB->pRcvQStruct;
			try {
				MriSharedInternals::VerifySignatureWithThrow 
					((PSIG_VERIFY)pRcvStruct, RCVQ_SIGNATURE, (USHORT)__LINE__);
				// Decrement the number of users and determine if it needs to be deleted
				pMRCB->pRcvQStruct->PortCount--;
				if (pMRCB->pRcvQStruct->PortCount == 0) {
					DeleteRcvQueueStructures (pMRCB);
				}
			}
			catch (STATUS status) {
				MRI_BIN_TRACE1 (MriSevere, 50545, status.errorcode, 
					"MRI DeleteMRCB Status=%x", 0);
				throw status;
			}

			// Delete the MRCB
			CloseHandle (pMRCB->hMRCBMMF);
			if (!UnmapViewOfFile (pMRCB)) 
			{
				MRI_BIN_TRACE (MriWarning, GetLastError());
			} // try
		} // if MRCB belongs to this board

		// Clean up the Attached_Ports Array for this MRCB and close any handles to 
		// currently mapped files.
		if (hOpen->hPortList) 
		{
			HANDLE hRcvQ = NULL, hRcvQMMF = NULL, hMRCB = NULL, hMRCBMMF = NULL;
			PMR_PORT_LIST pPortList = (PMR_PORT_LIST)hOpen->hPortList;
			MriSharedInternals::VerifySignatureWithThrow 
						((PSIG_VERIFY)pPortList, PLIST_SIGNATURE, (USHORT)__LINE__);
			// Set this Port entry's signature to Inactive
			pPortList->PortEntry [RouteId][PortId].Signature = INACTV_SIGNATURE;

			// Delete the objects associated with the Receive Queue
			hRcvQ	 =	pPortList->PortEntry [RouteId][PortId].hRcvQ;
			hRcvQMMF =	pPortList->PortEntry [RouteId][PortId].hRcvQMMF;
			if (hRcvQ)  {
				if (!UnmapViewOfFile (hRcvQ)) 
				{
					MRI_BIN_TRACE (MriWarning, GetLastError());
				}
				if (!CloseHandle (hRcvQMMF)) 
				{
					MRI_BIN_TRACE (MriWarning, GetLastError());
				}
			}

			// Delete the objects asociated with the MRCB
			hMRCB	 =	pPortList->PortEntry [RouteId][PortId].hMRCB;
			hMRCBMMF =	pPortList->PortEntry [RouteId][PortId].hMRCBMMF;
			if (hMRCB)  {
				if (!UnmapViewOfFile (hMRCB)) 
				{
					MRI_BIN_TRACE (MriWarning, GetLastError());
				}
				if (!CloseHandle (hMRCBMMF)) 
				{
					MRI_BIN_TRACE (MriWarning, GetLastError());
				}
			}
		}  // PortList exists


	}

	EM_ELLIPSES_CATCH
	(
		MRI_BIN_TRACE2 (MriSevere, 50545, GetLastError(), pMRCB, 
				"MRI DeleteMRCB Status=%x, pMRCB=%x", 0);
		return;
	)


	MRI_BIN_TRACE2 (MriVerbose, 50545, Status, pMRCB,
		"MRI DeleteMRCB Status=%x, pMRCB=%x", 0);
	return; 

}	// DeleteMRCB



//.public:MriAttachHandler
/*================================================================================

  MriAttachHandler

  Purpose:
    Constructor.

================================================================================*/
MriAttachHandler::MriAttachHandler(Message_Routing_Interface *pMri)
{
	m_pMri = pMri;
}

/*================================================================================

  ~MriAttachHandler

  Purpose:
    Destructor.

================================================================================*/
MriAttachHandler::~MriAttachHandler()
{

}


/******************************************************************************************
*
*	Name
*		ProcessApplicationFailure
*
*	Description
*		This function handles a client issuing an Attach request for a Port that is already
*		attached. If the MRCB passed in is Null than it is presumed the application has  
*		either crashed or terminating without closing all its ports. In this situation
*		the OS frees the memory and MRI must only clear its table.
*		If the MRCB is non-null and the Open Handle is different then the previous handle
*		it is presumed that the application is in an error mode but has not restarted.  
*		In this scenario this function 
*		closes all ports associated with the stale open handle of this application and
*		then reattaches the new port.  If this Open Handle is the same as the old one then
*		only the current port is detached and reattached.
*
*******************************************************************************************/

MR_STATUS	MriAttachHandler::ProcessApplicationFailure (
										PMR_MRCB			StaleMRCB,
										UINT_8				RouteId,
										UINT_8				PortId,
										UINT_8				BoardId,
										PMR_OPEN_HANDLE		hOpen,
										MR_HANDLE			*PortHandle) {

	STATUS				Status;
	Status.errorcode	= MR_STATUS_SUCCESS;
	MR_HANDLE			StaleOpenHandle;

    MRI_BIN_TRACE4 (MriSevere, 50557, RouteId, PortId, BoardId, hOpen,
				"MRI: Application Attempting To Reattach" 
					"RouteId=%x,PortId=%x,BoardId=%x,hOpen=%x", 0);

	// Unlock so we can recursively call into MRI
	if (hOpen->hRDSMutex)
		m_pMri->ReleaseLock (hOpen->hRDSMutex);

	if (StaleMRCB == NULL) 
	{
		// The application has previosuly terminated and is now restarted. The OS should
		// have cleared all old structures so MRI just needs to clear out the stale
		// Port Vector.
		HANDLE				PortStruct;

		// This Routing Class is stale - need to recreate the Port Vector for this 
		// process so it can be accessed.
		if (Status.errorcode =  CreatePortVector 
				(RouteId, (MR_ROUTING_STRUCT *)hOpen->hRoutingVector, &PortStruct)) 
		{
			MRI_BIN_TRACE (MriWarning, Status.errorcode);
			throw Status;
		}
	} 
	else 
	{

		// There is an old MRCB that is still valid in this context so we need to
		// release its structures. This is caused by an application restarted on
		// a remote board and this board gets notified of a second attach
		StaleOpenHandle = StaleMRCB->OpenHandle;
		// only detach this port
		MRI_BIN_TRACE4 (MriWarning, 50546, StaleMRCB, RouteId, PortId, Status.errorcode,
			"MRI: Detaching Existing Port from prior run and reattaching, StaleMRCB=%x,	RouteId=%x, PortId=%x, Status=%x",
			0);
		if (Status.errorcode = m_pMri->DetachPort (StaleMRCB)) 
		{
			MRI_BIN_TRACE4 (MriWarning, 50547, StaleMRCB, RouteId, PortId, Status.errorcode,
				"MRI: Couldn't Detach Stale MRCB Handle, StaleMRCB=%x, RouteId=%x, PortId=%x, Status==%x",
				0);
		return Status.errorcode;
		}

	} // Stale MRCB

	MRI_BIN_TRACE4 (MriVerbose, 50548, StaleMRCB, RouteId, PortId, Status.errorcode,
				"MRI: ProcessApplicationFailure, StaleMRCB=%x,	RouteId=%x, PortId=%x, Status=%x",
				0);

	// Reattach the port
	MR_HANDLE	ReAttachedPort;
	if (Status.errorcode = 
		this->AttachThisPort 
				(RouteId, PortId, BoardId, hOpen, &ReAttachedPort)) {
				// Lock down the Routing Table with its Mutex
		if (!m_pMri->GetLock (hOpen->hRDSMutex, MR_TIME_TO_WAIT)) 
			MRI_BIN_TRACE (MriSevere, GetLastError());
		return Status.errorcode;
	} else {
		*PortHandle = ReAttachedPort;
		// Get the Lock again so Lock down the Routing Table with its Mutex
		if (!m_pMri->GetLock (hOpen->hRDSMutex, MR_TIME_TO_WAIT)) 
			MRI_BIN_TRACE (MriSevere, GetLastError());
		return MR_STATUS_SUCCESS;
	}



} // ProcessApplicationFailure





/***************************************************************************************
*
*	Name:
*		MapVNMSRouteId
*
*	Description:
*		This function changes the RouteId to either VNMSA or VNMSB depending on which
*		MDP is requesting the Attach. This allows both MDPs to attach to the same
*		VNMS classes and then MRI can internally detect which is the proper board to 
*		pass it to.  This is necessary for IO Redundancy.
*
*		Note that this function is only run on the MDP that is making the  
*		attach request. The reason is that once the attach notification is sent to
*		the other boards, the RouteId has already been converted to VNMSA or VNMSB.
*
****************************************************************************************/



MR_STATUS  MriAttachHandler::MapVNMSRouteId (UINT_8 BoardId, UINT_8 *RouteId) {

    MR_STATUS           status = MR_STATUS_SUCCESS;
    TCHAR               ComputerName[ MAX_PATH ] = L"";
    TCHAR*              pName = NULL;
    // EoMDP
    //bool              mdpA;

    try 
    {
        wcscpy( ComputerName, GetHostName().c_str() );
        pName=wcschr( ComputerName, DELIMITER );
        if ( pName == NULL )
        {
            MRI_BIN_TRACE1 ( MriSevere, 50551,  BoardId, 
                    "MRI MapVNMSRouteId Failed to get the board name, BoardId=%x", 0 );
            return MR_STATUS_FAILURE;
        }
        else
        {
            pName++;
        }
        pName=wcschr( pName, DELIMITER );
        if ( pName == NULL )
        {
            MRI_BIN_TRACE1 ( MriSevere, 50551,  BoardId, 
                    "MRI MapVNMSRouteId Failed to get the board name, BoardId=%x", 0 );
            return MR_STATUS_FAILURE;
        }
        else
        {
            pName++;
        }
        // Check if the board is SCA.
        // mdpA=(wcscmp(pName,_T("MDPA")) == 0) || (wcscmp(pName,_T("mdpa")) == 0);
        if( ( wcscmp( pName, SCA ) == ZERO ) || 
            ( wcscmp( pName, sca ) == ZERO ) )
        {
            *RouteId = VNMSA;
        } 
        else 
        {
            //if ((wcscmp(pName,_T("MDPB")) == 0) || (wcscmp(pName,_T("mdpb")) == 0))
            if( ( wcscmp( pName, SCB ) == ZERO ) || 
                ( wcscmp( pName, scb ) == ZERO ) )
            {
                *RouteId = VNMSB;
            }
            else  // this isn't an MDP board
            {
                MRI_BIN_TRACE2 ( MriSevere, 50551,  BoardId, RouteId, 
                    "MRI MapVNMSRouteId Failed Due to Not MDP, BoardId=%x, RouteId=%x", 0 );
                return MR_STATUS_INVALID_REQUEST;
            }

        }
    }

    EM_ELLIPSES_CATCH
    (
        MRI_BIN_TRACE3 ( MriSevere, 50551, BoardId, RouteId, GetLastError(), 
            "MRI MapVNMSRouteId Failed, BoardId=%x, RouteId=%x, Error=%x", 0 );
        return MR_STATUS_UNKNOWN_EXCEPTION_THROWN;
    )


    MRI_BIN_TRACE2 ( MriInform, 50551, *RouteId, BoardId, 
            "MRI Mapped VNMS Attach to RouteId=%x for Board=%x", 0 );

    return status;

}


/******************************************************************************************
*   
*	Name:
*		CreateMMF
*
*	Description:
*		This function creates a memory mapped file and maps a view to it.
*
*	Parameters:
*		FileName - the name that is to be used for the file
*		FileSize - how big the file should be
*		hFile - a handle to the file which is returned to the caller
*
******************************************************************************************/

MR_STATUS   MriAttachHandler::CreateMMF 
	(WCHAR FileName[200], int FileSize, HANDLE *hFile, HANDLE *hMappedFile) {
	
	HANDLE	hMapFile;
	HANDLE	hMappedView;

	try {

		// Handle security issues		
		// This provides a NULL DACL which will allow access to everyone.

        SECURITY_DESCRIPTOR sd;

        if (!InitializeSecurityDescriptor( &sd, SECURITY_DESCRIPTOR_REVISION ))
        {
            MRI_BIN_TRACE (MriWarning, GetLastError());
			return MR_STATUS_COULD_NOT_CREATE_MMF;
        }

        if (!SetSecurityDescriptorDacl( &sd, TRUE, NULL, FALSE ))
        {
            MRI_BIN_TRACE (MriWarning, GetLastError());
			return MR_STATUS_COULD_NOT_CREATE_MMF;
        }

        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof sa;
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = FALSE;

		hMapFile = CreateFileMapping(
			INVALID_HANDLE_VALUE,			// Current file handle. 
			&sa,                            // Default security. 
			PAGE_READWRITE,                 // Read/write permission. 
			0,                              // Max. object size. 
			FileSize,                       // Size of File. 
			FileName);				        // Name of mapping object. 
 
		if (hMapFile == NULL) 
		{ 
            MRI_BIN_TRACE (MriWarning, GetLastError());
			return MR_STATUS_COULD_NOT_CREATE_MMF; 
		} 


		hMappedView = MapViewOfFile(hMapFile, // Handle to mapping object. 
			FILE_MAP_ALL_ACCESS,               // Read/write permission 
			0,                                 // Max. object size. 
			0,                                 // Size of File. 
			0);                                // Map entire file. 
 
		if (hMappedView == NULL) 
		{ 
            MRI_BIN_TRACE (MriWarning, GetLastError());
			return MR_STATUS_COULD_NOT_CREATE_MMF; 
		} 


		ZeroMemory(hMappedView, FileSize);
		
		*hFile = hMapFile;
		*hMappedFile = hMappedView;

	}

	EM_ELLIPSES_CATCH
	(
        MRI_BIN_TRACE (MriWarning, GetLastError());
		return MR_STATUS_UNKNOWN_EXCEPTION_THROWN;
	)

	MRI_BIN_TRACE1 (MriVerbose, 50549, FileName, "MRI CreateMMF, FileName=%s", 0);

	return MR_STATUS_SUCCESS;

} // Create_MMF





/*******************************************************************************************
*
*	Name:
*		CeateRcvQ
*
*	Description:
*		The Receive Queue is created based on the Route Id and the Port Id. A Memory
*		Mapped File is used so that the Q can be seen across processes.  Along with the 
*		Receive Queue, the Mutex and Sempahore for locking the queue are also created.
*
*		The Receive Queue Name, the Mutex Name and the Semaphore Name are all stored
*		in the MRCB so they can be retrieved across processes. The Name of each object is 
*		created based on the RouteId and the PortId.
*
*	Parameters:
*		pMRCB :		The MRCB that the Receive Queue will be stored in.
*		RouteId:	The Routing Class that this Receive Queue will service.
*		PortId:		The Op Class that this Receive Queue will service.
*
*******************************************************************************************/

MR_STATUS MriAttachHandler::CreateRcvQ 
									(PMR_MRCB			pMRCB, 
									UINT_8				RouteId, 
									UINT_8				PortId) {

	PMR_Q_STRUCT		pRcvQ;
	HANDLE				pMMF, pMMFMapped;
	WCHAR				MMF_Name [200];
	WCHAR				ObjectName [200];
	STATUS				Status;
	Status.errorcode	= MR_STATUS_SUCCESS;

	try {
		// Handle security issues		
		// This provides a NULL DACL which will allow access to everyone.

        SECURITY_DESCRIPTOR sd;

        if (!InitializeSecurityDescriptor( &sd, SECURITY_DESCRIPTOR_REVISION ))
        {
            MRI_BIN_TRACE (MriWarning, GetLastError());
        }

        if (!SetSecurityDescriptorDacl( &sd, TRUE, NULL, FALSE ))
        {
            MRI_BIN_TRACE (MriWarning, GetLastError());
        }

        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof sa;
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = FALSE;

		// Create the Receive Queue - use MMF
		wsprintf (MMF_Name, MR_RCV_Q_MMF, RouteId, PortId); 
		Status.errorcode = CreateMMF (MMF_Name, (sizeof(Q_STRUCT)), &pMMF, &pMMFMapped);
		if (Status.errorcode == MR_STATUS_SUCCESS) {
			pRcvQ = (PMR_Q_STRUCT)pMMFMapped;
			pRcvQ->Signature = RCVQ_SIGNATURE;
			pRcvQ->hMMF = pMMF;
			pMRCB->pRcvQStruct = pRcvQ;
			memcpy(pMRCB->RcvQSName, MMF_Name, MMF_size);
			pRcvQ->PortCount++;  // Increment for number of users on queue

		} else {
			throw Status;
		}

		// Create the Mutex
		wsprintf (ObjectName, MR_RCVQ_MUTEX_NAME, RouteId, PortId); 
		pMRCB->pUserMutex = CreateMutex (&sa, FALSE, ObjectName);
		if (pMRCB->pUserMutex) {
			memcpy(pRcvQ->MutexName, ObjectName, MMF_size);
			pRcvQ->hMutex    = pMRCB->pUserMutex;
		} else {
			Status.errorcode = MR_STATUS_MUTEX_FAILURE;
			throw Status;
		}

		// Create the Semaphore
		wsprintf (ObjectName, MR_SEMAPHORE_NAME, RouteId, PortId);
		pMRCB->pUserSemaphore = CreateSemaphore (&sa, 0, MAX_QUEUE_DEPTH, ObjectName);
		if (pMRCB->pUserSemaphore) {
			pRcvQ->hSemaphore		=  pMRCB->pUserSemaphore;
			memcpy(pRcvQ->SemaphoreName, ObjectName, MMF_size);
		} else {
			Status.errorcode = MR_STATUS_SEMAPHORE_FAILURE;
			throw Status;
		}


		
	}

	catch (STATUS status) {
		MRI_BIN_TRACE3 (MriSevere, 50550, status.errorcode, PortId, RouteId, 
			"MRI CreateRcvQ Failed, Status=%x, PortId=%x, RouteId=%x", 0);
		return status.errorcode;
		// Any structures allocated here are deleted in the AttachPort cleanup 
	}	// catch1


	EM_ELLIPSES_CATCH
	(
		MRI_BIN_TRACE3 (MriSevere, 50550, Status.errorcode, PortId, RouteId, 
			"MRI CreateRcvQ Failed, Status=%x, PortId=%x, RouteId=%x", 0);
		return MR_STATUS_UNKNOWN_EXCEPTION_THROWN;
	)

	MRI_BIN_TRACE3 (MriVerbose, 50550, Status.errorcode, PortId, RouteId, 
		"MRI CreateRcvQ, Status=%x, PortId=%x, RouteId=%x", 0);

	return Status.errorcode;
}




/***************************************************************************************
*
*	Name:
*		CreatePortVector
*
*	Description:
*		This function creates a Port Vector in the Routing Data Structure based. The 
*		Port Vector is named based on the RouteId and a Memory Mapped File is used
*		in order to share the Port Vector across processes (i.e. the user thread and
*		MRI process).
*
*	Parameters:
*		RouteId :			The Routing Class that is to be enabled
*		pRouting Struct:	Pointer to the RDS
*		PortStruct :		The address of the Port Vector to be returned to caller.
*
***************************************************************************************/

MR_STATUS  MriAttachHandler::CreatePortVector (
						UINT_8				RouteId,
						PMR_ROUTING_STRUCT	pRoutingStruct,
						HANDLE				*PortStruct) {

	WCHAR 				PortFileName [MMF_size];
	HANDLE				pMMF, pMMFMapped;
	PMR_PORT_STRUCT		pPortStruct;
	MR_STATUS			status = MR_STATUS_SUCCESS;

	try {

		wsprintf (PortFileName, MR_PORT_VECTOR_MMF, RouteId); 
		status = CreateMMF (PortFileName, sizeof(MR_PORT_STRUCT), 
										&pMMF, &pMMFMapped); 
		if (status == MR_STATUS_SUCCESS) { 
			// Save the PortVector pointer for this process (i.e on user thread)
			pPortStruct = (MR_PORT_STRUCT *)pMMFMapped; // For local use
			pPortStruct->hPortMMF = pMMF;
			pPortStruct->Signature = PORT_SIGNATURE;
			pRoutingStruct->RouteInfo[RouteId].bPortInUse = TRUE;
			pRoutingStruct->RouteInfo[RouteId].pUserPortVector = pPortStruct;
			memcpy(pRoutingStruct->RouteInfo[RouteId].PortVectorMMF, PortFileName, MMF_size);
			*PortStruct = pPortStruct;
			// InitializePortVector (pPortStruct);
		}
	}

	EM_ELLIPSES_CATCH
	(
		MRI_BIN_TRACE2 (MriSevere, 50551, status, RouteId, 
			"MRI CreatePortVector Failed, Status=%x, RouteId=%x", 0);
		return MR_STATUS_UNKNOWN_EXCEPTION_THROWN;
	)


	MRI_BIN_TRACE2 (MriVerbose, 50551, status, RouteId, 
			"MRI CreatePortVector, Status=%x, RouteId=%x", 0);

	return status;

}



/****************************************************************************************
*
*	Name:
*		CreateMRCB
*
*	Description:
*		This function creates a Message Routing Control Block based on the application's
*		RouteId and PortId. The MRCB is also initialized.
*
****************************************************************************************/

MR_STATUS MriAttachHandler::CreateMRCB 
				(UINT_8			RouteId, 
				UINT_8			PortId,
				UINT_8			SlotId,
				PMR_PORT_STRUCT	hPort,
				PMR_OPEN_HANDLE	hOpen,
				PMR_MRCB		*MRCB)		{


	WCHAR				MRCBName [MMF_size];
	HANDLE				pMMF, pMMFMapped;
	PMR_MRCB			pMRCB;
	PMR_PORT_STRUCT		pPortStruct;
	MR_STATUS			status = MR_STATUS_SUCCESS;

	try {
	
		pPortStruct = hPort;
		wsprintf (MRCBName, MR_MRCB_MMF, RouteId, PortId); 
		status = CreateMMF (MRCBName, sizeof(MR_MRCB), &pMMF, &pMMFMapped); 
		if (status == MR_STATUS_SUCCESS) {
			// Save the MRCB Pointer for this thread in the PortStruct
			pMRCB =(MR_MRCB *)pMMFMapped;
			pMRCB->hMRCBMMF = pMMF;
			pMRCB->Signature = MRCB_SIGNATURE;
			pPortStruct->PortInfo[PortId].pUserMRCB = pMRCB;
			memcpy (pPortStruct->PortInfo[PortId].MRCB_NAME, MRCBName, MMF_size);
			pMRCB->ProcessId = GetCurrentProcess ();
			pMRCB->PortId  = PortId;
			pMRCB->RouteId = RouteId;
			pMRCB->BoardId = SlotId;
			pMRCB->OpenHandle = hOpen;
			pMRCB->NextMRCB = NULL;
			pMRCB->PreviousMRCB = NULL;
			*MRCB = pMRCB;
		} 

		// indicate that another port is in use
		hOpen->hRoutingVector->AttachedPortCount++;
	}

	EM_ELLIPSES_CATCH
	(
		MRI_BIN_TRACE4 (MriSevere, 50552, status, RouteId, PortId, SlotId, 
			"MRI: CreateMRCB Failed, Status=%x, RouteId=%x, PortId=%x, SlotId=%x", 0);
			return MR_STATUS_UNKNOWN_EXCEPTION_THROWN;
	)

	MRI_BIN_TRACE4 (MriVerbose, 50552, status, RouteId, PortId, SlotId,
			"MRI: CreateMRCB, Status=%x, RouteId=%x, PortId=%x, SlotId=%x", 0);

	return status;

}


/***************************************************************************************
*	
*	Name:	
*		AssignRcvQToMRCB
*	
*	Description:
*		This function takes Receive Queue information stored in a port's MRCB and 
*		copies it into another port's MRCB.  The ultimate purpose of this function
*		is to share a Receive Queue across multiple ports in a process.
*		Also note that the Receive Structure is updated to indicate that there is 
*		another port using this Receive Queue. This is necessary to determine when 
*		to destroy the Queue when a detach is done.
*
*	Parameters:
*		OldMRCB - the MRCB that the Receive Queue information is to be copied from.
*		NewMRCB - the MRCB that the Receive Queue information is to be copied into.
*
****************************************************************************************/

MR_STATUS MriAttachHandler::AssignRcvQToMRCB (
									PMR_MRCB		OldMRCB,
									PMR_MRCB		NewMRCB) {

	PMR_Q_STRUCT		pRcvStruct;
	MR_STATUS			status = MR_STATUS_SUCCESS;

	try {

		// Ensure the Original MRCB is a valid one
		MriSharedInternals::VerifySignatureWithThrow 
			((PSIG_VERIFY)OldMRCB, MRCB_SIGNATURE, (USHORT)__LINE__);
		pRcvStruct = OldMRCB->pRcvQStruct;
		MriSharedInternals::VerifySignatureWithThrow 
			((PSIG_VERIFY)pRcvStruct, RCVQ_SIGNATURE, (USHORT)__LINE__);

		// Update the Receive Queue structure to indicate another user
		pRcvStruct->PortCount++;  // Increment for number of users on queue

		// Update the new MRCB
		NewMRCB->pRcvQStruct		= pRcvStruct;
		memcpy(NewMRCB->RcvQSName, OldMRCB->RcvQSName, MMF_size);
		NewMRCB->pUserMutex			= OldMRCB->pUserMutex;
		NewMRCB->pUserSemaphore		= OldMRCB->pUserSemaphore;
	
	}

	EM_ELLIPSES_CATCH
	(
		MRI_BIN_TRACE3 (MriSevere, 50553, status, OldMRCB, NewMRCB,
			"MRI AssignRcvQToMRCB Failed, Status=%x, OldMRCB=%x, NewMRCB=%x", 0);
		return MR_STATUS_UNKNOWN_EXCEPTION_THROWN;
	)

	MRI_BIN_TRACE3 (MriVerbose, 50553, status, OldMRCB, NewMRCB,
			"MRI AssignRcvQToMRCB, Status=%x, OldMRCB=%x, NewMRCB=%x", 0);
	return status;

}


/****************************************************************************************
*	Name:	
*		LinkMrcbToList
*
*	Description:
*		This function links a newly created MRCB into the list of existing MRCBs.
*
*****************************************************************************************/

void MriAttachHandler::LinkMrcbToList (
								PMR_OPEN_HANDLE		hOpen,
								PMR_MRCB			hNewMRCB) {

	PMR_MRCB		CurrentMRCB, PreviousMRCB;

	try {

		CurrentMRCB = (PMR_MRCB)hOpen->hFirstMRCB;

		while (CurrentMRCB) {
			PreviousMRCB = CurrentMRCB;
			// Get the Next MRCB in the list
			CurrentMRCB = (PMR_MRCB)CurrentMRCB->NextMRCB;
			if (CurrentMRCB == NULL) {
				// Set the previous MRCB to point to the newly created one
				PreviousMRCB->NextMRCB = hNewMRCB;
				// Set the newly created MRCB to point back to the previous MRCB
				hNewMRCB->PreviousMRCB = PreviousMRCB;
			}
		}
	}

	EM_ELLIPSES_CATCH
	(
		MRI_BIN_TRACE (MriSevere, GetLastError());
		return;
	)

	MRI_BIN_TRACE2 (MriVerbose, 50554, hOpen, hNewMRCB,
		"MRI: LinkMrcbToList, hOpen=%p, hNewMrcb=%p", 0);
	return;

} // LinkMrcbToList




#pragma optimize( "", off )

/******************************************************************************************
*
*	Name:
*		AttachThisPort
*
*	Description:
*		This function assumes the Routing Vector stays locked by the calling function.
*	
*******************************************************************************************/

MR_STATUS MriAttachHandler::AttachThisPort (
										  UINT_8				RouteId,
										  UINT_8				PortId,
										  UINT_8				BoardId,
										  PMR_OPEN_HANDLE		hOpen,
										  MR_HANDLE				*PortHandle) {

	STATUS				Status;
	volatile PMR_PORT_STRUCT		pPortStruct	= NULL;
	PMR_MRCB			pMRCB		= NULL;
	HANDLE				PortStruct;
	volatile PMR_ROUTING_STRUCT	pRoutingStruct;
	MR_HANDLE			ReAttachedPort;
	Status.errorcode	= MR_STATUS_SUCCESS;

	try {

		// Get the Routing Structure

		pRoutingStruct = (MR_ROUTING_STRUCT *)hOpen->hRoutingVector;
		MriSharedInternals::VerifySignatureWithThrow 
			((PSIG_VERIFY)pRoutingStruct, RDS_SIGNATURE, (USHORT)__LINE__);

		// If it's a RouteId of VNMS then map it to appropriate MDP board
		if (RouteId == VNMS)
		{
			// This will change the RouteId to either VNMSA or VNMSB 
			Status.errorcode = MapVNMSRouteId (BoardId, &RouteId);
			if (Status.errorcode)
				throw Status;
		}


		// Get the Port Vector
		if (pRoutingStruct->RouteInfo[RouteId].bPortInUse) {
			// Routing Class already exists so just get pointer to Port Vector
			pPortStruct = 
				(PMR_PORT_STRUCT)pRoutingStruct->RouteInfo[RouteId].pUserPortVector;
			MriSharedInternals::VerifySignatureWithThrow 
				((PSIG_VERIFY)pPortStruct, PORT_SIGNATURE, (USHORT)__LINE__); 
		} else {
			// This Routing Class hasn't been used before so create the Port Vector
			Status.errorcode = 
					CreatePortVector (RouteId, pRoutingStruct, &PortStruct); 
			if (Status.errorcode == MR_STATUS_SUCCESS) {
				pPortStruct =  (PMR_PORT_STRUCT) PortStruct;
			} else {
				throw Status;
			}
		} // Get the Port Vector

		// Allocate an MRCB
		if (pPortStruct->PortInfo[PortId].pUserMRCB) {
			// This Port has already been attached. In this situation, the user 
			// has not terminated but wishes to reuse the port. 
			// Detach the Port and Reattach.
			Status.errorcode = this->ProcessApplicationFailure (
								(PMR_MRCB)pPortStruct->PortInfo[PortId].pUserMRCB, 
								RouteId, PortId, BoardId, hOpen, &ReAttachedPort);
			*PortHandle = ReAttachedPort;
			// return so that no further processing takes place
	        MRI_BIN_TRACE4 (MriInform, 50555, RouteId, PortId, BoardId, hOpen,
				"MRI: AttachThisPort Reattaching,RouteId=%x,PortId=%x,BoardId=%x,hOpen=%p", 0);
			return Status.errorcode;
					
		} else {
			// Create an MRCB for this port 
			if (Status.errorcode = 
				CreateMRCB (RouteId, PortId, BoardId, pPortStruct, hOpen, &pMRCB)) {
				throw Status;
			}
		} // Allocate an MRCB

		// If the Attach is for this board then Get Receive Queue from the Open Handle. 
		// If there is no MRCB in the Open Handle then there is no Receive Q 
		// and thus one must be created. 
		UINT_8 ThisSlotId;
		m_pMri->GetMySlot(&ThisSlotId);

		if (ThisSlotId == BoardId) {
			if (hOpen->hFirstMRCB) {
				// User already has a receive queue
				Status.errorcode = 
					AssignRcvQToMRCB ((PMR_MRCB)hOpen->hFirstMRCB, pMRCB); 
				if (Status.errorcode != MR_STATUS_SUCCESS) {
					Status.errorcode = MR_STATUS_INVALID_PORT_HANDLE;
					throw Status;
				}
			} else { 
				// Create the Receive Queue
				if (Status.errorcode = CreateRcvQ (pMRCB, RouteId, PortId)) {
					throw Status;
				}
			} // FirstMRCB
			// Notify the other boards about this new Port
			m_pMri->SendInterBoardNotification 
				(PortId, RouteId, MR_ATTACH_NOTIFICATION, MR_SENDALLBOARDS);
			// Notify Receive Monitor on this board so it can count this attach
			// **TBD - The following is needed for PerfMon but it doesn't work yet
			// m_pMri->SendInterBoardNotification 
				// (PortId, RouteId, MR_ATTACH_NOTIFICATION, ThisSlotId);
		} // ThisSlotId == BoardId


		// Link MRCBs together
		if (hOpen->hFirstMRCB) {
			// Link this MRCB in with the other existing MRCBs
			LinkMrcbToList (hOpen, pMRCB);
		} else {
			// this is the First MRCB
			hOpen->hFirstMRCB = (HANDLE)pMRCB;
		}

		// Port Successfully Attached - Return the MRCB for this port
		*PortHandle = (MR_HANDLE)pMRCB;
		pMRCB->Attached = TRUE;
		
	}


	catch (STATUS	Status) {
		if ((Status.errorcode == MR_STATUS_INVALID_DATAPTR) && (pPortStruct)) {
			// The Port Vector handle must have been invalid. This is caused by the 
			// application failing and then restarting. 
	        MRI_BIN_TRACE4 (MriWarning, 50556, RouteId, PortId, BoardId, Status.errorcode,
				"MRI: AttachThisPort ,RouteId=%x,PortId=%x,BoardId=%x,Status=%x", 0);
			Status.errorcode = this->ProcessApplicationFailure 
								(NULL, RouteId, PortId, BoardId, hOpen, &ReAttachedPort);
			*PortHandle = ReAttachedPort;
		} else {
			// some other error occured
	        MRI_BIN_TRACE4 (MriSevere, 50556, RouteId, PortId, BoardId, Status.errorcode,
				"MRI: AttachThisPort Failed,RouteId=%x,PortId=%x,BoardId=%x,Status=%x", 0);
			// If the MRCB exists handle RcvQ and then delete the MRCB 
			if (pMRCB) {
				MriDetachHandler	*Detach;
				Detach = new MriDetachHandler(m_pMri); 
				Detach->DeleteMRCB (pMRCB, pPortStruct);
				delete Detach;
			}
		}
		return Status.errorcode;
	}


    MRI_BIN_TRACE4 (MriInform, 50556, RouteId, PortId, BoardId, Status.errorcode,
				"MRI: AttachThisPort, RouteId=%x,PortId=%x,BoardId=%x,Status=%x", 0);

	return Status.errorcode;
	
}	// AttachThisPort

#pragma optimize( "", on )


/****************************************************************************************
*					
*	Name:
*		BuildSetSendResponse
*	
*	Description:
*		This routine builds the Set_Send_Board_Ack op. This is a reply to IO Service
*		indicating that all boards have changed their sending board to the new one.
*
***************************************************************************************/


MR_STATUS Message_Routing_Interface::BuildSetSendReply (
						  MR_MESSAGE				**pMsg,
						  PMR_GLOBALS				pGlob)  {
	

	MR_MESSAGE	*msg;
	MR_STATUS	Status = MR_STATUS_SUCCESS;
    int msgLen = sizeof(MR_MESSAGE) + sizeof(mrisetsendbdop);

	MR_STATUS result = AllocateBuf( msgLen, &msg);
	if (result)
	{
		MRI_BIN_TRACE1(MriSevere, 50528, msgLen, 
				"Could Not Allocate Memory to Send SetBoardAck, MsgSize=%x", 0);
		return MR_STATUS_RESOURCE_FAILURE;
	}

	UINT_8 SlotId;
	GetMySlot(&SlotId);
		
    //Clear the allocated area
    memset(msg, 0, msgLen);

    msg->MsgType = MR_CONTIGUOUS;
    msg->DataLen = sizeof(mrisetsendbdop);
    msg->lpvData = (PBYTE)msg + sizeof(MR_MESSAGE);
    msg->Pid = GetCurrentProcessId();
    msg->lpvCallback = NULL;
	msg->SrcSlotId = SlotId;
	msg->DestSlotId = SlotId;

    msg->OpCode = OP_RC_SET_SEND_BD_ACK;
    msg->OpClass = CL_HST_MGT;
    msg->Rclass = pGlob->RequestingMDP;
    
    mrisetsendbdop *SetSendAck = (mrisetsendbdop *)msg->lpvData;
    SetSendAck->opcode     = OP_RC_SET_SEND_BD_ACK;
    int AckLen = sizeof(mrisetsendbdop);
    memcpy(SetSendAck->oplength, (char*)&AckLen, 3);
	SetSendAck->reqiomgr   = pGlob->RequestingMDP;
	SetSendAck->sendiomgr  = pGlob->SendingMDP[pGlob->HostNumber];
	SetSendAck->hostnumber = pGlob->HostNumber;
   	
	*pMsg = msg;

	MRI_BIN_TRACE2(MriVerbose, 50528, msgLen, Status,
				"BuildSetSendReply Successful, MsgSize=%x, Status=%x", 0);

	return Status;

}




/**************************************************************************************
*
*	Name: 
*		ProcessSendBoardUpdated
*
*	Description:
*		This function handles the MR_SET_SEND_BOARD Request and Reply messages. The 
*		request switches the MDP that is sending messages to VNMS. This is done by 
*		changing the Sending MDP board in the Routing Table.
*
*		The reply indicates that ReceiveMonitor on another board has completed 
*		changing its MRCBs for the new MDP. Once all the boards have responded then a
*		reply is sent to IO Services to indicate the switchover is complete.
*
***************************************************************************************/

MR_STATUS Message_Routing_Interface::ProcessSendBoardUpdated (UINT_8 BoardId) 
{
	PMR_GLOBALS		pGlobals = m_pMRIGlobal;
	UINT_8			ActiveBoardCount = 0;
	bool			BoardFound = FALSE;
	STATUS			Status;
	Status.errorcode = MR_STATUS_SUCCESS;

	try 
	{

		MRI_BIN_TRACE1(MriInform, 50903, BoardId,  
				"ProcessSendBoardUpdated Msg Received, SourceSlotId=%x", 0);

		// Get the globals pointer and verify it
		if (!pGlobals)
		{
			MRI_BIN_TRACE1(MriSevere, 50528, pGlobals, 
				"Globals is null when processing Send Board Update, Globals=%x", 0);
		}
		VerifySignatureWithThrow 
			((PSIG_VERIFY)pGlobals, CMM_SIGNATURE, (USHORT)__LINE__);


		ActiveBoardCount = pGlobals->ActiveBoardCount;

        /*EoMDP
        // If board count is zero, iterate through IBCM targets and update the board count.
        if ( ActiveBoardCount == BOARD_COUNT_ZERO )
        {
            MRI_BIN_TRACE1( MriWarning, 50903, 0,
                "Mri: ActiveBoardCount is zero. Iterate through IBCM targets", 0 );

            // Get list of targets.
            IBCM_Targets Targets;

#ifdef _D_DESKTOP
#else
            m_Comm.GetTargets( Targets );
#endif
            // Lock the GLOBALS MMF down before calling the function.
            VerifySignatureWithThrow( ( PSIG_VERIFY )m_hOpen, OPEN_SIGNATURE, (USHORT)__LINE__ );

            HANDLE hGlobalMutex = m_hOpen->hGlobMutex;
            if ( !GetLock( hGlobalMutex, MR_TIME_TO_WAIT ) )
            {
                MRI_BIN_TRACE( MriSevere, GetLastError() );
                Status.errorcode = MR_STATUS_MUTEX_FAILURE;
            }

            for ( IBCM_Targets::iterator iTarget = Targets.begin();
                iTarget != Targets.end();
                iTarget++ )
            {
                // Track all active MDPs and SCs.
                if ( ( iTarget->IsMDP() ) || ( iTarget->IsSystemController() ) )
                {
                    pGlobals->ActiveBoards[ ActiveBoardCount ].SlotId = iTarget->GetDeviceID();
                    pGlobals->ActiveBoards[ ActiveBoardCount ].MDP = iTarget->IsMDP();
                    pGlobals->ActiveBoards[ ActiveBoardCount ].SwitchOverComplete = FALSE;
                    ActiveBoardCount++;
                }
            }

            // If board count is zero here, there could be problems with IBCM targets.
            if ( ActiveBoardCount == BOARD_COUNT_ZERO )
            {
                MRI_BIN_TRACE1( MriWarning, 50903, 0,
                    "Mri: ActiveBoardCount zero after iterating through IBCM targets", 0 );
            }

            pGlobals->ActiveBoardCount = ActiveBoardCount;

            ReleaseLock( hGlobalMutex );
        } */

		// Ensure that a Switchover is in progress
		if ((!pGlobals->MdpSwitchOver) || (ActiveBoardCount == 0))
		{
            MRI_BIN_TRACE2( MriSevere, 50903, ActiveBoardCount, pGlobals->MdpSwitchOver,
                "ProcessSendBoardUpdated Failed Due to Not In Progress, BoardCount=%x, MdpSwitchOver=%x", 0 );
			throw MR_STATUS_FAILURE;
		}

		// * Temporary Kludge until PCI Drivers can recognize the death of an MDP
		// If the reply is from this board then make message from MDP
		if (BoardId == DEAD_MDP)
		{
			// Mark the MDP as having responded
			for (int i=0; i < ActiveBoardCount; i++) 
			{
				if (pGlobals->ActiveBoards[i].MDP)
				{
					BoardFound = TRUE;
					pGlobals->ActiveBoards[i].SwitchOverComplete = TRUE;
					MRI_BIN_TRACE2(MriInform, 50903, pGlobals->ActiveBoards[i].SlotId, 
						ActiveBoardCount, 
						"Dead MDP %x has {responded} to SetSendBoard, ActiveBoards=%x", 0);
				break;
				}
			}
		}
		else
		{
			// Go through the normal path
			
			// Mark this board as having responded
			for (int i=0; i < ActiveBoardCount; i++) 
			{
				if (pGlobals->ActiveBoards[i].SlotId == BoardId)
				{
					BoardFound = TRUE;
					pGlobals->ActiveBoards[i].SwitchOverComplete = TRUE;
					MRI_BIN_TRACE2(MriInform, 50903, BoardId, ActiveBoardCount, 
						"Board %x has responded to SetSendBoard, ActiveBoards=%x", 0);
					break;
				}
			}
		}

		// If this board wasn't found this CMM is in trouble; expect sending problems
		if (!BoardFound)
		{
			MRI_BIN_TRACE2(MriSevere, 50903, ActiveBoardCount, BoardId,
				"ProcessSendBoardUpdated Failed Due to Board Not Active, BoardCount=%x, Responding Board=%x", 0);
			throw MR_STATUS_FAILURE;
		}


		// Determine if all the boards have responded.  
		BOOLEAN		AllBoardsSwitchedOver = TRUE;
		for (int x=0; x < ActiveBoardCount; x++) 
		{
			if (!pGlobals->ActiveBoards[x].SwitchOverComplete)
			{
				AllBoardsSwitchedOver = FALSE;
				break;
			}
		}

		// If all boards have switched reset array elements and return a reply. 
		if (AllBoardsSwitchedOver)
		{
			MRI_BIN_TRACE1(MriInform, 50903, ActiveBoardCount, 
				"SwitchComplete Notification Sent to IOServices, BoardCount=%x", 0);
			MR_MESSAGE		*SetSendAckReply;
			pGlobals->MdpSwitchOver = FALSE;
			pGlobals->ActiveBoardCount = 0;
			BuildSetSendReply (&SetSendAckReply, pGlobals);
			SendMRIMsg (SetSendAckReply, FALSE);
		}

	} // try
		

	
	EM_ELLIPSES_CATCH
	(
		MRI_BIN_TRACE1(MriSevere, 50903, GetLastError(),  
			"ProcessSendBoardUpdated Failed, Status=%x", 11300);
		return MR_STATUS_UNKNOWN_EXCEPTION_THROWN;
	)

	return Status.errorcode; 

} // ProcessSendBoardUpdated




/*****************************************************************************************
*	Name:
*		SendInterBoardNotification
*
*	Description:
*		This function builds a notification message from MRI on one board to 
*		MRI running on a remote board(s). This notification can be sent to a single board 
*		or all other boards.
*
******************************************************************************************/


MR_STATUS Message_Routing_Interface::SendInterBoardNotification (UINT_8 PortId,
																 UINT_8 RoutingId,
																 UINT_8 MsgType,
																 UINT_8 DestSlot) {
	
	MR_STATUS			status = MR_STATUS_SUCCESS;
	PMR_MESSAGE			pMsg;

    MRI_BIN_TRACE4 (MriVerbose, 50556, RoutingId, PortId, MsgType, DestSlot,
				"MRI: SendInterBoard, RouteId=%x,PortId=%x,MsgType=%x,DestSlot=%x", 0);

	typedef struct msg_op {
		BYTE	opcode;
		BYTE	oplen[3];
		int		reserved1;
		int		reserved2;
	} MSG_OP;
	
	typedef struct attachMsg {
		MR_MESSAGE	MD;
		MSG_OP		OP;
	} ATTACHMSG;

	MSG_OP	*pOP;

	try {
#ifdef _D_DESKTOP
		return status;
#endif
		int	oplen = sizeof(MSG_OP);

		int	NotificationCode;
		switch (MsgType) {

		case MR_ATTACH_NOTIFICATION:
			NotificationCode = OP_ATTACH_PORT;
			break;

		case MR_BOARD_FAILURE:
			NotificationCode = OP_MDP_FAILURE;
			break;

		case MR_DETACH_NOTIFICATION:
			NotificationCode = OP_DETACH_PORT;
			break;
		
		case MR_GREETINGS_REQUEST:
			NotificationCode = OP_GREETINGS_REQUEST;
			break;

		case MR_GREETINGS_RESPONSE:
			NotificationCode = OP_GREETINGS_RESPONSE;
			break;

		case MR_INITIALIZATION_COMPLETE:
			NotificationCode = OP_INIT_COMPLETE;
			break;

		case MR_ATTACHED_PORTS_NOTIFICATION:
			NotificationCode = OP_AVAILABLE_PORTS;
			break;

		case MR_SEND_BOARD_UPDATED:
			NotificationCode = OP_RC_SET_SEND_BD_ACK;
			break;

		default:
			NotificationCode = NULL;
			MRI_BIN_TRACE4 (MriWarning, 50558, RoutingId, PortId, MsgType, DestSlot,
				"MRI: SendInterBoardNotification Unknown MsgType"
				"RouteId=%x,PortId=%x,MsgType=%x,DestSlot=%x", 0);
			break;

		} // switch


		// allocate a buffer
		if (status = AllocateBuf((sizeof(MSG_OP) + sizeof(MR_MESSAGE)), &pMsg))
		{
			throw (MR_STATUS)status;
		}

		UINT_8 SlotId;
		GetMySlot(&SlotId);
		
		// build a message
		pMsg->MsgLink = 0;
		pMsg->MsgType = MsgType;
		pMsg->DataLen = sizeof(MSG_OP); 
		pMsg->lpvData = (PBYTE)pMsg + sizeof(MR_MESSAGE);
		pMsg->Pid = GetCurrentProcessId();
		pMsg->DestSlotId = DestSlot;
		pMsg->SrcSlotId = SlotId;
		pMsg->OpCode = NotificationCode;
		pMsg->OpClass = PortId;
		pMsg->Vnmsid = 0;
		pMsg->Rclass = RoutingId;
		pMsg->SendElapsed = 0;
		pMsg->SendResult = 0;
		pMsg->hEqueue = 0;
		pMsg->lpvCallback = NULL;
		pMsg->Signature = MD_SIGNATURE;
		pOP = (MSG_OP *)pMsg->lpvData;
		pOP->opcode = NotificationCode;
		memcpy (pOP->oplen, (char *)&oplen, 3);
		pOP->reserved1 = 0;
		pOP->reserved2 = 0;

		ProcessInterBoardMsg (pMsg);

#ifdef PCIEMULATOR
		if ( (status = FreeBuf(pMsg)) )  {
			throw MR_STATUS(status);
		}
#endif
	} //try

	catch (STATUS status) {
		MRI_BIN_TRACE4 (MriSevere, 50559, RoutingId, PortId, MsgType, status.errorcode,
				"MRI: SendInterBoardNotification Failed"
				"RouteId=%x,PortId=%x,MsgType=%x,Status=%x", 0);
		if (status.errorcode == MR_STATUS_SUCCESS)
		{
			status.errorcode = MR_STATUS_FAILURE;
		}
    }  

	MRI_BIN_TRACE4 (MriVerbose, 50560, RoutingId, PortId, MsgType, DestSlot,
				"MRI: SendInterBoardNotification Successful"
				"RouteId=%x,PortId=%x,MsgType=%x,DestSlot=%x", 0);

	return status;

} // SendInterBoardNotification


/*****************************************************************************************
*	Name:
*		SendInterBoardNotification
*
*	Description:
*		This function builds a notification message from MRI on one board to 
*		MRI running on a remote board(s). This notification can be sent to a single board 
*		or all other boards.
*	
*		Unlike the other SendInterBoardNotification, this functions handles data
*
******************************************************************************************/


MR_STATUS Message_Routing_Interface::SendInterBoardNotification (
															UINT_8 MsgType,
															UINT_8 DestSlot,
															UINT_32	DataLen,
															HANDLE	DataPtr) {

	MRI_BIN_TRACE3 (MriVerbose, 50556, MsgType, DestSlot, DataLen,
				"MRI: SendInterBoard, MsgType=%x,DestSlot=%x,DataLen=%x", 0);

	MR_STATUS			status = MR_STATUS_SUCCESS;
	PMR_MESSAGE			pMsg;

	try {
#ifdef _D_DESKTOP
		return status;
#endif

		int	NotificationCode;
		switch (MsgType) {

		case MR_ATTACHED_PORTS_NOTIFICATION:
			NotificationCode = OP_AVAILABLE_PORTS;
			break;

		default:
			NotificationCode = NULL;
			MRI_BIN_TRACE2 (MriWarning, 50561, MsgType, DestSlot,
				"MRI: SendInterBoardNotification Unknown MsgType=%x,DestSlot=%x", 0);
			break;

		} // switch


		// allocate a buffer
		status = AllocateBuf((DataLen + sizeof(MR_MESSAGE)), &pMsg);
		if (status)  	
		{
			throw (MR_STATUS)status;
		}

		UINT_8 SlotId;
		GetMySlot(&SlotId);
		
		// build a message
		pMsg->MsgLink = 0;
		pMsg->MsgType = MsgType;
		pMsg->DataLen = DataLen; 
		pMsg->lpvData = (PBYTE)pMsg + sizeof(MR_MESSAGE);
		pMsg->Pid = GetCurrentProcessId();
		pMsg->DestSlotId = DestSlot;
		pMsg->SrcSlotId = SlotId;
		pMsg->OpCode = NotificationCode;
		pMsg->OpClass = NULL;
		pMsg->Vnmsid = 0;
		pMsg->Rclass = NULL;
		pMsg->SendElapsed = 0;
		pMsg->SendResult = 0;
		pMsg->hEqueue = 0;
		pMsg->lpvCallback = NULL;
		memcpy (pMsg->lpvData, DataPtr, DataLen);

		ProcessInterBoardMsg (pMsg);

#ifdef PCIEMULATOR
		if ( (status = FreeBuf(pMsg)) )  
		{
			throw MR_STATUS(status);
		}
#endif
	} //try

	catch (STATUS status) 
	{
		MRI_BIN_TRACE3 (MriSevere, 50562, MsgType, DestSlot, status.errorcode,
				"MRI: SendInterBoardNotification Failed"
				"MsgType=%x, DestSlot=%x, Status=%x", 0);
		if (status.errorcode == MR_STATUS_SUCCESS)
		{
			status.errorcode = MR_STATUS_FAILURE;
		}
    }  

	MRI_BIN_TRACE3 (MriVerbose, 50562, MsgType, DestSlot, status,
				"MRI: SendInterBoardNotification, MsgType=%x, DestSlot=%x, Status=%x", 0);
	return status;

} // SendInterBoardNotification

/*
*****************************************************************************************
*
*	Name:	
*		InitGlobals
*
*	Description:
*		Initialize the MriGlobals member data.
*
*
*****************************************************************************************
*/
void MriSharedInternals::InitGlobals (PMR_GLOBALS pGlobal)
{
	unsigned int	hostid;
	
	for (hostid=0; hostid<MAXHOSTS; hostid++)  {
		pGlobal->VNMSpaths[hostid].slotid = NULL;
		pGlobal->VNMSpaths[hostid].rclass = VNMS;
	}
	pGlobal->cmmID = NULL;
	pGlobal->masterHost = NULL;
	pGlobal->VNMSpath = NULL;
    pGlobal->is_HMSPresent = false;
    pGlobal->is_HMSInstalled = false;
    pGlobal->is_CMM_NonShared = false;
    pGlobal->CMM_NonShared_Host = 0;
    pGlobal->is_SCA = false;

    // Is this the SCA?

    TCHAR           ComputerName[ MAX_PATH ] = L"";
    TCHAR*          pName = NULL;

    try  {

        //EoMDP
        wcscpy( ComputerName, GetHostName().c_str() );
        pName=wcschr( ComputerName, DELIMITER );
        if( pName != NULL)
        {
            pName++;
        }
        else
        {
            MRI_BIN_TRACE1 ( MriSevere, 50551,0, 
                    "MriSharedInternals::InitGlobals Failed to get the board name ", 0 );
            return;

        }
        pName=wcschr( pName, DELIMITER );
        if( pName != NULL)
        {
            pName++;
        }
        else
        {
            MRI_BIN_TRACE1 ( MriSevere, 50551,0, 
                    "MriSharedInternals::InitGlobals Failed to get the board name pass 2 ", 0 );
            return;
        }
        pGlobal->is_SCA = ( ( wcscmp( pName, SCA ) == ZERO ) ||
                            ( wcscmp( pName, sca ) == ZERO ) );
    }
    EM_ELLIPSES_CATCH
    (
            MRI_BIN_TRACE1 ( MriSevere, 50551,0, 
                    "MriSharedInternals::InitGlobals Error while getting the board name 3", 0 );
    )


}
