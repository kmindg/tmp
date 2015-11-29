//++
//
//      Copyright (C) EMC Corporation, 1998 - 2007
//      All rights reserved.
//      Licensed material - Property of EMC Corporation.
//
//--


//++
//
//  Module:
//
//      K10RemoteMirrorAdminExport.h
//
//  Description:
//
//      This file contains the exported definitions needed by software that wishes to interface
//      with the Remote Mirror driver. This file is used by the RemoteMirrorAdminLib as well as
//      Navisphere to provide management capabilities of the Driver. This file is also used by 
//      the Driver and all internal testing tools.
//
//--


#ifndef K10_RM_ADMIN_EXPORT_H
#define K10_RM_ADMIN_EXPORT_H


//
//  Define the atomic name of the RMAdminLib according to the K10 Admin documentation.
//

#define K10_REMOTEMIRROR_LIBRARY_NAME             "K10RemoteMirrorAdmin"


//
//  Definition for the "data" for setting the Sync rate.
//
typedef enum
{
      RmSyncRateHigh = 0,
      RmSyncRateMedium = 1,
      RmSyncRateLow = 2,
      RmSyncRateUserDefined = 3
}  RM_SYNC_RATE, *PRM_SYNC_RATE;

//
//  Define the IK10Admin database Id's that the RMAdminLib supports. Currently this
//  feature is not widely but for future modifications created.
//
////////////////////////////////////////////////////////////////////////////////
//
// Database Specifiers. Use in lDbId parameter of Admin interface
//
//
enum K10_REMOTE_MIRROR_ADMIN_DB {
    K10_REMOTE_MIRROR_ADMIN_DB_MIN = 0,
    K10_REMOTEMIRROR_DB_ID_DEFAULT  = K10_REMOTE_MIRROR_ADMIN_DB_MIN,
//  K10_REMOTEMIRROR_DB_ID_REV1
//  K10_REMOTEMIRROR_DB_ID_REV2,
//  K10_REMOTEMIRROR_DB_ID_REV3,
//  K10_REMOTEMIRROR_DB_ID_REV4,
    K10_REMOTE_MIRROR_ADMIN_DB_MAX = K10_REMOTEMIRROR_DB_ID_DEFAULT
};

//
// The following defines whether or not the ability to read a LU is allowed
// on the secondary image.  A value of 1 indicates that it is possible
// to allow reads on the secondary.  A value of 0 indicates that reads on
// the secondary are not possible.
//
// If this is set to 0 then check for breaking compatibility between free
// admin dll and checked driver (and vice versa) because of the entry
// for RM_MODIFY_SECONDARY_READ_STATUS in the RM_OPERATION_INDEX!!
//
#define RM_SECONDARY_READS_ALLOWED                  1



//
//  The Remote Mirror Mirror State indicates the actual state of a mirror instantiation.
//
//  RM_MIRROR_STATE_INACTIVE
//      This state indicates that the mirror is inactive and it will not process any
//      host write -or- read requests.
//
//  RM_MIRROR_STATE_ACTIVE
//      This state indicates that the mirror is active and functioning.
//
//  RM_MIRROR_STATE_ATTENTION
//      This state indicates that there is a problem with the mirror that requires
//      administrative intervention for corrective procedures.
//
typedef enum _RM_MIRROR_STATE
{
    RM_MIRROR_STATE_INVALID = -1,
    RM_MIRROR_STATE_INACTIVE,
    RM_MIRROR_STATE_ACTIVE,
    RM_MIRROR_STATE_ATTENTION,

    //  Add new values here...

    RM_MIRROR_STATE_MAX

}   RM_MIRROR_STATE, *PRM_MIRROR_STATE;


//
//  The Remote Mirror Image State indicates the relationship between the data stored upon
//  a slave image and that slave's master image.
//
//  RM_IMAGE_STATE_OUT_OF_SYNC
//      This state indicates that no known relationship can be established between the
//      slave image and its master image.
//
//  RM_IMAGE_STATE_IN_SYNC
//      This state indicates that the slave image is an exact block-for-block duplicate
//      of its master image.
//
//  RM_IMAGE_STATE_CONSISTENT
//      This state indicates that the slave image is an exact block-for-block duplicate
//      of the master at some point in the past.
//
//  RM_IMAGE_STATE_SYNCING
//      This state indicates that the slave image is currently being synchronized with the
//      master image.
//
//  RM_IMAGE_STATE_INCONSISTENT
//      This state indicates that the Clone Source is currently being synchronized by MirrorView driver.
//
typedef enum _RM_IMAGE_STATE
{
    RM_IMAGE_STATE_INVALID = -1,
    RM_IMAGE_STATE_OUT_OF_SYNC,
    RM_IMAGE_STATE_IN_SYNC,
    RM_IMAGE_STATE_CONSISTENT,
    RM_IMAGE_STATE_SYNCING,

#ifdef CLONES

    RM_IMAGE_STATE_REVERSE_OUT_OF_SYNC,
    RM_IMAGE_STATE_REVERSE_SYNCING,
    RM_IMAGE_STATE_INCONSISTENT,

#endif //CLONES

    //  Add new values here...

    RM_IMAGE_STATE_MAX

}   RM_IMAGE_STATE, *PRM_IMAGE_STATE;



//
//  Type:           RM_CONFIG_IMAGE_CONDITION
//
//  Description:    This enumeration is a list of the possible image conditions
//                  as perceived by the Configuration Manager.
//
//                                             Mirror Manager
//        +----------------------------------- Synchronization -----+
//        |                                       Complete          |
//        |                                                 +---------------+
//        |                                             +---| Synchronizing |--+
//        |                                             |   +---------------+  |
//        |                                             |           ^          |
//        |                 +-------- Administrative ---+----+      |          |
//        |                 |             Fracture           |  Scheduled      |
//        |                 v                                |      |          |
//        |           +---------------+                    +-----------------+ |
//        |        +->| Administrator |- Administrative -->| Synchronization |-+
//        |        |  |   Fracture    |  Synchronization   |      Queue      | |
//        |        |  +---------------+                    +-----------------+ |
//        |        |        ^                                       ^          |
//        | Administrative  |                                       |          |
//        |   Fracture or   |                                       |          |
//        |  Media Failure  |                                       |          |
//        |        |        |                                       |          |
//        |        |  Administrative                                |          |
//        v        |     Fracture                                   |          |
//    +--------+   |        |                                       |*         |
//    | Normal |---+        +---------------------------------------+--------+ |
//    +--------+   |        |                                       |        | |
//        ^        |        |      +---- System Synchronization ----+        | |
//        |        |        |      |                                |        | |
//        |        |        | Failover Manager                      |        | |
//        | Failover Manager|   Reachable                    Administrative  | |
//        |   Unreachable   |  Notification                  Synchronization | |
//        |   Notification  |      |                                |        | |
//        |        |        |      |                                |        | |
//        |        |  +----------+ |                    +----------------+   | |
//   +---------+   |  |  System  | | Failover Manager   |   Waiting On   |---+ |
// +-| Unknown |---+->| Fracture |-+--- Reachable ----->|  Administrator |-----+
// | +---------+      +----------+ |   Notification     +----------------+     |
// |      ^                ^       |                                           |
// |      |*               |*      |                                           |
// +------+----------------+-------+                                           |
//        |                |       |*                    Failover Manager      |
//        |                +-------+---------------------- Unreachable  -------+
//        |                        |                       Notification
//      Start                      |
//                           Decision Based
//                                 on
//                           Recovery Policy
//
//      * Indicates that transitions overlap for drawing purposes.  There is no
//        semantic meaning associated with the specified transition, it is just
//        a limitation of ASCII drawing.
//
//      Doesn't this drawing make things crystal clear now?
//
//
//  Members:
//      RmConfigInvalidImageCondition
//          The lower bounds enumeration terminator.
//
//      RmConfigUnknownImageCondition
//          The image condition for the specified image is unknown.  This state
//          is used by the master when the mirror is forming.
//
//     RmConfigMasterImageCondition
//          The image is the master of the mirror.  This image is ignored during
//          processing.
//
//      RmConfigNormalImageCondition
//          The image is "normal" as far as the Configuration Manager is
//          concerned.  This means that there are no system or administrative
//          fractures or synchronizations occurring.  Normal I/O is allowed.
//
//      RmConfigAdminFracImageCondition
//          The image has been administratively fractured.  Notifications
//          from the Failover Manager regarding the reachability are ignored
//          for this image.
//
//      RmConfigSynchQueueImageCondition
//          The image has been placed in the internal Configuration Manager
//          synchronization queue, but it has not been started by the Mirror
//          Manager.  It must be scheduled first.
//
//      RmConfigSynchingImageCondition
//          The image is in the process of being synchronized.  This can be the
//          result of an administrative operation or automatically by Remote
//          Mirroring.  This condition must correspond to the
//          RM_IMAGE_STATE_SYNCING image state.
//
//      RmConfigWaitingOnAdminImageCondition
//          The image is now reachable after previously being unreachable.  The
//          Recovery Policy for the image is set to administrator synchronize.
//          So the Configuration Manager does not automatically start a
//          synchronization when the image becomes reachable.  We are in a state
//          where we wait on the administrator to start a synchronization.
//
//      RmConfigSysFracImageCondition
//          The system (Remote Mirroring) has fractured the image automatically
//          because the image became unreachable.
//
//      RmConfigMaximumImageCondition
//          The upper bounds enumeration terminator.
//
typedef enum
{
    RmConfigInvalidImageCondition   = -1,

    RmConfigUnknownImageCondition,

    RmConfigMasterImageCondition,

    RmConfigNormalImageCondition,

    RmConfigAdminFracImageCondition,

    RmConfigSynchQueueImageCondition,

    RmConfigSynchingImageCondition,

    RmConfigWaitingOnAdminImageCondition,

    RmConfigSysFracImageCondition,

    RmConfigMaximumImageCondition

} RM_CONFIG_IMAGE_CONDITION, *PRM_CONFIG_IMAGE_CONDITION;


//
//  To simplify setting the Synch Delay will provide a method for predefined settings
//  Testing should provide what good settings will be for the different enums which
//  are defined below and can be changed as necessary
//  The User_defined setting allows for a numerical input of the synch delay
//
enum RM_SYNCH_DELAY_TYPE {
    RM_SYNCH_DELAY_TYPE_MIN = 0,
    FAST  = RM_SYNCH_DELAY_TYPE_MIN,
    MEDIUM,
    SLOW,
    USER_DEFINED,
    RM_SYNCH_DELAY_TYPE_MAX = USER_DEFINED
};

//
// Enumeration of CG Promote values used with specifying TAG_RM_PROMOTE_TYPE
//
enum CG_PROMOTE_TYPE {
    CG_PROMOTE_TYPE_MIN = 0,
    CG_PROMOTE_NORMAL  = CG_PROMOTE_TYPE_MIN,
    CG_PROMOTE_FORCE,
    CG_PROMOTE_LOCAL_ONLY,
	CG_PROMOTE_OOS,
    CG_PROMOTE_TYPE_MAX = CG_PROMOTE_OOS
};

//
//  These are the Administrative Operations that are understood by the Remote
//  Mirrors Driver.
//  Temporarily adjusted the starting number of the enumeration to remove conflict
//  with the K10 Standard admin operations.
//
typedef enum _RM_OPERATION_INDEX
{
    RM_OPERATION_INDEX_MIN = 512,
    K10_BIND = RM_OPERATION_INDEX_MIN,
    K10_UNBIND,
    K10_ENUMERATE_OBJECTS,
    K10_GET_GENERIC_PROPS,      // 515
    K10_PUT_GENERIC_PROPS,
    K10_GET_SPECIFIC_PROPS,
    K10_PUT_SPECIFIC_PROPS,
    K10_GET_COMPAT_MODE,
    K10_PUT_COMPAT_MODE,        // 520
    RM_CREATE_MIRROR,
    RM_DESTROY_MIRROR,
    RM_ENUM_MIRRORS,
    RM_ADD_SLAVE,
    RM_REMOVE_SLAVE,            // 525
    RM_GET_MIRROR_PROPERTIES,
    RM_SET_MIRROR_PROPERTIES,
    RM_GET_IMAGE_PROPERTIES,
    RM_SET_IMAGE_PROPERTIES,
    RM_ENUM_UNUSED_OBJECTS,     // 530
    RM_PROMOTE_SLAVE,
    RM_FRACTURE_SLAVE,
    RM_SYNCHRONIZE_SLAVE,
    RM_SET_DRIVER_PROPERTIES,
    RM_GET_DRIVER_PROPERTIES,   // 535
    RM_K10_QUIESCE,
    RM_K10_SHUTDOWN,
    RM_K10_REASSIGN,            // 538
	RM_GET_RAW_PERFORMANCE_DATA,  	// 539

	//
    // If allowed, define the secondary read IOCTL.  This allows read operations
    // on the secondary.  It is turned off by default and must be turned on after boot.
    //	
	#if RM_SECONDARY_READS_ALLOWED // This has always been defined
		RM_MODIFY_SECONDARY_READ_STATUS, 	// 540
	#endif // RM_SECONDARY_READS_ALLOWED

	// 
	// These are reserved for internal use of the driver to detect legacy debug op
	// This is because two IOCTLS under '#if DBG' once preceeded these. 
	//
	RM_GET_RAW_PERFORMANCE_DATA_DBG_INTERNAL = 541,	
	#if RM_SECONDARY_READS_ALLOWED // This has always been defined
		RM_MODIFY_SECONDARY_READ_STATUS_DBG_INTERNAL, 
	#endif // RM_SECONDARY_READS_ALLOWED

	//
	// Enum value is assigned to start from a clear, fixed number
	//
	RM_CG_CREATE = 543,	
	RM_CG_DESTROY,
	RM_CG_ADD_MEMBER,
	RM_CG_REMOVE_MEMBER, 
	RM_CG_FRACTURE, 
	RM_CG_SYNC, 
	RM_CG_PROMOTE, 
    RM_CG_GET_PROPERTIES, 
	RM_CG_SET_PROPERTIES,

    RM_GET_IMAGE_LU_PROPERTIES,

    //
    // Enum values introduced by Delta Polling
    // 
    K10_RM_POLL,
    RM_POLL_GET_CONTAINER_SIZE,
    RM_POLL_RETRIEVE_DATA,
    RM_RESET_IOSTATS,
    RM_GET_MIRROR_TRESPASS_LICENSE,

    // For Unity environment only
#ifndef ALAMOSA_WINDOWS_ENV
    RM_ENABLE_CMI_OVER_FC,
#endif //  ALAMOSA_WINDOWS_ENV


	//
    // IMPORTANT: For the sake of compatibility, any new operations would go here...  
    //
	
	// RM_YOUR_NEXT_OP_HERE!

	//
	// Conditional Debug IOCTLS are now at the end of the 512-767 range,
	// where their conditional presense will not effect other operation values.
	// Only use the test IOCTLs in the checked (debug) build.
    //
#if DBG // Checked (debug) build.
        RM_TEST_FAKE_LOST_CONTACT = 720,
        RM_TEST_MODIFY_DEBUG_PARAMETERS

#endif  // DBG
}   RM_OPERATION_INDEX, *PRM_OPERATION_INDEX;



//
//  Define the base value for errors returned to the Admin Interface.
//  Value was assigned by Dave Zeryck, Czar of Admin.
//

#define K10_ERROR_BASE_RM                               0x71050000


//
//	All TAG_* definitions have now moved to interface\_Tags.h.
//	Do not define any new tags here!
//

#endif // K10_RM_ADMIN_EXPORT_H
