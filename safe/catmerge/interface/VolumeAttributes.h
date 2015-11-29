/*******************************************************************************
 * Copyright (C) EMC Corporation, 1998 - 2007
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * VolumeAttributes.h
 *
 * This header file defines constants that define common volume attributes.
 *
 ******************************************************************************/

#if !defined (VolumeAttributes_H)
#define VolumeAttributes_H


// A set of boolean volume attributes.  The parity of each attribute
// should be set so that if a volume is composed of two sub-volumes,
// the "or" of the bits from each sub-volume will create the correct
// result.  
// NOTE: this type is passed between SPs, so its defintion affects NDU.
typedef ULONG VOLUME_ATTRIBUTE_BITS , *PVOLUME_ATTRIBUTE_BITS;

// see http://aseweb/docs/core/main/software/LayeredDrivers/DiskDeviceIRPs.html

// Sequences of unused bits ("gaps") are noted below.

# define VOL_ATTR_DEGRADED_BY_REDIRECTION     0x00000001  // Inter-SP hop
# define VOL_ATTR_RAID_PROTECTION_DEGRADED    0x00000002  // RAID protection degraded due to disk failure.
# define VOL_ATTR_CACHE_DISABLED              0x00000004  // Write cache is off
# define VOL_ATTR_PERF_MON_DISABLED           0x00000008  // Performance monitoring is disabled
# define VOL_ATTR_HAS_BEEN_WRITTEN            0x00000010  // The volume is no longer freshly bound.
# define VOL_ATTR_ACTIVE                      0x00000020  // At least one component volume has serviced an I/O within the idle time
# define VOL_ATTR_IDLE_TIME_MET               0x00000040  // At least one component volume has not serviced an I/O within the idle time
# define VOL_ATTR_STANDBY_TIME_MET            0x00000080  // At least one component volume has not serviced an I/O within the standby time
# define VOL_ATTR_STANDBY                     0x00000100  // At least one component volume is in a low power state.
# define VOL_ATTR_BECOMING_ACTIVE             0x00000200  // At least one component volume is in the process of changing to the active state
# define VOL_ATTR_EXPLICIT_STANDBY            0x00000400  // At least one component received a STOP request
# define VOL_ATTR_STANDBY_POSSIBLE            0x00000800  // At least one component of the volume has nothing preventing STANDBY if the standby time was met.
# define VOL_ATTR_PATH_NOT_PREFERRED          0x00001000  // A given path is currently sub-optimal, alternate I/O path should be used if available
# define VOL_ATTR_UNATTACHED                  0x00002000  // The volume is not capable of doing I/O
# define VOL_ATTR_IO_ONLY_ON_OWNER            0x00004000  // The volume is only capable of I/O if preceeded by TRESPASS EXECUTE
# define VOL_ATTR_NOTIFY_EXTENT_CHANGE        0x00008000  // Extent change notifications are required by this driver or a lower driver.
# define VOL_ATTR_IS_REDIRECTOR_OWNER         0x00010000  // Set if this SP has the redirector ownership token for closest redirector.
# define VOL_ATTR_REPORT_NON_OWNER_OPTIMIZED  0x00020000  // Indicates that both SPs should be considered optimized.
# define VOL_ATTR_BELOW_SPACE_THRESHOLD_ALERT 0x00040000  // The storage pool associated with the volume has reached the user specified capacity threshold.
# define VOL_ATTR_LIMIT_READ_AHEAD            0x00080000  // Large read ahead not appropriate for volume (i.e. may be Fast Cached)

# define VOL_ATTR_FORCE_ON_BIT1             0x00100000  // Future bit - guaranteed set by all drivers
# define VOL_ATTR_FORCE_ON_BIT2             0x00200000  // Future bit - guaranteed set by all drivers
                                                        // gap
# define VOL_ATTR_FORCE_OFF_BIT1            0x01000000  // Future bit - guaranteed clear by all drivers
# define VOL_ATTR_FORCE_OFF_BIT2            0x02000000  // Future bit - guaranteed clear by all drivers
                                                        // gap
# define VOL_ATTR_PPME_NOT_READY            0x80000000  // PPME is still testing paths (future/Invista)

# define VOL_ATTR_DEFAULT_FORCE_ON  ( VOL_ATTR_FORCE_ON_BIT1 | VOL_ATTR_FORCE_ON_BIT2 )
# define VOL_ATTR_DEFAULT_FORCE_OFF ( VOL_ATTR_FORCE_OFF_BIT1 | VOL_ATTR_FORCE_OFF_BIT2 )

// Helper macro to correctly force bits.  This macro takes a volume attributes
// bitmask and returns the new, massaged, bitmask.
// Usage: volumeAttributes = VOL_ATTR_FORCE_BITS( volumeAttributes );
# define VOL_ATTR_FORCE_BITS( _vol_attr_ ) \
    ( ( ( _vol_attr_ ) | VOL_ATTR_DEFAULT_FORCE_ON ) & ~VOL_ATTR_DEFAULT_FORCE_OFF )

// Shorthand define for standby states that launch a spinup internally based on arriving commands 
// Note: Explicit standby requires a LOAD_MEDIA before handling media-access commands 
#define IMPLICIT_SPINUP_NEEDED (VOL_ATTR_STANDBY|VOL_ATTR_IDLE_TIME_MET|VOL_ATTR_STANDBY_TIME_MET)

#endif /* !defined (VolumeAttributes_H) */

