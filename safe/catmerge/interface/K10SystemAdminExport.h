// K10SystemAdminExport.h
//
// Copyright (C) 1998-2011 Data General Corporation
//
//
// Header for SP_specific IK10Admin interface usage
//
//	Revision History
//	----------------
//	20 Nov 98	D. Zeryck	Initial version.
//	30 Nov 98	D. Zeryck	Fleshed out after initial draft of doc.
//	22 Jul 99	D. Zeryck	Added K10_SYSTEM_ADMIN_DB_CONTROL
//	03 Aug 99	D. Zeryck	Add Quiesce, unquiesce to control
//	28 Sep 99	H. Weiner	K10_SYSTEM_ADMIN_OPC_ATTRIBUTES_AVAILABLE added to enum
//	25 Oct 99	D. Zeryck	Added K10_SYSTEM_ADMIN_OPC_LU_SCRUB
//	04 Nov 99	D. Zeryck	Added K10_SYSTEM_ADMIN_ERROR_PSM_NOT_CONFIGURED
//	13 Dec 99	D. Zeryck	Add Front Bus data stuff
//	19 jan 00	D. Zeryck	Add restart options
//	26 Jan 00	D. Zeryck	Add K10_SYSTEM_DB_GLOBALS
//	 4 Feb 00	D. Zeryck	Changed K10_SYSTEM_DB_GLOBALS
//	 8 Feb 00	B. Yang		Added K10_SYSTEM_ADMIN_ERROR_MISSING_DATA, MULTIPLE_WWN and MULTIPLE_LU
//  17 Feb 00	B. Yang		Added K10_SYSTEM_ADMIN_ERROR_LPORTID_NOT_PRESENT and K10_SYSTEM_ADMIN_ERROR_SET_HOST_PORT_FAIL 
//   7 Mar 00	B. Yang		Added severity to error codes.
//	14 Mar 00	D. Zeryck	Added K10_SYSTEM_ADMIN_ERROR_IOCTL
//	 6 Apr 00	H. Weiner	Added K10_SYSTEM_ADMIN_ERROR_TLD_DATA_TYPE
//	12 May 00	B. Yang		Added K10_SYSTEM_ADMIN_ERROR_IOCTL_UNEXPECTED_unsigned charS
//	22 Jun 00	B. Yang		Added K10_SYSTEM_ADMIN_ERROR_EXECUTION.
//	19 Jul 00	Dave Z.		Add ISPEC to shutdown to deal with 1416
//	10 Aug 00	B. Yang		Bug 1616
//	11 Aug 00	D. Zeryck	Bug 946 - implement drive FW download
//	23 Aug 00	D. Zeryck	946 - move stuff out that is problematic for Navi
//	31 Aug 00	B. Yang		Task 1774 - Cleanup the header files
//	25 Sep 00	D. Zeryck	chg 1944 - reboot with prejudice
//	25 Oct 00	D. Zeryck	Bug 10113
//	15 Dec 00	B. Yang		Added K10_SYSTEM_ADMIN_ERROR_LU_EXPANDING
//	 5 Jan 01	H. Weiner	Phase2: Added K10_SYSTEM_ADMIN_OPC_FIRMWARE_BIOS_PROM_INSTALL
//	19 Apr 01	B. Yang		Added K10_SYSTEM_ADMIN_ERROR_DATA_MISMATCH and gangId field in K10_system_admin_LU_assign
//	 2 May 01	H. Weiner	Phase3: expand K10_system_admin_frontbus_data struct to support port speed control
//	 4 Oct 01	C. Hopkins  Added K10_SYSTEM_ADMIN_OPC_CONTROL_PANIC_SP in reboot enum.
//	29 Nov 01	B. Yang		Added K10SystemAdminWriteFrontbus.
//  07 Feb 02   K. Keisling Include windows.h for scsitarg.h
//	 7 Jan 02	H. Weiner	Added K10_SYSTEM_ADMIN_ISPEC_FIRMWARE_BIOSPROM enum to support
//							loading the BIOS, PROM, etc. separately.
//							Also added new FW error codes.
//	 1 Oct 02	C. Hopkins	Added K10_SYSTEM_ADMIN_ERROR_DRIVER_EXCLUDED
//	31 Jan 03	E. Petschin Added NiceName field to K10_system_admin_LU_assign;
//	 7 Oct 03	C. Hopkins	Added Worklist Create option and modified Delete worklist node
//							to handle both create and delete
//	17 Nov 03   L. Glanville Dims 95769 - Added bInternal support to consume node for back out
// 20 Jan 04   M. Salha Added K10_WorkListDataLunCfg struct, updated K10_WorkListDataU
//	15 Mar 04	C. Hopkins	Mods to K10_WorkListDataLunCfg structure
//   8 Dec 03   C. Hughes   Layered Driver LU Expansion
//	24 JUn 04	H. Weiner	DIMS 108371 Add K10_WORK_LIST_TYPE_QUICK_DELETE
//	08 Mar 05	G. Poteat	DIMS 122040 Add K10_SYSTEM_ADMIN_ERROR_LU_NOT_FOUND
//	02 May 05	M. Brundage	DIMS 124879 Added structures for K10_SYSTEM_ADMIN_OPC_ISCSI_PORT_SYNC
//  06 Jun 05	R. Hicks	DIMS 121463 Added opcodes to get and set SP statistics logging
//	27 Sep 05	R. Hicks	DIMS 130874: support delivering EAST
//	03 Oct 05	M. Brundage	DIMS 132479: Added OPC_CONTROL opcodes for reset-and-hold and poweroff.
//	20 Dec 05	K. Boland	DIMS 135031: Added RequestedLinkSpeed to K10SystemAdminFrontBusData
//	27 Dec 05	M. Brundage	DIMS 137265: Added support for drivers in a device stack multiple times.
//	10 Jan 06	R. Hicks	Added support for K10_SYSTEM_ADMIN_OPC_LU_QUIESCE
//	19 Jan 06	R. Hicks	DIMS 138321: Added K10_SYSTEM_ADMIN_ISPEC_LU_QUIESCE itemspec
//	05 May 06	R. Hicks	DIMS 115386: Support specifying new lun number when spoofing a lun
//	11 Jan 07	V. Chotaliya	Member Variable Changed from btUnused to btTracePath in K10_system_admin_init_devmap
//								for AdminTool Verbose Enhancement
//	31 Jan 07	V. Chotaliya	Code changes for IPv6 Support on Management Port
//	23 Apr 07	R. Hicks	DIMS 167779: Added support for quiesce worklist op
//	27 Jul 07	V. Chotaliya	Changes for IPv6 Gateway Support
//	19 Sep 07	D. Hesi		Added  K10_SYSTEM_ADMIN_OPC_NETID_MGMT_PORT_DATA and 
//							K10_SYSTEM_ADMIN_OPC_NETID_MGMT_IP_SETUP
//	13 Feb 08	R.Saravanan	DIMS 167160 Added K10_SYSTEM_ADMIN_ERROR_LU_BROKENUNIT in K10_SYSTEM_ADMIN_ERROR.
//	14 Mar 08	D. Hesi		DIMS 193199: Added  K10_SYSTEM_ADMIN_OPC_NETID_IPV4_DATA to support new K10SystemAdmin
//							interface
//	14 Mar 08	R.Saravanan	DIMS 192289 Modified the error string K10_SYSTEM_ADMIN_ERROR_LU_BROKENUNIT to K10_SYSTEM_ADMIN_ERROR_LU_DISK_FAULT
//	18 Mar 08	V. Chotaliya	DIM 192501 Added K10_SYSTEM_ADMIN_DB_TLU and K10_SYSTEM_ADMIN_OPC_LU_CLEAR_CACHE_DIRTY 
//								to Notify K10MLUAdmin
//	08 Aug 08	R. Hicks	DIMS 204304: Added suport for K10_SYSTEM_ADMIN_OPC_AT_MAX_PUBLIC_LUS
//	02 Sep 08	R. Hicks	Added non-TLD interfaces for Network Admin library / VLAN support
//	24 Oct 08	D. Hesi		DIMS 197128: Added Persist/Clear transaction support for quiesce/unquiesce operations
//	04 Dec 08	V. Chotaliya	Added K10_SYSTEM_ADMIN_OPC_WORKLIST_PROCESS_WITH_RETURN_DATA to return TLD data for CREATE operation
//	15 Dec 08	R. Saravanan	Disk Spin Down feature support new error code K10_SYSTEM_ADMIN_ERROR_RG_POWERSAVINGS_ON
//	18 Dec 08	V. Chotaliya	Modified K10_WorkListData to add lBufSize
//	03 Apr 09	D. Hesi		DIMS 197085: Added interface for SNMP persistence
//	27 Oct 09	R. Hicks	DIMS 239841: Added lun delete notification to driver unbind
//	04 Nov 09	R. Hicks	DIMS 241841: Added device map fields to K10_System_Admin_LU_ExtendedList
//	01 Feb 10	R. Hicks	Added LunList, luId fields to K10_WorkListData
//	17 May 10	E. Carter	ARS 366043 - Add bPreventAddToPhysical to K10_system_admin_LU_assign
//	14 Apr 11	D. Hesi		ARS 407127 - J2B Phase 2 : Added drive Bus_Encl_Slot info in Flare IOCTL buffer
//      19 Jul 13       V. Kan          ARS 487894 - Change the iSCSI port config sync API so that ports that are present, but not ready, still get included in the returned status list as PORT_ERROR instances with an new extended status field.
//
#ifndef K10_SYSTEM_ADMIN_EXPORT
#define K10_SYSTEM_ADMIN_EXPORT

// This library implements the interfaces described in the "K10 Storage 
// Processor Specific Interfaces Specification", which you should refer
// to for all semantics information.
// 

//----------------------------------------------------------------------INCLUDES
#ifndef K10_DEFS_H
#include "k10defs.h"
#include "csx_ext.h"
#endif

#ifndef GM_INSTALL_EXPORT_H
#include "gm_install_export.h"
#endif

#ifndef MMAN_DISPATCH_H
#include "MManDispatch.h"	
#endif

#include <windows.h>

#include "K10DiskDriverAdminExport.h"
#include "scsitarg.h"

//-----------------------------------------------------------------------DEFINES

#define K10_SYSTEM_ADMIN_LIBRARY_NAME				"K10SystemAdmin"

#define K10_SYSTEM_ADMIN_ERROR_BASE					0x79500000

#define K10_SYSTEM_ADMIN_DB_SECURITY_LEVEL_REMOVE	0x00
	// Used to remove an account.
#define K10_SYSTEM_ADMIN_DB_SECURITY_LEVEL_USER		0x01
	// Defines typical user-level account; no access to services, 
	// user creation, etc.
#define K10_SYSTEM_ADMIN_DB_SECURITY_LEVEL_ADMIN	0x02
	// Defines an administer on the system: can start/stop services, create 
	// users, etc. Some access may be denied to this security level. Cannot create users with CSE level security, however (see next).
#define K10_SYSTEM_ADMIN_DB_SECURITY_LEVEL_CSE		0x03
	// As Admin level security, but allowed access to certain directores and 
	// registry entries denied to Admin. Can create a CSE-level account.



#define K10_SYSTEM_ADMIN_DB_HBA_PORT_BE		0x00
	// This port used as a back-end Flare virtual bus.
#define K10_SYSTEM_ADMIN_DB_HBA_PORT_FS			0x01
	// This port used as a frontside (Host attach) bus.
#define K10_SYSTEM_ADMIN_DB_HBA_PORT_INT		0x02
	// This port used as a private bus, non-Flare, non-frontside.

#define K10_SYSTEM_ADMIN_LU_EXTENDED_VERSION	2

//Min size in megs for PSM LU
#define K10_PSM_MIN_SIZE		512

#define  SYSADMIN_ERR_STRING_LEN		256

//-----------------------------------------------------------------ENUMS


////////////////////////////////////////////////////////////////////////
//
// Error codes
//
//
enum K10_SYSTEM_ADMIN_ERROR {
	K10_SYSTEM_ADMIN_ERROR_SUCCESS = 0x00000000L,	//Successful execution.

	K10_SYSTEM_ADMIN_INFO_MIN = K10_SYSTEM_ADMIN_ERROR_BASE | K10_COND_INFO, 
	K10_SYSTEM_ADMIN_INFO_REBOOT = K10_SYSTEM_ADMIN_INFO_MIN, // we're rebooting
	K10_SYSTEM_ADMIN_INFO_WL_COMPLETE,
	K10_SYSTEM_ADMIN_INFO_ISCSI_PORT_SYNCED,

	K10_SYSTEM_ADMIN_WARN_MIN = K10_SYSTEM_ADMIN_ERROR_BASE | K10_COND_WARNING, 
	K10_SYSTEM_ADMIN_WARN_REBOOT = K10_SYSTEM_ADMIN_WARN_MIN, // default reboot did not work, using backup
	K10_SYSTEM_ADMIN_WARN_WL_COMPLETE,

	K10_SYSTEM_ADMIN_ERROR_ERROR_MIN = K10_SYSTEM_ADMIN_ERROR_BASE | K10_COND_ERROR, 
													// The min value of 'error' class
	K10_SYSTEM_ADMIN_ERROR_BAD_DBID = K10_SYSTEM_ADMIN_ERROR_ERROR_MIN, 
													// Unrecognised database ID
	K10_SYSTEM_ADMIN_ERROR_BAD_OPC,					// Unrecognised opcode.
	K10_SYSTEM_ADMIN_ERROR_BAD_ISPEC,				// Unrecognised item spec.
	K10_SYSTEM_ADMIN_ERROR_IOCTL_TIMEOUT,			// Timeout when waiting for DeviceIoControl
	K10_SYSTEM_ADMIN_ERROR_BAD_DATA,				// wrong size for struct expected or the input data is not valid
	K10_SYSTEM_ADMIN_ERROR_UNRECOGNISED_ACCOUNT,	
		//The account name supplied is not found on the machine/network.
	K10_SYSTEM_ADMIN_ERROR_INVALID_PARAMETER,		// A precondition was not met.
	K10_SYSTEM_ADMIN_ERROR_REFERENCE_REDEFINITION,	// A reference is already in use
	K10_SYSTEM_ADMIN_ERROR_BUS_NOT_PRESENT,	
		// Attempted to configure nonexistant bus
	K10_SYSTEM_ADMIN_ERROR_SLOT_NOT_PRESENT,
		// Attempted to configure nonexistant slot
	K10_SYSTEM_ADMIN_ERROR_PASSWORD,				// Incorrect password used
	K10_SYSTEM_ADMIN_ERROR_AUTHORIZATION,	
		//The security level of the request was not sufficient to authorize.
	K10_SYSTEM_ADMIN_ERROR_LU_ACTIVE,				// Cannot unbind the LU
	K10_SYSTEM_ADMIN_ERROR_LU_DB_INCONSISTANT,		// calls to layered driver DB don't match
	K10_SYSTEM_ADMIN_ERROR_LU_CANT_FILTER_CREATOR,	// trying to add/remove creator of LU
	K10_SYSTEM_ADMIN_ERROR_DEVMAP_INCONSISTANT,		// Device map is inconsistant
	K10_SYSTEM_ADMIN_ERROR_MISSING_DATA,			// Missing some data (TAG, data, field etc.) which should be there																	
	K10_SYSTEM_ADMIN_ERROR_BAD_TAG,					// Unrecognised TLD tag.
	K10_SYSTEM_ADMIN_ERROR_MULTIPLE_WWN,			// Multiple WWNs in an ATTRIBUTES_SET_BLOCK
	K10_SYSTEM_ADMIN_ERROR_MULTIPLE_LU,				// LU in varray more than once
	K10_SYSTEM_ADMIN_ERROR_LPORTID_NOT_PRESENT,		// Attempt to set a nonexistant port
	K10_SYSTEM_ADMIN_ERROR_SET_HOST_PORT_FAIL,		// failed in setting host port ( i.e. set preferred loop id )
	K10_SYSTEM_ADMIN_ERROR_MEMORY_WHACKED,			// pointing past the end--did not find the driver, memory whacked or
													// programmer error.
	K10_SYSTEM_ADMIN_ERROR_IOCTL,					// IOCTL failed, hr in data section
	K10_SYSTEM_ADMIN_ERROR_GET_MACHINENAME_FAILURE, // Getting MachineName from Registry failed.
	K10_SYSTEM_ADMIN_ERROR_GET_IPADDRESS_FAILURE,	// Getting IPAddr from Registry failed.
	K10_SYSTEM_ADMIN_ERROR_GET_SUBNETMASK_FAILURE,	// Getting SubnetMask from Registry failed.
	K10_SYSTEM_ADMIN_ERROR_GET_GATEWAY_FAILURE,		// Getting DefaultGateway from Registry failed.
	K10_SYSTEM_ADMIN_ERROR_GET_DHCP_FAILURE,		// Getting DHCP flag from Registry failed.
	K10_SYSTEM_ADMIN_ERROR_GET_DOMAINNAME_FAILURE,	// Getting DomainName from Registry failed.
	K10_SYSTEM_ADMIN_ERROR_SET_MACHINENAME_FAILURE, // Setting MachineName to Registry failed. 
	K10_SYSTEM_ADMIN_ERROR_SET_IPADDRESS_FAILURE,	// Setting IpAddr to Registry failed.
	K10_SYSTEM_ADMIN_ERROR_SET_SUBNETMASK_FAILURE,	// Setting SubnetMask to Registry failed.
	K10_SYSTEM_ADMIN_ERROR_SET_GATEWAY_FAILURE,		// Setting DefaultGateway to Registry failed.
	K10_SYSTEM_ADMIN_ERROR_SET_DHCP_FAILURE,		// Setting DHCP to Registry failed.
	K10_SYSTEM_ADMIN_ERROR_UNKNOWN_NETID,			// Unknown NetId
	K10_SYSTEM_ADMIN_ERROR_TLD_DATA_TYPE,			// TLD data is wrong type, usually
													// means expected numeric data has
													// size > sizeof(long)
	K10_SYSTEM_ADMIN_ERROR_IOCTL_UNEXPECTED_BYTES,	// Unexpected unsigned chars returned from an IOCTL call.
	K10_SYSTEM_ADMIN_ERROR_EXECUTION_ERROR,			// see accompanying data
	K10_SYSTEM_ADMIN_ERROR_PROG_ERROR,				// programming error - should not see this in field!
	K10_SYSTEM_ADMIN_ERROR_LU_SCRUBBED,				// An XLU was scrubbed from the database due to inconsistant data
	K10_SYSTEM_ADMIN_ERROR_LU_EXPANDING,			// Add layer fails if the lun is expanding
	K10_SYSTEM_ADMIN_ERROR_DATA_MISMATCH, 
	K10_SYSTEM_ADMIN_ERROR_BAD_FW_TYPE,				// Unknown Firmware was specified in a command
	K10_SYSTEM_ADMIN_ERROR_BIOS,					// Error accessing BIOS file
	K10_SYSTEM_ADMIN_ERROR_POST,					// Error accessing POST file
	K10_SYSTEM_ADMIN_ERROR_DDBS,					// Error accessing DDBS file
	K10_SYSTEM_ADMIN_ERROR_LU_NOT_READY,			// Lun is binding/rebuilding.
	K10_SYSTEM_ADMIN_ERROR_DRIVER_EXCLUDED,			// Filter driver excluded from LUN stack
	K10_SYSTEM_ADMIN_ERROR_BAD_WORKLIST,			// Invalid work list node type found
	K10_SYSTEM_ADMIN_ERROR_WL_FAILED,				// Processing of a Layered Feature worklist failed
	K10_SYSTEM_ADMIN_ERROR_WL_BACKOUT_FAILED,		// Backout of a failing worklist also failed
	K10_SYSTEM_ADMIN_ERROR_ILLEGAL_CHANGE,
	K10_SYSTEM_ADMIN_ERROR_WL_DELETE_FAILED,
    K10_SYSTEM_ADMIN_ERROR_OP_NOT_SUPPORTED_DURING_INSERT,
        // Inserting driver doesn't support the operation already in progress
        // (e.g. expansion) on the LU.
    K10_SYSTEM_ADMIN_ERROR_OP_NOT_SUPPORTED_BY_FEATURE,
        // The requested operation (e.g. expansion) is not supported by one
        // of the layered drivers currently in the device stack.
    K10_SYSTEM_ADMIN_ERROR_CANT_MAKE_PRIVATE_OP_IN_PROGRESS,
        // The LU cannot be made private because there is an operation
        // already in progress (e.g. expansion) on the LU.
	K10_SYSTEM_ADMIN_ERROR_LU_NOT_FOUND,
	K10_SYSTEM_ADMIN_ERROR_GPS_FW,
	K10_SYSTEM_ADMIN_ERROR_LU_DISK_FAULT,			// Feature Object deleted, but component LUN(s) could not be unbound
	K10_SYSTEM_ADMIN_ERROR_GET_IPV6_MODE,			//Getting IPv6 Mode is failed
	K10_SYSTEM_ADMIN_ERROR_RG_POWERSAVINGS_ON,		// Tries to insert layered driver where RG is enabled
	K10_SYSTEM_ADMIN_ERROR_GET_VLAN_FAILURE,		// Getting VLAN config failed.
	K10_SYSTEM_ADMIN_ERROR_SET_VLAN_FAILURE,		// Setting VLAN config failed.
	K10_SYSTEM_ADMIN_ERROR_GET_MACADDRESS_FAILURE,	// Getting MAC Address failed.
	K10_SYSTEM_ADMIN_ERROR_ERROR_MAX =
        K10_SYSTEM_ADMIN_ERROR_GET_MACADDRESS_FAILURE
		// The max value of 'error' class
};


////////////////////////////////////////////////////////////////////////////////
//
// Database Specifiers. Use in lDbId parameter of Admin interface
//
//
enum K10_SYSTEM_ADMIN_DB {
	K10_SYSTEM_ADMIN_DB_MIN = 0,
	K10_SYSTEM_ADMIN_DB_SECURITY = K10_SYSTEM_ADMIN_DB_MIN,
	K10_SYSTEM_ADMIN_DB_HBA,
	K10_SYSTEM_ADMIN_DB_VBUS,
	K10_SYSTEM_ADMIN_DB_NETWORK,
	K10_SYSTEM_ADMIN_DB_HW,
	K10_SYSTEM_ADMIN_DB_DISK,
	K10_SYSTEM_ADMIN_DB_LU_EXTENDED,
	K10_SYSTEM_ADMIN_DB_CONTROL,
	K10_SYSTEM_ADMIN_DB_FRONTBUS,
	K10_SYSTEM_ADMIN_DB_CMI,
	K10_SYSTEM_DB_GLOBALS,
	K10_SYSTEM_ADMIN_DB_FIRMWARE,
	K10_SYSTEM_ADMIN_DB_WORKLIST,
	K10_SYSTEM_ADMIN_DB_DISKDRIVEROPC, // Use K10_DISK_ADMIN_OPC lOpcode values
	K10_SYSTEM_ADMIN_DB_TLU,
	K10_SYSTEM_ADMIN_DB_MAX = K10_SYSTEM_ADMIN_DB_TLU
};

////////////////////////////////////////////////////////////////////////////////
//

// Opcode Specifiers. Use in lOpcode parameter of Admin interface
//
// NONE. The opcode in the passthrough is used to hold page code

enum K10_SYSTEM_ADMIN_OPC_SECURITY {			// K10_SYSTEM_ADMIN_DB_SECURITY
	K10_SYSTEM_ADMIN_OPC_SECURITY_MIN = 0,
	K10_SYSTEM_ADMIN_OPC_SECURITY_TOKEN = K10_SYSTEM_ADMIN_OPC_SECURITY_MIN,	// acquire a security token
	K10_SYSTEM_ADMIN_OPC_SECURITY_ACCOUNT,		// machine accounts
	K10_SYSTEM_ADMIN_OPC_SECURITY_MAX = K10_SYSTEM_ADMIN_OPC_SECURITY_ACCOUNT
};


enum K10_SYSTEM_ADMIN_OPC_HBA_DATA {			// K10_SYSTEM_ADMIN_DB_HBA
	K10_SYSTEM_ADMIN_OPC_HBA_DATA_MIN = 0,
	K10_SYSTEM_ADMIN_OPC_HBA_DATA = K10_SYSTEM_ADMIN_OPC_HBA_DATA_MIN,			// Get HBA vendor data
	K10_SYSTEM_ADMIN_OPC_HBA_SETUP,				// Assign logical bus number
	K10_SYSTEM_ADMIN_OPC_HBA_DATA_MAX = K10_SYSTEM_ADMIN_OPC_HBA_SETUP
};


enum K10_SYSTEM_ADMIN_OPC_VBUS {				// K10_SYSTEM_ADMIN_DB_VBUS
	K10_SYSTEM_ADMIN_OPC_VBUS_MIN = 0,
	K10_SYSTEM_ADMIN_OPC_VBUS_DATA = K10_SYSTEM_ADMIN_OPC_VBUS_MIN,					// Get VBus setup
	K10_SYSTEM_ADMIN_OPC_VBUS_SETUP,
	K10_SYSTEM_ADMIN_OPC_VBUS_MAX = K10_SYSTEM_ADMIN_OPC_VBUS_SETUP
};

enum K10_SYSTEM_ADMIN_OPC_NETWORK {				// K10_SYSTEM_ADMIN_DB_NETWORK
	K10_SYSTEM_ADMIN_OPC_NETWORK_MIN = 0,
	K10_SYSTEM_ADMIN_OPC_NETID_DATA = K10_SYSTEM_ADMIN_OPC_NETWORK_MIN,
	K10_SYSTEM_ADMIN_OPC_NETID_IP_SETUP,			// set network ID
	K10_SYSTEM_ADMIN_OPC_DOMAIN_SETUP,
	K10_SYSTEM_ADMIN_OPC_ISCSI_PORT_SYNC,
	K10_SYSTEM_ADMIN_OPC_NETID_IPV6_GLOBAL_DATA,  
	K10_SYSTEM_ADMIN_OPC_NETID_IPV6_LINKLOCAL_DATA,
	K10_SYSTEM_ADMIN_OPC_NETID_MGMT_PORT_DATA,
	K10_SYSTEM_ADMIN_OPC_NETID_MGMT_IP_SETUP,
	K10_SYSTEM_ADMIN_OPC_NETID_IPV4_DATA,
	K10_SYSTEM_ADMIN_OPC_NETID_IP_SETUP_NON_TLD,
	K10_SYSTEM_ADMIN_OPC_NETID_MGMT_IP_SETUP_NON_TLD,
	K10_SYSTEM_ADMIN_OPC_NETID_MGMT_PORT_PHYSICAL_DATA,
	K10_SYSTEM_ADMIN_OPC_NETID_SNMP_PERSIST,
	K10_SYSTEM_ADMIN_OPC_NETWORK_MAX = K10_SYSTEM_ADMIN_OPC_NETID_SNMP_PERSIST

};


enum K10_SYSTEM_ADMIN_OPC_HW {					// K10_SYSTEM_ADMIN_DB_CPU
	K10_SYSTEM_ADMIN_OPC_HW_MIN = 0,
	K10_SYSTEM_ADMIN_OPC_HW_DATA = K10_SYSTEM_ADMIN_OPC_HW_MIN,			// CPU data.
	K10_SYSTEM_ADMIN_OPC_HW_RAM,					// RAM data.
	K10_SYSTEM_ADMIN_OPC_HW_MAX = K10_SYSTEM_ADMIN_OPC_HW_RAM
};


enum K10_SYSTEM_ADMIN_OPC_DISK {				// K10_SYSTEM_ADMIN_DB_DISK
	K10_SYSTEM_ADMIN_OPC_DISK_MIN = 0,
	K10_SYSTEM_ADMIN_OPC_DISK_SPACE = K10_SYSTEM_ADMIN_OPC_DISK_MIN,		// not used yet
	K10_SYSTEM_ADMIN_OPC_DISK_MAX = K10_SYSTEM_ADMIN_OPC_DISK_SPACE
};


enum K10_SYSTEM_ADMIN_OPC_LU {			// K10_SYSTEM_ADMIN_DB_LU_EXTENDED
	K10_SYSTEM_ADMIN_OPC_LU_MIN = 0,
	K10_SYSTEM_ADMIN_OPC_LU_EXTENDED = K10_SYSTEM_ADMIN_OPC_LU_MIN,		// Get or set extended attribs
	K10_SYSTEM_ADMIN_OPC_LU_INIT_DEVMAP,		// Reinitialize device map
	K10_SYSTEM_ADMIN_OPC_LU_ASSIGN,				// (re)assign LU to consumer (TCD only, will add record)
	K10_SYSTEM_ADMIN_OPC_LU_FIX_LU_STATE,		// analyzes state of devmap, applies desired 
												// extended attribute
	K10_SYSTEM_ADMIN_OPC_LU_ATTRIBUTES_AVAILABLE,	// Get a list of available attributes
	K10_SYSTEM_ADMIN_OPC_LU_SCRUB,				// Scrubs HostAdmin database
	K10_SYSTEM_ADMIN_OPC_LU_QUIESCE,			// quiesces/unquiesces LU
	K10_SYSTEM_ADMIN_OPC_LU_CLEAR_CACHE_DIRTY,	// Inform ClearCacheDirtyLun operation
	K10_SYSTEM_ADMIN_OPC_AT_MAX_PUBLIC_LUS,		// determines if the max number of public LUs have been created
	K10_SYSTEM_ADMIN_OPC_LU_MAX = K10_SYSTEM_ADMIN_OPC_AT_MAX_PUBLIC_LUS
};

enum K10_SYSTEM_ADMIN_OPC_CONTROL {		// K10_SYSTEM_ADMIN_DB_CONTROL
	K10_SYSTEM_ADMIN_OPC_CONTROL_MIN = 0,
	K10_SYSTEM_ADMIN_OPC_CONTROL_SHUTDOWN_ALL = K10_SYSTEM_ADMIN_OPC_CONTROL_MIN,	// Shut down disk processing, all drivers
	K10_SYSTEM_ADMIN_OPC_CONTROL_RESTART,		// Shut down and restart disk processing
	K10_SYSTEM_ADMIN_OPC_CONTROL_QUIESCE,		// Quiesce all drivers
	K10_SYSTEM_ADMIN_OPC_CONTROL_UNQUIESCE,		// Unquiesce all drivers
	K10_SYSTEM_ADMIN_OPC_CONTROL_PANIC_SP,
    K10_SYSTEM_ADMIN_OPC_CONTROL_SHUTDOWN_POWER,
    K10_SYSTEM_ADMIN_OPC_CONTROL_GET_STATS_LOGGING,	// get state of SP statistics logging
    K10_SYSTEM_ADMIN_OPC_CONTROL_SET_STATS_LOGGING,	// modify SP statistics logging
	K10_SYSTEM_ADMIN_OPC_CONTROL_POWEROFF,			// Power down SP
	K10_SYSTEM_ADMIN_OPC_CONTROL_RESETANDHOLD,		// Reboot SP and stop before OS loads
	K10_SYSTEM_ADMIN_OPC_CONTROL_MAX = K10_SYSTEM_ADMIN_OPC_CONTROL_RESETANDHOLD
};

enum K10_SYSTEM_ADMIN_OPC_FRONTBUS {	// K10_SYSTEM_ADMIN_DB_FRONTBUS
	K10_SYSTEM_ADMIN_OPC_FRONTBUS_MIN = 0,
	K10_SYSTEM_ADMIN_OPC_FRONTBUS_DATA = K10_SYSTEM_ADMIN_OPC_FRONTBUS_MIN,			// get all data, all ports
	K10_SYSTEM_ADMIN_OPC_FRONTBUS_SETUP,			// Set preferred loop ID(s)
	K10_SYSTEM_ADMIN_OPC_FRONTBUS_SETSPEED,			// Set port Fibre channel speed
	K10_SYSTEM_ADMIN_OPC_FRONTBUS_MAX = K10_SYSTEM_ADMIN_OPC_FRONTBUS_SETSPEED
};

enum K10_SYSTEM_ADMIN_OPC_CMI{				// K10_SYSTEM_ADMIN_DB_CMI
	K10_SYSTEM_ADMIN_OPC_CMI_MIN = 0,
	K10_SYSTEM_ADMIN_OPC_CMI_ENUMERATE_ARRAYS = K10_SYSTEM_ADMIN_OPC_CMI_MIN,	//  a list of arrays 
	K10_SYSTEM_ADMIN_OPC_CMI_MAX = K10_SYSTEM_ADMIN_OPC_CMI_ENUMERATE_ARRAYS
};

enum K10_SYSTEM_ADMIN_OPC_GLOBALS {			//K10_SYSTEM_DB_GLOBALS
	K10_SYSTEM_ADMIN_OPC_GLOBALS_MIN = 0,
	K10_SYSTEM_ADMIN_OPC_GLOBALS_ALL = K10_SYSTEM_ADMIN_OPC_GLOBALS_MIN,
	K10_SYSTEM_ADMIN_OPC_GLOBALS_DEGRADED,
	K10_SYSTEM_ADMIN_OPC_GLOBALS_MAX = K10_SYSTEM_ADMIN_OPC_GLOBALS_DEGRADED
};

// OBSOLETE -- This is here ONLY for backward compatibility for Navisphere
// Delete as soon as possible!!!!
enum K10_SYSTEM_ADMIN_OPC_DISK_FIRMWARE {//K10_SYSTEM_ADMIN_DB_FIRMWARE
	K10_SYSTEM_ADMIN_OPC_DISK_FIRMWARE_MIN = 0,
	K10_SYSTEM_ADMIN_OPC_DISK_FIRMWARE_INSTALL = K10_SYSTEM_ADMIN_OPC_DISK_FIRMWARE_MIN,
	K10_SYSTEM_ADMIN_OPC_DISK_FIRMWARE_MAX = K10_SYSTEM_ADMIN_OPC_DISK_FIRMWARE_INSTALL
};
// End OBSOLETE


enum K10_SYSTEM_ADMIN_OPC_FIRMWARE {//K10_SYSTEM_ADMIN_DB_FIRMWARE
	K10_SYSTEM_ADMIN_OPC_FIRMWARE_MIN = 0,
	K10_SYSTEM_ADMIN_OPC_FIRMWARE_DISK_FIRMWARE_INSTALL = K10_SYSTEM_ADMIN_OPC_FIRMWARE_MIN,
	K10_SYSTEM_ADMIN_OPC_FIRMWARE_BIOS_PROM_INSTALL,
	K10_SYSTEM_ADMIN_OPC_FIRMWARE_MAX = K10_SYSTEM_ADMIN_OPC_FIRMWARE_BIOS_PROM_INSTALL
};


// For Write() requests, lItemSpec will be a TLD *.
// For Update() requests, lItemSpec will be a K10_WorklistRequest *.
enum K10_SYSTEM_ADMIN_OPC_WORKLIST {//K10_SYSTEM_ADMIN_DB_WORKLIST
	K10_SYSTEM_ADMIN_OPC_WORKLIST_MIN = 0,
	K10_SYSTEM_ADMIN_OPC_WORKLIST_PROCESS = K10_SYSTEM_ADMIN_OPC_WORKLIST_MIN,
	K10_SYSTEM_ADMIN_OPC_WORKLIST_BACK_OUT,
	K10_SYSTEM_ADMIN_OPC_WORKLIST_PROCESS_WITH_RETURN_DATA,
	K10_SYSTEM_ADMIN_OPC_WORKLIST_MAX = K10_SYSTEM_ADMIN_OPC_WORKLIST_PROCESS_WITH_RETURN_DATA
};

////////////////////////////////////////////////////////////////////////////////
//
// Item Specifiers

enum K10_SYSTEM_ADMIN_ISPEC_LU_FILTER {	//K10_SYSTEM_ADMIN_DB_LU_EXTENDED
	K10_SYSTEM_ADMIN_ISPEC_LU_FILTER_MIN = 0,
	K10_SYSTEM_ADMIN_ISPEC_LU_FILTER_NONE = K10_SYSTEM_ADMIN_ISPEC_LU_FILTER_MIN,	// all LUs
	K10_SYSTEM_ADMIN_ISPEC_LU_FILTER_CONSUMED,	// only consumed LUs
	K10_SYSTEM_ADMIN_ISPEC_LU_FILTER_UNCONSUMED,	// not consumed LUs
	K10_SYSTEM_ADMIN_ISPEC_LU_FILTER_MAX = K10_SYSTEM_ADMIN_ISPEC_LU_FILTER_UNCONSUMED
};

enum K10_SYSTEM_ADMIN_ISPEC_LU_ASSIGN { // K10_SYSTEM_ADMIN_OPC_LU_ASSIGN
	K10_SYSTEM_ADMIN_ISPEC_LU_ASSIGN_MIN = 0,
	K10_SYSTEM_ADMIN_ISPEC_LU_ASSIGN_NOTRANSACT = K10_SYSTEM_ADMIN_ISPEC_LU_ASSIGN_MIN,
	K10_SYSTEM_ADMIN_ISPEC_LU_ASSIGN_TRANSACT,
	K10_SYSTEM_ADMIN_ISPEC_LU_ASSIGN_MAX = K10_SYSTEM_ADMIN_ISPEC_LU_ASSIGN_TRANSACT
};

enum K10_SYSTEM_ADMIN_ISPEC_LU_QUIESCE { // K10_SYSTEM_ADMIN_OPC_LU_QUIESCE
	K10_SYSTEM_ADMIN_ISPEC_LU_QUIESCE_MIN = 0,
	K10_SYSTEM_ADMIN_ISPEC_LU_QUIESCE_QUEUED = K10_SYSTEM_ADMIN_ISPEC_LU_QUIESCE_MIN,
	K10_SYSTEM_ADMIN_ISPEC_LU_QUIESCE_BUSY,
	K10_SYSTEM_ADMIN_ISPEC_LU_QUIESCE_MAX = K10_SYSTEM_ADMIN_ISPEC_LU_QUIESCE_BUSY
};

enum K10_SYSTEM_ADMIN_ISPEC_RESTART { // K10_SYSTEM_ADMIN_OPC_CONTROL_RESTART
	K10_SYSTEM_ADMIN_ISPEC_RESTART_MIN = 0,
	K10_SYSTEM_ADMIN_ISPEC_RESTART_NONE = K10_SYSTEM_ADMIN_ISPEC_RESTART_MIN,
	K10_SYSTEM_ADMIN_ISPEC_RESTART_THIS_SP,
	K10_SYSTEM_ADMIN_ISPEC_RESTART_BOTH_SP,
	K10_SYSTEM_ADMIN_ISPEC_RESTART_PEER,
	K10_SYSTEM_ADMIN_ISPEC_RESTART_THIS_SP_NOFAIL,	// absolutely do not fail to restart
	K10_SYSTEM_ADMIN_ISPEC_KILL_SP,			// Kill the SP immediately with HAL trap (must add noshutdown)
	K10_SYSTEM_ADMIN_ISPEC_RESTART_MAX = K10_SYSTEM_ADMIN_ISPEC_KILL_SP
};

// OR this flag with "this" or "both" ISPECS above to skip shutdown step on local SP
#define K10_SYSTEM_ADMIN_ISPEC_RESTART_NOSHUTDOWN_FLAG	0x10000000
// OR this with K10_SYSTEM_ADMIN_ISPEC_TARGET_SP or K10_SYSTEM_ADMIN_ISPEC_RESTART
#define K10_SYSTEM_ADMIN_ISPEC_TARGET_SP_NOFAIL_FLAG	0x20000000

// For OPC_CONTROL POWEROFF and RESETANDHOLD commands
enum K10_SYSTEM_ADMIN_ISPEC_TARGET_SP {
    K10_SYSTEM_ADMIN_ISPEC_TARGET_SP_MIN = 0,
    K10_SYSTEM_ADMIN_ISPEC_TARGET_SP_NONE = K10_SYSTEM_ADMIN_ISPEC_TARGET_SP_MIN,
    K10_SYSTEM_ADMIN_ISPEC_TARGET_SP_THIS,
    K10_SYSTEM_ADMIN_ISPEC_TARGET_SP_BOTH,
    K10_SYSTEM_ADMIN_ISPEC_TARGET_SP_PEER,
    K10_SYSTEM_ADMIN_ISPEC_TARGET_SP_MAX = K10_SYSTEM_ADMIN_ISPEC_TARGET_SP_PEER
};

//Define the flavors of FW that can be downloaded  
enum K10_SYSTEM_ADMIN_ISPEC_FIRMWARE_BIOSPROM
{
	K10_SYSTEM_ADMIN_ISPEC_FIRMWARE_BIOSPROM_MIN = 0,
	K10_SYSTEM_ADMIN_ISPEC_FIRMWARE_BIOSPROM_BIOS = K10_SYSTEM_ADMIN_ISPEC_FIRMWARE_BIOSPROM_MIN,
	K10_SYSTEM_ADMIN_ISPEC_FIRMWARE_BIOSPROM_POST,	// Also known as PROM
	K10_SYSTEM_ADMIN_ISPEC_FIRMWARE_BIOSPROM_DDBS,
	K10_SYSTEM_ADMIN_ISPEC_FIRMWARE_BIOSPROM_GPS_FW,
	K10_SYSTEM_ADMIN_ISPEC_FIRMWARE_BIOSPROM_MAX = K10_SYSTEM_ADMIN_ISPEC_FIRMWARE_BIOSPROM_GPS_FW
};

enum K10_SYSTEM_ADMIN_ISPEC_SET_STATS_LOGGING
{
	K10_SYSTEM_ADMIN_ISPEC_SET_STATS_LOGGING_MIN = 0,
	K10_SYSTEM_ADMIN_ISPEC_SET_STATS_LOGGING_OFF = K10_SYSTEM_ADMIN_ISPEC_SET_STATS_LOGGING_MIN,
	K10_SYSTEM_ADMIN_ISPEC_SET_STATS_LOGGING_ON,
	K10_SYSTEM_ADMIN_ISPEC_SET_STATS_LOGGING_MAX = K10_SYSTEM_ADMIN_ISPEC_SET_STATS_LOGGING_ON
};

////////////////////////////////////////////////////////////////////////////////
//
// Worklist types

enum K10_WORK_LIST_TYPE {
	K10_WORK_LIST_TYPE_NONE = 0,
	K10_WORK_LIST_TYPE_MIN = 0x1, 
	K10_WORK_LIST_TYPE_LD = K10_WORK_LIST_TYPE_MIN,
	K10_WORK_LIST_TYPE_ASSIGN,
	K10_WORK_LIST_TYPE_CONSUME,
	K10_WORK_LIST_TYPE_SPOOF,
	K10_WORK_LIST_TYPE_RENUMBER,
	K10_WORK_LIST_TYPE_DELETE,
	K10_WORK_LIST_TYPE_COMPLETE,
	K10_WORK_LIST_TYPE_CHECK,
	K10_WORK_LIST_TYPE_CREATE,
	K10_WORK_LIST_TYPE_LUN_CFG,
	K10_WORK_LIST_TYPE_QUICK_DELETE,
	K10_WORK_LIST_TYPE_COMPLETE_NQ,
	K10_WORK_LIST_TYPE_QUIESCE,
	K10_WORK_LIST_TYPE_MAX = K10_WORK_LIST_TYPE_QUIESCE
};

enum K10_WORK_LIST_PROCESSING_STATE {
	K10_WL_STATE_IDLE = 0,
	K10_WL_STATE_PROCESSED,
	K10_WL_STATE_IN_PROGRESS_0,
	K10_WL_STATE_IN_PROGRESS_1,
	K10_WL_STATE_IN_PROGRESS_2,
	K10_WL_STATE_IN_PROGRESS_3,
	K10_WL_STATE_IN_PROGRESS_4,
	K10_WL_STATE_IN_PROGRESS_5,
   K10_WL_STATE_IN_PROGRESS_6,

};

enum K10_WORK_LIST_CHECK_TYPE {
	K10_WL_CHECK_NOCHECK = 0,
	K10_WL_CHECK_NOLAYER,
	K10_WL_CHECK_SIZE_CHANGE,
	K10_WL_CHECK_CONSUME,
	K10_WL_CHECK_MAX = K10_WL_CHECK_CONSUME
};

////////////////////////////////////////////////////////////////////////////////
//
// Misc. enums

enum K10_SYSTEM_ADMIN_OPC_CPU_TYPE {	// used in K10_SYSTEM_ADMIN_OPC_CPU_DATA
	K10_SYSTEM_ADMIN_OPC_CPU_TYPE_MIN = 0,
	K10_SYSTEM_ADMIN_OPC_CPU_TYPE_DESCHUTES = K10_SYSTEM_ADMIN_OPC_CPU_TYPE_MIN,
	K10_SYSTEM_ADMIN_OPC_CPU_TYPE_XEON,
	K10_SYSTEM_ADMIN_OPC_CPU_TYPE_MENDOCINO,
	K10_SYSTEM_ADMIN_OPC_CPU_TYPE_MAX = K10_SYSTEM_ADMIN_OPC_CPU_TYPE_MENDOCINO 
};

enum K10_SYSTEM_ADMIN_FRONTPORT_USAGE {	// K10_SYSTEM_ADMIN_DB_FRONTBUS
	K10_SYSTEM_ADMIN_FRONTPORT_USAGE_MIN = 0,
	K10_SYSTEM_ADMIN_FRONTPORT_USAGE_NONE = K10_SYSTEM_ADMIN_FRONTPORT_USAGE_MIN,	// invalid, used for initialiazation
	K10_SYSTEM_ADMIN_FRONTPORT_USAGE_CMI,
	K10_SYSTEM_ADMIN_FRONTPORT_USAGE_HOSTATTACH,
	K10_SYSTEM_ADMIN_FRONTPORT_USAGE_MAX = K10_SYSTEM_ADMIN_FRONTPORT_USAGE_HOSTATTACH
};

// Operation types for use in K10_WL_CHECK_SIZE_CHANGE and K10_WL_CHECK_CONSUME
typedef enum K10_WL_LayeredOperations {
	K10_WL_LAYERED_OP_NONE = 0,
	K10_WL_LAYERED_OP_MIN = K10_WL_LAYERED_OP_NONE,
    K10_WL_LAYERED_OP_SIZE_CHANGE,
    K10_WL_LAYERED_OP_MAX = K10_WL_LAYERED_OP_SIZE_CHANGE
} K10_WL_LAYERED_OPERATIONS;


////////////////////////////////////////////////////////////////////////////////
//
// Structure definitions for use in data interface


// for K10_SYSTEM_ADMIN_OPC_LU_ATTRIBUTES_AVAILABLE

typedef struct _K10_system_admin_atttribs_avail {

	long			count;
	K10_DVR_NAME	names[1];

}K10SystemAdminAtttribsAvail;

#define K10_SYSTEM_ADMIN_ATTRIBS_AVAIL_SIZE( count ) \
	(sizeof(K10SystemAdminAtttribsAvail) + \
	(count*sizeof(K10SystemAdminAtttribsAvail)) - sizeof(K10SystemAdminAtttribsAvail) )

// for K10_SYSTEM_ADMIN_OPC_LU_EXTENDED

typedef struct K10_system_admin_LU_filter {

	K10_DVR_NAME	driverName;
	unsigned char	bEnabled;
	unsigned char	checkOnly;
	unsigned char	location;
	unsigned char	btReserved[1];

} K10_System_Admin_LU_Filter;

typedef struct K10_system_admin_LU_extended {

	K10_LU_ID					LU_ID;
	K10_DVR_NAME				exportDriver;
	K10_DVR_NAME				consumingDriver;
	unsigned char				btPublicLU;
	unsigned char				reserved;
	unsigned char				btNoExtend;
	unsigned char				btFilterListLength;
	K10_System_Admin_LU_Filter	filterList[1];

} K10_System_Admin_LU_Extended;

typedef struct K10_system_admin_LU_extended_list {
	unsigned char					version; // K10_SYSTEM_ADMIN_LU_EXTENDED_VERSION
	unsigned char					btFlags;
	unsigned char					btDeleteLun;
	unsigned char					reserved[1];
	unsigned long					DeviceMap;	// may be cast to K10DeviceMapPrivate *
	unsigned long					LunList;	// may be cast to K10LunExtendedList *
	long							luCount;
	K10_System_Admin_LU_Extended	luList[1];

} K10_System_Admin_LU_ExtendedList;


// for K10_SYSTEM_ADMIN_OPC_LU_INIT_DEVMAP

typedef struct K10_system_admin_init_devmap {
	unsigned char					version; // K10_SYSTEM_ADMIN_LU_EXTENDED_VERSION
	unsigned char					btTracePath; // Whether Verbose ON or OFF
	unsigned char					btInit;
	unsigned char					reserved;
                                                                                                                                                                                                                                                                       
} K10_System_Admin_Init_Devmap;

// for K10_SYSTEM_ADMIN_OPC_LU_ASSIGN

typedef struct K10_system_admin_LU_assign {
	K10_WWID						luId;
	unsigned char					bAssign;	// 0 = unassing, 1 = assign
	unsigned char					bAT;		// 1 = AutoTrespass enable
	unsigned char					bAA;		// 1 = AutoAssign enable
	unsigned char					bDefOwn;	// 0 = SPA, 1 = SPB
	K10_WWID						gangId;	
	K10_LOGICAL_ID					NiceName;
	unsigned char					bInternal;	// "Internal" flag for Navi
	unsigned long					DeviceMap;	// may be cast to K10DeviceMapPrivate *
	unsigned long					LunList;	// may be cast to K10LunExtendedList *
	unsigned char					bPreventAddToPhysical;	// Don't allow LUN to be added to ~physical SG.
	
} K10_System_Admin_LU_Assign;

// for K10_SYSTEM_ADMIN_OPC_LU_QUIESCE

enum K10_SYSTEM_ADMIN_OPERATION
{
	K10_SYSTEM_ADMIN_OPERATION_NONE = 0,
	K10_SYSTEM_ADMIN_OPERATION_SHRINK
};

typedef struct K10_system_admin_LU_quiesce {
	K10_WWID						luId;
	unsigned char					bQuiesce;	// 0 = unquiesce, 1 = quiesce
	K10_SYSTEM_ADMIN_OPERATION		operation;	// Indicates type of operation
												// Pesist Trans if K10_SYSTEM_ADMIN_OPERATION_SHRINK and Clear otherwise
	void*							pAttr;		// Stored to avoid rebuilding devicemap durring unquiesce

} K10_System_Admin_LU_Quiesce;


// for K10_SYSTEM_ADMIN_OPC_LU_FIX_LU_STATE
// uses K10_TxnNode, which is in K10GlobalMgmt.h. It is NOT
// an exported (to 3rd parties) function.


// for K10_SYSTEM_ADMIN_OPC_FRONTBUS_DATA 'read'

typedef struct K10_system_admin_frontbus_data {
	long							LogicalPortId;
	unsigned char					PreferredLoopId;
	unsigned char					AcquiredLoopId;
	unsigned long					lNportId;
	unsigned short					AvailableSpeeds;
	unsigned short					CurrentLinkSpeed;
	unsigned short					RequestedLinkSpeed;
} K10SystemAdminFrontBusData;

typedef struct K10_system_admin_frontbus_data_list {
	long							lCount;
	K10SystemAdminFrontBusData		data[1];
} K10SystemAdminFrontBusDataList;

#define K10_SYSTEM_ADMIN_FRONTBUS_DATALIST_SIZE( count ) \
	(sizeof(K10SystemAdminFrontBusDataList) + \
	(count*sizeof(K10SystemAdminFrontBusData)) - sizeof(K10SystemAdminFrontBusData) )

// for K10_SYSTEM_ADMIN_OPC_CMI_ENUMERATE_ARRAYS 'read'

typedef struct K10_system_admin_cmi_data_list{
	long							lCount;
	K10_ARRAY_ID					data[1];
} K10SystemAdminCmiDataList;

// for K10_SYSTEM_ADMIN_OPC_DISK_FIRMWARE

#define FDF_FILENAME_PATH_LEN    512
typedef drive_selected_element_t ODFU_DISK_DETAIL;
typedef drive_selected_element_t *PODFU_DISK_DETAIL;

typedef struct K10_system_admin_write_firmware {

	unsigned char		btVendorId[INQ_VENDOR_ID_SIZE];
	unsigned char		btProductId[INQ_PRODUCT_ID_SIZE];
	unsigned char		btFirmwareRev[INQ_FIRMWARE_REV_SIZE];
	unsigned char		btDriveBitmap[FDF_MAX_SELECTED_BYTES]; // optional if All flag set
	unsigned char		bCheckRevision;		// 0 or 1
	unsigned char		bForceUpdate;		// 0 or 1
	unsigned char		bDoAll;				// if nonzero writes FW to all CRUs
	unsigned char		bMetaDataInFile;	// vendor ID etc. is embedded in file
	long				lFileNameLen;	// including terminating NULL
	char				sFileName[FDF_FILENAME_PATH_LEN];	// NULL_TERMINATED string of given length
	NAPtr<ODFU_DISK_DETAIL> napDiskDetailList; // This will have list of ODFU_DISK_DETAIL objects
	short				shSelectedDriveCnt; // Number of drives selected for upgrade, when "doall" not selected

} K10SystemAdminWriteFirmware;

// for K10_SYSTEM_ADMIN_DB_FRONTBUS

typedef struct K10_system_admin_write_frontbus {
	HOST_PORT_KEY PortId;
	unsigned char ucPrefLId;
	unsigned short usSpeed;
	bool bHasPref;
	bool bHasSpeed;

} K10SystemAdminWriteFrontbus;

// for K10_SYSTEM_ADMIN_DB_CONTROL

typedef struct K10_system_admin_stats_log {
	bool bStatsLog;

} K10SystemAdminStatsLog;

// for K10_SYSTEM_ADMIN_DB_WORKLIST

typedef struct K10_WorkListData_LD {
	K10_LU_ID			luId;
	unsigned char		location;
	unsigned char		bInsert; // 0 = remove, 1 = insert

	// Save this stuff in case we have to back the operation out
	// This is what would normally get saved as part of an old
	// transaction node.
	K10_DVR_OBJECT_NAME		oldTopDevName;	// If top not consumer, will need old devName
	K10_DVR_NAME			topDvr;			// driver on top of operDvr in stack
	unsigned char			topLocation;	// topDvr location value
	unsigned char			btIsConsumer;	// is topDvr a consumer (else layerer)

} K10_WorkListDataLD;

typedef struct K10_WorkListData_Consume {
	K10_LU_ID			luId;
	unsigned char		bConsume; // 0 = unconsume, 1 = consume
	unsigned char		bAllowLayers; // 0 = Do not proceed if extra filters present on LU
									  // 1 = Extra Filters OK.

	// Info we need if we are to back this operation out.
	unsigned char		bWasAssigned;
	unsigned char		bAT;		// 1 = AutoTrespass enable
	unsigned char		bAA;		// 1 = AutoAssign enable
	unsigned char		bDefOwn;	// 0 = SPA, 1 = SPB
    unsigned char       bInternal;  // 1 = Internal lun
	K10_WWID			gangId;	
} K10_WorkListDataConsume;

typedef struct K10_WorkListData_Spoof {
	K10_LU_ID			luId;
	unsigned char		location;
	K10_LU_ID			NewluId;
	DWORD				NewluNum;			// new LU number, INVALID_UNIT indicates the
											// LU number of the spoofed LU should be used

	// Save this stuff in case we have to back the operation out
	// This is what would normally get saved as part of an old
	// transaction node.
	DWORD					OldluNum;		// old LU number, if changing
	K10_DVR_OBJECT_NAME		oldTopDevName;	// If top not consumer, will need old devName
	K10_DVR_NAME			topDvr;			// driver on top of operDvr in stack
	unsigned char			topLocation;	// topDvr location value
	K10_DVR_OBJECT_NAME		oldBotDevName;	// need old devName for spoofed LU
	K10_DVR_NAME			botDvr;			// driver creating spoofed LU
	unsigned char			btIsConsumer;	// is topDvr a consumer (else layerer)
	unsigned char			SpoofState;		// Holder for progress indicator

} K10_WorkListDataSpoof;

typedef struct K10_WorkListData_Renumber {
	K10_LU_ID			luId;
	long				OldNumber;
} K10_WorkListDataRenumber;

typedef struct K10_WorkListData_Delete {
	K10_LU_ID			luId;			// WWN that object goes by
	ULONG       		opaque0;
	ULONG       		opaque1;
} K10_WorkListDataCreateDelete;

typedef struct K10_WorkListData_Complete {
	K10_LU_ID			luId;
	ULONG       		opaque0;
	ULONG       		opaque1;
} K10_WorkListDataComplete;

typedef struct K10_WorkListData_Quiesce {
	K10_LU_ID							luId;
	unsigned char						bQuiesce;		// 0 = unquiesce, 1 = quiesce
	K10_SYSTEM_ADMIN_ISPEC_LU_QUIESCE	quiesceMode;
} K10_WorkListDataQuiesce;

//
// Structure of *ppOut output parameter for
// K10_DISK_ADMIN_OPC_LIST_SUPPORTED/ACTIVE_OPS.
// 
// The errorCode field may be set to S_OK, in which case core admin will
// provide a generic error if the requested operation is unsupported, or the
// LD Admin may provide a more specific error to help the user.
//
// The count field indicates the number of operations returned.
//
// The willAbsorb flag should be set to true if the driver will absorb the
// size change.  (see also mustReachMe flag below)
//
// The 1-sized array is a placeholder -- space should be allocated (using
// K10_DISK_ADMIN_OPLIST_SIZE) for however many operations are returned
// in the list.
//
typedef struct K10DiskAdmin_Ops_List
{
    HRESULT     				errorCode;
    unsigned int                count;
    BOOL                        willAbsorb;
    K10_WL_LAYERED_OPERATIONS	opsList[1];
} K10DiskAdmin_OpsList;

#define K10_DISK_ADMIN_OPLIST_SIZE(m_count) \
    sizeof(K10DiskAdmin_OpsList) + \
        (((((m_count) == 0) ? 1 : (m_count)) - 1) * \
         sizeof(K10_WL_LAYERED_OPERATIONS))

typedef struct K10_Work_List_Data_Size_Change
{
	ULONGLONG	                            newSize;
    K10_DISK_ADMIN_LIST_SUPPORTED_REASON	checkReason;
    BOOL                                    mustReachMe;
                        // This flag, if set, indicates that the driver
                        // initiating the size change must be reached while
                        // proceeding down the stack checking if an
                        // operation is supported.  (see also willAbsorb
                        // flag above)
} K10_WORK_LIST_DATA_SIZE_CHANGE;

typedef union K10_WorkListData_Check_Params
{
	K10_WORK_LIST_DATA_SIZE_CHANGE	sizeChangeParams;
} K10_WORK_LIST_DATA_CHECK_PARAMS;

typedef struct K10_WorkListData_Check {
	K10_LU_ID					    luId;
	unsigned long        		    CheckType;
	K10_WORK_LIST_DATA_CHECK_PARAMS	checkParams;
} K10_WorkListDataCheck;


typedef struct K10_WorkListData_LunCfg {
   K10_LU_ID         luId;
   unsigned char     location;
   K10_LU_ID         destLuId;
   K10_LU_ID         tmpLuId;          // temporay LU Id assigned to the swapped newLuId
   
   // Save this stuff in case we have to back the operation out
   // This is what would normally get saved as part of an old
   // transaction node.
   K10_DVR_NAME         destBotDrvr;   // bottom driver for the destination Lun
   K10_DVR_OBJECT_NAME  destBotDev;    // need old devName for the destination Lun
   K10_DVR_NAME         srcBotDrvr;    // driver creating the source Lun
   K10_DVR_OBJECT_NAME  srcBotDev;     // need old devName for the source Lun
   K10_DVR_NAME         srcTopDrvr;    // driver creating the source Lun
   K10_DVR_OBJECT_NAME  srcTopDev;     // need old devName for the source Lun
   K10_LOGICAL_ID       srcNiceName;   // nice name for the destination Lun
   ULONG				BindStamp;		
   UCHAR				RaidType;
   bool			        topIsFilter;        // is topDvr a consumer (else layerer)
   UCHAR				srcTopLocation; // location value for driver above target driver

   ULONG                opaque0;        // opaque field 0 for future usage
   ULONG                opaque1;        // opaque filed 1 for future usage 	
} K10_WorkListDataLunCfg;


typedef union K10_WorkListData_U {
	K10_WorkListDataLD				nodeDataLD;
	K10_System_Admin_LU_Assign		nodeDataAssign;  // Same structure as a normal assign
	K10_WorkListDataRenumber		nodeDataRenumber;
	K10_WorkListDataCreateDelete	nodeDataCreDel;
	K10_WorkListDataConsume			nodeDataConsume;
	K10_WorkListDataSpoof			nodeDataSpoof;
	K10_WorkListDataComplete		nodeDataComplete;
	K10_WorkListDataCheck			nodeDataCheck;
	K10_WorkListDataLunCfg			nodeDataLunCfg;
	K10_WorkListDataQuiesce			nodeDataQuiesce;
} K10_WorkListDataU;

typedef struct K10_WorkList_Node {
	K10_WORK_LIST_TYPE			type;
	unsigned char				NodeState;
	K10_WorkListDataU			uData;
} K10_WorkListNode;

typedef struct K10_WorkList {
	long					version;
	long					count;
	K10_DVR_NAME			CurrDriver;
	K10_WorkListNode		NodeList[1];
} K10_WorkList;

/* K10_WorkListData structure used to return ppWorkList or ppTldData
ppWorkList : [OUT]	Will be returned by layered driver admin while it is returning worklist
						for K10_DISK_ADMIN_OPC_GET_WORKLIST
			 [IN]	Core admin will supply it while calling K10_SYSTEM_ADMIN_OPC_WORKLIST_PROCESS_WITH_RETURN_DATA
ppBuff     : [OUT]	TLD data returned by Layered driver admin if any
lBufSize   : [OUT]	Size of WorkList returned by Layered driver admin.
LunList    : [IN]	May be cast to a K10LunExtendedList *
luId       : [IN]	The associated lun ID, if applicable
*/
typedef struct K10_WorkListData {
	csx_uchar_t		**ppWorkList;
	csx_uchar_t		**ppBuff;
	long			lBufSize;
	unsigned long		LunList;	// may be cast to K10LunExtendedList *
	K10_LU_ID		luId;
} K10_WorkListData;


#define K10_SYSTEM_ADMIN_WORKLIST_SIZE( count ) \
	(sizeof(K10_WorkList) + \
	((count)*sizeof(K10_WorkListNode)) - sizeof(K10_WorkListNode) )

typedef struct K10_AuxDelList {
	long		Count;
	K10_LU_ID	WWNList[1];
}K10_AuxDelList;

#define K10_SYSTEM_ADMIN_AUXDELLIST_SIZE( count ) \
	(sizeof(K10_AuxDelList) + \
	((count)*sizeof(K10_LU_ID)) - sizeof(K10_LU_ID) )


// for K10_SYSTEM_ADMIN_OPC_ISCSI_PORT_SYNC

enum K10_SYSTEM_ADMIN_PORTSYNC_STATUS {
	PORT_INVALID = 0,	// Illegal value - coding error if seen
	PORT_OK,			// Matches miniport, no changes needed
	PORT_MODIFIED,		// Updated to match miniport settings
	PORT_ERROR			// May not/does not match miniport, but couldn't correct
};

typedef struct _K10_system_admin_iscsi_port_status {
	int									logicalPortID;
	K10_SYSTEM_ADMIN_PORTSYNC_STATUS	status;
	HRESULT extendedStatus;
} K10_System_Admin_Iscsi_Port_Status;


typedef struct _K10_system_admin_iscsi_sync_list {

	long								count;
	K10_System_Admin_Iscsi_Port_Status	portStatus[1];

} K10_System_Admin_Iscsi_Sync_List;

#define K10_SYSTEM_ADMIN_ISCSI_SYNC_LIST_SIZE( count ) \
	(sizeof(K10_System_Admin_Iscsi_Sync_List) + \
	((count)*sizeof(K10_System_Admin_Iscsi_Port_Status)) -  \
	sizeof(K10_System_Admin_Iscsi_Port_Status))

// for K10_SYSTEM_ADMIN_OPC_NETID_MGMT_IP_SETUP_NON_TLD
typedef struct _K10_system_admin_mgmt_port_setup_physical_data
{
	long	requestedSpeed;
	long	requestedDuplexMode;
} K10_system_admin_mgmt_port_setup_physical_data;

// for K10_SYSTEM_ADMIN_OPC_NETID_MGMT_PORT_PHYSICAL_DATA

typedef struct _K10_system_admin_mgmt_port_physical_data
{
	bool	speedSupported;
	bool	linkUp;
	bool	autoNegotiate;
	long	currentSpeed;
	long	requestedSpeed;
	long	requestedDuplexMode;
	long	duplexMode;
	long	allowedSpeeds;
} K10_system_admin_mgmt_port_physical_data;

#endif //K10_SYSTEM_ADMIN_EXPORT
