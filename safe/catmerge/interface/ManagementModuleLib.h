#ifndef __MANAGEMENT_MODULE_LIB__
#define __MANAGEMENT_MODULE_LIB__

/**********************************************************************
 *      Company Confidential
 *          Copyright (c) EMC Corp, 2003
 *          All Rights Reserved
 *          Licensed material - Property of EMC Corp
 **********************************************************************/

/**********************************************************************
 *
 *  File Name: 
 *          ManagementModuleLib.h
 *
 *  Description:
 *          Function prototypes for the Management Module functions 
 *          exported as a statically linked library.
 *
 *  Revision History:
 *          07-Feb-15       Phil Leef   Created
 *          
 **********************************************************************/

#include "spid_types.h"
#include "management_module_micro.h"

#ifdef __cplusplus
extern "C"
{
#endif

void ManagementModuleLibInitMgmtAttr(SPID_PLATFORM_INFO platformInfo);

typedef struct _MGMT_ATTR_REQUEST
{
    MGMT_MODULE_REG_BLOCK   regBlock;
    char                    niceName[40];
    UCHAR                   readOffset;
    ULONG                   readByteCount;
    UCHAR                   writeOffset;
    ULONG                   writeByteCount;
    BOOL                    writeable;
    BOOL                    valid;
} MGMT_ATTR_REQUEST, *PMGMT_ATTR_REQUEST;

extern MGMT_ATTR_REQUEST mgmtAttr[MGMT_MODULE_TOTAL_BLOCK];

#ifdef __cplusplus
}
#endif

#endif // __MANAGEMENT_MODULE_LIB__
