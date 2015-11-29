#ifndef __FBE_CLI_WISD_H__
#define __FBE_CLI_WISD_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!*************************************************************************
 * @file fbe_cli_wisd.h
 ***************************************************************************
 *
 * @brief
 *  This file contains WISD(Where Is System Drives) command cli definitions.
 *
 * @version
 *  
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_cli.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_job_service_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
    
typedef enum 
{
    FBE_CLI_WISD_CMD_TYPE_INVALID = 0,
    FBE_CLI_WISD_CMD_TYPE_DISPLAY_INFO,
    FBE_CLI_WISD_CMD_TYPE_EDIT,
    FBE_CLI_WISD_CMD_TYPE_WRITE,
    FBE_CLI_WISD_CMD_TYPE_READ,
    FBE_CLI_WISD_CMD_TYPE_CHECKSUM,
    FBE_CLI_WISD_CMD_TYPE_GETBUFFER,
    FBE_CLI_WISD_CMD_TYPE_VERIFYBLOCK,
    FBE_CLI_WISD_CMD_TYPE_WRITE_VERIFY,
    FBE_CLI_WISD_CMD_TYPE_CHECKSUM_BUFFER,
    FBE_CLI_WISD_CMD_TYPE_INVALIDATE_CHECKSUM,
    FBE_CLI_WISD_CMD_TYPE_KILL_FRU,
    FBE_CLI_WISD_CMD_TYPE_SET_HIBERNATE_STATE,
    FBE_CLI_WISD_CMD_TYPE_SEEK,
    FBE_CLI_WISD_CMD_TYPE_TUR,
    FBE_CLI_WISD_CMD_TYPE_SET_FRU_WRITE_CACHE_STATE, /* Not Implemented. */
    FBE_CLI_WISD_CMD_TYPE_MAX
}
fbe_cli_wisd_cmd_type;

typedef enum 
{
    FBE_CLI_WISD_SWITCH_TYPE_INVALID = 0,
    FBE_CLI_WISD_SWITCH_TYPE_SB,
    FBE_CLI_WISD_SWITCH_TYPE_WS,
    FBE_CLI_WISD_SWITCH_TYPE_ND,
    FBE_CLI_WISD_SWITCH_TYPE_SF,
    FBE_CLI_WISD_SWITCH_TYPE_FUA,
    FBE_CLI_WISD_SWITCH_TYPE_MAX
}
fbe_cli_wisd_switch_type;

typedef enum wisd_w_count_enum
{
    FBE_CLI_WISD_W_COUNT_NONE = 0,
    FBE_CLI_WISD_W_COUNT_ONE, 
    FBE_CLI_WISD_W_COUNT_TWO, 
    FBE_CLI_WISD_W_COUNT_THREE
}
fbe_cli_wisd_w_count_enum_t;

typedef enum 
{
    FBE_CLI_WISD_TYPE_INVALID = 0,
    FBE_CLI_WISD_TYPE_LDO,
    FBE_CLI_WISD_TYPE_PDO,
    FBE_CLI_WISD_TYPE_MAX
}
fbe_cli_wisd_type;


void fbe_cli_wisd(int argc , char ** argv);
void fbe_cli_wisd_get_cmds(int argc,char **argv,fbe_cli_wisd_switch_type switch_type);
fbe_status_t fbe_cli_wisd_process_cmds(void);
fbe_status_t fbe_cli_wisd_get_wwn_seed_from_prom_resume(fbe_u32_t * wwn_seed);
fbe_status_t fbe_cli_wisd_block_operation(fbe_payload_block_operation_opcode_t wisd_block_op,
                                        void* one_block_buffer,
                                        fbe_u64_t block_count,
                                        fbe_lba_t lba,
                                        fbe_job_service_bes_t phys_location);
void fbe_cli_wisd_output_info_to_user(void);

#define WISD_CMD_USAGE "\
Usage: wisd -h\
       wisd <operation>\n\
Operations:\n\
     f  - Find system drives in array, eg: wisd f  \n" 

/*************************
 * end file fbe_cli_wisd.h
 *************************/

#endif /* end __FBE_CLI_WISD_H__ */
