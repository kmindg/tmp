/*******************************************************************************
 * Copyright (C) EMC Corporation, 2008.
 * All rights reserved.
 * Licensed material - Property of Data General Corporation
 ******************************************************************************/

/*******************************************************************************
 * PowerSavingPolicy.h
 *
 * This header file defines constants and structures associated with IOCTL_FLARE_SET_POWER_SAVING_POLICY
 * defined by CLARiiON.
 * See http://aseweb.us.dg.com/docs/core/main/software/LayeredDrivers/DiskDeviceIRPs.html#IOCTL_FLARE_SET_POWER_SAVING_POLICY
 *
 ******************************************************************************/

#if !defined (PowerSavingPolicy_H)
#define PowerSavingPolicy_H


/*******************************/
/* Power Policy Global values. */
/*******************************/

// Duration a unit must be idle before becoming a candidate for spindown.  
#define DISABLE_IdleTimeSpindown 0xFFFFFFFF

// Effective value to disable spindown - as immediate spinup is unachievable. 
#define DISABLE_MaxSpinUpTime    0

/***********************************/
/* Power Policy Global Structures. */
/***********************************/

//
// Name: FLARE_POWER_SAVING_POLICY
//
// Description:  This is the structure, which is passed in for the IOCTL_FLARE_SET_POWER_SAVING_POLICY.
//
// See http://aseweb.us.dg.com/docs/core/main/software/LayeredDrivers/DiskDeviceIRPs.html#IOCTL_FLARE_SET_POWER_SAVING_POLICY
//
typedef struct FLARE_POWER_SAVING_POLICY 
{
    // The minimum time to wait before an implicit spin down.
    ULONG          IdleSecondsBeforeReportingNotReady;

    // The maximum time that a spin up may take. This parameter affects the 
    // decision about whether/how to spin down. 
    // Note: this can only hold up to about 482 seconds.
    ULONG          MaxSpinUpTimeIn100NanoSeconds;

#ifdef __cplusplus
    bool operator ==(const FLARE_POWER_SAVING_POLICY & other) const {
        return IdleSecondsBeforeReportingNotReady == other.IdleSecondsBeforeReportingNotReady
            && MaxSpinUpTimeIn100NanoSeconds == other.MaxSpinUpTimeIn100NanoSeconds;
    }
    bool operator != (const FLARE_POWER_SAVING_POLICY & other) const {
        return !(*this == other);
    }
# endif

} FLARE_POWER_SAVING_POLICY, *PFLARE_POWER_SAVING_POLICY;

#endif /* !defined (PowerSavingPolicy_H) */
