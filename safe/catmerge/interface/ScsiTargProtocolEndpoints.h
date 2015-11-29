/*******************************************************************************
 * Copyright (C) EMC Corporation, 2015
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * ScsiTargProtocolEndpoints.h
 *
 * This header file contains structures and constants which are needed by other
 * drivers and applications when performing PE (Protocol Endpoint) IOCTLs to TCD.
 *
 ******************************************************************************/

#ifndef ScsiTargProtcolEndpoints_h
#define ScsiTargProtcolEndpoints_h 1


///////////////////////////////////////////////////////////////////////////////
// A PE_PARAM structure will exist for each Protocol Endpoint (PE). This structure 
// defines how a TxD will address and access a PE.
// 
// Note, the contents of this structure are persisted in the PSM database as an XLU.
//
// Revision:
//		Format of this structure.
//
// Key:	
//      The WWN of the PE, which is how it is identified.
//
// DefaultOptimizedSP:
//      Specifies which SP this PE is optimized on by default. (Changeable).
//
// UserDefinedName:
//		LUN Nice Name.
//
// Reserved*:
//		Must be zero.
//
///////////////////////////////////////////////////////////////////////////////

typedef struct _PE_PARAM {

	ULONG			Revision;

#ifndef ALAMOSA_WINDOWS_ENV
    csx_u32_t       Pad;
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */

	PE_WWN			Key;
	SP_ASSIGNMENT	DefaultOptimizedSP;		// Default optimized SP for this PE
	CHAR			UserDefinedName[sizeof(K10_LOGICAL_ID)];  
	ULONG			Reserved[4];	        // Must be 0.

} PE_PARAM, *PPE_PARAM;

//  Current revision level of PE_PARAM
#define CURRENT_PE_PARAM_REV	1


///////////////////////////////////////////////////////////////////////////////
// PE_ENTRY may be read for previously configured PEs (PE XLUs).
//
// Revision:
//		Revision of structure.
//
// CurrentOptimizedSP:
//		SPA : 0, SPB : 1, None : -1
//
// UserSGWwn:
//      WWN of the user SG containing this PE.
//
// Params:
//      Params information about this PE.  Params.Key is unique across PEs.
//
// Reserved:
//		Must be zero.
//
///////////////////////////////////////////////////////////////////////////////

typedef struct _PE_ENTRY {
	
	ULONG			Revision;
	SP_ASSIGNMENT	CurrentOptimizedSP;		// SP that is currently optimized
	SG_WWN			UserSGWwn;			    // User SG that the PE is contained in
	PE_PARAM		Params;                 // Params 
	ULONG			Reserved[4];	        // Must be 0.
	
} PE_ENTRY, *PPE_ENTRY;

//  Current revision level of PE_ENTRY
#define CURRENT_PE_ENTRY_REV	1


// An PE_ENTRY_LIST structure contains a list of PE entries, used by IOCTL_SCSITARG_GET_ALL_PE_ENTRIES.

typedef struct _PE_ENTRY_LIST {
	
	ULONG		Count;
	ULONG		Revision;
	PE_ENTRY	Info[1];
	
} PE_ENTRY_LIST, *PPE_ENTRY_LIST;


// PE Entry List size.

#define PE_ENTRY_LSIZE(PECount)					    \
	(sizeof(PE_ENTRY_LIST) - sizeof(PE_ENTRY) +	    \
	(PECount) * sizeof(PE_ENTRY))


#endif
