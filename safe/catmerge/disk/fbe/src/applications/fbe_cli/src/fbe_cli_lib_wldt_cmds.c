/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
* @file fbe_cli_lib_wldt_cmds.c
 ***************************************************************************
 *
 * @brief
*  This file contains cli functions for the WLDT (WriteLogDebugTest) related features in FBE CLI.
 *
 * @ingroup fbe_cli
 *
 * @version
 *  07/06/2012 - Created from fbe_cli_lib_rdt_cmds.c. Dave Agans
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdlib.h> /*for atoh*/
#include <stdio.h>
#include "fbe_cli_private.h"
#include "fbe_trace.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_trace.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_utils.h"
#include "fbe_api_raid_group_interface.h"
#include "fbe_api_base_config_interface.h"
#include "fbe_api_block_transport_interface.h"
#include "fbe_api_lun_interface.h"
#include "fbe_cli_wldt.h"
#include "fbe/fbe_data_pattern.h"
#include "fbe_parity_write_log.h"
#include "fbe_raid_library.h"

#include "fbe_xor_api.h"
#include "fbe/fbe_random.h"

void fbe_cli_wldt_parse_cmds(int argc, char** argv);
void fbe_cli_wldt_display_on_disk_slot(fbe_u8_t * slot_buffer_p,
                                       fbe_u32_t current_disk_index,
                                       fbe_bool_t b_all_headers,
                                       fbe_bool_t b_raw_headers,
                                       fbe_u32_t raw_disk,
                                       fbe_u32_t raw_disk_block_num,
                                       fbe_u32_t raw_disk_block_count);
void fbe_cli_wldt_display_sector(fbe_u8_t * in_addr);
fbe_u32_t fbe_cli_wdlt_get_hex_arg(char **argv);

/*!*******************************************************************
 *  fbe_cli_wldt
 *********************************************************************
 * @brief Function to implement wldt commands execution.
 *    fbe_cli_wldt executes a SEP/PP Tester operation.  This command
 *               is for simulation/lab debug only.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @revision:
 *  07/01/2010 - Created. Swati Fursule
 *
 *********************************************************************/
void fbe_cli_wldt(int argc, char** argv)
{
    fbe_cli_printf("%s", "\n");
    fbe_cli_wldt_parse_cmds(argc, argv);

    return;
}
/******************************************
 * end fbe_cli_wldt()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_wldt_parse_cmds()
 ****************************************************************
 *
 * @brief   Parse wldt commands
 * wldt <-lun_num in hex | -object_id in hex> [-slot_id in hex]
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return  None.
 *
 * @revision:
 *  07/09/2012 - Rewritten from rdt for wldt.  Dave Agans
 *
 ****************************************************************/
void fbe_cli_wldt_parse_cmds(int argc, char** argv)
{
    fbe_object_id_t   lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t   rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_raid_group_number_t   rg_number = FBE_OBJECT_ID_INVALID;
    fbe_bool_t        b_all_slots = FBE_FALSE;
    fbe_bool_t        b_all_headers = FBE_FALSE;
    fbe_bool_t        b_raw_headers = FBE_FALSE;
    fbe_u32_t         selected_slot = FBE_PARITY_WRITE_LOG_INVALID_SLOT;
    fbe_u32_t         raw_disk = FBE_RAID_INVALID_DISK_POSITION;
    fbe_u32_t         raw_disk_block_num = 0;
    fbe_u32_t         raw_disk_block_count = 1;
    fbe_u32_t         block_count; /* this depends on the raid group element size, calc later */
     /* Raid always exports 520 (FBE_BE_BYTES_PER_BLOCK), with an optimal block size 
     * that is set to the raid group's optimal size, which gets set by the 
     * monitor's "get block size" condition. */
    fbe_u32_t         block_size = FBE_BE_BYTES_PER_BLOCK;
    fbe_u32_t         lunnum = INVALID_LUN_ID;
    fbe_lba_t         lba;
    fbe_status_t      status = FBE_STATUS_OK;
    fbe_payload_block_operation_t   block_operation;
    fbe_sg_element_t  *sg_ptr = NULL;
    fbe_block_size_t   optimal_block_size = 1;
    fbe_u32_t         current_slot_id = FBE_PARITY_WRITE_LOG_INVALID_SLOT;
    fbe_database_lun_info_t lun_info;
    fbe_sg_element_t  sg_elements[FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS+1] = {0}; 
    fbe_block_transport_block_operation_info_t  block_operation_info = {0};
    fbe_database_raid_group_info_t rg_info;
    fbe_object_id_t   disk_id;
    fbe_u32_t         current_disk_index;
    fbe_parity_get_write_log_info_t write_log_info;
    fbe_parity_get_write_log_slot_t *slot_p;
    fbe_u8_t display_string[32];
    fbe_u8_t* slot_buffer_p = NULL;

    if (argc == 0)
    {
        fbe_cli_printf("wldt: ERROR: Too few args.\n");
        fbe_cli_printf("%s", WLDT_CMD_USAGE);
        return;
    }
    if ((strcmp(*argv, "-help") == 0) ||
        (strcmp(*argv, "-h") == 0) )
    {
        /* If they are asking for help, just display help and exit.
        */
        fbe_cli_printf("%s", WLDT_CMD_USAGE);
        return;
    }
    /* Parse args
     */
    while (argc && **argv == '-')
    {
        if (   !strcmp(*argv, "-all_slots")
            || !strcmp(*argv, "-as"))
        {
            b_all_slots = FBE_TRUE;
        }
        else if (   !strcmp(*argv, "-all_headers")
                 || !strcmp(*argv, "-ah"))
        {                                
            b_all_headers = FBE_TRUE;
        }
        else if (   !strcmp(*argv, "-raw_headers")
                 || !strcmp(*argv, "-rh"))
        {                                
            b_raw_headers = FBE_TRUE;
        }
        else if (   !strcmp(*argv, "-slot")
                 || !strcmp(*argv, "-s"))
        {
            argc--;
            argv++;

            /* Next argument is hexadecimal slot id value.
             */
            selected_slot = fbe_cli_wdlt_get_hex_arg(argv);
        }
        else if (   !strcmp(*argv, "-raw_disk")
                 || !strcmp(*argv, "-rd"))
        {
            argc--;
            argv++;

            if (argc < 3)
            {
                fbe_cli_printf("wldt: ERROR: Need 3 args for -raw_disk/-rd.\n");
                fbe_cli_printf("%s", WLDT_CMD_USAGE);
                return;
            }
            /* Next argument is hexadecimal raw disk position value.
             */
            raw_disk = fbe_cli_wdlt_get_hex_arg(argv);
            if (raw_disk > FBE_RAID_MAX_DISK_ARRAY_WIDTH)
            {
                fbe_cli_printf("wldt: ERROR: Invalid raw disk position.\n");
                fbe_cli_printf("%s", WLDT_CMD_USAGE);
                return;
            }

            argc--;
            argv++;

            /* Next argument is hexadecimal block offset value.
             */
            raw_disk_block_num = fbe_cli_wdlt_get_hex_arg(argv);

            argc--;
            argv++;

            /* Next argument is hexadecimal block count value.
             */
            raw_disk_block_count = fbe_cli_wdlt_get_hex_arg(argv);
        }
        else if (   !strcmp(*argv, "-object_id")
                 || !strcmp(*argv, "-o"))
        {
            argc--;
            argv++;

            if (argc < 1)
            {
                fbe_cli_printf("wldt: ERROR: Need arg for -object_id/-o.\n");
                fbe_cli_printf("%s", WLDT_CMD_USAGE);
                return;
            }
            /* Next argument is hexadecimal raid group object id value.
             */
            rg_object_id = fbe_cli_wdlt_get_hex_arg(argv);
        }
        else if (   !strcmp(*argv, "-raid_group")
                 || !strcmp(*argv, "-rg"))
        {
            argc--;
            argv++;

            if (argc < 1)
            {
                fbe_cli_printf("wldt: ERROR: Need arg for -raid_group/-rg.\n");
                fbe_cli_printf("%s", WLDT_CMD_USAGE);
                return;
            }
            /* Next argument is hexadecimal raid group number value.
             */
            rg_number = fbe_cli_wdlt_get_hex_arg(argv);

            if ((rg_number < 0))
            {
                fbe_cli_printf("wldt: ERROR: Invalid raid_group number.\n");
                fbe_cli_printf("%s", WLDT_CMD_USAGE);
                return;
            }
        }
        else if (   !strcmp(*argv, "-lun_num")
                 || !strcmp(*argv, "-l"))
        {
            argc--;
            argv++;

            if (argc < 1)
            {
                fbe_cli_printf("wldt: ERROR: Need arg for -lun_num/-l.\n");
                fbe_cli_printf("%s", WLDT_CMD_USAGE);
                return;
            }
            /* Next argument is hexadecimal lun number value.
             */
            lunnum = fbe_cli_wdlt_get_hex_arg(argv);

            if ((lunnum < 0))
            {
                fbe_cli_printf("wldt: ERROR: Invalid lun number.\n");
                fbe_cli_printf("%s", WLDT_CMD_USAGE);
                return;
            }

            status = fbe_api_database_lookup_lun_by_number(lunnum, 
                                                           &lun_object_id);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("wldt: ERROR: Lun %d does not exist\n",
                               lunnum);
                return;
            }
            /* Now that we know that the object correspond to LUN 
             * get the details of LUN
             */
            lun_info.lun_object_id = lun_object_id;
            status = fbe_api_database_get_lun_info(&lun_info);

            /* return error from here since we failed to get the details 
             * of valid LUN object */
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("wldt: ERROR: Lun 0x%x details are not available\n ", lunnum);
                return;
            }
        }
        else
        {
            fbe_cli_printf("wldt: ERROR: Invalid switch.\n");
            fbe_cli_printf("%s", WLDT_CMD_USAGE);
            return;
        }
        argc--;
        argv++;
    }

    /* Get and check raid group ids -- argument precedence is lun, rg_num, obj_id
     */
    if (lunnum == INVALID_LUN_ID)
    {
        if (rg_number == FBE_OBJECT_ID_INVALID)
        {
            if (rg_object_id == FBE_OBJECT_ID_INVALID)
            {
                fbe_cli_printf("rdt: ERROR: Must specify lun number, raid_group number, or raid group object id.\n");
                fbe_cli_printf("%s", WLDT_CMD_USAGE);
                return;
            }
        }
        else
        {
            status = fbe_api_database_lookup_raid_group_by_number(rg_number, 
                                                                  &rg_object_id);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("wldt: ERROR: Raid group %d does not exist\n",
                               rg_number);
                return;
            }
        }
    }
    else
    {
        rg_object_id = lun_info.raid_group_obj_id;
    }

    status = fbe_api_database_lookup_raid_group_by_object_id(rg_object_id, &rg_number);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("wldt: ERROR: Raid group id: 0x%x raid group number is not available\n",
                       rg_object_id);
        return;
    }

    /* Now get the raid group info so we can walk the disk objects
     */
    status = fbe_api_database_get_raid_group(rg_object_id, &rg_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("wldt: ERROR: Raid group id: 0x%x info is not available\n",
                       rg_object_id);
        return;
    }

    /* Verify that this is a parity before barging on ahead
     */
    if (   rg_info.rg_info.raid_type != FBE_RAID_GROUP_TYPE_RAID3
        && rg_info.rg_info.raid_type != FBE_RAID_GROUP_TYPE_RAID5
        && rg_info.rg_info.raid_type != FBE_RAID_GROUP_TYPE_RAID6)
    {
        fbe_cli_printf("wldt: ERROR: Raid group id: 0x%x is not a parity, and thus has no write log\n",
                       rg_object_id);
        return;
    }

    /* Get the slot info from the raid group
     */
    status = fbe_api_raid_group_get_write_log_info(rg_object_id,
                                                   &write_log_info, 
                                                   FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("rginfo: ERROR: Not able to get the write log information for RG object id 0x%x\n", 
                       rg_object_id);
        return;
    }

    /* check the params now that we have the required slot size and count
     */
    if (selected_slot >= write_log_info.slot_count)
    {
        fbe_cli_printf("wldt: ERROR: Invalid slot number 0x%x >= RG slot count 0x%x.\n",
                       selected_slot,
                       write_log_info.slot_count);
        fbe_cli_printf("%s", WLDT_CMD_USAGE);
        return;
    }

    if (raw_disk_block_num >= write_log_info.slot_size)
    {
        fbe_cli_printf("wldt: ERROR: Invalid raw disk block number 0x%x >= RG slot size 0x%x.\n",
                       raw_disk_block_num,
                       write_log_info.slot_size);
        fbe_cli_printf("%s", WLDT_CMD_USAGE);
        return;
    }

    if (   raw_disk_block_count == 0
        || raw_disk_block_num + raw_disk_block_count > write_log_info.slot_size)
    {
        fbe_cli_printf("wldt: ERROR: Invalid raw disk block number 0x%x and count 0x%x not within RG slot range 0x%x.\n",
                       raw_disk_block_num,
                       raw_disk_block_count,
                       write_log_info.slot_size);
        fbe_cli_printf("%s", WLDT_CMD_USAGE);
        return;
    }

    /* Now we can set up the block count to the slot size, which depends on the raid group element size
     */
    block_count = write_log_info.slot_size;

    /* Allocate a buffer.
     */
    slot_buffer_p = (fbe_u8_t *)fbe_api_allocate_memory(block_count * block_size);

    if (slot_buffer_p == NULL)
    {
        fbe_cli_printf("wldt: ERROR: Could not allocate a buffer\n");
        return;
    }

    /* Set up the sg_ptr to use the SG list
     */
    sg_ptr = sg_elements;

    status = fbe_data_pattern_sg_fill_with_memory(sg_ptr,
                                slot_buffer_p,
                                block_count,
                                block_size,
                                FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS,
                                FBE_DATA_PATTERN_MAX_BLOCKS_PER_SG);
    if (status != FBE_STATUS_OK )
    {
        fbe_cli_printf("wldt: ERROR: Setting up sg failed.\n");
        return;
    }

    /* Display the lun/rg/obect info
     */
    if (lunnum != INVALID_LUN_ID)
    {
        fbe_cli_printf("LUN 0x%x on ", 
                       lunnum);
    }

    /* Display rg-global write log info
     */
    fbe_cli_printf("RAID GROUP 0x%x OBJECT_ID 0x%x parity write log:  start_lba:0x%llx  slot_size:0x%x  slot_count:0x%x  quiesced:%d\n", 
                   rg_number,
                   rg_object_id,
                   write_log_info.start_lba,
                   write_log_info.slot_size,
                   write_log_info.slot_count,
                   write_log_info.quiesced);

    /* Walk each slot
     */
    for (current_slot_id = 0; current_slot_id < write_log_info.slot_count; current_slot_id++)
    {
        slot_p = &write_log_info.slot_array[current_slot_id];
        if (   b_all_slots
            || selected_slot == current_slot_id
            || (selected_slot == FBE_PARITY_WRITE_LOG_INVALID_SLOT
                && (   slot_p->state == FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED
                    || slot_p->state == FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED_FOR_FLUSH
                    || slot_p->state == FBE_PARITY_WRITE_LOG_SLOT_STATE_FLUSHING)))
        {
            switch (slot_p->state)
            {
                case FBE_PARITY_WRITE_LOG_SLOT_STATE_FREE:
                    strcpy(display_string, "FREE");
                    break;

                case FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED:
                    strcpy(display_string, "ALLOCATED");
                    break;

                case FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED_FOR_FLUSH:
                    strcpy(display_string, "ALLOCATED_FOR_FLUSH");
                    break;

                case FBE_PARITY_WRITE_LOG_SLOT_STATE_FLUSHING:
                    strcpy(display_string, "FLUSHING");
                    break;

                case FBE_PARITY_WRITE_LOG_SLOT_STATE_INVALID:
                default:
                    strcpy(display_string, "INVALID");
                    break;
            }
            fbe_cli_printf("  SlotID:0x%x  state:%s  ", current_slot_id, display_string); 

            switch (slot_p->invalidate_state)
            {
                case FBE_PARITY_WRITE_LOG_SLOT_INVALIDATE_STATE_SUCCESS:
                    strcpy(display_string, "SUCCESS");
                    break;

                case FBE_PARITY_WRITE_LOG_SLOT_INVALIDATE_STATE_DEAD:
                    strcpy(display_string, "DEAD");
                    break;

                default:
                    strcpy(display_string, "INVALID");
                    break;
            }
            fbe_cli_printf("invalidate_state:%s  ", display_string); 

            /* Get the slot lba based on the slot number
             */
            lba =   rg_info.rg_info.physical_offset
                  + rg_info.rg_info.write_log_start_pba
                  + (current_slot_id * write_log_info.slot_size);

            fbe_cli_printf("logical disk offset:0x%llx\n", lba); 

            /* Walk each disk
             */
            for (current_disk_index = 0; current_disk_index < rg_info.pvd_count; current_disk_index++)
            {
                /* Get the object ID and do the read
                 */
                disk_id = rg_info.pvd_list[current_disk_index];

                fbe_cli_printf("    Disk index:0x%x  pvd_ID:0x%x  ", current_disk_index, disk_id);

                if (disk_id != FBE_OBJECT_ID_INVALID)
                {
                    /* Build the block operation and attach it to the block_operation_info
                     */
                    status = fbe_payload_block_build_operation(&block_operation,
                                                               FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                                               lba,
                                                               block_count,
                                                               block_size,
                                                               optimal_block_size,
                                                               NULL);
                    if (status != FBE_STATUS_OK)
                    {
                        fbe_cli_printf("\nwldt: Error: Block operation build failed for raid group 0x%x\n", 
                                       rg_object_id);
                        return;
                    }
                    block_operation_info.block_operation = block_operation;

                    /* This is wrong API! we should send packet and not bloc operation */
                    status = fbe_api_block_transport_send_block_operation(
                                                     disk_id,
                                                     FBE_PACKAGE_ID_SEP_0,
                                                     &block_operation_info,
                                                     (fbe_sg_element_t*)sg_ptr);
                    if (status != FBE_STATUS_OK)
                    {
                        fbe_cli_printf("\nwldt: Error: Block operation failed to object_id: %d pkt_status: 0x%x\n", 
                                       disk_id, 
                                       status);
                        return;
                    }

                    block_operation = block_operation_info.block_operation;

                    switch (block_operation.status)
                    {
                        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS:
                            fbe_cli_printf("OK\n");

                            /* Now display the result, and compare csum_of_csums for this disk
                             */
                            fbe_cli_wldt_display_on_disk_slot(slot_buffer_p,
                                                              current_disk_index,
                                                              b_all_headers,
                                                              b_raw_headers,
                                                              raw_disk,
                                                              raw_disk_block_num,
                                                              raw_disk_block_count);
                            break;

                        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR:
                            fbe_cli_printf("Media error\n");
                            break;

                        default:
                            fbe_cli_printf("Error: status:0x%x  qualifier:0x%x\n",
                                           block_operation.status,
                                           block_operation.status_qualifier);
                            break;
                    }
                }
            }
        }
    }

    fbe_cli_printf("\n");

    fbe_api_free_memory(slot_buffer_p);

    return;
}
/******************************************
 * end fbe_cli_wldt_parse_cmds()
 ******************************************/

/*!*******************************************************************
 *  fbe_cli_wdlt_get_hex_arg
 *********************************************************************
 * @brief Function to get a hex argument from the command line.
 *
 * @param    argv - argument string
 *
 * @return   value represented by the string
 *
 * @revision:
 *  07/12/2012 - Created. Dave Agans
 *
 *********************************************************************/
fbe_u32_t fbe_cli_wdlt_get_hex_arg(char **argv)
{
    fbe_char_t *tmp_ptr;

    /* Argument is hexadecimal value.
    */
    tmp_ptr = *argv;
    if ((*tmp_ptr == '0') && 
        (*(tmp_ptr + 1) == 'x' || *(tmp_ptr + 1) == 'X'))
    {
        tmp_ptr += 2;
    }
   return fbe_atoh(tmp_ptr);
}
/******************************************
 * end fbe_cli_wdlt_get_hex_arg()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_wldt_display_on_disk_slot()
 ****************************************************************
 *
 * @brief   This function is used to display the result of wldt
 *          operation.
 *
 * @param   slot_buffer_p - pointer to buffer containing slot header and data read from disk
 *          b_all_headers - if FBE_TRUE, print valid and invalid headers; if false, print valid headers
 *
 * @return  None.
 *
 * @revision:
 *  07/09/2012 - Created.  Dave Agans
 *
 ****************************************************************/
void fbe_cli_wldt_display_on_disk_slot(fbe_u8_t *slot_buffer_p,
                                       fbe_u32_t current_disk_index,
                                       fbe_bool_t b_all_headers,
                                       fbe_bool_t b_raw_headers,
                                       fbe_u32_t raw_disk,
                                       fbe_u32_t raw_disk_block_num,
                                       fbe_u32_t raw_disk_block_count)
{
    fbe_parity_write_log_header_t *header_p = (fbe_parity_write_log_header_t *)slot_buffer_p;
    fbe_parity_write_log_header_disk_info_t *disk_info_p;
    fbe_u16_t disk_info_index;
    fbe_u16_t block_index;
    fbe_sector_t *blkptr;
    fbe_u16_t csum_of_csums;

    /* Display the header if valid or -ah flag
     */
    /*!@todo When there are more than one version, need to accept any version here 
     */
    if (   b_all_headers
        || (   header_p->header_state == FBE_PARITY_WRITE_LOG_HEADER_STATE_VALID
            && header_p->header_version == FBE_PARITY_WRITE_LOG_CURRENT_HEADER_VERSION))
    {
        fbe_cli_printf("        Header: state:%s  version:0x%llx  timestamp sec:0x%llx  usec:0x%llx\n",
                       header_p->header_state == FBE_PARITY_WRITE_LOG_HEADER_STATE_VALID
                        ? "VALID" : "INVALID",
                       header_p->header_version,
                       header_p->header_timestamp.sec,
                       header_p->header_timestamp.usec);
        fbe_cli_printf("                write_bitmap:0x%x  start_lba:0x%x  xfer_cnt:0x%x  parity_start:0x%x  parity_cnt:0x%x\n",
                       header_p->write_bitmap,
                       (unsigned int)header_p->start_lba,
                       header_p->xfer_cnt,
                       (unsigned int)header_p->parity_start,
                       header_p->parity_cnt);

        /* Walk the disk info
         */
        for (disk_info_index = 0; disk_info_index < FBE_PARITY_MAX_WIDTH; disk_info_index++)
        {
            disk_info_p = &header_p->disk_info[disk_info_index];
            if (   disk_info_p->offset != 0
                || disk_info_p->block_cnt != 0
                || disk_info_p->csum_of_csums != 0)
            {
                fbe_cli_printf("                  disk_info: pos:0x%x  offset:0x%x  block_cnt:0x%x  csum_of_csums:0x%x",
                               disk_info_index,
                               disk_info_p->offset,
                               disk_info_p->block_cnt,
                               disk_info_p->csum_of_csums);
                
                /* Check the checksum of checksums if this is the current disk and display result
                 */
                if (disk_info_index == current_disk_index)
                {
                    /* Initialize the checksum field
                     */
                    csum_of_csums = 0;

                    for (block_index = 0; block_index < disk_info_p->block_cnt; block_index++) 
                    {
                        blkptr = (fbe_sector_t *)(slot_buffer_p) + block_index + 1; /* +1 to skip over the header */

                        /* Running checksum of sector checksums. */
                        csum_of_csums ^= blkptr->crc; 

                    } /* for each block */

                    /* Cook Csum
                     */
                    csum_of_csums ^= FBE_PARITY_WRITE_LOG_HEADER_CSUM_SEED;

                    if (csum_of_csums == disk_info_p->csum_of_csums)
                    {
                        fbe_cli_printf(" matches data blocks:0x%x\n",
                                       csum_of_csums);
                    }
                    else
                    {
                        fbe_cli_printf(" MISMATCHES data blocks:0x%x\n",
                                       csum_of_csums);
                    }

                }
                else
                {
                    fbe_cli_printf("\n");
                }
            }
        }
    }
    if (b_raw_headers
        || raw_disk == current_disk_index)
    {
        fbe_cli_printf("                raw disk header\n");
        fbe_cli_wldt_display_sector(slot_buffer_p);
    }
    if (raw_disk == current_disk_index)
    {
        for (block_index = raw_disk_block_num; block_index < raw_disk_block_num + raw_disk_block_count; block_index++)
        {
            fbe_cli_printf("                data block:0x%x\n", block_index);
            fbe_cli_wldt_display_sector(slot_buffer_p + ((block_index + 1) * FBE_BE_BYTES_PER_BLOCK)); /* +1 to skip header */
        }
    }
}

/************************************************************************
 *  fbe_cli_wldt_display_sector()
 ************************************************************************
 *
 * @brief
 *   This function dispays ONE SECTOR of data.  
 *
 * @param in_addr - buffer address
 *
 * @return (none)
 *
 * @revision
 *  07/11/12 - Created. Dave Agans
 ************************************************************************/

void fbe_cli_wldt_display_sector(fbe_u8_t * in_addr)
{
    fbe_u32_t    *word_ptr = (fbe_u32_t *)in_addr;
    fbe_u32_t    offset = 0;
    fbe_u32_t    word_count = FBE_BE_BYTES_PER_BLOCK / 4;

    /* This loop displays most of the sector (512 bytes or 128 words).
     */

    fbe_cli_printf("                  offset  data\n");
    fbe_cli_printf("                  ------  -----------------------------------------------------------------------\n");

    fbe_cli_printf("                  %03X     ", offset);
    while (word_count > 2)
    {
        fbe_cli_printf("%08X ",
                       *word_ptr);
        word_ptr++;
        word_count--;
        offset += 4;

        if ((offset % 32) == 0)
        {
            fbe_cli_printf("\n                  %03X     ",
                           offset);
        }
    }
    if ((offset % 32) != 0)
    {
        fbe_cli_printf("\n                  %03X     ",
                       offset);
    }

    /* Print the last 2 words of the sector (the overhead bytes).
     */
    fbe_cli_printf("%08X %08X\n",
                   *(word_ptr),
                   *(word_ptr+1));

    return;
}                               
/* end of fbe_cli_wldt_display_sector() */

/*************************
 * end file fbe_cli_lib_wldt_cmds.c
 *************************/
