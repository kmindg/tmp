/***************************************************************************
 * Copyright (C) EMC Corporation 2009 - 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *
 *  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.
 *  ACCESS TO THIS SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT.
 *  THIS CODE IS TO BE KEPT STRICTLY CONFIDENTIAL.
 ***************************************************************************/

#ifndef HORNET_API_H
#define HORNET_API_H

#include "csx_ext.h"
#include "HornetInterface.h"

#ifdef __cplusplus 
extern "C" 
{
#endif

#ifndef SIMMODE_ENV 
#define HORNET_FIRMWARE_PATH "/EMC/Firmware/eHornet/"
#else
#define HORNET_FIRMWARE_PATH "/home/c4dev/eHornet/"
#define EHORNET_FW_SIM_SOURCE_FILE "http://10.6.96.63/dist/Firmware/eHornet/eHornet-1.2.3-release.fw"
#define EHORNET_FW_SIM_FILE "eHornetSim.fw"
#endif  //SIMMODE_ENV

csx_status_e hornet_api_get_mirror_info(mirror_info *info);
csx_status_e hornet_api_get_storage_info(mirror_info *info);
csx_status_e hornet_api_get_fw_version(csx_string_t version, csx_size_t size);
csx_status_e hornet_api_get_hornet_system_state(HORNET_SYSTEM_STATE *state);
csx_status_e hornet_api_write_file(csx_cstring_t path);
csx_status_e hornet_api_read_file(csx_cstring_t path, csx_nuint_t bytes_count);
csx_status_e hornet_api_erase_hornet(void);
csx_status_e hornet_api_self_test(csx_uoffset_t dst_ofs, csx_size_t size, csx_uchar_t cmd, csx_nuint_t count, csx_nuint_t thread_num);
csx_status_e hornet_api_reset_device(csx_void_t);
csx_status_e hornet_api_upgrade_firmware(csx_cstring_t firmware_path);
csx_status_e hornet_api_set_fw_mode(hornet_mode_e mode);
csx_status_e hornet_api_reclaim(csx_uoffset_t ofs, csx_size_t size);
csx_status_e hornet_api_set_cache_size(csx_size_t size);
csx_status_e hornet_api_set_trace_level(hornet_trace_level_t lvl);
csx_status_e hornet_api_insert_override(csx_bool_t val);
csx_status_e hornet_api_dump_inout_buffer(csx_uchar_t buf_id);

/* Implemented in hornet_lib for NVCM */
csx_status_e hornet_api_read_mirror(csx_phys_address_t dst_pa, csx_uoffset_t src_ofs, csx_size_t size);
csx_status_e hornet_api_write_mirror_list(csx_nuint_t count, CSX_CONST hornet_sg_element_t *list);
csx_status_e hornet_api_write_mirror(csx_uoffset_t dst_ofs, csx_phys_address_t src_pa, csx_size_t size);

#ifdef SIMMODE_ENV
/* Test api only*/
csx_status_e hornet_api_sim_insert(csx_void_t);
csx_status_e hornet_api_sim_remove(csx_void_t);
csx_status_e hornet_api_wait_manager_up(csx_timeout_secs_t sec);
csx_status_e hornet_api_wait_manager_down(csx_timeout_msecs_t msec);
csx_status_e peer_api_loose_heartbeat(csx_void_t);
csx_status_e peer_api_reset(csx_void_t);
csx_status_e peer_api_reply(csx_bool_t val);
csx_status_e peer_api_set_logging_lvl(hornet_trace_level_t lvl);
csx_status_e peer_api_emulate_failure(peer_failure_t fail);
#endif // SIMMODE_ENV

#ifdef __cplusplus 
} 
#endif

#endif // HORNET_API_H
