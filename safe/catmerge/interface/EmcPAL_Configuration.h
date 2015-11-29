//***************************************************************************
// Copyright (C) EMC Corporation 2011
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/
/*!
 * @file EmcPAL_Configuration.h
 * @addtogroup emcpal_configuration
 * @{
 */

/*!
 * @brief
 *      The header file that configures the EmcPAL abstraction layer
 *
 *
 * The VNX-Block build process uses the following defines
 * to control EmcPAL configuration and to select between the different
 * EmcPAL implementations:
 *
 *  UMODE_ENV -  When UMODE_ENV is defined, EmcPAL needs to generate 
 *               production user space programs and shared libraries.
 *               In addition, production user code needs access to 
 *               flare_ioctls.h and flare_internal_ioctls.h, which appear
 *               to require use of the Windows NT ntddk.h kernel definitions.
 *
 * SIMMODE_ENV - When SIMMODE_ENV is defined, EmcPAL needs to generate
 *               user space programs and shared libraries.
 *               In addition, the execution environment emulates the 
 *               Windows Kernel Device Module environment which
 *               requires including ntddk.h. Note, all production code and
 *               emulation code is required to use EmcPAL and may directly
 *               use CSX.
 * 
 * @note NOTE THIS INCLUDE file SHOULD NOT BE DIRECTLY INCLUDED IN ANY 
 *      VNX-BLOCK CODE
 *
 * @note Whenever UMODE_ENV and SIMMODE_ENV are NOT defined, EmcPAL needs to
 *       generate kernel mode driver sys files and EmcPAL needs to configure
 *       and provide the Windows Kernel Mode implementation to drivers.
 *
 *
 * @version
 *     02-11-12 Martin Buckley  Initial creation
 *
 */

#ifndef _EMCPAL_CONFIGURATION_H_
#define _EMCPAL_CONFIGURATION_H_

/*
 * PAL general configuration
 */

/*!
 * EMCPAL_CASE_C4_OR_WU - C4(Linux,ISC,SSPG) case or Windows user space case
 * EMCPAL_CASE_C4_OR_WK - C4(Linux,ISC,SSPG) case or Windows kernel space case
 * EMCPAL_CASE_WK - Windows kernel space case
 * EMCPAL_CASE_MAINLINE - Mainline data path case
 */
#ifndef ALAMOSA_WINDOWS_ENV

#if defined(UMODE_ENV)
#define EMCPAL_CASE_C4_OR_WU
#elif defined(SIMMODE_ENV)
#define EMCPAL_CASE_C4_OR_WU
#else
#define EMCPAL_CASE_C4_OR_WK
#define EMCPAL_CASE_MAINLINE
#endif

#else

#if defined(ALAMOSA_WINDOWS_ENV) && defined(I_AM_A_KERNEL_DRIVER_DEBUG_EXTENSION)
#define EMCPAL_CASE_NTDDK_EXPOSE
#endif

#if defined(CSX_BV_LOCATION_KERNEL_SPACE) || defined(EMCPAL_CASE_NTDDK_EXPOSE)
#define EMCPAL_CASE_WK
#define EMCPAL_CASE_C4_OR_WK
#endif
#if defined(CSX_BV_LOCATION_USER_SPACE)
#define EMCPAL_CASE_C4_OR_WU
#endif
#if !(defined(UMODE_ENV) || defined(SIMMODE_ENV))
#define EMCPAL_CASE_MAINLINE
#endif

#endif /* ALAMOSA_WINDOWS_ENV - ARCH - structural differences */

/*!
 * PAL Packaging configuration
 */

#if defined(CSX_BV_ENVIRONMENT_WINDOWS)
#define EMCPAL_OPT_REAL_WINDOWS_AVAILABLE
#endif

#ifndef ALAMOSA_WINDOWS_ENV

#ifdef EMCPAL_CASE_MAINLINE
#define EMCPAL_OPT_DRIVERSHELL_INLINED
#else
#define EMCPAL_OPT_DRIVERSHELL_LIBRARY
#endif

#else

#if !defined(CSX_BV_LOCATION_USER_SPACE)
#define EMCPAL_OPT_DRIVERSHELL_INLINED
#endif
#if defined(CSX_BV_LOCATION_USER_SPACE)
#define EMCPAL_OPT_DRIVERSHELL_LIBRARY
#endif

#endif /* ALAMOSA_WINDOWS_ENV - ARCH - structural differences */

/*!
 * PAL option selections
 */

#ifndef ALAMOSA_WINDOWS_ENV

#define EMCPAL_USE_CSX_PRINTS
#define EMCPAL_USE_CSX_ASSERTS
#define EMCPAL_USE_CSX_STRINGS
#define EMCPAL_USE_CSX_MEMORY /* Use CSX for memory manipulation ops */
#define EMCPAL_USE_CSX_ARE  /* Use CSX Automatic Reset Events */
#define EMCPAL_USE_CSX_MRE  /* Use CSX Manual Reset Events */
#define EMCPAL_USE_CSX_SEM  /* Use CSX Semaphores */
#define EMCPAL_USE_CSX_MUT  /* Use CSX Mutex */
#define EMCPAL_USE_CSX_RMUT /* Use CSX Recursive Mutex */
#define EMCPAL_USE_CSX_SPL  /* Use CSX Spinlocks */
#define EMCPAL_USE_CSX_THR  /* Use CSX Threads */
#define EMCPAL_USE_CSX_INTERLOCKED /* Use CSX for interlocked ops */
#define EMCPAL_USE_CSX_MMIO /* Use CSX for memory mapped IO */
#define EMCPAL_USE_CSX_DPC /* Use CSX for Deferred Procedure Calls */
#define EMCPAL_USE_CSX_LISTS /* Use CSX for Lists */
#define EMCPAL_USE_CSX_DCALL       /* Use CSX for IRP abstraction */
#define EMCPAL_USE_CSX_MEMORY_ALLOC /* Use CSX for memory allocation ops */
#if defined(SIMMODE_ENV) || defined(UMODE_ENV)
#define EMCPAL_USE_SIMULATED_IOMEMORY
#else
#define EMCPAL_USE_CSX_IOMEMORY /* Use CSX for IO memory - requires CSX-style initialization */
#endif
#define EMCPAL_USE_CSX_TIM_DPC  /* Use CSX timers and Deferred Procedure Calls */

#else

#ifdef EMCPAL_CASE_C4_OR_WU
#define EMCPAL_USE_CSX_PRINTS
#define EMCPAL_USE_CSX_ASSERTS
#define EMCPAL_USE_CSX_STRINGS
#define EMCPAL_USE_CSX_MEMORY /* Use CSX for memory manipulation ops */
#define EMCPAL_USE_CSX_ARE  /* Use CSX Automatic Reset Events */
#define EMCPAL_USE_CSX_MRE  /* Use CSX Manual Reset Events */
#define EMCPAL_USE_CSX_SEM  /* Use CSX Semaphores */
#define EMCPAL_USE_CSX_MUT  /* Use CSX Mutex */
#define EMCPAL_USE_CSX_RMUT /* Use CSX Recursive Mutex */
#define EMCPAL_USE_CSX_SPL  /* Use CSX Spinlocks */
#define EMCPAL_USE_CSX_THR  /* Use CSX Threads */
#define EMCPAL_USE_CSX_INTERLOCKED /* Use CSX for interlocked ops */
#define EMCPAL_USE_CSX_MMIO /* Use CSX for memory mapped IO */
#define EMCPAL_USE_CSX_DPC /* Use CSX for Deferred Procedure Calls */
#define EMCPAL_USE_CSX_LISTS /* Use CSX for Lists */
#define EMCPAL_USE_CSX_DCALL       /* Use CSX for IRP abstraction */
#define EMCPAL_USE_CSX_MEMORY_ALLOC /* Use CSX for memory allocation ops */
#if defined(SIMMODE_ENV) || defined(UMODE_ENV)
#define EMCPAL_USE_SIMULATED_IOMEMORY
#else
#define EMCPAL_USE_CSX_IOMEMORY /* Use CSX for IO memory - requires CSX-style initialization */
#endif
#define EMCPAL_USE_CSX_TIM_DPC  /* Use CSX timers and Deferred Procedure Calls */
#else
#define EMCPAL_USE_CSX_TIM_DPC  /* Use CSX timers and Deferred Procedure Calls */
#endif

#endif /* ALAMOSA_WINDOWS_ENV - ARCH - structural differences */

#if defined(_MSC_VER)
#define EMCPAL_TARGET_WINDOWS_NT
#endif

#ifdef ALAMOSA_WINDOWS_ENV
#define EMCPAL_OPT_WINDOWS_DEBUG_API
#endif

/*!
 *	@} end group emcpal_configuration
 *  @} EmcPAL_Configuration.h
 */

#endif // _EMCPAL_CONFIGURATION_H_
