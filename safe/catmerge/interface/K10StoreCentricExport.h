// K10StoreCentricExport.h
//
// Copyright (C) 1998-2002	EMC Corporation
//
//
// Public Header for K10StorCentric AdminLib usage
//
//	Revision History
//	----------------
//	18 Jun 98	D. Zeryck	Initial version.
//	03 Nov 98	D. Zeryck	Merge with K10Hostadmin - note admin lib name change
//	23 Sep 99	D. Zeryck	Adapt to new SC, TCD interface changes
//	13 Oct 99	D. Zeryck	Move internal defs out
//	 3 Mar 00	D. Zeryck	Add K10_STRCNTRC_ERROR_BAD_INITIATOR_OPTIONS
//	 3 Mar 00	D. Zeryck	Add K10_STRCNTRC_ERROR_BAD_INITIATOR_OPTIONS
//	 5 Mar 00	H. weiner	Add severity bits to error codes.
//	21 Apr 00	D. Zeryck	Add file error
//	27 Apr 00	D. Zeryck	Put SC Database IDs in here
//	31 Aug 00	B. Yang		Task 1774 - Cleanup the header files
//	 1 Apr 02	H. Weiner	DIMS 69670 - Add K10_STRCNTRC_ERROR_PORT_TOO_MANY_INITIATORS

#ifndef _K10StorageCentric_Export_h_
#define _K10StorageCentric_Export_h_

#include "k10defs.h"
//-----------------------------------------------------------------------DEFINES

#define K10_STRCNTRC_ERROR_BASE		0x70000000

//--------------------------------------------------------------------------ENUMS

enum K10_STRCNTRC_ERROR {						// specific error codes
	K10_STRCNTRC_ERROR_SUCCESS = 0x00000000L,

	K10_STRCNTRC_ERROR_ERROR_MIN = (K10_STRCNTRC_ERROR_BASE | K10_COND_ERROR),
	K10_STRCNTRC_ERROR_BAD_DBID = K10_STRCNTRC_ERROR_ERROR_MIN,
												// Unrecog. database ID in CDB
	K10_STRCNTRC_ERROR_BAD_OPC,					// Unrecog. opcode in CDB
	K10_STRCNTRC_ERROR_BAD_ISPEC,				// Unrecod item specifier
	K10_STRCNTRC_ERROR_IOCTL_TIMEOUT,			// Timeout when waiting for DeviceIoControl
	K10_STRCNTRC_ERROR_IOCTL,					// Other DeviceIoControl error
	K10_STRCNTRC_ERROR_NOT_SUPPORTED,			// we dont do this
	K10_STRCNTRC_ERROR_WRONG_DATASIZE,			// Don't have correct size of element
	K10_STRCNTRC_ERROR_WRONG_FILE_VER,			// Don't understand version of file in PSM
	K10_STRCNTRC_ERROR_LU_NOT_PUBLIC,			// Trying to put pvt LU into VArray
	K10_STRCNTRC_ERROR_NOT_NULL,				// An expected NULL pointer wasn't
	K10_STRCNTRC_ERROR_BAD_ARG,					// Illegal input arg
	K10_STRCNTRC_ERROR_BAD_INITIATOR_OPTIONS,	// illegal options bits for an initiator record
	K10_STRCNTRC_ERROR_FILE_ERROR,				// Some file error, see description
	K10_STRCNTRC_ERROR_PORT_TOO_MANY_INITIATORS,	// Attempt to add more initiators than a port allows
	K10_STRCNTRC_ERROR_ERROR_MAX = K10_STRCNTRC_ERROR_PORT_TOO_MANY_INITIATORS
};


//-----------------------------------------------------------------------------------
//
// The storage centric database IDs used by Navipshere/front end
//

enum FRONTEND_SC_DB_ID {
	FRONTEND_SC_DB_ID_MIN = 0,
	FRONTEND_SC_DB_ID_PHYSICAL_ARRAY = FRONTEND_SC_DB_ID_MIN,
	FRONTEND_SC_DB_ID_INITIATOR,
	FRONTEND_SC_DB_ID_VIRTUAL_ARRAY,
	FRONTEND_SC_DB_ID_ATF_RESERVED,
	FRONTEND_SC_DB_ID_FEATURE_LICENSE,
	FRONTEND_SC_DB_ID_RESERVED_SC_5,
	FRONTEND_SC_DB_ID_RESERVED_SC_6,
	FRONTEND_SC_DB_ID_RESERVED_SC_7,
	FRONTEND_SC_DB_ID_RESERVED_K10_8, // this is probably snapcopy...
	FRONTEND_SC_DB_ID_RESERVED_K10_9,
	FRONTEND_SC_DB_ID_RESERVED_K10_10,
	FRONTEND_SC_DB_ID_RESERVED_K10_11,
	FRONTEND_SC_DB_ID_RESERVED_K10_12,
	FRONTEND_SC_DB_ID_RESERVED_K10_13,
	FRONTEND_SC_DB_ID_RESERVED_K10_14,
	FRONTEND_SC_DB_ID_RESERVED_K10_15,
	FRONTEND_SC_DB_ID_MEGAPOLL,
	FRONTEND_SC_DB_ID_MAX = FRONTEND_SC_DB_ID_MEGAPOLL
};
#endif
