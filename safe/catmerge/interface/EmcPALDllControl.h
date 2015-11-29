//***************************************************************************
// Copyright (C) EMC Corporation 2011
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/
/*!
 * @file EmcPALDllControl.h
 * @addtogroup emcpal_configuration
 * @{
 *
 * @brief
 *      Prototype declarations for the logic used manage EmcPAL resources 
 *      via the DllInitialize() and DllUnload() functions
 *
 *  Functions:
 *          EmcPALDllInitialize()
 *          EmcPALDllUnload()
 *
 * @version
 *      01-Ja-11   MJBuckley  Initialy Created.
 */

#ifndef __EMCPALDLLCONTROLL__
#define __EMCPALDLLCONTROLL__

# include "EmcPAL.h"

#ifdef __cplusplus
/*!
 * all of these functions need standard C linkage.  Otherwise, they can not be referenced from C.
 * Note, thee implementation of these functions is within a cpp file, which allows these functions to
 * access C++ instances, instance methods and class methods
 */
extern "C" {
#endif

/*! @brief Initialize EmcPAL DLL
 *  @details Routine EmcPALDllInitialize creates a CSX Module within the current CSX
 *  Containter and returns an EMCPAL_CLIENT pointer needed for managing
 *   EmcPAL entities, within this DLL.
 *  This routine should be invoked by a shared Libraries DllInitialize() function
 *  @param moduleName - specifies the name used when creating a CSX Module
 *  @return none
 */
void EmcPALDllInitialize(TEXT * moduleName);

/*! @brief Unload EmcPAL DLL
 *  @details Routine EmcPALDllUnload Releasese EmcPAL/CSX Resources. 
 * This routine should be invoed by a Shared Libraries DllUnload() function
 *  @return none
 */
void EmcPALDllUnload(void);


/*! @brief Get EmcPAL DLL client
 *  @return The EMCPAL_ClLIENT pointer needed to manage EmcPAL entities
 */
PEMCPAL_CLIENT EmcPALDllGetClient(void);

#ifdef __cplusplus
};
#endif
/*!
 * @} end group emcpal_configuration
 * @} end file EmcPALDllControl.h
 */
#endif // __EMCPALDLLCONTROLL__
