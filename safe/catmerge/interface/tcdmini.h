/*******************************************************************************
 * Copyright (C) Data General Corporation, 1998
 * All rights reserved.
 * Licensed material - Property of Data General Corporation
 ******************************************************************************/

/*******************************************************************************
 * tcdmini.h
 *
 * This file defines the interface between a Target Mode capable Miniport and
 * the SCSI Target class driver (TCD).
 *
 ******************************************************************************/

#ifndef _TCDMINI_H_
#define _TCDMINI_H_

#include "minishare.h"
#include "tcdshare.h"
#include "k10ntddk.h"
//#include <storport.h>
//#include <ntddscsi.h>

// This enumeration defines the possible types of miniport drivers.
// PARALLEL_SCSI does not support World Wide Names or soft addressing.
// Fibre Channel supports both of these.

typedef enum _CONNECT_CLASS
{

	CC_PARALLEL_SCSI	= 1,
	CC_FIBRE_CHANNEL	= 2,

} CONNECT_CLASS;

// The following defines are used directly in Inquiry Data generated
// by the miniports.

#define VendorIdCLARiiON	'C','L','A','R','i','i','O','N'

#define InquiryIdStrings												\
	CHAR			VendorIdCLARiiONStr[] = {VendorIdCLARiiON, '\0'};

// Used to get/put the PathID (bus number) within the HBA into the ControlCode

#define CTM_OPCODE_MASK					0x800000ffL
#define CTM_PATH_ID_MASK				0x00ff0000L		// Path ID goes here.
#define CTM_PATH_ID_SHIFT				16
#define CTM_PATH_ID(Ctm)				((UCHAR)(((ULONG)(Ctm) >> 16) & 0xffL))
#define CTM_ADD_PATH_ID(PathId)			((ULONG)(PathId) << 16)

// Errors returned from SRB_FUNCTION_IO_CONTROL in the
// ReturnCode field of PSRB_IO_CONTROL.

typedef enum _RETURN_CODE
{

	RC_STRUCT_MISMATCH,
	RC_INVALID_PARAMETER,
	RC_BAD_SIGNATURE,
	RC_TARGET_ENABLED,
	RC_TARGET_DISABLED,

} RETURN_CODE;


/*************************************************************************
 *
 * The following proprietary ControlCodes are issued to the miniport
 * in an SRB_FUNCTION_IO_CONTROL SRB with a Signature of "CLARiiON".
 * These fields are both in the SRB_IO_CONTROL structure, whose nonpagable
 * kernel address can be found in Srb->DataBuffer.  Any data which is
 * specific to a particular control code follows the SRB_IO_CONTROL structure.
 *
 * A ControlCode must be supported by all types of target miniports unless
 * otherwise noted.
 *
 * There is one device object for each HBA with a sub-object for each
 * bus on that HBA. All of these ControlCodes are targeted at a specific
 * bus sub-object.
 *
 * Since the ScsiPort Driver overwrites the Srb.PathId field on an
 * SRB_FUNCITON_IO_CONTROL, there's not a good way to tell the MiniPort
 * Driver which SCSI Port the current IOCTL is intended for.  Therefore,
 * we are encoding the Path ID into the ControlCode itself.
 *
 * If there is only one bus per HBA, then the PathID subfield is always zero.
 *
 **************************************************************************/

/*******************************************************************************
 * CTM_GET_TARGET_MINI_CONFIG retrieves data about a port.
 ******************************************************************************/

#define	CTM_GET_TARGET_MINI_CONFIG	0x80000001L	// data: TARGET_MINI_CONFIG

typedef struct _TARGET_MINI_CONFIG {

	CONNECT_CLASS		ConnectType;			// Read-Only
	ULONG				SystemIoBusNumber;		// Read-Only
	ULONG				SlotNumber;				// Read-Only

	// The number of Accept CCBs that TCD must allocate.
	// Zero means no specific requirement.

	ULONG				RequiredAcceptCcbs;		// Read-Only

} TARGET_MINI_CONFIG, *PTARGET_MINI_CONFIG;


/*******************************************************************************
 * CTM_GET_TARGET_WWN retrieves the WWN of a port.  CTM_SET_TARGET_WWN specifies
 * the WWN of a port. Parallel SCSI miniports don't support these IOCTLs.
 ******************************************************************************/

#define	CTM_GET_TARGET_WWN	0x80000002L	// data: WWN

#define	CTM_SET_TARGET_WWN	0x80000003L	// data: WWN

typedef struct _WWN
{

	ULONGLONG	Node;
	ULONGLONG	Port;

} WWN, *PWWN;

typedef	struct _SRB_CTM_IOCTL_SET_WWN {

	SRB_IO_CONTROL	SrbIoControl;
	WWN				WWNs;

} SRB_CTM_IOCTL_SET_WWN;

/*******************************************************************************
 * CTM_GET_TARGET_QFULL_RESPONSE retrieves how the miniport should behave if 
 * there are no Accept CCBs.  CTM_SET_TARGET_QFULL_RESPONSE specifies how the 
 * miniport should behave if there are no Accept CCBs.
 ******************************************************************************/

#define	CTM_GET_TARGET_QFULL_RESPONSE	0x80000004L	// data: QFULL_RESPONSE

#define	CTM_SET_TARGET_QFULL_RESPONSE	0x80000005L	// data: QFULL_RESPONSE


/*******************************************************************************
 * CTM_GET_PORT_INFO retrieves general Fibre Channel Port information. 
 * Not supported by parallel SCSI miniports.
 ******************************************************************************/

#define CTM_GET_PORT_INFO	0x80000006L	// data: SRB_CTM_FC_PORT_INFO

typedef	struct {

	UCHAR			LinkStatus;		// Values defined below.
	UCHAR			PortState;		// Values defined below.
	UCHAR			ConnectType;	// Values defined below.
	UCHAR			AcquiredLoopId;	// Actual Loop ID acquired.
	ULONG			NportID;		// ALPA (arbitrated loop) or S_ID (fabric).
	WWN				FabricWWN;		// Only valid if connected to a fabric.
	USHORT			LinkSpeed;		// Current FC link speed.

} FC_PORT_INFO, *PFC_PORT_INFO;

#define LST_LINK_DOWN		0
#define LST_LINK_UP_RCV		1
#define LST_LINK_UNINITIALIZED	2

#define PS_OFFLINE							0
#define PS_PENDING							1
#define PS_POINT_TO_POINT					2
#define PS_NONPARTICIPATING_ARBITRATED_LOOP	3
#define PS_PARTICIPATING_ARBITRATED_LOOP	4
#define	PS_FAULTED							5

#define CT_POINT_TO_POINT	1
#define CT_ARBITRATED_LOOP	2
#define CT_NOT_CONNECTED	5

#pragma pack(4)

typedef struct _SRB_CTM_FC_PORT_INFO
{

	SRB_IO_CONTROL	SrbIoControl;
	FC_PORT_INFO	SrbPortInfo;

} SRB_CTM_FC_PORT_INFO, *PSRB_CTM_FC_PORT_INFO;

#pragma pack()

/*******************************************************************************
 * CTM_ENABLE_TARGET is called during initialization to allow the miniport to 
 * start operating in target mode.  This sets up a callback interface into the 
 * higher-level drivers.
 ******************************************************************************/

#define	CTM_ENABLE_TARGET	0x80000007L	// data: SET_CALLBACKS

typedef PCCB (*PGET_CCB_CALLBACK)(PVOID Context);
typedef PACCEPT_CCB	(*PGET_ACCEPT_CCB_CALLBACK)(PVOID Context);

typedef struct _SET_CALLBACKS {

	// The value to pass for the first parameter when making the callbacks.

	PVOID						Context;					// CwMr

	// The miniport will call this to notify of asynchronous events.

	PASYNC_CALLBACK				AsyncCallback;				// CwMr

	// The miniport will call this to dequeue a waiting Continue CCB.
	// Returns NULL if no waiting Continue CCBs.

	PGET_CCB_CALLBACK			GetPendingContinueCallback;	// CwMr

	// The miniport will call this to allocate an Accept CCB.  Returns
	// NULL if there are no more free CCBs.

	PGET_ACCEPT_CCB_CALLBACK	GetFreedAcceptCallback;		// CwMr
	PVOID						CallbackHandle;				// CrMw

} SET_CALLBACKS, *PSET_CALLBACKS;

typedef struct _SRB_CTM_IOCTL_ENABLE_TARGET {

	SRB_IO_CONTROL	SrbIoControl;
	SET_CALLBACKS	SrbCtmIoCtlSetCallbacks;

} SRB_CTM_IOCTL_ENABLE_TARGET;


/*******************************************************************************
 * CTM_DISABLE_TARGET is called during shutdown to stop the miniport from 
 * making direct callbacks to the higher-level driver to allocate Accept CCBs 
 * or to get pending Continue CCBs.  The miniport is not necessarily done with 
 * all CCBs when this call is complete and the caller should, therefore, wait 
 * until all CCBs have been returned after this IOCTL is returned and before
 * shutting down.  It is legal to send new Continue CCBs after this IOCTL
 * and the miniport will either process them or abort them.
 ******************************************************************************/

#define	CTM_DISABLE_TARGET	0x80000008L	// data: DISABLE_TARGET

typedef struct _DISABLE_TARGET {

	PVOID					CallbackHandle;

} DISABLE_TARGET, *PDISABLE_TARGET;

typedef struct _SRB_CTM_IOCTL_DISABLE_TARGET {

	SRB_IO_CONTROL	SrbIoControl;
	DISABLE_TARGET	SrbCtmIoCtlUnsetCallbacks;

} SRB_CTM_IOCTL_DISABLE_TARGET;


/*******************************************************************************
 * CTM_ASYNC_EVENT_DECREMENT is called to decrement the "freeze" count in the
 * miniport.  Asynchronous event conditionally require a corresponding decrement
 * in order to unfreeze the outgoing Continue CCB queue in the miniport.
 ******************************************************************************/

#define	CTM_ASYNC_EVENT_DECREMENT	0x80000009L	// data: none


/*******************************************************************************
 * CTM_MORE_WORK_PENDING is called by TCD to notify the miniport that there may
 * be more pending Continue CCBs.  The miniport pulls Continue CCBs from TCD 
 * via a callback function whenever it needs more work.  If the miniport
 * goes idle, it needs to be stimulated to poll for pending Continue CCBs.
 ******************************************************************************/

#define	CTM_MORE_WORK_PENDING	0x8000000aL	// data: none


/*******************************************************************************
 * PARALLEL:	Soft addressing is always false and cannot be changed.
 * FIBRE:		Soft addressing defaults to true.  With soft addressing,
 *				the ALPA may change. A value of FALSE specifies that
 *				hard addressing should be used.
 ******************************************************************************/

#define	CTM_GET_TARGET_SOFT_ADDRESSING	0x8000000bL	// data: BOOLEAN
#define	CTM_SET_TARGET_SOFT_ADDRESSING	0x8000000cL	// data: BOOLEAN


/*******************************************************************************
 * PARALLEL:	The Target ID the HBA will respond to.
 * FIBRE:		The preferred loopID for soft addressing or the required
 *				loopID for hard addressing.
 *				The loop ID is a 0..126 mapping of the ALPA (0..255).
 * The target ID may only be changed before the target is initialized.
 ******************************************************************************/

#define	CTM_GET_SCSI_TARGET_ID	0x8000000dL	// data: ULONG
#define	CTM_SET_SCSI_TARGET_ID	0x8000000eL	// data: ULONG


/*******************************************************************************
 * Has the specified function executed in the context of the miniport.
 * (*IsrCallback.Callback)(IsrCallback.p1, IsrCallback.p2)
 ******************************************************************************/

#define	CTM_CALL_FUNCTION_IN_ISR_CONTEXT	0x8000000fL	// data: ISR_CALLBACK

typedef UCHAR	(*PISR_FUNCTION)(PVOID p1, PVOID p2);

typedef struct _ISR_CALLBACK {

	PISR_FUNCTION		CallbackFunction;
	PVOID				p1;					// First arg to function
	PVOID				p2;					// second arg to function.

} ISR_CALLBACK, *PISR_CALLBACK;


// Details for 

typedef	struct _SRB_CTM_CALL_FUNCTION_IN_ISR_CONTEXT {

	SRB_IO_CONTROL	SrbIoControl;
	ISR_CALLBACK	IsrCallback;

} SRB_CTM_CALL_FUNCTION_IN_ISR_CONTEXT;

/*******************************************************************************
 * Retrieve extended configuration information. Supported only in hptl.sys.
 ******************************************************************************/

#define	CTM_GET_TARGET_MINI_CFG2	0x80000010L	// data: TARGET_MINI_CFG2

// Value returned by IOCTL (CTM_GET_TARGET_MINI_CFG2).
//
//	The miniport will consider that it is compatible with the received
//	structure and contents if BOTH the following are true:
//	a)	The miniport knows about and supports the format version identified
//		in the "FmtVer_Major" field.
//	b)	The value in the ".Length" field of the "SRB_IO_CONTROL" structure
//		is greater than or equal to the minimum size for this major version
//		of this structure.  See "FmtVer_Major" field for specifics.
//
//	Within a major format version, it is expected that compatible extensions
//	to this structure can be added by the following means:
//	a)	Convert reserved fields to defined fields.  Choose the values and
//		rules for these newly defined fields to be compatible with older
//		miniport and upper level driver code that thinks these are reserved
//		fields.  If the choice of values and rules is sufficient for
//		backwards compatibility, bits in the ".FmtFlags" field should not
//		be required.
//	b)	Add more fields to the end of this structure (increasing it's size)
//		and indicate the presence of these fields by bits in the ".FmtFlags"
//		field.  Older versions of code would simply not know about these
//		fields.

#define MINI_CFG2_RSV_CNT 30

typedef struct _TARGET_MINI_CFG2 {

	////////////////////
	// BEGIN OLD SECTION
	////////////////////

	CONNECT_CLASS		ConnectType;			// OUTPUT ONLY.

	ULONG				SystemIoBusNumber;		// OUTPUT ONLY.

	ULONG				SlotNumber;				// OUTPUT ONLY.

	// The number of Accept CCBs that TCD must allocate.  Zero means
	// no specific requirement. Currently the value here is the same as
	// the value in "cdb_inbound_max", but this should not be assumed.

	ULONG				RequiredAcceptCcbs;		// OUTPUT ONLY.

	////////////////////
	// BEGIN NEW SECTION
	////////////////////

	// Used to indicate a major format change which isn't compatible
	// with previous/later versions unless the code specifically knows
	// about and supports multiple major versions.  The current major
	// version number is defined in the #define here.
	//
	//  HOPEFULLY we never need to change the major version number because
	// we can do make any extensions by adding to the end of this
	// structure in a way which is forwards and backwards compatible.

	USHORT FmtVer_Major;				// INPUT ONLY.

	// INPUT:
	//	Used to indicate option extensions used/supported by the sender
	//	relative to the verion indicated in (FmtVer_Major).  All such
	//	extensions will be backward compatible.  MUST be explicitly
	//	set by the sender, even if the value is (0) (as is the case
	//	now).
	// OUTPUT:
	//	Used to indicate optional extensions used/supported by the
	//	driver.  MUST be updated by the driver before the IOCTL is
	//	completed.
	// The bit definitions will be the same for the INPUT and OUTPUT
	// values of this field, and all extensions indicated by these
	// bits will be defined to be backward compatabile.
	// Currently no bits are defined.
	// IMPORTANT -- bits in this field should NOT be used to indicate
	// extensions made using the fields currently designated as reserved
	// unless the convention specified here for handling of reserved fields
	// isn't sufficient to provide backward/forward compatability.

	USHORT FmtFlags;					// INPUT & OUTPUT.

	// TRUE if miniport is configured to initiate logins with all devices
	// on the local loop.

	BOOLEAN LoginsActive;				// OUTPUT ONLY.

	// Combined (inbound + outbound) maximum number of concurrent CDBs
	// supported.

	USHORT cdb_total_max;				// OUTPUT ONLY.

	// OUTPUT Maximum number of concurrent inbound CDBs.  Equal to the number of
	// CCB accept blocks required by the driver.

	USHORT cdb_inbound_max;				// OUTPUT ONLY.

	// Number of back-end FLARE drives supported by the driver.  Currently
	// this value is either (0) or (120).

	USHORT k10_drives;					// OUTPUT ONLY.

	// Number of additional target devices the driver is configured to
	// support in addition to back-end FLARE drives (if any).  Currently the
	// flexability here is limited, and the driver always supports at least
	// a couple even if the value reported here is (0) because none where
	// explicitly requested.

	USHORT k10_tgts_more;				// OUTPUT ONLY.

	// Number of initiator devices the driver has been configured to support
	// (for communication with hosts and CMI devices).  Most useful is just
	// whether this value is zero or non-zero.

	USHORT k10_initiators;				// OUTPUT ONLY.

	// Prefered loop ID in range (0..125) if value is positive.
	// Negative value means prefered loop ID not specified.

	int pref_loopid;					// OUTPUT ONLY.

	// INPUT:
	//	Address of buffer to receive pass-thru configuration string
	//	from the miniport's "K" registry string.  Length of buffer is
	//	given by "PassThruBufLen".
	// OUTPUT:
	//	EOS-terminated string which consists of the contents of the
	//	parameter clause of the "K" pass-thru string option from the
	//	miniport's port configuration string.

	CHAR *PassThruBuf;					// OUTPUT & OUTPUT.

	// Size of buffer to receive pass-thru configuration string from
	// the miniport's "K" registry string.

	USHORT PassThruBufLen;				// INPUT ONLY.

	// Indicates if errors occured in handling the pass-thru string as
	// follows:
	//  0:	No errors.
	//  1:	String truncated within the driver because string too long for
	//		the buffer used by the driver to remember the string.
	//  2:	String truncated when returning this string due to string
	//		saved in miniport longer than the buffer provided here.
	//  3:	Both of (1) and (2) conditions occured.
	//  4:	Syntax error which probably results in loss of information.
	//		Not currently reported.

	USHORT PassThruStrErr;				// OUTPUT ONLY.

	USHORT AvailableSpeeds;				// OUTPUT ONLY.

	////////////////////////////////
	// RESERVED FIELDS FOR EXPANSION
	////////////////////////////////

	UCHAR rsvd_char[MINI_CFG2_RSV_CNT];		// INPUT & OUPUT.

} TARGET_MINI_CFG2, *PTARGET_MINI_CFG2;

#define MINI_CFG2_FMTVER_MAJOR 1

#define MINI_ULDSTR_ERR_ULDBUF 1;
#define MINI_ULDSTR_ERR_MINIBUF 2;
#define MINI_ULDSTR_ERR_SYNTAX 4;


/*******************************************************************************
 * Retrieves loop id's required to report Mode Page 0x25. 
 * Not supported by parallel SCSI (obviously) or other than hptl.sys.
 ******************************************************************************/

#define CTM_GET_LOOP_IDS	0x80000011L	// data: SRB_CTM_GET_LOOP_IDS

typedef struct _SRB_CTM_GET_LOOP_IDS {

	SRB_IO_CONTROL	SrbIoControl;
	UCHAR			PreferredLoopId;	// Hard ID set by rotary switches
	UCHAR			AcquiredLoopId;
	ULONG			NportId;

} SRB_CTM_GET_LOOP_IDS;


#endif /* _TCDMINI_H_ */
