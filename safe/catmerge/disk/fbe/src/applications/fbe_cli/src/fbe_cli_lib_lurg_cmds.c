/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
* @file fbe_cli_lurg_cmds.c
 ***************************************************************************
 *
 * @brief
*  This file contains cli functions for the LURG related features in FBE CLI.
 *
 * @ingroup fbe_cli
 *
 * @date
 *  4/10/2010 - Created. Sanjay Bhave
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
#include <ctype.h>
#include "fbe_cli_private.h"
#include "fbe_trace.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_transport_trace.h"
#include "fbe_block_transport_trace.h"
#include "fbe/fbe_api_trace.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe_cli_lurg.h"
#include "fbe_database.h"
#include "fbe/fbe_api_lurg_interface.h"
#include "fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_logical_error_injection_interface.h"
#include "fbe/fbe_protocol_error_injection_service.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe_base_object_trace.h"
#include "fbe_provision_drive_trace.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe_cli_lib_sepls_cmd.h"
#include "fbe/fbe_random.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "fbe/fbe_data_pattern.h"
#include "fbe/fbe_api_enclosure_interface.h"

static fbe_cli_lustat_cmd_line_opt_t lustat_cmd_line;
static fbe_bool_t fbe_cli_unbind_warn_user(void);
static fbe_bool_t fbe_cli_bind_warn_user(void);
static void fbe_cli_lun_operation_warning(fbe_object_id_t, fbe_database_lun_operation_code_t,fbe_status_t);
static void fbe_cli_slit_wwn(char *line, int *argc, char **argv);
static fbe_status_t fbe_cli_convert_wwn(char *str, fbe_assigned_wwid_t *wwn);
static void fbe_cli_print_wwn(fbe_assigned_wwid_t *wwn);

static fbe_status_t fbe_cli_paint_buffer(fbe_u8_t *buffer, fbe_lba_t request_size);

/*!****************************************************************************
 *          fbe_cli_lib_drive_type_to_string()
 ****************************************************************************** 
 * 
 * @brief   Convert the drive type to a (abbreviated) string.
 *
 * @param   drive_type - Numeric drive type
 * @param   drive_type_string_pp - Address of returned string
 *
 * @return  fbe_status_t - Return status of the operation.
 * 
 * @author
 *  07/15/2015  Ron Proulx  - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_cli_lib_drive_type_to_string(fbe_drive_type_t drive_type, fbe_u8_t **drive_type_string_pp)
{
    fbe_status_t    status = FBE_STATUS_OK;

    switch(drive_type) {
        case FBE_DRIVE_TYPE_INVALID:
            *drive_type_string_pp = "INVALID";
            break;
        case FBE_DRIVE_TYPE_FIBRE:
            *drive_type_string_pp = "FIBRE";
            break;
        case FBE_DRIVE_TYPE_SAS:
            *drive_type_string_pp = "SAS";
            break;
        case FBE_DRIVE_TYPE_SATA:
            *drive_type_string_pp = "SATA";
            break;
        case FBE_DRIVE_TYPE_SAS_FLASH_HE:
            *drive_type_string_pp = "SAS_FLASH_HE";
            break;
        case FBE_DRIVE_TYPE_SATA_FLASH_HE:
            *drive_type_string_pp = "SATA_FLASH_HE";
            break;
        case FBE_DRIVE_TYPE_SAS_FLASH_ME:
            *drive_type_string_pp = "SAS_FLASH_ME";
            break;
        case FBE_DRIVE_TYPE_SAS_FLASH_LE:
            *drive_type_string_pp = "SAS_FLASH_LE";
            break;
        case FBE_DRIVE_TYPE_SAS_FLASH_RI:
            *drive_type_string_pp = "SAS_FLASH_RI";
            break;
        case FBE_DRIVE_TYPE_SAS_NL:
            *drive_type_string_pp = "NL_SAS";
            break;
        case FBE_DRIVE_TYPE_SATA_PADDLECARD:
            *drive_type_string_pp = "SATA_PADDLECARD";
            break;
        case FBE_DRIVE_TYPE_SAS_SED:
            *drive_type_string_pp = "SAS SED";
            break;
        default:
            *drive_type_string_pp = "UNEXPECTED DRIVE TYPE VALUE";
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }
    return status;
}

/*!****************************************************************************
 *          fbe_cli_lib_performance_tier_to_string()
 ****************************************************************************** 
 * 
 * @brief   Convert the performance tier to a string
 *
 * @param   performance_tier - Numeric performance tier
 * @param   performance_tier_string_pp - Address of returned string
 *
 * @return  fbe_status_t - Return status of the operation.
 * 
 * @author
 *  07/15/2015  Ron Proulx  - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_cli_lib_performance_tier_to_string(fbe_performance_tier_number_t performance_tier, fbe_u8_t **performance_tier_string_pp)
{
    fbe_status_t    status = FBE_STATUS_OK;

    switch(performance_tier) {
        case FBE_PERFORMANCE_TIER_INVALID:
            *performance_tier_string_pp = "FBE_PERFORMANCE_TIER_INVALID";
            break;
        case FBE_PERFORMANCE_TIER_ZERO:
            *performance_tier_string_pp = "FBE_PERFORMANCE_TIER_ZERO";
            break;
        case FBE_PERFORMANCE_TIER_ONE:
            *performance_tier_string_pp = "FBE_PERFORMANCE_TIER_ONE";
            break;
        case FBE_PERFORMANCE_TIER_TWO:
            *performance_tier_string_pp = "FBE_PERFORMANCE_TIER_TWO";
            break;
        case FBE_PERFORMANCE_TIER_THREE:
            *performance_tier_string_pp = "FBE_PERFORMANCE_TIER_THREE";
            break;
        case FBE_PERFORMANCE_TIER_FOUR:
            *performance_tier_string_pp = "FBE_PERFORMANCE_TIER_FOUR";
            break;
        case FBE_PERFORMANCE_TIER_FIVE:
            *performance_tier_string_pp = "FBE_PERFORMANCE_TIER_FIVE";
            break;
        case FBE_PERFORMANCE_TIER_SIX:
            *performance_tier_string_pp = "FBE_PERFORMANCE_TIER_SIX";
            break;
        case FBE_PERFORMANCE_TIER_SEVEN:
            *performance_tier_string_pp = "FBE_PERFORMANCE_TIER_SEVEN";
            break;
        default:
            *performance_tier_string_pp = "UNEXPECTED PERFORMANCE TIER VALUE";
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }
    return status;
}

/*!****************************************************************************
 *          fbe_cli_lib_get_drive_type_to_performance_tier_mapping()
 ****************************************************************************** 
 * 
 * @brief   Get the drive_type to performance tier mapping.
 *
 * @param   get_performance_tier_p - Pointer to drive type to performance tier
 *              table to display.
 *
 * @return  none
 * 
 * @author
 *  07/15/2015  Ron Proulx  - Created.
 * 
 ******************************************************************************/
static void fbe_cli_lib_get_drive_type_to_performance_tier_mapping(fbe_api_virtual_drive_get_performance_tier_t *get_performance_tier_p)
{
    fbe_u32_t       drive_type = 0;
    fbe_s32_t       performance_tier = -1;
    fbe_u8_t       *drive_type_string_p = "UNINITIALIZED"; 
    fbe_u8_t       *performance_tier_string_p = "UNINITIALIZED"; 

    if (get_performance_tier_p == NULL)
    {
        fbe_cli_error("lurg: Get performance pointer is NULL \n");
        return;
    }
    if (get_performance_tier_p->last_valid_drive_type_plus_one != FBE_DRIVE_TYPE_LAST)
    {
        fbe_cli_error("lurg: Get performance last valid plus one: %d not expected: %d\n",
                      get_performance_tier_p->last_valid_drive_type_plus_one,
                      FBE_DRIVE_TYPE_LAST);
        return;
    }

    fbe_cli_printf("\t\tDrive Type to Performance Group Mapping Table\n");
    for (drive_type = 0; drive_type < get_performance_tier_p->last_valid_drive_type_plus_one; drive_type++)
    {
        performance_tier = get_performance_tier_p->performance_tier[drive_type];
        fbe_cli_lib_drive_type_to_string(drive_type, &drive_type_string_p);
        fbe_cli_lib_performance_tier_to_string(performance_tier, &performance_tier_string_p);
    fbe_cli_printf("Drive Type: %2d (%-16s) ==> Performance Group: %2d (%-32s)\n", 
                   drive_type, drive_type_string_p, 
                   performance_tier, performance_tier_string_p);
    }
    return;
}

/*!*******************************************************************
 * @var fbe_cli_hs_configuration()
 *********************************************************************
 * @brief Function to implement binding of hot spare.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  4/20/2010 - Created. Dhaval Patel
 *********************************************************************/
void fbe_cli_hs_configuration(fbe_s32_t argc, fbe_s8_t ** argv)
{
    fbe_job_service_bes_t   bes_info;
    fbe_status_t            status = FBE_STATUS_INVALID;
    fbe_bool_t              configure_spare = FBE_FALSE;
    fbe_bool_t              unconfigure_spare = FBE_FALSE;
    fbe_bool_t              b_get_performance_tier = FBE_FALSE;
    fbe_u32_t               bus_number = FBE_PORT_NUMBER_INVALID; 
    fbe_bool_t              list_hs_in_bus = FBE_FALSE;
    fbe_u64_t swap_delay = 300; /*Default delay in HS swap*/
    fbe_api_virtual_drive_permanent_spare_trigger_time_t spare_config;
    fbe_object_id_t         pvd_object_id;
    fbe_api_provision_drive_info_t pvd_info;
    fbe_api_virtual_drive_get_performance_tier_t get_performance_tier;
    fbe_u32_t               performance_tier = FBE_PERFORMANCE_TIER_INVALID;

    if (argc < 1)
    {
        fbe_cli_error("%s Not enough arguments to configure hot spare\n", __FUNCTION__);
        fbe_cli_printf("%s", LURG_HS_USAGE);
        return;
    }
    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /* If they are asking for help, just display help and exit. */
            fbe_cli_printf("%s", LURG_HS_USAGE);
            return;
        }
        else if ((strcmp(*argv, "-c") == 0))
        {
            /* Get the disk in b_e_d format, from command line */
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Disk in b_e_d format, argument is expected\n");
                return;
            }

            /* Validate that the disk numbers are given correctly*/
            /* Convert to B_E_D format*/
            /* Check for case where no drive is specified and return*/
            status = fbe_cli_convert_diskname_to_bed(argv[0], &bes_info);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("Failed to convert the diskname: %s. Sts=%d\n", *argv, status);
                return;
            }

            /* increment the argument. */
            argc--;
            argv++;
        }
        else if (strcmp(*argv, "-u") == 0)
        {
            /* Get the disk in b_e_d format, from command line */
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Disk in b_e_d format, argument is expected\n");
                return;
            }

            /* Validate that the disk numbers are given correctly*/
            /* Convert to B_E_D format*/
            /* Check for case where no drive is specified and return*/
            status = fbe_cli_convert_diskname_to_bed(argv[0], &bes_info);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("Failed to convert the diskname: %s. Sts=%d\n", *argv, status);
                return;
            }

            unconfigure_spare = TRUE;

            /* increment the argument. */
            argc--;
            argv++;
        }
        else if (strcmp(*argv, "-d") == 0)
        {
            /* [FBECLI - TODO] */
            argc--;
            argv++;
            /* if (argc <= 0), then show all hot spares present in system */
            if (argc <= 0)
            {
                fbe_cli_hs_display(bus_number, list_hs_in_bus);
                return;
            }
            list_hs_in_bus = TRUE;
            bus_number = atoi(*argv);

            if ((bus_number >  FBE_API_PHYSICAL_BUS_COUNT) || (bus_number < 0))
            {
                fbe_cli_error("Invalid Bus number\n");
                return;
            }

            fbe_cli_hs_display(bus_number, list_hs_in_bus);
            return;

        }
        else if (strcmp(*argv, "-delay_swap") == 0)
        {
            argc--;
            argv++;
            status = fbe_api_virtual_drive_get_permanent_spare_trigger_time(&spare_config);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_error ("%s:Failed to get current permanent spare trigger time",__FUNCTION__);
                return;
            }
            if (argc <= 0)
            {
                fbe_cli_printf("Current permanent spare trigger time: %llu seconds.\n",
			       (unsigned long long)spare_config.permanent_spare_trigger_time);
                return;
            }

            swap_delay = atoi(*argv);

            fbe_cli_hs_set_swap_delay(swap_delay,&spare_config);
            return;
        }
        else if (strcmp(*argv, "-m") == 0)
        {
            /* Get the drive type to performance tier mapping */
            argc--;
            argv++;

            /* There should be no other arguments.*/
            if (argc > 0)
            {
                fbe_cli_error("Get drive type to performance mapping does not accept arguments\n");
                return;
            }

            /* Get the drive type to performance tier mapping. */
            status = fbe_api_virtual_drive_get_performance_tier(&get_performance_tier);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf ("%s:Failed to get drive type to performance tier mapping - status: %d\n",
                                __FUNCTION__, status);
                return;
            }
            
            /* Display the drive type to performance tier mapping */
            fbe_cli_lib_get_drive_type_to_performance_tier_mapping(&get_performance_tier);
            return;
        }
        else if (strcmp(*argv, "-p") == 0)
        {
            /* Get the performance tier for this drive type. */
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Disk in b_e_d format, argument is expected\n");
                return;
            }

            /* Validate that the disk numbers are given correctly*/
            /* Convert to B_E_D format*/
            /* Check for case where no drive is specified and return*/
            status = fbe_cli_convert_diskname_to_bed(argv[0], &bes_info);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("Failed to convert the diskname: %s. Sts=%d\n", *argv, status);
                return;
            }

            /* Flag that this is `get the performance tier for this drive' */
            b_get_performance_tier = FBE_TRUE;

            /* increment the argument. */
            argc--;
            argv++;

        }
        else
        {
            /* The command line parameters should be properly entered */
            fbe_cli_printf("%s", LURG_HS_USAGE);
            return;
        }
    }

    if(configure_spare)
    {
        /* configure a spare. */
        status = fbe_cli_configure_spare(bes_info.bus, bes_info.enclosure, bes_info.slot);
        if (status != FBE_STATUS_OK)
        {
            /* if HS creation request failed then return error from here */
            fbe_cli_printf ("%s:Failed to configure drive %d_%d_%d as Hotspare\n",
                            __FUNCTION__, bes_info.bus, bes_info.enclosure, bes_info.slot);
            return;
        }

        /* HS creation successful */
        fbe_cli_printf ("%s:Hotspare %d_%d_%d created successfully\n",
                        __FUNCTION__, bes_info.bus,  bes_info.enclosure, bes_info.slot);
        return;
    }

    if(unconfigure_spare)
    {
        /* configure a spare. */
        status = fbe_cli_unconfigure_spare(bes_info.bus, bes_info.enclosure, bes_info.slot);
        if (status != FBE_STATUS_OK)
        {
            /* if HS creation request failed then return error from here */
            fbe_cli_printf ("%s:Failed to unconfigure spare %d_%d_%d\n",
                            __FUNCTION__, bes_info.bus, bes_info.enclosure, bes_info.slot);
            return;
        }
    
        /* HS creation successful */
        fbe_cli_printf ("%s:Hotspare %d_%d_%d unconfigured successfully\n",
                        __FUNCTION__, bes_info.bus,  bes_info.enclosure, bes_info.slot);
        return;
    }

    if (b_get_performance_tier)
    {
        /* Get the performance tier information for this drive.
         */
        status = fbe_api_provision_drive_get_obj_id_by_location(bes_info.bus,
                                                                bes_info.enclosure,
                                                                bes_info.slot,
                                                                &pvd_object_id);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf ("%s:Failed to get pvd object id for %d_%d_%d\n",
                            __FUNCTION__, bes_info.bus, bes_info.enclosure, bes_info.slot);
            return;
        }


        /* Get drive type */
        status = fbe_api_provision_drive_get_info(pvd_object_id, &pvd_info);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf ("%s:Failed to pvd info for %d_%d_%d\n",
                            __FUNCTION__, bes_info.bus, bes_info.enclosure, bes_info.slot);
            return;
        }

        /* get the peformance tier*/
        status = fbe_api_virtual_drive_get_performance_tier(&get_performance_tier);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf ("%s:Failed to get performance tier for %d_%d_%d\n",
                            __FUNCTION__, bes_info.bus, bes_info.enclosure, bes_info.slot);
            return;
        }
 
        /* Validate drive type */
        status = fbe_api_virtual_drive_get_performance_tier(&get_performance_tier);
        if ((pvd_info.drive_type == FBE_DRIVE_TYPE_INVALID) ||
            (pvd_info.drive_type >= FBE_DRIVE_TYPE_LAST)       )
        {
            fbe_cli_printf ("%s:Unexpected drive type: %d for %d_%d_%d\n",
                            __FUNCTION__, pvd_info.drive_type,
                            bes_info.bus, bes_info.enclosure, bes_info.slot);
            return;
        }

        /* Now display the drive type and peformanc tier */   
        performance_tier = get_performance_tier.performance_tier[pvd_info.drive_type];      
        fbe_cli_printf ("%s: Performance group for %d_%d_%d drive type: %d is: %d\n",
                        __FUNCTION__, bes_info.bus, bes_info.enclosure, bes_info.slot,
                        pvd_info.drive_type, performance_tier);
        return;
    }
    return;
}
/******************************************
 * end fbe_cli_hs_configuration()
 ******************************************/


/*!*******************************************************************
 * @var fbe_cli_bind
 *********************************************************************
 * @brief Function to implement binding of LUNs.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  4/20/2010 - Created. Sanjay Bhave
 *********************************************************************/
void fbe_cli_bind(fbe_s32_t argc, fbe_s8_t ** argv)
{

    fbe_api_lun_create_t                     lun_create_req;
    fbe_api_raid_group_create_t              rg_request;
    fbe_u32_t                                rg_num = FBE_OBJECT_ID_INVALID;
    fbe_raid_group_number_t                  rg_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t                             status = FBE_STATUS_INVALID;
    fbe_u8_t                                 *rg_type = NULL;
    fbe_u32_t                                drive_count = 0;
    /* By default create LUN with capacity of 0x1000 */
    fbe_lba_t                                lun_capacity = FBE_LBA_INVALID;
    fbe_bool_t                               is_ndb_req = 0;
    fbe_bool_t                               is_export = 0;
    fbe_bool_t                               is_noinitialverify_req = 0;
    fbe_bool_t                               is_user_private = FBE_FALSE;
    fbe_lba_t                                addr_offset = FBE_LBA_INVALID;
    fbe_raid_group_type_t                    raid_type = FBE_RAID_GROUP_TYPE_UNKNOWN;
    fbe_lun_number_t                         lun_no = FBE_OBJECT_ID_INVALID;
    fbe_block_transport_placement_t          block_transport_placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_bool_t                               is_create_rg_b = FBE_FALSE;
    fbe_bool_t                               is_rgnum_given = FBE_FALSE;
    fbe_bool_t                               is_rgtype_given = FBE_FALSE;
    fbe_bool_t                               is_disk_given = FBE_FALSE;
    fbe_api_raid_group_get_info_t            raid_group_info_p;
    fbe_assigned_wwid_t                      wwn;   
    fbe_bool_t                               has_wwn_assigned = FBE_FALSE;
    fbe_u32_t                                random_seed = (fbe_u32_t)fbe_get_time();
    fbe_u32_t                                assign_wwn_count;
    fbe_object_id_t                          lu_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                                index = 0;
    fbe_u32_t                                cnt = FBE_CLI_BIND_DEFAULT_COUNT;
    fbe_api_lun_calculate_capacity_info_t    capacity_info;
    fbe_job_service_error_type_t             job_error_type;
	fbe_bool_t 								 continue_bind = FBE_FALSE;


	/*bind is dangerous, we want to do it via NAVI CLI so all layers above us see the LUN, let's warn the user*/
	continue_bind = fbe_cli_bind_warn_user();
	if (FBE_IS_FALSE(continue_bind)) {
		return;
	}


    fbe_zero_memory(&lun_create_req, sizeof(fbe_api_lun_create_t));
    fbe_zero_memory(&rg_request, sizeof(fbe_api_raid_group_create_t));
    memset(&wwn, '0', sizeof(fbe_assigned_wwid_t));
    if (argc < 1)
    {
        fbe_cli_error("%s Not enough arguments to bind\n", __FUNCTION__);
        fbe_cli_printf("%s", LURG_BIND_USAGE);
        return;
    }
    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /* If they are asking for help, just display help and exit.
            */
            fbe_cli_printf("%s", LURG_BIND_USAGE);
            return;
        }
        else if ((strcmp(*argv, "-rg") == 0))
        {
            /* Get the RG number from command line */
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("RAID group number argument is expected\n");
                return;
            }

            rg_num = atoi(*argv);

            /* Make sure that RG number received from command line is valid */
            status = fbe_api_database_lookup_raid_group_by_number(rg_num, &rg_id);
            if (status == FBE_STATUS_GENERIC_FAILURE)
            {
                fbe_cli_error("RAID group %d does not exist or unable to retrieve information for this RAID Group.\n",rg_num);
                return;
            }
            is_rgnum_given = FBE_TRUE;
            argc--;
            argv++;
        }
        else if (strcmp(*argv, "-type") == 0)
        {
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("RAID type argument is expected\n");
                return;
            }

            rg_type = *argv;

            /* Check validity of Raid Type */
            raid_type = fbe_cli_check_valid_raid_type(rg_type);
            if (raid_type == FBE_RAID_GROUP_TYPE_UNKNOWN)
            {
                /* if RG mentioned is not valid RAID type 
                * then don't proceed further with LUN creation 
                */
                return;
            }

            is_rgtype_given = FBE_TRUE;
            argc--;
            argv++;
        }
        else if (strcmp(*argv, "-disk") == 0)
        {
            /* [FBECLI - TODO] */
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("disk argument is expected\n");
                return;
            }
            /*Get all the drives*/
            /* FBECLI - TODO*/
            for(drive_count = 0; argc > 0 && (strchr(*argv, '-') == NULL); drive_count++)
            {

                /* Validate that the disk numbers are given correctly*/
                /* Convert to B_E_D format*/
                /* Check for case where no drive is specified and return*/

                status = fbe_cli_convert_diskname_to_bed(argv[0], &rg_request.disk_array[drive_count]);

                if(status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("%s:\tSpecified disk number %s is invalid. %d\n", __FUNCTION__, *argv, status);
                    return;
                }
                is_disk_given = FBE_TRUE;
                argc--;
                argv++;

            }
        }
        else if (strcmp(*argv, "-cap") == 0)
        {
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("LUN capacity argument is expected\n");
                return;
            }
            lun_capacity = fbe_cli_convert_str_to_lba(*argv);
            argc--;
            argv++;
        }
        else if (strcmp(*argv,"-sq")==0)
        {
            argc--;
            argv++;
            if(argc<=0)
            {
                fbe_cli_error("Lun Size should be GB, MB, BL or LBA\n");
            }
            if((strcmp(*argv,"GB") && strcmp(*argv,"Gb") && strcmp(*argv,"gb") && strcmp(*argv,"gB")) == 0)
            {
                lun_capacity = fbe_cli_convert_size_unit_to_lba(lun_capacity,"GB");
            }
            else if((strcmp(*argv,"MB") && strcmp(*argv,"Mb") && strcmp(*argv,"mb") && strcmp(*argv,"mB")) == 0)
            {
                lun_capacity = fbe_cli_convert_size_unit_to_lba(lun_capacity,"MB");
            }
            else if((strcmp(*argv,"BL") && strcmp(*argv,"bL") && strcmp(*argv,"Bl") && strcmp(*argv,"bl")) == 0)
            {
                lun_capacity = fbe_cli_convert_size_unit_to_lba(lun_capacity,"BL");
            }
            else
            {
                fbe_cli_error("Unknown size_unit specified \n");
                return;
            }
            argc--;
            argv++;
        }
        else if (strcmp(*argv, "-ndb") == 0)
        {
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("NDB argument is expected\n");
                return;
            }
            is_ndb_req = atoi(*argv);
            argc--;
            argv++;
        }
        else if (strcmp(*argv, "-addroffset") == 0)
        {
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("valid address offset  argument is expected\n");
                return;
            }
            addr_offset = fbe_cli_convert_str_to_lba(*argv);
            argc--;
            argv++;
        }
        else if (strcmp(*argv, "-lun") == 0)
        {
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("LUN number argument is expected\n");
                return;
            }
            lun_no = atoi(*argv);
            argc--;
            argv++;
        }
        else if (strcmp(*argv, "-placement") == 0)
        {
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("placement argument is expected\n");
                return;
            }
            block_transport_placement = atoi(*argv);
            argc--;
            argv++;
        }
        else if (strcmp(*argv, "-wwn") == 0)
        { 
            /* [FBECLI - TODO] */
            argc--;
            argv++;
                
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("wwn argument is expected\n");
                return;
            }

            has_wwn_assigned = FBE_TRUE;
            status = fbe_cli_convert_wwn(*argv, &wwn);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s: fail to convert the wwn string to wwn bytes\n", __FUNCTION__);
                return;
            }
            
            if(fbe_cli_checks_for_simillar_wwn_number(wwn))
            {
                fbe_cli_printf("%s:\tSpecified wwn number is already assigned to existing lun. \n", __FUNCTION__);
                return;        
            }
            argc--;
            argv++;
        }
        else if (strcmp(*argv, "-cnt") == 0)
        {
            argc--;
            argv++;
            if (argc <= 0)
            {
                fbe_cli_error("count number argument is expected\n");
                return;
            }
            cnt = atoi(*argv);
            argc--;
            argv++;
        }
        else if (strcmp(*argv, "-noinitialverify") == 0)
        {
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Noinitialverify argument is expected\n");
                return;
            }
            is_noinitialverify_req = atoi(*argv);
            argc--;
            argv++; 
        }
        else if (strcmp(*argv, "-userprivate") == 0)
        {
            argc--;
            argv++;
            is_user_private = FBE_TRUE;
            argc--;
            argv++; 
        }
        else if (strcmp(*argv, "-export") == 0)
        {
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Export argument is expected\n");
                return;
            }
            is_export = atoi(*argv);
            argc--;
            argv++; 
        }
        else
        {
            /* The command line parameters should be properly entered */
            fbe_cli_printf("%s", LURG_BIND_USAGE);
            return;
        }
    }

    if(!(is_rgtype_given && is_disk_given))
    {
        /* Make sure that RG number is also not given.
         * We can find out rest of the information from RG number
         */
        if (is_rgnum_given == FBE_FALSE)
        {
            fbe_cli_printf("%s:\tRG type and Disks are mandatory for creating LUN.\n", __FUNCTION__);
            fbe_cli_printf("%s", LURG_BIND_USAGE);
            return;
        }
    }
    else
    {
        /* Check validity of disk mentioned for given RG type*/
        /* (drive_count-1) will give number of disk mentioned on command line*/
        status = fbe_cli_check_disks_for_raid_type(drive_count, raid_type);

        if(status != FBE_STATUS_OK)
        {

            if(status == FBE_STATUS_INSUFFICIENT_RESOURCES)
            {
                fbe_cli_printf("%s:\tNumber of drives provided [%d drive(s)] for RG type \"%s\" is incorrect.\n", __FUNCTION__, drive_count, rg_type);
                return;
            }

            if(status == FBE_STATUS_INVALID)
            {
                fbe_cli_printf("%s:\tRaid type \"%s\" given for RG creation is invalid.\n", __FUNCTION__, rg_type);
            }
        }
    }

    if (raid_type == FBE_RAID_GROUP_TYPE_SPARE)
    {

        status = fbe_cli_configure_spare(rg_request.disk_array[0].bus, rg_request.disk_array[0].enclosure, rg_request.disk_array[0].slot);

        if (status != FBE_STATUS_OK)
        {
            /* if RG creation request failed then return error from here */
            fbe_cli_printf ("%s:Failed to configure drive %d_%d_%d as Hotspare\n", __FUNCTION__, rg_request.disk_array[0].bus, rg_request.disk_array[0].enclosure, rg_request.disk_array[0].slot);
            return;
        }

        /* HS creation successful */
        fbe_cli_printf ("%s:Hotspare %d_%d_%d created successfully\n", __FUNCTION__, rg_request.disk_array[0].bus, rg_request.disk_array[0].enclosure, rg_request.disk_array[0].slot);
        return;
    }


    /* Check if RG number is given on command line or not */
    if (is_rgnum_given == FBE_TRUE)
    {
        /* since RG number is given make sure that it exists on the system */
        status = fbe_api_database_lookup_raid_group_by_number(rg_num, &rg_id);

        if (status != FBE_STATUS_OK)
        {
            /* The RG passed does not exist
             * return the error to user.
             */
            fbe_cli_printf ("RG %d does not exist\n", rg_num);
            return;
        }
    }
    else
    {
        /* Now that we know that RG does not exist find the smallest 
         * possible RG number that we can create on this system
         */
        is_create_rg_b = FBE_TRUE;
        status = fbe_cli_find_smallest_free_raid_group_number(&rg_num);

        if (status == FBE_STATUS_INSUFFICIENT_RESOURCES)
        {
            fbe_cli_printf("%s:\tMaximum number of RG (%d) exists. RG creation is not allowed. %d\n", __FUNCTION__, MAX_RAID_GROUP_ID, status);
            return;
        }

        fbe_cli_printf("%s:\tRG number is not mentioned. Creating RG having number %d.\n", __FUNCTION__, rg_num);
        
        /* Check validity of Raid Type */
        raid_type = fbe_cli_check_valid_raid_type(rg_type);
        if (raid_type == FBE_RAID_GROUP_TYPE_UNKNOWN)
        {
            /* if RG mentioned is not valid RAID type 
             * then don't proceed further with LUN creation 
             */
            return;
        }
    }

    /* if LUN number is not passed on command line then select smallest possible LUN number */
    if(lun_no == FBE_OBJECT_ID_INVALID)
    {
        status = fbe_cli_find_smallest_free_lun_number(&lun_no);

        if (status == FBE_STATUS_INSUFFICIENT_RESOURCES)
        {
            fbe_cli_printf("%s:\tMaximum number of LUN (%d) exists. LUN creation is not allowed. %d\n", __FUNCTION__, MAX_LUN_ID, status);
            return;
        }

         fbe_cli_printf("%s:\tLUN number is not mentioned. Creating LUN having number %d.\n", __FUNCTION__, lun_no);
    }

    /* Check if RG number is a supported number on this system */
    if(fbe_cli_is_valid_raid_group_number(rg_num) != FBE_TRUE)
    {
        fbe_cli_printf("%s:\tRG number %d is not supported. %d\n", __FUNCTION__, rg_num, status);
        return;
    }

    /* if we need to create the RG then do that before LUN creation */
    if (is_create_rg_b == FBE_TRUE &&  raid_type != FBE_RAID_GROUP_TYPE_SPARE)
    {
        /* Populate the RG creation request structure */
        rg_request.capacity = FBE_LBA_INVALID;
        rg_request.drive_count = drive_count;
        //rg_request.explicit_removal = FBE_TRUE;
        rg_request.is_private = FBE_TRI_FALSE;
        rg_request.max_raid_latency_time_is_sec = 120;
        rg_request.power_saving_idle_time_in_seconds = 120;
        rg_request.b_bandwidth = FBE_FALSE;
        rg_request.raid_type = raid_type;
        rg_request.raid_group_id = rg_num;
        rg_request.user_private = is_user_private;
        
        /*Send request to create a RG*/
        status = fbe_api_create_rg(&rg_request, FBE_TRUE, RG_READY_TIMEOUT, &rg_id, &job_error_type);

        if (status != FBE_STATUS_OK)
        {
            /* if RG creation request failed then return error from here */
            fbe_cli_printf ("%s:RG creation failed...can not proceed with LUN creation\n", __FUNCTION__);
            return;
        }
    }
    /* Now that we know that a valid RG exist
     * we can create a LUN in that RG 
     * It is assumed that create RG function will return only when RG is created 
     */
    if (is_rgnum_given == FBE_FALSE)
    {
        status = fbe_api_database_lookup_raid_group_by_number(rg_num, &rg_id);
        if (status != FBE_STATUS_OK)
        {
            /* Not able to create RG and hence no RG ID and hence 
             * will not be able to create LUN 
             */
            return;
        }
    }

    /* Now that we've correct RG id assign it to LUN creation request */
    lun_create_req.raid_group_id = rg_num;

    /* Now that we know the RG exists get the information for the RG */
    status = fbe_api_raid_group_get_info(rg_id, &raid_group_info_p, FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Not able to get the information for RG %d\n", rg_num);
        return;
    }

    /* Set LUN capacity to RG capacity when user does not specify the LUN capacity */
    if (lun_capacity == FBE_LBA_INVALID)
    {
        /* set the LUN capacity to maximum LUN size this RG can handle */
        capacity_info.exported_capacity = 0;
        capacity_info.lun_align_size = raid_group_info_p.lun_align_size;
        capacity_info.imported_capacity = raid_group_info_p.capacity;
        
        status = fbe_api_lun_calculate_exported_capacity(&capacity_info);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Unable to caculate LUN exported capacity\n");
            return;
        }
        lun_capacity = capacity_info.exported_capacity;
        
    }

    lun_create_req.capacity = lun_capacity;

    /* If the cmd doesn't provide the wwn argument, we generate one for it */
    if (!has_wwn_assigned) 
    {
        fbe_cli_generate_random_wwn_number(&wwn);
        wwn.bytes[FBE_WWN_BYTES-1] = '\0';
        assign_wwn_count = 1;
        if (fbe_cli_checks_for_simillar_wwn_number(wwn))
        {
            /* The random seed allows us to make tests more random. 
             * We seed the random number generator with the time. 
             */
            fbe_random_set_seed(random_seed);
        }
        while (fbe_cli_checks_for_simillar_wwn_number(wwn))
        {
            fbe_cli_generate_random_wwn_number(&wwn);
            wwn.bytes[FBE_WWN_BYTES-1] = '\0';
            if (assign_wwn_count++ > 200)
            {
                fbe_cli_printf("Try to assign wwn for %d times\n", assign_wwn_count);
                if (assign_wwn_count > 5000)
                    break;
            }
        }
    }
    /* fill in the LUN creation request based on the command line parameters */
    lun_create_req.raid_type = raid_type;
    lun_create_req.ndb_b = is_ndb_req;
    lun_create_req.noinitialverify_b = is_noinitialverify_req;
    lun_create_req.addroffset = addr_offset;
    lun_create_req.lun_number = lun_no;
    lun_create_req.placement = block_transport_placement;
    lun_create_req.world_wide_name = wwn;
    lun_create_req.user_private = is_user_private;
    lun_create_req.export_lun_b = is_export;

    for(index = 0 ; index < cnt ; index ++)
    {
        fbe_cli_printf("Creating LUN having number %d.\n", lun_no);
        status = fbe_cli_validate_lun_request(&lun_create_req);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf ("Error in command line parameters for LUN creation request\n");
            return;
        }

        status = fbe_api_create_lun(&lun_create_req, FBE_TRUE, LU_READY_TIMEOUT, &lu_id, &job_error_type);
        if (status == FBE_STATUS_OK)
        {
            fbe_cli_printf ("Created LU:%d with object ID:0x%X\n",lun_create_req.lun_number ,lu_id);
        }
        else if (status == FBE_STATUS_TIMEOUT) 
        {
            fbe_cli_printf ("Timed out waiting for LU to become ready\n");
        }
        else 
        {
            fbe_cli_printf ("LU creation failue: error code: %d\n", status );
        }

        fbe_cli_lun_operation_warning(lu_id, FBE_DATABASE_LUN_CREATE_FBE_CLI, status);
        
        status = fbe_cli_find_smallest_free_lun_number(&lun_no);
        if (status == FBE_STATUS_INSUFFICIENT_RESOURCES)
        {
            fbe_cli_printf("%s:\tMaximum number of LUN (%d) exists. LUN creation is not allowed. %d\n", __FUNCTION__, MAX_LUN_ID, status);
            return;
        }
        lun_create_req.lun_number = lun_no;
    }
    
    return;
}
/******************************************
 * end fbe_cli_bind()
 ******************************************/
 
/*!*******************************************************************
 * @var fbe_cli_hs_display
 *********************************************************************
 * @brief Function to implement displaying of hot spares.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  4/20/2010 - Created. Prafull P
 *********************************************************************/
void fbe_cli_hs_display(fbe_u32_t bus_number, fbe_bool_t list_hs_in_bus)
{
    fbe_u32_t                                         index = 0;
    fbe_status_t                                      status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_provision_drive_info_t                    provision_drive_info;
    fbe_api_provisional_drive_get_spare_drive_pool_t  spare_drive_pool ;
    
    status = fbe_api_provision_drive_get_spare_drive_pool(&spare_drive_pool);
    if(status != FBE_STATUS_OK) {
        fbe_cli_error ("Could not get spare drive pool,status:%d ",status);
        return;
    }
    if(spare_drive_pool.number_of_spare == 0) {
        fbe_cli_printf ("No hot spare configured in the system\n");
        return;
    }

    
    fbe_cli_printf (" Obj ID | Disk  | Config Type   |	Lifecycle state    |		  Drive Type		   |	Capacity   | Serial No.\n");
    fbe_cli_printf ("------------------------------------------------------------------------------------------------------------------\n");
    
    for(index = 0; index < spare_drive_pool.number_of_spare; index ++)
    {
        fbe_api_provision_drive_get_info(spare_drive_pool.spare_object_list[index],&provision_drive_info);
        if(provision_drive_info.config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE)
        {
            if(!(list_hs_in_bus))
            {
                fbe_cli_display_hot_spare_row(spare_drive_pool.spare_object_list[index], &provision_drive_info);
            }
            else if((list_hs_in_bus) && (provision_drive_info.port_number == bus_number))
            {
                fbe_cli_display_hot_spare_row(spare_drive_pool.spare_object_list[index], &provision_drive_info);
            }
        }
     }
}
/******************************************
 * end fbe_cli_hs_display()
 ******************************************/

/*!*******************************************************************
 * @var fbe_cli_display_hot_spare_row
 *********************************************************************
 * @brief Function to display the prepare the output line to display hot spare.
 *
 *  @param lun_details - details of the hot spare to display.
 *
 * @return -  FBE status.  
 *
 * @author
 *  4/30/2010 - Created. Prafull P
 * 10/25/2012 - Added serial number display. Wenxuan Yin
 *********************************************************************/
void fbe_cli_display_hot_spare_row(fbe_object_id_t object_id, fbe_api_provision_drive_info_t *hs_details)
{
    fbe_u8_t          line_buffer[fbe_cli_lustat_line_size];
    fbe_u8_t*         CSX_MAYBE_UNUSED line_seeker = fbe_cli_lustat_reset_line(line_buffer);
    fbe_status_t      status;
    fbe_lifecycle_state_t lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_u32_t         actual_capacity_of_hs;
    fbe_u32_t         actual_cap_in_bytes = 0;
    fbe_u8_t          sprintf_buffer[fbe_cli_lustat_line_size];

    /* If block size is 520 or 4160, then calculate the capacity. Note, the capacity is always in terms of 520 block sector size. */
    if ((hs_details->configured_physical_block_size == FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_520) ||
        (hs_details->configured_physical_block_size == FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_4160))
    {
        /* Capacity is always is terms of 520-bps */
        actual_capacity_of_hs = (fbe_u32_t)((hs_details->capacity * FBE_BE_BYTES_PER_BLOCK)/(FBE_BYTES_PER_MEGABYTE));
        actual_cap_in_bytes = (fbe_u32_t)((hs_details->capacity * FBE_BE_BYTES_PER_BLOCK) % (FBE_BYTES_PER_MEGABYTE));
    
        /* Default display size is GB. If the disk size is too small 
        * we will display in MB */
        if (actual_capacity_of_hs < 1024)
        {
            lustat_cmd_line.display_size = DISPLAY_IN_MB;
        }
        else 
        {
            lustat_cmd_line.display_size = DISPLAY_IN_GB;
        }
    
        if (lustat_cmd_line.display_size == DISPLAY_IN_MB)
        {
            /* Display MegaBytes.
            */
            if (actual_capacity_of_hs > 0)
            {
                sprintf(sprintf_buffer, "%d MB       ", actual_capacity_of_hs);
            }
            else
            {
                sprintf(sprintf_buffer, "%d B       ", actual_cap_in_bytes);
            }
        }
        else if (lustat_cmd_line.display_size == DISPLAY_IN_GB )
        {
            sprintf(sprintf_buffer, "%d.%d GB       ", (fbe_u32_t)(actual_capacity_of_hs >> 10), (fbe_u32_t)((actual_capacity_of_hs & 0x3ff)/103));
        }
    }
    else
    {
        sprintf(sprintf_buffer, "INVALID        ");
    }
    status = fbe_api_get_object_lifecycle_state(object_id, &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
    /* Display the row of info for this object.
    */
    fbe_cli_printf(" 0x%x\t|%1d_%1d_%d\t|%s\t|0x%02x %10s\t|0x%02x %10s\t|%8s|%s\t\n",
                   object_id,    /* Hex display */
                   hs_details->port_number, hs_details->enclosure_number, hs_details->slot_number,
                   fbe_provision_drive_trace_get_config_type_string(hs_details->config_type),
                   lifecycle_state,    
                   fbe_base_object_trace_get_state_string(lifecycle_state),
                   hs_details->drive_type,
                   fbe_provision_drive_trace_get_drive_type_string(hs_details->drive_type),
                   sprintf_buffer,
                   hs_details->serial_num);
     
}
/******************************************
 * end fbe_cli_display_hot_spare_row()
 ******************************************/
 
/*!*******************************************************************
 * @var fbe_cli_hs_set_swap_delay
 *********************************************************************
 * @brief Function to set the delay in swapping of hot spare.
 *
 *  @param swap_delay   - Delay in swapping the hot spare (in seconds)
 *         spare_config - contain current permanent spare trigger time
 *
 * @return -  None.  
 *
 * @author
 *  4/09/2011 - Created. Vinay Patki
 *********************************************************************/
void fbe_cli_hs_set_swap_delay(fbe_u64_t swap_delay,fbe_api_virtual_drive_permanent_spare_trigger_time_t *spare_config)
{
    fbe_status_t status = FBE_STATUS_INVALID;
    

    if(swap_delay == 0)
    {
        fbe_cli_error("Can not set Swap delay to %d!\nValue must be other than Zero (0 seconds)\n",
            (int)swap_delay);
        return;
    }

    if(spare_config->permanent_spare_trigger_time == swap_delay)
    {
        fbe_cli_error("Can not set Swap delay to %d!\nValue must be other than Current value (%d)\n",
            (int)swap_delay,(int)spare_config->permanent_spare_trigger_time);
        return;
    }

    /*Set the delay value*/
    status = fbe_api_update_permanent_spare_swap_in_time(swap_delay);

    if (status != FBE_STATUS_OK)
    {
        /* Setting swap delay request failed then return error from here */
        fbe_cli_printf ("%s:Failed to set the delay of %d in Swap! %d,Status:0x%x\n",
                        __FUNCTION__, (int)spare_config->permanent_spare_trigger_time,(int)swap_delay, status);
        return;
    }

    fbe_cli_printf("Delay swap time for hot spare is successfully changed!\n");
    fbe_cli_printf("Current value: %d seconds    New value: %d seconds\n\n",(int)spare_config->permanent_spare_trigger_time,(int)swap_delay);
    return;

}
/******************************************
 * end fbe_cli_hs_set_swap_delay()
 ******************************************/
 
/*!*******************************************************************
 * @var fbe_cli_lustat
 *********************************************************************
 * @brief Function to implement displaying of LUNs.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  4/20/2010 - Created. Sanjay Bhave
 *********************************************************************/
void fbe_cli_lustat(fbe_s32_t argc, fbe_s8_t ** argv)
{
    fbe_status_t                status;
    fbe_object_id_t             rg_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                   rg_num;
    fbe_bool_t                  list_lun_in_rg = FBE_FALSE;
    fbe_sepls_display_object_t  current_display_format;
    fbe_sepls_display_object_t  lustat_display_format = FBE_SEPLS_DISPLAY_ONLY_LUN_OBJECTS;
    fbe_object_id_t             current_object_id_to_display = FBE_OBJECT_ID_INVALID;

    /* Do Parameter processing */
    current_display_format = fbe_cli_lib_sepls_get_display_format();
    if (argc == 0)
    {
        lustat_cmd_line.display_size = DISPLAY_NO_USER_PREF;
    }
    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /*  just display help and exit.
             */
            fbe_cli_printf("%s", LURG_LUSTAT_USAGE);
            return;
        }
        else if ((strcmp(*argv, "-rg") == 0))
        {
            /* Get the RG number from command line */
            argc--;
            argv++;

            if(argc > 0)
            {
                rg_num = atoi(*argv);

                /* Make sure that RG number received from command line is valid */
                status = fbe_api_database_lookup_raid_group_by_number(rg_num, &rg_id);
                if (status == FBE_STATUS_GENERIC_FAILURE)
                {
                    fbe_cli_error("RAID group %d does not exist or unable to retrieve information for this RAID Group.\n",rg_num);
                    return;
                }
            }

            list_lun_in_rg = FBE_TRUE;

            argc--;
            argv++;
        }
        else if ((strcmp(*argv, "-dsz") == 0))
        {
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("display size argument is expected\n");
                return;
            }
            /* Get the LUN display size in either mb/MB or gb/GB or tb/TB */
            /* Default display size is in GB */
            if ((_stricmp(*argv, "mb") == 0) ||
                (_stricmp(*argv, "MB") == 0) 
                )
            {
                lustat_cmd_line.display_size = DISPLAY_IN_MB;
            }
            if ((_stricmp(*argv, "gb") == 0) ||
                (_stricmp(*argv, "GB") == 0) 
                )
            {
                lustat_cmd_line.display_size = DISPLAY_IN_GB;
            }
            if ((_stricmp(*argv, "tb") == 0) ||
                (_stricmp(*argv, "TB") == 0) 
                )
            {
                lustat_cmd_line.display_size = DISPLAY_IN_TB;
            }
            argc--;
            argv++;
        }
        else
        {
            /* The command line parameters should be properly entered */
            fbe_cli_printf("%s", LURG_LUSTAT_USAGE);
            return;
        }
    }

    /* Print the header.
     */
    fbe_cli_printf("Type    Obj ID       ID    Object Info    Lifecycle State  Downstream                        Drives\n");
    fbe_cli_printf("                                                           Objects\n");
    fbe_cli_printf("------------------------------------------------------------------------------------------------------------------------------------------------------\n");

    /* List the LUNs in the RG */
    if (list_lun_in_rg == FBE_TRUE)
    {
        /* List the LUNs in the raid group object specified.
         */
        current_object_id_to_display = fbe_cli_lib_sepls_get_current_object_id_to_display();
        fbe_cli_lib_sepls_set_display_format(FBE_SEPLS_DISPLAY_OBJECT_TREE);
        fbe_cli_lib_sepls_set_object_id_to_display(rg_id);
        fbe_cli_sepls_rg_info(rg_id, FBE_PACKAGE_ID_SEP_0);
        fbe_cli_lib_sepls_set_object_id_to_display(current_object_id_to_display);
    }
    else
    {        
        /* List all the LUNs in the system.
         */
        fbe_cli_lib_sepls_set_display_format(lustat_display_format);
        fbe_cli_sepls_rg_info(FBE_OBJECT_ID_INVALID, FBE_PACKAGE_ID_SEP_0);	
    }    
    
    /* Restore the default display format.
     */
    fbe_cli_lib_sepls_set_display_format(current_display_format);
    
    return;
}
/******************************************
 * end fbe_cli_lustat()
 ******************************************/

/*!*******************************************************************
 * @var fbe_cli_unbind
 *********************************************************************
 * @brief Function to implement unbinding of LUNs.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  4/23/2010 - Created. Sanjay Bhave
 *********************************************************************/
void fbe_cli_unbind(fbe_s32_t argc, fbe_s8_t ** argv)
{
    fbe_api_lun_destroy_t lun_destroy_req;
    fbe_u32_t                         lun_num = FBE_OBJECT_ID_INVALID;
    fbe_status_t                      status;
    fbe_object_id_t                   lun_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t                        is_lun_id = FBE_FALSE;
    fbe_bool_t                        is_lun_number = FBE_FALSE;
    fbe_bool_t                        allow_destroy_broken_lun = FBE_FALSE;
    fbe_job_service_error_type_t      job_error_type;
	fbe_bool_t							continue_unbind = FBE_FALSE;


	/*unbind is dangerous, let's warn the user*/
	continue_unbind = fbe_cli_unbind_warn_user();
	if (FBE_IS_FALSE(continue_unbind)) {
		return;
	}

    fbe_zero_memory(&lun_destroy_req, sizeof(fbe_api_lun_destroy_t));
    if (argc == 0)
    {
        /* Print the help and return */
        fbe_cli_printf("%s", LURG_UNBIND_USAGE);
        return;
    }

    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /*  just display help and exit.
             */
            fbe_cli_printf("%s", LURG_UNBIND_USAGE);
            return;
        }
        else if ((strcmp(*argv, "-lun") == 0))
        {
            /* Get the RG number from command line */
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("LUN number argument is expected\n");
                return;
            }
            lun_num = atoi(*argv);

            is_lun_number = FBE_TRUE;
            /* Make sure that LUN number received from command line is valid */
            status = fbe_api_database_lookup_lun_by_number(lun_num, &lun_id);
            if (status == FBE_STATUS_GENERIC_FAILURE)
            {
                fbe_cli_error("LUN %d does not exist or unable to retrieve information for this LUN.",lun_num);
                return;
            }
            argc--;
            argv++;
        }
        else if ((strcmp(*argv, "-lun_id") == 0))
        {
            /* Get the RG number from command line */
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("LUN object id argument is expected\n");
                return;
            }
            lun_id = atoi(*argv);

            is_lun_id = FBE_TRUE;

            /* Make sure that LUN number received from command line is valid */
            status = fbe_api_database_lookup_lun_by_object_id(lun_id, &lun_num);
            if (status == FBE_STATUS_GENERIC_FAILURE)
            {
                fbe_cli_error("LUN %d does not exist or unable to retrieve information for this LUN.\n",lun_num);
                return;
            }
            argc--;
            argv++;
        }
        else if ((strcmp(*argv, "-unbind_broken_lun") == 0))
        {
            allow_destroy_broken_lun = FBE_TRUE;

            argc--;
            argv++;
        }
        else
        {
            /* Print the help and return */
            fbe_cli_printf("%s", LURG_UNBIND_USAGE);
            return;
        }
    }

    if(is_lun_number && is_lun_id)
    {
        fbe_cli_error("Please provide either Lun number OR object id of the lun to unbind, not both.\n");
        return;

    }
    lun_destroy_req.lun_number = lun_num;
    lun_destroy_req.allow_destroy_broken_lun = allow_destroy_broken_lun;

    status= fbe_api_destroy_lun(&lun_destroy_req, FBE_TRUE,LU_DESTROY_TIMEOUT, &job_error_type);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("not able to destroy a LUN %d\n", lun_num);
        return;
    }

    fbe_cli_printf("LUN %d deleted successfully\n", lun_num);

    fbe_cli_lun_operation_warning(lun_id, FBE_DATABASE_LUN_DESTROY_FBE_CLI, status);
    
    return;
}
/******************************************
 * end fbe_cli_unbind()
 ******************************************/

/*!*******************************************************************
* @var fbe_cli_createrg
*********************************************************************
* @brief Function to implement processing of FBE Cli command for creation of RG.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
* @return - none.  
*
* @author
*  4/29/2010 - Created. Vinay Patki
*********************************************************************/
void fbe_cli_createrg(fbe_s32_t argc, fbe_s8_t ** argv)
{

    fbe_raid_group_number_t                  raid_group;
    fbe_raid_group_type_t                    raid_type;
    fbe_object_id_t                          drive_count;
    fbe_u8_t                                 *rg_type = NULL;
    fbe_status_t                             status;
    fbe_object_id_t                          rg_id;
    fbe_api_raid_group_create_t  rg_request;

    fbe_bool_t                               is_rgid_given = FBE_FALSE;
    fbe_bool_t                               is_rgtype_given = FBE_FALSE;
    fbe_bool_t                               is_disk_given = FBE_FALSE;
    fbe_u64_t                                ps_idle_time = FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME;
    fbe_lba_t                                capacity = FBE_LBA_INVALID;
    fbe_bool_t                               b_bandwidth = FBE_FALSE;
    fbe_job_service_error_type_t             job_error_type;

    fbe_zero_memory(&rg_request, sizeof(fbe_api_raid_group_create_t));
    if (argc < 2)
    {
        fbe_cli_printf("%s Not enough arguments to create/ modify raid group\n", __FUNCTION__);
        fbe_cli_printf("%s", LURG_CREATERG_USAGE);
        return;
    }

    /* Set raid_group as invalid initially*/
    raid_group = FBE_OBJECT_ID_INVALID;

    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /* If they are asking for help, just display help and exit.
            */
            fbe_cli_printf("%s", LURG_CREATERG_USAGE);
            return;
        }
        else if (strcmp(*argv, "-rg") == 0)
        {
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("RAID group number argument is expected\n");
                return;
            }
            raid_group = atoi(*argv);
            is_rgid_given = FBE_TRUE;
            argc--;
            argv++;
        }
        else if (strcmp(*argv, "-type") == 0)
        {
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("RAID type argument is expected\n");
                return;
            }
            rg_type = *argv;
            is_rgtype_given = FBE_TRUE;
            argc--;
            argv++;
        }
        else if (strcmp(*argv, "-disk") == 0)
        {
            /* [FBECLI - TODO] */
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("disk argument is expected\n");
                return;
            }
            /*Get all the drives*/
            /* FBECLI - TODO*/
            for(drive_count = 0; argc > 0 && (strchr(*argv, '-') == NULL); drive_count++)
            {

                /* Validate that the disk numbers are given correctly*/
                /* Convert FRU or E_D number to B_E_D format*/
                /* Check for case where no drive is specified and return*/

                status = fbe_cli_convert_diskname_to_bed(argv[0], &rg_request.disk_array[drive_count]);

                if(status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("%s:\tSpecified disk number %s is invalid. %d\n", __FUNCTION__, *argv, status);
                    return;
                }

                is_disk_given = FBE_TRUE;

                argc--;
                argv++;

            }
        }
        else if (strcmp(*argv, "-cap") == 0)
        {
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Capacity argument is expected\n");
                return;
            }
            capacity = fbe_cli_convert_str_to_lba(*argv);
            argc--;
            argv++;
        }
        else if (strcmp(*argv, "-ps_idle_time") == 0)
        {
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("power saving idle time argument is expected\n");
                return;
            }
            ps_idle_time = _strtoui64(*argv, NULL, 10);
            argc--;
            argv++;
        }
        else if (strcmp(*argv, "-large_elsz") == 0)
        {
            argc--;
            argv++;
            b_bandwidth = FBE_TRUE;
        }
        else if (strcmp(*argv, "-userprivate") == 0)
        {
            argc--;
            argv++;
            rg_request.user_private = FBE_TRUE;
        }
        else
        {
            fbe_cli_printf("%s Not enough arguments to create/ modify raid group\n", __FUNCTION__);
            fbe_cli_printf("%s", LURG_CREATERG_USAGE);
            return;
        }
    }

    if(!(is_rgtype_given && is_disk_given))
    {
        fbe_cli_printf("%s:\tRG type and Disks are mandatory for creating RG.\n", __FUNCTION__);
        fbe_cli_printf("%s", LURG_CREATERG_USAGE);
        return;
    }

    /* [FBECLI - TODO] */
    /* If RG Number is not given (or if it will be set to UNSIGNED_MINUS_1), set the RG number to current best smallest possible number*/
    /* -rg is not a mandatory option - April 30, 2010*/
    if(raid_group == FBE_OBJECT_ID_INVALID)
    {
        status = fbe_cli_find_smallest_free_raid_group_number(&raid_group);

        if (status == FBE_STATUS_INSUFFICIENT_RESOURCES)
        {
            fbe_cli_printf("%s:\tMaximum number of RG (%d) exists. RG creation is not allowed. %d\n", __FUNCTION__, MAX_RAID_GROUP_ID, status);
            return;
        }

         fbe_cli_printf("%s:\tRG number is not mentioned. Creating RG having number %d.\n", __FUNCTION__, raid_group);
    }
    else
    {
        /* Check if RG number is a supported number on this system */
        if(fbe_cli_is_valid_raid_group_number(raid_group) != FBE_TRUE)
        {
            fbe_cli_printf("%s:\tRG number %d is not supported. %d\n", __FUNCTION__, raid_group, status);
            return;
        }

        /* Check if RG already exists */
        status = fbe_api_database_lookup_raid_group_by_number(raid_group, &rg_id);

        if (status == FBE_STATUS_OK)
        {
            fbe_cli_printf("%s:\tRG %d already exist. %d\n", __FUNCTION__, raid_group, status);
            return;
        }
    }
    /* Check validity of Raid Type*/
    raid_type = fbe_cli_check_valid_raid_type(rg_type);

    if(raid_type == FBE_RAID_GROUP_TYPE_UNKNOWN)
    {
        fbe_cli_printf("%s:\tRaid type %s given for RG creation is invalid.\n", __FUNCTION__, rg_type);
        return;
    }


    /* Check validity of disk mentioned for given RG type*/
    /* (drive_count-1) will give number of disk mentioned on command line*/
    status = fbe_cli_check_disks_for_raid_type(drive_count, raid_type);

    if(status != FBE_STATUS_OK)
    {

        if(status == FBE_STATUS_INSUFFICIENT_RESOURCES)
        {
            fbe_cli_printf("%s:\tDrives provided for RG type %s are insufficient.\n", __FUNCTION__, rg_type);
            return;
        }

        if(status == FBE_STATUS_INVALID)
        {
            fbe_cli_printf("%s:\tRaid type %s given for RG creation is invalid.\n", __FUNCTION__, rg_type);
        }

    }

    /* Fillup all other parameters*/
    rg_request.b_bandwidth = b_bandwidth;
    rg_request.capacity = capacity;
    rg_request.power_saving_idle_time_in_seconds = (ps_idle_time != 0 ? ps_idle_time : 120);
    rg_request.max_raid_latency_time_is_sec = 120;
    rg_request.raid_type = raid_type;
    rg_request.raid_group_id = raid_group;
    //rg_request.explicit_removal = 0;
    rg_request.is_private = 0;
    rg_request.drive_count = drive_count;

    /*Send request to create a RG*/
    status = fbe_api_create_rg(&rg_request, FBE_TRUE, RG_WAIT_TIMEOUT, &rg_id, &job_error_type);

    if (status != FBE_STATUS_OK)
    {
        /* if RG creation request failed then return error from here */
        fbe_cli_printf ("%s:RG creation failed...can not proceed with LUN creation\n", __FUNCTION__);
        return;
     }

    return;
}

/******************************************
 * end fbe_cli_createrg()
 ******************************************/

/*!*******************************************************************
* @var fbe_cli_removerg
*********************************************************************
* @brief Function to implement processing of FBE Cli command for removal of RG.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
* @return - none.  
*
* @author
*  5/06/2010 - Created. Vinay Patki
*********************************************************************/
void fbe_cli_removerg(fbe_s32_t argc, fbe_s8_t ** argv)
{
    fbe_status_t status;
    fbe_object_id_t object_id;
    fbe_api_job_service_raid_group_destroy_t *in_fbe_raid_group_destroy_req_p = NULL;
    fbe_job_service_error_type_t              error_type = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    fbe_status_t                              job_status = FBE_STATUS_OK;

    fbe_bool_t is_rg_id = FBE_FALSE;
    fbe_bool_t is_object_id = FBE_FALSE;

    if (argc == 0)
    {
        fbe_cli_printf("%s Not enough arguments to create/ modify raid group\n", __FUNCTION__);
        fbe_cli_printf("%s", LURG_REMOVERG_USAGE);
        return;
    }

    in_fbe_raid_group_destroy_req_p = (fbe_api_job_service_raid_group_destroy_t *) fbe_api_allocate_memory(sizeof(fbe_api_job_service_raid_group_destroy_t));

    /* Set raid_group as invalid initially*/
    /* TODO: check later whether raid_group_id can be set to INVALID OBJECT */
    in_fbe_raid_group_destroy_req_p->raid_group_id = FBE_OBJECT_ID_INVALID;
    in_fbe_raid_group_destroy_req_p->wait_destroy = FBE_FALSE; 
    in_fbe_raid_group_destroy_req_p->destroy_timeout_msec = RG_WAIT_TIMEOUT;

    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0) || (argc > 3))
        {
            /* If they are asking for help, just display help and exit.
            */
            fbe_cli_printf("%s", LURG_REMOVERG_USAGE);
            return;
        }
        else if (strcmp(*argv, "-rg") == 0)
        {
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("RAID group number argument is expected\n");
                return;
            }
            in_fbe_raid_group_destroy_req_p->raid_group_id = atoi(*argv);
            is_rg_id = FBE_TRUE;
            argc--;
            argv++;
        }
        else if (strcmp(*argv, "-objid") == 0)
        {
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("RAID group object id argument is expected\n");
                return;
            }
            object_id = atoi(*argv);
            is_object_id = FBE_TRUE;
            argc--;
            argv++;
        }
        else if (strcmp(*argv, "-remove_broken_rg") == 0)
        {
            in_fbe_raid_group_destroy_req_p->allow_destroy_broken_rg = FBE_TRUE;
            argc--;
            argv++;
        }
        else
        {
            /* The command line parameters should be properly entered */
            fbe_cli_printf("%s", LURG_REMOVERG_USAGE);
            return;
        }
    }

    if(is_rg_id == FBE_TRUE && is_object_id == FBE_FALSE)
    {
        /* Check validity of RG number */
        status = fbe_api_database_lookup_raid_group_by_number(
                           in_fbe_raid_group_destroy_req_p->raid_group_id, &object_id);
    
    }
    else if(is_rg_id == FBE_FALSE && is_object_id == FBE_TRUE)
    {
        /* Check validity of Object ID and get RG number */
        status = fbe_api_database_lookup_raid_group_by_object_id(
                           object_id, &in_fbe_raid_group_destroy_req_p->raid_group_id);
    
    }
    else if(is_rg_id == FBE_TRUE && is_object_id == FBE_TRUE)
    {
    // Check if both are for same RG
        fbe_object_id_t object_id_check;
    
        /* Check validity of RG number */
        status = fbe_api_database_lookup_raid_group_by_number(
                           in_fbe_raid_group_destroy_req_p->raid_group_id, &object_id_check);
    
        if(object_id != object_id_check)
        {
            fbe_cli_printf("%s:\tRG id %d and Object ID %d does not match. %d\n", __FUNCTION__,
                           in_fbe_raid_group_destroy_req_p->raid_group_id, object_id, status);
            fbe_cli_printf("No Raid group was removed./n");
            return;
        }
    }
    else
    {
        fbe_cli_printf("%s:\tUser did not supply required arguments (RG ID or Object ID). \n", __FUNCTION__);
        fbe_cli_printf("No Raid group was removed./n");
        fbe_cli_printf("%s", LURG_REMOVERG_USAGE);

        return;
    }
    
    /* Check the status for SUCCESS*/
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s:\tRG %d is invalid. %d\n", __FUNCTION__,
                in_fbe_raid_group_destroy_req_p->raid_group_id, status);

        return;
    }

    /* Now destroy the raid group*/
    status = fbe_api_job_service_raid_group_destroy(in_fbe_raid_group_destroy_req_p);

    /* Check the status for SUCCESS*/
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s:\tRG %d removal failed %d\n", __FUNCTION__,
                in_fbe_raid_group_destroy_req_p->raid_group_id, status);
        return;
    }

    /* wait for it*/
    status = fbe_api_common_wait_for_job(in_fbe_raid_group_destroy_req_p->job_number, 
                                         RG_WAIT_TIMEOUT, &error_type, &job_status, NULL);
    if (status != FBE_STATUS_OK || job_status != FBE_STATUS_OK) 
    {
        fbe_cli_printf("%s: wait for RG job failed. Status: 0x%X, job status:0x%X, job error:0x%x\n", 
                       __FUNCTION__, status, job_status, error_type);

        return;
    }

    /* RG probably has LUNs in it.. Check for it.. */
    if(error_type == FBE_JOB_SERVICE_ERROR_REQUEST_OBJECT_HAS_UPSTREAM_EDGES)
    {
        fbe_cli_printf("RG: %d has upstream objects in it (probably LUNs).. Delete those first..\n",
                       in_fbe_raid_group_destroy_req_p->raid_group_id);
    }

    /* Check the database if the RG is really removed.. If so, we will get an OBJECT_ID_INVALID. 
     * If not, then check SP logs for more info. There is a chance that RG contains some LUNs.. 
     * We cannot destroy a RG if it has LUNs or upstream edges connected to it.. 
     */
    status = fbe_api_database_lookup_raid_group_by_number(
                       in_fbe_raid_group_destroy_req_p->raid_group_id, &object_id);

    if(object_id == FBE_OBJECT_ID_INVALID)
    {
        /*** Print the success message***/
        fbe_cli_printf("%s:\tRG %d deleted successfully. %d\n", __FUNCTION__,
                    in_fbe_raid_group_destroy_req_p->raid_group_id, status);
    }
    else
    {
        /*** Print the failure message***/
        fbe_cli_printf("RG %d (Obj: 0x%x) destroy Failed.\n", 
                       in_fbe_raid_group_destroy_req_p->raid_group_id, object_id);
    }

    return;
}

/******************************************
 * end fbe_cli_removerg()
 ******************************************/
 /*************************************************************************
 *                          fbecli_validate_atoi_args()
 *************************************************************************
 *
 *  DESCRIPTION:
 *    fbecli_validate_atoi_arg -- this function is used to check the options
 *  entered in fbecli to make sure that they are numeric.
 *   *
 *  PARAMETERS:
 *    arg_value (INPUT) POINTER TO USER SPECIFIED OPTION
 *
 *
 *  RETURN VALUES:
 *    BOOL arg_is_valid 
 *       TRUE - the string input by the user contains numeric characters
 *       FALSE - the string contained non numeric characters  
 *
 *  NOTES:
 *    Usage:
 *
 *  HISTORY:
 *    21-sept-2010 DDY -- Created
 *************************************************************************/

fbe_bool_t fbecli_validate_atoi_args(const TEXT * arg_value)
{

/*********************************
 **    VARIABLE DECLARATIONS    **
 *********************************/

    fbe_bool_t arg_is_valid = TRUE;   /* return value, is this option acceptable */
    fbe_u32_t loop_index;          /* index to loop through arg_value */


/*****************
 **    BEGIN    **
 *****************/

    if(arg_value == NULL)
    {
        return(FALSE);
    }

    for (loop_index = 0; loop_index < strlen(arg_value); loop_index++)
    {
        /* Check that all characters in the array are numeric */

        if (!isxdigit(arg_value[loop_index]))
        {
            arg_is_valid = FALSE;
        }
    }
    return arg_is_valid;
}

/*****************************************
 ** End of fbecli_validate_atoi_args()    **
 *****************************************/

/*!*******************************************************************
* @var fbe_cli_convert_diskname_to_bed
*********************************************************************
*
*  @param    disk_name_a         - disk name
*
* @return - none.  
*
* @author
*  4/29/2010 - Created. Vinay Patki
*********************************************************************/
fbe_status_t  fbe_cli_convert_diskname_to_bed(fbe_u8_t disk_name_a[], fbe_job_service_bes_t * phys_location)
{
    /*********************************
    **    VARIABLE DECLARATIONS    **
    *********************************/
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t  bus = FBE_PORT_NUMBER_INVALID;
    fbe_u32_t  encl = FBE_ENCLOSURE_NUMBER_INVALID;
    fbe_u32_t  disk = FBE_SLOT_NUMBER_INVALID;
    fbe_object_id_t pdo_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t  scanned = 0;
    fbe_u8_t   *first_ocr;
    fbe_u8_t   *second_ocr;
    fbe_u8_t   *third_ocr;

    /*****************
    **    BEGIN    **
    *****************/
    if(disk_name_a == NULL)
    {
        fbe_cli_printf("Invalid disk name: %s\n", disk_name_a);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Validate that input buffer is in correct format
     * Correct format is [0-9][0-9]_[0-9][0-9]_[0-9][0-9][0-9]
     * Make sure that there are 2 underscores in the buffer 
     * no more and no less.
     */
    first_ocr = (fbe_u8_t*) strchr((const char*)disk_name_a, '_');
    if (first_ocr == NULL)
    {
        fbe_cli_printf("The disk string is not in b_e_d format\n");
        return FBE_STATUS_INVALID;
    }

    /* go to one position next in the string */
    first_ocr++;

    second_ocr = (fbe_u8_t*) strchr((const char*)first_ocr, '_');
    if (second_ocr == NULL)
    {
        fbe_cli_printf("The disk string is not in b_e_d format\n");
        return FBE_STATUS_INVALID;
    }

    /* go to one position next in the string */
    second_ocr++;

    /* Now we know that there are 2 _ in the input 
     * Make sure that there are no more than 2 _ in input 
     */
    third_ocr = (fbe_u8_t*) strchr((const char*)second_ocr, '_');
    if (third_ocr != NULL)
    {
        /* We got a problem here there are 3 _ in the input 
         * not allowed
         */
        fbe_cli_printf("The disk string is not in b_e_d format\n");
        return FBE_STATUS_INVALID;
    }

    /* initialize the bus, enclosure, and slot number */
    phys_location->bus = FBE_API_PHYSICAL_BUS_COUNT;
    phys_location->enclosure = FBE_API_ENCLOSURE_COUNT;
    phys_location->slot = FBE_API_ENCLOSURE_SLOTS;

    /* scan to ge the bus, encl, and disk info */
    scanned = sscanf((const char*)disk_name_a, "%d_%d_%d", &bus, &encl, &disk);
    if (scanned < 3)
    {
        fbe_cli_printf("Unable to scan the input correctly\n");
        return FBE_STATUS_INVALID;
    }

    /* check to make sure that the bus, encl, and disk are not invalid. */
    if ((bus == FBE_PORT_NUMBER_INVALID) &&  
        (encl == FBE_ENCLOSURE_NUMBER_INVALID) &&
        (disk == FBE_SLOT_NUMBER_INVALID))
    {
        fbe_cli_printf("Disk info: %d_%d_%d is invalid.\n", bus, encl, disk);
        return FBE_STATUS_NO_DEVICE;
    }

    /* Validate that the drive exist.
     */
    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, disk, &pdo_object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get PDO object id for %d_%d_%d - status: %d\n", bus, encl, disk, status);
        return status;
    }

    /* assign the bus, enclosure, and disk to physical location */
    phys_location->bus = bus;
    phys_location->enclosure = encl;
    phys_location->slot = disk;

    return FBE_STATUS_OK;
}

/******************************************
 * end fbe_cli_convert_diskname_to_bed()
 ******************************************/
 
/*!*******************************************************************
* @var fbe_cli_convert_encl_to_be
*********************************************************************
*  @brief
*  This function accepts the argument (enclosure id received in B_E format)
*  and identifies the Bus and Enclosure number
*
*
*
*  @param    disk_name_a         - disk name
*
*  @return - none.  
*
* @author
*  01/19/2011 - Created. Vinay Patki
*********************************************************************/
fbe_status_t  fbe_cli_convert_encl_to_be(fbe_u8_t disk_name_a[], fbe_job_service_be_t * phys_location)
{
    /*********************************
    **    VARIABLE DECLARATIONS    **
    *********************************/

    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t  bus = FBE_PORT_NUMBER_INVALID;
    fbe_u32_t  encl = FBE_ENCLOSURE_NUMBER_INVALID;
    fbe_u32_t  scanned = 0;
    fbe_u8_t   *first_ocr;
    fbe_u8_t   *second_ocr;
    fbe_object_id_t encl_object_id = FBE_OBJECT_ID_INVALID;

    /*****************
    **    BEGIN    **
    *****************/
    if(disk_name_a == NULL)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }
    /* Validate that input buffer is in correct format
     * Correct format is [0-9][0-9]_[0-9][0-9]_[0-9][0-9][0-9]
     * Make sure that there are 2 underscores in the buffer 
     * no more and no less.
     */
    first_ocr = (fbe_u8_t*) strchr((const char*)disk_name_a, '_');
    if (first_ocr == NULL)
    {
        fbe_cli_printf("The disk string is not in b_e format\n");
        return FBE_STATUS_INVALID;
    }

    /* go to one position next in the string */
    first_ocr++;

    /* Now we know that there is 1 "_" in the input 
     * Make sure that there are not two or more "_" in input
     */
    second_ocr = (fbe_u8_t*) strchr((const char*)first_ocr, '_');
    if (second_ocr != NULL)
    {
        fbe_cli_printf("The disk string is not in b_e format\n");
        return FBE_STATUS_INVALID;
    }

    /* initialize the bus, enclosure, and slot number */
    phys_location->bus = FBE_API_PHYSICAL_BUS_COUNT;
    phys_location->enclosure = FBE_API_ENCLOSURE_COUNT;

    /* scan to ge the bus and encl info */
    scanned = sscanf((const char*)disk_name_a, "%d_%d", &bus, &encl);
    if (scanned < 2)
    {
        fbe_cli_printf("Unable to scan the input correctly\n");
        return FBE_STATUS_INVALID;
    }

    /* Validate that the enclosure exists.
     */
    status = fbe_api_get_enclosure_object_id_by_location(bus, encl, &encl_object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to enclosure object id for %d_%d - status %d\n", bus, encl, status);
        return status;
    }

    phys_location->bus = bus;
    phys_location->enclosure = encl;

    return FBE_STATUS_OK;

}

/******************************************
 * end fbe_cli_convert_encl_to_be()
 ******************************************/

/*!*******************************************************************
* @var fbe_cli_check_disks_for_raid_type
*********************************************************************
*
*
*  @param    argc - argument count
*  @param    argv - argument string
*
* @return - none.  
*
* @author
*  4/29/2010 - Created. Vinay Patki
*********************************************************************/
fbe_status_t fbe_cli_check_disks_for_raid_type(fbe_object_id_t drive_count, fbe_raid_group_type_t raid_type)
{

    /*
    Raid 0    - 3 -16
    Raid 1    - 2
    Raid 1/0  - Even (multiple of 2)
    Raid 3    - 5 or 9
    Raid 5    - 3-16
    Raid 6    - 4,6,8,10,12,14,16
    Raid Disk - 1
    Raid HS   - 1
    */

    if (raid_type == FBE_RAID_GROUP_TYPE_RAID0)
    {
        if ( (drive_count == RAID_INDIVIDUAL_DISK_WIDTH) ||
             ((drive_count > RAID_STRIPED_MIN_DISKS) && (drive_count <= RAID_STRIPED_MAX_DISK)))
        {
            return FBE_STATUS_OK;
        }

        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* RAID-1 (mirrored disks) */
    else if (raid_type == FBE_RAID_GROUP_TYPE_RAID1)
    {
        if(drive_count == RAID_MIRROR_MIN_DISKS)
        {
            return FBE_STATUS_OK;
        }

        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* RAID-3 */
    else if (raid_type == FBE_RAID_GROUP_TYPE_RAID3)
    {
        if((drive_count != RAID3_STRIPED_MIN_DISK) || (drive_count != RAID3_STRIPED_MAX_DISK))
        {
            return FBE_STATUS_OK;
        }

        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* RAID-5 */
    else if (raid_type == FBE_RAID_GROUP_TYPE_RAID5)
    {
        if((drive_count > RAID_STRIPED_MIN_DISKS) && (drive_count < RAID_STRIPED_MAX_DISK))
        {
            return FBE_STATUS_OK;
        }

        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* RAID-6 */
    else if (raid_type == FBE_RAID_GROUP_TYPE_RAID6)
    {
        if((drive_count > RAID6_STRIPED_MIN_DISK) && (drive_count < RAID_STRIPED_MAX_DISK) && (drive_count % 2 == 0))
        {
            return FBE_STATUS_OK;
        }

        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* RAID-1/0 */
    else if (raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        if((drive_count > RAID10_MIR_STRIPED_MIN_DISK) && (drive_count < RAID_STRIPED_MAX_DISK) && (drive_count % 2 == 0))
        {
            return FBE_STATUS_OK;
        }

        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* hot spare */
    else if (raid_type == FBE_RAID_GROUP_TYPE_SPARE)
    {
        if(drive_count == HOT_SPARE_NUM_DISK)
        {
            return FBE_STATUS_OK;
        }

        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* [FBE CLI - TODO] */
    /* Individual disks */

    else
    {
        /* unknown RAID type */
        fbe_cli_printf("Unknown RAID type %d, (r0,r1,r3,r5,r6,hi5,r1_0,id,hs)\n",
		       raid_type);
        return FBE_STATUS_INVALID;
    }   
}

/******************************************
 * end fbe_cli_check_disks_for_raid_type()
 ******************************************/

/*!*******************************************************************
* @var fbe_cli_check_valid_raid_type
*********************************************************************
*
*
*  @param    argc - argument count
*  @param    argv - argument string
*
* @return - none.  
*
* @author
*  4/29/2010 - Created. Vinay Patki
*********************************************************************/
fbe_raid_group_type_t fbe_cli_check_valid_raid_type(fbe_u8_t *rg_type)
{
    /* if input is NULL pointer then send error back */
    if (rg_type == NULL)
    {
        fbe_cli_printf("NULL rg_type received\n");
        return FBE_RAID_GROUP_TYPE_UNKNOWN;
    }
    /* RAID-0 */
    if (strcmp(rg_type, "r0") == 0)
    {
        return FBE_RAID_GROUP_TYPE_RAID0;
    }
    /* RAID-0 */
    if (strcmp(rg_type, "id") == 0)
    {
        return FBE_RAID_GROUP_TYPE_RAID0;
    }

    /* RAID-1 (mirrored disks) */
    else if (strcmp(rg_type, "r1") == 0)
    {
        return FBE_RAID_GROUP_TYPE_RAID1;
    }

    /* RAID-3 */
    else if (strcmp(rg_type, "r3") == 0)
    {
        return FBE_RAID_GROUP_TYPE_RAID3;
    }

    /* RAID-5 */
    else if (strcmp(rg_type, "r5") == 0)
    {
        return FBE_RAID_GROUP_TYPE_RAID5;
    }

    /* RAID-6 */
    else if (strcmp(rg_type, "r6") == 0)
    {
        return FBE_RAID_GROUP_TYPE_RAID6;
    }

    /* RAID-1/0 */
    else if (strcmp(rg_type, "r1_0") == 0)
    {
        return FBE_RAID_GROUP_TYPE_RAID10;
    }

    /* RAID-Hotspare */
    else if (strcmp(rg_type, "hs") == 0)
    {
        return FBE_RAID_GROUP_TYPE_SPARE;
    }

    /* [FBE CLI - TODO] */
    /* Individual disks */

    else
    {
        /* unknown RAID type */
        fbe_cli_printf("\nUnknown RAID type %s, (r0,r1,r3,r5,r6,r1_0,hs,id)\n",rg_type);

        return FBE_RAID_GROUP_TYPE_UNKNOWN;
    }
}

/******************************************
 * end fbe_cli_check_valid_raid_type()
 ******************************************/


/*!**************************************************************
 * fbe_cli_is_valid_raid_group_number()
 ****************************************************************
 * @brief
 * This function is used to determine if the Raid Group id is valid
 *
 * @param raid_group_id.
 *
 * @return TRUE - The raid group id is valid
 * @return FALSE - The raid group id is invalid
 *
 * @author
 *  05/06/2010 - Created. Vinay Patki
 ****************************************************************/
static fbe_bool_t fbe_cli_is_valid_raid_group_number(fbe_raid_group_number_t rg_id)
{
    if (((rg_id > (FBE_RG_USER_COUNT - 1)) && (rg_id < RAID_GROUP_COUNT )) ||
         (rg_id > MAX_RAID_GROUP_ID))
    {
        return (FBE_FALSE);
    }

    return (FBE_TRUE);

}
/******************************************
 * end fbe_cli_is_valid_raid_group_number()
 ******************************************/
 
/*!*******************************************************************
 * @var fbe_cli_get_state_name
 *********************************************************************
 * @brief Function to return TEXT corresponding to lifecycle state.
 *              Any addition of LifeCycle state should be added here.
 *
 *  @param    state - life cycle state of object
 *  @param    pp_state_name - TEXT corresponding to lifecycle state enumeration.
 *
 * @return -FBE status.  
 *
 * @author
 *  4/27/2010 - Created. Sanjay Bhave
 *********************************************************************/
fbe_status_t
fbe_cli_get_state_name(fbe_lifecycle_state_t state, const fbe_char_t  ** pp_state_name)
{
    *pp_state_name = NULL;
    switch (state) {
        case FBE_LIFECYCLE_STATE_SPECIALIZE:
            *pp_state_name = "SPECLZ";
            break;
        case FBE_LIFECYCLE_STATE_ACTIVATE:
            *pp_state_name = "ACTVT";
            break;
        case FBE_LIFECYCLE_STATE_READY:
            *pp_state_name = "READY";
            break;
        case FBE_LIFECYCLE_STATE_HIBERNATE:
            *pp_state_name = "HIBR";
            break;
        case FBE_LIFECYCLE_STATE_OFFLINE:
            *pp_state_name = "OFFLN";
            break;
        case FBE_LIFECYCLE_STATE_FAIL:
            *pp_state_name = "FAIL";
            break;
        case FBE_LIFECYCLE_STATE_DESTROY:
            *pp_state_name = "DSTRY";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_READY:
            *pp_state_name = "PND_RDY";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_ACTIVATE:
            *pp_state_name = "PND_ACT";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_HIBERNATE:
            *pp_state_name = "PND_HIBR";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_OFFLINE:
            *pp_state_name = "PND_OFFLN";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_FAIL:
            *pp_state_name = "PND_FAIL";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_DESTROY:
            *pp_state_name = "PND_DSTRY";
            break;
        case FBE_LIFECYCLE_STATE_NOT_EXIST:
            *pp_state_name = "ST_NT_EXST";
            break;
        case FBE_LIFECYCLE_STATE_INVALID:
            *pp_state_name = "INVALID";
            break;
        default:
            return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cli_get_state_name()
 ******************************************/
 
/*!*******************************************************************
 * @var fbe_cli_get_RG_name
 *********************************************************************
 * @brief Function to return type of RG given the enumeration number.
 *              Any addition of RG type should be added here.
 *
 *  @param    rg_type - enumeration for type of RG
 *  @param    pp_raid_type_name - TEXT corresponding to RAID enumeration.
 *
 * @return -FBE status.  
 *
 * @author
 *  4/27/2010 - Created. Sanjay Bhave
 *********************************************************************/
fbe_status_t
fbe_cli_get_RG_name(fbe_u32_t  rg_type, const fbe_char_t** pp_raid_type_name)
{
    *pp_raid_type_name = NULL;
    switch (rg_type) {
        case FBE_RAID_GROUP_TYPE_UNKNOWN:
            *pp_raid_type_name = "UNKNOWN";
            break;
        case FBE_RAID_GROUP_TYPE_RAID1:
            *pp_raid_type_name = "RAID-1";
            break;
        case FBE_RAID_GROUP_TYPE_RAID10:
            *pp_raid_type_name = "RAID1_0";
            break;
        case FBE_RAID_GROUP_TYPE_RAID3:
            *pp_raid_type_name = "RAID-3";
            break;
        case FBE_RAID_GROUP_TYPE_RAID0:
            *pp_raid_type_name = "RAID-0";
            break;
        case FBE_RAID_GROUP_TYPE_RAID5:
            *pp_raid_type_name = "RAID-5";
            break;
        case FBE_RAID_GROUP_TYPE_RAID6:
            *pp_raid_type_name = "RAID-6";
            break;
        case FBE_RAID_GROUP_TYPE_SPARE:
            *pp_raid_type_name = "HS";
            break;
        case FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER:
            *pp_raid_type_name = "STRIPER/MIRROR";
            break;
        case FBE_RAID_GROUP_TYPE_INTERNAL_METADATA_MIRROR:
            *pp_raid_type_name = "MIRROR META";
            break;
        default:
            return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cli_get_RG_name()
 ******************************************/

/*!*******************************************************************
 * @var fbe_cli_get_lun_details
 *********************************************************************
 * @brief Function to get all the information about LUN
 *
 *  @param    lun_id - object ID corresponding to a LUN in the system.
 *
 * @return -  FBE status.  
 *
 * @author
 *  4/29/2010 - Created. Sanjay Bhave
 *********************************************************************/
fbe_status_t fbe_cli_get_lun_details(fbe_object_id_t lun_id, fbe_cli_lurg_lun_details_t* lun_details)
{
    fbe_status_t status;
    status = fbe_api_database_lookup_lun_by_object_id(lun_id, &lun_details->lun_number);
    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        /* The LUN does not exist or object ID is not for a LUN send error back anyways */
        return status;
    }
    status = fbe_api_get_object_lifecycle_state(lun_id, &lun_details->lifecycle_state, FBE_PACKAGE_ID_SEP_0);
    status = fbe_cli_get_state_name(lun_details->lifecycle_state, &lun_details->p_lifecycle_state_str);

    lun_details->lun_info.lun_object_id = lun_id;
    status = fbe_api_database_get_lun_info(&lun_details->lun_info);
    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        /* send error back since not able to retrieve LUN details anyways */
        return status;
    }

    status = fbe_api_lun_get_zero_status(lun_id, &lun_details->fbe_api_lun_get_zero_status);
    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        /* send error back since not able to retrieve LUN details anyways */
        return status;
    }

    status = fbe_cli_get_RG_name(lun_details->lun_info.raid_info.raid_type, &lun_details->p_rg_str);
    return FBE_STATUS_OK;

}

/******************************************
 * end fbe_cli_get_lun_details()
 ******************************************/

/*!*******************************************************************
 * @var fbe_cli_get_rg_details
 *********************************************************************
 * @brief Function to get all the information about RG
 *
 *  @param    lun_id     - object ID corresponding to a LUN in the system.
 *            rg_details - details of RG to which LUN belongs.
 *
 * @return -  FBE status.  
 *
 * @author
 *  4/29/2010 - Created. Sanjay Bhave
 *********************************************************************/
fbe_status_t fbe_cli_get_rg_details(fbe_object_id_t obj_id, fbe_cli_lurg_rg_details_t* rg_details)
{
    fbe_status_t                                 status = FBE_STATUS_OK;
    fbe_api_base_config_downstream_object_list_t downstream_object_list = {0};
    fbe_api_base_config_downstream_object_list_t downstream_raid_object_list = {0};
    fbe_u32_t                                    index, pvd_obj_indx,disk_index,vd_index = 0,loop_count = 0;
    fbe_u32_t                                    rg_id;
    fbe_class_id_t                               class_id = FBE_CLASS_ID_INVALID;
    fbe_status_t                                 lifecycle_status = FBE_STATUS_INVALID;
    fbe_lifecycle_state_t                        lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_bool_t                                   is_system_rg = FBE_FALSE;
    fbe_api_virtual_drive_get_configuration_t    configuration_mode;
    
    /* Find out if given obj ID is for RAID group or LUN */
    status = fbe_api_get_object_class_id(obj_id, &class_id, FBE_PACKAGE_ID_SEP_0);
    /* if status is generic failure then return that back */
   
    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    if (status == FBE_STATUS_BUSY)
    {
        lifecycle_status = fbe_api_get_object_lifecycle_state(obj_id, 
                                                              &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
        if(lifecycle_status != FBE_STATUS_OK)
        {
            fbe_cli_error("Fail to get the Lifecycle State for object id:0x%x, status:0x%x\n",
                           obj_id,lifecycle_status);
        }
        else if (lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE)
            return status;
    }

    /* check which class id it is */
    if (class_id == FBE_CLASS_ID_LUN)
    {
        /* initialize the downstream object list. */
        downstream_object_list.number_of_downstream_objects = 0;
        for(index = 0; index < FBE_MAX_DOWNSTREAM_OBJECTS; index++)
        {
            downstream_object_list.downstream_object_list[index] = FBE_OBJECT_ID_INVALID;
        }

        status = fbe_api_base_config_get_downstream_object_list(obj_id, &downstream_object_list);

        /* There has to be RG ID associated with LUN object */
        if (downstream_object_list.number_of_downstream_objects == 0)
        {
            fbe_cli_error("No RAID group associated with LUN %d",obj_id);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        rg_id = downstream_object_list.downstream_object_list[0];
    }
    else
    {
        rg_id = obj_id;
    }

    if (rg_details->direction_to_fetch_object & DIRECTION_DOWNSTREAM)
    {
        /* Get the Drives associated with this RG */
    
        /* initialize the downstream object list. */
        rg_details->downstream_vd_object_list.number_of_downstream_objects = 0;
        for(index = 0; index < FBE_MAX_DOWNSTREAM_OBJECTS; index++)
        {
            rg_details->downstream_vd_object_list.downstream_object_list[index] = FBE_OBJECT_ID_INVALID;
            rg_details->downstream_vd_mode_list[index] = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;
        }

        rg_details->downstream_pvd_object_list.number_of_downstream_objects = 0;
        for(index = 0; index < FBE_MAX_DOWNSTREAM_OBJECTS + 1; index++)
        {
            rg_details->downstream_pvd_object_list.downstream_object_list[index] = FBE_OBJECT_ID_INVALID;
        }

        /* If this is a virtual drive skip the raid group id (doesn't have one).
         */
        if (class_id != FBE_CLASS_ID_VIRTUAL_DRIVE)
        {
            /* Get the RG number associated with this RG */
            status = fbe_api_database_lookup_raid_group_by_object_id(rg_id, &rg_details->rg_number);
    
            /* RG ID must have RG number associated with it */
            if (status == FBE_STATUS_GENERIC_FAILURE)
            {
                /* Return the status back indicating that LUN is not associated 
                 * RG having valid RG number 
                 */
                return status;
            }
        }

        /* Get Virtual Drives associated with this RG */
        status = fbe_api_base_config_get_downstream_object_list(rg_id, &downstream_raid_object_list);
        if (status == FBE_STATUS_GENERIC_FAILURE)
        {
            return status;
        }

        /* 1) check whether it is a system rg or not
           2) deal with it accordingly
        */
        fbe_api_database_is_system_object(rg_id, &is_system_rg); 
        if ((is_system_rg) || (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE))
        {
            for(index = 0; index < downstream_raid_object_list.number_of_downstream_objects ;index ++)
            {
                rg_details->downstream_pvd_object_list.downstream_object_list[index] = downstream_raid_object_list.downstream_object_list[index];
                rg_details->downstream_pvd_object_list.number_of_downstream_objects = downstream_raid_object_list.number_of_downstream_objects;
            }
            disk_index = 0;
            for(pvd_obj_indx = 0; pvd_obj_indx < downstream_raid_object_list.number_of_downstream_objects; pvd_obj_indx++)
            {
                /* every PVD will have port, enclosure and slot which will correspond to drives in RG */
                /* Get the location details of the drives. */
                /* Using index variable in array subscript below will have the drive stored in consecutive 
                 * array location otherwise drive in position 0(may be other position as well) 
                 * will be overwritten every time.
                 */
                status = fbe_cli_get_bus_enclosure_slot_info(downstream_raid_object_list.downstream_object_list[pvd_obj_indx], 
                                                    FBE_CLASS_ID_PROVISION_DRIVE,
                                                    &rg_details->drive_list[disk_index].port_num,
                                                    &rg_details->drive_list[disk_index].encl_num,
                                                    &rg_details->drive_list[disk_index].slot_num,
                                                    FBE_PACKAGE_ID_SEP_0);
                /* This function does not return the status indicating the error or success back 
                 * In case of error it sets all parameters port, enclosure and slot number to INVALID
                 * check for invalid number and return error in case we're not able to get the drive details
                 */
                if (rg_details->drive_list[pvd_obj_indx].port_num == FBE_PORT_NUMBER_INVALID)
                {
                    if (downstream_object_list.downstream_object_list[pvd_obj_indx] < FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST &&
                    downstream_object_list.downstream_object_list[pvd_obj_indx] > FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_LAST)

		       	return FBE_STATUS_GENERIC_FAILURE;
                	
                    else
                    {
                        rg_details->drive_list[pvd_obj_indx].port_num = 0;
                        rg_details->drive_list[pvd_obj_indx].encl_num = 0;
                        rg_details->drive_list[pvd_obj_indx].slot_num = downstream_raid_object_list.downstream_object_list[pvd_obj_indx] -1;
                    }
                }
                disk_index++;
            }
        }

        else
        {
            /* RAID-10 raid groups will have downstream objects class id of 
             * FBE_CLASS_ID_MIRROR.
             */
            status = fbe_api_get_object_class_id(downstream_raid_object_list.downstream_object_list[0], &class_id, FBE_PACKAGE_ID_SEP_0);

            /* if status is generic failure then return that back */
            if (status == FBE_STATUS_GENERIC_FAILURE)
            {
                return status;
            }

            /* If the class id is not virtual drive it must be mirror.
             */
            if (class_id != FBE_CLASS_ID_VIRTUAL_DRIVE)
            {
                /* Must be mirror
                 */
                if (class_id != FBE_CLASS_ID_MIRROR)
                {
                    fbe_cli_error("rg_id: 0x%x unsupported downstream class_id: %d obj: 0x%x \n",
                                  rg_id, class_id, downstream_raid_object_list.downstream_object_list[0]);
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                /* For each mirror group get the vd ids.*/
                for(index = 0; index < downstream_raid_object_list.number_of_downstream_objects ;index ++)
                {
                    status = fbe_api_base_config_get_downstream_object_list(downstream_raid_object_list.downstream_object_list[index], &downstream_object_list);
                    if (status == FBE_STATUS_GENERIC_FAILURE)
                    {
                        return status;
                    }
    
                    for(loop_count = 0; loop_count< downstream_object_list.number_of_downstream_objects ;loop_count++)
                    {
                        rg_details->downstream_vd_object_list.downstream_object_list[vd_index] = downstream_object_list.downstream_object_list[loop_count];
                        vd_index++;
                    }
                }
                rg_details->downstream_vd_object_list.number_of_downstream_objects = vd_index;
            }
            else
            {
                for(index = 0; index < downstream_raid_object_list.number_of_downstream_objects ;index ++)
                {
                    rg_details->downstream_vd_object_list.downstream_object_list[index] = downstream_raid_object_list.downstream_object_list[index];
                    rg_details->downstream_vd_object_list.number_of_downstream_objects = downstream_raid_object_list.number_of_downstream_objects;
                }
            }
    
            //set disk index to zero
            disk_index = 0;
            /* For every Virtual Drive find the corresponding PVD */
            for (index = 0; index < rg_details->downstream_vd_object_list.number_of_downstream_objects; index++)
            {
                status = fbe_api_base_config_get_downstream_object_list(rg_details->downstream_vd_object_list.downstream_object_list[index],
                                                                        &downstream_object_list);
                if (status == FBE_STATUS_GENERIC_FAILURE)
                {
                    return status;
                }

                status = fbe_api_virtual_drive_get_configuration(rg_details->downstream_vd_object_list.downstream_object_list[index], 
                                                                 &configuration_mode);
                if (status == FBE_STATUS_GENERIC_FAILURE)
                {
                    return status;
                }
                rg_details->downstream_vd_mode_list[index] = configuration_mode.configuration_mode;
                
                for(pvd_obj_indx = 0; pvd_obj_indx < downstream_object_list.number_of_downstream_objects; pvd_obj_indx++)
                {
                    /* every PVD will have port, enclosure and slot which will correspond to drives in RG */
                    /* Get the location details of the drives. */
                    /* Using index variable in array subscript below will have the drive stored in consecutive 
                     * array location otherwise drive in position 0(may be other position as well) 
                     * will be overwritten every time.
                     */
                    status = fbe_cli_get_bus_enclosure_slot_info(downstream_object_list.downstream_object_list[pvd_obj_indx], 
                                                        FBE_CLASS_ID_PROVISION_DRIVE,
                                                        &rg_details->drive_list[disk_index].port_num,
                                                        &rg_details->drive_list[disk_index].encl_num,
                                                        &rg_details->drive_list[disk_index].slot_num,
                                                        FBE_PACKAGE_ID_SEP_0);
                    /* This function does not return the status indicating the error or success back 
                     * In case of error it sets all parameters port, enclosure and slot number to INVALID
                     * check for invalid number and return error in case we're not able to get the drive details
                     */
                    if (rg_details->drive_list[pvd_obj_indx].port_num == FBE_PORT_NUMBER_INVALID)
                    {
                        if(rg_details->drive_list[pvd_obj_indx].slot_num == -1)
                        {
                            rg_details->drive_list[pvd_obj_indx].slot_num = FBE_SLOT_NUMBER_INVALID;
                        }
						
                    }
                    rg_details->downstream_pvd_object_list.downstream_object_list[disk_index] = downstream_object_list.downstream_object_list[pvd_obj_indx];
                    disk_index++;
                }
            }
            rg_details->downstream_pvd_object_list.number_of_downstream_objects = disk_index;

        }
    }
    if (rg_details->direction_to_fetch_object & DIRECTION_UPSTREAM)
    {
        /* initialize the upstream object list. */
        rg_details->upstream_object_list.number_of_upstream_objects = 0;
        for(index = 0; index < FBE_MAX_UPSTREAM_OBJECTS; index++)
        {
            rg_details->upstream_object_list.upstream_object_list[index] = FBE_OBJECT_ID_INVALID;
        }
        status = fbe_api_base_config_get_upstream_object_list(rg_id, &rg_details->upstream_object_list);
        if (status == FBE_STATUS_GENERIC_FAILURE)
        {
            return status;
        }
    }

    return status;
}

/******************************************
 * end fbe_cli_get_rg_details()
 ******************************************/

fbe_bool_t fbe_cli_wwn_is_equal(fbe_assigned_wwid_t *wwn1, fbe_assigned_wwid_t *wwn2)
{
    fbe_u32_t index;

    if (!wwn1 || !wwn2)
        return FBE_FALSE;

    for (index = 0; index < FBE_WWN_BYTES; index++)
    {
        if (wwn1->bytes[index] != wwn2->bytes[index])
            return FBE_FALSE;
    }

    return FBE_TRUE;
}
/*!*******************************************************************
 * fbe_cli_checks_for_simillar_wwn_by_class
 *********************************************************************
 * @brief Function checks for simillar wwn number.
 *
 *  @param    
 *
 * @return -  fbe_bool_t.  
 *
 * @author
 *  9/22/2010 - Created. Prafull Parab
 *********************************************************************/
fbe_bool_t fbe_cli_checks_for_simillar_wwn_by_class(fbe_assigned_wwid_t wwn,    
                                                    fbe_class_id_t class_to_check)
{
    fbe_status_t              status;
    fbe_u32_t                 i;
    fbe_u32_t                 total_luns_in_system;
    fbe_object_id_t           *object_ids;
    fbe_cli_lurg_lun_details_t lun_details;
    
    /* get the number of LUNs in the system */
    status = fbe_api_enumerate_class(class_to_check, FBE_PACKAGE_ID_SEP_0, &object_ids, &total_luns_in_system);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("Failed to enumerate classes\n");
        return FBE_FALSE;
    }

    /* We want to skip the first LUN for now as it is private/sysem LUN */
    /* TODO: change the processing once LUN number for system LUN is exposed */
    for (i = 1; i < total_luns_in_system; i++)
    {
        /* get the LUN details based on object ID of LUN */
        status = fbe_cli_get_lun_details(object_ids[i], &lun_details);

        if (status != FBE_STATUS_OK)
        {
            /* error in getting the details skip to next object */
            continue;
        }

        if (fbe_cli_wwn_is_equal(&wwn, &lun_details.lun_info.world_wide_name))
        {
            fbe_cli_error("LUN: %d has the same wwn, please change the wwn!\n", lun_details.lun_info.lun_number);
            fbe_cli_error("LUN: %d WWN: ", lun_details.lun_info.lun_number);
            fbe_cli_print_wwn(&lun_details.lun_info.world_wide_name);
            fbe_api_free_memory(object_ids);
            return FBE_TRUE;
        }
    }
    fbe_api_free_memory(object_ids);    
    return FBE_FALSE;
}

/*!*******************************************************************
 * @var fbe_cli_checks_for_simillar_wwn_number
 *********************************************************************
 * @brief Function checks for simillar wwn number.
 *
 *  @param    
 *
 * @return -  fbe_bool_t.  
 *
 * @author
 *  9/22/2010 - Created. Prafull Parab
 *********************************************************************/
fbe_bool_t fbe_cli_checks_for_simillar_wwn_number(fbe_assigned_wwid_t wwn)
{
    return fbe_cli_checks_for_simillar_wwn_by_class(wwn, FBE_CLASS_ID_LUN);
}

/******************************************
 * end fbe_cli_checks_for_simillar_wwn_number()
 ******************************************/

/*!*******************************************************************
 * @var fbe_cli_generate_random_wwn_number 
 *********************************************************************
 * @brief Function generate random wwn number.
 *
 *  @param    
 *
 * @return -  fbe_bool_t.  
 *
 * @author
 *  10/29/2013 - Created. gaoh1 
 *********************************************************************/
fbe_status_t fbe_cli_generate_random_wwn_number(fbe_assigned_wwid_t *wwn)
{
    fbe_u32_t random;
    fbe_u32_t i, j, k;

    if (wwn == NULL) {
        fbe_cli_error("input wwn is NULL pointer \n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
        
    k = 0;
    for (i = 0; i < FBE_WWN_BYTES/4; i++)
    {
        random = fbe_random();
        for (j = 0; j < 4; j++)
            wwn->bytes[k++] = (random >> 8*i) & 0xf;
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cli_generate_random_wwn_number()
 ******************************************/

/*!*******************************************************************
* @var fbe_cli_is_system_lun
*********************************************************************
* @brief Function to get all the information about RG
*
*  @param object_id_to_check - Check the object against system object
*  @param system_object_list - list of system_objects
*  @param total_system_objects - total number of system objects.
*
* @return -  TRUE  - if object is system object.
*            FALSE - otherwise.
*
* @author
*  10/25/2010 - Created. Sanjay Bhave
*********************************************************************/
fbe_bool_t fbe_cli_is_system_lun(fbe_object_id_t object_id_to_check, 
                                 fbe_object_id_t *system_object_list,
                                 fbe_u32_t total_system_objects)
{
    fbe_bool_t b_found = FBE_FALSE;
    fbe_u32_t obj_index = 0;

    /* simply iterate through the system object list to see if we find 
    * 'object_id_to_check' object in that list
    */
    for (obj_index = 0; obj_index < total_system_objects; obj_index++)
    {
        if(object_id_to_check == system_object_list[obj_index])
        {
            b_found = FBE_TRUE;
            break;
        }
    }

    return b_found;
}

/*!*******************************************************************
 * @var fbe_cli_display_lun_stat
 *********************************************************************
 * @brief Function to display the prepare the output line to display.
 *
 *  @param lun_details - details of the LUN to display.
 *         rg_details  - details of RG to display.
 *
 * @return -  FBE status.  
 *
 * @author
 *  4/30/2010 - Created. Sanjay Bhave
 *********************************************************************/
fbe_status_t fbe_cli_display_lun_stat(fbe_cli_lurg_lun_details_t *lun_details, fbe_cli_lurg_rg_details_t *rg_details)
{
    fbe_lba_t         capacity;
    fbe_u8_t          line_buffer[fbe_cli_lustat_line_size];
    fbe_u8_t*         line_seeker = fbe_cli_lustat_reset_line(line_buffer);
    fbe_u8_t          sprintf_buffer[fbe_cli_lustat_line_size];
    fbe_status_t      status;
    fbe_block_size_t  exported_block_size;
    fbe_u32_t         actual_capacity_of_lun;
    fbe_u32_t         actual_cap_in_bytes;
    fbe_u32_t         index;
    fbe_u32_t         rg_width = 0;
    fbe_lifecycle_state_t lifecycle_state;

    /* Add LUN number to output */
    sprintf(sprintf_buffer, "%4d  ", lun_details->lun_number);
    line_seeker = fbe_cli_lustat_add_string(line_buffer, line_seeker, sprintf_buffer);

    /* Add RG number to output */
    sprintf(sprintf_buffer, "%3d    ", rg_details->rg_number);
    line_seeker = fbe_cli_lustat_add_string(line_buffer, line_seeker, sprintf_buffer);

    /* Add LUN lifecycle state to output */
    sprintf(sprintf_buffer, "%5s    ", lun_details->p_lifecycle_state_str);
    line_seeker = fbe_cli_lustat_add_string(line_buffer, line_seeker, sprintf_buffer);

    /* Add RAID type to output */
    sprintf(sprintf_buffer, "%8s    ", lun_details->p_rg_str);
    line_seeker = fbe_cli_lustat_add_string(line_buffer, line_seeker, sprintf_buffer);  
    
    capacity =  lun_details->lun_info.capacity;
    exported_block_size = lun_details->lun_info.raid_info.exported_block_size;

    /* Convert the capacity to Mega Bytes */
    actual_capacity_of_lun = (fbe_u32_t)((capacity * exported_block_size)/(1024*1024));
    actual_cap_in_bytes = (fbe_u32_t)((capacity * exported_block_size) % (1024*1024));
    status = fbe_cli_get_capacity_displaysize(actual_capacity_of_lun);
    if (lustat_cmd_line.display_size == DISPLAY_IN_MB )
    {
        /* Display MegaBytes.
         */
        if (actual_capacity_of_lun > 0)
        {
            sprintf(sprintf_buffer, "%6d MB   ", actual_capacity_of_lun);
        }
        else
        {
            sprintf(sprintf_buffer, "%6d B   ", actual_cap_in_bytes);
        }
        line_seeker = fbe_cli_lustat_add_string(line_buffer, line_seeker, sprintf_buffer); 

    }
    else if (lustat_cmd_line.display_size == DISPLAY_IN_GB )
    {

        /* It's at least 1 GB.  Display in GB, to the nearest 0.1 GB.
         * First, add 50 MB, which effectively rounds the capacity up
         * to the nearest 100 MB (0.1 GB).
         */
         capacity += 50;

        /* Capacity>>10 gives us the whole number of GB.
         * Then we want "tenths" of a GB:  capacity & 0x3FF extracts
         * the fractional part of a GB, i.e. some number of MB between
         * 0-1023.  Dividing that by 103 gives us APPROX tenths of a GB.
         */
         sprintf(sprintf_buffer, "%6d.%d GB   ", (fbe_u32_t)(actual_capacity_of_lun >> 10), (fbe_u32_t)((actual_capacity_of_lun & 0x3ff)/103));
         line_seeker = fbe_cli_lustat_add_string(line_buffer, line_seeker, sprintf_buffer); 
    }
    else 
    {
        /* It's at least 1 TB.  Display in TB, to the nearest 0.1 TB.
         * First, add 50000 MB, which effectively rounds the capacity up
         * to the nearest 100000 MB (0.1 TB).
         */
         capacity += 50000; 

        /* Capacity>>20 gives us the whole number of TB.
         * Then we want "tenths" of a TB:  capacity & 0xFFFFF extracts
         * the fractional part of a TB, i.e. some number of MB between
         * 1024 - 1048576.  Dividing that by 100005 gives us APPROX tenths of a GB.
         */
         sprintf(sprintf_buffer, "%6d.%d TB   ", (fbe_u32_t)(actual_capacity_of_lun >> 20), (fbe_u32_t)((actual_capacity_of_lun & 0xfffff)/103000));
         line_seeker = fbe_cli_lustat_add_string(line_buffer, line_seeker, sprintf_buffer); 
    }

    /*we need to double the  RG width for R1_0 type*/
    if(lun_details->lun_info.raid_info.raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        rg_width = lun_details->lun_info.raid_info.width * 2;
    }
    else 
    {
        rg_width = lun_details->lun_info.raid_info.width;
    }

    for (index = 0; index <rg_width ; index++)
    {
        sprintf(sprintf_buffer, " %d_%d_%d", rg_details->drive_list[index].port_num, 
                 rg_details->drive_list[index].encl_num, rg_details->drive_list[index].slot_num);
        line_seeker = fbe_cli_lustat_add_string(line_buffer, line_seeker, sprintf_buffer);

        /*print if drive is dead*/
        status = fbe_api_get_object_lifecycle_state(rg_details->downstream_pvd_object_list.downstream_object_list[index], &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s Get lifecycle failed: index: %d obj id: 0x%x status: 0x%x \n",
                           __FUNCTION__, index, rg_details->downstream_pvd_object_list.downstream_object_list[index], status);
            return status;
        }
        if(lifecycle_state == FBE_LIFECYCLE_STATE_FAIL)
        {
            sprintf(sprintf_buffer, "%s","(DEAD)");
            line_seeker = fbe_cli_lustat_add_string(line_buffer, line_seeker, sprintf_buffer);
        }
        else
        {
            /*Print rebuild percentage*/
            status = fbe_cli_get_drive_rebuild_status(lun_details,rg_details,index);
            if(status != FBE_STATUS_OK)
            {
                /*above fuction display an error. no need to display here*/
                return status;
            }
            /*End of printing rebuild percentage*/
        }
    }

    /*Print LUN Zeroing percentage*/
    if (lun_details->fbe_api_lun_get_zero_status.zero_checkpoint != FBE_LBA_INVALID)
    {
        sprintf(sprintf_buffer, "  (BZR:%d%%)",
            lun_details->fbe_api_lun_get_zero_status.zero_percentage);
        line_seeker = fbe_cli_lustat_add_string(line_buffer, line_seeker, sprintf_buffer);
    }

    sprintf(sprintf_buffer, "\n");
    fbe_cli_lustat_print_line(line_buffer);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cli_display_lun_stat()
 ******************************************/
/*!*******************************************************************
 * @var fbe_cli_lustat_add_string
 *********************************************************************
 * @brief Function to add lustat output to display line.
 *
 *  @param line   - This will be displayed ultimately
 *         seeker - Position in the string where to write.
 *         text   - Text to write in the line
 *
 * @return -  seeker to start for next operation.  
 *
 * @author
 *  4/30/2010 - Created. Sanjay Bhave
 *********************************************************************/
fbe_u8_t* fbe_cli_lustat_add_string(fbe_u8_t* line, fbe_u8_t* seeker, const fbe_u8_t* text)
{
    size_t text_length = strlen(text);

    // write the line only if
    // the text size is shorter than the line size AND
    // the seeker is pointing to a location within the line AND
    // adding the text to the line will not cause buffer overflow
    //
    if ((text_length < fbe_cli_lustat_line_size) &&  
        (seeker < (line + fbe_cli_lustat_line_size)) && 
        (seeker >= line) &&
        ((seeker + text_length) < (line + fbe_cli_lustat_line_size - 1)))
    {        
        // copy the text to the line
        //
        fbe_copy_memory(seeker, text, (fbe_u32_t)text_length);

    
        // advance the seeker by the length of the text
        //
        seeker += text_length;
    }

    return seeker;    
}
/******************************************
 * end fbe_cli_lustat_add_string()
 ******************************************/

/*!*******************************************************************
 * @var fbe_cli_lustat_reset_line
 *********************************************************************
 * @brief Function to reset the display line.
 *
 *  @param line   - This will be displayed ultimately
 *
 * @return -  seeker to start for next operation.  
 *
 * @author
 *  4/30/2010 - Created. Sanjay Bhave
 *********************************************************************/
fbe_u8_t* fbe_cli_lustat_reset_line(fbe_u8_t* line)
{
    fbe_u8_t * seek = 0;

    if (line)
    {
        fbe_zero_memory(line, fbe_cli_lustat_line_size);

        seek = line;
    }

    return seek;
}

/******************************************
 * end fbe_cli_lustat_reset_line()
 ******************************************/

/*!*******************************************************************
 * @var fbe_cli_lustat_print_line
 *********************************************************************
 * @brief Function to display the line formatted with lustat output.
 *
 *  @param line   - The formatted output line to display.
 *
 * @return -  None.  
 *
 * @author
 *  4/30/2010 - Created. Sanjay Bhave
 *********************************************************************/
void fbe_cli_lustat_print_line(fbe_u8_t* line)
{
    if (line)
    {
        fbe_cli_printf("%s\n", line);
    }
}

/******************************************
 * end fbe_cli_lustat_print_line()
 ******************************************/
/*!*******************************************************************
 * @var fbe_cli_get_rebuild_progress
 *********************************************************************
 * @brief 
 *  This function calculates the rebuild percentage based on rebuild
 *  checkpoint of a given drive.
 *
 *  @param pointer to drive location
 *  @param rebuild checkpoint of a drive
 *
 * @return -  fbe_u16_t - rebuild percentage
 *                        [0-100 for Valid and 101 for invalid]
 *
 * @author
 *  12/21/2010 - Created. Vinay Patki
 *********************************************************************/
fbe_u16_t fbe_cli_get_drive_rebuild_progress(fbe_cli_lurg_pes_t * drive_location, fbe_lba_t reb_checkpoint)
{
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_object_id_t drive_obj_id;
    fbe_api_provision_drive_info_t provision_drive_info;

    /* Get the object id of the PVD */
    status = fbe_api_provision_drive_get_obj_id_by_location(drive_location->port_num,
    drive_location->encl_num, drive_location->slot_num, &drive_obj_id);

    if (status != FBE_STATUS_OK)
    {
        /* if unable to get the object id of the drive, then return error from here */
        fbe_cli_printf ("%s:Failed to get object id of %d_%d_%d.    %d\n", __FUNCTION__, drive_location->port_num, 
        drive_location->encl_num, drive_location->slot_num, status);
        return FBE_INVALID_PERCENTAGE;
    }

    /*Get provision drive info of a drive*/
    status = fbe_api_provision_drive_get_info(drive_obj_id,&provision_drive_info);
    if (status != FBE_STATUS_OK)
    {
        /* if unable to get PVD info*/
        fbe_cli_printf ("%s:Failed to get information of Provisional drive %d (%d_%d_%d).    %d\n", __FUNCTION__, drive_obj_id, drive_location->port_num, 
         drive_location->encl_num, drive_location->slot_num, status);
        return FBE_INVALID_PERCENTAGE;
    }

    return((fbe_u16_t) ((reb_checkpoint * 100)/ provision_drive_info.capacity));
}
/******************************************
 * end fbe_cli_get_rebuild_progress()
 ******************************************/
/*!*******************************************************************
 * @var fbe_cli_get_capacity_displaysize
 *********************************************************************
 * @brief Function to set the appropriate display size of the LUN 
 *        for lustat output. This function sets the global variable
 *        which will determine the output string to be used.
 *
 *  @param status   - success or failure status.
 *
 * @return -  None.  
 *
 * @author
 *  05/03/2010 - Created. Sanjay Bhave
 *********************************************************************/
fbe_status_t fbe_cli_get_capacity_displaysize(fbe_u32_t capacity)
{
    if (lustat_cmd_line.display_size == 0)
        /* User did not care the way the LUN size displays 
         * We will determine by the size of LUN */
    {
        if (capacity < 1024)
        {
            lustat_cmd_line.display_size = DISPLAY_IN_MB;
        }
        else if ((capacity >= 1024) && (capacity < 1048576))
        {
            lustat_cmd_line.display_size = DISPLAY_IN_GB;
        }
        else
        {
            lustat_cmd_line.display_size = DISPLAY_IN_TB;
        }
    }
    else if (lustat_cmd_line.display_size == DISPLAY_IN_MB)
    {
        /* User chooses to display in MB. But if the lun size is bigger than
         * gigabyte, we default to gigabyte */
        if ((capacity < 1024) || ((capacity >= 1024) && (capacity < 1048576)))
        {
            lustat_cmd_line.display_size = DISPLAY_IN_MB;
        }
        else
        {
            /* We will default to GB because the # is really too big */
            lustat_cmd_line.display_size = DISPLAY_IN_GB;
        }
    }
    else if (lustat_cmd_line.display_size == DISPLAY_IN_GB)
    {
        /* User chooses to display in GB. If the lun size is too small 
         * we will display in MB */
        if (capacity < 1024)
        {
            lustat_cmd_line.display_size = DISPLAY_IN_MB;
        }
        else 
        {
            lustat_cmd_line.display_size = DISPLAY_IN_GB;
        }
    }
    else 
    {
        if (capacity < 1024)
        {
            lustat_cmd_line.display_size = DISPLAY_IN_MB;
        }
        else 
        {
            lustat_cmd_line.display_size = DISPLAY_IN_TB;
        }
    }
    return FBE_STATUS_OK;
}

/******************************************
 * end fbe_cli_get_capacity_displaysize()
 ******************************************/

/******************************************
 * end fbe_cli_lurg_get_drive_num()
 ******************************************/

/*!**************************************************************
 * fbe_cli_find_smallest_free_raid_group_number()
 ****************************************************************
 * @brief
 * This function is used to determine smallest possible RG number
 * in the system.
 *
 * @param raid_group_id.
 *
 * @return TRUE - The raid group id is valid
 * @return FALSE - The raid group id is invalid
 *
 * @author
 *  05/10/2010 - Vinay Patki Created. 
 ****************************************************************/
fbe_status_t fbe_cli_find_smallest_free_raid_group_number(fbe_raid_group_number_t * raid_group_id)
{
        fbe_u32_t i = 0;
        fbe_object_id_t object_id;

    if (raid_group_id == NULL)
    {
        return (FBE_STATUS_INSUFFICIENT_RESOURCES);
    }
    for(i = 0; i <= MAX_RAID_GROUP_ID;i++)
    {
        if(fbe_api_database_lookup_raid_group_by_number(i, &object_id) == FBE_STATUS_OK)
        {
            continue;
        }
        else
        {
            *raid_group_id = i;
            return (FBE_STATUS_OK);
        }
    }

    return (FBE_STATUS_INSUFFICIENT_RESOURCES);
}
/*****************************************************
 * end fbe_cli_find_smallest_free_raid_group_number()
 *****************************************************/

/*!**************************************************************
 * fbe_cli_convert_str_to_lba()
 ****************************************************************
 * @brief
 * This function is used to convert given string to LBA value.
 *
 *
 * @param raid_group_id.
 *
 * @return TRUE - The raid group id is valid
 * @return FALSE - The raid group id is invalid
 *
 * @author
 *  05/13/2010 - Sanjay Bhave Created. 
 ****************************************************************/
fbe_lba_t fbe_cli_convert_str_to_lba(fbe_u8_t *str)
{
    fbe_lba_t lba_val = 0;
    fbe_bool_t hex_num = FBE_FALSE;
    fbe_u8_t *str_copy = str;

    /* TODO: strlen should be replaced by corresponding FBE 
     * compatible function 
     */
    if (str == NULL) {
       fbe_cli_error ("%s:String is empty\n", __FUNCTION__);
	   return FBE_LBA_INVALID;
    }

    if (strlen(str) == 0)
        return FBE_LBA_INVALID;

    /* Check if str is a hex value*/
    str = str_copy;
    if (*str == '0') {
       *str++;
       if (*str == 'x') {
          hex_num = FBE_TRUE;
          str++;
       }
       else {
          str = str_copy;
       }
    }
    
    while(*str != '\0')
    {
       if (hex_num) {
          fbe_u64_t temp_str = *str - '0';
          fbe_bool_t faulty = FBE_FALSE;

          if (!isdigit(*str)) {
             switch (*str) {
             case 'a':
                temp_str = 10;
                break;
             case 'b':
                temp_str = 11;
                break;
             case 'c':
                temp_str = 12;
                break;
             case 'd':
                temp_str = 13;
                break;
             case 'e':
                temp_str = 14;
                break;
             case 'f':
                temp_str = 15;
                break;
             default:
                faulty = FBE_TRUE;
                break;
             }
             if (faulty) {
                return FBE_LBA_INVALID;
             }
          }
          lba_val *= 16;
          lba_val += temp_str;
       }

       else {
           /* TODO: strlen should be replaced by corresponding FBE 
            * compatible function 
            */
           if (!isdigit(*str))
           {
               /* Return Invalid capacity in case any we get non digit value */
               return FBE_LBA_INVALID;
           }
           lba_val *= 10;
           lba_val += *str - '0';
       }
        str++;
    }

    return lba_val;
}
/***********************************
 * end fbe_cli_convert_str_to_lba()
 ***********************************/

/*!**************************************************************
 * fbe_cli_find_smallest_free_lun_number()
 ****************************************************************
 * @brief
 * This function is used to determine smallest possible LUN number
 * in the system.which is available for creation.
 *
 * @param pointer to lun number available in the system.
 *
 * @return status indicating whether smallest lun number found 
 *         or not
 *
 * @author
 *  05/14/2010 - Sanjay Bhave Created. 
 ****************************************************************/
fbe_status_t fbe_cli_find_smallest_free_lun_number(fbe_lun_number_t* lun_number_p)
{
        fbe_u32_t i = 0;
        fbe_object_id_t object_id;

    if (lun_number_p == NULL)
    {
        return (FBE_STATUS_INSUFFICIENT_RESOURCES);
    }
    for(i = 0; i <= MAX_LUN_ID;i++)
    {
        if(fbe_api_database_lookup_lun_by_number(i, &object_id) == FBE_STATUS_OK)
        {
            continue;
        }
        else
        {
            *lun_number_p = i;
            return (FBE_STATUS_OK);
        }
    }

    return (FBE_STATUS_INSUFFICIENT_RESOURCES);
}
/*****************************************************
 * end fbe_cli_find_smallest_free_lun_number()
 *****************************************************/
/*!**************************************************************
 * fbe_cli_validate_lun_request()
 ****************************************************************
 * @brief
 * This function is used to validate the command line parameters 
 * passed to create a LUN
 *
 * @param pointer to lun creation structure.
 *
 * @return status of validation
 *
 * @author
 *  05/14/2010 - Sanjay Bhave Created. 
 ****************************************************************/
fbe_status_t fbe_cli_validate_lun_request(fbe_api_lun_create_t *lun_create_req)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Make sure that LUN number is in range */
    if (lun_create_req->lun_number > MAX_LUN_ID && lun_create_req->lun_number < 0)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/**************************************
 * end fbe_cli_validate_lun_request()
 /*************************************/

/*!**************************************************************
* fbe_cli_configure_spare()
****************************************************************
* @brief
* This function configures the drive as a hotspare.
*
* @param
* Drive number in Bus, Enclosure and Slot format
*
* @return status of HS creation operation.
*
*
* @author
*  10/21/2010 - Vinay Patki Created.
****************************************************************/
fbe_status_t fbe_cli_configure_spare(fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot)
{
    fbe_object_id_t                 object_id;          //  object id of the PVD 
    fbe_status_t                    status;             //  fbe status


    //  Get the object id of the PVD 
    status = fbe_api_provision_drive_get_obj_id_by_location(bus, enclosure, slot, &object_id);

    if (status != FBE_STATUS_OK)
    {
        /* if unable to get the object id of the drive, then return error from here */
        fbe_cli_printf ("%s:Failed to get object id of %d_%d_%d.    %d\n", __FUNCTION__, bus, enclosure, slot, status);
        return status;
    }

    //  Create a PVD configured as a hot spare 
    status = fbe_cli_update_pvd_configuration(object_id, FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    //  Log a trace 
    fbe_cli_printf ("%s: Successfully configured %d_%d_%d as Hot spare\n", __FUNCTION__, bus, enclosure, slot);

    //  Return 
    return FBE_STATUS_OK;

}
/*****************************************
* end fbe_cli_configure_spare()
*******************************************/

/*!**************************************************************
* fbe_cli_unconfigure_spare()
****************************************************************
* @brief
* This function is used to unconfigre the hot spare.
*
* @param
* Drive number in Bus, Enclosure and Slot format
*
* @return status of HS creation operation.
*
*
* @author
*  10/21/2010 - Vinay Patki Created.
****************************************************************/
fbe_status_t fbe_cli_unconfigure_spare(fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot)
{
    fbe_object_id_t         object_id;          //  object id of the PVD 
    fbe_status_t            status;             //  fbe status


    //  Get the object id of the PVD 
    status = fbe_api_provision_drive_get_obj_id_by_location(bus, enclosure, slot, &object_id);
    if (status != FBE_STATUS_OK)
    {
        /* if unable to get the object id of the drive, then return error from here */
        fbe_cli_printf ("%s:Failed to get object id of %d_%d_%d, status:%d\n", __FUNCTION__, bus, enclosure, slot, status);
        return status;
    }

    //  update pvd configuration as unknown
    status = fbe_cli_update_pvd_configuration(object_id, FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    //  Log a trace 
    fbe_cli_printf ("%s: Successfully unconfigured %d_%d_%d as Hot spare\n", __FUNCTION__, bus, enclosure, slot);

    //  Return 
    return FBE_STATUS_OK;

}
/*****************************************
* end fbe_cli_unconfigure_spare()
*******************************************/

/*!**************************************************************
* fbe_cli_update_pvd_configuration()
****************************************************************
* @brief
*  This function sends a request to job service to configure
*  the provision drive as hot spare.
*
* @param pvd_object_id  - pvd object-id.
* @param config_type    - configuration type of pvd object
*
* @return none
*
*
* @author
*  10/21/2010 - Vinay Patki Created.
****************************************************************/
fbe_status_t fbe_cli_update_pvd_configuration(fbe_object_id_t  pvd_object_id,
                                              fbe_provision_drive_config_type_t config_type)
{
    fbe_api_job_service_update_provision_drive_config_type_t    update_pvd_config_type;
    fbe_status_t                                                status;
    fbe_status_t                                                job_status;
    fbe_job_service_error_type_t                                job_error_type;

    /* initialize error type as no error. */
    job_error_type = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* config type update should be either unknown or spare. */
    if((config_type != FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED) &&
       (config_type != FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE))
    {
        fbe_cli_printf ("%s: Config type %d is invalid\n", __FUNCTION__, config_type);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* configure the pvd as hot spare. */
    update_pvd_config_type.pvd_object_id = pvd_object_id;
    update_pvd_config_type.config_type = config_type;

    /* update the provision drive config type as hot spare. */
    status = fbe_api_job_service_update_provision_drive_config_type(&update_pvd_config_type);
    if (status != FBE_STATUS_OK)
    {
        /* if unable to get the object id of the drive, then return error from here */
        fbe_cli_printf ("%s:Failed to update PVD %d config type, status:%d\n", __FUNCTION__, pvd_object_id, status);
        status;

    }

    /* wait for the notification from the job service. */
    status = fbe_api_common_wait_for_job(update_pvd_config_type.job_number, 120000, &job_error_type, &job_status, NULL);
    if((job_status != FBE_STATUS_OK) || 
       (job_error_type != FBE_JOB_SERVICE_ERROR_NO_ERROR))
    {
        fbe_cli_printf ("%s: Failed to update PVD %d config type:%d, job_stat:%d, job_err_type:%d\n",
                        __FUNCTION__, pvd_object_id, config_type, job_status, job_error_type);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_cli_printf ("%s: Successfully updated PVD %d config type:%d\n", __FUNCTION__, pvd_object_id, config_type);
    return FBE_STATUS_OK;

}
/****************************
* end fbe_cli_configure_pvd_as_spare()
****************************/

/*!**************************************************************
* fbe_cli_convert_lba_to_other_size_unit()
****************************************************************
* @brief
*  This function converts lba to supported
*  size unit MB GB or BLOCKS.
*
* @param fbe_lba_t *  - Size
* @param fbe_u8_t *  - Str
*
* @return fbe_u32_t  Size converted to given unit
*
*
* @author
*  12/20/2010 - Hardik Joshi Created.
****************************************************************/

fbe_lba_t fbe_cli_convert_size_unit_to_lba(fbe_lba_t unit_size, fbe_u8_t *str)
{
    fbe_lba_t calculated_size = 0;
    if (unit_size<= 0)
    {
          fbe_cli_error("LUN capacity argument is expected\n");
          return calculated_size;
    }
   if ((strcmp(str, "MB") == 0))
    {
        calculated_size =( unit_size * FBE_MEGABYTE) / 520;
    }
    else if ((strcmp(str, "GB") == 0))
    {
        calculated_size =( unit_size * FBE_GIGABYTE) / 520;
    }
    else if((strcmp(str, "BL") == 0))
    {
        /*Nothing to do*/
    }
    return calculated_size;
}

/****************************
* end fbe_cli_convert_lba_to_other_size_unit()
****************************/

/*!*******************************************************************
 * @var fbe_cli_lib_zero_disk
 *********************************************************************
 * @brief       Function to zero the disk that is specified.
 *
 *  @param      argc - argument count
 *  @param      argv - argument string
 *
 * @return      none.
 *
 * @author
 *  12/29/2010 - Created. Musaddique Ahmed
 *********************************************************************/
void fbe_cli_zero_disk(fbe_s32_t argc, fbe_s8_t** argv)
{
    fbe_u32_t bus = FBE_PORT_NUMBER_INVALID;
    fbe_u32_t enclosure = FBE_ENCLOSURE_NUMBER_INVALID;
    fbe_u32_t slot = FBE_SLOT_NUMBER_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t status;
    fbe_lba_t curr_checkpoint;
    fbe_bool_t zd_status = FBE_FALSE;
    fbe_bool_t consumed_only = FBE_FALSE;

    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /* If they are asking for help, just display help and exit.
            */
            fbe_cli_printf("%s", ZERO_DISK_USAGE);
            return;
        }
        else if ((strcmp(*argv, "-b") == 0))
        {
            /* Get the Bus number from command line */
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("bus is expected\n");
                return;
            }

            bus = atoi(*argv);

            argc--;
            argv++;
        }
        /* Get the Enclosure number from command line */
        else if ((strcmp(*argv, "-e") == 0))
        {
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Enclosure is expected\n");
                return;
            }

            enclosure = atoi(*argv);

            argc--;
            argv++;
        }
        /* Get the Slot number from command line */
        else if ((strcmp(*argv, "-s") == 0))
        {
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Slot is expected\n");
                return;
            }

            slot = atoi(*argv);

            argc--;
            argv++;
        }
        else if ((strcmp(*argv, "-status") == 0))
        {
            argc--;
            argv++;
            zd_status = FBE_TRUE;
        }
        else if ((strcmp(*argv, "-consumed") == 0))
        {
            argc--;
            argv++;
            consumed_only = FBE_TRUE;
        }
        else
        {
            /* The command line parameters should be properly entered */
            fbe_cli_error("Invalid argument.\n");
            fbe_cli_printf("%s", ZERO_DISK_USAGE);
            return;
        }
    }

    if( bus == FBE_PORT_NUMBER_INVALID ||
        enclosure == FBE_ENCLOSURE_NUMBER_INVALID ||
        slot == FBE_SLOT_NUMBER_INVALID)
    {
        fbe_cli_error("Invalid argument.\n");
        fbe_cli_printf("%s", ZERO_DISK_USAGE);
        return;
    }

    status = fbe_api_provision_drive_get_obj_id_by_location(bus,
                                                            enclosure,
                                                            slot,
                                                            &pvd_object_id);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Error getting the PVD id for disk %d_%d_%d\n",bus,enclosure,slot);
        return;
    }

    if(zd_status)
    {
        /* get the current disk zeroing checkpoint */
        status = fbe_api_provision_drive_get_zero_checkpoint(pvd_object_id, &curr_checkpoint);
        
        if (status != FBE_STATUS_OK){
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get Lifecycle state failed with status %d\n", __FUNCTION__, status);          
            return ;
        }
        fbe_cli_printf("Disk %d_%d_%d - Zero Chkpt: 0x%llX\n",
		       bus,enclosure,slot, (unsigned long long)curr_checkpoint);
    }
    else
    {
        /* Initiate the Disk zeroing. */
        if (consumed_only)
        {
            status = fbe_api_provision_drive_initiate_consumed_area_zeroing(pvd_object_id);
        }
        else
        {
            status = fbe_api_provision_drive_initiate_disk_zeroing(pvd_object_id);
        }
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Error starting zeroing of disk %d_%d_%d\n",bus,enclosure,slot);
            return;
        }
        fbe_cli_printf("Starting zeroing of disk %d_%d_%d\n",bus,enclosure,slot);
    }

    return;
}
/****************************
* end fbe_cli_lib_zero_disk()
****************************/


/*!*******************************************************************
 * @var fbe_cli_zero_lun
 *********************************************************************
 * @brief       Function to zero the lun that is specified.
 *
 *  @param      argc - argument count
 *  @param      argv - argument string
 *
 * @return      none.
 *
 * @author
 *  2/7/2012 - Created. He,Wei
 *********************************************************************/
void fbe_cli_zero_lun(fbe_s32_t argc, fbe_s8_t** argv)
{
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_u32_t lun_number = FBE_LUN_ID_INVALID;
    fbe_object_id_t lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t reboot_zero = FBE_FALSE;
    fbe_cmi_service_get_info_t cmi_info;

    if (argc == 0)
    {
        /* If they are asking for help, just display help and exit.*/
        fbe_cli_printf("%s", ZERO_LUN_USAGE);
        return;
    }

    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /* If they are asking for help, just display help and exit.*/
            fbe_cli_printf("%s", ZERO_LUN_USAGE);
            return;
        }
        else if ((strcmp(*argv, "-lun") == 0))
        {
            /* Get the LUN from command line */
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("LUN number is expected\n");
                return;
            }

            lun_number = atoi(*argv);

            argc--;
            argv++;
        }
        /* Get the Enclosure number from command line */
        else if ((strcmp(*argv, "-lun_objid") == 0))
        {
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("LUN object id is expected\n");
                return;
            }

            lun_object_id = atoi(*argv);

            argc--;
            argv++;
        }
        else if ((strcmp(*argv, "-r") == 0))
        {
            reboot_zero = FBE_TRUE;
            argc--;
            argv++;
        }
        else
        {
            /* The command line parameters should be properly entered */
            fbe_cli_error("Invalid argument.\n");
            fbe_cli_printf("%s", ZERO_LUN_USAGE);
            return;
        }
    }

    if(lun_number != FBE_LUN_ID_INVALID)
    {
        status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Invalid LUN number: %d, status:0x%x\n",lun_number, status);
            return;
        }
    }
    
    if(reboot_zero == FBE_TRUE)
    {
        status = fbe_api_cmi_service_get_info(&cmi_info);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Get SP CMI State Failed\n");
            return;
        }

        if(cmi_info.sp_state != FBE_CMI_STATE_ACTIVE)
        {
            fbe_cli_printf("Command is only available on active side\n");
            return;
        }

        status = fbe_api_lun_initiate_reboot_zero(lun_object_id);
    }
	else {
        status = fbe_api_lun_initiate_zero(lun_object_id);
	}

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Command faild, status:0x%x\n", status);
        return;
    }

    if(reboot_zero == FBE_TRUE)
        fbe_cli_printf("Please reboot both SP at the same time...\n");
    
    return;
}

/****************************
* end fbe_cli_zero_lun()
****************************/

/*!*******************************************************************
 * @var fbe_cli_private_luns_access
 *********************************************************************
 * @brief       Function to provide access to private LUNs such as Lockbox and Audit Log.
 *
 *  @param      argc - argument count
 *  @param      argv - argument string
 *
 * @return      none.
 *
 * @author
 *  12/04/2013 - Created.
 *********************************************************************/
void fbe_cli_private_luns_access(fbe_s32_t argc, fbe_s8_t** argv)
{
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_object_id_t lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_block_count_t  io_size = 0x800;  /* default to 1M IO */
    fbe_lba_t          request_size = 0xa000;  /* default to 20M request */
    fbe_lba_t          offset = 0;   /* default to beginning of the LUN */
    fbe_u32_t          q_depth = 4;
    fbe_bool_t         paint = FBE_FALSE;
    fbe_u8_t          *buffer = NULL;
    fbe_u8_t          *buffer_read = NULL;
    fbe_time_t         start_time_for_write;
    fbe_time_t         start_time_for_read;
    fbe_time_t         elapsed_time_for_read;

    if (argc == 0)
    {
        /* If they are asking for help, just display help and exit.*/
        fbe_cli_printf("%s", LURG_PRIVATE_LUNS_ACCESS_USAGE);
        return;
    }

    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /* If they are asking for help, just display help and exit.*/
            fbe_cli_printf("%s", LURG_PRIVATE_LUNS_ACCESS_USAGE);
            return;
        }

        if ((strcmp(*argv, "-lun_obj_id") == 0))
        {
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("either lockbox or auditlog lun object id (in decimal) is expected\n");
                return;
            }

            lun_object_id = atoi(*argv);

            if ((lun_object_id == FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CBE_LOCKBOX) ||
                (lun_object_id == FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CBE_AUDIT_LOG))
            {
                argc--;
                argv++;
                
                continue;                
            }
            else
            {
                fbe_cli_error("Lockbox (50) or Auditlog (37) LUN object ID is expected.\n");
                return;
            }
        }
        else if ((strcmp(*argv, "-size") == 0))
        {
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("request size is expected\n");
                return;
            }

            request_size = atoi(*argv);

            argc--;
            argv++;
        }
        else if ((strcmp(*argv, "-iosize") == 0))
        {
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("io size is expected\n");
                return;
            }

            io_size = atoi(*argv);

            argc--;
            argv++;
        }
        else if ((strcmp(*argv, "-q") == 0))
        {
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("q depth is expected\n");
                return;
            }

            q_depth = atoi(*argv);

            argc--;
            argv++;
        }
        else if ((strcmp(*argv, "-offset") == 0))
        {
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("offset is expected\n");
                return;
            }

            offset = atoi(*argv);

            argc--;
            argv++;
        }
        else if ((strcmp(*argv, "-paint") == 0))
        {
            paint = FBE_TRUE;
            argc--;
            argv++;
        }
        else
        {
            /* The command line parameters should be properly entered */
            fbe_cli_error("Invalid argument.\n");
            fbe_cli_printf("%s", LURG_PRIVATE_LUNS_ACCESS_USAGE);
            return;
        }
    }
  
    if (lun_object_id == FBE_OBJECT_ID_INVALID)
    {
        fbe_cli_error("LUN object ID is not valid.  Make sure -lun_obj_id is specified.\n");
        return;            
    }

    if(paint == FBE_TRUE)
    {
        buffer = (fbe_u8_t *)fbe_api_allocate_memory((fbe_u32_t)request_size * FBE_BE_BYTES_PER_BLOCK);

        if (buffer == NULL)
        {
            fbe_cli_printf("failed to get buffer memory\n");
            return;
        }

        buffer_read = (fbe_u8_t *)fbe_api_allocate_memory((fbe_u32_t)request_size * FBE_BE_BYTES_PER_BLOCK);
        if (buffer_read == NULL)
        {
            fbe_api_free_memory(buffer);    
            fbe_cli_printf("failed to get read buffer memory\n");
            return;
        }

        fbe_cli_paint_buffer(buffer, request_size);

        start_time_for_write = fbe_get_time();

        status = fbe_api_block_transport_write_data(lun_object_id, 
                                                   FBE_PACKAGE_ID_SEP_0,
                                                   (fbe_u8_t *) buffer, 
                                                   request_size, 
                                                   offset, 
                                                   io_size,
                                                   q_depth);
        if (status == FBE_STATUS_OK)
        {

            start_time_for_read = fbe_get_time();
            status = fbe_api_block_transport_read_data(lun_object_id, 
                                                       FBE_PACKAGE_ID_SEP_0,
                                                       (fbe_u8_t *) buffer_read, 
                                                       request_size, 
                                                       offset, 
                                                       io_size,
                                                       q_depth);
            elapsed_time_for_read = fbe_get_time() - start_time_for_read;
            if (status == FBE_STATUS_OK)
            {
                if (!fbe_equal_memory(buffer_read, buffer, (fbe_u32_t)(request_size * FBE_BE_BYTES_PER_BLOCK)))
                {
                    fbe_cli_printf("write took %llu ms, read took %llu ms for LUN: %d\n", 
                                   (unsigned long long)(start_time_for_read - start_time_for_write), 
                                   (unsigned long long)elapsed_time_for_read, lun_object_id);
                    fbe_cli_printf("data miscompared\n");
                    status = FBE_STATUS_GENERIC_FAILURE;
                }
            }
        }

        fbe_api_free_memory(buffer);    
        fbe_api_free_memory(buffer_read);    

        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Command failed for lun: %d, status:0x%x\n", lun_object_id, status);
        }
        else
        {
            fbe_cli_printf("write took %llu ms, read took %llu ms for LUN: %d\n", 
                           (unsigned long long)(start_time_for_read - start_time_for_write), 
                           (unsigned long long)elapsed_time_for_read, lun_object_id);
        }
    }
    else
    {
        fbe_cli_printf("No -paint requested for LUN: %d...\n", lun_object_id);
    }
    
    return;
}

/****************************
* end fbe_cli_private_luns_access()
****************************/

/*!*******************************************************************
 * @var fbe_cli_paint_buffer
 *********************************************************************
 * @brief       fill in data buffer
 *
 *  @param      buffer - buffer storing the data
 *  @param      request_size - # of blocks to fill
 *
 * @return      status.
 *
 * @author
 *  12/04/2013 - Created.
 *********************************************************************/
static fbe_status_t fbe_cli_paint_buffer(fbe_u8_t *buffer, fbe_lba_t request_size)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_sector_t   *sector_p = NULL;
    fbe_data_pattern_info_t  data_pattern_info;
    fbe_lba_t       seed = 0;
    fbe_u32_t       block_index;

    status = fbe_data_pattern_build_info(&data_pattern_info,
                                         FBE_DATA_PATTERN_LBA_PASS,
                                         FBE_DATA_PATTERN_FLAGS_INVALID,
                                         (fbe_u32_t)0,
                                         0x55,  /* sequence id used in seed; arbitrary */
                                         0,     /* no header words */
                                         NULL   /* no header array */);
    sector_p = (fbe_sector_t *)buffer;

    for (block_index = 0; block_index < request_size; block_index++)
    {
        /* Always update the seed value
         */
        status = fbe_data_pattern_update_seed(&data_pattern_info,
                                              seed /*incremented seed*/);
        fbe_data_pattern_fill_sector(1, /* 1 block. */
                                         &data_pattern_info,
                                         (fbe_u32_t *)sector_p,
                                         FBE_TRUE, /* append CRC */ 
                                         FBE_BE_BYTES_PER_BLOCK,
                                         FBE_TRUE /* Already mapped in, so its 'virtual' */);
    
        if (seed != (fbe_u32_t) - 1)
        {
            seed++;
        }
        sector_p++;
    
    } /* end for all sectors to fill */

    return status;
}

/*!*******************************************************************
* fbe_cli_recover_system_object_config
*********************************************************************
*  @brief    Reinitialize the system object config and persist config to disk
*
*  @param    argc - argument count
*  @param    argv - argument string
*
* @return - none.  
*
* @author
*  5/24/2013 - Created. Yingying Song
*********************************************************************/
void fbe_cli_recover_system_object_config(fbe_s32_t argc, fbe_s8_t** argv)
{
    fbe_object_id_t system_object_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t status = FBE_STATUS_OK;

    if (argc == 0)
    {
        /* If they are asking for help, just display help and exit.*/
        fbe_cli_printf("%s", RECOVER_SYSTEM_OBJECT_CONFIG_USAGE);
        return;
    }

    if ((strcmp(*argv, "-help") == 0) || (strcmp(*argv, "-h") == 0))
    {
        /* If they are asking for help, just display help and exit.*/
        fbe_cli_printf("%s", RECOVER_SYSTEM_OBJECT_CONFIG_USAGE);
        return;
    }
    else if (strcmp(*argv, "-object_id") == 0)
    {
        argc--;
        argv++;

        if (argc <= 0)
        {
            fbe_cli_error("object_id is expected\n");
            return;
        }
        
        while(argc > 0)
        {
            system_object_id = atoi(*argv);
            argc--;
            argv++;

            status = fbe_api_database_generate_system_object_config_and_persist(system_object_id);

            if(status != FBE_STATUS_OK)
            {
                fbe_cli_error("Failed to generate config for obj 0x%x\n",system_object_id);
                return;
            }
            else
            {
                fbe_cli_printf("Successfully generate config for obj 0x%x\n",system_object_id);
            }
        }
        return;
    }
     else if(strcmp(*argv, "-all") == 0)
     {
         argc--;
         argv++;

         status = fbe_api_database_generate_config_for_all_system_rg_and_lun_and_persist();

         if(status != FBE_STATUS_OK)
         {
             fbe_cli_error("Failed to generate config for all system rg and lun\n");
             return;
         }
         else
         {
             fbe_cli_printf("Successfully generate config for all system rg and lun\n");
         }

         return;
     }
     else
     {
           fbe_cli_error("Invalid argument <%s>\n",*argv);
           return;
     }

}/*!*******************************************************************
* END of fbe_cli_recover_system_object_config
*********************************************************************/




/*!*******************************************************************
* fbe_cli_modify_system_object_config
*********************************************************************
*  @brief    Modify object_entry/user_entry/edge_entry of the system object
*
*  @param    argc - argument count
*  @param    argv - argument string
*
* @return - none.  
*
* @author
*  5/22/2013 - Created. Yingying Song
*********************************************************************/
void fbe_cli_modify_system_object_config(fbe_s32_t argc, fbe_s8_t** argv)
{
    fbe_object_id_t system_object_id = FBE_OBJECT_ID_INVALID;

    if (argc == 0)
    {
        /* If they are asking for help, just display help and exit.*/
        fbe_cli_printf("%s", MODIFY_SYSTEM_OBJECT_CONFIG_USAGE);
        return;
    }

    if ((strcmp(*argv, "-help") == 0) || (strcmp(*argv, "-h") == 0))
    {
        /* If they are asking for help, just display help and exit.*/
        fbe_cli_printf("%s", MODIFY_SYSTEM_OBJECT_CONFIG_USAGE);
        return;
    }
    else if (strcmp(*argv, "-object_id") == 0)
    {
        argc--;
        argv++; 

        if (argc <= 0)
        {
            fbe_cli_error("object id is expected\n");
            return;
        }

        system_object_id = atoi(*argv);

        argc--;
        argv++;

        if(argc <= 0)
        {
            fbe_cli_error("-object_entry/-user_entry/-edge_entry is expected.\n");
            return;
        }

        if(strcmp(*argv, "-object_entry") == 0)
        {
            argc--;
            argv++;

            fbe_cli_modify_object_entry_of_system_object(system_object_id, argc, argv);

            return;
        }
        else if(strcmp(*argv,"-user_entry") == 0)
        {
            argc--;
            argv++;

            fbe_cli_modify_user_entry_of_system_object(system_object_id, argc, argv);

            return;
        }
        else if(strcmp(*argv,"-edge_entry") == 0)
        {
            argc--;
            argv++;
            
            fbe_cli_modify_edge_entry_of_system_object(system_object_id, argc, argv);

            return;
        }            
        else
        {
            fbe_cli_error("Invalid argument <%s>.\n",*argv);
            fbe_cli_printf("%s", MODIFY_SYSTEM_OBJECT_CONFIG_USAGE);
            return;
        }

    }
    else
    {
        /* The command line parameters should be properly entered */
        fbe_cli_error("Invalid argument <%s>.\n",*argv);
        fbe_cli_printf("%s", MODIFY_SYSTEM_OBJECT_CONFIG_USAGE);
        return;
    }
}/*!*******************************************************************
* END of fbe_cli_modify_system_object_config
*********************************************************************/



/*!*******************************************************************
* fbe_cli_modify_object_entry_of_system_object
*********************************************************************
*  @brief    Modify object_entry of the system object
*
*  @param    fbe_object_id_t system_object_id 
*  @param    argc - argument count
*  @param    argv - argument string
*
* @return - none.  
*
* @author
*  5/22/2013 - Created. Yingying Song
*********************************************************************/
void fbe_cli_modify_object_entry_of_system_object(fbe_object_id_t system_object_id, fbe_s32_t argc, fbe_s8_t** argv)
{
    database_object_entry_t object_entry;
    fbe_database_control_system_db_op_t system_db_cmd;
    fbe_status_t status = FBE_STATUS_OK;

    fbe_zero_memory(&object_entry, sizeof (database_object_entry_t));
    fbe_zero_memory(&system_db_cmd, sizeof (fbe_database_control_system_db_op_t));
 
    if(argc == 0)
    {
        fbe_cli_error("Not enough argument to modify the object entry\n");
        return;
    }

    system_db_cmd.cmd = FBE_DATABASE_CONTROL_SYSTEM_DB_READ_OBJECT;
    system_db_cmd.object_id = system_object_id;
    status = fbe_api_database_system_db_send_cmd(&system_db_cmd);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("Failed to get original object_entry for object %d\n", system_db_cmd.object_id);
        return;
    }

    fbe_copy_memory(&object_entry, system_db_cmd.cmd_data, sizeof(database_object_entry_t));
    object_entry.header.object_id = system_object_id;
    

    while(argc > 0)
    {
        if(strcmp(*argv, "-state") == 0)
        {
            argc--;
            argv++;

            if (argc <= 0)
            {
                fbe_cli_error("state 0/1 is expected\n");
                return;
            }

            (object_entry.header).state = atoi(*argv);

            argc--;
            argv++;
        }
        else if(strcmp(*argv,"-version_header_size") == 0)
        {
            argc--;
            argv++;

            if (argc <= 0)
            {
                fbe_cli_error("version_header size is expected\n");
                return;
            }

            object_entry.header.version_header.size = atoi(*argv);

            argc--;
            argv++;

        }
        else if(strcmp(*argv,"-db_class_id") == 0)
        {
            argc--;
            argv++;

            if (argc <= 0)
            {
                fbe_cli_error("pvd/parity/mirror/striper/lun is expected\n");
                return;
            }

            if(strcmp(*argv, "pvd") == 0)
            {
                object_entry.db_class_id = DATABASE_CLASS_ID_PROVISION_DRIVE;
                argc--;
                argv++;
            }
            else if(strcmp(*argv,"parity") == 0)
            {
                object_entry.db_class_id = DATABASE_CLASS_ID_PARITY;
                argc--;
                argv++;
            }
            else if(strcmp(*argv,"mirror") == 0)
            {
                object_entry.db_class_id = DATABASE_CLASS_ID_MIRROR;
                argc--;
                argv++;
            }
            else if(strcmp(*argv,"striper") == 0)
            {
                object_entry.db_class_id = DATABASE_CLASS_ID_STRIPER;
                argc--;
                argv++;
            }                    
            else if(strcmp(*argv,"lun") == 0)
            {
                object_entry.db_class_id = DATABASE_CLASS_ID_LUN;
                argc--;
                argv++;
            }
            else if(strcmp(*argv,"0") == 0)
            {
                object_entry.db_class_id = 0;
                argc--;
                argv++;
            }
            else
            {
                fbe_cli_error("Invalid argument <%s>\n",*argv);
                fbe_cli_printf("pvd/parity/mirror/striper/lun is expected.\n");
                return;
            }
        }
        else if(strcmp(*argv,"-set_config_union") == 0)
        {
            argc--;
            argv++;

            if (argc <= 0)
            {
                fbe_cli_error("set_config_union: 0 is expected\n");
                return;
            }

            if(strcmp(*argv,"0") == 0)
            {
                fbe_zero_memory(&(object_entry.set_config_union), sizeof(object_entry.set_config_union));
                argc--;
                argv++;
            }
        }
        else
        {
            fbe_cli_error("Invalid argument <%s>.\n",*argv);
            fbe_cli_printf("%s", MODIFY_SYSTEM_OBJECT_CONFIG_USAGE);
            return;
        }
    }
    system_db_cmd.cmd = FBE_DATABASE_CONTROL_SYSTEM_DB_WRITE_OBJECT;
    system_db_cmd.object_id = system_object_id;
    system_db_cmd.persist_type = FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE;
    fbe_copy_memory(system_db_cmd.cmd_data, &object_entry, sizeof(database_object_entry_t));

    status = fbe_api_database_system_db_send_cmd(&system_db_cmd);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("Failed to modify object entry for object %d\n", object_entry.header.object_id);
        return;
    }
    else
    {
        fbe_cli_printf("Modify system object entry successfully\n");
        return;
    }
}/*!*******************************************************************
  * END of fbe_cli_modify_object_entry_of_system_object
  *********************************************************************/




/*!*******************************************************************
* fbe_cli_modify_user_entry_of_system_object
*********************************************************************
*  @brief    Modify user_entry of the system object
*
*  @param    fbe_object_id_t system_object_id 
*  @param    argc - argument count
*  @param    argv - argument string
*
* @return - none.  
*
* @author
*  5/22/2013 - Created. Yingying Song
*********************************************************************/
void fbe_cli_modify_user_entry_of_system_object(fbe_object_id_t system_object_id, fbe_s32_t argc, fbe_s8_t** argv)
{
    database_user_entry_t user_entry;
    fbe_database_control_system_db_op_t system_db_cmd;
    fbe_status_t status = FBE_STATUS_OK;

    fbe_zero_memory(&user_entry, sizeof (database_user_entry_t));
    fbe_zero_memory(&system_db_cmd, sizeof (fbe_database_control_system_db_op_t));

    if(argc == 0)
    {
        fbe_cli_error("Not enough argument to modify the user entry\n");
        return;
    }

    system_db_cmd.cmd = FBE_DATABASE_CONTROL_SYSTEM_DB_READ_USER;
    system_db_cmd.object_id = system_object_id;
    status = fbe_api_database_system_db_send_cmd(&system_db_cmd);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("Failed to get original user_entry for object %d\n", system_db_cmd.object_id);
        return;
    }

    fbe_copy_memory(&user_entry, system_db_cmd.cmd_data, sizeof(database_user_entry_t));
    user_entry.header.object_id = system_object_id;

    while(argc > 0)
    {
        if(strcmp(*argv, "-state") == 0)
        {
            argc--;
            argv++;

            if (argc <= 0)
            {
                fbe_cli_error("state 0/1 is expected\n");
                return;
            }

            (user_entry.header).state = atoi(*argv);

            argc--;
            argv++;
        }
        else if(strcmp(*argv,"-version_header_size") == 0)
        {
            argc--;
            argv++;

            if (argc <= 0)
            {
                fbe_cli_error("version_header size is expected\n");
                return;
            }

            user_entry.header.version_header.size = atoi(*argv);

            argc--;
            argv++;

        }
        else if(strcmp(*argv,"-db_class_id") == 0)
        {
            argc--;
            argv++;

            if (argc <= 0)
            {
                fbe_cli_error("pvd/parity/mirror/striper/lun is expected\n");
                return;
            }

            if(strcmp(*argv, "pvd") == 0)
            {
                user_entry.db_class_id = DATABASE_CLASS_ID_PROVISION_DRIVE;
                argc--;
                argv++;
            }
            else if(strcmp(*argv,"parity") == 0)
            {
                user_entry.db_class_id = DATABASE_CLASS_ID_PARITY;
                argc--;
                argv++;
            }
            else if(strcmp(*argv,"mirror") == 0)
            {
                user_entry.db_class_id = DATABASE_CLASS_ID_MIRROR;
                argc--;
                argv++;
            }
            else if(strcmp(*argv,"striper") == 0)
            {
                user_entry.db_class_id = DATABASE_CLASS_ID_STRIPER;
                argc--;
                argv++;
            }                    
            else if(strcmp(*argv,"lun") == 0)
            {
                user_entry.db_class_id = DATABASE_CLASS_ID_LUN;
                argc--;
                argv++;
            }
            else if(strcmp(*argv,"0") == 0)
            {
                user_entry.db_class_id = 0;
                argc--;
                argv++;
            }
            else
            {
                fbe_cli_error("Invalid argument <%s>\n",*argv);
                fbe_cli_printf("pvd/parity/mirror/striper/lun is expected.\n");
                return;
            }
        }
        else if(strcmp(*argv,"-user_data_union") == 0)
        {
            argc--;
            argv++;

            if (argc <= 0)
            {
                fbe_cli_error("user_data_union : 0 is expected\n");
                return;
            }

            if(strcmp(*argv,"0") == 0)
            {
                fbe_zero_memory(&(user_entry.user_data_union), sizeof(user_entry.user_data_union));
                argc--;
                argv++;
            }
        }
        else
        {
            fbe_cli_error("Invalid argument <%s>.\n",*argv);
            fbe_cli_printf("%s", MODIFY_SYSTEM_OBJECT_CONFIG_USAGE);
            return;
        }
    }
    system_db_cmd.cmd = FBE_DATABASE_CONTROL_SYSTEM_DB_WRITE_USER;
    system_db_cmd.object_id = system_object_id;
    system_db_cmd.persist_type = FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE;
    fbe_copy_memory(system_db_cmd.cmd_data, &user_entry, sizeof(database_user_entry_t));

    status = fbe_api_database_system_db_send_cmd(&system_db_cmd);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("Failed to modify the user entry for object %d\n", system_object_id);
        return;
    }
    else
    {
        fbe_cli_printf("Modify system user entry successfully\n");
        return;
    }
}/*!*******************************************************************
  * END of fbe_cli_modify_user_entry_of_system_object
  *********************************************************************/




/*!*******************************************************************
 * fbe_cli_modify_edge_entry_of_system_object
 *********************************************************************
 *  @brief    Modify edge_entry of the system object
 *
 *  @param    fbe_object_id_t system_object_id 
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  5/22/2013 - Created. Yingying Song
 *********************************************************************/
void fbe_cli_modify_edge_entry_of_system_object(fbe_object_id_t system_object_id, fbe_s32_t argc, fbe_s8_t** argv)
{
    database_edge_entry_t edge_entry;
    fbe_database_control_system_db_op_t system_db_cmd;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t object_id = system_object_id;

    fbe_zero_memory(&edge_entry, sizeof (database_edge_entry_t));
    fbe_zero_memory(&system_db_cmd, sizeof (fbe_database_control_system_db_op_t));

    if(argc == 0)
    {
        fbe_cli_error("Not enough argument to modify the edge entry\n");
        return;
    }

    system_db_cmd.cmd = FBE_DATABASE_CONTROL_SYSTEM_DB_READ_EDGE;
    system_db_cmd.object_id = object_id;
    system_db_cmd.edge_index = 0;
    if(argc > 0 && strcmp(*argv, "-edge_index") == 0)
    {
       argc--;
       argv++;

       if (argc <= 0)
       {
           fbe_cli_error("edge index is expected\n");
           return;
       }

       system_db_cmd.edge_index = atoi(*argv);

       argc--;
       argv++;
       
    }

    status = fbe_api_database_system_db_send_cmd(&system_db_cmd);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("Failed to get original edge_entry for object %d\n", system_db_cmd.object_id);
        return;
    }

    fbe_copy_memory(&edge_entry, system_db_cmd.cmd_data, sizeof(database_edge_entry_t));
    edge_entry.header.object_id = object_id;

    while(argc > 0)
    {
        if(strcmp(*argv, "-state") == 0)
        {
            argc--;
            argv++;

            if (argc <= 0)
            {
                fbe_cli_error("state 0/1 is expected\n");
                return;
            }

            (edge_entry.header).state = atoi(*argv);

            argc--;
            argv++;
        }
        else if(strcmp(*argv,"-version_header_size") == 0)
        {
            argc--;
            argv++;

            if (argc <= 0)
            {
                fbe_cli_error("version_header size is expected\n");
                return;
            }

            edge_entry.header.version_header.size = atoi(*argv);

            argc--;
            argv++;
        }
        else if(strcmp(*argv,"-server_id") == 0)
        {
            argc--;
            argv++;

            if (argc <= 0)
            {
                fbe_cli_error("server_id is expected\n");
                return;
            }

            edge_entry.server_id = atoi(*argv);

            argc--;
            argv++;

        }
        else if(strcmp(*argv,"-capacity") == 0)
        {
            argc--;
            argv++;

            if (argc <= 0)
            {
                fbe_cli_error("capacity is expected\n");
                return;
            }

            edge_entry.capacity = (fbe_lba_t)_strtoui64(*argv, NULL, 10);

            argc--;
            argv++;

        }						
        else if(strcmp(*argv,"-offset") == 0)
        {
            argc--;
            argv++;

            if (argc <= 0)
            {
                fbe_cli_error("offset is expected\n");
                return;
            }

            edge_entry.offset = (fbe_lba_t)_strtoui64(*argv, NULL, 10);

            argc--;
            argv++;
        }
        else if(strcmp(*argv,"-ignore_offset") == 0)
        {
            argc--;
            argv++;

            if (argc <= 0)
            {
                fbe_cli_error("ignore_offset: ignore/invalid is expected\n");
                return;
            }

            if(strcmp(*argv,"ignore") == 0)
            {                        
                edge_entry.ignore_offset = FBE_EDGE_FLAG_IGNORE_OFFSET;
                argc--;
                argv++;
            }
            else if(strcmp(*argv,"invalid") == 0)
            {                        
                edge_entry.ignore_offset = FBE_EDGE_FLAG_INVALID;
                argc--;
                argv++;
            }
            else
            {
                fbe_cli_error("ignore/invalid is expected. Invalid argument <%s>.\n",*argv);
                return;
            }
        }					
        else
        {
            fbe_cli_error("Invalid argument <%s>.\n",*argv);
            fbe_cli_printf("%s", MODIFY_SYSTEM_OBJECT_CONFIG_USAGE);
            return;
        }
    }
    edge_entry.header.object_id = object_id;
    system_db_cmd.cmd = FBE_DATABASE_CONTROL_SYSTEM_DB_WRITE_EDGE;
    system_db_cmd.object_id = object_id;
    system_db_cmd.persist_type = FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE;
    fbe_copy_memory(system_db_cmd.cmd_data, &edge_entry, sizeof(database_edge_entry_t));

    status = fbe_api_database_system_db_send_cmd(&system_db_cmd);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("Failed to modify the edge entry for object %d\n", object_id);
        return;
    }
    else
    {
        fbe_cli_printf("Modify system edge entry successfully\n");
        return;
    } 
}/*!*******************************************************************
  * END of fbe_cli_modify_edge_entry_of_system_object
  *********************************************************************/



/*!*******************************************************************
* @var fbe_cli_recreate_system_object
*********************************************************************
* @brief Function to implement recreation of system PVD, RG or LUNs.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
* @return - none.  
*
* @author
*  8/23/2012 - Created. gaoh1
*********************************************************************/
 void fbe_cli_recreate_system_object(fbe_s32_t argc, fbe_s8_t** argv)
{
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_u32_t lun_number = FBE_LUN_ID_INVALID;
    fbe_object_id_t lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t rg_number = FBE_RAID_ID_INVALID;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t  ndb = 0;
    fbe_u32_t  pvd_need_zero = 0;
    fbe_u32_t  pvd_need_notify_NR = 0;
    fbe_u8_t   recreate_flags = 0;
    fbe_bool_t reset_all_flags = FBE_FALSE;

    if (argc == 0)
    {
        /* If they are asking for help, just display help and exit.*/
        fbe_cli_printf("%s", RECREATE_SYSTEM_OBJECT_USAGE);
        return;
    }

    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /* If they are asking for help, just display help and exit.*/
            fbe_cli_printf("%s", RECREATE_SYSTEM_OBJECT_USAGE);
            return;
        }
        else if ((strcmp(*argv, "-reset") == 0))
        {
            argc--;
            argv++;
            reset_all_flags = FBE_TRUE;
        }
        else if ((strcmp(*argv, "-lun") == 0))
        {
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("LUN number is expected\n");
                return;
            }

            lun_number = atoi(*argv);

            argc--;
            argv++;
        }
        else if ((strcmp(*argv, "-lun_id") == 0))
        {
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("LUN object id is expected\n");
                return;
            }

            lun_object_id = atoi(*argv);

            argc--;
            argv++;
        }
        else if ((strcmp(*argv, "-rg") == 0))
        {
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("RG object id is expected\n");
                return;
            }

            rg_number = atoi(*argv);

            argc--;
            argv++;
        }
        else if ((strcmp(*argv, "-rg_id") == 0))
        {
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("RG object id is expected\n");
                return;
            }

            rg_object_id = atoi(*argv);

            argc--;
            argv++;
        }
        else if ((strcmp(*argv, "-pvd_id") == 0))
        {
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("RG object id is expected\n");
                return;
            }

            pvd_object_id = atoi(*argv);

            argc--;
            argv++;
        }
        else if ((strcmp(*argv, "-ndb") == 0))
        {
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("NDB flag is expected\n");
                return;
            }

            ndb = atoi(*argv);

            argc--;
            argv++;
        }
        else if ((strcmp(*argv, "-need_zero") == 0))
        {
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Need zero flag is expected\n");
                return;
            }

            pvd_need_zero = atoi(*argv);

            argc--;
            argv++;
        } else if ((strcmp(*argv, "-force_nr") == 0))
        {
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("force NR flag is expected\n");
                return;
            }

            pvd_need_notify_NR = atoi(*argv);

            argc--;
            argv++;
        } else 
        {
            /* The command line parameters should be properly entered */
            fbe_cli_error("Invalid argument.\n");
            fbe_cli_printf("%s", ZERO_LUN_USAGE);
            return;
        }
    }

    if(lun_number != FBE_LUN_ID_INVALID)
    {
        status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Invalid LUN number: %d, status:0x%x\n",lun_number, status);
            return;
        }
    }

    if(rg_number != FBE_RAID_ID_INVALID)
    {
        status = fbe_api_database_lookup_raid_group_by_number(rg_number, &rg_object_id);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Invalid RG number: %d, status:0x%x\n",rg_number, status);
            return;
        }
    }
    
    if (reset_all_flags == FBE_TRUE)
    {
       status = fbe_api_database_reset_system_object_recreation();
    }

    status = FBE_STATUS_OK;

    /* If Lun object id is valid, recreate the system lun*/
    if (lun_object_id != FBE_OBJECT_ID_INVALID)
    {
        if (!fbe_private_space_layout_object_id_is_system_lun(lun_object_id))
        {
            fbe_cli_printf("Invalid LUN object id, it should be a system LUN id (from %d to %d)\n",
                           FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_FIRST,
                           FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_LAST);
            return;
        }
        if (ndb == 1)
        {
            recreate_flags |= FBE_SYSTEM_OBJECT_OP_NDB_LUN;
        }
        recreate_flags |= FBE_SYSTEM_OBJECT_OP_RECREATE;
        status = fbe_api_database_set_system_object_recreation(lun_object_id, recreate_flags);
    }

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Command faild, status:0x%x\n", status);
        return;
    }

    /* If RG object id is valid, recreate the system RG*/
    if (rg_object_id != FBE_OBJECT_ID_INVALID)
    {
        if (!fbe_private_space_layout_object_id_is_system_raid_group(rg_object_id))
        {
            fbe_cli_printf("Invalid RG object id, it should be a system RG id (from %d to %d)\n",
                           FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_FIRST,
                           FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_LAST);
            return;
        }
        recreate_flags |= FBE_SYSTEM_OBJECT_OP_RECREATE;
        status = fbe_api_database_set_system_object_recreation(rg_object_id, recreate_flags);
    }

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Command faild, status:0x%x\n", status);
        return;
    }

    /* If PVD object id is valid, recreate the system PVD*/
    if (pvd_object_id != FBE_OBJECT_ID_INVALID)
    {
        if (!fbe_private_space_layout_object_id_is_system_pvd(pvd_object_id))
        {
            fbe_cli_printf("Invalid PVD object id, it should be a system PVD id(from %d to %d)\n",
                            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST,
                            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_LAST);
            return;                           
        }
        if (pvd_need_zero == 1)
        {
            recreate_flags |= FBE_SYSTEM_OBJECT_OP_PVD_ZERO;
        }
        if (pvd_need_notify_NR == 1)
        {
            recreate_flags |= FBE_SYSTEM_OBJECT_OP_PVD_NR;
        }
        recreate_flags |= FBE_SYSTEM_OBJECT_OP_RECREATE;
        status = fbe_api_database_set_system_object_recreation(pvd_object_id, recreate_flags);
    }

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Command faild, status:0x%x\n", status);
        return;
    }


}
/****************************
* end fbe_cli_recreate_system_object()
****************************/

/*!*******************************************************************
* @var fbe_cli_destroy_broken_system_parity_rg_and_lun
*********************************************************************
* @brief Function to destroy system parity raid group and related Luns.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
* @return - none.  
*
* @author
*  11/02/2012 - Created. gaoh1
*********************************************************************/
void fbe_cli_destroy_broken_system_parity_rg_and_lun(fbe_s32_t argc, fbe_s8_t** argv)
{
    fbe_status_t    status;
    fbe_u32_t       index;
    fbe_u32_t       lun_num;
    fbe_api_lun_destroy_t lun_destroy_req;
    fbe_api_job_service_raid_group_destroy_t raid_group_destroy_req;
    fbe_job_service_error_type_t      error_type = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    fbe_u32_t  reg_index;
    fbe_private_space_layout_region_t region = {0};
    fbe_private_space_layout_lun_info_t lun_info = {0};
    fbe_lifecycle_state_t lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_u8_t recreate_flags = 0;
    fbe_status_t    job_status;

    fbe_zero_memory(&lun_destroy_req, sizeof(fbe_api_lun_destroy_t));
    fbe_zero_memory(&raid_group_destroy_req, sizeof(fbe_api_job_service_raid_group_destroy_t));
    
    if (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /* If they are asking for help, just display help and exit.*/
            fbe_cli_printf("%s", DESTROY_SYSTEM_PARITY_RG_AND_LUN);
            return;
        }
    }

    /* generate config for parity RG */
    for(reg_index = 0; reg_index < FBE_PRIVATE_SPACE_LAYOUT_MAX_REGIONS; reg_index++){
        status = fbe_private_space_layout_get_region_by_index(reg_index, &region);
        if (status != FBE_STATUS_OK) {
            fbe_cli_printf("fail to get pri space layout for region index %d,stat:0x%x\n",
                           reg_index, status);
            return;
        }
        else
        {
            // Hit the Terminator
            if(region.region_id == FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID) 
            {
                break;
            }
            else if(region.region_type != FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP)
            {
                continue;
            } else
            {
                /*Only process the Parity RG */
                if (region.raid_info.raid_type != FBE_RAID_GROUP_TYPE_RAID3 &&
                    region.raid_info.raid_type != FBE_RAID_GROUP_TYPE_RAID5 &&
                    region.raid_info.raid_type != FBE_RAID_GROUP_TYPE_RAID6)
                {
                    continue;
                }

                /*First we check whether the RG is in good state */
                status = fbe_api_get_object_lifecycle_state(region.raid_info.object_id, 
                                                                      &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
                if (status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("%s Get lifecycle failed: obj id: 0x%x status: 0x%x \n",
                                   __FUNCTION__, region.raid_info.object_id, status);
                    return;
                }
                /*If the Raid group is not in fail and specialize state, skip it */
                if (lifecycle_state != FBE_LIFECYCLE_STATE_FAIL && lifecycle_state != FBE_LIFECYCLE_STATE_SPECIALIZE)
                    continue;


                /* Before destroy the RG, destroy all the LUN on it.
                          *  We can't use fbe_api_base_config_get_upstream_object_list function to 
                          *  get all the LUN list if all the objects are in specialize state.
                          *  We use the PSL to get all the LUN above the RG.
                          */
                for (index = 0; index <= FBE_PRIVATE_SPACE_LAYOUT_MAX_PRIVATE_LUNS; index++)
                {
                    status = fbe_private_space_layout_get_lun_by_index(index, &lun_info);
                    if (status != FBE_STATUS_OK)
                    {
                        fbe_cli_printf("fail to get pri space layout for LUN index %d,stat: 0x%x\n",
                                       index, status);
                        return;
                    }
                    else if(lun_info.lun_number == FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID)
                    {
                        // hit the terminator
                        break;
                    }
                    else if(region.raid_info.raid_group_id == lun_info.raid_group_id)
                    {
                        status = fbe_api_database_lookup_lun_by_object_id(lun_info.object_id, &lun_num);
                        if (status != FBE_STATUS_OK)
                        {
                            fbe_cli_error("LUN %d does not exist or unable to retrieve information for this LUN.\n",lun_num);
                            continue; // We should continue to destroy next one.
                        }
                        lun_destroy_req.lun_number = lun_num;
                        lun_destroy_req.allow_destroy_broken_lun = FBE_TRUE;
                        status= fbe_api_destroy_lun(&lun_destroy_req, FBE_TRUE,LU_DESTROY_TIMEOUT, &error_type);
                        if (status != FBE_STATUS_OK)
                        {
                            fbe_cli_error("not able to destroy a LUN %d\n", lun_num);
                            return;
                        }
                        fbe_cli_printf("Destroy System LUN: %d\n", lun_num);
                    }

                }

                /*After all the above LUNs destroyed, destroy the RG*/
                raid_group_destroy_req.raid_group_id = region.raid_info.raid_group_id;
                raid_group_destroy_req.allow_destroy_broken_rg = FBE_TRUE;
                status = fbe_api_job_service_raid_group_destroy(&raid_group_destroy_req);

                /* Check the status for SUCCESS*/
                if (status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("%s:\tRG %d removal failed %d\n", __FUNCTION__,
                                  raid_group_destroy_req.raid_group_id, status);
                    return;
                }

                /* wait for it*/
                status = fbe_api_common_wait_for_job(raid_group_destroy_req.job_number, 
                                         RG_WAIT_TIMEOUT, &error_type, &job_status, NULL);
                if (status != FBE_STATUS_OK || job_status != FBE_STATUS_OK) 
                {
                    fbe_cli_printf("%s: wait for RG job failed. Status: 0x%X, job status:0x%X, job error:0x%x\n", 
                               __FUNCTION__, status, job_status, error_type);

                    return;
                }
                fbe_cli_printf("Destroy System RG: %d\n", region.raid_info.raid_group_id);
                /*For system RG, we also need to zero out the nonpaged metadata. */
                recreate_flags |= FBE_SYSTEM_OBJECT_OP_RECREATE;
                status = fbe_api_database_set_system_object_recreation(region.raid_info.object_id, recreate_flags);

            }            

        }
            
    }

    return;
}
/****************************
  * end fbe_cli_destroy_broken_system_parity_rg_and_lun()
*****************************/

/*!*******************************************************************
* @var fbe_cli_generate_config_for_system_parity_rg_and_lun
*********************************************************************
* @brief Function to generate system parity raid group and related Luns from PSL.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
* @return - none.  
*
* @author
*  11/02/2012 - Created. gaoh1
*********************************************************************/
void fbe_cli_generate_config_for_system_parity_rg_and_lun(fbe_s32_t argc, fbe_s8_t** argv)
{
    fbe_status_t status = FBE_STATUS_OK;
    
    if (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /* If they are asking for help, just display help and exit.*/
            fbe_cli_printf("%s", GENERATE_CONFIG_FOR_SYSTEM_PARITY_RG_AND_LUN);
            return;
        }
        else if(strcmp(*argv,"-ndblun") == 0)
        {
             status = fbe_api_database_generate_configuration_for_system_parity_rg_and_lun(FBE_TRUE);
             if (status != FBE_STATUS_OK)
             {
                 fbe_cli_printf("fail to generate config for system parity rg and lun\n");
                 return;
             }
             fbe_cli_printf("success to generate config for system parity rg and lun, ndb is true.\n");
             return;
        }
        else
        {
            fbe_cli_printf("Invalid argument <%s>", *argv);
            return;
        }
    }

    status = fbe_api_database_generate_configuration_for_system_parity_rg_and_lun(FBE_FALSE);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("fail to generate the configuration for system parity rg and lun\n");
        return;
    }
    fbe_cli_printf("success to generate the configuration for system parity rg and lun\n");

    return;
}
/****************************
  * end fbe_cli_generate_config_for_system_parity_rg_and_lun()
*****************************/


/*!*******************************************************************
 * @var fbe_cli_get_drive_rebuild_status
 *********************************************************************
 * @brief       Function displays rebuild status.
 *
 *  @param      lun_details - lun detail
 *  @param      rg_details - raid group detail
 *  @param      index - context of lun.
 *
 * @return      status.
 *
 * @author
 * Created. Harshal Wanjari
 *********************************************************************/
fbe_status_t fbe_cli_get_drive_rebuild_status(fbe_cli_lurg_lun_details_t *lun_details,fbe_cli_lurg_rg_details_t *rg_details,fbe_u32_t index)
{
    fbe_u8_t          line_buffer[fbe_cli_lustat_line_size];
    fbe_u8_t*         line_seeker = fbe_cli_lustat_reset_line(line_buffer);
    fbe_u8_t          sprintf_buffer[fbe_cli_lustat_line_size];
    fbe_status_t      status;
    fbe_u16_t         reb_percentage;
    fbe_api_raid_group_get_info_t raid_group_info;
    fbe_object_id_t obj_id;
    fbe_lifecycle_state_t lifecycle_state;



    status = fbe_api_database_lookup_raid_group_by_number(rg_details->rg_number,&obj_id);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Fail to get the Raid Group Object id for Rg:0x%x, status:0x%x",rg_details->rg_number,status);
        return status;
    }

    //  Get the raid group information
    status = fbe_api_raid_group_get_info(obj_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Fail to get the Raid Group Info for object id:0x%x, status:0x%x",obj_id,status);
        return status;
    }

    if ((lun_details->lun_info.raid_info.rebuild_checkpoint[index] != FBE_LBA_INVALID) && raid_group_info.b_rb_logging[index] == FBE_FALSE)
    {
        /* Get the object id of the PVD */
        status = fbe_api_provision_drive_get_obj_id_by_location(rg_details->drive_list[index].port_num,
        rg_details->drive_list[index].encl_num, rg_details->drive_list[index].slot_num, &obj_id);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Fail to get PVD object id for %d_%d_%d, status:0x%x",rg_details->drive_list[index].port_num,
        rg_details->drive_list[index].encl_num, rg_details->drive_list[index].slot_num,status);
            return status;
        }

        status = fbe_api_get_object_lifecycle_state(obj_id, &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Fail to get the Lifecycle State Info for object id:0x%x, status:0x%x",obj_id,status);
            return status;
        }

        if(lifecycle_state == FBE_LIFECYCLE_STATE_READY)
        {
            reb_percentage = fbe_cli_get_drive_rebuild_progress(&rg_details->drive_list[index],
                lun_details->lun_info.raid_info.rebuild_checkpoint[index]);

            if(reb_percentage == 101)
            {
                return FBE_STATUS_OK;
            }

            sprintf(sprintf_buffer, "(REB:%d%%) ", reb_percentage);
            line_seeker = fbe_cli_lustat_add_string(line_buffer, line_seeker, sprintf_buffer);
        }
    }

    return status;
}
/***************************************
* end fbe_cli_get_drive_rebuild_status()
****************************************/


/*!*******************************************************************
 * @var fbe_cli_object_id_list_sort
 *********************************************************************
 * @brief       Function sort raid group objectid or lun objectid.
 *
 *  @param      object_list - raid group object id list
 *
 * @return      status.
 *
 * @author
 * Created. Harshal Wanjari
 *********************************************************************/
fbe_status_t fbe_cli_object_id_list_sort(fbe_cli_object_list_t *object_list)
{
    fbe_status_t              status;
    fbe_u32_t                 current_number = 0,index=0,current_index;
    fbe_raid_group_number_t   target_number;
    fbe_object_id_t           object_id;
    fbe_package_id_t      package_id = FBE_PACKAGE_ID_SEP_0;
    fbe_class_id_t            class_id;
    /*sorting of array is done by selection sort methode 
    * in this case here we can sort both raid group or lun object id 
    * according to there number
    */
    status = fbe_api_get_object_class_id(object_list->object_list[index],&class_id,package_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("Unable to get class id for object id:0x%x ,stauts:0x%x",object_list->object_list[index],status);
        return status;
    }

    for(index = 0;index < object_list->number_of_objects;index++)
    {
        if(class_id == FBE_CLASS_ID_LUN)
        {
            status = fbe_api_database_lookup_lun_by_object_id(object_list->object_list[index],&target_number);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("Unable to get Raid group number for object id:0x%x ,stauts:0x%x",object_list->object_list[index],status);
                return status;
            }
        }
        else
        {
            status = fbe_api_database_lookup_raid_group_by_object_id(object_list->object_list[index],&target_number);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("Unable to get Raid group number for object id:0x%x ,stauts:0x%x",object_list->object_list[index],status);
                return status;
            }
        }

        for(current_index = index+1;current_index<object_list->number_of_objects;current_index++)
        {
            if(class_id == FBE_CLASS_ID_LUN)
            {
                status = fbe_api_database_lookup_lun_by_object_id(object_list->object_list[current_index],&current_number);
                if (status != FBE_STATUS_OK)
                {
                    fbe_cli_error("Unable to get Raid group number for object id:0x%x ,stauts:0x%x",object_list->object_list[current_index],status);
                    return status;
                }
            }
            else
            {
                status = fbe_api_database_lookup_raid_group_by_object_id(object_list->object_list[current_index],&current_number);
                if (status != FBE_STATUS_OK)
                {
                    fbe_cli_error("Unable to get Raid group number for object id:0x%x ,stauts:0x%x",object_list->object_list[current_index],status);
                    return status;
                }
            }
            if(current_number < target_number)
            {
                object_id = object_list->object_list[index];
                object_list->object_list[index] = object_list->object_list[current_index];
                object_list->object_list[current_index] = object_id;
                target_number = current_number;
            }
        }
        
    }
    return status;
}
/****************************
* end fbe_cli_object_id_list_sort()
****************************/

void fbe_cli_lun_set_write_bypass_mode(fbe_s32_t argc, fbe_s8_t** argv)
{
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_u32_t lun_number = FBE_LUN_ID_INVALID;
    fbe_object_id_t lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t write_bypass_mode = FBE_FALSE;

    if (argc == 0)
    {
        /* If they are asking for help, just display help and exit.*/
        fbe_cli_printf("%s", ZERO_LUN_USAGE);
        return;
    }

    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /* If they are asking for help, just display help and exit.*/
            fbe_cli_printf("%s", LUN_WRITE_BYPASS_USAGE);
            return;
        }
        else if ((strcmp(*argv, "-lun") == 0))
        {
            /* Get the Bus number from command line */
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("LUN number is expected\n");
                return;
            }

            lun_number = atoi(*argv);

            argc--;
            argv++;
        }
        /* Get the Enclosure number from command line */
        else if ((strcmp(*argv, "-lun_id") == 0))
        {
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("LUN object id is expected\n");
                return;
            }

            lun_object_id = atoi(*argv);

            argc--;
            argv++;
        }
        else if ((strcmp(*argv, "-m") == 0))
        {
            argc--;
            argv++;
            if (argc <= 0)
            {
                fbe_cli_error("Bypass mode is expected\n");
                return;
            }

            write_bypass_mode = atoi(*argv);
            argc--;
            argv++;
        }
        else
        {
            /* The command line parameters should be properly entered */
            fbe_cli_error("Invalid argument.\n");
            fbe_cli_printf("%s", LUN_WRITE_BYPASS_USAGE);
            return;
        }
    }

    if(lun_number != FBE_LUN_ID_INVALID)
    {
        status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Invalid LUN number: %d, status:0x%x\n",lun_number, status);
            return;
        }
    }
    

    status = fbe_api_lun_set_write_bypass_mode( lun_object_id, write_bypass_mode);

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Command failed, status:0x%x\n", status);
        return;
    }

    fbe_cli_printf("Lun id %x bypass mode was set to %d\n", lun_object_id, write_bypass_mode);
    return;
}

/*!*******************************************************************
 * @var fbe_cli_create_system_rg
 *********************************************************************
 * @brief Function to implement create system RGs.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  11/13/2012 - Created. gaoh1
 *********************************************************************/
void fbe_cli_create_system_rg(fbe_s32_t argc, fbe_s8_t ** argv)
{
    fbe_api_raid_group_create_t             rg_request;
    fbe_raid_group_number_t                 rg_num = FBE_RAID_GROUP_INVALID;
    fbe_object_id_t                         rg_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                         rg_id_out;
    fbe_status_t                            status;
    fbe_job_service_error_type_t            job_error_type;

    if (argc < 1)
    {
        fbe_cli_error("%s Not enough arguments to create system obj\n", __FUNCTION__);
        fbe_cli_printf("%s", LURG_CREATE_SYSTEM_RG_USAGE);
        return;
    }
    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /* If they are asking for help, just display help and exit.
            */
            fbe_cli_printf("%s", LURG_CREATE_SYSTEM_RG_USAGE);
            return;
        }
        else if ((strcmp(*argv, "-rg_id") == 0))
        {
            /* Get the RG number from command line */
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("RAID group object id argument is expected\n");
                return;
            }

            rg_id = atoi(*argv);

            if (!fbe_private_space_layout_object_id_is_system_raid_group(rg_id))
            {
                fbe_cli_error("system RG object id is expected.\n");
                return;
            }

            argc--;
            argv++;
        }
        else if ((strcmp(*argv, "-rg") == 0))
        {
            /* Get the RG number from command line */
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("RAID group number argument is expected\n");
                return;
            }

            rg_num = atoi(*argv);

            argc--;
            argv++;
        }
        else
        {
            /* The command line parameters should be properly entered */
            fbe_cli_printf("%s", LURG_CREATE_SYSTEM_RG_USAGE);
            return;
        }
    }

    fbe_zero_memory(&rg_request, sizeof(fbe_api_raid_group_create_t));
    rg_request.is_system_rg = FBE_TRUE;
    rg_request.rg_id = rg_id;
    rg_request.raid_group_id = rg_num;
    
    /*Send request to create a RG*/
    status = fbe_api_create_rg(&rg_request, FBE_TRUE, RG_WAIT_TIMEOUT, &rg_id_out, &job_error_type);

    if (status != FBE_STATUS_OK)
    {
        /* if RG creation request failed then return error from here */
        fbe_cli_printf ("%s:RG creation failed... status = %d\n", __FUNCTION__, status);
        return;
    } else {
        fbe_cli_printf ("%s: Create RG %d with object id %d\n", __FUNCTION__, rg_request.raid_group_id, rg_request.rg_id);
        return;
    }

}
/******************************************
 * end fbe_cli_create_system_rg()
 ******************************************/

/*!*******************************************************************
 * @var fbe_cli_bind_system_lun
 *********************************************************************
 * @brief Function to implement bind system LUNs.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  11/13/2012 - Created. gaoh1
 *********************************************************************/
void fbe_cli_bind_system_lun(fbe_s32_t argc, fbe_s8_t ** argv)
{
    fbe_api_lun_create_t                    lun_create_req;
    fbe_lun_number_t                        lun_num = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                         lun_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t                              ndb = FBE_FALSE;
    fbe_status_t                            status;
    fbe_job_service_error_type_t            job_error_type;
    fbe_assigned_wwid_t                     wwn;   
    fbe_user_defined_lun_name_t             udn;    /*! User-Defined Name for the LUN */
    fbe_u8_t                                *p;
    fbe_u32_t                               index = 0;
    fbe_private_space_layout_lun_info_t     lun_info;

    fbe_zero_memory(&wwn, sizeof(fbe_assigned_wwid_t));
    fbe_zero_memory(&udn, sizeof(fbe_user_defined_lun_name_t));
    if (argc < 1)
    {
        fbe_cli_error("%s Not enough arguments to create system obj\n", __FUNCTION__);
        fbe_cli_printf("%s", LURG_BIND_SYSTEM_LUN_USAGE);
        return;
    }
    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /* If they are asking for help, just display help and exit.
            */
            fbe_cli_printf("%s", LURG_BIND_SYSTEM_LUN_USAGE);
            return;
        }
        else if ((strcmp(*argv, "-lun_id") == 0))
        {
            /* Get the RG number from command line */
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("RAID group object id argument is expected\n");
                return;
            }

            lun_id = atoi(*argv);

            if (!fbe_private_space_layout_object_id_is_system_lun(lun_id))
            {
                //fbe_cli_error("system LUN object id is expected.\n");
                fbe_cli_error("system LUN object id is expected. lun_id = %d \n", lun_id);
                return;
            }

            argc--;
            argv++;
        }
        else if ((strcmp(*argv, "-lun") == 0))
        {
            /* Get the RG number from command line */
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("LUN number argument is expected\n");
                return;
            }

            lun_num = atoi(*argv);

            argc--;
            argv++;
        }
        else if ((strcmp(*argv, "-ndb") == 0))
        {
            /* Get the RG number from command line */
            argc--;
            argv++;

            ndb = FBE_TRUE;
        }
        else if (strcmp(*argv, "-wwn") == 0)
        { 
            argc--;
            argv++;
                
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("wwn argument is expected\n");
                return;
            }

            status = fbe_cli_convert_wwn(*argv, &wwn);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s: fail to convert the wwn string to wwn bytes\n", __FUNCTION__);
                return;
            }

			if(fbe_cli_checks_for_simillar_wwn_number(wwn))
            {
                fbe_cli_printf("%s: Specified wwn number is already assigned to existing lun. \n", __FUNCTION__);
                return;        
            }
			 
            argc--;
            argv++;
        }
        else if (strcmp(*argv, "-udn") == 0)
        { 
            argc--;
            argv++;
                
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("wwn argument is expected\n");
                return;
            }

            p = *argv;

            index = 0;
            while ((*p != '\0')  && index < FBE_USER_DEFINED_LUN_NAME_LEN - 1)
            {
                /* We use '\' instead of blank space */
                if ( *p == '\\')
                {
                    udn.name[index++] = ' ';
                } 
                else 
                {
                    udn.name[index++] = *p;
                }
                p++;
            }
            udn.name[index] = '\0';
            argc--;
            argv++;

            //fbe_cli_printf("%s:\t udn = %s \n", __FUNCTION__, udn.name);
        }
        else
        {
            /* The command line parameters should be properly entered */
            fbe_cli_printf("%s", LURG_BIND_SYSTEM_LUN_USAGE);
            return;
        }
    }

    if (!fbe_private_space_layout_object_id_is_system_lun(lun_id))
    {
        fbe_cli_error("system LUN object id is expected. %d \n", lun_id);
        return;
    }

    status = fbe_private_space_layout_get_lun_by_lun_number(lun_num, &lun_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("Fail to get lun info from psl by lun nubmer: %d.\n", lun_num);
        return;
    }

    if (lun_id != lun_info.object_id) {
        fbe_cli_error("Lun number(%d) doesn't match the lun object id(%d)\n", lun_num, lun_id);
        return;
    }
    /*Initialize the lun create request by the information from PSL*/
    fbe_zero_memory(&lun_create_req, sizeof(fbe_api_lun_create_t));
    lun_create_req.is_system_lun = FBE_TRUE;
    lun_create_req.lun_id = lun_id;
    lun_create_req.lun_number = lun_num;
    lun_create_req.ndb_b = ndb;
    lun_create_req.raid_group_id = lun_info.raid_group_id;
	lun_create_req.world_wide_name = wwn;
	lun_create_req.user_defined_name = udn;

    status = fbe_api_create_lun(&lun_create_req, FBE_TRUE, LU_READY_TIMEOUT, &lun_id, &job_error_type);
    if (status == FBE_STATUS_OK)
    {
        fbe_cli_printf ("Created LU:%d with object ID:0x%X\n",lun_create_req.lun_number ,lun_id);
    }
    else if (status == FBE_STATUS_TIMEOUT) 
    {
        fbe_cli_printf ("Timed out waiting for LU to become ready\n");
    }
    else 
    {
        fbe_cli_printf ("LU creation failue: error code: %d\n", status );
    }

    return;
}
/******************************************
 * end fbe_cli_bind_system_lun()
 ******************************************/

static fbe_bool_t fbe_cli_unbind_warn_user(void)
{
    fbe_char_t buff[64]; 

	fbe_cli_printf("\n\n+==============================================================+\n");
	fbe_cli_printf("|                       W a r n i n g !!!                      |\n");
	fbe_cli_printf("|Unbinding a LUN via fbecli is dangerous and could lead to     |\n");
	fbe_cli_printf("|a system panic because upper layers are not informed by Admin |\n");
	fbe_cli_printf("|about the lun removal                                         |\n");
	fbe_cli_printf("+==============================================================+\n");
																						 
    fbe_cli_printf("\n\nAre you sure you want to Unbind a lun? (y/n)");
    scanf("%s", buff);
    
    if (!strlen(buff))
    {
        strncpy(buff, "n", 64);
    }

    if (buff[0] == 'y' || buff[0] == 'Y')   // random interations
    {        
        return FBE_TRUE;
    }
    else  // fixed num reactivations
    {
		return FBE_FALSE;
    }
    
    
}

static fbe_bool_t fbe_cli_bind_warn_user(void)
{
    fbe_char_t buff[64]; 

	fbe_cli_printf("\n\n+==============================================================+\n");
	fbe_cli_printf("|                       W a r n i n g !!!                      |\n");
	fbe_cli_printf("|Binding a LUN via fbecli is dangerous and could lead to       |\n");
	fbe_cli_printf("|a system panic because upper layers are not informed by Admin |\n");
	fbe_cli_printf("|about the lun creation.                                       |\n");
	fbe_cli_printf("+==============================================================+\n");
																						 
    fbe_cli_printf("\n\nAre you sure you want to Bind a lun? (y/n)");
    scanf("%s", buff);
    
    if (!strlen(buff))
    {
        strncpy(buff, "n", 64);
    }

    if (buff[0] == 'y' || buff[0] == 'Y')   // random interations
    {        
        return FBE_TRUE;
    }
    else  // fixed num reactivations
    {
		return FBE_FALSE;
    }
    
    
}


/*!*******************************************************************
 * @var fbe_cli_lun_operation_warning
 *********************************************************************
 * @brief Function to send a warning ktrace message when LU is created/destroyed via fbecli.
 *
 *  @param    fbe_object_id_t - LUN object ID
 *  @param    fbe_database_lun_operation_code_t - To identify whether LUN was created or destroyed.
 *  @param    fbe_status_t - Status of LUN create or destroy operation.
 *
 * @return - none.  
 *
 * @author
 *  1/23/2013 - Created. Preethi Poddatur
 *********************************************************************/
void fbe_cli_lun_operation_warning(fbe_object_id_t lun_id, fbe_database_lun_operation_code_t operation,fbe_status_t result)
{
   fbe_status_t status = FBE_STATUS_OK;
   status = fbe_api_database_lun_operation_ktrace_warning(lun_id, operation, result);
   if (status != FBE_STATUS_OK) {
      fbe_cli_error("fbe_cli_lun_op: FAILED to send the warn message op: %s\n", 
           (operation == FBE_DATABASE_LUN_CREATE_FBE_CLI)? "LUN Create" : "LUN Destroy");
   }

}

/******************************************
 * end fbe_cli_lun_operation_warning()
 ******************************************/

/*!*******************************************************************
 * @var fbe_cli_slit_wwn
 *********************************************************************
 * @brief Function to split WWN string into argv
 *
 *  @param    
 *  @param    
 *  @param    
 *
 * @return - none.  
 *
 * @author
 *********************************************************************/
static void fbe_cli_slit_wwn(char *line, int *argc, char **argv)
{
    static fbe_u8_t * wwn_delimit = ":";

    *argc = 0;
    *argv = strtok(line, wwn_delimit);
    while (*argv != NULL) {
        *argc = *argc + 1;
        *argv++;
        *argv = strtok(NULL, wwn_delimit);
    }

    return;
}
/******************************************
 * end fbe_cli_slit_wwn()
 ******************************************/

/*!*******************************************************************
 * @var fbe_cli_convert_wwn
 *********************************************************************
 * @brief Function to convert the WWN string to WWN bytes 
 *
 *  @param    
 *  @param    
 *  @param    
 *
 * @return - none.  
 *
 * @author Created by gaoh1
 *********************************************************************/
fbe_status_t fbe_cli_convert_wwn(char *str, fbe_assigned_wwid_t *wwn)
{
    fbe_s32_t       wwn_bytes = 0;
    fbe_char_t      *wwn_e[FBE_WWN_BYTES + 1] = {0};
    fbe_char_t      **wwn_p;
    fbe_u32_t       index, wwn_byte;

    fbe_cli_slit_wwn(str, &wwn_bytes, wwn_e);
    /*wwn bytes can't exceed FBE_WWN_BYTES */
    if (wwn_bytes > FBE_WWN_BYTES)
    {
        fbe_cli_printf("%s:\tSpecified wwn %s is invalid. wwn has %d bytes(should be %d bytes) \n",
                       __FUNCTION__, str, wwn_bytes, FBE_WWN_BYTES);
        return FBE_STATUS_GENERIC_FAILURE;        
    }

    wwn_p = wwn_e;
    index = 0;
    while ((wwn_bytes > 0) && (index < FBE_WWN_BYTES))
    {
        if ((strlen(*wwn_p) == 0) || (strlen(*wwn_p) > 2))
        {
            fbe_cli_printf("%s:\tSpecified wwn is invalid. every element should be in [0, FF] \n",
                           __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;        
        }

        // debug logs
        //fbe_cli_printf("%s:\t debug: wwn_bytes = %d, wwn_p = %s, index = %d \n", __FUNCTION__, wwn_bytes, *wwn_p, index);

        wwn_byte = fbe_atoh(*wwn_p);
        if (wwn_byte == FBE_U32_MAX) 
        {
            fbe_cli_printf("%s:\tSpecified wwn is invalid. every element should be in [0, FF] \n",
                           __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;        
        }
        wwn->bytes[index++] = wwn_byte;

        wwn_bytes--;
        wwn_p++;
    }
    return FBE_STATUS_OK;
}

void fbe_cli_print_wwn(fbe_assigned_wwid_t *wwn)
{
    fbe_u32_t i;
    for (i = 0; i < FBE_WWN_BYTES; i++) 
    {
        fbe_cli_printf("%x:", wwn->bytes[i] );
    }
    fbe_cli_printf("\n");

    return;
}

/*!*******************************************************************
 * @var fbe_cli_lun_hard_zero
 *********************************************************************
 * @brief Function to implement hard zero a lun
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  4/20/2010 - Created. Sanjay Bhave
 *********************************************************************/
void fbe_cli_lun_hard_zero(fbe_s32_t argc, fbe_s8_t ** argv)
{
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_u32_t lun_number = FBE_LUN_ID_INVALID;
    fbe_object_id_t lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_lba_t       lun_lba = FBE_LBA_INVALID;
    fbe_block_count_t lun_blocks = 0;
    fbe_u64_t threads = 0;
    fbe_time_t start_time_for_zero;
    fbe_time_t zero_time_in_ms;
    fbe_bool_t clear_paged_metadata = FBE_FALSE;

    if (argc == 0)
    {
        /* If they are asking for help, just display help and exit.*/
        fbe_cli_printf("%s", HARD_ZERO_LUN_USAGE);
        return;
    }

    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /* If they are asking for help, just display help and exit.*/
            fbe_cli_printf("%s", HARD_ZERO_LUN_USAGE);
            return;
        }
        else if ((strcmp(*argv, "-lun") == 0))
        {
            /* Get the LUN from command line */
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("LUN number is expected\n");
                return;
            }

            lun_number = atoi(*argv);

            argc--;
            argv++;
        }
        /* Get the Enclosure number from command line */
        else if ((strcmp(*argv, "-lun_objid") == 0))
        {
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("LUN object id is expected\n");
                return;
            }

            lun_object_id = (fbe_object_id_t)fbe_cli_convert_str_to_lba(*argv);

            argc--;
            argv++;
        }
        else if ((strcmp(*argv, "-lba") == 0))
        {
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("LUN lba is expected\n");
                return;
            }

            lun_lba = fbe_cli_convert_str_to_lba(*argv);

            argc--;
            argv++;
        }
        else if ((strcmp(*argv, "-blocks") == 0))
        {
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("LUN blocks is expected\n");
                return;
            }

            lun_blocks = fbe_cli_convert_str_to_lba(*argv);

            argc--;
            argv++;
        }
        else if ((strcmp(*argv, "-threads") == 0))
        {
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Threads is expected\n");
                return;
            }

            threads = atoi(*argv);

            argc--;
            argv++;
        }
        else if ((strcmp(*argv, "-paged") == 0))
        {
            argc--;
            argv++;
            clear_paged_metadata = FBE_TRUE;
        }
        else
        {
            /* The command line parameters should be properly entered */
            fbe_cli_error("Invalid argument.\n");
            fbe_cli_printf("%s", HARD_ZERO_LUN_USAGE);
            return;
        }
    }

    if(lun_number != FBE_LUN_ID_INVALID)
    {
        status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Invalid LUN number: %d, status:0x%x\n",lun_number, status);
            return;
        }
    }
    
    start_time_for_zero = fbe_get_time();
    status = fbe_api_lun_initiate_hard_zero(lun_object_id, lun_lba, lun_blocks,
                                            threads, clear_paged_metadata);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Command faild, status:0x%x\n", status);
        return;
    }
    zero_time_in_ms = fbe_get_time() - start_time_for_zero;
    fbe_cli_printf("Command success, time cout: %lld seconds\n", zero_time_in_ms / 1000);
    

    return;
}


/*************************
 * end file fbe_cli_lib_lurg_cmds.c
 *************************/
