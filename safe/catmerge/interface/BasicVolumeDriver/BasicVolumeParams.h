//***************************************************************************
// Copyright (C) EMC Corporation 2005-2009
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

///////////////////////////////////////////////////////////
//  BasicVolumeParams.h
//  Definition of the BasicVolumeParams class, which holds
//  the configuration parameters common to all volume types.
//
//  Created on:      26-Jan-2005 4:52:11 PM
///////////////////////////////////////////////////////////

#if !defined(_BasicVolumeParams_h_)
#define _BasicVolumeParams_h_

#include "generic_types.h"
#include "k10defs.h"

// A VolumeIdentifier defines a key that uniquely identifies the volume across all volumes
// within a single Driver.  The BasicVolumeDriver uses the volume WWN as such a key.
typedef  K10_WWID VolumeIdentifier;

// This structure defines the layout of volume configuration parameters. This structure is
// intended to be sub-classed and persisted, and therefore must not have any virtual
// functions.
struct BasicVolumeParams {

public:
    // The unique key that identifies the volume.
    VolumeIdentifier    LunWwn;

    // Identifies the Trespass Gang, which is the unique set of volumes which trespass
    // together. Volumes with the same GangId must trespass together.
    VolumeIdentifier    GangId;

    // The name of the object to expose to other layers.
    CHAR                UpperDeviceName[K10_DEVICE_ID_LEN];

    // If non-NULL, an alternative object name, usually one that allows access via user
    // space.
    CHAR                DeviceLinkName[K10_DEVICE_ID_LEN];
};


struct BasicVolumeDriverParams {    
    CHAR DeviceName[K10_DEVICE_ID_LEN];
    CHAR DeviceLinkName[K10_DEVICE_ID_LEN];
};

//Definition of request/response structures for IOCTL_BVD_GET_VOLUME_DRIVER_PARAMS
//Request param is NULL
//Response param
typedef  BasicVolumeDriverParams BasicVolumeGetVolumeDriverParamsResp;

//Definition of request/response structures for IOCTL_BVD_GET_VOLUME_DRIVER_PARAMS
//Request param
typedef  BasicVolumeDriverParams BasicVolumeSetVolumeDriverParamsRequest;
//Response param is NULL


//Definition of request/response structures for IOCTL_BVD_QUERY_VOLUME_DRIVER_STATUS
typedef enum {
    BVD_DRIVER_STATUS_NORMAL = 0x0,
    BVD_DRIVER_STATUS_INITIALIZING = 0x1,
    BVD_DRIVER_STATUS_ERROR
} BasicVolumeDriverState;

struct BasicVolumeDriverStatus {
   // The maximum number of volumes supported by the driver
    UINT_32 MaxVolumes;

    // The current number of volumes instantiated.
    UINT_32 NumVolumes;

    // The name of the driver.
    char DriverName[K10_DEVICE_ID_LEN];

    // The current state.
    BasicVolumeDriverState DriverState;
    BasicVolumeDriverState Pad; /* SAFEBUG - to keep this 8 byte aligned */

    // Counter, for each driver/volume change.
    UINT_64 DriverChangeCount;

    // The driver change count below which deleted volumes cannot be reported.
    UINT_64 VolumeDeletionHistoryChangeCount;
    
    // Currently committed level. This may be zero if persistence isn't used.
    UINT_32 CurrentCommitLevel;
    
    // The maximum supported commit level by this driver.
    UINT_32 SupportedCommitLevel;
};  

struct BasicVolumeWwnChange {
    // The old WWN, which was the key for the existing volume
    VolumeIdentifier OldWwn;

    // The new WWN, must not already exist
    VolumeIdentifier NewWwn;
};

// Defines the conditions under which IOCTL_BVD_QUERY_VOLUME_STATUS operation 
// will complete.  If there are no input parameters, the IOCTL will complete immediately.
// If input parameters are specified, the IOCTL will complete when the conditions
// are met or it is cancelled.
struct BasicDriverStatusFilter {
     // Status IOCTL will complete as soon as BasicVolumeDriverStatus::DriverChangeCount
     // does not equal this value. 
     UINT_64 LastDriverChangeCount;

};

//Request param is NULL
//Response param
typedef  BasicVolumeDriverStatus BasicVolumeQueryVolumeDriverStatusResp;

// On this boot, how was the volume initially created on this SP?
enum BasicVolumeCreationMethod { BvdVolumeCreate_Unknown=0, BvdVolumeCreate_Registry=1, 
                                 BvdVolumeCreate_AutoInsert=2, BvdVolumeCreate_PeristenceIoctl = 3 };

//Definition of response structures for IOCTL_BVD_QUERY_VOLUME_STATUS
struct BasicVolumeStatus {
    // Counts the (# opens) - (# closes)
    UINT_32                       NumOpens;

    // Changes whenever volume params or the volume status changes.
    UINT_32                       VolumeTimeStamp;

    // On this boot, how was the volume initially created on this SP?
    BasicVolumeCreationMethod   HowCreated;

    // Are the volume parameters currently persisted?
    bool                        ParametersPersisted;

    // Any changes in the volume wil increment the count.
    UINT_64               LastVolumeChangeDriverChangeCount;

    // When was the volume created?
    UINT_64               VolumeCreationDriverChangeCount; 

    // Provision for future change.
    UINT_32                        Spare[2];
};

struct BasicAction {

    enum ActionType{ 
       ActionAllSPs  = 1,    // data returned, if any is from issuing SP
       ActionThisSP = 2,    // Event functions only called on the issuing SP
       ActionSpecificSP     // Event functions only called on single SP.
    } ActionKind;
 
    UINT_32                  SPEngineNumber;  // which SP (0 => A, 1 =>B), if ActionSpecificSP 
    
};

//Definition of response structures for IOCTL_BVD_PERFORM_DRIVER_ACTION
struct BasicDriverAction : public BasicAction {
    
};

//Definition of response structures for IOCTL_BVD_PERFORM_VOLUME_ACTION
struct BasicVolumeAction : public BasicAction {
    // Volume Identifier
    VolumeIdentifier    LunWWN;  

   
};

// Define structure for possible future volume statistics
#define BasicVolumeStatisticsSpares (4)
struct BasicVolumeStatistics {
    UINT_32 spares[BasicVolumeStatisticsSpares];
};  

// Define structure for possible future volume driver statistics
#define BasicVolumeDriverStatisticsSpares (4)
struct BasicVolumeDriverStatistics {
    UINT_32 spares[BasicVolumeDriverStatisticsSpares];
};  

// Define structure for BVD commit
struct BasicVolumeDriverCommitParams {
    UINT_32 NewCommitLevel;
};

//Request
typedef VolumeIdentifier BasicVolumeQueryVolumeStatusRequest;

//Response
typedef BasicVolumeStatus BasicVolumeQueryVolumeStatusResp;

//Request
typedef VolumeIdentifier BasicVolumeGetVolumeStatisticsRequest;

//Response
typedef BasicVolumeStatistics BasicVolumeGetVolumeStatisticsResp;

//Response
typedef BasicVolumeDriverStatistics BasicVolumeGetVolumeDriverStatisticsResp;

// Definition of request structure for IOCTL_BVD_CHANGE_LUN_WWN
//Request 
typedef BasicVolumeWwnChange BasicVolumeWwnChangeRequest;

//Definition of request/response structures for IOCTL_BVD_GET_VOLUME_PARAMS
//Request
typedef VolumeIdentifier BasicVolumeGetVolumeParamsRequest;

//Response
typedef BasicVolumeParams BasicVolumeGetVolumeParamsResp;

//Definition of request/response structures for IOCTL_BVD_SET_VOLUME_PARAMS
//Request
typedef BasicVolumeParams BasicVolumeSetVolumeParamsRequest;
//Response is NULL


//Definition of request/response structures for IOCTL_BVD_DESTROY_VOLUME
//Request
typedef VolumeIdentifier BasicVolumeDestroyVolumeRequest;
//Response is NULL

//Definition of request/response structures for IOCTL_BVD_CREATE_VOLUME
//Request
typedef BasicVolumeParams BasicVolumeCreateVolumeRequest;
//Response is NULL

//Definition of request/response structures for IOCTL_BVD_ENUM_VOLUMES
struct BasicVolumeEnumVolumesResp {
    UINT_32 VolCount;
    VolumeIdentifier VolId[1];
};

// A filter driver, where there is a 1:1 relationship with a device below, is an type of
// BasicVolume, with support in BVD.
struct  FilterDriverVolumeParams : public BasicVolumeParams
{
     CHAR                LowerDeviceName[K10_DEVICE_ID_LEN];
};


#endif // !defined(_BasicVolumeParams_h_)
