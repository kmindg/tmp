#ifndef __CacheStates_h__
#define __CacheStates_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2009
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      CacheStates.h
//
// Contents:
//      This file defines the Cache States names used externally to the Cache.
//
// NOTE: This defines the states for the DRAM Cache layered driver.
//
// Revision History:
//--

enum CacheState {
    // Enabled State
    Enabled = 3,

    // Quiescing State - Requests are in the process of stopping.
    Quiescing = 4,

    // Quiesced State - Requests have stopped processing 
    Quiesced = 5,

    // Dumping State - Dumping dirty data to the vault LUN.
    Dumping = 6,

    // Cleaning State - Writing dirty to the backing store for all volumes currently assigned.
    Disabling = 7,

    // WriteThru State - Writes are written to the backing store before acknowledging the host.
    Disabled = 8,

    // FaultedLuns - Cache enabled but at least one LUN is faulted.
    FaultedLUNs = 9,

    // FaultedWriteThru - Cache in write thru state with faulted LUNs.
    FaultedWriteThru = 10
};

#endif  // __CacheStates_h__

