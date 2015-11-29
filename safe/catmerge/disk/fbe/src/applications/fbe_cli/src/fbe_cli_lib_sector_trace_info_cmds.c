/**************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 *************************************************************************/

/*!************************************************************************
* @file fbe_cli_lib_sector_trace_info_cmds.c
 **************************************************************************
 *
 * @brief
 *  This file contains cli functions for the RGINFO related features in FBE CLI
 *  This new command has following syntax
 *  sector_trace_info|stinfo -v|-verbose  -set_trace_level|stl <value>
 *      -set_trace_mask|-stm <value> -reset_counters -display_counters
 * 
 * @ingroup fbe_cli
 *
 * @date
 *  8/10/2010 - Created. Swati Fursule
 *
 **************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
#include "fbe_cli_private.h"
#include "fbe_trace.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_api_utils.h"
#include "fbe_api_sector_trace_interface.h"
#include "fbe_cli_sector_trace_info.h"

void fbe_cli_sector_trace_info_parse_cmds(int argc, char** argv);
/*!*******************************************************************
 * @var fbe_cli_sector_trace_info
 *********************************************************************
 * @brief Function to implement sector_trace_info commands.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  08/10/2010 - Created. Swati Fursule
 *********************************************************************/
void fbe_cli_sector_trace_info(int argc, char** argv)
{
    fbe_cli_printf("%s", "\n");
    fbe_cli_sector_trace_info_parse_cmds(argc, argv);
    fbe_cli_printf("%s", "\n");

    return;
}
/******************************************
 * end fbe_cli_sector_trace_info()
 ******************************************/
 
/*!**************************************************************
 *          fbe_cli_sector_trace_info_parse_cmds()
 ****************************************************************
 *
 * @brief   Parse rginfo commands
 *  sector_trace_info|stinfo -v|-verbose  -set_trace_level|stl <value>
 *     -set_trace_mask|-stm <value> -reset_counters -display_counters
 * @param   argument count
*  @param arguments list
 *
 * @return  None.
 *
 * @author
 *  08/10/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_sector_trace_info_parse_cmds(int argc, char** argv)
{
    fbe_status_t                     status = FBE_STATUS_OK;
    fbe_bool_t                       detail = FBE_FALSE;
    fbe_bool_t persist = FBE_FALSE;
    fbe_bool_t stop_system_on_error = FBE_FALSE;
    /* 
     * Parse the sector_trace_info commands and call particular functions
     */
    if (argc == 0)
    {
        status = fbe_api_sector_trace_display_info(FBE_FALSE);
        if ( FBE_STATUS_OK != status)
        {
            fbe_cli_printf("stinfo ERROR: "
                           "Get sector trace information failed\n");
        }
        else
        {
            fbe_cli_printf("stinfo SUCCESS: "
                           "Get sector trace information succeeded\n");
        }

        return;
    }
    if ((strcmp(*argv, "-help") == 0) ||
        (strcmp(*argv, "-h") == 0) )
    {
        /* If they are asking for help, just display help and exit.
        */
        fbe_cli_printf("%s", SECTOR_TRACE_INFO_CMD_USAGE);
        return;
    }
    /* Parse args
     */
    while (argc && **argv == '-')
    {
        /* Display the raid group information. 
         */
        if (!strcmp(*argv, "-verbose") || 
            !strcmp(*argv, "-v") )
        {
            argc--;
            argv++;
            detail = FBE_TRUE;
        }
        if ( argc == 0)
        {
            status = fbe_api_sector_trace_display_info(detail);
            if ( FBE_STATUS_OK != status)
            {
                fbe_cli_printf("stinfo ERROR: "
                               "Get sector trace information failed\n");
            }
            else
            {
                fbe_cli_printf("stinfo SUCCESS: "
                               "Get sector trace information succeeded\n");
            }
            return;
        }
        if (!strcmp(*argv, "-set_trace_level") || 
            !strcmp(*argv, "-stl") )
        {
            fbe_sector_trace_error_level_t new_sector_trace_level;
            argc--;
            if ( argc == 0)
            {
                status = fbe_api_sector_trace_restore_default_level();
                if ( FBE_STATUS_OK == status)
                {
                    fbe_cli_printf("stinfo SUCCESS: "
                                   "Restore trace level succeeded\n");
                }
            }
            else
            {
                fbe_char_t *tmp_ptr;
                argv++;
                argc--;
                /* Set the sector tracing trace level.
                 */
                /* get the value*/
                tmp_ptr = *argv;
                if ((*tmp_ptr == '0') && 
                    (*(tmp_ptr + 1) == 'x' || *(tmp_ptr + 1) == 'X'))
                {
                    tmp_ptr += 2;
                }
                new_sector_trace_level = fbe_atoh(*argv);
                /*!todo : accept multiple flags value*/
                if ((new_sector_trace_level < 0) || 
                    (new_sector_trace_level > FBE_SECTOR_TRACE_ERROR_LEVEL_MAX))
                {
                    fbe_cli_printf("stinfo ERROR: "
                                   "Invalid argument (sector_trace_level).\n");
                    fbe_cli_printf("%s", SECTOR_TRACE_INFO_CMD_USAGE);
                    return;
                }
				
                /*We need to further check if -persist is specified*/
                if(argc > 0 ) {
                    argc--;
                    argv++;
                    if(!strcmp(*argv, "-persist")){
                         persist = FBE_TRUE;
                    }
                }
                status = fbe_api_sector_trace_set_trace_level(
                                                  new_sector_trace_level,persist);
                if ( FBE_STATUS_OK == status)
                {
                    fbe_cli_printf("stinfo SUCCESS: "
                                   "Set trace level succeeded\n");
                }
            }
            if ( FBE_STATUS_OK != status)
            {
                fbe_cli_printf("stinfo ERROR: "
                               "Set / Restore trace level failed\n");
            }
        }
        else if (!strcmp(*argv, "-set_trace_mask") || 
                 !strcmp(*argv, "-stm")               )
        {
            fbe_sector_trace_error_flags_t new_error_flags;
            argc--;
            if ( argc == 0)
            {
                status = fbe_api_sector_trace_restore_default_flags();
                if ( FBE_STATUS_OK == status)
                {
                    fbe_cli_printf("stinfo SUCCESS: "
                                   "Restore trace flags/mask succeeded\n");
                }
            }
            else
            {
                fbe_char_t *tmp_ptr;
                argc--;
                argv++;
                /* Set the default raid library debug flag.
                 */
                /* get the value*/
                tmp_ptr = *argv;
                if ((*tmp_ptr == '0') && 
                    (*(tmp_ptr + 1) == 'x' || *(tmp_ptr + 1) == 'X'))
                {
                    tmp_ptr += 2;
                }
                new_error_flags = fbe_atoh(tmp_ptr);
                if ((new_error_flags & ~FBE_SECTOR_TRACE_ERROR_FLAG_MASK) != 0) 
                {
                    fbe_cli_printf("stinfo ERROR: "
                                   "Invalid argument ( trace_flags/mask).\n");
                    fbe_cli_printf("%s", SECTOR_TRACE_INFO_CMD_USAGE);
                    return;
                }
				
                /*We need to further check if -persist is specified*/
                if(argc > 0 ) {
                    argc--;
                    argv++;
                    if(!strcmp(*argv, "-persist")){
                       persist = FBE_TRUE;
                    }
                }
                status = fbe_api_sector_trace_set_trace_flags(
                                                 new_error_flags,persist);
                if ( FBE_STATUS_OK == status)
                {
                    fbe_cli_printf("stinfo SUCCESS: "
                                   "Set trace flags/mask succeeded\n");
                }
            }
            if ( FBE_STATUS_OK != status)
            {
                fbe_cli_printf("stinfo ERROR: "
                               "Set / Restore  trace flags/mask failed\n");
            }
        }
        else if (!strcmp(*argv, "-reset_counters") )
        {
            argc--;
            status = fbe_api_sector_trace_reset_counters();
            if ( FBE_STATUS_OK != status)
            {
                fbe_cli_printf("stinfo ERROR: Reset counters failed\n");
            }
            else
            {
                fbe_cli_printf("stinfo SUCCESS: Reset counters succeeded\n");
            }
        }
        else if (!strcmp(*argv, "-display_counters") )
        {
            argc--;
            status = fbe_api_sector_trace_display_counters(detail);
            if ( FBE_STATUS_OK != status)
            {
                fbe_cli_printf("stinfo ERROR: Display counters failed\n");
            }
            else
            {
                fbe_cli_printf("stinfo SUCCESS: Display counters succeeded\n");
            }
        }
        else if (!strcmp(*argv, "-set_stop_on_error_flags") || 
                 !strcmp(*argv, "-ssoef")                      )
        {
            fbe_sector_trace_error_flags_t stop_on_error_flags = 0;
            fbe_u32_t additional_event = 0;

            /* Only allowed in engineering mode
            */
            if (operating_mode != ENG_MODE) 
            {
                fbe_cli_printf("stinfo FAILURE: "
                               "-stop_on_error_on_error Only allowed in engineering mode\n");
                return;
            }
            argc--;
            if ( argc == 0)
            {
                status = fbe_api_sector_trace_set_stop_on_error_flags(0,0,FBE_FALSE);
                if ( FBE_STATUS_OK == status)
                {
                    fbe_cli_printf("stinfo SUCCESS: "
                                   "Restore stop on error flags succeeded\n");
                }
                status = fbe_api_sector_trace_set_stop_on_error(FBE_FALSE);
                if ( FBE_STATUS_OK == status)
                {
                    fbe_cli_printf("stinfo SUCCESS: "
                                   "Clear stop on error succeeded, will not change persist status, pls add -persist to clear registry if you need\n");
                }
            }
            else
            {
                fbe_char_t *tmp_ptr;
                argc--;
                argv++;
                /* Set the default raid library debug flag.
                 */

				/* get the value*/
                tmp_ptr = *argv;
                if ((*tmp_ptr == '0') && 
                    (*(tmp_ptr + 1) == 'x' || *(tmp_ptr + 1) == 'X'))
                {
                    tmp_ptr += 2;
                }
                stop_on_error_flags = fbe_atoh(tmp_ptr);
                if ((stop_on_error_flags & ~FBE_SECTOR_TRACE_ERROR_FLAG_MASK) != 0)
                {
                    fbe_cli_printf("stinfo ERROR: "
                                   "Invalid argument ( trace_flags/mask).\n");
                    fbe_cli_printf("%s", SECTOR_TRACE_INFO_CMD_USAGE);
                    return;
                }
				/*We need to further check if -event is specified*/
                if(argc > 0 ) {
                     argc--;
                     argv++;
                     if(!strcmp(*argv, "-event")){
                        argc--;
                        argv++;
                        /* get the value*/
                        tmp_ptr = *argv;
                        if ((*tmp_ptr == '0') && 
                        (*(tmp_ptr + 1) == 'x' || *(tmp_ptr + 1) == 'X'))
                        {
                            tmp_ptr += 2;
                         }
                        additional_event = fbe_atoh(tmp_ptr);
									/*We need to further check if -persist is specified*/
                        if(argc > 0 ) {
                          argc--;
                          argv++;
                          if(!strcmp(*argv, "-persist")){
                            persist = FBE_TRUE;
                          }
                        }
                     }
                     else if(!strcmp(*argv, "-persist")){
                        persist = FBE_TRUE;
                    }
                }

				
                status = fbe_api_sector_trace_set_stop_on_error_flags(stop_on_error_flags,additional_event,persist);
                if ( FBE_STATUS_OK == status)
                {
                    fbe_cli_printf("stinfo SUCCESS: "
                                   "Set stop on error flags succeeded 0x%x additional_event = 0x%x persist=%d\n", stop_on_error_flags,additional_event,persist);
                }
                if( stop_on_error_flags !=  FBE_SECTOR_TRACE_ERROR_FLAG_NONE){
                    stop_system_on_error = FBE_TRUE;
                }
                status = fbe_api_sector_trace_set_stop_on_error(stop_system_on_error);
                if ( FBE_STATUS_OK == status)
                {
                    fbe_cli_printf("stinfo SUCCESS: "
                                   "Set stop on error succeeded to be %d\n", stop_system_on_error);
                }
            }
            if ( FBE_STATUS_OK != status)
            {
                fbe_cli_printf("stinfo ERROR: "
                               "Set / Restore  trace flags/mask failed\n");
            }
        }
        else
        {
            fbe_cli_printf("stinfo ERROR: Invalid Arguments \n");
            fbe_cli_printf("%s", SECTOR_TRACE_INFO_CMD_USAGE);
            return;
        }
        argv++;
    }
    return;
}
/******************************************
 * end fbe_cli_sector_trace_info_parse_cmds()
 ******************************************/

void fbe_cli_error_trace_counters(int argc, char** argv)
{
    fbe_api_trace_error_counters_t sep_error_counters;
    fbe_api_trace_error_counters_t pp_error_counters;
	fbe_status_t status;

	if(argc > 0){ /* We have arguments */
		if ((strcmp(*argv, "-help") == 0) || (strcmp(*argv, "-h") == 0) ) {
			/* If they are asking for help, just display help and exit. */
			fbe_cli_printf("%s", ERROR_TRACE_COUNTERS_CMD_USAGE);
			return;
		}
		if ((strcmp(*argv, "-clear") == 0)) {
			fbe_cli_printf("Clear error counters for SEP\n");
			fbe_api_trace_clear_error_counter(FBE_PACKAGE_ID_SEP_0);
			fbe_cli_printf("Clear error counters for PP\n");
			fbe_api_trace_clear_error_counter(FBE_PACKAGE_ID_PHYSICAL);
			return;
		}
	}

    status = fbe_api_trace_get_error_counter(&pp_error_counters, FBE_PACKAGE_ID_PHYSICAL);
	if(status != FBE_STATUS_OK){
		fbe_cli_printf("Failed to get error counters for PP status %d\n", status);
		return;
	}

	fbe_api_trace_get_error_counter(&sep_error_counters, FBE_PACKAGE_ID_SEP_0);
	if(status != FBE_STATUS_OK){
		fbe_cli_printf("Failed to get error counters for SEP status %d\n", status);
		return;
	}

    fbe_cli_printf("Package     Errors     Critical errors \n");
    fbe_cli_printf("PP          %d         %d \n", pp_error_counters.trace_error_counter, pp_error_counters.trace_critical_error_counter);
    fbe_cli_printf("SEP         %d         %d \n", sep_error_counters.trace_error_counter, sep_error_counters.trace_critical_error_counter);

    return;
}



/*************************
 * end file fbe_cli_lib_sector_trace_info_cmds.c
 *************************/
