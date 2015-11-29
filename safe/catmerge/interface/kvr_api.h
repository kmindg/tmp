#ifndef __KVR_API__
#define __KVR_API__

/**********************************************************************
 *      Company Confidential
 *          Copyright (c) EMC Corp, 2012
 *          All Rights Reserved
 *          Licensed material - Property of EMC Corp
 **********************************************************************/

/**********************************************************************
 *
 *  File Name: 
 *          kvr_api.h
 *
 *  Description:
 *      This document describes the C interface for the KVR Library aka 
 *      Key Value Pair Repository Library. These are to be used by
 *      multiple clients such as RepV2 and Vvol.
 *      
 *      The first revision for Kitty Hawk provides interfaces required by 
 *      RepV2 running co-located in the same process. The API will be made
 *      available to remote processes via RPC in the future post KH.
 *
 *  Revision History:
 *      - 1.1	Faramarz Rabii  12-Sep-12   Created
 *          
 **********************************************************************/

#include <stdint.h>
#include "spid_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define IN 
#define OUT
#define INOUT

typedef struct kvr_clid {
    char * repository_name;
    uint32_t name_size;
    char * object_id;
    uint32_t id_size;
} kvr_clid_t;

typedef void * kvr_rephdl;

typedef struct kvr_iorecord {
    char *key;
    char *data;
    uint32_t mode;
    uint32_t size;
    EMCPAL_STATUS result;
} kvr_iorecord_t;

/*
 * Write once repository flag written at format time
 */
#define KVR_BUILD_REC_HAS_HDR            0x001
#define KVR_BUILD_MULTIPLE_INDEX         0x002
#define KVR_BUILD_VECTORUPDALLOWED       0x004
#define KVR_BUILD_SNAPSHOTTED            0x008
#define KVR_BUILD_REPLICATED             0x010
#define KVR_BUILD_BACKINGGUARANTEED      0x020

#define KVR_BUILD_DEFAULT (KVR_BUILD_REC_HAS_HDR|KVR_BUILD_BACKINGGUARANTEED)

/*
 * Repository modifiable flags
 */
#define KVR_REP_ISDIRTY     0x01
#define KVR_REP_INITIALIZED 0x02

/*
 *   Build a repository
 *
 *   This will:
 *       1) obtain the required space based on the flags
 *       2) assign it to a client based on its name
 *       2) format it to be a new KVR repository based on input parameters
 *
 *   The repository will remain until it is destroyed using the destroy API.
 * 
 */
EMCPAL_STATUS CSX_MOD_EXPORT kvr_build_repository
(
    IN kvr_clid_t *client_id, 
    IN uint32_t repository_size, IN uint32_t key_size, IN uint32_t data_size, IN uint64_t flags
);

/*
 *   Deallocate a repository
 *
 *   The repository will is destroyed and the space returned to whence it came from
 *
 */
EMCPAL_STATUS CSX_MOD_EXPORT kvr_destroy_repository
(
    IN kvr_clid_t *client_id
);

/*
 * Open modes
 */
#define KVR_OPEN_RW      0x00            /* Repository opened read-write */
#define KVR_OPEN_RO      0x01            /* Repository opened read-only */
#define KVR_OPEN_DCACHE  0x02            /* Repository opened with data cache */

/*
 *   Open a repository
 *
 *   This will 
 *       1) open an already formatted repository. 
 *       2) perform the basic sanity checks.
 *       3) instantiate in memory objects.
 *       4) read in the key data index.
 *       5) instantiate in memory objects
 *       6) return a handle to use for IO.
 *
 *   Note: this does not modify the on disk content.
 *   Note: in KH we support a single open at a time per repository. Post KH
 *         multiple opens may be supported.
 */
EMCPAL_STATUS CSX_MOD_EXPORT kvr_open_repository(
    IN kvr_clid_t *client_id, 
    IN uint32_t open_mode, 
    OUT kvr_rephdl *rephdl
);

/*
 *   Close a repository
 *
 *   This will 
 *       1) Close an already opened repository.
 *       2) deallocate in memory objects.
 *
 *   Note: this does not modify the on disk content.
 *   Note: in KH we support one open and a matching close. Post KH
 *         with multiple opens only the final close deallocates the 
 *         in memory objects. The rest just derecement a reference.
 *
 */
EMCPAL_STATUS CSX_MOD_EXPORT kvr_close_repository
(
    IN kvr_rephdl rephdl
);

/*
 * Write modes 
 * 
 * mode flags UPDATE and CREATE  may be or'ed meaning the records is created if not present
 * and updated if it is present
 */
#define KVR_WRITE_CREATE 0x01            /* Create a new record in the repository */
#define KVR_WRITE_UPDATE 0x02            /* Update an existing record */

/*
 *   Write a record
 *
 *   This will act based on the write mode
 *       1) Write a new record to a repository
 *       2) Modify an existing record in a repository
 *       3) Add or modify a record
 *
 *   Note: the whole write is atomic
 */
EMCPAL_STATUS CSX_MOD_EXPORT kvr_write_record
(
    IN kvr_rephdl rephdl,
    IN char *key,
    IN uint32_t keySize,
    IN char *data,
    IN uint32_t dataSize,
    IN uint32_t mode
);

/*
 *   Read a record
 *
 *   Note: this does not modify the on disk content.
 */
EMCPAL_STATUS CSX_MOD_EXPORT kvr_read_record
(
    IN kvr_rephdl rephdl,
    IN char *key,
    IN uint32_t keySize,
    OUT char *data,
    INOUT uint32_t dataSize
);

/*
 *   Delete a record
 *
 *   Note: the whole operation is atomic
 */
EMCPAL_STATUS CSX_MOD_EXPORT kvr_delete_record
(
    IN kvr_rephdl *rephdl,
    IN char *key
);

/*
 *   Write a set of records
 *
 *   This will act based on the write mode as a normal write
 *       1) There must be nrecords in the records array
 *       2) The keys must be all unique
 *
 *   Note: the whole operation is atomic
 *   Note: API will be implmented post KH.
 *   Note: There is a per record error and an overall error.
 */
EMCPAL_STATUS CSX_MOD_EXPORT kvr_write_record_by_vector
(
    IN kvr_rephdl rephdl,
    IN uint32_t nrecords,
    IN kvr_iorecord_t *records[]
);

/*
 *   Read a set of records
 *
 *   This will read a set of record in a repository
 *       1) There must be nrecords in the records array
 *       2) The keys must be all unique
 *
 *   Note: this does not modify the on disk content.
 *   Note: API will be implmented post KH.
 *   Note: There is a per record error and an overall error.
 */
EMCPAL_STATUS CSX_MOD_EXPORT kvr_read_record_by_vector
(
    IN kvr_rephdl rephdl,
    IN uint32_t nrecords,
    INOUT kvr_iorecord_t *records[]
);

/*
 *   Delete a set of records
 *       1) There must be nrecords number of keys
 *       2) The keys must be all unique
 *
 *   Note: the whole operation is atomic
 *   Note: API will be implmented post KH.
 */
EMCPAL_STATUS CSX_MOD_EXPORT kvr_delete_record_by_vector
(
    IN kvr_rephdl *rephdl,
    IN char *key[]
);

/*
 * Several new API have been requested by Vvol and are planned for post KH 
 * These include:
 *
 * ** vector delete - signature shown above
 * ** enumeration of records - not fully designed
 * ** register notification callback - tbd
 * ** update notification - requires input from Vvol team
 * ** remote api - tbd
 */

#ifdef __cplusplus
}
#endif

#endif // __KVR_API__
