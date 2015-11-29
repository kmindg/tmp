/*******************************************************************************
 * Copyright (C) EMC Corporation, 1997 - 2015
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * scsitarg.h
 *
 * This header file contains structures and constants which are needed by other
 * drivers and applications when performing IOCTLs to TCD.
 *
 * Notes:
 *
 * Kernel mode drivers must include ntddk.h, and user mode applications must 
 * include winioctl.h before including this header file.
 *
 ******************************************************************************/

#ifndef _SCSITARG_H_
#define _SCSITARG_H_

#ifndef K10_DEFS_H
#include "k10defs.h"	// K10_LOGICAL_ID_LEN, K10_DEVICE_ID_LEN
#endif

// Include more specific parts of the interface...

#include "ScsiTargGlobals.h"
#include "scsitarghostports.h"
#include "ScsiTargInitiators.h"
#include "ScsiTargPortalGroups.h"
#include "ScsiTargITNexus.h"
#include "ScsiTargLoginSessions.h"
#include "ScsiTargHosts.h"
#include "ScsiTargSystemOptions.h"
#include "ScsiTargStorageGroups.h"
#include "ScsiTargLogicalUnits.h"
#include "ScsiTargProtocolEndpoints.h"
#include "ScsiTargVirtualVolumes.h"


// FEATURE_ENTRY - Used to pass information about the
//                 hard coded features defined in the system.

typedef struct _FEATURE_ENTRY
{

	ULONG		Id;
	UCHAR		name[64];
	ULONG		name_len;
	ULONG		status;

} FEATURE_ENTRY, *PFEATURE_ENTRY;

#define FEATURE_KEY_STATUS_SUPPORTED      0x00000004
#define FEATURE_KEY_STATUS_INCLUDED       0x00000008

// A FEATURE_LIST structure is used to contain 
// an entire list of Feature Entries.

typedef struct _FEATURE_LIST {

	ULONG			Revision;
	ULONG			Count; // the number of entries.
	FEATURE_ENTRY	Feature[1];

} FEATURE_LIST, *PFEATURE_LIST;

#define CURRENT_FEATURE_LIST_REV	1	

// Macro: FEATURE_LSIZE
// Determines the number of bytes required for 
// a FEATURE_LIST with IdCount entries.

#define FEATURE_LSIZE(IdCount)							\
	(sizeof(FEATURE_LIST) - sizeof(FEATURE_ENTRY) +		\
	(IdCount) * sizeof(FEATURE_ENTRY))


/*******************************************************************
 *
 * Limits Structure - Used to pass information about the current
 *                  hostside limits and limit ranges.
 *
 *******************************************************************/

typedef struct _LIMIT_RANGES
{

    ULONG       MinimumConfigurable;
    ULONG       MaximumConfigurable;
    ULONG       CurrentMaximum;

} LIMIT_RANGES, *PLIMIT_RANGES;

typedef struct _HOSTSIDE_LIMITS {

    ULONG			Revision;
	ULONG			Size;       // Size of entire structure
    LIMIT_RANGES    StorageGroups;
    LIMIT_RANGES    StorageConnectedHosts;

} HOSTSIDE_LIMITS, *PHOSTSIDE_LIMITS;

#define CURRENT_LIMITS_REV	1


typedef struct _CDB_IOCTL_IN
{
	UCHAR			QueueAction;
	ULONGLONG		Lun;
	USHORT			TagId;
	UCHAR			Cdb[16];
	UCHAR			DataOut[1];		// Must be last. May be longer.
} CDB_IOCTL_IN, *PCDB_IOCTL_IN;

#if		defined(UMODE_ENV) || defined(SIMMODE_ENV)
typedef struct _CDB_IOCTL_OUT
{
	LONG			NTStatus;
	UCHAR			ScsiStatus;
	UCHAR			SenseData[18];
	UCHAR			DataIn[1];		// Must be last. May be longer.
} CDB_IOCTL_OUT, *PCDB_IOCTL_OUT;
#else
typedef struct _CDB_IOCTL_OUT
{
	EMCPAL_STATUS		NTStatus;
	UCHAR			ScsiStatus;
	SENSE_DATA		SenseData;
	UCHAR			DataIn[1];		// Must be last. May be longer.
} CDB_IOCTL_OUT, *PCDB_IOCTL_OUT;
#endif

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
typedef struct _TCD_TRESPASS_IOCTL_IN
{
	XLU_WWN			Wwn;
	ULONG			TrespassCode;
	BOOLEAN			TrespassLogSuppress;

    // For each reason (i.e., driver) which may be asking for ownership locking to 
    // be suppressed, this field has a corresponding bit.  If this bit is clear, then
    // ownership should not be moved from the owner if the owner is asserting this lock
    // reason.  If this bit is set, ignore that ownership lock reason.
    //
    // Therfore, if this field is 0, then all ownership locks are obeyed, if ~0, ownership 
    // locking will occur regardless.
    //
    // Proper type is TRESPASS_DISABLE_REASONS, but this creates include problems.
    ULONG  TrespassDisableReasonsToIgnore;

} TCD_TRESPASS_IOCTL_IN, *PTCD_TRESPASS_IOCTL_IN;
#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */

typedef struct _HPORT_ADJUSTMENT {

	HOST_PORT_TYPE	PortType:16;
	UCHAR			PhysicalPortNumber;
	UCHAR			LogicalPortID;

} HPORT_ADJUSTMENT, *PHPORT_ADJUSTMENT;


/**********************************************************************************/
/**********************************************************************************/
/************************************  IOCTLs  ************************************/
/**********************************************************************************/
/**********************************************************************************/

#define SCSITARG_CTL_CODE(Op)			(ULONG)\
	EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_SCSITARG, (Op), EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

		
#ifdef ALAMOSA_WINDOWS_ENV
#define SCSITARG_CTL_CODE_DIRECT_IN(Op)			(ULONG)\
		EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_SCSITARG, (Op), EMCPAL_IOCTL_METHOD_IN_DIRECT, EMCPAL_IOCTL_FILE_ANY_ACCESS)
#else
#define SCSITARG_CTL_CODE_DIRECT_IN(Op)			(ULONG)\
		EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_SCSITARG, (Op), EMCPAL_IOCTL_METHOD_OUT_DIRECT, EMCPAL_IOCTL_FILE_ANY_ACCESS)
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - this should be OUT_DIRECT.   IN_DIRECT treats the output buffer as another input buffer.  here we're using the output buffer for output */

/****************************************************/
/* XLU IOCTLs use SCSITARG_CTL_CODE(0x800 to 0x83F).*/
/****************************************************/

// IOCTL_SCSITARG_ENUM_XLUS returns a list of XLU keys.
//
// Input Buffer:	None.
//
// Output buffer:	XLU_KEY_LIST.
//
//		Note: XKL->Count is the total number of XLUs.

#define IOCTL_SCSITARG_ENUM_XLUS				SCSITARG_CTL_CODE(0x800)

// IOCTL_SCSITARG_GET_XLU_ENTRY returns information about a particular XLU.
//
// Input Buffer:	XLU_WWN - identifies the XLU.
//
// Output buffer:	XLU_ENTRY.

#define IOCTL_SCSITARG_GET_XLU_ENTRY			SCSITARG_CTL_CODE(0x801)

// IOCTL_SCSITARG_SET_XLU_PARAM stores information about a particular XLU 
// persisently.
//
// Input Buffer:	XLU_PARAM identifies the data to write.  
//
//		Note: XP->Key specifies which XLU to update, or the WWN of a new XLU.
//
// Output buffer:	None.
//
// Restrictions:	Can't change Type  field.
//					Must be quiesced to change DeviceName, RequestedSize, Offset.

#define IOCTL_SCSITARG_SET_XLU_PARAM			SCSITARG_CTL_CODE(0x802)

// IOCTL_SCSITARG_DELETE_XLU removes ALL information about a particular XLU.  
//
// Input Buffer:	XLU_WWN - identifies the XLU to delete
//
// Output buffer:	None.
//
// Restrictions:	Can't delete if referenced by any Storage Group.

#define IOCTL_SCSITARG_DELETE_XLU				SCSITARG_CTL_CODE(0x803)

// The following Quiesce All IOCTLs place all XLUs in a quiesced mode on the 
// receiving SP only.  These operations are not shared with the other SP!
// IOCTL_SCSITARG_QUIESCE_ALL_DYING is the only supported function.  Once
// all XLUs are placed in a quiesced mode, there is no way to get them out
// of that mode (must reboot SP).
//
// Input Buffer:	None.
//
// Output buffer:	None.

#define IOCTL_SCSITARG_QUIESCE_ALL_DYING		SCSITARG_CTL_CODE(0x806)

// The following Quiesce/Unquiesce IOCTLs either place an XLU in the quiesced 
// mode or remove an XLU from the quiesced mode.
//
// Input Buffer:	XLU_WWN - identifies the XLU.
//
// Output buffer:	None.

#define IOCTL_SCSITARG_QUIESCE_XLU_QUEUED		SCSITARG_CTL_CODE(0x807)
#define IOCTL_SCSITARG_QUIESCE_XLU_BUSY			SCSITARG_CTL_CODE(0x808)
#define IOCTL_SCSITARG_UNQUIESCE_XLU			SCSITARG_CTL_CODE(0x809)

// IOCTL_SCSITARG_GET_XLU_GANG_LIST takes an XLU WWN as input, and returns
// a list of all XLU WWNs that belong to the specified XLU's gang.
//
// Input buffer:	XLU_WWN - identifies the XLU.
//
// Output buffer:	XLU_KEY_LIST.
//
// Restrictions:	None.

#define IOCTL_SCSITARG_GET_XLU_GANG_LIST		SCSITARG_CTL_CODE(0x80A)

// IOCTL_SCSITARG_GET_ALL_XLU_ENTRIES returns a list of XLU entries.
//
// Input Buffer:	None.
//
// Output buffer:	XLU_ENTRY_LIST.  
//
		
#define IOCTL_SCSITARG_GET_ALL_XLU_ENTRIES			SCSITARG_CTL_CODE_DIRECT_IN(0x80B)
		
/**************************************************************/
/* Storage Group IOCTLs use SCSITARG_CTL_CODE(0x840 to 0x87F).*/
/**************************************************************/

// IOCTL_SCSITARG_ENUM_STORAGE_GROUPS returns a list of V.A. keys.  This will
// include the "Null Virtaul Array" which is automatically generated by TCD.
//
// Input Buffer:	None.
//
// Output buffer:	SG_KEY_LIST.
//
//		Note: VAKL->Count is the total number of Storage Groups.

#define IOCTL_SCSITARG_ENUM_STORAGE_GROUPS		SCSITARG_CTL_CODE(0x840)

// IOCTL_SCSITARG_GET_STORAGE_GROUP_ENTRY returns information about a 
// particular STORAGE_GROUP.
//
// Input Buffer:	SG_WWN - identifies the STORAGE_GROUP.
//
// Output buffer:	STORAGE_GROUP_ENTRY.

#define IOCTL_SCSITARG_GET_STORAGE_GROUP_ENTRY	SCSITARG_CTL_CODE(0x841)

// IOCTL_SCSITARG_SET_STORAGE_GROUP_PARAM stores information about a particular 
// Virtaul Array persisently.
//
// Input Buffer:  STORAGE_GROUP_PARAM - identifies the data to write.  
//
//		Note: VAP->Key specifies which V.A. to update, or the WWN of a new V.A.
//
// Output buffer:	None.
//
// Restrictions:	Can't add or change the NULL Storage Group.
//					Can't add (but okay to change) the Physical Storage Group.
//					Lun field in each LunMap[] entry must be unique.
//					XLU specified in each LunMap[] entry must exist.

#define IOCTL_SCSITARG_SET_STORAGE_GROUP_PARAM	SCSITARG_CTL_CODE(0x842)

// IOCTL_SCSITARG_DELETE_STORAGE_GROUP removes ALL information about a 
// particular STORAGE_GROUP.  
//
// Input Buffer:	SG_WWN - identifies the Storage Group to delete
//
// Output buffer:	None.
//
// Restrictions:	Can't delete the NULL or Physical Array.
//					Can't delete if referenced by any HostPort or Initiator.

#define IOCTL_SCSITARG_DELETE_STORAGE_GROUP		SCSITARG_CTL_CODE(0x843)

/***************************************************************/
/* System Options IOCTLs use SCSITARG_CTL_CODE(0x880 to 0x8BF).*/
/***************************************************************/

// IOCTL_SCSITARG_SET_SYSTEM_OPTIONS sets the system's (new initiator's) type
// and options in ALL port entries.
//
// Input Buffer:  HOST_SYSTEM_OPTIONS
//
// Output Buffer: None

#define IOCTL_SCSITARG_SET_SYSTEM_OPTIONS		SCSITARG_CTL_CODE(0x880)


// IOCTL_SCSITARG_GET_SYSTEM_OPTIONS returns the system's type and options
// data to the caller.
//
// Input Buffer:  None
//
// Output Buffer: HOST_SYSTEM_OPTIONS

#define IOCTL_SCSITARG_GET_SYSTEM_OPTIONS		SCSITARG_CTL_CODE(0x881)

// IOCTL_SCSITARG_GET_NON_PERSISTED_SYSTEM_INFO returns System information NOT saved
// in the PSM.  Info in this format avoids NDU upgrade issues.
//
// Input Buffer:  buffer (set InfoType to indicate information requested)
//
// Output Buffer: buffer (contains requested info)

#define IOCTL_SCSITARG_GET_NON_PERSISTED_SYSTEM_INFO		SCSITARG_CTL_CODE(0x882)

/*****************************************************/
/* Port IOCTLs use SCSITARG_CTL_CODE(0x8C0 to 0x8FF).*/
/*****************************************************/

// IOCTL_SCSITARG_ENUM_PORTS returns a list of PORT keys that are either 
// installed or configured.
//
// Input Buffer:	None.
//
// Output buffer:	HOST_PORT_KEY_LIST.  
//
//		Note: HPKL->Count is the total number of ports.

#define IOCTL_SCSITARG_ENUM_PORTS				SCSITARG_CTL_CODE(0x8C0)

// IOCTL_SCSITARG_GET_PORT_ENTRY returns information about a particular port.
//
// Input Buffer:	HOST_PORT_KEY - identifies the port.
//
// Output buffer:	HOST_PORT_ENTRY, 

#define IOCTL_SCSITARG_GET_PORT_ENTRY			SCSITARG_CTL_CODE(0x8C1)

// IOCTL_SCSITARG_SET_PORT_PARAM stores information about a particular port 
//	persisently.
//
// Input Buffer:	HOST_PORT_PARAM - identifies the data to write.  
//
//		Note: HPP->Key specifies which port to update.
//
// Output buffer:	None.
//
// Restrictions:	PortEntry must exist prior to this operation.
//					Can only change TargetId if it is zero (WWN not set).

#define IOCTL_SCSITARG_SET_PORT_PARAM				SCSITARG_CTL_CODE(0x8C2)
#define IOCTL_SCSITARG_SET_PORT_WITHOUT_RESET		SCSITARG_CTL_CODE(0x8C5)

// IOCTL_SCSITARG_DELETE_PORT removes persistent information about a particular 
// port.  If the port is online, or otherwise exists, the Port object will 
// remain visible, but it will be marked as "temporary".
//
// Input Buffer:  HOST_PORT_KEY - identifies the port to delete.
//
// Output buffer: None.

#define IOCTL_SCSITARG_DELETE_PORT				SCSITARG_CTL_CODE(0x8C3)

// IOCTL_SCSITARG_START_FE_PORTS enables all front end ports (who have a WWN)
// on the receiving SP.  This operation is not shared with the other SP!
// This function must be called once (and only once) per SP.
//
// Input Buffer:	None.
//
// Output buffer:	None.

#define IOCTL_SCSITARG_START_FE_PORTS			SCSITARG_CTL_CODE(0x8C4)

// IOCTL_SCSITARG_SET_PORT_WWN_ID is a "recovery" IOCTL used to re-create the
// the Port Conversion Table.  Any temporary ports that are affected by this 
// IOCTL are updated on both SPs.  Any persisted port records that are affected
// by this IOCTL are updated in the PSM file.  An entry is added to the Port
// Conversion Table on both SPs, and the updated Port Conversion Table is 
// written-out to PSM.  NOTE: The Port Conversion Table only contains fibre
// channel information.
//
// Input Buffer:	HPORT_ADJUSTMENT
//
// Output buffer:	None.
//
// Restrictions:	Affected Port records cannot be reference (by an ITNexus).

#define IOCTL_SCSITARG_SET_PORT_WWN_ID			SCSITARG_CTL_CODE(0x8C6)

// IOCTL_SCSITARG_GET_ALL_PORTS returns a list of PORT entries that are either 
// installed or configured.
//
// Input Buffer:	None.
//
// Output buffer:	HOST_PORT_LIST.  
//
//		Note: HPortList->Count is the total number of ports.

#define IOCTL_SCSITARG_GET_ALL_PORTS			SCSITARG_CTL_CODE(0x8C7)

// IOCTL_SCSITARG_GET_PORTAL_NR returns a list of "portal numbers" for the 
// specified Virtual Port on the specified iSCSI Port.
//
// Zero to 'N' IP Addresses may be assigned to an iSCSI port.  For each IP Address, 
// Scsitarg must select a portal number, which is a value from 0 to N-1.  Note that 
// 'N' is the number of IP Addresses that the port can accept.  Anyway the portal 
// number (called the Portal NR) is simply a non-persisted number that Scsitarg 
// selects for each IP Address it submits to the miniport.
//
// A Virtual Port number is selected by the array administrator for each IP Address 
// that they set-up.  Scsitarg persists the Virtual Port number in the Port record,
// but uses it for nothing except this IOCTL.
//
// IP Addresses can be of type IPv4 or IPv6.  This IOCTL will return the portal numbers
// that correlate with the specified Virtual Port number on the specified port and are
// of the specified IP Address type.
//
// Input Buffer:	VIRTUAL_PORT_SELECTOR
//
// Output buffer:	PORTAL_NUMBER_LIST
//
// Restrictions:	Output buffer must be at least 8 bytes in length, and a multiple of 4 bytes.
//					IOCTL must be called at passive IRQL.
//					IOCTL must be issued on SP that port resides on.

#define IOCTL_SCSITARG_GET_PORTAL_NR			SCSITARG_CTL_CODE(0x8C8)


/**********************************************************/
/* Initiator-Target Nexus IOCTLs use SCSITARG_CTL_CODE(0x900 to 0x93F).*/
/**********************************************************/

// IOCTL_SCSITARG_ENUM_I_T_NEXUS returns list of Initiator keys for all
// Initiators (configured or not).
//
// Input Buffer:	None.
//
// Output buffer:	I_T_NEXUS_KEY_LIST.
//
//		Note: IKL->Count is the total number of Initiators.

#define IOCTL_SCSITARG_ENUM_I_T_NEXUS			SCSITARG_CTL_CODE(0x900)

// IOCTL_SCSITARG_GET_I_T_NEXUS_ENTRY returns information about a particular 
// I_T_NEXUS.
//
// Input Buffer:	I_T_NEXUS_KEY - identifies the I_T_NEXUS.
//
// Output buffer:	I_T_NEXUS_ENTRY.

#define IOCTL_SCSITARG_GET_I_T_NEXUS_ENTRY		SCSITARG_CTL_CODE(0x901)

// IOCTL_SCSITARG_SET_I_T_NEXUS_PARAM stores information about a particular 
// Initiator persisently.
//
// Input Buffer:	I_T_NEXUS_PARAM - identifies the data to write.  
//
//		Note: IP->Key specifies which Initiator to update, or the INIIATOR_KEY 
//				of a new Initiator.
//
// Output buffer:	None.
//
// Restrictions:	StorageGroupWWN must specify a Storage Group that exists.
//					InitiatorType must be valid.
//					Specified Host Port must be persisted.

#define IOCTL_SCSITARG_PRE_R16_SET_ITN			0x902
#define IOCTL_SCSITARG_SET_I_T_NEXUS_PARAM		SCSITARG_CTL_CODE(0x902)

// IOCTL_SCSITARG_DELETE_I_T_NEXUS removes ALL information about a particular 
// I_T_NEXUS.  
//
// Input Buffer:	I_T_NEXUS_KEY - identifies the Initiator to delete.
//
// Output buffer:	None.
//
// Restrictions:	Specified Initiator must be persisted or logged in.

#define IOCTL_SCSITARG_DELETE_I_T_NEXUS			SCSITARG_CTL_CODE(0x903)

// IOCTL_SCSITARG_GET_ALL_I_T_NEXUS_ENTRIES returns a list of ITNexus entries
//
// Input Buffer:	None.
//
// Output buffer:	I_T_NEXUS_LIST.  
//
		
#define IOCTL_SCSITARG_GET_ALL_I_T_NEXUS_ENTRIES		SCSITARG_CTL_CODE_DIRECT_IN(0x904)

		
/*********************************************************/
/* Features IOCTLs use SCSITARG_CTL_CODE(0x940 to 0x97F).*/
/*********************************************************/

#define IOCTL_SCSITARG_ENUM_FEATURES			SCSITARG_CTL_CODE(0x940)

#define IOCTL_SCSITARG_GET_LIMITS               SCSITARG_CTL_CODE(0x941)


/****************************************************/
/* TxD IOCTLs use SCSITARG_CTL_CODE(0x980 to 0x9BF).*/
/****************************************************/

//	Allow Target X Drivers, where X represents Disk, Tape, etc., to register
//	with the Target Class Driver and then to activate itself.

#define IOCTL_TCD_TXD_LINK						SCSITARG_CTL_CODE(0x980)
#define IOCTL_TCD_TXD_ACTIVATE					SCSITARG_CTL_CODE(0x981)
#define IOCTL_QOS_MSG                           SCSITARG_CTL_CODE(0x982)   


/**********************************************************/
/* Ownership IOCTLs use SCSITARG_CTL_CODE(0x9C0 to 0x9FF).*/
/**********************************************************/

// IOCTL_SCSITARG_EXECUTE_CDB executes a SCSI CDB as if it were submitted
// by an initiator over the SCSI connection. The physical array is always
// assumed. Only a limited set of operations are supported at this time.
//
// Input Buffer:	CDB_IOCTL_IN.
//
// Output buffer:	CDB_IOCTL_OUT.

#define IOCTL_SCSITARG_EXECUTE_CDB				SCSITARG_CTL_CODE(0x9C0)

// IOCTL_SCSITARG_TCD_TRESPASS trespasses a WWN in the Physical array
//
// Input Buffer:	TCD_TRESPASS_IOCTL_IN.
//
// Output buffer:	None.

#define IOCTL_SCSITARG_TCD_TRESPASS				SCSITARG_CTL_CODE(0x9C1)

//  IOCTL_SCSITARG_TCD_ASSIGN_NOTIFICATION - Informs TCD that something may
//   need to be assigned.  This is typically sent when Flare sees a
//   shutdown unit is now assignable.
//
// Input Buffer:	None.
//
// Output buffer:	None.

#define IOCTL_SCSITARG_TCD_ASSIGN_NOTIFICATION	SCSITARG_CTL_CODE(0x9C2)

/***********************************************************/
/* Statistics IOCTLs use SCSITARG_CTL_CODE(0xA00 to 0xA3F).*/
/***********************************************************/

// IOCTL_SCSITARG_GET_XLU_STATS returns the stats for a LUN
//
// Input Buffer:  XLU_WWN - identifies the XLU
//
// Output Buffer: XLU_STATISTICS

#define IOCTL_SCSITARG_GET_XLU_STATS             SCSITARG_CTL_CODE(0xA00)

// IOCTL_SCSITARG_GET_SYSTEM_STATS returns the stats for a LUN
//
// Input Buffer:  None
//
// Output Buffer: XLU_STATISTICS

#define IOCTL_SCSITARG_GET_SYSTEM_STATS                  SCSITARG_CTL_CODE(0xA01)

// IOCTL_SCSITARG_GET_PORT_STATS returns the stats for a Port
//
// Input Buffer:  PORT_KEY - identifies the PORT
//
// Output Buffer: XLU_STATISTICS

#define IOCTL_SCSITARG_GET_PORT_STATS            SCSITARG_CTL_CODE(0xA02)

// IOCTL_SCSITARG_GET_IO_ACTIVITY returns the IO activity info on this SP.
//
// Input Buffer:  None
//
// Output Buffer: TCD_IO_ACTIVITY

#define IOCTL_SCSITARG_GET_IO_ACTIVITY            SCSITARG_CTL_CODE(0xA03)

/***************************************************************/
/* Authentication IOCTLs use SCSITARG_CTL_CODE(0xA40 to 0xA7F).*/
/***************************************************************/

// IOCTL_SCSITARG_ENUM_AUTHENTICATION_ENTRIES returns a list of authentication entry keys.
//
// Input Buffer:	None.
//
// Output buffer:	AUTHENTICATION_KEY_LIST.
//

#define IOCTL_SCSITARG_ENUM_AUTHENTICATION_ENTRIES	SCSITARG_CTL_CODE(0xA40)

// IOCTL_SCSITARG_GET_AUTHENTICATION_ENTRY returns information about a 
// particular authentication record.
//
// Input Buffer:	AUTHENTICATION_KEY - Identifies the authentication record.
//
// Output buffer:	AUTHENTICATION_ENTRY.

#define IOCTL_SCSITARG_GET_AUTHENTICATION_ENTRY	SCSITARG_CTL_CODE(0xA41)

// IOCTL_SCSITARG_SET_AUTHENTICATION_PARAM stores information about a particular 
// authentication record persistently.
//
// Input Buffer:  AUTHENTICATION_PARAM - identifies the data to write.  
//
// Output buffer:	None.

#define IOCTL_SCSITARG_SET_AUTHENTICATION_PARAM		SCSITARG_CTL_CODE(0xA42)

// IOCTL_SCSITARG_DELETE_AUTHENTICATION_ENTRY removes ALL information about a 
// particular authentication record.  
//
// Input Buffer:	AUTHENTICATION_KEY - identifies the record to delete.
//
// Output buffer:	None.

#define IOCTL_SCSITARG_DELETE_AUTHENTICATION_ENTRY	SCSITARG_CTL_CODE(0xA43)

/**************************************************************/
/* Login Session IOCTLs use SCSITARG_CTL_CODE(0xA80 to 0xABF).*/
/**************************************************************/

// IOCTL_SCSITARG_ENUM_LOGIN_SESSIONS returns a list of LOGIN_SESSION keys.
//
// Input Buffer:	None.
//
// Output buffer:	LOGIN_SESSION_KEY_LIST.
//

#define IOCTL_SCSITARG_ENUM_LOGIN_SESSIONS	SCSITARG_CTL_CODE(0xA80)

// IOCTL_SCSITARG_GET_LOGIN_SESSION_ENTRY returns information about a 
// particular LOGIN session.
//
// Input Buffer:	LOGIN_SESSION_KEY - identifies the session.
//
// Output buffer:	LOGIN_SESSION_ENTRY.

#define IOCTL_SCSITARG_GET_LOGIN_SESSION_ENTRY	SCSITARG_CTL_CODE(0xA81)

// IOCTL_SCSITARG_SET_LOGIN_SESSION_PARAM stores information about a particular 
// LOGIN session persistently. << THIS IOCTL IS OBSOLETE!!! >>
//
// Input Buffer:  LOGIN_SESSION_PARAM - identifies the data to write.  
//
// Output buffer:	None.
//
// Restrictions:	???

#define IOCTL_SCSITARG_SET_LOGIN_SESSION_PARAM	SCSITARG_CTL_CODE(0xA82)

// IOCTL_SCSITARG_DELETE_LOGIN_SESSION removes ALL information about a 
// particular LOGIN session.  
//
// Input Buffer:	LOGIN_SESSION_KEY - identifies the session to delete.
//
// Output buffer:	None.
//
// Restrictions:	???

#define IOCTL_SCSITARG_DELETE_LOGIN_SESSION		SCSITARG_CTL_CODE(0xA83)

// IOCTL_SCSITARG_GET_ALL_LOGIN_SESSIONS returns a list of LOGIN_SESSION entries. 
//
// Input Buffer:	None.
//
// Output buffer:	LOGIN_SESSION_ENTRY_LIST.
//
// Restrictions:	???

#define IOCTL_SCSITARG_GET_ALL_LOGIN_SESSIONS		SCSITARG_CTL_CODE(0xA84)

// IOCTL_SCSITARG_GET_FAILED_LOGIN_SESSIONS returns the failed login sessions from both SP's.
//
// Input Buffer:	None.
//
// Output buffer:	LOGIN_SESSION_KEY_LIST - identifies the failed login sessions.
//
// Restrictions:	None.

#define IOCTL_SCSITARG_GET_FAILED_LOGIN_SESSIONS	SCSITARG_CTL_CODE(0xA85)

// IOCTL_SCSITARG_CLEAR_FAILED_LOGIN_SESSIONS clears the failed login sessions from both SP's. 
//
// Input Buffer:	None.
//
// Output buffer:	None.
//
// Restrictions:	None.

#define IOCTL_SCSITARG_CLEAR_FAILED_LOGIN_SESSIONS	SCSITARG_CTL_CODE(0xA86)

/**********************************************************/
/* Initiator IOCTLs use SCSITARG_CTL_CODE(0xAC0 to 0xAFF).*/
/**********************************************************/

// IOCTL_SCSITARG_ENUM_INITIATOR returns a list of Initiator keys.
//
// Input Buffer:	None.
//
// Output buffer:	INITIATOR_KEY_LIST.
//

#define IOCTL_SCSITARG_ENUM_INITIATOR	SCSITARG_CTL_CODE(0xAC0)

// IOCTL_SCSITARG_GET_INITIATOR_ENTRY returns information about a 
// particular Initiator.
//
// Input Buffer:	INITIATOR_KEY - identifies the Initiator.
//
// Output buffer:	INITIATOR_ENTRY.

#define IOCTL_SCSITARG_GET_INITIATOR_ENTRY	SCSITARG_CTL_CODE(0xAC1)

// IOCTL_SCSITARG_SET_INITIATOR_PARAM stores information about a particular 
// Initiator persistently.
//
// Input Buffer:  INITIATOR_PARAM - identifies the data to write.  
//
// Output buffer:	None.
//
// Restrictions:	???

#define IOCTL_SCSITARG_SET_INITIATOR_PARAM	SCSITARG_CTL_CODE(0xAC2)

// IOCTL_SCSITARG_DELETE_INITIATOR removes ALL information about a 
// particular Initiator.  
//
// Input Buffer:	INITIATOR_KEY - identifies the Initiator to delete.
//
// Output buffer:	None.
//
// Restrictions:	???

#define IOCTL_SCSITARG_DELETE_INITIATOR		SCSITARG_CTL_CODE(0xAC3)

/*****************************************************/
/* Host IOCTLs use SCSITARG_CTL_CODE(0xB00 to 0xB3F).*/
/*****************************************************/

// IOCTL_SCSITARG_ENUM_HOSTS returns a list of Host keys.
//
// Input Buffer:	None.
//
// Output buffer:	HOST_KEY_LIST.
//

#define IOCTL_SCSITARG_ENUM_HOSTS	SCSITARG_CTL_CODE(0xB00)

// IOCTL_SCSITARG_GET_HOST_ENTRY returns information about a 
// particular Host.
//
// Input Buffer:	HOST_KEY - identifies the Host.
//
// Output buffer:	HOST_ENTRY.

#define IOCTL_SCSITARG_GET_HOST_ENTRY	SCSITARG_CTL_CODE(0xB01)

// IOCTL_SCSITARG_SET_HOST_PARAM stores information about a particular 
// Host persistently.
//
// Input Buffer:  HOST_PARAM - identifies the data to write.  
//
// Output buffer:	None.
//
// Restrictions:	???

#define IOCTL_SCSITARG_SET_HOST_PARAM	SCSITARG_CTL_CODE(0xB02)

// IOCTL_SCSITARG_DELETE_HOST removes ALL information about a 
// particular Host.  
//
// Input Buffer:	HOST_KEY - identifies the Host to delete.
//
// Output buffer:	None.
//
// Restrictions:	???

#define IOCTL_SCSITARG_DELETE_HOST		SCSITARG_CTL_CODE(0xB03)


/*************************************************************/
/* Portal Group IOCTLs use SCSITARG_CTL_CODE(0xB40 to 0xB7F).*/
/*************************************************************/

// IOCTL_SCSITARG_ENUM_PORTAL_GROUP returns a list of Portal Group keys.
//
// Input Buffer:	None.
//
// Output buffer:	PORTAL_GROUP_LIST.
//

#define IOCTL_SCSITARG_ENUM_PORTAL_GROUP	SCSITARG_CTL_CODE(0xB40)

// IOCTL_SCSITARG_GET_PORTAL_GROUP_ENTRY returns information about a 
// particular Portal Group.
//
// Input Buffer:	PORTAL_GROUP_KEY - identifies the Portal Group.
//
// Output buffer:	PORTAL_GROUP_ENTRY.

#define IOCTL_SCSITARG_GET_PORTAL_GROUP_ENTRY	SCSITARG_CTL_CODE(0xB41)

// IOCTL_SCSITARG_SET_PORTAL_GROUP_PARAM stores information about a particular 
// Portal Group persistently.
//
// Input Buffer:  PORTAL_GROUP_PARAM - identifies the data to write.  
//
// Output buffer:	None.
//
// Restrictions:	???

#define IOCTL_SCSITARG_SET_PORTAL_GROUP_PARAM	SCSITARG_CTL_CODE(0xB42)

// IOCTL_SCSITARG_DELETE_PORTAL_GROUP removes ALL information about a 
// particular Portal Group.  
//
// Input Buffer:	PORTAL_GROUP_KEY - identifies the Portal Group to delete.
//
// Output buffer:	None.
//
// Restrictions:	???

#define IOCTL_SCSITARG_DELETE_PORTAL_GROUP	SCSITARG_CTL_CODE(0xB43)


/************************************************************************/
/* PE (Protocol Endpoint) IOCTLs use SCSITARG_CTL_CODE(0xB80 to 0xBBF). */
/************************************************************************/

// IOCTL_SCSITARG_ENUM_PES returns a list of PE keys.  The PE key is the WWN.
// Since PEs are stored as XLUs (and SGs), the type of the list is an XLU key
// list. 
//
// Input Buffer:	None.
//
// Output buffer:	XLU_KEY_LIST.
//

#define IOCTL_SCSITARG_ENUM_PES		        SCSITARG_CTL_CODE(0xB80)


// IOCTL_SCSITARG_GET_PE_ENTRY returns information about a particular PE.
//
// Input Buffer:	PE_WWN - identifies the PE.   
//
// Output buffer:	PE_ENTRY.

#define IOCTL_SCSITARG_GET_PE_ENTRY	        SCSITARG_CTL_CODE(0xB81)


// IOCTL_SCSITARG_SET_PE_PARAM stores information about a particular PE 
// persistently.
//
// Input Buffer:    PE_PARAM - identifies the data to write.  
//
//		Note: PE->Key specifies which PE to update, or the WWN of a new PE.
//
// Output buffer:	None.
//
// Restrictions:	VVols can not be added to a PE via this IOCTL -  use 
//                  IOCTL_SCSITARG_BIND_VVOLS instead.

#define IOCTL_SCSITARG_SET_PE_PARAM	        SCSITARG_CTL_CODE(0xB82)


// IOCTL_SCSITARG_DELETE_PE removes all information about a 
// particular PE.  
//
// Input Buffer:	PE_WWN - identifies the PE to delete.  
//
// Output buffer:	None.
//
// Restrictions:	Can't delete if referenced by any storage group.
//                  Can't delete if contains any VVols.

#define IOCTL_SCSITARG_DELETE_PE	        SCSITARG_CTL_CODE(0xB83)


// IOCTL_SCSITARG_GET_ALL_PE_ENTRIES returns a list of PE entries.  A storage group may 
// optionally be specified as input, in which case the list of PEs is for that SG.  
// If a SG not specified, the command returns the list of all PEs on the array.  
//
// Input Buffer:	SG_KEY - Optional - storage group WWN to get PEs for. 
//
// Output buffer:	PE_ENTRY_LIST.  
//

#define IOCTL_SCSITARG_GET_ALL_PE_ENTRIES		SCSITARG_CTL_CODE(0xB84)


/************************************************************************/
/* VVOL IOCTLs use SCSITARG_CTL_CODE(0xBC0 to 0xBFF).                   */
/************************************************************************/

// IOCTL_SCSITARG_ENUM_VVOLS returns a list of VVol keys.  The VVol key is the WWN. 
// Since VVols are stored as XLUs, the type of the list is an XLU key list. 
//
// Input Buffer:	None.
//
// Output buffer:	XLU_KEY_LIST.
//

#define IOCTL_SCSITARG_ENUM_VVOLS		        SCSITARG_CTL_CODE(0xBC0)


// IOCTL_SCSITARG_GET_ALL_VVOL_ENTRIES returns a list of VVol entries.  
//
// Input Buffer:	None.
//
// Output buffer:	VVOL_ENTRY_LIST.  
//

#define IOCTL_SCSITARG_GET_ALL_VVOL_ENTRIES		SCSITARG_CTL_CODE(0xBC1)


// IOCTL_SCSITARG_GET_ALL_VVOLS_FOR_SG returns a list of the VVol entries for the given
// storage group.  A PE may optionally be specified as input, in which case the list of 
// VVols returned is for that PE in the SG.  
//
// Input Buffer:	VVOL_GET_ALL_INPUT_DATA:
//                      SG_WWN - storage group WWN to get VVols for. 
//                      PE_WWN - Optional - PE in the SG to get VVols for. 
//
// Output buffer:	VVOL_ENTRY_LIST.  
//

#define IOCTL_SCSITARG_GET_ALL_VVOLS_FOR_SG	    SCSITARG_CTL_CODE(0xBC2)


// IOCTL_SCSITARG_BIND_VVOLS will perform bind(s) of the VVol(s) to the given storage
// group.  A list of up to two PE(s) may optionally be specified from which to select 
// the PE for the bind. 
//
// A bind of a VVol does not create the VVol, but rather causes Hostside to expose a  
// VVol that already exists in MLU and below.  The first bind of a VVol will cause the 
// driver stack to be created from Hostside down to MLU.  Subsequent binds of the VVol 
// may attach it to different PE-SG combinations.  
//
// Input Buffer:	VVOL_BIND_PARAM structure:
//                      SG_WWN              - storage group WWN to bind VVol(s) to.  
//                      PE_WWN[2]           - Optional - one or two PE(s) eligible for the bind. 
//                      VVOL_BIND_CONTEXT   - type of bind: Normal or RebindStart. 
//                      VVOL_WWN[MLU_VVOL_MAX_BIND_COUNT] 
//                                           - array of VVol(s) to bind.  
//
// Output buffer:	VVOL_BIND_OUTPUT_LIST - list of VVOL_BIND_OUTPUT structures: 
//                      VVOL_WWN            - VVol WWN.
//                      EMCPAL_STATUS       - Status of the bind
//                      PE_WWN              - WWN of PE that was selected for the bind.
//                      ULONGLONG           - Secondary ID (aka HLU) assigned to the VVol on this PE.
//                  
// Restrictions:	The VVol(s) must have been created in MLU prior to issuing the bind.
//                  RebindStart is not supported at this time.
//

#define IOCTL_SCSITARG_BIND_VVOLS               SCSITARG_CTL_CODE(0xBC3)


// IOCTL_SCSITARG_UNBIND_VVOLS_FROM_HOST will unbind a VVol(s) from the given PE for the given 
// storage group.  
//
// An unbind will detach the VVol from the PE and SG.  The last unbind of the VVol will cause the 
// driver stack to be torn down from Hostside down to MLU. 
//
// Input Buffer:	VVOL_UNBIND_PARAM structure:
//                      SG_WWN              - storage group WWN to unbind VVol(s) from.  
//                      VVOL_UNBIND_CONTEXT - type of unbind: Normal, RebindStart, or RebindEnd 
//                      VVOL_PE_PAIR[MLU_VVOL_MAX_BIND_COUNT] 
//                                          - array of PE-VVol pairs to unbind.  PE WWN (key) and 
//                                            Vvol WWN (key) are specified for each.
//
// Output buffer:	None.
//                  
// Restrictions:	RebindStart and RebindEnd are not supported at this time.
//

#define IOCTL_SCSITARG_UNBIND_VVOLS_FROM_HOST   SCSITARG_CTL_CODE(0xBC4)


// IOCTL_SCSITARG_UNBIND_ALL_VVOLS_FROM_HOST will unbind all the VVol(s) in the given storage group,
// for all the PEs in the SG. 
//
// Input Buffer:	SG_KEY - storage group WWN to unbind VVols from.
//
// Output buffer:	None.
//                  

#define IOCTL_SCSITARG_UNBIND_ALL_VVOLS_FROM_HOST   SCSITARG_CTL_CODE(0xBC5)


// IOCTL_SCSITARG_UNBIND_VVOLS_FROM_ALL_HOSTS will unbind the given VVol(s) from all the storage groups,
// for all the PEs in the SGs. 
//
// Input Buffer:	XLU_KEY_LIST - key list of VVol(s) to unbind.  (Since VVols are stored as XLUs, the 
//                                 type of the list is an XLU key list.) 
//
// Output buffer:	None.
//                  

#define IOCTL_SCSITARG_UNBIND_VVOLS_FROM_ALL_HOSTS   SCSITARG_CTL_CODE(0xBC6)


/**********************************************************/
/* All other IOCTLs use SCSITARG_CTL_CODE(0xE00 to 0xEFF).*/
/**********************************************************/

// Commit an NDU operation.
//
// Input Buffer:	None.
//
// Output buffer:	None.

#define IOCTL_SCSITARG_NDU_COMMIT				SCSITARG_CTL_CODE(0xE00)

// Admin issues IOCTL_SCSITARG_IPADDRESS_CHANGE_NOTIFICATION to notify 
// ScsiTarg about Dynamic Change in IP Address(Now IPv6 - Global Unicast).
//
// Input Buffer:	None.
//
// Output buffer:	None.

#define IOCTL_SCSITARG_IPADDRESS_CHANGE_NOTIFICATION	SCSITARG_CTL_CODE(0xE01)
/****************************************************************************************************/
/* IOCTLs using SCSITARG_CTL_CODE(0xF00 to 0xFFF) are private, and defined in a private header file.*/
/****************************************************************************************************/

#endif /* _SCSITARG_H_ */
