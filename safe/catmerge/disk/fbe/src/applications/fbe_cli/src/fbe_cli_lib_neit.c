/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_neit.c
 ***************************************************************************
 *
 * @brief This file is part of NEIT (NTBE Error Injection Tool) service and
 * provides the interface
 * to NEIT on FBE Cli.
 *
 * @ingroup fbe_cli
 *
 * @date June 28, 2010
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
#include "fbe_cli_neit.h"
#include "fbe/fbe_api_lurg_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe_api_logical_error_injection_interface.h"
#include "fbe_protocol_error_injection_service.h"
#include "fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_xor_api.h"
#include "fbe_time.h"
#include "fbe/fbe_api_provision_drive_interface.h"

/*!*******************************************************************
 * @var fbe_cli_neit
 *********************************************************************
 * @brief Function to implement error injection tool in FBE Cli.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author June 28, 2010 - Created. Vinay Patki
 *********************************************************************/
void fbe_cli_neit(int argc, char** argv)
{

    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_protocol_error_injection_error_record_t record_p;
    fbe_protocol_error_injection_record_handle_t record_handle = NULL;

    if (argc < 1)
    {
        /*If there are no arguments provided, print NEIT usage.*/
        fbe_cli_error("%s Not enough arguments to bind\n", __FUNCTION__);
        fbe_cli_printf("%s", LURG_NEIT_USAGE);
        return;
    }

    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /*Print NEIT usage*/
            fbe_cli_printf("%s", LURG_NEIT_USAGE);
            return;
        }
        /*NEIT -start command processing*/
        if (strcmp(*argv, "-start") == 0)
        {

            /*Start the error injection*/
            status = fbe_api_protocol_error_injection_start();

            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s:\tNEIT: Failed to enable error injection. %X\n",
                    __FUNCTION__, status);
                return;
            }

            fbe_cli_printf("%s:\tNEIT: Successfully enabled the error injection. %X\n",
                __FUNCTION__, status);
            return;
        }
        /*NEIT -stop command processing*/
        if (strcmp(*argv, "-stop") == 0)
        {
            /*Stop the error injection.*/
            status = fbe_api_protocol_error_injection_stop();

            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s:\tNEIT: Failed to disable error injection. %X\n",
                    __FUNCTION__, status);
                return;
            }

            fbe_cli_printf("%s:\tNEIT: Successfully disabled the error injection. %X\n",
                __FUNCTION__, status);
            return;
        }

        /*NEIT -add command processing*/
        if (strcmp(*argv, "-add") == 0)
        {

            /*Initialize the Error record*/
            fbe_cli_neit_err_record_initialize(&record_p);

            /*Get the Error injection parameters.*/
            status = fbe_cli_neit_usr_intf_add(&record_p);
            if (status != FBE_STATUS_OK)
            {
                /* FBE_STATUS_CANCELED is returned when user explicitly
                * quits neit command. The message is displayed at the point
                * of quitting. This condition is checked over here.
                */
                if (FBE_STATUS_CANCELED != status)
                {
                    fbe_cli_printf("%s:\tNEIT: Failed to create a record. %X\n", __FUNCTION__, status);
                }
                /* flush whitespaces so that FBE_CLI prompt is not displayed twice
                */
                fflush(stdin);
                return;
            }
        
            /* flush whitespaces so that FBE_CLI prompt is not displayed twice
            */
            fflush(stdin);

            /*Create a new record.*/
            status = fbe_api_protocol_error_injection_add_record(&record_p,&record_handle);

            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s:\tNEIT: Failed to create a record. %X\n", __FUNCTION__, status);
                return;
            }

            fbe_cli_printf("%s:\tNEIT: Successfully created an error record. %X\n", __FUNCTION__, status);
            return;
        }

        /*NEIT command processing for removing a record*/
        if (strcmp(*argv, "-rem") == 0)
        {
            /*Initialize the Error record*/
            fbe_cli_neit_err_record_initialize(&record_p);

            /*Get the Error injection parameters.*/
            status = fbe_cli_neit_usr_intf_remove(&record_handle, &record_p);
            if ((status != FBE_STATUS_OK) || (record_handle == NULL))
            {
                /* FBE_STATUS_CANCELED is returned when user explicitly
                * quits neit command. The message is displayed at the point
                * of quitting. This condition is checked over here.
                */
                if (FBE_STATUS_CANCELED != status)
                {
                    fbe_cli_printf("%s:\tNEIT: Error occured while removing error record. %X\n", __FUNCTION__, status);
                }
                /* flush whitespaces so that FBE_CLI prompt is not displayed twice
                */
                fflush(stdin);
                return;
            }
            
            /* flush whitespaces so that FBE_CLI prompt is not displayed twice
            */
            fflush(stdin);

            status = fbe_api_protocol_error_injection_remove_record(record_handle);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s:\tNEIT: Error occured while removing error record. %X\n", __FUNCTION__, status);
                return;
            }   

            fbe_cli_printf("%s:\tNEIT: Successfully deleted one error record. %X\n", __FUNCTION__, status);
            return;
        }

        /*NEIT command processing for searcing a record*/
        if (strcmp(*argv, "-search") == 0)
        {
            /*Initialize the Error record*/
            fbe_cli_neit_err_record_initialize(&record_p);

            /*Get the Error injection parameters.*/
            status = fbe_cli_neit_usr_intf_search(record_handle, &record_p);
            if (status != FBE_STATUS_OK)
            {
                /* FBE_STATUS_CANCELED is returned when user explicitly
                * quits neit command. The message is displayed at the point
                * of quitting. This condition is checked over here.
                */
                if (FBE_STATUS_CANCELED != status)
                {
                    fbe_cli_printf("%s:\tNEIT: No record exists as per given criteria. %X\n", __FUNCTION__, status);
                }
                /* flush whitespaces so that FBE_CLI prompt is not displayed twice
                */
                fflush(stdin);
                return;
            }
            
            /* flush whitespaces so that FBE_CLI prompt is not displayed twice
            */
            fflush(stdin);

            //  print the record
            status= fbe_cli_neit_print_record(&record_p);
            if (FBE_STATUS_OK != status)
            {
                fbe_cli_printf("%s:\tNEIT: Error occured while printing. %X\n", __FUNCTION__, status);
            }
            return;
        }

        /*NEIT command processing for finding a record*/
        if (strcmp(*argv, "-find") == 0)
        {
            /*Initialize the Error record*/
            fbe_cli_neit_err_record_initialize(&record_p);

            /*Get the Error injection parameters.*/
            status = fbe_cli_neit_usr_intf_search(record_handle, &record_p);

            if (status != FBE_STATUS_OK)
            {
                /* FBE_STATUS_CANCELED is returned when user explicitly
                * quits neit command. The message is displayed at the point
                * of quitting. This condition is checked over here.
                */
                if (FBE_STATUS_CANCELED != status)
                {
                    fbe_cli_printf("%s:\tNEIT: No record exists as per given criteria. %X\n", __FUNCTION__, status);
                }
                /* flush whitespaces so that FBE_CLI prompt is not displayed twice
                */
                fflush(stdin);
                return;
            }
            
            /* flush whitespaces so that FBE_CLI prompt is not displayed twice
            */
            fflush(stdin);

            //  print the record
            status= fbe_cli_neit_print_record(&record_p);
            return;
        }

        /*NEIT command processing for removing an object*/
        if (strcmp(*argv, "-rmobj") == 0)
        {
            /* NEIT - TODO
             * Create a menu to accept the inputs for removing an object.
             */

            fbe_cli_printf("%s:\tNEIT: Command processing for removing an object is currently not supported.\n", __FUNCTION__);
            return;
        }
        /*NEIT command processing for listing records*/
        if (strcmp(*argv, "-list") == 0)
        {
            record_handle = NULL;
            status = fbe_cli_neit_list_record(&record_handle);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s:\tNEIT: Failed to list the record. %X\n", __FUNCTION__, status);
                return;
            }
            return;
        }
        /*NEIT command processing for quickly adding/ changing a record*/
        if (strcmp(*argv, "-count_change") == 0)
        {

            // FORMAT: "neit -count_change f_r_u error count"
            status = fbe_cli_neit_usr_intf_count_change(argc, argv, &record_p);

            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s:\tNEIT: Successfully changed record. %X\n", __FUNCTION__, status);
                return;
            }

            /*Start the error injection*/
            status = fbe_api_protocol_error_injection_start();

            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s:\tNEIT: Failed to start error injection. %X\n", __FUNCTION__, status);
                return;
            }

            fbe_cli_printf("%s:\tNEIT: Successfully started the error injection. %X\n", __FUNCTION__, status);
            return;
        }
        // /*NEIT command processing for parsing XML file */
        //if (strcmp(*argv, "-xml") == 0)
        //{
        //    fbe_u8_t* filename= NULL;

        //    /* Get the xml file name from command line.
        //    */
        //    argc--;
        //    argv++;

        //    /* If argc is greater than 0 means user had provided configuration XML file name.
        //     * If argc is equal to zero means user had not provided configuration XML file name
        //     * so read default file name i.e. config.xml
        //     */
        //    if (argc <= 0)
        //    {
        //        /* Use the default file name. i.e. config.xml 
        //         */
        //        fbe_cli_printf("Reading default xml file. \n");
        //        filename = FBE_NEIT_CONFIG_FILE;
        //    }
        //    else
        //    {
        //        filename = *argv;
        //    }

        //    /* Initialize error injection in Neit Package.
        //     */
        //    fbe_cli_neit_init(filename);
        //    return;

        //}

        fbe_cli_printf("%s", LURG_NEIT_USAGE);
        return;
        
    }
}
/**************************************
 * end fbe_cli_neit()
 **************************************/
/**********************************************************************
 * @var fbe_cli_neit_usr_intf_add
 **********************************************************************
 * 
 * @brief This function collects the parameters for error insertion,
 * updates the Error record.
 *
 * @param Pointer to error record
 *
 * @return     fbe_status_t
 *
 * @author June 28, 2010 - Created. Vinay Patki
 * 03/24/2011 - Edited.  Ashwin Jambhekar   
 *********************************************************************/
fbe_status_t fbe_cli_neit_usr_intf_add(fbe_protocol_error_injection_error_record_t* record_p)
{
    fbe_lba_t lba_range[2];
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_job_service_bes_t bes;

    /*Get the fru number from User in b_e_d format.*/
    status = fbe_cli_neit_usr_get_fru(&bes);
    if (FBE_STATUS_OK != status)
    {
        return status;
    }

    /*Find the object id of a fru*/
    status = fbe_api_get_physical_drive_object_id_by_location(bes.bus, bes.enclosure,
        bes.slot, &record_p->object_id);

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s:\tNEIT: Error occured while finding the Object ID for drive %d_%d_%d. %X\n",
            __FUNCTION__, bes.bus, bes.enclosure, bes.slot, status);

        return FBE_STATUS_FAILED;
    }

    /*Ge the LBA range from User.*/
    status = fbe_cli_neit_usr_get_lba_range(lba_range);
    if (FBE_STATUS_OK != status)
    {
        return status;
    }

    record_p->lba_start = lba_range[0];
    record_p->lba_end = lba_range[1];

    /*Validate the LBA range.*/
    status = fbe_cli_neit_validate_lba_range(record_p, &bes);

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s:\tNEIT: Start and Stop LBA range (%llu - %llu) invalid. %X\n",
                       __FUNCTION__, (unsigned long long)record_p->lba_start,
                   (unsigned long long)record_p->lba_end, status);

        return FBE_STATUS_FAILED;
    }

    /*Get the Error and its details from User.*/
    status = fbe_cli_neit_usr_get_error(record_p);
    if (FBE_STATUS_OK != status)
    {
        return status;
    }

    /*Get number of times to insert the error.*/
    status = fbe_cli_neit_usr_get_times_to_insert_err(record_p);
    if (FBE_STATUS_OK != status)
    {
        return status;
    }

    /*Get reactivation data i.e. delay before reactivatation and no of times to reactivate*/
    status = fbe_cli_neit_usr_get_reactivation_data(record_p);
    if (FBE_STATUS_OK != status)
    {
        return status;
    }

    /*Get frequency of the error insertion.*/
    status = fbe_cli_neit_usr_get_frequency(record_p);
    if (FBE_STATUS_OK != status)
    {
        return status;
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end of fbe_cli_neit_usr_intf_add
 *****************************************/
/**********************************************************************
 * @var fbe_cli_neit_usr_intf_remove
 **********************************************************************
 *
 * @brief This function collects the parameters for error record removal.
 *
 * @param Pointer to error record handle
 * @param Pointer to error record
 *
 * @return     fbe_status_t
 *
 * @author July 08, 2010 - Created. Vinay Patki
 * 03/24/2011 - Edited.  Ashwin Jambhekar   
 *********************************************************************/
static fbe_status_t fbe_cli_neit_usr_intf_remove(fbe_protocol_error_injection_record_handle_t* handle_p,
                                                 fbe_protocol_error_injection_error_record_t* record_p)
{

    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    FBE_NEIT_SEARCH_PARAMS* search_params_ptr = NULL;

    search_params_ptr = (FBE_NEIT_SEARCH_PARAMS *) fbe_api_allocate_memory(sizeof(FBE_NEIT_SEARCH_PARAMS));

    status = fbe_cli_neit_usr_get_search_params(search_params_ptr);
    if (FBE_STATUS_OK != status)
    {
        return status;
    }

    record_p->protocol_error_injection_error_type = search_params_ptr->error_type;

    /*Find the object id of a fru*/
    status = fbe_api_get_physical_drive_object_id_by_location(search_params_ptr->bes.bus,
        search_params_ptr->bes.enclosure, search_params_ptr->bes.slot, &record_p->object_id);

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s:\tNEIT: Error occured while finding the Object ID for drive %d_%d_%d. %X\n",
            __FUNCTION__, search_params_ptr->bes.bus, search_params_ptr->bes.enclosure, search_params_ptr->bes.slot, status);

        fbe_api_free_memory(search_params_ptr);
        return FBE_STATUS_FAILED;
    }

    //  Fill-up error record
    record_p->lba_start = search_params_ptr->lba_start;
    record_p->lba_end = search_params_ptr->lba_end;

    /*Validate the LBA range.*/
    status = fbe_cli_neit_validate_lba_range(record_p, &(search_params_ptr->bes));

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s:\tNEIT: Start and Stop LBA range (%llu - %llu) invalid. %X\n",__FUNCTION__,
                       (unsigned long long)record_p->lba_start,
               (unsigned long long)record_p->lba_end, status);

        fbe_api_free_memory(search_params_ptr);
        return FBE_STATUS_FAILED;
    }

    status = fbe_api_protocol_error_injection_get_record_handle(record_p, handle_p);

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s:\tNEIT: Unable to get handle of an error record. %X\n",__FUNCTION__, status);

        fbe_api_free_memory(search_params_ptr);
        return FBE_STATUS_FAILED;
    }

    fbe_api_free_memory(search_params_ptr);
    return FBE_STATUS_OK;
}
/******************************************
 * end of fbe_cli_neit_usr_intf_remove
 *****************************************/
/**********************************************************************
 * @var fbe_cli_neit_usr_intf_search
 **********************************************************************
 *
 * @brief This function collects the parameters for searching the error
 * record.
 *
 * @param Error record handle
 * @param Pointer to error record
 *
 * @return     fbe_status_t
 *
 * @author July 08, 2010 - Created. Vinay Patki
  * 03/24/2011 - Edited.  Ashwin Jambhekar  
 *********************************************************************/
static fbe_status_t fbe_cli_neit_usr_intf_search(fbe_protocol_error_injection_record_handle_t handle_p,
                                                 fbe_protocol_error_injection_error_record_t* record_p)
{

    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    FBE_NEIT_SEARCH_PARAMS* search_params_ptr = NULL;

    search_params_ptr = (FBE_NEIT_SEARCH_PARAMS *) fbe_api_allocate_memory(sizeof(FBE_NEIT_SEARCH_PARAMS));

    status = fbe_cli_neit_usr_get_search_params(search_params_ptr);
    if (status != FBE_STATUS_OK)
    {
        if (status != FBE_STATUS_CANCELED)
        {
            status = FBE_STATUS_INVALID;
        }
        return status;
    }

    record_p->protocol_error_injection_error_type = search_params_ptr->error_type;

    /*Find the object id of a fru*/
    status = fbe_api_get_physical_drive_object_id_by_location(
        search_params_ptr->bes.bus, search_params_ptr->bes.enclosure, search_params_ptr->bes.slot,
        &record_p->object_id);

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s:\tNEIT: Error occured while finding the Object ID for drive %d_%d_%d. %X\n",
            __FUNCTION__, search_params_ptr->bes.bus, search_params_ptr->bes.enclosure,
            search_params_ptr->bes.slot, status);

        fbe_api_free_memory(search_params_ptr);
        return FBE_STATUS_FAILED;
    }

    /*Fill-up error record*/
    record_p->lba_start = search_params_ptr->lba_start;
    record_p->lba_end = search_params_ptr->lba_end;

    /*Validate the LBA range.*/
    status = fbe_cli_neit_validate_lba_range(record_p, &(search_params_ptr->bes));

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s:\tNEIT: Start and Stop LBA range (%llu - %llu) invalid. %X\n",
                       __FUNCTION__, (unsigned long long)record_p->lba_start,
               (unsigned long long)record_p->lba_end, status);

        fbe_api_free_memory(search_params_ptr);
        return FBE_STATUS_FAILED;
    }

    status = fbe_api_protocol_error_injection_get_record_handle(record_p, &handle_p);

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s:\tNEIT: Unable to get handle of an error record. %X\n",__FUNCTION__, status);

        fbe_api_free_memory(search_params_ptr);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_protocol_error_injection_get_record(handle_p, record_p);

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s:\tNEIT: Unable to get data of an error record. %X\n",__FUNCTION__, status);

        fbe_api_free_memory(search_params_ptr);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_api_free_memory(search_params_ptr);
        return FBE_STATUS_OK;
    }
}
/******************************************
 * end of fbe_cli_neit_usr_intf_search
 *****************************************/
/**********************************************************************
 * fbe_cli_neit_usr_intf_count_change() 
 **********************************************************************
 * @brief:
 *  This function is callback for user interface. This call is 
 *  triggered when the user enters count_change command. Refer to 
 *  fbe_neit_neit_error_count_change for the functionality.
 *  
 *  
 * @param:
 *  args,   [I] - number of commandline arguments
 *  argv,   [I] - command line arguments
 *
 *  The following are the parameters sent via command line
 *  argv[2] - FRU Number
 *  argv[3] - Error type
 *  argv[4] - Error count
 *  
 * @return:
 *  fbe_status_t
 *
 * @author July 08, 2010 - Created. Vinay Patki
 *
 ***********************************************************************/
fbe_status_t fbe_cli_neit_usr_intf_count_change(fbe_u32_t args, char* argv[],
                                                fbe_protocol_error_injection_error_record_t* record_p)
{
    fbe_bool_t val;
    fbe_u8_t error[FBE_NEIT_MAX_STR_LEN]; /* Need this buffer to save error in uppercase. */
    fbe_job_service_bes_t bes;
    fbe_status_t status;

    /* Verify if the arguments provided are valid and then proceed.
    */    
    if( (args > 2) && (argv[3] != NULL) && (argv[4] != NULL))
    {

        //  Scan the fru number
        status = fbe_cli_convert_diskname_to_bed(argv[0], &bes);

        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s:\tSpecified disk number %s is invalid. %X\n", __FUNCTION__, *argv, status);
            fbe_cli_printf("%s", LURG_NEIT_COUNT_CHANGE_USAGE);
            return status;
        }

        /*Find the object id of a fru*/
        status = fbe_api_get_physical_drive_object_id_by_location(bes.bus,
            bes.enclosure, bes.slot, &record_p->object_id);

        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s:\tNEIT: Error occured while finding the Object ID for drive %d_%d_%d. %X\n",
                __FUNCTION__, bes.bus, bes.enclosure, bes.slot, status);

            return status;
        }

        /* Convert the presented argument of error into uppercase.
        */
        fbe_neit_str_to_upper(&error[0], argv[3]);

        /* call the required operation. Arguments passed here are
        * FRU (bes), error value and Error count (argv[4]).
        */
        val = fbe_cli_neit_error_count_change(bes, error, 
            fbe_neit_strtoint(argv[4]));
        if(!val)
        {
            fbe_cli_printf("One record modified.\n");

            return FBE_STATUS_OK;
        }
        else
        {
            fbe_cli_printf("Operation failed\n");

            return FBE_STATUS_FAILED;
        }
    }
    else
    {
        fbe_cli_printf("Not valid format.\nneit count_change <fru> <error> <count>\n");
        fbe_cli_printf("%s", LURG_NEIT_COUNT_CHANGE_USAGE);
    }

    /* Return
    */
    return FBE_STATUS_CONTINUE;
}
/*****************************************
 * end of fbe_cli_neit_usr_intf_count_change
 * **************************************/
/**********************************************************************
 * fbe_cli_neit_error_count_change
 **********************************************************************
 * @brief:
 *  This function either adds a new record or changes the existing
 *  record. If a record with specified error type for the given FRU
 *  does not exist, then a new record is created with the given error
 *  count. If a record with the specified error type does exist for 
 *  the given FRU, then the error count is changed to the input error
 *  count value and the logs value will be reset to 0.
 * 
 * Assumptions:
 *  Only SK related errors can be inserted.
 *  
 * @param:
 *  bes,      [I] - Drive number in B_E_D format.
 *  err_type, [I] - Defines the error type.
 *  num_of_times_to_insert,  [I] - Number of times to insert the error.
 *
 * @return:
 *  fbe_bool_t
 *
 * @author July 08, 2010 - Created. Vinay Patki
 *
 *
 ***********************************************************************/
fbe_bool_t fbe_cli_neit_error_count_change(fbe_job_service_bes_t bes,
                                           fbe_u8_t *err_type, 
                                           fbe_u32_t num_of_times_to_insert)
{
    fbe_protocol_error_injection_error_record_t record_p;
    fbe_protocol_error_injection_record_handle_t record_handle = NULL;
    fbe_job_service_bes_t bes_tmp;
    fbe_status_t status;
    fbe_bool_t val = FBE_FALSE;
    fbe_package_id_t package_id = FBE_PACKAGE_ID_PHYSICAL;

    /* NEIT - TODO: Check if fru_num from the SAS bus*/

    //--> Check if the record exists (FRU, error type, count)

    /*Initialize the Error record*/
    fbe_cli_neit_err_record_initialize(&record_p);

    /* Coverity fix. The record_hanlde won't change in the loop. It only runs once. Remove do-while.*/
        //  For getting first record, we send handle_p as NULL.
        status = fbe_api_protocol_error_injection_get_record_next(record_handle, &record_p);

        /* Find the fru for class id = FBE_CLASS_ID_SAS_PHYSICAL_DRIVE
         */
        status = fbe_cli_get_bus_enclosure_slot_info(record_p.object_id,
            FBE_CLASS_ID_SAS_PHYSICAL_DRIVE, &bes_tmp.bus, &bes_tmp.enclosure, &bes_tmp.slot, package_id);

        if(bes_tmp.bus == bes.bus && bes_tmp.enclosure == bes.enclosure && bes_tmp.slot == bes.slot)
        {
            if(record_p.protocol_error_injection_error_type == 0)
            {
                //  Match found. Modify the match
                record_p.num_of_times_to_insert = num_of_times_to_insert;

                //  Update record
                status = fbe_api_protocol_error_injection_record_update_times_to_insert(record_handle, &record_p);

                if (status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("%s:\tNEIT: Failed to update a record. %X\n", __FUNCTION__, status);
                    return FBE_FALSE;
                }

                val++;
            }
        }

    if(val > 0)
    {
        return FBE_TRUE;
    }

    /* Reaching here signifies that record does not exist.
     * Add it using 'add record' interface.
     */
    /* Update the record values.*/

    /*Find the object id of a fru*/
    status = fbe_api_get_physical_drive_object_id_by_location(bes.bus,
        bes.enclosure, bes.slot, &record_p.object_id);

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s:\tNEIT: Error occured while finding the Object ID for drive %d_%d_%d. Sts:%X\n",
            __FUNCTION__, bes.bus, bes.enclosure, bes.slot, status);
        return FBE_FALSE;
    }

    record_p.num_of_times_to_insert = num_of_times_to_insert;

    /* Fill other parameters of a record */


    /*Create a new record.*/
    status = fbe_api_protocol_error_injection_add_record(&record_p, &record_handle);

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s:\tNEIT: Failed to create a record. %X\n", __FUNCTION__, status);
        return FBE_FALSE;
    }

    fbe_cli_printf("%s:\tNEIT: Successfully created an error record. %X\n", __FUNCTION__, status);

    return FBE_TRUE;
}
/**************************************
 * end of fbe_cli_neit_error_count_change
 *************************************/
/**********************************************************************
 * @var fbe_cli_neit_err_record_initialize
 **********************************************************************
 * 
 * @brief This function initializes the error record to default values.
 *
 * @param Pointer to error record
 *
 * @return     None
 *
 * @author June 28, 2010 - Created. Vinay Patki
 *  
 *********************************************************************/
VOID fbe_cli_neit_err_record_initialize(fbe_protocol_error_injection_error_record_t* record_p)
{

    /*This function initliazes the Error record.*/

    record_p->lba_start = 0;
    record_p->lba_end= 0;

    record_p->object_id = FBE_OBJECT_ID_INVALID;

    record_p->frequency = 0;

    record_p->protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_INVALID;
    fbe_zero_memory(&(record_p->protocol_error_injection_error), sizeof(record_p->protocol_error_injection_error));

    record_p->num_of_times_to_insert = 1;
    record_p->num_of_times_to_reactivate = 0;
    record_p->secs_to_reactivate = FBE_NEIT_INVALID_SECONDS; 

    record_p->times_inserted = 0;
    record_p->times_reset = 0;
    record_p->max_times_hit_timestamp = fbe_get_time();
    record_p->skip_blks = FBE_U32_MAX;
}
/******************************************
 * end of fbe_cli_neit_err_record_initialize
 *****************************************/
/**********************************************************************
 * fbe_cli_neit_usr_get_fru()
 **********************************************************************
 * 
 * @brief This function initializes the error record to default values.
 *
 * @param pointer to structure of fru number in b_e_d format
 *
 * @return     fbe_status_t
 *                  FBE_STATUS_CANCELED  - "quit" pressed
 *                  FBE_STATUS_OK             -  success 
 *
 * @author June 28, 2010 - Created. Omer Miranda
 * 01/04/2011 - Edited.  Ashwin Jambhekar   
 *********************************************************************/
fbe_status_t fbe_cli_neit_usr_get_fru(fbe_job_service_bes_t* bes)
{
    fbe_u8_t                buff[FBE_NEIT_MAX_STR_LEN];
    fbe_status_t            status;
    fbe_bool_t              loop = FALSE;

    /* Prompt the user to enter the values in one of the suggested
    * formats.
    */

    do
    {
        fbe_cli_printf("FRU Number:\n (Valid Format - bus_encl_slot): ");
        scanf("%s",buff);

        // Exit from the connand if "quit" is entered;
        if (fbe_cli_neit_exit_from_command(buff))
        {
            fbe_cli_printf("\nNEIT: Quitting NEIT command...\n\n");
            return FBE_STATUS_CANCELED;
        }
            
        status = fbe_cli_convert_diskname_to_bed(buff, bes);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("That is an invalid entry. Please try again\n");
        }
        else
        {
            loop = FBE_TRUE;
        }
    } while( !loop );

    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_cli_neit_usr_get_fru
 *****************************************/
/*!**************************************************************
 * @var fbe_cli_neit_user_get_lba_range
 ****************************************************************
 * @brief
 *  This function will prompt user to enter the lba range. If user 
 *  wants to store default values they have to enter 0x0 or 0X0. 
 * 
 * @param fbe_lba_t * - Array of LBA range.
 *
 * @return     fbe_status_t
 *                  FBE_STATUS_CANCELED  - "quit" pressed
 *                  FBE_STATUS_OK             -  success *
 * @author
 *  06/03/2010 - Omer Miranda Created. 
 *  07/09/2010 - Vinay Patki. Edited
 *  01/04/2011 - Sandeep Chaudhari. Edited
 * 03/24/2011 - Edited.  Ashwin Jambhekar  
 ****************************************************************/
fbe_status_t fbe_cli_neit_usr_get_lba_range(fbe_lba_t *lba_range_ptr)
{
    fbe_u8_t        buff[FBE_NEIT_MAX_STR_LEN];
    fbe_u32_t       MAX_NO_OF_LBA_VALUES = 2, ith_lba_value=0;
    fbe_char_t      *tmp_ptr;
    
    lba_range_ptr[0] = 0;
    lba_range_ptr[1] = 0;

    while(ith_lba_value < MAX_NO_OF_LBA_VALUES)
    {
        switch(ith_lba_value)
        {
            case GET_START_LBA:
            {
                /* Get lba start from the user.*/
                fbe_cli_printf("Start LBA (Use 0x prefix for Hex) [Enter 0x0 or 0X0 for Default]: ");
                break;
            }
            
            case GET_END_LBA:
            {
                /* Get lba end from the user.*/
                fbe_cli_printf("LBA End. (Use 0x prefix for Hex) [Enter 0x0 or 0X0 for Default]: ");
                break;
            }

            default:
            {
                fbe_cli_printf("Check default value : %d", ith_lba_value);
                return FBE_STATUS_OK;
            }
        }

        EmcpalZeroVariable(buff);
        scanf("%s",buff);
        
        // Exit from the connand if "quit" is entered
        if (fbe_cli_neit_exit_from_command(buff))
        {
            fbe_cli_printf("\nNEIT: Quitting NEIT command...\n\n");
            return FBE_STATUS_CANCELED;
        }

        tmp_ptr= buff;

        /* Check string for "0x" or "0X" prefix
         */
        if(('0' == *tmp_ptr) && 
            (('x' == *(tmp_ptr+1)) || ('X' == *(tmp_ptr+1))))
        {
            tmp_ptr += 2;

            if(!strcmp(tmp_ptr,""))
            {
                fbe_cli_printf("Invalid LBA value detected. Please provide correct LBA value in hex format. \n");
            }
            else
            {
                lba_range_ptr[ith_lba_value]= fbe_atoh(tmp_ptr);

                if(lba_range_ptr[ith_lba_value] < 0 || lba_range_ptr[ith_lba_value]> MAX_DISK_LBA)
                {
                    fbe_cli_error("\nInvalid LBA value detected. Please try again\n");
                }
                else
                {
                    ith_lba_value++;
                }
            }
        }
        else /* Provided format is not in hex format */
        {
            fbe_cli_printf("Invalid LBA value detected. Please provide correct LBA value in hex format. \n");
        }
    }

    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_cli_neit_user_get_lba_range
 *****************************************/

/*!**************************************************************
 * @var fbe_cli_neit_validate_lba_range
 ****************************************************************
 * @brief
 *  This function validates the LBA range of a given error record
 *  pointer. If the LBA start and LBA end of the error rule is 0,
 *  then the user space is considered. Otherwise, the values entered
 *  are treated as the values. The user space will also be considered
 *  if the LBA end entered is less than LBA start. 
 * 
 * @param err_rec_ptr - Pointer to error Record to be inserted to the table.
 *
 * @return fbe_u32_t
 *
 * @author
 *  06/03/2010 - Created. Omer Miranda
 *  01/04/2011 - Edited.  Sandeep Chaudhari 
 ****************************************************************/
fbe_u32_t fbe_cli_neit_validate_lba_range(fbe_protocol_error_injection_error_record_t* 
err_rec_ptr, fbe_job_service_bes_t *bes)
{
    fbe_lba_t fru_capacity = 0;
    
    /* Get fru capacity */
    fru_capacity = fbe_fru_capacity(bes);

    if(FBE_STATUS_FAILED == fru_capacity)
    {
        return FBE_STATUS_FAILED;
    }
    
    /* If LBA start is smaller than 0 or greater than 
     * disk capacity, then point it at start of the user space.
     */
    if((err_rec_ptr->lba_start < 0) || (err_rec_ptr->lba_start > fru_capacity))
    {
        /* Set lba_start at beginning of the user space.*/
        err_rec_ptr->lba_start = 0;
    }
    
    /* If LBA end is smaller than 0 or greater than disk capacity,
     * then point it at end of the user space.
     */
    if((err_rec_ptr->lba_end < 0) || (err_rec_ptr->lba_end > fru_capacity))
    {
        /* Set lba_end at end of the user space.*/
        err_rec_ptr->lba_end = fru_capacity;
    }

    /* If LBA start and LBA end are both equal to 0 or LBA start is 
     * greater than LBA end, then the entered values are considered
     * not valid. Hence replace the values with entire user space range.
     */
    if( ((err_rec_ptr->lba_start == 0) && (err_rec_ptr->lba_end == 0)) ||
        (err_rec_ptr->lba_start > err_rec_ptr->lba_end))
     {
        /* Get the beginning of the user space.*/
        err_rec_ptr->lba_start = 0;

        /* Get the end of the user space.*/
        err_rec_ptr->lba_end = fru_capacity;
    } 

    if( ((err_rec_ptr->lba_start == 0) && (err_rec_ptr->lba_end == 0)) ||
        ((err_rec_ptr->lba_start > err_rec_ptr->lba_end)) ||
        (err_rec_ptr->lba_start == FBE_NEIT_ANY_RANGE))
    {
        return FBE_NEIT_ANY_RANGE;
    }
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_cli_neit_validate_lba_range
 *****************************************/
/*!**************************************************************
 * @var fbe_cli_neit_user_get_error
 ****************************************************************
 * @brief
 * This function is used to get the error type from the user.
 * 
 * @param fbe_protocol_error_injection_error_record_t* 
 *
 * @return None
 *
 * @author   June 28, 2010 - Created. Vinay Patki
 * 03/24/2011 - Edited.  Ashwin Jambhekar  
 *
 ****************************************************************/
fbe_status_t fbe_cli_neit_usr_get_error(fbe_protocol_error_injection_error_record_t* record_ptr)
{
    fbe_u8_t                buff[FBE_NEIT_MAX_STR_LEN];
    fbe_u8_t                error_type = 0;
    BOOL                    loop = FBE_FALSE;
    fbe_status_t         status;
    fbe_sata_command_t sata_command;

    /*
    [FBECLI - TODO]    will need to get the drive type here
    drive_type = cm_fru_query_drive_type(fru_num, FALSE);
    */

    do
    {
        /* prompt the user to enter the error type.
        */
        fbe_cli_printf("Please enter the type of Error you want to inject.\n");
        fbe_cli_printf("1. SCSI\n2. FIS\n3. Port\n");
        fbe_cli_printf("Enter your choice: ");
        scanf("%s",buff);

        // Exit from the connand if "quit" is entered;
        if (fbe_cli_neit_exit_from_command(buff))
        {
            fbe_cli_printf("\nNEIT: Quitting NEIT command...\n\n");
            return FBE_STATUS_CANCELED;
        }
        
        if(strlen(buff) )
        {
            error_type = fbe_neit_strtoint(buff);
            if(error_type == FBE_STATUS_INVALID)
            {
                fbe_cli_error("\nInvalid entry. Please try again\n");
            }
            else
            {
                loop = FBE_TRUE;
            }
        }
        else
        {
            fbe_cli_error("\nInvalid entry. Please try again\n");
        }
    }while(!loop);


    switch(error_type)
    {
    case 1:
        record_ptr->protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI;

        /* Get the scsi error parameters.
        */
        status = fbe_cli_neit_usr_get_scsi_error(record_ptr);
        if (FBE_STATUS_OK != status)
        {
            return status;
        }
            
        break;
    case 2:
        record_ptr->protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_FIS;

        /* If  FBE_NEIT_QUIT is returned means 
            user has quit neit command
        */
        sata_command = fbe_cli_neit_usr_get_sata_command();
        if (FBE_NEIT_QUIT == sata_command)
        {
            return FBE_STATUS_CANCELED;
        }     
        record_ptr->protocol_error_injection_error.protocol_error_injection_sata_error.sata_command = sata_command;
        break;
    case 3:
        record_ptr->protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_PORT;
        break;

    default:
        record_ptr->protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_INVALID;
        break;
    }

    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_cli_neit_user_get_error
 *****************************************/
/*!**************************************************************
 * @var fbe_cli_neit_usr_get_scsi_command
 ****************************************************************
 * @brief
 * This function is used to get the scsi SCSI command for 
 * which the error should be inserted from user.
 * 
 * @param None. 
 *
 * @return fbe_u8_t* 
 *              SCSI Command - Success
 *              NULL - Failure
 *
 * @author
 *  06/15/2010 - Omer Miranda Created. 
 * 03/24/2011 - Edited.  Ashwin Jambhekar   
 ****************************************************************/
fbe_u8_t* fbe_cli_neit_usr_get_scsi_command(void)
{
    fbe_u8_t                buff[FBE_NEIT_MAX_STR_LEN];
    BOOL                    loop = FBE_FALSE;
    fbe_u8_t*               temp_str_ptr;
    fbe_u8_t                scsi_command;

    do
    {
        fbe_cli_printf("Enter the SCSI command for which the error should be inserted (in hex format): ");
        scanf("%s",buff);

        // Exit from the connand if "quit" is entered;
        if (fbe_cli_neit_exit_from_command(buff))
        {
            fbe_cli_printf("\nNEIT: Quitting NEIT command...\n\n");
            return NULL;
        }
        
        /* Check for a hex number: */
        temp_str_ptr = strstr(buff, "0x");       /* Returns a pointer to the beginning of the 0x, or NULL if not found */

        while (temp_str_ptr != NULL)             /* One-time 'loop' */
        {
            /* This may be a valid hex value.
            */
            temp_str_ptr += 2;                        /* We have a leading 0x that we need to skip over. */
            if (fbe_atoh(temp_str_ptr) == 0xFFFFFFFFU)  /* atoh is really ascii hex to integer!!! */
            {
                fbe_cli_printf("Invalid hex value. Please try again.\n");                
                break;   /* ugh.  Not a valid hex number, even though it starts with 0x */
            }
            else 
            {
                /* atoh told us that all chars are valid hex digits. */
                scsi_command = fbe_atoh(temp_str_ptr);
                loop = FBE_TRUE;
                break;  /* We have a valid hex value here, return a pointer to the string */
            }    
        }
    }while(!loop);

    buff[1] = 'x';
    _strupr(buff+2);
    temp_str_ptr = buff;
    
    return temp_str_ptr;
}
/*****************************************
 * end fbe_cli_neit_usr_get_scsi_command
 *****************************************/
/*!**************************************************************
 * @var fbe_cli_neit_usr_get_sense_key
 ****************************************************************
 * @brief
 * This function is used to get the scsi sense key from user.
 * 
 * @param None. 
 *
 * @return fbe_scsi_sense_key_t  - Success
 *              -1    - "quit" entered
 *
 * @author
 *  06/15/2010 - Omer Miranda Created. 
 * 03/24/2011 - Edited.  Ashwin Jambhekar  
 ****************************************************************/
fbe_scsi_sense_key_t fbe_cli_neit_usr_get_sense_key(void)
{
    fbe_u8_t                buff[FBE_NEIT_MAX_STR_LEN];
    fbe_bool_t                    loop = FBE_FALSE;
    fbe_u8_t*               temp_str_ptr;
    fbe_scsi_sense_key_t    sense_key;

    do
    {
        fbe_cli_printf("Enter the sense key (in hex format): ");
        scanf("%s",buff);

        // Exit from the connand if "quit" is entered;
        if (fbe_cli_neit_exit_from_command(buff))
        {
            fbe_cli_printf("\nNEIT: Quitting NEIT command...\n\n");
            return  FBE_SCSI_SENSE_KEY_NO_SENSE;
        }
        
        /* Check for a hex number: */
        temp_str_ptr = strstr(buff, "0x");       /* Returns a pointer to the beginning of the 0x, or NULL if not found */
        while (temp_str_ptr != NULL)                  /* One-time 'loop' */
        {
            /* This may be a valid hex value.
            */
            temp_str_ptr += 2;                        /* We have a leading 0x that we need to skip over. */
            if (fbe_atoh(temp_str_ptr) == 0xFFFFFFFFU )  /* atoh is really ascii hex to integer!!! */
            {
                fbe_cli_printf("Invalid hex value. Please try again.\n");                
                break;   /* ugh.  Not a valid hex number, even though it starts with 0x */
            }
            else 
            {
                /* atoh told us that all chars are valid hex digits. */
                sense_key = fbe_atoh(temp_str_ptr);
                loop = FBE_TRUE;
                break;  /* We have a valid hex value here, return a pointer to the string */
            }    
        }
    }while(!loop);

    return sense_key;
}
/*****************************************
 * end fbe_cli_neit_usr_get_sense_key
 *****************************************/
/*!**************************************************************
 * @var fbe_cli_neit_usr_get_sense_code
 ****************************************************************
 * @brief
 * This function is used to get the scsi additional sense code from user.
 * 
 * @param None. 
 *
 * @return fbe_scsi_additional_sense_code_t - Success
 *              -1    -   "quit" entered
 *
 * @author
 *  06/15/2010 - Omer Miranda Created. 
 * 03/24/2011 - Edited.  Ashwin Jambhekar  
 ****************************************************************/
fbe_scsi_additional_sense_code_t fbe_cli_neit_usr_get_sense_code(void)
{
    fbe_u8_t                            buff[FBE_NEIT_MAX_STR_LEN];
    fbe_bool_t                                loop = FBE_FALSE;
    fbe_u8_t*                           temp_str_ptr;
    fbe_scsi_additional_sense_code_t    sense_code;

    do
    {
        fbe_cli_printf("Enter the scsi additional sense code in (hex format): ");
        scanf("%s",buff);

        // Exit from the connand if "quit" is entered;
        if (fbe_cli_neit_exit_from_command(buff))
        {
            fbe_cli_printf("\nNEIT: Quitting NEIT command...\n\n");
            return FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO;
        }
        
        /* Check for a hex number: */
        temp_str_ptr = strstr(buff, "0x");       /* Returns a pointer to the beginning of the 0x, or NULL if not found */
        while (temp_str_ptr != NULL)             /* One-time 'loop' */
        {
            /* This may be a valid hex value.
            */
            temp_str_ptr += 2;                        /* We have a leading 0x that we need to skip over. */
            if (fbe_atoh(temp_str_ptr) == 0xFFFFFFFFU )  /* atoh is really ascii hex to integer!!! */
            {
                fbe_cli_printf("Invalid hex value. Please try again.\n");                
                break;   /* ugh.  Not a valid hex number, even though it starts with 0x */
            }
            else 
            {
                /* atoh told us that all chars are valid hex digits. */
                sense_code = fbe_atoh(temp_str_ptr);
                loop = FBE_TRUE;
                break;  /* We have a valid hex value here, return a pointer to the string */
            }    
        }
    }while(!loop);

    return sense_code;
}
/*****************************************
 * end fbe_cli_neit_usr_get_sense_code
 *****************************************/
/**********************************************************************
 * @var fbe_cli_neit_usr_get_sense_code_qualifier
 **********************************************************************
 * 
 * @brief
 * This function is used to get the scsi additional sense code qualifier
 * from user.
 *
 * @param
 *     None
 *      
 * @return
 *     additional sense code qualifier - Success
 *     -1     -   "quit" entered
 *
 * @author
 *    Created. 06/15/2010 - Omer Miranda, Vinay Patki
 *    03/24/2011 - Edited.  Ashwin Jambhekar    
 *********************************************************************/
fbe_scsi_additional_sense_code_qualifier_t fbe_cli_neit_usr_get_sense_code_qualifier(void)
{
    fbe_u8_t                                    buff[FBE_NEIT_MAX_STR_LEN];
    BOOL                                        loop = FBE_FALSE;
    fbe_u8_t*                                   temp_str_ptr;
    fbe_scsi_additional_sense_code_qualifier_t  sense_code_qualifier;

    do
    {
        fbe_cli_printf("Enter the scsi additional sense code qualifier (in hex format): ");
        scanf("%s",buff);

        // Exit from the connand if "quit" is entered;
        if (fbe_cli_neit_exit_from_command(buff))
        {
            fbe_cli_printf("\nNEIT: Quitting NEIT command...\n\n");
            return FBE_SCSI_ASCQ_NO_ADDITIONAL_SENSE_INFO;
        }
        
        /* Check for a hex number: */
        temp_str_ptr = strstr(buff, "0x");       /* Returns a pointer to the beginning of the 0x, or NULL if not found */
        while (temp_str_ptr != NULL)             /* One-time 'loop' */
        {
            /* This may be a valid hex value.
            */
            temp_str_ptr += 2;                        /* We have a leading 0x that we need to skip over. */
            if (fbe_atoh(temp_str_ptr) == 0xFFFFFFFFU )  /* atoh is really ascii hex to integer!!! */
            {
                fbe_cli_printf("Invalid hex value. Please try again.\n");                
                break;   /* ugh.  Not a valid hex number, even though it starts with 0x */
            }
            else 
            {
                /* atoh told us that all chars are valid hex digits. */
                sense_code_qualifier = fbe_atoh(temp_str_ptr);
                loop = FBE_TRUE;
                break;  /* We have a valid hex value here, return a pointer to the string */
            }    
        }
    }while(!loop);

    return sense_code_qualifier;
}
/*****************************************
 * end fbe_cli_neit_usr_get_sense_code_qualifier
 *****************************************/
/*!**************************************************************
 * @var fbe_cli_neit_usr_get_sata_command
 ****************************************************************
 * @brief
 * This function is used to get the scsi SATA command for 
 * which the error should be inserted from user.
 * 
 * @param None. 
 *
 * @return fbe_sata_command_t - Success
 *              FBE_NEIT_QUIT    -   "quit" entered
 *
 * @author
 *  06/25/2010 - Vinay Patki Created. 
 * 03/24/2011 - Edited.  Ashwin Jambhekar  
 ****************************************************************/
fbe_sata_command_t fbe_cli_neit_usr_get_sata_command(void)
{
    fbe_u8_t                buff[FBE_NEIT_MAX_STR_LEN];
    BOOL                    loop = FBE_FALSE;
    fbe_u8_t*               temp_str_ptr;
    fbe_sata_command_t                sata_command;

    do
    {
        fbe_cli_printf("Enter the SATA command for which the error should be inserted (in hex format): ");
        scanf("%s",buff);

        // Exit from the connand if "quit" is entered;
        if (fbe_cli_neit_exit_from_command(buff))
        {
            fbe_cli_printf("\nNEIT: Quitting NEIT command...\n\n");
            return FBE_NEIT_QUIT;
        }
                    
        /* Check for a hex number: */
        temp_str_ptr = strstr(buff, "0x");       /* Returns a pointer to the beginning of the 0x, or NULL if not found */

        while (temp_str_ptr != NULL)             /* One-time 'loop' */
        {
            /* This may be a valid hex value.
            */
            temp_str_ptr += 2;                        /* We have a leading 0x that we need to skip over. */
            if (fbe_atoh(temp_str_ptr) == 0xFFFFFFFFU )  /* atoh is really ascii hex to integer!!! */
            {
                fbe_cli_printf("Invalid hex value. Please try again.\n");                
                break;   /* ugh.  Not a valid hex number, even though it starts with 0x */
            }
            else 
            {
                /* atoh told us that all chars are valid hex digits. */
                sata_command = fbe_atoh(temp_str_ptr);
                loop = FBE_TRUE;
                break;  /* We have a valid hex value here, return a pointer to the string */
            }    
        }
    }while(!loop);

    return sata_command;
}
/*****************************************
 * end fbe_cli_neit_usr_get_sata_command
 *****************************************/
/**********************************************************************
 * @var fbe_cli_neit_usr_get_reactivation_data
 **********************************************************************
 * 
 * @brief
 *    This function prompts the user to provide the details about the reactivation policy
 *    of an error record after it has been disabled.
 *
 * @param
 *    Pointer to Error record
 *
 * @return  fbe_status_t 
 *              FBE_STATUS_OK - Success    
 *              Otherwise           - Failure 
 * @author
 *    06/25/2010 - Vinay Patki Created. 
 * 03/24/2011 - Edited.  Ashwin Jambhekar  
 *  
 *********************************************************************/    
fbe_status_t fbe_cli_neit_usr_get_reactivation_data(fbe_protocol_error_injection_error_record_t* record_ptr)
{
    fbe_u8_t        buff[FBE_NEIT_MAX_STR_LEN];
    BOOL            loop = FBE_FALSE;

    fbe_cli_printf("Reactivate record after it is disabled? (y/n): ");
    scanf("%s",buff);

    // Exit from the connand if "quit" is entered;
    if (fbe_cli_neit_exit_from_command(buff))
    {
        fbe_cli_printf("\nNEIT: Quitting NEIT command...\n\n");
        return FBE_STATUS_CANCELED;
    }

    if( strlen(buff) && (!strcmp(buff, "Y") || !strcmp(buff, "y")))
    {
        /* User entered Y, indicating they want reactivation.
         * Get the seconds to reactivate this record in after
         * it is hit the requisite number of times.
         */
        do
        {
            fbe_cli_printf("SECONDS to reactivate the record in. (0..FFFF): ");
            scanf("%s",buff);

            // Exit from the connand if "quit" is entered;
            if (fbe_cli_neit_exit_from_command(buff))
            {
                fbe_cli_printf("\nNEIT: Quitting NEIT command...\n\n");
                return FBE_STATUS_CANCELED;
            }
            record_ptr->secs_to_reactivate = fbe_neit_strtoint(buff);

            if(record_ptr->secs_to_reactivate  == FBE_STATUS_INVALID)
            {
                fbe_cli_error("\nInvalid entry. Please try again\n");
            }
            else
                loop = FBE_TRUE;
        }while(!loop);

        loop = FBE_FALSE;

        do
        {
            fbe_cli_printf("NUMBER OF TIMES to reactivate (0..FFFF): ");
            scanf("%s",buff);

            // Exit from the connand if "quit" is entered;
            if (fbe_cli_neit_exit_from_command(buff))
            {
                fbe_cli_printf("\nNEIT: Quitting NEIT command...\n\n");
                return FBE_STATUS_CANCELED;
            }
            
            record_ptr->num_of_times_to_reactivate = fbe_neit_strtoint(buff);

            if(record_ptr->num_of_times_to_reactivate == FBE_STATUS_INVALID)
            {
                fbe_cli_error("\nInvalid entry. Please try again\n");
            }
            else
                loop = FBE_TRUE;
        }while(!loop);
    }
    else
    {
        /* No reactivation is asked for, just disable reactivation
         * by using FBE_NEIT_INVALID_SECONDS.
         */
        record_ptr->secs_to_reactivate = FBE_NEIT_INVALID_SECONDS; 
        record_ptr->num_of_times_to_reactivate = 0;
    }

    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_cli_neit_usr_get_reactivation_data
 *****************************************/
/*!**************************************************************
 * @var fbe_cli_neit_usr_get_times_to_insert_err
 ****************************************************************
 * @brief
 *  This function will prompt the user to enter 
 *  number of times the error is to be inserted.
 * 
 * @param
 *  Pointer to Error record
 *
 * @return fbe_status_t
 *              FBE_STATUS_OK - Success    
 *              Otherwise           - Failure 
 * @author
 *  06/10/2010 - Omer Miranda Created. 
 * 03/24/2011 - Edited.  Ashwin Jambhekar   
 ****************************************************************/
fbe_status_t fbe_cli_neit_usr_get_times_to_insert_err(fbe_protocol_error_injection_error_record_t* record_p)
{
    fbe_u8_t        buff[FBE_NEIT_MAX_STR_LEN];
    BOOL            loop = FBE_FALSE;

    do
    {
        /* prompt the user to enter the frequency.
        */
        fbe_cli_printf("NUMBER of times to insert the error: ");
        scanf("%s",buff);

        // Exit from the connand if "quit" is entered;
        if (fbe_cli_neit_exit_from_command(buff))
        {
            fbe_cli_printf("\nNEIT: Quitting NEIT command...\n\n");
            return FBE_STATUS_CANCELED;
        }
        
        if(strlen(buff) )
        {
            record_p->num_of_times_to_insert = fbe_neit_strtoint(buff);

            if(record_p->num_of_times_to_insert < 0)
            {
                fbe_cli_error("\nInvalid entry. Please try again.\n");
            }
            else
            {
                loop = FBE_TRUE;
            }
        }
        else
        {
            fbe_cli_error("\nInvalid entry. Please try again\n");
        }
    }while(!loop);

    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_cli_neit_usr_get_times_to_insert_err
 *****************************************/
/*!**************************************************************
 * @var fbe_cli_neit_usr_get_frequency
 ****************************************************************
 * @brief
 *  This function will prompt the user to enter the frequency at which
 *  the error is to be inserted.
 * 
 * @param Pointer to error record.
 *
 * @return fbe_u32_t
 *              FBE_STATUS_OK - Success    
 *              Otherwise           - Failure 
 *
 * @author    06/18/2010 - Vinay Patki Created.
 * 03/24/2011 - Edited.  Ashwin Jambhekar   
 ****************************************************************/
fbe_status_t fbe_cli_neit_usr_get_frequency(fbe_protocol_error_injection_error_record_t* record_p)
{
    fbe_u8_t        buff[FBE_NEIT_MAX_STR_LEN];
    BOOL            loop = FBE_FALSE;

    do
    {
        /* prompt the user to enter the frequency.
        */
        fbe_cli_printf("The FREQ is a NUMBER of IOs to skip per error insertion.\n");
        fbe_cli_printf("Enter FREQ: ");
        scanf("%s",buff);

        // Exit from the connand if "quit" is entered;
        if (fbe_cli_neit_exit_from_command(buff))
        {
            fbe_cli_printf("\nNEIT: Quitting NEIT command...\n\n");
            return FBE_STATUS_CANCELED;
        }
        
        if(strlen(buff))
        {
            record_p->frequency = fbe_neit_strtoint(buff);
            if(record_p->frequency < 0)
            {
                fbe_cli_error("\nInvalid entry. Please try again\n");
            }
            else
            {
                loop = FBE_TRUE;
            }
        }
        else
        {
            fbe_cli_error("\nInvalid entry. Please try again\n");
        }
    }while(!loop);

    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_cli_neit_usr_get_frequency
 *****************************************/
/**********************************************************************
 * @var fbe_neit_strtoint
 **********************************************************************
 * 
 * @brief Converts String to Int.
 *
 * @param pointer to a string
 *
 * @return fbe_u32_t                      
 *
 * @author    06/18/2010 - Vinay Patki Created. 
 *  
 *********************************************************************/    
fbe_u32_t fbe_neit_strtoint(fbe_u8_t* src_str)
{
    fbe_u8_t* temp_str_ptr;
    fbe_u32_t intval, src_len;    

    if(src_str == NULL)
    {
        return FBE_STATUS_INVALID;
    }

    if(strstr(src_str, "0x") == NULL)
    {
        /* Make sure there are no alpha characters. 
         */
        src_len = (fbe_u32_t)strlen(src_str);
        while(src_len)
        {
            src_len--;
            /* 48 is ASCII notation for 0 and 57 is for ASCII notation for
            * 9.
            */
            if( (src_str[src_len] < 48 ) || (src_str[src_len] > 57))
            {
                fbe_cli_printf("Invalid user parameter detected, defaulting value to 0xffffffff.\n");
                return 0xffffffff;
            }
        }
        /* This is a decimal value.
         */
        intval = atoi(src_str);
    }
    else
    {
        /* This is a hex value.
         */
        temp_str_ptr = strtok(src_str, "x");
        temp_str_ptr = strtok(NULL, "x");
        if(temp_str_ptr == NULL)
        {
            return FBE_STATUS_INVALID;
        }
        else
        {
            intval = fbe_atoh(temp_str_ptr);
        }
    }
    return intval;
}
/*****************************************
 * end fbe_neit_strtoint
 *****************************************/
/**********************************************************************
 * fbe_neit_str_to_upper()
 **********************************************************************
 * @brief:
 *  This is a quick implementation of converting a given string to
 *  the uppercase. 
 * 
 *  
 * @param:
 *  dest_ptr,   [O] - Array of characters with all upper case.
 *  src_ptr,    [I] - Array of characters that need to be converted 
 *                  to uppercase.
 *
 * @return:
 *  The string with all uppercase characters.
 *
 * @author    Created. 06/18/2010 - Omer Miranda
 *            Edited              - Vinay Patki
 *
 ***********************************************************************/
fbe_u8_t* fbe_neit_str_to_upper(fbe_u8_t *dest_ptr, fbe_u8_t *src_ptr)
{
    while(*src_ptr != '\0')
    {
        *dest_ptr = toupper(*src_ptr);        
        dest_ptr++;
        src_ptr++;
    } 
    *dest_ptr =  '\0';

    return dest_ptr;
}
/**************************
 * end of fbe_neit_str_to_upper
 *************************/
/****************************************************************
 * @var fbe_neit_what_is_fru_private_space
 ****************************************************************
 *  @brief                                                
 *        This function is called to determine where the private   
 *        space ends and the user space begins.                    
 *
 *  @param:                                                      
 *        The fru number. Since we may have disks with different
 *        private space reserved, we need to know what that value
 *        is for the specific fru we are looking at.
 *
 *  @return: LBA
 *
 *  @author    June 28, 2010 - Created. Vinay Patki
 *      
 *****************************************************************/
fbe_lba_t fbe_cli_neit_what_is_fru_private_space(fbe_job_service_bes_t* bes)
{
    /*
    #if defined(ALPINE_ENV) && !defined(UMODE_ENV)
    return (private_space);
    #else
    return (FRU_PRIVATE_SPACE);
    #endif
    */
    return MAX_DISK_LBA;
}
/*****************************************
 * end fbe_neit_what_is_fru_private_space
 *****************************************/
/****************************************************************
 * @var fbe_fru_capacity
 ****************************************************************
 *  @brief This function returns the capacity of a fru
 *
 *  @param: bes - drive number
 *
 *  @return: LBA
 *
 *  @author    June 28, 2010 - Created. Vinay Patki
 *            January 04, 2011 - Edited. Sandeep Chaudhari
 *****************************************************************/
fbe_lba_t fbe_fru_capacity(fbe_job_service_bes_t* bes)
{
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_object_id_t pvd_id = FBE_OBJECT_ID_INVALID;
    fbe_api_provision_drive_info_t pvd_info;

    /* Get the provision drive object using b_e_s info */
    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(bes->bus, bes->enclosure, bes->slot, &pvd_id);
    
    if(status != FBE_STATUS_OK)
    {
        /* if unable to get the object id of the drive, then return error from here */
        fbe_cli_printf ("%s:Failed to get object id of %d_%d_%d ! Sts:%X\n", __FUNCTION__, bes->bus, bes->enclosure, bes->slot, status);
        return FBE_STATUS_FAILED;
    }

    /* Get the provision object information */
    fbe_zero_memory(&pvd_info, sizeof pvd_info);
    status = fbe_api_provision_drive_get_info(pvd_id, &pvd_info);
    
    if(status != FBE_STATUS_OK)
    {
        /* if unable to get the pvd object info, then return error from here */
        fbe_cli_printf ("%s:Failed to get pvd object info ! Sts:%X\n", __FUNCTION__, status);
        return FBE_STATUS_FAILED;
    }
    
    return pvd_info.capacity;
}
/*****************************************
 * end fbe_fru_capacity
 *****************************************/
/**********************************************************************
 * fbe_neit_usr_get_search_params() 
 **********************************************************************
 * @brief:
 *  This function is used to prompt the user for search parameters viz.
 *  fru, lba range and opcode. This function will be used by both
 *  search and delete error rules from the user prompt. 
 * 
 *  
 * @param:
 *  search_params_ptr,  [O]- Fill up the search parameter values.
 *
 * @return:   fbe_status_t   
 *                 FBE_STATUS_OK - Success  
 *                 FBE_STATUS_CANCELLED - "quit" entered"
 *  @author
 *      June 28, 2010 - Created. Vinay Patki
 *      January 04, 2011 - Edited. Sandeep Chaudhari
 *      03/24/2011 - Edited.  Ashwin Jambhekar   
 **********************************************************************/
fbe_status_t fbe_cli_neit_usr_get_search_params(FBE_NEIT_SEARCH_PARAMS* search_params_ptr)
{
    fbe_lba_t fru_capacity = 0;
    fbe_lba_t lba_range[2];
    fbe_status_t status = FBE_STATUS_INVALID;

    /* The passed in pointer is not valid. So just return at this 
    * point or we end up panicking.
    */
    if(search_params_ptr == NULL)
    {
        fbe_cli_printf ("%s:Invalid search parameter input. ! Sts:%X\n", __FUNCTION__, status);
        return FBE_STATUS_INVALID;
    }
    /* Initialize the LBA Values.
    */
    search_params_ptr->lba_start=0;
    search_params_ptr->lba_end=0;

    /* Get the FRU number.
    */
    status = fbe_cli_neit_usr_get_fru(&search_params_ptr->bes);
    if (FBE_STATUS_OK != status)
    {
        return status;
    }

    /* Get the LBA Range.
    */    
    status = fbe_cli_neit_usr_get_lba_range(lba_range);
    if (FBE_STATUS_OK != status)
    {
        return status;
    }

    search_params_ptr->lba_start = lba_range[0];
    search_params_ptr->lba_end = lba_range[1];

    /* Get the fru capacity */
    fru_capacity = FBE_NEIT_DEFAULT_LBA_END(&search_params_ptr->bes);
    
    /* Validate the LBA Ranges. 
    */
    if((lba_range[0] < 0) || (lba_range[0] > fru_capacity))
    {
        search_params_ptr->lba_start = 0;
    }

    if((lba_range[1] < 0) ||(lba_range[1] > fru_capacity))
    {
        search_params_ptr->lba_end = fru_capacity;
    }
    
    if( (lba_range[1] < lba_range[0]) || 
        ((lba_range[0] == 0) && (lba_range[1] == 0)))
    {
        search_params_ptr->lba_start = 0; 
        search_params_ptr->lba_end = FBE_NEIT_DEFAULT_LBA_END(&search_params_ptr->bes);
    }
    
    /* Get the error type.
     */
    search_params_ptr->error_type = fbe_cli_neit_user_get_err_type();
    if (FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_INVALID == search_params_ptr->error_type)
    {
        fbe_cli_printf("Provided error type is invalid.\n");
    }

    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_cli_neit_usr_get_search_params
 *****************************************/
/*!**************************************************************
 * @var fbe_cli_neit_user_get_err_type
 ****************************************************************
 * @brief
 * This function is used to get the error type from the user.
 * 
 * @param None 
 *
 * @return fbe_protocol_error_injection_error_type_t    - Success
 *              FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_INVALID - "quit" entered
 * @author   June 28, 2010 - Created. Vinay Patki
 * 03/24/2011 - Edited.  Ashwin Jambhekar  
 ****************************************************************/
fbe_protocol_error_injection_error_type_t fbe_cli_neit_user_get_err_type(void)
{
    fbe_u8_t                buff[FBE_NEIT_MAX_STR_LEN];
    fbe_u8_t                error_type = 0;
    BOOL                    loop = FBE_FALSE;

    /*
    [FBECLI - TODO] need to get the drive type here
    drive_type = cm_fru_query_drive_type(fru_num, FALSE);
    */

    do
    {
        /* prompt the user to enter the error type.
        */
        fbe_cli_printf("Please enter the type of Error you want to inject.\n");
        fbe_cli_printf("1. SCSI\n2. FIS\n3. Port\n");
        fbe_cli_printf("Enter your choice: ");
        scanf("%s",buff);

        // Exit from the connand if "quit" is entered;
        if (fbe_cli_neit_exit_from_command(buff))
        {
            fbe_cli_printf("\nNEIT: Quitting NEIT command...\n\n");
            return FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_INVALID;
        }

        if(strlen(buff) )
        {
            error_type = fbe_neit_strtoint(buff);
            if(error_type == FBE_STATUS_INVALID)
            {
                fbe_cli_error("\nInvalid entry. Please try again\n");
            }
            else
            {
                loop = FBE_TRUE;
            }
        }
        else
        {
            fbe_cli_error("\nInvalid entry. Please try again\n");
        }
    }while(!loop);

    switch(error_type)
    {
    case 1:
        return(FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI);

    case 2:
        return(FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_FIS);

    case 3:
        return(FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_PORT);

    default:
        return(FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_INVALID);
    }
}
/*****************************************
 * end fbe_cli_neit_user_get_err_type
 *****************************************/
/*!**************************************************************
 * @var fbe_cli_neit_print_record
 ****************************************************************
 * @brief
 * This function is used to print the record.
 * 
 * @param Pointer to error record 
 *
 * @return fbe_status_t
 *
 * @author   July 27, 2010 - Created. Vinay Patki
 *           February 21, 2011 - Modified. Sandeep Chaudhari
 ****************************************************************/
fbe_status_t fbe_cli_neit_print_record(fbe_protocol_error_injection_error_record_t* record_p)
{
    fbe_status_t status;
    fbe_job_service_bes_t bes;
    fbe_package_id_t package_id = FBE_PACKAGE_ID_PHYSICAL;
    fbe_u8_t error_type[9];

    /* Find the fru for class id = FBE_CLASS_ID_SAS_PHYSICAL_DRIVE 
     */
    status = fbe_cli_get_bus_enclosure_slot_info(record_p->object_id,
        FBE_CLASS_ID_SAS_PHYSICAL_DRIVE, &bes.bus, &bes.enclosure, &bes.slot, package_id);

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s:\tNEIT: Failed to get bes info. %X\n", __FUNCTION__, status);
        return status;
    }

    fbe_cli_printf("| %d_%d_%02d |", bes.bus, bes.enclosure, bes.slot);
    fbe_cli_printf(" 0x%08llx |", (unsigned long long)record_p->lba_start);
    fbe_cli_printf(" 0x%08llx |", (unsigned long long)record_p->lba_end);
    fbe_cli_printf("  %3d |", record_p->frequency);

    switch(record_p->protocol_error_injection_error_type)
    {
    case FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI:
        strcpy(error_type,"SCSI");
        break;

    case FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_FIS:
        strcpy(error_type,"FIS");
        break;

    case FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_PORT:
        strcpy(error_type,"PORT");
        break;

    case FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_INVALID:
    default:
        strcpy(error_type,"INVALID");
        break;
    }

    fbe_cli_printf(" %7s |", error_type);
    fbe_cli_printf("      %3d |", record_p->num_of_times_to_insert);
    fbe_cli_printf("        %3d |", record_p->num_of_times_to_reactivate);
    fbe_cli_printf("        %3d |", record_p->secs_to_reactivate);
    
    switch(record_p->protocol_error_injection_error_type)
    {
    case FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI:
        {
            fbe_cli_printf("Command    : %-12s|\n", record_p->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command);

            fbe_cli_printf("|        |            |            |      |         |          |            |            |Sense Key  : 0x%-8x  |\n", record_p->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key);
            fbe_cli_printf("|        |            |            |      |         |          |            |            |Sense Code : 0x%-8x  |\n", record_p->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code);
            fbe_cli_printf("|        |            |            |      |         |          |            |            |Code Qualif: 0x%-8x  |\n", record_p->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code_qualifier);
            fbe_cli_printf("|        |            |            |      |         |          |            |            |Port Status: 0x%-8x  |\n", record_p->protocol_error_injection_error.protocol_error_injection_scsi_error.port_status);
        }
        break;

    case FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_FIS:
        {
            fbe_cli_printf("Command    : 0x%-8x  |\n", record_p->protocol_error_injection_error.protocol_error_injection_sata_error.sata_command);
            fbe_cli_printf("|        |            |            |      |         |          |            |            |Resp:%-20s|\n", record_p->protocol_error_injection_error.protocol_error_injection_sata_error.response_buffer);
            fbe_cli_printf("|        |            |            |      |         |          |            |            |Port Status: 0x%-8x  |\n", record_p->protocol_error_injection_error.protocol_error_injection_sata_error.port_status);
        }
        break;

    case FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_PORT:
        {
            fbe_cli_printf("Port Status: 0x%-8x  |\n", record_p->protocol_error_injection_error.protocol_error_injection_port_error.port_status);
        }
        break;

    case FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_INVALID:
        {
            fbe_cli_printf("Invalid Error Type !     |\n");
        }
        break;

    default:
        /* No parameters to print
         */
        fbe_cli_printf("                         |\n");
        break;
    }
    fbe_cli_printf("|--------|------------|------------|------|---------|----------|------------|------------|-------------------------|\n");
    return FBE_STATUS_OK;
}
/*****************************************
* end fbe_cli_neit_print_record
*****************************************/
/*!**************************************************************
 * @var fbe_cli_neit_list_record
 ****************************************************************
 * @brief
 * This function is used to print the record.
 * 
 * @param Pointer to error record handle
 *
 * @return fbe_status_t
 *
 * @author   July 27, 2010 - Created. Vinay Patki
 *
 ****************************************************************/
fbe_status_t fbe_cli_neit_list_record(fbe_protocol_error_injection_record_handle_t *handle_p)
{
    fbe_status_t status;
    fbe_protocol_error_injection_error_record_t record_p;

    /*Initialize the Error record*/
    fbe_cli_neit_err_record_initialize(&record_p);

    fbe_cli_neit_display_table_header();

    do{
        /* For getting first record, we send handle_p as NULL. */
        status = fbe_api_protocol_error_injection_get_record_next(handle_p, &record_p);

        if(*handle_p == NULL || status != FBE_STATUS_OK)
        {
            return status;
        }

        status = fbe_cli_neit_print_record(&record_p);

    }while(*handle_p != NULL);

    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_cli_neit_list_record
 *****************************************/
/*!**************************************************************
 * fbe_cli_neit_display_table_header()
 ****************************************************************
 * @brief
 *  Just trace out some header info for where we are displaying
 *  a table of base objects.
 *
 * @param - None.
 *
 * @return None.   
 *
 * @author
 *  8/12/2010 - Created. Vinay Patki
 *  2/22/2011 - Edited.  Sandeep Chaudhari
 ****************************************************************/
static void fbe_cli_neit_display_table_header(void)
{
    fbe_cli_printf("+------------------------------------------------------------------------------------------------------------------+\n");
    fbe_cli_printf("|  FRU   |    LBA     |    LBA     | Freq |  Error  | No of    | No of      | Seconds to |          Error          |\n");
    fbe_cli_printf("|        |   start    |    end     |      |   type  | times to | times to   | reactivate |         Parameters      |\n");
    fbe_cli_printf("|        |            |            |      |         | insert   | reactivate |            |                         |\n");
    fbe_cli_printf("|--------|------------|------------|------|---------|----------|------------|------------|-------------------------|\n");
    return;
}
/******************************************
 * end fbe_cli_neit_display_table_header()
 ******************************************/

/*!*******************************************************************
 * fbe_cli_neit_exit_from_command()
 *********************************************************************
 * @brief       Function to decide whether to exit from command.
 *
 *  @param      command_str - command string
 *
 * @return      fbe_bool_t  true for exit, else false
 *
 * @author
 *  03/23/2011 - Created. Ashwin Jambhekar
 *********************************************************************/
fbe_bool_t fbe_cli_neit_exit_from_command(fbe_char_t *command_str)
{
    if ((0 == strcmp(command_str, "quit") || 0 == strcmp(command_str, "q")) && 
        NULL != command_str)
    {
        return FBE_TRUE;
    }
    else
    {
        return FBE_FALSE;
    }
}
/****************************
* fbe_cli_neit_exit_from_command()
****************************/

/*!*******************************************************************
 * fbe_cli_neit_usr_get_scsi_error()
 *********************************************************************
 * @brief       Function which prompts user to enter scsi error parameters.
 *
 *  @param      command_str - command string
 *
 * @return      fbe_status_t  
                    FBE_STATUS_OK - Success
                    FBE_STATUS_CANCELLED - "quit" entered
                    Otherwise           - Failure 
 *
 * @author
 *  04/18/2011 - Created. Ashwin Jambhekar
 *********************************************************************/
fbe_status_t fbe_cli_neit_usr_get_scsi_error(fbe_protocol_error_injection_error_record_t* record_ptr)
{
    fbe_u8_t              *command_buff = NULL;
    fbe_scsi_sense_key_t                                    sense_key;
    fbe_scsi_additional_sense_code_t                   sense_code;
    fbe_scsi_additional_sense_code_qualifier_t      sense_qualifier;


    command_buff = fbe_cli_neit_usr_get_scsi_command();

    /*  Status FBE_STATUS_CANCELED means user wants to quit command
    */
    if (NULL == command_buff)
    {
        return FBE_STATUS_CANCELED;
    }

    // used strcpy so as to display entire command string
    strcpy(record_ptr->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command, 
        command_buff);

    /* If  FBE_SCSI_SENSE_KEY_NO_SENSE is returned means 
        user has quit neit command
    */
    sense_key = fbe_cli_neit_usr_get_sense_key();
    if ( FBE_SCSI_SENSE_KEY_NO_SENSE == sense_key)
    {
        return FBE_STATUS_CANCELED;
    }        
     record_ptr->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key = sense_key;

    /* If  FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO is returned means 
        user has quit neit command
    */
    sense_code= fbe_cli_neit_usr_get_sense_code();
    if (FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO == sense_code)
    {
        return FBE_STATUS_CANCELED;
    }        
    record_ptr->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code = sense_code;

    /* If  FBE_SCSI_ASCQ_NO_ADDITIONAL_SENSE_INFO is returned means 
        user has quit neit command
    */        
    sense_qualifier= fbe_cli_neit_usr_get_sense_code_qualifier();
    if (FBE_SCSI_ASCQ_NO_ADDITIONAL_SENSE_INFO == sense_qualifier)
    {
        return FBE_STATUS_CANCELED;
    }        
    record_ptr->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code_qualifier = sense_qualifier;


    record_ptr->protocol_error_injection_error.protocol_error_injection_scsi_error.port_status
        = FBE_PORT_REQUEST_STATUS_SUCCESS;

    return FBE_STATUS_OK;
}
/****************************
* fbe_cli_neit_usr_get_scsi_error()
****************************/

/*********************************
 * end file fbe_cli_lib_neit.c
 *********************************/
