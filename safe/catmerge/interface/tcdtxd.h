/*******************************************************************************
 * Copyright (C) EMC Corporation, 1997 - 2010
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * tcdtxd.h
 *
 * This header file is shared between the K10 Host Side drivers, especially 
 * the Target Class Driver (TCD) and the Target Disk Driver (TDD).
 *
 * Notes:
 *
 *	Throughout this file, you will see notation such as CrXw, CrwXw, CXrw, etc.
 *	'C' indicates the Target 'C'lass Driver (TCD).
 *	'X' indicates a random type of Target Driver (TxD) that "links" to the TCD.
 *	'r' following a 'C' or an 'X' means read-only for the 'C' or 'X' driver.
 *	'w' means primarily write-only.
 *	'rw' means read-write.
 *	''	(no letter) means neither read nor write.  
 *
 *	As an example, CXrw means that the Target Class Driver does not use 
 *	whatever it is notating, but the TxD both reads from and writes to it.
 *
 ******************************************************************************/

#ifndef	_TCDTXD_H_
#define	_TCDTXD_H_

#include "scsitarg.h"
#include "csx_ext.h"
#include "tcdshare.h"
#include "SinglyLinkedList.h"
#include "EmcPAL_Inline.h"


//
// Definitions that everyone may care about.
//

#define DEFAULT_MAX_OUTSTANDING_OPS_PER_SPINDLE 8

#ifndef _33RD_INITIATOR_WORKAROUND
#define _33RD_INITIATOR_WORKAROUND	1
#endif

//*******************************************************************
//*
//* QUERY_PEER_QUIESCE_SUPPORT_LEVEL exists to define the minimum level 
//* required to send the I_SC_GET_QUIESCED_XLUS message to the peer.
//*
#define	QUERY_PEER_QUIESCE_SUPPORT_LEVEL			2
//*
//* STATS_REQ_TCD_SUPPORT_LEVEL exists to define the minimum level 
//* required to send the following messages to the peer:
//*
//* TCD_MPS_GET_XLU_STATS and TCD_MPS_GET_SYSTEM_STATS
//*
#define	STATS_REQ_TCD_SUPPORT_LEVEL					3
//*
//* TCD_SET_XLU_HR_SUPPORT_LEVEL exists to define the minimum level required
//* to send the HOST_SCSITARG_SET_XLU_PARAM Intention (set XLU param host request) message.
//*
#define	TCD_SET_XLU_HR_SUPPORT_LEVEL				4
//*
//* TCD_AUTHENTICATION_SUPPORT_LEVEL exists to define the minimum level required
//* to send authentication update messages.
//*
#define	TCD_AUTHENTICATION_SUPPORT_LEVEL			5
//*
//* TCD_MANAGEMENT_IP_ADDRESS_SUPPORT_LEVEL exist to define the minimum level
//* required to get peer's IP address using Inquiry VPD page 0x85
//*
#define	TCD_MANAGEMENT_IP_ADDRESS_SUPPORT_LEVEL		6
//*
//* TCD_SET_PORT_WITHOUT_RESET_SUPPORT_LEVEL exists to define the minimum level 
//* required to send the IOCTL_SCSITARG_SET_PORT_WITHOUT_RESET Intention.
//*
#define	TCD_SET_PORT_WITHOUT_RESET_SUPPORT_LEVEL	7
//*
//* TCD_ALUA_SUPPORT_LEVEL exists to define the minimum level 
//* required to support ALUA failover mode.
//*
#define	TCD_ALUA_SUPPORT_LEVEL						8
//*
//* TCD_SEND_QUIESCE_ERRORED_SUPPORT_LEVEL exists to define the minimum level 
//* required to send the TCD_MPS_SEND_QUIESCE_ERRORED_STATUS message.
//*
#define	TCD_SEND_QUIESCE_ERRORED_SUPPORT_LEVEL		9
//*
//* TCD_SCSI2_RESERVATIONS_FOR_ALUA_SUPPORT_LEVEL exists to define the minimum level 
//* required where SCSI2 Reserve/Release is supported for ALUA hosts.
//*
#define	TCD_SCSI2_RESERVATIONS_FOR_ALUA_SUPPORT_LEVEL	10
//*
//* TCD_VPORT_SUPPORT_LEVEL exists to define the minimum level where Scsitarg supports
//* Virtual Ports.  That is, multiple IP Addresses per port, and authentication specified
//* at the Portal Group (versus Host Port)
//*
#define	TCD_VPORT_SUPPORT_LEVEL						11
//*
//* TCD_GREATER_THAN_256_LUNs_SUPPORT_LEVEL exists to define the minimum level 
//* required to support >256 LUNs per user defined storage group.
//*
#define	TCD_GREATER_THAN_256_LUNs_SUPPORT_LEVEL		11
//*
//* TCD_CAS_SUPPORT_LEVEL exists to define a minimum level required
//* to support the compare and swap command.
//*
#define TCD_CAS_SUPPORT_LEVEL						12
//*
//* TCD_VAAI_SUPPORT_LEVEL exists to define a minimum level required
//* to support the VPD B0, Unmap, CAS(89h) commands.
//*
#define TCD_VAAI_SUPPORT_LEVEL						13
//*
//* This support level is not used, see the next support level.
//*
#define TCD_AUTO_ASSOCIATION_BIT_SUPPORT_LEVEL		14    //GJF Changed from 13 to 14 to avoid conflict with VAAI_SUPPORT_LEVEL
//*
//* TCD_AUTO_ASSOCIATION_BIT_SUPPORT_LEVEL exists to define a minimum level required
//* to support Celerra auto-association.
//*
#define TCD_AUTO_ASSOCIATION_BIT_SUPPORT_LEVEL_REV1	15    
//*
//* TCD_OFFLOAD_DATA_XFER_SUPPORT exists to define a minimum level required
//* to support ODX commands Populate Token, Write Using Token, and Receive ROD Token Info.
//*
#define TCD_OFFLOAD_DATA_XFER_SUPPORT_LEVEL			16    
//*
//* TCD_VVOL_BLOCK_SUPPORT_LEVEL exists to define a minimum level required
//* to support Block VVOL.
//*
#define TCD_VVOL_BLOCK_SUPPORT_LEVEL				17    
//*
//*
//* Support level for initiator scalability across NDU
//*
#define	TCD_FLEET_INITIATOR_SCALABILITY_SUPPORT_LEVEL	TCD_GREATER_THAN_256_LUNs_SUPPORT_LEVEL
//*
//* Support level for seperate stats logging bits across NDU
//*
#define TCD_SEPERATE_A_B_STATS_SUPPORT_LEVEL		TCD_CAS_SUPPORT_LEVEL

//*
#define MIN_TCD_SUPPORT_LEVEL						TCD_SET_XLU_HR_SUPPORT_LEVEL
//*
#define R16_TCD_SUPPORT_LEVEL						TCD_AUTHENTICATION_SUPPORT_LEVEL
//*
#define	CURRENT_TCD_SUPPORT_LEVEL					TCD_VVOL_BLOCK_SUPPORT_LEVEL
//*
//*******************************************************************


// -----------------------------------------------------------------------------
// A set of IOCTL codes which are relevant to the TCD<>TxD interface are defined
// in scsitarg.h.  They are defined there so all IOCTLs for this device are kept
// together.  The subset  defined for the TCD<>TxD interface are:
//
//#define IOCTL_TCD_TXD_LINK								\
//	CTL_CODE(FILE_DEVICE_SCSITARG, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
//
//#define IOCTL_TCD_TXD_ACTIVATE									\
//	CTL_CODE(FILE_DEVICE_SCSITARG, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
// -----------------------------------------------------------------------------


//	A Target Driver (TxD) registers itself with the Target Class Driver (TCD)
//	by sending the IOCTL_TCD_TXD_LINK IOCTL to "\Device\ScsiTarg0".  A
//	TCD_TXD_LINK structure is passed via this IOCTL.
//
//	Prior to sending this structure, TxD fills in its Major Device
//	Type (See TARGET_MAJOR enum below) and a pointer to its interface table.
//
//	The Target Class Driver (TCD) returns the same TCD_TXD_LINK structure after
//	both assigning a Minor number to the registering driver and filling in the
//	address of its own interface table.
//
//	TxdInterface is a PVOID because the actual structure is Major-Type-
//	Dependent.  TcdInterface is always the same.

typedef struct _TCD_INTERFACE *PTCD_INTERFACE;	// Forward decl. required here.

typedef struct _TCD_TXD_LINK {

	ULONG				Major;			// CrXw
	PVOID				TxdInterface;	// CrXw
	ULONG				Minor;			// CwXr
	PTCD_INTERFACE		TcdInterface;	// CwXr
	ULONG				IRP_StackSize;

} TCD_TXD_LINK, *PTCD_TXD_LINK;

//	A Target Driver (TxD) has itself activated by the Target Class Driver (TCD)
//	by sending the IOCTL_TCD_TXD_ACTIVATE IOCTL to "\Device\ScsiTarg0".  A
//	TCD_TXD_ACTIVATE structure is passed via this IOCTL.
//
//	Prior to sending this structure, TxD fills in its Major Device 
//	Type (See TARGET_MAJOR enum below) and the Minor device number that was
//	returned (when IOCTL_TCD_TXD_LINK was previously called).

typedef struct _TCD_TXD_ACTIVATE {

	ULONG				Major;			// CrXw
	ULONG				Minor;			// CrXw

} TCD_TXD_ACTIVATE, *PTCD_TXD_ACTIVATE;

// A PXLU points to an XLU.  Since an XLU is a TxD-specific structure that 
// uniquely identifies a Logical Unit (such as a Disk Logical Unit (DLU)),
// we will define it as a PVOID here.  Note the an XLU must have an XLU_HDR
// as its first field (and TCD assumes it does).

typedef PVOID			PXLU;

// SCSIMESS_ACA_QUEUE_TAG should have the same value as FC_ACA_QUEUE_TAG

#define SCSIMESS_ACA_QUEUE_TAG		0x2f

// Only TCD understands the VLU_ALLOC structure, but the XLU header has a list that we need the type for.

typedef struct _VLU_ALLOC  VLU_ALLOC, *PVLU_ALLOC;

SLListDefineListType(VLU_ALLOC, ListOfVLU_ALLOC);


//	This is the header of every XLU structure whose pointer must be put into
//	the VLU structure (Vlu->Xlu) by a TxD when its ConnectXlu function is
//	called if and only if that function returns TRUE to the TCD.  It returns
//	TRUE if the TxD intends to handle all requests with the same BITL (or
//	equivalent).
//
//	An accepting TxD must also fill in the header for a one-time sanity check
//	by the TCD.  Fill in the header by simply passing the InitializeXluHdr()
//	macro a pointer to your XLU structure, making sure that that structure
//	has an XLU_HDR structure declared as its first element.
//
//	NOTE:	Do NOT reinitialize this structure when subsequent VLUs become
//			assigned to the same XLU.  Do not touch the header once it's
//			initialized;  It's the property of the TCD.
//
//	Just to keep them in sync, the macro that the TCD uses to verify the
//	existence of a proper header is also included here.

typedef struct _XLU_HDR XLU_HDR, *PXLU_HDR;


struct _XLU_HDR {

	// The ConfigMemorySemaphore must be held when manipulating the 
	// XLU list or the VluList.
	PXLU_HDR			NextXlu;	     // Linking all XLUs for all ScsiTargs.

	// WARNING: The CCB_QUEUE lock is held assuming its global to prevent
	// additions to this list while TcdSetUnitAttention is running, in
	// addition to the ConfigMemorySemaphore.
	ListOfVLU_ALLOC		VluList;		// List head linking all VLUs...
										// ...connected to this XLU.
	ULONG				HeaderLength;	// Must be sizeof(XLU_HDR).
	XLU_WWN				Wwn;			// From XLU->Params.Key.
	UCHAR				Signature[4];	// Must be {'T', 'C', 'D', ' '}.
	ULONG				MaxOutstandingOps;
	ULONG				RaidGroup;
	ULONG				Flu;
	LONG				BroadcastAbortCount; // Outstanding aborts from broadcasted async event.

	UCHAR				QueueError : 1;	// Copied from Mode Page 0x0A
	UCHAR				WCE : 1;		// Write Cache Enable bit copied from Mode Page 0x8.
	UCHAR				RCD : 1;		// Read Cache Disable bit copied from Mode Page 0x8.
	UCHAR				AsyncEventBroadcastInProgress : 1; 
};

SLListDefineListType(XLU_HDR, ListOfXLU_HDR);
SLListDefineInlineListTypeFunctions(XLU_HDR, ListOfXLU_HDR, NextXlu);

#define InitializeXluHdr(Xlu, DefaultMaxOps)								\
	{																		\
		PXLU_HDR			Hdr = (PXLU_HDR)(Xlu);							\
																			\
		EmcpalZeroMemory(Hdr, (ULONG)sizeof(*Hdr));							\
		Hdr->HeaderLength = sizeof(XLU_HDR);								\
		strncpy((char*)&Hdr->Signature[0], "TCD ", sizeof(Hdr->Signature));		\
		Hdr->MaxOutstandingOps = (DefaultMaxOps);							\
		Hdr->BroadcastAbortCount = 0;										\
		Hdr->WCE = 0;														\
		Hdr->RCD = 0;														\
		Hdr->AsyncEventBroadcastInProgress = 0;								\
		/* by zero:ListOfVLU_ALLOCInitialize(&Hdr->VluList);*/   			\
	} 

//	Per LUN Information structure.

typedef struct _LUN_INFO {

	PUCHAR					ModeSenseBlock;
	ULONG					ModeSenseBlockSize;
	LONGLONG				DiskStart;
	LONGLONG				DiskEnd;
	ULONG					DiskSz;

} LUN_INFO, *PLUN_INFO;

//-------------------------------------------------
//                SCSI Definitions
//-------------------------------------------------

//
// Sense codes from SCSI.H (included here for reference)
//

//#define SCSI_SENSE_NO_SENSE         0x00
//#define SCSI_SENSE_RECOVERED_ERROR  0x01
//#define SCSI_SENSE_NOT_READY        0x02
//#define SCSI_SENSE_MEDIUM_ERROR     0x03
//#define SCSI_SENSE_HARDWARE_ERROR   0x04
//#define SCSI_SENSE_ILLEGAL_REQUEST  0x05
//#define SCSI_SENSE_UNIT_ATTENTION   0x06
//#define SCSI_SENSE_DATA_PROTECT     0x07
//#define SCSI_SENSE_BLANK_CHECK      0x08
//#define SCSI_SENSE_UNIQUE           0x09
//#define SCSI_SENSE_COPY_ABORTED     0x0A
//#define SCSI_SENSE_ABORTED_COMMAND  0x0B
//#define SCSI_SENSE_EQUAL            0x0C
//#define SCSI_SENSE_VOL_OVERFLOW     0x0D
//#define SCSI_SENSE_MISCOMPARE       0x0E
//#define SCSI_SENSE_RESERVED         0x0F

//
// Additional Sense Codes
// These includes both the codes from SCSI.H that we use (included for reference)
// and ones we define ourselves.
//

//#define SCSI_ADSENSE_NO_SENSE								0x00
//#define SCSI_ADSENSE_LUN_NOT_READY						0x04
#define SCSI_ADSENSE_NOT_RESPONDING_TO_SELECTION			0x05
#define SCSI_ADSENSE_COPY_DATA_ERROR						0x0D
#define SCSI_ADSENSE_UNRECOVERED_READ_ERROR					0x11
#define SCSI_ADSENSE_PARAMETER_LIST_LENGTH_ERROR			0x1A
#define SCSI_ADSENSE_SYNCHRONOUS_DATA_TRANSFER_ERROR		0x1B
#define SCSI_ADSENSE_MISCOMPARE_DURING_VERIFY_OPERATION		0x1D
//#define SCSI_ADSENSE_ILLEGAL_COMMAND						0x20
//#define SCSI_ADSENSE_ILLEGAL_BLOCK						0x21
#define SCSI_ADSENSE_INVALID_TOKEN_OPERATION				0x23
//#define SCSI_ADSENSE_INVALID_CDB							0x24
//#define SCSI_ADSENSE_INVALID_LUN							0x25
#define SCSI_ADSENSE_INVALID_FIELD_IN_PARAMETER_LIST		0x26
//#define SCSI_ADSENSE_WRITE_PROTECT						0x27
//#define SCSI_ADSENSE_BUS_RESET							0x29
#define SCSI_ADSENSE_PARAMETERS_CHANGED						0x2A
#define SCSI_ADSENSE_COMMAND_SEQUENCE_ERROR					0x2C
#define SCSI_ADSENSE_CMDS_CLEARED_OTHER_INITIATOR			0x2F
#define SCSI_ADSENSE_ENCLOSURE_SERVICES_FAILURE				0x35
#define SCSI_ADSENSE_THIN_PROVISIONING_SOFT_THRESHOLD_REACHED 0x38
#define SCSI_ADSENSE_TARGET_CONDITIONS_CHANGED				0x3F
#define SCSI_ADSENSE_INTERNAL_TARGET_FAILURE				0x44
#define SCSI_ADSENSE_INVALID_MESSAGE_ERROR					0x49
#define SCSI_ADSENSE_OVERLAPPED_COMMANDS_ATTEMPTED			0x4E
#define SCSI_ADSENSE_SYSTEM_RESOURCE_FAILURE				0x55
#define SCSI_ADSENSE_SET_TARGET_PORT_GROUPS_COMMAND_FAILURE	0x67
#define SCSI_ADSENSE_LOGICAL_UNIT_NOT_CONFIGURED			0x68
#define SCSI_ADSENSE_LOGICAL_UNIT_ACCESS_NOT_AUTHORIZED		0x74
#define SCSI_ADSENSE_FORCE_RESERVE							0x80
#define SCSI_ADSENSE_GROUP_RESERVE							0x80

//
// Additional Sense Code Qualifiers
// These include both the codes from SCSI.H that we use (included for reference)
// and ones we define ourselves.
//

// SCSI_ADSENSE_NO_SENSE (0x00) aualifiers

#define SCSI_SENSEQ_NO_SENSE							0x00
#define SCSI_SENSEQ_DRIVER_STACK_COMMUNICATION_ERROR	0x02
#define SCSI_SENSEQ_IO_PROCESS_TERMINATED				0x06
#define SCSI_SENSEQ_EXOP_IN_PROGRESS					0x16

// SCSI_ADSENSE_LUN_NOT_READY (0x04) qualifiers

//#define SCSI_SENSEQ_CAUSE_NOT_REPORTABLE			0x00
//#define SCSI_SENSEQ_BECOMING_READY				0x01
#define SCSI_SENSEQ_INITIALIZING_COMMAND_REQUIRED	0x02
//#define SCSI_SENSEQ_MANUAL_INTERVENTION_REQUIRED	0x03
//#define SCSI_SENSEQ_FORMAT_IN_PROGRESS			0x04
//#define SCSI_SENSEQ_REBUILD_IN_PROGRESS			0x05
//#define SCSI_SENSEQ_RECALCULATION_IN_PROGRESS		0x06
//#define SCSI_SENSEQ_OPERATION_IN_PROGRESS			0x07
//#define SCSI_SENSEQ_LONG_WRITE_IN_PROGRESS		0x08
#define SCSI_SENSEQ_TARGET_PORT_UNAVAILABLE			0x0C

// SCSI_ADSENSE_NOT_RESPONDING_TO_SELECTION (0x05) qualifiers

#define SCSI_SENSEQ_NOT_RESPONDING_TO_SELECTION		0x00

// SCSI_ADSENSE_COPY_DATA_ERROR (0x0D) qualifiers

#define SCSI_SENSEQ_TARGET_INACCESSABLE				0x02
#define SCSI_SENSEQ_DATA_UNDERRUN					0x04
#define SCSI_SENSEQ_DATA_OVERRUN					0x05

// SCSI_ADSENSE_UNRECOVERED_READ_ERROR (0x11) qualifiers

#define SCSI_SENSEQ_UNRECOVERED_READ_ERROR			0x00
#define SCSI_SENSEQ_AUTO_REALLOCATE_FAILED			0x04

// SCSI_ADSENSE_PARAMETER_LIST_LENGTH_ERROR (0x1A) qualifiers

#define SCSI_SENSEQ_PARAMETER_LIST_LENGTH_ERROR		0x00

// SCSI_ADSENSE_SYNCHRONOUS_DATA_TRANSFER_ERROR (0x1B) qualifiers

#define SCSI_SENSEQ_SYNCHRONOUS_DATA_TRANSFER_ERROR	0x00

// SCSI_ADSENSE_MISCOMPARE_DURING_VERIFY_OPERATION (0x1D) qualifiers

#define SCSI_SENSEQ_MISCOMPARE_DURING_VERIFY_OPERATION		0x00

// SCSI_ADSENSE_ILLEGAL_COMMAND (0x20) qualifiers

#define SCSI_SENSEQ_ILLEGAL_COMMAND					0x00
#define SCSI_SENSEQ_INVALID_LU_IDENTIFIER			0x09

// SCSI_ADSENSE_ILLEGAL_BLOCK (0x21) qualifiers

#define SCSI_SENSEQ_ILLEGAL_BLOCK					0x00
#define SCSI_SENSEQ_MEDIA_ERROR_INJECTION			0x12

// SCSI_ADSENSE_INVALID_TOKEN_OPERATION (0x23) qualifiers

#define SCSI_SENSEQ_CAUSE_NOT_REPORTABLE			0x00
#define SCSI_SENSEQ_TOKEN_UNKNOWN					0x04
#define SCSI_SENSEQ_INVALID_TOKEN_LENGTH			0x0A

// SCSI_ADSENSE_INVALID_CDB (0x24) qualifiers

#define SCSI_SENSEQ_INVALID_CDB						0x00

// SCSI_ADSENSE_INVALID_LUN (0x25) qualifiers

#define SCSI_SENSEQ_LOGICAL_UNIT_NOT_SUPPORTED		0x00
#define SCSI_SENSEQ_UNACTIVATED_SNAPCOPY_UNIT		0x01

// SCSI_ADSENSE_INVALID_FIELD_IN_PARAMETER_LIST (0x26) qualifiers

#define SCSI_SENSEQ_INVALID_FIELD_IN_PARAMETER_LIST	0x00
#define SCSI_SENSEQ_PARAMETER_VALUE_INVALID			0x02
#define SCSI_SENSEQ_INVALID_RELEASE_OF_PERSISTENT_RESERVATION	0x04
#define SCSI_SENSEQ_TOO_MANY_TARGET_DESCRIPTORS		0x06
#define SCSI_SENSEQ_UNSUPPORTED_TARGET_DESCRIPTOR	0x07
#define SCSI_SENSEQ_TOO_MANY_SEGMENT_DESCRIPTORS	0x08
#define SCSI_SENSEQ_UNSUPPORTED_SEGMENT_DESCRIPTOR	0x09

// SCSI_ADSENSE_WRITE_PROTECT (0x27) qualifiers

#define SCSI_SENSEQ_WRITE_PROTECT					0x00
#define SCSI_SENSEQ_SPACE_ALLOCATION_FAILED_WRITE_PROTECT				0x07

// SCSI_ADSENSE_BUS_RESET (0x29) qualifiers

#define SCSI_SENSEQ_BUS_RESET						0x00

// SCSI_ADSENSE_PARAMETERS_CHANGED (0x2A) qualifiers

#define SCSI_SENSEQ_MODE_PARAMETERS_CHANGED			0x01
#define SCSI_SENSEQ_RESERVATIONS_RELEASED			0x04
#define SCSI_SENSEQ_RESERVATIONS_PREEMPTED			0x05
#define SCSI_SENSEQ_INITIATOR_PARAMETERS_CHANGED	0x80
#define SCSI_SENSEQ_LUN_CAPACITY_CHANGED			0x09
//#define SCSI_SENSEQ_STORAGE_GROUP_PARAMETERS_CHANGED	0x81
#define SCSI_SENSEQ_FLU_NAME_CHANGED				0x82
#define SCSI_SENSEQ_ARRAY_NAME_CHANGED				0x83

// SCSI_ADSENSE_COMMAND_SEQUENCE_ERROR (0x2C) qualifiers

#define SCSI_SENSEQ_COMMAND_SEQUENCE_ERROR			0x00

// SCSI_ADSENSE_CMDS_CLEARED_OTHER_INITIATOR (0x2F) qualifiers

#define SCSI_SENSEQ_CMDS_CLEARED_OTHER_INITIATOR	0x00

// SCSI_ADSENSE_ENCLOSURE_SERVICES_FAILURE (0x35) qualifiers

#define SCSI_SENSEQ_UNSUPPORTED_SEND_DIAGNOSTIC_PAGE	0x01

//	SCSI_ADSENSE_THIN_PROVISIONING_SOFT_THRESHOLD_REACHED (0x38)

#define SCSI_ADSENSEQ_THIN_PROVISIONING_SOFT_THRESHOLD_REACHED (0x07)

// SCSI_ADSENSE_TARGET_CONDITIONS_CHANGED (0x3f) qualifiers

#define SCSI_SENSEQ_VOLUME_SET_CREATED_OR_MODIFIED	0x0A
#define SCSI_SENSEQ_REPORT_LUNS_DATA_CHANGED		0x0E

// SCSI_ADSENSE_INTERNAL_TARGET_FAILURE (0x44) qualifiers

#define SCSI_SENSEQ_INTERNAL_TARGET_FAILURE			0x00

// SCSI_ADSENSE_INVALID_MESSAGE_ERROR (0x49) qualifiers

#define SCSI_SENSEQ_INVALID_MESSAGE_ERROR			0x00

// SCSI_ADSENSE_OVERLAPPED_COMMANDS_ATTEMPTED (0x4E) qualifiers

#define SCSI_SENSEQ_OVERLAPPED_COMMANDS_ATTEMPTED	0x00

// SCSI_ADSENSE_SYSTEM_RESOURCE_FAILURE (0x55) qualifiers

#define SCSI_SENSEQ_INSUFFICIENT_RESOURCES				0x03
#define SCSI_SENSEQ_INSUFFICIENT_REGISTRATION_RESOURCES	0x04

// SCSI_ADSENSE_SET_TARGET_PORT_GROUPS_COMMAND_FAILURE (0x67) qualifiers

#define SCSI_SENSEQ_SERVICE_ACTION_FAILED			0x0A

// SCSI_ADSENSE_LOGICAL_UNIT_NOT_CONFIGURED (0x68) qualifiers

#define SCSI_SENSEQ_LOGICAL_UNIT_NOT_CONFIGURED		0x00
#define SCSI_SENSEQ_SUBSIDIARY_LOGICAL_UNIT_NOT_CONFIGURED		0x01

// SCSI_ADSENSE_LOGICAL_UNIT_ACCESS_NOT_AUTHORIZED (0x74) qualifiers

#define SCSI_SENSEQ_LOGICAL_UNIT_ACCESS_NOT_AUTHORIZED	0x71

// SCSI_ADSENSE_FORCE_RESERVE (0x80) and SCSI_ADSENSE_GROUP_RESERVE (0x80) qualifiers

#define SCSI_SENSEQ_RESERVATION_PREEMPTED_BY_FORCE_RESERVE	0x01
#define SCSI_SENSEQ_INITIATOR_NOT_PART_OF_GROUP		0x18
#define SCSI_SENSEQ_MAXIMUM_NUMBER_OF_GROUPS_IN_USE	0x19
#define SCSI_SENSEQ_NO_GROUP_REGISTRATION_EXISTS	0x20
#define SCSI_SENSEQ_SPECIFIED_KEY_NOT_IN_USE		0x21
#define SCSI_SENSEQ_SERVICE_ACTION_KEY_ZERO			0x22

#define SCSI_SENSEQ_ASYMMETRIC_STATE_CHANGED		0x06


//
// Persistent Reservation Definitions.
// These include both the codes from SCSI.H that we use (included for reference)
// and ones we define ourselves.
//

// Persistent Reserve In Service Actions

//#define RESERVATION_ACTION_READ_KEYS						0x00
//#define RESERVATION_ACTION_READ_RESERVATIONS				0x01

// Persistent/Group Reserve Out Service Actions

//#define RESERVATION_ACTION_REGISTER						0x00
//#define RESERVATION_ACTION_RESERVE						0x01
//#define RESERVATION_ACTION_RELEASE						0x02
//#define RESERVATION_ACTION_CLEAR							0x03
#define RESERVATION_ACTION_PENETRATE_RESERVATION			0x03
//#define RESERVATION_ACTION_PREEMPT						0x04
//#define RESERVATION_ACTION_PREEMPT_ABORT					0x05
//#define RESERVATION_ACTION_REGISTER_IGNORE_EXISTING		0x06
#define RESERVATION_ACTION_KILL_GROUP_ID					0x10
#define RESERVATION_ACTION_REGISTER_AND_RESERVE				0x1F
#define RESERVATION_ACTION_DEREGISTER_GROUP					0x20
#define RESERVATION_ACTION_DEREGSITER_GROUP_IGNORE_EXISTING	0x26
#define RESERVATION_ACTION_REGISTER_IF_KEY_EXISTS			0x46
#define RESERVATION_INTERNAL_ACTION_CLEAR_PERSISTENT		0xFE
#define RESERVATION_INTERNAL_ACTION_CLEAR_GROUP				0xFF

// Persistent/Group Reservation Scopes

//#define RESERVATION_SCOPE_LU								0x00

// Persistent/Group Reservation Type

#define RESERVATION_TYPE_READ_SHARED						0x00
//#define RESERVATION_TYPE_WRITE_EXCLUSIVE					0x01
#define RESERVATION_TYPE_READ_EXCLUSIVE						0x02
//#define RESERVATION_TYPE_EXCLUSIVE						0x03
#define RESERVATION_TYPE_SHARED_ACCESS						0x04
//#define RESERVATION_TYPE_WRITE_EXCLUSIVE_REGISTRANTS		0x05
//#define RESERVATION_TYPE_EXCLUSIVE_REGISTRANTS			0x06

// This is a 'special' Group Reservation Key.
// If specified by the host, it allows that host to bypass ALL reservation checking.
// This is the same key that Symm uses to perform this function.
// (It's not SCSI, but this is the best place to place the definition).

#define RESERVATION_PENETRATION_KEY							0xFEA51B1E



static __inline LONG EncodeUnitAttention(UCHAR addsense, UCHAR qual, USHORT ts) {
	return addsense << 24 | qual << 16 | ts ;
}
static __inline VOID DecodeUnitAttention(LONG ua, UCHAR * addsense, UCHAR * qual) {
	*addsense  = (UCHAR)(ua >> 24);
	*qual  = (UCHAR)(ua >> 16);
}


#define XLU_PRIVATE_SIZE	40

//	This is a per-VLU structure that the TCD keeps, both for permanent
//	and temporary VLUs.
//

typedef struct _VLU {

	PXLU_HDR	Xlu;							// CrXrw (Starts with XLU_HDR).
	UCHAR		XluPrivate[XLU_PRIVATE_SIZE];	// CXrw
	BOOLEAN		WriteProtect;					// CwXr
    BOOLEAN     ReportTargetPortGroupsReceived; // since login.
	ULONG		UnitAttn;						// Most Significant Unit Attention.	

} VLU, *PVLU;

//	TARGET_MAJOR is an enumeration of MAJOR types of target drivers.
//
//	NOTE:	Never delete, renumber, or reorder this list.  Add to the list only
//			by placing new entries before the TcdMajor entry.

typedef enum _TARGET_MAJOR {

	TgdMajor = -1,			// Target Generic Driver
	TddMajor,				// Target Disk Driver
	TcmiMajor,				// Communication Manager Interface (CMI)
							//
							// Add new Majors above.  Leave what's below alone.
							//
	TcdMajor,				// Target Class Driver (NoDev device handling)
	TxdEnd,					// List Terminator - Must be next to last.
	TXD_MAJORS				// Number of valid Major Target Types (last).

} TARGET_MAJOR, *PTARGET_MAJOR;

//	For the XluContext in the CCB_HEADER structure that's defined in tcdshare.h,
//	define the size of a block of memory to be used by the TxDs to help control
//	and track Accept CCBs and their associated Continue CCBs.
//
//	XLU_CONTEXT_SIZE bytes are allocated for each Accept CCB.  It is
//	for whatever use the TxDs would like to make of it.

#ifdef ALAMOSA_WINDOWS_ENV
#define XLU_CONTEXT_SIZE	272
#else
#define XLU_CONTEXT_SIZE	336
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - this size was not covering TDD_WORKORDER and chaos was the result */

typedef PVOID PXLU_CONTEXT;

//	Though the MiniPort Driver only sees the CCB structure, the rest of us
//	CAN see the XLU_CCB structure.  It's not used all the time, so a macro
//	is provided to convert a PCCB to a PXLU_CCB (XLU_CCB_ADDR).

typedef struct _XLU_CCB  XLU_CCB, *PXLU_CCB;

struct _XLU_CCB {

	PXLU_CCB			NextXluCcb;			// TCD/XLU-use only.
	PVOID				XluContext;			// Shared by Accept + its Continues.
	PVOID				XluCcbPrivate;		// Whatever XLU wants to do with it.
	CCB					Ccb;
};

SLListDefineListType(XLU_CCB, ListOfXLU_CCB);
SLListDefineInlineListTypeFunctions(XLU_CCB, ListOfXLU_CCB, NextXluCcb);

typedef struct _XLU_ACCEPT_CCB {

	PCCB                AssocContinue;        // TCD/XLU-use only.
	ULONGLONG           Reserved;             // 
	PXLU_CONTEXT        XluContext;           // Shared by Accept + its Continues.
	INITIATOR_TYPE      InitiatorType;        //
	ULONG               InitiatorOptions;     // Used to make types available per operation
	CPD_SGL_SUPPORT     SGLSupport;           // Miniport SGL support and preference.
	BOOLEAN				VirtualAddressRequired; // Port only supports virtual address SGLs.
	USHORT              PoolNo;               // Resource Pool No
	UINT_32             PreferredCPU;         // CPU TDD workorder was started with.
	ACCEPT_CCB          Ccb;                  

} XLU_ACCEPT_CCB, *PXLU_ACCEPT_CCB;


typedef struct _DISASSOCIATED_BUFFER_ALLOC {

	PVOID	Buffer;
	ULONG	Size;
	ULONG	Pool;
	PVOID	Handle;

} DISASSOCIATED_BUFFER_ALLOC, * PDISASSOCIATED_BUFFER_ALLOC;


#define MAX_EXT_COPIES	255

#define	DM_GLOBAL_SIZE_FOR_TDD	512
#define MAX_TDD_DMR_SET_SIZE		8

typedef struct _DM_OBJECT {

	BOOLEAN						IsInDM;
	ULONG						XferSizeInBlocks;
	ULONG						BlockOffset;
	PVOID						DmRequest;
	DISASSOCIATED_BUFFER_ALLOC	DBufferAlloc;

} DM_OBJECT, *PDM_OBJECT;

typedef struct _DM_RESOURCES {

	USHORT			ReferenceCount;
	USHORT			DmrCount;
	USHORT			ActiveDmrs;
	EMCPAL_SPINLOCK	DmrLock;
	DM_OBJECT		DmReqs[MAX_TDD_DMR_SET_SIZE];

} DM_RESOURCES, *PDM_RESOURCES;

typedef struct _EXT_COPY_RSENSE_DATA {

	ULONG		BytesTransferred;
	UCHAR		FinalScsiCommandStatus;
	BOOLEAN		ResidueValid;
	UCHAR		SenseKey;
	ULONG		Residue;
	USHORT		Segment;
	UCHAR		AddSenseCode;
	UCHAR		ASCQualifier;
	BOOLEAN		SpecificFieldsValid;
	BOOLEAN		FieldPtrFromSegStart;
	BOOLEAN		BitPtrValid;
	UCHAR		BitNumber;
	USHORT		FieldPtr;

} EXT_COPY_RSENSE_DATA, *PEXT_COPY_RSENSE_DATA;

typedef struct _EXT_COPY_PARAMS {

	TI_OBJECT_ID			LuWwn[2];
	UCHAR					SourceIndex:2;
	UCHAR					DestinationIndex:2;
	UCHAR					MadeDestinationEqualSource:1;
	UCHAR					PrimaryIsDestinationOnly:1;
	UCHAR					Resv:2;
	USHORT					NumberOfBlocks;
	ULONGLONG				SourceLBA;
	ULONGLONG				DestinationLBA;

} EXT_COPY_PARAMS, * PEXT_COPY_PARAMS;

#define	TXD_ROD_TOKEN_SIZE	512

typedef struct _RCV_ROD_TOKEN_INFO_POPULATE_TOKEN {

	UCHAR	AvailableData[4];
	UCHAR	ResponseToServiceAction;
	UCHAR	CopyOperationStatus;
	UCHAR	OperationCounter[2];
	UCHAR	EstimatedStatusUpdateDelay[4];
	UCHAR	ExtendedCopyCompletionStatus;
	UCHAR	SenseDataFieldLength;
	UCHAR	SenseDataLength;
	UCHAR	TransferCountUnits;
	UCHAR	TransferCount[8];
	UCHAR	SegmentsProcessed[2];
	UCHAR	Resv1[6];
	UCHAR	ErrorCode;
	UCHAR	Resv2;
	UCHAR	SenseKey;
	UCHAR	Residue[4];
	UCHAR	AdditionalSenseLength;
	UCHAR	Resv3[2];
	UCHAR	SegmentNumberOfError[2];
	UCHAR	AddSenseCode;
	UCHAR	ASCQualifier;
	UCHAR	Resv4;
	UCHAR	BitPointer;
	UCHAR	FieldPointer[2];
	UCHAR	Resv5[2];
	UCHAR	RodTokenDescriptorsLength[4];
	UCHAR	CSCDID[2];
	UCHAR	RodToken[TXD_ROD_TOKEN_SIZE];

} RCV_ROD_TOKEN_INFO_POPULATE_TOKEN, *PRCV_ROD_TOKEN_INFO_POPULATE_TOKEN;

static __inline PXLU_ACCEPT_CCB XLU_ACCEPT_CCB_ADDR(PACCEPT_CCB Pccb)
{
	return	(PXLU_ACCEPT_CCB)((PCHAR)(Pccb) - CSX_OFFSET_OF(XLU_ACCEPT_CCB, Ccb));
}

static __inline PXLU_CCB XLU_CCB_ADDR(PCCB Pccb)
{
	return	(PXLU_CCB)((PCHAR)(Pccb) - CSX_OFFSET_OF(XLU_CCB, Ccb));
}

typedef struct _LU_STATE_VALUES {

	ULONGLONG	ActualSize;
	BOOLEAN		Attached;
	UCHAR		Quiesced;
	BOOLEAN		IownLun;
	UCHAR		Ready;
	ULONG		VolumeAttributes;

} LU_STATE_VALUES, *PLU_STATE_VALUES;


//	Every function call to a TxD from the TCD is through a TGD_INTERFACE
//	structure.  Each MAJOR type of TxD can have a unique TCD<>TxD interface,
//	as demonstated by the TDD_INTERFACE structure.  This is only an idea,
//	and probably won't be desireable from the TCD's point of view.

typedef struct _TGD_INTERFACE {

	PXLU_HDR	(*ConnectXlu)(PXLU_ENTRY, PVLU);
	BOOLEAN		(*AllocateDlu)(PXLU_ENTRY, BOOLEAN);
	BOOLEAN		(*OpenXlu)(PXLU_ENTRY, UCHAR);
	VOID		(*UpdateXlu)(PXLU_ENTRY);
	VOID		(*DisconnectXlu)(PXLU, PVLU, PINITIATOR_ID, UCHAR *, ULONGLONG);
	PVOID		(*PrepareDeleteXlu)(PXLU_ENTRY);
	VOID		(*CompleteDeleteXlu)(PVOID);
	VOID		(*AcceptIo)(PACCEPT_CCB, PCCB, PVLU);
	VOID		(*QuiesceXlu)(PXLU_ENTRY, PVOID, UCHAR);
	VOID		(*QuiesceXluIntention)(PXLU_ENTRY, PVOID, UCHAR);
	VOID		(*QuiesceCloseXlu)(PXLU_ENTRY, PVOID);
	EMCPAL_STATUS	(*UnquiesceXlu)(PXLU_ENTRY, PLU_STATE_VALUES);
	EMCPAL_STATUS	(*UnquiesceXluIntention)(PXLU_ENTRY);
	VOID		(*UnquiesceXluCommit)(PXLU_ENTRY, PLU_STATE_VALUES);
	VOID		(*AsyncEventCleanup)(PXLU);
	VOID		(*AbortReservationsAtIsr)(PVLU, BOOLEAN);
	VOID		(*PrintOpState)(PACCEPT_CCB);
	BOOLEAN		(*GetStats)(PXLU_WWN, PXLU_STATISTICS);
	VOID		(*SetLogStatus)(BOOLEAN);
	VOID		(*UpdateXluStatus)(PXLU_ENTRY);
	VOID		(*UpdatePriInfo)(VOID);
	VOID		(*BroadcastEvent) (UCHAR, PXLU);
	VOID		(*ReplyToBroadcastEvent)(PXLU);
	VOID		(*UpdateCommittedRevisionLevel) (ULONG, BOOLEAN);
	EMCPAL_STATUS	(*CheckPriAndIscsiHeader) (VOID);	
	BOOLEAN		(*InitializeXlu)(PXLU_ENTRY, UCHAR);
	VOID		(*CancelDMRequest) (PDM_RESOURCES);
	PVOID		(*GetActiveIrp)(PXLU_ACCEPT_CCB);
	PXLU_HDR	(*GetXluHdr)(PXLU_ENTRY);
	VOID        (*Unused1)(VOID);
	VOID		(*Unused2)(VOID);

} TGD_INTERFACE, *PTGD_INTERFACE;

// The following constants are passed as the 3rd argument to the QuiesceXlu().

#define XLU_NOT_QUIESCED	0
#define XLU_QUIESCED_QUEUED	1
#define XLU_QUIESCED_BUSY	2
#define XLU_QUIESCED_ERROR	0xFF

typedef struct _TDD_INTERFACE {

	TGD_INTERFACE	Common;		// Must remain first.
	ULONG			TddSomething;

} TDD_INTERFACE, *PTDD_INTERFACE;


// CMIscd specific part of the TCD -> CMIscd interface.
typedef struct _CMISCD_SUB_INTERFACE {

	EMCPAL_STATUS	(*EnableCMIConnectionIntent)(PI_T_NEXUS_KEY, PCMI_SCD_MV_TARGET);
	VOID		(*EnableCMIConnectionCommit)(PI_T_NEXUS_KEY, PCMI_SCD_MV_TARGET);
	VOID		(*EnableCMIConnectionCancel)(PI_T_NEXUS_KEY, PCMI_SCD_MV_TARGET);
	VOID		(*DisableCMIConnection)(PI_T_NEXUS_KEY, PCMI_SCD_MV_TARGET);

} CMISCD_SUB_INTERFACE, *PCMISCD_SUB_INTERFACE;


// TCD -> CMIscd interface.

typedef struct _CMISCD_INTERFACE {

	TGD_INTERFACE			Common;		// Must remain first.
	CMISCD_SUB_INTERFACE	CMIScd;		// CMIscd specific part.

} CMISCD_INTERFACE, *PCMISCD_INTERFACE;


/*
 * The BUFFMGR_CALLBACK type is used to describe a callback 
 * function passed to the Buffer Manager.
 */

typedef VOID (*CCB_CALLBACK)(PCCB);
typedef VOID (*BUFFMAN_CALLBACK)(PCCB);

#define BUFFMAN_MIN_ALLOCATION_SIZE (PAGE_SIZE * 2)

#define BUFFMAN_AAQ_ALLOCATION_SIZE (PAGE_SIZE * 16)

//	Every call to the TCD from ALL TxDs is through this table.

typedef struct _TCD_INTERFACE {

	ULONG		(*TranslateCcbToScsiport)(PACCEPT_CCB);
	PCCB		(*AllocateContinueCcb)(PACCEPT_CCB, BOOLEAN);
	VOID		(*ReleaseContinueCcb)(PCCB);
	VOID		(*ReleaseAcceptCcb)(PACCEPT_CCB);
	VOID		(*QuiesceXluComplete)(PVOID, XLU_WWN, PLU_STATE_VALUES);
	VOID		(*QuiesceXluCompleteIntention)(PVOID, XLU_WWN);
	EMCPAL_STATUS	(*Disconnect)(ULONG, ULONG);	// Major, Minor Type
	VOID		(*SetUnitAttnXlu)(PXLU, PINITIATOR_ID, UCHAR *, ULONGLONG, UCHAR, UCHAR); // Xlu, MatchingWwn, IscsiName, SessionId, ASC, ASCQ
	VOID		(*SetOwnershipUnitAttnXlu)(PXLU, PVLU, BOOLEAN, BOOLEAN, BOOLEAN, BOOLEAN);
	VOID		(*SetTransitionTypeUnitAttnXlu)(PXLU);
	VOID		(*SetCapacityChangeUnitAttnXlu)(PXLU, PXLU_ENTRY);
	VOID		(*StateChangeOnXlu)(PXLU, PXLU_ENTRY, PLU_STATE_VALUES);
	BOOLEAN		(*HandleUnitAttention)(PCCB, BOOLEAN, BOOLEAN ,BOOLEAN *);
	VOID		(*BufferFree)(PCCB);
	VOID		(*BufferAlloc)(PCCB, ULONG size, BUFFMAN_CALLBACK callback, BOOLEAN);
	PVOID		(*DisassociateBufferFromContinueCcb)(PCCB);
	VOID		(*DisassociatedBufferFree)(PVOID);
	VOID		(*XferData)(PCCB, CCB_CALLBACK NextState);
	EMCPAL_STATUS	(*AbortTaskSetForPreempt)(PXLU, PINITIATOR_ID, UCHAR *, ULONGLONG, PXLU_WWN, PACCEPT_CCB);
	UINT_32		(*ReturnModePage3FReport)(PACCEPT_CCB, PCCB, PVLU, PUCHAR);
	VOID		(*SignalAssignThread)(void);
	VOID		(*HandleEvent) (UCHAR,PXLU);
	VOID		(*BroadcastComplete) (PXLU);
	ULONG		AllPortType;
	BOOLEAN		AnyIscsiPorts;
	PBOOLEAN	IsPeerQuiesceErrored;
	ULONG		(*InitializeInquiryData) (PCCB, BOOLEAN);
	ULONG		(*GetAvailableFEPortsPerSP) (ULONG);
	VOID		(*ActivatExtCopyInfoBlock)(PVOID, BOOLEAN);
	VOID		(*GetExtCopyInfo)(PCCB, PEXT_COPY_PARAMS, PVOID *, PDM_RESOURCES *, PVOID *);
	VOID		(*DisassociatedBufferAlloc)(PDISASSOCIATED_BUFFER_ALLOC);
	ULONG		(*GetMaxBufferSize)(void);
	VOID		(*GetTPCopyInfo)(PCCB, PVOID *, PVOID *);
	VOID		(*UpdatePortStatsForWUT)(PACCEPT_CCB, ULONG);
	VOID		(*SetupContinueCcbToUseSpecialBuffer)(PCCB, PVOID, ULONG);

} TCD_INTERFACE; 		// PTCD_INTERFACE is declared earlier in the file.


//  Version descriptors for SCSI3 inquiry page

#define INQ_VER_DESCRIPTOR_1        0x003C  /* SAM, ANSI X3.270:1996 */ 
#define INQ_VER_DESCRIPTOR_2        0x0260  /* SPC-2, no version claimed */
#define INQ_VER_DESCRIPTOR_3        0x019C  /* SBC, ANSI NCITS.306:1998 */
#define INQ_VER_DESCRIPTOR_4_FC     0x08C0  /* FCP, no version claimed */
#define INQ_VER_DESCRIPTOR_4_ISCSI  0x0960  /* iSCSI, no version claimed */
#define INQ_VER_DESCRIPTOR_4_FCOE     0x08C0  /* FCP, no version claimed */
#define INQ_VER_DESCRIPTOR_4_SAS    0x0BE0  /* SAS, no version claimed */
#define INQ_VER_DESCRIPTOR_5_FC     0x1340  /* FC-PLDA, no version claimed */
#define INQ_VER_DESCRIPTOR_5_FCOE     0x1340  /* FC-PLDA, no version claimed */
#define INQ_VER_DESCRIPTOR_5_ISCSI  0x0000
#define INQ_VER_DESCRIPTOR_5_SAS    0x0000
#define INQ_VER_DESCRIPTOR_6        0x0000
#define INQ_VER_DESCRIPTOR_7        0x0000
#define INQ_VER_DESCRIPTOR_8        0x0000


//
// Additional Mode Page definitions
//

// Selectors:

#define MODE_PAGE_PROTOCOL_LUN_PAGE_CODE	0x18
#define MODE_PAGE_PROTOCOL_PORT_PAGE_CODE	0x19
#define MODE_PAGE_BIND_CODE					0x20
#define MODE_PAGE_UNBIND_CODE				0x21
#define MODE_PAGE_TRESPASS_CODE				0x22
#define MODE_PAGE_VERIFY_CODE				0x23
#define MODE_PAGE_BACKDOOR_POWER_CODE		0x24
#define MODE_PAGE_PEERINFO_CODE				0x25
#define MODE_PAGE_SP_REPORT_CODE			0x26
#define MODE_PAGE_LUN_REPORT_CODE			0x27
#define MODE_PAGE_UNSOLICITED_CODE			0x28
#define MODE_PAGE_EXT_LUN_REPORT_CODE		0x29
#define MODE_PAGE_UNIT_CONFIGURATION_CODE	0x2B
#define MODE_PAGE_SP_PERFORMANCE_CODE		0x2C
#define MODE_PAGE_UNIT_PERFORMANCE_CODE		0x2D
#define MODE_PAGE_ULOG_CTL_CODE				0x2E
#define MODE_PAGE_DISK_CRU_INFO_CODE		0x2F
#define MODE_PAGE_CRU_PASS_THRUOGH_CODE		0x30
#define MODE_PAGE_EXT_RETRIEVE_UNSOL_CODE	0x31
#define MODE_PAGE_DAQ_CODE					0x32
#define MODE_PAGE_ENCLOSURE_CODE			0x33
#define MODE_PAGE_EXT_DISK_CRU_INFO_CODE	0x34
#define MODE_PAGE_SYS_CFG_PAGE_CODE			0x37
#define MODE_PAGE_MEM_CFG_PAGE_CODE			0x38
#define MODE_PAGE_DUMP_UPLOAD_INFO			0x39
#define MODE_PAGE_RAID_GROUP_INFO_CODE		0x3A
#define MODE_PAGE_RG_CONFIG_CODE			0x3B
#define MODE_PAGE_CRU_CONFIG_CODE			0x3C
#define MODE_PAGE_HOST_TRAFFIC_CODE			0x3E
#define MODE_PAGE_ALL_PAGES_CODE			0x3F


// Define Disconnect-Reconnect page.
// This is a copy of the structure from SCSI.H, updated for SCSI-2.

typedef struct K10_MODE_DISCONNECT_PAGE {

    UCHAR PageCode : 6;
    UCHAR Reserved : 1;
    UCHAR PageSavable : 1;
    UCHAR PageLength;
    UCHAR BufferFullRatio;
    UCHAR BufferEmptyRatio;
    UCHAR BusInactivityLimit[2];
    UCHAR BusDisconnectTime[2];
    UCHAR BusConnectTime[2];
    UCHAR MaximumBurstSize[2];
    UCHAR DataTransferDisconnect : 2;
	UCHAR DImm : 2;
	UCHAR FAStat : 1;
	UCHAR FAWrt : 1;
	UCHAR FARd : 1;
	UCHAR EMDP : 1;
    UCHAR Reserved2;
	UCHAR FirstBurstSize[2];

} K10_MODE_DISCONNECT_PAGE, *PK10_MODE_DISCONNECT_PAGE;

//
// Define mode caching page.
//

typedef struct _K10_MODE_CACHING_PAGE {

	UCHAR PageCode : 6;
	UCHAR Reserved : 1;
	UCHAR PageSavable : 1;
	UCHAR PageLength;
	UCHAR ReadDisableCache : 1;
	UCHAR MultiplicationFactor : 1;
	UCHAR WriteCacheEnable : 1;
	UCHAR Size : 1;
	UCHAR Disc : 1;
	UCHAR Cap : 1;
	UCHAR ABPF : 1;
	UCHAR IC : 1;
	UCHAR WriteRetensionPriority : 4;
	UCHAR ReadRetensionPriority : 4;
	UCHAR DisablePrefetchTransfer[2];
	UCHAR MinimumPrefetch[2];
	UCHAR MaximumPrefetch[2];
	UCHAR MaximumPrefetchCeiling[2];
	UCHAR Reserved2 : 3;
	UCHAR VS : 2;
	UCHAR DRA : 1;
	UCHAR LBCSS : 1;
	UCHAR FSW : 1;
	UCHAR NumberOfCacheSegments;
	UCHAR CacheSegmentSize[2];
	UCHAR Reserved3;
	UCHAR NonCacheSegmentSize[3];

} K10_MODE_CACHING_PAGE, *PK10_MODE_CACHING_PAGE;


// Define Manual Trespass SP Mode Page

typedef struct _TRESPASS_PAGE {

    UCHAR PageCode : 6;
    UCHAR Reserved : 1;
    UCHAR PageSavable : 1;
    UCHAR PageLength;
    UCHAR TrespassCode : 6;
	UCHAR TrespassLogSuppress: 1;	// May only be used by the Ioctl Initiator
    UCHAR HonorReservation : 1;
    UCHAR LunOrRaidGroup[8];

} TRESPASS_PAGE, *PTRESPASS_PAGE;


// Define SP Report Page

typedef struct _SPREPORT_PAGE {

	UCHAR PageCode : 6;
	UCHAR Reserved : 1;
	UCHAR PageSavable : 1;
	UCHAR PageLength;
	UCHAR CabinetId;
	UCHAR SpId;
	UCHAR SpModel[4];
	UCHAR UCodePassHigh;
	UCHAR UCodeMajorRevision;
	UCHAR UCodeMinorRevision;
	UCHAR UCodePassLow;
	UCHAR HardErrors[4];
	UCHAR MaximumOutstandingRequests[4];
	UCHAR AverageOutstandingRequests[4];
	UCHAR BusyTicks[4];
	UCHAR IdleTicks[4];
	UCHAR ReadCount[4];
	UCHAR WriteCount[4];

} SPREPORT_PAGE, *PSPREPORT_PAGE;


// Define Peer SP Mode Page

typedef struct _PEERINFO_PAGE {

	UCHAR PageCode : 6;
	UCHAR Reserved : 1;
	UCHAR PageSavable : 1;
	UCHAR PageLength;
	UCHAR TargetSpPreferredLoopId;
	UCHAR PeerSpPreferredLoopId;
	UCHAR TargetSpSignature[4];
	UCHAR PeerSpSignature[4];
	UCHAR CabinetId;
	UCHAR SpId;
	UCHAR TargetSpAcquiredLoopId;
	UCHAR PeerSpAcquiredLoopId;
	UCHAR TargetSpNportId[3];
	UCHAR PeerSpNportId[3];
	UCHAR Reserved2[2];

} PEERINFO_PAGE, *PPEERINFO_PAGE;


// Define Unit Configuration Page

typedef struct _UNIT_CONFIG_PAGE {

	UCHAR PageCode : 6;
	UCHAR Reserved : 1;
	UCHAR PageSavable : 1;
	UCHAR PageLength;
	UCHAR Lun;
	UCHAR NM1 : 1;
	UCHAR Reserved1 : 7;
	UCHAR RCE : 1;
	UCHAR WCE : 1;
	UCHAR AA : 1;
	UCHAR AT : 1;
	UCHAR BCV : 1;
	UCHAR NotBound : 1;		// Lun is private, non-existant, or being bound.
	UCHAR OWN : 1;
	UCHAR Reserved3 : 1;
	UCHAR MaximumVerifyTime;
	UCHAR IdleThreshold;
	UCHAR IdleDelayTime;
	UCHAR WriteAsideSize[2];
	UCHAR DefaultOwnership;
	UCHAR MaximumRebuildTime;
	UCHAR SRCP : 1;
	UCHAR Retention : 3;
	UCHAR PrefetchType : 4;
	UCHAR SegmentSize_Multiplier;
	UCHAR PrefetchSize_Multiplier[2];
	UCHAR MaximumPrefetch[2];
	UCHAR PrefetchDisable[2];
	UCHAR PrefetchIdleCount[2];
	UCHAR Reserved4[10];

} UNIT_CONFIG_PAGE, *PUNIT_CONFIG_PAGE;

// Define Protocol Specific LUN Page for FC, iSCSI and SAS

typedef struct _PROTOCOL_LUN_PAGE_FC {

	UCHAR PageCode : 6;
	UCHAR Reserved : 1;
	UCHAR PageSavable : 1;
	UCHAR PageLength;
	UCHAR ProtocolId : 4;
	UCHAR Reserved1 : 4;
	UCHAR EPDC : 1;
	UCHAR Reserved2 : 7;
	UCHAR Reserved3[4];

} PROTOCOL_LUN_PAGE_FC, *PPROTOCOL_LUN_PAGE_FC;

typedef struct _PROTOCOL_LUN_PAGE_ISCSI {

	UCHAR PageCode : 6;
	UCHAR Reserved : 1;
	UCHAR PageSavable : 1;
	UCHAR PageLength;
	UCHAR ProtocolId : 4;
	UCHAR Reserved1 : 4;

} PROTOCOL_LUN_PAGE_ISCSI, *PPROTOCOL_LUN_PAGE_ISCSI;

typedef struct _PROTOCOL_LUN_PAGE_FCOE {

	UCHAR PageCode : 6;
	UCHAR Reserved : 1;
	UCHAR PageSavable : 1;
	UCHAR PageLength;
	UCHAR ProtocolId : 4;
	UCHAR Reserved1 : 4;

} PROTOCOL_LUN_PAGE_FCOE, *PPROTOCOL_LUN_PAGE_FCOE;


typedef struct _PROTOCOL_LUN_PAGE_SAS {

	UCHAR PageCode : 6;
	UCHAR Reserved : 1;
	UCHAR PageSavable : 1;
	UCHAR PageLength;
	UCHAR ProtocolId : 4;
	UCHAR TransportLayerRetries : 1;
	UCHAR Reserved1 : 3;
	UCHAR Reserved2;
	UCHAR Reserved3[4];

} PROTOCOL_LUN_PAGE_SAS, *PPROTOCOL_LUN_PAGE_SAS;

// Define Protocol Specific Port Page FC, iSCSI and SAS 

typedef struct _PROTOCOL_PORT_PAGE_FC {

	UCHAR PageCode : 6;
	UCHAR Reserved : 1;
	UCHAR PageSavable : 1;
	UCHAR PageLength;
	UCHAR ProtocolId : 4;
	UCHAR Reserved1 : 4;
	UCHAR DTOLI : 1;
	UCHAR DTIPE : 1;
	UCHAR ALWI : 1;
	UCHAR RHA : 1;
	UCHAR DLM : 1;
	UCHAR DDIS : 1;
	UCHAR PLPB : 1;
	UCHAR DTFD : 1;
	UCHAR Reserved2[2];
	UCHAR RRTOVU : 3;
	UCHAR Reserved3 : 5;
	UCHAR RRTOV;

} PROTOCOL_PORT_PAGE_FC, *PPROTOCOL_PORT_PAGE_FC;


typedef struct _PROTOCOL_PORT_PAGE_ISCSI {

	UCHAR PageCode : 6;
	UCHAR Reserved : 1;
	UCHAR PageSavable : 1;
	UCHAR PageLength;
	UCHAR ProtocolId : 4;
	UCHAR Reserved1 : 4;

} PROTOCOL_PORT_PAGE_ISCSI, *PPROTOCOL_PORT_PAGE_ISCSI;


typedef struct _PROTOCOL_PORT_PAGE_FCOE {

	UCHAR PageCode : 6;
	UCHAR Reserved : 1;
	UCHAR PageSavable : 1;
	UCHAR PageLength;
	UCHAR ProtocolId : 4;
	UCHAR Reserved1 : 4;

} PROTOCOL_PORT_PAGE_FCOE, *PPROTOCOL_PORT_PAGE_FCOE;

typedef struct _PROTOCOL_PORT_PAGE_SAS {

	UCHAR PageCode : 6;
	UCHAR Reserved : 1;
	UCHAR PageSavable : 1;
	UCHAR PageLength;
	UCHAR ProtocolId : 4;
	UCHAR ReadyLED : 1;
	UCHAR BroadcastAsyncEvent : 1;
	UCHAR ContinueAWT : 1;
	UCHAR Reserved1 : 1;
	UCHAR Reserved2;
	USHORT ITNexusLossTime;
	USHORT InitiatorResponseTimeout;
	USHORT RejectToOpenLimit;
	UCHAR Reserved3[6];

} PROTOCOL_PORT_PAGE_SAS, *PPROTOCOL_PORT_PAGE_SAS;

// Protocol IDs used in the above pages

#define PROTOCOL_ID_FIBRE		0
#define PROTOCOL_ID_FCOE		0
#define PROTOCOL_ID_PARALLEL	1
#define PROTOCOL_ID_ISCSI		5
#define PROTOCOL_ID_SAS			6

// The SCSI standard specifies that persistent reservations use the ISCSI name and session ID to 
// identify the initiator that has a registration or reservation. However in TDD we use a 128 bit
// ID. A table of ISCSI names that may have reservations is kept by TDD. The index from that table 
// is used by TDD along with the session ID to generate a 128 bit ID. Once the ID has been  generated
// it is stored as a hint in the TCD login session along with the generation number of the table so that 
// subsequent accepts do not need to regenerate the ID.

// Structure to hold the hint info in the login session entry.

typedef struct _PRESV_HINT_INFO {

	ULONG	GenerationNumber;
	USHORT	Index;
	USHORT	Resv;

} PRESV_HINT_INFO, *PPRESV_HINT_INFO;

// Macro exported by TCD that allows TDD to opaquely get the address of a login session entry
// parameter key. It is used to give TDD access to the iscsi name and session ID for the
// initiator.

#define GET_LS_KEY(Acc) &((PLOGIN_SESSION_ENTRY)(Acc)->Path.login_context)->Params.Key

// TCD exported macro to allow TDD to get the hint data from the login session entry.

#define GET_LS_HINT(Acc) ((PPRESV_HINT_INFO)(((PLOGIN_SESSION_ENTRY)(Acc)->Path.login_context) + 1))

// TCD exported macro to allow TDD to set the hint data in the login session entry.

#define SET_LS_HINT(Acc, Gen, Idx)				\
	(GET_LS_HINT(Acc))->GenerationNumber = (Gen);	\
	(GET_LS_HINT(Acc))->Index = (Idx);

//
// Name: GET_LBA_STATUS_HEADER
//
// Description:  This is the structure which is returned by the driver to describe all extents to hosts
// 
//
typedef struct _GET_LBA_STATUS_HEADER
{ 
       // The number of LBA_STATUS_DESCRIPTOR blocks being returned for Extent Data.
       ULONG ParameterDataLength; 

       // Reserved bytes for future expansion.
       ULONG reserved; 

} GET_LBA_STATUS_HEADER, *PGET_LBA_STATUS_HEADER;

//
// Name: LBA_STATUS_DESCRIPTOR
//
// Description:  This is the structure returned by the driver to an initiator to describe LBA data (extents).
//
// Provisioning Status as defined by T-10 committee (06/2011)
#define EXTENT_IS_MAPPED       0x00
#define EXTENT_IS_DEALLOCATED  0x01
#define EXTENT_IS_ANCHORED     0x02

#pragma pack(4)
typedef struct _LBA_STATUS_DESCRIPTOR
{ 
       // The starting LBA of the Extent.
       ULONGLONG LogicalBlockAddress; 

       // The number of sectors in this extent.
       ULONG     ExtentLength; 

       // Attributes of this extent
       UINT_8E   ProvisioningStatus;

       // Reserved bytes in SCSI command.
       UINT_8E    Reserved1;  // Must be zero.
       UINT_16E   Reserved2;  // Must be zero.

} LBA_STATUS_DESCRIPTOR, *PLBA_STATUS_DESCRIPTOR;
#pragma pack()

#endif /* _TCDTXD_H_ */
