#ifndef DBA_EXPORT_TYPES_H
#define DBA_EXPORT_TYPES_H 0x0000001
#define FLARE__STAR__H
/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  dba_export_types.h
 ***************************************************************************
 *
 *      Description
 *
 *         This file contains the definitions for types used by the portion 
 *         of the interface to the
 *         Database Access "dba" library in the Flare driver that is used
 *         outside of Flare.
 *
 *  History:
 *
 *      01/21/08 JED    Split from dba_exports.h
 *      10/02/01 LBV    Add fru_flag ODBS_QUEUED_SWAP
 *      10/17/01 CJH    move defines from dba_api.h
 *      11/01/01 HEW    Added ifdefs for C++ compilation
 *      04/17/02 HEW    Added CALL_TYPE to decl for dba_frusig_get_cache
 ***************************************************************************/

#include "generics.h"
#include "k10defs.h"
#include "cm_config_exports.h"
#include "vp_exports.h"
#include "bgs_exports.h" // for BGS Monitor types


/************
 *  Types   *
 ************/
typedef UINT_32 TIMESTAMP;
typedef UINT_32 REVISION_NUMBER;

/************
 * Literals *
 ************/

/*** SYSCONFIG PAGE STUFF ****/

/* Length definitions for arrays in the SysConfig Page.
 */
#define SYSTEM_ID_FULL_LENGTH   16  // bytes allocated for System ID
#define SYSTEM_ID_LENGTH        15  // bytes used for System ID
#define OLD_SYSTEM_ID_LENGTH    12  // bytes used for System ID on previous products

#define PRODUCT_PN_LENGTH       16  // bytes allocated for product part number
#define PRODUCT_REV_LENGTH       3  // bytes allocated for product rev
/*** END SYSCONFIG PAGE STUFF ***/

/*-------------------------
 * LUN REBUILD LIMITS
 *  DIMS 177488 - The rebuild time is now (has always been)
 *      entered as a priority:  ASAP, HIGH, MEDIUM or LOW.
 *      In the past the priority corresponded to exactly the
 *      rebuild time in seconds.  The changes for this dims
 *      defines the rebuild_time passed from Admin as a priority
 *      and Flare converts that to a rebuild time in seconds.
 *-------------------------*/
#define LU_REBUILD_PRIORITY_ASAP    0
#define LU_REBUILD_PRIORITY_HIGH    6
#define LU_REBUILD_PRIORITY_MED     12
#define LU_REBUILD_PRIORITY_LOW     18
#define LU_REBUILD_PRIORITY_MAX     31
#define LU_MIN_REBUILD_PRIORITY     (LU_REBUILD_PRIORITY_ASAP)
#define LU_MAX_REBUILD_PRIORITY     (LU_REBUILD_PRIORITY_MAX)
#define LU_DEFAULT_REBUILD_PRIORITY (LU_REBUILD_PRIORITY_HIGH)

/*-------------------------
 * LUN background VERIFY LIMITS
 *  DIMS 179755 - The verify time is now (has always been)
 *      entered as a priority:  ASAP, HIGH, MEDIUM or LOW.
 *      In the past the priority corresponded to exactly the
 *      verify time in hours.  The changes for this dims
 *      renames the LUN_BIND_INFO verify_time to verify_priority
 *      and uses routines/macros to translate between the two.
 *-------------------------*/
#define LU_VERIFY_PRIORITY_ASAP     0
#define LU_VERIFY_PRIORITY_HIGH     6
#define LU_VERIFY_PRIORITY_MED      12
#define LU_VERIFY_PRIORITY_LOW      18
#define LU_VERIFY_PRIORITY_MAX      254
#define LU_MIN_VERIFY_PRIORITY      (LU_VERIFY_PRIORITY_ASAP)
#define LU_MAX_VERIFY_PRIORITY      (LU_VERIFY_PRIORITY_MAX)
#define LU_DEFAULT_VERIFY_PRIORITY  (LU_VERIFY_PRIORITY_MED)

#define LU_ASAP_ZERO_THROTTLE_RATE  0    //The defaulted background zeroing rate 0GB/Hr
#define LU_MIN_ZERO_THROTTLE_RATE   4    //The defaulted background zeroing rate 4GB/Hr
#define LU_MED_ZERO_THROTTLE_RATE   6    //The defaulted background zeroing rate 6GB/Hr
#define LU_MAX_ZERO_THROTTLE_RATE   12   //The defaulted background zeroing rate 12GB/Hr

#define LU_DEFAULT_ZERO_THROTTLE_RATE LU_MIN_ZERO_THROTTLE_RATE

// A system ID is a 16 byte quantity that's a unique identifier for the chassis
typedef struct _system_id
{
    TEXT idString[SYSTEM_ID_FULL_LENGTH];
} SYSTEM_ID, *SYSTEM_ID_PTR;

// The product part number
typedef struct _product_pn
{
    TEXT productPn[PRODUCT_PN_LENGTH];
} PRODUCT_PN, *PRODUCT_PN_PTR;

// The product revision
typedef struct _product_rev
{
    TEXT productRev[PRODUCT_REV_LENGTH];
} PRODUCT_REV, *PRODUCT_REV_PTR;

typedef struct _product_fields_struct
{
    SYSTEM_ID   prod_sn;
    PRODUCT_PN  prod_pn;
    PRODUCT_REV prod_rev;
    BOOL sn_valid;
    BOOL pn_valid;
    BOOL rev_valid;
} PRODUCT_FIELDS_STRUCT;



/************
 *  Types   *
 ************/


/* bits that can be set in fru state
 */

typedef enum _fru_state_T
{
    PFSM_STATE_REMOVED     = 0x000,
    PFSM_STATE_FAULTED     = 0x001,
    PFSM_STATE_POWERING_UP = 0x002,
    PFSM_STATE_DEFINED     = 0X003,
    PFSM_STATE_READY       = 0x004
}fru_state_T;

/***************************************************************************
 *
 * DESCRIPTION
 *    A probational drive can be in one of the following states.
 *
 ***************************************************************************/
typedef enum _PROBATIONAL_DRIVE_STATE
{
    PD_IDLE,            /* FRU is not marked for probation */
    PD_INIT,            /* FRU is the PD in init state */
    PD_ACTIVE,          /* FRU is the PD in Active state */
    PD_POWERUP,         /* FRU is the PD in powerup state */
    PD_STOPPING,        /* FRU is the PD in stopping state */
    PD_CANCEL           /* FRU is the PD and probation is
                         * being cancelled
                         */
}PROBATIONAL_DRIVE_STATE;

/***************************************************************************
 *
 * DESCRIPTION
 *    The reason why the drive is put in probation.
 *
 ***************************************************************************/
typedef enum _PD_REASON

{
    PD_REASON_NO_FAULT = 0,   
    PD_REASON_DH_EXT_TIMEOUT = 1,
    PD_REASON_POWER_CYCLE_CMD = 2,
    PD_REASON_POWER_OFF_CMD = 3,
    PD_REASON_RESET_CMD = 4,
    PD_REASON_REQ_BYPASS = 5,
    PD_REASON_FW_DOWNLOAD = 6
}PD_REASON;

/***************************************************************************
 *
 * DESCRIPTION
 *    The fault type that fru stability code handles.
 *
 ***************************************************************************/
typedef enum pd_stability_fault_tag
{
   FRU_STABILITY_ASSERTED_PBC = 0,
   FRU_STABILITY_DH_EXT_TIMEOUT,
   FRU_STABILITY_FAULT_TYPE_MAX
}FRU_STABILITY_FAULT_TYPE;

/***************************************************************************
 *
 *  Online Firmware Download info
 *     
 ***************************************************************************/

/***************************************************************************
 *
 * DESCRIPTION
 *    A drive in online fw download can be in one of the following states.
 *
 ***************************************************************************/
typedef enum _DRIVE_FW_DL_STATE
{
    OFD_FRU_IDLE,            /* FRU is not marked for download */
    OFD_FRU_WAITING,         /* FRU is waiting for download */
    OFD_FRU_INIT,            /* FRU is in download init state */
    OFD_FRU_ACTIVE,          /* FRU is in download active state */
    OFD_FRU_POWERUP,         /* FRU is in download powerup state */
    OFD_FRU_COMPLETE,        /* FRU is in download complete state */
    OFD_FRU_FAIL,            /* FRU fails in download */
    OFD_FRU_WAITING_FOR_NTMIRROR,  /* FRU is waiting for NTmirror rebuild */
} DRIVE_FW_DL_STATE;

/***************************************************************************
 *
 * DESCRIPTION
 *    A drive in fw download can fail with one of the following reasons.
 *
 ***************************************************************************/
typedef enum _DRIVE_FW_DL_FAIL_REASON
{
    OFD_FRU_FAIL_NO_ERROR,             /* FRU has been successful so far  */
    OFD_FRU_FAIL_NONQUALIFIED,         /* FRU  is in a non-redundant RG */
    OFD_FRU_FAIL_NO_PROBATION,         /* FRU is not in probation */
    OFD_FRU_FAIL_WRITE_BUFFER,         /* FRU fails in WRITE BUFFER command */
    OFD_FRU_FAIL_POWER_UP,             /* FRU fails to power up */
    OFD_FRU_FAIL_RETRY_EXCEED,         /* FRU waits too long */
    OFD_FRU_FAIL_WRONG_REV,            /* Firmware Revision is not correct */
    OFD_FRU_FAIL_CREATE_LOG,           /* FRU fails to create rebuild log */
}   DRIVE_FW_DL_FAIL_REASON;


/***************************************************************************
 *
 * DESCRIPTION
 *    Drive death reason. (NOTE: Enum value should not be more than 0xFFFF)
 *
 ***************************************************************************/
typedef enum _DRIVE_DEAD_REASON
{
    DRIVE_DEATH_UNKNOWN = -1,               /* Drive death reason unknown */
    DRIVE_ALIVE,                            /* Drive is alive */
    DRIVE_PHYSICALLY_REMOVED,               /* Drive physically removed */
    DRIVE_PEER_REQUESTED,                   /* Peer requested to power down */
    DRIVE_ASSERTED_PBC,                     /* Drive PBC asserted */
    DRIVE_USER_INITIATED_CMD,               /* User initiated command (simulate) for drive removal */
    DRIVE_ENCL_MISSING,                     /* Enclosure missing */
    DRIVE_ENCL_FAILED,                      /* Enclosure failed */
    DRIVE_PACO_COMPLETED,                   /* Proactive copy completed */
    DRIVE_UNSUPPORTED,                      /* Unsupported drive */
    DRIVE_UNSUPPORTED_PARTITION,            /* Drive unsupported partition */
    DRIVE_FRU_ID_VERIFY_FAILED,             /* FRU ID verify failed */
    DRIVE_FRU_SIG_CACHE_DIRTY_FLG_INVALID,  /* CACHE dirty flag is invalid */
    DRIVE_ODBS_REQUESTED,                   /* ODBS has requested for drive removal */
    DRIVE_DH_REQUESTED,                     /* DH has requested for drive removal */
    DRIVE_LOGIN_FAILED,                     /* Drive failed to login */
    DRIVE_BE_FLT_REC_REQUESTED,             /* BE fault recovery has requested for drive removal */
    DRIVE_ENCL_TYPE_MISMATCH,               /* Drive mismatch detected */
    DRIVE_FRU_SIG_FAILURE,                  /* FRU signature was incorrect - in some instances, we'll
                                             * fault the drive - ie) proactive copy in progress */
    DRIVE_USER_INITIATED_POWER_CYCLE_CMD,   /* User initiated drive power cycle command */
    DRIVE_USER_INITIATED_POWER_OFF_CMD,     /* Uset initiated drive power off command */
    DRIVE_USER_INITIATED_RESET_CMD,         /* Uset initiated drive reset off command */
    DRIVE_ENCL_COOLING_FAULT,               /* When there is a multi-blower fault, power supply physically removed
                                             * or ambient overtemp */
    DRIVE_TYPE_RG_SAS_IN_SATA2,             /* A SAS drive is inserted into a SATA2 raid group */
    DRIVE_TYPE_RG_SATA2_IN_SAS,             /* A SATA2 drive is inserted into a SAS raid group */
    DRIVE_TYPE_SYS_SAS_IN_SATA2,            /* A SAS drive is inserted into a SATA2 system raid group */
    DRIVE_TYPE_SYS_SATA2_IN_SAS,            /* A SATA2 drive is inserted into a SAS system raid group */

    DRIVE_TYPE_SYS_MISMATCH ,
    DRIVE_TOO_SMALL,                        /* Drive is too small for the raid group */
    DRIVE_TYPE_UNSUPPORTED,                 /* The drive is of the wrong technology (e.g. SATA1 in a SAS/SATA2 systems) */
    DRIVE_TYPE_RG_MISMATCH,                 /* Generic mismatch for catch-all */
    DRIVE_FW_DOWNLOAD,                      /* Drive is dead to download firmware */
    DRIVE_SERIAL_NUM_INVALID,               /* The drive had an invalid serial number */
    DRIVE_PRICE_CLASS_MISMATCH,
    DRIVE_PACO_NO_USER_LUN                 /* The Drive has no user Luns bound. Since no Paco can start on this, need to poweroff this fru. */

} DRIVE_DEAD_REASON;


/* HOST FRU states are listed below.  These states indicate
 * which state (from the host's perspective) we have placed it in
 *
 * The host fru states were changed to be defined by 16 bits.  With
 * the addition of LUN partitions, it is now possible for a fru to
 * be in multiple states.  DPB
 *
 * NOTE: We will try to keep the internal flare fru states and the host
 *       fru states in synch as much as possible.
 */

typedef enum _fru_host_state_T
{
  HOST_FRU_OFF_STATE                = 0x0000,   /* Fru present, but power is off */
  HOST_FRU_REMOVED_STATE            = 0x0001,   /* Fru has been removed */
  HOST_FRU_POWERUP_STATE            = 0x0002,   /* Fru is powering up */
/* POWERED                     0x0004 */
/* POWERUP_WAIT                0x0008 */
  HOST_FRU_FORMAT_STATE             = 0x0010,   /* Fru is formatting */
  HOST_FRU_BINDING_STATE            = 0x0020,   /* Fru is binding */
/* FRU_IDLE                    0x0020 */
  HOST_FRU_BIND_STATE               = 0x0040,   /* Fru is bound, but not assigned */
  HOST_FRU_ASSIGN_STATE             = 0x0080,   /* Fru is bound and assigned */
  HOST_FRU_ENABLED_STATE            = 0x0100,   /* Fru is enabled */
  HOST_FRU_REBUILD_STATE            = 0x0200,   /* Fru is being rebuilt */
  HOST_FRU_EQUALIZE_STATE           = 0x0400,   /* Fru is equalizing */
/* POWERUP_DENIED              0x0400 */
  HOST_FRU_UNSUPPORTED_STATE        = 0x0800,   /* This type of disk is not allowed
                                                 * for the currently configured
                                                 * system type
                                                */
  HOST_FRU_UNFORMATTED_STATE        = 0x1000,   /* FRU needs format */
  HOST_FRU_EXPANDING_STATE          = 0x2000,   /* Fru is part of a raid group
                                                 * that is expanding.
                                                 */
  HOST_FRU_HS_STANDBY_STATE         = 0x4000,   /* Hotspare is ready */
  HOST_FRU_ZEROING_STATE            = 0x8000,   /* Disk is being zeroed */
  HOST_FRU_PROBATION_STATE          = 0x10000,  /* FRU is in probation */
  HOST_FRU_PROACTIVE_COPYING_STATE  = 0x20000,   /* FRU is proactive copying */
  HOST_FRU_FOREIGN_BROKEN_STATE = 0x40000,
  HOST_FRU_CRITICAL_STATE    = 0x80000,
  HOST_FRU_MISSING_STATE   = 0x100000,
} fru_host_state_T;


/* FRU based flags - bits that can be set in FRU flags */
typedef enum _fru_fru_flags_T
{
    FRU_INITIALIZED                         = 0x0001,       // Fru survived init.
    FRU_BASE_SIGNATURE_OK                   = 0x0002,       // Fru base FRU SIGNATURE OK flag
    CM_UNSUPPORTED_DRIVE                    = 0x0004,       // Fru inserted but unsupported
    NEEDS_PROACTIVELY_SPARED                = 0x0008,       // used as precondition for proactive sparing.
    EXPANSION_FRU                           = 0x0010,       // This is an expansion fru
    FRU_NO_RESPONSE                         = 0x0020,       // No response on FRU_SIG read
    NEEDS_DB_REBUILD                        = 0x0040,       // Fru DB needs to be rebuilt

    /* 0x0080 not used */

    ODBS_QUEUED_SWAP                        = 0x0100,       // A fru was swapped while another
                                                            // ODBS event was being processed
    FRU_POWERUP_DONE                        = 0x0200,       // replaces powerup_done field
    HOT_SPARE_FRU                           = 0x0400,       // Fru is to be used for hot sparing
    
    /* 0x0800 not used */
    
    FRU_DH_REMOVED_DRIVE                    = 0x1000,       // DH removed the drive
                                                            // when the cache was unstable
                                                            // so force LCC code to check
                                                            // for re-insert
    ODBS_EVENT_IN_PROGRESS                  = 0x2000,       // Indicates that the CM has a
                                                            // request outstanding to ODBS
    ODBS_QUEUED_REMOVE                      = 0x4000,       // A FRU was removed while another
                                                            // ODBS event was being processed
    ODBS_QUEUED_INSERT                      = 0x8000,       // A FRU was inserted while another
                                                            // ODBS event was being processed
    ODBS_QUEUED_DELETE_ZERO_MARK                            // A FRU zero mark was set and
                                            = 0x10000,      // needs to be deleted
    ODBS_QUEUED_INVALIDATE                  = 0x20000,      // A ODBS FRU invalidate while another
                                                            // ODBS event was being processed

    FRU_NR_IN_PROGRESS                      = 0x40000,      // NR in progress on the FRU.
    /* Unused 0x80000 */
    /* Unused 0x100000 */
    /* Unused 0x200000 */
    FRU_DH_REQUESTED_DRIVE_REMOVAL          = 0x400000,     // DH has requested this drive removed
    FRU_CM_REQUESTED_DRIVE_REMOVAL          = 0x800000,     // CM has requested this drive removed
    FRU_ODBS_REQUESTED_DRIVE_REMOVAL        = 0x1000000,    // ODBS has requested this drive removed
    FRU_BE_REC_REQUESTED_DRIVE_REMOVAL      = 0x2000000,    // BE recovery has requested this drive removed
    FRU_MISMATCH_REQUESTED_DRIVE_REMOVAL    = 0x4000000,    // Drive mismatch detected

    FRU_MARKED_FOR_DB_UPDATE_AT_INIT        = 0x8000000,
    // It is marked by cm_db when it finds that
    // db is committed, but a FRU entry is not upgraded.
    // It is then used by cm_init_read_databases() to flush the db entry
    // after all reads complete.

    FRU_NEEDS_DISK_ZERO                     = 0x10000000,   // Disk zeroing has been requested for this disk;
                                                            // flag is cleared when zeroing has completed
    /* Unused 0x20000000 */
    FRU_DH_PERSISTENT_DRIVE_REMOVAL         = 0x40000000,   // DH corner case failure

    FRU_PRICE_CLASS_MISMATCH_REQUESTED_DRIVE_REMOVAL = 0x80000000   // The price class of the drive is incompatible

} fru_fru_flags_T;


#define PERSISTENT_FRU_FRU_FLAGS (HOT_SPARE_FRU | \
                                  NEEDS_DB_REBUILD)
// EXPANSION_FRU is removed from Persistent flag set. This is made non-persistent,
// Updates occur: 
//      - When RG table is read and/or updated with expansion info 
//      - During expansion command processing, expansion completion processing.



#define NON_PERSISTENT_FRU_FRU_FLAGS (~(PERSISTENT_FRU_FRU_FLAGS))

/*-------------------------------------------------------------------------
 * FRU_NP_FLAGS - for non persistent flags only.
 *-------------------------------------------------------------------------*/
typedef enum fru_np_flags_T
{
    // This is an ***internal*** flag used in DB Update Manager.
    // This should not be accessed from or used by any other code.
    // This is used to keep track of GLUT updates.
    FRU_DB_UPDATE_MANAGER_FLAG   = 0x00000001,
    
    // Non-Persistent Bit to indicate that the Fru powerdown is due to Hotspare/PACO SwapIn/Out.
    FRU_HS_PS_SWAP_OP_FLAG       = 0x00000002
    
}FRU_NP_FLAGS_T;

/* bits that can be set in fru partition flags */
typedef enum _fru_partition_flags_T
{
    NEEDS_REBUILD               = 0x0001,   /* Fru partition needs to be REBUILT. */
    WAIT_FRU_NEEDS_REBUILD      = 0x0010,   /* Coordinating w/ peer for needs-rbld */
    PEER_MARKED_NEEDS_RBLD      = 0x0020,   /* The peer told us to mark needs rbld */
    WAIT_PEER_FRU_NEEDS_REBUILD = 0x0040,   /* Peer is running needs rebuild */

    FRU_ZEROING                 = 0x0080,
    FRU_BLOCK_CONTINUE          = 0x0200,
    FRU_SIGNATURE_OK            = 0x0400,
    FRU_REBUILD_COMPLETING      = 0x0800,    /* Set while glut table is writing to DB for rebuild */
    FRU_NR_COMPLETING           = 0x1000     /* Set while glut table is writing to DB for NR      */
} fru_partition_flags_T;

/* Bits that can be set in the probational drive flags field
 * of a FRU table.
 */
typedef enum _PD_FLAGS
{
    PD_NO_FLAGS     = 00,
    PD_SNIFF_PASSED = 01
}
PD_FLAGS;


#define PERSISTENT_GLUT_FRU_FLAGS (NEEDS_REBUILD)

#define NON_PERSISTENT_GLUT_FRU_FLAGS (~(PERSISTENT_GLUT_FRU_FLAGS))

/* Spare fru types, persistent in the raid group table, non-persistent in the fru table */
typedef enum _spare_fru_types_T
{
    FRU_NO_SPARE            = 0x0,      /* Fru is not used for sparing */
    FRU_HOT_SPARING         = 0x1,      /* Fru is hot sparing */
    FRU_PROACTIVE_SPARING   = 0x2,      /* Fru is proactive sparing */
} spare_fru_types_T;

/*---------------------------------------------------------------
 * VP_FLAGS
 *
 *    This field describes the state of the Verify Process (VP)
 *    for this RAID Group.
 *--------------------------------------------------------------*/


typedef enum _vp_flags_T
{
     RG_BACKGROUND_VERIFY          = 0x0001,
     RG_SNIFF_VERIFY               = 0x0002,
     RG_PEER_VERIFY                = 0x0004

} vp_flags_T;

/*---------------------------------------------------------------
 * RAID_GROUP_STATE_FLAGS
 *
 *      This field describes the state of the RAID Group.
 *--------------------------------------------------------------*/

typedef enum _raid_group_flags_T
{
     RAID_GROUP_STATE_MASK   = 0x00FF,
        /* Masks off the valid bits in the RAID Group state field.
         */

     RG_INVALID              = 0x0000,
        /* Indicates that this RAID Group has not been created yet.
         */

     RG_EXPLICIT_REMOVAL     = 0x0001,
        /* Indicates that the RAID Group will need to be explicitly removed
         * after the last lun is unbind in the RAID Group.
         */

     RG_VALID                = 0x0002,
        /* Indicates that this is a valid RAID Group.
         */

     RG_EXPANDING            = 0x0004,
        /* Indicates that the RAID Group is currently expanding.
         */

     RG_DEFRAGMENTING        = 0x0008,
        /* Indicates that the RAID Group is currently defragmenting.
         */
     RG_BUSY                 = 0x0020,
        /* Indicates that the RAID Group is doing some operation that
         * effects the databases.  No other processes that use the
         * databases can occur when this flag is set.
         */

     RG_ZEROING             = 0x0040,
        /* RG is zeroing the expansion disks */

     RG_FCLI_REQUESTED_BUSY     = 0x8000         
} raid_group_flags_T;



//Persistent flags persist in the CM database.
#define PERSISTENT_RAID_GROUP_FLAGS (RG_EXPLICIT_REMOVAL | RG_VALID | \
                                     RG_EXPANDING | RG_DEFRAGMENTING | RG_ZEROING)

#define NON_PERSISTENT_RAID_GROUP_FLAGS (RAID_GROUP_STATE_MASK & ~(PERSISTENT_RAID_GROUP_FLAGS))


//
// Moved from cm_raid_group.h
//


/*---------------------------------------------------------------
 * STATUS_FLAGS
 *
 *      This field describes some general flags for
 *  this RAID Group.
 *
 * RG_BUSY_CONTROL_ODD - This tracks the last SP
 *   to successfully assign a LUN in this raid group.
 *
 * RG_STATUS_FLAGS_NONE - This flag will be used to cleare 
 * status_flags. ARS : 459799.
 *--------------------------------------------------------------*/

typedef enum _raid_status_flags_T
{
    RG_STATUS_FLAGS_NONE            = 0x0000,
    RG_EXPANSION_HALTED_NUSED       = 0x0001,
    RG_EXPANSION_XL_FLAG_NUSED      = 0x0002,
    RG_EXPANSION_ZEROING_NUSED      = 0x0004,
    RG_EXPANSION_XL_ZEROING_NUSED   = 0x0008,
    RG_BUSY_CONTROL_ODD             = 0x0010,
    RG_FULLY_BOUND                  = 0x0020,
    RG_SHUTDOWN                     = 0x0040,
    RG_SUPPORT_DB_LOCKING           = 0x0080,
    RG_BGZERO_HALTED                = 0x0100,
    RG_MARKED_FOR_DB_UPDATE_AT_INIT = 0x0200,
    // It is marked by cm_db when it finds that
    // db is committed, but a RG entry is not upgraded.
    // It is then used by cm_init_read_databases() to flush the db entry
    // after all reads complete.
    
} raid_status_flags_T;

#define PERSISTENT_RAID_STATUS_FLAGS ( \
    RG_FULLY_BOUND | RG_SUPPORT_DB_LOCKING)

#define NON_PERSISTENT_RAID_STATUS_FLAGS (~(PERSISTENT_RAID_STATUS_FLAGS))

/*----------------------------------------------------------------
 * ATTRIBUTES
 *  To indicate the various arrtibutes of the RAID GROUP
 *
 * RG_SYSTEM - the Raid group resides in private space and can
 *             not be seen by the user.
 * RG_USER   - the Raid group is visible to the user.
 * RG_SATA_KLONDIKE  - Indicates that the RAID Group is created on a Serial
 *                     ATA Drives on Klondike enclosure.
 * RG_FIBRE_CHANNEL  - Indicates that the RAID Group is created on a
 *                     Fibre Channel Drives
 * RG_SATA_NORTHSTAR - Indicates that the RAID Group is created on
 *                     NORTH STAR Drives
 * RG_SATA_ON_SAS    - Indicates that the RAID group is created on a Serial
 *                     ATA Drives on SAS bus.
 * RG_PRIVATE - The Raid group was flagged as private on creation.
 *
 *----------------------------------------------------------------*/

typedef enum _raid_attributes_T
{
    RG_SYSTEM           = 0x0001,
    RG_USER             = 0x0002,
    RG_SATA_KLONDIKE    = 0x0004,
    RG_FIBRE_CHANNEL    = 0x0008,
    RG_SATA_NORTHSTAR   = 0x0010,
    RG_PRIVATE          = 0x0020,
    RG_FSSD             = 0x0040,
    RG_SAS              = 0x0080,
    RG_SATA_ON_SAS      = 0x0100
} raid_attributes_T;

/* The following definitions are special values describing the location of the
 * maximum contiguous free space within a raid group.  These values are returned
 * in the location parameter of cm_rg_free_cont_capacity().
 */
#define RG_CONTIG_LOC_NONE   0
#define RG_CONTIG_LOC_BEGIN  1
#define RG_CONTIG_LOC_MIDDLE 2
#define RG_CONTIG_LOC_END    3

/* DIMS 242252 -Added RG_SATA_ON_SAS and RG_SAS */
#define RG_DRIVE_TYPES ( RG_SATA_KLONDIKE | RG_FIBRE_CHANNEL | RG_SATA_NORTHSTAR | RG_SATA_ON_SAS| RG_SAS| RG_FSSD)

/* The following definitions are special values used to indicate
 * that the particular field has no valid entry.
 */
#define INVALID_UNIT             UNSIGNED_MINUS_1
#define INVALID_UNIT_TYPE        UNSIGNED_MINUS_1
#define INVALID_FRU              UNSIGNED_MINUS_1
#define INVALID_STATE            UNSIGNED_MINUS_1
#define INVALID_BV_CHKPT         UNSIGNED_MINUS_1
#define INVALID_RAID_GROUP       UNSIGNED_MINUS_1
#define INVALID_PARTITION_INDEX  (UINT_16E)UNSIGNED_MINUS_1
#define INVALID_FRUSIG_TIMESTAMP (UINT_16E)UNSIGNED_MINUS_1
#define INVALID_POSITION         UNSIGNED_MINUS_1
#define INVALID_INDEX            UNSIGNED_MINUS_1

/* The following definitions are special values used to indicate
 * that the particular field indicates all the the specified item
 * should be included in the action.
 */
#define ALL_FRUS                 UNSIGNED_MINUS_2               

/* The following defines the logical partition checkpoint when the partition 
 * is fully rebuilt.
 */
#define LOGICAL_PARTITION_END    0xFFFFFFFFFFFFFFFF

typedef struct _UNIT_CACHE_PARAMS
{
    UINT_32 priority_lru_length;
    // % of cache dedicated to the read cache lru

    UINT_16E prefetch_segment_length;
    // Segment size used for prefetching

    UINT_16E prefetch_total_length;
    // Total size that should be prefetched

    UINT_16E prefetch_maximum_length;
    // Maximum size that a prefetch can be

    UINT_16E prefetch_disable_length;
    // Do not prefetch if requests are larger than this

    BITS_8E cache_config_flags;
    // Various cache related flags

    UINT_8E read_retention_priority;
    // see mode page 8

    UINT_8E cache_idle_threshold;
    // Number of requests that make a unit busy (non-idle)

    UINT_8E cache_idle_delay;   /* */ /* 1 byte */
    // Time (sec) that a unit must be idle
    // before idle time activity starts

    UINT_32 cache_write_aside;
    // I/O block size above which requests write around the cache

    UINT_32 cache_dirty_word;
    // indicates if the cache currently has dirty pages.

    UINT_32 prefetch_idle_count;
    // ??

    UINT_32 L2_cache_dirty_word;
    // CACHE_DIRTY indicates that there is L2 cache info in memory
    // for this LUN

    UINT_32 read_be_req_size;
    // read_be_req_size forces all read cache requests below a certain size
    // to a standard size

} UNIT_CACHE_PARAMS;

typedef enum _LU_STATE
{
     LU_UNBOUND                       = 0x0000,
     LU_BIND                          = 0x0001,
     LU_BROKEN                        = 0x0002,
     LU_READY                         = 0x0003,
     LU_ASSIGN                        = 0x0004,
     LU_ENABLED                       = 0x0005,
     LU_DEGRADED                      = 0x0006,
     LU_SHUTDOWN_FAIL                 = 0x0007,
     LU_SHUTDOWN_TRESPASS             = 0x0008,
     LU_SHUTDOWN_UNBIND               = 0x0009,
     LU_SHUTDOWN_INTERNAL_ASSIGN      = 0x000A,
     LU_UNBIND                        = 0x000B,
     LU_PEER_BIND                     = 0x000C,
     LU_PEER_ASSIGN                   = 0x000D,
     LU_PEER_ENABLED                  = 0x000E,
     LU_PEER_DEGRADED                 = 0x000F,
     LU_PEER_SHUTDOWN_FAIL            = 0x0010,
     LU_PEER_SHUTDOWN_TRESPASS        = 0x0011,
     LU_PEER_SHUTDOWN_UNBIND          = 0x0012,
     LU_PEER_SHUTDOWN_INTERNAL_ASSIGN = 0x0013,
     LU_PEER_UNBIND                   = 0x0014
}  LU_STATE;


/*-------------------------------------------------------------------------
 * STATUS
 *
 *  Describes the bind status of the unit.
 *-------------------------------------------------------------------------*/

/* These status flags are used to determine which controller is
 * controlling the bind.
 */
typedef enum _LU_STATUS
{
     LU_BOUND_BY_EVEN_CONTROLLER = 0x0001,
     LU_BOUND_BY_ODD_CONTROLLER  = 0x0002,
     LU_PEER_BIND_NO_CM_ELEMENT  = 0x0004,
     LU_UNBINDING_QUEUED         = 0x0008
} LU_STATUS;

// ??? Why 2 flags, one for even and other for odd? Are they not mutually exclusive ?
#define PERSISTENT_LU_STATUS_FLAGS (LU_BOUND_BY_EVEN_CONTROLLER | LU_BOUND_BY_ODD_CONTROLLER)
#define NON_PERSISTENT_LU_STATUS_FLAGS (~(PERSISTENT_LU_STATUS_FLAGS))

/*---------------------------------------------------------------------------
 * LU_ATTRIBUTES
 *
 *  This value describes the unit's assignment permissions (such as the
 *  default owner, auto assign and auto trespass permission for example).
 *  It also describes the unit type in terms of who can access it.
 *  (The DEFAULT ASSIGN, AUTO ASSIGN, and AUTO TRESPASS definitions come
 *   from the old assign_perm field which is now replaced by the
 *   lu_attributes field.  The LU type definitions are new for LU Centric.)
 *
 *  NOTE: This field exists in both the LOGICAL_UNIT AND
 *  BIND_INFO structures!  Use these defines for both.
 *------------------------------------------------------------------------*/

#define LU_DEFAULT_ASSIGN_MASK         0x0000FFFF
#define LU_DEFAULT_ASSIGN_EVEN_SP      0x0
#define LU_DEFAULT_ASSIGN_ODD_SP       0x1

#define LU_AUTO_ASSIGN_MASK            0x00010000
#define LU_AUTO_ASSIGN_ENABLE          0x0
#define LU_AUTO_ASSIGN_DISABLE         0x10000

#define LU_AUTO_TRESPASS_MASK          0x00020000
#define LU_AUTO_TRESPASS_ENABLE        0x20000
#define LU_AUTO_TRESPASS_DISABLE       0x0

#define LU_AUTO_TRESPASS_LOCK_BIT      0x00040000

/* This mask bit only mask the above bits.
 */
#define LU_ATTRIBUTES_ASSIGN_PERM_MASK 0x00070001

/* Defines the accessibility for an LU.
 */
#define LU_GENERAL                     0x00100000
#define LU_SP_ONLY                     0x00200000
#define LU_FLARE_ONLY                  0x00400000

/* Defines the visibility (user space
 * vs private space) for an LU.
 */
#define LU_USER                        0x00800000
#define LU_SYSTEM                      0x01000000

// More Defines
// Unit may be assigned on both SPs.
#define LU_ATTR_DUAL_ASSIGNABLE        0x02000000

// Unit was flagged as private on creation
#define LU_PRIVATE                     0x04000000

// Unit is a Celerra Control LUN (VCX)
// These 4 bits make up a 4-bit field representing the suggested HLU to use
// all 0's -> not a Celerra LUN
// 0001 -> Celerra LU 0
// 0010 -> Celerra LU 1
// 0011 -> Celerra LU 2
// 0100 -> Celerra LU 3
// 0101 -> Celerra LU 4
// 0110 -> Celerra LU 5
// others -> currently unused
#define LU_VCX0                        0x10000000
#define LU_VCX1                        0x20000000
#define LU_VCX2                        0x40000000
#define LU_VCX3                        0x80000000


#define LU_ATTR_VCX_0                  LU_VCX0
#define LU_ATTR_VCX_1                  LU_VCX1
#define LU_ATTR_VCX_2                  (LU_VCX0 | LU_VCX1)
#define LU_ATTR_VCX_3                  LU_VCX2
#define LU_ATTR_VCX_4                  (LU_VCX0 | LU_VCX2)
#define LU_ATTR_VCX_5                  (LU_VCX1 | LU_VCX2)

#ifdef C4_INTEGRATED
#define LU_ATTR_MORPHEUS               (LU_VCX0 | LU_VCX1 | LU_VCX2)
#endif /* C4_INTEGRATED - C4ARCH - admin_topology */

#define LU_ATTRIBUTES_VCX_MASK         0xF0000000
#define LU_ATTRIBUTES_VCX_SHIFT        28

typedef enum _LU_LUN_TYPE
{
    LU_LUN_TYPE_NON_FLARE,
    LU_LUN_TYPE_FLARE,
    LU_LUN_TYPE_CELERRA
} LU_LUN_TYPE;

/* Enum used with dba_unit_get_fed_priority() */
/* Enum used with dba_unit_get_fed_priority() */
typedef enum _DBA_FED_UNIT_PRIORITY
{
    LU_NORMAL_NT_PRIORITY,
    LU_HIGH_NT_PRIORITY,
    LU_NO_NT_ACCESS,
} DBA_FED_UNIT_PRIORITY;

/* Enum used with dba_unit_get_fed_priority() */
typedef enum _DBA_SWAP_RQ_TYPE
{
    HS_RQ = 1,
    PS_RQ = 2,
    HS_PS_RQ = 3,
} DBA_SWAP_RQ_TYPE;


typedef enum _LU_TYPE
{
     UNIT_TYPE_MASK    =  0x00fff000,
     /* masks off the valid bits in the unit type field
     */

     LU_RAID5_DISK_ARRAY = 0x1000,
    /* LU_RAID5_DISK_ARRAY indicates that the unit is a N+1 Disk Array.
     * NOTE - LU_DISK_ARRAY unit type actually refers to a RAID5 disk array.
     * In initial revs of flare, this unit type was named as such. Starting
     * Rev 5, we will have a LU_RAID3_DISK_ARRAY but we are keeping
     * LU_DISK_ARRAY to refer to RAID5 arrays to avoid having to
     * update all the numerous files that use the old name. In
     * future, we'll try to use LU_RAID<x>_DISK_ARRAY to refer to
     * array of RAIDx type.
     */

     LU_DISK = 0x2000,
    /* Indicates that the unit is an individually addressable disk.
     */

    /* LU_SCSI = 0x4000, OBSOLETE */
    /* Indicates that the unit is a SCSI target.  CDB's will be
     * passed from the host's device driver directly to the SCSI target
     * unit.
     */

     LU_STRIPED_DISK = 0x8000,
    /* Indicates that the unit is a striped collection
     * of disks.  NO PARITY is kept on this type of array.  (if it can
     * be called that.  In this type of orginization, if a disk failure
     * occurs, then the data is history.  NO RECOVERY IS POSSIBLE.
     */

     LU_DISK_MIRROR = 0x10000,
    /* Indicates that the unit is made up of two identical
     * disk drives.  Each drive contains a mirror image of the data
     * contained on the other.  In the case of failure of one unit,
     * data can be recovered from the other.
     */

     LU_HOT_SPARE = 0x20000,
    /* Indicates that the unit is a hot spare, free to take over
     * for any failed FRU.
     */

    /* LU_RAID3_DISK_ARRAY = 0x40000, OBSOLETE */

     LU_RAID10_DISK_ARRAY = 0x80000,
    /* Indicates that the unit is bound as RAID 10 array
     * type (striped mirrors).
     */

     LU_FAST_RAID3_ARRAY = 0x100000,
    /* Indicates that the unit is bound as RAID 3 array
     * type (parallel access array).
     * THE NEW RAID-3
     */

     LU_HI5_DISK_ARRAY = 0x200000,
    /* Indicates that the unit is bound as a HI5 array
     * type (Log-based RAID consisting of a RAID 5 store
     * and a L2 cache implemented as a Log).
     */

     LU_HI5_LOG = 0x400000,
     /* Indicates that the unit is a Log for a HI5 array
     * type.
     */

     LU_RAID6_DISK_ARRAY = 0x800000
     /* This is a Raid 6 unit type, which is a parity unit
      * and a striped unit.
      */
}  LU_TYPE;   //REM


/* Bit definitions of the "flare_characteristics_flags" word. The
 * Flare characteristics flags is a 32-bit quantity that is used to describe
 * the characteristics of the CLARiiON box independently of the host.
 */
   /* auto-format drives on startup enabled/disabled */
#define SYSCONFIG_AUTO_FORMAT_BIT               0x0001

   /* Vault Fault Override switch enabled/disabled */
#define SYSCONFIG_VAULT_FAULT_OVERRIDE          0x0002

//
// This is set by the conversion tool(XP--Hammer) and SP does certain operations based on this flag.
// When the conversion related tasks are done and database is synced up,
// this is reset at commit.
// This is a persistent bit.
//
#define SYSCONFIG_CONV_IN_PROGRESS              0x0004

// Flag to define if the flag to define user ability to control of the
// enable/disable state of sniff verify.
#define SYSCONFIG_USER_SNIFF_CONTROL_ENABLED    0x0008

// Flag bits defining the platform-type. This occupies 4 bits covering
//  15 possible platform types.
// When CONV_IN_PROGRESS is set, it indicates the platform type
//       prior to conversion
// When CONV_IN_PROGRESS flag bit is not set, it indicates the current platform type.
//  Platform type Enums are defined in line below.
#define SYSCONFIG_PLATFORM_TYPE_MASK            0x00F0

#define SYSCONFIG_MARKED_FOR_DB_UPDATE_AT_INIT  0x00000100
    // It is marked by cm_db when it finds that
    // db is committed, but a SYSCONFIG entry is not upgraded.
    // It is then used by cm_init_read_databases() to flush the db entry
    // after all reads complete.

/* WCA enabled/disabled */
#define SYSCONFIG_WCA_STATE                     0x00000200

/* enable power savings statistics logging */
#define SYSCONFIG_PWR_SAVING_STATS_LOGGING_ENABLED  0x00000400

/* enable power savings. */
#define SYSCONFIG_PWR_SAVING_ENABLED  0x00000800


    // It is marked by cm_db when it finds that
    // db is committed, but a SYSCONFIG entry is not upgraded.
    // It is then used by cm_init_read_databases() to flush the db entry
    // after all reads complete.

#define PERSISTENT_SYSCONFIG_FLAGS                              \
       (SYSCONFIG_AUTO_FORMAT_BIT |                             \
        SYSCONFIG_VAULT_FAULT_OVERRIDE |                        \
        SYSCONFIG_CONV_IN_PROGRESS |                            \
        SYSCONFIG_USER_SNIFF_CONTROL_ENABLED |                  \
        SYSCONFIG_PLATFORM_TYPE_MASK |                          \
        SYSCONFIG_WCA_STATE |                                   \
        SYSCONFIG_PWR_SAVING_STATS_LOGGING_ENABLED |            \
        SYSCONFIG_PWR_SAVING_ENABLED)


#define NON_PERSISTENT_SYSCONFIG_FLAGS (~(PERSISTENT_SYSCONFIG_FLAGS))

/* Platform Type enumerations
 * Note that this enumeration defines values withing
 * SYSCONFIG_PLATFORM_TYPE_MASK bits.
 */
typedef enum _sysconfig_platform_type
{
    SYSCONFIG_PT_NONE = 0x00,
    SYSCONFIG_PT_NT = 0x10,
    SYSCONFIG_PT_FISH = 0x20,
    SYSCONFIG_PT_HAMMER = 0x30,
    SYSCONFIG_PT_FLEET = 0x40,
    SYSCONFIG_PT_MAMBA  = 0x50
}
SYSCONFIG_PLATFORM_TYPE;

/***************************************************************************
 *      Generic Unit Type Mask
 *
 *      These masks relate the generic unit type.
 ***************************************************************************/


#define LU_RAW_ARRAY      (LU_DISK | LU_STRIPED_DISK)
#define LU_PARITY_ARRAY   (LU_HI5_DISK_ARRAY |        \
                           LU_HI5_LOG |               \
                           LU_FAST_RAID3_ARRAY |      \
                           LU_RAID5_DISK_ARRAY |      \
                           LU_RAID6_DISK_ARRAY)
#define LU_BANDWIDTH_ARRAY   LU_FAST_RAID3_ARRAY
#define LU_MIRRORED_ARRAY    (LU_DISK_MIRROR| LU_RAID10_DISK_ARRAY)
#define LU_STRIPED_ARRAY_MASK  (LU_RAID5_DISK_ARRAY |    \
                                LU_RAID10_DISK_ARRAY |   \
                                LU_HI5_DISK_ARRAY |      \
                                LU_HI5_LOG |             \
                                LU_STRIPED_DISK |        \
                                LU_FAST_RAID3_ARRAY |    \
                                LU_RAID6_DISK_ARRAY)


/*----------------------------------------------------------------------
 *  LU_FLAGS_T enum now changes to #defines
 *
 *  Available for any special statuses or indicators regarding
 *  the unit. Put Background verify flags here as well (they no
 *  longer have their own special field).
 *----------------------------------------------------------------------*/

/*-------------------------------------------------------------------
 *  When a unit needs a background verify performed, NEEDS_BV
 *  remains set until the background verify completes successfully.
 *
 *  To maintain backward compatability, NEEDS_BV must remain 0x02.
 *-------------------------------------------------------------------*/

#define RESTART_BV      0x00000001  // Restart BG Verify from LU's
                                               // starting LBA
#define NEEDS_BV             0x00000002  // BG Verify is needed
#define LU_WAIT_SNIFF_CMPLT  0x00000004  // Indicates that this unit
                                               // was in the process of
                                               // ASSIGNING but must wait

#define CA_INVALIDATED_UNIT   0x00000008
#define LU_SFE_SHUTDOWN_DONE          0x00000010

// Following flag is made obsolete, since the concept of session_id per LUN used
  // during assign time is removed.
#define CM_GIVE_UNIT_NEW_SESSION_ID_OBSOLETE     0x00000018
#define VP_SNIFFING_ENABLED              0x00000040
#define LU_USER_ABORT_BIND              0x00000080
#define LU_NO_BG_VERIFY_ON_BIND        0x00000100
#define LU_ASSIGN_QUEUED                 0x00000200
#define LU_MARKED_FOR_DB_UPDATE_AT_INIT 0x00000400
  // It is marked by cm_db when it finds that
  // db is committed, but a GLUT entry is not upgraded.
  // It is then used by cm_init_read_databases() to flush the db entry
  // after all reads complete.

// Copied this from original enum
// LU_VERIFY_STOPPING              = 0x00000800,

#define READONLY_BV                     0x00001000  // Read-only BG Verify
#define LU_SWAP_BEGUN                   0x00002000
#define LU_BIND_QUEUED          0x00004000
#define LU_UNIT_WRITTEN         0x00008000
#define LU_UNIT_MAYBE_DIRTY_PEER        0x00010000
#define LU_UNIT_L2_MAYBE_DIRTY_PEER     0x00020000
// Note: LU_EXPANDING (0x00040000) was used in R28 and cannot be reused until we no longer support R28
#define LU_DISABLED                     0x00080000
#define LU_RESERVED                     0x00100000
#define LU_CHANGING_BIND_PARAMS         0x00200000
#define LU_DUAL_ASSIGNABLE              0x00400000
// Note: LU_UNBIND_IS_PENDING is now replaced with LU_HOST_UNBIND_IN_PROGRESS
#define LU_HOST_UNBIND_IN_PROGRESS      0x00800000
#define LU_FRESHLY_BOUND_FLAG           0x01000000
// Unit may be assigned on both SPs.
 // DIMS 193918 - LU_UNBINDING is being replaced by LU_UNBINDING_DB_UPDATE
 // since that is what it used for during Unbind.
#define LU_UNBINDING_DB_UPDATE          0x02000000
// A capacity change notification is required
#define LU_CAPACITY_NOTIFY_NEEDED       0x04000000
// DIMS 193918 - LU_PEER_UNBINDING becomes obsolete. 
  // Note: LU_UNBINDING_DB_UPDATE will be used instead.
  // LU_PEER_UNBINDING            = 0x08000000,
#define LU_UNBIND_INITIATE_SWAP_OUT     0x08000000
// Introduced in the fix for 125281 - used only for dual-assignable units
#define LU_PEER_ASSIGNING               0x10000000
#define LU_ZERO_ON_DEMAND               0x20000000
#define LU_CHANGE_ZERO_RATE             0x40000000
#define LU_ZERO_FILL_OP                 0x80000000
  /* shrink flags */
  //LU Shrink Internal capacity pending notification flag
#define LU_INT_CAPACITY_CHANGE_PENDING  0x100000000
#define LU_ZOD_BD_SHRINKING             0x200000000

/* Flag indicating the unbind is initiated from peer SP */
#define LU_HOST_PEER_UNBIND_IN_PROGRESS 0x400000000

#define PERSISTENT_LU_FLAGS (RESTART_BV | NEEDS_BV | VP_SNIFFING_ENABLED | READONLY_BV | LU_CAPACITY_NOTIFY_NEEDED | \
                             LU_ZERO_ON_DEMAND | LU_NO_BG_VERIFY_ON_BIND | LU_INT_CAPACITY_CHANGE_PENDING | LU_ZOD_BD_SHRINKING)

#define NON_PERSISTENT_LU_FLAGS (~(PERSISTENT_LU_FLAGS))

/*-------------------------------------------------------------------------
 * LU_NP_FLAGS - for non persistent flags only as an extension of the above
 * flags (both persistent and non-persistent) that are set in the glut.
 * These flags will be set in the GLUT np_flags field.
 *-------------------------------------------------------------------------*/
typedef enum lu_np_flags_T
{
    LU_RECEIVED_TE              = 0x00000001,
    LU_FAULT_NOT_DEGRADED       = 0x00000002,

    // This is an ***internal*** flag used in DB Update Manager.
    //  This should not be accessed from or used by any other code.
    //  This is used to keep track of GLUT updates.
    LU_DB_UPDATE_MANAGER_FLAG   = 0x00000004,
    LU_FAULT_DRIVE_IN_PUP       = 0x00000008,

    // Unit with invalid zero map detected.
    LU_INVALID_ZERO_MAP         = 0x00000010,
}LU_NP_FLAGS_T;

/*-------------------------------------------------------------------
 * CACHE_CONFIG_FLAGS -
 *
 *  NOTE: This field exists in BOTH the LOGICAL_UNIT AND the
 *  BIND_INFO structures!!  Use these defines for both.
 *
 *------------------------------------------------------------------*/
#define LU_PREFETCH_TYPES_MASK             0x000C

#define LU_READ_CACHE_ENABLED              0x0001
#define LU_WRITE_CACHE_ENABLED             0x0002
#define LU_CONSTANT_PREFETCH               0x0004
#define LU_MULTIPLICATIVE_PREFETCH         0x0008
#define LU_HI5_LOG_ENABLED                 0x0010
#define LU_ALWAYS_PREFETCH                 0x0020
#define LU_READ_ENTIRE_PAGE                0x0040
#define LU_ELEMENT_CUTOFF                  0x0080


/*-------------------------------------------------------------------
 * CACHE_PAGE_SIZE
 *
 *  Allowable cache page sizes (in blocks)...
 *  Note, the hashing function requires that the page size
 *  be a power of 2.
 *------------------------------------------------------------------*/

#define LU_CACHE_PAGE_SIZE_2K           4
#define LU_CACHE_PAGE_SIZE_4K           8
#define LU_CACHE_PAGE_SIZE_8K          16
#define LU_CACHE_PAGE_SIZE_16K         32
#define LU_CACHE_PAGE_SIZE_DEFAULT     (LU_CACHE_PAGE_SIZE_8K)

/*-------------------------------------------------------------------
 * DRIVE CLASS INFORMATION
 *
 *  The defines for Drive Class and Enclosure Drive Class are
 *  shared.
 *
 *------------------------------------------------------------------*/
#define DRIVE_CLASS_UNKNOWN        0
#define DRIVE_CLASS_FC             1
#define DRIVE_CLASS_ATA_KLONDIKE   2
#define DRIVE_CLASS_SATA           3
#define DRIVE_CLASS_ATA_NORTHSTAR  4
#define DRIVE_CLASS_SAS            5 // Added for SAS drive.
#define DRIVE_CLASS_FC_FSSD        6
#define DRIVE_CLASS_SATA_FSSD      7
#define DRIVE_CLASS_SATA_FLASH     8
#define DRIVE_CLASS_SAS_FLASH      9
#define DRIVE_CLASS_SAS_NL         10
#define DRIVE_CLASS_SAS_FLASH_VP   11

/*-------------------------------------------------------------------
 * DRIVE SPEED INFORMATION
 *
 *  The defines for Drive Speed and Enclosure Speed are
 *  shared.
 *
 *------------------------------------------------------------------*/

#define DRIVE_SPEED_UNKNOWN        0
#define DRIVE_SPEED_1GB            1
#define DRIVE_SPEED_2GB            2
#define DRIVE_SPEED_4GB            3
#define DRIVE_SPEED_3GB            4 /*SATA*/
#define DRIVE_SPEED_6GB            5 /*SAS*/
#define DRIVE_SPEED_1_5GB          6 /*SATA*/
#define DRIVE_SPEED_8GB            7
#define DRIVE_SPEED_10GB           8

/*-------------------------------------------------------------------
 * READ_CACHE_RETENTION_PRIORITY
 *
 * This field described the read cache page replacement policies
 * There are three different values:
 *
 * 1:  The page replacement algorithm will replace data
 *     put into the cache via a host read request sooner
 *     than data put into the via other means (ie prefetch).
 *     Host data has lower priority than prefetch data.
 * 2:  The page replacement algorithm give priority to
 *     data put into the cache via a host read.
 *     prefetch data has lower priority than host data.
 * 3:  The page replacement algorithm will not distingiush
 *     between data types.  (Note that a retention priority of "0"
 *     will disadvantage this unit in relation to the other ones!)
 *-------------------------------------------------------------------*/
#define LU_NO_RETENTION_PRIORITY            0x00
#define LU_PREFETCH_DATA_HIGH_PRIORITY      0x01
#define LU_HOST_DATA_HIGH_PRIORITY          0x02
#define LU_HOST_PREFETCH_EQUAL_PRIORITY     0x03


/*
 * FRU Signature structures
 */
typedef struct fru_sig_partition_table
{
    UINT_32 lun;
    // The LU that this partition on this fru is a part of

    TIMESTAMP timestamp;
    // The same pseudo-random number which is stored in the fru table
    // and the GLUT entry for this disk

    UINT_32 group_id;
    // The group id that this Partition is a part of
} FRU_SIGNATURE_PARTITION_TABLE;

typedef struct fru_signature
{
    REVISION_NUMBER rev_num;
    // A revision number for the format of this fru signature entry

    UINT_32E wwn_seed;
    // An identifier that ties this disk to a particular system chassis

    UINT_32 fru;
    // The fru number that this disk is supposed to be.  It should match
    // the slot number that the drive is actually inserted in.

    FRU_SIGNATURE_PARTITION_TABLE partition_table[MAX_FRU_PARTITIONS];
    // Table of several per partition items that need to be maintained.

#ifndef ALAMOSA_WINDOWS_ENV
    LBA_T CSX_ALIGN_N(8) zeroed_mark;
#else
    LBA_T zeroed_mark;
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */

    UINT_32 zeroed_stamp;
    // Two fields than indicate how much of a FRU has been zeroed.

} FRU_SIGNATURE;

// Define possible FRU Signature errors (bitmask for multiple errors)
#define FRU_SIG_INVALID_WWN_SEED        0x01
#define FRU_SIG_INVALID_SLOT            0x02
#define FRU_SIG_INVALID_PARTITION       0x04
#define FRU_SIG_INVALID_UNIT            0x08
#define FRU_SIG_INVALID_REVISION        0x10

typedef enum
{
    FRU_PUP_BLOCKED_BY_NONE                 = 0x00000000,
    FRU_PUP_BLOCKED_BY_FOREIGN_DRIVE        = 0x00000001,
    FRU_PUP_BLOCKED_BY_CRITICAL_DRIVE       = 0x00000002,
    FRU_PUP_BLOCKED_BY_UNKNOWN              = 0xFFFFFFFF
}FRU_PUP_BLOCKED_TYPE;


/***************************************************************************
 *      Background Services Information
 *
 ***************************************************************************/

/*
**  BGS enable flags
**   These are used in the DBA to identify both the reasons, and where, individual
**   services are enabled
*/
#define DBA_BGS_ENABLED_BY_USER                 0x0001
#define DBA_BGS_ENABLED_BY_BGS_ERROR_HANDLING   0x0002
#define DBA_BGS_ENABLED_BY_VER_CMD_PROC         0x0004
#define DBA_BGS_ENABLED_BY_EXP_DEF              0x0008
#define DBA_BGS_ENABLED_BY_UNBIND               0x0010
#define DBA_BGS_ENABLED_BY_PEER_BOOT            0x0020
#define DBA_BGS_ENABLED_BY_LUN_SHRINK           0x0040
#define DBA_BGS_ENABLED_BY_DB_COMMIT            0x0080
#define DBA_BGS_ENABLED_BY_CM_QUIESCE           0x0100
#define DBA_BGS_ENABLED_BY_CACHE_DUMP           0x0200
#define DBA_BGS_ENABLED_BY_NR                   0x0400
#define DBA_BGS_ENABLED_BY_NDU                  0x0800
#define DBA_BGS_ENABLED_BY_REG_KEY              0x1000
#define DBA_BGS_ENABLED_BY_VAULT_ZERO           0x2000


#define  DBA_BGS_ENABLED_BY_ALL                                   \
    (DBA_BGS_ENABLED_BY_USER |                                    \
     DBA_BGS_ENABLED_BY_BGS_ERROR_HANDLING |                      \
     DBA_BGS_ENABLED_BY_VER_CMD_PROC |                            \
     DBA_BGS_ENABLED_BY_EXP_DEF |                                 \
     DBA_BGS_ENABLED_BY_UNBIND |                                  \
     DBA_BGS_ENABLED_BY_PEER_BOOT |                               \
     DBA_BGS_ENABLED_BY_LUN_SHRINK |                              \
     DBA_BGS_ENABLED_BY_DB_COMMIT |                               \
     DBA_BGS_ENABLED_BY_CM_QUIESCE |                              \
     DBA_BGS_ENABLED_BY_CACHE_DUMP |                              \
     DBA_BGS_ENABLED_BY_NR |                                      \
     DBA_BGS_ENABLED_BY_NDU |                                     \
     DBA_BGS_ENABLED_BY_REG_KEY |                                 \
     DBA_BGS_ENABLED_BY_VAULT_ZERO)

/*
 * dba_bgs_enable_flags_bitfield_t is a bitmask type which is used
 *  to store BGS enable flags for a single service, on a single service
 *  level. For example, each LU has a dba_bgs_enable_flags_bitfield_t
 *  for each background service, for storing LU-specific BGS enable 
 *  flags for each service. Each dba_bgs_enable_flags_bitfield_t contains
 *  a flag for each background service control Requestor, which could be
 *  FLARE, User, or BGS Internal Error Handling. Every Requestor in
 *  the system must have that background service, on that service level,
 *  set to TRUE (enabled) for that background service to be enabled 
 *  on that service level (on that element) as a whole. Via the use of
 *  the dba_bgs_enable_flags_bitfield_t, we can track the enable states
 *  of each defined requestor independently and avoid collision conflicts.
 */
typedef UINT_32 dba_bgs_enable_flags_bitfield_t;

/* BGS-TODO: There are to be more master controller flags to be added 
 */

/* The following #defines are for background services master controller flags
 * used in setting the sysconfig.bgs_mc_flags field.
 */
#define SYSTEM_CONFIG_BGS_SCHED_MC      0x00000001 // Master Controller flag for the Scheduler
#define SYSTEM_CONFIG_BGS_DISK_MC       0x00000002 // Master Controller flag for disk services
#define SYSTEM_CONFIG_BGS_ALL_MC        SYSTEM_CONFIG_BGS_SCHED_MC | SYSTEM_CONFIG_BGS_DISK_MC // Master Controller flag that sets all bits

#endif
