#ifndef ADM_API_H
#define ADM_API_H 0x0000001
#define FLARE__STAR__H
/***************************************************************************
 * Copyright (C) EMC Corporation 2001,2004
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  adm_api.h
 ***************************************************************************
 *
 *  Description
 *
 *      This file contains the external interface to the Admin
 *      Interface "adm" module in the Flare driver.
 *
 *      This file only contains the top level of the interface, lower
 *      levels are included indirectly.  DBA interfaces are also
 *      included indirectly.
 *
 *  History:
 *      06/15/01 CJH    Created
 *      11/01/01 HEW    Added ifdefs for C++ compilation
 ***************************************************************************/


#if DBG || defined(UMODE_ENV) || defined(SIMMODE_ENV)
#define ADM_USE_INTERFACE_FUNCTIONS
#endif

#include "adm_sp_api.h"
#include "adm_enclosure_api.h"
#include "adm_unit_api.h"
#include "adm_fru_api.h"
#include "adm_raid_group_api.h"
#include "environment.h"

/************
 * Literals
 ************/

// Used in percentage fields if current percentage is unknown or not
// applicable (valid values are 0-100)
#define ADM_INVALID_PERCENTAGE      0xff
#define ADM_PEER_BIND_PERCENTAGE    0xfe

/*
 * Constants for resumé PROMs
 */
#define ADM_EMC_PART_NUMBER_SIZE        16
#define ADM_ARTWORK_REVISION_SIZE       3
#define ADM_ASSEMBLY_REVISION_SIZE      3
#define ADM_EMC_SERIAL_NO_SIZE          16
#define ADM_VENDOR_PART_NUMBER_SIZE     32
#define ADM_VENDOR_SERIAL_NO_SIZE       32
#define ADM_VENDOR_NAME_SIZE            32
#define ADM_LOCATION_MANUFACTURE_SIZE   32
#define ADM_DATE_MANUFACTURE_SIZE       8
#define ADM_ASSEMBLY_NAME_SIZE          32
#define ADM_MAX_PROGRAMMABLES           15
#define ADM_PROGRAMMABLE_NAME_SIZE      8
#define ADM_PROGRAMMABLE_REV_SIZE       4
#define ADM_MAX_MAC_ADDRESSES           1
#define ADM_MAC_ADDRESS_SIZE            6
#define ADM_SYSTEM_ORIENTATION_SIZE     2

/* ADM_MAX_BUSSES is synonymous with the PHYSICAL_BUS_COUNT and applies 
   to all platforms.  This is a change that corrects defining ADM_MAX_BUSSES
   as PHYSICAL_BUS_COUNT + 1 */
#define ADM_MAX_BUSES PHYSICAL_BUS_COUNT
#define ADM_MAX_ENCLOSURES  (ENCLOSURE_COUNT + 1)

#define ADM_LOCAL_SP_ID ((AM_I_ODD_CONTROLLER) ? ADM_SP_B : ADM_SP_A)
#define ADM_PEER_SP_ID ((AM_I_ODD_CONTROLLER) ? ADM_SP_A : ADM_SP_B)

// Isolated Fault Codes
//  The Flare Fault Isolation system is new for the Bigfoot/AX350 product.
//  See the "Flare Fault Isolation Design and Functional Specification"
//   document (created Jan 2006) for details.
//  These codes are only used on platforms that support Fault Isolation.
//  Within the Isolated Fault Code, bits are set that identify the Code
//   as either Definitive or Ambiguous Fault, and to identify the Code's
//   CRU type.
typedef enum adm_isolated_fault_code_tag
{
  // Isolated Faults are not supported on this CRU
  ADM_ISOLATED_FAULT_UNSUPPORTED                         = 0xFFFFFFFF,

  // Default.. No Isolated Fault on this CRU
  ADM_NO_ISOLATED_FAULT                                  = 0x00000000,

  // Definitive Faults are all AND'd with the 0x100000 mask

  // Definitive LCC Faults
  //   ADM_LCC_IN_PORT_CABLE_ISOLATED_FAULT - NOT in Bigfoot Version 1
  //   ADM_LCC_IN_PORT_CABLE_DEGRRADED_ISOLATED_FAULT - The SAS Cable to
  //      the Input Port has a problem where it is still usable, but results
  //      in decreased IO speeds. Likely a broken SAS Lane.
  //   ADM_LCC_IN_PORT_ISOLATED_FAULT - The Input port of the LCC
  //      is completely failed. The LCC needs to be replaced.
  //      NOT in Bigfoot Version 1
  //   ADM_LCC_OUT_PORT_ISOLATED_FAULT - The Output port of the LCC
  //      is completely failed. The LCC needs to be replaced.
  //      NOT in Bigfoot Version 1
  //   ADM_LCC_IN_PORT_DEGRADED_ISOLATED_FAULT - The Input port of the LCC
  //      has a fault where it cannot drive the connection at full speed.
  //      Likely a broken PHY, and the LCC needs to be replaced.
  //   ADM_LCC_OUT_PORT_DEGRADED_ISOLATED_FAULT - The Output port of the LCC
  //      has a fault where it cannot drive the connection at full speed.
  //      Likely a broken PHY, and the LCC needs to be replaced.
  //   ADM_LCC_DISK_ACCESS_ISOLATED_FAULT - The LCC has an internal failure
  //      that results in it being unable to communicate with one or more
  //      Drives. The LCC needs to be replaced to fix the problem.
  //   ADM_LCC_MISSING_ISOLATED_FAULT - The LCC is NOT inserted into the 
  //      slot. Either it is not set properly, or completely removed.
  //   ADM_LCC_FAILED_ISOLATED_FAULT - This is a generic fault code for when
  //      it is unknown exactly what failed with the LCC, but it is 
  //      Failed Nonetheless. Likely needs replacing.
  //   ADM_LCC_STATUS_MONITOR_ISOLATED_FAULT - The LCC is returning information
  //      but having other issues. With Bigfoot AX350 HW, this basically
  //      translates to a MC problem
  //   ADM_INTERPOSER_INTERFACE_ISOLATED_FAULT - There is a problem with
  //      the LCCs ability to manage the Interposer. On Bigfoot HW, this is
  //      like a TWI (Two-Wire-Interface) failure where the LCC cannot
  //      manage the MUXs on the Interposer.
  //   ADM_LCC_OVERHEATING_ISOLATED_FAULT - There is a temperature control
  //      problem with the LCC, reported by the LCC. Should we only return
  //      this if there is not a concurrent fan fault?
  ADM_LCC_IN_PORT_CABLE_ISOLATED_FAULT                   = 0x01001001,
  ADM_LCC_IN_PORT_CABLE_DEGRADED_ISOLATED_FAULT          = 0x01001002,
  ADM_LCC_IN_PORT_ISOLATED_FAULT                         = 0x01001003,
  ADM_LCC_OUT_PORT_ISOLATED_FAULT                        = 0x01001004,
  ADM_LCC_IN_PORT_DEGRADED_ISOLATED_FAULT                = 0x01001005,
  ADM_LCC_OUT_PORT_DEGRADED_ISOLATED_FAULT               = 0x01001006,
  ADM_LCC_DISK_ACCESS_ISOLATED_FAULT                     = 0x01001007,
  ADM_LCC_MISSING_ISOLATED_FAULT                         = 0x01001008,
  ADM_LCC_FAILED_ISOLATED_FAULT                          = 0x01001009,

  ADM_LCC_STATUS_MONITOR_ISOLATED_FAULT                  = 0x0100100A,
  ADM_LCC_INTERPOSER_INTERFACE_ISOLATED_FAULT            = 0x0100100B,
  ADM_LCC_OVERHEATING_ISOLATED_FAULT                     = 0x0100100C,
  ADM_LCC_FAN_CONTROLLER_ISOLATED_FAULT                  = 0x0100100D,

  ADM_LCC_MISCABLING_LOOP_ISOLATED_FAULT                 = 0x01001010,
  ADM_LCC_MISCABLING_PEER_DOWNSTREAM_ISOLATED_FAULT      = 0x01001011,
  ADM_LCC_MISCABLING_CONNECTED_TO_FOREIGN_ARRAY_ISOLATED_FAULT = 0x01001012,
  ADM_LCC_MULTIPLE_CONNECTIVITY_ISOLATED_FAULT           = 0x01001013,

  ADM_LCC_SP_MISSING_ISOLATED_FAULT                      = 0x01001014,
  // TBD... FAULT CODES FOR HBC / LSI SAS CONTROLLER FAULTS

  // Definitive Interposer Faults
  //   ADM_INTERPOSER_FAILED_ISOLATED_FAULT - This is a generic fault code for when
  //      it is unknown exactly what failed with the Interposer, but it is 
  //      Failed Nonetheless. Likely needs replacing.
  //      An example might be a failed Resume PROM?
  //      This is reported on the Interposer Isolated Fault Code Bubble
  //   ADM_INTERPOSER_MUX_LCC_A_INTERFACE_ISOLATED_FAULT - There is a fault with
  //      the Mux that affects management with the LCC in Slot A. The entire
  //      Interposer needs to be replaced.
  //      This is reported on the per-Mux Isolated Fault Code Bubble.
  //   ADM_INTERPOSER_MUX_LCC_B_INTERFACE_ISOLATED_FAULT - There is a fault with
  //      the Mux that affects management with the LCC in Slot B. The entire
  //      Interposer needs to be replaced.
  //      This is reported on the per-Mux Isolated Fault Code Bubble.
  ADM_INTERPOSER_FAILED_ISOLATED_FAULT                   = 0x01002000,

  // Definitive Mux(Multiplexer) Interposer Faults
  ADM_INTERPOSER_MUX_LCC_A_INTERFACE_ISOLATED_FAULT      = 0x01002001,
  ADM_INTERPOSER_MUX_LCC_A_IO_ISOLATED_FAULT             = 0x01002002,
  ADM_INTERPOSER_MUX_LCC_B_INTERFACE_ISOLATED_FAULT      = 0x01002003,
  ADM_INTERPOSER_MUX_LCC_B_IO_ISOLATED_FAULT             = 0x01002004,

  // Definitive Disk Faults
  //   ADM_DISK_FAILED_ISOLATED_FAULT - The Disk itself is failed and needs
  //      to be replaced.
  //   ADM_DISK_REMOVED_ISOLATED_FAULT - The Disk is Not Inserted
  //   ADM_DISK_SAS_IN_SATA2_RG_ISOLATED_FAULT - The Disk is a SAS drive
  //      that is in the slot that is part of a SATA2 RAID Group.
  //   ADM_DISK_SATA2_IN_SAS_RG_ISOLATED_FAULT - The Disk is a SATA2 drive
  //      that is in the slot that is part of a SAS RAID Group.
  //   ADM_DISK_UNSUPPORTED_ISOLATED_FAULT - The Disk type is not supported.
  //      (i.e. SATA1 drive)
  //   ADM_DISK_SAS_IN_SATA2_SYSTEM_ISOLATED_FAULT - The Disk is a SAS drive
  //      that is in the slot that is part of a SATA2 System RAID Group.
  //      The disk does not contain user RG or user LUs.
  //   ADM_DISK_SATA2_IN_SAS_SYSTEM_ISOLATED_FAULT - The Disk is a SATA2 drive
  //      that is in the slot that is part of a SAS System RAID Group.
  //      The disk does not contain user RG or user LUs.
  ADM_DISK_FAILED_ISOLATED_FAULT                         = 0x01003000,
  ADM_DISK_REMOVED_ISOLATED_FAULT                        = 0x01003001,
  ADM_DISK_SAS_IN_SATA2_RG_ISOLATED_FAULT                = 0x01003002,
  ADM_DISK_SATA2_IN_SAS_RG_ISOLATED_FAULT                = 0x01003003,
  ADM_DISK_UNSUPPORTED_DRIVE_TYPE_ISOLATED_FAULT         = 0x01003004,
  ADM_DISK_IS_WRONG_SIZE_ISOLATED_FAULT                  = 0x01003005,
  ADM_DISK_SAS_IN_SATA2_SYSTEM_ISOLATED_FAULT         = 0x01003006,
  ADM_DISK_SATA2_IN_SAS_SYSTEM_ISOLATED_FAULT         = 0x01003007,

  // Definitive Power Supply Faults
  //   ADM_PS_FAILED_ISOLATED_FAULT - This is a generic fault code for when
  //      it is unknown exactly what failed with the PS, but it is 
  //      Failed Nonetheless. Likely needs replacing.
  //   ADM_PS_REMOVED_ISOLATED_FAULT - The PS is not Inserted into the slot
  //   ADM_PS_SHUTDOWN_ISOLATED_FAULT - The PS is turned off
  //   ADM_PS_AC_FAIL_ISOLATED_FAULT - There is an AC Input problem which
  //      is causing a Power Supply Failure
  //   ADM_PS_xxxBLOWER_ON_SP/LCC_ISOLATED_FAULT - One or more of the fans
  //      on the indicated power supplies have failed.
  ADM_PS_FAILED_ISOLATED_FAULT                           = 0x01004000,
  ADM_PS_REMOVED_ISOLATED_FAULT                          = 0x01004001,
  ADM_PS_SHUTDOWN_ISOLATED_FAULT                         = 0x01004002,
  ADM_PS_AC_FAIL_ISOLATED_FAULT                          = 0x01004003,
  ADM_PS_SINGLE_BLOWER_ON_SP_ISOLATED_FAULT              = 0x01004004,
  ADM_PS_MULTI_BLOWER_ON_SP_ISOLATED_FAULT               = 0x01004005,
  ADM_PS_SINGLE_BLOWER_ON_BOTH_SP_ISOLATED_FAULT         = 0x01004006,
  ADM_PS_MULTI_BLOWER_ON_BOTH_SP_ISOLATED_FAULT          = 0x01004007,
  ADM_PS_SINGLE_BLOWER_ON_LCC_ISOLATED_FAULT             = 0x01004008,
  ADM_PS_MULTI_BLOWER_ON_LCC_ISOLATED_FAULT              = 0x01004009,
  ADM_PS_SINGLE_BLOWER_ON_BOTH_LCC_ISOLATED_FAULT        = 0x0100400A,
  ADM_PS_MULTI_BLOWER_ON_BOTH_LCC_ISOLATED_FAULT         = 0x0100400B,

  // Definitive Fan Pack Faults
  //   ADM_SINGLE_FAN_ON_SP_ISOLATED_FAULT - There is a single fan
  //      failure in only one fan pack within the dual-SP DPE. It is
  //      implied that there are multiple fans within the fanpack, and
  //      the peer SP is up and running. This fault is reported on the
  //      fanpack that has the single fan fault.
  //      Suggested Alert Message - "Single Fan Failure on this SP.
  //                                 If the other SP is removed, the
  //                                 system will automatically shut 
  //                                 down."
  //   ADM_MULTI_FAN_ON_SP_ISOLATED_FAULT - There are one more more fan
  //      failures within one fan pack within the dual-SP DPE. It is
  //      implied that there are multiple fans within the fanpack, and
  //      peer SP is up and running with no fan failures. It is possible
  //      for all of the fans in the fanpack to be failed. This fault is
  //      reported on the fanpack that has the multiple fan faults.
  //      Suggested Alert Message - "Multiple Fan Failures on this SP,
  //                                 and this SP has or will automatically
  //                                 shut down. If the other SP is removed,
  //                                 the system will immediately shut
  //                                 down and data could be lost."
  //   ADM_SINGLE_FAN_ON_BOTH_SP_ISOLATED_FAULT - A single fan has failed
  //      within the fanpack(s) on each SP within the dual-SP DPE. It is
  //      implied that there are multiple fans within the fanpack. This
  //      isolated fault code is reported on the fanpack of EACH SP so
  //      that Navisphere will have each SP's fanpack listed under the
  //      Alert Message.
  //      Suggested Alert Message - "A single fan has failed on each SP.
  //                                 The system must be cleanly shut down
  //                                 to repair. Since there are multiple
  //                                 faults, hot-swapping the fan modules
  //                                 is not possible. If either SP is removed,
  //                                 the system will automatically shut down."
  //   ADM_MULTI_FAN_ON_SP_AND_SINGLE_FAN_ON_PEER_SP_ISOLATED_FAULT -
  //      There is a single fan fault within the fanpack of this SP, while
  //      there are multiple individual fan faults (with Bigfoot, 2) within
  //      the fanpack of the peer SP. It is implied that there are multiple
  //      fans within the fanpack. The total number of individual fan faults
  //      required for automatic complete system shutdown has not yet been met.
  //      This isolated fault code is reported on the fanpack of the SP that
  //      only has the single fan fault, since that SP should not be
  //      automatically shutting down. 
  //      Suggested Alert Message - "A single fan has failed on this SP,
  //                                 and there are multiple fan faults on the
  //                                 peer SP. The system must be cleanly
  //                                 shut down to repair. Since there are
  //                                 multiple faults, hot-swapping the fan
  //                                 modules is not possible. The peer SP has
  //                                 or will be automatically shutdown.
  //                                 If this SP is removed, the system will
  //                                 automatically shut down."
  //   ADM_MULTI_FAN_ON_BOTH_SP_ISOLATED_FAULT - There are fan faults on each
  //      SP, and the maximum total number of individual fan faults required
  //      for automatic complete system shutdown has been met or exceeded. 
  //      Suggested Alert Message - "There are one or more fan faults on each
  //                                 SP, and the maximum total number of
  //                                 individual fan faults required for
  //                                 automatic system shutdown has been met.
  //                                 The system is shutting down."
  //   The LCC fault codes more or less mean the same thing as the SP, except
  //    instead of powering down the entire system or the SP, the Alert Message
  //    should identify that the DPE will be powered down or the LCC.
  ADM_SINGLE_FAN_ON_SP_ISOLATED_FAULT                    = 0x01005000,
  ADM_MULTI_FAN_ON_SP_ISOLATED_FAULT                     = 0x01005001,
  ADM_SINGLE_FAN_ON_BOTH_SP_ISOLATED_FAULT               = 0x01005002,
  ADM_SINGLE_FAN_ON_SP_AND_MULTI_FAN_ON_PEER_SP_ISOLATED_FAULT = 0x01005003,
  ADM_MULTI_FAN_ON_BOTH_SP_ISOLATED_FAULT                = 0x01005004,

  ADM_SINGLE_FAN_ON_LCC_ISOLATED_FAULT                   = 0x01005005,
  ADM_MULTI_FAN_ON_LCC_ISOLATED_FAULT                    = 0x01005006,
  ADM_SINGLE_FAN_ON_BOTH_LCC_ISOLATED_FAULT              = 0x01005007,
  ADM_SINGLE_FAN_ON_LCC_AND_MULTI_FAN_ON_PEER_LCC_ISOLATED_FAULT = 0x01005008,
  ADM_MULTI_FAN_ON_BOTH_LCC_ISOLATED_FAULT               = 0x01005009,

  ADM_BUS_TOO_MANY_ENCLOSURES_ISOLATED_FAULT                           = 0x01006000,
  ADM_BUS_ASYMMETRICAL_ENCLOSURE_ORDERING_UNSUPPORTED_ISOLATED_FAULT   = 0x01006001,


  // Ambiguous Faults are all AND'd with the 0x200000 mask

  // Ambiguous LCC Faults, related to Connectivity between LCCs
  //   ADM_LCC_CABLE_ELSE_SELF_ELSE_UPSTREAM_LCC_ISOLATED_FAULT -
  //      The Input cable is likely broken or removed and needs to be repaired.
  //      Could also potentially be the LCC the fault is reported on, or the
  //      the Upstream LCCC 
  ADM_LCC_CABLE_ELSE_SELF_ELSE_UPSTREAM_LCC_ISOLATED_FAULT= 0x02001001,
  ADM_LCC_SELF_ELSE_UPSTREAM_LCC_ELSE_CABLE_ISOLATED_FAULT= 0x02001002, // UNLIKELY in Bigfoot V1
  ADM_LCC_SELF_ELSE_UPSTREAM_LCC_ISOLATED_FAULT           = 0x02001003, // UNLIKELY in Bigfoot V1
  ADM_LCC_SELF_ELSE_SOMETHING_DOWNSTREAM_ISOLATED_FAULT   = 0x02001004, // NOT in Bigfoot V1
  ADM_LCC_CABLE_ELSE_SELF_ELSE_UPSTREAM_LCC_DEGRADED_ISOLATED_FAULT= 0x02001005,

  // The following fault is returned on an LCC. It identifies a problem with
  //  either the LCC (in paticular the Status Monitor, or the MC) or the 
  //  Interposer. This is probably an error with one LCC being able to access
  //  the Interposer Resume, while the peer LCC has no problem accessing the 
  //  Interposer Resume.
  ADM_LCC_MANAGEMENT_CONTROLLER_ELSE_INTERPOSER_ISOLATED_FAULT = 0x0200100B,

  // Ambiguous LCC Faults, related to LCC to Disk Access
  ADM_LCC_DISK_ELSE_SELF_ELSE_MUX_ISOLATED_FAULT          = 0x02001006,
  ADM_LCC_SELF_ELSE_MUX_ISOLATED_FAULT                    = 0x02001007,
  ADM_LCC_MUX_ELSE_SELF_ISOLATED_FAULT                    = 0x02001008,
  ADM_LCC_SELF_ELSE_MUX_INTERFACE_ISOLATED_FAULT          = 0x02001009,
  ADM_LCC_DISK_ELSE_SELF_ISOLATED_FAULT                   = 0x0200100A,
  ADM_LCC_SELF_ELSE_INTERPOSER_ISOLATED_FAULT             = 0x0200100B,


  // Ambiguous Interposer Faults
  ADM_INTERPOSER_MUX_ELSE_SELF_ISOLATED_FAULT             = 0x02002000
}
ADM_ISOLATED_FAULT_CODE;

#define ADM_DEFINITIVE_ISOLATED_FAULT_CODE_MASK         0x01000000
#define ADM_AMBIGUOUS_ISOLATED_FAULT_CODE_MASK          0x02000000


#define OLD_ADM_OFD_FILENAME_LENGTH 30  // The previous value is needed to keep the admin object happy.
#define ADM_OFD_FILENAME_LENGTH     80  // The filename length is also set in FDF_FILENAME_LEN, in gm_install_export.h.

// The filename length being used for Encryption Backup
// Matching size to KeyManager
#define ADM_ENCRYPTION_CONFIG_FILE_NAME_LENGTH     128 

/************
 *  Types
 ************/

// This structure is used to return error information for ADM IOCTLs
typedef struct adm_status_struct
{
    UINT_32 condition_code;
    UINT_32 sunburst_error;
}
ADM_STATUS;

/***************************
 *  Macro Definitions
 ***************************/


/********************************
 *  Function Prototypes
 ********************************/

#if defined(__cplusplus)
#define CALL_TYPE __stdcall
extern "C"
{
#else
#define CALL_TYPE
#endif

// Returns the size needed for the poll header buffer. The poll header buffer must be allocated
// by the client.
UINT_32 CALL_TYPE  dyn_get_poll_header_size(void);

// The client must issue the IOCTL to get the poll data into "buffer", then call
// this routine to return the handle used for other calls.  The client must have
// allocated poll_header_buffer to be the size returned by dyn_get_poll_header_size().
OPAQUE_PTR CALL_TYPE  dyn_get_handle( VOID *buffer, UINT_32 buf_size, VOID * poll_header_buffer);

// returns the number of user LUNs
UINT_32 CALL_TYPE  dyn_lun_count(OPAQUE_PTR handle);

// returns the number of private (SP-only, Flare-only, L2 cache) LUNs
UINT_32 CALL_TYPE  dyn_private_lun_count(OPAQUE_PTR handle);

// returns the number of licensed FRUs for platform
UINT_32 CALL_TYPE  dyn_licensed_fru_count(OPAQUE_PTR handle);

// returns the number of raid groups
UINT_32 CALL_TYPE  dyn_raid_group_count(OPAQUE_PTR handle);

// returns the number of private RAID groups
UINT_32 CALL_TYPE  dyn_private_raid_group_count(OPAQUE_PTR handle);

// returns status of statistics logging
BOOL CALL_TYPE  dyn_stats_enabled(OPAQUE_PTR handle);

// returns status of power saving statistics logging
BOOL CALL_TYPE  dyn_power_saving_stats_enabled(OPAQUE_PTR handle);

// return error status of poll
ADM_STATUS CALL_TYPE  dyn_poll_status(OPAQUE_PTR handle);

// return faulted status for the CMI channel
BOOL CALL_TYPE dyn_cmi_faulted(OPAQUE_PTR handle);

// returns the current speed of the bus
UINT_32 CALL_TYPE dyn_bus_current_speed(OPAQUE_PTR handle, UINT_32 bus);

//returns a bitmask of capable speeds of the bus
UINT_32 CALL_TYPE dyn_bus_capable_speeds(OPAQUE_PTR handle, UINT_32 bus);

//returns TRUE if there is a duplicate enclosure detected on the bus
BOOL CALL_TYPE dyn_bus_duplicate_enclosure(OPAQUE_PTR handle, UINT_32 bus);

//returns TRUE if bus specified has max enclosures exceeded.
BOOL CALL_TYPE dyn_bus_max_enclosures_exceeded(OPAQUE_PTR handle, UINT_32 bus);

// returns the number of back end buses on the array
UINT_32 CALL_TYPE dyn_num_be_buses(OPAQUE_PTR handle);

csx_pchar_t CALL_TYPE dyn_GBB_tier_level(OPAQUE_PTR handle);

UINT_32 CALL_TYPE dyn_GBB_max_enclosures(OPAQUE_PTR handle);

BOOL CALL_TYPE dyn_GBB_SAS_supported(OPAQUE_PTR handle);

UINT_32  CALL_TYPE dyn_bus_isolated_fault_code(
    OPAQUE_PTR handle, UINT_32 bus);

// TRUE if the LCC has a definitive isolated fault
BOOL  CALL_TYPE dyn_bus_has_definitive_isolated_fault(
    OPAQUE_PTR handle, UINT_32 bus);

// TRUE if the LCC has anambiguous isolated fault
BOOL  CALL_TYPE dyn_bus_has_ambiguous_isolated_fault(
    OPAQUE_PTR handle, UINT_32 bus);

UINT_32 CALL_TYPE dyn_bus_get_num_current_enclsoures(OPAQUE_PTR handle, UINT_32 bus);
UINT_32 CALL_TYPE dyn_bus_get_max_enclosures(OPAQUE_PTR handle, UINT_32 bus);


// Returns whether or not a second SP is supported in the array.
extern
BOOLEAN admDualEnabledHardware(void);

#if defined(__cplusplus)
#define CALL_TYPE __stdcall
}
#endif
#endif /* ADM_API_H */

