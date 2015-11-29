#ifndef EMCUTIL_NETWORK_H_
#define EMCUTIL_NETWORK_H_

//***************************************************************************
// Copyright (C) EMC Corporation 2011-2012
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

/*! @file  EmcUTIL_Network.h */
//
// Contents:
//      Header file for the EMCUTIL_Network APIs.

/*! @addtogroup emcutil_network
 *  @{
 */

#include "EmcUTIL.h"

CSX_CDECLS_BEGIN

////////////////////////////////////

#if defined(UMODE_ENV) || defined(SIMMODE_ENV)

EMCUTIL_API VOID EMCUTIL_CC
EmcutilNetworkWorldInitializeAlways(VOID);

EMCUTIL_API int EMCUTIL_CC
EmcutilNetworkWorldInitializeMaybe(VOID); // 0 == success

EMCUTIL_API VOID EMCUTIL_CC
EmcutilNetworkWorldDeinitializeAlways(VOID);

EMCUTIL_API int EMCUTIL_CC
EmcutilNetworkWorldDeinitializeMaybe(VOID); // 0 == success

EMCUTIL_API int EMCUTIL_CC
EmcutilLastNetworkErrorGet(VOID);

EMCUTIL_API VOID EMCUTIL_CC
EmcutilLastNetworkErrorSet(int error);

#endif

////////////////////////////////////

CSX_CDECLS_END

/* @} END emcutil_network */

//++
//.End file EmcUTIL_Network.h
//--

#endif                                     /* EMCUTIL_NETWORK_H_ */
