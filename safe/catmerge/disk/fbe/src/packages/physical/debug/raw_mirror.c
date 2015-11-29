/***************************************************************************
 *  Copyright (C)  EMC Corporation 2000-11
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/*!**************************************************************************
 * @file raw_mirror.c
 ***************************************************************************
 *
 * @brief
 *  This file contains debugging commands about raw mirror
.*
 * @version
 *   25-Oct-2011:  Created. Trupti Ghate
 *
 ***************************************************************************/

#include "pp_dbgext.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_class.h"
#include "fbe_raw_mirror_debug.h"

/* ***************************************************************************
 *
 * @brief
 *  Debug macro to display the raw mirror information of database service and non paged metadata service.
 *
 *  Arguments: None
 *  Return Value: None
 * 
 * @version
 *   15-Nov-2011:  Created. Trupti Ghate
 *
 ***************************************************************************/
#pragma data_seg ("EXT_HELP$4raw_mirror")
static char CSX_MAYBE_UNUSED usageMsg_raw_mirror[] =
"!raw_mirror [-db] | [-npmetadata]\n"
"  Displays all the database service and nonpaged metadata raw mirror related information.\n"
"  By default we show all the information\n"
"  -db or -d  - Displays database service raw mirror information.\n"
"  -npmetadata or -n - Displays database service raw mirror information.\n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(raw_mirror, "raw_mirror")
{
    fbe_dbgext_ptr raw_mirror_ptr = 0;
    fbe_bool_t mirror_initialized = FBE_FALSE;
    fbe_bool_t flag_db_service = FBE_TRUE;
    fbe_bool_t flag_npmetadata_service = FBE_TRUE;
    fbe_bool_t flag_homewrecker_db = FBE_TRUE;
    fbe_char_t *str = NULL;

    /* We either are in simulation or on hardware.
     * Depending on where we are, use a different module name. 
     * We validate the module name by checking the ptr size. 
     */
    const fbe_u8_t * module_name;
    module_name = "sep";

    /* Check if user has specified any one option */
    str = strtok((char*)args, " \t");
    if(str != NULL)
    {
         if(strncmp(str, "-db", 3) == 0 || strncmp(str, "-d", 2) == 0)
         {
              flag_db_service = FBE_TRUE;
              flag_npmetadata_service = FBE_FALSE;
              flag_homewrecker_db = FBE_FALSE;
         }
        if(strncmp(str, "-npmetadata", 11) == 0 || strncmp(str, "-n", 2) == 0)
         {
              flag_npmetadata_service = FBE_TRUE;
              flag_db_service = FBE_FALSE;
              flag_homewrecker_db = FBE_FALSE;
         }
        if(strncmp(str, "-homewrecker", 12) == 0 || strncmp(str, "-h", 2) == 0)
        {
            flag_homewrecker_db = FBE_TRUE;
            flag_npmetadata_service = FBE_FALSE;
            flag_db_service = FBE_FALSE;
        }
    }
     /* Based on arguments, display the respective raw mirror */
     while(flag_db_service == FBE_TRUE || flag_npmetadata_service == FBE_TRUE || flag_homewrecker_db == FBE_TRUE)
     {
         if(flag_db_service == FBE_TRUE)        /* Get the database service raw mirror ptr. */
         {
             flag_db_service = FBE_FALSE;
             fbe_debug_trace_func(NULL, "******* fbe_database_service_raw_mirror ********** \n");
             FBE_GET_EXPRESSION(module_name, fbe_database_system_db , &raw_mirror_ptr);
             if(raw_mirror_ptr == 0)
             {
                 fbe_trace_indent(fbe_debug_trace_func, NULL, 4);
                 fbe_debug_trace_func(NULL, "fbe_database_service_raw_mirror_ptr is not available \n");
                 return;
             }
        }
        else if(flag_npmetadata_service == FBE_TRUE)         /* Get the non paged metadata service raw mirror ptr. */
        {
             flag_npmetadata_service = FBE_FALSE;
             fbe_debug_trace_func(NULL, "\n******* fbe_metadata_nonpaged_raw_mirror ********** \n");
        
             FBE_GET_EXPRESSION(module_name, fbe_metadata_nonpaged_raw_mirror , &raw_mirror_ptr);
             if(raw_mirror_ptr == 0)
             {
                 fbe_trace_indent(fbe_debug_trace_func, NULL, 4);
                 fbe_debug_trace_func(NULL, "fbe_metadata_nonpaged_raw_mirror_ptr is not available \n");
                 return;
             }
         else if(flag_homewrecker_db== FBE_TRUE)         /* Get the homewrecker db raw mirror ptr. */
        {
             flag_homewrecker_db = FBE_FALSE;
             fbe_debug_trace_func(NULL, "\n******* fbe_database_homewrecker_db ********** \n");
        
             FBE_GET_EXPRESSION(module_name, fbe_database_homewrecker_db, &raw_mirror_ptr);
             if(raw_mirror_ptr == 0)
             {
                 fbe_trace_indent(fbe_debug_trace_func, NULL, 4);
                 fbe_debug_trace_func(NULL, "fbe_database_homewrecker_db_ptr is not available \n");
                 return;
             }
         }
     }

         /* Display the respective raw mirror only if initialized flag is True */
         FBE_GET_FIELD_DATA(module_name, 
                            raw_mirror_ptr,
                            fbe_raw_mirror_s,
                            b_initialized,
                            sizeof(fbe_bool_t),
                            &mirror_initialized);
        if(mirror_initialized == FBE_TRUE)
         {
             fbe_debug_trace_func(NULL, " b_initialized: %d\n", mirror_initialized);
             fbe_raw_mirror_debug(module_name, raw_mirror_ptr, fbe_debug_trace_func, NULL);
         }
         else
         {
             fbe_trace_indent(fbe_debug_trace_func, NULL, 4);
             fbe_debug_trace_func(NULL, "This raw_mirror is not initialized\n");
          }
    }
    return;
}
/* end of raw_mirror.c */
