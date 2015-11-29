/***************************************************************************
 *  Copyright (C)  EMC Corporation 2012
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 *  All rights reserved.
 ***************************************************************************/

/*!**************************************************************************
 * @file memory_dps_service_ext.c
 ***************************************************************************
 *
 * @brief
 *   Memory dps service debug extensions.
 *
 * @author
 *  04/05/2011 - Created. Swati Fursule
 *
 ***************************************************************************/

#include "pp_dbgext.h"

//#include "fbe/fbe_topology_interface.h"
//#include "fbe/fbe_raid_common.h"
//#include "fbe_transport_debug.h"
#include "fbe_memory_dps_debug.h"
#include "fbe/fbe_memory.h"

/* ***************************************************************************
 *
 * @brief
 *  Extension to display dps statistics.
 *
 *  Arguments: None
 *  Return Value: None
 * 
 * @version
 *   03/23/2011:  Created. Swati Fursule
 *
 ***************************************************************************/

#pragma data_seg ("EXT_HELP$4dps")
static char CSX_MAYBE_UNUSED usageMsg_dps[] =
"!dps\n"
"  Displays DPS statistics by Pool or DPS priority.\n"
" -pool : DPS memory statistics by Pool\n"
" -priority : DPS memory statistics by DPS-Priority\n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(dps, "dps")
{
    fbe_dbgext_ptr dps_ptr;
    fbe_status_t status;
    const fbe_u8_t * module_name =  "sep";
    fbe_bool_t   status_by_pool = FBE_FALSE;
    fbe_bool_t   status_by_priority = FBE_FALSE;

    if (strlen(args) && strncmp(args,"-pool",5) == 0)
    {
        status_by_pool = FBE_TRUE;
    }
    else if (strlen(args) && strncmp(args,"-priority",9) == 0)
    {
        status_by_priority =FBE_TRUE;
    }

    /* Get the topology table. */
    FBE_GET_EXPRESSION(module_name, dps_pool_stats_p, &dps_ptr);

    status = fbe_memory_dps_statistics_debug_trace(module_name, 
                                                   fbe_debug_trace_func, 
                                                   NULL, 
                                                   dps_ptr,
                                                   4 /*spaces_to_indent*/);




}

/*************************
 * end file memory_dps_service_ext.c
 *************************/
