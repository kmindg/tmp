//***************************************************************************
// Copyright (C) EMC Corporation 2011
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/
/* // omit this from doxygen output since this is all internal detail
 * @file EmcPAL_Platform_Includes.h
 * @addtogroup emcpal_windows
 * @{
 *
 * @brief
 *      This header file needs to contain the most commonly OS specific 
 *      include files needed to configure EmcPAL. 
 *
 *      At a minimum, all system specific include files required to
 *      describe the structs and APIs contain in EmcPAL.h must reside here.
 *
 *      Ultimately, when references to os specific include files have been
 *      expunged from VNX-Block code base, this file will be the only EmcPAL
 *      include file that references OS specific include files.
 *
 *      @note THIS INCLUDE file SHOULD NOT BE DIRECTLY INCLUDED IN ANY 
 *      VNX-BLOCK CODE
 *
 * @version
 *     02-11-12 Martin Buckley  Initial creation
 *
 */

#ifndef _EMCPAL_PLATFORM_INCLUDES_H_
#define _EMCPAL_PLATFORM_INCLUDES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "csx_ext.h"
#include "EmcPAL_Configuration.h"
#include "EmcPAL_Generic.h"

#include "generic_types.h" //PKPK - UGLY

#if defined(UMODE_ENV)

#if defined(I_AM_DEBUG_EXTENSION) && !defined(I_AM_NATIVE_CODE) && !defined(I_AM_NONNATIVE_CODE)
#ifdef ALAMOSA_WINDOWS_ENV
#include <ntddk.h>
#include <basetsd.h>
#include <windef.h>
#else
#include "safe_win_k_stoff.h"
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */
#elif defined(I_AM_NONNATIVE_CODE)
#ifdef ALAMOSA_WINDOWS_ENV
#include <ntddk.h>
#include <basetsd.h>
#include <windef.h>
#else
#include "safe_win_k_stoff.h"
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */
#elif defined(I_AM_NATIVE_CODE)
#ifdef ALAMOSA_WINDOWS_ENV
#include <windows.h>
#include <devioctl.h>
#else
#include "safe_win_u_stoff.h"
#endif
#else /* I_AM_ */
#ifdef ALAMOSA_WINDOWS_ENV
#include <windows.h>
#include <devioctl.h>
#include <winternl.h>
#else
#include "safe_win_u_stoff.h"
#endif
#endif /* I_AM_ */

#elif defined(SIMMODE_ENV)

#if defined(I_AM_DEBUG_EXTENSION) && !defined(I_AM_NATIVE_CODE) && !defined(I_AM_NONNATIVE_CODE)
#ifdef ALAMOSA_WINDOWS_ENV
#include <ntddk.h>
#include <basetsd.h>
#include <windef.h>
#else
#include "safe_win_k_stoff.h"
#endif
#elif defined(I_AM_NATIVE_CODE)
#ifdef ALAMOSA_WINDOWS_ENV
#include <windows.h>
#include <devioctl.h>
#else
#include "safe_win_u_stoff.h"
#endif
#else /* I_AM_ */
#ifdef ALAMOSA_WINDOWS_ENV
#include <ntddk.h>
#include <basetsd.h>
#include <windef.h>
#else
#include "safe_win_k_stoff.h"
#endif
#endif /* I_AM_ */

#else /* _ENV */

#if defined(I_AM_DEBUG_EXTENSION) && !defined(I_AM_NATIVE_CODE) && !defined(I_AM_NONNATIVE_CODE)
#ifdef ALAMOSA_WINDOWS_ENV
#include <ntddk.h>
#include <basetsd.h>
#include <windef.h>
#else
#include "safe_win_k_stoff.h"
#endif
#elif defined(I_AM_NATIVE_CODE)
#ifdef ALAMOSA_WINDOWS_ENV
#include <windows.h>
#include <devioctl.h>
#else
#include "safe_win_u_stoff.h"
#endif
#else /* I_AM_ */
#ifdef ALAMOSA_WINDOWS_ENV
#include <ntddk.h>
#include <basetsd.h>
#include <windef.h>
#else
#include "safe_win_k_stoff.h"
#endif
#endif /* I_AM_ */

#endif /* _ENV */

/* this provides the Unicode structures needed for base EMCPAL APIs */
#include "csx_rt_wcl_if.h"

#ifdef __cplusplus
};
#endif

#ifndef ALAMOSA_WINDOWS_ENV
#include "safe_fix_null.h"
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */

/*
 * @} end group emcpal_windows
 * @} end file EmcPAL_Platform_Includes.h
 */

#endif  // _EMCPAL_PLATFORM_INCLUDES_H_
