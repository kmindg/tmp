/***************************************************************************
 * Copyright (C) EMC Corporation 2015 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
* @file fbe_cli_lib_replace_chassis_cmds.c
 ***************************************************************************
 *
 * @brief
*  This file contains cli functions for the Replace Chassis related features in FBE CLI.
 *
 * @ingroup fbe_cli
 *
 * @version 1.0
 * @basic logic as following:
 read fru_descriptor from system db drives, and count the invalid disk num according to the fru_descriptor got.
 if (invalid_system_disk_num >=2)
             reject to update chassis wwn_seed.
 if (invalid_system_disk_num==1)
            only invalid disk is NEW_DISK or NO_DISK ,the other disks wwn_seeds are consistent, then allow to update chassis wwn_seed with the command >rc -update_wwn_seed -force
            for other cases, reject to update chassis wwn_seed.
 if( invalid_system_disk_num ==0)
            only if wwn_seed is consistent between system disks and user disks, then allow to update chassis wwn_seed with command > rc -update_wwn_seed
            for other cases, reject to update chassis wwn_seed.
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
#include "fbe_cli_wisd.h"
#include "fbe_cli_replace_chassis.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_data_pattern.h"

#include "fbe_xor_api.h"
#include "fbe/fbe_random.h"
#include "fbe/fbe_api_resume_prom_interface.h"
#include "fbe/fbe_resume_prom_info.h"
#include "fbe/fbe_api_enclosure_interface.h"

#include "fbe_fru_signature.h"
#include "fbe_private_space_layout.h"




/*!*******************************************************************
 *         fbe_cli_replace_chassis
 *********************************************************************
 * @brief Function to implement rc commands execution.
 *        fbe_cli_replace_chassis executes a PP Tester or EE operation.  
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
void fbe_cli_replace_chassis(int argc, char** argv)
{
    fbe_u32_t                        wwn_seed = 0;
    fbe_u32_t                        chassis_wwn_seed = 0;
    fbe_u32_t                        UserDiskSampleCount = 0;
    fbe_u8_t                         retry_count = 0;
    fbe_u8_t                         *wwn_seed_str = NULL;
    fbe_status_t                     status;
    fbe_u64_t                        wwn_user = 0;
    homewrecker_system_disk_info_t 	 system_db_drive_table[3] = {0};
    if (argc == 0)
    {
        fbe_cli_error("\n Too few args.\n");
        fbe_cli_printf("%s", RC_CMD_USAGE);
        return;
    }
    if ((strncmp(*argv, "-help",5) == 0) ||
        (strncmp(*argv, "-h",2) == 0) )
    {
        /* If they are asking for help, just display help and exit.
        */
        fbe_cli_printf("%s", RC_CMD_USAGE);
        return;
    }
    while(argc>0)
    {
      if(strcmp(*argv,"-get_system_drive_status") == 0)
      {    
           fbe_cli_printf("\n"); 
           status =  fbe_cli_wisd_get_wwn_seed_from_prom_resume(&chassis_wwn_seed);
           while((status != FBE_STATUS_OK) && (retry_count < FBE_CLI_FAILED_IO_RETRIES))
           {
               /* Wait 1 second */
               fbe_thread_delay(FBE_CLI_FAILED_IO_RETRY_INTERVAL);
               retry_count++;
               fbe_cli_printf(" try again to get chassis wwn_seed!\n");
               status =  fbe_cli_wisd_get_wwn_seed_from_prom_resume(&chassis_wwn_seed);
           }
           status = fbe_cli_get_system_drive_status(system_db_drive_table);
		   if(status != FBE_STATUS_OK)
	       {
			    fbe_cli_printf(" get_system_drive_status fail!\n");
                return;
		   }
           fbe_cli_display_std_info(system_db_drive_table);
           status = fbe_cli_get_wwn_seed_from_userdisk(&wwn_user,&UserDiskSampleCount);
           if( status != FBE_STATUS_OK)
           {
                fbe_cli_printf(" most user disks wwnseed are not consistent ,can't get wwn_user!\n");
           }
           if(UserDiskSampleCount == 0)
           {
                fbe_cli_printf(" valid user disks samplecount equals 0.\n");
           }
           else
           {
                fbe_cli_printf("most user disks wwnseed are consistent,wwn_user = 0x%llx .\n ",wwn_user);
           }
		   return;
      }
	  else if(strcmp(*argv,"-update_wwn_seed") == 0)
	  {
           argc--;
		   argv++;
		   if(argc == 0)
		   {
			  status = fbe_cli_replace_chassis_process_cmds(FBE_FALSE);
              if(status != FBE_STATUS_OK)
              {
                  fbe_cli_printf("update Chassis wwn_seed Fail! \n");
                  fbe_cli_printf("Please use 'FBE_CLI>rc -get_system_drive_status ' command to check system info.\n");
              }

		   }
		   else if( strcmp(*argv,"-force") == 0)
		   {
		       status = fbe_cli_replace_chassis_process_cmds(FBE_TRUE); 
               if(status != FBE_STATUS_OK)
               {
                   fbe_cli_printf("update Chassis wwn_seed Fail! \n");
                   fbe_cli_printf("Please use 'FBE_CLI>rc -get_system_drive_status ' command to check system info.\n");
               }

		   }
		   else 
		   {
		       fbe_cli_error("invalid command operation after update_wwn_seed.\n");
			   fbe_cli_printf("%s", RC_CMD_USAGE);
		   }

		   return;
	   }
	   else if(strcmp(*argv,"-force_wwn_seed") == 0)
	   {
		   argc--;
		   argv++;
		   if(argc == 0)
		   {
			  fbe_cli_error("Please specify wwn_seed_value.\n");
			  fbe_cli_printf("%s", RC_CMD_USAGE);
              return;
		   }
		   else
		   {
			   wwn_seed_str=*argv;
			   if ((*wwn_seed_str == '0') && (*(wwn_seed_str + 1) == 'x' || *(wwn_seed_str + 1) == 'X'))
			   {
			        wwn_seed_str += 2;
			   }
			   wwn_seed = fbe_atoh(wwn_seed_str);
				 
			   if(wwn_seed == 0)
		       {
				    fbe_cli_printf("Invalidate wwn seed!\n");
				    return;
			   }
			   fbe_cli_set_wwn_seed_from_prom_resume(&wwn_seed);	
			   return;				 
		   }
       }
	   else
	   {		     	
		    fbe_cli_error("\nInvalid command line option.\n");
			fbe_cli_printf("%s", RC_CMD_USAGE);
			return;
	   }
	}

    return;
		
}
  
/******************************************
 * end fbe_cli_replace_chassis

 ******************************************/



static void fbe_cli_display_fru_descriptor(fbe_homewrecker_fru_descriptor_t* fru_descriptor)
{
    if(NULL == fru_descriptor)
    {
        fbe_cli_printf("fru_descriptor is NULL.\n");
        return;
    }

    /*output the FRU Descriptor into trace */
    fbe_cli_printf("Homewrecker: FRU_Descriptor:\n");
    fbe_cli_printf("Magic string: %s\n", fru_descriptor->magic_string);
    fbe_cli_printf("Chassis wwn seed: 0x%x\n", fru_descriptor->wwn_seed);
    fbe_cli_printf("Sys drive 0 SN: %s\n", fru_descriptor->system_drive_serial_number[0]);
    fbe_cli_printf("Sys drive 1 SN: %s\n", fru_descriptor->system_drive_serial_number[1]);
    fbe_cli_printf("Sys drive 2 SN: %s\n", fru_descriptor->system_drive_serial_number[2]);
    fbe_cli_printf("Sys drive 3 SN: %s\n", fru_descriptor->system_drive_serial_number[3]);
    fbe_cli_printf("Chassis replacement flag: 0x%x\n", fru_descriptor->chassis_replacement_movement);
    fbe_cli_printf("Sequence number: %d\n", fru_descriptor->sequence_number);
    fbe_cli_printf("Structure version: 0x%x\n", fru_descriptor->structure_version);
    return;
}


static void fbe_cli_display_fru_signature(fbe_fru_signature_t* fru_signature)
{
    if(NULL == fru_signature)
    {
        fbe_cli_printf("fru_signature is NULL.\n");
        return;
    }

    fbe_cli_printf("Magic string: %s\n", fru_signature->magic_string);
    fbe_cli_printf("Version: %d\n",fru_signature->version);
    fbe_cli_printf("System_wwn_seed: 0x%llx\n", fru_signature->system_wwn_seed);
    fbe_cli_printf("Bus: %d\n",fru_signature->bus);
    fbe_cli_printf("Enclosure: %d\n", fru_signature->enclosure);
    fbe_cli_printf("Slot: %d\n",fru_signature->slot);
    return;
}


/********************************************************************************************
function: fbe_cli_get_wwn_seed_from_userdisk
when do the logic decision about whether update the chassis wwn_seed, we should dertermine 
whether the wwn_seed is consistent between system drives and user disks.

For wwn_user , we will scan the user disks and get FBE_CLI_RC_USER_DISK_SAMPLE_COUNT samples. 
after read the fru_signature from the sample disks, we can get every wwn_seed value from samples.
If above 80% user disks wwn_seeds are consistent , then we get the wwn_user, return FBE_STATUS_OK.
Else we can't get the wwn_user, return FBE_STATUS_GENERIC_FAILURE.

For one special case, if no user disks in the array, we still return FBE_STATUS_OK.

*********************************************************************************************/

static fbe_status_t fbe_cli_get_wwn_seed_from_userdisk(fbe_u64_t * wwn_user, fbe_u32_t *sample_count)
{
	fbe_status_t                status = FBE_STATUS_OK;
	fbe_u32_t				    ldo_count = 0;
	fbe_object_id_t *		    ldo_object_array = NULL;
	fbe_u32_t					index = 0;
	fbe_u32_t                   index2 = 0;
	fbe_drive_phy_info_t        physical_drive_info;
	fbe_job_service_bes_t       drive_location;
	fbe_u8_t *					read_buffer_one_block_520 = NULL;
	fbe_fru_signature_t *  	    fru_signature_p = NULL;
	fbe_lifecycle_state_t	    lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
	fbe_u64_t 					wwn_user_sample[10]={0};
	fbe_u32_t 					CountNum[10]={0};
	fbe_u32_t 					SampleCount = 0;
	fbe_u64_t 					wwn_seed_maxCountNum = 0;
	fbe_u32_t 				    maxCount = 0;
    fbe_lba_t                   fru_signature_lba = 0;
		
    status = fbe_cli_get_fru_signature_lba(&fru_signature_lba);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get frusignature lba. \n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
	/* Get the number of drives in current array */
	status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST, FBE_PACKAGE_ID_PHYSICAL, &ldo_count );
	if (status != FBE_STATUS_OK) {
		fbe_cli_printf("Failed to get total LDOs in the system. \n");
		return FBE_STATUS_GENERIC_FAILURE;
	}
		
	if (ldo_count == 0)
	{
		fbe_cli_printf("No LDO found in current array! \n");
		return FBE_STATUS_GENERIC_FAILURE;
	}
	else
	{
		ldo_count = 0;
        //enumerate all the disks, get the objectid
		status = fbe_api_enumerate_class(FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST, FBE_PACKAGE_ID_PHYSICAL, &ldo_object_array, &ldo_count);
		if (status != FBE_STATUS_OK) {
			fbe_cli_printf("Failed to get a list of LDOs. \n");
			fbe_api_free_memory(ldo_object_array);
			return FBE_STATUS_GENERIC_FAILURE;
		}
        read_buffer_one_block_520 = fbe_api_allocate_memory(FBE_BE_BYTES_PER_BLOCK);
        if (read_buffer_one_block_520 == NULL)
        {
            fbe_cli_printf("Failed to allocate memory for one_block_buffer. \n");
            fbe_api_free_memory(ldo_object_array);
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
			//	fbe_cli_printf("\nFailed to get ldo state, object_id: 0x%x.  bypass it this time.\n", ldo_object_array[index]);
				continue;
			}else {
				if (lifecycle_state != FBE_LIFECYCLE_STATE_READY)
				{
					//fbe_cli_printf("\nThe ldo with object_id: 0x%x is NOT ready.bypass it this time. \n", ldo_object_array[index]);
					continue;
				}
			}
				
			/* Get ldo info, which records the drive's location for now */
			status = fbe_api_physical_drive_get_phy_info(ldo_object_array[index], &physical_drive_info);
			if (status != FBE_STATUS_OK)
			{
			//	fbe_cli_printf("Fail to get ldo info whose object_id: 0x%x.\n", ldo_object_array[index]);
				continue;
			}
				
			/* Read fru_signature from drive, which records the drive's original location  */
			drive_location.bus = physical_drive_info.port_number;
			drive_location.enclosure = physical_drive_info.enclosure_number;
			drive_location.slot = physical_drive_info.slot_number;
            
            //we only process the user disks, for system disks, continue
	        if( (drive_location.bus == 0)&& (drive_location.enclosure == 0)&& (drive_location.slot < 4))
            {
                continue;
            }
			status = fbe_cli_wisd_block_operation(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
							                      read_buffer_one_block_520,
                                                        1,
						                          fru_signature_lba,
							                      drive_location);
			if (status != FBE_STATUS_OK)
			{
			//	fbe_cli_printf("Fail to get fru_signature from ldo object_id: 0x%x.\n", ldo_object_array[index]);
				continue;
			}				
			fru_signature_p = (fbe_fru_signature_t *)read_buffer_one_block_520;
	
			/* Validate the fru_signature via magic_string */
			if (strcmp(fru_signature_p->magic_string, FBE_FRU_SIGNATURE_MAGIC_STRING))
			{
				/* It is a new drive */
				continue;
			}
			//fbe_cli_display_fru_signature(fru_signature_p);
            // get the samples of the user disks till the SampleCount equals FBE_CLI_RC_USER_DISK_SAMPLE_COUNT
            wwn_user_sample[SampleCount] = fru_signature_p->system_wwn_seed;
			SampleCount++;
			if(SampleCount >= FBE_CLI_RC_USER_DISK_SAMPLE_COUNT)
			{
				break;
			}
		
		}
			
		fbe_api_free_memory(ldo_object_array);
        fbe_api_free_memory(read_buffer_one_block_520);
	}
 
    // calculate every wwn_seed frequency, and get the wwn_seed_maxCountNum which occurs most
    maxCount = CountNum[0];
    wwn_seed_maxCountNum = wwn_user_sample[0];
	for (index=0; index < SampleCount; index++)
	{
		for(index2=0; index2<SampleCount; index2++)
		{
			if(wwn_user_sample[index] == wwn_user_sample[index2])
			{ 
                CountNum[index]++; 
            }
		}
        if(CountNum[index] > maxCount)
        {
            maxCount = CountNum[index];
            wwn_seed_maxCountNum = wwn_user_sample[index];
        }
        // if one wwwn_seed occurs more than half, then this wwn_seed must be the wwn_seed_maxCountNum, no need process again
        if(maxCount >= FBE_CLI_RC_USER_DISK_SAMPLE_COUNT/2)
        {
            break;
        }

	}
	*sample_count = SampleCount;	
	//fbe_cli_printf("SampleCount = %d,maxCount = %d, wwn_seed_maxCountNum = 0x%llx.\n",SampleCount,maxCount,wwn_seed_maxCountNum);
    if(SampleCount == 0)
    {
        return FBE_STATUS_OK;
    }
	if( maxCount*100/SampleCount >= 80 )
	{
		*wwn_user = wwn_seed_maxCountNum;
		return FBE_STATUS_OK;	
	}
	else 
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}
	
}





static void fbe_cli_display_std_info(homewrecker_system_disk_info_t* system_db_drive_table)
{
	fbe_u32_t index = 0;
    if(NULL == system_db_drive_table)
    {
            fbe_cli_printf("system_db_drive_table is NULL.\n");
        	return;
    }

        /*output the FRU Descriptor into trace */
	for (index = 0;index < FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER ; index++)
	{
	        fbe_cli_printf("\nsystem disk index %d,bus:%d,enclosure:%d,slot:%d info. as following:\n",index,system_db_drive_table[index].actual_bus,system_db_drive_table[index].actual_encl,system_db_drive_table[index].actual_slot);
    		fbe_cli_printf("Homewrecker: FRU_Descriptor:\n");
    		fbe_cli_printf("Magic string: %s\n", system_db_drive_table[index].fru_descriptor.magic_string);
   		    fbe_cli_printf("Chassis wwn seed: 0x%x\n", system_db_drive_table[index].fru_descriptor.wwn_seed);
    		fbe_cli_printf("Sys drive 0 SN: %s\n", system_db_drive_table[index].fru_descriptor.system_drive_serial_number[0]);
    		fbe_cli_printf("Sys drive 1 SN: %s\n", system_db_drive_table[index].fru_descriptor.system_drive_serial_number[1]);
    		fbe_cli_printf("Sys drive 2 SN: %s\n", system_db_drive_table[index].fru_descriptor.system_drive_serial_number[2]);
    		fbe_cli_printf("Sys drive 3 SN: %s\n", system_db_drive_table[index].fru_descriptor.system_drive_serial_number[3]);
    		fbe_cli_printf("Chassis replacement flag: 0x%x\n", system_db_drive_table[index].fru_descriptor.chassis_replacement_movement);
    		fbe_cli_printf("Sequence number: %d\n", system_db_drive_table[index].fru_descriptor.sequence_number);
    		fbe_cli_printf("Structure version: 0x%x\n", system_db_drive_table[index].fru_descriptor.structure_version);
     
            fbe_cli_printf("Homewrecker: FRU signature:\n");
		    fbe_cli_printf("Magic string: %s\n",system_db_drive_table[index].fru_sig.magic_string);
		    fbe_cli_printf("Version: %d\n",system_db_drive_table[index].fru_sig.version);
		    fbe_cli_printf("System_wwn_seed: 0x%llx\n",system_db_drive_table[index].fru_sig.system_wwn_seed);
		    fbe_cli_printf("Bus: %d\n",system_db_drive_table[index].fru_sig.bus);
		    fbe_cli_printf("Enclosure: %d\n",system_db_drive_table[index].fru_sig.enclosure);
		    fbe_cli_printf("Slot: %d\n",system_db_drive_table[index].fru_sig.slot);

		   // fbe_cli_printf("system disk %d disktype[0:disk_invalid,1:no_disk,2:new_disk,3:sys_disk,4:usr_disk,0xff:unknown]=%d ,is_fru_descriptor_valid = %d, is_fru_signature_valid = %d. \n",index,system_db_drive_table[index].disk_type, system_db_drive_table[index].is_fru_descriptor_valid, system_db_drive_table[index].is_fru_signature_valid);

	}
    return;
}


static fbe_status_t fbe_cli_get_system_drive_status(homewrecker_system_disk_info_t* system_db_drive_table)
{
		fbe_status_t				    status = FBE_STATUS_OK;
		fbe_u32_t						index = 0;
		fbe_job_service_bes_t 			drive_location;
		fbe_u8_t *						read_buffer_one_block_520 = NULL;
        fbe_lba_t                       fru_descriptor_lba = FBE_LBA_INVALID;
        fbe_lba_t                       fru_signature_lba = FBE_LBA_INVALID;
        fbe_sector_t *                  blkptr = NULL;
        fbe_raw_mirror_sector_data_t*   raw_blkptr = NULL;
        fbe_u16_t                       crc = 0;
        fbe_object_id_t                 objid = FBE_OBJECT_ID_INVALID;
		
        status = fbe_cli_get_fru_descriptor_lba(&fru_descriptor_lba);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Failed to get fru descriptor lba. \n");
            return FBE_STATUS_GENERIC_FAILURE;
        }
        status = fbe_cli_get_fru_signature_lba(&fru_signature_lba);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Failed to get fru signature lba. \n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        read_buffer_one_block_520 = fbe_api_allocate_memory(FBE_BE_BYTES_PER_BLOCK);
        if (read_buffer_one_block_520 == NULL)
        {
             fbe_cli_printf("Failed to allocate memory for one_block_buffer. \n");
             return FBE_STATUS_GENERIC_FAILURE;
        }
		/* initialize system_disk_table with default values */
		for (index = 0; index < FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER; index++)
		{
			system_db_drive_table[index].disk_type = FBE_HW_DISK_TYPE_UNKNOWN;
			fbe_zero_memory(&system_db_drive_table[index].fru_sig, sizeof(system_db_drive_table[index].fru_sig));
			system_db_drive_table[index].is_fru_signature_valid = FBE_FALSE;
			fbe_zero_memory(&system_db_drive_table[index].fru_descriptor, sizeof(system_db_drive_table[index].fru_descriptor));
			system_db_drive_table[index].is_fru_descriptor_valid = FBE_FALSE;
			system_db_drive_table[index].actual_bus = 0;
			system_db_drive_table[index].actual_encl = 0;
			system_db_drive_table[index].actual_slot = index;
		}
		/* Go through all drives, get fru_descriptor*/
		for (index = 0; index < FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER; index++)
		{
			/* Read fru_signature from drive, which records the drive's original location  */
			drive_location.bus = 0;
			drive_location.enclosure = 0;
			drive_location.slot = index;
	
			//get fru_descriptor
			fbe_zero_memory(read_buffer_one_block_520, FBE_BE_BYTES_PER_BLOCK);
			status = fbe_cli_wisd_block_operation(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                                  read_buffer_one_block_520,
				                                       1,
						                          fru_descriptor_lba,
						                          drive_location);
			if (status != FBE_STATUS_OK)
			{
			//	fbe_cli_printf("Fail to get fru_descriptor from disk index: 0x%x.\n", index);
				continue;
			}
            
            //validate the crc,magic_number,magic_string for the block data we got
            blkptr = (fbe_sector_t *)read_buffer_one_block_520;
            raw_blkptr = (fbe_raw_mirror_sector_data_t*)blkptr;
            crc = xorlib_cook_csum(xorlib_calc_csum(blkptr->data_word),0);
            if(crc != blkptr->crc)
            {
                continue;
            }
            if (raw_blkptr->magic_number != FBE_MAGIC_NUMBER_RAW_MIRROR_IO)
            {
                continue;
            }				
			fbe_copy_memory( &system_db_drive_table[index].fru_descriptor,read_buffer_one_block_520,sizeof(fbe_homewrecker_fru_descriptor_t));
		        /* Validate the fru_descriptor via magic_string */
			if (strcmp(system_db_drive_table[index].fru_descriptor.magic_string, FBE_FRU_DESCRIPTOR_MAGIC_STRING))
			{
			      /* It is not a valid fru_descriptor */
				continue;
			}
			system_db_drive_table[index].is_fru_descriptor_valid= true;
		}
			
		for (index = 0 ; index < FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER ; index++)
		{
            status = fbe_api_get_physical_drive_object_id_by_location(0,0,index,&objid);
            if( (status != FBE_STATUS_OK) || (objid == FBE_OBJECT_ID_INVALID) )
            {
                system_db_drive_table[index].disk_type = FBE_HW_NO_DISK;
                continue;
            }
			/* Read fru_signature from drive, which records the drive's original location  */
			drive_location.bus = 0;
			drive_location.enclosure = 0;
			drive_location.slot = index;
				//get fru_signature
			fbe_zero_memory(read_buffer_one_block_520, FBE_BE_BYTES_PER_BLOCK);
			status = fbe_cli_wisd_block_operation(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,	
                                                  read_buffer_one_block_520,
							       	 	           1,
									              fru_signature_lba,
	     							              drive_location);
			if (status != FBE_STATUS_OK)
			{
			//	fbe_cli_printf("Fail to get fru_signature from disk index: 0x%x.\n", index);
				continue;
			}
			memcpy( &system_db_drive_table[index].fru_sig,read_buffer_one_block_520,sizeof(fbe_fru_signature_t));
				/* Validate the fru_signature via magic_string */
			if (strcmp(system_db_drive_table[index].fru_sig.magic_string, FBE_FRU_SIGNATURE_MAGIC_STRING))
			{
		               /* It is not a valid fru_signature */
				system_db_drive_table[index].disk_type =  FBE_HW_NEW_DISK;
				continue;
			}
			system_db_drive_table[index].is_fru_signature_valid= true;
		}

		fbe_api_free_memory(read_buffer_one_block_520);
		return FBE_STATUS_OK;
	
}




static fbe_status_t fbe_cli_replace_chassis_process_cmds(fbe_bool_t isForce)
{
    fbe_status_t                     status = FBE_STATUS_OK;
    fbe_u32_t                        chassis_wwn_seed = 0;
    fbe_u32_t                        index = 0;
	homewrecker_system_disk_info_t   system_db_drive_table[3]={0};
	fbe_bool_t 			             all_fru_descriptors_consistent = FBE_FALSE;
	fbe_u64_t    			         wwn_user = 0;
	fbe_u32_t 			             invalid_system_disk_num = 0;
	fbe_u32_t 			             invalid_system_disk_index = 0;
    fbe_u32_t                        user_disk_sample_count = 0;
    fbe_u8_t                         retry_count = 0;
    fbe_system_disk_type_t           DiskType = FBE_HW_DISK_TYPE_UNKNOWN; 

   	 /* Get wwn_seed from chassis PROM resume */
    status = fbe_cli_wisd_get_wwn_seed_from_prom_resume(&chassis_wwn_seed);
    while((status != FBE_STATUS_OK) && (retry_count < FBE_CLI_FAILED_IO_RETRIES))
    {
        /* Wait 1 second */
        fbe_thread_delay(FBE_CLI_FAILED_IO_RETRY_INTERVAL);
        retry_count++;
        fbe_cli_printf(" Try again to get chassis wwn_seed !\n");
        status = fbe_cli_wisd_get_wwn_seed_from_prom_resume(&chassis_wwn_seed);
    }

	if(status != FBE_STATUS_OK)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}

	status = fbe_cli_get_system_drive_status(system_db_drive_table);
	if(status != FBE_STATUS_OK)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}
		
	status = fbe_cli_get_wwn_seed_from_userdisk(&wwn_user,&user_disk_sample_count);
	if(status != FBE_STATUS_OK)
	{
        fbe_cli_printf(" Most user disks wwnseed are not consistent, can't get wwn_user, so rejct update chassis wwnseed.\n");
		return FBE_STATUS_GENERIC_FAILURE;
	}
    
    //calculate the invalid_system_disk_num
    for(index = 0; index < FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER ; index++)
    {
    	if(!system_db_drive_table[index].is_fru_descriptor_valid)
    	{ 
           invalid_system_disk_num++;
        }
        else
        {
           // if no user disks, we will ignore wwn_user, we can treat wwn_user equals one of the system disk wwn_seed.
           if(user_disk_sample_count == 0)
           {
               wwn_user = system_db_drive_table[index].fru_descriptor.wwn_seed;
           }
        }
    }
	
    if( invalid_system_disk_num >= 2)
    {
        fbe_cli_printf(" more than two system drives don't have valid fru_descriptor, reject to update Chassis wwnseed!\n ");
		return FBE_STATUS_GENERIC_FAILURE;
    }
	if( invalid_system_disk_num == 1)
	{
		all_fru_descriptors_consistent = FBE_TRUE;
        // determine whether the other two disks wwn_seeds are consistent and get the index of the invalid system disk
		for (index = 0; index < FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER; index ++)
		{
			if(system_db_drive_table[index].is_fru_descriptor_valid )
		  	{
		  	  	if( wwn_user != system_db_drive_table[index].fru_descriptor.wwn_seed)
		  	  	{
		  	  	   	   all_fru_descriptors_consistent = FBE_FALSE; 
		  	  	}
		  	}
			else
			{
			  	invalid_system_disk_index = index; 
			}  	
		 }
         DiskType = system_db_drive_table[invalid_system_disk_index].disk_type;
         // if the invalid disk is NEW_DISK or NO_DISK and the other two sytem disks wwn_seeds are consistent,allow update chassis wwn_seed with -force.
		 if( ((DiskType == FBE_HW_NEW_DISK) || (DiskType == FBE_HW_NO_DISK)) && all_fru_descriptors_consistent )
		 {
               		if(isForce)
              		{
              	        if( wwn_user == chassis_wwn_seed)
                        {
                                fbe_cli_printf("wwn_seed is matched. No need update chassis wwnseed again! run the command successfully!\n");
                                return FBE_STATUS_OK;
                        }
                        else
                        {
				                status = fbe_cli_set_wwn_seed_from_prom_resume ((fbe_u32_t*)(&wwn_user));
                                if(status != FBE_STATUS_OK)
                                {
                                    return FBE_STATUS_GENERIC_FAILURE;
                                }
				                return FBE_STATUS_OK;
                        }
				 
			       }
			       else
			       {
			  	        fbe_cli_printf("disk %d is new drive, reject to Update Chassis wwnseed without force.\n",invalid_system_disk_index);
				        return FBE_STATUS_GENERIC_FAILURE;
			       }
	
		  }
		  else
		  {
		  	  fbe_cli_printf(" invalid disk %d is not new drive or NO disk, or the other wwnseeds not consistent. \n",invalid_system_disk_index);
			  return FBE_STATUS_GENERIC_FAILURE;
		  }
		
	}
	
	if( invalid_system_disk_num == 0)
	{
        if( (system_db_drive_table[0].fru_descriptor.wwn_seed == system_db_drive_table[1].fru_descriptor.wwn_seed) && (system_db_drive_table[1].fru_descriptor.wwn_seed == system_db_drive_table[2].fru_descriptor.wwn_seed))
        {
          	  all_fru_descriptors_consistent = true;
        }
		 
        //if all disks wwn_seeds are consistent, allow to update chassis wwn_seed; or else rejedt
		if(all_fru_descriptors_consistent && (system_db_drive_table[0].fru_descriptor.wwn_seed == wwn_user) )
		{
		  if(wwn_user == chassis_wwn_seed )
		  {
		  	  fbe_cli_printf("wwn_seed is matched, no need to update again,run the command successfully!\n ");
			  return FBE_STATUS_OK;
		  }
		  else
		  {
			  status = fbe_cli_set_wwn_seed_from_prom_resume (&system_db_drive_table[0].fru_descriptor.wwn_seed);
              if(status != FBE_STATUS_OK)
              {
                return FBE_STATUS_GENERIC_FAILURE;
              }
			  return FBE_STATUS_OK;
		  }
		}
		else if (all_fru_descriptors_consistent && (system_db_drive_table[0].fru_descriptor.wwn_seed != wwn_user))
		{
		  	fbe_cli_printf(" wwnseed is only consistent between system drives, not same with wwn_user. \n");
			return FBE_STATUS_GENERIC_FAILURE;
		}
		else
		{
		  	fbe_cli_printf("wwnseed is not consistent between system drives. \n");
			return FBE_STATUS_GENERIC_FAILURE;
		}	
	}
	
    return FBE_STATUS_GENERIC_FAILURE;    
}




static fbe_status_t fbe_cli_set_wwn_seed_from_prom_resume(fbe_u32_t * wwn_seed)
{
    fbe_u32_t                   read_buf_p = *wwn_seed;
    fbe_u32_t                   object_handle = FBE_OBJECT_ID_INVALID;
    fbe_u8_t                    retry_count = 0;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_resume_prom_cmd_t       *pWriteResumePromCmd;
    pWriteResumePromCmd = (fbe_resume_prom_cmd_t*)fbe_api_allocate_memory(sizeof(fbe_resume_prom_cmd_t));
    if(pWriteResumePromCmd == NULL)
    {
        fbe_cli_printf("Not enough memory to allocate for  pWriteResumePromCmd\n");
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    fbe_zero_memory(pWriteResumePromCmd,sizeof(fbe_resume_prom_cmd_t));

    
    pWriteResumePromCmd->deviceType = FBE_DEVICE_TYPE_ENCLOSURE;      // Input
    pWriteResumePromCmd->deviceLocation.sp = SP_A;
    pWriteResumePromCmd->deviceLocation.enclosure = 0;
    pWriteResumePromCmd->deviceLocation.port = 0;
    pWriteResumePromCmd->deviceLocation.slot = 0;
    pWriteResumePromCmd->deviceLocation.bus = 0;
    pWriteResumePromCmd->resumePromField = RESUME_PROM_WWN_SEED;
    pWriteResumePromCmd->offset =CSX_OFFSET_OF(RESUME_PROM_STRUCTURE,wwn_seed);
   
    pWriteResumePromCmd->pBuffer =(fbe_u8_t*) &read_buf_p;
    pWriteResumePromCmd->bufferSize = sizeof(fbe_u32_t);

    status = fbe_api_get_enclosure_object_id_by_location(0, 0, &object_handle);

    if (status != FBE_STATUS_OK ||object_handle == FBE_OBJECT_ID_INVALID)
    {
        fbe_cli_printf("Fail to get board object id. \n");
        fbe_api_free_memory(pWriteResumePromCmd);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* fbe_cli_printf("\nSending asynchronous Resume Read, board object ID: %d. \n", object_handle);*/

    /* Get the Resume Info - ASynchronous */
    status = fbe_api_resume_prom_write_sync(object_handle, pWriteResumePromCmd);   
    while((status != FBE_STATUS_OK) && (retry_count < FBE_CLI_FAILED_IO_RETRIES))
    {
        /* Wait 1 second */
        fbe_thread_delay(FBE_CLI_FAILED_IO_RETRY_INTERVAL);
        retry_count++;
        status = fbe_api_resume_prom_write_sync(object_handle, pWriteResumePromCmd);
    }
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Fail to write chassis wwn_seed. \n");
        fbe_api_free_memory(pWriteResumePromCmd);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_cli_printf("Write array chassis wwn_seed with value 0x%x successfully.\n", *wwn_seed);

    fbe_api_free_memory(pWriteResumePromCmd);
    return FBE_STATUS_OK;
}



static fbe_status_t fbe_cli_get_fru_descriptor_lba(fbe_lba_t *lba)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_private_space_layout_region_t   psl_region_info;
    status = fbe_private_space_layout_get_region_by_region_id(FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_HOMEWRECKER_DB, &psl_region_info);
    if(status != FBE_STATUS_OK)
    {
       fbe_cli_printf("when get fru_descriptor, couldn't get psl region info\n");
       return FBE_STATUS_GENERIC_FAILURE;
    }
                                                    
    *lba = psl_region_info.starting_block_address;
  // fbe_cli_printf("fru_descriptor LBA address is 0x%llx.\n",*lba);
    return FBE_STATUS_OK;   
}


static fbe_status_t fbe_cli_get_fru_signature_lba(fbe_lba_t *lba)
{
    fbe_status_t                         status = FBE_STATUS_GENERIC_FAILURE;
    fbe_private_space_layout_region_t    psl_region_info;
    status = fbe_private_space_layout_get_region_by_region_id(FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_FRU_SIGNATURE, &psl_region_info);
    if(status != FBE_STATUS_OK)
    {
       fbe_cli_printf("when get fru_signature, couldn't get psl region info\n");
       return FBE_STATUS_GENERIC_FAILURE;
    }
                                                    
    *lba = psl_region_info.starting_block_address;
   // fbe_cli_printf("fru_signature LBA address is 0x%llx.\n",*lba);
    return FBE_STATUS_OK;   
}



/*************************
 * end file fbe_cli_lib_replace_chassis_cmds.c
 *************************/

