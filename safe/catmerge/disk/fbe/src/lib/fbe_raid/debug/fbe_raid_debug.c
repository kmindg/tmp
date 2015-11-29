/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_raid_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the raid library.
 *
 * @author
 *  11/24/2009 - Created. Rob Foley
 *
 ***************************************************************************/
 
/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe/fbe_transport.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_raid_ts.h"
#include "fbe_transport_debug.h"
#include "fbe_raid_algorithm.h"
#include "fbe_raid_geometry.h"
#include "fbe_raid_iots_accessor.h"
#include "fbe_raid_siots_accessor.h"
#include "fbe/fbe_time.h"
#include "fbe_raid_library_debug.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

fbe_status_t fbe_raid_library_debug_print_common(const fbe_u8_t * module_name,
                                                 fbe_dbgext_ptr base_ptr,
                                                 fbe_trace_func_t trace_func,
                                                 fbe_trace_context_t trace_context,
                                                 fbe_debug_field_info_t *field_info_p,
                                                 fbe_u32_t spaces_to_indent);

static fbe_status_t fbe_raid_library_debug_print_siots_queue(const fbe_u8_t * module_name,
                                                             fbe_dbgext_ptr siots_queue_p,
                                                             fbe_trace_func_t trace_func,
                                                             fbe_trace_context_t trace_context,
                                                             fbe_u32_t spaces_to_indent);

static fbe_status_t fbe_raid_library_debug_print_parity_write_log_slot(const fbe_u8_t * module_name,
                                                                       fbe_dbgext_ptr raid_geometry_p,
                                                                       fbe_trace_func_t trace_func,
                                                                       fbe_trace_context_t trace_context,
                                                                       fbe_u32_t spaces_to_indent,
                                                                       fbe_u32_t slot_id);

static fbe_dbgext_ptr fbe_raid_library_debug_get_raid_geometry_from_siots(const fbe_u8_t * module_name,
                                                                          fbe_dbgext_ptr siots_p);

static fbe_status_t fbe_raid_library_debug_print_siots_queue_summary(const fbe_u8_t * module_name,
                                                                     fbe_dbgext_ptr siots_queue_p,
                                                                     fbe_trace_func_t trace_func,
                                                                     fbe_trace_context_t trace_context,
                                                                     fbe_u32_t spaces_to_indent);

/*!*******************************************************************
 * @struct fbe_raid_debug_common_flags_trace_info
 *********************************************************************
 * @brief Provides a mapping of enum to string for raid common flags
 *
 *********************************************************************/
fbe_debug_enum_trace_info_t fbe_raid_debug_common_flags_trace_info[] = 
{
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_REQ_STARTED),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_REQ_BUSIED),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_EMBED_ALLOC),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_COMPLETE),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_DEFERRED_MEMORY_ALLOCATION),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_MEMORY_REQUEST_ABORTED),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_ERROR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_MEMORY_FREE_ERROR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_REQ_RETRIED),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_ON_WAIT_QUEUE),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_RESTART),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_UNUSED2),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_UNUSED3),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_UNUSED3),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_UNUSED4),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_UNUSED5),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_UNUSED6),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_UNUSED7),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_UNUSED8),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_UNUSED9),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_TYPE_UNUSED1),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_TYPE_UNUSED2),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_TYPE_UNUSED3),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_TYPE_UNUSED4),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_TYPE_UNUSED5),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_TYPE_VR_TS),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_TYPE_CX_TS),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_TYPE_FRU_TS),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_TYPE_NEST_SIOTS),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_TYPE_SIOTS),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_TYPE_IOTS),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_COMMON_FLAG_MAX),
    FBE_DEBUG_ENUM_TRACE_INFO_TERMINATOR,
};

/*!*******************************************************************
 * @struct fbe_raid_debug_siots_flags_trace_info
 *********************************************************************
 * @brief Provides a mapping of enum to string for siots flags
 *
 *********************************************************************/
fbe_debug_enum_trace_info_t fbe_raid_debug_siots_flags_trace_info[] = 
{
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_EMBEDDED_SIOTS),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_WAIT_LOCK),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_WAS_DELAYED),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_DONE_GENERATING),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_UNUSED_0),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_QUIESCED),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_ERROR_PENDING),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_ONE_SIOTS),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_SINGLE_STRIP_MODE),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_UNUSED_1),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_WAITING_SHUTDOWN_CONTINUE),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_RECEIVED_CONTINUE),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_SINGLE_ERROR_RECOVERY),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_UNUSED_2),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_ERROR_INJECTED),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_WROTE_CORRECTIONS),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_WRITE_STARTED),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_UNUSED_3),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_UNUSED_4),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_UNUSED_5),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE_LOCK),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_UPGRADE_OWNER),
	FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_JOURNAL_SLOT_ALLOCATED),
	FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_COMPLETE_IMMEDIATE),
	FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_LOGGING_ENABLED),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_WAIT_WRITE_LOG_SLOT),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_WRITE_LOG_HEADER_REQD),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_FLAG_LAST),
    FBE_DEBUG_ENUM_TRACE_INFO_TERMINATOR,
};

/*!*******************************************************************
 * @struct fbe_raid_debug_iots_flags_trace_info
 *********************************************************************
 * @brief Provides a mapping of enum to string for iots flags
 *
 *********************************************************************/
fbe_debug_enum_trace_info_t fbe_raid_debug_iots_flags_trace_info[] = 
{
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_ERROR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_GENERATING),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_STATUS_SENT),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_WAS_QUIESCED),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_LOCK_GRANTED),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_WAIT_LOCK),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_GENERATE_DONE),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_RAID_LIBRARY_TEST),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_ALLOCATING_SIOTS),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_GENERATING_SIOTS),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_UPGRADE_WAITERS),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_EMBEDDED_SIOTS_IN_USE),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_ABORT),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_ABORT_FOR_SHUTDOWN),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_QUIESCE),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_REMAP_NEEDED),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_RESTART_IOTS),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_NON_DEGRADED),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_SIOTS_CONSUMED),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_LAST_IOTS_OF_REQUEST),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_FAST_WRITE),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_INCOMPLETE_WRITE),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_WRITE_LOG_FLUSH_REQUIRED),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_CHUNK_INFO_INITIALIZED),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_REKEY_MARK_DEGRADED),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_FLAG_LAST),
    FBE_DEBUG_ENUM_TRACE_INFO_TERMINATOR,
};

/*!*******************************************************************
 * @struct fbe_raid_debug_iots_status_trace_info
 *********************************************************************
 * @brief Provides a mapping of enum to string for iots status
 *
 *********************************************************************/
fbe_debug_enum_trace_info_t fbe_raid_debug_iots_status_trace_info[] = 
{
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_STATUS_INVALID),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_STATUS_COMPLETE),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_STATUS_UPGRADE_LOCK),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_STATUS_NOT_STARTED_TO_LIBRARY),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_STATUS_NOT_USED),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_STATUS_AT_LIBRARY),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_STATUS_LIBRARY_WAITING_FOR_CONTINUE),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_STATUS_WAITING_FOR_QUIESCE),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_IOTS_STATUS_LAST),
    FBE_DEBUG_ENUM_TRACE_INFO_TERMINATOR,
};

/*!*******************************************************************
 * @struct fbe_raid_debug_siots_algorithm_trace_info
 *********************************************************************
 * @brief Provides a mapping of enum to string for siots algoritms
 *
 *********************************************************************/
fbe_debug_enum_trace_info_t fbe_raid_debug_siots_algorithm_trace_info[] = 
{
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(RG_NO_ALG),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(R5_SM_RD),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(R5_RD),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(R5_468),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(R5_MR3),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(OBSOLETE_R5_ALIGNED_MR3_WR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(OBSOLETE_ALG_WAS_R5_DEG_WR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(OBSOLETE_R5_DWR_BKFILL),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(OBSOLETE_R5_DWRBK_468_RECOVERY),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(OBSOLETE_R5_DWRBK_MR3_RECOVERY),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(R5_DEG_RD),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(OBSOLETE_R5_EXP_WR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(OBSOLETE_R5_SDRD),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(R5_VR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(R5_RB),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(OBSOLETE_R5_RB_READ),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(OBSOLETE_ALG_WAS_R5_RB_SHED),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(OBSOLETE_ALG_WAS_R5_RB_BUFFER),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(R5_RD_VR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(R5_468_VR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(R5_MR3_VR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(R5_CORRUPT_DATA),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(RAID0_RD),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(RAID0_WR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(RAID0_VR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(OBSOLETE_RAID0_CORRUPT_CRC),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(MIRROR_RD),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(MIRROR_WR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(MIRROR_VR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(MIRROR_CORRUPT_DATA),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(MIRROR_RB),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(MIRROR_RD_VR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(OBSOLETE_RG_PASS_THRU),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(OBSOLETE_ALG_WAS_RG_EQUALIZE),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(OBSOLETE_ALG_WAS_R5_EQZ_VR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(MIRROR_COPY),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(MIRROR_COPY_VR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(OBSOLETE_ALG_WAS_R5_RD_REMAP),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(OBSOLETE_ALG_WAS_MIRROR_RD_REMAP),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(OBSOLETE_ALG_WAS_R0_REMAP_RVR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(OBSOLETE_R5_REMAP_RVR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(MIRROR_REMAP_RVR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(OBSOLETE_MIRROR_VR_BLOCKS),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(RG_ZERO),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(OBSOLETE_RG_SNIFF_VR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(R5_DEG_MR3),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(R5_DEG_468),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(R5_DEG_VR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(R5_DRD_VR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(MIRROR_VR_WR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(RAID0_BVA_VR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(OBSOLETE_R5_EXP_RB),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(RAID0_RD_VR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(R5_DEG_RVR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(MIRROR_PACO),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(MIRROR_VR_BUF),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(RG_CHECK_ZEROED),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(RG_FLUSH_JOURNAL),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(R5_SMALL_468),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(RG_FLUSH_JOURNAL_VR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(RG_WRITE_LOG_HDR_RD),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(R5_REKEY),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(MIRROR_REKEY),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(R5_RCW),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(R5_DEG_RCW),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(R5_RCW_VR),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RAID_SIOTS_ALGORITHM_LAST),
    FBE_DEBUG_ENUM_TRACE_INFO_TERMINATOR,
};

/*!*******************************************************************
 * @struct fbe_raid_debug_raid_group_type_trace_info
 *********************************************************************
 * @brief Provides a mapping of enum to string for raid group type
 *
 *********************************************************************/
fbe_debug_enum_trace_info_t fbe_raid_debug_raid_group_type_trace_info[] = 
{
    FBE_RAID_GROUP_TYPE_UNKNOWN, "UNKNOWN",
    FBE_RAID_GROUP_TYPE_RAID1, "RAID1",
    FBE_RAID_GROUP_TYPE_RAID10, "RAID10",
    FBE_RAID_GROUP_TYPE_RAID3, "RAID3",
    FBE_RAID_GROUP_TYPE_RAID0, "RAID0",
    FBE_RAID_GROUP_TYPE_RAID5, "RAID5",
    FBE_RAID_GROUP_TYPE_RAID6, "RAID6",
    FBE_RAID_GROUP_TYPE_SPARE, "SPARE",
    FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER, "INTERNAL_MIRROR_UNDER_STRIPER",
    FBE_RAID_GROUP_TYPE_INTERNAL_METADATA_MIRROR, "INTERNAL_METADATA_MIRROR",
    FBE_RAID_GROUP_TYPE_RAW_MIRROR, "RAW_MIRROR",
    FBE_DEBUG_ENUM_TRACE_INFO_TERMINATOR,
};

/*!*******************************************************************
 * @struct fbe_raid_debug_raid_geometry_flag_trace_info
 *********************************************************************
 * @brief Provides a mapping of enum to string for raid group geometry flags
 *
 *********************************************************************/
fbe_debug_enum_trace_info_t fbe_raid_debug_raid_geometry_flags_trace_info[] = 
{
    {FBE_RAID_GEOMETRY_FLAG_INITIALIZED, "INITIALIZED"},
    {FBE_RAID_GEOMETRY_FLAG_CONFIGURED, "CONFIGURED"},
    {FBE_RAID_GEOMETRY_FLAG_METADATA_CONFIGURED, "METADATA_CONFIGURED"},
    {FBE_RAID_GEOMETRY_FLAG_BLOCK_SIZE_VALID, "BLOCK_SIZE_VALID"},
    {FBE_RAID_GEOMETRY_FLAG_NEGOTIATE_FAILED, "NEGOTIATE_FAILED"},
    {FBE_RAID_GEOMETRY_FLAG_BLOCK_SIZE_INVALID, "BLOCK_SIZE_INVALID"},
    {FBE_RAID_GEOMETRY_FLAG_RAW_MIRROR, "RAW_MIRROR"},
    FBE_DEBUG_ENUM_TRACE_INFO_TERMINATOR,
};
/*!*******************************************************************
 * @struct fbe_raid_debug_raid_debug_trace_info
 *********************************************************************
 * @brief Provides a mapping of enum to string for raid group debug flags
 *
 *********************************************************************/
fbe_debug_enum_trace_info_t fbe_raid_debug_raid_debug_flags_trace_info[] = 
{    
    {FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_TRACING, "SIOTS_TRACING"},
    {FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING, "FRUTS_TRACING"},
    {FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_DATA_TRACING, "FRUTS_DATA_TRACING"},
    {FBE_RAID_LIBRARY_DEBUG_FLAG_REBUILD_TRACING, "REBUILD_TRACING"},
    {FBE_RAID_LIBRARY_DEBUG_FLAG_XOR_ERROR_TRACING, "XOR_ERROR_TRACING"},
    {FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING, "SIOTS_ERROR_TRACING"},
    {FBE_RAID_LIBRARY_DEBUG_FLAG_WRITE_SECTOR_CHECKING, "WRITE_SECTOR_CHECK"},
    {FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_INFO_TRACING, "MEMORY_INFO_TRACING"},
    {FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_TRACKING, "MEMORY_TRACKING"},
    {FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_TRACING, "MEMORY_TRACING"},
    {FBE_RAID_LIBRARY_DEBUG_FLAG_WRITE_JOURNAL_TRACING, "JOURNAL_TRACING"},
    {FBE_RAID_LIBRARY_DEBUG_FLAG_XOR_VALIDATE_DATA, "XOR_VALIDATE_DATA"},
    {FBE_RAID_LIBRARY_DEBUG_FLAG_XOR_ERROR_TRACING, "XOR_ERROR_TRACING"},
    {FBE_RAID_LIBRARY_DEBUG_FLAG_PATTERN_DATA_CHECKING, "PATTERN_CHECKING"},
    {FBE_RAID_LIBRARY_DEBUG_FLAG_INVALID_DATA_CHECKING, "INVALID_CHECKING"},
    {FBE_RAID_LIBRARY_DEBUG_FLAG_REBUILD_LOG_TRACING, "REBUILD_LOG_TRACING"},
    FBE_DEBUG_ENUM_TRACE_INFO_TERMINATOR,
};

/*!*******************************************************************
 * @struct fbe_raid_debug_raid_debug_trace_info
 *********************************************************************
 * @brief Provides a mapping of enum to string for raid group attribute flags
 *
 *********************************************************************/
fbe_debug_enum_trace_info_t fbe_raid_debug_raid_attribute_flags_trace_info[] = 
{
    FBE_RAID_ATTRIBUTE_NONE, "NONE",
    FBE_RAID_ATTRIBUTE_NON_OPTIMAL, "OPTIMAL",
    FBE_RAID_ATTRIBUTE_PROACTIVE_SPARING, "SPARING",
    FBE_RAID_ATTRIBUTE_VAULT, "VAULT",
    FBE_DEBUG_ENUM_TRACE_INFO_TERMINATOR,
};

/*!*******************************************************************
 * @struct fbe_raid_debug_write_slot_state_trace_info
 *********************************************************************
 * @brief Provides a mapping of enum to string for write log slot state
 *
 *********************************************************************/
fbe_debug_enum_trace_info_t fbe_raid_debug_write_slot_state_trace_info[] = 
{
    FBE_PARITY_WRITE_LOG_SLOT_STATE_INVALID, "INVALID",
    FBE_PARITY_WRITE_LOG_SLOT_STATE_FREE, "FREE",
    FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED, "ALLOCATED",
    FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED_FOR_FLUSH, "ALLOCATED FOR FLUSH",
    FBE_PARITY_WRITE_LOG_SLOT_STATE_FLUSHING, "FLUSHING",
    FBE_DEBUG_ENUM_TRACE_INFO_TERMINATOR,
};

/*!*******************************************************************
 * @struct fbe_raid_debug_write_slot_invalidate_state_trace_info
 *********************************************************************
 * @brief Provides a mapping of enum to string for write log slot invalidate state
 *
 *********************************************************************/
fbe_debug_enum_trace_info_t fbe_raid_debug_write_slot_invalidate_state_trace_info[] = 
{
    FBE_PARITY_WRITE_LOG_SLOT_INVALIDATE_STATE_SUCCESS, "SUCCESS",
    FBE_PARITY_WRITE_LOG_SLOT_INVALIDATE_STATE_DEAD, "DEAD",
    FBE_DEBUG_ENUM_TRACE_INFO_TERMINATOR,
};

/*!*******************************************************************
 * @def FBE_RAID_DEBUG_MAX_SYMBOL_NAME_LENGTH
 *********************************************************************
 * @brief This is the max length we use to size arrays that will hold
 *        a symbol name.
 *
 *********************************************************************/
#define FBE_RAID_DEBUG_MAX_SYMBOL_NAME_LENGTH 256

/*!**************************************************************
 * fbe_raid_library_debug_print_raid_common()
 ****************************************************************
 * @brief
 *  Display the raid common structure
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param common_p - ptr to common to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
static fbe_status_t fbe_raid_library_debug_print_raid_common(const fbe_u8_t * module_name,
                                                        fbe_dbgext_ptr common_p,
                                                        fbe_trace_func_t trace_func,
                                                        fbe_trace_context_t trace_context,
                                                        fbe_u32_t spaces_to_indent)
{
    fbe_u32_t flags;
    fbe_u32_t offset;

    /* Validate non-null
     */
    if (common_p != 0)
    {
        trace_func(trace_context, "common_p:0x%llx ",
		   (unsigned long long)common_p);
        FBE_GET_FIELD_OFFSET(module_name, fbe_raid_common_t, "flags", &offset);
        FBE_READ_MEMORY(common_p + offset, &flags, sizeof(fbe_u32_t));
        trace_func(trace_context, " fl:0x%x", flags);
        trace_func(trace_context, "\n");

        /* Display flags if they are set.
         */
        if (flags != 0)
        {
            fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
            fbe_debug_trace_flags_strings(module_name, common_p + offset, trace_func, trace_context, 
                                          NULL, spaces_to_indent, fbe_raid_debug_common_flags_trace_info);
            trace_func(trace_context, "\n");
        }
    }
    else
    {
        trace_func(trace_context, "\n");
    }

    return FBE_STATUS_OK;
}
/************************************************
 * end fbe_raid_library_debug_print_raid_common()
 ************************************************/


/*!**************************************************************
 * fbe_raid_library_debug_print_write_log_header_state()
 ****************************************************************
 * @brief
 *  Display the parity write slot state field.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  5/3/2012 - Created. Dave Agans
 *
 ****************************************************************/

fbe_status_t fbe_raid_library_debug_print_write_log_header_state(const fbe_u8_t * module_name,
                                                          fbe_dbgext_ptr base_ptr,
                                                          fbe_trace_func_t trace_func,
                                                          fbe_trace_context_t trace_context,
                                                          fbe_debug_field_info_t *field_info_p,
                                                          fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u16_t header_state = 0;

    /* Display state
     */
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &header_state, sizeof(fbe_u16_t));

    trace_func(trace_context, "header_state: ");

    switch (header_state)
    {
    case FBE_PARITY_WRITE_LOG_HEADER_STATE_VALID:
    {
        trace_func(trace_context, "VALID ");
        break;
    }
    case FBE_PARITY_WRITE_LOG_HEADER_STATE_INVALID:
    {
        trace_func(trace_context, "INVALID ");
        break;
    }
    default:
    {
        trace_func(trace_context, "GARBAGE (0x%x)", header_state);
        break;
    }
    }

    return status;
}
/**************************************
 * end fbe_raid_library_debug_print_write_log_header_state
 **************************************/

/*!*******************************************************************
 * @var fbe_parity_write_log_header_timestamp_field_info
 *********************************************************************
 * @brief Information about each of the fields of the write log header timestamp.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_parity_write_log_header_timestamp_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("sec", fbe_u64_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("usec", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_parity_write_log_header_timestamp_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        write log header timestamp.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_parity_write_log_header_timestamp_t,
                              fbe_parity_write_log_header_timestamp_struct_info,
                              &fbe_parity_write_log_header_timestamp_field_info[0]);


/*!*******************************************************************
 * @var fbe_parity_write_log_header_field_info
 *********************************************************************
 * @brief Information about each of the fields of the write log header.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_parity_write_log_header_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("write_bitmap", fbe_u16_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("start_lba", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("xfer_cnt", fbe_u16_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("parity_start", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("parity_cnt", fbe_u16_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("version", fbe_u64_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("header_state", fbe_u16_t, FBE_FALSE, "0x%x",
                                    fbe_raid_library_debug_print_write_log_header_state),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_parity_write_log_header_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        write log header.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_parity_write_log_header_t,
                              fbe_parity_write_log_header_struct_info,
                              &fbe_parity_write_log_header_field_info[0]);

/*!*******************************************************************
 * @var fbe_parity_write_log_header_disk_info_field_info
 *********************************************************************
 * @brief Information about each of the fields of the write log header disk info.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_parity_write_log_header_disk_info_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("offset", fbe_u16_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("block_cnt", fbe_u16_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("csum_of_csums", fbe_u16_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_parity_write_log_header_disk_info_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        write log header disk info.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_parity_write_log_header_disk_info_t,
                              fbe_parity_write_log_header_disk_info_struct_info,
                              &fbe_parity_write_log_header_disk_info_field_info[0]);


/*!**************************************************************
 * fbe_raid_library_debug_print_fruts_write_log_header()
 ****************************************************************
 * @brief
 *  Display the first buffer in the fruts if it's a valid version of a write log header
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param header_p - ptr to possible write log header to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  7/2/2012 - Created. Dave Agans
 *
 ****************************************************************/
static fbe_status_t fbe_raid_library_debug_print_fruts_write_log_header(const fbe_u8_t * module_name,
                                                                        fbe_dbgext_ptr header_p,
                                                                        fbe_trace_func_t trace_func,
                                                                        fbe_trace_context_t trace_context,
                                                                        fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u64_t field_p = 0;
    fbe_u32_t offset;
    fbe_u16_t header_state;
    fbe_u64_t header_version;

    FBE_GET_FIELD_OFFSET(module_name, fbe_parity_write_log_header_t, "header_version", &offset);
    field_p = header_p + offset;
    FBE_READ_MEMORY(field_p, &header_version, sizeof(fbe_u64_t));

    FBE_GET_FIELD_OFFSET(module_name, fbe_parity_write_log_header_t, "header_state", &offset);
    field_p = header_p + offset;
    FBE_READ_MEMORY(field_p, &header_state, sizeof(fbe_u16_t));

    /* when more versions are available, look for all and print version
    */
    if (   header_version == FBE_PARITY_WRITE_LOG_HEADER_VERSION_1
        && header_state == FBE_PARITY_WRITE_LOG_HEADER_STATE_VALID)
    {
        fbe_dbgext_ptr header_timestamp_p;

        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func (trace_context, "write_log_header: ");
        status = fbe_debug_display_structure(module_name, trace_func, trace_context, header_p,
                                             &fbe_parity_write_log_header_struct_info,
                                             5 /* fields per line */,
                                             spaces_to_indent + 2);

        if (status == FBE_STATUS_OK)
        {
            FBE_GET_FIELD_OFFSET(module_name, fbe_parity_write_log_header_t, "header_timestamp", &offset);
            header_timestamp_p = header_p + offset;

            trace_func (trace_context, "timestamp ");
            status = fbe_debug_display_structure(module_name, trace_func, trace_context, header_timestamp_p,
                                                 &fbe_parity_write_log_header_timestamp_struct_info,
                                                 2 /* fields per line */,
                                                 spaces_to_indent + 2);
        }
        if (status == FBE_STATUS_OK)
        {
            fbe_dbgext_ptr disk_info_array_p;
            fbe_u32_t disk_info_pos;
            fbe_dbgext_ptr disk_info_p;
            fbe_u16_t disk_info_offset;
            fbe_u16_t disk_info_block_cnt;
            fbe_u16_t disk_info_csum_of_csums;

            FBE_GET_FIELD_OFFSET(module_name, fbe_parity_write_log_header_t, "disk_info", &offset);
            disk_info_array_p = header_p + offset;

            for (disk_info_pos = 0; disk_info_pos < FBE_PARITY_MAX_WIDTH; disk_info_pos++)
            {
                disk_info_p = disk_info_array_p + (disk_info_pos * sizeof(fbe_parity_write_log_header_disk_info_t));

                FBE_GET_FIELD_OFFSET(module_name, fbe_parity_write_log_header_disk_info_t, "offset", &offset);
                field_p = disk_info_p + offset;
                FBE_READ_MEMORY(field_p, &disk_info_offset, sizeof(fbe_u16_t));
                FBE_GET_FIELD_OFFSET(module_name, fbe_parity_write_log_header_disk_info_t, "block_cnt", &offset);
                field_p = disk_info_p + offset;
                FBE_READ_MEMORY(field_p, &disk_info_block_cnt, sizeof(fbe_u16_t));
                FBE_GET_FIELD_OFFSET(module_name, fbe_parity_write_log_header_disk_info_t, "csum_of_csums", &offset);
                field_p = disk_info_p + offset;
                FBE_READ_MEMORY(field_p, &disk_info_csum_of_csums, sizeof(fbe_u16_t));

                if (   disk_info_offset != 0
                    || disk_info_block_cnt != 0
                    || disk_info_csum_of_csums != 0)
                {
                    trace_func (trace_context, "\n");
                    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 4);
                    trace_func (trace_context, "disk_info_pos: 0x%x ", disk_info_pos);
                    status = fbe_debug_display_structure(module_name, trace_func, trace_context, disk_info_p,
                                                         &fbe_parity_write_log_header_disk_info_struct_info,
                                                         6 /* fields per line */,
                                                         spaces_to_indent + 4);
                    if (status != FBE_STATUS_OK)
                    {
                        break;
                    }
                }
            }
        }
        trace_func (trace_context, "\n");
    }
    return (status);
}
/******************************************
 * end fbe_raid_library_debug_print_fruts()
 ******************************************/


/*!*******************************************************************
 * @var fbe_raid_fruts_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_raid_fruts_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("blocks", fbe_block_count_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("position", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("opcode", fbe_payload_block_operation_opcode_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_raid_fruts_flags_t, FBE_FALSE, "0x%x"), 
    FBE_DEBUG_DECLARE_FIELD_INFO("retry_count", fbe_u32_t, FBE_FALSE, "0x%x"), 
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),

    FBE_DEBUG_DECLARE_FIELD_INFO("sg_p", fbe_u64_t, FBE_FALSE, "0x%x"), 
    FBE_DEBUG_DECLARE_FIELD_INFO("sg_id", fbe_memory_id_t, FBE_FALSE, "0x%x"), 
    FBE_DEBUG_DECLARE_FIELD_INFO("sg_element_offset", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("status", fbe_payload_block_operation_status_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("status_qualifier", fbe_payload_block_operation_qualifier_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("media_error_lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("pvd_object_id", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),

    FBE_DEBUG_DECLARE_FIELD_INFO_FN("time_stamp", fbe_time_t, FBE_FALSE, "0x%x",
                                    fbe_debug_trace_time_brief),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("io_packet", fbe_packet_t, FBE_FALSE, "0x%x",
                                    fbe_debug_display_field_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_raid_fruts_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        fruts.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_raid_fruts_t,
                              fbe_raid_fruts_struct_info,
                              &fbe_raid_fruts_field_info[0]);
/*!**************************************************************
 * fbe_raid_library_debug_print_fruts()
 ****************************************************************
 * @brief
 *  Display the raid siots structure.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param fruts_p - ptr to fruts to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/23/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_library_debug_print_fruts(const fbe_u8_t * module_name,
                                                       fbe_dbgext_ptr fruts_p,
                                                       fbe_trace_func_t trace_func,
                                                       fbe_trace_context_t trace_context,
                                                       fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_u32_t ptr_size;
    fbe_u32_t preread_desc_offset;
    fbe_u32_t sg_memory_offset;
    fbe_u32_t packet_offset;
    fbe_u32_t packet_status_offset;
    fbe_block_count_t block_count;
    fbe_u64_t sg_list = 0;
    fbe_u64_t write_log_header_p = 0;
    fbe_u64_t sg_id = 0;
    fbe_u32_t sg_element_offset = 0;
    fbe_lba_t lba;

    /* This is used to align the second and subsequent lines
     * under the first line.
     */
    const char *align_string_p = "       ";
    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);

    trace_func(trace_context, "FRUTS: 0x%llx ", (unsigned long long)fruts_p);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, fruts_p,
                                         &fbe_raid_fruts_struct_info,
                                         100 /* fields per line */,
                                         spaces_to_indent + 7);

    /* If we have an sg id, then display a pointer to the sg list itself.
     */
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_sg_1_pool_entry_t, "sg", &sg_memory_offset);
    FBE_GET_FIELD_DATA(module_name, fruts_p, fbe_raid_fruts_t, sg_id, sizeof(sg_id), &sg_id);
    FBE_GET_FIELD_DATA(module_name, fruts_p, fbe_raid_fruts_t, sg_element_offset, sizeof(sg_element_offset), &sg_element_offset);
    if (sg_id != 0)
    {
        trace_func(trace_context, " memory:0x%llx ", (unsigned long long)sg_id + sg_memory_offset + (sizeof(fbe_sg_element_t) * sg_element_offset));
        FBE_GET_FIELD_DATA(module_name, sg_id + sg_memory_offset, fbe_sg_element_t, address, ptr_size, &write_log_header_p);
    }

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_fruts_t, "io_packet", &packet_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "packet_status", &packet_status_offset);
    fbe_packet_status_debug_trace_no_field_ptr(module_name, fruts_p + packet_offset + packet_status_offset,
                                               trace_func, trace_context, 0);

    trace_func(trace_context, "\n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "%s", align_string_p);

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_fruts_t, "write_preread_desc", &preread_desc_offset);

    FBE_GET_FIELD_DATA(module_name, fruts_p + preread_desc_offset, fbe_payload_pre_read_descriptor_t, lba, sizeof(lba), &lba);
    trace_func(trace_context, " preread desc:  lba:0x%llx", lba);
    FBE_GET_FIELD_DATA(module_name, fruts_p + preread_desc_offset, fbe_payload_pre_read_descriptor_t, block_count, sizeof(block_count), &block_count);
    trace_func(trace_context, " block_count:0x%llx", block_count);
    FBE_GET_FIELD_DATA(module_name, fruts_p + preread_desc_offset, fbe_payload_pre_read_descriptor_t, sg_list, sizeof(sg_list), &sg_list);
    trace_func(trace_context, " sg_list:0x%llx", sg_list);

    trace_func(trace_context, "\n");

    if (write_log_header_p != 0)
    {
        fbe_raid_library_debug_print_fruts_write_log_header(module_name, write_log_header_p, trace_func, trace_context, spaces_to_indent + 7);
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_library_debug_print_fruts()
 ******************************************/

/*!*******************************************************************
 * @var fbe_raid_fruts_summary_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_raid_fruts_summary_field_info[] =
{
    /* base config is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("blocks", fbe_block_count_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("position", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("opcode", fbe_payload_block_operation_opcode_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("time_stamp", fbe_time_t, FBE_FALSE, "0x%x",
                                    fbe_debug_trace_time_brief),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("io_packet", fbe_packet_t, FBE_FALSE, "0x%x",
                                    fbe_debug_display_field_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_raid_fruts_summary_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        fruts.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_raid_fruts_t,
                              fbe_raid_fruts_summary_struct_info,
                              &fbe_raid_fruts_summary_field_info[0]);

/*!**************************************************************
 * fbe_raid_library_debug_print_fruts_summary()
 ****************************************************************
 * @brief
 *  Display a summary view of the fruts.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param fruts_p - ptr to fruts to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  3/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_library_debug_print_fruts_summary(const fbe_u8_t * module_name,
                                                               fbe_dbgext_ptr fruts_p,
                                                               fbe_trace_func_t trace_func,
                                                               fbe_trace_context_t trace_context,
                                                               fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_u32_t ptr_size;
    fbe_u32_t flags_offset;
    fbe_u32_t flags;
    fbe_u32_t packet_offset;
    fbe_u64_t packet_p = 0;

    /* This is used to align the second and subsequent lines
     * under the first line.
     */
    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_fruts_t, "flags", &flags_offset);
    FBE_READ_MEMORY(fruts_p + flags_offset, &flags, sizeof(fbe_u32_t));

    /* Only display summary for outstanding fruts.
     */
    if ((flags & FBE_RAID_FRUTS_FLAG_REQUEST_OUTSTANDING) == 0)
    {
        return FBE_STATUS_OK;
    }
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);

    trace_func(trace_context, "FRUTS: 0x%llx ", (unsigned long long)fruts_p);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, fruts_p,
                                         &fbe_raid_fruts_summary_struct_info,
                                         15 /* all fields in one line. */,
                                         spaces_to_indent + 2);

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_fruts_t, "io_packet", &packet_offset);
    trace_func(trace_context, "\n");
    packet_p = fruts_p + packet_offset;
    fbe_transport_print_packet_summary(module_name, packet_p, trace_func, trace_context, spaces_to_indent + 1);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_library_debug_print_fruts_summary()
 ******************************************/

/*!**************************************************************
 * fbe_raid_library_debug_print_fruts_queue()
 ****************************************************************
 * @brief
 *  Display the raid fruts queue.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param siots_queue_p - ptr to siots queue to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/23/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_library_debug_print_fruts_queue(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr fruts_queue_p,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           const char *queue_name_p,
                                                           fbe_u32_t spaces_to_indent,
                                                           fbe_bool_t b_summary)
{
    fbe_u64_t fruts_queue_head_ptr;
    fbe_u64_t queue_head_p = 0;
    fbe_u64_t queue_element_p = 0;
    fbe_u64_t next_queue_element_p = 0;
    fbe_u64_t prev_queue_element_p = 0;
    fbe_u32_t ptr_size;
    fbe_u32_t length = 0;

    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

    fruts_queue_head_ptr = fruts_queue_p;
    
    FBE_READ_MEMORY(fruts_queue_head_ptr, &queue_head_p, ptr_size);
    queue_element_p = queue_head_p;

    /* If the list is not empty then display a header.
     */
    if (!b_summary)
    {
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(trace_context, "%s:\n", queue_name_p);
    }

    /* Now that we have the head of the list, iterate over it, displaying each fruts.
     * We finish when we reach the end of the list (head) or null. 
     */
    while ((queue_element_p != fruts_queue_head_ptr) && 
           (queue_element_p != NULL) &&
           (queue_element_p != prev_queue_element_p) &&
           (length < FBE_RAID_MAX_DISK_ARRAY_WIDTH))
    {
        fbe_u64_t fruts_p = queue_element_p;
        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_CANCELED;
        }
        if (b_summary)
        {
            fbe_raid_library_debug_print_fruts_summary(module_name, fruts_p, trace_func, trace_context, spaces_to_indent);
        }
        else
        {
            fbe_raid_library_debug_print_fruts(module_name, fruts_p, trace_func, trace_context, spaces_to_indent + 2);
        }
        FBE_READ_MEMORY(queue_element_p, &next_queue_element_p, ptr_size);
        prev_queue_element_p = queue_element_p;
        queue_element_p = next_queue_element_p;
        length++;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_library_debug_print_fruts_queue()
 ******************************************/
/*!**************************************************************
 * fbe_raid_library_debug_print_siots_history()
 ****************************************************************
 * @brief
 *  Display the entire raid siots state history.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param siots_p - ptr to siots to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/23/2009 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_raid_library_debug_print_siots_history(const fbe_u8_t * module_name,
                                                             fbe_dbgext_ptr siots_p,
                                                             fbe_trace_func_t trace_func,
                                                             fbe_trace_context_t trace_context,
                                                             fbe_u32_t spaces_to_indent)
{
    csx_string_t functionName;       /* Name of the function from the history. */
    fbe_u32_t state_count;          /* Counter of number of states output. */
    fbe_u64_t state_address = 0;
    fbe_u64_t state_value = 0;
    fbe_u32_t ptr_size;
    fbe_u32_t indent;
    fbe_u16_t state_index;  /* Entry to start printing at. */
    fbe_u32_t prev_state_offset;
    fbe_u64_t log_p = 0;
    fbe_u32_t max_log_entries = FBE_RAID_SIOTS_MAX_STACK;
    fbe_u32_t prev_state_stamp_offset;
    fbe_u64_t log_stamp_p = 0;
    fbe_u64_t stamp_address = 0;
    fbe_u32_t stamp_value = 0;
    fbe_u32_t prev_stamp_value = 0;
    fbe_u32_t delta_value_in_ms = 0;
    fbe_time_t siots_time_stamp = 0;
    fbe_u32_t perv_state_index = 0;  

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_siots_t, "prevState", &prev_state_offset);
    log_p = siots_p + prev_state_offset;
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_siots_t, "prevStateStamp", &prev_state_stamp_offset);
    log_stamp_p = siots_p + prev_state_stamp_offset;
    FBE_GET_FIELD_DATA(module_name, siots_p, fbe_raid_siots_t, state_index, sizeof(state_index), &state_index);

    FBE_GET_FIELD_DATA(module_name, siots_p, fbe_raid_siots_t, time_stamp, sizeof(fbe_time_t), &siots_time_stamp);

    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    
    trace_func(trace_context, "History: ");

    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

    /* Loop over all the states.
     */
    for (state_count = 0;
         state_count < max_log_entries; 
         state_count++)
    {
        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_CANCELED;
        }

        /* Update the index pointer.  Wrap around if we hit the beginning of
         * the circular buffer.
         */
        if (state_index == 0)
        {
            state_index = max_log_entries;
        }
        state_index--;
        perv_state_index = state_index ;
        if (perv_state_index == 0)
        {
            perv_state_index = max_log_entries;
        }
        perv_state_index--;

        /* Calculate and read in the state address and the value of the state.
         */
        state_address = (log_p + ( state_index * ptr_size) );

	    FBE_READ_MEMORY(state_address, &state_value, ptr_size);

        stamp_address = (log_stamp_p + ( state_index * sizeof(fbe_u32_t)) );
	    FBE_READ_MEMORY(stamp_address, &stamp_value, sizeof(fbe_u32_t));
        stamp_address = (log_stamp_p + ( perv_state_index * sizeof(fbe_u32_t)) );
        FBE_READ_MEMORY(stamp_address, &prev_stamp_value, sizeof(fbe_u32_t));
        if ((prev_stamp_value==0) || (state_count == (max_log_entries-1)))
        {
            prev_stamp_value = (fbe_u32_t) siots_time_stamp;  // only care about the lower 32 bits
            if (prev_stamp_value > stamp_value)
            {
                prev_stamp_value = stamp_value;
            }
        }
        delta_value_in_ms = stamp_value  - prev_stamp_value ;

        functionName = csx_dbg_ext_lookup_symbol_name(state_value);

        /* If we have no more data in the buffer, break out of the loop.
         */
        if ((functionName == NULL) || (*functionName == '\0'))
        {
            /* If functionName isn't NULL, csx dbg allocates the memory.
             * We should free it here
             */
            if (functionName != NULL)
                csx_dbg_ext_free_symbol_name(functionName);

            break;
        }
        else
        {
            /* Insert an extra newline every 2 states to keep output reasonable.
             */
            if ((state_count != 0) && (state_count % 3 == 0))
            {
                trace_func(trace_context, "\n");
    
                for (indent = 0; indent < spaces_to_indent; indent ++)
                {
                    trace_func(trace_context, " ");
                }
                trace_func(trace_context, "         ");
            }
            
            trace_func(trace_context, "[%d]:%s(%dms) ", state_count,  functionName, delta_value_in_ms);

        }/* end else function name not null. */
        csx_dbg_ext_free_symbol_name(functionName);

    } /* end for state_count < max_log_entries */

    trace_func(trace_context, "\n");

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_library_debug_print_siots_history()
 ******************************************/
/*!**************************************************************
 * fbe_raid_siots_debug_print_fruts_lists()
 ****************************************************************
 * @brief
 *  Display the lists of fruts inside the siots.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param siots_p - ptr to siots to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
static fbe_status_t fbe_raid_siots_debug_print_fruts_lists(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr siots_p,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_u32_t spaces_to_indent,
                                                           fbe_bool_t b_summary)
{
    fbe_u32_t fruts_queue_offset;
    fbe_u64_t fruts_queue_head_ptr;

    /* First display the read fruts queue.
     */
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_siots_t, "read_fruts_head", &fruts_queue_offset);
    fruts_queue_head_ptr = siots_p + fruts_queue_offset;
    fbe_raid_library_debug_print_fruts_queue(module_name, fruts_queue_head_ptr, trace_func, trace_context, 
                                             "read fruts",
                                             spaces_to_indent + 2 /* spaces to indent */,
                                             b_summary);

    /* First display the read2 fruts queue.
     */
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_siots_t, "read2_fruts_head", &fruts_queue_offset);
    fruts_queue_head_ptr = siots_p + fruts_queue_offset;
    fbe_raid_library_debug_print_fruts_queue(module_name, fruts_queue_head_ptr, trace_func, trace_context,  
                                             "read2 fruts",
                                             spaces_to_indent + 2 /* spaces to indent */,
                                             b_summary);

    /* First display the write fruts queue.
     */
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_siots_t, "write_fruts_head", &fruts_queue_offset);
    fruts_queue_head_ptr = siots_p + fruts_queue_offset;
    fbe_raid_library_debug_print_fruts_queue(module_name, fruts_queue_head_ptr, trace_func, trace_context, 
                                             "write fruts",
                                             spaces_to_indent + 2 /* spaces to indent */,
                                             b_summary);

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_siots_debug_print_fruts_lists()
 **************************************/

/*!**************************************************************
 * fbe_raid_library_debug_print_siots_algorithm()
 ****************************************************************
 * @brief
 *  Display the siots algorithm field.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  1/26/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_library_debug_print_siots_algorithm(const fbe_u8_t * module_name,
                                                          fbe_dbgext_ptr base_ptr,
                                                          fbe_trace_func_t trace_func,
                                                          fbe_trace_context_t trace_context,
                                                          fbe_debug_field_info_t *field_info_p,
                                                          fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_siots_algorithm_e algorithm = 0;

    /* Display algorithm if they are set.
     */
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &algorithm, sizeof(fbe_raid_siots_algorithm_e));

    trace_func(trace_context, "alg: 0x%x ", algorithm);

    status = fbe_debug_trace_enum_strings(module_name, base_ptr, trace_func, trace_context, 
                                          field_info_p, spaces_to_indent, fbe_raid_debug_siots_algorithm_trace_info);
    return status;
}
/**************************************
 * end fbe_raid_library_debug_print_siots_algorithm
 **************************************/

/*!**************************************************************
 * fbe_raid_library_debug_print_siots_flags()
 ****************************************************************
 * @brief
 *  Display the siots flags field.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param base_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  1/26/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_library_debug_print_siots_flags(const fbe_u8_t * module_name,
                                                      fbe_dbgext_ptr base_ptr,
                                                      fbe_trace_func_t trace_func,
                                                      fbe_trace_context_t trace_context,
                                                      fbe_debug_field_info_t *field_info_p,
                                                      fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t flags = 0;

    /* Display flags if they are set.
     */
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &flags, sizeof(fbe_u32_t));

    if (flags != 0)
    {
        fbe_debug_trace_flags_strings(module_name, base_ptr, trace_func, trace_context, 
                                      field_info_p, spaces_to_indent, fbe_raid_debug_siots_flags_trace_info);
    }
    return status;
}
/**************************************
 * end fbe_raid_library_debug_print_siots_flags
 **************************************/
/*!**************************************************************
 * fbe_raid_library_debug_print_siots_flags_brief()
 ****************************************************************
 * @brief
 *  Display the siots flags fields related to waiting.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param base_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  4/4/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_library_debug_print_siots_flags_brief(const fbe_u8_t * module_name,
                                                      fbe_dbgext_ptr base_ptr,
                                                      fbe_trace_func_t trace_func,
                                                      fbe_trace_context_t trace_context,
                                                      fbe_debug_field_info_t *field_info_p,
                                                      fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t flags = 0;

    /* Display flags if they are set.
     */
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &flags, sizeof(fbe_u32_t));

    if (flags != 0)
    {
        if (flags & FBE_RAID_SIOTS_FLAG_WAIT_LOCK)
        {
            trace_func(trace_context, "WAIT SIOTS Lock ");
        }
        if (flags & FBE_RAID_SIOTS_FLAG_QUIESCED)
        {
            trace_func(trace_context, "Quiesced ");
        }
        if (flags & FBE_RAID_SIOTS_FLAG_WAITING_SHUTDOWN_CONTINUE)
        {
            trace_func(trace_context, "WAIT shutdown continue ");
        }
        if (flags & FBE_RAID_SIOTS_FLAG_WAIT_WRITE_LOG_SLOT)
        {
            trace_func(trace_context, "WAIT write log slot ");
        }
    }
    return status;
}
/**************************************
 * end fbe_raid_library_debug_print_siots_flags_brief
 **************************************/
/*!**************************************************************
 * fbe_raid_library_debug_print_iots_flags_brief()
 ****************************************************************
 * @brief
 *  Display the siots flags fields related to waiting.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param base_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  4/4/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_library_debug_print_iots_flags_brief(const fbe_u8_t * module_name,
                                                      fbe_dbgext_ptr base_ptr,
                                                      fbe_trace_func_t trace_func,
                                                      fbe_trace_context_t trace_context,
                                                      fbe_debug_field_info_t *field_info_p,
                                                      fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t flags = 0;

    /* Display flags if they are set.
     */
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &flags, sizeof(fbe_u32_t));

    if (flags != 0)
    {
        if (flags & FBE_RAID_IOTS_FLAG_QUIESCE)
        {
            trace_func(trace_context, "Quiesce ");
        }
        if (flags & FBE_RAID_IOTS_FLAG_ABORT)
        {
            trace_func(trace_context, "Abort ");
        }
        if (flags & FBE_RAID_IOTS_FLAG_ABORT_FOR_SHUTDOWN)
        {
            trace_func(trace_context, "Abort-Shutdown ");
        }
        if (flags & FBE_RAID_IOTS_FLAG_WAS_QUIESCED)
        {
            trace_func(trace_context, "Was-Quiesced ");
        }
    }
    return status;
}
/**************************************
 * end fbe_raid_library_debug_print_iots_flags_brief
 **************************************/
/*!**************************************************************
 * fbe_raid_library_debug_print_common_flags_brief()
 ****************************************************************
 * @brief
 *  Display the siots flags fields related to waiting.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param base_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  4/4/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_library_debug_print_common_flags_brief(const fbe_u8_t * module_name,
                                                             fbe_dbgext_ptr base_ptr,
                                                             fbe_trace_func_t trace_func,
                                                             fbe_trace_context_t trace_context,
                                                             fbe_debug_field_info_t *field_info_p,
                                                             fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t flags = 0;

    /* Display flags if they are set.
     */
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &flags, sizeof(fbe_u32_t));

    if (flags != 0)
    {
        if (flags & FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY)
        {
            trace_func(trace_context, "WAIT memory ");
        }
        if (flags & FBE_RAID_COMMON_FLAG_MEMORY_REQUEST_ABORTED)
        {
            trace_func(trace_context, "Memory-Aborted");
        }
    }
    return status;
}
/**************************************
 * end fbe_raid_library_debug_print_common_flags_brief
 **************************************/
/*!**************************************************************
 * fbe_raid_library_debug_print_siots_write_log_slot()
 ****************************************************************
 * @brief
 *  Display the siots write log slot.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  5/02/2012 - Created. Dave Agans
 *
 ****************************************************************/

fbe_status_t fbe_raid_library_debug_print_siots_write_log_slot(const fbe_u8_t * module_name,
                                                          fbe_dbgext_ptr base_ptr,
                                                          fbe_trace_func_t trace_func,
                                                          fbe_trace_context_t trace_context,
                                                          fbe_debug_field_info_t *field_info_p,
                                                          fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t slot_id = FBE_PARITY_WRITE_LOG_INVALID_SLOT;
    fbe_dbgext_ptr raid_group_p;

    /* Display slot id and if they are set.
     */
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &slot_id, sizeof(fbe_u32_t));

    trace_func(trace_context, "write_log_slot_id: 0x%x ", slot_id);

    if (slot_id == FBE_PARITY_WRITE_LOG_INVALID_SLOT)
    {
        trace_func(trace_context, "INVALID - not allocated");
    }
    else
    {
        /* Get pointer to slot structure and print out the slot
         */
        raid_group_p = fbe_raid_library_debug_get_raid_geometry_from_siots(module_name,
                                                                           base_ptr);
        if (raid_group_p)
        {
            status = fbe_raid_library_debug_print_parity_write_log_slot(module_name,
                                                                        raid_group_p,
                                                                        trace_func,
                                                                        trace_context,
                                                                        spaces_to_indent + 2,
                                                                        slot_id);
        }
        else
        {
            trace_func(trace_context, "error - slot not found");
        }
    }

    return status;
}
/**************************************
 * end fbe_raid_library_debug_print_siots_write_log_slot
 **************************************/

/*!*******************************************************************
 * @var fbe_raid_siots_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_raid_siots_field_info[] =
{
    /* base config is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("common", fbe_raid_common_t, FBE_TRUE, "0x%x",
                                    fbe_raid_library_debug_print_common),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_raid_siots_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("start_lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("xfer_count", fbe_block_count_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("media_error_lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("error", fbe_payload_block_operation_status_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("qualifier", fbe_payload_block_operation_qualifier_t, FBE_FALSE, "0x%x"),

    FBE_DEBUG_DECLARE_FIELD_INFO("wait_count", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("algorithm", fbe_raid_siots_algorithm_e, FBE_FALSE, "0x%x",
                                    fbe_raid_library_debug_print_siots_algorithm),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("time_stamp", fbe_time_t, FBE_FALSE, "0x%x",
                                    fbe_debug_trace_time),

    FBE_DEBUG_DECLARE_FIELD_INFO("parity_start", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("parity_count", fbe_block_count_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("parity_pos", fbe_raid_position_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("start_pos", fbe_raid_position_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("dead_pos", fbe_raid_position_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("dead_pos_2", fbe_raid_position_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("data_disks", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("drive_operations", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("page_size", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_pages", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("total_blocks_to_allocate", fbe_block_count_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("continue_bitmask", fbe_u16_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("needs_continue_bitmask", fbe_u16_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("journal_slot_id", fbe_u32_t, FBE_FALSE, "0x%x",
                                    fbe_raid_library_debug_print_siots_write_log_slot),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("flags", fbe_raid_siots_flags_t, FBE_FALSE, "0x%x",
                                    fbe_raid_library_debug_print_siots_flags),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_raid_siots_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base config.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_raid_siots_t,
                              fbe_raid_siots_struct_info,
                              &fbe_raid_siots_field_info[0]);
/*!**************************************************************
 * fbe_raid_library_debug_print_siots()
 ****************************************************************
 * @brief
 *  Display the raid siots structure.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param siots_p - ptr to siots to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/23/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_library_debug_print_siots(const fbe_u8_t * module_name,
                                                fbe_dbgext_ptr siots_p,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context,
                                                fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_u32_t siots_queue_offset;
    fbe_u64_t siots_queue_head_ptr;

    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "SIOTS: 0x%llx ",
	       (unsigned long long)siots_p);

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, siots_p,
                                         &fbe_raid_siots_struct_info,
                                         6 /* fields per line */,
                                         spaces_to_indent + 2);
    trace_func(trace_context, "\n");
    if (status == FBE_STATUS_OK)
    { 
        status = fbe_raid_library_debug_print_siots_history(module_name, siots_p, trace_func, trace_context, 
                                                            spaces_to_indent + 2);
    }
    if (status == FBE_STATUS_OK)
    {
        status = fbe_raid_siots_debug_print_fruts_lists(module_name, siots_p, 
                                                        trace_func, trace_context, spaces_to_indent + 2,
                                                        FBE_FALSE);
    }

    /* Get the offset to the head of the siots queue and the first element on the siots 
     * queue. 
     */
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_siots_t, "nsiots_q", &siots_queue_offset);
    siots_queue_head_ptr = siots_p + siots_queue_offset;

    if (status == FBE_STATUS_OK)
    {
        /* Display nested siots queue.
         */
        status = fbe_raid_library_debug_print_siots_queue(module_name, siots_queue_head_ptr, 
                                                          trace_func, trace_context, spaces_to_indent + 2);
    }
    return status;
}
/******************************************
 * end fbe_raid_library_debug_print_siots()
 ******************************************/

/*!**************************************************************
 * fbe_raid_library_debug_print_siots_queue()
 ****************************************************************
 * @brief
 *  Display the raid siots structure.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param siots_queue_p - ptr to siots queue to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/23/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_library_debug_print_siots_queue(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr siots_queue_p,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_u32_t spaces_to_indent)
{
    fbe_u64_t siots_queue_head_ptr;
    fbe_u64_t queue_head_p = 0;
    fbe_u64_t queue_element_p = 0;
    fbe_u64_t next_queue_element_p = 0;
    fbe_u32_t ptr_size;

    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

    siots_queue_head_ptr = siots_queue_p;
    
    FBE_READ_MEMORY(siots_queue_head_ptr, &queue_head_p, ptr_size);
    queue_element_p = queue_head_p;

    if ((queue_element_p == siots_queue_head_ptr) || (queue_element_p == NULL))
    {
        return FBE_STATUS_OK;
    }

    /* Now that we have the head of the list, iterate over it, displaying each siots.
     * We finish when we reach the end of the list (head) or null. 
     */
    while ((queue_element_p != siots_queue_head_ptr) && (queue_element_p != NULL))
    {
        fbe_u64_t siots_p = queue_element_p;
        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_CANCELED;
        }
        fbe_raid_library_debug_print_siots(module_name, siots_p, trace_func, trace_context, spaces_to_indent);
        FBE_READ_MEMORY(queue_element_p, &next_queue_element_p, ptr_size);
        queue_element_p = next_queue_element_p;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_library_debug_print_siots_queue()
 ******************************************/


/*!**************************************************************
 * fbe_raid_library_write_log_queue_debug_trace()
 ****************************************************************
 * @brief
 *  Display the raid write log request queue.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param write_log_queue_p - ptr to write log queue to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  5/8/2012 - Created. Dave Agans
 *
 ****************************************************************/

static fbe_status_t fbe_raid_library_write_log_queue_debug_trace(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr write_log_queue_p,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_u32_t spaces_to_indent)
{
    fbe_u64_t write_log_queue_head_ptr;
    fbe_u64_t queue_head_p = 0;
    fbe_u64_t queue_element_p = 0;
    fbe_u64_t next_queue_element_p = 0;
    fbe_u32_t ptr_size;
    fbe_u64_t siots_count = 0;

    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

    write_log_queue_head_ptr = write_log_queue_p;
    
    FBE_READ_MEMORY(write_log_queue_head_ptr, &queue_head_p, ptr_size);
    queue_element_p = queue_head_p;


    trace_func(trace_context, "\n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "request_queue: ");

    /* Now that we have the head of the list, iterate over it, displaying each siots.
     * We finish when we reach the end of the list (head) or null. 
     */
    while ((queue_element_p != write_log_queue_head_ptr) && (queue_element_p != NULL))
    {
        fbe_u32_t offset = 0;
        fbe_u64_t siots_p = 0;

        if (siots_count >= 6)
        {
            trace_func(trace_context, "\n");
            fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 15);
            siots_count = 0;
        }

        FBE_GET_FIELD_OFFSET(module_name, fbe_raid_siots_t, "journal_q_elem", &offset);
        siots_p = queue_element_p - offset;

        trace_func(trace_context, "0x%llx ", (unsigned long long)siots_p);

        FBE_READ_MEMORY(queue_element_p, &next_queue_element_p, ptr_size);
        queue_element_p = next_queue_element_p;
        siots_count++;

        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_CANCELED;
        }
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_library_debug_print_siots_queue()
 ******************************************/

/*!*******************************************************************
 * @var fbe_debug_raid_write_log_info_field_info
 *********************************************************************
 * @brief Information about each of the fields of raid write log info.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_debug_raid_write_log_info_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("start_lba", fbe_lba_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("slot_size", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("slot_count", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("quiesced", fbe_bool_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_debug_raid_write_log_info_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        raid write log info structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_parity_write_log_info_t, 
                              fbe_debug_raid_write_log_info_struct_info,
                              &fbe_debug_raid_write_log_info_field_info[0]);


/*!*******************************************************************
 * @var fbe_debug_raid_geometry_field_info
 *********************************************************************
 * @brief Information about each of the fields of raid geometry.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_debug_raid_geometry_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("object_id", fbe_object_id_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("object_id", fbe_object_id_t, FBE_TRUE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("class_id", fbe_class_id_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("raid_type", fbe_raid_group_type_t, FBE_FALSE, "0x%x",
                                    fbe_raid_library_raid_type_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("geometry_flags", fbe_raid_geometry_flags_t, FBE_FALSE, "0x%x",
                                    fbe_raid_library_geometry_flags_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("debug_flags", fbe_raid_library_debug_flags_t, FBE_FALSE, "0x%x",
                                    fbe_raid_library_debug_flags_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("attribute_flags", fbe_raid_attribute_flags_t, FBE_FALSE, "0x%x",
                                    fbe_raid_library_attribute_flags_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO("prefered_mirror_pos", fbe_mirror_prefered_position_t , FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),

    FBE_DEBUG_DECLARE_FIELD_INFO_FN("generate_state", fbe_raid_common_state_t, FBE_FALSE, "0x%x",
                                    fbe_debug_display_function_ptr),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("element_size", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("elements_per_parity", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("bitmask_4k", fbe_raid_position_bitmask_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("exported_block_size", fbe_block_size_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("imported_block_size", fbe_block_size_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("optimal_block_size", fbe_block_size_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("max_blocks_per_drive", fbe_block_count_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("block_edge_p", fbe_u32_t*, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("width", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("configured_capacity", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("metadata_start_lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("metadata_capacity", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("metadata_copy_offset", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("metadata_element_p", fbe_u32_t*, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_debug_raid_geometry_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        raid geometry structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_raid_geometry_t, 
                              fbe_debug_raid_geometry_struct_info,
                              &fbe_debug_raid_geometry_field_info[0]);



/*!**************************************************************
 * fbe_raid_library_debug_print_raid_geometry()
 ****************************************************************
 * @brief
 *  Display the raid geo structure.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param raid_geometry_p - ptr to iots to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  3/11/2010 - Created. Rob Foley
 *  5/8/2012  - Added parity write log. Dave Agans
 *
 ****************************************************************/
fbe_status_t fbe_raid_library_debug_print_raid_geometry(const fbe_u8_t * module_name,
                                                        fbe_dbgext_ptr raid_geometry_p,
                                                        fbe_trace_func_t trace_func,
                                                        fbe_trace_context_t trace_context,
                                                        fbe_u32_t spaces_to_indent,
                                                        fbe_bool_t print_write_log_b)
{
    fbe_u64_t field_p = 0;
    fbe_u32_t offset;
    fbe_u32_t ptr_size;
    fbe_raid_group_type_t raid_type = 0;
    fbe_dbgext_ptr write_log_info_p;
    fbe_dbgext_ptr wait_queue_p;
    fbe_u32_t slot_id = 0;
    fbe_u32_t slot_count = 0;
    fbe_dbgext_ptr slot_array_p = 0;
    fbe_dbgext_ptr write_log_slot_p = 0;
    fbe_parity_write_log_slot_state_t slot_state;
    fbe_u32_t slot_state_count[FBE_PARITY_WRITE_LOG_SLOT_STATE_REMAPPING + 1];
    fbe_u32_t slot_state_index;

    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

    trace_func(trace_context, "raid_geometry_p:0x%llx ", (unsigned long long)raid_geometry_p);
    if (raid_geometry_p != 0)
    {
        trace_func(trace_context, "\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);	
        fbe_debug_display_structure(module_name, trace_func, trace_context, raid_geometry_p,
                                    &fbe_debug_raid_geometry_struct_info,
                                    4 /* fields per line */,
                                    spaces_to_indent);
        trace_func(trace_context, "\n");

        /* if this is a parity group, print the write log info
         */
        FBE_GET_FIELD_OFFSET(module_name, fbe_raid_geometry_t, "raid_type", &offset);
        field_p = raid_geometry_p + offset;
        FBE_READ_MEMORY(field_p, &raid_type, sizeof(fbe_raid_group_type_t));

        if (print_write_log_b
            &&(   raid_type == FBE_RAID_GROUP_TYPE_RAID3
               || raid_type == FBE_RAID_GROUP_TYPE_RAID5
               || raid_type == FBE_RAID_GROUP_TYPE_RAID6))
        {
            /* journal_info is a member of the raid_type_specific union, and write_log_info is the only member of that,
             * therefore write_log_info has the same address as raid_type_specific
             */
            FBE_GET_FIELD_OFFSET(module_name, fbe_raid_geometry_t, "raid_type_specific", &offset);
            field_p = raid_geometry_p + offset;
            FBE_READ_MEMORY(field_p, &write_log_info_p, ptr_size);

            if (write_log_info_p)
            {
            fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
            trace_func(trace_context, "parity write log:  ");
            fbe_debug_display_structure(module_name, trace_func, trace_context, write_log_info_p,
                                        &fbe_debug_raid_write_log_info_struct_info,
                                        5 /* fields per line */,
                                        spaces_to_indent + 2);

            /* Get the offset to the head of the waiting queue and display it if there are any siots. 
             */
            FBE_GET_FIELD_OFFSET(module_name, fbe_parity_write_log_info_t, "request_queue_head", &offset);
            wait_queue_p = write_log_info_p + offset;

            fbe_raid_library_write_log_queue_debug_trace(module_name, wait_queue_p, trace_func, trace_context,
                                                         spaces_to_indent + 2);

                /* Display aggregate slot info.
                 */
                FBE_GET_FIELD_OFFSET(module_name, fbe_parity_write_log_info_t, "slot_count", &offset);
                field_p = write_log_info_p + offset;
                FBE_READ_MEMORY(field_p, &slot_count, sizeof(fbe_u32_t));

                FBE_GET_FIELD_OFFSET(module_name, fbe_parity_write_log_info_t, "slot_array", &offset);
                slot_array_p = write_log_info_p + offset;

                /* get the state field offset once for use in each iteration of the loop */
                FBE_GET_FIELD_OFFSET(module_name, fbe_parity_write_log_slot_t, "state", &offset);

                /* Want to keep SPA and SPB totals separately, since they're used separately per SP
                 * Zero the counters, then SPA gets the first half of the slots
                 */
                for (slot_state_index = 0; slot_state_index <= FBE_PARITY_WRITE_LOG_SLOT_STATE_REMAPPING; slot_state_index++)
                {
                    slot_state_count[slot_state_index] = 0;
                }

                for (slot_id = 0; slot_id < (slot_count/2); slot_id++)
                {
                    write_log_slot_p = slot_array_p + (slot_id * sizeof(fbe_parity_write_log_slot_t));
                    field_p = write_log_slot_p + offset;
                    FBE_READ_MEMORY(field_p, &slot_state, sizeof(fbe_parity_write_log_slot_state_t));
                    if (slot_state <= FBE_PARITY_WRITE_LOG_SLOT_STATE_REMAPPING)
                    {
                        slot_state_count[slot_state] += 1;
                    }
                }

                trace_func(trace_context, "\n");
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 2);
                trace_func(trace_context, "SPA slot state totals: invalid: 0x%x  free: 0x%x  allocated: 0x%x\n",
                           slot_state_count[FBE_PARITY_WRITE_LOG_SLOT_STATE_INVALID],
                           slot_state_count[FBE_PARITY_WRITE_LOG_SLOT_STATE_FREE],
                           slot_state_count[FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED]);
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 2);
                trace_func(trace_context, "                       allocated_for_flush: 0x%x  allocated_for_remap: 0x%x  ",
                           slot_state_count[FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED_FOR_FLUSH],
                           slot_state_count[FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED_FOR_REMAP]);
                trace_func(trace_context, "flushing: 0x%x  remapping: 0x%x",
                           slot_state_count[FBE_PARITY_WRITE_LOG_SLOT_STATE_FLUSHING],
                           slot_state_count[FBE_PARITY_WRITE_LOG_SLOT_STATE_REMAPPING]);

                /* Zero the counters, then SPB gets the second half of the slots
             */
                for (slot_state_index = 0; slot_state_index <= FBE_PARITY_WRITE_LOG_SLOT_STATE_REMAPPING; slot_state_index++)
            {
                    slot_state_count[slot_state_index] = 0;
                }

                for (slot_id = (slot_count/2); slot_id < slot_count; slot_id++)
                {
                    write_log_slot_p = slot_array_p + (slot_id * sizeof(fbe_parity_write_log_slot_t));
                    field_p = write_log_slot_p + offset;
                    FBE_READ_MEMORY(field_p, &slot_state, sizeof(fbe_parity_write_log_slot_state_t));
                    if (slot_state <= FBE_PARITY_WRITE_LOG_SLOT_STATE_REMAPPING)
                    {
                        slot_state_count[slot_state] += 1;
                    }
                }

                trace_func(trace_context, "\n");
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 2);	
                trace_func(trace_context, "SPB slot state totals: invalid: 0x%x  free: 0x%x  allocated: 0x%x\n",
                           slot_state_count[FBE_PARITY_WRITE_LOG_SLOT_STATE_INVALID],
                           slot_state_count[FBE_PARITY_WRITE_LOG_SLOT_STATE_FREE],
                           slot_state_count[FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED]);
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 2);
                trace_func(trace_context, "                       allocated_for_flush: 0x%x  allocated_for_remap: 0x%x  ",
                           slot_state_count[FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED_FOR_FLUSH],
                           slot_state_count[FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED_FOR_REMAP]);
                trace_func(trace_context, "flushing: 0x%x  remapping: 0x%x",
                           slot_state_count[FBE_PARITY_WRITE_LOG_SLOT_STATE_FLUSHING],
                           slot_state_count[FBE_PARITY_WRITE_LOG_SLOT_STATE_REMAPPING]);
            }
        }

        fbe_trace_indent(trace_func, trace_context, 3);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_library_debug_print_raid_geometry()
 ******************************************/

/*!**************************************************************
 * fbe_raid_library_debug_print_full_raid_group_geometry()
 ****************************************************************
 * @brief
 *  Display the raid group geo structure with full write log info.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param raid_group_p - ptr to raid_group to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  5/08/2012 - Created. Dave Agans
 *
 ****************************************************************/
fbe_status_t fbe_raid_library_debug_print_full_raid_group_geometry(const fbe_u8_t * module_name,
                                                        fbe_dbgext_ptr raid_group_p,
                                                        fbe_trace_func_t trace_func,
                                                        fbe_trace_context_t trace_context,
                                                        fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t offset;
    fbe_u32_t ptr_size;
    fbe_dbgext_ptr raid_geometry_p = 0;

    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

    if (raid_group_p != 0)
    {
        FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_t, "geo", &offset);
        raid_geometry_p = raid_group_p + offset;

        status = fbe_raid_library_debug_print_raid_geometry(module_name, raid_geometry_p,
                                                            trace_func, trace_context, spaces_to_indent,
                                                            TRUE); /* print write log */
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_library_debug_print_full_raid_group_geometry()
 ******************************************/

/*!**************************************************************
 * fbe_raid_geometry_debug_trace()
 ****************************************************************
 * @brief
 *  Display all the trace information about the raid group object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param geo_p - Ptr to raid group object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 * @param spaces_to_indent - Spaces to indent this display.
 *
 * @return - FBE_STATUS_OK  
 *
 ****************************************************************/

fbe_status_t fbe_raid_geometry_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr base_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
                                           fbe_debug_field_info_t *field_info_p,
                                           fbe_u32_t overall_spaces_to_indent)
{
    fbe_raid_library_debug_print_raid_geometry(module_name, base_p + field_info_p->offset,
                                               trace_func, trace_context, overall_spaces_to_indent + 2,
                                               TRUE); /* print parity write log */
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_geometry_debug_trace()
 **************************************/

/*!**************************************************************
 * fbe_raid_geometry_ptr_debug_trace()
 ****************************************************************
 * @brief
 *  Display all the trace information about the raid geometry.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param base_p - Ptr to raid group object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 * @param spaces_to_indent - Spaces to indent this display.
 *
 * @return - FBE_STATUS_OK  
 *
 ****************************************************************/

fbe_status_t fbe_raid_geometry_ptr_debug_trace(const fbe_u8_t * module_name,
                                               fbe_dbgext_ptr base_p,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_debug_field_info_t *field_info_p,
                                               fbe_u32_t overall_spaces_to_indent)
{
    fbe_u64_t geometry_p = NULL;
    fbe_u32_t ptr_size; 
    fbe_debug_get_ptr_size(module_name, &ptr_size);

    /* The offset address contains a pointer to our geometry.
     * Thus we need to read the contents of this ptr, since the display function 
     * takes the pointer on the target. 
     */
    FBE_READ_MEMORY(base_p + field_info_p->offset, &geometry_p, ptr_size);

    fbe_trace_indent(trace_func, trace_context, overall_spaces_to_indent);
    fbe_raid_library_debug_print_raid_geometry(module_name, geometry_p,
                                               trace_func, trace_context, overall_spaces_to_indent + 2,
                                               FALSE);  /* don't print write log */
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_geometry_ptr_debug_trace()
 **************************************/
/*!**************************************************************
 * fbe_raid_library_debug_print_common_flags()
 ****************************************************************
 * @brief
 *  Display the common flags field.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  11/18/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_library_debug_print_common_flags(const fbe_u8_t * module_name,
                                                       fbe_dbgext_ptr base_ptr,
                                                       fbe_trace_func_t trace_func,
                                                       fbe_trace_context_t trace_context,
                                                       fbe_debug_field_info_t *field_info_p,
                                                       fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    trace_func(trace_context, "\n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    fbe_debug_trace_flags_strings(module_name, base_ptr, trace_func, trace_context, 
                                  field_info_p, spaces_to_indent, fbe_raid_debug_common_flags_trace_info);
    return status;
}
/**************************************
 * end fbe_raid_library_debug_print_common_flags
 **************************************/

/*!*******************************************************************
 * @var fbe_raid_common_field_info
 *********************************************************************
 * @brief Information about each of the fields of raid common.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_raid_common_field_info[] =
{
    /* base config is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_raid_common_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("parent_p", fbe_raid_common_t, FBE_FALSE, "0x%x",
                                    fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("state", fbe_raid_common_state_t, FBE_FALSE, "0x%x",
                                    fbe_debug_display_function_ptr),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("flags", fbe_raid_common_flags_t, FBE_FALSE, "0x%x",
                                    fbe_raid_library_debug_print_common_flags),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_raid_common_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        raid common.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_raid_common_t,
                              fbe_raid_common_struct_info,
                              &fbe_raid_common_field_info[0]);

/*!**************************************************************
 * fbe_raid_library_debug_print_common()
 ****************************************************************
 * @brief
 *  Display a pointer value.  This reads in the pointer located
 *  at this field's offset.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  3/15/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_library_debug_print_common(const fbe_u8_t * module_name,
                                                 fbe_dbgext_ptr base_ptr,
                                                 fbe_trace_func_t trace_func,
                                                 fbe_trace_context_t trace_context,
                                                 fbe_debug_field_info_t *field_info_p,
                                                 fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, 
                                         base_ptr + field_info_p->offset,
                                         &fbe_raid_common_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent + 2);
    return status;
}
/**************************************
 * end fbe_raid_library_debug_print_common
 **************************************/
/*!**************************************************************
 * fbe_raid_library_debug_print_iots_flags()
 ****************************************************************
 * @brief
 *  Display the iots flags field.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  11/17/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_library_debug_print_iots_flags(const fbe_u8_t * module_name,
                                                     fbe_dbgext_ptr base_ptr,
                                                     fbe_trace_func_t trace_func,
                                                     fbe_trace_context_t trace_context,
                                                     fbe_debug_field_info_t *field_info_p,
                                                     fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    status = fbe_debug_trace_flags_strings(module_name, base_ptr, trace_func, trace_context, 
                                           field_info_p, spaces_to_indent, fbe_raid_debug_iots_flags_trace_info);
    return status;
}
/**************************************
 * end fbe_raid_library_debug_print_iots_flags
 **************************************/
/*!**************************************************************
 * fbe_raid_library_debug_print_iots_status()
 ****************************************************************
 * @brief
 *  Display the iots flags field.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  11/17/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_library_debug_print_iots_status(const fbe_u8_t * module_name,
                                                     fbe_dbgext_ptr base_ptr,
                                                     fbe_trace_func_t trace_func,
                                                     fbe_trace_context_t trace_context,
                                                     fbe_debug_field_info_t *field_info_p,
                                                     fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    status = fbe_debug_trace_enum_strings(module_name, base_ptr, trace_func, trace_context, 
                                          field_info_p, spaces_to_indent, fbe_raid_debug_iots_status_trace_info);
    return status;
}
/**************************************
 * end fbe_raid_library_debug_print_iots_status
 **************************************/

/*!*******************************************************************
 * @var fbe_debug_raid_group_chunk_entry_field_info
 *********************************************************************
 * @brief Information about each of the fields of raid geometry.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_debug_raid_group_chunk_entry_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("verify_bits", fbe_u8_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("needs_rebuild_bits", fbe_raid_position_bitmask_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("rekey", fbe_u8_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_debug_raid_group_chunk_entry_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        raid geometry structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_raid_group_paged_metadata_t, 
                              fbe_debug_raid_group_chunk_entry_struct_info,
                              &fbe_debug_raid_group_chunk_entry_field_info[0]);



/*!**************************************************************
 * fbe_raid_library_debug_print_raid_group_chunk_entry()
 ****************************************************************
 * @brief
 *  Display the raid geo structure.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param raid_group_chunk_entry_p - ptr to iots to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  3/11/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_library_debug_print_raid_group_chunk_entry(const fbe_u8_t * module_name,
                                                                 fbe_dbgext_ptr raid_group_chunk_entry_p,
                                                                 fbe_trace_func_t trace_func,
                                                                 fbe_trace_context_t trace_context,
                                                                 fbe_u32_t spaces_to_indent)
{
    if (raid_group_chunk_entry_p != 0)
    {
        fbe_debug_display_structure(module_name, trace_func, trace_context, raid_group_chunk_entry_p,
                                    &fbe_debug_raid_group_chunk_entry_struct_info,
                                    4    /* fields per line */,
                                    spaces_to_indent);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_library_debug_print_raid_group_chunk_entry()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_chunk_entry_debug_trace()
 ****************************************************************
 * @brief
 *  Display all the trace information about the raid group's chunk info..
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param base_p - ptr to chunk info.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 * @param spaces_to_indent - Spaces to indent this display.
 *
 * @return - FBE_STATUS_OK  
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_chunk_entry_debug_trace(const fbe_u8_t * module_name,
                                                    fbe_dbgext_ptr base_p,
                                                    fbe_trace_func_t trace_func,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_debug_field_info_t *field_info_p,
                                                    fbe_u32_t overall_spaces_to_indent)
{
    fbe_u64_t chunk_info_p = base_p + field_info_p->offset;
    fbe_u32_t index;
    #define FBE_RAID_GROUP_CHUNK_ENTRY_PER_LINE 2

    /* Next display each of the chunk infos.
     */
    for ( index = 0; index < FBE_RAID_IOTS_MAX_CHUNKS; index++)
    {
        if ((index != 0) &&
            (index % FBE_RAID_GROUP_CHUNK_ENTRY_PER_LINE) == 0)
        {
            trace_func(trace_context, "\n");
            fbe_trace_indent(trace_func, trace_context, overall_spaces_to_indent);
        }

        /* Note we use index * size to offset into the current chunk to display.
         */
        trace_func(trace_context, "[%2d] ", index);
        fbe_raid_library_debug_print_raid_group_chunk_entry(module_name, 
                                                            chunk_info_p + (field_info_p->size * index),
                                                            trace_func, trace_context, overall_spaces_to_indent + 2);
    }
    trace_func(trace_context, "\n");
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_chunk_entry_debug_trace()
 **************************************/

/*!*******************************************************************
 * @var fbe_raid_iots_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_raid_iots_field_info[] =
{
    /* base config is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("common", fbe_raid_common_t, FBE_TRUE, "0x%x",
                                    fbe_raid_library_debug_print_common),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_raid_iots_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("callback", fbe_raid_iots_completion_function_t, FBE_FALSE, "0x%x", fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("callback_context", fbe_raid_iots_completion_context_t, FBE_FALSE, "0x%x", fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("packet_p", fbe_packet_t, FBE_FALSE, "0x%x", fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO("error", fbe_payload_block_operation_status_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("qualifier", fbe_payload_block_operation_qualifier_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("status", fbe_raid_iots_status_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("status", fbe_raid_iots_status_t, FBE_FALSE, "0x%x",
                                    fbe_raid_library_debug_print_iots_status),
    FBE_DEBUG_DECLARE_FIELD_INFO("lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("blocks", fbe_block_count_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("current_lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("outstanding_requests", fbe_raid_iots_status_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("blocks_remaining", fbe_raid_iots_status_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("blocks_transferred", fbe_raid_iots_status_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("host_start_offset", fbe_block_count_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("rebuild_logging_bitmask", fbe_raid_position_bitmask_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("rekey_bitmask", fbe_raid_position_bitmask_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("raid_geometry_p", fbe_raid_geometry_t, FBE_FALSE, "0x%x",
                                    fbe_raid_geometry_ptr_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("time_stamp", fbe_time_t, FBE_FALSE, "0x%x",
                                    fbe_debug_trace_time),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("flags", fbe_raid_iots_flags_t, FBE_FALSE, "0x%x",
                                    fbe_raid_library_debug_print_iots_flags),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("time_stamp", fbe_time_t, FBE_FALSE, "0x%x",
                                    fbe_debug_trace_time),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("chunk_info", fbe_raid_group_paged_metadata_t, FBE_FALSE, "0x%x",
                                    fbe_raid_group_chunk_entry_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_raid_iots_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base config.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_raid_iots_t,
                              fbe_raid_iots_struct_info,
                              &fbe_raid_iots_field_info[0]);
/*!**************************************************************
 * fbe_raid_library_debug_print_iots()
 ****************************************************************
 * @brief
 *  Display the raid siots structure.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param iots_p - ptr to iots to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/23/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_library_debug_print_iots(const fbe_u8_t * module_name,
                                               fbe_dbgext_ptr iots_p,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_u32_t overall_spaces_to_indent)
{
    fbe_u64_t siots_queue_head_ptr = 0;
    fbe_u32_t siots_queue_offset;
    fbe_status_t status;
    fbe_u32_t spaces_to_indent = overall_spaces_to_indent + 4;

    fbe_trace_indent(trace_func, trace_context, overall_spaces_to_indent);
    trace_func(trace_context, "IOTS: 0x%llx ",
	       (unsigned long long)iots_p);

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, iots_p,
                                         &fbe_raid_iots_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);
    
    /* Get the offset to the head of the siots queue and the first element on the siots 
     * queue. 
     */
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_iots_t, "siots_queue", &siots_queue_offset);
    siots_queue_head_ptr = iots_p + siots_queue_offset;

    /* Display all the individual siots.
     */
    fbe_raid_library_debug_print_siots_queue(module_name, siots_queue_head_ptr, trace_func, trace_context, 
                                           spaces_to_indent + 2 /* spaces to indent */);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_library_debug_print_iots()
 ******************************************/
 
/*!**************************************************************
 * fbe_raid_library_raid_type_debug_trace()
 ****************************************************************
 * @brief
 *  Display the raid type
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param geo_p - Ptr to raid group object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 * @param spaces_to_indent - Spaces to indent this display.
 *
 * @return - FBE_STATUS_OK  
 *
 ****************************************************************/

fbe_status_t fbe_raid_library_raid_type_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr base_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
                                           fbe_debug_field_info_t *field_info_p,
                                           fbe_u32_t overall_spaces_to_indent)
{
    fbe_raid_group_type_t raid_type;
    fbe_status_t status = FBE_STATUS_OK;
    if (field_info_p->size != sizeof(fbe_raid_group_type_t))
    {
        trace_func(trace_context, "path state size is %d not %llu\n",
                   field_info_p->size, (unsigned long long)sizeof(fbe_u32_t));
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    FBE_READ_MEMORY(base_p + field_info_p->offset, &raid_type, field_info_p->size);
	
    trace_func(trace_context, "%s: %d ( ", field_info_p->name, raid_type);
    status = fbe_debug_trace_enum_strings(module_name, base_p, trace_func, trace_context, 
                                          field_info_p, overall_spaces_to_indent, fbe_raid_debug_raid_group_type_trace_info);
    trace_func(trace_context, ") ");
    return status;

}
/******************************************
 * end fbe_raid_library_geometry_flags_debug_trace()
 ******************************************/
/*!**************************************************************
 * fbe_raid_library_geometry_flags_debug_trace()
 ****************************************************************
 * @brief
 *  Display the geometry flags of raid group object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param geo_p - Ptr to raid group object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 * @param spaces_to_indent - Spaces to indent this display.
 *
 * @return - FBE_STATUS_OK  
 *
 ****************************************************************/

fbe_status_t fbe_raid_library_geometry_flags_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr base_ptr,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
                                           fbe_debug_field_info_t *field_info_p,
                                           fbe_u32_t overall_spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t flags = 0;

    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &flags, sizeof(fbe_u32_t));
    trace_func(trace_context, "%s: 0x%x ( ", field_info_p->name, flags);

    /* Display flags if they are set.
     */
    if (flags != 0)
    {
        fbe_debug_trace_flags_strings(module_name, base_ptr, trace_func, trace_context, 
                                      field_info_p, overall_spaces_to_indent, fbe_raid_debug_raid_geometry_flags_trace_info);
    }
    else
    {
        trace_func(trace_context, " NONE ");
   }
   trace_func(trace_context,") ");
    return status;
}

/******************************************
 * end fbe_raid_library_geometry_flags_debug_trace()
 ******************************************/
/*!**************************************************************
 * fbe_raid_library_debug_flags_debug_trace()
 ****************************************************************
 * @brief
 *  Display the debug flags of raid group object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param geo_p - Ptr to raid group object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 * @param spaces_to_indent - Spaces to indent this display.
 *
 * @return - FBE_STATUS_OK  
 *
 ****************************************************************/

fbe_status_t fbe_raid_library_debug_flags_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr base_ptr,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
                                           fbe_debug_field_info_t *field_info_p,
                                           fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t flags = 0;

    /* Display flags if they are set.
     */
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &flags, sizeof(fbe_u32_t));
    trace_func(trace_context, "%s: 0x%x ( ", field_info_p->name, flags);

    if (flags != 0)
    {
        fbe_debug_trace_flags_strings(module_name, base_ptr, trace_func, trace_context, 
                                      field_info_p, spaces_to_indent, fbe_raid_debug_raid_debug_flags_trace_info);
    }
    else
    {
             trace_func(trace_context, "NONE ");
   }
   trace_func(trace_context, ") ");
    return status;
}
/******************************************
 * end fbe_raid_library_debug_flags_debug_trace()
 ******************************************/
 /*!**************************************************************
 * fbe_raid_library_attribute_flags_debug_trace()
 ****************************************************************
 * @brief
 *  Display the attribute flags of raid group object.
 *
 * @param module_name - This is the name of the module we are debugging.
 * @param geo_p - Ptr to raid group object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 * @param spaces_to_indent - Spaces to indent this display.
 *
 * @return - FBE_STATUS_OK  
 *
 ****************************************************************/

fbe_status_t fbe_raid_library_attribute_flags_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr base_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
                                           fbe_debug_field_info_t *field_info_p,
                                           fbe_u32_t overall_spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_attribute_flags_t attribute_flag;
    if (field_info_p->size != sizeof(fbe_raid_attribute_flags_t))
    {
        trace_func(trace_context, "path state size is %d not %llu\n",
                   field_info_p->size, (unsigned long long)sizeof(fbe_u32_t));
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    FBE_READ_MEMORY(base_p + field_info_p->offset, &attribute_flag, field_info_p->size);
    trace_func(trace_context, "%s:  0x%x ( ", field_info_p->name, attribute_flag);

    if (attribute_flag != 0)
    {
        fbe_debug_trace_flags_strings(module_name, base_p, trace_func, trace_context, 
                                      field_info_p, overall_spaces_to_indent, fbe_raid_debug_raid_attribute_flags_trace_info);
    }
    else
    {
             trace_func(trace_context, "NONE ");

   }
   trace_func(trace_context, ") ");
    return status;
}

/******************************************
 * end fbe_raid_library_attribute_flags_debug_trace()
 ******************************************/

/*!**************************************************************
 * fbe_raid_library_debug_get_raid_geometry_from_iots()
 ****************************************************************
 * @brief
 *  Get the raid_geometry ptr from an iots.
 *
 * @param module_name - This is the name of the module we are debugging
 * @param iots_p - ptr to iots
 * 
 * @return fbe_dbgext_ptr - ptr to raid_group struct   
 *
 * @author
 *  5/3/2012 - Created. Dave Agans
 *
 ****************************************************************/

fbe_dbgext_ptr fbe_raid_library_debug_get_raid_geometry_from_iots(const fbe_u8_t * module_name,
                                                                        fbe_dbgext_ptr iots_p)
{

    fbe_u64_t field_p = 0;
    fbe_u32_t offset;
    fbe_u32_t ptr_size;
    fbe_u64_t raid_geometry_p = 0;

    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

    if (iots_p)
    {
        FBE_GET_FIELD_OFFSET(module_name, fbe_raid_iots_t, "raid_geometry_p", &offset);
        field_p = iots_p + offset;
        FBE_READ_MEMORY(field_p, &raid_geometry_p, ptr_size);
    }

    return (fbe_dbgext_ptr)raid_geometry_p;
}
/**************************************
 * end fbe_raid_library_debug_get_raid_geometry_from_iots
 **************************************/

/*!**************************************************************
 * fbe_raid_library_debug_get_raid_geometry_from_siots()
 ****************************************************************
 * @brief
 *  Get the raid_geometry ptr from a siots.
 *
 * @param module_name - This is the name of the module we are debugging
 * @param siots_p - ptr to siots
 * 
 * @return fbe_dbgext_ptr - ptr to raid_geometry struct   
 *
 * @author
 *  5/3/2012 - Created. Dave Agans
 *
 ****************************************************************/

fbe_dbgext_ptr fbe_raid_library_debug_get_raid_geometry_from_siots(const fbe_u8_t * module_name,
                                                                   fbe_dbgext_ptr siots_p)
{

    fbe_u64_t siots_common_p = 0; 
    fbe_u64_t field_p = 0;
    fbe_u64_t siots_common_parent_p = 0;
    fbe_u32_t offset;
    fbe_u32_t flags; 
    fbe_u32_t ptr_size;
    fbe_dbgext_ptr raid_geometry_p = 0;

    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

    /* Depending on the type, find the iots.  
     */
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_siots_t, "common", &offset);
    siots_common_p = siots_p + offset;
    FBE_GET_FIELD_DATA(module_name, siots_common_p, fbe_raid_common_t, flags, sizeof(flags), &flags);
    switch(flags & FBE_RAID_COMMON_FLAG_TYPE_ALL_STRUCT_TYPE_MASK)
    {
        case FBE_RAID_COMMON_FLAG_TYPE_NEST_SIOTS:
            /* Nested siots,
             * Go all the way up to the IOTS and set siots_common_parent.
             */
            FBE_GET_FIELD_OFFSET(module_name, fbe_raid_common_t, "parent_p", &offset);
            field_p = siots_common_p + offset;
            FBE_READ_MEMORY(field_p, &siots_common_parent_p, ptr_size);
            siots_common_p = siots_common_parent_p;
            field_p = siots_common_p + offset;
            FBE_READ_MEMORY(field_p, &siots_common_parent_p, ptr_size);
            break;

        case FBE_RAID_COMMON_FLAG_TYPE_SIOTS:
            /* Regular SIOTS, iots is direct parent.
             */
            FBE_GET_FIELD_OFFSET(module_name, fbe_raid_common_t, "parent_p", &offset);
            field_p = siots_common_p + offset;
            FBE_READ_MEMORY(field_p, &siots_common_parent_p, ptr_size);
            break;
        default:
            break;
    } /* end switch siots_p->common.flags & RAID_ALL_STRUCT_TYPE_MASK */

    if (siots_common_parent_p)
    {
        raid_geometry_p = fbe_raid_library_debug_get_raid_geometry_from_iots(module_name,
                                                                             siots_common_parent_p);
    }

    return raid_geometry_p;
}
/**************************************
 * end fbe_raid_library_debug_get_raid_geometry_from_siots
 **************************************/

/*!**************************************************************
 * fbe_raid_library_debug_print_write_log_slot_state()
 ****************************************************************
 * @brief
 *  Display the parity write slot state field.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  5/3/2012 - Created. Dave Agans
 *
 ****************************************************************/

fbe_status_t fbe_raid_library_debug_print_write_log_slot_state(const fbe_u8_t * module_name,
                                                          fbe_dbgext_ptr base_ptr,
                                                          fbe_trace_func_t trace_func,
                                                          fbe_trace_context_t trace_context,
                                                          fbe_debug_field_info_t *field_info_p,
                                                          fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_parity_write_log_slot_state_t state = FBE_PARITY_WRITE_LOG_SLOT_STATE_INVALID;

    /* Display state
     */
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &state, sizeof(fbe_parity_write_log_slot_state_t));

    trace_func(trace_context, "state: ");

    status = fbe_debug_trace_enum_strings(module_name, base_ptr, trace_func, trace_context, 
                                          field_info_p, spaces_to_indent, fbe_raid_debug_write_slot_state_trace_info);
    return status;
}
/**************************************
 * end fbe_raid_library_debug_print_write_log_slot_state
 **************************************/

/*!**************************************************************
 * fbe_raid_library_debug_print_write_log_slot_invalidate_state()
 ****************************************************************
 * @brief
 *  Display the parity write slot invalidate state field.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  5/3/2012 - Created. Dave Agans
 *
 ****************************************************************/

fbe_status_t fbe_raid_library_debug_print_write_log_slot_invalidate_state(const fbe_u8_t * module_name,
                                                          fbe_dbgext_ptr base_ptr,
                                                          fbe_trace_func_t trace_func,
                                                          fbe_trace_context_t trace_context,
                                                          fbe_debug_field_info_t *field_info_p,
                                                          fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_parity_write_log_slot_invalidate_state_t state = FBE_PARITY_WRITE_LOG_SLOT_INVALIDATE_STATE_SUCCESS;

    /* Display invalidate state
     */
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &state, sizeof(fbe_parity_write_log_slot_state_t));

    trace_func(trace_context, "invalidate_state: ");

    status = fbe_debug_trace_enum_strings(module_name, base_ptr, trace_func, trace_context, 
                                          field_info_p, spaces_to_indent, fbe_raid_debug_write_slot_invalidate_state_trace_info);
    return status;
}
/**************************************
 * end fbe_raid_library_debug_print_write_log_slot_invalidate_state
 **************************************/

/*!*******************************************************************
 * @var fbe_parity_write_log_slot_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_parity_write_log_slot_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("state", fbe_parity_write_log_slot_state_t, FBE_TRUE, "0x%x",
                                    fbe_raid_library_debug_print_write_log_slot_state),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("invalidate_state", fbe_parity_write_log_slot_invalidate_state_t, FBE_FALSE, "0x%x",
                                    fbe_raid_library_debug_print_write_log_slot_invalidate_state),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_parity_write_log_slot_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base config.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_parity_write_log_slot_t,
                              fbe_parity_write_log_slot_struct_info,
                              &fbe_parity_write_log_slot_field_info[0]);

/*!**************************************************************
 * fbe_raid_library_debug_print_parity_write_log_slot()
 ****************************************************************
 * @brief
 *  Display the parity write log slot structure.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param raid_geometry_p - ptr to raid_geometry containing slot to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * @param slot_id - slot to display
 * 
 * @return fbe_status_t
 *
 * @author
 *  5/3/2012 - Created. Dave Agans
 *
 ****************************************************************/

fbe_status_t fbe_raid_library_debug_print_parity_write_log_slot(const fbe_u8_t * module_name,
                                                                fbe_dbgext_ptr raid_geometry_p,
                                                                fbe_trace_func_t trace_func,
                                                                fbe_trace_context_t trace_context,
                                                                fbe_u32_t spaces_to_indent,
                                                                fbe_u32_t slot_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u64_t field_p = 0;
    fbe_u32_t offset;
    fbe_u32_t ptr_size;
    fbe_dbgext_ptr write_log_info_p = 0;
    fbe_dbgext_ptr write_log_slot_p = 0;
    fbe_u32_t width = 0;
    fbe_u32_t slot_count = 0;

    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

    if (raid_geometry_p)
    {
        /* journal_info is a member of the raid_type_specific union, and write_log_info is the only member of that,
         * therefore write_log_info has the same address as raid_type_specific
         */
        FBE_GET_FIELD_OFFSET(module_name, fbe_raid_geometry_t, "raid_type_specific", &offset);
        field_p = raid_geometry_p + offset;
        FBE_READ_MEMORY(field_p, &write_log_info_p, ptr_size);

        FBE_GET_FIELD_OFFSET(module_name, fbe_raid_geometry_t, "width", &offset);
        field_p = raid_geometry_p + offset;
        FBE_READ_MEMORY(field_p, &width, sizeof(fbe_u32_t));
    }
    if (write_log_info_p)
    {
        FBE_GET_FIELD_OFFSET(module_name, fbe_parity_write_log_info_t, "slot_array", &offset);
        write_log_slot_p = write_log_info_p + offset;
        FBE_GET_FIELD_OFFSET(module_name, fbe_parity_write_log_info_t, "slot_count", &offset);
        field_p = write_log_info_p + offset;
        FBE_READ_MEMORY(field_p, &slot_count, sizeof(fbe_u32_t));
    }
    if (write_log_slot_p)
    {
        if (slot_id < slot_count)
        {
            write_log_slot_p += slot_id * sizeof(fbe_parity_write_log_slot_t);

        trace_func(trace_context, "slot_p: 0x%llx ", (unsigned long long)write_log_slot_p);

        status = fbe_debug_display_structure(module_name, trace_func, trace_context, write_log_slot_p,
                                             &fbe_parity_write_log_slot_struct_info,
                                             6 /* fields per line */,
                                             spaces_to_indent + 2);
    }
        else
        {
            trace_func(trace_context, "write log slot_ID 0x%x out of range 0x%x", slot_id, slot_count);    
        }
    }
    return status;
}
/******************************************
 * end fbe_raid_library_debug_print_parity_write_log_slot()
 ******************************************/

/*!*******************************************************************
 * @var fbe_raid_common_summary_field_info
 *********************************************************************
 * @brief Information about each of the fields of common.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_raid_common_summary_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("flags", fbe_raid_common_flags_t, FBE_FALSE, "0x%x",
                                    fbe_raid_library_debug_print_common_flags_brief),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_raid_common_summary_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        common summary.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_raid_common_t,
                              fbe_raid_common_summary_struct_info,
                              &fbe_raid_common_summary_field_info[0]);
/*!*******************************************************************
 * @var fbe_raid_siots_summary_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_raid_siots_summary_field_info[] =
{
    /* base config is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("start_lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("xfer_count", fbe_block_count_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("wait_count", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("algorithm", fbe_raid_siots_algorithm_e, FBE_FALSE, "0x%x",
                                    fbe_raid_library_debug_print_siots_algorithm),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("time_stamp", fbe_time_t, FBE_FALSE, "0x%x",
                                    fbe_debug_trace_time_brief),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("flags", fbe_raid_siots_flags_t, FBE_FALSE, "0x%x",
                                    fbe_raid_library_debug_print_siots_flags_brief),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_raid_siots_summary_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base config.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_raid_siots_t,
                              fbe_raid_siots_summary_struct_info,
                              &fbe_raid_siots_summary_field_info[0]);
/*!**************************************************************
 * fbe_raid_library_debug_print_siots_summary()
 ****************************************************************
 * @brief
 *  Display a summary of the raid siots structure.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param siots_p - ptr to siots to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  3/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_library_debug_print_siots_summary(const fbe_u8_t * module_name,
                                                        fbe_dbgext_ptr siots_p,
                                                        fbe_trace_func_t trace_func,
                                                        fbe_trace_context_t trace_context,
                                                        fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_u32_t siots_queue_offset;
    fbe_u64_t siots_queue_head_ptr;

    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "SIOTS: 0x%llx ", (unsigned long long)siots_p);

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, siots_p,
                                         &fbe_raid_siots_summary_struct_info,
                                         20 /* fields per line */,
                                         spaces_to_indent + 2);

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, siots_p,
                                         &fbe_raid_common_summary_struct_info,
                                         20 /* fields per line */,
                                         spaces_to_indent + 2);
    trace_func(trace_context, "\n");
    
    if (status == FBE_STATUS_OK)
    {
        status = fbe_raid_siots_debug_print_fruts_lists(module_name, siots_p, 
                                                        trace_func, trace_context, spaces_to_indent + 2,
                                                        FBE_TRUE /* Yes summary */);
    }

    /* Get the offset to the head of the siots queue and the first element on the siots 
     * queue. 
     */
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_siots_t, "nsiots_q", &siots_queue_offset);
    siots_queue_head_ptr = siots_p + siots_queue_offset;

    if (status == FBE_STATUS_OK)
    {
        /* Display nested siots queue.
         */
        status = fbe_raid_library_debug_print_siots_queue_summary(module_name, siots_queue_head_ptr, 
                                                                  trace_func, trace_context, spaces_to_indent + 2);
    }
    return status;
}
/******************************************
 * end fbe_raid_library_debug_print_siots()
 ******************************************/
/*!**************************************************************
 * fbe_raid_library_debug_print_siots_queue_summary()
 ****************************************************************
 * @brief
 *  Display summary of the siots queue.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param siots_queue_p - ptr to siots queue to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  3/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_library_debug_print_siots_queue_summary(const fbe_u8_t * module_name,
                                                                     fbe_dbgext_ptr siots_queue_p,
                                                                     fbe_trace_func_t trace_func,
                                                                     fbe_trace_context_t trace_context,
                                                                     fbe_u32_t spaces_to_indent)
{
    fbe_u64_t siots_queue_head_ptr;
    fbe_u64_t queue_head_p = 0;
    fbe_u64_t queue_element_p = 0;
    fbe_u64_t next_queue_element_p = 0;
    fbe_u32_t ptr_size;

    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

    siots_queue_head_ptr = siots_queue_p;
    
    FBE_READ_MEMORY(siots_queue_head_ptr, &queue_head_p, ptr_size);
    queue_element_p = queue_head_p;

    if ((queue_element_p == siots_queue_head_ptr) || (queue_element_p == NULL))
    {
        return FBE_STATUS_OK;
    }

    /* Now that we have the head of the list, iterate over it, displaying each siots.
     * We finish when we reach the end of the list (head) or null. 
     */
    while ((queue_element_p != siots_queue_head_ptr) && (queue_element_p != NULL))
    {
        fbe_u64_t siots_p = queue_element_p;
        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_CANCELED;
        }
        fbe_raid_library_debug_print_siots_summary(module_name, siots_p, trace_func, trace_context, spaces_to_indent);
        FBE_READ_MEMORY(queue_element_p, &next_queue_element_p, ptr_size);
        queue_element_p = next_queue_element_p;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_library_debug_print_siots_queue_summary()
 ******************************************/
/*!*******************************************************************
 * @var fbe_raid_iots_summary_field_info
 *********************************************************************
 * @brief Information about each of the fields of iots.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_raid_iots_summary_field_info[] =
{
    /* base config is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("blocks", fbe_block_count_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("time_stamp", fbe_time_t, FBE_FALSE, "0x%x",
                                    fbe_debug_trace_time_brief),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("flags", fbe_raid_iots_flags_t, FBE_FALSE, "0x%x",
                                    fbe_raid_library_debug_print_iots_flags_brief),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_raid_iots_summary_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base config.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_raid_iots_t,
                              fbe_raid_iots_summary_struct_info,
                              &fbe_raid_iots_summary_field_info[0]);

/*!*******************************************************************
 * @var fbe_raid_common_state_field_info
 *********************************************************************
 * @brief Information about each of the fields of iots.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_raid_common_state_field_info[] =
{
    /* Just display state info.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("state", fbe_raid_common_state_t, FBE_FALSE, "0x%x",
                                    fbe_debug_display_function_symbol),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_raid_common_state_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base config.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_raid_common_t,
                              fbe_raid_common_state_struct_info,
                              &fbe_raid_common_state_field_info[0]);
/*!**************************************************************
 * fbe_raid_library_debug_print_iots_summary()
 ****************************************************************
 * @brief
 *  Display a one line summary of the raid iots structure.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param iots_p - ptr to iots to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  3/25/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_library_debug_print_iots_summary(const fbe_u8_t * module_name,
                                                       fbe_dbgext_ptr iots_p,
                                                       fbe_trace_func_t trace_func,
                                                       fbe_trace_context_t trace_context,
                                                       fbe_u32_t overall_spaces_to_indent)
{
    fbe_u64_t siots_queue_head_ptr = 0;
    fbe_u32_t siots_queue_offset;
    fbe_status_t status;
    fbe_u32_t spaces_to_indent = overall_spaces_to_indent + 2;

    fbe_trace_indent(trace_func, trace_context, overall_spaces_to_indent);
    trace_func(trace_context, "IOTS: 0x%llx ", (unsigned long long)iots_p);

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, iots_p,
                                         &fbe_raid_common_state_struct_info,
                                         20 /* fields per line */,
                                         spaces_to_indent + 2);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, iots_p,
                                         &fbe_raid_iots_summary_struct_info,
                                         20 /* fields per line */,
                                         spaces_to_indent + 1);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, iots_p,
                                         &fbe_raid_common_summary_struct_info,
                                         20 /* fields per line */,
                                         spaces_to_indent + 2);
    
    /* Get the offset to the head of the siots queue and the first element on the siots 
     * queue. 
     */
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_iots_t, "siots_queue", &siots_queue_offset);
    siots_queue_head_ptr = iots_p + siots_queue_offset;
    trace_func(trace_context, "\n");
    /* Display all the individual siots.
     */
    fbe_raid_library_debug_print_siots_queue_summary(module_name, siots_queue_head_ptr, trace_func, trace_context, 
                                                     overall_spaces_to_indent + 2 /* spaces to indent */);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_library_debug_print_iots_summary()
 ******************************************/
/*************************
 * end file fbe_raid_debug.c
 *************************/
