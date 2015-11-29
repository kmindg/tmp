/***************************************************************************
* Copyright (C) EMC Corporation 2011 
* All rights reserved. 
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!*************************************************************************
*                 @file fbe_cli_lib_copy_cmds.c
****************************************************************************
*
* @brief
*  This file contains cli functions for the copy related features in
*  FBE CLI.
*
* @ingroup fbe_cli
*
* @date
*  06/04/2012 - Created. Vera Wang
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cli_private.h"
#include "fbe_cli_lib_copy_cmds.h"
#include "fbe/fbe_api_provision_drive_interface.h"


/**************************
*   LOCAL FUNCTIONS
**************************/
static void fbe_cli_copy_initial_copy_to(fbe_object_id_t source_object_id, fbe_object_id_t dest_object_id);
static void fbe_cli_copy_initial_proactive_spare(fbe_object_id_t source_object_id);


/*!**********************************************************************
*                      fbe_cli_lib_copy_cmds()                 
*************************************************************************
*
*  @brief
*    fbe_cli_database - Get database information in database service
	
*  @param    argc - argument count
*  @param    argv - argument string
*
*  @return
*            None
************************************************************************/
void fbe_cli_copy(fbe_s32_t argc, fbe_s8_t ** argv)
{
    fbe_object_id_t   source_object_id;
    fbe_object_id_t   dest_object_id;
    fbe_job_service_bes_t phys_location_source;
    fbe_job_service_bes_t phys_location_dest;
    fbe_status_t status;

    if (argc < 1)
    {
        /*If there are no arguments show usage*/
        fbe_cli_error("\nToo few arguments. \n");
        fbe_cli_printf("%s", FBE_CLI_COPY_USAGE);
        return;
    }
    if((1 == argc) && 
        ((strcmp(*argv, "-help") == 0) ||(strcmp(*argv, "-h") == 0)))
    {
        /* If they are asking for help, just display help and exit. */
        fbe_cli_printf("%s", FBE_CLI_COPY_USAGE);
        return;
    }
    else if ((strncmp(*argv, "-copyTo", 7) == 0)
             || (strncmp(*argv, "-copyto", 7) == 0)) {
        /* Next argument is object id value(interpreted in HexaDECIMAL).
        */
        argv++;

        if (*argv != NULL)
        {
            if(strcmp(*argv, "-sd") == 0) 
            {
                argv++;
                status = fbe_cli_convert_diskname_to_bed(argv[0], &phys_location_source);
                if(status != FBE_STATUS_OK)
                {
                    fbe_cli_error("\n Please enter the source disk in the correct format b_e_d e.g. 0_1_0. \n");
                    fbe_cli_printf("%s", FBE_CLI_COPY_USAGE);
                    return;
                }
                argv++;
                if (*argv != NULL)
                {
                    if(strcmp(*argv, "-dd") == 0) 
                    {
                        argv++;
                        status = fbe_cli_convert_diskname_to_bed(argv[0], &phys_location_dest);
                        if(status != FBE_STATUS_OK)
                        {
                            fbe_cli_error("\n Please enter the destination disk in the correct format b_e_d e.g. 0_1_0. \n");
                            fbe_cli_printf("%s", FBE_CLI_COPY_USAGE);
                            return;
                        }
                    }

                    status = fbe_api_provision_drive_get_obj_id_by_location(phys_location_source.bus, 
                                                                            phys_location_source.enclosure,
                                                                            phys_location_source.slot,    
                                                                            &source_object_id);
                    if(status != FBE_STATUS_OK)
                    {
                        fbe_cli_error("Unable to get the object ID for Source Disk\n");
                        return;
                    }
                    status = fbe_api_provision_drive_get_obj_id_by_location(phys_location_dest.bus, 
                                                                            phys_location_dest.enclosure,
                                                                            phys_location_dest.slot,    
                                                                            &dest_object_id);
                    if(status != FBE_STATUS_OK)
                    {
                        fbe_cli_error("Unable to get the object ID for destination Disk\n");
                        return;
                    }
                    fbe_cli_copy_initial_copy_to(source_object_id, dest_object_id);
                    return;
                }
                else
                {
                    fbe_cli_error("\n Please enter the source and destination disk in the correct format b_e_d e.g. 0_1_0. \n");
                    fbe_cli_printf("%s", FBE_CLI_COPY_USAGE);
                    return;
                }
            }
            source_object_id = fbe_atoh(*argv);
            /* need to check the object id is pvd. */
            /*if (object_id_for_get_tables < 0 || object_id_for_get_tables > max_objects)
            {
                fbe_cli_printf("\n");
                fbe_cli_error("Invalid Object Id.\n\n");
                fbe_cli_printf("%s", FBE_CLI_COPY_USAGE);
                return;
            }*/
            argv++;
            if(*argv != NULL) {
                dest_object_id = fbe_atoh(*argv);
            }
            else{  
                fbe_cli_error("\nYou need to specify dest_object_id in copyTo command\n\n");
                fbe_cli_printf("%s", FBE_CLI_COPY_USAGE);
                return;
            } 
            fbe_cli_copy_initial_copy_to(source_object_id, dest_object_id);
        }
        else
        {  
            fbe_cli_error("\nYou need to specify both Source and Destination in copyTo command\n\n");
            fbe_cli_printf("%s", FBE_CLI_COPY_USAGE);
            return;
        }

        return;
        
    }
    else if ((strncmp(*argv, "-proactiveSpare", 15) == 0)
             || (strncmp(*argv, "-proactivespare", 15) == 0)) {
        /* Next argument is object id value(interpreted in HexaDECIMAL).
        */
        argv++;

        if (*argv != NULL)
        {
            source_object_id = fbe_atoh(*argv);
            /* need to check the object id is pvd. */
            /*if (object_id_for_get_tables < 0 || object_id_for_get_tables > max_objects)
            {
                fbe_cli_printf("\n");
                fbe_cli_error("Invalid Object Id.\n\n");
                fbe_cli_printf("%s", FBE_CLI_COPY_USAGE);
                return;
            }*/
            fbe_cli_copy_initial_proactive_spare(source_object_id);
        }
        else
        {  
            fbe_cli_error("\nYou need to specify both source_object_id in proactiveSpare command\n\n");
            fbe_cli_printf("%s", FBE_CLI_COPY_USAGE);
            return;
        }
        return;        
    }
    else
    {
        /*If there are no arguments show usage*/
        fbe_cli_error("%s", FBE_CLI_COPY_USAGE);
        return;
    }
}

/*!**************************************************************
 * fbe_cli_copy_initial_copy_to(source_object_id, dest_object_id)
 ****************************************************************
 * @brief
 *  This function initial copy to.
 *
 * @param source_object_id
 *        dest_object_id
 * 
 * @return
 *        None
 ***************************************************************/
static void fbe_cli_copy_initial_copy_to(fbe_object_id_t source_object_id, fbe_object_id_t dest_object_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_provision_drive_copy_to_status_t copy_status;
    
    status = fbe_api_copy_to_replacement_disk(source_object_id, dest_object_id, &copy_status);

    if ((status == FBE_STATUS_OK))
    {
        fbe_cli_printf("\nSuccessfully initiate copyTo command from source object 0x%x to dest object 0x%x.\n\n", source_object_id, dest_object_id);    
    }
    else
    {
        fbe_cli_printf("\nFailed to initiate copyTo command ... Error: %d, Copy_status: %d\n\n", status, copy_status);             
    }

    return;
}

/*!**************************************************************
 * fbe_cli_copy_initial_proactive_spare(source_object_id)
 ****************************************************************
 * @brief
 *  This function initial proactive spare.
 *
 * @param source_object_id
 * 
 * @return
 *        None
 ***************************************************************/
static void fbe_cli_copy_initial_proactive_spare(fbe_object_id_t source_object_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_provision_drive_copy_to_status_t copy_status;
    
    status = fbe_api_provision_drive_user_copy(source_object_id, &copy_status);

    if ((status == FBE_STATUS_OK))
    {
        fbe_cli_printf("\nSuccessfully initiate proactiveSpare command from source object 0x%x.\n\n", source_object_id);    
    }
    else
    {
        fbe_cli_printf("\nFailed to initiate proactiveSpare command ... Error: %d, Copy_status: %d\n\n", status, copy_status);            
    }

    return;
}
