/***************************************************************************
 *  Copyright (C)  EMC Corporation 2010
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/*!**************************************************************************
 * @file job_service.c
 ***************************************************************************
 *
 * @brief
 *  This file contains debugging commands for the Job service.
.*
 * @version
 *   23-Dec-2010:  Created. Harisingh Chauhan
 *
 ***************************************************************************/

#include "pp_dbgext.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_sps_mgmt_debug.h"
#include "fbe_base_object_debug.h"
#include "fbe/fbe_devices.h"
#include "fbe_base_environment_debug.h"
#include "fbe_job_service_debug.h"
#include "fbe_topology_debug.h"


/* ***************************************************************************
 *
 * @brief
 *  Extension to display Job Service Information.
 *
 *  Arguments: None
 *  Return Value: None
 * 
 * @version
 *   23-Dec-2010:  Created. Hari Singh Chauhan
 *
 ***************************************************************************/
#pragma data_seg ("EXT_HELP$4job_service")
static char CSX_MAYBE_UNUSED usageMsg_job_service[] =
"!job_service\n"
"   Display information for the job service.\n"
"   Usage !job_service -qtype <queue type> -qelement <number of elements to display>"
"   Where option -qtype displays information for specified queue type.\n"
"    -qtype : \n"
"        1  FBE_JOB_RECOVERY_QUEUE\n"
"        2  FBE_JOB_CREATION_QUEUE\n"
"   Option -qelement displays information about the specified number of queue elements(Maximum 15 elements).\n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(job_service, "job_service")
{
    fbe_char_t* str = NULL;
    fbe_u64_t last_queue_element = 0;
    fbe_job_control_queue_type_t queue_type = FBE_JOB_INVALID_QUEUE;

    str = strtok((char*)args," \t");
    if(str != NULL)
    {
        if(strncmp(str, "-qelement", 9) == 0)
        {
            str = strtok(NULL," \t");
            if(str != NULL)
            {
                fbe_debug_trace_func(NULL, "filter only %lu queue element from job service\n", strtoul(str, 0, 0));
                last_queue_element = (fbe_u64_t)strtoul(str, 0, 0);

                if(last_queue_element > 15 || last_queue_element <= 0)
                {
                    fbe_debug_trace_func(NULL, "Please provide number for displaying queue elements between 1 to 15\n");
                    return;
                }
                str = strtok(NULL," \t");
            }
            else
            {
                fbe_debug_trace_func(NULL, "Please provide number for displaying queue elements between 1 to 15\n");
                return;
            }
        }
    
        if(str != NULL && strncmp(str, "-qtype", 6) == 0)
        {
            str = strtok(NULL," \t");
            if(str != NULL)
            {
                fbe_debug_trace_func(NULL, "filter by job type: %lu \n",strtoul(str, 0, 0));
                queue_type = (fbe_job_control_queue_type_t)strtoul(str, 0, 0);

                if(!(queue_type == FBE_JOB_RECOVERY_QUEUE || queue_type == FBE_JOB_CREATION_QUEUE))
                {
                    fbe_debug_trace_func(NULL, "Unknown queue type\n");
                    fbe_debug_trace_func(NULL, "Please provide valid queue type:\nFBE_JOB_RECOVERY_QUEUE    %d \nFBE_JOB_CREATION_QUEUE    %d\n",
                                                          FBE_JOB_RECOVERY_QUEUE,FBE_JOB_CREATION_QUEUE);
                    return;
                }
            }
            else
            {
                fbe_debug_trace_func(NULL, "Please provide valid queue type:\nFBE_JOB_RECOVERY_QUEUE    %d \nFBE_JOB_CREATION_QUEUE    %d\n",
                                                          FBE_JOB_RECOVERY_QUEUE,FBE_JOB_CREATION_QUEUE);
                return;
            }
        }
    }

    if(!strcmp(pp_ext_module_name, "sep"))
    {
        fbe_job_service_debug_trace(pp_ext_module_name, fbe_debug_trace_func, NULL, last_queue_element, queue_type);
        fbe_debug_trace_func(NULL, "\n");
    }
    else
    {
        fbe_debug_trace_func(NULL, "Please set_module_name_sep \n");
    }
}

//end of esp_commands.c
