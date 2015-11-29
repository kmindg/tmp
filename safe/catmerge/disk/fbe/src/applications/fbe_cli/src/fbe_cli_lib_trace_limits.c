/***************************************************************************
 * Copyright (C) EMC Corporation 2011 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!*************************************************************************
 * @file fbe_cli_src_trace_limits.c
 ***************************************************************************
 *
 * @brief
 *  This file contains cli functions for the cli trace limits.
 *
 * @ingroup fbe_cli
 *
 * @date
 *  1/10/2011 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
#include "fbe_cli_private.h"
#include "fbe/fbe_api_common.h"
#include "fbe_cli_trace_limits.h"

static void fbe_cli_trace_limits_display(fbe_package_id_t package_id);

/*!**************************************************************
 * fbe_cli_trace_limits()
 ****************************************************************
 * @brief
 *  Displays or sets the trace limits.
 *  
 *  @param    argc - argument count
 *  @param    argv - argument string
 * 
 * @notes
 *  
 * @return - none.  
 *
 * @author
 *  1/10/2011 - Created. Rob Foley
 *
 ****************************************************************/
        
void fbe_cli_trace_limits(int argc, char ** argv)
{
    fbe_status_t status;
    fbe_trace_level_t trace_level = FBE_TRACE_LEVEL_INVALID;
    fbe_api_trace_error_limit_action_t action = FBE_API_TRACE_ERROR_LIMIT_ACTION_INVALID;
    fbe_u32_t error_count = FBE_U32_MAX;
    fbe_package_id_t package_id = FBE_PACKAGE_ID_SEP_0;
    fbe_bool_t b_action_specified = FBE_FALSE;
    fbe_bool_t b_display = FBE_FALSE;

    if (argc == 0)
    {
        fbe_cli_error("trace_limits: Not enough arguments to trace limits\n");
        fbe_cli_printf("%s", TRACE_LIMITS_CMD_USAGE);
        return;
    }
    
    /* Parse the command line arguments.
     */
    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /* If they are asking for help, just display help and exit.
             */
            fbe_cli_printf("%s", TRACE_LIMITS_CMD_USAGE);
            return;
        }
        else if (strcmp(*argv, "-d") == 0)
        {
            b_display = FBE_TRUE;
        }
        else if ((strcmp(*argv, "-level") == 0) ||
                 (strcmp(*argv, "-l") == 0))
        {
            /* Set the level this limit will apply to.
             */
            argc--;
            argv++;
            trace_level = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if ((strcmp(*argv, "-stop") == 0) ||
                 (strcmp(*argv, "-s") == 0))
        {
            /* Set the action to stop the system.
             */
            action = FBE_API_TRACE_ERROR_LIMIT_ACTION_STOP_SYSTEM;
            b_action_specified = FBE_TRUE;
        }
        else if ((strcmp(*argv, "-disable") == 0) ||
                 (strcmp(*argv, "-dis") == 0))
        {
            /* Set the action to stop the system.
             */
            action = FBE_API_TRACE_ERROR_LIMIT_ACTION_INVALID;
            b_action_specified = FBE_TRUE;
        }
        else if ((strcmp(*argv, "-trace") == 0) ||
                 (strcmp(*argv, "-t") == 0))
        {
            /* Set action to trace.
             */
            action = FBE_API_TRACE_ERROR_LIMIT_ACTION_TRACE;
            b_action_specified = FBE_TRUE;
        }
        else if ((strcmp(*argv, "-error_count") == 0) ||
                 (strcmp(*argv, "-e") == 0))
        {
            /* Set number of errors before limit hits.
             */
            argc--;
            argv++;
            error_count = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if ((strcmp(*argv, "-package") == 0) ||
                 (strcmp(*argv, "-p") == 0))
        {
            /* Filter by one object id.
             */
            argc--;
            argv++;
            if (argc == 0)
            {
                fbe_cli_error("-package, expected package_id, too few arguments \n");
                return;
            }
            if ((strcmp(*argv, "phy") == 0) ||
                (strcmp(*argv, "pp") == 0))
            {
                package_id = FBE_PACKAGE_ID_PHYSICAL;
            }
            else if (strcmp(*argv, "sep") == 0)
            {
                package_id = FBE_PACKAGE_ID_SEP_0;
            }
            else if (strcmp(*argv, "esp") == 0)
            {
                package_id = FBE_PACKAGE_ID_ESP;
            }
            else
            {
                fbe_cli_error("-package, expected package id (phy,pp,sep,esp) not found. \n");
            }
        } /* end handle -package */

        argc--;
        argv++;

    } /* end of while */

    /* We process the commands outside the loop to allow all arguments to be processed 
     * before we get to this point. 
     */
    if (b_display)
    {
        fbe_cli_trace_limits_display(package_id);
        return;
    }

    if (b_action_specified)
    {
        /* If trace level is invalid, then set both supported levels.
         */
        if (trace_level == FBE_TRACE_LEVEL_INVALID)
        {
            if (error_count == FBE_U32_MAX)
            {
                /* If error count is invalid, make it one.
                 */
                error_count = 1;
            }
            fbe_cli_printf("Set trace limit level: %d action: %d err_count: %d package: %d\n",
                           FBE_TRACE_LEVEL_ERROR, action, error_count, package_id);
            status = fbe_api_trace_set_error_limit(FBE_TRACE_LEVEL_ERROR, action, error_count, package_id);
            if (status == FBE_STATUS_OK) 
            { 
                fbe_cli_printf("Set trace limit level: %d action: %d err_count: %d package: %d\n",
                               FBE_TRACE_LEVEL_CRITICAL_ERROR, action, error_count, package_id);
                status = fbe_api_trace_set_error_limit(FBE_TRACE_LEVEL_CRITICAL_ERROR, action, error_count, package_id);
            }
        }
        else
        {
            fbe_cli_printf("Set trace limit level: %d action: %d err_count: %d package: %d\n",
                           trace_level, action, error_count, package_id);
            status = fbe_api_trace_set_error_limit(trace_level, action, error_count, package_id);
        }

        if (status == FBE_STATUS_OK) 
        { 
            fbe_cli_printf("Set trace limit completed with success for pkg: %d\n", package_id);
        }
        else
        {
            fbe_cli_error("ERROR: Set trace limit completed with status: 0x%x ofr pkg: %d\n", status, package_id);
        }
    }
    return;
}
/**************************************
 * end fbe_cli_trace_limits()
 **************************************/

/*!**************************************************************
 * fbe_cli_trace_limits_display()
 ****************************************************************
 * @brief
 *  Display a single error limits record.
 *  
 * @param rec_p - Pointer to record to display.
 *
 * @return - none.  
 *
 * @author
 *  1/10/2011 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_cli_trace_limits_display_record(fbe_api_trace_get_error_limit_record_t *rec_p)
{
    const char *action_p = NULL;
    switch (rec_p->action)
    {
        case FBE_API_TRACE_ERROR_LIMIT_ACTION_STOP_SYSTEM:
            action_p = "stop system";
            break;
        case FBE_API_TRACE_ERROR_LIMIT_ACTION_TRACE:
            action_p = "trace";
            break;
        case FBE_API_TRACE_ERROR_LIMIT_ACTION_INVALID:
            action_p = "invalid";
            break;
        default:
            action_p = "unknown";
            break;
    };
    fbe_cli_printf("  num_errors: %d\n", rec_p->num_errors);
    fbe_cli_printf("  error_limit: %d\n", rec_p->error_limit);
    fbe_cli_printf("  action: %s (%d)\n", action_p, rec_p->action);
    return;
}
/**************************************
 * end fbe_cli_trace_limits_display_record()
 **************************************/
/*!**************************************************************
 * fbe_cli_trace_limits_display()
 ****************************************************************
 * @brief
 *  Display the trace limits for the given package
 *  
 *  @param  package_id - Current package to get trace limits for.
 *
 * @return - none.  
 *
 * @author
 *  1/10/2011 - Created. Rob Foley
 *
 ****************************************************************/

static void fbe_cli_trace_limits_display(fbe_package_id_t package_id)
{
    fbe_status_t status;
    fbe_api_trace_get_error_limit_t error_limits;

    status = fbe_api_trace_get_error_limit(package_id, &error_limits);

    if (status == FBE_STATUS_OK)
    {
        fbe_cli_printf("Get trace limit completed with success for pkg: %d\n", package_id);
        fbe_cli_printf("Error level limits: \n");
        fbe_cli_trace_limits_display_record(&error_limits.records[FBE_TRACE_LEVEL_ERROR]);
        fbe_cli_printf("Critical Error level limits: \n");
        fbe_cli_trace_limits_display_record(&error_limits.records[FBE_TRACE_LEVEL_CRITICAL_ERROR]);
    }
    else
    {
        fbe_cli_error("ERROR: Get trace limit completed with status: 0x%x for pkg: %d\n", status, package_id);
    }

    return;
}
/**************************************
 * end fbe_cli_trace_limits_display()
 **************************************/

/*************************
 * end file fbe_cli_src_trace_limits.c
 *************************/
