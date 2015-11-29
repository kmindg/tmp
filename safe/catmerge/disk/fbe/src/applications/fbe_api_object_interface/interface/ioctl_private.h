/*********************************************************************
 * Copyright (C) EMC Corporation, 1989 - 2011
 * All Rights Reserved.
 * Licensed Material-Property of EMC Corporation.
 * This software is made available solely pursuant to the terms
 * of a EMC license agreement which governs its use.
 *********************************************************************/

/*********************************************************************
 *
 * Description:
 *      This header file contains Ioctl system and user file 
 *      includes. 
 *
 *  History:
 *      12-May-2010 : initial version
 *
 *********************************************************************/

#ifndef IOCTL_PRIVATE_H
#define IOCTL_PRIVATE_H

/*********************************************************************
 * Includes : system files 
 *********************************************************************/

//#include <assert.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*********************************************************************
 * Indcludes catmerge\interface 
 *********************************************************************/

#include "SimpUtil.h"
#include "EmcPAL.h"
#include "K10Assert.h"
#include "ktrace.h"
#include "flare_export_ioctls.h"

/*********************************************************************
 * Includes : // catmerge\interface\Cache
 *********************************************************************/

//#include "DRAMCacheDebugDisplay.h"
#include "Cache/CacheDriverParams.h"
#include "Cache/CacheDriverStatus.h"
#include "Cache/CacheVolumeStatus.h"
#include "Cache/CacheVolumeParams.h"
#include "Cache/CacheVolumeStatistics.h"
#include "Cache/CacheDriverStatistics.h"

extern "C" {

/*********************************************************************
 * fbe includes // catmerge\disk\interface\fbe directory (fbe)
 *********************************************************************/

#include "fbe/fbe_types.h"

} // *** end of extern "C" { ... } ***


#endif /* IOCTL_PRIVATE_H */
