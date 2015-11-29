/*******************************************************************************
 * Copyright (C) EMC Corporation, 1997 - 2008
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * ScsiTargHostPorts.h
 *
 * This header file contains structures and constants which are needed by other
 * drivers and applications when performing Host Port IOCTLs to TCD.
 *
 ******************************************************************************/

#ifndef ScsiTargHostPorts_h 
#define ScsiTargHostPorts_h 1

#include "spid_types.h"
#include <ipexport.h>
#include "speeds.h"

/*************/
/* HOST PORT */
/*************/
#define MAC_ADDR_LEN 6

typedef struct _MAC_ADDRESS {

	UCHAR Address[MAC_ADDR_LEN];
	
}MAC_ADDRESS, *PMAC_ADDRESS;

// The following are valid values for HOST_PORT_ENTRY.PortType
// and HOST_PORT_KEY.PortType

typedef enum HOST_PORT_TYPE
{
    HOST_NONE = 0,
    HOST_FIBRE_CHANNEL = 2,	
    HOST_ISCSI = 3,
    HOST_SAS = 4,
    HOST_FCOE = 5,
    HOST_PORT_TYPE_MAX = 6
} 
HOST_PORT_TYPE;

// A particular (target-capable) port is identified by 
// its logical port number and the ID of the SP it resides on,
// along with the type of port.
// We use bit fields to get a 4 byte aligned structure.
//
// PortType : HOST_FIBRE_CHANNEL or HOST_ISCSI
// SPId     : Which SP is the port connected to: SP_A or SP_B
// LogicalPortId: A unique number identifying the port within one SP.

typedef struct HOST_PORT_KEY {

    HOST_PORT_TYPE	PortType:16;	
    SP_ID			SPId:8;		
    unsigned int	LogicalPortId:8;	

} HOST_PORT_KEY, *PHOST_PORT_KEY;

// CMIscd needs to be notiified when an Initiator from a remote array is being enabled or disabled for
// MirrorView (/S).  Scsitarg provides those notifications via CMIscd interface calls (EnableCMIConnectionIntent, 
// EnableCMIConnectionCancel, EnableCMIConnectionCommit, and DisableCMIConnection).  Those interface calls need 
// to specify the Initiator's name (WWN for fibre channel, iqn for iSCSI) and the Target.
//
// For fibre channel, the Target is simply the FC port which can be identified by the SP and Port Number.
//
// For iSCSI, the Target must also specify which miniport portal number (portal_nr) was used by Scsitarg when it 
// submitted the TCP/IP Address that the Initiator is connected to.  In the case where the notification is for
// and ITNexus whose port is not on the local SP, we are also providing the Virtual Port number.
//
// The CMI_SCD_MV_TARGET is used to contain the "Target" argument to the CMIscd interface calls.
//
// PortType : HOST_FIBRE_CHANNEL or HOST_ISCSI
// SPId              : SP_A or SP_B
// PhysicalPortNumber: A unique number identifying the port within one SP.
// PortalNumber      : The portal_nr described above.
// VirtualPortNumber : Virtual Port number described above.
// Reserved          : Not used; must be zero.

typedef struct _CMI_SCD_MV_TARGET {

	HOST_PORT_TYPE	PortType:8;
	SP_ID			SPId:8;
	unsigned int	PhysicalPortNumber:8;
	unsigned int	PortalNumber:8;
	USHORT			VirtualPortNumber;
	USHORT			Reserved;

} CMI_SCD_MV_TARGET, *PCMI_SCD_MV_TARGET;

// A HOST_PORT_KEY_LIST structure is used to contain a list of Host Port keys.

typedef struct _HOST_PORT_KEY_LIST 
{
    ULONG           Revision;
    ULONG           Count;          // Number of elements
    HOST_PORT_KEY   Key[1];
} 
HOST_PORT_KEY_LIST, *PHOST_PORT_KEY_LIST;

#define CURRENT_HOST_PORT_KEY_LIST_REV	1

// Determine the number of bytes required for a HOST_PORT_KEY_LIST
// with KeyCount elements.
#define HPORT_KEY_LSIZE(KeyCount)                             \
        (sizeof(HOST_PORT_KEY_LIST) - sizeof(HOST_PORT_KEY) + \
        (KeyCount) * sizeof(HOST_PORT_KEY))

// Compares two HOST_PORT_KEY structures.

#define HP_EQUAL(a,b)	\
		(((a)->SPId == (b)->SPId) && ((a)->PortType == (b)->PortType) && ((a)->LogicalPortId == (b)->LogicalPortId))

#define PEER_COUNTERPART_PORT(a,b)	\
		(((a)->SPId != (b)->SPId) && ((a)->PortType == (b)->PortType) && ((a)->LogicalPortId == (b)->LogicalPortId))

#define FC_HP_ITN_EQUAL(a,b)							\
		(((a)->PortType == (b)->PortType) &&			\
		((a)->SPId == (b)->fc.SPId) &&					\
		((a)->LogicalPortId == (b)->fc.LogicalPortId))

#define FCOE_HP_ITN_EQUAL(a,b)							\
		(((a)->PortType == (b)->PortType) &&			\
		((a)->SPId == (b)->fcoe.SPId) &&					\
		((a)->LogicalPortId == (b)->fcoe.LogicalPortId))

#define SAS_HP_ITN_EQUAL(a,b)							\
		(((a)->PortType == (b)->PortType) &&			\
		((a)->SPId == (b)->sas.SPId) &&					\
		((a)->LogicalPortId == (b)->sas.LogicalPortId))


///////////////////////////////////////////////////////////////////////////////
// A HOST_PORT_PARAM_FIBRE contains the Fibre Channel specific parts 
// of the HOST_PORT_PARAM structure.
//
// TargetId:
//		For Fibre Channel this structure
//		contains the Node World Wide Name and the Port World Wide Name.  Note
//		that WorldWideNames should be assigned by CLARiiON for the port.  No 
//		two FC Ports should be assigned the same WWN.
//
// PreferredLoopId:
//		Preferred Loop ID to provide to miniport.
// RequestedLinkSpeed:
//      Specifies a requested FC link speed. 

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
typedef struct _HOST_PORT_PARAM_FIBRE {

	TI_OBJECT_ID	TargetId;
	UCHAR			PreferredLoopId;
	UCHAR			ReservedBytes[3];
	ULONG			RequestedLinkSpeed;
	ULONG			Reserved[16];       // Must be zero

} HOST_PORT_PARAM_FIBRE, *PHOST_PORT_PARAM_FIBRE;

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
//////////////////////////////////////////////////////////////////////////////
// A HOST_PORT_PARAM_SAS contains the SAS specific parts 
// of the HOST_PORT_PARAM structure.
//
// TargetId:
//		For SAS this structure
//		contains the Device Name and SAS address which is similar to Node World 
//		Wide Name and the Port World Wide Name in Fibre channel.  Note
//		that WorldWideNames should be assigned by CLARiiON for the port.  No 
//		two SAS Ports should be assigned the same WWN.
//
// RequestedLinkSpeed:
//      Specifies a requested FC link speed. 

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
typedef struct _HOST_PORT_PARAM_SAS {
	
	TI_OBJECT_ID	TargetId;
	ULONG			RequestedLinkSpeed;
	USHORT			Reserved;			 // Must be zero
	ULONG			Reserved1[8];        // Must be zero

} HOST_PORT_PARAM_SAS, *PHOST_PORT_PARAM_SAS;

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */

// An IP_KEY uniquely identifies an IP_PARAMS structure.
// An IP_KEY has no information about the IP endpoint itself.

typedef TI_OBJECT_ID IP_KEY, *PIP_KEY;

// Defines the maximum size of dns names supported.

#define NETWORK_NAME_SIZE 256
#define MAX_IP_PARAMS_PER_PORT 96

///////////////////////////////////////////////////////////////////////////////
// An IP_PARAMS structure contains configuration information 
// about an IP end-point. 
//
// Currently, this structure cannot cross ports, and it
// is therefore persisted with the HOST_PORT_PARAM
// structure.  If IP address could span ports, then
// this would need to be persisted separately.
//
// Key:
//   Identifies this structure.  Must be unique across
//   all ports.
//
// VirtualPortNumber: Virtual Port number.
//
// VLAN: Virtual LAN tag ID.
//
// IpAddr: IP Address.
//
// SubnetMask: Subnet mask.
//
// DefaultGateway: Default Gateway IP Address.

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */

typedef enum _AL_IP_VERSION {
	AL_IP_NONE,
	AL_IPV4,
	AL_IPV6
} AL_IP_VERSION, *PAL_IP_VERSION;

typedef struct _IP_PARAMS {

	IP_KEY			Key;
	USHORT			VirtualPortNumber;
	UCHAR			Reserved1;
	UCHAR			IpType;
	ULONG			VLAN;
	IN6_ADDR		IpAddr;			// IPv4 or IPv6 address.
	IN6_ADDR   		SubnetMask;		// IPv4 only.
	IN6_ADDR		DefaultGateway;	// This field not used.
	ULONG			Reserved2[4];

} IP_PARAMS, *PIP_PARAMS;

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */

// VLAN masks.

#define VLAN_VALID_FLAG		0x80000000
#define VLAN_RESERVED_BITS	0x7FFF0000
#define	VLAN_TAG_MASK		0x0000FFFF
#define	VLAN_AUTO_CONFIG	0x40000000

// Macro to compare two IN6_ADDR sructures.

#define IN6_ADDR_NOT_EQUAL(t, a, b)	((t) == AL_IPV4) ? memcmp((a), (b), 4) != 0 : memcmp((a), (b), sizeof(IN6_ADDR)) != 0


///////////////////////////////////////////////////////////////////////////////
// A LOGIN_AUTHENTICATION_TYPE specifies either:
//  AT_NONE - no authentication at login
//  AT_CHAP - Chap authentication.

typedef enum _LOGIN_AUTHENTICATION_TYPE {
    AT_NONE,
    AT_CHAP
} LOGIN_AUTHENTICATION_TYPE;

///////////////////////////////////////////////////////////////////////////////
// A HOST_PORT_PARAM_ISCSI structure contains configuration information 
// about host port that is specific to iSCSI ports.
//
// iSCSIName: The null terminated iSCSI name of this port.
// 
// iSCSIAlias: The null terminated iSCSI alias name of this port.
//
// RequestedIPSpeed: Specifies a requested iSCSI IP speed. 
// 
// RequestedMTU: Specifies the TCP Max Transfer Unit.
// 
// RequestedHostWindowSize: Specifies the maximum TCP Window Size
// 							to be used for hosts.
// 
// RequestedAppWindowSize: Specifies the maximum TCP Window Size
// 						   to be used for layered applications.
// 
// RequestedFlowControl: Specifies a requested flow control
//
// NumIpParams: The number of IpParams structures that are defined.
//              Must be <= MAX_IP_PARAMS_PER_PORT.
//
// IpParams: describe what IP addresses to respond to.

#define MAX_ISCSI_NAME_SIZE	256

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
typedef struct _HOST_PORT_PARAM_ISCSI {

	UCHAR       iSCSIName[MAX_ISCSI_NAME_SIZE];
	UCHAR       iSCSIAlias[MAX_ISCSI_NAME_SIZE];
	ULONG       RequestedIpSpeed;
	ULONG		RequestedMTU;
	ULONG		RequestedHostWindowSize;
	ULONG		RequestedAppWindowSize;
	ULONG		Reserved[8];
	USHORT		Reserved1;
	USHORT		RequestedEdcMode;
	USHORT		RequestedFlowControl;
	USHORT		NumIpParams;
	IP_PARAMS	IpParams[MAX_IP_PARAMS_PER_PORT];

} HOST_PORT_PARAM_ISCSI, *PHOST_PORT_PARAM_ISCSI;

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */

///////////////////////////////////////////////////////////////////////////////
// A HOST_PORT_PARAM_FILE structure contains configuration information 
// about host port that is specific to file ports.
//


#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */

#ifndef ALAMOSA_WINDOWS_ENV

#define MAX_FILE_PORT_NAME_SIZE 256
typedef struct _HOST_PORT_PARAM_FILE 
{
    UCHAR       fileName[MAX_FILE_PORT_NAME_SIZE];
    UCHAR       fileAlias[MAX_FILE_PORT_NAME_SIZE];
    ULONG       RequestedIpSpeed;
    ULONG       RequestedMTU;
    ULONG       RequestedHostWindowSize;
    ULONG       RequestedAppWindowSize;
    ULONG       Reserved[8];
    USHORT      Reserved1;
    USHORT      RequestedEdcMode;
    USHORT      RequestedFlowControl;
    USHORT      NumIpParams;
    IP_PARAMS	IpParams[MAX_IP_PARAMS_PER_PORT];
}
HOST_PORT_PARAM_FILE, *PHOST_PORT_PARAM_FILE;

typedef struct _HOST_PORT_PARAM_BOND 
{
    UCHAR       bondName[MAX_FILE_PORT_NAME_SIZE];
    UCHAR       bondAlias[MAX_FILE_PORT_NAME_SIZE];
    ULONG       RequestedIpSpeed;
    ULONG       RequestedMTU;
    ULONG       RequestedHostWindowSize;
    ULONG       RequestedAppWindowSize;
    ULONG       Reserved[8];
    USHORT      Reserved1;
    USHORT      RequestedEdcMode;
    USHORT      RequestedFlowControl;
    USHORT      NumIpParams;
    IP_PARAMS	IpParams[MAX_IP_PARAMS_PER_PORT];
}
HOST_PORT_PARAM_BOND, *PHOST_PORT_PARAM_BOND;

#endif /* ALAMOSA_WINDOWS_ENV - ARCH - admin_differences */

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */

///////////////////////////////////////////////////////////////////////////////
// An FCOE_VLAN_PARAMS structure contains configuration information 
// about an FCoE end-point. 
//
// PortWwn:
//   Identifies this VLAN
//
// VLAN: Virtual LAN tag ID.
//

typedef struct _FCOE_VLAN_PARAMS {

	TI_OBJECT_ID	PortWwn;
	UINT_32E		VLAN;
	ULONG			Reserved1;
	ULONG			NameID;
	ULONG			FC_Map;
	TI_OBJECT_ID	FabricWwn;
	ULONG			Reserved2[10];

} FCOE_VLAN_PARAMS, *PFCOE_VLAN_PARAMS;


//Maximum Vlans allowed per FCoE port.

#define MAX_VLAN_PARAMS_PER_PORT     32

///////////////////////////////////////////////////////////////////////////////
// A HOST_PORT_PARAM_FCOE structure contains configuration information 
// about host port that is specific to FCoE ports.
//
// RequestedPortSpeed: Specifies a requested FCoE Port speed. 
//
//RequestedMTU: Specifies a requested FCoE Port MTU.
//
// NumVlans: The number of VlanParam structures that are defined.
//              Must be <= MAX_VLAN_PARAMS_PER_PORT.
//
// FCOE_VLAN_PARAMS: describe what Vlans to respond to.

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
typedef struct _HOST_PORT_PARAM_FCOE {

	ULONG				RequestedPortSpeed;
	ULONG				RequestedMTU;
	ULONG				Reserved[13];	// Must be zero.
	ULONG				NumVlans;
	FCOE_VLAN_PARAMS	VlanParams[MAX_VLAN_PARAMS_PER_PORT];

} HOST_PORT_PARAM_FCOE, *PHOST_PORT_PARAM_FCOE;

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */

///////////////////////////////////////////////////////////////////////////////
// A INITIATOR_BEHAVIOR structure defines how the array should
// behave for a particular Nexus.
//
// InitiatorType: Defines what class of behavior we want. This
//   is usually a function of what OS the host is running.
//   A value of USE_DEFAULT_INITIATOR_TYPE indicates that
//   we should look up the session, I_T nexus, initiator,
//   host, system heirarchy for the actual initiator type.
//
// InitiatorOptions: Modifies the behavior specified by
//   InitiatorType.  The bit definitions are the same
//   as those for SYSCONFIG_OPTIONS::system_intfc_options.
//   A value of USE_DEFAULT_OPTIONS indicates that
//   we should look up the session, I_T nexus, initiator,
//   host, system heirarchy for the actual initiator type.   
//
// StorageGroupWWN: Determines which Storage Group
//   is visible to the host via the associated path.
//   A value of (SC_DEFAULT_VARRAY_UPPER, SC_DEFAULT_VARRAY_LOWER) 
//   indicates that
//   we should look up the session, I_T nexus, initiator,
//   host, system heirarchy for the actual initiator type.   

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
typedef struct _INITIATOR_BEHAVIOR {

	INITIATOR_TYPE	InitiatorType;
	ULONG			InitiatorOptions;
	ULONG			Reserved[2];	// Must be zero.
	SG_WWN			StorageGroupWWN;

} INITIATOR_BEHAVIOR, * PINITIATOR_BEHAVIOR;
#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */

///////////////////////////////////////////////////////////////////////////////
// A HOST_PORT_PARAM may be set for target-mode capable ports.
// When set, these parameters persist until changed or explicitly removed.
// Events on real ports that match the Key will use this data, if it has been
// configured.
//
// Revision:
//		Format of this structure.
//
// Key:
//	    Defines exactly which port the parameters are for. Key consists of
//      port type, SPID, and LogicalBusNumber. (Not chaneable).
//
// PortNameLength:
//		Number of valid bytes in the PortName[] field.
//
// PortName:
//		Uninterpretted storage set and retrieved by the GUI.  It is intended
//		to be used for a "friendly" name for the user. (Changeable).
//
// fc:  parameters specific to FibreChannel ports
// iscsi: parameters specific to iSCSI ports
// fcoe: parameters specific to FCoE ports
// sas: parameters specific to SAS ports

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
typedef struct _HOST_PORT_PARAM 
{
    ULONG           Revision;
    HOST_PORT_KEY   Key;
    ULONG           Reserved[9];
    ULONG           PortNameLength;
    UCHAR           PortName[K10_MAX_HPORT_NAME_LEN];

    union 
    {
        HOST_PORT_PARAM_FIBRE	fc;
        HOST_PORT_PARAM_ISCSI   iscsi;
        HOST_PORT_PARAM_FCOE	fcoe;
        HOST_PORT_PARAM_SAS     sas;
#ifndef ALAMOSA_WINDOWS_ENV
        HOST_PORT_PARAM_FILE    file;
        HOST_PORT_PARAM_BOND    bond;
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - admin_differences */
    };

} HOST_PORT_PARAM, *PHOST_PORT_PARAM;

#define CURRENT_HOST_PORT_PARAM_REV	10

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */

///////////////////////////////////////////////////////////////////////////////
// A HOST_PORT_ENTRY_FIBRE contains Fibre Channel specific data about a
// specific  port.  This data is not persisted.
//
// PortState:
//		State of the FC port.
//
// ConnectType:
//		The type of connection the port has.
//
// AcquiredLoopId:
//		Loop ID acquired during negotiation.
//
// NPortId:
//		The S_ID when communicating over fibre channel.
//
// FabricWWN:
//		The WWN of the switch.
//
// AvailableFCSpeeds:
//		A bit pattern representing available FC link speeds, or 0.
//
// CurrentLinkSpeed
//		The current FC link speed, or 0.
//
// Reserved:
//		Must be zero.
///////////////////////////////////////////////////////////////////////////////

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
typedef struct _HOST_PORT_ENTRY_FIBRE {

	ULONG			PortState;
	UCHAR			ConnectType;
	UCHAR			AcquiredLoopId;
	UCHAR			Resv1[2];	// Must be 0.
	ULONG			NPortId;
	ULONG			Resv2;	// Must be 0.
	TI_OBJECT_ID	FabricWWN;
	ULONG			AvailableFCSpeeds;
	ULONG			CurrentLinkSpeed;
	ULONG			Resv3[12];	// Must be 0.

} HOST_PORT_ENTRY_FIBRE, *PHOST_PORT_ENTRY_FIBRE;

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
// Values for PortState.

#define PSTATE_OFFLINE				0x00
#define PSTATE_OFFLINE_PENDING		0x01
#define PSTATE_POINT_TO_POINT		0x02
#define PSTATE_NON_PARTICIPATING	0x03
#define PSTATE_PARTICIPATING		0x04

// Values for ConnectType.

#define CTYPE_POINT_TO_POINT		0x01
#define CTYPE_ARBITRATED_LOOP		0x02
#define CTYPE_NONE					0x05

// Values for LinkState (as per enum FCOE_LINK_STATE_INFO in cpd_interface.h)

#define LSTATE_INFO_INIT			0
#define LSTATE_INFO_NO_ADD_INFO	1
#define LSTATE_INFO_NO_FCF			2


///////////////////////////////////////////////////////////////////////////////
// A HOST_PORT_ENTRY_ISCSI may be read for real or previously configured iSCSI ports.
//
// State:
//		State of the iSCSI port.
//
// NumberOfVirtualPortsAllowed:
//		Number of Virtual Ports (VLANs) that can be set on this port.
//
// AvailableFlowControl:
// 		A bit pattern representing available flow control settings.
//
// CurrentFlowControl:
// 		The current flow control setting.
//
// NumberOfIPv4AddressesPerVP
//		The number of IPv4 Addresses allowed per Virtual Port.
//
// NumberOfIPv6AddressesPerVP
//		The number of IPv6 Addresses allowed per Virtual Port.
//
// AvailableIpSpeeds:
//		A bit pattern representing available iSCSI link speeds.
//
// CurrentIpSpeed
//		The current iSCSI link speed.
//
// MacAddress:
//		The MAC address of this port.
//
// MaxMTU:
//		The Max TCP Transfer Unit supported by this port.
//
// CurrentMTU:
//		The current TCP MTU for this port.
//
// MinMTU:
//		The Min TCP Transfer Unit supported by this port.
//
// MaxWindowSize:
// 		The maximum window size.
//
// MinWindowSize:
// 		The minimum window size.
//
// CurrentHostWindowSize:
// 		The current window size for host connections.
//
// CurrentAppWindowSize:
// 		The current window size for layered application connections.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
typedef struct _HOST_PORT_ENTRY_ISCSI {

	USHORT			State;
	USHORT			NumberOfVirtualPortsAllowed;
	USHORT			AvailableFlowControl;
	USHORT			CurrentFlowControl;
	USHORT			NumberOfIPv4AddressesPerVP;
	USHORT			NumberOfIPv6AddressesPerVP;
	ULONG			AvailableIpSpeeds;
	ULONG			CurrentIpSpeed;
	UCHAR			MacAddress[6];
	USHORT			DesiredEdcMode;
	ULONG			MaxMTU;
	ULONG			CurrentMTU;
	ULONG			MinMTU;
	ULONG			MaxWindowSize;
	ULONG			MinWindowSize;
	ULONG			CurrentHostWindowSize;
	ULONG			CurrentAppWindowSize;
	ULONG			Resv3[6];	// Must be 0.

} HOST_PORT_ENTRY_ISCSI, *PHOST_PORT_ENTRY_ISCSI;

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
///////////////////////////////////////////////////////////////////////////////
// A FCOE_ACQUIRED structure contains info about virtual port returned from miniport.
//
// VNPortMacAddress
//		MAC address of virtual port.
//
// VNPortLinkState
//		Link state (or reason for a link event) of virtual port
//
// FabricWWN
//		Fabric that VLAN is attached to.
//
// VNPortVLAN
//		VLAN id returned from miniport in case of vlan auto config.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
typedef struct _FCOE_ACQUIRED {

	MAC_ADDRESS		VNPortMacAddress;
	USHORT			VNPortLinkState;
	TI_OBJECT_ID	FabricWWN;
	UINT_32E		VNPortVLAN;

} FCOE_ACQUIRED, *PFCOE_ACQUIRED;
#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */

///////////////////////////////////////////////////////////////////////////////
// A HOST_PORT_ENTRY_FCOE may be read for real or previously configured FCoE ports.
//
// PortState:
//		State of the FCoE port.
//
// AvailableSpeeds:
//		A bit pattern representing available link speeds.
//
// CurrentSpeed
//		The current link speed.
//
// ENodeMacAddress:
//		The MAC address of this port.
//
// FabricWWN:
//		The WWN of the switch.
//
// AvailableMTU:
//		The Max Transfer Unit supported by this port.
//
// CurrentMTU:
//		The current MTU for this port.
//
// NumberOfVlansSupported:
//		Number of IP Addresses that can be set on this port.
//
// AcquiredVNPortInfo[]
//		Info acquired from miniport for all virtual ports on this physical port.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
typedef struct _HOST_PORT_ENTRY_FCOE {

	ULONG			PortState;
	ULONG			AvailableSpeeds;
	ULONG			CurrentSpeed;
	ULONG			MinMTU;
	ULONG			MaxMTU;
	ULONG			CurrentMTU;
	UCHAR			ConnectType;
	MAC_ADDRESS		ENodeMacAddress;
	UCHAR			NumberOfVlansSupported;
	FCOE_ACQUIRED	AcquiredVNPortInfo[MAX_VLAN_PARAMS_PER_PORT];

} HOST_PORT_ENTRY_FCOE, *PHOST_PORT_ENTRY_FCOE;

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
///////////////////////////////////////////////////////////////////////////////
// A HOST_PORT_ENTRY_SAS contains SAS specific data about a
// specific  port.  This data is not persisted.
//
// PhyMap:
//		Indicates which PHY's on the SAS controller are indicated as part of this 
//		port (least significant bit is PHY 0, next least significant bit is PHY 1, etc
//		
// NumberOfPhys:
//		Total number of PHY's comprising this port
//
// NumberOfPhysEnabled:
//		Total number of PHY's that are enabled.
//
// NumberOfPhysUp:
//		Total number of PHY's that are up.
//
// PortState:
//		State of the SAS port.
//
// AvailableSASpeeds:
//		A bit pattern representing available SAS link speeds, or 0.
//
// CurrentLinkSpeed
//		The current SAS link speed, or 0.
//
// Reserved:
//		Must be zero.
///////////////////////////////////////////////////////////////////////////////

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
typedef struct _HOST_PORT_ENTRY_SAS {
	
	USHORT			PhyMap;
	USHORT			NumberOfPhys;
	USHORT			NumberOfPhysEnabled;
	USHORT			NumberOfPhysUp;
	USHORT			PortState;
	ULONG			AvailableSASSpeeds;
	USHORT			CurrentLinkSpeed;
	USHORT			Resv;				// Must be 0.
	ULONG			Resv1[2];			// Must be 0.

} HOST_PORT_ENTRY_SAS, *PHOST_PORT_ENTRY_SAS;

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */


///////////////////////////////////////////////////////////////////////////////
// A HOST_PORT_ENTRY_FILE contains FILE specific data about a
// specific  port.  This data is not persisted.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */

#ifndef ALAMOSA_WINDOWS_ENV

typedef struct _HOST_PORT_ENTRY_FILE 
{
    USHORT  State;
    USHORT  NumberOfIpAddressesAllowed;
    USHORT  AvailableFlowControl;
    USHORT  CurrentFlowControl;
    ULONG   Resv1;
    ULONG   AvailableIpSpeeds;
    ULONG   CurrentIpSpeed;
    UCHAR   MacAddress[6];
    USHORT  DesiredEdcMode;
    ULONG   MaxMTU;
    ULONG   CurrentMTU;
    ULONG   MinMTU;
    ULONG   MaxWindowSize;
    ULONG   MinWindowSize;
    ULONG   CurrentHostWindowSize;
    ULONG   CurrentAppWindowSize;
    ULONG   Resv3[6];               // Must be 0.
} 
HOST_PORT_ENTRY_FILE;

#define MAX_FILES_PER_BOND 12

typedef struct _HOST_PORT_ENTRY_BOND
{
    USHORT  State;
    USHORT  NumberOfIpAddressesAllowed;
    USHORT  AvailableFlowControl;
    USHORT  CurrentFlowControl;
    ULONG   Resv1;
    ULONG   AvailableIpSpeeds;
    ULONG   CurrentIpSpeed;
    UCHAR   MacAddress[6];
    USHORT  DesiredEdcMode;
    ULONG   MaxMTU;
    ULONG   CurrentMTU;
    ULONG   MinMTU;
    ULONG   MaxWindowSize;
    ULONG   MinWindowSize;
    ULONG   CurrentHostWindowSize;
    ULONG   CurrentAppWindowSize;
    ULONG   Resv3[6];               // Must be 0.
    UCHAR   FilePortIndex[MAX_FILES_PER_BOND];
    USHORT  NumOfPorts;
} 
HOST_PORT_ENTRY_BOND;

#endif /* ALAMOSA_WINDOWS_ENV - ARCH - admin_differences */

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */


///////////////////////////////////////////////////////////////////////////////
// A HOST_PORT_ENTRY may be read for real or previously configured FE ports.
//
// Revision:
//		Revision of data structure.
//
// Params:
//      Persistent information about this port, when Temporary == FALSE.
//		Otherwise default information about this port.  Params.Key is unique 
//		across all HOST_PORTS.
//
// SystemIoBusNumber, SlotNumber, and PortNumber:
//		Together these define the port's physical location in the machine (less
//		SP numbering).  They are the PCI Bus number, the Adapter Slot number on
//		that bus, and the SCSI Port number on the adapter card, respectively.
//
// Temporary:
//		TRUE if there is no persistent information stored about this port.
//
// OnLine:
//		TRUE if the port physically exists.
//		The following fields are valid only if OnLine is TRUE.
//
// LinkStatus:
//		Status of the physical link.
//
// SfpState:
//		Status of Small Formfactor Pluggable component.
//
// PhysicalPortNumber:
//		Value expected to be printed above the pysical port (on the back of the SP).
//
// PortType:
//	    Defines what type of port (Parallel or Fibre).
//
// Reserved:
//		Must be zeroes.
//
// fc:    Fibre Channel specific information.
// iscsi: iSCSI specific information.
// fcoe: FCoE specific information
//
///////////////////////////////////////////////////////////////////////////////

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
typedef struct _HOST_PORT_ENTRY 
{
    ULONG           Revision;
    HOST_PORT_PARAM Params;				// Persistently stored state.
    UCHAR           SystemIoBusNumber;	// PCI bus number.
    UCHAR           SlotNumber;			// PCI slot number.
    UCHAR           PortNumber;			// Port number on the card.
    UCHAR           Temporary;			// TRUE if params are not persisted.
    UCHAR           OnLine;				// Currently operational.
    UCHAR           LinkStatus;
    UCHAR           SfpState;
    UCHAR           PhysicalPortNumber;
    HOST_PORT_TYPE  PortType;
    UCHAR           PciFuncAdjustment;	// Used by HostConfCLI only.
    UCHAR           Resv[3];
    ULONG           Reserved[7];
    union 
    {
        HOST_PORT_ENTRY_FIBRE   fc;
        HOST_PORT_ENTRY_ISCSI   iscsi;
        HOST_PORT_ENTRY_FCOE    fcoe;
        HOST_PORT_ENTRY_SAS     sas;
#ifndef ALAMOSA_WINDOWS_ENV
        HOST_PORT_ENTRY_FILE    file;
        HOST_PORT_ENTRY_BOND    bond;
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - admin_differences */
    };
}
HOST_PORT_ENTRY, *PHOST_PORT_ENTRY;

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
// This enum defines the various revisions of the HOST_PORT structure

typedef enum  _HOST_PORT_REV
{
	HOST_PORT_ENTRY_PRE_R16_REV = 7,
	HOST_PORT_ENTRY_R16_REV = 8,
	HOST_PORT_ENTRY_R31_REV = 9

} HOST_PORT_REV;

// Values for HOST_PORT_ENTRY.SfpState:

#define	HPE_SFP_NONE	0
#define	HPE_SFP_FAULTED	1
#define	HPE_SFP_REMOVED	2
#define	HPE_SFP_OKAY	3
#define HPE_SFP_UNEQUAL	4

typedef enum _HOST_PORT_KEY_LIST_REV
{
	HOST_PORT_KEY_LIST_PRE_R16_REV	= 1,
	HOST_PORT_KEY_LIST_R16_REV = 2,
	HOST_PORT_KEY_LIST_R31_REV = 3

} HOST_PORT_KEY_LIST_REV;

#define CURRENT_HOST_PORT_ENTRY_REV	HOST_PORT_ENTRY_R31_REV

#define CURRENT_HOST_PORT_KLIST_REV	HOST_PORT_KEY_LIST_R31_REV

// A HOST_PORT_LIST structure is used to contain a list of all Host Ports.

typedef struct _HOST_PORT_LIST 
{
    ULONG           Revision;
    ULONG           Count;
    HOST_PORT_ENTRY Info[1];
} 
HOST_PORT_LIST, *PHOST_PORT_LIST;

// Determines the number of bytes necessary for
// a HOST_PORT_LIST that describes PortCount ports.
#define HOST_PORT_LSIZE(PortCount)                          \
        (sizeof(HOST_PORT_LIST) - sizeof(HOST_PORT_ENTRY) + \
        (PortCount) * sizeof(HOST_PORT_ENTRY))

// An HPORT_PARAM_LIST structure is used to persist a list of Host Ports.

typedef struct _HPORT_PARAM_LIST 
{
    ULONG           Revision;
    ULONG           Count;
    HOST_PORT_PARAM Info[1];
}
HPORT_PARAM_LIST, *PHPORT_PARAM_LIST;

// Determines the number of bytes necessary for
// a HPORT_PARAM_LIST that describes PortCount ports.
#define HPORT_PARAM_LSIZE(PortCount)                            \
        (sizeof(HPORT_PARAM_LIST) - sizeof(HOST_PORT_PARAM) +   \
        (PortCount) * sizeof(HOST_PORT_PARAM))


// A VIRTUAL_PORT_SELECTOR is used to specify a group of 
// IP Addresses that are assigned to a specific iSCSI port.

typedef struct _VIRTUAL_PORT_SELECTOR {

	ULONG	Reserved;
	ULONG	PhysicalPortNumber;
	USHORT	VirtualPortNumber;
	USHORT	IpType;	// AL_IPV4, AL_IPV6 or AL_IP_NONE.

} VIRTUAL_PORT_SELECTOR, *PVIRTUAL_PORT_SELECTOR;


// A PORTAL_NUMBER_LIST contains the portal numbers
// (used for miniport interaction) currently assigned
// to a specific group of IP Addresses on an iSCSI port.

typedef struct _PORTAL_NUMBER_LIST {

	ULONG	Reserved;
	ULONG	PortalNumberCount;
	ULONG	PortalNumber[1];

} PORTAL_NUMBER_LIST, *PPORTAL_NUMBER_LIST;

#define PORTAL_NUMBER_LIST_SIZE(s)	\
	sizeof(PORTAL_NUMBER_LIST) - sizeof(ULONG) + ((s) * sizeof(ULONG))

   /********************************************/
   /*               Authentication             */
   /********************************************/

// Maximum length (including Null terminator) for Authentication iSCSI name entry
/* BUG - this is > max u8 */
#define AUTH_MAX_iSCSI_NAME_LEN             256

// Maximum length (including Null terminator) for Authentication iSCSI name entry
#define AUTH_MAX_iSCSI_USER_NAME_LEN        256

// Maximum length of iSCSI secret (NOT null terminated)
// Note: There is an implicit dependency with the size CPD_AUTH_PASSWORD_MAX_SIZE
#define AUTH_MAX_iSCSI_SECRET_LEN           100  

// Number of characters to save for opaque Host data
#define AUTH_MAX_AUTHENTICATION_INFO_LEN      4 

// Indicators of the type of record being managed (Local or Remote)

#define AUTH_REMOTE_SECRET_RECORD       0x0001
#define AUTH_LOCAL_SECRET_RECORD        0x0002
#define AUTH_SECRET_INVALID             0x0004

// Authentication FLAG values 
// Wildcard flags for generic specification of valid authentication requests
#define AUTH_WILDCARD_iSCSI_NAME        0x0010
#define AUTH_WILDCARD_USER_NAME         0x0020

// Indicator of Target/Initiator Authentication record
#define AUTH_INITIATOR_MODE_RECORD		0x0040	
#define AUTH_TARGET_MODE_RECORD			0x0080

// Types of Authentication records supported 
typedef enum AUTH_AUTHENTICATION_TYPE_TAG {
	AUTH_TYPE_NONE       = 0,
	AUTH_TYPE_iSCSI_CHAP = 1
} AUTHENTICATION_TYPE;

#define CURRENT_AUTHENTICATION_KEY_LIST_REV 1

#define CURRENT_AUTHENTICATION_ENTRY_REV    1

typedef struct {
  UINT32  Data1;
  UINT16  Data2;
  UINT16  Data3;
  UINT8   Data4[8];
} UUID_COMPAT;

// Key value identifier of Authentication record for management purposes 
typedef UUID_COMPAT AUTHENTICATION_KEY;
typedef UUID_COMPAT *PAUTHENTICATION_KEY;

/////////////////////////////////////////////////////////////////////////////////////////
// An AUTHENTICATION_KEY_LIST structure is used to contain a list of Authentication keys.
//
// Revision:
//      Unique value identifying authentication revision
//
// Count:
//		Total number of valid authentication records in database
//
// Key
//      List of Key ID's. Note: Only as many Key-ID's in list as will fit in the supplied 
//      buffer. This is NOT necessarily the 'Count' value 
//      (i.e. - if a too small buffer was supplied then not all key ID's would be in the list)
//
/////////////////////////////////////////////////////////////////////////////////////////

#pragma pack(4)
typedef struct _AUTHENTICATION_KEY_LIST {
	ULONG			    Revision;
	ULONG			    Count;
	AUTHENTICATION_KEY	Key[1];
} AUTHENTICATION_KEY_LIST, *PAUTHENTICATION_KEY_LIST;
#pragma pack()

// Authentication Key List size macro to determine buffer size for supplied count.
#define AUTHENTICATION_KEY_LSIZE(KeyCount)						\
	(sizeof(AUTHENTICATION_KEY_LIST) - sizeof(AUTHENTICATION_KEY) +	\
	(KeyCount) * sizeof(AUTHENTICATION_KEY))

///////////////////////////////////////////////////////////////////////////////
// An AUTHENTICATION_iSCSI_CHAP manages iSCSI CHAP specific data for an 
// authentication record.
//
// iSCSI_name_len:
//      Length of the iSCSI name associated with the authentication record
//
// User_name_len:
//      Length of the user name associated with the authentication record
//
// secret_len:
//      Length of the stored secret
//
// flags:
//      Special case indicators for Wildcards, validity of secret fields, secret direction, etc.
//
// iSCSI_name:
//      iSCSI name of iSCSI device logging into array
//
// User_name:
//      username of session logging into array
//
// secret:
//      secret associated with CHAP authentication 
//
// AuthorizationInfo:
//		Uninterpreted storage set and retrieved by the GUI.  It is intended
//		to be used for Authorization record "information." 
//
///////////////////////////////////////////////////////////////////////////////

#pragma pack(4)

/* iSCSI CHAP Authentication record type */
typedef struct _AUTHENTICATION_iSCSI_CHAP
{
	UCHAR      iSCSI_name_len;
	UCHAR      user_name_len;
	UCHAR      secret_length;
	UCHAR      flags;
	UCHAR      iSCSI_Name[AUTH_MAX_iSCSI_NAME_LEN];
	UCHAR      User_Name[AUTH_MAX_iSCSI_USER_NAME_LEN];
	UCHAR      Secret[AUTH_MAX_iSCSI_SECRET_LEN];
	UCHAR      AuthenticationInfo[AUTH_MAX_AUTHENTICATION_INFO_LEN];
	ULONG      Reserved[8];
} AUTHENTICATION_iSCSI_CHAP;
#pragma pack()

///////////////////////////////////////////////////////////////////////////////
// An AUTHENTICATION_PARAM manages the user-settable values associated with
// a single authentication entry
//
// Revision:
//		Format of this structure.
//
// ID_Key:
//		Unique identifier of the authentication record.
//      
// authentication_type:
//		Describes how the remainder of the authentication record is formatted 
//
// auth: (type specific information)
//		chap: Remainder of information is in the iSCSI CHAP format
///////////////////////////////////////////////////////////////////////////////
#pragma pack(4)

typedef struct _AUTHENTICATION_PARAM
{

	ULONG				Revision;
	AUTHENTICATION_KEY	ID_Key;
	AUTHENTICATION_TYPE	authentication_type;
	ULONG				Reserved[8];

	union {  // Data for the type of Authentication record being managed
		AUTHENTICATION_iSCSI_CHAP chap;
	} auth;

} AUTHENTICATION_PARAM, *PAUTHENTICATION_PARAM;

#pragma pack()

#define CURRENT_AUTHENTICATION_PARAM_REV	1

///////////////////////////////////////////////////////////////////////////////
// An AUTHENTICATION_ENTRY manages a single authentication record
//
// Revision:
//     Indication as to format of saved data
//
// Params:
//     User supplied data for a single authentication record.
//
///////////////////////////////////////////////////////////////////////////////
#pragma pack(4)

typedef struct _AUTHENTICATION_ENTRY
{
	ULONG					Revision;
	ULONG					Reserved[4];
	AUTHENTICATION_PARAM	Params;

} AUTHENTICATION_ENTRY, *PAUTHENTICATION_ENTRY;

#pragma pack()

///////////////////////////////////////////////////////////////////////////////
// An AUTHENTICATION_LIST is returned on Enumeration commands
//
// Count:
//     Number of authentication records TOTAL saved in the system
//
// Revision:
//     Revision number associated with saved format of returned ID Keys.
//
// Info: 
//     Sequential list of authentication record identifiers
//      NOTE: this is only the number of identifiers that will FIT in the supplied
//            return buffer, NOT the number indicated in 'Count'.
///////////////////////////////////////////////////////////////////////////////
#pragma pack(4)
typedef struct _AUTHENTICATION_LIST
{
	ULONG                 Count;
	ULONG	              Revision;
	AUTHENTICATION_ENTRY  Info[1];
} AUTHENTICATION_LIST, *PAUTHENTICATION_LIST, ADMIN_AUTH_LIST;
#pragma pack()

// Authentication List size macro.
//  (returns Total authentication database size needed for passed in record count)
#define AUTHENTICATION_LSIZE(ACount)							\
	(sizeof(AUTHENTICATION_LIST) - sizeof(AUTHENTICATION_ENTRY) +	\
	((ACount) * sizeof(AUTHENTICATION_ENTRY)))


// An AUTH_PARAM_LIST is used to persist a list of authentication records.

#pragma pack(4)

typedef struct _AUTH_PARAM_LIST {
	ULONG                 Count;
	ULONG	              Revision;
	AUTHENTICATION_PARAM  Info[1];

} AUTH_PARAM_LIST, *PAUTH_PARAM_LIST;

#pragma pack()

// Authentication Param List size macro.

#define AUTH_PARAM_LSIZE(ACount)	\
	(sizeof(AUTH_PARAM_LIST) - sizeof(AUTHENTICATION_PARAM)+ ((ACount) * sizeof(AUTHENTICATION_PARAM)))

#if __cplusplus
// Define == operators for all of the types defined in this file.

extern "C++" inline bool operator ==(const HOST_PORT_KEY & f, const HOST_PORT_KEY & s) 
{
	return (f.PortType == s.PortType && f.SPId == s.SPId 
		&& f.LogicalPortId == s.LogicalPortId);
}

extern "C++" inline bool operator ==(const INITIATOR_BEHAVIOR & f, const INITIATOR_BEHAVIOR & s) 
{
	return (f.InitiatorType == s.InitiatorType && f.InitiatorOptions == s.InitiatorOptions 
		&& f.StorageGroupWWN == s.StorageGroupWWN);
}

#endif

#endif
