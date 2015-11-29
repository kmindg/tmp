/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *
 *  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.
 *  ACCESS TO THIS SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT.
 *  THIS CODE IS TO BE KEPT STRICTLY CONFIDENTIAL.
 ***************************************************************************/
/***************************************************************************
 *
 * Description:
 *      IOCTL interface structures to the Hornet driver
 *
 * Functions:
 *
 * Notes:
 *
 * History:
 *      02-Jul-2012:	Initial Creation. 
 *
 *
 **************************************************************************/

#ifndef HORNET_INTERFACE_H
#define HORNET_INTERFACE_H

#include "csx_ext.h"

#ifdef __cplusplus 
extern "C" 
{
#endif

#define HORNET_FW_VERSION_SIZE       (128)

/* set of hornet states returned by HORNETMANAGER_IOCTL_GET_SYSTEM_STATE */
typedef enum {
    HORNET_SYSTEM_STATE_REMOVED=0,              // PEER is removed (when removed we cannot separate between SP & Hornet.  Starting state 
    HORNET_SYSTEM_STATE_SP_DETECTED,            // SP is inserted. Terminal. Remove SP to leave this state 
    HORNET_SYSTEM_STATE_INITIALIZING,           // eHornet is inserted, Transport is esablishing communications; Transitional
    HORNET_SYSTEM_STATE_UPGRADING,              // eHornet is performing firmware upgrade; Transitional
    HORNET_SYSTEM_STATE_FLASH_DEGRADED,         // eHornet is inserted, there are some problems with flash storage on the eHornet
    HORNET_SYSTEM_STATE_ERROR_DENSITY_HIGH,     // eHornet is inserted, there are some problems with overal eHornet functionality
    HORNET_SYSTEM_STATE_ERASED,                 // eHornet is inserted, all data from eHornet was erased
    HORNET_SYSTEM_STATE_FAILED,                 // eHornet is inserted, fatal error, eHornet is non functional
    HORNET_SYSTEM_STATE_OK,                     // eHornet is inserted, fully operational
    HORNET_SYSTEM_STATE_COUNT
} HORNET_SYSTEM_STATE;

typedef enum {
    HORNET_MODE_READY = 0,
    HORNET_MODE_RESTORE,
    HORNET_MODE_AUTOVAULT,
    HORNET_MODE_UPGRADE,
    HORNET_MODE_VAULT,
} hornet_mode_e;

typedef struct {
    csx_size_t   queued_count;
    csx_size_t   queued_bytes;
    csx_size_t   read_count;
    csx_size_t   read_bytes;
    csx_size_t   write_count;
    csx_size_t   write_bytes;
    csx_size_t   sgl_count;
    csx_size_t   flush_force;
    csx_size_t   overruns;
    csx_nuint_t  max_request_len;
    csx_nuint_t  max_transfer_len;
} hornet_iostat_t;

typedef struct {
    csx_u64_t sbe_errors;
    csx_u64_t mbe_errors;
} hornet_err_cnts_t;

typedef struct {   
    csx_u64_t bad_blocks_count;
} hornet_flash_err_cnts_t;

typedef struct {
    csx_u64_t memory_size;
    csx_u64_t cache_begin;
    csx_u64_t cache_size;
    csx_u64_t storage_size;
    csx_u64_t block_size;
    csx_u64_t flash_size;
    csx_u64_t good_blocks_count;
    csx_u64_t bad_blocks_count;
    csx_u64_t blocks_reserved;
} mirror_info;


/**
 * Hornet driver IOCTL interface
 **/
#define HORNET_FIRMWARE_PATH_SIZE (512)

/* Commands */
typedef enum {
    HORNET_IOCTL_SET_MODE = 0,
    HORNET_IOCTL_SET_CACHE_SIZE,
    HORNET_IOCTL_GET_MIRROR_INFO,
    HORNET_IOCTL_GET_STORAGE_INFO,
    HORNET_IOCTL_GET_SDRAM_ERR_COUNT,
    HORNET_IOCTL_RESET_SDRAM_ERR_COUNT,
    HORNET_IOCTL_GET_FLASH_BAD_BLOCK_COUNT,
    HORNET_IOCTL_RECLAIM,
    HORNET_IOCTL_READ_MIRROR,
    HORNET_IOCTL_WRITE_MIRROR,
    HORNET_IOCTL_WRITE_MIRROR_LIST,
    HORNET_IOCTL_ZERORING_MIRROR,
    HORNET_IOCTL_NAND_CLEAN,
    HORNET_IOCTL_GET_FW_VERSION,
    HORNET_IOCTL_UPGRADE_FIRMWARE,
    HORNET_IOCTL_ERASE_HORNET,
    HORNET_IOCTL_READ_FILE,
    HORNET_IOCTL_WRITE_FILE,
    HORNET_IOCTL_SELF_TEST,
    HORNET_IOCTL_DUMP_INOUT_BUFFER,
#ifdef SIMMODE_ENV
    HORNET_IOCTL_SIM_INSERT,
    HORNET_IOCTL_SIM_REMOVE,
#endif
    HORNET_IOCTL_INSERT_OVERRIDE,
    HORNET_IOCTL_SET_TRACE_LEVEL,
    HORNET_IOCTL_NVCM_CALLBACK,
    //hornetmanager ioctls
    HORNET_IOCTL_GET_INFO,
    HORNET_IOCTL_RECALCULATE_RSVD_PEBS,
    HORNET_IOCTL_ERROR_NOTIFICATION,
    HORNET_IOCTL_GET_SYSTEM_STATE,
    HORNET_IOCTL_ERASE_HORNET_FROM_MANAGER,
    HORNET_IOCTL_RESET_HORNET,
} hornet_cmd_t;

typedef enum {
    PEER_IOCTL_LOOSE_HEARTBEAT,
    PEER_IOCTL_RESET,
    PEER_IOCTL_REPLY,
    PEER_IOCTL_SET_LOGGING_LVL,
    PEER_IOCTL_EMULATE_FAILURE
} peer_cmd_t;

typedef enum {
    HORNET_TRACE_ERR = 0,
    HORNET_TRACE_WARN,
    HORNET_TRACE_INFO,
    HORNET_TRACE_DBG
} hornet_trace_level_t;

typedef enum {
    PEER_FAIL_NO,
    PEER_FAIL_DRAM_ECC_MBE,
    PEER_FAIL_DRAM_ECC_SBE,
    PEER_FAIL_FLASH_ERROR,
    PEER_FAIL_OLD_FW,
    PEER_FAIL_FLASH_DEGRADED
} peer_failure_t;

/*Memory buffer and module title*/
typedef struct {
    csx_p_bufman_t memory_manager;
    csx_p_bufman_sgl_entry_t memory_block;
} MEMORY_BUFFER;

/* Hornet IOCTL structures */
/* IOCTL IN data */
typedef struct {
    hornet_mode_e mode;
} hornet_ioctl_in_set_mode_t;

typedef struct {
    csx_size_t size;
} hornet_ioctl_in_set_cache_size_t;

typedef struct {
    csx_uoffset_t ofs;
    csx_size_t size;
} hornet_ioctl_in_reclaim_t;

typedef struct {
    csx_phys_address_t dst;
    csx_uoffset_t src;
    csx_size_t size;
} hornet_ioctl_in_read_mirror_t;

typedef struct {
    csx_uoffset_t dst;
    csx_phys_address_t src;
    csx_size_t size;
} hornet_ioctl_in_write_mirror_t;

typedef struct {
    csx_char_t path[HORNET_FIRMWARE_PATH_SIZE];
    csx_nuint_t bytes;
} hornet_ioctl_file_t;

typedef struct {
    csx_char_t path[HORNET_FIRMWARE_PATH_SIZE];
} hornet_ioctl_in_upgrade_firmware_t;

enum {
    HORNET_SGLIST_MAXLEN = 64,
    HORNET_SGLIST_RESERVE = HORNET_SGLIST_MAXLEN + 1, // Reserve for termination
};

typedef struct {
    csx_phys_address_t src;
    csx_uoffset_t      dst_offset;
    csx_size_t         size;
} hornet_sg_element_t;

typedef hornet_sg_element_t hornet_sg_array_t[HORNET_SGLIST_MAXLEN];

typedef struct {
    csx_nuint_t count;
    hornet_sg_array_t list;
} hornet_ioctl_in_write_mirror_list_t;

typedef struct {
    csx_bool_t val;
} hornet_ioctl_in_peer_reply_t;

typedef struct {
    csx_uchar_t buf_id;
} hornet_ioctl_dump_inout_buffer_t;

typedef struct {
    csx_uoffset_t dst_ofs;
    csx_size_t size;
    csx_uchar_t cmd;
    csx_nuint_t count;
    csx_nuint_t thread_num;
} hornet_ioctl_in_self_test_t;

typedef struct {
    csx_bool_t val;
} hornet_ioctl_in_insert_override_t;

typedef struct {
    hornet_trace_level_t level;
} hornet_ioctl_in_trace_level_t;

typedef struct {
    peer_failure_t fail;
} hornet_ioctl_in_peer_fail_t;


/* Input for the HORNET_IOCTL_NVCM_CALLBACK;
   pNVCM_callback - function pointer; if NULL, the callback will be removed;
   context - void pointer to the NVCM specific context
   Output status is returned in the hornet_ioctl_out_status_t*/
typedef csx_void_t (*NVCM_CALLBACK)(csx_pvoid_t, HORNET_SYSTEM_STATE);
typedef struct {
    NVCM_CALLBACK callback;   /* This function will be called when Hornet state changes */
    csx_pvoid_t   context;    /* NVCM can supply pointer to the context */
} hornet_ioctl_in_nvcm_callback_t;

/* IOCTL OUT data */
typedef struct {
    csx_status_e status;
} hornet_ioctl_out_status_t;

typedef struct {
    csx_status_e status;
    mirror_info mirror;
} hornet_ioctl_out_get_mirror_info_t;

typedef struct {
    csx_status_e status;
    hornet_flash_err_cnts_t error_info;
} hornet_ioctl_out_get_flash_err_info_t;

typedef struct {
    csx_status_e status;
    hornet_err_cnts_t error_info;
} hornet_ioctl_out_get_error_info_t;

typedef struct {
    csx_status_e status;
    csx_char_t   version[HORNET_FW_VERSION_SIZE];
} hornet_ioctl_out_get_fw_version_t;


typedef struct {
    csx_status_e status;
    HORNET_SYSTEM_STATE state;
} hornet_ioctl_out_system_state_t;

#ifdef __cplusplus 
} // extern C
#endif

#endif // HORNET_INTERFACE_H
