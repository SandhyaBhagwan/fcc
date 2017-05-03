#ifndef MRI_API_H
#define MRI_API_H

#include <atlbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <comdef.h>
#include <MRI/mri.h>
#include <BuffMgr/BufferMgr.h>
#include <MRI/opcode.h>
//#include <MRI/mrishared.h>


// libraries needed by MRI
#ifdef _DEBUG
//	pragma comment(lib, "IBCMD.lib")
//	pragma comment(lib, "TRB_BinTraceD.lib")
//	pragma comment(lib, "TR_TraceD.lib" )
	#pragma comment(lib, "CLD.lib" )
	#pragma comment(lib, "EM_EventLogD.lib" )
	#pragma comment(lib, "CL_TraceD.lib" )
//	pragma comment(lib, "BufferMgrD.lib")
	#pragma comment(lib, "PerfLibD.lib" )
	#pragma comment(lib, "NapLogD.lib" )
	#pragma	comment(lib, "GlobD.lib")
	#pragma	comment(lib, "ParamMgrD.lib")
	#pragma	comment(lib, "NL_NetworkLookupD.lib")
//	pragma	comment(lib, "FR_FailoverD.lib")
	#pragma	comment(lib, "THR_ThreadsD.lib")
#else
//	pragma comment(lib, "IBCM.lib")
//	pragma comment(lib, "TRB_BinTrace.lib")
	#pragma comment(lib, "TR_Trace.lib" )
	#pragma comment(lib, "CL.lib" )
	#pragma comment(lib, "EM_EventLog.lib" )
	#pragma comment(lib, "CL_Trace.lib" )
//	pragma comment(lib, "BufferMgr.lib")
	#pragma comment(lib, "PerfLib.lib" )
	#pragma comment(lib, "NapLog.lib" )
	#pragma	comment(lib, "Glob.lib")
	#pragma	comment(lib, "ParamMgr.lib")
	#pragma	comment(lib, "NL_NetworkLookup.lib")
//	pragma	comment(lib, "FR_Failover.lib")
	#pragma	comment(lib, "THR_Threads.lib")
#endif

#include <MRI/MriDefines.h>
#endif
