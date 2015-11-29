//++
//
//  Module:
//
//      K10SnapCopyAdminExport.h
//
//  Description:
//
//      This file contains the exported definitions needed by software that wishes to interface
//      with the SnapCopy driver. This file is used by the SnapCopyAdminLib as well as Navisphere
//      to provide management capabilities of the Driver. This file is also used by the Driver
//      and all internal testing tools.
//
//  Notes:
//
//      None.
//
//  History:
//
//      9-Dec-98    PTM;    Created file.
//     26-Sep-00    BY      Task 1774 - Cleanup the header files
//
//--


#ifndef K10_SNAPCOPY_ADMIN_EXPORT_H
#define K10_SNAPCOPY_ADMIN_EXPORT_H

//
//  Define the current version of the api by which SnapView and the administrative tools
//  such as admsnap and Navi. This has been reworked to include the possible values for
//  all released software and to allow enough definition for the driver and admsnap to
//  perform the correct functions.
//

typedef enum _K10_SNAPCOPY_REVISION
{
    K10_SC_API_REV_LB1_THRU_LB2 = 1,

    K10_SC_API_REV_LB3,

    K10_SC_API_REV_LB4,

    K10_SC_API_REV_4

} K10_SNAPCOPY_REVISION;


#define K10_SC_CURRENT_API_REVISION             K10_SC_API_REV_LB4

//
// Define the current API Revision needed by Admsnap.  
//
#define K10_SC_CURRENT_ADMSNAP_API_REVISION     K10_SC_API_REV_4


//
//  Define the atomic name of the K10SnapCopyAdminLib according to the K10 Admin documentation.
//

#define K10_SNAPCOPY_LIBRARY_NAME               "K10SnapCopyAdmin"


//
//  Define the IK10Admin database Id's that the SnapCopyAdminLib supports. Currently this
//  feature is not widely used so all commands sent to the library use the default id.
//

typedef enum _K10_SNAPCOPY_DB_ID
{
    K10_SNAPCOPY_DB_ID_MIN                      = 0,
    
    K10_SNAPCOPY_DB_ID_DEFAULT                  = K10_SNAPCOPY_DB_ID_MIN,
    
    K10_SNAPCOPY_DB_ID_MAX                      = K10_SNAPCOPY_DB_ID_DEFAULT

} K10_SNAPCOPY_DB_ID;


//
//  Define the base value for errors returned to the Admin Interface.
//  Value was assigned by Dave Zeryck, Czar of Admin.
//

#define K10_ERROR_BASE_SNAPCOPY                 0x71000000


//
//  Define an enumeration that represents the op code values of all of our IOCTL commands.
//  These values are used to generate the real IOCTL command value and are used by callers of
//  the SnapCopyAdminLib to indicate which function is to be performed.
//
//  We start this enumeration with a value far away from those defined in K10DiskDriverAdmin.h
//  which contains the OpCode values that all layered drivers must support.
//

typedef enum _K10_SNAPCOPY_OPC
{
    //
    //  Configuration Management Operations.
    //
    
    K10_SNAPCOPY_OPC_MIN                        = 1024,

    K10_SNAPCOPY_OPC_GET_CONFIG_INFO            = K10_SNAPCOPY_OPC_MIN,
    K10_SNAPCOPY_OPC_PUT_CONFIG_INFO,           //1025
    K10_SNAPCOPY_OPC_CONFIG_GET_VERSION,        //1026
    
    K10_SNAPCOPY_OPC_CONFIG_INFO_RES2,          //1027
    K10_SNAPCOPY_OPC_CONFIG_INFO_RES3,          //1028
    K10_SNAPCOPY_OPC_CONFIG_INFO_RES4,          //1029
                                 

    //
    //  Cache Management Operations.
    //
    K10_SNAPCOPY_OPC_CACHE_ADD,                 //1030
    K10_SNAPCOPY_OPC_CACHE_REMOVE,              //1031
    K10_SNAPCOPY_OPC_CACHE_STATUS,              //1032
    K10_SNAPCOPY_OPC_CACHE_ENUM_OBJECTS,        //1033

    K10_SNAPCOPY_OPC_CACHE_RES1,                //1034
    K10_SNAPCOPY_OPC_CACHE_RES2,                //1035
    K10_SNAPCOPY_OPC_CACHE_RES3,                //1036
    K10_SNAPCOPY_OPC_CACHE_RES4,                //1037


    //
    //  Session Management Operations.
    //
    K10_SNAPCOPY_OPC_SESSION_START,             //1038
    K10_SNAPCOPY_OPC_SESSION_STOP,              //1039
    K10_SNAPCOPY_OPC_SESSION_STATUS,            //1040
    K10_SNAPCOPY_OPC_SESSION_ENUM_OBJECTS,      //1041


    K10_SNAPCOPY_OPC_SESSION_ROLL_BACK,         //1042

    K10_SNAPCOPY_OPC_SESSION_SET_RB_FLUSH_DELAY,//1043


    K10_SNAPCOPY_OPC_SESSION_UNDEFER_ROLLBACK,  //1044
    K10_SNAPCOPY_OPC_SESSION_RES4,              //1045


    //
    //  SnapLu Management Operations.
    //
    K10_SNAPCOPY_OPC_SNAPLU_CREATE,             //1046
    K10_SNAPCOPY_OPC_SNAPLU_REMOVE,             //1047
    K10_SNAPCOPY_OPC_SNAPLU_STATUS,             //1048
    K10_SNAPCOPY_OPC_SNAPLU_ENUM_OBJECTS,       //1049
    K10_SNAPCOPY_OPC_SNAPLU_ACTIVATE,           //1050
    K10_SNAPCOPY_OPC_SNAPLU_DEACTIVATE,         //1051
    
    K10_SNAPCOPY_OPC_SNAPLU_REMOVE_NO_STACK_OP, //1052
    K10_SNAPCOPY_OPC_SNAPLU_RES4,               //1053


    //
    //  Read/Write Buffer Managment Operations ( INTERNAL ).
    //
    K10_SNAPCOPY_OPC_READ_ERROR,                //1054
    K10_SNAPCOPY_OPC_READ_SESSION_NAME,         //1055
    K10_SNAPCOPY_OPC_GET_CURRENT_API_REVISION,  //1056

    K10_SNAPCOPY_OPC_READ_RES1,                 //1057
    K10_SNAPCOPY_OPC_READ_RES2,                 //1058
    K10_SNAPCOPY_OPC_READ_RES3,                 //1059
    K10_SNAPCOPY_OPC_READ_RES4,                 //1060

    //
    // Performance Monitor IOCTL.
    //

    K10_SNAPCOPY_OPC_GET_RAW_PERFORMANCE_DATA,  //1061

    //
    //  TargetLu Management Operations
    //
    K10_SNAPCOPY_OPC_TARGET_STATUS,             //1062
    K10_SNAPCOPY_OPC_TARGET_ENUM_OBJECTS,       //1063


    //
    //  Incremental SnapView Session Operations.
    //
    K10_SNAPCOPY_OPC_SESSION_MARK,              //1064
    K10_SNAPCOPY_OPC_SESSION_UNMARK,            //1065

    K10_SNAPCOPY_OPC_SESSION_CLEAR_ALL_BITS,    //1066
    K10_SNAPCOPY_OPC_SESSION_SET_GRANULARITY,   //1067

    K10_SNAPCOPY_OPC_SESSION_SET_COPY_TYPE,     //1068

    K10_SNAPCOPY_OPC_COPY_START,                //1069
    K10_SNAPCOPY_OPC_COPY_COMPLETE,             //1070
    
    K10_SNAPCOPY_OPC_CHUNK_GET_NEXT,            //1071
    K10_SNAPCOPY_OPC_CHUNK_CLEAR,               //1072

    K10_SNAPCOPY_OPC_SESSION_GET_GRANULARITY,   //1073

    //
    //  Multi-Lun Consistent Session Operations.
    //
    K10_SNAPCOPY_OPC_CONSISTENT_SESSIONS_MARK,  //1074
    K10_SNAPCOPY_OPC_CONSISTENT_SESSION_START,  //1075

    //
    // LU Expansion Test Operation
    //
    K10_SNAPCOPY_OPC_TEST_VSM_MIGRATE_MAPS,     //1076

    K10_SNAPCOPY_OPC_QUERY_SNAP_DEVICE_TYPE,    //1077

    K10_SNAPCOPY_OPC_GET_ACTIVE_OPS,            //1078

    K10_SNAPCOPY_OPC_GET_LU_STATUS,             //1079

    K10_SNAPCOPY_OPC_END_OF_ENUMERATION,        //1080

    K10_SNAPCOPY_OPC_SET_MIN_MEMORY_THRESHOLD_INCREMENT,//1081

    K10_SNAPCOPY_OPC_SET_MEMORY_GUARD,          //1082

    K10_SNAPCOPY_OPC_GET_ALL_MEMORY_PARAMETERS, //1083
    
    K10_SNAPCOPY_OPC_SET_ALL_MEMORY_PARAMETERS, //1084

    K10_SNAPCOPY_OPC_GOBBLE_MEMORY,             //1085

    K10_SNAPCOPY_OPC_RELEASE_GOBBLED_MEMORY,    //1086

    K10_SNAPCOPY_OPC_SET_MEMORY_GUARD_TIMER,    //1087

    //
    // Paging Debug Operations
    //
    K10_SNAPCOPY_OPC_BYPASS_LRU_QUEUE,          //1088

    K10_SNAPCOPY_OPC_BYPASS_MEM_SUBSYSTEM,      //1089

    K10_SNAPCOPY_OPC_DRAIN_LRU_QUEUE,           //1090

    //
    //  Single SP management get operations
    //
    K10_SNAPCOPY_OPC_GET_FULL_DATA_TREE,        //1091

    K10_SNAPCOPY_OPC_GET_PRUNED_DATA_TREE,      //1092

    K10_SNAPCOPY_OPC_GET_REDUCED_VERSION,       //1093

    K10_SNAPCOPY_OPC_ALL_SNAPLU_STATUSES,       //1094

    K10_SNAPCOPY_OPC_ALL_SESSION_STATUSES,      //1095

    //
    //  Delta Polling commands.
    //
    K10_SNAPCOPY_OPC_POLL_REQUEST,                      //1096

    K10_SNAPCOPY_OPC_POLL_REQUEST_GET_DELTA_TOKEN,      //1097

    K10_SNAPCOPY_OPC_DELTA_POLL_GET_DELETED_OBJECTS,    //1098

    K10_SNAPCOPY_OPC_DELETE_SNAPSHOT_PSM,               //1099

    K10_SNAPCOPY_OPC_DELETE_CACHE_PSM,                  //1100

    K10_SNAPCOPY_OPC_DELETE_SESSIONS_PSM,               //1101

    K10_SNAPCOPY_OPC_DELETE_TARGET_PSM,                 //1102

    //220177
    //IOCTLS to be used for locking a target list while doing session starts
    K10_SNAPCOPY_OPC_SESSION_START_LIST_ADD_REMOVE,     //1103

    K10_SNAPCOPY_OPC_ROLLBACK_ENQUIRE_LUS_PROGRESS,          //1104 

    K10_SNAPCOPY_OPC_MAX = K10_SNAPCOPY_OPC_ROLLBACK_ENQUIRE_LUS_PROGRESS

} K10_SNAPCOPY_OPC;


#define K10_SNAPCOPY_OPC_SNAPBACK_TEST 1420
//++
//
//  Define the device type value.  Note that values used by Microsoft
//  Corporation are in the range 0-32767, and 32768-65535 are reserved for use
//  by customers.
//
//--

#define FILE_DEVICE_SNAPCOPY                    45000


//++
//
//  The SnapCopy device driver IOCTLs and their helper macros.  
//
//--
#define SC_CTL_CODE( DeviceType, Function, Method, Access )    \
        ( ( ( DeviceType ) << 16 ) | ( ( Access ) << 14 ) | ( ( Function ) << 2 ) | ( Method ) )


#define SC_BUILD_IOCTL( _IoctlValue_ )          SC_CTL_CODE( FILE_DEVICE_SNAPCOPY,             \
                                                             _IoctlValue_,                     \
                                                             EMCPAL_IOCTL_METHOD_BUFFERED,                  \
                                                             EMCPAL_IOCTL_FILE_ANY_ACCESS )


#define IOCTL_SC_GET_CONFIG_INFO                SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_GET_CONFIG_INFO )
#define IOCTL_SC_PUT_CONFIG_INFO                SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_PUT_CONFIG_INFO )
#define IOCTL_SC_GET_VERSION                    SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_CONFIG_GET_VERSION )


#define IOCTL_SC_CACHE_ADD                      SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_CACHE_ADD )
#define IOCTL_SC_CACHE_REMOVE                   SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_CACHE_REMOVE )
#define IOCTL_SC_CACHE_STATUS                   SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_CACHE_STATUS )
#define IOCTL_SC_CACHE_ENUM_OBJECTS             SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_CACHE_ENUM_OBJECTS )

#define IOCTL_SC_SESSION_START_ROLLBACK         SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_SESSION_ROLL_BACK )

#define IOCTL_SC_SESSION_SET_RB_FLUSH_DELAY     SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_SESSION_SET_RB_FLUSH_DELAY )

#define IOCTL_SC_SESSION_UNDEFER_ROLLBACK       SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_SESSION_UNDEFER_ROLLBACK )

#define IOCTL_SC_TARGET_STATUS                  SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_TARGET_STATUS )

#define IOCTL_SC_TARGET_ENUM_OBJECTS            SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_TARGET_ENUM_OBJECTS )

#define IOCTL_SC_SESSION_START                  SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_SESSION_START )
#define IOCTL_SC_SESSION_STOP                   SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_SESSION_STOP )
#define IOCTL_SC_SESSION_STATUS                 SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_SESSION_STATUS )
#define IOCTL_SC_SESSION_ENUM_OBJECTS           SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_SESSION_ENUM_OBJECTS )


#define IOCTL_SC_SNAPLU_CREATE                  SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_SNAPLU_CREATE )
#define IOCTL_SC_SNAPLU_REMOVE                  SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_SNAPLU_REMOVE )
#define IOCTL_SC_SNAPLU_STATUS                  SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_SNAPLU_STATUS )
#define IOCTL_SC_SNAPLU_ENUM_OBJECTS            SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_SNAPLU_ENUM_OBJECTS )
#define IOCTL_SC_SNAPLU_ACTIVATE                SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_SNAPLU_ACTIVATE )
#define IOCTL_SC_SNAPLU_DEACTIVATE              SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_SNAPLU_DEACTIVATE )


#define IOCTL_SC_GET_RAW_PERFORMANCE_DATA       SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_GET_RAW_PERFORMANCE_DATA )


#define IOCTL_SC_SESSION_MARK                   SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_SESSION_MARK )
#define IOCTL_SC_SESSION_UNMARK                 SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_SESSION_UNMARK )  
#define IOCTL_SC_SESSION_SET_COPY_TYPE          SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_SESSION_SET_COPY_TYPE )
#define IOCTL_SC_CLEAR_ALL_BITS                 SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_SESSION_CLEAR_ALL_BITS )
#define IOCTL_SC_COPY_SET_GRANULARITY           SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_SESSION_SET_GRANULARITY )
#define IOCTL_SC_COPY_GET_GRANULARITY           SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_SESSION_GET_GRANULARITY )

#define IOCTL_SC_COPY_START                     SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_COPY_START )
#define IOCTL_SC_COPY_COMPLETE                  SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_COPY_COMPLETE )

#define IOCTL_SC_CHUNK_GET_NEXT                 SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_CHUNK_GET_NEXT )
#define IOCTL_SC_CHUNK_CLEAR                    SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_CHUNK_CLEAR )


#define IOCTL_SC_CONSISTENT_SESSIONS_MARK       SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_CONSISTENT_SESSIONS_MARK )

#define IOCTL_SC_TEST_VSM_MIGRATE_MAPS          SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_TEST_VSM_MIGRATE_MAPS )


#define IOCTL_SC_CONSISTENT_SESSION_START       SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_CONSISTENT_SESSION_START )

#define IOCTL_SC_TEST_VSM_MIGRATE_MAPS          SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_TEST_VSM_MIGRATE_MAPS )

#define IOCTL_SC_QUERY_SNAP_DEVICE_TYPE         SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_QUERY_SNAP_DEVICE_TYPE )
#define IOCTL_SC_GET_ACTIVE_OPS                 SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_GET_ACTIVE_OPS )

#define IOCTL_SC_GET_LU_STATUS                  SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_GET_LU_STATUS ) 

#define IOCTL_SC_SET_MIN_MEMORY_THRESHOLD_INCREMENT   SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_SET_MIN_MEMORY_THRESHOLD_INCREMENT )

#define IOCTL_SC_SET_MEMORY_GUARD               SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_SET_MEMORY_GUARD )

#define IOCTL_SC_GET_ALL_MEMORY_PARAMETERS      SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_GET_ALL_MEMORY_PARAMETERS )

#define IOCTL_SC_SET_ALL_MEMORY_PARAMETERS      SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_SET_ALL_MEMORY_PARAMETERS )

#define IOCTL_SC_BYPASS_LRU_QUEUE               SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_BYPASS_LRU_QUEUE )

#define IOCTL_SC_BYPASS_MEM_SUBSYSTEM           SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_BYPASS_MEM_SUBSYSTEM )

#define IOCTL_SC_DRAIN_LRU_QUEUE                SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_DRAIN_LRU_QUEUE )

#define IOCTL_SC_GOBBLE_MEMORY                  SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_GOBBLE_MEMORY )

#define IOCTL_SC_RELEASE_GOBBLED_MEMORY         SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_RELEASE_GOBBLED_MEMORY )

#define IOCTL_SC_SET_MEMORY_GUARD_TIMER         SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_SET_MEMORY_GUARD_TIMER )

#define IOCTL_SC_POLL_REQUEST                   SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_POLL_REQUEST )

#define IOCTL_SC_POLL_REQUEST_GET_DELTA_TOKEN   SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_POLL_REQUEST_GET_DELTA_TOKEN )

#define IOCTL_SC_DELTA_POLL_GET_DELETED_OBJECTS SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_DELTA_POLL_GET_DELETED_OBJECTS )

#define IOCTL_SC_DELETE_SNAPSHOT_PSM            SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_DELETE_SNAPSHOT_PSM )

#define IOCTL_SC_DELETE_CACHE_PSM               SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_DELETE_CACHE_PSM )

#define IOCTL_SC_DELETE_SESSIONS_PSM            SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_DELETE_SESSIONS_PSM )

#define IOCTL_SC_DELETE_TARGET_PSM              SC_BUILD_IOCTL( K10_SNAPCOPY_OPC_DELETE_TARGET_PSM )

#define IOCTL_SC_SESSION_START_LIST_ADD_REMOVE  SC_BUILD_IOCTL(K10_SNAPCOPY_OPC_SESSION_START_LIST_ADD_REMOVE)

#define IOCTL_SC_ROLLBACK_ENQUIRE_LUS_PROGRESS  SC_BUILD_IOCTL(K10_SNAPCOPY_OPC_ROLLBACK_ENQUIRE_LUS_PROGRESS)

#define IOCTL_SC_SNAPBACK_TEST                  SC_BUILD_IOCTL(K10_SNAPCOPY_OPC_SNAPBACK_TEST)
//++
//
//  Define the lengths and structures for various names used by SnapCopy. We define values
//  for Session Names, Hostnames, and Data Object Names.
//  These values were determined with help from Veritas folks who wanted to be able to
//  specify a very descriptive name.
//
//-- 
#define SC_SESSION_NAME_LENGTH                  256


typedef char                                    SC_SESSION_NAME[ SC_SESSION_NAME_LENGTH ];


//++
//
//  The following defines the maximum number of sessions per source device.
//
//--

#define SC_MAX_SESSIONS_PER_DEVICE              8

//++
//
//  The following defines the maximum number of snapshots per source device.
//
//--
#define SC_MAX_SNAPSHOTS_PER_DEVICE             8


//++
//
//  Define values for Read/Write buffers sent to the driver.
//
//--
#define SC_RWB_SESSION_START                    0x00
#define SC_RWB_SESSION_STOP                     0x01
#define SC_RWB_READ_ERROR                       0x02
#define SC_RWB_READ_SESSION_NAME                0x03
#define SC_RWB_SNAPLU_ACTIVATE                  0x04
#define SC_RWB_SNAPLU_DEACTIVATE                0x05
#define SC_RWB_GET_CURRENT_API_REVISION         0x06
#define SC_RWB_GET_SNAPSHOT_STATUS              0x07

//
// Starting with these two OP codes, all future Op codes will be sub-Op
// Code that will use these two Op codes and their input/output buffers
// to communicate with SnapView.
//
#define SC_RWB_GENERIC_REQUEST                  0x8
#define SC_RWB_GENERIC_RESPONSE                 0x9
#define SC_RWB_CONSISTENT_START                 0xA

#ifdef SNAP_FAULT_INJECTION_PROTO

#define SC_RWB_FAULT_INJECTION_TEST             0xB
#define SC_RWB_MAX_OPCODE     SC_RWB_FAULT_INJECTION_TEST

#else

#define SC_RWB_MAX_OPCODE     SC_RWB_CONSISTENT_START

#endif


//
// Define copy type structure for an Incremental Session
//

typedef enum _SC_INCREMENTAL_COPY_TYPE
{
    SC_INCREMENTAL_COPY_TYPE_INVALID,
    
    SC_INCREMENTAL_COPY_TYPE_FULL,

    SC_INCREMENTAL_COPY_TYPE_INCREMENTAL,

    SC_INCREMENTAL_COPY_TYPE_BULK

} SC_INCREMENTAL_COPY_TYPE;


//++
//
//  Name:
//
//      SC_SESSION_STATE
//
//  Description:
//
//      Enumeration for state value that a session can be in.
//
//      SC_SESSION_STARTED - Denotes that the session is started and normal.
//      SC_SESSION_STOPPING - Denotes that the session is in the act of stopping.
//      SC_SESSION_PARTIALLY_STOPPED - Denotes that the session failed to stop
//                                     entirely on every source lun involved in the 
//                                     session.
//      SC_SESSION_FAULTED - Denotes that the session is faulted.
//    
//      SC_SESSION_MAX - Denotes the last entry in the enumeration.
//
//--

#define SC_SESSION_STATE_MAX 0xffffffff   

typedef enum _SC_SESSION_STATE
{
    SC_SESSION_STARTED = 0,

    SC_SESSION_STOPPING,

    SC_SESSION_PARTIALLY_STOPPED,

    SC_SESSION_FAULTED,

    SC_SESSION_MAX = SC_SESSION_STATE_MAX

} SC_SESSION_STATE;

//++
//
//  Tag definitions have moved to interface\_Tags.h.
//
//  Do not define any new TAG_ literals in this file!
//
//--

#endif // K10_SNAPCOPY_ADMIN_EXPORT_H
