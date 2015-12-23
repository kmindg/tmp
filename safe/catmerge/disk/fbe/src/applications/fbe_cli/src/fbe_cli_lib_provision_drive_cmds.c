/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
* @file fbe_cli_provision_drive_cmds.c
 ***************************************************************************
 *
 * @brief
*  This file contains cli functions for the Provisional Drive related features in FBE CLI.
 *
 * @ingroup fbe_cli
 *
 * @date
 *  1/13/2011 - Created. Sandeep Chaudhari
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
//#include <ctype.h>
#include "fbe_cli_private.h"
#include "fbe_cli_lib_provision_drive_cmds.h"
#include "fbe_trace.h"
#include "fbe_cli_lurg.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_system_bg_service_interface.h"
#include "fbe/fbe_api_system_time_threshold_interface.h"
#include "fbe_provision_drive_trace.h"
#include "fbe_cli_lib_paged_utils.h"
#include "fbe_private_space_layout.h"

/*************************
 * Externs
 *************************/
extern fbe_char_t * fbe_convert_configured_physical_block_size_to_string
	(fbe_provision_drive_configured_physical_block_size_t configured_physical_block_size);

/*************************
 * Forward Declerations
 *************************/
void fbe_cli_display_pvd_slf_info(fbe_provision_drive_slf_state_t slf_state);
fbe_char_t * fbe_cli_get_pvd_slf_state_string(fbe_provision_drive_slf_state_t slf_state);
static void fbe_cli_get_system_timethreshold(void);
static void fbe_cli_set_system_timethreshold(fbe_u64_t timethreshold_in_minutes);
static void fbe_cli_pvdinfo_paged_summary(fbe_object_id_t pvd_object_id, fbe_bool_t b_force_unit_access);
static void fbe_cli_pvdinfo_paged_operation(fbe_object_id_t pvd, int argc, char **argv);
fbe_status_t fbe_cli_pvd_parse_bes_opt(char *bes_str, fbe_object_id_t * pvd_object_id);

/*!*******************************************************************
 * @var fbe_cli_provision_drive_set_debug_flag()
 *********************************************************************
 * @brief Function to set debug flag for provision drive either on object level or class level.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  1/13/2011 - Created. Sandeep Chaudhari
 *********************************************************************/
void fbe_cli_provision_drive_set_debug_flag(fbe_s32_t argc, fbe_s8_t ** argv)
{
    fbe_status_t    status = FBE_STATUS_INVALID;
    fbe_u32_t       hex_number = -1;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;

    if(0 == argc)
    {
        fbe_cli_error("Too few arguments. \n");
        fbe_cli_printf("%s", PVD_SET_DEBUG_FLAG_USAGE);
        return;
    }
    
    if((1 == argc) && 
        ((strcmp(*argv, "-help") == 0) ||(strcmp(*argv, "-h") == 0)))
    {
        /* If they are asking for help, just display help and exit. */
        fbe_cli_printf("%s", PVD_SET_DEBUG_FLAG_USAGE);
        return;
    }
    else
    {
        while (argc > 0)
        {
            if(strcmp(*argv, "-object_id") == 0)
            {
                /* Get the object id from command line. */
                argc--;
                argv++;

                /* argc should be greater than 0 here */
                if (argc <= 0)
                {
                    fbe_cli_error("Please provide value for -object_id switch. \n");
                    return;
                }
                
                if(!strcmp(*argv,"all"))
                {
                    pvd_object_id = FBE_OBJECT_ID_INVALID;
                }
                else
                {
                    fbe_char_t *tmp_ptr = *argv;

                    if(('0' == *tmp_ptr) && 
                        (('x' == *(tmp_ptr+1)) || ('X' == *(tmp_ptr+1))))
                    {
                        tmp_ptr += 2;

                        if(!strcmp(tmp_ptr,""))
                        {
                            fbe_cli_error("Invalid object id. Please provide correct object id in hex format. \n");
                            return;
                        }
                    }
                    else
                    {
                        fbe_cli_error("Invalid object id. Please provide correct object id in hex format. \n");
                        return;
                    }

                    pvd_object_id = fbe_atoh(tmp_ptr);

                    if(pvd_object_id > FBE_MAX_SEP_OBJECTS)
                    {
                        fbe_cli_error("\nInvalid PVD Object Id 0x%x.\n", pvd_object_id);
                        fbe_cli_printf("%s", PVD_SET_DEBUG_FLAG_USAGE);
                        return;
                    }
                }

                /* Increment the arguments */
                argc--;
                argv++;
            }
            else if (strcmp(*argv, "-bes") == 0)
            {
                argc--;
                argv++;
                status = fbe_cli_pvd_parse_bes_opt(*argv, &pvd_object_id);
                if (status != FBE_STATUS_OK)
                {
                    /* the previous called function prints the error msgs.*/
                    return;
                }
                argc--;
                argv++;
            }
            else if (strcmp(*argv, "-debug_flags") == 0)
            {
                /* Get the debug flag from command line. */
                argc--;
                argv++;
        
                /* argc should be greater than 0 here */
                if (argc <= 0)
                {
                    fbe_cli_error("Please provide debug flag value in hex format. \n");
                    return;
                }
                else
                {
                    fbe_char_t *tmp_ptr = *argv;
                    

                    if(('0' == *tmp_ptr) && 
                        (('x' == *(tmp_ptr+1)) || ('X' == *(tmp_ptr+1))))
                    {
                        tmp_ptr += 2;

                        if(!strcmp(tmp_ptr,""))
                        {
                            fbe_cli_error("Invalid debug flag value. Please provide correct debug flag value in hex format.\n");
                            return;
                        }
                    }
                    else
                    {
                        fbe_cli_error("Invalid debug flag value. Please provide correct debug flag value in hex format.\n");
                        return;
                    }

                    /* Convert the input string to hex number */
                    hex_number = fbe_atoh(tmp_ptr);

                    /* Check Is conversion successful?*/
                    if((0 > hex_number) || (FBE_PROVISION_DRIVE_DEBUG_FLAG_LAST <=
                        hex_number))
                    {
                        fbe_cli_error("Invalid debug flag value. Please provide correct debug flag value in hex format.\n");
                        return;
                    }
                    
                    /* Increment the arguments */
                    argc--;
                    argv++;
                }
            }
            else
            {
                /* The command line parameters should be properly entered */
                fbe_cli_error("Invalid arguments. Please check the command \n");
                fbe_cli_printf("%s", PVD_SET_DEBUG_FLAG_USAGE);
                return;
            }

            // No more argument, call the function to set debug flag.
            if( (0 == argc) && (-1 != hex_number))
            {
                fbe_u32_t shiftBit=0, i=0;
                if(FBE_OBJECT_ID_INVALID == pvd_object_id)
                {
                    status = fbe_api_provision_drive_set_class_debug_flag(hex_number);

                    if(FBE_STATUS_OK != status)
                    {
                        fbe_cli_error("Failed to set class debug flag for provision drive in %s \n", __FUNCTION__);
                        return;
                    }
                    else
                    {
                        if(0 == hex_number)
                        {
                            fbe_cli_printf("Successfully clear debug flags for provision drive at class level. \n");
                            return;
                        }
                        else
                        {
                            fbe_cli_printf("Successfully set following debug flags for provision drive at class level. \n");
                        }
                    }
                }
                else // Set debug flag for object
                {
                    status = fbe_api_provision_drive_set_debug_flag(pvd_object_id, hex_number);

                    if(FBE_STATUS_OK != status)
                    {
                        fbe_cli_error("Failed to set debug flag for provision drive object in %s \n", __FUNCTION__);
                        return;
                    }
                    else
                    {
                        if(0 == hex_number)
                        {
                            fbe_cli_printf("Successfully clear debug flags for provision drive at object level. \n");
                            return;
                        }
                        else
                        {
                            fbe_cli_printf("Successfully set following debug flags for provision drive at object level. \n");
                        }
                    }
                }
                
                
                while(shiftBit<FBE_PROVISION_DRIVE_DEBUG_FLAG_LAST)
                {
                    shiftBit = BIT0 << i++;

                    if (hex_number & shiftBit)
                    {
                        switch(shiftBit)
                        {
                            case FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING:
                                fbe_cli_printf("0x%x \t General Tracing \n", shiftBit);
                                break;

                            case FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING:
                                fbe_cli_printf("0x%x \t Background Zeroing Tracing \n", shiftBit);
                                break;

                            case FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING:
                                fbe_cli_printf("0x%x \t User Zero Tracing \n", shiftBit);
                                break;

                            case FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING:
                                fbe_cli_printf("0x%x \t Zeroing On Demand Tracing \n", shiftBit);
                                break;

                            case FBE_PROVISION_DRIVE_DEBUG_FLAG_VERIFY_TRACING:
                                fbe_cli_printf("0x%x \t Verify Tracing \n", shiftBit);
                                break;

                            case FBE_PROVISION_DRIVE_DEBUG_FLAG_METADATA_TRACING:
                                fbe_cli_printf("0x%x \t Metadata Tracing \n", shiftBit);
                                break;

                            case FBE_PROVISION_DRIVE_DEBUG_FLAG_LOCK_TRACING:
                                fbe_cli_printf("0x%x \t File Lock Tracing \n", shiftBit);
                                break;

                            case FBE_PROVISION_DRIVE_DEBUG_FLAG_SNIFF_TRACING:
                                fbe_cli_printf("0x%x \t File Lock Tracing \n", shiftBit);
                                break;

                            case FBE_PROVISION_DRIVE_DEBUG_FLAG_SLF_TRACING:
                                fbe_cli_printf("0x%x \t File Lock Tracing \n", shiftBit);
                                break;

                            case FBE_PROVISION_DRIVE_DEBUG_FLAG_HEALTH_CHECK_TRACING:
                                fbe_cli_printf("0x%x \t Health Check Tracing \n", shiftBit);
                                break;

                            case FBE_PROVISION_DRIVE_DEBUG_FLAG_NO_WRITE_SAME:
                                fbe_cli_printf("0x%x \t Write Same Support \n", shiftBit);
                                break;

                            case FBE_PROVISION_DRIVE_DEBUG_FLAG_ENCRYPTION:
                                fbe_cli_printf("0x%x \t Encryption Tracing \n", shiftBit);
                                break;

                            case FBE_PROVISION_DRIVE_DEBUG_FLAG_HEALTH_TRACING:
                                fbe_cli_printf("0x%x \t Provision Drive Health \n", shiftBit);
                                break;   
                        }
                    }
                    else if(hex_number<shiftBit)
                    {
                        // Break the while loop
                        break;
                    }
                }
            }
            else if(0 == argc)
            {
                fbe_cli_error("You have not provided debug flag value. Please provide debug flag value. \n");
                fbe_cli_printf("%s", PVD_SET_DEBUG_FLAG_USAGE);
                return;
            }
        }
    }
}
/******************************************
 * end fbe_cli_provision_drive_set_debug_flag()
 ******************************************/

/*!*******************************************************************
 * @var fbe_cli_provision_drive_info()
 *********************************************************************
 * @brief Function to get pvd object info.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  3/02/2011 - Created. Vishnu Sharma
 *********************************************************************/
void fbe_cli_provision_drive_info(fbe_s32_t argc, fbe_s8_t ** argv)
{
    fbe_status_t        status = FBE_STATUS_INVALID;
    fbe_object_id_t     pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_char_t         *tmp_ptr = NULL;
    fbe_chunk_index_t   chunk_index = 0;
    fbe_chunk_count_t   chunk_count = 1;
    fbe_class_id_t      class_id;
    fbe_bool_t          print_metadata_flag = FBE_FALSE;
    fbe_bool_t          b_force_unit_access = FBE_FALSE; /* By default allow read from cache */
    fbe_bool_t          sv_enabled;
    fbe_bool_t          test_slf_flag = FBE_FALSE;
    fbe_u32_t           op = 0;
    fbe_u32_t           fua = 0;

        
    if((1 == argc) && 
        ((strcmp(*argv, "-help") == 0) ||(strcmp(*argv, "-h") == 0)))
    {
        /* If they are asking for help, just display help and exit. */
        fbe_cli_printf("%s", PVD_GET_INFO_USAGE);
        return;
    }
    /*No argument display info for all pvd objects in the system*/
    if(argc == 0)
    {
        //Display All pvd objects info
        fbe_cli_display_all_pvd_objects(FBE_FALSE);
        return;
    }
    
    while (argc > 0)
    {
        if((strcmp(*argv, "-global_sniff_verify") == 0) ||
            (strcmp(*argv, "-gsv") == 0)) {
            argv++;
            argc--;

            if(argc <= 0 ) {
                fbe_cli_error("Please provide correct arguments. \n");
                fbe_cli_printf("%s", PVD_GET_INFO_USAGE);
                return;
            } 
            if(strcmp(*argv, "enable")==0) {
                status = fbe_api_enable_system_sniff_verify();
                if (status != FBE_STATUS_OK) {
                    fbe_cli_error( "%s: failed to enable global sniff verify.\n", __FUNCTION__);
                    return;
                }
                fbe_cli_printf("\nSuccessfully enabled global sniff verify.\n\n");
            }
            else if(strcmp(*argv, "disable")==0){
                status = fbe_api_disable_system_sniff_verify();
                if (status != FBE_STATUS_OK){
                    fbe_cli_error( "%s: failed to disable global sniff verify.\n", __FUNCTION__);
                    return;
                }
                fbe_cli_printf("\nSuccessfully disabled global sniff verify\n\n");
            }
            else if(strcmp(*argv, "status")==0){
                sv_enabled = fbe_api_system_sniff_verify_is_enabled();
                if (sv_enabled) {
                    fbe_cli_printf("\nSuccessfully get global sniff verify status: enabled.\n\n");
                }
                else{
                    fbe_cli_printf("\nSuccessfully get global sniff verify status: disabled.\n\n");
                }
            }
            else {
                fbe_cli_error("\nPlease provide correct arguments, either enable or disable or status. \n");
                fbe_cli_printf("%s", PVD_GET_INFO_USAGE);
                return;
            }
            return;
        }
        else if((strcmp(*argv, "-single_loop_failure") == 0) ||
            (strcmp(*argv, "-slf") == 0)) {
            argv++;
            argc--;

            if(argc == 0 ) {
                fbe_bool_t slf_enabled;
                status = fbe_api_provision_drive_get_slf_enabled(&slf_enabled);
                if (status != FBE_STATUS_OK) {
                    fbe_cli_error( "%s: failed to get slf_enabled.\n", __FUNCTION__);
                    return;
                }
                fbe_cli_printf("\nsingle loop failure is %s.\n\n", slf_enabled ? "ENABLED" : "DISABLED");
                return;
            } 
            if(strcmp(*argv, "enable")==0) {
                status = fbe_api_provision_drive_set_slf_enabled(FBE_TRUE);
                if (status != FBE_STATUS_OK) {
                    fbe_cli_error( "%s: failed to enable single loop failure.\n", __FUNCTION__);
                    return;
                }
                fbe_cli_printf("\nSuccessfully enabled single loop failure.\n\n");
            }
            else if(strcmp(*argv, "disable")==0){
                status = fbe_api_provision_drive_set_slf_enabled(FBE_FALSE);
                if (status != FBE_STATUS_OK){
                    fbe_cli_error( "%s: failed to disable single loop failure.\n", __FUNCTION__);
                    return;
                }
                fbe_cli_printf("\nSuccessfully disabled single loop failure\n\n");
            }
            else {
                fbe_cli_error("\nPlease provide correct arguments, either enable or disable. \n");
                fbe_cli_printf("%s", PVD_GET_INFO_USAGE);
                return;
            }
            return;
        }
        if(strcmp(*argv, "-object_id") == 0)
        {
            /* Get the object id from command line. */
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Please provide value for -object_id switch. \n");
                return;
            }
            
            
            tmp_ptr = *argv;
            if(('0' == *tmp_ptr) && 
                (('x' == *(tmp_ptr+1)) || ('X' == *(tmp_ptr+1))))
            {
                tmp_ptr += 2;

                if(!strcmp(tmp_ptr,""))
                {
                    fbe_cli_error("Invalid object id. Please provide correct object id in hex format. \n");
                    return;
                }
            }
            else
            {
                fbe_cli_error("Invalid argument object id. Please provide correct object id in hex format. \n");
                return;
            }
            
            pvd_object_id = fbe_atoh(tmp_ptr);

            if(pvd_object_id > FBE_MAX_SEP_OBJECTS)
            {
                fbe_cli_error("Invalid Provision Drive Object Id. \n\n");
                fbe_cli_printf("%s", PVD_GET_INFO_USAGE);
                return;
            }
          
        }
        else if (strcmp(*argv, "-bes") == 0)
        {
            argc--;
            argv++;
            status = fbe_cli_pvd_parse_bes_opt(*argv, &pvd_object_id);
            if (status != FBE_STATUS_OK)
            {
                /* the previous called function prints the error msgs.*/
                return;
            }
            argc--;
            argv++;
        }
        else if(strcmp(*argv, "-chunk_index") == 0)
        {
            /* Get the object id from command line. */
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Please provide value for -chunk_index switch. \n\n");
                return;
            }
            chunk_index = atoi(*argv);
            
            if(chunk_index < 0)
            {
                fbe_cli_error("Please provide valid meta data chunk index.\n\n");
                return;
            }
            print_metadata_flag = TRUE;
        
        }else if(strcmp(*argv, "-list") == 0){
            fbe_cli_display_all_pvd_objects(FBE_TRUE);
            return;
        }
        else if (strcmp(*argv, "-list_block_size") == 0)
        {
            fbe_cli_display_all_pvd_objects_block_size();
            return;
        }
        else if(strcmp(*argv, "-test_slf") == 0)
        {
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Please provide value for read[0]/write[1] switch. \n\n");
                return;
            }
            op = atoi(*argv);
            test_slf_flag = FBE_TRUE;
        }
        else if(strcmp(*argv, "-chunk_count") == 0)
        {
            /* Get the chunk count to display from command line. */
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Please provide value for -chunk_index switch. \n\n");
                return;
            }
            chunk_count = atoi(*argv);
            
            print_metadata_flag = TRUE;
        }
        else if(strcmp(*argv, "-map_lba") == 0)
        {
            fbe_lba_t lba;
            fbe_provision_drive_map_info_t map_info;
            if (pvd_object_id == FBE_OBJECT_ID_INVALID)
            {
                fbe_cli_error("-map_lba: Please provide object id for pvd\n\n");
                return;
            }
            /* Get the chunk count to display from command line. */
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Please provide value for -chunk_index switch. \n\n");
                return;
            }
            fbe_atoh64(*argv, &lba);

            map_info.lba = lba;
            status = fbe_api_provision_drive_map_lba_to_chunk(pvd_object_id, &map_info);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("error 0x%x in pvd map lba to chunk\n", status);
                return;
            }
            fbe_cli_printf("lba: 0x%llx\n", (unsigned long long)lba);
            fbe_cli_printf("chunk index: 0x%x\n", (unsigned int)map_info.chunk_index);
            fbe_cli_printf("metadata lba: 0x%x\n", (unsigned int)map_info.metadata_pba);
            return;
        }
        else if(strcmp(*argv, "-paged_summary") == 0)
        {
            /* By default the summary gets the data from disk */
            b_force_unit_access = FBE_TRUE;
            argc--;
            argv++;

            /* Determine if FUA was requested */
            if (argc > 0)
            {
                argc--;
                argv++;
                if(strcmp(*argv, "-fua") == 0)
                {
                    /* argc should be greater than 0 here */
                    if (argc <= 0)
                    {
                        fbe_cli_error("Please provide value for -fua switch. \n\n");
                        return;
                    }
                    fua = fbe_atoh(*argv);
                    b_force_unit_access = (fua == 0) ? FBE_FALSE : FBE_TRUE;
                }
            }
            fbe_cli_pvdinfo_paged_summary(pvd_object_id, b_force_unit_access);
            return;
        }
        else if(strcmp(*argv, "-fua") == 0)
        {
            /* Only allowed with display option*/
            if (print_metadata_flag == FBE_FALSE)
            {
                fbe_cli_error("-fua is only allowed with display metadata and summary. \n\n");
                return;
            }

            /* FUA was requested */
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Please provide value for -fua switch. \n\n");
                return;
            }
            fua = fbe_atoh(*argv);
            b_force_unit_access = (fua == 0) ? FBE_FALSE : FBE_TRUE;
        }
        else if (strcmp(*argv, "-paged_set_bits") == 0   ||
                 strcmp(*argv, "-paged_clear_bits") == 0 ||
                 strcmp(*argv, "-paged_write") == 0      ||
                 strcmp(*argv, "-clear_cache") == 0         )
        {
            if (pvd_object_id == FBE_OBJECT_ID_INVALID)
            {
                fbe_cli_error("%s: Please provide object id for pvd\n\n", *argv);
                return;
            }
            fbe_cli_pvdinfo_paged_operation(pvd_object_id, argc, argv);
            return;
        }
        else if(strcmp(*argv, "-get_ssd_stats") == 0)
        {
            fbe_api_provision_drive_get_ssd_statistics_t get_stats;
            fbe_api_provision_drive_get_ssd_block_limits_t get_block_limits;

            argc--;
            argv++;

            if (pvd_object_id == FBE_OBJECT_ID_INVALID)
            {
                fbe_cli_error("%s: Please provide object id for pvd\n\n", *argv);
                return;
            }
			status = fbe_api_provision_drive_get_ssd_statistics(pvd_object_id, &get_stats);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_error("\nFailed to get SSD statistics. pvd:0x%x.\n", pvd_object_id);
                return;
            }
            fbe_cli_printf("\nSSD max erase count: 0x%llx\n", get_stats.get_ssd_statistics.max_erase_count);                
            fbe_cli_printf("SSD average erase count: 0x%llx\n", get_stats.get_ssd_statistics.average_erase_count);                
            fbe_cli_printf("SSD EOL PE cycle: 0x%llx\n", get_stats.get_ssd_statistics.eol_PE_cycles);     
            fbe_cli_printf("SSD Power on hours: 0x%llx\n\n", get_stats.get_ssd_statistics.power_on_hours);             
            
            status = fbe_api_provision_drive_get_ssd_block_limits(pvd_object_id, &get_block_limits);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_error("\nFailed to get SSD block_limits. pvd:0x%x.\n", pvd_object_id);
                return;
            }             

            fbe_cli_printf("\nSSD max transferfer length: 0x%llx\n", get_block_limits.get_ssd_block_limits.max_transfer_length);                
            fbe_cli_printf("SSD max write_same_length: 0x%llx\n", get_block_limits.get_ssd_block_limits.max_write_same_length);                
            fbe_cli_printf("SSD max unmap lba count: 0x%llx\n", get_block_limits.get_ssd_block_limits.max_unmap_lba_count);                
            fbe_cli_printf("SSD max unmap block descriptor count: 0x%llx\n", get_block_limits.get_ssd_block_limits.max_unmap_block_descriptor_count);                
            fbe_cli_printf("SSD opimal unmap granularity: 0x%llx\n", get_block_limits.get_ssd_block_limits.optimal_unmap_granularity);                
            fbe_cli_printf("SSD unmap granularity alignment: 0x%llx\n\n", get_block_limits.get_ssd_block_limits.unmap_granularity_alignment);                

            return;
        }
        else
        {
            fbe_cli_error("Invalid Arguments. \n");
            fbe_cli_printf("%s", PVD_GET_INFO_USAGE);
            return;
        }

        /* Increment the arguments */
        argc--;
        argv++;
    }
    
    if(pvd_object_id != FBE_OBJECT_ID_INVALID)
    {
        status = fbe_api_get_object_class_id(pvd_object_id,&class_id,FBE_PACKAGE_ID_SEP_0);
        if (status != FBE_STATUS_OK) 
        {
            fbe_cli_error("Failed to get object class id.The object may be SPECIALIZE\n");
            return ;
        }
        if(class_id != FBE_CLASS_ID_PROVISION_DRIVE)
        {
            fbe_cli_error("Not a provision drive object id \n");
            return ;
        }
        
        fbe_cli_display_pvd_info(pvd_object_id, FBE_FALSE);
        
        if(print_metadata_flag)
        {
            fbe_cli_display_pvd_paged_metadata(pvd_object_id, chunk_index, chunk_count, b_force_unit_access);
        }

        if(test_slf_flag)
        {
            fbe_api_provision_drive_test_slf(pvd_object_id, op);
        }
        return;
    }
    else
    {
        fbe_cli_error("Invalid Arguments. \n");
        fbe_cli_printf("%s", PVD_GET_INFO_USAGE);
        return;
    }
    
}

/*!*******************************************************************
 * @var fbe_cli_display_all_pvd_objects()
 *********************************************************************
 * @brief Function to display all pvd objects info in the system.
 *
 *
 * @return - none.  
 *
 * @author
 *  3/02/2011 - Created. Vishnu Sharma
 *********************************************************************/
void fbe_cli_display_all_pvd_objects(fbe_bool_t list)
{
    fbe_u32_t num_objects          = 0;
    fbe_object_id_t *obj_list_p    = NULL;
    fbe_u32_t index = 0;
    fbe_status_t    status         = FBE_STATUS_INVALID;
    
    
    status = fbe_api_enumerate_class(FBE_CLASS_ID_PROVISION_DRIVE, 
                                     FBE_PACKAGE_ID_SEP_0, 
                                     &obj_list_p, 
                                     &num_objects);
    if(status == FBE_STATUS_OK)
    {
        /* Iterate over each of the objects we found.
                    */
        if(list)
        {
			fbe_cli_printf(" Obj ID |  Disk  |      Drive Type      | EOL State | SLF State | Zero Chkpt | Zeroing %% \n");
			fbe_cli_printf("------------------------------------------------------------------------------------------------\n");
        }
        for ( index = 0; index < num_objects; index++)
        {
            /* Display  the provision drive info.
                for this object */
           fbe_cli_display_pvd_info(obj_list_p[index], list);
          
        }
        fbe_api_free_memory(obj_list_p);
    }
    else
    {
        fbe_cli_error("No provision drive configured in the system. \n");
    }
}

void fbe_cli_display_all_pvd_objects_block_size(void)
{
    fbe_u32_t num_objects          = 0;
    fbe_object_id_t *obj_list_p    = NULL;
    fbe_u32_t index = 0;
    fbe_status_t    status         = FBE_STATUS_INVALID;
    
    
    status = fbe_api_enumerate_class(FBE_CLASS_ID_PROVISION_DRIVE, 
                                     FBE_PACKAGE_ID_SEP_0, 
                                     &obj_list_p, 
                                     &num_objects);
    if(status == FBE_STATUS_OK)
    {
        /* Iterate over each of the objects we found.
         */

        fbe_cli_printf(" Obj ID |  Disk  |  Config  | Block Size \n");
        fbe_cli_printf("-----------------------------------------\n");

        for ( index = 0; index < num_objects; index++)
        {
            /* Display  the provision drive info.
                for this object */
           fbe_cli_display_pvd_info_block_size(obj_list_p[index]);
          
        }
        fbe_api_free_memory(obj_list_p);
    }
    else
    {
        fbe_cli_error("No provision drive configured in the system. \n");
    }
}

/*!*******************************************************************
 * @var fbe_cli_display_pvd_info()
 *********************************************************************
 * @brief Function to display pvd object info.
 *
 *
 * @return - none.  
 *
 * @author
 *  3/02/2011 - Created. Vishnu Sharma
 *********************************************************************/

void fbe_cli_display_pvd_info(fbe_object_id_t pvd_object_id, fbe_bool_t list)
{
    fbe_lba_t  zero_checkpoint;
    fbe_status_t    status         = FBE_STATUS_INVALID;
    fbe_api_provision_drive_info_t provision_drive_info;
    fbe_provision_drive_verify_report_t  out_verify_report;
    fbe_provision_drive_get_verify_status_t  out_verify_status;
    fbe_provision_drive_set_priorities_t  get_priorities;
    fbe_u16_t      zeroing_percentage;
    fbe_status_t lifecycle_status = FBE_STATUS_INVALID;
    fbe_lifecycle_state_t lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;


    status = fbe_api_provision_drive_get_info(pvd_object_id,&provision_drive_info);
    if(status == FBE_STATUS_BUSY)
    {
        lifecycle_status = fbe_api_get_object_lifecycle_state(pvd_object_id, 
                                                    &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
        if (lifecycle_status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s Get lifecycle failed: obj id: 0x%x status: 0x%x \n",
                           __FUNCTION__, pvd_object_id, lifecycle_status);
            return;
        }
        else if (lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE)
        {
            fbe_cli_printf("\nPVD is in SPECIALIZE, Object ID 0x%x\n", pvd_object_id);
            return;
        }
    }
    else if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("Provision drive object is not exist. \n");
        return;
    }
    status = fbe_api_provision_drive_get_zero_checkpoint(pvd_object_id,&zero_checkpoint);

    status = fbe_api_provision_drive_get_zero_percentage(pvd_object_id, &zeroing_percentage);

    if(list){ /* Short version */
        fbe_cli_printf("0x%-5x  %02d_%02d_%02d %20s      %-5s     %-8s    %llX      %d%%  \n",   pvd_object_id,
                                                provision_drive_info.port_number,
                                                provision_drive_info.enclosure_number,
                                                provision_drive_info.slot_number,
                                                fbe_provision_drive_trace_get_drive_type_string(provision_drive_info.drive_type),
                                                provision_drive_info.end_of_life_state ? "TRUE" : "FALSE",
                                                fbe_cli_get_pvd_slf_state_string(provision_drive_info.slf_state),
                                                zero_checkpoint, zeroing_percentage );
        return;
    }

    status = fbe_api_provision_drive_get_verify_report(pvd_object_id,FBE_PACKET_FLAG_NO_ATTRIB,&out_verify_report);
    status = fbe_api_provision_drive_get_verify_status(pvd_object_id,FBE_PACKET_FLAG_NO_ATTRIB,&out_verify_status);
    status = fbe_api_provision_drive_get_background_priorities(pvd_object_id,&get_priorities);
    
    
    fbe_cli_printf ("\n Obj ID | Disk  | Config Type   |    Lifecycle state    |          Drive Type           |    Capacity   | Serial No.\n");
    fbe_cli_printf ("----------------------------------------------------------------------------------------------------------------------\n");
    
    fbe_cli_display_hot_spare_row(pvd_object_id,&provision_drive_info);
    
       
    fbe_cli_display_pvd_sniff_verify_info(&out_verify_report,&out_verify_status);
        
    fbe_cli_printf("----------------Traffic Priorites------------------------------\n\n");
    fbe_cli_printf("Provision Drive Zero Priority is: %s\n",fbe_api_traffic_priority_to_string(get_priorities.zero_priority));
    fbe_cli_printf("Provision Drive verify Priority is: %s\n\n",fbe_api_traffic_priority_to_string(get_priorities.verify_priority));
    fbe_cli_printf("Provision Drive verify Invalidate Priority is: %s\n\n",
                   fbe_api_traffic_priority_to_string(get_priorities.verify_invalidate_priority));
    
    fbe_cli_printf("----------------Other Information------------------------------\n\n");
    fbe_cli_printf("Provision Drive Zero Chkpt: 0x%llX\n",
		   (unsigned long long)zero_checkpoint);
    fbe_cli_printf("Provision Drive Max Transfer Limit: 0x%llX\n",
		   (unsigned long long)provision_drive_info.max_drive_xfer_limit);
    fbe_cli_printf("Provision Drive End Of Life State : %s\n", 
                   provision_drive_info.end_of_life_state ? "TRUE" : "FALSE");
    fbe_cli_printf("Provision Drive Drive Fault State : %s\n", 
                   provision_drive_info.drive_fault_state ? "TRUE" : "FALSE");
    fbe_cli_printf("Provision Drive Media Error Lba : 0x%llx\n",(unsigned long long)provision_drive_info.media_error_lba);
    fbe_cli_printf("Provision Drive Zero on demand flag : %s\n\n", 
                   provision_drive_info.zero_on_demand ? "TRUE" : "FALSE");
    fbe_cli_display_metadata_element_state(pvd_object_id);
    
    fbe_cli_display_pvd_debug_flags_info(provision_drive_info.debug_flags);
    fbe_cli_display_pvd_zeroing_percentage(pvd_object_id);
    fbe_cli_display_pvd_slf_info(provision_drive_info.slf_state);
    fbe_cli_printf("Provision Drive Spin Down Qualified flag : %s\n\n", 
                   provision_drive_info.spin_down_qualified ? "TRUE" : "FALSE");
    fbe_cli_printf("Provision Drive Created after Encryption Enabled: %s\n", 
                   provision_drive_info.created_after_encryption_enabled? "TRUE ": "FALSE");
    fbe_cli_printf("Provision Drive Scrubbing in progress: %s\n", 
                   provision_drive_info.scrubbing_in_progress? "TRUE ": "FALSE");

    fbe_cli_printf ("-----------------------------------------------------------------\n\n");
}

/*!*******************************************************************************
 * @var fbe_cli_get_pvd_info_block_size()
 *********************************************************************************
 * @brief Function to get pvd object's block size.
 *
 *
 * @return - none.  
 *
 * @author
 *  6/18/2014 - Created and restructured from another function. SaranyaDevi Ganesan
 **********************************************************************************/

fbe_u32_t fbe_cli_get_pvd_info_block_size(fbe_object_id_t pvd_object_id)
{
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_api_provision_drive_info_t provision_drive_info;
    fbe_status_t lifecycle_status = FBE_STATUS_INVALID;
    fbe_lifecycle_state_t lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_u32_t block_size = 0;


    status = fbe_api_provision_drive_get_info(pvd_object_id,&provision_drive_info);
    if(status == FBE_STATUS_BUSY)
    {
        lifecycle_status = fbe_api_get_object_lifecycle_state(pvd_object_id, 
                                                    &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
        if (lifecycle_status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s Get lifecycle failed: obj id: 0x%x status: 0x%x \n",
                           __FUNCTION__, pvd_object_id, lifecycle_status);
            return 0;
        }
        else if (lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE)
        {
            fbe_cli_printf("\nPVD is in SPECIALIZE, Object ID 0x%x\n", pvd_object_id);
            return 0;
        }
    }
    else if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("Provision drive object does not exist. \n");
        return 0;
    }


    if (provision_drive_info.configured_physical_block_size == FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_520)
    {
       block_size = 520;
    }
    else if (provision_drive_info.configured_physical_block_size == FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_4160)
    {
        block_size = 4160;
    }

    return(block_size);
}
/***************************************
 * end fbe_cli_get_pvd_info_block_size()
 ***************************************/

/*!*******************************************************************
 * @var fbe_cli_display_pvd_info_block_size()
 *********************************************************************
 * @brief Function to display the provision drive's block size.
 *
 * @param pvd_object_id
 * 
 * @return - none.  
 *
 * @author
 *  6/18/2014 - Rewrote. Saranyadevi Ganesan
 *********************************************************************/
void fbe_cli_display_pvd_info_block_size(fbe_object_id_t pvd_object_id)
{
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_api_provision_drive_info_t provision_drive_info;
    fbe_status_t lifecycle_status = FBE_STATUS_INVALID;
    fbe_lifecycle_state_t lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_u32_t block_size = 0;


    status = fbe_api_provision_drive_get_info(pvd_object_id,&provision_drive_info);
    if(status == FBE_STATUS_BUSY)
    {
        lifecycle_status = fbe_api_get_object_lifecycle_state(pvd_object_id, 
                                                    &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
        if (lifecycle_status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s Get lifecycle failed: obj id: 0x%x status: 0x%x \n",
                           __FUNCTION__, pvd_object_id, lifecycle_status);
            return;
        }
        else if (lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE)
        {
            fbe_cli_printf("\nPVD is in SPECIALIZE, Object ID 0x%x\n", pvd_object_id);
            return;
        }
    }
    else if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("Provision drive object is not exist. \n");
        return;
    }

    block_size = fbe_cli_get_pvd_info_block_size(pvd_object_id);
    if(block_size == 0)
    {
        return;
    }

    fbe_cli_printf("0x%-5x  %02d_%02d_%02d   %-8s  %-5d\n",   
                   pvd_object_id,
                   provision_drive_info.port_number, provision_drive_info.enclosure_number, provision_drive_info.slot_number,
                   fbe_provision_drive_trace_get_config_type_string(provision_drive_info.config_type),
                   block_size);
}
/*******************************************
 * end fbe_cli_display_pvd_info_block_size()
 *******************************************/

/*!*******************************************************************
 * @var fbe_cli_display_pvd_sniff_verify_info()
 *********************************************************************
 * @brief Function to display pvd objects sniff verify info.
 *
 *
 * @return - none.  
 *
 * @author
 *  3/02/2011 - Created. Vishnu Sharma
 *********************************************************************/

void fbe_cli_display_pvd_sniff_verify_info(fbe_provision_drive_verify_report_t  *pvd_verify_report_p,
                                           fbe_provision_drive_get_verify_status_t  *verify_status_p)
{
    fbe_bool_t  system_sniff_verify_enabled;
    system_sniff_verify_enabled = fbe_api_system_sniff_verify_is_enabled();

    fbe_cli_printf("\n----------------Sniff Verify Report------------------------------\n\n");
    fbe_cli_printf("\nSniff Verify is %s.", (verify_status_p->verify_enable)? "Enabled":"Disabled");
    fbe_cli_printf("\nSystem sniff verify is %s.", (system_sniff_verify_enabled)? "Enabled":"Disabled");
    fbe_cli_printf("\nSniff Verify Checkpoint: 0x%llX", (unsigned long long)verify_status_p->verify_checkpoint);
    fbe_cli_printf("\nProvision Drive Exported Capacity: 0x%llX\n", (unsigned long long)verify_status_p->exported_capacity);

    
    fbe_cli_printf("\nSniff Verify Pass count:%d", pvd_verify_report_p->pass_count);
    fbe_cli_printf("\nRevision Number:%d\n", pvd_verify_report_p->revision);

    fbe_cli_printf("\n Report                |  Recoverable  | Unrecoverable |\n");
    fbe_cli_printf("--------------------------------------------------------");
    fbe_cli_printf("\n Read errors - Total   |%14d |%14d", pvd_verify_report_p->totals.recov_read_count,
        pvd_verify_report_p->totals.unrecov_read_count);
    fbe_cli_printf("\n Read errors - Current |%14d |%14d", pvd_verify_report_p->current.recov_read_count,
        pvd_verify_report_p->current.unrecov_read_count);
    fbe_cli_printf("\n Read errors - Previous|%14d |%14d\n\n", pvd_verify_report_p->previous.recov_read_count,
        pvd_verify_report_p->previous.unrecov_read_count);
     

}
/*!*******************************************************************
 * @var fbe_cli_display_pvd_debug_flags_info()
 *********************************************************************
 * @brief Function to display pvd objects debug flag info.
 *
 *
 * @return - none.  
 *
 * @author
 *  3/02/2011 - Created. Vishnu Sharma
 *********************************************************************/

void fbe_cli_display_pvd_debug_flags_info(fbe_provision_drive_debug_flags_t debug_flags)
{
    fbe_u32_t shiftBit=0, i=0;

    fbe_cli_printf("Provision Drive Debug Flag: \n");
    
    if(debug_flags == 0)
    {
        fbe_cli_printf("%s ", "FBE_PROVISION_DRIVE_DEBUG_FLAG_NONE\n");
        return;
    }

    while(shiftBit<FBE_PROVISION_DRIVE_DEBUG_FLAG_LAST)
    {
        shiftBit = BIT0 << i++;

        if (debug_flags & shiftBit)
        {
            switch(shiftBit)
            {
                case FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING:
                    fbe_cli_printf("0x%x \t General Tracing \n", shiftBit);
                    break;

                case FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING:
                    fbe_cli_printf("0x%x \t Background Zeroing Tracing \n", shiftBit);
                    break;

                case FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING:
                    fbe_cli_printf("0x%x \t User Zero Tracing \n", shiftBit);
                    break;

                case FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING:
                    fbe_cli_printf("0x%x \t Zeroing On Demand Tracing \n", shiftBit);
                    break;

                case FBE_PROVISION_DRIVE_DEBUG_FLAG_VERIFY_TRACING:
                    fbe_cli_printf("0x%x \t Verify Tracing \n", shiftBit);
                    break;

                case FBE_PROVISION_DRIVE_DEBUG_FLAG_METADATA_TRACING:
                    fbe_cli_printf("0x%x \t Metadata Tracing \n", shiftBit);
                    break;

                case FBE_PROVISION_DRIVE_DEBUG_FLAG_LOCK_TRACING:
                    fbe_cli_printf("0x%x \t File Lock Tracing \n", shiftBit);
                    break;
            }
        }
    }
}

/*!*******************************************************************
 * @var fbe_cli_display_pvd_paged_metadata()
 *********************************************************************
 * @brief Function to display the provision drive's paged metadata.
 *
 * @param pvd_object_id
 * @param chunk_index
 * @param chunk_count
 * @param b_force_unit_access - FBE_TRUE - Data must come from disk
 * 
 * @return - none.  
 *
 * @author
 *  6/4/2012 - Rewrote. Rob Foley
 *********************************************************************/
void fbe_cli_display_pvd_paged_metadata(fbe_object_id_t pvd_object_id,
                                        fbe_chunk_index_t chunk_offset,
                                        fbe_chunk_count_t chunk_count,
                                        fbe_bool_t b_force_unit_access)
{
    fbe_api_provision_drive_info_t pvd_info;
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_u32_t index;
    fbe_chunk_count_t total_chunks;
    fbe_chunk_index_t chunks_remaining;
    fbe_chunk_index_t current_chunks;
    fbe_u32_t chunks_per_request;
    fbe_provision_drive_paged_metadata_t *current_paged_p = NULL;
    fbe_provision_drive_paged_metadata_t *paged_p = NULL;

    /* Get the information about the capacity so we can calculate number of chunks.
     */
    status = fbe_api_provision_drive_get_info(pvd_object_id, &pvd_info);

    if (status != FBE_STATUS_OK) 
    { 
        fbe_cli_printf("%s: rg get info failed, obj 0x%x, status: 0x%x\n", 
                        __FUNCTION__, pvd_object_id, status);
        return ; 
    }

    total_chunks = pvd_info.total_chunks;

    if (chunk_offset > total_chunks)
    {
        fbe_cli_printf("%s: input chuck offset 0x%llx is greater than total chunks 0x%x\n", 
                        __FUNCTION__, (unsigned long long)chunk_offset, total_chunks);
        return; 
    }
    status = fbe_api_provision_drive_get_max_read_chunks(&chunks_per_request);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: get max paged failed, obj 0x%x, status: 0x%x\n", 
                        __FUNCTION__, pvd_object_id, status);
        return; 
    }
    paged_p = fbe_api_allocate_memory(chunks_per_request * sizeof(fbe_provision_drive_paged_metadata_t));

    chunks_remaining = FBE_MIN(chunk_count, (total_chunks - chunk_offset));

    /* Loop over all the chunks, and sum up the bits across all chunks. 
     * We will fetch as many chunks as we can per request (chunks_per_request).
     */
    while (chunks_remaining > 0)
    {
        current_chunks = FBE_MIN(chunks_per_request, chunks_remaining);
        if (current_chunks > FBE_U32_MAX)
        {
            fbe_api_free_memory(paged_p);
            fbe_cli_printf("%s: current_chunks: %d > FBE_U32_MAX\n", __FUNCTION__, (int)current_chunks);
            return ;
        }
        status = fbe_api_provision_drive_get_paged_metadata(pvd_object_id,
                                                            chunk_offset, current_chunks, paged_p,
                                                            b_force_unit_access);
        if (status != FBE_STATUS_OK) 
        { 
            fbe_api_free_memory(paged_p);
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: get paged failed, obj 0x%x, status: 0x%x\n", 
                          __FUNCTION__, pvd_object_id, status);
            return; 
        }

        /* For each chunk in this request, do display.
         */
        fbe_cli_printf("0x%-8llx | Valid | Need Zero | User Zero | Consumed | Unused |\n",  chunk_offset);
        
        for ( index = 0; index < current_chunks; index++)
        {
            current_paged_p = &paged_p[index];

            fbe_cli_printf("  %8x |   %3d |     %3d   |     %3d   |    %3d   | 0x%llx\n", 
                           index, 
                           current_paged_p->valid_bit,
                           current_paged_p->need_zero_bit,
                           current_paged_p->user_zero_bit,
                           current_paged_p->consumed_user_data_bit,
                           (unsigned long long)current_paged_p->unused_bit);
        }
        chunks_remaining -= current_chunks;
        chunk_offset += current_chunks;
    }

    fbe_api_free_memory(paged_p);
}
/**************************************
 * end fbe_cli_display_pvd_paged_metadata()
 **************************************/

/*!*******************************************************************
 * @var fbe_cli_display_pvd_zeroing_percentage()
 *********************************************************************
 * @brief Function to display pvd objects zeroing percentage.
 *
 *
 * @return - none.
 *
 * @author
 *  8/31/2011 - Created. Jian Gao
 *********************************************************************/
void fbe_cli_display_pvd_zeroing_percentage(fbe_object_id_t pvd_object_id)
{
    fbe_status_t    status      = FBE_STATUS_INVALID;   
    fbe_u16_t      zeroing_percentage;

    status = fbe_api_provision_drive_get_zero_percentage(pvd_object_id, &zeroing_percentage);

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("Failed to return drive zeroing percentage. \n");
        
    }else
    {
        fbe_cli_printf("\nProvision Drive Zeroing Percentage: %d\n", zeroing_percentage);
    }
}

/*!*******************************************************************
 * @var fbe_cli_remove_zero_checkpoint()
 *********************************************************************
 * @brief Function to invalidate the zero checkpoint of a given PVD object
 *
 *
 * @return - none.  
 *
 * @author
 *  3/02/2011 - Created. Ryan Gadsby
 *********************************************************************/
void fbe_cli_remove_zero_checkpoint(fbe_s32_t argc, fbe_s8_t ** argv){
    fbe_status_t    status = FBE_STATUS_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_char_t *tmp_ptr = *argv;
    fbe_u32_t num_objects          = 0;
    fbe_object_id_t *obj_list_p    = NULL;
    fbe_u32_t index = 0;    
        
    if(0 == argc) {
        fbe_cli_error("Please provide a PVD object ID. \n");
        fbe_cli_printf("%s", REMOVE_ZERO_CHECKPOINT_USAGE);
        return;
    }
    
    if(1 == argc){	 
        if((strcmp(*argv, "-help") == 0) ||(strcmp(*argv, "-h") == 0)) { 
            /* If they are asking for help, just display help and exit. */
            fbe_cli_printf("%s", REMOVE_ZERO_CHECKPOINT_USAGE);
            return;
        }
        if((strcmp(*argv, "-all") == 0)){
            status = fbe_api_enumerate_class(FBE_CLASS_ID_PROVISION_DRIVE, 
                                             FBE_PACKAGE_ID_SEP_0, 
                                             &obj_list_p, 
                                             &num_objects);
            if(status == FBE_STATUS_OK) {
                /* Iterate over each of the objects we found. */
                for ( index = 0; index < num_objects; index++) {
                     status = fbe_api_provision_drive_set_zero_checkpoint(obj_list_p[index], FBE_LBA_INVALID);
                     if (status == FBE_STATUS_OK) {
                         fbe_cli_printf("Operation Completed Successfully for id 0x%X\n", obj_list_p[index]);
                     } else {
                         fbe_cli_printf("Operation failed for id 0x%X - status: %d\n",obj_list_p[index], status);
                     }
                }
                fbe_api_free_memory(obj_list_p);
            } else {
                fbe_cli_error("No provision drive configured in the system. \n");
            }
            return;
        }
    }

    /* Chop off "0x" */
    if(('0' == *tmp_ptr) && 
             (('x' == *(tmp_ptr+1)) || ('X' == *(tmp_ptr+1))))
    {
        tmp_ptr += 2;
        if(!strcmp(tmp_ptr,""))
        {
            fbe_cli_error("Invalid object id. Please provide correct object id in hex format. \n");
            return;
        }
     }
     else
     {
         fbe_cli_error("Invalid object id. Please provide correct object id in hex format. \n");
         return;
     }

     pvd_object_id = fbe_atoh(tmp_ptr);

     //set checkpoint to invalid
     status = fbe_api_provision_drive_set_zero_checkpoint(pvd_object_id, FBE_LBA_INVALID);
     if (status == FBE_STATUS_OK) {
         fbe_cli_printf("Operation Completed Successfully\n");
     }
     else{
         fbe_cli_printf("Operation failed - status: %d\n", status);
     }
}

/*!*******************************************************************
 * fbe_cli_display_pvd_slf_info()
 *********************************************************************
 * @brief Function to display pvd slf information.
 *
 *
 * @return - none.  
 *
 * @author
 *  06/20/2012 - Created. Lili Chen
 *********************************************************************/
void fbe_cli_display_pvd_slf_info(fbe_provision_drive_slf_state_t slf_state)
{

    fbe_cli_printf("Provision Drive Link Fault State: ");
    
    switch(slf_state)
    {
        case FBE_PROVISION_DRIVE_SLF_NONE:
            fbe_cli_printf("NONE \n");
            break;

        case FBE_PROVISION_DRIVE_SLF_THIS_SP:
            fbe_cli_printf("THIS SP \n");
            break;

        case FBE_PROVISION_DRIVE_SLF_PEER_SP:
            fbe_cli_printf("PEER SP \n");
            break;

        case FBE_PROVISION_DRIVE_SLF_BOTH_SP:
            fbe_cli_printf("BOTH SPS \n");
            break;
    }
}

/*!*******************************************************************
 * fbe_cli_get_pvd_slf_state_string()
 *********************************************************************
 * @brief
  *  Return a string for PVD single loop failure state.
  *
  * @param slf_state - slf state type for the object.
  *
  * @return - fbe_char_t * string that represents slf state.
  *
  * @author
  *  09/29/2012 - Created. Wenxuan Yin
  *
 *********************************************************************/
fbe_char_t * fbe_cli_get_pvd_slf_state_string(fbe_provision_drive_slf_state_t slf_state)
{
    fbe_char_t *pvd_slf_state_string;

    switch(slf_state)
    {
        case FBE_PROVISION_DRIVE_SLF_NONE:
            pvd_slf_state_string = "NONE";
            break;

        case FBE_PROVISION_DRIVE_SLF_THIS_SP:
            pvd_slf_state_string = "THIS SP";
            break;

        case FBE_PROVISION_DRIVE_SLF_PEER_SP:
            pvd_slf_state_string = "PEER SP";
            break;

        case FBE_PROVISION_DRIVE_SLF_BOTH_SP:
            pvd_slf_state_string = "BOTH SPs";
            break;

        default:
            break;
    }
    return (pvd_slf_state_string);
}


/*!*******************************************************************
 * @var fbe_cli_cmd_clear_pvd_drive_states()
 *********************************************************************
 * @brief Function to clear persisted drive states
 * 
 * @return - none.  
 *
 * @author
 *  07/11/2012 - Created. kothal
 *  06/23/2014 - Rewrote 'set_block_size' option. SaranyaDevi Ganesan
 *********************************************************************/
void fbe_cli_cmd_clear_pvd_drive_states(fbe_s32_t argc, fbe_s8_t ** argv)
{
    
    fbe_status_t    status = FBE_STATUS_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;    
    fbe_api_provision_drive_info_t provision_drive_info;
    fbe_api_provision_drive_get_swap_pending_t get_swap_pending_info;
     
    if(0 == argc)
    {
        fbe_cli_error("\nToo few arguments. \n");
        fbe_cli_printf("%s", CLEAR_DRIVE_FAULT_USAGE);
        return;
    }

    if((1 == argc) && 
        ((strcmp(*argv, "-help") == 0) ||(strcmp(*argv, "-h") == 0))) 
    {
        /* If they are asking for help, just display help and exit. */
        fbe_cli_printf("%s", CLEAR_DRIVE_FAULT_USAGE);
        return;
    }
     
    while (argc > 0)
    {
        /* Object ID option */
        if(strcmp(*argv, "-object_id") == 0)
        {
            /* Get the object id from command line. */
            argc--;
            argv++;            
        
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("\nPlease provide value for -object_id switch. \n");
                return;
            } 
            
            /* The "-object_id all" will clear for all PVS. */
            if(!strcmp(*argv,"all"))
            {
                pvd_object_id = FBE_OBJECT_ID_INVALID;
            }
            else
            {
                /* Get the PVD object ID from the argument */
                fbe_char_t *tmp_ptr = *argv;
                
                if(('0' == *tmp_ptr) && 
                        (('x' == *(tmp_ptr+1)) || ('X' == *(tmp_ptr+1))))
                {
                    tmp_ptr += 2;
            
                    if(!strcmp(tmp_ptr,""))
                    {
                        fbe_cli_error("\nInvalid object id. Please provide correct object id in hex format. \n");
                        return;
                    }
                }
                else
                {
                    fbe_cli_error("\nInvalid argument object id. Please provide correct object id in hex format. \n");
                    return;
                }
                
                pvd_object_id = fbe_atoh(tmp_ptr);
            
                if(pvd_object_id > FBE_MAX_SEP_OBJECTS)
                {
                    fbe_cli_error("\nInvalid PVD Object Id 0x%x .\n", pvd_object_id);            
                    return;
                }
            }
            
            /* Increment the arguments */
            argc--;
            argv++;            
        }
        else if (strcmp(*argv, "-bes") == 0)
        {
            argc--;
            argv++;
            status = fbe_cli_pvd_parse_bes_opt(*argv, &pvd_object_id);
            if (status != FBE_STATUS_OK)
            {
                /* the previous called function prints the error msgs.*/
                return;
            }
            argc--;
            argv++;
        }


        if (argc == 0) 
        {
            fbe_cli_error("\nPlease provide the next switch. \n");
            fbe_cli_printf("%s", CLEAR_DRIVE_FAULT_USAGE);
            return;            
        }

        /* -drive_fault operation <clear/status> for individual PVD object */
        if(strcmp(*argv, "-drive_fault") == 0)
        {
            /* Get the object id from command line. */
            argc--;
            argv++;
            
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("\nPlease provide correct arguments. \n");
                return;
            }

            /* clear the drive fault state for individual PVD object */
            if(strcmp(*argv, "clear")==0)
            {
                if (pvd_object_id != FBE_OBJECT_ID_INVALID) 
                {
                    status = fbe_api_provision_drive_clear_swap_pending(pvd_object_id);
                    if (status != FBE_STATUS_OK)
                    {
                        fbe_cli_error( "\nFailed to clear Swap Pending NP flag for PVD: 0x%x.\n", pvd_object_id);
                        return;
                    }
                    status = fbe_api_provision_drive_clear_drive_fault(pvd_object_id);
                    if (status != FBE_STATUS_OK)
                    {
                        fbe_cli_error( "%s: failed to clear drive fault state for PVD: 0x%x.\n", __FUNCTION__, pvd_object_id);
                        return;
                    }

                    fbe_cli_printf("\nSuccessfully cleared drive fault state for PVD: 0x%x\n\n", pvd_object_id);
                    return;                    
                }
                else
                {
                    fbe_cli_error( "Please provide valid -object_id.  The '-object_id all' can ONLY be used with -all_states or -set_block_size option.\n");

                    /* If they are asking for help, just display help and exit. */
                    fbe_cli_printf("%s", CLEAR_DRIVE_FAULT_USAGE);
                    return;
                }
            }

            /* get the drive fault state of the individual PVD object */
            if(strcmp(*argv, "status")==0)
            {
                if (pvd_object_id != FBE_OBJECT_ID_INVALID) 
                {
                    status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
                    if(status != FBE_STATUS_OK)
                    {
                        fbe_cli_error("\nFailed to get PVD Info for object ID: 0x%x. \n", pvd_object_id);
                        return;
                    }
    
                    fbe_cli_printf("\nPVD Drive Fault State : %s\n", 
                                   provision_drive_info.drive_fault_state ? "TRUE" : "FALSE");                
                    return;
                }
                else
                {
                    fbe_cli_error( "Please provide valid -object_id.  The '-object_id all' can ONLY be used with -all_states or -set_block_size option.\n");

                    /* If they are asking for help, just display help and exit. */
                    fbe_cli_printf("%s", CLEAR_DRIVE_FAULT_USAGE);
                    return;
                }
            }
            else 
            {
                fbe_cli_error("\nPlease provide correct arguments. \n");
                fbe_cli_printf("%s", CLEAR_DRIVE_FAULT_USAGE);
                return;
            }
        }
        /* -eol operation <clear/status> for individual PVD object */
        else if(strcmp(*argv, "-eol") == 0)
        {
            /* Get the object id from command line. */
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("\nPlease provide correct arguments. \n");
                return;
            }
            /* clear End of Life State for individual PVD object */
            if(strcmp(*argv, "clear")==0)
            {
                if (pvd_object_id != FBE_OBJECT_ID_INVALID) 
                {
                    status = fbe_api_provision_drive_clear_swap_pending(pvd_object_id);
                    if (status != FBE_STATUS_OK)
                    {
                        fbe_cli_error( "\nFailed to clear Swap Pending NP flag for PVD: 0x%x.\n", pvd_object_id);
                        return;
                    }
                    status = fbe_api_provision_drive_clear_eol_state(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
                    if (status != FBE_STATUS_OK)
                    {
                        fbe_cli_error( "%s: failed to clear End of Life State for PVD: 0x%x.\n", __FUNCTION__, pvd_object_id);
                        return;
                    }
                    fbe_cli_printf("\nSuccessfully cleared End of Life State for PVD: 0x%x.\n\n", pvd_object_id);
                    return;
                }
                else
                {
                    fbe_cli_error( "Please provide valid -object_id.  The '-object_id all' can ONLY be used with -all_states or -set_block_size option.\n");

                    /* If they are asking for help, just display help and exit. */
                    fbe_cli_printf("%s", CLEAR_DRIVE_FAULT_USAGE);
                    return;
                }
            }

            /* Set End of Life State for individual PVD object */
            if(strcmp(*argv, "set")==0)
            {
                if (pvd_object_id != FBE_OBJECT_ID_INVALID) 
                {
                    status = fbe_api_provision_drive_set_eol_state(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
                    if (status != FBE_STATUS_OK)
                    {
                        fbe_cli_error( "%s: failed to Set End of Life State for PVD: 0x%x.\n", __FUNCTION__, pvd_object_id);
                        return;
                    }
                    fbe_cli_printf("\nSuccessfully Set End of Life State for PVD: 0x%x.\n\n", pvd_object_id);
                    return;
                }
                else
                {
                    fbe_cli_error( "Please provide valid -object_id.  The '-object_id all' can ONLY be used with -all_states or -set_block_size option.\n");

                    /* If they are asking for help, just display help and exit. */
                    fbe_cli_printf("%s", CLEAR_DRIVE_FAULT_USAGE);
                    return;
                }
            }

            /* get the End of Life state of the individual PVD object */
            if(strcmp(*argv, "status")==0)
            {
                if (pvd_object_id != FBE_OBJECT_ID_INVALID) 
                {
                    status = fbe_api_provision_drive_get_info(pvd_object_id,&provision_drive_info);
                     if(status != FBE_STATUS_OK)
                    {
                        fbe_cli_error("\nFailed to get PVD info for object ID: 0x%x. \n", pvd_object_id);
                        return;
                    }   
                    fbe_cli_printf("\nProvision Drive End of Life State: %s\n", 
                                   provision_drive_info.end_of_life_state ? "TRUE" : "FALSE");                
                    return;
                }
                else
                {
                    fbe_cli_error( "Please provide valid -object_id.  The '-object_id all' can ONLY be used with -all_states or -set_block_size option.\n");

                    /* If they are asking for help, just display help and exit. */
                    fbe_cli_printf("%s", CLEAR_DRIVE_FAULT_USAGE);
                    return;
                }
            }
            else 
            {
                fbe_cli_error("\nPlease provide correct arguments. \n");
                fbe_cli_printf("%s", CLEAR_DRIVE_FAULT_USAGE);
                return;
            }    
        }
        else if(strcmp(*argv, "-swap_pending") == 0)
        {
            /* Get the object id from command line. */
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("\nPlease provide correct arguments. \n");
                return;
            }
            /* clear swap pending for individual PVD object */
            if(strcmp(*argv, "clear")==0)
            {
                if (pvd_object_id != FBE_OBJECT_ID_INVALID) 
                {
                    status = fbe_api_provision_drive_clear_swap_pending(pvd_object_id);
                    if (status != FBE_STATUS_OK)
                    {
                        fbe_cli_error( "%s: failed to clear Swap Pending NP flag for PVD: 0x%x.\n", __FUNCTION__, pvd_object_id);
                        return;
                    }
                    fbe_cli_printf("\nSuccessfully cleared Swap Pending NP flag for PVD: 0x%x.\n\n", pvd_object_id);
                    return;
                }
                else
                {
                    fbe_cli_error( "Please provide valid -object_id.  The '-object_id all' can ONLY be used with -all_states or -set_block_size option.\n");

                    /* If they are asking for help, just display help and exit. */
                    fbe_cli_printf("%s", CLEAR_DRIVE_FAULT_USAGE);
                    return;
                }
            }

            /* Logically mark individual PVD swap pending */
            if(strcmp(*argv, "set")==0)
            {
                if (pvd_object_id != FBE_OBJECT_ID_INVALID) 
                {
                    status = fbe_api_provision_drive_set_swap_pending(pvd_object_id);
                    if (status != FBE_STATUS_OK)
                    {
                        fbe_cli_error( "%s: failed to Set Swap Pending NP flag for PVD: 0x%x.\n", __FUNCTION__, pvd_object_id);
                        return;
                    }
                    fbe_cli_printf("\nSuccessfully Set Swap Pending NP flag for PVD: 0x%x.\n\n", pvd_object_id);
                    return;
                }
                else
                {
                    fbe_cli_error( "Please provide valid -object_id.  The '-object_id all' can ONLY be used with -all_states or -set_block_size option.\n");

                    /* If they are asking for help, just display help and exit. */
                    fbe_cli_printf("%s", CLEAR_DRIVE_FAULT_USAGE);
                    return;
                }
            }

            /* get the End of Life state of the individual PVD object */
            if(strcmp(*argv, "status")==0)
            {
                if (pvd_object_id != FBE_OBJECT_ID_INVALID) 
                {
                    status = fbe_api_provision_drive_get_swap_pending(pvd_object_id, &get_swap_pending_info);
                    if(status != FBE_STATUS_OK)
                    {
                        fbe_cli_error("\nFailed to get Swap Pending info for object ID: 0x%x. \n", pvd_object_id);
                        return;
                    }   
                    fbe_cli_printf("\nProvision Drive Swap Pending Reason: %d\n", 
                                   get_swap_pending_info.get_swap_pending_reason);
                    return;
                }
                else
                {
                    fbe_cli_error( "Please provide valid -object_id.  The '-object_id all' can ONLY be used with -all_states or -set_block_size option.\n");

                    /* If they are asking for help, just display help and exit. */
                    fbe_cli_printf("%s", CLEAR_DRIVE_FAULT_USAGE);
                    return;
                }
            }
            else 
            {
                fbe_cli_error("\nPlease provide correct arguments. \n");
                fbe_cli_printf("%s", CLEAR_DRIVE_FAULT_USAGE);
                return;
            }    
        }
        /* -all_states operation <clear/status> for individual/all PVD objects */
        else if(strcmp(*argv, "-all_states") == 0)
        {
            /* Get the object id from command line. */
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("\nPlease provide correct arguments. \n");
                return;
            }

            /* clear both EOL and drive fault states */
            if(strcmp(*argv, "clear")==0)
            {
                /* clear both EOL and drive fault states for individual PVD object */
                if (pvd_object_id != FBE_OBJECT_ID_INVALID) 
                {
                    fbe_cli_printf("\nClearing Swap Pending NP flag for PVD: 0x%x...\n\n", pvd_object_id);
                    status = fbe_api_provision_drive_clear_swap_pending(pvd_object_id);
                    if (status != FBE_STATUS_OK)
                    {
                        fbe_cli_error( "\nFailed to clear Swap Pending NP flag for PVD: 0x%x.\n", pvd_object_id);
                        return;
                    }
                    fbe_cli_printf("\nSuccessfully cleared Swap Pending NP flag for PVD: 0x%x.\n\n", pvd_object_id);

                    fbe_cli_printf("\nClearing eol State for PVD: 0x%x...\n\n", pvd_object_id);
                    status = fbe_api_provision_drive_clear_eol_state(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
                    if (status != FBE_STATUS_OK)
                    {
                        fbe_cli_error( "\nFailed to clear End of Life State for PVD: 0x%x.\n", pvd_object_id);
                        return;
                    }
                    fbe_cli_printf("\nSuccessfully cleared End of Life State for PVD: 0x%x.\n\n", pvd_object_id);

                    fbe_cli_printf("\nClearing Drive Fault State for PVD: 0x%x...\n\n", pvd_object_id);
                    status = fbe_api_provision_drive_clear_drive_fault(pvd_object_id);
                    if (status != FBE_STATUS_OK)
                    {
                        fbe_cli_error("\nFailed to clear drive fault state for PVD: 0x%x.\n", pvd_object_id);
                        return;
                    }
                    fbe_cli_printf("\nSuccessfully cleared drive fault state for PVD: 0x%x\n\n", pvd_object_id);
                    return;                    
                }
                /* clear both EOL and drive fault states for all PVD objects */
                else
                {
                    /* remove all PVDs EOL and drive fault states */
                    status = fbe_api_provision_drive_clear_all_pvds_drive_states();
                    if (status != FBE_STATUS_OK)
                    {
                        fbe_cli_error("\nFailed to clear Swap Pending, EOL and Drive Fault states for all PVDs.\n\n");
                        return;
                    }

                    fbe_cli_printf("\nSuccessfully cleared Swap Pending, EOL and Drive Fault states for all PVDs.\n\n");
                    return;                    

                }
            }

            /* get both EOL and Drive fault states for individual PVD object */
            if(strcmp(*argv, "status")==0)
            {
                if (pvd_object_id != FBE_OBJECT_ID_INVALID) 
                {
                    status = fbe_api_provision_drive_get_info(pvd_object_id,&provision_drive_info);
                    if(status != FBE_STATUS_OK)
                    {
                        fbe_cli_error("\nFailed to get PVD info for object ID: 0x%x. \n", pvd_object_id);
                        return;
                    }
    
                    fbe_cli_printf("\nPVD End of Life State: %s\n", 
                                   provision_drive_info.end_of_life_state ? "TRUE" : "FALSE");                
                    fbe_cli_printf("PVD Drive Fault State: %s\n", 
                                   provision_drive_info.drive_fault_state ? "TRUE" : "FALSE");                
                    return;
                }
                else
                {
                    fbe_cli_printf("\nDoes not support for the '-object_id all' option.  Please use 'pvdinfo' to get all PVDs info.\n");
                    return;
                }
            }
            else 
            {
                fbe_cli_error("\nPlease provide correct arguments. \n");
                fbe_cli_printf("%s", CLEAR_DRIVE_FAULT_USAGE);
                return;
            }    
            
        }
        else if(strcmp(*argv, "-set_block_size") == 0)
        {
            fbe_u32_t blksz;
            fbe_u32_t pvd_current_blksz = 0;
            fbe_provision_drive_configured_physical_block_size_t block_size;
            fbe_u32_t num_objects = 0;
            fbe_object_id_t *obj_list = NULL;
            fbe_u32_t index = 0;
            fbe_bool_t is_Complete = TRUE;

            argc--;
            argv++;

            blksz = (fbe_u32_t)strtoul(*argv,NULL,0);
            if (blksz == 520)
            {
                block_size = FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_520;
            }
            else if (blksz == 4160)
            {
                block_size = FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_4160;
            }
            else
            {
                fbe_cli_error("\nUnsupported block size. \n");
                return;
            }
            
            if (pvd_object_id != FBE_OBJECT_ID_INVALID)
            {
                status = fbe_api_job_service_update_provision_drive_block_size(pvd_object_id, block_size);
                if(status != FBE_STATUS_OK)
                {
                    fbe_cli_error("\nFailed to set block size. pvd:0x%x. %s \n", pvd_object_id, fbe_convert_configured_physical_block_size_to_string(block_size));
                    return;
                }
                return;
            }
            else
            {
                 /* Get all the provision drive objects*/
                status = fbe_api_enumerate_class(FBE_CLASS_ID_PROVISION_DRIVE, 
                                     FBE_PACKAGE_ID_SEP_0, 
                                     &obj_list, 
                                     &num_objects);
                if(status == FBE_STATUS_OK)
                {
                    /* Iterate over each of the objects we found.*/
                    for ( index = 0; index < num_objects; index++)
                    {
                        pvd_current_blksz = fbe_cli_get_pvd_info_block_size(obj_list[index]);

                        if (pvd_current_blksz == 520 || pvd_current_blksz == 4160)
                        {
                            if (obj_list[index] > FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_LAST &&
                                pvd_current_blksz != blksz)
                            {
                                status = fbe_api_job_service_update_provision_drive_block_size(obj_list[index], block_size);
                                if(status != FBE_STATUS_OK)
                                {
                                    /*Partial set of drives configured with blocksize; better than returning after one error*/
                                    fbe_cli_error("\nFailed to set block size. pvd:0x%x. %s \n", obj_list[index], fbe_convert_configured_physical_block_size_to_string(block_size));
                                    is_Complete = FALSE;
                                }
                            }
                        }
                        else
                        {
                            fbe_cli_error("\nUnsupported block size. \n");
                            return;
                        }
                    }
                    fbe_api_free_memory(obj_list);
                }
                else
                {
                    fbe_cli_error("No provision drive configured in the system. \n");
                }
                
                /*Print error or success messages*/
                if(!is_Complete)
                {
                    fbe_cli_error("\nFailed to set block size for all provision drives. Look above log for individual drive errors.\n\n");
                    return;
                }
                else
                {
                    fbe_cli_printf("\nSuccessfully set block size for all provision drives.\n\n");
                    return;
                }
            }
        }
        else
        {
            fbe_cli_error("\nNeed to provide the command option. \n");
            fbe_cli_printf("%s", CLEAR_DRIVE_FAULT_USAGE);
            return;
        }
    }    
} /* end of fbe_cli_cmd_clear_pvd_drive_states() */

void fbe_cli_sleep_at_prompt_for_twenty_seconds(fbe_s32_t argc, fbe_s8_t ** argv){
    fbe_api_sleep(20000);
}
/*!*******************************************************************
 * @var fbe_cli_pvd_time_threshold()
 *********************************************************************
 * @brief Function to invalidate the zero checkpoint of a given PVD object
 *
 *
 * @return - none.  
 *
 * @author
 *  7/18/2012 - Created. Yang Zhang
 *********************************************************************/
void fbe_cli_pvd_time_threshold(fbe_s32_t argc, fbe_s8_t ** argv){
        
    if(0 == argc) {
        fbe_cli_printf("%s", PVD_DESTROY_TIME_THRESHOLD_USAGE);
        return;
    }
    
    if(argc >= 1){	 
        if((strcmp(*argv, "-help") == 0) ||(strcmp(*argv, "-h") == 0)) { 
            /* If they are asking for help, just display help and exit. */
            fbe_cli_printf("%s", PVD_DESTROY_TIME_THRESHOLD_USAGE);
            return;
        }
        else if((strcmp(*argv, "-get") == 0)){
            fbe_cli_get_system_timethreshold();
            return;
        }
        else if((strcmp(*argv, "-set") == 0)){
            argc--;
            argv++;
            if(argc == 0 || *argv == NULL){
                fbe_cli_error("Please provide the tiem threshold in minutes! \n");
                return;
            }else{
                fbe_cli_set_system_timethreshold((fbe_u64_t)fbe_atoi(*argv));
                return;
            }
        }
        else {
            fbe_cli_error("Invalid input opetion! \n");
            return;
        }
    }

}

static void fbe_cli_get_system_timethreshold(void)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_system_time_threshold_info_t            get_timethreshold ;
    status = fbe_api_get_system_time_threshold_info(&get_timethreshold);
    if(FBE_STATUS_OK != status)
    {
        fbe_cli_printf("\nFailed to get System time threshold for destroying PVD\n");
        return;
    }
    fbe_cli_printf("System time threshold for destroying PVD: %d minutes\n", (int)get_timethreshold.time_threshold_in_minutes);
   
    return;
}

static void fbe_cli_set_system_timethreshold(fbe_u64_t timethreshold_in_minutes)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_system_time_threshold_info_t            time_threshold;

    time_threshold.time_threshold_in_minutes = timethreshold_in_minutes;

    fbe_cli_printf("Write System time threshold for destroying PVD in database service: %d minutes\n", (int)timethreshold_in_minutes);
    status = fbe_api_set_system_time_threshold_info(&time_threshold);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error ("%s:Failed to set System time threshold for destroying PVD\n", 
            __FUNCTION__);
        return;
    }
    else
    {
        fbe_cli_printf("Set System time threshold for destroying PVD successfully\n");
    }
    return;
}

/*!**************************************************************
 *  fbe_cli_pvdinfo_paged_summary()
 ****************************************************************
 *
 * @brief   Display the paged metadata.
 *
 * @param   pvd object_id
 * @param   b_force_unit_access - FBE_TRUE - Must get data from disk
 *
 * @return  None.
 *
 * @author
 *  4/9/2013 - Created. Rob Foley
 *
 ****************************************************************/
static void fbe_cli_pvdinfo_paged_summary(fbe_object_id_t pvd_object_id, fbe_bool_t b_force_unit_access)
{
    fbe_u32_t index;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_provision_drive_get_paged_info_t paged_info;

    status = fbe_api_provision_drive_get_paged_bits(pvd_object_id, &paged_info, b_force_unit_access);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("pvd_info error %d on get paged bits\n", status);
        return;
    }
    fbe_cli_printf("chunk_count:        0x%llx\n", (unsigned long long)paged_info.chunk_count);
    fbe_cli_printf("num_valid_chunks:   0x%llx\n", paged_info.num_valid_chunks);
    fbe_cli_printf("num_nz_chunks:      0x%llx\n", paged_info.num_nz_chunks);
    fbe_cli_printf("num_uz_chunks:      0x%llx\n", paged_info.num_uz_chunks);
    fbe_cli_printf("num_cons_chunks:    0x%llx\n", paged_info.num_cons_chunks);
    fbe_cli_printf("\n");
    fbe_cli_printf("valid nz uz cons: count\n");

    /* Valid set first.
     */
    for ( index = 0; index < FBE_API_PROVISION_DRIVE_BIT_COMBINATIONS; index++)
    {
        if (index & 1)
        {
            fbe_cli_printf(" %d    %d  %d    %d      0x%lld\n",
                           (index & 1), /* Is valid set ? */
                           (index & (1 << 1)) ? 1 : 0,  /* Is nz set? */
                           (index & (1 << 2)) ? 1 : 0,  /* Is uz set? */
                           (index & (1 << 3)) ? 1 : 0,  /* Is cons set? */
                            paged_info.bit_combinations_count[index]);
        }
    }

    /* Valid not set last.
     */
    for ( index = 0; index < FBE_API_PROVISION_DRIVE_BIT_COMBINATIONS; index++)
    {
        if ((index & 1) == 0)
        {
            fbe_cli_printf(" %d    %d  %d    %d      %lld\n",
                           (index & 1), /* Is valid set ? */
                           (index & (1 << 1)) ? 1 : 0,  /* Is nz set? */
                           (index & (1 << 2)) ? 1 : 0,  /* Is uz set? */
                           (index & (1 << 3)) ? 1 : 0,  /* Is cons set? */
                            paged_info.bit_combinations_count[index]);
        }
    }
    return;
}
/******************************************
 * end fbe_cli_pvdinfo_paged_summary()
 ******************************************/
static void fbe_cli_pvdinfo_paged_operation_usage(const char *name)
{
    if (strcmp(name, "-clear_cache") == 0) {
        fbe_cli_printf("Usage: %s \n",
                       name);
    } else {
        fbe_cli_printf("Usage: %s <chunk_offset> <chunk_count>"
                       " -valid <v> -nz <v> -uz <v> -cons <v> -reserved <v>\n",
                       name);
    }
}

static fbe_status_t fbe_cli_pvdinfo_paged_check_valid(fbe_object_id_t pvd,
                                                      fbe_cli_paged_change_t *change)
{
    fbe_api_provision_drive_info_t pvd_info;
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_chunk_count_t total_chunks;
    fbe_chunk_index_t chunk_offset;
    fbe_chunk_count_t chunk_count;

    /* Get the information about the capacity so we can calculate number of chunks.
     */
    status = fbe_api_provision_drive_get_info(pvd, &pvd_info);

    if (status != FBE_STATUS_OK) {
        fbe_cli_printf("%s: pvd get info failed, obj 0x%x, status: 0x%x\n",
                        __FUNCTION__, pvd, status);
        return status;
    }

    /* If clear cache return success
     */
    if (change->operation == FBE_CLI_PAGED_OP_CLEAR_CACHE) {
        return FBE_STATUS_OK;
    }

    total_chunks = pvd_info.total_chunks;
    chunk_offset = change->chunk_index;
    chunk_count = change->chunk_count;

    if (chunk_offset >= total_chunks) {
        fbe_cli_printf("%s: input chuck offset 0x%llx is greater than total chunks 0x%x\n",
                        __FUNCTION__, (unsigned long long)chunk_offset, total_chunks);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (chunk_offset + chunk_count > total_chunks) {
        change->chunk_count = (fbe_chunk_count_t) (total_chunks - chunk_offset);
        fbe_cli_printf("%s: shrink chunk count to %u\n",
                       __FUNCTION__, (fbe_u32_t)change->chunk_count);
    }
    return FBE_STATUS_OK;
}

static int pvdinfo_paged_operation_parse_operation(fbe_cli_paged_change_t *change,
                                                   int argc, char **argv)
{
    if (argc <= 0) {
        fbe_cli_error("Not enough arguments\n");
        return -1;
    }

    if (strcmp(argv[0], "-paged_set_bits") == 0) {
        change->operation = FBE_CLI_PAGED_OP_SET_BITS;
    } else if (strcmp(argv[0], "-paged_clear_bits") == 0) {
        change->operation = FBE_CLI_PAGED_OP_CLEAR_BITS;
    } else if (strcmp(argv[0], "-paged_write") == 0) {
        change->operation = FBE_CLI_PAGED_OP_WRITE;
    } else if (strcmp(argv[0], "-clear_cache") == 0) {
        change->operation = FBE_CLI_PAGED_OP_CLEAR_CACHE;
    } else {
        fbe_cli_error("Invalid paged operation '%s'\n", argv[0]);
        return -1;
    }
    return 1;
}

static int pvdinfo_paged_operation_parse_chunk(fbe_cli_paged_change_t *change,
                                               int argc, char **argv)
{
    fbe_u64_t v;
    fbe_chunk_index_t chunk_index;
    fbe_chunk_count_t chunk_count;
    fbe_status_t status;

    if (argc < 2) {
        fbe_cli_error("Not enough arguments\n");
        return -1;
    }
    status = fbe_atoh64(argv[0], &chunk_index);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("Invalid chunk index '%s'\n", argv[0]);
        return -1;
    }
    status = fbe_atoh64(argv[1], &v);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("Invalid chunk count '%s'\n", argv[1]);
        return -1;
    }

    chunk_count = (fbe_u32_t)v;
    change->metadata_size = sizeof(fbe_provision_drive_paged_metadata_t);
    change->chunk_index = chunk_index;
    change->chunk_count = chunk_count;

    return 2;
}

static int pvdinfo_paged_operation_parse_data(fbe_cli_paged_change_t *change,
                                              int argc, char **argv)
{
    fbe_provision_drive_paged_metadata_t *bitmap = (fbe_provision_drive_paged_metadata_t *)&change->bitmap[0];
    fbe_provision_drive_paged_metadata_t *data = (fbe_provision_drive_paged_metadata_t *)&change->data[0];
    fbe_u64_t v;
    fbe_u16_t value;
    fbe_status_t status;
    int orig_argc = argc;

    do {
        if (argc < 2) {
            fbe_cli_error("Not enough arguments\n");
            return -1;
        }
        status = fbe_atoh64(argv[1], &v);
        if (status != FBE_STATUS_OK) {
            fbe_cli_error("Invalid value '%s'\n", argv[1]);
            return -1;
        }

        value = (fbe_u16_t)v;
        if (strcmp(argv[0], "-valid") == 0) {
            data->valid_bit = value;
            bitmap->valid_bit = 0x1;
        } else if (strcmp(argv[0], "-nz") == 0) {
            data->need_zero_bit = value;
            bitmap->need_zero_bit = 0x1;
        } else if (strcmp(argv[0], "-uz") == 0) {
            data->user_zero_bit = value;
            bitmap->user_zero_bit = 0x1;
        } else if (strcmp(argv[0], "-cons") == 0) {
            data->consumed_user_data_bit = value;
            bitmap->consumed_user_data_bit = 0x1;
        } else if (strcmp(argv[0], "-reserved") == 0) {
            data->unused_bit = value;
            bitmap->unused_bit = 0xfff;
        } else {
            fbe_cli_error("Invalid switch '%s'\n", argv[0]);
            return -1;
        }
        argc -= 2;
        argv += 2;
    } while (argc > 0);

    return orig_argc - argc;
}
/*!**************************************************************
 *  fbe_cli_pvdinfo_paged_operation_parse()
 ****************************************************************
 *
 * @brief   parse paged operation command line
 *
 * @param   change
 * @param   argc
 * @param   argv
 *
 * @return  FBE_STATUS_OK for success, else error.
 *
 * @author
 *  5/4/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static fbe_status_t fbe_cli_pvdinfo_paged_operation_parse(fbe_cli_paged_change_t *change,
                                                          int argc, char **argv)
{
    int ret;
    const char *name;

    memset(change, 0, sizeof(fbe_cli_paged_change_t));
    if (argc <= 0) {
        fbe_cli_pvdinfo_paged_operation_usage("-paged_set_bits/clear_bits/write/clear_cache");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    name = argv[0];

    ret = pvdinfo_paged_operation_parse_operation(change, argc, argv);
    if (ret < 0) {
        fbe_cli_pvdinfo_paged_operation_usage("-paged_set_bits/clear_bits/write/clear_cache");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    argc -= ret;
    argv += ret;

    if (change->operation == FBE_CLI_PAGED_OP_CLEAR_CACHE) {
        if (argc != 0) {
            fbe_cli_pvdinfo_paged_operation_usage("-clear_cache");
            return FBE_STATUS_GENERIC_FAILURE;
        }
        return FBE_STATUS_OK;
    }
    ret = pvdinfo_paged_operation_parse_chunk(change, argc, argv);
    if (ret < 0) {
        fbe_cli_pvdinfo_paged_operation_usage(name);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    argc -= ret;
    argv += ret;

    ret = pvdinfo_paged_operation_parse_data(change, argc, argv);
    if (ret < 0) {
        fbe_cli_pvdinfo_paged_operation_usage(name);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    argc -= ret;
    argv += ret;

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cli_pvdinfo_paged_operation_parse()
 ******************************************/
/*!**************************************************************
 *  fbe_cli_pvdinfo_paged_operation()
 ****************************************************************
 *
 * @brief   do paged operation
 *
 * @param   pvd object_id
 * @param   argc
 * @param   argv
 *
 * @return  None
 *
 * @author
 *  5/4/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static void fbe_cli_pvdinfo_paged_operation(fbe_object_id_t pvd, int argc, char **argv)
{
    fbe_status_t status;
    fbe_cli_paged_change_t change;

    status = fbe_cli_pvdinfo_paged_operation_parse(&change, argc, argv);
    if (status != FBE_STATUS_OK)
        return;

    status = fbe_cli_pvdinfo_paged_check_valid(pvd, &change);
    if (status != FBE_STATUS_OK)
        return;

    status = fbe_cli_paged_switch(pvd, &change);
    if (status != FBE_STATUS_OK) {
        fbe_cli_printf("%s failed: %u\n",
                       fbe_cli_paged_get_operation_name(change.operation),
                       (fbe_u32_t)status);
    }
}
/******************************************
 * end fbe_cli_pvdinfo_paged_operation_parse()
 ******************************************/


/*!**************************************************************
 *  fbe_cli_pvd_parse_bes_opt()
 ****************************************************************
 *
 * @brief   Parses the b_e_d format option and returns its PVD
 *          object ID.
 * 
 * @param bes_str - string with format b_e_s.
 * @param pvd_object_id - object id
 * 
 * @return returns OK if PVD object is valid and in Ready state.
 * 
 * @note There may be more than one PVD for a given b_e_s, for cases
 *       where a drive was replaced.  In this case one PVD is in
 *       FAILED and other in Ready.  We only want the PVD in Ready.
 *
 * @author
 *  6/13/2014 - Created. Wayne Garrett
 *
 ****************************************************************/
fbe_status_t fbe_cli_pvd_parse_bes_opt(char *bes_str, fbe_object_id_t * pvd_object_id_p)
{
    fbe_status_t status;
    fbe_job_service_bes_t location;
    fbe_lifecycle_state_t lifecycle_state;


    status = fbe_cli_convert_diskname_to_bed(bes_str, &location);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nInvalid b_e_d format\n");        
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(location.bus, location.enclosure, location.slot, pvd_object_id_p);
    if (status != FBE_STATUS_OK || (*pvd_object_id_p) == FBE_OBJECT_ID_INVALID)
    {
        fbe_cli_error("\nFailed to get object ID for %d_%d_%d. status:%d\n", 
                      location.bus, location.enclosure, location.slot, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_api_get_object_lifecycle_state(*pvd_object_id_p, &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFailed to get lifecycle state for pvd:0x%x, status:%d\n", *pvd_object_id_p, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (lifecycle_state != FBE_LIFECYCLE_STATE_READY)
    {
        fbe_cli_error("\nLifecycle state not Ready. pvd:0x%x, state:%d\n", *pvd_object_id_p, lifecycle_state);
        fbe_cli_error("\nTry using -object_id instead\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 *  fbe_cli_pvd_set_wear_level_timer()
 ****************************************************************
 *
 * @brief   Sets the pvd class wear leveling reporting timer
 * 
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  8/04/2015 - Created. Deanna Heng
 *
 ****************************************************************/
void fbe_cli_provision_drive_wear_level_timer(fbe_s32_t argc, fbe_s8_t ** argv)
{
    fbe_status_t status = FBE_STATUS_INVALID;

    if(0 == argc)
    {
        fbe_cli_error("Too few arguments. \n");
        fbe_cli_printf("%s", PVD_WEAR_LEVELING_TIMER_USAGE);
        return;
    }
    
    if((1 == argc) && 
        ((strcmp(*argv, "-help") == 0) ||(strcmp(*argv, "-h") == 0)))
    {
        /* If they are asking for help, just display help and exit. */
        fbe_cli_printf("%s", PVD_WEAR_LEVELING_TIMER_USAGE);
        return;
    }
    while (argc > 0)
    {
        if(strcmp(*argv, "-set") == 0)
        {
            fbe_u64_t timer_seconds = 0;
            argc--;
            argv++;

             /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Please provide value for -set switch. \n\n");
                return;
            }
            fbe_atoh64(*argv, &timer_seconds);

            status = fbe_api_provision_drive_set_wear_leveling_timer(timer_seconds);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("error 0x%x in pvd set wear level timer\n", status);
                return;
            }
            fbe_cli_printf("Successfully set PVD wear level timer to 0x%llx seconds\n", timer_seconds);
            return;
        }
    }
}

/*************************
 * end file fbe_cli_lib_provision_drive_cmds.c
 *************************/
