//++
//
//      Copyright (C) EMC Corporation, 2003
//
//      All rights reserved.
//
//      Licensed material - Property of EMC Corporation.
//
//--


//++
//
//  Module:
//
//      K10FarAdminExport.h
//
//  Description:
//
//      This file contains the exported definitions needed by software that wishes to interface
//      with the Far Service. This file is used by FAR as well as Navisphere
//      to provide management capabilities of the Driver. This file is also used by the Driver
//      and all internal testing tools.
//
//  Notes:
//
//
//  History:
//
//      7/24/03;    WPH     Created file.
//     10/08/03;    ALT     Added enum shadow lu tag.
//     02/08/05;    WPH     Code Review Changes put in.
//
//--


#ifndef K10_FAR_ADMIN_EXPORT_H
#define K10_FAR_ADMIN_EXPORT_H


//
//  Define the current version of our API.
//

#define FAR_CURRENT_API_REVISION                         1


//
//  Define the atomic name of the FARAdminLib according to the K10 Admin documentation.
//

#define K10_FAR_LIBRARY_NAME             "K10FarAdmin"


//
//	Max update period in seconds = (60 sec * 60 min * 24 hr * 7 days * 4 weeks) = 2419200
//	this is equivalent to the number of seconds in 4 weeks
//
#define MAX_UPDATE_PERIOD_IN_SECONDS    (60 * 60 * 24 * 7 * 4 )  

//
//	default defines for latency, bandwidth and enum rates
//

// The default throttle values have been changed from
// 2,4,7 to 5,7,10 and latency has been changed from
// 16000 to 11000. The changes have been made to allow
// SANCopy to have enough memory to create 50 ISC sessions
// on cx500 arrays.

#define K10_FAR_LATENCY_DEFAULT				11000	 // this is a value in msec
#define K10_FAR_BANDWIDTH_DEFAULT			148000	 // this is Kbytes/sec  
#define K10_FAR_GRANULARITY_DEFAULT			16384	 // this is by default 16 KB ( 16 * 1024 )

#define K10_FAR_SLOW_THROTTLE				5
#define K10_FAR_MED_THROTTLE				7
#define K10_FAR_FAST_THROTTLE				10

//
// In general the below defines are not used but are left for possible backward 
//	compatability
#define K10_FAR_SYNCH_DELAY_SLOW           	1000000
#define K10_FAR_SYNCH_DELAY_MEDIUM         	100  // Currently same as default
#define K10_FAR_SYNCH_DELAY_FAST           	0

//
// IoRole is passed into create mirror function by navi
// Navi passes in a 1 if mirror secondary should accept reads
// and a 0 if mirror secondary should not accept reads
//

enum FAR_BDR_TOGGLE {
    FAR_BDR_TOGGLE_MIN = 0,
    FAR_SECONDARY_DOESNT_ACCEPT_READS = FAR_BDR_TOGGLE_MIN,
    FAR_SECONDARY_ACCEPTS_READS,
    FAR_BDR_TOGGLE_MAX = FAR_SECONDARY_ACCEPTS_READS
};

//
//  Possible Fracture types for Periodic Mirroring.
//	Enum of the different possible types, although not all are currently
//	planned for support in the product.
//

enum FAR_FRACTURE_TYPE {
    FAR_FRACTURE_TYPE_MIN = 0,
    PAUSE_CURRENT_UPDATE  = FAR_FRACTURE_TYPE_MIN,    // currently unsupported
    ABORT_CURRENT_UPDATE,
    FAR_UNUSED_FRACTURE_TYPE,
    FAR_FRACTURE_TYPE_MAX = FAR_UNUSED_FRACTURE_TYPE
};        

//
//  Possible Sync types for Periodic Mirroring.
//	Enum of the different possible types, although not all are currently
//	planned for support in the product.
//

enum FAR_SYNC_TYPE {
    FAR_SYNC_TYPE_MIN = 0,
    START_FULL_UPDATE  = FAR_SYNC_TYPE_MIN,   
    RESUME_CURRENT_UPDATE,
    SCHEDULE_NEXT_UPDATE,
    FAR_SYNC_TYPE_MAX = SCHEDULE_NEXT_UPDATE
};        

//
//  Possible Promote types for Periodic Mirroring.
//	Enum of the different possible types, although not all are currently
//	planned for support in the product.
//	Currently the promote all enum is not supported
enum FAR_PROMOTE_TYPE {
    FAR_PROMOTE_TYPE_MIN = 0,
    NORMAL_PROMOTE  = FAR_PROMOTE_TYPE_MIN,   
    PROMOTE_OOS,
    PROMOTE_LOCAL_IMAGE_ONLY,
    PROMOTE_ALL,
    FAR_PROMOTE_TYPE_MAX = PROMOTE_ALL
};        

//
//  When enumerating mirrors an option field can be specified to indicate
//	what data should be returned.
//	This enumeration provides an indication of what data is to be returned.
//	   
enum ENUM_MIRROR_DATA_FLAG {
    FAR_ENUM_DATA_FLAG_MIN = 0,
    RETURN_LIST_MIRRORS  = FAR_ENUM_DATA_FLAG_MIN,
    RETURN_SPECIFIC_MIRROR_PROPERTIES,
    RETURN_MIRRORS_AND_PROPERTIES,
    RETURN_ALL_MIRRORS_AND_IMAGE_PROPERTIES,
    FAR_ENUM_DATA_FLAG_MAX = RETURN_ALL_MIRRORS_AND_IMAGE_PROPERTIES
}; 

//
//  Update scheduling types enumeration
//	  the types of MAX_DATA_DIFFERENCE 
//	               AVAIL_REPLICATION_CACHE are not currently supported
enum ENUM_FAR_UPDATE_TYPE {
    FAR_UPDATE_ENUM_MIN = 0,
    FAR_UPDATE_TIME_SINCE_LAST_END  = FAR_UPDATE_ENUM_MIN,
    FAR_UPDATE_TIME_SINCE_LAST_START,
    FAR_UPDATE_MAX_DATA_DIFFERENCE,
    FAR_UPDATE_AVAIL_REPLICATION_CACHE,
    FAR_UDPATE_MANUAL,
    FAR_UPDATE_ENUM_MAX = FAR_UDPATE_MANUAL
}; 

#define FAR_UPDATE_TYPE_DEFAULT = FAR_UPDATE_TIME_SINCE_LAST_END
#define FAR_UPDATE_PERIOD_DEFAULT = ( 60 * 60 )		// 1 hour = 60 min * 60 sec/min = 3600 secs

//
//  A speed enumeration for input values
//	  
enum ENUM_FAR_RATE {
    FAR_RATE_ENUM_MIN = 0,
    FAR_RATE_ENUM_SLOW  = FAR_RATE_ENUM_MIN,
    FAR_RATE_ENUM_MEDIUM,
    FAR_RATE_ENUM_FAST,
    FAR_RATE_ENUM_AUTO,     // Currently Unused
    FAR_RATE_ENUM_USER_DEFINED,
    FAR_RATE_ENUM_MAX = FAR_RATE_ENUM_USER_DEFINED
}; 



//
//  Define the IK10Admin database Id's that the FARAdminLib supports. Currently this
//  feature is not widely used but for future modifications created.
//
////////////////////////////////////////////////////////////////////////////////
//
// Database Specifiers. Use in lDbId parameter of Admin interface
//
//
enum K10_FAR_ADMIN_DB {
    K10_FAR_ADMIN_DB_MIN = 0,
    K10_FAR_DB_ID_DEFAULT  = K10_FAR_ADMIN_DB_MIN,
    K10_FAR_ADMIN_DB_MAX = K10_FAR_DB_ID_DEFAULT
};

//
//  These are the Administrative Operations that are understood by FAR.
//  Temporarily adjusted the starting number of the enumeration to remove conflict
//  with the K10 Standard admin operations.
//
typedef enum _FAR_OPERATION_INDEX
{
    FAR_OPERATION_INDEX_MIN = 768,
    FAR_K10_BIND = FAR_OPERATION_INDEX_MIN,
    FAR_K10_UNBIND,                   //
    FAR_K10_REASSIGN,                 // 770
    FAR_K10_QUIESCE,                  //
    FAR_K10_SHUTDOWN,                 // 
    FAR_K10_ENUM_OBJECTS,             // 
    FAR_K10_GET_GENERIC_PROPERTIES,   //
    FAR_K10_PUT_GENERIC_PROPERTIES,   // 775
    FAR_K10_GET_SPECIFIC_PROPERTIES,  //
    FAR_K10_PUT_SPECIFIC_PROPERTIES,  //
    FAR_K10_GET_COMPATIBILITY_MODE,   //
    FAR_K10_PUT_COMPATIBILITY_MODE,   //
    FAR_CREATE_MIRROR,                // 780
    FAR_DESTROY_MIRROR,               //
    FAR_ENUM_MIRRORS,                 //
    FAR_ADD_SECONDARY_IMAGE,          //
    FAR_REMOVE_SECONDARY_IMAGE,       //    
    FAR_GET_MIRROR_PROPERTIES,        // 785
    FAR_SET_MIRROR_PROPERTIES,        //
    FAR_GET_IMAGE_PROPERTIES,         //
    FAR_SET_IMAGE_PROPERTIES,         //
    FAR_REGENERATE_DEVICE_MAP,        // Regenerate devicemap _stackop
    FAR_PROMOTE_SECONDARY,            // 790
    FAR_FRACTURE_SECONDARY,           //
    FAR_SYNCHRONIZE_SECONDARY,        //
    FAR_GET_PROPERTIES,               //
    FAR_SET_PROPERTIES,               //
    FAR_TRESPASS_LU,                  // 795
    FAR_CREATE_SHADOW_LU,             // 796 _stackop
    FAR_DESTROY_SHADOW_LU,            // 797 _stackop
    FAR_ENUM_SHADOW_LU,               //
    FAR_GET_DEVICE_PROPERTIES,        //
    FAR_SET_DEVICE_PROPERTIES,        // 800
    FAR_QUERY_OWNERSHIP_AND_HOLD,     //
    FAR_RELEASE_OWNERSHIP_HOLD,       // 802

    // Enums used for CG Operations in MV/A
    MV_CG_CREATE,                     // 803
    MV_CG_DESTROY,                    //
    MV_CG_ADD_MEMBER,                 // 805
    MV_CG_REMOVE_MEMBER,              //
    MV_CG_FRACTURE,                   //
    MV_CG_SYNC,                       //
    MV_CG_PROMOTE,                    //
    MV_CG_GET_PROPERTIES,             // 810
    MV_CG_SET_PROPERTIES,             //
    FAR_GET_RAW_PERFORMANCE_DATA,     // 812

    //
    //  Backdoor read for secondary LU
    //
    FAR_SET_BACKDOOR_READ,            // 813

    FAR_GET_SIZE_INFO,                // 814
    FAR_SET_SIZE_INFO,                // 815

    FAR_FORCE_EXPANSION,              // 816
    FAR_SET_OPEN_CLONE_QUERY,         // 817   - New for clone of a mirror work
    FAR_GET_OPEN_CLONE_QUERY,         // 818   - New for clone of a mirror work
    FAR_SET_MIRROR_DATA_IN_DRIVER,    // 819   - New for clone of a mirror work
    FAR_GET_MIRROR_DATA_IN_DRIVER,    // 820   - New for clone of a mirror work
    FAR_SET_MIRROR_FLAG_IN_DRIVER,    // 821   - New for clone of a mirror work
    FAR_GET_MIRROR_FLAG_IN_DRIVER,    // 822   - New for clone of a mirror work

    //
    //  Addition for iSCSI transport.
    //
    MV_CONNECTION_UPDATE,             // 823

    //
    //  For the love of God, please make sure you add new operations here and not
    //  in the middle of the above list (which you should well know would
    //  cause interface revision incompatability problems :-)
    //

    // ADD DEBUG/CHECKED ITEMS BELOW HERE. ADD FREE BUILD ITEMS ABOVE HERE!!
    //
    // Add test IOCTLs inside the conditional compilation.  Only use the test
    // IOCTLs in the checked (debug) build.
    //

    #if DBG // Checked (debug) build.

    FAR_TEST_MODIFY_DEBUG_PARAMETERS,  // maybe 817

    #endif  // DBG



    FAR_OPERATION_INDEX_MAX            // 817 or 818 if DBG Build

}   FAR_OPERATION_INDEX, *PFAR_OPERATION_INDEX;

//
//  Define the base value for errors returned to the Admin Interface.
//
#define K10_ERROR_BASE_FAR                               0x71520000

#define FAR_MAKE_KERNEL_ERROR( ErrorNum ) \
    ( ( ErrorNum & 0x0000FFFF ) | K10_ERROR_BASE_FAR )

//++
//
//	Tag definitions have moved to interface\_Tags.h.
//
//	Do not define any new TAG_ literals in this file!
//
//--

#endif // K10_FAR_ADMIN_EXPORT_H
