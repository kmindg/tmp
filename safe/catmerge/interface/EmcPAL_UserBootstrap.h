#ifndef _EMCPAL_USERBOOTSTRAP_H
#define _EMCPAL_USERBOOTSTRAP_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
/*!
 * @file EmcPAL_UserBootstrap.h
 * @addtogroup emcpal_configuration
 * @{
 */

/*!
 **************************************************************************
 *  EmcPAL_UserBootstrap.h
 ***************************************************************************
 *
 *  @brief
 *      Initialization/Deinitialization routines for PAL and CSX to be used
 *      by userspace applications
 *
 *  @version
 *      05/20/09 jcaisse    Created
 ***************************************************************************/


#include "csx_ext.h"

#ifdef __cplusplus
extern "C" {
#endif

csx_nsint_t CSX_MOD_WINCC initPALandCSX(csx_void_t);

csx_nsint_t CSX_MOD_WINCC deinitPALandCSX( csx_void_t);
    
#ifdef __cplusplus
}
#endif

/*!
 * @} end group emcpal_configuration
 * @} end file EmcPAL_UserBootstrap.h
 */

#endif /* _EMCPAL_USERBOOTSTRAP_H */
