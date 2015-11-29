#ifndef _EMCPAL_MISC_H_
#define _EMCPAL_MISC_H_
//***************************************************************************
// Copyright (C) EMC Corporation 2011-2014
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/
/*!
 * @file EmcPAL_Misc.h
 * @addtogroup emcpal_misc
 * @{
 *
 * @brief
 *      The header file is for Generic functions in EmcPAL abstraction layer
 *
 * @version
 *     04/27/2011  Manjit Singh Initial creation
 *
 */
 
//++
// Include files
//--

#include "csx_ext.h"
#include "EmcPAL_Retired.h"

#define EMCPAL_EXPORT CSX_MOD_EXPORT		/*!< EmcPAL export */
#define EMCPAL_IMPORT CSX_MOD_IMPORT		/*!< EmcPAL import */

/*! @brief EmcPAL API */
#ifdef EMCPAL_PAL_BUILD
#define EMCPAL_API EMCPAL_EXPORT
#else
#define EMCPAL_API EMCPAL_IMPORT
#endif

#if (defined(UMODE_ENV) || defined(SIMMODE_ENV))
#define EmcpalDbgPrint EmcutilDebugPathPrint // C4 or windows user space
#else 
#include "ktrace.h"
#define EmcpalDbgPrint KvPrint  // C4 or Windows kernel space
#endif

#if (defined(UMODE_ENV) || defined(SIMMODE_ENV))
#define EmcpalDbgPrintRaw EmcutilDebugPathPrint
#elif defined(ALAMOSA_WINDOWS_ENV)
#define EmcpalDbgPrintRaw DbgPrint 
#else
#define EmcpalDbgPrintRaw EmcutilDebugPathPrint 
#endif

#if defined(DBG) && (DBG != 0)
#define EmcpalDbgPrintIfDbg(_x_) EmcpalDbgPrint _x_
#else
#define EmcpalDbgPrintIfDbg
#endif

#if defined(DBG) && (DBG != 0)
#define EmcpalDbgPrintRawIfDbg(_x_) EmcpalDbgPrintRaw _x_
#else
#define EmcpalDbgPrintRawIfDbg
#endif

/*!
 * @} end group emcpal_misc
 * @} end file EmcPAL_Misc.h
 */
#endif // _EMCPAL_MISC_H_
