/***************************************************************************
 * Copyright (C) EMC Corporation 2011 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
* @file fbe_cli_lib_wisd_cmds.c
 ***************************************************************************
 *
 * @brief
*  This file contains cli functions for the WISD related features in FBE CLI.
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
#include "fbe_cli_wisd.h"
#ifdef FBE_KH_MISC_DEPEN
#include "fbe/fbe_api_logical_drive_interface.h"
#else
#include "fbe/fbe_api_physical_drive_interface.h"
#endif
#include "fbe/fbe_data_pattern.h"

#include "fbe_xor_api.h"
#include "fbe/fbe_random.h"
#include "fbe/fbe_api_resume_prom_interface.h"
#include "fbe/fbe_resume_prom_info.h"
#include "fbe/fbe_api_enclosure_interface.h"

#include "fbe_fru_signature.h"

typedef struct fbe_cli_wisd_sys_drive_info_s
{
    fbe_job_service_bes_t               drive_location_now;
    fbe_job_service_bes_t               drive_location_previous;
    fbe_u64_t                                       current_array_wwn_seed;
    fbe_u64_t                                       wwn_seed_in_fru_signature;
}
fbe_cli_wisd_sys_drive_info_t;

/* In Rockies we have 4 system drives in one array */
fbe_cli_wisd_sys_drive_info_t current_system_drive_info[4];
fbe_u32_t index_of_current_system_drive_info = 0;

/* Blindly preserve 100 entries for foreign system drives, 
   * it is almost impossible to have 100 foreign system drives in one array.
   * So, I think the preserved space is enough.
   */
fbe_cli_wisd_sys_drive_info_t foreign_system_drive_info[100];
fbe_u32_t index_of_foreign_system_drive_info = 0;

/*!*******************************************************************
 *          fbe_cli_wisd
 *********************************************************************
 * @brief Function to implement wisd commands execution.
 *        fbe_cli_wisd executes a PP Tester or EE operation.  
 *        This command is for simulation/lab debug and for EE guys to find system drives in customer array.
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
void fbe_cli_wisd(int argc, char** argv)
{

    if (argc == 0)
    {
        fbe_cli_error("\n Too few args.\n");
        fbe_cli_printf("%s", WISD_CMD_USAGE);
        return;
    }
    if ((strncmp(*argv, "-help",5) == 0) ||
        (strncmp(*argv, "-h",2) == 0) )
    {
        /* If they are asking for help, just display help and exit.
        */
        fbe_cli_printf("%s", WISD_CMD_USAGE);
        return;
    }
    if (**argv == 'f')
    {
        fbe_cli_wisd_process_cmds();
    }
    else
    {
        fbe_cli_error("\nInvalid command line option.\n");
        fbe_cli_printf("%s", WISD_CMD_USAGE);
        return;
    }
    return;
}
/******************************************
 * end fbe_cli_wisd()
 ******************************************/

fbe_status_t fbe_cli_wisd_process_cmds(void)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                        chassis_wwn_seed = 0;
    fbe_u32_t                        ldo_count = 0;
    fbe_object_id_t *         ldo_object_array = NULL;
    fbe_u32_t                        index = 0;
    fbe_drive_phy_info_t physical_drive_info;
    fbe_job_service_bes_t drive_location;
    void *                                 read_buffer_one_block_520 = NULL;
    fbe_fru_signature_t *  fru_signature_p = NULL;
    fbe_lifecycle_state_t    lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;

    /* Initialize those two global index */
    index_of_current_system_drive_info = 0;
    index_of_foreign_system_drive_info = 0;

    /* Zero those two global buffer */
    fbe_zero_memory(current_system_drive_info, sizeof(fbe_cli_wisd_sys_drive_info_t)*4);
    fbe_zero_memory(foreign_system_drive_info, sizeof(fbe_cli_wisd_sys_drive_info_t)*100);
    
    read_buffer_one_block_520 = malloc(FBE_BE_BYTES_PER_BLOCK);
    if (read_buffer_one_block_520 == NULL)
    {
        fbe_cli_printf("Failed to allocate memory for one_block_buffer. \n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get wwn_seed from chassis PROM resume */
    status = fbe_cli_wisd_get_wwn_seed_from_prom_resume(&chassis_wwn_seed);
    if (status != FBE_STATUS_OK) {
        free(read_buffer_one_block_520);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the number of drives in current array */
#ifdef FBE_KH_MISC_DEPEN
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_LOGICAL_DRIVE, FBE_PACKAGE_ID_PHYSICAL, &ldo_count );
#else
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST, FBE_PACKAGE_ID_PHYSICAL, &ldo_count );
#endif
    if (status != FBE_STATUS_OK) {
        fbe_cli_printf("Failed to get total LDOs in the system. \n");
        free(read_buffer_one_block_520);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if (ldo_count == 0)
    {
        fbe_cli_printf("No LDO found in current array! \n");
        free(read_buffer_one_block_520);
        return status;
    }
    else
    {
        ldo_count = 0;
#ifdef FBE_KH_MISC_DEPEN
        status = fbe_api_enumerate_class(FBE_CLASS_ID_LOGICAL_DRIVE, FBE_PACKAGE_ID_PHYSICAL, &ldo_object_array, &ldo_count);
#else
        status = fbe_api_enumerate_class(FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST, FBE_PACKAGE_ID_PHYSICAL, &ldo_object_array, &ldo_count);
#endif
        if (status != FBE_STATUS_OK) {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to get a list of LDOs. \n", __FUNCTION__);
            fbe_api_free_memory(ldo_object_array);
            free(read_buffer_one_block_520);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Go through all drives, get ldo_info and read fru_signature from it*/
        for (index = 0; index < ldo_count; index++)
        {
            fbe_zero_memory(&physical_drive_info, sizeof(physical_drive_info));
            fbe_zero_memory(read_buffer_one_block_520, FBE_BE_BYTES_PER_BLOCK);
            
            status = fbe_api_get_object_lifecycle_state(ldo_object_array[index], &lifecycle_state, FBE_PACKAGE_ID_PHYSICAL);
            if (status != FBE_STATUS_OK) 
            {
                fbe_cli_printf("\nFailed to get ldo state, object_id: 0x%x. Wisd would bypass it this time. \nPlease rerun wisd after checking the state of ldo with 'FBE_CLI> ldo' command.\n", ldo_object_array[index]);
                continue;
            }else {
                if (lifecycle_state != FBE_LIFECYCLE_STATE_READY)
                {
                fbe_cli_printf("\nThe ldo with object_id: 0x%x is NOT ready. Wisd would bypass it this time. \nPlease rerun wisd after checking the state of ldo with 'FBE_CLI> ldo' command.\n", ldo_object_array[index]);
                continue;
            }
            }
            
            /* Get ldo info, which records the drive's location for now */
#ifdef FBE_KH_MISC_DEPEN
            status = fbe_api_logical_drive_get_phy_info(ldo_object_array[index], &physical_drive_info);
#else
            status = fbe_api_physical_drive_get_phy_info(ldo_object_array[index], &physical_drive_info);
#endif
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("Fail to get ldo info whose object_id: 0x%x.\n", ldo_object_array[index]);
                continue;
            }
            
            /* Read fru_signature from drive, which records the drive's original location  */
            drive_location.bus = physical_drive_info.port_number;
            drive_location.enclosure = physical_drive_info.enclosure_number;
            drive_location.slot = physical_drive_info.slot_number;
            
            status = fbe_cli_wisd_block_operation(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                                                                          read_buffer_one_block_520,
                                                                                          1,
                                                                                          0x7000,
                                                                                          drive_location);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("Fail to get fru_signature from ldo object_id: 0x%x.\n", ldo_object_array[index]);
                continue;
            }
            
            fru_signature_p = (fbe_fru_signature_t *)read_buffer_one_block_520;

            /* Validate the fru_signature via magic_string */
            if (strcmp(fru_signature_p->magic_string, FBE_FRU_SIGNATURE_MAGIC_STRING))
            {
                /* It is a new drive */
                continue;
            }
            
            /* Find system drives, by location and wwn_seed */
            if (fru_signature_p->bus != 0 || fru_signature_p->enclosure !=0 || fru_signature_p->slot > 3)
            {
                /* It is not system drive */
                continue;
            }

            if (fru_signature_p->system_wwn_seed != chassis_wwn_seed)
            {
                /* It is not current array drive, but it is a system drive.
                  * So it is a foreign system drive
                  */
                foreign_system_drive_info[index_of_foreign_system_drive_info].current_array_wwn_seed = chassis_wwn_seed;
                foreign_system_drive_info[index_of_foreign_system_drive_info].wwn_seed_in_fru_signature = fru_signature_p->system_wwn_seed;
                
                foreign_system_drive_info[index_of_foreign_system_drive_info].drive_location_now.bus = physical_drive_info.port_number;
                foreign_system_drive_info[index_of_foreign_system_drive_info].drive_location_now.enclosure = physical_drive_info.enclosure_number;
                foreign_system_drive_info[index_of_foreign_system_drive_info].drive_location_now.slot = physical_drive_info.slot_number;
                
                foreign_system_drive_info[index_of_foreign_system_drive_info].drive_location_previous.bus = fru_signature_p->bus;
                foreign_system_drive_info[index_of_foreign_system_drive_info].drive_location_previous.enclosure = fru_signature_p->enclosure;
                foreign_system_drive_info[index_of_foreign_system_drive_info].drive_location_previous.slot = fru_signature_p->slot;

                index_of_foreign_system_drive_info++;
                continue;
            }

            /* The drive gets here is current array system drive 
              * Check what is the location it should be in and what 
              * is the location it is in.
              */

            /* Record the system drives information in to local variables */
            if (index_of_current_system_drive_info > 3)
            {
                /* Some stupid thing happened */
                fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, find more than 4 system drives in array, it is very ODD. \n", __FUNCTION__);
                fbe_cli_printf("Find more than 4 system drives in array, it is very ODD.\n");
                fbe_api_free_memory(ldo_object_array);
                free(read_buffer_one_block_520);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            current_system_drive_info[index_of_current_system_drive_info].current_array_wwn_seed = chassis_wwn_seed;
            current_system_drive_info[index_of_current_system_drive_info].wwn_seed_in_fru_signature = fru_signature_p->system_wwn_seed;
                
            current_system_drive_info[index_of_current_system_drive_info].drive_location_now.bus = physical_drive_info.port_number;
            current_system_drive_info[index_of_current_system_drive_info].drive_location_now.enclosure = physical_drive_info.enclosure_number;
            current_system_drive_info[index_of_current_system_drive_info].drive_location_now.slot = physical_drive_info.slot_number;
              
            current_system_drive_info[index_of_current_system_drive_info].drive_location_previous.bus = fru_signature_p->bus;
            current_system_drive_info[index_of_current_system_drive_info].drive_location_previous.enclosure = fru_signature_p->enclosure;
            current_system_drive_info[index_of_current_system_drive_info].drive_location_previous.slot = fru_signature_p->slot;

            index_of_current_system_drive_info++;
            
        }
        
        fbe_api_free_memory(ldo_object_array);
    }

    /* Output information to user */
    fbe_cli_wisd_output_info_to_user();
    free(read_buffer_one_block_520);
 
    return status;
    
}

void fbe_cli_wisd_output_info_to_user(void)
{
    fbe_u32_t index = 0;
    fbe_bool_t drive000_missing = FBE_TRUE;
    fbe_bool_t drive001_missing = FBE_TRUE;
    fbe_bool_t drive002_missing = FBE_TRUE;
    fbe_bool_t drive003_missing = FBE_TRUE;
    
    if (current_system_drive_info[0].current_array_wwn_seed == 0 && index_of_current_system_drive_info == 0)
    {
        /* Means no entry initialized */
        fbe_api_trace(FBE_TRACE_LEVEL_INFO, "\n%s, No system drive in array, it is very BAD. \n", __FUNCTION__);
        fbe_cli_printf("\nNo system drive in array, it is very BAD.\n");        
    }
    else /* We found some system drives, output them info */
    {
        fbe_cli_printf("\nCurrent array system drive info as below:\n");        

        for (index = 0; index < index_of_current_system_drive_info; index++)
        {
            if (0 == current_system_drive_info[index].drive_location_previous.slot)
                drive000_missing = FBE_FALSE;
            else if (1 == current_system_drive_info[index].drive_location_previous.slot)
                drive001_missing = FBE_FALSE;
            else if (2 == current_system_drive_info[index].drive_location_previous.slot)
                drive002_missing = FBE_FALSE;
            else if (3 == current_system_drive_info[index].drive_location_previous.slot)
                drive003_missing = FBE_FALSE;
                        
            if (current_system_drive_info[index].drive_location_now.bus == current_system_drive_info[index].drive_location_previous.bus &&
                 current_system_drive_info[index].drive_location_now.enclosure == current_system_drive_info[index].drive_location_previous.enclosure &&
                 current_system_drive_info[index].drive_location_now.slot == current_system_drive_info[index].drive_location_previous.slot)
            {
                fbe_cli_printf("  The drive in %d_%d_%d is a system drive in the correct location.\n", 
                                            current_system_drive_info[index].drive_location_now.bus,
                                            current_system_drive_info[index].drive_location_now.enclosure,
                                            current_system_drive_info[index].drive_location_now.slot);
            }
            else
                fbe_cli_printf("  The drive in %d_%d_%d is a system drive; however, it is in WRONG location. It should be in %d_%d_%d. Please move it back.\n",
                                            current_system_drive_info[index].drive_location_now.bus,
                                            current_system_drive_info[index].drive_location_now.enclosure,
                                            current_system_drive_info[index].drive_location_now.slot,
                                            current_system_drive_info[index].drive_location_previous.bus,
                                            current_system_drive_info[index].drive_location_previous.enclosure,
                                            current_system_drive_info[index].drive_location_previous.slot);
        }
    }

    /* Print information about missing system drive */
    if (drive000_missing)
        fbe_cli_printf("  The system drive which was in 0_0_0 is missing.\n");
    if (drive001_missing)
        fbe_cli_printf("  The system drive which was in 0_0_1 is missing.\n");
    if (drive002_missing)
        fbe_cli_printf("  The system drive which was in 0_0_2 is missing.\n");
    if (drive003_missing)
        fbe_cli_printf("  The system drive which was in 0_0_3 is missing.\n");

    
    if (foreign_system_drive_info[0].wwn_seed_in_fru_signature== 0)
    {
        /* Means no entry initialized */
        fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s, No foreign system drive in array, it is very Good. \n", __FUNCTION__);
        //fbe_cli_printf("\nNo foreign system drive in array, it is very Good. \n");        

        return;
    }
    else /* We found some foreign system drives, output them info */
    {
        fbe_cli_printf("\nWe found foreign system drive in array, they are:\n");        

        for (index = 0; index < index_of_foreign_system_drive_info; index++)
        {
            fbe_cli_printf("  The drive in %d_%d_%d is foreign system drive, it should be moved back to array with wwn_seed: 0x%llx and it should be in %d_%d_%d of it. Please move it back.\n",
                                        foreign_system_drive_info[index].drive_location_now.bus,
                                        foreign_system_drive_info[index].drive_location_now.enclosure,
                                        foreign_system_drive_info[index].drive_location_now.slot,
                                        (unsigned long long)foreign_system_drive_info[index].wwn_seed_in_fru_signature,
                                        foreign_system_drive_info[index].drive_location_previous.bus,
                                        foreign_system_drive_info[index].drive_location_previous.enclosure,
                                        foreign_system_drive_info[index].drive_location_previous.slot);
        }
    }

}

/*!***************************************************************
 * @fn  fbe_cli_wisd_get_wwn_seed_from_prom_resume
 ****************************************************************
 * @brief:
 *  Function to read the wwn_seed from resume prom.
 *
 * @return 
 *  FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *   12/25/2013:  Created. Jian
 *
 ****************************************************************/
fbe_status_t fbe_cli_wisd_get_wwn_seed_from_prom_resume(fbe_u32_t * wwn_seed)
{
    fbe_u8_t            *read_buf_p;
    fbe_u32_t           object_handle;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_resume_prom_get_resume_async_t      *pGetResumeProm = NULL;
    RESUME_PROM_STRUCTURE            * pResumeData = NULL;

    pGetResumeProm = (fbe_resume_prom_get_resume_async_t *) malloc (sizeof (fbe_resume_prom_get_resume_async_t));
    if(pGetResumeProm == NULL)
    {
        fbe_cli_printf("Not enough memory to allocate for pGetResumeProm \n");
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Allocate the read & write buffer */
    read_buf_p = (fbe_u8_t *)malloc(sizeof(RESUME_PROM_STRUCTURE));
    if (read_buf_p == NULL)
    {
        free(pGetResumeProm); /* Free the pGetResumeProm buffer if read buffer is NULL */
        fbe_cli_printf("Not enough memory to allocate for read_buf_p \n");
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_zero_memory(read_buf_p, sizeof(RESUME_PROM_STRUCTURE));
    
    pGetResumeProm->resumeReadCmd.deviceType = FBE_DEVICE_TYPE_ENCLOSURE;      // Input
    pGetResumeProm->resumeReadCmd.deviceLocation.sp = SP_A;
    pGetResumeProm->resumeReadCmd.deviceLocation.enclosure = 0;
    pGetResumeProm->resumeReadCmd.deviceLocation.port = 0;
    pGetResumeProm->resumeReadCmd.deviceLocation.slot = 0;
    pGetResumeProm->resumeReadCmd.deviceLocation.bus = 0;
    pGetResumeProm->resumeReadCmd.resumePromField = RESUME_PROM_ALL_FIELDS;
    pGetResumeProm->resumeReadCmd.offset = 0;
    pGetResumeProm->resumeReadCmd.pBuffer = read_buf_p;
    pGetResumeProm->resumeReadCmd.bufferSize = sizeof(RESUME_PROM_STRUCTURE);

    status = fbe_api_get_enclosure_object_id_by_location(0, 0, &object_handle);

    if (status != FBE_STATUS_OK ||object_handle == FBE_OBJECT_ID_INVALID)
    {
        fbe_cli_printf("Fail to get board object id. \n");
        free(pGetResumeProm);
        free(read_buf_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* fbe_cli_printf("\nSending asynchronous Resume Read, board object ID: %d. \n", object_handle);*/

    /* Get the Resume Info - ASynchronous */
    status = fbe_api_resume_prom_read_sync(object_handle, &pGetResumeProm->resumeReadCmd);     
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Fail to read chassis wwn_seed. \n");
        fbe_cli_printf("Status: %d\n", status);
        free(pGetResumeProm);
        free(read_buf_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    pResumeData = (RESUME_PROM_STRUCTURE *)read_buf_p;
    *wwn_seed = pResumeData->wwn_seed;
    fbe_cli_printf("Current array chassis wwn_seed, 0x%x\n", *wwn_seed);

    free(read_buf_p);
    free(pGetResumeProm);
    return FBE_STATUS_OK;
}


/*!**************************************************************
 *          fbe_cli_wisd_block_operation()
 ****************************************************************
 *
 * @brief This function allocate sg list to buffer and do the 
 *        block operation.
 *
 * @param   wisd_block_op  -block operation.
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
fbe_status_t fbe_cli_wisd_block_operation(fbe_payload_block_operation_opcode_t wisd_block_op,
                                        void* one_block_buffer,
                                        fbe_u64_t block_count,
                                        fbe_lba_t lba,
                                        fbe_job_service_bes_t phys_location)
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
    fbe_payload_block_operation_qualifier_t qualifier_status;
    fbe_u32_t retry_count;

    /* Set retry_count = 5 */
    retry_count = 5;


    /* Get the logical drive object id for location. */
#ifdef FBE_KH_MISC_DEPEN
    status = fbe_api_get_logical_drive_object_id_by_location(phys_location.bus, phys_location.enclosure, phys_location.slot,&object_id);
#else
    status = fbe_api_get_physical_drive_object_id_by_location(phys_location.bus, phys_location.enclosure, phys_location.slot,&object_id);
#endif
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error(" Failed to get logical drive object id for %d_%d_%d status:0x%x\n",
                                    phys_location.bus,phys_location.enclosure,phys_location.slot,status);
        return status;
    }
    
    if(wisd_block_op != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY)
    {
        sg_ptr = sg_elements;

        /* Generate a sg list with block counts.
        *  Fill the sg list passed in.
        *  Fill in the memory portion of this sg list from the
        *  contiguous range passed in.
        */
        status = fbe_data_pattern_sg_fill_with_memory(sg_ptr,
                                    (fbe_u8_t*)one_block_buffer,
                                    block_count,
                                    block_size,
                                    FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS,
                                    FBE_DATA_PATTERN_MAX_BLOCKS_PER_SG);
        if (status != FBE_STATUS_OK )
        {
            fbe_cli_printf(" wisd: ERROR: Setting up Sg failed.\n");
            return status;
        }
    }
        
    status = fbe_payload_block_build_operation(&block_operation,
                                               wisd_block_op,
                                               lba,
                                               block_count,
                                               block_size,
                                               FBE_CLI_DDT_OPTIMUM_BLOCK_SIZE,
                                               NULL);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error(" wisd: Block operation build failed 0x%x.\n", 
                       object_id);
        return status;
    }

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
       /* Get block operation qualifier */
        qualifier_status = block_operation_info.block_operation.status_qualifier;
        while ((qualifier_status == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE) && retry_count > 0) 
        {
            /* make some delay here */
            fbe_api_sleep(200);
            /* Retry the block operation */
            status = fbe_api_block_transport_send_block_operation(
                                                          object_id,
                                                          package_id,
                                                          &block_operation_info,
                                                          (fbe_sg_element_t*)sg_ptr);
            if (status == FBE_STATUS_OK)
                break;
            else
                qualifier_status = block_operation_info.block_operation.status_qualifier;
            
            retry_count--;
        }
        
        /* if we get status != OK here, it shows we used up all retry chance, and still get block operation failure */
        if (status != FBE_STATUS_OK) {
        fbe_cli_error(" wisd:Block operation failed; object_id: 0x%x, pkt_status: 0x%x, block_operation_status: 0x%x, status_qualifier: 0x%x\n", 
                       object_id, 
                       status,
                       block_operation_info.block_operation.status,
                       block_operation_info.block_operation.status_qualifier);
        return status;
        }
    }

    block_operation = block_operation_info.block_operation;

     return status;
}
/******************************************
 * end fbe_cli_wisd_block_operation()
 ******************************************/


/*************************
 * end file fbe_cli_lib_ddt_cmds.c
 *************************/

