#ifndef __FBE_CLI_DDT_H__
#define __FBE_CLI_DDT_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!*************************************************************************
 * @file fbe_cli_ddt.h
 ***************************************************************************
 *
 * @brief
 *  This file contains DDT command cli definitions.
 *
 * @version
 *  
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_cli.h"
#include "fbe/fbe_lun.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_job_service_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
    
typedef enum 
{
    FBE_CLI_DDT_CMD_TYPE_INVALID = 0,
    FBE_CLI_DDT_CMD_TYPE_DISPLAY_INFO,
    FBE_CLI_DDT_CMD_TYPE_EDIT,
    FBE_CLI_DDT_CMD_TYPE_WRITE,
    FBE_CLI_DDT_CMD_TYPE_READ,
    FBE_CLI_DDT_CMD_TYPE_CHECKSUM,
    FBE_CLI_DDT_CMD_TYPE_GETBUFFER,
    FBE_CLI_DDT_CMD_TYPE_VERIFYBLOCK,
    FBE_CLI_DDT_CMD_TYPE_WRITE_VERIFY,
    FBE_CLI_DDT_CMD_TYPE_CHECKSUM_BUFFER,
    FBE_CLI_DDT_CMD_TYPE_INVALIDATE_CHECKSUM,
    FBE_CLI_DDT_CMD_TYPE_KILL_FRU,
    FBE_CLI_DDT_CMD_TYPE_SET_HIBERNATE_STATE,
    FBE_CLI_DDT_CMD_TYPE_SEEK,
    FBE_CLI_DDT_CMD_TYPE_TUR,
    FBE_CLI_DDT_CMD_TYPE_SET_FRU_WRITE_CACHE_STATE, /* Not Implemented. */
    FBE_CLI_DDT_CMD_TYPE_MAX
}
fbe_cli_ddt_cmd_type;

typedef enum 
{
    FBE_CLI_DDT_SWITCH_TYPE_INVALID = 0,
    FBE_CLI_DDT_SWITCH_TYPE_SB,
    FBE_CLI_DDT_SWITCH_TYPE_WS,
    FBE_CLI_DDT_SWITCH_TYPE_ND,
    FBE_CLI_DDT_SWITCH_TYPE_SF,
    FBE_CLI_DDT_SWITCH_TYPE_FUA,
    FBE_CLI_DDT_SWITCH_TYPE_MAX
}
fbe_cli_ddt_switch_type;

typedef enum ddt_w_count_enum
{
    FBE_CLI_DDT_W_COUNT_NONE = 0,
    FBE_CLI_DDT_W_COUNT_ONE, 
    FBE_CLI_DDT_W_COUNT_TWO, 
    FBE_CLI_DDT_W_COUNT_THREE
}
fbe_cli_ddt_w_count_enum_t;

typedef enum 
{
    FBE_CLI_DDT_TYPE_INVALID = 0,
    FBE_CLI_DDT_TYPE_LDO,
    FBE_CLI_DDT_TYPE_PDO,
    FBE_CLI_DDT_TYPE_MAX
}
fbe_cli_ddt_type;


void fbe_cli_ddt(int argc , char ** argv);
void fbe_cli_ddt_get_cmds(int argc,char **argv,fbe_cli_ddt_switch_type switch_type);
void fbe_cli_ddt_process_cmds(fbe_lba_t lba,
                              fbe_cli_ddt_switch_type switch_type,
                              fbe_cli_ddt_cmd_type cmd_type,
                              fbe_job_service_bes_t phys_location,
                              fbe_u64_t count,
                              fbe_u32_t data,
                              fbe_u32_t offset);
void fbe_cli_ddt_cmd_read(fbe_lba_t lba,
                                        fbe_cli_ddt_switch_type switch_type,
                                        fbe_job_service_bes_t phys_location,
                                        fbe_u64_t count);
void fbe_cli_ddt_cmd_write(fbe_lba_t lba,
                                        fbe_cli_ddt_switch_type switch_type,
                                        fbe_job_service_bes_t phys_location,
                                        fbe_u64_t count);
void fbe_cli_ddt_cmd_display(fbe_u64_t count);
void fbe_cli_ddt_cmd_getbuffer(fbe_u64_t count);
void fbe_cli_ddt_cmd_edit(fbe_u32_t data,fbe_u32_t offset,fbe_u64_t count);
void fbe_cli_ddt_print_buffer_contents(fbe_sg_element_t *sg_ptr, 
                                 fbe_lba_t disk_address,
                                 fbe_u64_t blocks,
                                 fbe_u32_t block_size);
void fbe_cli_ddt_display_result(fbe_payload_block_operation_t *block_operation_p,
                                fbe_sg_element_t *sg_p,fbe_lba_t lba,
                                fbe_cli_ddt_switch_type switch_type);
void fbe_cli_ddt_display_sector(fbe_lba_t in_lba, 
                                fbe_u8_t * in_addr, 
                                fbe_u64_t w_count);
void fbe_cli_ddt_block_operation(fbe_payload_block_operation_opcode_t ddt_block_op,
                                        fbe_u64_t block_count,
                                        fbe_lba_t lba,
                                        fbe_job_service_bes_t phys_location,
                                        fbe_cli_ddt_switch_type switch_type);
static fbe_u8_t *fbe_cli_ddt_get_ptr( fbe_u32_t offset,fbe_sg_element_t *sg_ptr);
void fbe_cli_ddt_cmd_verify( fbe_lba_t lba,
                           fbe_cli_ddt_switch_type switch_type,
                           fbe_job_service_bes_t phys_location,
                           fbe_u64_t count);
void fbe_cli_ddt_cmd_checksum_buffer( fbe_lba_t lba,
                           fbe_cli_ddt_switch_type switch_type,
                           fbe_job_service_bes_t phys_location,
                           fbe_u64_t count);
void fbe_cli_ddt_cmd_invalidate_checksum( fbe_lba_t lba,
                           fbe_cli_ddt_switch_type switch_type,
                           fbe_job_service_bes_t phys_location,
                           fbe_u64_t count);

void fbe_cli_ddt_cmd_kill_drive( fbe_lba_t lba,
                           fbe_cli_ddt_switch_type switch_type,
                           fbe_job_service_bes_t phys_location);

void fbe_cli_ddt_cmd_set_hibernate_state( fbe_lba_t lba,
                           fbe_cli_ddt_switch_type switch_type,
                           fbe_job_service_bes_t phys_location);



void fbe_cli_ddt_get_edit_cmd(int argc,char **argv,
                               fbe_lba_t lba,
                               fbe_cli_ddt_switch_type switch_type,
                               fbe_cli_ddt_cmd_type cmd_type,
                               fbe_job_service_bes_t phys_location,
                               fbe_u64_t count);
void fbe_cli_ddt_get_checksum_cmd(int argc,char **argv,
                               fbe_lba_t lba,
                               fbe_cli_ddt_switch_type switch_type,
                               fbe_cli_ddt_cmd_type cmd_type,
                               fbe_job_service_bes_t phys_location,
                               fbe_u64_t count);
void fbe_cli_ddt_get_invalidate_checksum_cmd(int argc,char **argv,
                               fbe_lba_t lba,
                               fbe_cli_ddt_switch_type switch_type,
                               fbe_cli_ddt_cmd_type cmd_type,
                               fbe_job_service_bes_t phys_location,
                               fbe_u64_t count);


void fbe_cli_ddt_get_cmd_attribute(int argc,char **argv,
                               fbe_cli_ddt_switch_type switch_type,
                               fbe_cli_ddt_cmd_type cmd_type);


#define DDT_CMD_USAGE "\
Usage: ddt -h\
       ddt [switches] <operation> <frunum> <lba (64bit)> [count]\n\
Switches:\n\
     -sb   - Save Buffer (use with 'r' or 'w' operations)\n\
     -ws   - Use 'Write Same' (use with 'w' operations)\n\
     -nd   - Do not display data (use with 'r' operations)\n\
     -sf   - Simulate failure (use with 'c' operations)\n\
     -fua  - Force Unit Access (use with 'r' operations)\n\
     -ldo  - use ldo (use with 'r' or 'w' operations)\n\
     -pdo  - use pdo (use with 'r' or 'w' operations)\n\
Operations:\n\
     d  - Display buffer\n\
     x  - Checksum buffer\n\
     e  - Edit buffer ('ddt e offset byte <byte...>')\n\
     r  - Read\n\
     w  - Write\n\
     b  - Write and verify\n\
     m  - Read and remap\n\
     v  - Verify blocks\n\
     s  - Seek\n\
     g  - Get buffer\n\
     f  - Free buffer\n\
     k  - Kill FRU (use this command format: 'ddt k <frunum> dead' )\n\
     c  - Modify FRU write cache (use format: 'ddt c <frunum> <0/1>')\n\
     u  - FRU standby state off(0)/on(1)(use format: 'ddt u <frunum> <0/1>')\n\
     q  - Send Test Unit Ready\n\
     i [pattern] - Invalidate the checksum for all sectors in buffer\n\
                   Supported patterns:\n\
                    klondike - Used by klondike enclosure\n\
                    dh       - Used by DH for invalidate after remap\n\
                    raid     - Used by RG driver for uncorrs found during verify\n\
                    corrupt  - Used by RG driver for intentionally corrupted blocks\n\
                    If no pattern is specified, only the checksum will be corrupted.\n\
" 

#define FBE_CLI_DDT_DEFAULT_COUNT 1
#define FBE_CLI_DDT_OPTIMUM_BLOCK_SIZE 1
#define FBE_CLI_DDT_MAX_STRING_SIZE 128
#define FBE_CLI_DDT_COLUMN_SIZE 4
#define FBE_CLI_DDT_DEFAULT_LBA 0
#define FBE_CLI_DDT_DATA_INVALID 100
#define FBE_CLI_DDT_DATA_BOGUS -1
#define FBE_CLI_DDT_OFFSET_INVALID 201

/* This is a dummy value used to adjust the display */
#define FBE_CLI_DDT_DUMMYVAL 3

/* This is value which used during declaration */
#define FBE_CLI_DDT_ZERO 0

/*************************
 * end file fbe_cli_ddt.h
 *************************/

#endif /* end __FBE_CLI_DDT_H__ */
