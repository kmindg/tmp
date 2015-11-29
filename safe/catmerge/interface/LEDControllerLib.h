#ifndef __LED_CONTROLLER_LIB__
#define __LED_CONTROLLER_LIB__

/**********************************************************************
 *      Company Confidential
 *          Copyright (c) EMC Corp, 2003
 *          All Rights Reserved
 *          Licensed material - Property of EMC Corp
 **********************************************************************/

/**********************************************************************
 *
 *  File Name: 
 *          LEDControllerLib.h
 *
 *  Description:
 *          Function prototypes for the LED Controller functions 
 *          exported as a statically linked library.
 *
 *  Revision History:
 *          09-Sept-17       Phil Leef   Created
 *          
 **********************************************************************/

#include "spid_types.h"
#include "PCA9552.h"

#ifdef __cplusplus
extern "C"
{
#endif

void LEDControllerLibInitLEDCtrlAttr(SPID_PLATFORM_INFO platformInfo);

typedef struct _LED_CTRL_ATTR_REQUEST
{
    PCA9552_REG_BLOCK       regBlock;
    char                    niceName[40];
    UCHAR                   readOffset;
    ULONG                   readByteCount;
    UCHAR                   writeOffset;
    ULONG                   writeByteCount;
    BOOL                    writeable;
    BOOL                    valid;
} LED_CTRL_ATTR_REQUEST, *PLED_CTRL_ATTR_REQUEST;

extern LED_CTRL_ATTR_REQUEST ledCtrlAttr[PCA9552_TOTAL_BLOCK];

#ifdef __cplusplus
}
#endif

#endif // __LED_CONTROLLER_LIB__
