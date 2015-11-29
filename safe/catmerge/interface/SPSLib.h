#ifndef __SPS_LIB__
#define __SPS_LIB__

/**********************************************************************
 *      Company Confidential
 *          Copyright (c) EMC Corp, 2010
 *          All Rights Reserved
 *          Licensed material - Property of EMC Corp
 **********************************************************************/

/**********************************************************************
 *
 *  File Name: 
 *          SPSLib.h
 *
 *  Description:
 *          Function prototypes for SPS functions 
 *          exported as a statically linked library.
 *
 *  Revision History:
 *      Apr-05-2010     Joe Ash - Created
 *          
 **********************************************************************/

#include "generic_types.h"
#include "SPS_micro.h"

#ifdef __cplusplus
extern "C"
{
#endif
void SPSLibinitSPSAttr(void);

typedef struct _SPS_ATTR_REQUEST
{
    SPS_COMMAND             command;
    CHAR                    niceName[40];
    CHAR                    readCommand[15];
    ULONG                   readByteCount;
    BOOL                    readable;
    CHAR                    writeCommand[15];
    ULONG                   writeByteCount;
    BOOL                    writeable;
    BOOL                    valid;
} SPS_ATTR_REQUEST, *PSPS_ATTR_REQUEST;

extern SPS_ATTR_REQUEST SPSAttr[SPS_TOTAL_COMMAND];

#ifdef __cplusplus
}
#endif

#endif // __SPS_LIB__
