#ifndef ENABLER_LIB_H
#define ENABLER_LIB_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/****************************************************************
 *  EnablerLib.h
 ****************************************************************
 *
 *  Description:
 *  This header file contains structures/definitions for enablers
 *  querying enablers/suites that are active within the system
 *
 *  History:
 *      Feb-13-2014 . Phil Leef - Created
 *
 ****************************************************************/


#include "generic_types.h"
#include "csx_ext.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum _ENABLER_PACKAGES
{
    EL_ENABLER_MIN = 0,

    /* Enabler Packages*/
    EL_ENABLER_ACCESS_LOGIX = EL_ENABLER_MIN,
    EL_ENABLER_ADVANCED_SNAPS,
    EL_ENABLER_AUTO_TIERING,
    EL_ENABLER_BLOCK,
    EL_ENABLER_COMPRESSION,
    EL_ENABLER_DATA_AT_REST_ENCRYPTION,
    EL_ENABLER_DEDUPLICATION,
    EL_ENABLER_EXTENDED_CACHE,
    EL_ENABLER_FAST_CACHE,
    EL_ENABLER_FILE,
    EL_ENABLER_MIRRORVIEW_S,
    EL_ENABLER_MIRRORVIEW_A,
    EL_ENABLER_QOS_MANAGER,
    EL_ENABLER_RPSPLITTER,
    EL_ENABLER_SANCOPY,
    EL_ENABLER_SNAPVIEW,
    EL_ENABLER_VIRTUAL_PROVISIONING,
    EL_ENABLER_ODX,
    EL_ENABLER_VAAI_XCOPY,
    EL_ENABLER_VNX_CA,
    EL_ENABLER_WRITE_CACHE_AVAILABILITY,
    EL_ENABLER_MAX,
} ENABLER_PACKAGES;

void EnablerLibInitEnablerAttr(void);

EMCUTIL_STATUS enablerIsPresent(ENABLER_PACKAGES     enabler,
                                BOOL                 *present);

/************************************************************
 * BELOW THIS LINE, ARE ITEMS USED INTERNALLY BY THE LIBARY
 ************************************************************/
typedef enum _REGISTRY_VALUE_TYPE {
    REG_VALUE_SZ,               //REG_SZ
    REG_VALUE_DWORD,            //REG_DWORD
    REG_VALUE_BINARY,           //REG_BINARY
    REG_VALUE_MULTISTRING,      //REG_MULTI_SZ
} REGISTRY_VALUE_TYPE;

 typedef struct _ENABLER_ATTR_REQUEST
{
    ENABLER_PACKAGES        enabler;
    char                    regPath[256];
    REGISTRY_VALUE_TYPE     valueType;
    char                    regKey[256];
    char                    displayName[256];
    BOOL                    valid;
} ENABLER_ATTR_REQUEST, *PENABLER_ATTR_REQUEST;

extern ENABLER_ATTR_REQUEST enablerAttr[EL_ENABLER_MAX];

#ifdef __cplusplus
}
#endif

#endif //ENABLER_LIB_H
