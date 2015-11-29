// fileheader
//
//   File:          TierDefinition.h
//
//   Description:   Definitions and macros used by the Tier Definition Library.
//                  This library is stateless and it builds in user and kernel mode.
//                  Clients of this library in kernel space should include this header file
//                  and modify their TARGETLIBS to link in TierDefinitionKernel.lib.
//                  User space should include this header file and modify their TARGETLIBS
//                  to pick up the TierDefinitionUser.lib functions they need.
//
//   Author(s):     Sam Mullis, Alan Sawyer
//
//   Change Log:    07-30-2009              Initial creation
//                  09-23-2009              Changes based on code review.
//                  10-19-2009              Changes to add new message catalog messages.
//      
//
//  Copyright (c) 2009, EMC Corporation.  All rights reserved.
//  Licensed Material -- Property of EMC Corporation.
//

#if !defined (TIERDEFINITION_H)
#define TIERDEFINITION_H

#if defined(UMODE_ENV)  // user and simulation mode library
// this block of code is to make it easier to include functions from this library if it is 
// built as a DLL.
#if TIERDEFINITION_EXPORTS // We're exporting the functions from the DLL
#define TIERDEFINITION_API CSX_MOD_EXPORT
#else // We're importing from the DLL
#define TIERDEFINITION_API CSX_MOD_IMPORT
#endif
#else
#define TIERDEFINITION_API
#endif

// define some kernel return types so the user space code will compile without a lot
// of embedded ifdefs

#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
#define RETURN_RESULT HRESULT
#define RETURN_SUCCESS S_OK
#else
#define RETURN_RESULT EMCPAL_STATUS
#define RETURN_SUCCESS EMCPAL_STATUS_SUCCESS
#endif


// includes
#include "drive_types.h"
#include "raid_type.h"
#include "K10TDLAutoTieringMessages.h"
#include "csx_ext.h"

/*      NUMBER_OF_COARSE_TIERS

`NUMBER_OF_COARSE_TIERS` is a constant, positive, int expression
specifying the number of defined coarse tiers.  Coarse tier indices
are implicitly 0 through one less than this value.

The coarse tiers are ordered such that increasing coarse tier indices
correspond to decreasing expected tier performance.  In particular,
the coarse tier with index 0 is the highest-performing tier.

*/

#define NUMBER_OF_COARSE_TIERS 3


/*      MIN_COARSE_TIER_INDEX

`MIN_COARSE_TIER_INDEX` The lowest valid index into an array of coarse tiers, is zero.
This is the index of the highest-performing coarse tier.

*/

#define MIN_COARSE_TIER_INDEX 0


/*      MAX_COARSE_TIER_INDEX

`MAX_COARSE_TIER_INDEX` The highest valid index into an array of coarse tiers, is a constant,
nonnegative, int expression.  This is the index of the lowest-performing coarse tier.
*/

#define MAX_COARSE_TIER_INDEX (NUMBER_OF_COARSE_TIERS - 1)


/*      coarseTierNiceName

`coarseTierNiceName(coarseTierIndex)` returns the nice name for the
coarse tier indexed by `coarseTierIndex`.  The return value must not
be changed by the caller.

*/
#ifdef __cplusplus
extern "C"
#endif
TIERDEFINITION_API RETURN_RESULT coarseTierNiceName(int coarseTierIndex,
                                                    const char **niceName);


/*      TIER_DESCRIPTOR

`TIER_DESCRIPTOR` is the type of a tier descriptor.  A tier descriptor
is treated abstractly.

A tier descriptor may be defined or undefined.  An undefined tier
descriptor is used to indicate that its associated object has not yet
been assigned a tier descriptor.

If a performance estimate cannot be derived a default value equal to the 
coarse tier index will be assigned to the tier descriptor.

*/

typedef ULONG64 TIER_DESCRIPTOR;

/*
The 64 bits consist of the top 56 bits of a double giving the
performance estimate followed by 8 bits giving the coarse tier index.
*/


#define UNDEFINED_TIER_DESCRIPTOR 0


/*      tierDescriptorIsDefined

`tierDescriptorIsDefined(tierDescriptor)` is an int expression that is
non-zero if and only if tier descriptor `tierDescriptor` is defined.

*/

#define tierDescriptorIsDefined(tierDescriptor) ((tierDescriptor) != UNDEFINED_TIER_DESCRIPTOR)


/*      coarseTierIndexFromTierDescriptor

`coarseTierIndexFromTierDescriptor(tierDescriptor)` returns the coarse
tier index for a tier descriptor.  `tierDescriptor` must be defined.

*/

#define coarseTierIndexFromTierDescriptor(tierDescriptor) ((int)((tierDescriptor) & 0x000000ff))


/*      performanceEstimateFromTierDescriptor

`performanceEstimateFromTierDescriptor(tierDescriptor)` returns the
performance estimate for a tier descriptor.  `tierDescriptor` must be
defined.

A performance estimate is a positive double value.  The ratio of the
performance estimates for two FLUs estimates the relative performance
of those FLUs.  A greater performance estimate value indicates better
performance.

*/

#define performanceEstimateFromTierDescriptor(tierDescriptor) ((tierDescriptor >> 8))

/*      deriveTierDescriptor

`deriveTierDescriptor( ... )`
returns the tier descriptor for a FLU with the given characteristics.

For Zeus, the following physical device properties will be used to derive the tier 
descriptor (and hence the performance estimate described below):
    Drive interface type (EFD, FC, SATA)
    RAID Type
    RAID group size (number of drives, size of smallest drive)
    FLU capacity in sectors.

    this function can return UNDEFINED_TIER_DESCRIPTOR if the drive type is invalid
    this function can return a tier descriptor equal to the coarse tier index if the 
    raid type is invalid

*/


#ifdef __cplusplus
extern "C"
#endif
TIERDEFINITION_API RETURN_RESULT deriveTierDescriptor(FLARE_DRIVE_TYPE driveType, 
                                                      UCHAR raidType,
                                                      ULONG32 numDisksInRG,
                                                      ULONG64 FluCapacityInSectors,
                                                      TIER_DESCRIPTOR *pTierDescriptor);

/*      deriveTierDescriptor

`deriveTierDescriptor( ... )`
returns the tier descriptor for a FLU with the given characteristics.

For Zeus, the following physical device properties will be used to derive the tier 
descriptor (and hence the performance estimate described below):
    Drive interface type (EFD, FC, SATA)
    RAID Type
    RAID group size (number of drives, size of smallest drive)
    RG capacity

    this function can return UNDEFINED_TIER_DESCRIPTOR if the drive type is invalid
    this function can return a tier descriptor equal to the coarse tier index if the 
    raid type is invalid

*/


#ifdef __cplusplus
extern "C"
#endif
TIERDEFINITION_API RETURN_RESULT deriveTierDescriptorRG(FLARE_DRIVE_TYPE driveType, 
                                                        UCHAR raidType,
                                                        ULONG32 numDisksInRG,
                                                        ULONG64 RGCapacityInSectors,
                                                        TIER_DESCRIPTOR *pTierDescriptor);

/*
`PERF_ESTIMATE_SCALE_FACTOR` is a constant used to scale-up the performance estimate because the raw
numbers tend to be fractional.
*/

#define PERF_ESTIMATE_SCALE_FACTOR 1000000

/*
`INVALID_IOPS_ESTIMATE` is a return value from raidGroupIOPSEstimate if invalid parameters are passed in.
*/
#define INVALID_IOPS_ESTIMATE 0


/*
`NUM_FLUS_IN_RG` the number of FLUs in an automatic RAID group used for a slice pool. 
*/
#define NUM_FLUS_IN_POOL_RG 1


/*
`raidGroupIOPSEstimate` is a function that returns a double (floating point) value that estimates the 
potential small, random IO Operations per second based on the given parameters passed to it.

For Zeus the parameters are driveType, raidType, number of disks in the RAID group, RaidGroupCapacityInSectors.
Also, for Zeus, this function takes the FluCapacityInSectors and calculates the RG capacity in sectors
using NUM_FLUS_IN_POOL_RG above and the number of disks in the RAID group since this is currently how automatic
RAID groups in pools are currently being created.
*/

#ifdef __cplusplus
extern "C"
#endif
TIERDEFINITION_API RETURN_RESULT raidGroupIOPSEstimate(FLARE_DRIVE_TYPE driveType, 
                                                       UCHAR raidType,
                                                       ULONG32 numDisksInRG,
                                                       ULONG64 FluCapacityInSectors,
                                                       double *pIOPSEstimate);


/*
`raidGroupIOPSEstimate` is a function that returns a double (floating point) value that estimates the 
potential small, random IO Operations per second based on the given parameters passed to it.

For Zeus the parameters are driveType, raidType, number of disks in the RAID group, FluCapacityInSectors.
*/
#ifdef __cplusplus
extern "C"
#endif
TIERDEFINITION_API RETURN_RESULT raidGroupIOPSEstimateRG(FLARE_DRIVE_TYPE driveType, 
                                                         UCHAR raidType,
                                                         ULONG32 numDisksInRG,
                                                         ULONG64 RGCapacityInSectors,
                                                         double *pIOPSEstimate);


/*
`raidGroupIOPSEstimateFromTierDescriptor` is a function that returns a double (floating point) value
that estimates the potential small, random IO Operations per second for a raid group based on the
performance estimate stored in the given tier descriptor passed to it.
*/
#ifdef __cplusplus
extern "C"
#endif
TIERDEFINITION_API RETURN_RESULT raidGroupIOPSEstimateFromTierDescriptor(
                                                         TIER_DESCRIPTOR TierDescriptor,
                                                         ULONG64 RGCapacityInSectors,
                                                         double *pIOPSEstimate);


/*
'DRIVE_TYPE_ORDER' is an integral value that indicates which drive type should be used first.  The 
priority is normal counting order, i.e. 0 followed by 1, etc.
*/
typedef ULONG64 DRIVE_TYPE_ORDER;


/*
'deriveDriveTypeOrder' is a function that generates a DRIVE_TYPE_ORDER based on the passed in 
driveType.  This DRIVE_TYPE_ORDER is passed back in pDriveTypeOrder based on the expected performance of the drive 
type passed in, where better performing drive types are assigned a lower number and should be used
first.
*/
#ifdef __cplusplus
extern "C"
#endif
TIERDEFINITION_API RETURN_RESULT deriveDriveTypeOrder(FLARE_DRIVE_TYPE driveType, DRIVE_TYPE_ORDER *pDriveTypeOrder);

#endif
