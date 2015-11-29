#ifndef __FSL_TYPES__
#define __FSL_TYPES__
//***************************************************************************
// Copyright (C) EMC Corporation 2010 - 2011
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

/**********************************************************************
 *
 *  File Name: 
 *          FslDataTypes.h
 *
 *  Description:
 *          This file contains FSL specific data types.
 *
 *  Revision History:
 *          04/14/2011 Prahlad Purohit  -- Created
 *          
 **********************************************************************/


typedef enum _FSL_STATUS_CODE{
        FSL_SUCCESS = 0,
        FSL_ALREADY_EXPORTED,
        FSL_INSUFFICIENT_RESOURCES,
        FSL_TOO_MANY_EXPORTS,
        FSL_DEVICE_NOT_FOUND,
        FSL_LUN_CONFLICT,
        FSL_LUN_DOES_NOT_EXIST,
        FSL_LUN_OFFLINE,        
        FSL_NOT_EXPORTED,
        FSL_INVALID_PARAMETER,
        FSL_INTERNAL_ERROR,
        FSL_GENERIC_ERROR,
        FSL_TIME_OUT,
        FSL_GENERIC_WINAPI_ERROR, /* Return code to denote Windows API failed for some reason*/
        FSL_UNSUPPORTED_FUNCTIONALITY,
}FSL_STATUS_CODE,*PFSL_STATUS_CODE;


//
// This structure contains FSL performance counters.
//
typedef struct _FSLBUS_PERFOMANCE_DATA
{
    ULONG64     ReadCount;
    ULONG64     WriteCount;
    ULONG64     BlocksRead;
    ULONG64     BlocksWritten;

} FSLBUS_PERFORMANCE_DATA, *PFSLBUS_PERFORMANCE_DATA;

#endif //__FSL_TYPES__