/*******************************************************************************
 * Copyright (C) EMC Corporation, 2015
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * ScsiTargVirtualVolumes.h
 *
 * This header file contains structures and constants which are needed by other
 * drivers and applications when performing VVol (Virtual Volume) IOCTLs to TCD.
 *
 ******************************************************************************/

#ifndef ScsiTargVirtualVolumes_h
#define ScsiTargVirtualVolumes_h 1



typedef struct _VVOL_PE_PAIR {

	VVOL_WWN  	    VVolWwn;
    PE_WWN          PEWwn;

} VVOL_PE_PAIR, *PVVOL_PE_PAIR;


typedef struct _VVOL_GET_ALL_INPUT_DATA {

	SG_WWN  	    SGWwn;
    PE_WWN          PEWwn;

} VVOL_GET_ALL_INPUT_DATA, *PVVOL_GET_ALL_INPUT_DATA;


///////////////////////////////////////////////////////////////////////////////
// A VVOL_BIND_PARAM structure will specify the input parameters to the 
// IOCTL_SCSITARG_BIND_VVOL ioctl.
// 
// Revision:
//		Format of this structure.
//
// SGWwn:	
//      The WWN of the Storage Group to bind the VVol(s) to.
//
// PEs:
//      The WWNs of PE(s) to which VVol(s) could be bound.
//
// Count:
//      Number of VVols.
//
// VVolWwns:
//		Array of WWNs of VVols to bind.
// 
// BindContext:
//		For future use. 
//
// Reserved*:
//		Must be zero.
//
///////////////////////////////////////////////////////////////////////////////

typedef struct _VVOL_BIND_PARAM {

	ULONG			    Revision;
	SG_WWN  	        SGWwn;
    ULONG               PEWwnsCount;
    PE_WWN              PEWwns[MAX_NUMBER_OF_PE_PER_SG];
    ULONG               BindContext;            // VVOL-TODO: create an enum, currently not used
    ULONG			    Reserved[4];	        // Must be 0.
    ULONG			    Count;
    VVOL_WWN     		VVolWwns[1];

} VVOL_BIND_PARAM, *PVVOL_BIND_PARAM;


//  Current revision level of VVOL_BIND_PARAM.

#define CURRENT_VVOL_BIND_PARAM_REV	1


// VVol Bind Param size macro.

#define VVOL_BIND_PARAM_SIZE(Count)					\
	(sizeof(VVOL_BIND_PARAM) - sizeof(VVOL_WWN) +	\
	(Count) * sizeof(VVOL_WWN))


///////////////////////////////////////////////////////////////////////////////
// VVOL_BIND_OUTPUT and VVOL_BIND_OUTPUT_LIST structures will specify the output 
// parameters to the IOCTL_SCSITARG_BIND_VVOL ioctl.
// 
// Revision:
//		Format of this structure.
//
// VVolWwn:
//		The WWN of a VVol.
//
// PEWwn:
//      The WWN of the PEs to which the VVol was bound.
//
// SecondaryID
//		Seconday ID (aka HLU) for PE::VVol combination.
//
// Status
//		Status of the bind.
//
// Count:
//      Number of VVOL_BIND_OUTPUT records.
//
///////////////////////////////////////////////////////////////////////////////

typedef struct _VVOL_BIND_OUTPUT {
	
	ULONG			    Revision;
	VVOL_WWN			VVolWwn;
	PE_WWN				PEWwn;
	ULONGLONG			SecondaryID;		
	EMCPAL_STATUS		Status;
	
} VVOL_BIND_OUTPUT, *PVVOL_BIND_OUTPUT;


typedef struct _VVOL_BIND_OUTPUT_LIST {
	
	ULONG				Count;
	ULONG				Revision;
	VVOL_BIND_OUTPUT	Info[1];
	
} VVOL_BIND_OUTPUT_LIST, *PVVOL_BIND_OUTPUT_LIST;

#define CURRENT_VVOL_BIND_OUTPUT_LIST_REV	1


// VVol Bind Output List size macro.

#define VVOL_BIND_OUTPUT_LSIZE(Count)					        \
	(sizeof(VVOL_BIND_OUTPUT_LIST) - sizeof(VVOL_BIND_OUTPUT) +	\
	(Count) * sizeof(VVOL_BIND_OUTPUT))



///////////////////////////////////////////////////////////////////////////////
// VVOL_ENTRY may be read for previously configured VVols.
//
// Revision:
//		Revision of structure.
// 
// VVolWwn:
//		VVol WWN.
// 
// SecondaryId:
//		VVol secondary ID.
// 
// LastIOTime:
//		VVol last IO time in 100nsec since 1601. 
// 
// Reserved:
//		Must be zero.
//
///////////////////////////////////////////////////////////////////////////////

typedef struct _VVOL_ENTRY {
	
	ULONG					Revision;
    VVOL_WWN        		VVolWwn;
    ULONGLONG       		SecondaryId;
	EMCPAL_TIME_100NSECS	LastIoTime;
    PE_WWN          		PEWwn;
	ULONG					Reserved[4];	        // Must be 0.
	
} VVOL_ENTRY, *PVVOL_ENTRY;

//  Current revision level of VVOL_ENTRY.

#define CURRENT_VVOL_ENTRY_REV	1


// An VVOL_ENTRY_LIST structure contains a list of VVol entries, used by IOCTL_SCSITARG_GET_ALL_VVOL_ENTRIES.

typedef struct _VVOL_ENTRY_LIST {
	
	ULONG		    Count;
	ULONG		    Revision;
	VVOL_ENTRY	    Info[1];
	
} VVOL_ENTRY_LIST, *PVVOL_ENTRY_LIST;


// VVol Entry List size.

#define VVOL_ENTRY_LSIZE(VVolCount)				        \
	(sizeof(VVOL_ENTRY_LIST) - sizeof(VVOL_ENTRY) +	    \
	(VVolCount) * sizeof(VVOL_ENTRY))
	

///////////////////////////////////////////////////////////////////////////////
// A VVOL_UNBIND_PARAM structure will specify the input parameters to the 
// IOCTL_SCSITARG_UNBIND_VVOL ioctl. 
// 
// Revision:
//		Format of this structure.
//
// SGWwn:	
//      The WWN of the Storage Group to unbind the VVol(s) from.
//
// VVolPEPair:
//      The WWN of a VVol to be unbound and the WWN of the PE to unbind it from.
//
// UnbindContext:
//		For future use. 
//
// Reserved*:
//		Must be zero.
//
///////////////////////////////////////////////////////////////////////////////

typedef struct _VVOL_UNBIND_PARAM {

	ULONG			    Revision;
	SG_WWN  	        SGWwn;
    ULONG               UnbindContext;                  // create an enum
    ULONG			    Reserved[4];	                // Must be 0.
    ULONG			    Count;
    VVOL_PE_PAIR        VVolPEPair[1];

} VVOL_UNBIND_PARAM, *PVVOL_UNBIND_PARAM;


//  Current revision level of VVOL_UNBIND_PARAM.

#define CURRENT_VVOL_UNBIND_PARAM_REV	1


// VVol Unbind Param size macro.

#define VVOL_UNBIND_PARAM_SIZE(Count)					\
	(sizeof(VVOL_UNBIND_PARAM) - sizeof(VVOL_PE_PAIR) +	\
	(Count) * sizeof(VVOL_PE_PAIR))


#endif
