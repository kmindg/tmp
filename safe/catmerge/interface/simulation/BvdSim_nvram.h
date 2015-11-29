#ifndef BVDSIM_NVRAM_H
#define BVDSIM_NVRAM_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  DESCRIPTION
 *     This file defines the functional interface to CMM.
 *   
 *
 *  HISTORY
 *     27-Sep-2009     Created.   -Austin Spang
 *
 *
 ***************************************************************************/
extern "C" {
# include "K10Plx.h"
# include "K10NvRam.h"
# include "K10NvRamLib.h"
# include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT
};

EMCPAL_STATUS CSX_MOD_EXPORT BvdSim_nvram_setPersistenceStatus(ULONG persistStatus);

void CSX_MOD_EXPORT BvdSim_nvram_reset();

#endif // BVDSIM_NVRAM_H
