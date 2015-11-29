#ifndef _CRASHHELPER_H_
#define _CRASHHELPER_H_

/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *              CrashHelper.h
 ***************************************************************************
 *
 * Description:
 *
 *   Defines crash helper functions
 *
 * Notes:
 *
 *
 * History:
 *
 *   Carl Kemp 2010/8/05
 ***************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include "safe_cp_code.h"

BOOL CreateDumpFile(const char *pDumpfileName, csx_pvoid_t pNativeExceptionInfo);

#ifdef __cplusplus
};
#endif /* extern "C" */

#endif /* _CRASHHELPER_H_ */
