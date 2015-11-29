/***********************************************************************
*.++
*  COMPANY CONFIDENTIAL:
*     Copyright (C) EMC Corporation, 2001
*     All rights reserved.
*     Licensed material - Property of EMC Corporation.
*.--                                                                    
***********************************************************************/

/***********************************************************************
*.++
* FILE NAME:
*      UceProto.h
*
* DESCRIPTION:
*      Universal Copy Engine Function Prototypes 
*
* REVISION HISTORY:
*      23-Apr-02  majmera   LargeLu Support
*      10-Jan-02  pmisra    Created.
*.--                                                                    
************************************************************************/
#ifndef _UCEPROTO_H_
#define _UCEPROTO_H_

#include "k10ntddk.h"
#include "EmcPAL_DriverShell.h"
#include "UceExport.h"


/***********************************************************************
*++
*
*  Interface Functions Prototypes for the Universal Copy Engine
*
*--
***********************************************************************/


EMCPAL_STATUS
UceInitialize ( IN   PEMCPAL_NATIVE_DRIVER_OBJECT   pDriverObject );


void_type 
UceQuiesce( IN PEMCPAL_DEVICE_OBJECT pDeviceObject );


void_type
UceUnQuiesce(  void_type ); 


void_type 
UceIOReturned( IN PUCE_BUF_DESCRIPTOR        pBufferInfo );


void_type 
UceIgnition(  IN ULONG                       StreamID,
              IN PUCE_PRIMITIVE_COPY_CMD     pPrimitiveCmd  );

EMCPAL_STATUS  
UcePause( IN COMMAND_ID                      CommandID );
                      

EMCPAL_STATUS  
UceResume( IN COMMAND_ID                     CommandID );



EMCPAL_STATUS  
UceThrottle( IN COMMAND_ID                   CommandID, 
             IN ULONG                        ThrottleValue );


EMCPAL_STATUS
UceStatus( IN COMMAND_ID                     Command_ID, 
              OUT EMCPAL_PLARGE_INTEGER              pTransferCount );


EMCPAL_STATUS  
UceAbort( IN COMMAND_ID                      CommandID );

EMCPAL_STATUS
UceStartCopy( IN  ULONG                      StreamID,
			  IN  PUCE_PRIMITIVE_COPY_CMD    pPrimitiveCmd,
			  IN  PPUCE_BUF_DESCRIPTOR       pBufferList,
			  IN  ULONG                      BufCount, 
			  IN  ULONG                      BufSize );

EMCPAL_STATUS
UceDeltaMapSetBit( IN  PDMH_DELTA_MAP        pDeltaMap,
				   IN  ULONG                 StreamID,
				   IN  ULONG                 BitNumber );

EMCPAL_STATUS
UceNotifySourceChange( IN  PDMH_DELTA_MAP    pDeltaMap,
				       IN  ULONG             StreamID,
				       IN  ULONG             BitNumber );

BOOLEAN
UceCheckCollision( IN  ULONG                 StreamID,
				   IN  ULONG                 ChunkNumber );



#endif  //_UCEPROTO_H_



