/***************************************************************************
 * Copyright (C) EMC Corporation 2011 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
* @file fbe_cli_lib_ddt_cmds.c
 ***************************************************************************
 *
 * @brief
*  This file contains cli functions for the DDT related features in FBE CLI.
 *
 * @ingroup fbe_cli
 *
 * @version
 *
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
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_trace.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_utils.h"
#include "fbe_api_base_config_interface.h"
#include "fbe_api_block_transport_interface.h"
#include "fbe_api_lun_interface.h"
#include "fbe_cli_ddt.h"
#include "fbe/fbe_data_pattern.h"

#include "fbe_xor_api.h"
#include "fbe/fbe_random.h"

static void*         buffer = NULL;
static fbe_cli_ddt_type type = FBE_CLI_DDT_TYPE_INVALID;
static xorlib_sector_invalid_reason_t      xor_invalid_reason = XORLIB_SECTOR_INVALID_WHO_UNKNOWN;

/*  Use ddt_block_count to save the global block count when we allocate or deallocate the buffer.
    We need to allocate a buffer when:
     1:  user entered a "get buffer" command
     2:  user entered a "read" command and either we do not have a buffer,
         or it is not the correct size.
*/
static fbe_u64_t     ddt_block_count = 0;

/*!*******************************************************************
 *          fbe_cli_ddt
 *********************************************************************
 * @brief Function to implement ddt commands execution.
 *        fbe_cli_ddt executes a SEP/PP Tester operation.  
 *        This command is for simulation/lab debug only.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @revision:
 *  
 *
 *********************************************************************/
void fbe_cli_ddt(int argc, char** argv)
{
    fbe_cli_ddt_switch_type switch_type = FBE_CLI_DDT_SWITCH_TYPE_INVALID;
    type = FBE_CLI_DDT_TYPE_INVALID;
 

    if (argc == 0)
    {
        fbe_cli_error("\n Too few args.\n");
        fbe_cli_printf("%s", DDT_CMD_USAGE);
        return;
    }
    if ((strncmp(*argv, "-help",5) == 0) ||
        (strncmp(*argv, "-h",2) == 0) )
    {
        /* If they are asking for help, just display help and exit.
        */
        fbe_cli_printf("%s", DDT_CMD_USAGE);
        return;
    }
    if (**argv == 'f')
    {
        /* Free buffer if one was allocated.
         */
        if (buffer != NULL)
        {
            fbe_api_free_memory(buffer);
            buffer = NULL;
            ddt_block_count = 0;
            fbe_cli_printf("\nBuffer Deleted.\n");
            return;
        }
        fbe_cli_error("\n\nNo Buffer Allocated!\n");
        return;
    }
    /* Todo - these switches will be implemented as part of next phase of checkin
     */
    while (argc && **argv == '-')
    {
        if (strncmp(*argv, "-sb",3)  == 0)    /* Todo - in next checkin*/
        {
            switch_type = FBE_CLI_DDT_SWITCH_TYPE_SB;
        }
        else if (strncmp(*argv, "-ws",3)  == 0)
        {
            switch_type = FBE_CLI_DDT_SWITCH_TYPE_WS;
        }
        else if (strncmp(*argv, "-nd",3)  == 0)
        {
            switch_type = FBE_CLI_DDT_SWITCH_TYPE_ND;
        }
        else if (strncmp(*argv, "-sf",3) == 0)    /* Todo - in next checkin*/
        {
            switch_type = FBE_CLI_DDT_SWITCH_TYPE_SF;
        }
        else if (strncmp(*argv, "-fua",4) == 0)    /* Todo - in next checkin*/
        {
            switch_type = FBE_CLI_DDT_SWITCH_TYPE_FUA;
        }
        else if (strncmp(*argv, "-passthru",9)  == 0)      /* Todo - in next checkin*/
        {
            argc--;
            argv++;
            /* Next argument is package id value(interpreted in HexaDECIMAL).
            */
            return;
        }

        else if(strncmp(*argv, "-ldo",4) == 0)
        {
            type = FBE_CLI_DDT_TYPE_LDO;
        }
        else if(strncmp(*argv, "-pdo",4) == 0)
        {
            type = FBE_CLI_DDT_TYPE_PDO;
        }
        else
        {
            fbe_cli_error("\nInvalid switch.\n");
            fbe_cli_printf("%s", DDT_CMD_USAGE);
            return;
        }
        argc--;
        argv++;
    }

    fbe_cli_ddt_get_cmds(argc,argv,switch_type);
    return;
}
/******************************************
 * end fbe_cli_ddt()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_ddt_get_cmds()
 ****************************************************************
 *
 * @brief  this function get ddt commands like read ,write
 *         display and edit
 *
 * @param   argc        - argument count
 *          argv        - argument string
 *          switch_type -contain cmd type for ddt
 *
 * @return  None.
 *
 * @revision:
 *
 ****************************************************************/
void fbe_cli_ddt_get_cmds(int argc,char **argv,fbe_cli_ddt_switch_type switch_type)
{
    fbe_lba_t         lba = FBE_CLI_DDT_DEFAULT_LBA;
    fbe_cli_ddt_cmd_type cmd_type = FBE_CLI_DDT_CMD_TYPE_INVALID;
    fbe_job_service_bes_t phys_location;
    fbe_u64_t         count = FBE_CLI_DDT_DEFAULT_COUNT;/* default = 1 sector*/
    fbe_u32_t         data = FBE_CLI_DDT_DATA_INVALID;
    fbe_u32_t         offset = FBE_CLI_DDT_OFFSET_INVALID;

    if (argc == 0)
    {
        fbe_cli_error("\n Too few args.\n");
        fbe_cli_printf("%s", DDT_CMD_USAGE);
        return;
    }
    if (strncmp(*argv, "r",1) == 0)/*read data from drive*/
    {
        argc--;
        argv++;
        cmd_type = FBE_CLI_DDT_CMD_TYPE_READ;
    }
    else if (strncmp(*argv, "w",1) == 0)/*Write data to drive*/
    {
        argc--;
        argv++;
        cmd_type = FBE_CLI_DDT_CMD_TYPE_WRITE;
    }
    else if (strncmp(*argv, "b",1) == 0)/*Write and verify data to drive*/
    {
        argc--;
        argv++;
        cmd_type = FBE_CLI_DDT_CMD_TYPE_WRITE_VERIFY;
    }
    else if (strncmp(*argv, "d",1) == 0)/*displays buffer data*/
    {
        argc--;
        argv++;
        cmd_type = FBE_CLI_DDT_CMD_TYPE_DISPLAY_INFO;

        phys_location.bus = FBE_API_PHYSICAL_BUS_COUNT;
        phys_location.enclosure = FBE_API_ENCLOSURE_COUNT;
        phys_location.slot = FBE_API_ENCLOSURE_SLOTS;

        fbe_cli_ddt_process_cmds(lba,switch_type,cmd_type,phys_location,count,data,offset);
        return;
    }
    else if (strncmp(*argv, "g",1) == 0)/*allocate memory to buffer*/
    {
        argc--;
        argv++;
        cmd_type = FBE_CLI_DDT_CMD_TYPE_GETBUFFER;
    }
    else if (strncmp(*argv, "v",1) == 0)/*Verify provided block*/
    {
        argc--;
        argv++;
        cmd_type = FBE_CLI_DDT_CMD_TYPE_VERIFYBLOCK;
    }
    else if (strncmp(*argv, "e",1) == 0)/*Edit buffer data*/
    {
        argc--;
        argv++;
        cmd_type = FBE_CLI_DDT_CMD_TYPE_EDIT;

        phys_location.bus = FBE_API_PHYSICAL_BUS_COUNT;
        phys_location.enclosure = FBE_API_ENCLOSURE_COUNT;
        phys_location.slot = FBE_API_ENCLOSURE_SLOTS;

        fbe_cli_ddt_get_edit_cmd(argc,argv,lba,switch_type,cmd_type,phys_location,count);
        return;
    }
    else if (strncmp(*argv, "x",1) == 0)/*Calculate checksum for buffer data*/
    {
        argc--;
        argv++;
        cmd_type = FBE_CLI_DDT_CMD_TYPE_CHECKSUM_BUFFER;

        phys_location.bus = FBE_API_PHYSICAL_BUS_COUNT;
        phys_location.enclosure = FBE_API_ENCLOSURE_COUNT;
        phys_location.slot = FBE_API_ENCLOSURE_SLOTS;

        fbe_cli_ddt_get_checksum_cmd(argc,argv,lba,switch_type,cmd_type,phys_location,count);
        return;
    }
    else if (strncmp(*argv, "i",1) == 0)/*Invalidate checksum for buffer data*/
    {
        argc--;
        argv++;
        cmd_type = FBE_CLI_DDT_CMD_TYPE_INVALIDATE_CHECKSUM;

        phys_location.bus = FBE_API_PHYSICAL_BUS_COUNT;
        phys_location.enclosure = FBE_API_ENCLOSURE_COUNT;
        phys_location.slot = FBE_API_ENCLOSURE_SLOTS;

        fbe_cli_ddt_get_invalidate_checksum_cmd(argc,argv,lba,switch_type,cmd_type,phys_location,count);
        return;

    }
    else if (strncmp(*argv, "k",1) == 0)/*Kill FRU*/
    {
        argc--;
        argv++;
        cmd_type = FBE_CLI_DDT_CMD_TYPE_KILL_FRU;
    }
    else if (strncmp(*argv, "u",1) == 0)/*Standby on-off.*/
    {
        argc--;
        argv++;
        cmd_type = FBE_CLI_DDT_CMD_TYPE_SET_HIBERNATE_STATE;
    }


    fbe_cli_ddt_get_cmd_attribute(argc,argv,switch_type,cmd_type);
    return;
}
/******************************************
 * end fbe_cli_ddt_get_cmds()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_ddt_process_cmds()
 ****************************************************************
 *
 * @brief  this function Process ddt commands like read ,write
 *         display and edit
 *
 * @param   lba           -lba starting point for drive
 *          switch_type   -contain cmd type for ddt
 *          cmd_type      -contain cmd type for ddt
 *          phys_location -physical location of drive
 *          count         -block count
 *          data          -data(this data is used while 
 *                         editing the buffer)
 *          offset        -offset of buffer
 *
 * @return  None.
 *
 * @revision:
 *
 ****************************************************************/
void fbe_cli_ddt_process_cmds(fbe_lba_t lba,
                            fbe_cli_ddt_switch_type switch_type,
                            fbe_cli_ddt_cmd_type cmd_type,
                            fbe_job_service_bes_t phys_location,
                            fbe_u64_t count,
                            fbe_u32_t data,
                            fbe_u32_t offset)
{
    switch (cmd_type)
    {
        case FBE_CLI_DDT_CMD_TYPE_DISPLAY_INFO:
            fbe_cli_ddt_cmd_display(count);
            break;
        case FBE_CLI_DDT_CMD_TYPE_GETBUFFER:
            fbe_cli_ddt_cmd_getbuffer(count);
            break;
        case FBE_CLI_DDT_CMD_TYPE_READ:
            fbe_cli_ddt_cmd_read(lba,switch_type,phys_location,count);
            break;
        case FBE_CLI_DDT_CMD_TYPE_WRITE:
            fbe_cli_ddt_cmd_write(lba,switch_type,phys_location,count);
            break;
        case FBE_CLI_DDT_CMD_TYPE_WRITE_VERIFY:
            fbe_cli_ddt_cmd_write(lba,switch_type,phys_location,count);
            break;
        case FBE_CLI_DDT_CMD_TYPE_EDIT:
            fbe_cli_ddt_cmd_edit(data,offset,count);
            break;
        case FBE_CLI_DDT_CMD_TYPE_VERIFYBLOCK:
            fbe_cli_ddt_cmd_verify(lba,switch_type,phys_location,count);
            break;
        case FBE_CLI_DDT_CMD_TYPE_CHECKSUM_BUFFER:
            fbe_cli_ddt_cmd_checksum_buffer(lba,switch_type,phys_location,count);
            break;
        case FBE_CLI_DDT_CMD_TYPE_INVALIDATE_CHECKSUM:
            fbe_cli_ddt_cmd_invalidate_checksum(lba,switch_type,phys_location,count);
            break;
        case FBE_CLI_DDT_CMD_TYPE_KILL_FRU:
            fbe_cli_ddt_cmd_kill_drive(lba,switch_type,phys_location);
            break;
        case FBE_CLI_DDT_CMD_TYPE_SET_HIBERNATE_STATE:
            fbe_cli_ddt_cmd_set_hibernate_state(lba,switch_type,phys_location);
            break;
#if 0
        case FBE_CLI_DDT_CMD_TYPE_SET_FRU_WRITE_CACHE_STATE:
            fbe_cli_ddt_cmd_set_drive_wc_state(lba,switch_type,phys_location);
            break;
#endif
        case FBE_CLI_DDT_CMD_TYPE_MAX:
        case FBE_CLI_DDT_CMD_TYPE_INVALID:
        default:
            fbe_cli_printf("Invalid DDT command type: 0x%x\n", cmd_type);
            break;
    } /* End of switch */
    return;
}
/******************************************
 * end fbe_cli_ddt_process_cmds()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_ddt_cmd_read()
 ****************************************************************
 *
 * @brief this function read data from drive & write to buffer
 *
 * @param   lba           -lba starting point for drive
 *          switch_type   -switch for the further functioning
 *          phys_location -physical location of drive
 *          count         -block count

 *
 * @return  None.
 *
 * @revision:
 *
 ****************************************************************/

void fbe_cli_ddt_cmd_read( fbe_lba_t lba,
                           fbe_cli_ddt_switch_type switch_type,
                           fbe_job_service_bes_t phys_location,
                           fbe_u64_t count)

{
    fbe_payload_block_operation_opcode_t ddt_block_op;
    fbe_u32_t         block_size = FBE_BE_BYTES_PER_BLOCK;
    
    /* We need to allocate a buffer if one doesn't exist.
     */
    if (buffer)
    {
        /* if buffer already exists, we free it up and reallocate a new one  to read in the fresh data
        */
        fbe_api_free_memory(buffer);
        ddt_block_count = 0;
        buffer = NULL;
    }

    if (buffer == NULL)
    {
        /* Reallocate a new buffer to read in the data*/

        buffer = fbe_api_allocate_memory((fbe_u32_t)count * block_size);
        if (buffer == NULL)
        {
            fbe_cli_error("\nUnable to allocate the memory to a buffer");
            return;
        }
        
        ddt_block_count = count;
    }

     ddt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ;

     fbe_cli_ddt_block_operation(ddt_block_op,ddt_block_count,lba,phys_location,switch_type);

     return;

}
/******************************************
 * end fbe_cli_ddt_cmd_read()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_ddt_cmd_verify()
 ****************************************************************
 *
 * @brief this function (SCSI VERIFY) will cause the drive to  
 *        internally attempt reading the media
 *
 * @param   lba           -lba starting point for drive
 *          switch_type   -switch for the further functioning
 *          phys_location -physical location of drive
 *          count         -block count

 *
 * @return  None.
 *
 * @revision:
 *
 ****************************************************************/

void fbe_cli_ddt_cmd_verify( fbe_lba_t lba,
                           fbe_cli_ddt_switch_type switch_type,
                           fbe_job_service_bes_t phys_location,
                           fbe_u64_t count)

{
    fbe_payload_block_operation_opcode_t ddt_block_op;
    fbe_u32_t         block_size = FBE_BE_BYTES_PER_BLOCK;
    
    /* We need to allocate a buffer if one doesn't exist.
     */
    if (buffer)
    {
        /* if buffer already exists, we free it up and reallocate a new one  to read in the fresh data
        */
        fbe_api_free_memory(buffer);
        ddt_block_count = 0;
        buffer = NULL;
    }

    if (buffer == NULL)
    {
        /* Reallocate a new buffer to read in the data*/
        buffer = fbe_api_allocate_memory((fbe_u32_t)count * block_size);
        if (buffer == NULL)
        {
            fbe_cli_error("\nUnable to allocate the memory to a buffer");
            return;
        }
        
        ddt_block_count = count;
    }

    ddt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY;

    fbe_cli_ddt_block_operation(ddt_block_op, count,lba,phys_location,switch_type);

     return;

}
/******************************************
 * end fbe_cli_ddt_cmd_verify()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_ddt_cmd_write()
 ****************************************************************
 *
 * @brief  this function write data on driver from buffer 
 *
 * @param   lba           -lba starting point for drive
 *          switch_type   -switch for the further functioning
 *          phys_location -physical location of drive
 *          count         -block count
 *
 * @return  None.
 *
 * @revision:
 *
 ****************************************************************/

void fbe_cli_ddt_cmd_write(fbe_lba_t lba,
                                        fbe_cli_ddt_switch_type switch_type,
                                        fbe_job_service_bes_t phys_location,
                                        fbe_u64_t count)

{
    fbe_payload_block_operation_opcode_t ddt_block_op;
    
    if (!buffer)
    {
        fbe_cli_error("\nData must be read into DDT buffer before a write/write same may be executed.\n");
        return;
      
    }

    /*checking block operation as per switch */
    switch (switch_type)
    {
        case FBE_CLI_DDT_SWITCH_TYPE_WS:
            ddt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME;
            break;
        case FBE_CLI_DDT_SWITCH_TYPE_INVALID:
            ddt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY;
            break;
        default:
            ddt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE;
            break;
    } /* End of switch */

    /* If this is a write_same (with buffer size = 1), then we can ignore
    * the buffer size rule.  Otherwise, the buffer size (fcli_ddt_count)
    * must be at least as big as the write that we want to do (count).
    */
    if (!((ddt_block_op == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME) && (count == 1)) && (count > ddt_block_count))
    {
        fbe_cli_error("\nSaved buffer is too small.\n");
        return;
    }
    
    fbe_cli_ddt_block_operation(ddt_block_op,count,lba,phys_location,switch_type);

     return;

}
/******************************************
 * end fbe_cli_ddt_cmd_write()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_ddt_cmd_display()
 ****************************************************************
 *
 * @brief  this fucntion displays buffer data
 *
 * @param   count - block count
 *
 * @return  None.
 *
 * @revision:
 *
 ****************************************************************/
void fbe_cli_ddt_cmd_display(fbe_u64_t count)
{
    fbe_u64_t         block_count;
    fbe_u32_t         block_size = FBE_BE_BYTES_PER_BLOCK;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_sg_element_t  sg_elements[FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS+1] = {FBE_CLI_DDT_ZERO};
    fbe_sg_element_t  *sg_ptr = NULL;

        
    if (!buffer)
    {
        fbe_cli_error("\nData must be read into DDT buffer before displaying \n");
        return;
      
    }
    block_count = count;
    sg_ptr = sg_elements;

    /* Generate a sg list with block counts.
    *  Fill the sg list passed in.
    *  Fill in the memory portion of this sg list from the
    *  contiguous range passed in.
    */
    status = fbe_data_pattern_sg_fill_with_memory(sg_ptr,
                                buffer,
                                block_count,
                                block_size,
                                FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS,
                                FBE_DATA_PATTERN_MAX_BLOCKS_PER_SG);
    if (status != FBE_STATUS_OK )
    {
        fbe_cli_error("\nSetting up Sg failed.\n");
        return;
    }

    fbe_cli_printf("\nHere's your data:\n");
    fbe_cli_printf("\n      BuffAddr          Sector        "
               "Offset  Data\n");
    fbe_cli_printf("  ----------------  ----------------  "
             "------  -----------------------------------\n");
    fbe_cli_ddt_print_buffer_contents(sg_ptr, 
                                      0,
                                      count, 
                                      FBE_BE_BYTES_PER_BLOCK);
    
    fbe_cli_printf("Data Buffer displays\n");
    return;
}
/******************************************
 * end fbe_cli_ddt_cmd_display()
 ******************************************/
 
/*!**************************************************************
 *          fbe_cli_ddt_cmd_getbuffer()
 ****************************************************************
 *
 * @brief  this fucntion allocate memory to buffer and also sg list
 *
 * @param   count - block count
 *
 * @return  None.
 *
 * @revision:
 *
 ****************************************************************/
void fbe_cli_ddt_cmd_getbuffer(fbe_u64_t count)
{
    fbe_u32_t         block_size = FBE_BE_BYTES_PER_BLOCK;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_sg_element_t  sg_elements[FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS+1] = {FBE_CLI_DDT_ZERO};
    fbe_sg_element_t  *sg_ptr = NULL;

    /* We need to allocate a buffer if one doesn't exist.
     */
    if (buffer)
    {
        /* if buffer already exists, we free it up and reallocate a new one  to read in the fresh data
        */
        fbe_api_free_memory(buffer);
        ddt_block_count = 0;
        buffer = NULL;
    }

    if (buffer == NULL)
    {
        /* Reallocate a new buffer to read in the data*/
        buffer = fbe_api_allocate_memory((fbe_u32_t)count * block_size);
        if (buffer == NULL)
        {
            fbe_cli_error("\nUnable to allocate the memory to a buffer");
            return;
        }
        
        ddt_block_count = count;
    }

    sg_ptr = sg_elements;

    /* Generate a sg list with block counts.
    *  Fill the sg list passed in.
    *  Fill in the memory portion of this sg list from the
    *  contiguous range passed in.
    */
    status = fbe_data_pattern_sg_fill_with_memory(sg_ptr,
                                buffer,
                                ddt_block_count,
                                block_size,
                                FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS,
                                FBE_DATA_PATTERN_MAX_BLOCKS_PER_SG);
    if (status != FBE_STATUS_OK )
    {
        fbe_cli_error("\nSetting up Sg failed.\n");
        return;
    }
    
    fbe_cli_printf("Data Buffer is allocated\n");
    return;
}
/******************************************
 * end fbe_cli_ddt_cmd_getbuffer()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_ddt_cmd_edit()
 ****************************************************************
 *
 * @brief this function read data from drive & write to buffer
 *
 * @param   data          -data(this data is used while 
 *                         editing the buffer)
 *          offset        -offset of buffer
 *          count         -block count
 *
 * @return  None.
 *
 * @revision:
 *
 ****************************************************************/
void fbe_cli_ddt_cmd_edit(fbe_u32_t data,fbe_u32_t offset,fbe_u64_t count)
{
    fbe_u64_t         block_count;
    fbe_u32_t         block_size = FBE_BE_BYTES_PER_BLOCK;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_sg_element_t  sg_elements[FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS+1] = {FBE_CLI_DDT_ZERO};
    fbe_sg_element_t  *sg_ptr = NULL;
    fbe_u8_t          *pdata = NULL;

        
    if (!buffer)
    {
        fbe_cli_error("\nData must be read into DDT buffer before displaying \n");
        return;
      
    }
    block_count = count;
    sg_ptr = sg_elements;

    /* Generate a sg list with block counts.
    *  Fill the sg list passed in.
    *  Fill in the memory portion of this sg list from the
    *  contiguous range passed in.
    */
    status = fbe_data_pattern_sg_fill_with_memory(sg_ptr,
                                buffer,
                                block_count,
                                block_size,
                                FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS,
                                FBE_DATA_PATTERN_MAX_BLOCKS_PER_SG);
    if (status != FBE_STATUS_OK )
    {
        fbe_cli_error("\nUnable to fill data Status:%d.\n",status);
        return;
    }

    pdata = fbe_cli_ddt_get_ptr(offset,sg_ptr);
    if (pdata== NULL)
    {
        fbe_cli_error("\nUnable to edit specified location!!\n");
        return;
    }

    *pdata= (fbe_u8_t) data;

    return;
}
/******************************************
 * end fbe_cli_ddt_cmd_edit()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_ddt_block_operation()
 ****************************************************************
 *
 * @brief This function allocate sg list to buffer and do the 
 *        block operation.
 *
 * @param   ddt_block_op  -block operation.
 *          block_count   -number of blocks
 *          lba           -lba starting point for drive
 *          phys_location -physical location of drive
 *          switch_type -physical location of drive
 *
 * @return  None.
 *
 * @revision:
 *
 ****************************************************************/
void fbe_cli_ddt_block_operation(fbe_payload_block_operation_opcode_t ddt_block_op,
                                        fbe_u64_t block_count,
                                        fbe_lba_t lba,
                                        fbe_job_service_bes_t phys_location,
                                        fbe_cli_ddt_switch_type switch_type)
{
    fbe_u32_t         block_size = FBE_BE_BYTES_PER_BLOCK;
    fbe_u32_t object_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_package_id_t package_id = FBE_PACKAGE_ID_PHYSICAL;
    fbe_sg_element_t  sg_elements[FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS+1] = {FBE_CLI_DDT_ZERO};
    fbe_sg_element_t  *sg_ptr = NULL;
    fbe_payload_block_operation_t   block_operation = {FBE_CLI_DDT_ZERO};
    fbe_block_transport_block_operation_info_t  block_operation_info = {FBE_CLI_DDT_ZERO};
    fbe_raid_verify_error_counts_t verify_error_counts = {FBE_CLI_DDT_ZERO};

    /*checking block operation as per switch */
    switch (type)
    {
        case FBE_CLI_DDT_TYPE_LDO:
        {
            fbe_cli_error("\nlogical drive object is not supported\n");
            return;
            break;
        }
        case FBE_CLI_DDT_TYPE_PDO:
        {
            /* Get the physical drive object id for location.
            */
            status = fbe_api_get_physical_drive_object_id_by_location(phys_location.bus, phys_location.enclosure, phys_location.slot,&object_id);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("\nFailed to get logical drive object id for %d_%d_%d status:0x%x\n",
                    phys_location.bus,phys_location.enclosure,phys_location.slot,status);
                return;
            }
            break;
        }
        default:
        {
            /* Get the pvd object id  for location.
            */
            status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(phys_location.bus, phys_location.enclosure, phys_location.slot,&object_id);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("\nFailed to get provision drive object id for %d_%d_%d status:0x%x\n",
                    phys_location.bus,phys_location.enclosure,phys_location.slot,status);
                return;
            }
            /* Change package id to SEP to support PVD object
             */
            package_id = FBE_PACKAGE_ID_SEP_0;
            break;
        }
    } /* End of switch */

    if(ddt_block_op != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY)
    {
        sg_ptr = sg_elements;

        /* Generate a sg list with block counts.
        *  Fill the sg list passed in.
        *  Fill in the memory portion of this sg list from the
        *  contiguous range passed in.
        */
        status = fbe_data_pattern_sg_fill_with_memory(sg_ptr,
                                    buffer,
                                    block_count,
                                    block_size,
                                    FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS,
                                    FBE_DATA_PATTERN_MAX_BLOCKS_PER_SG);
        if (status != FBE_STATUS_OK )
        {
            fbe_cli_printf("ddt: ERROR: Setting up Sg failed.\n");
            return;
        }
    }
        
    status = fbe_payload_block_build_operation(&block_operation,
                                               ddt_block_op,
                                               lba,
                                               block_count,
                                               block_size,
                                               FBE_CLI_DDT_OPTIMUM_BLOCK_SIZE,
                                               NULL);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nddt: Block operation build failed %d\n", 
                       object_id);
        return;
    }

    /*
    fbe_payload_block_set_verify_error_count_ptr(&block_operation,
                                                 &verify_error_counts);
    */

    block_operation_info.block_operation = block_operation;
    block_operation_info.verify_error_counts = verify_error_counts;

    /* This is wrong API! we should send packet and not bloc operation */
    status = fbe_api_block_transport_send_block_operation(
                                                          object_id,
                                                          package_id,
                                                          &block_operation_info,
                                                          (fbe_sg_element_t*)sg_ptr);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nddt:Block operation failed to object_id: %d pkt_status: 0x%x\n", 
                       object_id, 
                       status);
        return;
    }

    block_operation = block_operation_info.block_operation;

     /*
     * Now display the result
     */
    fbe_cli_ddt_display_result(&block_operation, 
                               (fbe_sg_element_t*)sg_ptr,lba,switch_type);

     return;
}
/******************************************
 * end fbe_cli_ddt_block_operation()
 ******************************************/
 
/*!**************************************************************
 *          fbe_cli_ddt_display_result()
 ****************************************************************
 *
 * @brief This function displays ddt operation result.
 *
 * @param   block_operation_p
 *
 * @return  None.
 *
 * @revision:
 *
 ****************************************************************/
void fbe_cli_ddt_display_result(fbe_payload_block_operation_t *block_operation_p,
                                fbe_sg_element_t *sg_p,fbe_lba_t lba,
                                fbe_cli_ddt_switch_type switch_type)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_block_operation_status_t block_op_status;
    fbe_payload_block_operation_qualifier_t block_op_qualifier;
    fbe_payload_block_operation_opcode_t block_op_opcode;
    
    fbe_cli_printf("\n");
    
    fbe_cli_printf("ddt: The FBE CLI has received a response message "
                   "from the SEP/PP \n");

    block_op_status = block_operation_p->status;
    block_op_qualifier = block_operation_p->status_qualifier;
    block_op_opcode = block_operation_p->block_opcode;

    switch (block_op_status)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS:
        {
            fbe_cli_printf("ddt: The SEP/PP reported that "
                           "the request succeeded!\n");

            if (block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)
            {
                if(switch_type != FBE_CLI_DDT_SWITCH_TYPE_ND)
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
                    fbe_cli_ddt_print_buffer_contents(sg_p, 
                        lba, (fbe_u32_t)block_operation_p->block_count, 
                        FBE_BE_BYTES_PER_BLOCK);
                }

                fbe_cli_printf("Data read into buffer\n");
            }
            break;
        }
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR:
            fbe_cli_printf("ddt: ERROR: %d The SEP/PP reported that "
                "op failed with a media error.\n", block_op_status);
            break;

        default:
        {
            /* To Do  This is not yet complete - will be implemented as part of next checkin*/
            fbe_u8_t status_str[FBE_CLI_DDT_MAX_STRING_SIZE] = "";
            fbe_u8_t qualifier_str[FBE_CLI_DDT_MAX_STRING_SIZE] = "";
            fbe_cli_printf("ddt: ERROR: The SEP/PP reported "
                "a failure.\n");
            status = fbe_api_block_transport_status_to_string(block_op_status,
                status_str,
                FBE_CLI_DDT_MAX_STRING_SIZE);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_error("\nFailed to transport block status to string for block_op_status:0x%x",block_op_status);
                return;
            }
            status = fbe_api_block_transport_qualifier_to_string(block_op_qualifier,
                qualifier_str,
                FBE_CLI_DDT_MAX_STRING_SIZE);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_error("\nFailed to transport block Qualifier to string for block_op_qurlifier:0x%x",block_op_qualifier);
                return;
            }
            break;
        }

    }
}

/******************************************
 * end fbe_cli_ddt_display_result()
 ******************************************/

/****************************************************************
 *  fbe_cli_print_buffer_contents()
 ****************************************************************
 *
 * @brief
 *  Print the contents of the buffer
 *
 * @param sg_ptr       - Buffer which contains the data to be 
 *                       printed.
 * @param disk_address - Starting disk address in sectors.
 * @param blocks       - Blocks to display.
 * @param block_size   - The logical block size for this request
 *
 * @return NONE
 *
 * @revision
 *  
 *
 ****************************************************************/

void fbe_cli_ddt_print_buffer_contents(fbe_sg_element_t *sg_ptr, 
                                 fbe_lba_t disk_address,
                                 fbe_u64_t blocks,
                                 fbe_u32_t block_size)
{
     fbe_u64_t count = FBE_CLI_DDT_ZERO, orig_count = FBE_CLI_DDT_ZERO;
     fbe_char_t *addr;
     fbe_u64_t words;
     fbe_bool_t warning=FALSE;
     fbe_u32_t current_block = FBE_CLI_DDT_ZERO;

     /* Sanity check the block size.
      */
     if ((block_size > FBE_BE_BYTES_PER_BLOCK) 
         || (block_size < FBE_BYTES_PER_BLOCK))
     {
         fbe_cli_error("\n %s: block_size: %d not supported.\n",
                __FUNCTION__, block_size);
         return;
     }

     /* Display the block using ddt display sector.
      */
     while (sg_ptr->count)
     {
         addr = sg_ptr->address;
         count = sg_ptr->count;

         while (count > FBE_CLI_DDT_ZERO)       /* One loop pass per sector */
         {
             if (count < block_size)
             {
                 warning = TRUE;
                 orig_count = count;
                 words = (count + FBE_CLI_DDT_DUMMYVAL) / FBE_CLI_DDT_COLUMN_SIZE; /* round up to a whole word */
             }
             else
             {
                 words = block_size / FBE_CLI_DDT_COLUMN_SIZE;
             }
      
             fbe_cli_ddt_display_sector(disk_address, addr, words);
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
                    " = %016llX\n\n", orig_count);
             }
         
         } /* end while there are more blocks in the sg entry*/
         sg_ptr++;
     }

     return;

} 
/*****************************************
 ** End of fbe_cli_ddt_print_buffer_contents()  **
 *****************************************/


/************************************************************************
 *  fbe_cli_ddt_display_sector()
 ************************************************************************
 *
 * @brief
 *   This function displays ONE SECTOR of data.  
 *
 * @param in_lba  - fru LBA
 * @param in_addr - buffer address
 * @param w_count - Number of 32-bit words to display
 *
 * @return (none)
 *
 * @revision
 * 
 ************************************************************************/

void fbe_cli_ddt_display_sector(fbe_lba_t in_lba, 
                                fbe_u8_t * in_addr, 
                                fbe_u64_t w_count)
{
    fbe_u32_t    *w_ptr;
    fbe_u32_t    offset = FBE_CLI_DDT_ZERO;
    fbe_u64_t    addr, lba;

    w_ptr = (fbe_u32_t *)in_addr;
    addr = (fbe_u64_t)in_addr;
    lba = in_lba;

    /* This loop displays most of the sector (512 bytes or 128 words).
     */
    while (w_count >= FBE_CLI_DDT_COLUMN_SIZE)
    {
        fbe_cli_printf("  %016llX  %016llX  %4X:   %08X %08X %08X %08X \n",
                   (unsigned long long)addr, (unsigned long long)lba,
               offset, *w_ptr, *(w_ptr + FBE_CLI_DDT_W_COUNT_ONE),
               *(w_ptr + FBE_CLI_DDT_W_COUNT_TWO),
               *(w_ptr + FBE_CLI_DDT_W_COUNT_THREE));
        offset += FBE_CLI_DDT_COLUMN_SIZE*FBE_CLI_DDT_COLUMN_SIZE;
        w_ptr += FBE_CLI_DDT_COLUMN_SIZE;
        w_count -= FBE_CLI_DDT_COLUMN_SIZE;
    }

    /* Print the last (normally 2) words of the sector (the overhead bytes)
     * or any excess if we're not a multiple of 520.
     */
    switch (w_count)
    {
        case FBE_CLI_DDT_W_COUNT_NONE :
            fbe_cli_printf(" \n");
            break;
        case FBE_CLI_DDT_W_COUNT_ONE:
            fbe_cli_printf("  %016llX  %016llX  %4X:   %08X \n\n",
                           (unsigned long long)addr, (unsigned long long)lba,
               offset, *w_ptr);
            break;
        case FBE_CLI_DDT_W_COUNT_TWO:
            fbe_cli_printf("  %016llX  %016llX  %4X:   %08X %08X \n\n",
                           (unsigned long long)addr, (unsigned long long)lba,
               offset, *w_ptr, *(w_ptr + FBE_CLI_DDT_W_COUNT_ONE));
            break;
        case FBE_CLI_DDT_W_COUNT_THREE:
            fbe_cli_printf("  %016llX  %016llX  %4X:   %08X %08X %08X \n\n",
                           (unsigned long long)addr, (unsigned long long)lba,
               offset, *w_ptr, *(w_ptr + FBE_CLI_DDT_W_COUNT_ONE),
               *(w_ptr + FBE_CLI_DDT_W_COUNT_TWO));
            break;
        default:
            fbe_cli_printf("\n");
            break;
    } /* End of switch */

    return;
}
/*****************************************
 ** End of fbe_cli_ddt_display_sector()  **
 *****************************************/

/*!**************************************************************
 *          fbe_cli_ddt_get_ptr()
 ****************************************************************
 *
 * @brief This function converts offset to adderss(memory adderss).
 *
 * @param   sg_ptr       - Buffer which contains the data to be 
 *                         printed.
 *          offset       - offset of buffer
 *
 * @return  fbe_u8_t     - 8 bit pointer which pointing provided 
 *                         offset location of buffer.
 *
 * @revision:
 *
 ****************************************************************/
static fbe_u8_t *fbe_cli_ddt_get_ptr( fbe_u32_t offset,fbe_sg_element_t *sg_ptr)
{
   /*
    * fbe_sg_element_t *pSGL will be used to iterate through the sg element list as well as to access
    * the data member of sg element
    */
  
    fbe_sg_element_t *pSGL = sg_ptr;

    if (!pSGL)
    {
        return NULL;
    }
    else
    {
        do
        {
            fbe_u32_t this_cnt;
            fbe_u32_t this_adr;

            this_cnt = pSGL->count;
            this_adr = (fbe_u32_t)pSGL->address;
            pSGL++;

            if ((!this_cnt) || (!this_adr) )
            {
                return NULL;
            }
            
             if (this_cnt > offset)
            {
                fbe_u8_t *p = (fbe_u8_t *)(fbe_ptrhld_t)this_adr;
                return &p[offset];
            }
            else
            {
                offset -= this_cnt;
            }
        }
        while (FBE_TRUE);
    }
    return NULL;
}
/*****************************************
 ** End of fbe_cli_ddt_get_ptr()  **
 *****************************************/
 
 /*!**************************************************************
 *          fbe_cli_ddt_get_edit_cmd()
 ****************************************************************
 *
 * @brief    This function process the edit command.
 *
 * @param   argc        - argument count
 *          argv        - argument string
 *          lba           -lba starting point for drive
 *          switch_type   -contain cmd type for ddt
 *          cmd_type      -contain cmd type for ddt
 *          phys_location -physical location of drive
 *          count         -block count
 *
 * @return  None.
 *
 * @revision:
 *
 ****************************************************************/
void fbe_cli_ddt_get_edit_cmd(int argc,char **argv,
                               fbe_lba_t lba,
                               fbe_cli_ddt_switch_type switch_type,
                               fbe_cli_ddt_cmd_type cmd_type,
                               fbe_job_service_bes_t phys_location,
                               fbe_u64_t count)
{
    fbe_u32_t         data = FBE_CLI_DDT_DATA_INVALID;
    fbe_u32_t         offset = FBE_CLI_DDT_OFFSET_INVALID;

    if (!buffer)
    {
        fbe_cli_error("\n No buffer to edit!\n");
        return;
    }
    if(argc<=0)
    {
        fbe_cli_error("\nOffset information missing!\n");
        fbe_cli_printf("%s", DDT_CMD_USAGE);
        return;
    }
    offset = fbe_atoh(*argv);
    argc--;
    argv++;

    if(argc<=0)
    {
        fbe_cli_error("\nData information missing!\n");
        fbe_cli_printf("%s", DDT_CMD_USAGE);
        return;
    }

    while ( argc >0 )
    {
        data = fbe_atoh(*argv);
        if (data == FBE_CLI_DDT_DATA_BOGUS)
        {
            fbe_cli_error("\nInvalid data byte!!\n");
            return;
        }
        fbe_cli_ddt_process_cmds(lba,switch_type,cmd_type,phys_location,count,data,offset);
        offset++;
        argc--;
        argv++;
    }
    fbe_cli_printf("\nEditing done sucessfully...\n\n");
    return;
}
/*****************************************
** End of fbe_cli_ddt_get_edit_cmd()  **
*****************************************/

/*!**************************************************************
 *          fbe_cli_ddt_get_cmd_attribute()
 ****************************************************************
 *
 * @brief This function get operation attribute like lba,bes,count.
 *
 * @param   argc        - argument count
 *          argv        - argument string
 *          switch_type -contain cmd type for ddt
 *          cmd_type    -contain cmd type for ddt
 *
 * @return  None
 *
 * @revision:
 *
 ****************************************************************/
void fbe_cli_ddt_get_cmd_attribute(int argc,char **argv,
                               fbe_cli_ddt_switch_type switch_type,
                               fbe_cli_ddt_cmd_type cmd_type)
{
    fbe_status_t     status;
    fbe_lba_t         lba = FBE_CLI_DDT_DEFAULT_LBA;
    fbe_job_service_bes_t phys_location;
    fbe_u64_t         count = FBE_CLI_DDT_DEFAULT_COUNT;/* default = 1 sector*/
    fbe_u32_t         data = FBE_CLI_DDT_DATA_INVALID;
    fbe_u32_t         offset = FBE_CLI_DDT_OFFSET_INVALID;
    /* The next argument is the fru number in bes format
    */
    if (argc)
    {
        status = fbe_cli_convert_diskname_to_bed(*argv, &phys_location);
        if (status != FBE_STATUS_OK)
        {
            /* above function displays appropriate error
            */
            return;
        }
        argc--;
        argv++;
    }
    else
    {
        fbe_cli_error("\nb_e_s info Missing\n");
        fbe_cli_printf("%s", DDT_CMD_USAGE);
        return;
    }
    /* The next argument is the LBA value (always interpreted in HEX)
     */
    if (argc)
    {
         status = fbe_atoh64(*argv, &lba);

        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("ddt: ERROR: Invalid LBA format.\n");
            fbe_cli_printf("%s", DDT_CMD_USAGE);
            return;
        }

        argc--;
        argv++;
    }
    else
    {
        fbe_cli_error("\nMissing required LBA.\n");
        fbe_cli_printf("%s", DDT_CMD_USAGE);
        
        /* Set the lba to an invalid value.
         */
        lba = FBE_LBA_INVALID;
        return;
    }

    /* The next argument is the count value (always interpreted in decimal) 
     * for default will take it as 1
     */
    if (argc)
    {
        status = fbe_atoh64(*argv, &count);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("ddt: ERROR: Invalid count format.\n");
            fbe_cli_printf("%s", DDT_CMD_USAGE);
            return;
        }
        argc--;
        argv++;
    }
    if (count==0)
    {
        fbe_cli_error("\ncount can not be 0.\n");
        fbe_cli_printf("%s", DDT_CMD_USAGE);
        return;
    }
    fbe_cli_ddt_process_cmds(lba,switch_type,cmd_type,phys_location,count,data,offset);
    return;
}
/*****************************************
** End of fbe_cli_ddt_get_cmd_attribute()  **
*****************************************/

 /*!**************************************************************
 *          fbe_cli_ddt_get_checksum_cmd()
 ****************************************************************
 *
 * @brief    This function process the get checksum command.
 *
 * @param   argc        - argument count
 *          argv        - argument string
 *          lba           -lba starting point for drive
 *          switch_type   -contain cmd type for ddt
 *          cmd_type      -contain cmd type for ddt
 *          phys_location -physical location of drive
 *          count         -block count
 *
 * @return  None.
 *
 * @revision:
 *
 ****************************************************************/
void fbe_cli_ddt_get_checksum_cmd(int argc,char **argv,
                               fbe_lba_t lba,
                               fbe_cli_ddt_switch_type switch_type,
                               fbe_cli_ddt_cmd_type cmd_type,
                               fbe_job_service_bes_t phys_location,
                               fbe_u64_t count)
{
    fbe_u32_t         data = FBE_CLI_DDT_DATA_INVALID;
    fbe_u32_t         offset = FBE_CLI_DDT_OFFSET_INVALID;

    if (!buffer)
    {
        fbe_cli_error("\n No buffer to calculate checksum!\n");
        return;
    }

    fbe_cli_ddt_process_cmds(lba,switch_type,cmd_type,phys_location,count,data,offset);
    fbe_cli_printf("\nChecksum calculation completed sucessfully...\n\n");
    return;
}
/*****************************************
** End of fbe_cli_ddt_get_checksum_cmd()  **
*****************************************/

/*!**************************************************************
 *          fbe_cli_ddt_cmd_checksum_buffer()
 ****************************************************************
 *
 * @brief this function will calculate the checksum for data buffer   
 *
 * @param   lba           -lba starting point for drive
 *          switch_type   -switch for the further functioning
 *          phys_location -physical location of drive
 *          count         -block count

 *
 * @return  None.
 *
 * @revision:
 *
 ****************************************************************/

void fbe_cli_ddt_cmd_checksum_buffer( fbe_lba_t lba,
                           fbe_cli_ddt_switch_type switch_type,
                           fbe_job_service_bes_t phys_location,
                           fbe_u64_t count)

{
    fbe_u64_t         block_count;
    fbe_u32_t         block_size = FBE_BE_BYTES_PER_BLOCK;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_sg_element_t  sg_elements[FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS+1] = {FBE_CLI_DDT_ZERO};
    fbe_sg_element_t  *sg_ptr = NULL;
    fbe_u8_t          *pdata = NULL;
    fbe_u32_t          oldsum;
    fbe_u32_t          newsum;
    
    if (!buffer)
    {
        fbe_cli_error("\n No buffer to calculate checksum!\n");
        return;
    }

    block_count = count;
    sg_ptr = sg_elements;

    /* Generate a sg list with block counts.
    *  Fill the sg list passed in.
    *  Fill in the memory portion of this sg list from the
    *  contiguous range passed in.
    */
    status = fbe_data_pattern_sg_fill_with_memory(sg_ptr,
                                buffer,
                                block_count,
                                block_size,
                                FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS,
                                FBE_DATA_PATTERN_MAX_BLOCKS_PER_SG);
    if (status != FBE_STATUS_OK )
    {
        fbe_cli_error("\nUnable to fill data Status:%d.\n",status);
        return;
    }

    pdata = fbe_cli_ddt_get_ptr(0x200,sg_ptr);     /* TODO */
    if (pdata== NULL)
    {
        fbe_cli_error("\nUnable to obtain current checksum location in buffer!!\n");
        return;
    }else{
        oldsum = *((fbe_u32_t *)pdata);
    }

    pdata = fbe_cli_ddt_get_ptr(0,sg_ptr);    
    if (pdata== NULL)
    {
        fbe_cli_error("\nUnable to obtain current start location of buffer!!\n");
        return;
    }else{
        newsum = fbe_xor_lib_calculate_checksum((fbe_u32_t *)pdata);        
        fbe_cli_printf("\n Buffer checksum: old=%04X, new=%04X\n",
                        (oldsum & 0x0000FFFF), (newsum & 0x0000FFFF));
    }
    
    return;

}
/******************************************
 * end fbe_cli_ddt_cmd_checksum_buffer()
 ******************************************/


/*!**************************************************************
 *          fbe_cli_ddt_get_invalidate_checksum_cmd()
 ****************************************************************
 *
 * @brief    This function process the edit command.
 *
 * @param   argc        - argument count
 *          argv        - argument string
 *          lba           -lba starting point for drive
 *          switch_type   -contain cmd type for ddt
 *          cmd_type      -contain cmd type for ddt
 *          phys_location -physical location of drive
 *          count         -block count
 *
 * @return  None.
 *
 * @revision:
 *
 ****************************************************************/
void fbe_cli_ddt_get_invalidate_checksum_cmd(int argc,char **argv,
                               fbe_lba_t lba,
                               fbe_cli_ddt_switch_type switch_type,
                               fbe_cli_ddt_cmd_type cmd_type,
                               fbe_job_service_bes_t phys_location,
                               fbe_u64_t count)
{
    fbe_u32_t         data = FBE_CLI_DDT_DATA_INVALID;
    fbe_u32_t         offset = FBE_CLI_DDT_OFFSET_INVALID;
    char *            invalidate_arg = NULL;

    if (!buffer)
    {
        fbe_cli_error("\n No buffer to invalidate checksum!\n");
        return;
    }

    if (argc == 0){
        xor_invalid_reason = XORLIB_SECTOR_INVALID_REASON_UNKNOWN;
    }else{
        if (strncmp(*argv, "dh",2) == 0){
            xor_invalid_reason = XORLIB_SECTOR_INVALID_REASON_DH_INVALIDATED;
        }
        else if (strncmp(*argv, "raid",4) == 0){
            xor_invalid_reason = XORLIB_SECTOR_INVALID_REASON_VERIFY;
        }
        else if (strncmp(*argv, "corrupt",7) == 0){
            xor_invalid_reason = XORLIB_SECTOR_INVALID_REASON_RAID_CORRUPT_CRC;
        }else{
            fbe_cli_error("\n No buffer to invalidate checksum!\n");
        }
        invalidate_arg = *argv;
    }

    fbe_cli_ddt_process_cmds(lba,switch_type,cmd_type,phys_location,count,data,offset);
    fbe_cli_printf("\nChecksum invalidation completed sucessfully... Option: %s\n\n",invalidate_arg);
    return;
}
/*****************************************
** End of fbe_cli_ddt_get_invalidate_checksum_cmd()  **
*****************************************/

/*!**************************************************************
 *          fbe_cli_ddt_cmd_invalidate_checksum() 
 ****************************************************************
 *
 * @brief this function will cause data buffer to update checksum
 *        to invalid value.
 *
 * @param   lba           -lba starting point for drive
 *          switch_type   -switch for the further functioning
 *          phys_location -physical location of drive
 *          count         -block count

 *
 * @return  None.
 *
 * @revision:
 *
 ****************************************************************/

void fbe_cli_ddt_cmd_invalidate_checksum( fbe_lba_t lba,
                           fbe_cli_ddt_switch_type switch_type,
                           fbe_job_service_bes_t phys_location,
                           fbe_u64_t count)

{
    fbe_u64_t         block_count;
    fbe_u32_t         block_size = FBE_BE_BYTES_PER_BLOCK;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_sg_element_t  sg_elements[FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS+1] = {FBE_CLI_DDT_ZERO};
    fbe_sg_element_t  *sg_ptr = NULL;
    
    if (!buffer)
    {
        fbe_cli_error("\n No buffer to invalidate checksum!\n");
        return;
    }

    block_count = count;
    sg_ptr = sg_elements;

    /* Generate a sg list with block counts.
    *  Fill the sg list passed in.
    *  Fill in the memory portion of this sg list from the
    *  contiguous range passed in.
    */
    status = fbe_data_pattern_sg_fill_with_memory(sg_ptr,
                                buffer,
                                block_count,
                                block_size,
                                FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS,
                                FBE_DATA_PATTERN_MAX_BLOCKS_PER_SG);
    if (status != FBE_STATUS_OK )
    {
        fbe_cli_error("\nUnable to fill data Status:%d.\n",status);
        return;
    }

    status = fbe_xor_lib_fill_invalid_sectors(sg_ptr, 0, block_count, 0, xor_invalid_reason, XORLIB_SECTOR_INVALID_WHO_CLIENT);
    if (status != FBE_STATUS_OK )
    {
        fbe_cli_error("\nError invalidating buffer.Status 0x%X!!\n", status);
        return;
    }
    
    return;

}
/******************************************
 * end fbe_cli_ddt_cmd_invalidate_checksum()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_ddt_cmd_kill_drive() 
 ****************************************************************
 *
 * @brief this function will cause the drive to transition to failed state  
 *
 * @param   lba           - should be set to 0xDEAD
 *          switch_type   -switch for the further functioning
 *          phys_location -physical location of drive *
 * @return  None.
 *
 * @revision:
 *
 ****************************************************************/

void fbe_cli_ddt_cmd_kill_drive( fbe_lba_t lba,
                           fbe_cli_ddt_switch_type switch_type,
                           fbe_job_service_bes_t phys_location)

{
    fbe_object_id_t     object_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t        status = FBE_STATUS_OK;

    if (lba != 0xDEAD){
        /* command format: 'ddt k <frunum> dead' */
        fbe_cli_error("\nError - Required option 0xDEAD not specified!!\n");
        return;
    }
    
    status = fbe_api_get_physical_drive_object_id_by_location(phys_location.bus, 
                                                              phys_location.enclosure, 
                                                              phys_location.slot, 
                                                              &object_id);    
    if ((status != FBE_STATUS_OK ) || (object_id == FBE_OBJECT_ID_INVALID))
    {
        fbe_cli_error("\nError looking up PDO object.Status 0x%X Object ID: 0x%X!!\n", status, object_id);
        return;
    }
    status = fbe_api_set_object_to_state(object_id, FBE_LIFECYCLE_STATE_FAIL, FBE_PACKAGE_ID_PHYSICAL);
    if (status == FBE_STATUS_OK ){
        fbe_cli_printf("\n Set PDO 0x%X object state to failed.\n", object_id);
    }else{
        fbe_cli_error("\nError setting object state to Failed.Status 0x%X Object ID: 0x%X!!\n", status, object_id);
        return;
    }
#if 0
    status = fbe_api_physical_drive_fail_drive(object_id,FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_SPECIALIZED_TIMER_EXPIRED);
    if (status != FBE_STATUS_OK ){
        fbe_cli_error("\nError setting object state to Failed.Status 0x%X Object ID: 0x%X!!\n", status, object_id);
        return;
    }
#endif
    return;

}
/******************************************
 * end fbe_cli_ddt_cmd_kill_drive()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_ddt_cmd_set_hibernate_state() 
 ****************************************************************
 *
 * @brief this function will cause the drive to  
 *        transition in or out of hibernate state.
 *
 * @param   lba           - Indicates new state - 1 - hibernate
                                                  0 - exit hibernate
 *          switch_type   -switch for the further functioning
 *          phys_location -physical location of drive *
 * @return  None.
 *
 * @revision:
 *
 ****************************************************************/

void fbe_cli_ddt_cmd_set_hibernate_state( fbe_lba_t lba,
                           fbe_cli_ddt_switch_type switch_type,
                           fbe_job_service_bes_t phys_location)

{
    fbe_object_id_t     object_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t        status = FBE_STATUS_OK;

    status = fbe_api_get_physical_drive_object_id_by_location(phys_location.bus, 
                                                              phys_location.enclosure, 
                                                              phys_location.slot, 
                                                              &object_id);    
    if ((status != FBE_STATUS_OK ) || (object_id == FBE_OBJECT_ID_INVALID))
    {
        fbe_cli_error("\nError looking up PDO object.Status 0x%X Object ID: 0x%X!!\n", status, object_id);
        return;
    }

#if 1
    fbe_cli_printf("\nDDT option deprecated..\n");
    fbe_cli_printf("\nPlease use green or gr command option to enable or change power save policy and state.\n");
#else
    if (lba == 1){
        status = fbe_api_enable_system_power_save();
        if (status != FBE_STATUS_OK ){
            fbe_cli_error("\nError setting object state to Hibernate.Status 0x%X Object ID: 0x%X!!\n", status, object_id);
            return;
        }    

        status = fbe_api_set_object_to_state(object_id, FBE_LIFECYCLE_STATE_HIBERNATE, FBE_PACKAGE_ID_PHYSICAL);
        if (status != FBE_STATUS_OK ){
            fbe_cli_error("\nError setting object state to Hibernate.Status 0x%X Object ID: 0x%X!!\n", status, object_id);
            return;
        }else{
            fbe_cli_printf("\n Requested PDO 0x%X transition to Hibernate. \n",object_id);
        }
    }
    else if (lba == 0){
        type = FBE_CLI_DDT_TYPE_PDO;
        /* Send a READ request to transition the drive out of Hibernate. */
        fbe_cli_ddt_block_operation(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ, 0, 0, phys_location, FBE_CLI_DDT_SWITCH_TYPE_FUA);
        fbe_cli_printf("\nIssued IO request to transition drive out of Hibernate state.\n");

    }
    else{
        fbe_cli_error("\n Invalid option specified.\n");
        return;
    }
#endif

    return;

}
/******************************************
 * end fbe_cli_ddt_cmd_set_hibernate_state()
 ******************************************/

/*************************
 * end file fbe_cli_lib_ddt_cmds.c
 *************************/
