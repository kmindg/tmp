#ifndef __CMI_UPPER_INTERFACE__
#define __CMI_UPPER_INTERFACE__

//***************************************************************************
// Copyright (C) EMC Corporation 1998-2015
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      CmiUpperInterface.h
//
// Contents:
//      Definitions of the exported IOCTL codes and data structures
//      that form the interface between the CMI driver and its higher-
//  level clients.
//
// Revision History:
//  19-Feb-98   Goudreau    Created.
//  21-Aug-98   Goudreau    Changed Transmit interface to allow two separate
//                          SGLs (source and destination) for Fixed data, as
//                          well as an address offset.  Also replaced the
//                          Floating data SGl with a buffer address and length.
//                          Removed the temporary IOCTL_CMI_SET_LOCAL_SP_ID and
//                          CMI_SET_LOCAL_SP_ID_IOCTL_INFO.
//  15-Sep-98   Goudreau    Added IOCTL_CMI_GET_LOCAL_SP_ID and
//                          CMI_GET_LOCAL_SP_ID_IOCTL_INFO.  Added
//                          Cmi_Filament_Conduit and Cmi_Heartbeat_Conduit.
//  09-Oct-98   Goudreau    Changed CMI_TRANSMIT_MESSAGE_IOCTL_INPUT_INFO and
//                          CMI_MESSAGE_TRANSMISSION_EVENT_DATA.  Moved CMI_SP_ID
//                          in here from its former home in CmiCommonInterface.h.
//  17-Nov-98   Goudreau    Added temporary hack to CMI_TRANSMIT_MESSAGE_IOCTL_INPUT_INFO.
//  07-Dec-98   Goudreau    Added field to CMI_MESSAGE_RECEPTION_EVENT_DATA.
//                          Changed size of CMI_MESSAGE_NUM_BYTES_EXTRA_DATA.
//                          Created IOCTL_CMI_SET_FIXED_DATA_BASE_ADDRESS and
//                          CMI_SET_FIXED_DATA_BASE_ADDRESS_IOCTL_INFO.
//                          Changed CMI_TRANSMIT_MESSAGE_IOCTL_INFO to use one
//                          SGL for Floating Data and one SGL for Fixed Data.
//                          Created CMI_SP_CONTACT_LOST_EVENT_DATA, and added
//                          Cmi_Conduit_Event_Sp_Contact_Lost enum value to
//                          CMI_CONDUIT_EVENT.
//  08-Dec-98   Goudreau    Minor spelling fixes.  Added description of how
//                          the implicit "release received message after
//                          callback runs" mechanism works.
//  01-Feb-99   Goudreau    Added Remote Mirroring Conduits.
//  12-Feb-99   Goudreau    Changed CMI_SP_ID.  Replaced _GET_LOCAL_SP_ID ioctl and
//                          type with _GET_SP_IDS ioctl and type.  Changed fields in
//                          CMI_OPEN_CONDUIT_IOCTL_INFO.
//	29-Mar-99	Goudreau	Changed CMI_TRANSMIT_MESSAGE_IOCTL_INFO to "Phase III"
//							interface.  Added <is_delayed_release_requested> field
//							to CMI_MESSAGE_RECEPTION_EVENT_DATA.
//	29-Jun-99	Goudreau	"Phase III.1" interface:  Added <is_delayed_ack_requested>
//							field to CMI_MESSAGE_RECEPTION_EVENT_DATA.
//	03-Mar-00	Goudreau	Added IOCTL_CMI_HALT_SP and CMI_HALT_SP_IOCTL_INFO.
//	12-Jun-00	Goudreau	Renamed Cmi_RM_Peer_Data_Conduit to Cmi_RM_Request_Data_Conduit,
//							and Cmi_RM_Remote_Data_Conduit to Cmi_RM_Response_Data_Conduit. 
//  12-Sep-00   Corzine     Added IOCTL_CMI_REGISTER_ALTERNATE_DATA_PATH.
//  20-Aug-01   DHarvey     CMID support, changing fixed data send receive interface, stats IOCTLS.
//                          CMI_MESSAGE_RECEPTION_EVENT_DATA now provides link field the 
//                          client can use to queue the request to a thrad without a memory allocation.
//  22-Oct-01   GSchaffer   Move/add client_private_links & client_private_event as "header".
//                          Remove <is_delayed_ack_requested>, <obsolete_fixed_data_sgl>.
//  10-Jan-02   GSchaffer   Add Cmi_RM_{Peer,Remote}_Response_Conduit for future use.
//  09-July-09  RABrisk     Bumped CMI_MAX_CONDUITS to 64 creating rev-lock w/Navi bundle 59.
//  18-Aug-09   RABrisk     Made Cmi_FaultIsolation_Conduit=30 so Taurus has #29 for future assignment.
//--


//
// Header files
//
#include "EmcPAL.h"
#include "CmiCommonInterface.h"
#include "spid_types.h"
#include "k10defs.h"
//#include <devioctl.h>
#include "StaticAssert.h"
#include "processorCount.h"

//
// Exported constants
//

//++
// Description:
//      The device object pathnames of the CMI pseudo-device.
//      CMI clients must open this device in order to send CMI
//      the ioctl requests described below.
//--
#define CMI_BASE_DEVICE_NAME    "Cmid"
#define CMI_DEVICE_NAME_CHAR    "\\\\.\\Cmid"

#define CMI_NT_DEVICE_PATH      "\\Device\\"
#define CMI_WIN32_DEVICE_PATH   "\\DosDevices\\"

#define CMI_NT_DEVICE_NAME      CMI_NT_DEVICE_PATH CMI_BASE_DEVICE_NAME
#define CMI_NT_DEVICE_NAME_CHAR "\\Device\\Cmid"
#define CMI_WIN32_DEVICE_NAME   CMI_WIN32_DEVICE_PATH CMI_BASE_DEVICE_NAME

// Name of PD device object for legacy support
#define CMIpd_BASE_DEVICE_NAME    "Cmi"

// Name of device object for CMID
#define CMID_NT_DEVICE_NAME         "\\Device\\Cmid"
#define CMID_WIN32_LINK_NAME        "\\DosDevices\\Cmid"

//++
// Description:
//      Various numeric constants used in the exported data types below.
//--
#define CMI_MAX_ARRAYS                      8
    //  The maximum number of CLARiiON disk arrays which can exist together
    //  in a CMI-connected multi-array environment.
#define CMI_MAX_SPS_PER_ARRAY               2

    //  The maximum number of SPs which can exist within a single CLARiiON
    //  disk array.  

// The maximum number of virtual circuits per SP.  We want this somewhat 
// larger than "Cmi_Invalid_Conduit" to allow new layer'd drivers without
// requiring a CMID rebuild.
// NOTE: if this value is changed, MAX_VC_PER_CMI in
// ./sys-common/Block_Admin/CoreAdminPerfData.h must also be changed
#define CMI_MAX_CONDUITS					200

//The number of buckets in the bundlesInFlight and messagesInBundle histograms
#define CMI_HISTOGRAM_BUCKETS                                  40

//The number of gates in the CMI per-gate stats
#define CMI_GATE_COUNT                                         8

	//	The number of entries in a Conduit-indexed table (see below for enum).
#define CMI_MESSAGE_NUM_BYTES_EXTRA_DATA    40 // Temporary hack

// How much client data describing the location to DMA fixed data to can be
//passed per message.
# define CMI_FIXED_DATA_BLOB_MAX_SIZE  2048
//
// Exported basic data types
//

//++
// Type:
//		CMI_VIRTUAL_SG_ELEMENT
//
// Description:
//		An element in a Virtual Scatter/Gather list as used by CMI.
//		A Scatter/Gather List itself is just an array of SGEs, terminated
//		by an entry whose <length> field is 0.
//--
#pragma pack(4)
typedef struct _CMI_VIRTUAL_SG_ELEMENT
{
    ULONG     length;
    PVOID     address;
#if !defined(_WIN64)
    ULONG     pad;      // Pad to make addresses line up with CMI_PHYSICAL_SGL
#endif
}	CMI_VIRTUAL_SG_ELEMENT, *PCMI_VIRTUAL_SG_ELEMENT, *CMI_VIRTUAL_SGL;
#pragma pack()
//.End

//++
// Type:
//      CMI_SP_ID
//
// Description:
//      The abstract Storage Processor (SP) identifier type.
//      In C++, basic operators (i.e., "==") work, and the type
//      is compatible with SP_ID.
//      In C, it is the same type as SP_ID.
//
//--
# ifdef __cplusplus

#pragma warning(disable: 4068) //Disable warnings about unknown pragmas
#pragma prefast(disable: 262) //Supress prefast warning 262- Stack overflow
struct CMI_SP_ID : public SPID {
    bool operator == (const SPID & other) const;
    bool operator != (const SPID & other) const;
	bool operator > (const SPID & other) const ;
	bool operator < (const SPID & other) const;
	bool operator >= (const SPID & other) const;
	bool operator <= (const SPID & other) const;

    CMI_SP_ID & operator = (const SPID & other);

    CMI_SP_ID();
    CMI_SP_ID(const SPID & other);

};
#pragma prefast(default: 262) //Restore default setting
#pragma warning(default: 4068) //Enable warnings about unknown pragmas

// Inlining the operators avoids having to create libraries
// for these trivial operations.
inline bool CMI_SP_ID::operator == (const SPID & other) const {
    return node == other.node && engine == other.engine;
}

inline bool CMI_SP_ID::operator != (const SPID & other) const {
    return !(node == other.node && engine == other.engine);
}

inline bool CMI_SP_ID::operator > (const SPID & other) const {
    return node > other.node 
		|| (node == other.node && engine > other.engine);
}
inline bool CMI_SP_ID::operator < (const SPID & other) const {
    return node < other.node 
		|| (node == other.node && engine < other.engine);
}
inline bool CMI_SP_ID::operator >= (const SPID & other) const {
    return node > other.node 
		|| (node == other.node && engine >= other.engine);
}
inline bool CMI_SP_ID::operator <= (const SPID & other) const {
    return node < other.node 
		|| (node == other.node && engine <= other.engine);
}


inline CMI_SP_ID & CMI_SP_ID::operator = (const SPID & other) {
    node = other.node;
    engine = other.engine;
    signature = other.signature;
    return *this;
}

inline     CMI_SP_ID::CMI_SP_ID()
{
}

inline    CMI_SP_ID::CMI_SP_ID(const SPID & other) 
{
    *(SPID*)this = other;
}

# else

typedef SPID CMI_SP_ID;

# endif

typedef CMI_SP_ID *PCMI_SP_ID;


//.End

//++
// Type:
//      CMI_CONDUIT_ID
//
// Description:
//      The abstract Conduit (channel) identifier type.  As new CMI
//      clients appear, this type must be updated to define new
//      Conduits for them.
//
// Note:
//      Redirector conduit definitions (Cmi_xxxRedirector_xxx_Conduit) are not used 
//      directly in the code. If changes are made to any of the Redirector conduit 
//      definitions, the following INF files may need to be modified accordingly:
//      1) catmerge\NTEnv\k10_install\Base\Base.inf,
//      2) catmerge\NTEnv\k10_install\INF\cmid.inf,
//      3) catmerge\NTEnv\k10_install\INF\upperredirector.inf or
//         catmerge\NTEnv\k10_install\INF\middleredirector.inf or
//         catmerge\NTEnv\k10_install\INF\lowerredirector.inf. or 
//         catmerge\NTEnv\k10_install\INF\topredirector.inf.
//      
//--
typedef enum _CMI_CONDUIT_ID
{
    // Flare processes talking to the peer SP
    Cmi_Control_Conduit=0,
    Cmi_L1CacheEvent_Conduit=1,   // L1 Cache to peer SP L1 Cache
    Cmi_CM_Conduit=2,             // CM to peer CM

    // Non-Flare processes talking to the peer SP or other SPs
    Cmi_Filament_Conduit=3,           // MPS (Message Passing Service) Filaments
    Cmi_Heartbeat_Conduit=4,          // Heartbeat services
    Cmi_RM_Request_Data_Conduit=5,    // Remote Mirroring inter-array data flow (requests)
    Cmi_RM_Response_Data_Conduit=6,   // Remote Mirroring inter-array data flow (responses)
    Cmi_RM_Peer_Command_Conduit=7,    // Remote Mirroring intra-array control info
    Cmi_RM_Remote_Command_Conduit=8,  // Remote Mirroring inter-array control info
    Cmi_L2CacheEvent_Conduit=9,   // L2 Cache to peer SP L2 Cache - We put this
    // At the end (instead of above after
    // Cmi_L1CacheEvent_Conduit) is to avoid NDU
    // Upgrade issues.

    Cmi_SnapBack_Request_Conduit=10,   // SnapBack peer requests
    Cmi_SnapBack_Response_Conduit=11,  // SnapBack peer responses

    Cmi_CL_Peer_Request_Conduit=12,    // Clones request conduit
    Cmi_CL_Peer_Response_Conduit=13,   // Clones response conduit

    Cmi_PSM_Conduit=14,				// PSM conduit

    Cmi_FAR_Peer_Request_Conduit=15,   // FAR request conduit

    Cmi_FAR_Peer_Response_Conduit=16,  // FAR response conduit

    // Redirectors have 4 conduits for deadlock avoidance.
    // Note If IoRedirector Conduit # changes then the registry override currently in cmid.inf also needs to change
    Cmi_UpperRedirector_Request_Conduit=17,    // Redirector conduit for requests (Upper)
 
    // Redirector conduit, where all messages handled during callback (Upper)   
    Cmi_UpperRedirector_NoHold_Conduit=18,     

    Cmi_RLT_Conduit=19,                        //  Flare Driver: RLT to peer RLT

    Cmi_UpperRedirector_Response_Conduit=20,   // Redirector conduit for responses (Upper)

    // Note If IoRedirector Conduit # changes then the registry override currently in cmid.inf also needs to change
    Cmi_LowerRedirector_Request_Conduit=21,    // Redirector conduit for requests (Lower)

    // Redirector conduit, where all messages handled during callback (Lower)   
    Cmi_LowerRedirector_NoHold_Conduit=22,     

    Cmi_LowerRedirector_Response_Conduit=23,   // Redirector conduit for responses (Lower)

    Cmi_MLU_Peer_Request_Conduit=24,   // MLU request conduit
    Cmi_MLU_Peer_Response_Conduit=25,  // MLU response conduit

    Cmi_LowerRedirector_Write_Conduit=26,   // Redirector conduit for Write responses (Lower)

    Cmi_DST_Conduit=27,                     // Flare DST to peer DST

    Cmi_UpperRedirector_Write_Conduit=28,   // Redirector conduit for Write responses (Upper)

    // Leave conduit 29 unassigned in Zeus so Taurus has 1 conduit available for future assignment.
    
    Cmi_FaultIsolation_Conduit = 30,        // Fault Isolation for local peer communications

    Cmi_FCT_Request_Conduit = 31,      //
    Cmi_FCT_Response_Conduit = 32,     // FCT Driver Related Conduits
    Cmi_FCT_NoHold_Conduit = 33,       //

    Cmi_EFUP_Conduit=34,                     // Flare EFUP to peer EFUP
    
    Cmi_SEP_Toplogy_Conduit = 35,		//used to send topology and metadata related messages on SEP
    Cmi_SEP_IO_Conduit = 36,			//used to send IO path related messages on SEP
    Cmi_ESP_Conduit = 37,				//used to send mesages between the two Environmental packages (ESP)
    Cmi_NEIT_Conduit = 38,				//used to send mesages between the two NEIT packages
   
    Cmi_CacheDriverConduit=39,             // SP cache main conduit.
    Cmi_SPCacheReferenceSenderConduitCore0 = 40,  // SP cache reference transmission conduit    



    // Note If IoRedirector Conduit # changes then the registry override currently in cmid.inf also needs to change
    Cmi_MiddleRedirector_Request_Conduit=41,    // Redirector conduit for requests (Middle)

    // Redirector conduit, where all messages handled during callback (Middle)   
    Cmi_MiddleRedirector_NoHold_Conduit=42,     

    Cmi_MiddleRedirector_Response_Conduit=43,   // Redirector conduit for responses (Middle)

    Cmi_MiddleRedirector_Write_Conduit=44,   // Redirector conduit for Write responses (Middle)



    // SP cache requires one conduit per core for performance reasons. 
    // Reserving below conduits for platforms for upto 20 cores.  
    Cmi_SPCacheReferenceSenderConduitCore1 = 45,
    Cmi_SPCacheReferenceSenderConduitCore2 = 46,
    Cmi_SPCacheReferenceSenderConduitCore3 = 47,
    Cmi_SPCacheReferenceSenderConduitCore4 = 48,
    Cmi_SPCacheReferenceSenderConduitCore5 = 49,
    Cmi_SPCacheReferenceSenderConduitCore6 = 50,
    Cmi_SPCacheReferenceSenderConduitCore7 = 51,
    Cmi_SPCacheReferenceSenderConduitCore8 = 52,
    Cmi_SPCacheReferenceSenderConduitCore9 = 53,
    Cmi_SPCacheReferenceSenderConduitCore10 = 54,
    Cmi_SPCacheReferenceSenderConduitCore11 = 55,
    Cmi_SPCacheReferenceSenderConduitCore12 = 56,
    Cmi_SPCacheReferenceSenderConduitCore13 = 57,
    Cmi_SPCacheReferenceSenderConduitCore14 = 58,
    Cmi_SPCacheReferenceSenderConduitCore15 = 59,
    Cmi_SPCacheReferenceSenderConduitCore16 = 60,
    Cmi_SPCacheReferenceSenderConduitCore17 = 61,
    Cmi_SPCacheReferenceSenderConduitCore18 = 62,
    Cmi_SPCacheReferenceSenderConduitCore19 = 63,


	/* SEP CMS conduits */
	Cmi_SEP_CMS_State_Machine_Conduit = 64,   // Conduit for the Clustered memory service State Machine
	Cmi_SEP_CMS_Tag_Conduit = 65,   // Conduit for the Clustered memory service Memory buffers

    // SEP SLF conduits
    // Reserving below conduits for platforms for upto 20 cores.  
    Cmi_SEP_IO_Conduit_Core0 = 66,
    Cmi_SEP_IO_Conduit_Core1 = 67,
    Cmi_SEP_IO_Conduit_Core2 = 68,
    Cmi_SEP_IO_Conduit_Core3 = 69,
    Cmi_SEP_IO_Conduit_Core4 = 70,
    Cmi_SEP_IO_Conduit_Core5 = 71,
    Cmi_SEP_IO_Conduit_Core6 = 72,
    Cmi_SEP_IO_Conduit_Core7 = 73,
    Cmi_SEP_IO_Conduit_Core8 = 74,
    Cmi_SEP_IO_Conduit_Core9 = 75,
    Cmi_SEP_IO_Conduit_Core10 = 76,
    Cmi_SEP_IO_Conduit_Core11 = 77,
    Cmi_SEP_IO_Conduit_Core12 = 78,
    Cmi_SEP_IO_Conduit_Core13 = 79,
    Cmi_SEP_IO_Conduit_Core14 = 80,
    Cmi_SEP_IO_Conduit_Core15 = 81,
    Cmi_SEP_IO_Conduit_Core16 = 82,
    Cmi_SEP_IO_Conduit_Core17 = 83,
    Cmi_SEP_IO_Conduit_Core18 = 84,
    Cmi_SEP_IO_Conduit_Core19 = 85,

    Cmi_PeerDeathNotification_Conduit = 86,

    Cmi_DCN_Peer_Request_Conduit  = 87,  // Data Change Notify driver request conduit
    Cmi_DCN_Peer_Response_Conduit = 88,  // Data Change Notify driver  response conduit

    /* More cores!!!!
     */
    Cmi_SPCacheReferenceSenderConduitCore20 = 89,
    Cmi_SPCacheReferenceSenderConduitCore21 = 90,
    Cmi_SPCacheReferenceSenderConduitCore22 = 91,
    Cmi_SPCacheReferenceSenderConduitCore23 = 92,
    Cmi_SPCacheReferenceSenderConduitCore24 = 93,
    Cmi_SPCacheReferenceSenderConduitCore25 = 94,
    Cmi_SPCacheReferenceSenderConduitCore26 = 95,
    Cmi_SPCacheReferenceSenderConduitCore27 = 96,
    Cmi_SPCacheReferenceSenderConduitCore28 = 97,
    Cmi_SPCacheReferenceSenderConduitCore29 = 98,
    Cmi_SPCacheReferenceSenderConduitCore30 = 99,
    Cmi_SPCacheReferenceSenderConduitCore31 = 100,
    Cmi_SPCacheReferenceSenderConduitCore32 = 101,
    Cmi_SPCacheReferenceSenderConduitCore33 = 102,
    Cmi_SPCacheReferenceSenderConduitCore34 = 103,
    Cmi_SPCacheReferenceSenderConduitCore35 = 104,
    Cmi_SPCacheReferenceSenderConduitCore36 = 105,
    Cmi_SPCacheReferenceSenderConduitCore37 = 106,
    Cmi_SPCacheReferenceSenderConduitCore38 = 107,
    Cmi_SPCacheReferenceSenderConduitCore39 = 108,
    Cmi_SPCacheReferenceSenderConduitCore40 = 109,
    Cmi_SPCacheReferenceSenderConduitCore41 = 110,
    Cmi_SPCacheReferenceSenderConduitCore42 = 111,
    Cmi_SPCacheReferenceSenderConduitCore43 = 112,
    Cmi_SPCacheReferenceSenderConduitCore44 = 113,
    Cmi_SPCacheReferenceSenderConduitCore45 = 114,
    Cmi_SPCacheReferenceSenderConduitCore46 = 115,
    Cmi_SPCacheReferenceSenderConduitCore47 = 116,
    Cmi_SPCacheReferenceSenderConduitCore48 = 117,
    Cmi_SPCacheReferenceSenderConduitCore49 = 118,
    Cmi_SPCacheReferenceSenderConduitCore50 = 119,
    Cmi_SPCacheReferenceSenderConduitCore51 = 120,
    Cmi_SPCacheReferenceSenderConduitCore52 = 121,
    Cmi_SPCacheReferenceSenderConduitCore53 = 122,
    Cmi_SPCacheReferenceSenderConduitCore54 = 123,
    Cmi_SPCacheReferenceSenderConduitCore55 = 124,
    Cmi_SPCacheReferenceSenderConduitCore56 = 125,
    Cmi_SPCacheReferenceSenderConduitCore57 = 126,
    Cmi_SPCacheReferenceSenderConduitCore58 = 127,
    Cmi_SPCacheReferenceSenderConduitCore59 = 128,
    Cmi_SPCacheReferenceSenderConduitCore60 = 129,
    Cmi_SPCacheReferenceSenderConduitCore61 = 130,
    Cmi_SPCacheReferenceSenderConduitCore62 = 131,
    Cmi_SPCacheReferenceSenderConduitCore63 = 132,

    Cmi_SEP_IO_Conduit_Core20 = 143,
    Cmi_SEP_IO_Conduit_Core21 = 144,
    Cmi_SEP_IO_Conduit_Core22 = 145,
    Cmi_SEP_IO_Conduit_Core23 = 146,
    Cmi_SEP_IO_Conduit_Core24 = 147,
    Cmi_SEP_IO_Conduit_Core25 = 148,
    Cmi_SEP_IO_Conduit_Core26 = 149,
    Cmi_SEP_IO_Conduit_Core27 = 150,
    Cmi_SEP_IO_Conduit_Core28 = 151,
    Cmi_SEP_IO_Conduit_Core29 = 152,
    Cmi_SEP_IO_Conduit_Core30 = 153,
    Cmi_SEP_IO_Conduit_Core31 = 154,
    Cmi_SEP_IO_Conduit_Core32 = 155,
    Cmi_SEP_IO_Conduit_Core33 = 156,
    Cmi_SEP_IO_Conduit_Core34 = 157,
    Cmi_SEP_IO_Conduit_Core35 = 158,
    Cmi_SEP_IO_Conduit_Core36 = 159,
    Cmi_SEP_IO_Conduit_Core37 = 160,
    Cmi_SEP_IO_Conduit_Core38 = 161,
    Cmi_SEP_IO_Conduit_Core39 = 162,
    Cmi_SEP_IO_Conduit_Core40 = 163,
    Cmi_SEP_IO_Conduit_Core41 = 164,
    Cmi_SEP_IO_Conduit_Core42 = 165,
    Cmi_SEP_IO_Conduit_Core43 = 166,
    Cmi_SEP_IO_Conduit_Core44 = 167,
    Cmi_SEP_IO_Conduit_Core45 = 168,
    Cmi_SEP_IO_Conduit_Core46 = 169,
    Cmi_SEP_IO_Conduit_Core47 = 170,
    Cmi_SEP_IO_Conduit_Core48 = 171,
    Cmi_SEP_IO_Conduit_Core49 = 172,
    Cmi_SEP_IO_Conduit_Core50 = 173,
    Cmi_SEP_IO_Conduit_Core51 = 174,
    Cmi_SEP_IO_Conduit_Core52 = 175,
    Cmi_SEP_IO_Conduit_Core53 = 176,
    Cmi_SEP_IO_Conduit_Core54 = 177,
    Cmi_SEP_IO_Conduit_Core55 = 178,
    Cmi_SEP_IO_Conduit_Core56 = 179,
    Cmi_SEP_IO_Conduit_Core57 = 180,
    Cmi_SEP_IO_Conduit_Core58 = 181,
    Cmi_SEP_IO_Conduit_Core59 = 182,
    Cmi_SEP_IO_Conduit_Core60 = 183,
    Cmi_SEP_IO_Conduit_Core61 = 184,
    Cmi_SEP_IO_Conduit_Core62 = 185,
    Cmi_SEP_IO_Conduit_Core63 = 186,
    Cmi_Tdx_Request_Conduit   = 187,  // TDX peer requests
    Cmi_Tdx_Response_Conduit  = 188,  // TDX peer responses

    Cmi_Qos_Driver_Request_Conduit = 189,

    Cmi_LXF_Request_Conduit   = 190,
    Cmi_LXF_Response_Conduit  = 191,
    
    Cmi_Qos_Driver_Response_Conduit = 192,

    // 
    // Redirector conduit, where all messages handled during callback (Top)   
    
    Cmi_TopRedirector_Response_Conduit=193,   // Redirector conduit for responses (Top)

    Cmi_TopRedirector_Write_Conduit=194,   // Redirector conduit for Write responses (Top)

    Cmi_TopRedirector_Request_Conduit=195,

    Cmi_TopRedirector_NoHold_Conduit=196,

    Cmi_Invalid_Conduit         // terminator for enumeration

}   CMI_CONDUIT_ID, *PCMI_CONDUIT_ID;
//.End

STATIC_ASSERT(MAX_CORE_COUNT_NE_64, MAX_CORES <= 64);

//++
// Type:
//      CMI_CONDUIT_EVENT
//
// Description:
//      An enumerated type for all the event-types which could
//      provoke the CMI pseudo-device driver to call back up to
//      a Conduit opener in order to request some service.
//
// Members:
//  Cmi_Conduit_Event_Message_Transmitted:  a message transmission
//      attempt has completed.  The <data> argument to the event-handler
//      function points to a CMI_MESSAGE_TRANSMISSION_EVENT_DATA packet
//      describing the message.
//  Cmi_Conduit_Event_Message_Received:  a message has been
//      received.  The <data> argument to the event-handler function
//      points to a CMI_MESSAGE_RECEPTION_EVENT_DATA packet describing
//      the message.
//  Cmi_Conduit_Event_DMA_Addresses_Needed: The client on the peer
//      sent data that needs to be DMAed into memory.   The <data> argument 
//      to the event-handler function points to a 
//      CMI_FIXED_DATA_DMA_NEEDED_EVENT_DATA packet describing
//      the message.
//  Cmi_Conduit_Event_Close_Completed:  a previous IOCTL_CMI_CLOSE_CONDUIT
//      operation has completed.  The <data> argument to the event-handler
//      function points to a CMI_CLOSE_COMPLETION_EVENT_DATA packet
//      describing the Conduit that finished closing.
//  Cmi_Conduit_Event_Sp_Contact_Lost:  the CMI driver has lost contact
//      with a remote SP with which it was previously able to communicate.
//  Cmi_Conduit_Event_Invalid:  terminator for the enumeration.
//--
typedef enum _CMI_CONDUIT_EVENT
{
    Cmi_Conduit_Event_Message_Transmitted,
    Cmi_Conduit_Event_Message_Received,
    Cmi_Conduit_Event_Close_Completed,
    Cmi_Conduit_Event_Sp_Contact_Lost,
    Cmi_Conduit_Event_DMA_Addresses_Needed,
    Cmi_Conduit_Event_Invalid           // terminator for enumeration

} CMI_CONDUIT_EVENT, *PCMI_CONDUIT_EVENT;
//.End


//
// IOCTL control codes
//

//
// A helper macro used to define our ioctls.  The 0xB00 function code base
// value was chosen for no particular reason other than the fact that it
// lies in the customer-reserved number range (according to <devioctl.h>).
//
#define CMI_CTL_CODE(code, method) (\
    EMCPAL_IOCTL_CTL_CODE(EMCPAL_IOCTL_FILE_DEVICE_UNKNOWN, 0xB00 + (code),   \
              (method), EMCPAL_IOCTL_FILE_ANY_ACCESS) )

#define IOCTL_CMI_OPEN_CONDUIT              \
    CMI_CTL_CODE(1, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_CMI_CLOSE_CONDUIT             \
    CMI_CTL_CODE(2, EMCPAL_IOCTL_METHOD_NEITHER)

#define IOCTL_CMI_TRANSMIT_MESSAGE          \
    CMI_CTL_CODE(3, EMCPAL_IOCTL_METHOD_NEITHER)

#define IOCTL_CMI_RELEASE_RECEIVED_MESSAGE  \
    CMI_CTL_CODE(4, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_CMI_GET_SP_IDS                \
    CMI_CTL_CODE(5, EMCPAL_IOCTL_METHOD_BUFFERED)

// obsolete 
#define IOCTL_CMI_SET_FIXED_DATA_BASE_ADDRESS  CMI_CTL_CODE(6, EMCPAL_IOCTL_METHOD_BUFFERED)
// obsolete delete after JP check-in
typedef struct _CMI_SET_FIXED_DATA_BASE_ADDRESS_IOCTL_INFO
{
    CMI_CONDUIT_ID  conduit_id;
    PVOID			base_address;

} CMI_SET_FIXED_DATA_BASE_ADDRESS_IOCTL_INFO, *PCMI_SET_FIXED_DATA_BASE_ADDRESS_IOCTL_INFO;

// UNUSED #define IOCTL_CMI_ACK_RECEIVED_MESSAGE CMI_CTL_CODE(7, METHOD_BUFFERED)

#define IOCTL_CMI_HALT_SP  \
    CMI_CTL_CODE(8, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_CMI_REGISTER_ALTERNATE_DATA_PATH \
    CMI_CTL_CODE(9, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_CMI_GET_STATISTICS \
    CMI_CTL_CODE(10, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_CMI_GET_BUNDLING_CONTROL \
    CMI_CTL_CODE(11, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_CMI_SET_BUNDLING_CONTROL \
    CMI_CTL_CODE(12, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_CMI_RELEASE_SP_CONTACT_LOST_EVENT_DATA \
    CMI_CTL_CODE(13, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_CMI_PURGE_CANCELLED_MESSAGES \
    CMI_CTL_CODE(14, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_CMI_GET_PATH_STATUS \
    CMI_CTL_CODE(15, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_CMI_PANIC_SP \
    CMI_CTL_CODE(16, EMCPAL_IOCTL_METHOD_BUFFERED)

//
// Structures used by the IOCTL codes
//

//++
// Type:
//      CMI_OPEN_CONDUIT_IOCTL_INFO
//
// Description:
//      Packet used by IOCTL_CMI_OPEN_CONDUIT, which specifies how
//      to open a CMI Conduit.  Only one opener at a time is allowed
//      on a given Conduit on a given SP, since only a single event
//      callback routine can be associated with the Conduit.
//
//		Possible return values from the ioctl are:
//
//		STATUS_SUCCESS:  the open request succeeded.
//		STATUS_INVALID_BUFFER_SIZE:  the input buffer argument passed
//			to the ioctl is not the correct size.
//		STATUS_FILES_OPEN:  the specified Conduit was already open.
//
// Members:
//  conduit_id:  the one to open.
//  event_callback_routine:  pointer to the function which the CMI pseudo-
//      device driver must call in order to handle all events (incoming
//      traffic, errors, etc.) that occur on the Conduit.  The function
//      takes two arguments:  a description of the type of event that
//      occurred, and a pointer to some event-specific information.
//      +---------------------------------------------------------------+
//      |   WARNING!  A Conduit callback routine must be capable of     |
//      |   running at DISPACH_LEVEL.  A callback routine is allowed    |
//      |   to submit additional CMI message transmission requests of   |
//      |   its own, but it must not block waiting for the completion   |
//      |   notification event of such a request.  It cannot block      |
//      |   waiting for resources to be freed by a previous request,    |
//      |   or for any resources dependent on a previous request.       |
//      +---------------------------------------------------------------+
//      A callback routine's return status is ignored for most events,
//      but is interpreted for Cmi_Conduit_Event_Message_Received events.
//      See the description of CMI_MESSAGE_RECEPTION_EVENT_DATA below
//      for details.
//  confirm_reception:  TRUE iff message receptions must be ACKed.
//  notify_of_remote_array_failures:  TRUE iff the opener wants to be notified
//      about all SP failures (even SPs in remote arrays), not just about
//      peer SPs in the local array.
//  persistent_memory, persistent_size:  allows the caller to provide the 
//      memory for the ring buffer, so that messages received will be placed
//      into that buffer rather than arbitarilly allocated memory, so the 
//      received messages have the characteristic of that memory (e.g., persistence 
//      across CPU reset).
//--
typedef struct _CMI_OPEN_CONDUIT_IOCTL_INFO
{
    CMI_CONDUIT_ID  conduit_id;
    EMCPAL_STATUS        (*event_callback_routine) (CMI_CONDUIT_EVENT event, PVOID data);
    BOOLEAN         confirm_reception;
    BOOLEAN         notify_of_remote_array_failures;

} CMI_OPEN_CONDUIT_IOCTL_INFO, *PCMI_OPEN_CONDUIT_IOCTL_INFO;

typedef struct _CMI_OPEN_PERSISTENT_CONDUIT_IOCTL_INFO
{
    CMI_CONDUIT_ID  conduit_id;
    EMCPAL_STATUS        (*event_callback_routine) (CMI_CONDUIT_EVENT event, PVOID data);
    BOOLEAN         confirm_reception;
    BOOLEAN         notify_of_remote_array_failures;

	 BOOLEAN			  send_ack_on_client_processed;

    // FIX: the CMID interface takes caller provided memory which might be persistent, not persistent memory

    ULONG           persistent_size;
    PVOID           persistent_memory;

} CMI_OPEN_PERSISTENT_CONDUIT_IOCTL_INFO, *PCMI_OPEN_PERSISTENT_CONDUIT_IOCTL_INFO;
//.End

//++
// Type:
//      CMI_CLOSE_CONDUIT_IOCTL_INFO
//
// Description:
//      Packet used by IOCTL_CMI_CLOSE_CONDUIT, which is the operation
//      which closes a CMI Conduit that was previously opened.
//
//      Note that this ioctl packet must persist for longer than just the
//      ioctl call itself.  Once submitted, it cannot be deallocated or
//      altered by the closer until the close attempt completes
//      and the closer receives a callback event of variety
//      Cmi_Conduit_Event_Conduit_Closed.  The event's
//      CMI_CLOSE_COMPLETION_EVENT_DATA packet will contain the
//      address of this original ioctl structure to let the original
//      closing process know which close attempt it was that got completed.
//
//      +---------------------------------------------------------------+
//      |   WARNING!  This ioctl packet must be allocated from NT's     |
//      |   NonPagedPool only.                                          |
//      +---------------------------------------------------------------+
//
//		Possible return values from the ioctl itself (as distinct from the
//		<close_status> structure element that gets reported via a
//		Cmi_Conduit_Event_Close_Completed event) are:
//
//		STATUS_SUCCESS:  the close request was accepted (but not
//			yet acted upon).
//		STATUS_INVALID_BUFFER_SIZE:  the input buffer argument passed
//			to the ioctl is not the correct size.
//
// Members:
//  close_handle:  an opaque but unique identifier for this close
//      attempt, usually the address of some data structure
//      that persists for the duration of the close attempt.
//  conduit_id:  the one to close.
//  close_status:  upon completion of the close attempt, this field
//      will be updated to indicate whether the close completed
//      correctly (STATUS_SUCCESS), or what kind of error occurred.
//		See the description of CMI_CONDUIT_CLOSE_COMPLETION_EVENT_DATA
//		below for details on specific <close_status> values.  This field
//		is not filled in by the original caller, and only becomes valid
//		when the close attempt finishes and the closer receives a callback
//		event of variety Cmi_Conduit_Event_Close_Completed.
//--
typedef struct _CMI_CLOSE_CONDUIT_IOCTL_INFO
{
    EMCPAL_LIST_ENTRY      client_private_links;
    UCHAR           client_private_event;
    UCHAR           spare[3];

    PVOID           close_handle;
    CMI_CONDUIT_ID  conduit_id;
    EMCPAL_STATUS        close_status;

} CMI_CLOSE_CONDUIT_IOCTL_INFO, *PCMI_CLOSE_CONDUIT_IOCTL_INFO;
//.End

//++
// Type:
//      CMI_TRANSMIT_MESSAGE_IOCTL_INFO
//
// Description:
//      Input packet used by IOCTL_CMI_TRANSMIT_MESSAGE, which sends a single
//      message to another SP via a previously-opened Conduit.  A CMI message
//      has two components, both of arbitrary size:  a "Floating" part and an
//      optional "Fixed" part.  Each component is described by a separate
//      Scatter/Gather List (SGL).
//
//      The difference between Fixed Data and Floating Data lies in how the
//      data are stored on the receiving SP.  Fixed Data will be stored at a
//      particular destination address which is determined by the receiving
//      client based on the fixedDataDescriptionBlob.
//
//      Incoming Floating Data, on the other hand, will be stored in an
//      arbitrary buffer whose exact location is at the discretion of the CMI
//      driver. No special prior action (other than opening the Conduit) is
//      required to send or receive Floating Data on a given Conduit.
//
//      Note that this ioctl packet must persist for longer than just the
//      ioctl call itself.  Once submitted, it cannot be deallocated or
//      altered by the sender until the message transmission completes
//      and the sender receives a callback event of variety
//      Cmi_Conduit_Event_Message_Transmitted.  The event's
//      CMI_MESSAGE_TRANSMISSION_EVENT_DATA packet will contain the
//      address of this original ioctl structure to let the original
//      sending process know which message it was that got transmitted.
//
//      +---------------------------------------------------------------+
//      |   WARNING!  This ioctl packet must be allocated from NT's     |
//      |   NonPagedPool only.  Furthermore, the <floating_data_sgl>    |
//      |   and <fixed_data_sgl> Scatter/Gather List pointers must      |
//      |   point to memory that was allocated from NonPagedPool only.  |
//      +---------------------------------------------------------------+
//
//		Possible return values from the ioctl itself (as distinct from the
//		<transmission_status> structure element that gets reported via a
//		Cmi_Conduit_Event_Message_Transmitted event) are:
//
//		STATUS_SUCCESS:  the transmission request was accepted and queued
//			for transmission, but not yet necessarily transmitted.
//		STATUS_INVALID_BUFFER_SIZE:  the input buffer argument passed
//			to the ioctl is not the correct size.
//		STATUS_PORT_DISCONNECTED:  no connection to the specified
//			destination SP was found.
//		STATUS_FILE_CLOSED:  the specified Conduit was not open.
//      STATUS_INVALID_PARAMETER: 
//
// Members:
//  transmission_handle:  an opaque but unique identifier for this
//      transmission attempt, usually the address of some data structure
//      that persists for the duration of the transmission attempt.  When
//      the message transmission completes, the sender will receive a
//      callback event of variety Cmi_Conduit_Event_Message_Transmitted,
//      and the event's CMI_MESSAGE_TRANSMISSION_EVENT_DATA packet will
//      contain this handle to let the original sending process know
//      which message it was that got transmitted.
//  conduit_id:  the Conduit over which the message is to be sent.
//  destination_sp:  the targeted SP.
//  fixed_data_sgl:  a physically contiguous Scatter/Gather List describing
//		where the Fixed data component of the message to be sent can be found
//      on the sending SP.  The total amount of Fixed data is implicit
//      from the NULL-terminiated Scatter/Gather List, whose 
//      entries are "pseudo-physical"
//		addresses that can be normalized into pure physical addresses by
//		adding physical_data_offset
//		to them. The sender must describe this fixed data to the client
//      on the peer SP in the fixedDataDescriptionBlob
//      ALL ADDRESSES AND LENGTHS MUST BE 4 byte aligned.
//      
// fixedDataDescriptionBlob - Client specific data describing the 
//      fixed_data_sgl that will be presented to the receiving client
//      (via a Cmi_Conduit_Event_DMA_Addresses_Needed event) 
//      before that data will be transferred into the receiver's memory.
//
// fixedDataDescriptionBlobLength - The number of bytes in 
//      fixedDataDescriptionBlob, max of CMI_FIXED_DATA_BLOB_MAX_SIZE.
//      Ignored if fixed_data_sgl==NULL.  
//      If non-zero, the fixed data offset set by IOCTL_CMI_SET_FIXED_DATA_BASE_ADDRESS
//      must be 0 (the default).
//
//  physical_floating_data_sgl:  a Scatter/Gather List (not necessarily
//		physically contiguous itself) describing where the Floating data
//		component of the message to be sent can be found on the sending SP.
//		The total amount of Floating data is implicit from the List, whose
//		addresses are "pseudo-physical"addresses that can be normalized into
//		pure physical addresses by adding <physical_data_offset>
//		to them. 
//      ALL ADDRESSES AND LENGTHS MUST BE 4 byte aligned.
//	physical_data_offset:  the address offset to be added to each
//		pseudo-physical address in <physical_floating_data_sgl> or
//      <fixed_data_sgl> in order to yield the actual physical address.
//      MUST BE 4 byte aligned.
//
//  virtual_floating_data_sgl:   a Scatter/Gather List (not necessarily
//		physically contiguous itself) describing where the Floating data
//		component of the message to be sent can be found on the sending SP.
//		The total amount of Floating data is implicit from the List, whose
//		addresses are system virtual addresses describing chunks of data
//		which are each virtually contiguous but not necessarily physically
//		contiguous.  At most one of the two Floating Data SGL pointers
//		(<physical_floating_data_sgl> and <virtual_floating_data_sgl>) may
//		be non-NULL.
//      ALL ADDRESSES AND LENGTHS MUST BE 4 byte aligned, except for last length.
//  extra_data:  (TEMPORARY HACK)  a fixed-size buffer of additional
//      Floating data to be sent.
//  transmission_status:  upon completion of the transmission attempt,
//      this field will be updated to indicate if the message was
//      delivered as specified (STATUS_SUCCESS), or what kind of
//      error prevented it from being delivered.  See the description of
//		CMI_MESSAGE_TRANSMISSION_EVENT_DATA below for details on
//		specific <transmission_status> values.  This field is not
//      filled in by the original caller, and only becomes valid when
//      the message transmission completes and the sender receives a
//      callback event of variety Cmi_Conduit_Event_Message_Transmitted.
//  cancelled: RO to the CMI driver.  Normally initialized to false.
//      messages with this field set to TRUE are rejected in the
//      initial dispatch routine.  This field is useful because it 
//      allows a client to cancel a message in one thread when the 
//      sending thread released locks but has not completed the 
//      IoCallDriver call.  This field is especially valuable to 
//      avoid a contact lost/contact reestablished race where
//      a message the client sent before the contact lost might 
//      have been sent successfully after the contract was re-established.
//	private_*: private queue entry field for use by the driver 
//      that has control of this operation.
//		 The caller should never need to read or write this field
//		(no initialization value is needed).
//--
#pragma pack(4)
typedef struct _CMI_TRANSMIT_MESSAGE_IOCTL_INFO
{
	union {
		
		// Usable by other drivers when operation not in progress.
        EMCPAL_LIST_ENTRY      client_private_links;

		// CMID Private fields. They could be VOID *, but then the
        // list operations in CMID would not be type-safe.
		struct {
			struct _CMI_TRANSMIT_MESSAGE_IOCTL_INFO * private_link;
#           ifdef __cplusplus
			class  CmiVirtualCircuit *			  private_vc;
#           else
			struct CmiVirtualCircuit *			  private_vc;
#           endif
		};
	};
    UCHAR               client_private_event;
    UCHAR               mPreferredCPU;
    UCHAR               spare[1];
	BOOLEAN             cancelled;

    PVOID               transmission_handle;
    CMI_CONDUIT_ID      conduit_id;
    CMI_SP_ID           destination_sp;
    CMI_PHYSICAL_SGL    fixed_data_sgl;
    CMI_PHYSICAL_SGL    physical_floating_data_sgl;
    SGL_PADDR           physical_data_offset;
    CMI_VIRTUAL_SGL     virtual_floating_data_sgl;
    UCHAR               extra_data[CMI_MESSAGE_NUM_BYTES_EXTRA_DATA];
    EMCPAL_STATUS       transmission_status;

    ULONG               fixedDataDescriptionBlobLength;
    PVOID               fixedDataDescriptionBlob;
    ULONG					cmiPrivateProducerIndex; // Producer index when added to ring
    ULONG					keepThisStructureMultipleOf8BytesPad;

    // CMI private usage.  The time the message was queued.
    ULONGLONG           mTransferStartTime;
} CMI_TRANSMIT_MESSAGE_IOCTL_INFO, *PCMI_TRANSMIT_MESSAGE_IOCTL_INFO;
#pragma pack()
//.End

//++
// Type:
//      CMI_PURGE_CANCELLED_MESSAGES_IOCTL_INFO
//
// Description:
//      IOCTL_CMI_PURGE_CANCELLED_MESSAGES.
//      Searches for messages queued by IOCTL_CMI_TRANSMIT_MESSAGE, and
//      be completes any message that has the "cancelled" flag set  with trasnmission_status
//      of "STATUS_CANCELED".  CANCELING a transmission ensures that
//      the message will not wait indefinitely for flow control, but
//      all processing of the message should be via the transmit complete
//      event.  
//
//
//  NOTE:
//      Prior to issuing this IOCTL, CMI_TRANSMIT_MESSAGE_IOCTL_INFO::cancelled
//      should be set to TRUE in messages to be cancelled.  This assignment should 
//      be done only if the client has ensured that the message has not already been
//      completed.
//      
//      A special guarantee is provided out of the "Contact Lost" event handler:
//      only CMI_TRANSMIT_MESSAGE_IOCTL_INFO::cancelled need be set to ensure
//      that another thread sending messages gets errors on messages 
//      created before the contact lost event.  This is important if the other 
//      SP comes back on-line quickly.
//
//      
//
// Members:
//  conduit_id/destination_sp:  Defines the connection to scan for cancelled messages.
//--
typedef struct _CMI_PURGE_CANCELLED_MESSAGES_IOCTL_INFO
{
    CMI_CONDUIT_ID		                conduit_id;
    CMI_SP_ID			                destination_sp;
} CMI_PURGE_CANCELLED_MESSAGES_IOCTL_INFO, *PCMI_PURGE_CANCELLED_MESSAGES_IOCTL_INFO;
//.End


//++
// Type:
//      CMI_RELEASE_RECEIVED_MESSAGE_IOCTL_INFO
//
// Description:
//      Packet used by IOCTL_CMI_RELEASE_RECEIVED_MESSAGE, which returns all
//      storage allocated by CMI to hold an incoming message (and its associated
//      data) which was not freed implicitly after the event-handler callback
//      routine ran.  See the description of CMI_MESSAGE_RECEPTION_EVENT_DATA
//      for details on the format of incoming messages, and for a description
//      of the circumstances in which calling this ioctl explicitly is required.
//
//		Possible return values from the ioctl are:
//
//		STATUS_SUCCESS:  the release request succeeded.
//		STATUS_INVALID_BUFFER_SIZE:  the input buffer argument passed
//			to the ioctl is not the correct size.
//
// Members:
//  reception_data_ptr:  The message reception event data packet address that
//      was previously passed to the Conduit's event-handler to describe the
//      incoming message.  After this ioctl call is made, the structure pointed
//      at by <reception_data_ptr> is no longer safe to access; likewise with
//      the memory to which reception_data_ptr->floating_message_data_ptr and
//      reception_data_ptr->fixed_message_sgl points.
//--
typedef struct _CMI_RELEASE_RECEIVED_MESSAGE_IOCTL_INFO
{
    struct _CMI_MESSAGE_RECEPTION_EVENT_DATA   *reception_data_ptr;

} CMI_RELEASE_RECEIVED_MESSAGE_IOCTL_INFO, *PCMI_RELEASE_RECEIVED_MESSAGE_IOCTL_INFO;
//.End



//++
// Type:
//      CMI_GET_SP_IDS_IOCTL_INFO
//
// Description:
//      Output packet which will be filled in by IOCTL_CMI_GET_SP_IDS,
//      which retrieves the SP ID of all reachable SPs.  Note that the
//      ioctl has no input packet.
//
//		Possible return values from the ioctl are:
//
//		STATUS_SUCCESS:  the Get-SP-IDs request succeeded.
//		STATUS_INVALID_BUFFER_SIZE:  the output buffer argument passed
//			to the ioctl is not the correct size.
//
// Members:
//  sp_ids:  A table of every SP ID that can be reached from this SP,
//      including itself.  All entries in the table for which the
//      CMI_IS_SP_ID_VALID() macro returns TRUE represent the IDs of
//      SPs in this array or in other arrays.  The invalid table entries
//      must be ignored.
//  index_of_local_array:  The index value of the above table's first
//      dimension that describes all SPs on the local array.   I.e., the
//      valid entries in the array sp_ids[index_of_local_array] include
//      the SP IDs of the local SP and its peer(s) within the local array.
//  index_of_local_sp:  The index value of the above table's second
//      dimension that describes the local SP. I.e., the local SP ID is
//      sp_ids[index_of_local_array][index_of_local_sp].
//--
typedef struct _CMI_GET_SP_IDS_IOCTL_INFO
{
    CMI_SP_ID   sp_ids[CMI_MAX_ARRAYS][CMI_MAX_SPS_PER_ARRAY];
    ULONG       index_of_local_array;
    ULONG       index_of_local_sp;

} CMI_GET_SP_IDS_IOCTL_INFO, *PCMI_GET_SP_IDS_IOCTL_INFO;
//.End

//++
// Macro:
//      CMI_IS_SP_ID_VALID
//
// Description:
//      Determine if an entry in a CMI_GET_SP_IDS_IOCTL_INFO.sp_ids[][]
//      table represents an SP or an unused entry.
//
// Arguments:
//      sp_id:  the SP ID whose validity is to be checked.
//
// Return Value:
//      TRUE:  the SP ID is valid.
//      FALSE:  the SP ID is invalid.
//--
#define CMI_INVALID_SP_ID 0xffffffff
#define CMI_IS_SP_ID_VALID(sp_id) ((sp_id).engine != CMI_INVALID_SP_ID)



//++
// Type:
//      CMI_HALT_SP_IOCTL_INFO
//
// Description:
//      Ioctl input packet used by IOCTL_CMI_HALT_SP,
//      which informs another SP to commit suicide immediately.
//
//		Possible return values from the ioctl are:
//
//		STATUS_SUCCESS:  always, since if the specified SP is not
//			reachable, it is already presumed to be dead.
//
// Members:
//  sp_id:  the SP to be killed.
//--
typedef struct _CMI_HALT_SP_IOCTL_INFO
{
    CMI_SP_ID  sp_id;

} CMI_HALT_SP_IOCTL_INFO, *PCMI_HALT_SP_IOCTL_INFO;
//.End

//++
// Type:
//      CMI_PANIC_SP_IOCTL_INFO
//
// Description:
//      Ioctl input packet used by IOCTL_CMI_PANIC_SP,
//      which informs another SP to commit suicide immediately.
//
//		Possible return values from the ioctl are:
//
//		STATUS_SUCCESS:  always, since if the specified SP is not
//			reachable, it is already presumed to be dead.
//
// Members:
//  conduit : Conduit used to send control op
//  sp_id:  the SP to be killed.
//--
typedef struct _CMI_PANIC_SP_IOCTL_INFO
{
    CMI_SP_ID  sp_id;
    CMI_CONDUIT_ID	conduit_id;
} CMI_PANIC_SP_IOCTL_INFO, *PCMI_PANIC_SP_IOCTL_INFO;
//.End


//
// Structures used by Conduit event-handler callback routines
//

//++
// Type:
//      CMI_MESSAGE_TRANSMISSION_EVENT_DATA
//
// Description:
//      Packet of data whose address is passed to a Conduit's event-handler
//      when a Cmi_Conduit_Event_Message_Transmitted event is signaled.
//
//		Possible status values for the <ioctl_info_ptr->transmission_status>
//		structure element are:
//
//		STATUS_SUCCESS:  the message was successfully sent to its destination,
//			and (if the Conduit was opened in Confirmed Delivery mode), an
//			Acknowledgement of the message's successful receipt was returned
//			here by the destination SP.
//		STATUS_PORT_DISCONNECTED:  no physical connection to the message's
//			destination SP was found, or all previously known connections
//			have timed out.
//		STATUS_UNSUCCESSFUL:  the destination SP was too busy or had too few
//			free resources to allow it to accept the message; or the message's
//			Conduit was not open at the destination SP.
//
// Members:
//  ioctl_info_ptr:  address of the CMI_TRANSMIT_MESSAGE_IOCTL_INFO
//      structure that was used to submit the original transmission request.
//--
typedef struct _CMI_MESSAGE_TRANSMISSION_EVENT_DATA
{
    PCMI_TRANSMIT_MESSAGE_IOCTL_INFO  ioctl_info_ptr;

} CMI_MESSAGE_TRANSMISSION_EVENT_DATA, *PCMI_MESSAGE_TRANSMISSION_EVENT_DATA;
//.End

//++
// Type:
//      CMI_FIXED_DATA_DMA_NEEDED_EVENT_DATA
//
// Description:
//      Packet of data whose address is passed to a Conduit's event-handler
//      when a Cmi_Conduit_Event_DMA_Addresses_Needed event is signaled.  
//    
//      The event handler in the client MUST translate this message into
//      a list of addresses to DMA the data being received to.  It must
//      update the output parameters. 
//      
//      The handler must specify PhysicalAddresses in the SglMemory, starting
//      a the first element.  The first element should correspond to 
//      the location to DMA the data for offset "currentOffset" in the transfer.
//      It should describe 1 or more bytes of the transfer, up to totalBytes,
//      or until the memory for the SGL is exceeded.
//
//      The event handler must make no assumptions about the order of events,
//      and must allow duplicate calls (therefore it should not keep any
//      state about which of these events it has seen.)
//
//      
//
// Input Members:
//  fixedDataDescriptionBlob - Client specific data describing the 
//      fixed data that the sender put on the wire.
//
//  fixedDataDescriptionBlobLength - The number of bytes in 
//      fixedDataDescriptionBlob, always > 0.  
//
//  currentBytes:	The number of bytes, excluding the header, that this transfer
//                  starts at. 0 <= currentBytes < totalBytes
//  totalBytes:		The total number of fixed Data bytes to be transferred.
//  sglMemory:		A pointer to already allocated array that can be used for
//					scatter/gather list memory.
//	maxSglElements:	The max number of elements in sglMemory.  This limits
//                  the amount of data that may be DMAed.
//  conduit_id:  the Conduit over which the message was received.
//  originating_sp:  the SP that sent the message to us.
//
//					  
// Output Members:
//  numSglElementsUsed - the number of SGL elements this transfer used.
//  numBytesToTransfer - the number of bytes to transfer,
//              1 <= numBytesToTransfer <= (totalBytes - currentBytes)
//    
//--

typedef struct _CMI_FIXED_DATA_DMA_NEEDED_EVENT_DATA
{
    ULONG                       fixedDataDescriptionBlobLength;
    PVOID                       fixedDataDescriptionBlob;
    ULONG			            currentBytes;
    ULONG				        totalBytes;
    ULONG				        maxSglElements;
    CMI_PHYSICAL_SG_ELEMENT *	sglMemory;
    

// Output Fields:
    ULONG                       numSglElementsUsed;
    ULONG                       numBytesToTransfer;

//  Source Identification.
    CMI_CONDUIT_ID		conduit_id;
    CMI_SP_ID			originating_sp;


} CMI_FIXED_DATA_DMA_NEEDED_EVENT_DATA, *PCMI_FIXED_DATA_DMA_NEEDED_EVENT_DATA;
//.End




//++
// Type:
//      CMI_MESSAGE_RECEPTION_EVENT_DATA
//
// Description:
//      Packet of data whose address is passed to a Conduit's event-handler
//      when a Cmi_Conduit_Event_Message_Received event is signaled.  Note
//      that there is no need to include a <message_id> element in
//      this structure, as CMI transparently and automatically takes
//      care of message ordering and confirmation for its clients.
//
//      For this event, the status value returned by the event-handler
//		callback routine is important, because it will tell CMI whether
//		or not the CMI_MESSAGE_RECEPTION_EVENT_DATA packet whose address
//		CMI had passed as a parameter to the callback function is still
//		accessible at the conclusion of the callback.  A return status of
//		STATUS_SUCCESS signals that the packet (and its associated message
//		data) still exists.  Alternatively, a return status of STATUS_FILE_DELETED
//		indicates that the packet (and its associated message data) has
//		already been released from within the callback function, which
//		did this via an explicit call to IOCTL_CMI_RELEASE_RECEIVED_MESSAGE.
//
//		If the event packet and message data have not been released from
//		within the callback function, there is a separate issue of how
//		they are eventually released after the callback completes.  The
//		<is_delayed_release_requested> field in the event packet controls
//		this feature.  CMI initializes this field to FALSE before invoking
//		the callback function, which has the option of leaving it alone or
//		changing it to TRUE.  If the flag stays FALSE, CMI will automatically
//		release the received message as soon as the callback function completes.
//		But if the callback function changes the flag to TRUE, CMI will let the
//		event packet and message data persist until Conduit client explicitly
//		releases them by calling IOCTL_CMI_RELEASE_RECEIVED_MESSAGE.
//
//		To sum up, there are three possible cases for the disposition of a
//		CMI_MESSAGE_RECEPTION_EVENT_DATA packet:
//
//		1)  Early release:  the Conduit client explicitly releases the
//			received message *from within the callback itself* by calling
//			IOCTL_CMI_RELEASE_RECEIVED_MESSAGE explicitly there.  (Of
//			course, after doing that, the callback routine can no
//			longer touch the CMI_MESSAGE_RECEPTION_EVENT_DATA packet.)
//			The callback function must therefore leave the packet's
//			<is_delayed_release_requested> element alone (since the whole
//			packet will be deleted anyway), and return STATUS_FILE_DELETED.
//
//		2)  Normal release:  the client wants CMI to implicitly release
//			the received message immediately upon conclusion of the
//			callback routine.  To do this, the callback function must
//			leave the packet's <is_delayed_release_requested> element alone
//			(i.e., set to FALSE) and return STATUS_SUCCESS.  Note that
//			in this case, the Conduit client must *not* ever call
//			IOCTL_CMI_RELEASE_RECEIVED_MESSAGE to release the message explicitly.
//
//		3)  Late release:  the client needs to keep the received message
//			(and its associated CMI_MESSAGE_RECEPTION_EVENT_DATA packet)
//			around for some indefinite time after the end of the callback
//			routine, and will eventually make an explicit call to
//			IOCTL_CMI_RELEASE_RECEIVED_MESSAGE to release the message.
//			To indicate this case, the callback function must set the
//			packet's <is_delayed_release_requested> element to TRUE
//			and return STATUS_SUCCESS.
//
// Members:
//  conduit_id:  the Conduit over which the message was received.
//  originating_sp:  the SP that sent the message to us.
//  num_bytes_floating_data:  the number of bytes of data pointed to
//      by <floating_data_ptr>.
//  floating_data_ptr:  pointer to a logically contiguous stretch of
//      memory on this SP containing the Floating data component of
//      the received message.
//  extra_data:  (TEMPORARY HACK) fixed-size buffer of additional
//      Floating data that was sent.
//	is_delayed_release_requested:  flag indicating whether or not the
//		received message should persist after the callback function is
//		over (so that the Conduit client can later call
//		IOCTL_CMI_RELEASE_RECEIVED_MESSAGE to release the message explicitly,
//		as opposed to having the message automatically released by CMI as
//		soon as the callback function returns.  CMI sets this flag to
//		FALSE before invoking the callback function; the callback must
//		change the value to TRUE if it wishes for the delayed-release
//		behavior described above.
//  version: the version of the protocol that received this message.
//  client_private_links: for use by the client.  useful for queueing
//      the message if is_delayed_release_requested is set to true.
//--
typedef struct _CMI_MESSAGE_RECEPTION_EVENT_DATA
{
    EMCPAL_LIST_ENTRY          client_private_links;
    UCHAR               client_private_event;
    UCHAR               conduit_id;
    BOOLEAN             is_delayed_release_requested;
    CMI_VERSION         version;
    ULONG				num_bytes_floating_data;

    CMI_SP_ID           originating_sp;
    PVOID				floating_data_ptr;
    UCHAR               extra_data[CMI_MESSAGE_NUM_BYTES_EXTRA_DATA];

} CMI_MESSAGE_RECEPTION_EVENT_DATA, *PCMI_MESSAGE_RECEPTION_EVENT_DATA;
//.End

//++
// Type:
//      CMI_CLOSE_COMPLETION_EVENT_DATA
//
// Description:
//      Packet of data whose address is passed to a Conduit's event-handler
//      when a Cmi_Conduit_Event_Close_Completed event is signaled.
//
//		Possible status values for the <ioctl_info_ptr->close_status>
//		structure element are:
//
//		STATUS_SUCCESS:  the specified Conduit was successfully closed.
//		STATUS_FILE_CLOSED:  the specified Conduit was not open.
//
// Members:
//  ioctl_info_ptr:  address of the CMI_CLOSE_CONDUIT_IOCTL_INFO
//      structure that was used to submit the original close request.
//--
typedef struct _CMI_CLOSE_COMPLETION_EVENT_DATA
{
    PCMI_CLOSE_CONDUIT_IOCTL_INFO  ioctl_info_ptr;

} CMI_CLOSE_COMPLETION_EVENT_DATA, *PCMI_CLOSE_COMPLETION_EVENT_DATA;
//.End

//++
// Type:
//      CMI_SP_CONTACT_LOST_EVENT_DATA
//
// Description:
//      Packet of data whose address is passed to a Conduit's event-handler
//      when a Cmi_Conduit_Event_Sp_Contact_Lost event is signaled.
//
//      The event-handler may decide to hold onto the event data to prevent
//      the associated SP from immediately coming back on-line. Here are
//      the options:
//   
//		1)  Normal release:  the client wants CMI to implicitly release
//			the received message immediately upon conclusion of the
//			callback routine.  To do this, the callback function must
//			leave the packet's <is_delayed_release_requested> element alone
//			(i.e., set to FALSE) and return STATUS_SUCCESS.  Note that
//			in this case, the Conduit client must *not* ever call
//			IOCTL_CMI_RELEASE_SP_CONTACT_LOST_EVENT_DATA
//          to release the message explicitly.
//
//		2)  Late release:  the client needs to keep the event
//			around for some indefinite time after the end of the callback
//			routine, and will eventually make an explicit call to
//			IOCTL_CMI_RELEASE_SP_CONTACT_LOST_EVENT_DATA to release the message.
//			To indicate this case, the callback function must set the
//			packet's <is_delayed_release_requested> element to TRUE
//			and return STATUS_SUCCESS.
//
// WARNING: There is a race between IOCTL_CMI_TRANSMIT_MESSAGE and CONTACT_LOST
//     events.  The IOCTL_CMI_TRANSMIT_MESSAGE may be executing in some thread
//     context, and is about to call IoCallDriver to issue the IRP to the CMI
//     driver. If the CONTACT_LOST event occured, and connection was very
//     quickly re-established, then its possible that a message formed before
//     the CONTACT_LOST would be successfully sent after contact was re-established.
//      
//     There are two mechanisms for removing this race:
//       1) Use the "Late release" method descibed aboce, and issue 
//          IOCTL_CMI_RELEASE_SP_CONTACT_LOST_EVENT_DATA when there are 
//          no longer any transmissions outstanding from the client's point of view.
//  
//       2) Issue IOCTL_CMI_CANCEL_TRANSMIT_MESSAGE in the lost contact 
//          lost event handler for every outstanding message sent.
//
//     There is a similar race when receive events are queued to a thread, and
//     there a multiple contact lost events. In this case, it is difficult
//     to order the receive and the lost contact events.  The "Late Release"
//     mechanism prevents the connection from being re-established until
//     the client has disposed of all prior events, thereby eliminating the
//     multiple contact lost events. 
//
//
//
// Members:
//  conduit_id:  ID of the open Conduit which is experiencing this event.
//  remote_sp:  ID of the SP which has died.
//  version: the version of the protocol that received this message.
//	is_delayed_release_requested:  flag indicating whether or not the
//		received message should persist after the callback function is
//		over (so that the Conduit client can later call
//		IOCTL_CMI_RELEASE_SP_CONTACT_LOST_EVENT_DATA to release the event explicitly,
//		as opposed to having the event automatically released by CMI as
//		soon as the callback function returns.  CMI sets this flag to
//		FALSE before invoking the callback function; the callback must
//		change the value to TRUE if it wishes for the delayed-release
//		behavior described above.
//--
typedef struct _CMI_SP_CONTACT_LOST_EVENT_DATA
{
	union {
		EMCPAL_LIST_ENTRY              client_private_links;
		struct _CMI_SP_CONTACT_LOST_EVENT_DATA * FLink;
	};

    UCHAR                   client_private_event;
    UCHAR                   spare[1];
    BOOLEAN                 is_delayed_release_requested;
    CMI_VERSION             version;

    CMI_CONDUIT_ID          conduit_id;
    CMI_SP_ID               remote_sp;

} CMI_SP_CONTACT_LOST_EVENT_DATA, *PCMI_SP_CONTACT_LOST_EVENT_DATA;
//.End

//++
// Type:
//      CMI_RELEASE_SP_CONTACT_LOST_EVENT_DATA
//
// Description:
//      Packet used by IOCTL_CMI_RELEASE_SP_CONTACT_LOST_EVENT_DATA.
//      Holding onto the IOCTL_CMI_RELEASE_SP_CONTACT_LOST_EVENT_DATA
//      allows a CMI client to prevent an SP from coming back on-line
//      after a failure, until the client has finished cleaning up from
//      the failure.
//
//		Possible return values from the ioctl are:
//
//		STATUS_SUCCESS:  the release request succeeded.
//		STATUS_INVALID_BUFFER_SIZE:  the input buffer argument passed
//			to the ioctl is not the correct size.
//
// Members:
//  event_data:  The lost contact event data packet address that
//      was previously passed to the Conduit's event-handler to describe the
//      event.  After this ioctl call is made, the structure pointed
//      at by <event_data> is no longer safe to access.
//--
typedef struct _CMI_RELEASE_SP_CONTACT_LOST_EVENT_DATA_IOCTL_INFO
{
    PCMI_SP_CONTACT_LOST_EVENT_DATA          event_data;

} CMI_RELEASE_SP_CONTACT_LOST_EVENT_DATA_IOCTL_INFO, *PCMI_RELEASE_SP_CONTACT_LOST_EVENT_DATA_IOCTL_INFO;
//.End


//++
// Type:
//      CMI_REGISTER_ALTERNATE_DATA_PATH_IOCTL_INFO
//
// Description:
//      Packet used by IOCTL_CMI_REGISTER_ALTERNATE_DATA_PATH, which
//      specifies to CMIpd how to coordinate with a functional path
//      that handles high-speed CMI data outside of CMI, while leaving
//      CMI to handle low-speed inter-SP data paths and connectivity
//      algorithms.
//
//		Possible return values from the ioctl are:
//
//		STATUS_SUCCESS:  the open request succeeded.
//		STATUS_INVALID_BUFFER_SIZE:  the input buffer argument passed
//			to the ioctl is not the correct size.
//
// Members:
//  datapath_activity_function:  pointer to the function which CMIpd calls within 
//      its heartbeat-generation algorithm to determine whether or not successful 
//      inter-SP data passing has occurred on the alternate data path since the 
//      previous call to the data-activity function.  (For the purposes of defining 
//      the word "previous" in the prior sentence, the result of the first call to 
//      the activity function shall reflect activity on the alternate path during the 
//      interval between the issuance of this registration ioctl and the first call 
//      to the activity function.)  The activity function shall return TRUE iff. traffic 
//      has passed on the high-speed path during the inter-call interval.
//--
typedef struct _CMI_REGISTER_ALTERNATE_DATA_PATH_IOCTL_INFO
{
    BOOLEAN     (*datapath_activity_function) (VOID);

} CMI_REGISTER_ALTERNATE_DATA_PATH_IOCTL_INFO, *PCMI_REGISTER_ALTERNATE_DATA_PATH_IOCTL_INFO;
//.End

//++
// Type:
//      CMI_BUNDLING_CONTROL
//
// Description: Specifies limits on bundling.  Useful for
//     understanding performance. 
//
//--
typedef struct _CMI_BUNDLING_CONTROL
{
    // How many bundles may be sent by this SP? The actual
    // number is the minimum of: 1) a compile time parameter
    // in CMID, 2) this value, 3) a value specified by the
    // other SP.
	ULONG		sendMaxBundles;

    // How many messages may be sent by this SP in any bundle? The actual
    // number is the minimum of: 1) a compile time parameter
    // in CMID, 2) this value, 
	ULONG		sendMaxMessagesPerBundle;

    // How many bytes may be sent by this SP in any bundle, if the
    // bundle contains more than one message? The actual
    // number is the minimum of: 1) this value.  This does not
    // affect bundles with one message.
	ULONG		sendMaxBytesPerBundle;

    // Should bundles be compressed if the receiver supports compression?
    BOOLEAN     sendBundleCompressionEnabled;

    // What is the goal for a minimum bundle size?  The larger the value,
    // the larger the target bundle size.  This affects bundle creation
    // only if there are other bundles in flight.
    ULONG       sendMinimumBundleSizeTarget;

    ULONG       spare[2];

} CMI_BUNDLING_CONTROL, *PCMI_BUNDLING_CONTROL;
//.End



//++
// Type:
//      _CMI_BUNDLING_CONTROL_IOCTL_INFO
//
// Description: 
//    Used by:
//     IOCTL_CMI_GET_BUNDLING_CONTROL
//     and IOCTL_CMI_SET_BUNDLING_CONTROL
// Members:
//    spId      - Specifies the SP the control is for.
//    bundleControl - the control structure set or gotten.
//--
typedef struct _CMI_BUNDLING_CONTROL_IOCTL_INFO
{
    CMI_SP_ID                spId;               
    CMI_BUNDLING_CONTROL     bundleControl;

} CMI_BUNDLING_CONTROL_IOCTL_INFO, *PCMI_BUNDLING_CONTROL_IOCTL_INFO;


//++
// Type:
//      _CMI_GET_PATH_STATUS_IOCTL_INFO
//
// Description: 
//    Used by:
//     IOCTL_CMI_GET_PATH_STATUS
// Members:
//   (Input) spId      - Specifies the SP the query is for.
//   numWorkingPaths      - The number of functioning paths
//   numPossiblePaths     - The number of possible paths to the SP.
//--
typedef struct _CMI_GET_PATH_STATUS_IOCTL_INFO
{
    CMI_SP_ID                spId;               
    ULONG                    numWorkingPaths;
    ULONG                    numPossiblePaths;

} CMI_GET_PATH_STATUS_IOCTL_INFO, *PCMI_GET_PATH_STATUS_IOCTL_INFO;
#endif // __CMI_UPPER_INTERFACE__
