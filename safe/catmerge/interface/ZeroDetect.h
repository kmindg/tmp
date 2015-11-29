//**********************************************************************
// FileHeader
//
//   File:          ZeroDetect.h
//
//   Description:   Definitions of types used in interface
//
//   Author(s):     Mike Haynes
//
//   Revision Log:
//      1.00   10-Sep-2009   MHaynes   Intial Creation 
//
//   Copyright (C) EMC Corporation, 2004 - 2005
//      All rights reserved.
//      Licensed material - Property of EMC Corporation.
//
//**********************************************************************

#ifndef __ZERO_DETECT_H__
#define __ZERO_DETECT_H__

#include "EmcPAL.h"

//++
// Name:
//    ZeroDetectExtentType
//
// Description:
//   Types of extents defined for Zero Detection
//
//--
typedef enum
{
    Hole = 0,		// The extent contains all zeroes
    Data,			// The extent contains data
    Undetermined	// The extent type is not yet known (first iteration)
}
ZERO_DETECT_EXTENT_TYPE, *PZERO_DETECT_EXTENT_TYPE;

//++
// Name:
//    ZDInstance
//
// Description:
//   This structure describes an IO to be scanned for zeroes.  The client must allocate
//   and initialize one of these structures for each IO it wants the ZD library to scan,
//   and it must maintain the structure unmodified until the last ZD iteration is complete.
//   Typically the client will fill it with information from an IRP, with the assistance
//   of an initialization function in the ZD library.  After the first iteration,
//  the ZD library uses this structure to remember where to start on the next iteration.
//
//--
typedef struct _ZDINSTANCE
{
  ULONG32                   Quantum;        //Size of Quantum in Bytes
  ULONG64                   StartLBA;       //First Block of next extent to examine
  ULONG32                   LengthInBlocks; //Remaining length beginning at StartLBA
  PCHAR                     pStartBuffer;   //First byte of next extent to examine
  ZERO_DETECT_EXTENT_TYPE   ExtentType;     //Type of extent StartLBA/StartBuffer point to
}ZDINSTANCE, *PZDINSTANCE;

//++
// Name:
//    ZDFindNextExtentAsynchFunction
//
// Description:
//   Prototype for ZDFindNextExtentAsynch() callback function
//--
typedef VOID (*ZD_FIND_NEXT_EXTENT_ASYNCH_FUNCTION)
    (PVOID					Context,
	 PCHAR              	pBuffer,
	 ULONG64             	StartLBA,
	 ULONG32             	LengthInBlocks,
	 ZERO_DETECT_EXTENT_TYPE Type,
	 BOOLEAN               	Done);

//++
// Name:
//    ZDINSTANCE_ASYNCH
//
// Description:
//   Parameter for ZDFindNextExtentAsynch()
//--
typedef struct _ZDINSTANCE_ASYNCH
{
  ZDINSTANCE       		  				ZDI;
  ZD_FIND_NEXT_EXTENT_ASYNCH_FUNCTION	Callback;
  PVOID		       		  				ClientContext;
}ZDINSTANCE_ASYNCH, *PZDINSTANCE_ASYNCH;



// Prototypes of Interface functions

VOID
ZDFindNextExtentAsynch( 
          IN   PVOID  	pContext
		  );

EMCPAL_STATUS 
ZDInitializeZDInstance (
         IN  PZDINSTANCE            pZDI,
         IN  ULONG32				Quantum,
		 IN  ULONG64                StartLBA,
		 IN  ULONG32                LengthInBlocks,
         IN  PCHAR		            pStartBuffer
		 );
	
EMCPAL_STATUS
ZDFindNextExtent ( 
          IN   PZDINSTANCE        	  	pZDI,
		  OUT  PCHAR*					ppBuffer,
		  OUT  PULONG64             	pStartLBA,
		  OUT  PULONG32             	pLengthInBlocks,
		  OUT  PZERO_DETECT_EXTENT_TYPE pType,
		  OUT  PBOOLEAN               	pDone
		  );

#endif // __ZERO_DETECT_H__
