/*******************************************************************************
 * Copyright (C) EMC Corporation, 2003 - 2007
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * ScsiTargSystemOptions.h
 *
 * This header file contains structures and constants which are needed by other
 * drivers and applications when performing System Options IOCTLs to TCD.
 *
 ******************************************************************************/

#ifndef ScsiTargSystemOptions_h
#define ScsiTargSystemOptions_h 1

#include "model_numbers.h"
#include "BasicLib/BasicPerformanceControlInterface.h"

typedef struct _SYSCONFIG_OPTIONS {

	INITIATOR_TYPE	system_type;
	ULONG			system_intfc_options;
	UCHAR			fairness_alg_18;		// From mode page 37 parameter 18
	UCHAR			fairness_alg_19;		// From mode page 37 parameter 19
	USHORT			uuid_base_address;		// For Tru64 UUID Generation.

} SYSCONFIG_OPTIONS, *PSYSCONFIG_OPTIONS;

// HOST_SYSTEM_OPTIONS structure is used to pass system information
// to/from TCD (mode page 37 stuff typically).  This data is to be 
// saved in PSM. Some of it is used by TCD and some is simply saved and
// provided on request.

typedef struct _HOST_SYSTEM_OPTIONS {

	ULONG				Revision;
	SYSCONFIG_OPTIONS	sysconfig;
	SG_WWN				default_va;
	BasicPerformancePolicy PerformancePolicy;
	UINT_32				Reserved[111];

} HOST_SYSTEM_OPTIONS, *PHOST_SYSTEM_OPTIONS;

// If the following line asserts at compile time, recalculate the size of 
// BasicPerformancePolicy in UINT_32s, figure out how much the multiplier in the
// following line of code needs to be changed, and subtract the difference from
// the Reserved field in HOST_SYSTEM_OPTIONS.  ie. if the multiplier below is 1
// and new size of BasicPerformancePolicy is 3 UINT_32s, then 3 - 1 = 2 so subtract
// 2 from the above Reserved field and set multiplier below to 3.
CSX_ASSERT_AT_COMPILE_TIME_GLOBAL_SCOPE(sizeof(BasicPerformancePolicy) == (sizeof(UINT_32) * 5));

// The size of HOST_SYSTEM_OPTIONS has changed.  This is persisted in PSM and thus
// if the size changes, code must be changed to convert the PSM storage and the
// revision must be bumped.  If you are trying to make use of additional Reserved
// words and not change the size of the overall structure then your calculations
// are off and the Reserved field must be recalculated.
CSX_ASSERT_AT_COMPILE_TIME_GLOBAL_SCOPE(sizeof(HOST_SYSTEM_OPTIONS) == (sizeof(UINT_32) * 124));

#define CURRENT_HOST_SYSTEM_OPTIONS_REV	5

// This is the structure that is actually saved in PSM
// holding the system type and options.

typedef struct _HOST_SAVED_OPTIONS {

	ULONG					Revision;
	HOST_SYSTEM_OPTIONS		options;

} HOST_SAVED_OPTIONS, *PHOST_SAVED_OPTIONS;

// Values for system options flags
// Bits 0-7, and 8-15 below are chosen to be compatible with 
// Host Interface Options,  Parameter 13,Value 0 and Value 1 of Mode Page 0x37

// These bits are settable on a per-initiator basis

#define SYSTEM_BSY_BIT				    0x00000001

// These bits define the Dual SP Auto Trespass behavior.

#define SYSTEM_DUAL_SP_BEHAVIOR_BITS	  0x0E000000 
#define SYSTEM_DUAL_SP_BEHAVIOR_LUN_BASED 0x00000000  // Let LUN select trespass behavior.
#define SYSTEM_DUAL_SP_BEHAVIOR_DMP       0x08000000  // Always DMP behavior for this initiator
#define SYSTEM_DUAL_SP_BEHAVIOR_FORCE_MT  0x0C000000  // Always Manual trespass for this initiator
#define SYSTEM_DUAL_SP_BEHAVIOR_PNR       0x04000000  // Always passiveNotReady behavior for this initiator
#define SYSTEM_DUAL_SP_BEHAVIOR_FORCE_AT  0x02000000  // Always auto trespass for this initiator
#define SYSTEM_DUAL_SP_BEHAVIOR_PAR		  0x0E000000  // PassiveAlwaysReady behaviour for this initiator
#define SYSTEM_DUAL_SP_BEHAVIOR_ALUA      0x06000000  // Active/Active behaviour for this initiator

#define OBSOLETE_SYSTEM_ATR_BIT			  0x40000000  // Obsoleted Auto Trespass bit

#define SYSTEM_LUN_Z_BIT                0x00000400 // Always report a LUN_Z
#define SYSTEM_LUN_SN_IN_VPP80_BIT          0x00000800    // Report LUN SN instead of array SN in VPP 80

// MUST BE 0 without more analysis/code.  A vaule of 1 implies no NACA support.
#define SYSTEM_ACA_TAG_TESTING_BIT        0x00001000  // 1=> allow control byte setting to cause
						      //     SCSI tag to be set to odd values
						      // Used during DVT/Unit test only...

// This bit is used to identify a COV (cooperating virtualizer) initiator type. 
// RPA (RecoverPoint Appliance) is a COV. 

#define SYSTEM_COOPERATING_VIRTUALIZER  0x00002000

#define SYSTEM_VALID_PER_INITIATOR_OPTIONS (SYSTEM_BSY_BIT|SYSTEM_DUAL_SP_BEHAVIOR_BITS|SYSTEM_LUN_Z_BIT| \
	    SYSTEM_LUN_SN_IN_VPP80_BIT|SYSTEM_ACA_TAG_TESTING_BIT|SYSTEM_COOPERATING_VIRTUALIZER)

// These bits are internal options bits that are a direct function of initiator type.
// They cannot be modified directly.

#define SYSTEM_PG8_BIT				    0x00000002 
#define SYSTEM_RCE_BIT				    0x00000004     // Historical
#define SYSTEM_SCSI3_BIT			    0x10000000

#define SYSTEM_DEFAULT_BITS (SYSTEM_RESTR_BIT | SYSTEM_FC_SOFT_ADDR_BIT | SYSTEM_SCSI3_BIT | \
								SYSTEM_DUAL_SP_BEHAVIOR_ALUA | SYSTEM_LUN_Z_BIT | SYSTEM_LOG_SPA | SYSTEM_LOG_SPB)

// These bits are independent of initiators, and can only be set on a port or system wide basis.

#define SYSTEM_FC_SOFT_ADDR_BIT		    0x00000020

#define SYSTEM_LOG_SPA			        0x20000000
#define SYSTEM_LOG_SPB			        0x00000008

#define SYSTEM_RESTR_BIT			    0x00000010     // Does not affect host-side

// This bit should not be set that the system level.  It indicates that 
// this level in the options heirarchy does not define the options, and
// therefore the options should be acquired from the next level.
// The lowest level is session, then I_T nexus, then Initiator, then
// host, then system (global).

#define USE_DEFAULT_OPTIONS             0x80000000

#define SYSTEM_AUTO_ASSOCIATION_BIT		0x00000040

// !!! Do not use 0x40000000 for a system option bit  !!!
//	   It has been obsoleted above and must not be used again

// Some useful masks for Admin to manage the options bits
//
//   Used when deriving current system type from options bits

#define NON_TYPE_SPECIFIC_BITS		(SYSTEM_LOG_SPA | \
									 SYSTEM_LOG_SPB | \
									 OBSOLETE_SYSTEM_ATR_BIT | \
									 SYSTEM_LUN_Z_BIT| \
									 SYSTEM_LUN_SN_IN_VPP80_BIT | \
									 SYSTEM_AUTO_ASSOCIATION_BIT)


//   This lists all the new and historical individual PG37 bits.
//   It's used to validate an Admin PG37 mode select.

#define ALL_INDIVIDUAL_PG37_BITS	(SYSTEM_BSY_BIT | \
									 SYSTEM_PG8_BIT | \
									 SYSTEM_RCE_BIT | \
									 SYSTEM_RESTR_BIT | \
									 SYSTEM_FC_SOFT_ADDR_BIT | \
									 SYSTEM_LUN_Z_BIT| \
                                     SYSTEM_LUN_SN_IN_VPP80_BIT)

//  This lists the page 37 host interface options bits
//  we allow setting individually

#define SETTABLE_PG37_BITS			(SYSTEM_BSY_BIT | \
									 SYSTEM_LUN_Z_BIT| \
                                     SYSTEM_LUN_SN_IN_VPP80_BIT)


// Define Structures for the generic NON_PERSISTED_SYSTEM_INFO block.
// This is overall system info that is NOT saved in the PSM database
// (and hence does not need NDU consideration)
//
// NON_PERSISTED_SYSTEM_INFO TYPES:
//     The following are the query types supported.
// 

typedef enum non_persisted_system_info_type
{
	 MODEL_INFO = 1,
	 STATS_INFO = 2

} NON_PERSISTED_SYSTEM_INFO_TYPE;

typedef struct _NON_PERSISTED_SYSTEM_INFO {

	NON_PERSISTED_SYSTEM_INFO_TYPE InfoType;

	union
    {
        /* 
         * Retrieve the System Identifier Information
         */
        struct {
			CHAR		AsciiModelName[K10_ASCII_MODEL_NUMBER_MAX_SIZE]; 
			CHAR		AsciiPlatformName[K10_ASCII_MODEL_NUMBER_MAX_SIZE]; 
			CHAR		Reserved[48];
        }
        ID_info;

        /* 
         * Retrieve the non-persisted System Statistics information
         */
        struct {
            ULONG   Stats_Timestamp;  /* Running time (in seconds) at which statistics was last cleared */
            ULONG   Current_Timestamp; /* Current running time (in seconds) of system */
	    } STATS_info;  /* Statistics information */

   } I_tag;         /* Information block tag union */

} NON_PERSISTED_SYSTEM_INFO, *PNON_PERSISTED_SYSTEM_INFO;

# endif
