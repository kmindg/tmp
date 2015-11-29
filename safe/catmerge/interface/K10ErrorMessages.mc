;//******************************************************************************
;//      COMPANY CONFIDENTIAL:
;//      Copyright (C) EMC Corporation, 2001
;//      All rights reserved.
;//      Licensed material - Property of EMC Corporation.
;//******************************************************************************
;
;//**********************************************************************
;//.++
;// FILE NAME:
;//      K10ErrorMessages.mc (Common Layered Driver - CLD Messages)
;//
;// DESCRIPTION:
;//      This file defines Status Codes and messages that are used by all the 
;//      layered drivers. Each Status Code has two forms, an
;//      internal status and an admin status:
;//      CLD_xxx and CLD_ADMIN_xxx.
;//
;// CONTENTS:
;//      
;//
;//
;// REVISION HISTORY:
;//      03-14-2005  prashant  Created
;//.--                                                                    
;//**********************************************************************
MessageIdTypedef= EMCPAL_STATUS

FacilityNames   = ( System         = 0x0
                    RpcRuntime     = 0x2:FACILITY_RPC_RUNTIME
                    RpcStubs       = 0x3:FACILITY_RPC_STUBS
                    Io             = 0x4:FACILITY_IO_ERROR_CODE
                    CommonLayered  = 0x15A:FACILITY_COMMON_LAYERED_DRIVERS 
                  )

SeverityNames   = ( Success       = 0x0 : STATUS_SEVERITY_SUCCESS
                    Informational = 0x1 : STATUS_SEVERITY_INFORMATIONAL
                    Warning       = 0x2 : STATUS_SEVERITY_WARNING
                    Error         = 0x3 : STATUS_SEVERITY_ERROR
                  )
                  
MessageId    = 0x0000
Severity     = Success
Facility     = CommonLayered
SymbolicName = CLD_STATUS_SUCCESS
Language     = English
All is well.
.
;#define K10_CLD_STATUS_SUCCESS            ( MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR( CLD_STATUS_SUCCESS ) ) 
;

;
;//++
;//
;//  Informational Status Codes.
;//
;//--
;


;
;//++
;//
;//  Warning Status Codes.
;//
;//--
;


;
;//++
;//
;//  Error Status Codes.
;//
;//--
;
;//------------------------------------------------------------------------------
;// Introduced In: Release 24 (Mars)
;//
;// Usage              :           All layered drivers, Navi 
;//
;// Severity           :           Error
;//
;// Symptom of Problem : 
;//        Used in conjunction with the new feature limits management feature. Used
;//        when any layered driver object creation fails while there is a pending NDU commit
;//        operation (that will allow this object to be created after a "commit due to an
;//        increase in the limit associated with that parameter). 
;//        This string requires two parameters: 
;//        %2 - Name of the layered driver (Eg: "SnapCopy") - Type string
;//        %3 - Parameter within the layered driver on which the limit was reached 
;//             (Eg: "Reserved LUNs) - Type string
;//        %4 - Current maximum value of parameter - Type long
;//        %5 - Maximum value of parameter after NDU commit - Type long
;//
;//
MessageId    = 0x8000           
Severity     = Error
Facility     = CommonLayered
SymbolicName = CLD_STATUS_PARAM_LIMIT_REACHED_BEFORE_COMMIT
Language     = English
%2: Maximum number of %3, %4 already reached on this array. A new maximum of %5 is available after committing 
the FLARE-Operating-Environment. Please retry this operation after the commit.
.
;#define K10_CLD_STATUS_PARAM_LIMIT_REACHED_BEFORE_COMMIT ( MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR( CLD_STATUS_PARAM_LIMIT_REACHED_BEFORE_COMMIT ) ) 
;




