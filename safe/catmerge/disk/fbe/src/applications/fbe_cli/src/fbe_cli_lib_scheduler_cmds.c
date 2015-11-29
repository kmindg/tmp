/***************************************************************************
 * Copyright (C) EMC Corporation 2009 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_scheduler_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contains cli functions for the scheduler.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *   09/01/2010:  Prafull Parab - created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
#include "fbe_cli_private.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_cli_scheduler_service.h"
#include "fbe/fbe_api_scheduler_interface.h"

static void fbe_cli_scheduler_set_scale(fbe_u32_t new_scale);
static void fbe_cli_scheduler_get_scale(void);

void fbe_cli_scheduler_hook_cmds(int argc , char ** argv)
{
    fbe_status_t status;
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t monitor_state = SCHEDULER_MONITOR_STATE_INVALID;
    fbe_u32_t monitor_substate = FBE_SCHEDULER_MONITOR_SUB_STATE_INVALID;
    fbe_u64_t val1 = 0;
    fbe_u64_t val2 = 0;
    fbe_u32_t check_type = SCHEDULER_CHECK_STATES;
    fbe_u32_t action = SCHEDULER_DEBUG_ACTION_LOG;
    fbe_bool_t b_add = FBE_FALSE;
    fbe_bool_t b_del = FBE_FALSE;
    fbe_bool_t b_get = FBE_FALSE;

    while (argc > 0)
    {
        if(strcmp(*argv, "-add_hook") == 0)
        {
            /* Get the object id from command line. */
            argc--;
            argv++;
            b_add = FBE_TRUE;
        }
        else if(strcmp(*argv, "-del_hook") == 0)
        {
            /* Get the object id from command line. */
            argc--;
            argv++;
            b_del = FBE_TRUE;
        }
        else if(strcmp(*argv, "-get_hook") == 0)
        {
            /* Get the object id from command line. */
            argc--;
            argv++;
            b_get = FBE_TRUE;
        }
        else if(strcmp(*argv, "-object_id") == 0)
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
            object_id = (fbe_object_id_t)strtoul(*argv, 0, 0);
            argc--;
            argv++;
        }
        else if(strcmp(*argv, "-state") == 0)
        {
            /* Get the object id from command line. */
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Please provide value for -monitor_state switch. \n");
                return;
            }
            monitor_state = (fbe_u32_t)strtoul(*argv, 0, 0);
            argc--;
            argv++;
        }
        else if(strcmp(*argv, "-substate") == 0)
        {
            /* Get the object id from command line. */
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Please provide value for -monitor_substate switch. \n");
                return;
            }
            monitor_substate = (fbe_u32_t)strtoul(*argv, 0, 0);
            argc--;
            argv++;
        }
        else if(strcmp(*argv, "-val1") == 0)
        {
            /* Get the object id from command line. */
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Please provide value for -val1 switch. \n");
                return;
            }
            val1 = (fbe_u64_t)strtoul(*argv, 0, 0);
            argc--;
            argv++;
        }
        else if(strcmp(*argv, "-val2") == 0)
        {
            /* Get the object id from command line. */
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Please provide value for -val2 switch. \n");
                return;
            }
            val2 = (fbe_u32_t)strtoul(*argv, 0, 0);
            argc--;
            argv++;
        }
        else if(strcmp(*argv, "-check_type") == 0)
        {
            /* Get the object id from command line. */
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Please provide value for -check_type switch. \n");
                fbe_cli_error("Valid values are: check_states, vals_eq, vals_lt, vals_gt\n"); 
                return;
            }
            if (strcmp(*argv, "check_states") == 0)
            {
                check_type = SCHEDULER_CHECK_STATES;
            }
            else if (strcmp(*argv, "vals_eq") == 0)
            {
                check_type = SCHEDULER_CHECK_VALS_EQUAL;
            }
            else if (strcmp(*argv, "vals_lt") == 0)
            {
                check_type = SCHEDULER_CHECK_VALS_LT;
            }
            else if (strcmp(*argv, "vals_gt") == 0)
            {
                check_type = SCHEDULER_CHECK_VALS_GT;
            }
            else
            {
                fbe_cli_error("Unknown argument for -check_type switch. \n");
                return;
            }
            argc--;
            argv++;
        }
        else if(strcmp(*argv, "-action") == 0)
        {
            /* Get the object id from command line. */
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Please provide value for -action switch. \n");
                fbe_cli_error("Valid values are: log, continue, pause, clear\n"); 
                return;
            }
            if (strcmp(*argv, "log") == 0)
            {
                action = SCHEDULER_DEBUG_ACTION_LOG;
            }
            else if (strcmp(*argv, "continue") == 0)
            {
                action = SCHEDULER_DEBUG_ACTION_CONTINUE;
            }
            else if (strcmp(*argv, "pause") == 0)
            {
                action = SCHEDULER_DEBUG_ACTION_PAUSE;
            }
            else if (strcmp(*argv, "clear") == 0)
            {
                action = SCHEDULER_DEBUG_ACTION_CLEAR;
            }
            else
            {
                fbe_cli_error("Unknown argument for -action switch. \n");
                return;
            }
            argc--;
            argv++;
        }
        else 
        {
            fbe_cli_error("Unknown argument\n");
            return;
        }
    }
    
    fbe_cli_printf("scheduler hook input arguments\n");
    fbe_cli_printf("object_id: 0x%x\n", object_id);
    fbe_cli_printf("state: 0x%x\n", monitor_state);
    fbe_cli_printf("substate: 0x%x\n", monitor_substate);
    fbe_cli_printf("val1: %llx val2: %llx\n", val1, val2);
    fbe_cli_printf("check_type: 0x%x\n", check_type);
    fbe_cli_printf("action: 0x%x\n\n\n", action);

    if (b_add)
    {
        fbe_cli_printf("Adding debug hook\n");
        status = fbe_api_scheduler_add_debug_hook(object_id,
                                                  monitor_state,
                                                  monitor_substate,
                                                  val1,
                                                  val2,
                                                  check_type,
                                                  action);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_error("Failed to add debug hook with status: %u\n", status);
            return;
        }
        
        fbe_cli_printf("Hook added successfully\n");
    }
    else if (b_del)
    {
        fbe_cli_printf("Deleting debug hook\n");
        status = fbe_api_scheduler_del_debug_hook(object_id,
                                                  monitor_state,
                                                  monitor_substate,
                                                  val1,
                                                  val2,
                                                  check_type,
                                                  action);

        if (status != FBE_STATUS_OK)
        {
            fbe_cli_error("Failed to add debug hook with status: %u\n", status);
            return;
        }
        fbe_cli_printf("Hook deleted successfully\n");
    }
    else if (b_get)
    {
        fbe_scheduler_debug_hook_t hook;
        hook.action = action;
        hook.check_type = check_type;
        hook.monitor_state = monitor_state;
        hook.monitor_substate = monitor_substate;
        hook.object_id = object_id;
        hook.val1 = val1;
        hook.val2 = val2;

        status = fbe_api_scheduler_get_debug_hook(&hook);

        if (status != FBE_STATUS_OK)
        {
            fbe_cli_error("Failed to add debug hook with status: %u\n", status);
            return;
        }
        fbe_cli_printf("object_id:  0x%x\n", hook.object_id);
        fbe_cli_printf("state:      0x%x\n", hook.monitor_state);
        fbe_cli_printf("substate:   0x%x\n", hook.monitor_substate);
        fbe_cli_printf("val1:       %llx \n", hook.val1);
        fbe_cli_printf("val2:       %llx\n", hook.val2);
        fbe_cli_printf("check_type: 0x%x\n", hook.check_type);
        fbe_cli_printf("action:     0x%x\n", hook.action);
        fbe_cli_printf("counter:     0x%llx\n", hook.counter);
    }
    else
    {
        fbe_cli_error("No option provided. -add_hook, -del_hook, or -get_hook expected\n");
    }
}
/*!**************************************************************
 * fbe_cli_cmd_scheduler()
 ****************************************************************
 * @brief
 *   This function will control some parameters in the schduler
 *  
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 ****************************************************************/

void fbe_cli_cmd_scheduler (int argc , char ** argv)
{
	fbe_u32_t	scale = 0;

	if (argc == 0) {
		fbe_cli_printf("Must have arguments\n");
		return;
	}
    
    if ((argc > 0) && ((strcmp(*argv, "-help") == 0) || (strcmp(*argv, "-h") == 0))){
		/* If they are asking for help, just display help and exit.
		 */
		fbe_cli_printf("%s", FBE_SCHEDULER_USAGE);
		return;
	}

	if (strcmp(*argv, "-set_scale") == 0){
		if (argc == 1){
			fbe_cli_printf("Must have arguments\n");
			return;
		}
		argv++;
		scale = (fbe_u32_t)strtoul(*argv, 0, 0);
		fbe_cli_scheduler_set_scale(scale);
		return;
	}

	if (strcmp(*argv, "-get_scale") == 0){
        fbe_cli_scheduler_get_scale();
		return;
	}

	if (strcmp(*argv, "-add_hook") == 0 ||
        strcmp(*argv, "-del_hook") == 0 ||
        strcmp(*argv, "-get_hook") == 0){
        fbe_cli_scheduler_hook_cmds(argc, argv);
		return;
	}

	
	fbe_cli_printf("Scheduler command option not supported\n");
	

}

static void fbe_cli_scheduler_set_scale(fbe_u32_t new_scale)
{
	fbe_scheduler_set_scale_t	set_scale;
	fbe_status_t				status;

	set_scale.scale = new_scale;

	if (new_scale < 75) {
		fbe_cli_printf("\nSetting the scale too low can significantly reduce the speed of background activities\n");
	}

	status = fbe_api_scheduler_set_scale(&set_scale);
	if (status != FBE_STATUS_OK) {
		fbe_cli_printf("Error in setting scale to:%d, error status: %d !\n", new_scale, status);
	}else{
		fbe_cli_printf("\nScheduler scale was set to:%d\n\n", new_scale);
	}
}

static void fbe_cli_scheduler_get_scale()
{
	fbe_scheduler_set_scale_t	set_scale;
	fbe_status_t				status;

    status = fbe_api_scheduler_get_scale(&set_scale);
	if (status != FBE_STATUS_OK) {
		fbe_cli_printf("Error in getting scale ,error status: %d !\n", status);
	}else{
		fbe_cli_printf("\nScheduler scale is:%d\n\n", (fbe_u32_t)set_scale.scale);
	}
}
