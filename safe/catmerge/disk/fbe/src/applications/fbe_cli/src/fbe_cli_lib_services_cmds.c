/***************************************************************************
* Copyright (C) EMC Corporation 2011 
* All rights reserved. 
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!************************************************************************
*                 @file fbe_cli_services_cmds.c
***************************************************************************
*
* @brief
*  This file contains cli functions for the Services related features in
*  FBE CLI.
*
* @ingroup fbe_cli
*
* @date
*  11/09/2010 - Created. Vinay Patki
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cli_private.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_trace.h"
#include "fbe_cli_services.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_metadata_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_system_bg_service_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_system_bg_service_interface.h"
#include "fbe/fbe_api_cluster_lock_interface.h"

/*************************************************************************
*                          fbe_cli_setverify()                           *
**************************************************************************
*                                                                        *
*  @brief                                                                *
*    fbe_cli_setverify - change/display verify parameters                *
*                                                                        *
*  @param                                                                *
*    argc     (INPUT) NUMBER OF ARGUMENTS                                *
*    argv     (INPUT) ARGUMENT LIST                                      *
*                                                                        *
*  @return                                                               *
*    None.                                                               *
*	                                                                     *
*    Usage:                                                              *
*                                                                        *
*     v -h                      - get help on usage	                     *
*		                                                                 *
*	    v <drive>               - Show Sniff Verify report for the drive *	
*	    v <drive> [options - sniff verify]	                             *
*		                                                                 *
*	    v <lun>                 - Show Background Verify report for the  *
*                                drive                                   *
*	    v <lun> [options - background verify]	                         *
*		                                                                 *
*	    v [options - general]	                                         *
*		                                                                 *
*	General:	                                                         *
*	    -rg	                                                             *
*	    -rg # <options>**	                                             *
*	    -all		                                                     *
*	    -all <options>**	                                             *
*			                                                             *
*	Options:		                                                     *
*	    For Sniff Verify		                                         *
*	        -sne <1/0> enable/disable sniff verify		                 *
*	        -cr  <1/0> clear/no  all verify reports for this LU	         *
*			                                                             *
*	    For Background Verify		                                     *
*	        -bve       start background verify	  
*	        -ro        start read only background verify (e.g. -bve -ro )*
*	        -cr  <1/0> clear/no  all verify reports for this LU	         *
*			                                                             *
*	 -rg <#>    change verify parameters on this raid group	             *
*	 -all       change verify parameters on this array	                 *
*		    	                                                         *
*	** Change operations on RG or ALL (full system) i.e 'sne', 'bve',    *
*     'cr' are yet to be implemented.	                                 *
*	*** Functionality yet to be implemented	                             *
*                                                                        *
*  @version                                                              *
*    12/07/2010 - Created. Vinay Patki                                   *
*                                                                        *
*************************************************************************/
void fbe_cli_setverify(fbe_s32_t argc, fbe_s8_t ** argv)
{
    fbe_bool_t is_sv = FALSE;
    fbe_bool_t is_bv = FALSE;

    fbe_u32_t arg_index;           /* looping argument index */
    fbe_u32_t opt_index;           /* index for optional argument */
    fbe_u32_t opt_value;           /* value for optional argument */

    fbe_u32_t             verify_num = -2;                        /* unit/group number, INVALID_UNIT is a valid number */
    FBE_TRI_STATE         sniffs_enabled = FBE_TRI_STATE_INVALID; /* enable sniff verify? */
    fbe_lun_verify_type_t do_verify = FBE_LUN_VERIFY_TYPE_NONE;   /* start a background verify? */
    fbe_bool_t            background_verify_enable = FALSE;       /* Background verify enable command */
    fbe_bool_t            clear_reports = FALSE;                  /* erase verify reports? */
    fbe_bool_t            is_group_command = FALSE;
    fbe_bool_t            get_report = TRUE;
    fbe_status_t          status = FBE_STATUS_INVALID;
    fbe_object_id_t       verify_obj_id = FBE_OBJECT_ID_INVALID;
    fbe_job_service_bes_t bes = {0};
    fbe_block_count_t     blocks = FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN;
    fbe_lba_t             lba = FBE_LUN_VERIFY_START_LBA_LUN_START;
    

    if (argc == 0)
    {
        fbe_cli_printf("%s Not enough arguments for operating Verify Service\n", __FUNCTION__);
        fbe_cli_printf("%s", SERVICES_SETVERIFY_USAGE);
        return;
    }

    if (!strcmp(argv[0], "-h") || !strcmp(argv[0], "-help") )
    {
        fbe_cli_printf("%s", SERVICES_SETVERIFY_USAGE);
        return;
    }

    if (!strcmp(argv[0], "-cr"))
    {
        printf("Please provide lun number for clearing BV report OR a disk number (b_e_d) for clearing Sniff Verify report.\n");
        fbe_cli_printf("%s", SERVICES_SETVERIFY_USAGE);
        return;
    }

    if (!strcmp(argv[0], "-sne"))
    {
        printf("Please supply a disk number (b_e_d).\n");
        fbe_cli_printf("%s", SERVICES_SETVERIFY_USAGE);
        return;
    }

    if (!strcmp(argv[0], "-bve") ||
        !strcmp(argv[0], "-ro") ||
        !strcmp(argv[0], "-system") ||
        !strcmp(argv[0], "-error") ||
        !strcmp(argv[0], "-incomplete_wr"))
    {
        printf("Please supply a lun number.\n");
        fbe_cli_printf("%s", SERVICES_SETVERIFY_USAGE);
        return;
    }


    // TODO : Check for any bound units
    // TODO : Verify permission

    for (arg_index = 0; arg_index < argc; arg_index++ )
    {
        opt_index = arg_index + 1;
        opt_value = 0;

        if (*argv[arg_index] != '-' ||
            !strcmp(argv[arg_index], "-rg") ||
            !strcmp(argv[arg_index], "-all"))
        {
            /* This is command starting point. */
            if (arg_index > 1)
            {
                /* The previous input is a legal request, send it immediately.
                */
                fbe_cli_dispatch_verify(verify_obj_id,
                    is_group_command,
                    get_report,
                    sniffs_enabled,
                    is_sv,
                    do_verify,
                    lba,
                    blocks,
                    is_bv,
                    clear_reports,
                    FBE_TRI_STATE_INVALID);
                {
                    /* reinitialize for the next command*/
                    sniffs_enabled = FBE_TRI_STATE_INVALID;
                    do_verify = FBE_LUN_VERIFY_TYPE_NONE;
                    background_verify_enable = FALSE;
                    clear_reports = FALSE;
                    is_group_command = FALSE;
                    get_report = TRUE;
                }
            }

            /* Possible unit number */        
            if (*argv[arg_index] != '-')
            {
                /* Check to see if the option is valid, needed because the C function
                * atoi returns 0 if a non-numeric string is passed in, or if the string
                * value is 0.
                */
                if (fbecli_validate_atoi_args(argv[arg_index]))
                {
                    verify_num = atoi(argv[0]);

                    // TODO - Check for valid lun number on this system or it is bound
                    // TODO - Check if we need to check lun ownership

                    status = fbe_api_database_lookup_lun_by_number(verify_num, &verify_obj_id);
                    if (status == FBE_STATUS_GENERIC_FAILURE)
                    {
                        fbe_cli_error("LUN %d does not exist or unable to retrieve information for this LUN.\n\n",verify_num);
                        return;
                    }

                    is_bv = TRUE;
                }
                else if (fbe_cli_convert_diskname_to_bed(argv[arg_index], &bes) == FBE_STATUS_OK)
                {

                    /*Find the object id of a fbe_api_provision_drive_get_obj_id_by_location*/
                    status = fbe_api_provision_drive_get_obj_id_by_location(bes.bus, bes.enclosure, bes.slot, &verify_obj_id);

                    if (status == FBE_STATUS_GENERIC_FAILURE)
                    {
                        fbe_cli_error("Drive %d_%d_%d does not exist or unable to retrieve information for this drive.\n\n",
                            bes.bus, bes.enclosure, bes.slot);
                        return;
                    }

                    is_sv = TRUE;
                }
                else
                {
                    /* The input parameter was not valid */
                    fbe_cli_printf("%s Invalid option %s\n", argv[0], argv[arg_index]);
                    fbe_cli_printf("%s", SERVICES_SETVERIFY_USAGE);
                    return;
                }
            }

            /* It is group verify, get the group number */
            if (!strcmp(argv[arg_index], "-rg"))
            {
                if (opt_index == argc)
                {
                    fbe_cli_printf("%s: %s requires argument value\n", argv[0], argv[arg_index] );
                    return;
                }

                if (fbecli_validate_atoi_args(argv[opt_index]))
                {
                    verify_num = atoi(argv[opt_index]);

                    /* Make sure that RG number received from command line is valid */
                    status = fbe_api_database_lookup_raid_group_by_number(verify_num, &verify_obj_id);
                    if (status == FBE_STATUS_GENERIC_FAILURE)
                    {
                        fbe_cli_error("RAID group %d does not exist or unable to retrieve information for this RAID Group.\n",verify_num);
                        return;
                    }

                    is_group_command = TRUE;
                }
                else
                {
                    fbe_cli_printf("%s: %s invalid option %s.\n", argv[0], argv[arg_index], argv[opt_index]);
                    fbe_cli_printf("%s", SERVICES_SETVERIFY_USAGE);
                    return;
                }
                arg_index++;  /* advance one for option */
            }

            /* It is system verify. */
            if (!strcmp(argv[arg_index], "-all"))
            {
                verify_num = FBE_RAID_GROUP_INVALID;
                is_group_command = TRUE;
            }
        }
        else
        {
            /*
            * NOTE: Assumes that all options except '-bve' and '-ro' have 
            * argument values associated with them
            */
            /* Only check the arguments if this is not a background verify.
            */
            get_report = FALSE;

            background_verify_enable = background_verify_enable ||
                                        (!strcmp(argv[arg_index], "-bve") ||
                                        !strcmp(argv[arg_index],"-ro")||
                                        !strcmp(argv[arg_index],"-system")||
                                        !strcmp(argv[arg_index],"-incomplete_wr")||
                                        !strcmp(argv[arg_index],"-error") );

            /* The check for the switch which does have corresponding value 
            * will be processed here. Example: '-bvt <#>'.
            */
            if (!background_verify_enable)
            {
                if (opt_index == argc)
                {
                    fbe_cli_printf("%s: %s requires argument value\n",
                        argv[0], argv[arg_index]);
                    return;
                }
                /* 
                * Check to see if the option is valid, needed because the C function
                * atoi returns 0 if a non-numeric string is passed in, or if the string
                * value is 0.
                */
                else if (fbecli_validate_atoi_args(argv[opt_index]))
                {
                    opt_value = atoi(argv[opt_index]);
                }
                else
                    /* The user entered a value that cannot be converted by atoi */
                {
                    fbe_cli_printf("%s %s Invalid option %s\n", argv[0], argv[arg_index],
                        argv[opt_index]);
                    fbe_cli_printf("%s", SERVICES_SETVERIFY_USAGE);
                    return;
                }
            }

            if (!strcmp(argv[arg_index], "-sne"))
            {
                if (strcmp(argv[opt_index], "1") && strcmp(argv[opt_index], "0"))
                {
                    fbe_cli_printf("%s: Sniff enable must be followed by 0 (zero) or 1 (one)\n",
                        argv[0]);
                    return;
                }

                /* TODO - Check if we need to check if user is permitted to control Sniff*/
                /*
                if (CM_IS_USER_SNIFF_CONTROL_DISABLED())
                {
                printf("%s: Invalid operation while user control of sniff verifies is disabled\n",
                argv[0]);
                return;
                }
                */
                sniffs_enabled = (opt_value == 1) ? TRUE : FALSE;
                arg_index++;
            }
            else if (!strcmp(argv[arg_index], "-bve"))
            {
                do_verify = FBE_LUN_VERIFY_TYPE_USER_RW;
            }
            else if (!strcmp(argv[arg_index], "-ro"))
            {
                do_verify = FBE_LUN_VERIFY_TYPE_USER_RO;
            }
            else if (!strcmp(argv[arg_index], "-system"))
            {
                do_verify = FBE_LUN_VERIFY_TYPE_SYSTEM;
            }
            else if (!strcmp(argv[arg_index], "-error"))
            {
                do_verify = FBE_LUN_VERIFY_TYPE_ERROR;
            }
            else if (!strcmp(argv[arg_index], "-incomplete_wr"))
            {
                do_verify = FBE_LUN_VERIFY_TYPE_INCOMPLETE_WRITE;
            }
            else if (!strcmp(argv[arg_index], "-lba") || !strcmp(argv[arg_index], "-blocks"))
            {
                if (opt_index == argc)
                {
                    fbe_cli_printf("%s requires argument value\n", argv[arg_index] );
                    return;
                }

                if (!strcmp(argv[arg_index], "-lba"))
                {
                    lba = fbe_atoh(argv[opt_index]);
                    arg_index++;
                }
                else
                {
                    blocks = (fbe_u32_t)strtoul(argv[opt_index], 0, 0);
                    arg_index++;
                }
            }
            else if (!strcmp(argv[arg_index], "-cr"))
            {
                if (strcmp(argv[opt_index], "1") && strcmp(argv[opt_index], "0"))
                {
                    fbe_cli_printf("%s: Clear reports must be followed by 0 (zero) or 1 (one)\n",
                        argv[0]);
                    return;
                }
                clear_reports = (opt_value == 1) ? TRUE : FALSE;
                arg_index++;
            }

            /* Read Only Background Verify("-ro")is sub-type of 
            * Read-Write (Normal)Background Verify("-bve").
            */
            else if (!strcmp(argv[arg_index],"-ro"))
            {
                /* FBE_BVT_READ_WRITE should be set as 
                * "-ro" follows "-bve".
                */
                if(do_verify == FBE_LUN_VERIFY_TYPE_USER_RW)
                {
                    do_verify = FBE_LUN_VERIFY_TYPE_USER_RO;
                }
                else
                {
                    fbe_cli_printf("%s: '-ro' option must follow '-bve' option\n",
                        argv[0]);
                    return;
                }
            }
            else
            {
                /* Unrecognized option */
                fbe_cli_printf("%s %s: Unrecognized option %s\n", __FUNCTION__, argv[0], argv[arg_index]);
                //                fcli_print_usage(argv[0], usage_p);
                fbe_cli_printf("%s", SERVICES_SETVERIFY_USAGE);
                return;
            }
        }

    }/* endfor, all args */


    /* Send the last command off to the verify process */
    fbe_cli_dispatch_verify(verify_obj_id,
        is_group_command,
        get_report,
        sniffs_enabled,
        is_sv,
        do_verify,
        lba,
        blocks,
        is_bv,
        clear_reports,
        FBE_TRI_STATE_INVALID);

    return;
}
/*****************************************
* End of fbe_cli_setverify ()          *
*****************************************/

/************************************************************************
*                          fbe_cli_dispatch_verify ()                   *
*************************************************************************
*
*  @brief
*    fbe_cli_dispatch_verify - send the right verify request.
*
*  @param
*      verify_obj_id       - Object id
*      is_group            - True if its a group command
*      get_report          - True for getting the report
*      sniffs_enabled      - SV states
*      is_sv               - True if its SV operation
*      do_verify           - BV states
*      is_bv               - True if its BV operation
*      clear_reports       - True for clearing report
*      user_sniff_control_enabled - User sniff control
*
*  @return
*    None
*
*
*  @version
*    12/07/2010 - Created. Vinay Patki
*
*************************************************************************/
void fbe_cli_dispatch_verify(fbe_object_id_t verify_obj_id,
                             fbe_bool_t is_group,
                             fbe_bool_t get_report,
                             FBE_TRI_STATE sniffs_enabled,
                             fbe_bool_t is_sv,
                             fbe_lun_verify_type_t do_verify,
                             fbe_lba_t lba,
                             fbe_block_count_t blocks,
                             fbe_bool_t is_bv,
                             fbe_bool_t clear_reports,
                             FBE_TRI_STATE user_sniff_control_enabled)
{
    fbe_u32_t verify_num = INVALID_RAID_GROUP_ID;
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_cli_lurg_lun_details_t lun_details;
    fbe_lun_verify_report_t lun_verify_report_p;
    fbe_bool_t is_verify_full_lun = FBE_FALSE;
    fbe_api_lun_get_lun_info_t lun_info;

    if (!is_group)
    {
        if(get_report == TRUE)
        {
            if(is_sv == TRUE)
            {

                //  "v <b_e_d>"
                /*Print Sniff Verify report of a drive*/
                fbe_cli_print_sniff_verify_report(verify_obj_id);
                return;
            }
            else if(is_bv == TRUE)
            {

                //  "v <lun>"
                /*Find the lun number by object id*/
                status =  fbe_cli_get_lun_details(verify_obj_id,&lun_details);
                /* return error from here since we failed to get the details of valid LUN object */
                if (status == FBE_STATUS_GENERIC_FAILURE)
                {
                    return;
                }

                /*Get the BV report of a lun for printing*/
                status = fbe_api_lun_get_verify_report(verify_obj_id, FBE_PACKET_FLAG_NO_ATTRIB,
                    &lun_verify_report_p);

                /* verify the status. */
                if (status == FBE_STATUS_GENERIC_FAILURE)
                {
                    fbe_cli_error("\nFailed to get BV report of lun %d\n", lun_details.lun_number);
                    return;
                }

                /*Print BV report of an individual lun*/
                fbe_cli_print_bg_verify_report(lun_details.lun_number,&lun_verify_report_p);
                return;
            }
        }

        if(do_verify!= FBE_LUN_VERIFY_TYPE_NONE)
        {
            is_verify_full_lun = (blocks == FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN && lba == FBE_LUN_VERIFY_START_LBA_LUN_START) ?
                                 FBE_TRUE : FBE_FALSE;
            if (is_verify_full_lun == FBE_FALSE && blocks == FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN)
            {
                status = fbe_api_lun_get_lun_info(verify_obj_id/*lun obj id*/, &lun_info); 
                if (status != FBE_STATUS_OK) 
                {
                    fbe_cli_error("\nCould not get lun info. Failed to initiate background verify of a lun\n");
                    return;
                }
                blocks = lun_info.capacity - lba;
            }
            
            status = fbe_api_lun_initiate_verify(verify_obj_id, 
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 do_verify,
                                                 is_verify_full_lun, /* Verify the entire lun*/ 
                                                 lba,
                                                 blocks); 

            /* verify the status. */
            if (status == FBE_STATUS_GENERIC_FAILURE)
            {
                fbe_cli_error("\nFailed to initiate background verify of a lun\n");
                return;
            }

            fbe_cli_printf("\nBackground Verify enable - Success! %d\n", status);
            return;
        }

        if(sniffs_enabled != FBE_TRI_STATE_INVALID)
        {
            /* "v <b_e_d> -sne 1/0"*/
            /*Configure sniff verify for a drive*/
            fbe_cli_set_sniff_verify_service(verify_obj_id, sniffs_enabled);
            return;
        }

        if(clear_reports == TRUE)
        {
            if(is_sv == TRUE)
            {

                //  "v <b_e_d> -cr 1/0"
                /*Clear a SV report of a drive*/
                status = fbe_api_provision_drive_clear_verify_report(verify_obj_id, FBE_PACKET_FLAG_NO_ATTRIB );

                /* verify the status. */
                if (status == FBE_STATUS_GENERIC_FAILURE)
                {
                    fbe_cli_printf("\nFailed while clearing the background verify report of a lun\n");
                    return;
                }

                fbe_cli_printf("\nClearing Sniff verify report - Success! %d\n", status);
                return;
            }
            else if(is_bv == TRUE)
            {
                /*  "v <lun> -cr 1/0"*/
                /*Clear the BV report of a lun*/
                status = fbe_api_lun_clear_verify_report(verify_obj_id, FBE_PACKET_FLAG_NO_ATTRIB);

                /* verify the status. */
                if (status == FBE_STATUS_GENERIC_FAILURE)
                {
                    fbe_cli_printf("\nFailed while clearing the background verify report of a lun\n");
                    return;
                }

                fbe_cli_printf("\nClearing Background verify report - Success! %d\n", status);
                return;
            }
        }
    }
    else
    {
        if (get_report)
        {
            /*This is a request to print BV report for entire group. If obj_id is invalid, it is for ALL groups/luns.*/
            if (verify_obj_id == FBE_OBJECT_ID_INVALID)
            {
                fbe_u32_t index;
                fbe_object_id_t rg_obj_id;

                /*Print background verify report for all raid group, one at a time*/
                for (index = 0; index < MAX_RAID_GROUP_ID; index++)
                {
                    if (fbe_api_database_lookup_raid_group_by_number(index, &rg_obj_id) == FBE_STATUS_OK)
                    {
                        /*Print the background verify report for RG*/
                        fbe_cli_print_group_verify_report(rg_obj_id);
                    }
                }
            }
            else
            {
                //  "v -rg"
                /*Request to print the background verify report for all the luns of given RG*/
                fbe_cli_print_group_verify_report(verify_obj_id);
            }
        }
        else
        {
            if (verify_obj_id != FBE_OBJECT_ID_INVALID)
            {
                /*Get the RG number from Object ID*/
                status = fbe_api_database_lookup_raid_group_by_object_id(verify_obj_id, &verify_num);

                /* verify the status. */
                if (status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("\nFailed to get the raid group number by object id\n");
                    return;
                }

                if (verify_num >= MAX_RAID_GROUP_ID)
                {
                    fbe_cli_error("%s: %d invalid raid group number.\n", "setverify", verify_num);
                    return;
                }
            }

            // "v -rg -sne 1"
            // "v -all -sne 1"
            /*This sends the group related command for processing*/
            fbe_cli_send_group_verify_cmd(verify_num,
                sniffs_enabled,
                do_verify,
                clear_reports,
                user_sniff_control_enabled);
        }
    }

    return;
}
/*****************************************
* End of fbe_cli_setverify ()          *
*****************************************/
/**************************************************************************
*                     fbe_cli_send_group_verify_cmd ()
***************************************************************************
*
*  @brief
*    fbe_cli_send_group_verify_cmd - Processes the request to configure SV
*                                    or BV for a group or all luns/drives.
*
*  @param
*    group                      - Raid group
*    sniffs_enabled             - Sniff states
*    do_verify                  - BV states
*    clear_reports              - For clearing the report
*    user_sniff_control_enabled - User control status
*
*
*  @return
*    None
*
*  @version
*    12/07/2010 - Created. Vinay Patki
*
***************************************************************************/
void fbe_cli_send_group_verify_cmd(fbe_u32_t group,
                                   FBE_TRI_STATE sniffs_enabled,
                                   fbe_lun_verify_type_t do_verify,
                                   fbe_bool_t clear_reports,
                                   FBE_TRI_STATE user_sniff_control_enabled)
{
    fbe_status_t              status;
    fbe_object_id_t           obj_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                 index;
    fbe_cli_lurg_rg_details_t                    rg_details;
    fbe_class_id_t                               class_id;

    /* Check if command is for a specific RG or for all RGs*/
    if (group != INVALID_RAID_GROUP_ID)
    {
        /*As the command is for a RG, get details of that Raid group.*/

        /* We want downstream as well as upstream objects for this RG indicate 
        * that by setting corresponding variable in structure
        */
        rg_details.direction_to_fetch_object = DIRECTION_BOTH;

        /* Get the object id of RG */
        status = fbe_api_database_lookup_raid_group_by_number(group, &obj_id);
        if (status == FBE_STATUS_GENERIC_FAILURE)
        {
            fbe_cli_error("RAID group %d does not exist or unable to retrieve information for this RAID Group.\n",group);
            return;
        }

        /* Get the details of RG */
        status = fbe_cli_get_rg_details(obj_id, &rg_details);
        /* return failure from here since we're not able to know objects related to this */
        if (status == FBE_STATUS_GENERIC_FAILURE)
        {
            fbe_cli_error("Unable to get details of this RAID Group %d.\n",group);
            return;
        }

        /*for Sniff Verify of all drives of a RG*/
        if(sniffs_enabled != FBE_TRI_STATE_INVALID)
        {
            /*Get objects related to a RG*/
            for (index = 0; index < rg_details.downstream_pvd_object_list.number_of_downstream_objects; index++)
            {
                /*Set the SV for a drive*/
                fbe_cli_set_sniff_verify_service(rg_details.downstream_pvd_object_list.downstream_object_list[index], sniffs_enabled);
            }

            fbe_cli_printf("\nConfiguring Sniff Verify for all drives of RG %d - Success! %d\n", group, status);
            return;
        }
        /*for Background Verify of all luns of a RG*/
        else if(do_verify != FBE_LUN_VERIFY_TYPE_NONE)
        {

            /*Get objects related to a RG*/
            for (index = 0; index < rg_details.upstream_object_list.number_of_upstream_objects; index++)
            {
                /* Find out if given obj ID is for RAID group or LUN */
                status = fbe_api_get_object_class_id(rg_details.upstream_object_list.upstream_object_list[index], &class_id, FBE_PACKAGE_ID_SEP_0);
                /* if status is generic failure then continue to next object */
                if (status == FBE_STATUS_GENERIC_FAILURE)
                {
                    continue;
                }

                /*Initiate BV on a lun*/
                status = fbe_api_lun_initiate_verify(rg_details.upstream_object_list.upstream_object_list[index], 
                                                     FBE_PACKET_FLAG_NO_ATTRIB, 
                                                     do_verify,
                                                     FBE_TRUE, /* Verify the entire lun*/ 
                                                     FBE_LUN_VERIFY_START_LBA_LUN_START,  
                                                     FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN);   

                /* verify the status. */
                if (status == FBE_STATUS_GENERIC_FAILURE)
                {
                    fbe_cli_error("\nFailed to initiate background verify of a lun. RG: %d LU id: %d Status %d\n", group,
                        rg_details.upstream_object_list.upstream_object_list[index], status);
                    return;
                }
            }
            fbe_cli_printf("\nConfiguring Background Verify on all luns of RG %d - Success! %d\n", group, status);
            return;
        }

        else if(clear_reports == FBE_TRUE)
        {
            /*FBECLI - TODO*/
            /*Processing of CR for SV and BV*/
           fbe_cli_printf("\nThis command functionality is under development.\n");
           return;
        }
        else
        {
            fbe_cli_error("\nUnknown arguments specified...\n");
        }
        return;
    }

    /*for all luns/drives on system*/
    else
    {
        /*for Sniff Verify of all drives*/
        if(sniffs_enabled != FBE_TRI_STATE_INVALID)
        {
            int i = 0;
            rg_details.direction_to_fetch_object = DIRECTION_BOTH;

            for(index = 0; index <= MAX_RAID_GROUP_ID;index++)
            {

                /* Get the object id of RG */
                status = fbe_api_database_lookup_raid_group_by_number(index, &obj_id);
                if (status == FBE_STATUS_GENERIC_FAILURE)
                {
                    continue;
                }

                /* Get the details of RG */
                status = fbe_cli_get_rg_details(obj_id, &rg_details);
                /* return failure from here since we're unable to know objects related to this */
                if (status == FBE_STATUS_GENERIC_FAILURE)
                {
                    fbe_cli_error("Unable to get details of this RAID Group %d.\n",group);
                    return;
                }

                /*Get objects related to a RG*/
                for (i = 0; i < rg_details.downstream_pvd_object_list.number_of_downstream_objects; i++)
                {
                    /*Set the SV for a drive*/
                    fbe_cli_set_sniff_verify_service(rg_details.downstream_pvd_object_list.downstream_object_list[i], sniffs_enabled);
                }

                fbe_cli_printf("\nConfiguring Sniff Verify for all drives of RG %d - Success! %d\n", index, status);

            }

            fbe_cli_printf("\nConfiguring Sniff Verify for all drives - Success! %d\n", status);
            return;
        }

        /*for Background Verify of all luns*/
        if(do_verify != FBE_LUN_VERIFY_TYPE_NONE)
        {
            status = fbe_api_lun_initiate_verify_on_all_existing_luns(do_verify); 

            /* verify the status. */
            if (status == FBE_STATUS_GENERIC_FAILURE)
            {
                fbe_cli_error("\nFailed to initiate background verify of a lun. Status %d\n", status);
                return;
            }

            fbe_cli_printf("\nBackground Verify enable - Success! %d\n", status);
            return;
        }
        else if(clear_reports == FBE_TRUE)
        {
            /*FBECLI - TODO*/
            /*Processing of CR for SV and BV*/
           fbe_cli_printf("\nThis command functionality is under development.\n");
           return;
        }
        else
        {
            fbe_cli_error("\nUnknown arguments specified...\n");
        }
        return;
    }
}
/*****************************************
* End of fbe_cli_send_group_verify_cmd ()*
*****************************************/
/************************************************************************
*                fbe_cli_print_group_verify_report ()
*************************************************************************
*
*  @brief
*    fbe_cli_print_group_verify_report - Prints the BV report of a raid group.
*
*  @param
*    rg_id - Raid group number
*
*  @return
*    None
*
*  @version
*    12/07/2010 - Created. Vinay Patki
*
*************************************************************************/
void fbe_cli_print_group_verify_report(fbe_object_id_t rg_id)
{
    fbe_status_t                                 status;
    fbe_u32_t                                    index;
    fbe_cli_lurg_rg_details_t                    rg_details;
    fbe_class_id_t                               class_id;
    fbe_cli_lurg_lun_details_t                   lun_details;
    fbe_lun_verify_report_t lun_verify_report_p;

    /* We want downstream as well as upstream objects for this RG indicate 
    * that by setting corresponding variable in structure
    */
    rg_details.direction_to_fetch_object = DIRECTION_BOTH;

    /* Get the details of RG associated with this LUN */
    status = fbe_cli_get_rg_details(rg_id, &rg_details);
    /* return failure from here since we're not able to know objects related to this */
    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        return;
    }

    for (index = 0; index < rg_details.upstream_object_list.number_of_upstream_objects; index++)
    {
        /* Find out if given obj ID is for RAID group or LUN */
        status = fbe_api_get_object_class_id(rg_details.upstream_object_list.upstream_object_list[index],
            &class_id, FBE_PACKAGE_ID_SEP_0);
        /* if status is generic failure then continue to next object */
        if (status == FBE_STATUS_GENERIC_FAILURE)
        {
            continue;
        }

        /* Now that we know that the object correspond to LUN 
        * get the details of LUN
        */
        status =  fbe_cli_get_lun_details(rg_details.upstream_object_list.upstream_object_list[index],
            &lun_details);
        /* return error from here since we failed to get the details of valid LUN object */
        if (status == FBE_STATUS_GENERIC_FAILURE)
        {
            return;
        }

        status = fbe_api_lun_get_verify_report(rg_details.upstream_object_list.upstream_object_list[index],
            FBE_PACKET_FLAG_NO_ATTRIB,&lun_verify_report_p);

        /* Check the status of fetching verify report. */
        if (status == FBE_STATUS_GENERIC_FAILURE)
        {
            fbe_cli_printf("\nFailed to get the Background Verify Report\n");
            return;
        }

        fbe_cli_print_bg_verify_report(lun_details.lun_number,&lun_verify_report_p);

        printf("\nPress any key to continue...\n");
        getchar();

    }

    return;
}
/*********************************************
* End of fbe_cli_print_group_verify_report ()*
**********************************************/
/**************************************************************************
*                      fbe_cli_print_sniff_verify_report ()               *
***************************************************************************
*                                                                         *
*  @brief
*    fbe_cli_print_sniff_verify_report - Prints Sniff Verify report of
*                                        a drive.
*
*  @param
*    verify_obj_id - object id of a drive
*
*  @return
*    None
*
*  @version
*    12/07/2010 - Created. Vinay Patki
*
***************************************************************************/
void fbe_cli_print_sniff_verify_report(fbe_object_id_t verify_obj_id)
{
    fbe_job_service_bes_t bes = {0};
    fbe_provision_drive_verify_report_t pvd_verify_report_p;
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_provision_drive_get_verify_status_t verify_status;
    fbe_bool_t                              system_sniff_verify_enabled;

    status = fbe_api_provision_drive_get_location(verify_obj_id, &bes.enclosure, &bes.enclosure, &bes.slot);

    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        fbe_cli_error("Drive %d_%d_%d does not exist or unable to retrieve information for this drive.\n\n",
            bes.bus, bes.enclosure, bes.slot);
        return;
    }

    status = fbe_api_provision_drive_get_verify_report(verify_obj_id, FBE_PACKET_FLAG_NO_ATTRIB,
        &pvd_verify_report_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("Unable to get Sniff Verify Report for Drive %d_%d_%d  Status %d\n\n",
            bes.bus, bes.enclosure, bes.slot, status);
        return;
    }

    fbe_cli_printf("\nSniff Verify Process Information for Drive: %d_%d_%d\n", bes.bus,bes.enclosure,bes.slot);

    status = fbe_api_provision_drive_get_verify_status(verify_obj_id,FBE_PACKET_FLAG_NO_ATTRIB, &verify_status);

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("Unable to get Sniff Verify Status for Drive %d_%d_%d.  Error status %d\n\n",
            bes.bus, bes.enclosure, bes.slot, status);
        return;
    }

    system_sniff_verify_enabled = fbe_api_system_sniff_verify_is_enabled();
    fbe_cli_printf("\nSniff Verify is %s.", (verify_status.verify_enable)? "Enabled":"Disabled");
    fbe_cli_printf("\nSystem sniff verify is %s.", (system_sniff_verify_enabled)? "Enabled":"Disabled");
    fbe_cli_printf("\nSniff Verify Checkpoint: %d", (int)verify_status.verify_checkpoint);
    fbe_cli_printf("\nSniff Verify Exported Capacity: %d\n", (int)verify_status.exported_capacity);

    // Actual prining for Sniff report of a drive is here.
    fbe_cli_printf("\nSniff Verify Pass count:%d", pvd_verify_report_p.pass_count);
    fbe_cli_printf("\nRevision Number:%d\n", pvd_verify_report_p.revision);

    fbe_cli_printf("\n Report                |  Recoverable  | Unrecoverable |\n");
    fbe_cli_printf("--------------------------------------------------------");
    fbe_cli_printf("\n Read errors - Total   |%14d |%14d", pvd_verify_report_p.totals.recov_read_count,
        pvd_verify_report_p.totals.unrecov_read_count);
    fbe_cli_printf("\n Read errors - Current |%14d |%14d", pvd_verify_report_p.current.recov_read_count,
        pvd_verify_report_p.current.unrecov_read_count);
    fbe_cli_printf("\n Read errors - Previous|%14d |%14d\n\n", pvd_verify_report_p.previous.recov_read_count,
        pvd_verify_report_p.previous.unrecov_read_count);
    return;
}
/*********************************************
* End of fbe_cli_print_sniff_verify_report ()*
**********************************************/
/**************************************************************************
*                      fbe_cli_print_bg_verify_report ()                  *
***************************************************************************
*                                                                         *
*  @brief
*    fbe_cli_print_bg_verify_report - Print the BV report for a lun.
*
*  @param
*    lun_num             - Lun number
*    lun_verify_report_p - BV report of lun
*
*  @return
*    None
*
*  @version
*    12/07/2010 - Created. Vinay Patki
*
***************************************************************************/

void fbe_cli_print_bg_verify_report(fbe_lun_number_t lun_num,fbe_lun_verify_report_t *lun_verify_report_p)
{

    fbe_cli_printf("\n\nBackground Verify Process Information for Lun: %d\n", lun_num);

    fbe_cli_printf("\nPass count: %d",lun_verify_report_p->pass_count);
    fbe_cli_printf("\nRevision: %d",lun_verify_report_p->revision);

    // Actual prining for BV report of a lun is here.
    fbe_cli_printf("\n\n[1] Total Report\n");
    fbe_cli_print_bg_verify_counts(&lun_verify_report_p->totals);
    fbe_cli_printf("\n\n[2] Current Report\n");
    fbe_cli_print_bg_verify_counts(&lun_verify_report_p->current);
    fbe_cli_printf("\n\n[3] Previous Report\n");
    fbe_cli_print_bg_verify_counts(&lun_verify_report_p->previous);
    return;
}
/*****************************************
* End of fbe_cli_print_bg_verify_report ()          *
*****************************************/
/**************************************************************************
*                      fbe_cli_print_bg_verify_counts ()                  *
***************************************************************************
*                                                                         *
*  @brief
*    fbe_cli_print_bg_verify_counts - Prints the BV counts which is part
*                                     of printing BV report
*
*  @param
*    lun_verify_counts_p - Pointer to counts of SV report
*
*  @return
*    None
*
*  @version
*    12/07/2010 - Created. Vinay Patki
*
***************************************************************************/

void fbe_cli_print_bg_verify_counts(fbe_lun_verify_counts_t *lun_verify_counts_p)
{

    /*Print the background verify counts*/
    fbe_cli_printf("                       |  Correctable  | Uncorrectable |\n");
    fbe_cli_printf("-------------------------------------------------------\n");
    fbe_cli_printf(" Single bit CRC errors |%14d |%14d \n",lun_verify_counts_p->correctable_single_bit_crc,
        lun_verify_counts_p->uncorrectable_single_bit_crc);
    fbe_cli_printf("  Multi bit CRC errors |%14d |%14d \n",lun_verify_counts_p->correctable_multi_bit_crc,
        lun_verify_counts_p->uncorrectable_multi_bit_crc);
    fbe_cli_printf("    Write Stamp errors |%14d |%14d \n",lun_verify_counts_p->correctable_write_stamp,
        lun_verify_counts_p->uncorrectable_write_stamp);
    fbe_cli_printf("     Time stamp errors |%14d |%14d \n",lun_verify_counts_p->correctable_time_stamp,
        lun_verify_counts_p->uncorrectable_time_stamp);
    fbe_cli_printf("     Shed stamp errors |%14d |%14d \n",lun_verify_counts_p->correctable_shed_stamp,
        lun_verify_counts_p->uncorrectable_shed_stamp);
    fbe_cli_printf("      LBA stamp errors |%14d |%14d \n",lun_verify_counts_p->correctable_lba_stamp,
        lun_verify_counts_p->uncorrectable_lba_stamp);
    fbe_cli_printf("      Coherency errors |%14d |%14d \n",lun_verify_counts_p->correctable_coherency,
        lun_verify_counts_p->uncorrectable_coherency);
    fbe_cli_printf("          Media errors |%14d |%14d \n",lun_verify_counts_p->correctable_media,
        lun_verify_counts_p->uncorrectable_media);
    fbe_cli_printf("     Soft media errors |%14d |             -\n\n",lun_verify_counts_p->correctable_soft_media);

    return;
}
/******************************************
* End of fbe_cli_print_bg_verify_counts ()*
*******************************************/
/**************************************************************************
*                      fbe_cli_set_sniff_verify_report ()                 *
***************************************************************************
*
*  @brief
*    fbe_cli_set_sniff_verify_report - Configure the sniff verify for a
*    drive
*
*  @param
*    verify_obj_id  - object id of drive
*    sniffs_enabled - TRUE; if SV is to be enabled
*                     FALSE for disabling it
*
*  @return
*    None
*
*  @version
*    12/07/2010 - Created. Vinay Patki
*....02/02/2012 - Modified Shubhada Savdekar
*
*  @Notes
*    
***************************************************************************/
void fbe_cli_set_sniff_verify_service(fbe_object_id_t verify_obj_id, 
    FBE_TRI_STATE sniffs_enabled)
{

    fbe_status_t                                                status = FBE_STATUS_INVALID;
    fbe_status_t                                                job_status = FBE_STATUS_INVALID;
    fbe_job_service_error_type_t                                js_error_type;

    if((sniffs_enabled != FALSE) && (sniffs_enabled != TRUE))
    {
        fbe_cli_error("\nIncorrect value. Should be either 0 or 1\n");
        return;
    }
    /*call The API*/
    status = fbe_api_job_service_set_pvd_sniff_verify(verify_obj_id, &job_status, &js_error_type, (fbe_bool_t)sniffs_enabled);
    
    /* verify the status. */
    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        fbe_cli_error("\nFailed while updating the provision drive sniff verify status\n");
        return;
    }

    if (status == FBE_STATUS_TIMEOUT)
       {
        fbe_cli_error("\nTimed out enabling sniff verify\n");
        return;
    }
    /* verify the job status. */
    if(js_error_type != FBE_JOB_SERVICE_ERROR_NO_ERROR)
    {
        fbe_cli_error("\nConfiguring PVD to enable sniff verify failed\n");
        return;
    }

    if(sniffs_enabled)
    {
        fbe_cli_printf("\nConfiguring PVD to enable sniff verify successful...\n");
    }
    else
    {
        fbe_cli_printf("\nConfiguring PVD to disable sniff verify successful...\n");
    }

    return;
}
/*****************************************
* End of fbe_cli_set_sniff_verify_report ()          *
*****************************************/

/**************************************************************************
*                      fbe_cli_config_invalidate_db ()                 *
***************************************************************************
*
*  @brief
*    fbe_cli_config_invalidate_db - sets the master record of configuration
*    service to invalid
*
*  @param
*
*  @return
*    None
*
*  @version
*    12/07/2010 - Created. Vinay Patki
*
***************************************************************************/
void fbe_cli_config_invalidate_db(fbe_s32_t argc, fbe_s8_t ** argv)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

    if (argc > 0)
    {
        fbe_cli_error("%s too much arguments to config_invalidate_db\n", __FUNCTION__);
        fbe_cli_printf("%s", SERVICES_CONFIG_INVALIDATE_DB_USAGE);
        return;
    }

    //status = fbe_api_configuration_set_master_record(0,0,0,0);
    if (status == FBE_STATUS_OK)
    {
        fbe_cli_printf("\n Configutation service:  Invalidate DB succeeded...\n");
    }
    else
    {
        fbe_cli_printf("\n Configutation service: Invalidate DB failed... Error: %d\n", status);
    }


    return;
}
/********************************************
* End of fbe_cli_config_invalidate_db ()*
*********************************************/

/**************************************************************************
*                      fbe_cli_config_emv ()                 *
***************************************************************************
*
*  @brief
*    fbe_cli_config_emv - Get/Set EMV value in configuration service
	
*  @param    argc - argument count
*  @param    argv - argument string
*
*
*  @return
*    None
***************************************************************************/
void fbe_cli_config_emv(fbe_s32_t argc, fbe_s8_t ** argv)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_emv_t  shared_emv_info = {0};

     /*  define a semv_info union to get seperate part of the Shared Expected Memory info data.
     *  Shared Expected Memory info is a 64-bits data, whose high 32 bits represent CTM while
     *  the low 32 bits represent SEMV value*/
    union 
    {
        fbe_u64_t      semv_info;
        struct
        {
            fbe_u32_t     semv;    //  Shared Expected Memory Value
            fbe_u32_t     ctm;     //  Conversion Target Memory
        } semv_info_data;
    } semv_info_u;

    if ((argc == 1)&& (strcmp(argv[0],"-get") == 0))
    {
        status = fbe_api_database_get_emv_params(&shared_emv_info);
        if ((status == FBE_STATUS_OK))
        {
            semv_info_u.semv_info = shared_emv_info.shared_expected_memory_info;
            fbe_cli_printf("\nshared emv value: 0x%x GB\nctm: 0x%x GB\n",
                                            semv_info_u.semv_info_data.semv,
                                            semv_info_u.semv_info_data.ctm);            
        }
        else
        {
            fbe_cli_printf("\nFailed to GET emv value ... Error: %d\n", status);            
        }
    }
    else if ((argc == 3)&& (strcmp(argv[0],"-set")==0))
    {
        semv_info_u.semv_info_data.semv = atoi(argv[1]);
        semv_info_u.semv_info_data.ctm = atoi(argv[2]);
        shared_emv_info.shared_expected_memory_info = semv_info_u.semv_info;
        
        status = fbe_api_database_set_emv_params(&shared_emv_info);
        if (status != FBE_STATUS_OK)    
        {
            fbe_cli_printf("\nFailed to set shared emv info ... Error: %d\n", status);
            return;            
        }

    }
    else
    {
        /* The input parameter was not valid */        
        fbe_cli_printf("%s",SERVICES_CONFIG_EMV_USAGE);       
    }
    return;
}
/********************************************
* End of fbe_cli_config_emv ()*
*********************************************/

/************************************************************************
*                          fbe_cli_bgo_enable ()                   *
*************************************************************************
*
*  @brief
*  Entry function to halt command
*
 *  @param    argc - argument count
 *  @param    argv - argument string
*
*
*  @return
*    None
*
*
*  @version
*    09/08/2011 - Created. Vishnu Sharma
*
*************************************************************************/
void fbe_cli_bgo_enable(fbe_s32_t argc, fbe_s8_t ** argv)
{
    // process bgo_halt fbe-cli command
    fbe_cli_bgo_enable_disable_common(argc, argv, FBE_BGO_ENABLE_CMD);

    return;
}
/*****************************************
* End of fbe_cli_bgo_enable ()          *
*****************************************/

/************************************************************************
*                          fbe_cli_bgo_disable ()                   *
*************************************************************************
*
*  @brief
*  Entry function to unhalt command
*
 *  @param    argc - argument count
 *  @param    argv - argument string
*
*
*  @return
*    None
*
*
*  @version
*    09/08/2011 - Created. Vishnu Sharma
*
*************************************************************************/
void fbe_cli_bgo_disable(fbe_s32_t argc, fbe_s8_t ** argv)
{
    // process bgo_unhalt fbe-cli command
    fbe_cli_bgo_enable_disable_common(argc, argv, FBE_BGO_DISABLE_CMD);

    return;
}
/*****************************************
* End of fbe_cli_bgo_disable ()          *
*****************************************/

/************************************************************************
*                          fbe_cli_bgo_status ()                   *
*************************************************************************
*
*  @brief
*  Entry function to unhalt command
*
 *  @param    argc - argument count
 *  @param    argv - argument string
*
*
*  @return
*    None
*
*
*  @version
*    09/08/2011 - Created. Vishnu Sharma
*
*************************************************************************/
void fbe_cli_bgo_status(fbe_s32_t argc, fbe_s8_t ** argv)
{
    // process bgo_status fbe-cli command
    fbe_cli_bgo_enable_disable_common(argc, argv, FBE_BGO_STATUS_CMD);

    return;
}
/*****************************************
* End of fbe_cli_bgo_status ()          *
*****************************************/


/************************************************************************
*                          fbe_cli_bgo_enable_disable_common ()            *
*************************************************************************
*
*  @brief
*  This function is common to both enable/disable entry functions and does
*  enabling/disabling of background operations. Arguments are passed to 
*  this function and this
*  function identifies the object id  and execures the operation.
*
*  @param    argc - argument count
*  @param    argv - argument string
*  @param    fbe_bgs_halt_unhalt_cmd_t - Halt/Unhalt command
*
*  @return
*    None
*
*
*  @version
*    09/08/2011 - Created. Vishnu Sharma
*
*************************************************************************/
void fbe_cli_bgo_enable_disable_common(fbe_s32_t argc, fbe_s8_t ** argv, fbe_bgo_enable_disable_cmd_t process_halt_unhalt)
{
    fbe_u32_t              operation_code = 0;
    fbe_object_id_t        obj_id = FBE_OBJECT_ID_INVALID;
    fbe_job_service_bes_t  disk_location = {0};    // for storing disk location given by user
    fbe_status_t           status = FBE_STATUS_INVALID;
    fbe_u8_t *             usages_string = NULL;
    fbe_u8_t *             operation_code_string = NULL;
    fbe_u32_t              rg = FBE_UNSIGNED_MINUS_1;                 
    
    usages_string = fbe_cli_lib_bgo_get_command_usages(process_halt_unhalt);

    if (argc == 0)
    {
        fbe_cli_printf("ERROR: Too few args.\n");
        fbe_cli_printf("\n%s",usages_string);
        return;
    }

    if (!strcmp(argv[0], "-h") || !strcmp(argv[0], "-help") )
    {
        fbe_cli_printf("Get information about bgo.\n");
        fbe_cli_printf("\n%s",usages_string);
        return;
    }

    /* For all background operations*/
    if (!strcmp(argv[0], "-all") )
    {
        fbe_cli_lib_bgo_enable_disable_all(process_halt_unhalt,usages_string);
        return;
    }
    if (!strcmp(argv[0], "-get_bgo_flags") )
    {
        fbe_cli_print_bgo_flags();
        return;
    }
    
    // check that the user has specified enough cmd-line arguments
    if (argc != 4)
    {
        fbe_cli_printf("ERROR: Too few args.\n");
        fbe_cli_printf("\n%s",usages_string);
        return;
    }
    
    while( argc > 0 )
    {

        // if specified, process disk notification argument
        if ( !strcmp(*argv, "-disk"))
        {
            argc--;
            

            // check for presence of disk data
            if ( argc == 0 )
            {
                fbe_cli_printf( "%s: option requires a value\n",
                             *argv);
                fbe_cli_printf("\n%s",usages_string);
                return;
            }
            argv++;
            status = fbe_cli_convert_diskname_to_bed(*argv, &disk_location);

            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("\t Specified disk number %s is invalid.\n",*argv);
                fbe_cli_printf("\n%s",usages_string);
                return;
            }

            status = fbe_api_provision_drive_get_obj_id_by_location(disk_location.bus,
                disk_location.enclosure, disk_location.slot, &obj_id);

            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s:\tUnable to get obj id of drive %d_%d_%d. %d\n", __FUNCTION__, 
                    disk_location.bus, disk_location.enclosure, disk_location.slot, status);
                fbe_cli_printf("\n%s",usages_string);
                return;
            }
         
        }
        // if specified, process RAID Group notification
        else if ( !strcmp(*argv, "-rg"))
        {
            argc--;
 
            // check for presence of RG notification data
            if ( 0 == argc )
            {
                fbe_cli_printf( "%s: option requires a value\n", *argv);
                fbe_cli_printf("\n%s",usages_string);
                return;
            }

            argv++;
            // check for valid RG notification data
            if ( !fbecli_validate_atoi_args(*argv) )
            {
                fbe_cli_printf( "%s: value invalid for switch %s\n",
                             *argv,*(argv-1));
                fbe_cli_printf("\n%s",usages_string);
                return;
            }

            // convert and store selected RG
            rg = atoi( *argv );

            /*Get the obj id of lun. This can confirm whether this RG is valid.*/
            status = fbe_api_database_lookup_raid_group_by_number(rg,&obj_id);

            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s:\tUnable to get obj id of RG %s. %d\n", __FUNCTION__, *argv, status);
                fbe_cli_printf("\n%s",usages_string);
                return;
            }
           
        }

        // if specified, process SP notification
        else if ( !strcmp(*argv, "-operation"))
        {
            
            argc--;
            // check for presence of RG notification data
            if ( 0 == argc )
            {
                fbe_cli_printf( "%s: option requires a value\n",
                             *argv);
                fbe_cli_printf("\n%s",usages_string);
                return;
            }

            argv++;
            operation_code = fbe_cli_lib_bgo_get_operation_code(*argv);

            if(operation_code == 0)
            {
                fbe_cli_error("Invalid Operation code. \n");
                fbe_cli_printf("\n%s",usages_string);
                return;
            } 
            operation_code_string = *argv;
        }

        argc--;
        argv++;
    }
    /* switch is not correct Like opera,operatio,op,disks,rgs etc.*/ 
    if(operation_code == 0 || obj_id == FBE_OBJECT_ID_INVALID )
    {
        fbe_cli_error("\n Please check the command. \n");
        fbe_cli_printf("\n%s",usages_string);
        return;
    }
    /*Validate operation string corresponding to rg or disk */
    if(fbe_cli_lib_validate_operation(operation_code_string,rg))
    {
        fbe_cli_lib_bgo_enable_disable(process_halt_unhalt,obj_id,operation_code,operation_code_string);
    }
    else
    {
        fbe_cli_error("\n Please provide correct operation corresponding to rg or disk.\n");
        fbe_cli_printf("\n%s",usages_string);
    }

    return;
}
/*****************************************
* End of fbe_cli_bgo_enable_disable_common ()          *
*****************************************/

/************************************************************************
*        fbe_cli_lib_validate_operation ()                   *
*************************************************************************
*
*  @brief
*  To validate operation corresponding  to rg or disk.
*
*  @param    argv                - Operation code string.
*  @param    rg                  - rg number
*
*  @return
*    FBE_TRUE if operation is correct else FBE_FALSE.
*
*
*  @version
*    09/08/2011 - Created. Vishnu Sharma
*
*************************************************************************/

fbe_bool_t fbe_cli_lib_validate_operation(fbe_u8_t* operation_code_string,fbe_u32_t rg)
{
    fbe_bool_t flag = FBE_FALSE;
    
    if(rg != FBE_UNSIGNED_MINUS_1)
    {
        if(strncmp(operation_code_string, "RG",2)==0)
        {
            flag = FBE_TRUE;
        }
        else
        {
            flag = FBE_FALSE;
            fbe_cli_error("\n We can not perform this operation on RG.\n");
        }
    }
    else
    {
        if(strncmp(operation_code_string, "PVD",3)==0)
        {
            flag =  FBE_TRUE;
        }    
        else
        {
            flag = FBE_FALSE;
            fbe_cli_error("\n We can not perform this operation on disk(or PVD).\n");
        }
    }
    return flag;
}

/*****************************************
* End of fbe_cli_lib_validate_operation ()          *
*****************************************/


/************************************************************************
*        fbe_cli_lib_bgo_get_operation_code ()                   *
*************************************************************************
*
*  @brief
*  To get the operation code string from operation code.
*
*  @param    argv                - Operation code string.
*  
*
*  @return
*    operation code.
*
*
*  @version
*    09/08/2011 - Created. Vishnu Sharma
*
*************************************************************************/

fbe_u32_t fbe_cli_lib_bgo_get_operation_code(fbe_s8_t * argv)
{
    fbe_u32_t operation_code = 0;

    if(strcmp(argv, "PVD_DZ")==0)
    {
        operation_code = FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING;
    }
    else if(strcmp(argv, "PVD_SNIFF")==0)
    {
        operation_code = FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF;
    }
    else if(strcmp(argv, "PVD_ALL")==0)
    {
        operation_code = FBE_PROVISION_DRIVE_BACKGROUND_OP_ALL;
    }
    else if(strcmp(argv, "RG_MET_REB")==0)
    {
        operation_code = FBE_RAID_GROUP_BACKGROUND_OP_METADATA_REBUILD;
    }
    else if(strcmp(argv, "RG_REB")==0)
    {
        operation_code = FBE_RAID_GROUP_BACKGROUND_OP_REBUILD;
    }
    else if(strcmp(argv, "RG_ERR_VER")==0)
    {
        operation_code = FBE_RAID_GROUP_BACKGROUND_OP_ERROR_VERIFY;
    }
    else if(strcmp(argv, "RG_RW_VER")==0)
    {
        operation_code = FBE_RAID_GROUP_BACKGROUND_OP_RW_VERIFY;
    }
    else if(strcmp(argv, "RG_RO_VER")==0)
    {
        operation_code = FBE_RAID_GROUP_BACKGROUND_OP_RO_VERIFY;
    }
    else if (strcmp(argv, "RG_ENCRYPTION")==0) 
    {
        operation_code = FBE_RAID_GROUP_BACKGROUND_OP_ENCRYPTION_REKEY;
    }
    else if(strcmp(argv, "RG_ALL")==0)
    {
        operation_code = FBE_RAID_GROUP_BACKGROUND_OP_ALL;
    }
    else
    {
        operation_code = 0;
    }
            
    return operation_code;
    
}
/*******************************************************
* End of fbe_cli_lib_bgo_get_operation_code ()  *
*******************************************************/

/************************************************************************
*               fbe_cli_lib_bgo_get_command_usages ()                   *
*************************************************************************
*
*  @brief
*  Helper function to get the command usages string.
*
*  @param    command - argument count
*  
*
*
*  @return
*    pointer to command usages.
*
*
*  @version
*    09/08/2011 - Created. Vishnu Sharma
*
*************************************************************************/

fbe_u8_t* fbe_cli_lib_bgo_get_command_usages(fbe_bgo_enable_disable_cmd_t command)
{
    fbe_u8_t* usages_string = NULL;

    switch(command)
    {
        case FBE_BGO_ENABLE_CMD:
            usages_string = SERVICES_BGO_ENABLE_USAGE;
            break;
        case FBE_BGO_DISABLE_CMD:
            usages_string = SERVICES_BGO_DISABLE_USAGE;
            break;
        case FBE_BGO_STATUS_CMD:
            usages_string = SERVICES_BGO_STATUS_USAGE;
            break;    
    }
  
    return usages_string;
}

/*****************************************
* End of fbe_cli_lib_bgo_get_command_usages ()          *
*****************************************/

/************************************************************************
*                      fbe_cli_lib_bgo_enable_disable ()                   *
*************************************************************************
*
*  @brief
*  This function will execute the background operation 
*  enable/disable API on the given object according to the command.
*  This function will also execute the API for accessing the status
*  of background operations.
*
*  @param    command               - BGO command
*  @param    operation_code        - BGO operation code
*  @param    operation_code_string - BGO operation code string 
*
*  @return
*    None
*
*
*  @version
*    09/08/2011 - Created. Vishnu Sharma
*
*************************************************************************/

void fbe_cli_lib_bgo_enable_disable(fbe_bgo_enable_disable_cmd_t command,
                             fbe_object_id_t obj_id,fbe_u32_t  operation_code,
                             fbe_u8_t* operation_code_string)

{
    fbe_bool_t             is_enabled = FBE_FALSE;
    fbe_status_t           status = FBE_STATUS_INVALID;
    
    switch(command)
    {
        case FBE_BGO_ENABLE_CMD:
            status = fbe_api_base_config_enable_background_operation(obj_id,operation_code);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s:\tUnable  to enable background operation on the object. obj_id: %d\n", __FUNCTION__,obj_id);
                return;
            }
            fbe_cli_printf("\n%s operation is enabled on this object.  object_id :%d\n",operation_code_string,obj_id);
            break;
        case FBE_BGO_DISABLE_CMD:
            status = fbe_api_base_config_disable_background_operation(obj_id , operation_code);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s:\tUnable to disable background operation on the object. obj_id: %d\n", __FUNCTION__,obj_id);
                return;
            }
            fbe_cli_printf("\n%s operation is disabled on this object. object_id :%d\n",operation_code_string,obj_id);
            break;
        case FBE_BGO_STATUS_CMD:
            status = fbe_api_base_config_is_background_operation_enabled(obj_id,operation_code,&is_enabled);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s:\tUnable to get the status of the background operation on the object. obj_id: %d\n", __FUNCTION__,obj_id);
                return;
            }

            fbe_cli_printf("\n%s operation is %s on this object. object_id : %d\n",operation_code_string,
                               is_enabled ? "enabled" : "disabled",obj_id);
                
            break;
        default:
            fbe_cli_printf("\n%s Wrong Command\n",operation_code_string);
            break;
                
    }

    return;
}

/*****************************************
* End of fbe_cli_lib_bgo_enable_disable ()          *
*****************************************/

/************************************************************************
*               fbe_cli_lib_bgo_enable_disable_all ()                   *
*************************************************************************
*
*  @brief
*  This function will execute the background operation 
*  enable/disable API for all available objects in the system
*  according to the command.
*
*  @param    command               - BGO command
*  @param    usages_string         - BGO usages string 
*
*  @return
*    None
*
*
*  @version
*    09/08/2011 - Created. Vishnu Sharma
*
*************************************************************************/

void fbe_cli_lib_bgo_enable_disable_all(fbe_bgo_enable_disable_cmd_t command,
                                        fbe_u8_t* usages_string)
{
    fbe_status_t           status = FBE_STATUS_INVALID;

    switch(command)
    {
        case FBE_BGO_ENABLE_CMD:
            status = fbe_api_base_config_enable_all_bg_services();
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s:\t Unable to enable all background operations in the system.\n", __FUNCTION__);
                return;
            }
            fbe_cli_printf("\n All background  operations are enabled.\n");
            break;
        case FBE_BGO_DISABLE_CMD:
            status = fbe_api_base_config_disable_all_bg_services();
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s:\tUnable to disable all background operations in the system.\n", __FUNCTION__);
                return;
            }
            fbe_cli_printf("\n All background  operations are disabled.\n");
            break;
        default:
            fbe_cli_printf("\n%s Wrong Command\n",usages_string);
            break;
                
    }

    return;
}

/********************************************************
* End of fbe_cli_lib_bgo_enable_disable_all ()          *
*********************************************************/

void fbe_cli_set_bg_op_speed_pvd_read(void)
{
    fbe_provision_drive_control_get_bg_op_speed_t get_bg_op;
    fbe_status_t    status;

    status = fbe_api_provision_drive_get_background_operation_speed(&get_bg_op);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error ("%s:Failed to get background operation speed for PVD, status 0x%x\n", 
                __FUNCTION__, status);
        return;
    }

    fbe_cli_printf("Disk Zeroing         : %d\n", get_bg_op.disk_zero_speed); 
    fbe_cli_printf("Sniff                : %d\n", get_bg_op.sniff_speed); 

    return;
    
}

void fbe_cli_set_bg_op_speed_rg_read(void)
{
    fbe_raid_group_control_get_bg_op_speed_t get_bg_op;
    fbe_status_t    status;

    status = fbe_api_raid_group_get_background_operation_speed(&get_bg_op);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error ("%s:Failed to get background operation speed for RG, status 0x%x\n", 
                __FUNCTION__, status);
        return;
    }

    fbe_cli_printf("Rebuild              : %d\n", get_bg_op.rebuild_speed); 
    fbe_cli_printf("Verify               : %d\n", get_bg_op.verify_speed); 

    return;
    
}

/*!*******************************************************************
 * @var fbe_cli_set_bg_op_speed()
 *********************************************************************
 * @brief Function to set/get Background operation speed in RG and PVD.
 *
 *
 * @return - none.  
 *
 * @author
 *  08/08/2012 - Created. Amit Dhaduk
 *********************************************************************/
void fbe_cli_set_bg_op_speed(fbe_s32_t argc, fbe_s8_t ** argv){
    fbe_status_t    status = FBE_STATUS_INVALID;
    fbe_char_t *tmp_ptr = *argv;
    fbe_u32_t   speed = 0;
    fbe_u32_t   bg_operation_code;
    fbe_char_t *bg_op_code;

        
    if(0 == argc) {
        fbe_cli_error("Please provide correct arguments. \n");
        fbe_cli_printf("%s", SET_BG_OP_SPEED_USAGE);
        return;
    }
    
    if(1 == argc)
    { 
        if((strcmp(*argv, "-help") == 0) ||(strcmp(*argv, "-h") == 0)) 
        { 
            	/* If they are asking for help, just display help and exit. */
            	fbe_cli_printf("%s", SET_BG_OP_SPEED_USAGE);
            	return;
        }
    }

           
    if(strcmp(*argv, "-operation") == 0)        
    {
        /* set the background operation speed */
        argc--;
        // check for presence of data
        if ( 0 == argc )
        {
            fbe_cli_printf( "%s: option requires a value\n",
                         *argv);
            fbe_cli_printf("\n%s",SET_BG_OP_SPEED_USAGE);
            return;
        }
        argv++;

        if(strcmp(*argv, "PVD_DZ")==0)
        {
            bg_operation_code = FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING;
        }
        else if(strcmp(*argv, "PVD_SNIFF")==0)
        {
            bg_operation_code = FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF;
        }

        else if(strcmp(*argv, "RG_REB")==0)
        {
            bg_operation_code = FBE_RAID_GROUP_BACKGROUND_OP_REBUILD;
        }
        else if(strcmp(*argv, "RG_VER")==0)
        {
            /* Currently background speed common for all verify operations */
            bg_operation_code = FBE_RAID_GROUP_BACKGROUND_OP_RW_VERIFY;
        }
        else 
        {
            fbe_cli_printf("Invalid background operation opcode.\n");
            fbe_cli_printf("\n%s",SET_BG_OP_SPEED_USAGE);
            return;
        }

        /* save backgrond opration opcode to use later on */
        bg_op_code = *argv;

        argc--;        
        if ( 0 == argc )
        {
            fbe_cli_printf( "%s: option requires a value\n", *argv);
            fbe_cli_printf("\n%s",SET_BG_OP_SPEED_USAGE);            
            return;
        }

        argv++;

        if(!strcmp(*argv, "-speed") == 0)
        {
            fbe_cli_printf("Invalid argument, please provide -speed options.\n");
            fbe_cli_printf("\n%s",SET_BG_OP_SPEED_USAGE);
            return;
        }

        argc--;        
        if ( 0 == argc )
        {
            fbe_cli_printf( "%s: option requires a -speed and value\n", *argv);
            fbe_cli_printf("\n%s",SET_BG_OP_SPEED_USAGE);
            return;
        }
        argv++; 

        /* get the value*/
        tmp_ptr = *argv;
        if ((*tmp_ptr == '0') && 
            (*(tmp_ptr + 1) == 'x' || *(tmp_ptr + 1) == 'X'))
        {
            fbe_cli_printf("ERROR: speed value should be in decimal.\n");
            return;
        }
        if (*tmp_ptr == '-')
        {
            fbe_cli_printf("ERROR: speed value can not be negative.\n");
            return;
        }
        
        speed = atoi(tmp_ptr);
        if (speed <= 0) 
        {
            fbe_cli_printf("ERROR: Invalid argument (speed value).\n");
            return;
        }      

        
        if((strcmp(bg_op_code, "PVD_DZ") == 0) || (strcmp(bg_op_code, "PVD_SNIFF") == 0))
        {
            /* send request to PVD to set speed */
            status = fbe_api_provision_drive_set_background_operation_speed(bg_operation_code, speed);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_error ("%s:Failed to set background operation speed\n", 
                        __FUNCTION__);
                return;
            }
            fbe_cli_printf( "Background op: %s: speed set to %d\n", bg_op_code, speed);
            return;
        }
        else if((strcmp(bg_op_code, "RG_REB") == 0) || (strcmp(bg_op_code, "RG_VER") == 0))
        {
            /* send request to RG to set speed */
            status = fbe_api_raid_group_set_background_operation_speed(bg_operation_code, speed);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_error ("%s:Failed to set background operation speed\n", 
                        __FUNCTION__);
                return;
            }
            fbe_cli_printf( "Background op: %s: speed set to %d\n", bg_op_code, speed);
            return;
        }
        else
        {
            fbe_cli_printf( "%s: Invalid command parameters\n", *argv);
            fbe_cli_printf("\n%s",SET_BG_OP_SPEED_USAGE);
            return;
        }

    }
    else if((strcmp(*argv, "-status") == 0) && (1 == argc))
    {        
        /* Read the current Background operation speed for all BG operations */
        fbe_cli_printf("Current Background operation speed\n\n");        
        fbe_cli_printf("Background Operation    Speed\n");        
        fbe_cli_set_bg_op_speed_pvd_read();
        fbe_cli_set_bg_op_speed_rg_read();
        return;

    }
    else
    {
        fbe_cli_printf("set_bg_op_speed command fail to execute, Invalid arguments\n");
        return;
    }
}

/************************************************************************
*               fbe_cli_print_bgo_flags ()                   *
*************************************************************************
*
*  @brief
*  Gets system bg service data
*
*  @param
*    None
*  @return
*    None
*
*
*  @version
*    05/16/2013 - Created. Preethi Poddatur
*
*************************************************************************/

void fbe_cli_print_bgo_flags(void)
{
    fbe_status_t           status = FBE_STATUS_INVALID;
    fbe_base_config_control_system_bg_service_t bg_service;

    status = fbe_api_get_system_bg_service_status(&bg_service);
    if (status != FBE_STATUS_OK){
        fbe_cli_printf("%s: failed to get system background services status.\n", __FUNCTION__);
    }
    else {
       fbe_cli_printf("Enable: %d\n",bg_service.enable);

       /* for this cli command, we are interested in viewing "base_config_control_system_bg_service_flag" */
       //fbe_cli_printf("Background service flag: %d\n",bg_service.bgs_flag);

       fbe_cli_printf("Enabled Externally: %d\n",bg_service.enabled_externally);
       fbe_cli_printf("Issued by NDU: %d\n",bg_service.issued_by_ndu);
       fbe_cli_printf("Base config control system bg service flag: 0x%llx \n",(unsigned long long)bg_service.base_config_control_system_bg_service_flag);
    }
    return;
}

/********************************************************
* End of fbe_cli_print_bgo_flags ()          *
*********************************************************/

/*************************************************************************
*                          fbe_cli_cluster_lock()                           *
**************************************************************************
*                                                                        *
*  @brief                                                                *
*    fbe_cli_cluster_lock - exercise cluster lock api                    *
*                                                                        *
*  @param                                                                *
*    argc     (INPUT) NUMBER OF ARGUMENTS                                *
*    argv     (INPUT) ARGUMENT LIST                                      *
*                                                                        *
*  @return                                                               *
*    None.                                                               *
*	                                                                     *
*  @version                                                              *
*                                                                        *
*************************************************************************/
void fbe_cli_cluster_lock(fbe_s32_t argc, fbe_s8_t ** argv)
{
    fbe_status_t status;
    fbe_bool_t  peer_written = FBE_FALSE;

    if (argc == 0)
    {
        fbe_cli_printf("%s Not enough arguments for operating cluster lock\n", __FUNCTION__);
        fbe_cli_printf("%s", SERVICES_CLUSTER_LOCK_USAGE);
        return;
    }

    if (!strcmp(argv[0], "-h") || !strcmp(argv[0], "-help") )
    {
        fbe_cli_printf("%s", SERVICES_CLUSTER_LOCK_USAGE);
        return;
    }

    if (!strcmp(argv[0], "-read"))
    {
        status = fbe_api_cluster_lock_shared_lock(&peer_written);
        fbe_cli_printf("aquire read lock, status 0x%x, peer writen %s\n", status, (peer_written)?"TRUE":"FALSE");
        return;
    }

    if (!strcmp(argv[0], "-release"))
    {
        status = fbe_api_cluster_lock_release_lock(&peer_written);
        fbe_cli_printf("release lock, status 0x%x\n", status);
        return;
    }

    if (!strcmp(argv[0], "-write"))
    {
        status = fbe_api_cluster_lock_exclusive_lock(&peer_written);
        fbe_cli_printf("aquire write lock, status 0x%x peer written %s\n", status, (peer_written)?"TRUE":"FALSE");
        return;
    }

    return;
}

/*************************
* end file fbe_cli_lib_services_cmds.c
*************************/
