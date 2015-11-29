#ifndef __WATCHDOG_COMMON_H__
#define __WATCHDOG_COMMON_H__

//***************************************************************************
// Copyright (C) EMC Corporation 2006
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//++
// FILE:
//      WatchdogCommon.h
//
// DESCRIPTION:
//      This header file contains definitions and typedefs 
//      for common watchdog functionality.  
//
// NOTES:
//
// HISTORY:
//      April, 2006 - Created, Ian McFadries
//--

#define INVALID_WATCHDOG_HANDLE  (-1)
#define SOFTWARE_WATCHDOG_HANDLE  (0)
#define HARDWARE_WATCHDOG_HANDLE  (1)

typedef ULONG WATCHDOG;
typedef WATCHDOG *PWATCHDOG;

#endif
