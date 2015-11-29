/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_event_log_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contains FBE cli functions for the event log services.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *   07/09/2010:  Vishnu Sharma - created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_cli_event_log_cmds.h"
#include "fbe/fbe_api_common.h"
#include "fbe_cli_private.h"
#include "fbe/fbe_api_event_log_interface.h"

/*************************************************************************
 *                            @fn fbe_cli_cmd_getlogs ()                 *
 *************************************************************************
 *
 *  @brief
 *   fbe_cli_cmd_getlogs performs the execution of the getlogs command
 *   to display event log messages.
 *
 * @param :
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   argv     (INPUT) ARGUMENT LIST
 *
 * @return:
 *   None
 *
 * @author
 *   25-October-2010 Created - Vishnu Sharma
 *
 *************************************************************************/

void fbe_cli_cmd_getlogs(fbe_s32_t argc,char ** argv)
{

    /*********************************
    **    VARIABLE DECLARATIONS    **
    *********************************/
    fbe_event_log_statistics_t event_log_statistics = {0};
    fbe_status_t status;
    fbe_u32_t loop_count = 0;
    fbe_package_id_t package_id = FBE_PACKAGE_ID_INVALID;

    fbe_u32_t total_events_copied = 0;
    fbe_event_log_get_event_log_info_t* events_array;
    /*****************
    **    BEGIN    **
    *****************/
    if ((argc == 1 )&&((strcmp(*argv, "-help") == 0) ||
        (strcmp(*argv, "-h") == 0)))
    {
        /* If they are asking for help, just display help and exi t.
            */
        fbe_cli_printf("\n");
        fbe_cli_printf("%s", GETLOGS_USAGE);
        return;
    }
    if (argc != 1 )
    {
        fbe_cli_printf("Invalid arguments .Please check the command \n");
        fbe_cli_printf("%s", GETLOGS_USAGE);
        return;
    }
    if((strcmp(*argv, "phy") == 0) ||
       (strcmp(*argv, "pp") == 0))
    {
        package_id = FBE_PACKAGE_ID_PHYSICAL;
    }
    else if(strcmp(*argv, "sep") == 0)
    {
        package_id = FBE_PACKAGE_ID_SEP_0;
    }
    else if (strcmp(*argv, "esp") == 0)
    {
        package_id = FBE_PACKAGE_ID_ESP;
    }
    else
    {
        fbe_cli_printf("Please enter valid package name \n");
        fbe_cli_printf("%s\n", GETLOGS_USAGE);
        return ;
    } 
    status = fbe_api_get_event_log_statistics(&event_log_statistics,package_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFailed to get event log statistics \n");
        return ;
    }
    if (event_log_statistics.total_msgs_logged == 0)
    {
         fbe_cli_printf("\n No message logged for this package \n");
         return ;
    }

    loop_count=0;
    events_array = (fbe_event_log_get_event_log_info_t *)malloc(sizeof(fbe_event_log_get_event_log_info_t) * FBE_EVENT_LOG_MESSAGE_COUNT);
    status = fbe_api_get_all_events_logs(&events_array[loop_count],&total_events_copied, FBE_EVENT_LOG_MESSAGE_COUNT,
                                                                 package_id);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get events.");
        return;
    }
    for(loop_count =0;loop_count<total_events_copied;loop_count++)
    {
        fbe_cli_printf("%02d:%02d:%02d  ",events_array[loop_count].event_log_info.time_stamp.hour,
                       events_array[loop_count].event_log_info.time_stamp.minute,
                       events_array[loop_count].event_log_info.time_stamp.second);
        fbe_cli_printf("0x%x  ",events_array[loop_count].event_log_info.msg_id);
        fbe_cli_printf("%s",events_array[loop_count].event_log_info.msg_string);

        fbe_cli_printf("\n");
    }

    return ;
}
/**************************************
* end of fbe_cli_cmd_getlogs()
***************************************/

/*************************************************************************
 *                            @fn fbe_cli_cmd_clearlogs ()               *
 *************************************************************************
 *
 *  @brief
 *   fbe_cli_cmd_clearlogs performs the execution of the clearlogs command
 *   to wipe out all event log messages for the given package.
 *
 * @param :
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   argv     (INPUT) ARGUMENT LIST
 *
 * @return:
 *   None
 *
 * @author
 *   28-October-2010 Created - Vishnu Sharma
 *
 *************************************************************************/

void fbe_cli_cmd_clearlogs(fbe_s32_t argc,char ** argv)
{
    /*********************************
    **    VARIABLE DECLARATIONS    **
    *********************************/
    fbe_status_t status;
    fbe_package_id_t package_id = FBE_PACKAGE_ID_INVALID;
    
    /*****************
    **    BEGIN    **
    *****************/
    
    if ((argc == 1 )&&((strcmp(*argv, "-help") == 0) ||
        (strcmp(*argv, "-h") == 0)))
    {
        /* If they are asking for help, just display help and exit.
            */
        fbe_cli_printf("\n");
        fbe_cli_printf("%s", CLEARLOGS_USAGE);
        return;
    }
    if (argc != 1 )
    {
        fbe_cli_printf("Invalid arguments .Please check the command \n");
        fbe_cli_printf("%s", CLEARLOGS_USAGE);
        return;
    }
    if((strcmp(*argv, "phy") == 0) ||
       (strcmp(*argv, "pp") == 0))
    {
        package_id = FBE_PACKAGE_ID_PHYSICAL;
    }
    else if(strcmp(*argv, "sep") == 0)
    {
        package_id = FBE_PACKAGE_ID_SEP_0;
    }
    else if (strcmp(*argv, "esp") == 0)
    {
        package_id = FBE_PACKAGE_ID_ESP;
    }
    else
    {
        fbe_cli_printf("Please enter valid package name \n");
        fbe_cli_printf("%s", CLEARLOGS_USAGE);
        return ;
    } 
    //clear event log statistics
    status = fbe_api_clear_event_log_statistics(package_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFailed to clear event log statistics for the given package, package_id:%d, Status:%d \n",package_id,status);
         return ;
    }
    //clear event log messages
    status = fbe_api_clear_event_log(package_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFailed to clear event logs for the given package, package_id:%d, Status:%d \n",package_id,status);
         return ;
    }
    fbe_cli_printf("Event log messages are cleared successfully for the given package, package_id:%d \n",package_id);
    return ;
}

/**************************************
* end of fbe_cli_cmd_clearlogs()
***************************************/

/************************************************************************
 **                       End of fbe_cli_lib_event_log_cmds ()                **
 ************************************************************************/

