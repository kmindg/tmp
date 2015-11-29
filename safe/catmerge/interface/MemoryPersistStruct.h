#ifndef MEMORYPERSISTSTRUCT_H
#define	MEMORYPERSISTSTRUCT_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  DESCRIPTION
 *     This file defines the Memory Persistence structures and defines 
 *     used to request that memory be persisted through a reboot/panic.
 *
 *     NOTE: This defines are based on the Functional Specification for 
 *           BIOS Memory Persistence Version 0.3.
 *   
 *
 *  HISTORY
 *     21-Feb-2007     Created.   -Austin Spang
 *
 *
 ***************************************************************************/


/************************** Global Defines *********************************/

/***************************************************************************
 * Memory Persistence Object ID 
 ***************************************************************************/
#define MEMORY_PERSISTENCE_OBJECT_ID 0xC4B8E0A3

/***************************************************************************
 * Memory Persistence Revision Number
 *  One for Dreadnought
 *  the other for Trident\Ironclad
 ***************************************************************************/
#define MEMORY_PERSISTENCE_REVISION_NUM_DR    0x02
#define MEMORY_PERSISTENCE_REVISION_NUM_TR_IC 0x02
#define MEMORY_PERSISTENCE_REVISION_NUM       0x02

/***************************************************************************
 * Memory Persistence Requested
 ***************************************************************************/
#define MEMORY_PERSISTENCE_REQUESTED 0x1

/***************************************************************************
 * Memory Persistence Status values (bit mask):
 *    MEMORY_PERSISTENCE_STATUS_SUCCESS - Memory Persistence reboot was 
 *          successful.
 *    MEMORY_PERSISTENCE_STATUS_STRUCT_INIT - Memory Persistence structure
 *          in NVRAM was initialized (bad object ID or revision number). 
 *
 *    MEMORY_PERSISTENCE_STATUS_STRUCT_CORRUPT - Memory Persistence 
 *          structure was invalid (bad BIOS application block).
 *
 *    MEMORY_PERSISTENCE_STATUS_POWER_FAIL - SP was power cycled. Memory 
 *          persistence not possible.
 *
 *    MEMORY_PERSISTENCE_STATUS_SLEEP_FAIL - System wasn't in S3 sleep 
 *          state.
 *
 *    MEMORY_PERSISTENCE_STATUS_NORMAL_BOOT - Executed normal boot.
 *
 *    MEMORY_PERSISTENCE_STATUS_IMPROPER_REQUEST - Memory Persistense was 
 *          not requested.
 *
 *    MEMORY_PERSISTENCE_STATUS_REG_RESTORE_ERR - BIOS fails to restore 
 *          the registers from NVRAM. 
 *
 *    MEMORY_PERSISTENCE_STATUS_ECC_FAILURE - Memory failed ECC check after
 *          being persisted.
 *
 *    MEMORY_PERSISTENCE_STATUS_SMI_RESET - SMI detected an error where 
 *          memory should not be persisted.
 *
 *    MEMORY_PERSISTENCE_STATUS_POST_ABORT - POST abort notification.
 *
 *    MEMORY_PERSISTENCE_STATUS_UTILITY_PARTITION_CLEARED - Utility Partition
 *          booted and cleared persistence flag.
 *
 ***************************************************************************/
#define MEMORY_PERSISTENCE_STATUS_SUCCESSFUL                      0x00000000
#define MEMORY_PERSISTENCE_STATUS_STRUCT_INIT                     0x00000001
#define MEMORY_PERSISTENCE_STATUS_STRUCT_CORRUPT                  0x00000002
#define MEMORY_PERSISTENCE_STATUS_POWER_FAIL                      0x00000004
#define MEMORY_PERSISTENCE_STATUS_SLEEP_FAIL                      0x00000008

#define MEMORY_PERSISTENCE_STATUS_NORMAL_BOOT                     0x00000010
#define MEMORY_PERSISTENCE_STATUS_IMPROPER_REQUEST                0x00000020
#define MEMORY_PERSISTENCE_STATUS_REG_RESTORE_ERR                 0x00000040
#define MEMORY_PERSISTENCE_STATUS_ECC_FAILURE                     0x00000080

#define MEMORY_PERSISTENCE_STATUS_SMI_RESET                       0x00000100
#define MEMORY_PERSISTENCE_STATUS_S3_FAIL                         0x00000200
#define MEMORY_PERSISTENCE_STATUS_BOOT_MODE_ERROR                 0x00000400
#define MEMORY_PERSISTENCE_STATUS_FIRMWARE_UPGRADED               0x00000800

#define MEMORY_PERSISTENCE_STATUS_CACHE_NOT_FLUSHED               0x00001000
#define MEMORY_PERSISTENCE_STATUS_FIRMWARE_UPDATE_SUCCESS         0x00002000
#define MEMORY_PERSISTENCE_STATUS_FIRMWARE_UPDATE_FAILURE         0x00004000

#define MEMORY_PERSISTENCE_STATUS_POST_ABORT                      0x00010000

#define MEMORY_PERSISTENCE_STATUS_UTILITY_PARTITION_CLEARED       0x80000000

/***************************************************************************
 * Application Block Types
 ***************************************************************************/
#define MEMORY_PERSISTENCE_UNUSED        0x00000000
#define MEMORY_PERSISTENCE_BIOS          0x42494F53
#define MEMORY_PERSISTENCE_POST          0x504F5354
#define MEMORY_PERSISTENCE_CLARIION      0x466C6172
#define MEMORY_PERSISTENCE_ENGINUITY     0x456E6769
#define MEMORY_PERSISTENCE_DART          0x44415254
#define MEMORY_PERSISTENCE_C4            0x43346334

/************************** Global Structures ******************************/
/***************************************************************************
 *          Memory Layout
 *
 *     ________________________
 *     |                      |
 *     | Memory Persistence   |
 *     | Structure            |
 *     |                      |
 *     ------------------------
 *     | Application Block 1  |
 *     |                      |
 *     ------------------------
 *     | Application Block 2  |
 *     |                      |
 *     ------------------------
 *     |                      |
 *                *
 *                *
 *     |                      |
 *     ------------------------
 *     | Application Block n  |
 *     |                      |
 *     ________________________
 *
 ***************************************************************************/

/***************************************************************************
 * Name:
 *      Memory Persistence structure
 *
 * Description:
 *      This structure is used by an application to request memory 
 *      persistence by the BIOS and to determine the status of a 
 *      previous memory persistence request.
 *
 * Members:
 *      ObjectId - Identifier for Memory Persistence Structure.
 *      RevisionId - Revision of the Memory Persistence Structure.
 *      PersistRequest - Memory Persistence request.
 *      PersistStatus - The status of the last memory persistence request.
 *      Reserved - Reserved locations in the structure.
 *
 * Note:
 *      See the Functional Specification for the BIOS Memory Persistence for
 *      details on the use of this structure and for valid values within
 *      the structure.
 ***************************************************************************/

typedef struct _MEMPERSIST
{
    UINT_32E ObjectId;
    UINT_16E RevisionId;
    UINT_16E PersistRequest;
    UINT_32E PersistStatus;
    UINT_32E Reserved[2];
} MEMPERSIST, *pMEMPERSIST;

/***************************************************************************
 * Name:
 *      Application Block Header Structure.
 *
 * Description:
 *      This structure is used by applications to determine if this 
 *      application block is theirs and what the length of the block is.
 *
 * Members:
 *      Type - An indicator of which application owns this block.
 *      Length - The length of the application block including the size
 *               of this structure.
 ***************************************************************************/

typedef struct _APPBLKHDR
{
    UINT_32E Type;
    UINT_32E Length;
}APPBLKHDR, *pAPPBLKHDR;

#endif   /* ifndef MEMORYPERSISTSTRUCT_H */
