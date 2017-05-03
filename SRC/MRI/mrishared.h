#ifndef MRI_SHARED_H
#define MRI_SHARED_H

#include <stdio.h>
#include <malloc.h>
#include <process.h>
#include <atlbase.h>
#include <mri\mriapi.h>
#include <NLog\naplog.h>

/*EoMDP start*/
#define  DELIMITER           ( ( wint_t )'-' )
#define  ZERO                0
#define  SCA                 _T( "SCA" )
#define  sca                 _T( "sca" )
#define  SCB                 _T( "SCB" )
#define  scb                 _T( "scb" )
/*EoMDP end*/
// Declare a specific Nlog object for this thread.
extern NLogBase * MriLogObj;

// Log and Trace defines
#define MRI_NLOG MriLogObj->NAPLOG

// Used for exception handling
typedef struct _STATUS  {
	MR_STATUS	errorcode;
} STATUS;



class MriAttachHandler
{

public:
	MriAttachHandler(Message_Routing_Interface *pMri = NULL);
	~MriAttachHandler();

	MR_STATUS		AttachThisPort 
							(UINT_8 RouteId, UINT_8 PortId, UINT_8 BoardId, 
							PMR_OPEN_HANDLE hOpen, MR_HANDLE *PortHandle);

	MR_STATUS		CreateMMF (WCHAR FileName[200], int FileSize, 
								HANDLE *hFile, HANDLE *hMappedFile);


	MR_STATUS		CreateRcvQ	(PMR_MRCB	pMRCB, UINT_8 RouteId, UINT_8	PortId);

	MR_STATUS		CreatePortVector (UINT_8 RouteId, PMR_ROUTING_STRUCT pRoutingStruct, 
									 HANDLE	*PortStruct);

	MR_STATUS		CreateMRCB  (UINT_8	RouteId, UINT_8	PortId, UINT_8 SlotId,
								PMR_PORT_STRUCT	hPort, PMR_OPEN_HANDLE	hOpen, 
								PMR_MRCB	*MRCB);

	MR_STATUS		AssignRcvQToMRCB (PMR_MRCB	OldMRCB, PMR_MRCB	NewMRCB);

	MR_STATUS		ProcessApplicationFailure (PMR_MRCB	StaleMRCB, UINT_8	RouteId,
											UINT_8 PortId, UINT_8 BoardId, 
											PMR_OPEN_HANDLE	hOpen, MR_HANDLE *PortHandle);

	void			LinkMrcbToList (PMR_OPEN_HANDLE	hOpen, PMR_MRCB	hNewMRCB);

	MR_STATUS		MapVNMSRouteId (UINT_8 BoardId, UINT_8 *RouteId);

private: // methods


protected: // data
	Message_Routing_Interface	*m_pMri;
	
};



class MriDetachHandler
{

public:
	MriDetachHandler(Message_Routing_Interface *pMri = NULL);
	~MriDetachHandler();

	void			DeleteRcvQueueStructures (PMR_MRCB	pMRCB);

	void			DeleteMRCB (PMR_MRCB pMRCB, 
								PMR_PORT_STRUCT	pPortStruct);

	void			DestroyPort (PMR_MRCB	pMRCB, 
								PMR_OPEN_HANDLE	hOpen);

private: // methods


protected: // data
	Message_Routing_Interface	*m_pMri;
	
};


#ifdef BIN_TRACE


// This define is used to trace only file name and status
#define MRI_BIN_TRACE(DebugLevel,Status) { \
	if (!DebugLevel.IsDisabled()){\
		ATLTRACE("Mri: Problem in file %s, Line %d, Status=%x\n",\
				__FILE__, __LINE__, Status); \
	    AS_FillLogBuffer(&MriInfo, 2, __FILE__, Status, __LINE__); \
	    AS_TRACEX(50500, &DebugLevel, MODULE_MRI_API, &MriInfo,\
                 "Mri: Problem in file=%s, Status=%x, Line=%d\n");\
		if (MriLogging)\
			MRI_NLOG(11200, &DebugLevel, MODULE_MRI_API, &MriInfo, "MRI: Check TraceFile");} \
}

#define MRI_BIN_TRACE1(DebugLevel, DebugCode, Param1, MriString, ReasonCode) { \
	if (!DebugLevel.IsDisabled()){\
		ATLTRACE(MriString, Param1);\
		ATLTRACE("\n");\
        AS_FillLogBuffer(&MriInfo, 1, Param1); \
        AS_TRACEX(DebugCode, &DebugLevel, MODULE_MRI_API, &MriInfo, MriString);\
		if ((ReasonCode) && (MriLogging))\
		   MRI_NLOG(ReasonCode, &DebugLevel, MODULE_MRI_API, &MriInfo, MriString); \
	}\
}

#define MRI_BIN_TRACE2(DebugLevel, DebugCode, Param1, Param2, MriString, ReasonCode) { \
	if (!DebugLevel.IsDisabled()){\
		ATLTRACE(MriString, Param1, Param2);\
		ATLTRACE("\n");\
	    AS_FillLogBuffer(&MriInfo, 2, Param1, Param2); \
		AS_TRACEX(DebugCode, &DebugLevel, MODULE_MRI_API, &MriInfo, MriString);\
		if ((ReasonCode) && (MriLogging))\
		   MRI_NLOG(ReasonCode, &DebugLevel, MODULE_MRI_API, &MriInfo, MriString); \
	}\
}

#define MRI_BIN_TRACE3(DebugLevel, DebugCode, Param1, Param2, Param3, MriString, ReasonCode) { \
	if (!DebugLevel.IsDisabled()){\
		ATLTRACE(MriString, Param1, Param2, Param3);\
		ATLTRACE("\n");\
	    AS_FillLogBuffer(&MriInfo, 3, Param1, Param2, Param3); \
	    AS_TRACEX(DebugCode, &DebugLevel, MODULE_MRI_API, &MriInfo, MriString); \
		if ((ReasonCode) && (MriLogging))\
		   MRI_NLOG(ReasonCode, &DebugLevel, MODULE_MRI_API, &MriInfo, MriString); \
	}\
}

#define MRI_BIN_TRACE4(DebugLevel, DebugCode, Param1, Param2, Param3, Param4, MriString, ReasonCode) { \
	if (!DebugLevel.IsDisabled()){\
		ATLTRACE(MriString, Param1, Param2, Param3, Param4);\
		ATLTRACE("\n");\
	    AS_FillLogBuffer(&MriInfo, 4, Param1, Param2, Param3, Param4);\
		AS_TRACEX(DebugCode, &DebugLevel, MODULE_MRI_API, &MriInfo, MriString);\
		if ((ReasonCode) && (MriLogging))\
		   MRI_NLOG(ReasonCode, &DebugLevel, MODULE_MRI_API, &MriInfo, MriString); \
	}\
}


#else



// This define is used to trace only line number and status
#define MRI_BIN_TRACE(DebugLevel,Status) { \
	if (!DebugLevel.IsDisabled())\
{\
		char MriString[ 500 ]; \
		sprintf(MriString, "MRI: Problem in file %s, Line %d, Status=%x",\
				__FILE__, __LINE__, Status); \
		ATLTRACE(MriString); \
		ATLTRACE("\n"); \
	    AS_FillLogBuffer(&MriInfo, 3, __FILE__, __LINE__, Status); \
        wchar_t  szPrintline[ 500 ], szWString[ 200 ]; \
        swprintf (szWString, _T ( "%S" ), MriString);\
        swprintf(szPrintline, szWString,\
                        MriInfo.Data3,\
                        MriInfo.Data4,\
                        MriInfo.Data5);\
	    TR_TRACE(DebugLevel, 0, szPrintline); \
		if (MriLogging)\
			MRI_NLOG(11300, &DebugLevel, MODULE_MRI_API, &MriInfo, MriString);} \
}

#define MRI_BIN_TRACE1(DebugLevel, DebugCode, Param1, MriString, ReasonCode) \
{ \
	if (!DebugLevel.IsDisabled()){\
		ATLTRACE(MriString, Param1);\
		ATLTRACE("\n");\
	    AS_FillLogBuffer(&MriInfo, 1, Param1); \
        wchar_t  szPrintline[ 500 ], szWString[ 200 ]; \
        swprintf (szWString, _T ( "%S" ), MriString);\
        swprintf(szPrintline, szWString,\
                        MriInfo.Data3,\
                        MriInfo.Data4,\
                        MriInfo.Data5);\
	    TR_TRACE(DebugLevel, 0, szPrintline); \
		if ((ReasonCode) && (MriLogging))\
		   MRI_NLOG(ReasonCode, &DebugLevel, MODULE_MRI_API, &MriInfo, MriString); \
	}\
}

#define MRI_BIN_TRACE2(DebugLevel, DebugCode, Param1, Param2, MriString, ReasonCode) \
{ \
	if (!DebugLevel.IsDisabled()){\
		ATLTRACE(MriString, Param1, Param2);\
		ATLTRACE("\n");\
	    AS_FillLogBuffer(&MriInfo, 2, Param1, Param2); \
        wchar_t  szPrintline[ 500 ], szWString[ 200 ]; \
        swprintf (szWString, _T ( "%S" ), MriString);\
        swprintf(szPrintline, szWString,\
                        MriInfo.Data3,\
                        MriInfo.Data4);\
	    TR_TRACE(DebugLevel, 0, szPrintline); \
		if ((ReasonCode) && (MriLogging))\
		   MRI_NLOG(ReasonCode, &DebugLevel, MODULE_MRI_API, &MriInfo, MriString); \
	}\
}

#define MRI_BIN_TRACE3(DebugLevel, DebugCode, Param1, Param2, Param3, MriString, ReasonCode)\
{ \
	if (!DebugLevel.IsDisabled()){\
		ATLTRACE(MriString, Param1, Param2, Param3);\
		ATLTRACE("\n");\
	    AS_FillLogBuffer(&MriInfo, 3, Param1, Param2, Param3); \
        wchar_t  szPrintline[ 500 ], szWString[ 200 ]; \
        swprintf (szWString, _T ( "%S" ), MriString);\
        swprintf(szPrintline, szWString,\
                        MriInfo.Data3,\
                        MriInfo.Data4,\
                        MriInfo.Data5);\
	    TR_TRACE(DebugLevel, 0, szPrintline); \
		if ((ReasonCode) && (MriLogging))\
		   MRI_NLOG(ReasonCode, &DebugLevel, MODULE_MRI_API, &MriInfo, MriString); \
	}\
}

#define MRI_BIN_TRACE4(DebugLevel, DebugCode, Param1, Param2, Param3, Param4, MriString, ReasonCode)\
{ \
	if (!DebugLevel.IsDisabled()){\
		ATLTRACE(MriString, Param1, Param2, Param3, Param4);\
		ATLTRACE("\n");\
	    AS_FillLogBuffer(&MriInfo, 4, Param1, Param2, Param3, Param4); \
        wchar_t  szPrintline[ 500 ], szWString[ 200 ]; \
        swprintf (szWString, _T ( "%S" ), MriString);\
        swprintf(szPrintline, szWString,\
                        MriInfo.Data3,\
                        MriInfo.Data4,\
                        MriInfo.Data5,\
                        MriInfo.Data6);\
	    TR_TRACE(DebugLevel, 0, szPrintline); \
		if ((ReasonCode) && (MriLogging))\
		   MRI_NLOG(ReasonCode, &DebugLevel, MODULE_MRI_API, &MriInfo, MriString); \
	}\
}


#endif  //TRACING DEFINES


#endif
