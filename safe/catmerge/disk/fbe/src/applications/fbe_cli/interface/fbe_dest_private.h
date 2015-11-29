#ifndef FBE_DEST_PRIVATE_HH
#define FBE_DEST_PRIVATE_HH
/*******************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *******************************************************************/

/*******************************************************************
 * fbe_dest_private.h
 *******************************************************************
 *
 * DESCRIPTION
 *  This file contains all the definitions, structures used by the
 *  DEST module. 
 *
 * Table of Contents:
 * 
 * History:
 *  05/01/2012  Created. kothal
 *******************************************************************/

#include "fbe/fbe_dest.h"

#include "fbe_cli_neit.h"

/*********** Defines for DEST *******************/

/* This is the maximum length of the strings used by DEST.
 * Especially for opcodes, errors and character buffers.
 */
#define DEST_MAX_STR_LEN 64
#define DEST_MAX_FRUS_PER_RECORD 32

/* dest_printf uses fcli print module. 
 */
#define dest_printf fbe_cli_printf

/* This is for invalid FRU.
 */
#define DEST_INVALID_FRU 0xffffffff
#define DEST_INVALID_VALUE  0xffffffff
#define DEST_INVALID_LBA_RANGE  0xffffffffffffffff
#define DEST_INVALID_VALUE_64 (UINT_64)-1
#define DEST_INVALID_RANGE  0xffffffff

#define DEST_ANY_STR    "ANY"
#define DEST_ANY_RANGE  0xffffffff

/* When num_of_times_to_reactivate is set to this value, the error 
 * record will always be reactivated after the reset time is past.  
 * It's OK to use 0 because reactivation is turned off by setting 
 * secs_to_reactivate to DEST_INVALID_SECONDS, rather than by 
 * setting num_of_times to 0.
 */
#define FBE_DEST_ALWAYS_REACTIVATE  0

/*Wrong location */

#define SCSI_COPY               0xD8
#define SCSI_DATA_MOVE          0xEB    /* Data mover */
#define SCSI_DEFINE_GROUP       0xE1
#define SCSI_FORCE_RESERVE      0xE4
#define SCSI_FORMAT_UNIT        0x04
#define SCSI_INQUIRY            0x12
#define SCSI_LOG_SENSE          0x4D
#define SCSI_MAINT_IN           0xA3    /* SCSI-3/SCC-2 command */
#define SCSI_MAINT_OUT          0xA4    /* SCSI-3/SCC-2 command */
#define SCSI_MODE_SELECT_6      0x15
#define SCSI_MODE_SELECT_10     0x55
#define SCSI_MODE_SENSE_6       0x1A
#define SCSI_MODE_SENSE_10      0x5A
#define SCSI_PER_RESERVE_IN     0x5E    /* SCSI-3 command */
#define SCSI_PER_RESERVE_OUT    0x5F    /* SCSI-3 command */
#define SCSI_PREFETCH           0x34
#define SCSI_READ               0x08
#define SCSI_READ_6             0x08
#define SCSI_READ_10            0x28
#define SCSI_READ_12            0xA8
#define SCSI_READ_16            0x88
#define SCSI_READ_BUFFER        0x3C
#define SCSI_READ_LONG_10       0x3E
#define SCSI_READ_LONG_16       0x9E
#define SCSI_READ_CAPACITY      0x25    /* 10 byte */
#define SCSI_READ_CAPACITY_16   0x9E
#define SCSI_REASSIGN_BLOCKS    0x07
#define SCSI_RCV_DIAGNOSTIC     0x1C
#define SCSI_RELEASE            0x17
#define SCSI_RELEASE_10         0x57    /* 10 byte SCSI-3 release */
#define SCSI_RELEASE_GROUP      0xE3
#define SCSI_REQUEST_SENSE      0x03
#define SCSI_REPORT_LUNS        0xA0
#define SCSI_RESERVE            0x16
#define SCSI_RESERVE_10         0x56    /* 10 byte SCSI-3 reserve */
#define SCSI_RESERVE_GROUP      0xE2
#define SCSI_REZERO             0x01
#define SCSI_SEND_DIAGNOSTIC    0x1D
#define SCSI_RECEIVE_DIAGNOSTIC 0x1C
#define SCSI_SKIPMASK_READ      0xE8
#define SCSI_SKIPMASK_WRITE     0xEA
#define SCSI_START_STOP_UNIT    0x1B
#define SCSI_SYNCRONIZE         0x35
#define SCSI_TEST_UNIT_READY    0x00
#define SCSI_VERIFY             0x2F    /* 10 byte */
#define SCSI_VERIFY_16          0x8F
#define SCSI_VOLUME_SET_IN      0xBE    /* SCSI-3/SCC-2 command */
#define SCSI_VOLUME_SET_OUT     0xBF    /* SCSI-3/SCC-2 command */
#define SCSI_WRITE              0x0A
#define SCSI_WRITE_6            0x0A
#define SCSI_WRITE_10           0x2A
#define SCSI_WRITE_12           0xAA
#define SCSI_WRITE_16           0x8A
#define SCSI_WRITE_BUFFER       0x3B
#define SCSI_WRITE_SAME         0x41    /* 10 byte */
#define SCSI_WRITE_SAME_16      0x93
#define SCSI_WRITE_VERIFY       0x2E    /* 10 byte */
#define SCSI_WRITE_VERIFY_16    0x8E
#define SCSI_WRITE_LONG_10      0x3F
#define SCSI_WRITE_LONG_16      0x9F

/**********************************************************************
 * DEST_SCSI_ERR_LOOKUP
 **********************************************************************
 * Structure to form the SCSI error lookup table. 
 **********************************************************************/
typedef struct DEST_SCSI_ERR_LOOKUP
{
    TEXT* scsi_name;    /* SCSI Error in String format. */
    fbe_u32_t scsi_value; /* SCSI Error actual value. */
    TEXT* error_code;   /* first 3 bytes represent SK/ASC/ASCQ */
    fbe_bool_t valid_lba;       /* sense data valid lba bit */
    
}FBE_DEST_SCSI_ERR_LOOKUP;

/**********************************************************************
 * DEST_PORT_ERR_LOOKUP
 **********************************************************************
 * Structure to form the SCSI error lookup table. 
 **********************************************************************/
typedef struct FBE_DEST_PORT_ERR_LOOKUP
{
    TEXT* scsi_name;    /* SCSI Error in String format. */
    fbe_u32_t scsi_value; /* SCSI Error actual value. */
    
}FBE_DEST_PORT_ERR_LOOKUP;

/**********************************************************************
 *  DEST_SCSI_OPCODE_LOOKUP
 **********************************************************************
 * Structure to form the SCSI opcode lookup table. 
 **********************************************************************/
typedef struct FBE_DEST_SCSI_OPCODE_LOOKUP
{
    TEXT* scsi_name;        /* SCSI name in string format. */
    fbe_u32_t scsi_value;     /* Corresponding SCSI Value. */
    TEXT* scsi_shortcut;    /* Short cut created for the value. */
}FBE_DEST_SCSI_OPCODE_LOOKUP;

/**********************************************************************
 * DEST_RECORD_FRU_TYPE
 **********************************************************************
 * FRU is the representation of a combination of Bus, Enlcosure and 
 * Slot numbers. The following structure encapsulates the same.  
 **********************************************************************/
typedef struct DEST_RECORD_FRU_TYPE
{
    fbe_u32_t bus;    /* Bus number for the record. */
    fbe_u32_t encl;   /* Enclosure for the record. */
    fbe_u32_t slot;   /* Slot number for the record. */
    
}FBE_DEST_FRU;

/**********************************************************************
 * FBE_DEST_CMDS_TABLE_TYPE
 **********************************************************************
 * Commands that are used to control the DEST tool over FCLI are listed 
 * in a table called command lookup table. The following is the 
 * structure for the command lookup table. 
 **********************************************************************/
typedef struct FBE_DEST_CMDS_TBL_TYPE
{    
	/* Command Name. 
	 */
    TEXT *cmd_name;
 
    /* Callback to be invoked for the given command name.
     */
    VOID (*routine) (fbe_u32_t argc, char* argv[], TEXT* usage);
 
    /* Usage of the command. 
     */
    TEXT *usage;
    
}FBE_DEST_CMDS_TBL;

#endif /* FBE_DEST_PRIVATE_HH */
