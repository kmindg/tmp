/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
* @file fbe_cli_lib_rdt_cmds.c
 ***************************************************************************
 *
 * @brief
*  This file contains cli functions for the RDT related features in FBE CLI.
 *
 * @ingroup fbe_cli
 *
 * @version
 *  07/01/2010 - Created. Swati Fursule
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
#include "fbe_cli_rdt.h"
#include "fbe/fbe_data_pattern.h"

#include "fbe_xor_api.h"
#include "fbe/fbe_random.h"

fbe_u32_t     fcli_rdt_count = 0;
void*         buffer = NULL;
fbe_u64_t     corrupt_blocks_bitmap= 0; /*! Bitmap for corrupted blocks pattern
                                        * when doing corrrupt crc or corrupt
                                        * data..
                                        */

void fbe_cli_rdt_parse_cmds(int argc, char** argv);
void fbe_cli_rdt_display_result(fbe_block_transport_block_operation_info_t *block_op_info_p,
                                fbe_sg_element_t *sg_p,
                                fbe_bool_t b_check_pattern,
                                fbe_bool_t b_check_zeros,
                                fbe_bool_t b_display);
void fbe_cli_rdt_display_sector(fbe_lba_t in_lba, 
                                fbe_u8_t * in_addr, 
                                fbe_u32_t w_count);
void fbe_cli_rdt_display_verify_counter(fbe_raid_verify_error_counts_t 
                                *verify_err_count_p);

void fbe_cli_rdt_print_buffer_contents(fbe_sg_element_t *sg_ptr, 
                                 fbe_lba_t disk_address,
                                 fbe_u32_t blocks,
                                 fbe_u32_t block_size);

/*!*******************************************************************
 *  fbe_cli_rdt
 *********************************************************************
 * @brief Function to implement rdt commands execution.
 *    fbe_cli_rdt executes a SEP/PP Tester operation.  This command
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
void fbe_cli_rdt(int argc, char** argv)
{
    fbe_cli_printf("%s", "\n");
    fbe_cli_rdt_parse_cmds(argc, argv);

    return;
}
/******************************************
 * end fbe_cli_rdt()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_rdt_parse_cmds()
 ****************************************************************
 *
 * @brief   Parse rdt commands
 * rdt <-d,-sp, -cp, -cz > <e|r|w|ws|c|d|v|f|z|wv|vw|vb|rb|wnc> 
 * <lun number in hex> <lba in hex, 0 to capacity> [count 0x0 .. 0x1000]
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return  None.
 *
 * @revision:
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rdt_parse_cmds(int argc, char** argv)
{
    fbe_bool_t        b_set_pattern = FBE_FALSE;
    fbe_bool_t        b_check_pattern = FBE_FALSE;
    fbe_bool_t        b_check_zeros = FBE_FALSE;
    fbe_bool_t        b_display = FBE_FALSE;
    fbe_char_t        rdt_op;
    fbe_u8_t *        rdt_op_str;
    fbe_u16_t         rdt_op_len;
    fbe_u32_t         block_count;
    fbe_bool_t        b_no_crc = FBE_FALSE;
    fbe_bool_t        b_bad_crc = FBE_FALSE;
     /* Raid always exports 520 (FBE_BE_BYTES_PER_BLOCK), with an optimal block size 
     * that is set to the raid group's optimal size, which gets set by the 
     * monitor's "get block size" condition. */
    fbe_u32_t         block_size = FBE_BE_BYTES_PER_BLOCK;
    fbe_u32_t         lunnum;
    fbe_lba_t         lba;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_char_t        *tmp_ptr;
    fbe_u32_t         count = 1;          /* default = 1 sector*/
    fbe_payload_block_operation_t   block_operation = {0};
    fbe_sg_element_t  *sg_ptr = NULL;
    fbe_block_size_t   optimal_block_size;
    fbe_payload_block_operation_opcode_t rdt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID; /* SAFEBUG - was never initialized - which is bad in invalid commands case */
    fbe_object_id_t   lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t   object_id_for_block_op = FBE_OBJECT_ID_INVALID;
    fbe_package_id_t  package_id_for_block_op = FBE_PACKAGE_ID_SEP_0;
    fbe_database_lun_info_t lun_info;
    fbe_sg_element_t  sg_elements[FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS+1] = {0}; 
    fbe_payload_block_operation_flags_t payload_flags = FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_INVALID;
    fbe_raid_verify_error_counts_t verify_error_counts = {0};
    fbe_block_transport_block_operation_info_t  block_operation_info = {0};
    fbe_data_pattern_info_t data_pattern_info = {0};
    fbe_u64_t  header_array[FBE_DATA_PATTERN_MAX_HEADER_PATTERN];
    fbe_data_pattern_flags_t pattern_flags = FBE_DATA_PATTERN_FLAGS_INVALID;
    fbe_block_count_t split_io_size = 0;
    fbe_bool_t        b_unmap = FBE_FALSE;

    if (argc == 0)
    {
        fbe_cli_printf("rdt: ERROR: Too few args.\n");
        fbe_cli_printf("%s", RDT_CMD_USAGE);
        return;
    }
    if ((strcmp(*argv, "-help") == 0) ||
        (strcmp(*argv, "-h") == 0) )
    {
        /* If they are asking for help, just display help and exit.
        */
        fbe_cli_printf("%s", RDT_CMD_USAGE);
        return;
    }
    if (**argv == 'f')
    {
        /* Return the buffer if one was allocated.
         */
        if (buffer)
        {
            fbe_api_free_memory(buffer);
            buffer = NULL;
            fbe_cli_printf("\nBuffer Deleted.\n");
        }
        else
        {
            fbe_cli_printf("\nrdt: ERROR: No Buffer Allocated!\n");
        }
        return;
    }
    /* Parse args
     */
    while (argc && **argv == '-')
    {
        if (!strcmp(*argv, "-just_display"))
        {
            fbe_cli_rdt_display_sector(0, buffer, FBE_BE_BYTES_PER_BLOCK / 4);
            return;
        }
        if (!strcmp(*argv, "-sp"))
        {
            b_set_pattern = FBE_TRUE;
        }
        else if (!strcmp(*argv, "-cp"))
        {                                
            b_check_pattern = FBE_TRUE;
        }
        else if (!strcmp(*argv, "-cz"))
        {                                
            b_check_zeros = FBE_TRUE;
        }
        else if (!strcmp(*argv, "-d"))
        {
            b_display = FBE_TRUE;
        }
        else if (!strcmp(*argv, "-no_crc"))
        {
            b_no_crc = FBE_TRUE;
        }
        else if (!strcmp(*argv, "-bad_crc"))
        {
            b_bad_crc = FBE_TRUE;
        }
        else if (!strcmp(*argv, "-unmap"))
        {
            b_unmap = FBE_TRUE;
        }
        else if (!strcmp(*argv, "-object_id"))
        {
            argc--;
            argv++;
            /* Next argument is object id value(interpreted in HexaDECIMAL).
            */
            tmp_ptr = *argv;
            if ((*tmp_ptr == '0') && 
                (*(tmp_ptr + 1) == 'x' || *(tmp_ptr + 1) == 'X'))
            {
                tmp_ptr += 2;
            }
            object_id_for_block_op = fbe_atoh(tmp_ptr);
        }
        else if (!strcmp(*argv, "-split_io"))
        {
            argc--;
            argv++;
            /* Next argument is object id value(interpreted in HexaDECIMAL).
            */
            tmp_ptr = *argv;
            if ((*tmp_ptr == '0') && 
                (*(tmp_ptr + 1) == 'x' || *(tmp_ptr + 1) == 'X'))
            {
                tmp_ptr += 2;
            }
            split_io_size = fbe_atoh(tmp_ptr);
        }
        else if (!strcmp(*argv, "-package_id"))
        {
            argc--;
            argv++;
            if (!strcmp(*argv, "physical"))
            {
                package_id_for_block_op = FBE_PACKAGE_ID_PHYSICAL;
            }
            else if (!strcmp(*argv, "sep"))
            {
                package_id_for_block_op = FBE_PACKAGE_ID_SEP_0;
            }
            else
            {
                fbe_cli_error("%s unknown package id found for -package_id (%s) argument\n", __FUNCTION__, *argv);
                return;
            }

        }
        else if (!strcmp(*argv, "-pattern"))
        {
            argc--;
            argv++;
            if (!strcmp(*argv, "lost"))
            {
                pattern_flags = FBE_DATA_PATTERN_FLAGS_DATA_LOST;
            }
            else if (!strcmp(*argv, "raid"))
            {
                pattern_flags = FBE_DATA_PATTERN_FLAGS_RAID_VERIFY;
            }
            else if (!strcmp(*argv, "cor_crc"))
            {
                pattern_flags = FBE_DATA_PATTERN_FLAGS_CORRUPT_CRC;
            }
            else if (!strcmp(*argv, "cor_data"))
            {
                pattern_flags = FBE_DATA_PATTERN_FLAGS_CORRUPT_DATA;
            }
            else
            {
                fbe_cli_error("%s unknown pattern flags found for -pattern (%s) argument\n", __FUNCTION__, *argv);
                return;
            }
        }
        else
        {
            fbe_cli_printf("rdt: ERROR: Invalid switch.\n");
            fbe_cli_printf("%s", RDT_CMD_USAGE);
            return;
        }
        argc--;
        argv++;
    }

    if (argc < 4)
    {
        fbe_cli_printf("rdt: ERROR: Too few args.\n");
        fbe_cli_printf("%s", RDT_CMD_USAGE);
        return;
    }
    /* The first argument is a single/multiple character representing the 
     * desired RDT op.      *******************************
     */
    rdt_op = **argv;
    rdt_op_str = *argv;
    rdt_op_len = (fbe_u16_t)strlen(rdt_op_str);

    argc--;
    argv++;

    /*
     * object_id_for_block_op is valid if user have provided it. In that case, 
     * do not parse lun information since value does not matter in this case.
     */
    if (object_id_for_block_op == FBE_OBJECT_ID_INVALID)
    {
        /* The next argument is the LUN (interpreted in HexaDECIMAL).
         */
        tmp_ptr = *argv;
        if ((*tmp_ptr == '0') && 
            (*(tmp_ptr + 1) == 'x' || *(tmp_ptr + 1) == 'X'))
        {
            tmp_ptr += 2;
        }
        lunnum = atoi(tmp_ptr);
        if ((lunnum < 0))
        {
            fbe_cli_printf("rdt: ERROR: Invalid argument (lunnum).\n");
            fbe_cli_printf("%s", RDT_CMD_USAGE);
            return;
        }

        status = fbe_api_database_lookup_lun_by_number(lunnum, 
                                                          &lun_object_id);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rdt: ERROR: Lun %d does not exists \n",
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
            fbe_cli_printf("rdt: ERROR: Lun %d details are not available\n ",
                           lunnum);
            return;
        }
        optimal_block_size = lun_info.raid_info.optimal_block_size;
    }
    else
    {
        /* Just ignore LUN value in case block operation is targeted on provided object id*/
    }


    argc--;
    argv++;

    /* The next argument is the LBA value (always interpreted in HEX)
     */
    if (argc)
    {
        status = fbe_atoh64(*argv, &lba);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rdt: ERROR: Invalid LBA format.\n");
            return;
        }

        argc--;
        argv++;
    }
    else
    {
        fbe_cli_printf("rdt: ERROR: Missing required LBA.\n");
        fbe_cli_printf("%s", RDT_CMD_USAGE);
        
        /* Set the lba to an invalid value.
         */
        lba = FBE_LBA_INVALID;
    }

    /* The next argument is the block count (interpreted in HEX).
     * The last argument in argv, argv[argc], is NULL 
     */
    if(argc)
    {
        /* So if argv[current index] does have something get it
         */
        tmp_ptr = *argv;

        if (!strcmp(*argv, "end"))
        {
            /* Just calculate the distance from here to end.
             */
            count = (fbe_u32_t)(lun_info.capacity - lba);
        }
        else 
        {
            if ((*tmp_ptr == '0') && 
                (*(tmp_ptr + 1) == 'x' || *(tmp_ptr + 1) == 'X'))
            {
                tmp_ptr += 2;
            }
            
            count = fbe_atoh(tmp_ptr);
        }

        /* Check to make sure that count has a valid value.
         * The max we can allocate is FBE_CLI_RDT_MAX_SECTORS_BLOCK_OP, since
         * this is the max we can fit in an sg list.
         * Zeros/Rebuild/Verify are allowed to transfer as much as they wish since
         * they do not need a buffer.
         */
        if ( 
            ((rdt_op != 'z') && (strcmp(*argv, "v") ) && (strcmp(*argv, "rb") ) && strcmp(rdt_op_str, "ws")) &&
            ((count == 0) || (count > FBE_CLI_RDT_MAX_SECTORS_BLOCK_OP))
            )

        {
            fbe_cli_printf("rdt: ERROR: Invalid argument (count).\n");
            fbe_cli_printf("%s", RDT_CMD_USAGE);
            return;
        }
    }

    /* Now that we have blocks and count, let's
     * check to see if patterns need to be set or checked.
     */
    if ( rdt_op != 'r' && (b_check_pattern || b_check_zeros ) )
    {
        if ( buffer )
        {
            /* Check for standard pattern or zeros.
             *
            fcli_rdt_check_pattern(lba, physical_block_size, b_check_zeros);*/
        }
        else
        {
            fbe_cli_printf("rdt: ERROR: no buffer allocated, "
                           "please do a read first.\n");
        }
        return;
    }
    else if ( b_display && rdt_op != 'r')
    {
        if ( buffer )
        {
            fbe_cli_printf("\nHere's your data:\n");
            fbe_cli_printf("\n      BuffAddr          "
                           "Sector        Offset  Data\n");
            fbe_cli_printf("  ----------------  ----------------  "
                           "------  -----------------------------------\n");
            
            /*fbe_cli_rdt_print_buffer_contents();*/
        }
        else
        {
            fbe_cli_printf("rdt: ERROR: no buffer allocated, "
                "please do a read first.\n");
        }
        
        return;
    }
    /*
     * object_id_for_block_op is valid if user have provided it. In that case, 
     * do not validate using lun information values.
     */
    if (object_id_for_block_op == FBE_OBJECT_ID_INVALID)
    {
        /* !TODO Give warning about the use of use physical block size and
         * it is only allowed for READ, WRITE or WRITE SAME commands.
         */
        /* If it is a verity, verify blocks or read remap, validate capacity.
         */
        if ( rdt_op == 'v' || rdt_op == 'e' )
        {
            fbe_lba_t physical_capacity = 
                (fbe_u32_t)( (lun_info.capacity) / 
                (lun_info.raid_info.num_data_disk) );
            
            /* These all take physical addresses relative to the unit
             * and not containing an address offset.
             * range is 0..capacity/data_disks.
             */
            if ( lba + count > physical_capacity )
            {
                fbe_cli_printf("rdt: ERROR: Invalid argument "
                              "(unit relative physical LBA).\n"
                       "This number does not include the address offset.\n");
                fbe_cli_printf("The valid range for LUN %d is 0..0x%llX\n",
	                       lunnum, (unsigned long long)physical_capacity);
                fbe_cli_printf("%s", RDT_CMD_USAGE);
                return;
            }
        }
        else if (lba + count > lun_info.capacity)
        {
            /* Check to make sure that the starting lba + count 
             * is not greater than unit capacity.
             * All other commands use logical block addresses.
             */
            fbe_cli_printf("rdt: ERROR: Invalid argument (Logical Address LBA).\n");
            fbe_cli_printf("The valid range for LUN %d is 0..0x%llX\n",
	                   lunnum, (unsigned long long)lun_info.capacity);
            fbe_cli_printf("%s", RDT_CMD_USAGE);
            return;
        }
    }

    if( (!strcmp(rdt_op_str, "v")) || 
        (!strcmp(rdt_op_str, "ev")) || 
        (!strcmp(rdt_op_str, "sv")) || 
        (!strcmp(rdt_op_str, "av")) || 
        (!strcmp(rdt_op_str, "rb")) ||
        (!strcmp(rdt_op_str, "z"))
        )
    {
        /* Verify, Rebuild, .  We don't have to supply a buffer.
         * for verify write and verify with buffer, we need sg_ptr
         */
        sg_ptr = NULL;
        /* 
        * set the block count
        */
        block_count = count;
    }
    else
    {
        if (rdt_op == 'r' || 
            ((rdt_op == 'w' || !strcmp(rdt_op_str, "ws")) && b_set_pattern))
        {
            /* We need to allocate a buffer if one doesn't exist.
            * Reads always allocate a buffer, and
            * writes that will be putting a pattern into the buffer
            * also allocate one.x
            */
            if (buffer)
            {
                /* The sg we have is too small. 
                * Free the sg here and get a new one below.
                */
                fbe_api_free_memory(buffer);
                buffer = NULL;
            }

            if (!buffer)
            {
                /* WRITE SAME uses a single block.
                */
                fcli_rdt_count = !strcmp(rdt_op_str, "ws") ? 1 : count;
                
                /* 
                * set the block count
                */
                block_count = fcli_rdt_count;
                buffer = fbe_api_allocate_memory(block_count * block_size);
                if (buffer == NULL)
                {
                    return;
                }
            }
        }
        else
        {
            /* Data must be read into a buffer before it can be written.
            */
            if (!buffer)
            {
                fbe_cli_printf("rdt: ERROR: Data must be read into RDT buffer"
                    " before a write/write same may be executed.\n");
                return;
            }
            else if (count > fcli_rdt_count)
            {
                if (fcli_rdt_count < 1 || strcmp(rdt_op_str, "ws") )
                {
                    fbe_cli_printf("rdt: ERROR: Saved buffer is too small.\n");
                    return;
                }
            }
            /* 
            * set the block count
            */
            block_count = count;
        }
        /* first sg element contains verify counts ptr*/
        sg_ptr = sg_elements;

        if (count > fcli_rdt_count) {
            status = fbe_data_pattern_sg_fill_with_memory(sg_ptr,
                                    buffer,
                                    fcli_rdt_count,
                                    block_size,
                                    FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS,
                                    FBE_DATA_PATTERN_MAX_BLOCKS_PER_SG);
        } else {
            status = fbe_data_pattern_sg_fill_with_memory(sg_ptr,
                                    buffer,
                                    block_count,
                                    block_size,
                                    FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS,
                                    FBE_DATA_PATTERN_MAX_BLOCKS_PER_SG);
        }
        if (status != FBE_STATUS_OK )
        {
            fbe_cli_printf("rdt: ERROR: Setting up Sg failed.\n");
            return;
        }
        /*
         * fill data pattern information structure
         */
        header_array[0] = FBE_RDT_HEADER_PATTERN;
        header_array[1] = FBE_RDT_HEADER_PATTERN;
        status = fbe_data_pattern_build_info(&data_pattern_info,
                                             FBE_DATA_PATTERN_LBA_PASS,
                                             0, /* No special data pattern flags */
                                             (fbe_u32_t)lba,
                                             0x55,
                                             2,
                                             header_array);
        if (status != FBE_STATUS_OK )
        {
            fbe_cli_printf("rdt: ERROR: Building data pattern info failed.\n");
            return;
        }
        /* 
        * set pattern
        */
        if ( b_set_pattern )
        {
            status = fbe_data_pattern_fill_sg_list(sg_ptr,
                                                   &data_pattern_info,
                                                   block_size,
                                                   0, /* Do not corrupt any blocks*/
                                                   FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS);
            if (status != FBE_STATUS_OK )
            {
                fbe_cli_printf("rdt: ERROR: Filling up Sg failed.\n");
                return;
            }
        }

        if (b_bad_crc)
        {
            /* Write with intentionally corrupted blocks.
             */
            data_pattern_info.pattern = FBE_DATA_PATTERN_CLEAR;
            status = fbe_data_pattern_fill_sg_list(sg_ptr,
                                                   &data_pattern_info,
                                                   block_size,
                                                   (fbe_u64_t)-1,
                                                   FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS);
            if (status != FBE_STATUS_OK )
            {
                fbe_cli_printf("rdt: ERROR: Filling up Sg failed.\n");
                return;
            }
        }
        else if (rdt_op == 'd')
        {
            /* 
            * corrupt DATA
            */
            data_pattern_info.pattern_flags |= FBE_DATA_PATTERN_FLAGS_CORRUPT_DATA;
            /*! @note Currently corrupt data corrupts all the blocks.
             */
            corrupt_blocks_bitmap = (fbe_u64_t)-1;
            status = fbe_data_pattern_fill_sg_list(sg_ptr,
                                                   &data_pattern_info,
                                                   block_size,
                                                   corrupt_blocks_bitmap, /* Corrupt data corrupts all blocks */
                                                   FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS);
            if (status != FBE_STATUS_OK )
            {
                fbe_cli_printf("rdt: ERROR: Filling up Sg failed.\n");
                return;
            }
        }
        else if (rdt_op == 'c')
        {
            data_pattern_info.pattern_flags |= FBE_DATA_PATTERN_FLAGS_CORRUPT_CRC;
            corrupt_blocks_bitmap = FBE_U64_MAX;
            status = fbe_data_pattern_fill_sg_list(sg_ptr,
                                                   &data_pattern_info,
                                                   block_size,
                                                   corrupt_blocks_bitmap,
                                                   FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS);
            if (status != FBE_STATUS_OK )
            {
                fbe_cli_printf("rdt: ERROR: Filling up Sg failed.\n");
                return;
            }
        }
        else if (pattern_flags != FBE_DATA_PATTERN_FLAGS_INVALID)
        {
            data_pattern_info.pattern_flags |= pattern_flags;
            corrupt_blocks_bitmap = FBE_U64_MAX;
            status = fbe_data_pattern_fill_sg_list(sg_ptr,
                                                   &data_pattern_info,
                                                   block_size,
                                                   corrupt_blocks_bitmap,
                                                   FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS);
            if (status != FBE_STATUS_OK )
            {
                fbe_cli_printf("rdt: ERROR: Filling up Sg failed.\n");
                return;
            }
        }
    }
    /*
     * object_id_for_block_op is valid if user have provided it.
     */
    /* Verify sends down the PBA not the LBA. Update
     * lba to reflect this.
     */
    if (object_id_for_block_op == FBE_OBJECT_ID_INVALID)
    {
        if ((rdt_op == 'v') || 
            (!strcmp(rdt_op_str, "vb")) ||
            (!strcmp(rdt_op_str, "rv"))
            )
        {
            lba += lun_info.offset;
            object_id_for_block_op = lun_info.raid_group_obj_id;

            if( (count % optimal_block_size)
                != 0 )
            {
                block_count = optimal_block_size;
            }
        }
        else
        {
            object_id_for_block_op = lun_object_id;
        }
    }
    else
    {
#if 0 /* we are sending pdo object id and raid group object id for which
       FBE_BLOCK_TRANSPORT_CONTROL_CODE_NEGOTIATE_BLOCK_SIZE is not supported
       Only ldo supports it*/
        fbe_block_transport_negotiate_t* negotiate_info_ptr = NULL;
        fbe_block_transport_negotiate_t negotiate_info = {0};
        negotiate_info.block_size = block_size;
        negotiate_info.block_count =  block_count;
        negotiate_info.requested_block_size =  block_size;

        status = fbe_api_block_transport_negotiate_info(
                                         object_id_for_block_op,
                                         package_id_for_block_op,
                                         &negotiate_info);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rdt: Error: Negotiate info operation failed %d\n", 
                           object_id_for_block_op);
            return;
        }
        optimal_block_size = negotiate_info.optimum_block_size;
#endif
        optimal_block_size = 1;
    }

    
    if(rdt_op_len == 1)
    {
        switch(rdt_op)
        {
        case 'r':
            rdt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ;
            break;
        case 'w':
            rdt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE;
            break;
        case 'z':
            rdt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO;
            break;
        case 'v':
            rdt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY;
            break;
        case 'c':
        case 'l':
            rdt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE;
            break;
        case 'd':
            rdt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA;
            break;
        default:
            break;
        }
    }
    else if( !strcmp(rdt_op_str, "rv"))
    {
        rdt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY;
    }
    else if( !strcmp(rdt_op_str, "ev"))
    {
        rdt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY;
    }
    else if( !strcmp(rdt_op_str, "iv"))
    {
        rdt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY;
    }
    else if( !strcmp(rdt_op_str, "sv"))
    {
        rdt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_SYSTEM_VERIFY;
    }    
    else if( !strcmp(rdt_op_str, "sav"))
    {
        rdt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_SPECIFIC_AREA;
    }
    else if( !strcmp(rdt_op_str, "rb"))
    {
        rdt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD;
    }
    else if( !strcmp(rdt_op_str, "wv"))
    {
        rdt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY;
    }
    else if( !strcmp(rdt_op_str, "ws"))
    {
        rdt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME;
    }
    else if( !strcmp(rdt_op_str, "wn"))
    {
        rdt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED;
    }
    else if( !strcmp(rdt_op_str, "vw"))
    {
        rdt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE;
    }
    else if( !strcmp(rdt_op_str, "vb"))
    {
        rdt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WITH_BUFFER;
    }
    else
    {
        fbe_cli_printf("rdt: ERROR: Invalid argument.\n");
        /* Return the buffer if one was allocated.
         */
        if (buffer)
        {
            fbe_api_free_memory(buffer);
            buffer = NULL;
            printf("\nBuffer Deleted.\n");
        }
        return;
    }
    status = fbe_payload_block_build_operation(&block_operation,
                                               rdt_block_op,
                                               lba,
                                               block_count,
                                               block_size,
                                               optimal_block_size,
                                               NULL);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("rdt: Error: Block operation build failed %d\n", 
                       object_id_for_block_op);
        return;
    }
    if ((rdt_block_op == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME) && b_unmap)
    {
        fbe_cli_printf("UNMAP for WRITE SAME command\n");
		fbe_zero_memory(sg_ptr->address, sg_ptr->count);
        fbe_payload_block_set_flag(&block_operation, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_UNMAP);
    }
    if (b_no_crc)
    {
        fbe_cli_printf("Check checksum was not requested.  Clearing check checksum flag\n");
        fbe_payload_block_clear_flag(&block_operation, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM);
    }
    if( (rdt_op == 'c') || (rdt_op == 'l') || (rdt_op == 'd'))
    {
        if ((rdt_op == 'c'))
        {
            payload_flags = FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CORRUPT_CRC;
            fbe_payload_block_set_flag(&block_operation, payload_flags);
        }
        /* clear check checksum flag for corrupt crc and data operation*/
        fbe_payload_block_clear_flag(&block_operation, 
            FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM);
    }
	/*
    fbe_payload_block_set_verify_error_count_ptr(&block_operation,
                                                 &verify_error_counts);
	*/

    block_operation_info.block_operation = block_operation;
    block_operation_info.verify_error_counts = verify_error_counts;

    /* testing split io interface */
    if ((split_io_size != 0) && (rdt_block_op == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ))
    {
        status = fbe_api_block_transport_read_data(object_id_for_block_op, 
                                                   package_id_for_block_op,
                                                   (fbe_u8_t *) buffer, 
                                                   block_count, 
                                                   lba, 
                                                   split_io_size,
                                                   4);
        block_operation_info.block_operation.status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
    }
    else if ((split_io_size != 0) && (rdt_block_op == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE))
    {
        status = fbe_api_block_transport_write_data(object_id_for_block_op, 
                                                   package_id_for_block_op,
                                                   (fbe_u8_t *) buffer, 
                                                   block_count, 
                                                   lba, 
                                                   split_io_size,
                                                   4);
        block_operation_info.block_operation.status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
    }
    else
    {
	    /* This is wrong API! we should send packet and not bloc operation */
        status = fbe_api_block_transport_send_block_operation(
                                         object_id_for_block_op,
                                         package_id_for_block_op,
                                         &block_operation_info,
                                         (fbe_sg_element_t*)sg_ptr);
    }
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("rdt: Error: Block operation failed to object_id: %d pkt_status: 0x%x\n", 
                       object_id_for_block_op, 
                       status);
        /*return;*/
    }

    block_operation = block_operation_info.block_operation;
    /*
     * Now display the result
     */
    fbe_cli_rdt_display_result(&block_operation_info, 
                               (fbe_sg_element_t*)sg_ptr,
                               b_check_pattern,
                               b_check_zeros,
                               b_display);
    /*
     * Display the verify count result
     */
	/*
    fbe_cli_rdt_display_verify_counter(
        (fbe_raid_verify_error_counts_t*)&(block_operation_info.verify_error_counts));
	*/
    fbe_cli_printf("\n");
    return;
}
/******************************************
 * end fbe_cli_rdt_parse_cmds()
 ******************************************/

/******************************************
 * end fbe_cli_rdt_send_operation()
 ******************************************/
/*!**************************************************************
 *          fbe_cli_rdt_display_and_check_pattern()
 ****************************************************************
 *
 * @brief   This function is used to display and check pattern
 *          recieved after block operation
 *
 * @param   block_operation_p
 * @param   sg_p
 * @param   b_check_pattern
 * @param   b_check_zeros
 *
 * @return  None.
 *
 * @revision:
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rdt_display_and_check_pattern(fbe_payload_block_operation_t *block_operation_p,
                                           fbe_sg_element_t *sg_p,
                                           fbe_bool_t b_check_pattern,
                                           fbe_bool_t b_check_zeros)
{
    fbe_status_t status;
    fbe_data_pattern_info_t data_pattern_info = {0};
    fbe_data_pattern_t pattern;
    fbe_u64_t  header_array[FBE_DATA_PATTERN_MAX_HEADER_PATTERN];
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;
    fbe_data_pattern_flags_t data_pattern_flags = 0;
      
    /* Check for corrupt crc
     */
    if ((block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) && 
        (fbe_payload_block_is_flag_set(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CORRUPT_CRC)) )
    {
        data_pattern_flags |= FBE_DATA_PATTERN_FLAGS_CORRUPT_CRC;
    }
    else if (block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA)
    {
        /* Check for corrupt data
         */
        data_pattern_flags |= FBE_DATA_PATTERN_FLAGS_CORRUPT_DATA;
    }

    /* Check for pattern
     */
    if (b_check_pattern)
    {
        pattern = FBE_DATA_PATTERN_LBA_PASS;
    }
    else if (b_check_zeros)
    {
        pattern = FBE_DATA_PATTERN_ZEROS;
    }
    /*
     * fill data pattern information structure
     */
    header_array[0] = FBE_RDT_HEADER_PATTERN;
    header_array[1] = FBE_RDT_HEADER_PATTERN;
    status = fbe_data_pattern_build_info(&data_pattern_info,
                                         pattern,
                                         data_pattern_flags, /* Set data pattern flags */
                                         (fbe_u32_t)block_operation_p->lba,
                                         0x55,/*pass_count*/
                                         2,
                                         header_array);
    if (status != FBE_STATUS_OK )
    {
        fbe_cli_printf("rdt: ERROR: Building data pattern info failed.\n");
        return;
    }

    /* If we are reading and the user asked to check
     * the pattern, then do so now.
     */
    /* Next, check the pattern in the blocks we read.
     */
    status = fbe_data_pattern_check_sg_list(sg_p,
                                            &data_pattern_info,
                                            block_operation_p->block_size,
                                            corrupt_blocks_bitmap, /*! @todo Assumes global `corrupt_blocks_bitmap' is still set properly!!*/
                                            object_id,
                                            FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS,
                                            FBE_FALSE /* do not panic on data miscompare */);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("rdt: The SEP/PP reported"
                  " that the RAID Check pattern Failed.\n");
        return;
    }
    fbe_cli_printf("rdt: The SEP/PP reported"
                  " that the RAID Check pattern Succeeded.\n");
}

void fbe_cli_rdt_display_error_counts(fbe_raid_verify_error_counts_t *counts_p)
{
    
    fbe_cli_printf("                       |  Correctable  | Uncorrectable |\n");
    fbe_cli_printf("-------------------------------------------------------\n");
    fbe_cli_printf("             CRC errors |%14d |%14d \n",
                   counts_p->c_crc_count, counts_p->u_crc_count);
    fbe_cli_printf("   Multi bit CRC errors |%14d |%14d \n",
                   counts_p->c_crc_multi_count, counts_p->u_crc_multi_count);
    fbe_cli_printf("  Single bit CRC errors |%14d |%14d \n",
                   counts_p->c_crc_single_count, counts_p->u_crc_single_count);
    fbe_cli_printf("     Write Stamp errors |%14d |%14d \n",
                   counts_p->c_ws_count, counts_p->u_ws_count);
    fbe_cli_printf("      Time stamp errors |%14d |%14d \n",
                   counts_p->c_ts_count, counts_p->u_ts_count);
    fbe_cli_printf("      Shed stamp errors |%14d |%14d \n",
                   counts_p->c_ss_count, counts_p->u_ss_count);
    fbe_cli_printf("       Coherency errors |%14d |%14d \n",
                   counts_p->c_coh_count, counts_p->u_coh_count);
    fbe_cli_printf("           Media errors |%14d |%14d \n",
                   counts_p->c_media_count, counts_p->u_media_count);
    fbe_cli_printf("      Soft media errors |%14d |             -\n",
                   counts_p->c_soft_media_count);
    fbe_cli_printf("        RM Magic errors |%14d |%14d \n", 
                   counts_p->c_rm_magic_count, counts_p->u_rm_magic_count);
    fbe_cli_printf("        RM Seq   errors |%14d |             -\n", 
                   counts_p->c_rm_seq_count);
    fbe_cli_printf("   Non-Retryable errors |%14d |             -\n", 
                   counts_p->non_retryable_errors);
    fbe_cli_printf("       Retryable errors |%14d |             -\n", 
                   counts_p->retryable_errors);
    fbe_cli_printf("        Shutdown errors |%14d |             -\n", 
                   counts_p->shutdown_errors);
}
/*!**************************************************************
 *          fbe_cli_rdt_display_result()
 ****************************************************************
 *
 * @brief   This function is used to display the result of rdt
 *          operation. RDT has following syntax
 * rdt <-d,-sp, -cp, -cz, -up> <b|e|r|w|s|c|d|v|f|z|stop h|
 *     stop s|start h|start s> <lun number in hex> <lba in hex, 0 to 
 *     capacity> [count 0x0 .. 0x1000]
 *
 * @param   block_operation_p
 *
 * @return  None.
 *
 * @revision:
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rdt_display_result(fbe_block_transport_block_operation_info_t *block_op_info_p,
                                fbe_sg_element_t *sg_p,
                                fbe_bool_t b_check_pattern,
                                fbe_bool_t b_check_zeros,
                                fbe_bool_t b_display)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_block_operation_status_t block_op_status;
    fbe_payload_block_operation_qualifier_t block_op_qualifier;
    fbe_payload_block_operation_opcode_t block_op_opcode;
    fbe_payload_block_operation_t *block_operation_p = &block_op_info_p->block_operation;
    fbe_u8_t status_str[128] = "";
    fbe_u8_t qualifier_str[128] = "";

    block_op_status = block_operation_p->status;
    block_op_qualifier = block_operation_p->status_qualifier;
    block_op_opcode = block_operation_p->block_opcode;
    
    fbe_cli_printf("\n");
    
    fbe_cli_printf("rdt: request completed\n");

    status = fbe_api_block_transport_status_to_string(block_op_status, status_str, 128);
    status = fbe_api_block_transport_qualifier_to_string(block_op_qualifier, qualifier_str, 128);

    fbe_cli_printf("\n");

    fbe_cli_printf("Verify Counts: \n");
    fbe_cli_rdt_display_error_counts(&block_op_info_p->verify_error_counts);
    fbe_cli_printf("\n");

    if ((block_op_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) ||
        (block_op_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR))
    {
        if (block_operation_p->block_opcode == 
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ
            && b_display)
        {
            /* User asked to display the buffer after reading it.
             */
            fbe_cli_printf("\nHere's your data:\n");
            fbe_cli_printf("\n      BuffAddr          Sector        "
                           "Offset  Data\n");
            fbe_cli_printf("  ----------------  ----------------  "
                           "------  -----------------------------------\n");

            /* here we might not go for blocks more than 32bit,
             * hence casting it to 32bit
             */

            fbe_cli_rdt_print_buffer_contents(sg_p, 
                                              0, (fbe_u32_t)block_operation_p->block_count, 
                                              FBE_BE_BYTES_PER_BLOCK);
            fbe_cli_printf("Data Read into Buffer\n");
        }
        else if (block_operation_p->block_opcode == 
                 FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ
                 && (b_check_pattern || b_check_zeros))
        {
            fbe_cli_rdt_display_and_check_pattern(block_operation_p,
                                                  sg_p,
                                                  b_check_pattern,
                                                  b_check_zeros);
        }
    }    /* end if status is good. */
}

/*!**************************************************************
 *  fbe_cli_rdt_display_verify_errors()
 ****************************************************************
 *
 * @brief   This function is used to display the errors from 
 *          verify counter pointers
 *
 * @param   verify_err_count_p
 *
 * @return  None.
 *
 * @revision:
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rdt_display_verify_errors(fbe_raid_verify_error_counts_t 
                                *verify_err_count_p)
{
    if (verify_err_count_p->u_crc_count)
    {
        fbe_cli_printf("\t\tun-correctable CRC: %d\n", 
            verify_err_count_p->u_crc_count);
    }
    if (verify_err_count_p->c_crc_count)
    {
        fbe_cli_printf("\t\tcorrectable CRC   : %d\n", 
            verify_err_count_p->c_crc_count);
    }
    if (verify_err_count_p->u_coh_count)
    {
        fbe_cli_printf("\t\tun-correctable COH: %d\n", 
            verify_err_count_p->u_coh_count);
    }
    if (verify_err_count_p->c_coh_count)
    {
        fbe_cli_printf("\t\tcorrectable COH   : %d\n", 
            verify_err_count_p->c_coh_count);
    }
    if (verify_err_count_p->u_ts_count)
    {
        fbe_cli_printf("\t\tun-correctable TS : %d\n", 
            verify_err_count_p->u_ts_count);
    }
    if (verify_err_count_p->c_ts_count)
    {
        fbe_cli_printf("\t\tcorrectable TS    : %d\n", 
            verify_err_count_p->c_ts_count);
    }
    if (verify_err_count_p->u_ws_count)
    {
        fbe_cli_printf("\t\tun-correctable WS : %d\n", 
            verify_err_count_p->u_ws_count);
    }
    if (verify_err_count_p->c_ws_count)
    {
        fbe_cli_printf("\t\tcorrectable WS    : %d\n", 
            verify_err_count_p->c_ws_count);
    }
    if (verify_err_count_p->u_ss_count)
    {
        fbe_cli_printf("\t\tun-correctable SS : %d\n", 
            verify_err_count_p->u_ss_count);
    }
    if (verify_err_count_p->c_ss_count)
    {
        fbe_cli_printf("\t\tcorrectable SS    : %d\n", 
            verify_err_count_p->c_ss_count);
    }

}
/*!**************************************************************
 *  fbe_cli_rdt_display_verify_counter()
 ****************************************************************
 *
 * @brief   This function is used to display the result of rdt
 *          operation. 
 *
 * @param   verify_err_count_p
 *
 * @return  None.
 *
 * @revision:
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rdt_display_verify_counter(fbe_raid_verify_error_counts_t 
                                *verify_err_count_p)
{
    fbe_bool_t error;     /* Whether any errors reported */
    if (verify_err_count_p != NULL)
    {
        /* Check verify error board
         */
        error = verify_err_count_p->u_crc_count +
            verify_err_count_p->c_crc_count +
            verify_err_count_p->u_coh_count +
            verify_err_count_p->c_coh_count +
            verify_err_count_p->u_ts_count +
            verify_err_count_p->c_ts_count +
            verify_err_count_p->u_ws_count +
            verify_err_count_p->c_ws_count +
            verify_err_count_p->u_ss_count +
            verify_err_count_p->c_ss_count;
        if (error)
        {
            fbe_cli_printf("\n\tVerify found the following %d errors ->\n", 
                error);
            fbe_cli_rdt_display_verify_errors(verify_err_count_p);
        }
    }
}

/******************************************
 * end fbe_cli_rdt_display_result()
 ******************************************/

/****************************************************************
 *  fcli_print_buffer_contents()
 ****************************************************************
 *
 * @brief
 *  Print the contents of the buffer
 *
 * @param sg_ptr       - Buffer which contains the data to be printed
 * @param disk_address - Starting disk address in sectors
 * @param blocks - Blocks to display.
 * @param block_size - The logical block size for this request
 *
 * @return NONE
 *
 * @revision
 *  8/12/10 - Created. Swati Fursule
 *
 ****************************************************************/

void fbe_cli_rdt_print_buffer_contents(fbe_sg_element_t *sg_ptr, 
                                 fbe_lba_t disk_address,
                                 fbe_u32_t blocks,
                                 fbe_u32_t block_size)
{
     fbe_u32_t count = 0, orig_count = 0;
     fbe_char_t *addr;
     fbe_u32_t words;
     fbe_bool_t warning=FALSE;
     fbe_u32_t current_block = 0;

     /* Sanity check the block size.
      */
     if ((block_size > FBE_BE_BYTES_PER_BLOCK) 
         || (block_size < FBE_BYTES_PER_BLOCK))
     {
         fbe_cli_printf("rdt Error: %s: block_size: %d not supported.\n",
                __FUNCTION__, block_size);
         return;
     }

     /* Display the block using ddt display sector.
      */
     while (sg_ptr->count)
     {
         addr = sg_ptr->address;
         count = sg_ptr->count;

         while (count > 0)       /* One loop pass per sector */
         {
             if (count < block_size)
             {
                 warning = TRUE;
                 orig_count = count;
                 words = (count + 3) / 4; /* round up to a whole word */
             }
             else
             {
                 words = block_size / 4;
             }
      
             fbe_cli_rdt_display_sector(disk_address, addr, words);
             addr += block_size;
             disk_address++;
             count -= block_size;

             current_block ++;
          
             if ( current_block >= blocks )
             {
                 /* We displayed more than required, exit.
                  */
                 return;
             }
          
             if (warning == TRUE)
             {
                 fbe_cli_printf("Warning:  SG element is not not a multiple"
                     "of block_size %d bytes.\n", block_size);
                 fbe_cli_printf("          count mod block_size"
                    " = %d\n\n", orig_count);
             }
         
         } /* end while there are more blocks in the sg entry*/
         sg_ptr++;
     }

     return;

} 
/*****************************************
 ** End of fbe_cli_rdt_print_buffer_contents()  **
 *****************************************/


/************************************************************************
 *  fbe_cli_rdt_display_sector()
 ************************************************************************
 *
 * @brief
 *   This function dispays ONE SECTOR of data.  
 *
 * @param in_lba  - fru LBA
 * @param in_addr - buffer address
 * @param w_count - Number of 32-bit words to display
 *
 * @return (none)
 *
 * @revision
 *  8/12/10 - Created. Swati Fursule
 ************************************************************************/

void fbe_cli_rdt_display_sector(fbe_lba_t in_lba, 
                                fbe_u8_t * in_addr, 
                                fbe_u32_t w_count)
{
    fbe_u32_t    *w_ptr;
    fbe_u32_t    offset = 0;
    fbe_u64_t    addr, lba;

    /* Map in the memory, (only maps if needed).
     *
    w_ptr = org_w_ptr = HEMI_MAP_MEMORY((UINT_PTR)in_addr, byte_count);*/
    w_ptr = (fbe_u32_t *)in_addr;
    addr = (fbe_u64_t)in_addr;
    lba = in_lba;

    /* This loop displays most of the sector (512 bytes or 128 words).
     */
    while (w_count >= 4)
    {
        fbe_cli_printf("  %016llX  %016llX  %4X:   %08X %08X %08X %08X \n",
		       (unsigned long long)addr, (unsigned long long)lba,
			offset, *w_ptr, *(w_ptr + 1), *(w_ptr + 2), *(w_ptr + 3));
        offset += 16;
        w_ptr += 4;
        w_count -= 4;
    }

    /* Print the last (normally 2) words of the sector (the overhead bytes)
     * or any excess if we're not a multiple of 520.
     */
    if (w_count == 0)
    {
        fbe_cli_printf(" \n");
    }
    else if (w_count == 1)
    {
        fbe_cli_printf("  %016llX  %016llX  %4X:   %08X \n\n",
                       (unsigned long long)addr, (unsigned long long)lba,
		       offset, *w_ptr);
    }
    else if (w_count == 2)
    {
        fbe_cli_printf("  %016llX  %016llX  %4X:   %08X %08X \n\n",
                       (unsigned long long)addr, (unsigned long long)lba,
		       offset, *w_ptr, *(w_ptr + 1));
    }
    else if (w_count == 3)
    {
        fbe_cli_printf("  %016llX  %016llX  %4X:   %08X %08X %08X \n\n",
                       (unsigned long long)addr, (unsigned long long)lba,
		       offset, *w_ptr, *(w_ptr + 1), *(w_ptr + 2));
    }

    /* Unmap Memory.
     *
    HEMI_UNMAP_MEMORY((PVOID)org_w_ptr, byte_count);*/

    return;
}                               
/* end of fbe_cli_rdt_display_sector() */

/*************************
 * end file fbe_cli_lib_rdt_cmds.c
 *************************/
