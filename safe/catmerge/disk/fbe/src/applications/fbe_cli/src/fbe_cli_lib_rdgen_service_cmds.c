/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!*************************************************************************
 * @file fbe_cli_src_rdgen_service_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contains cli functions for the logical drive object.
 *
 * @ingroup fbe_cli
 *
 * @date
 *  3/27/2010 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
#include "fbe_cli_private.h"
#include "fbe_trace.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_transport_trace.h"
#include "fbe_block_transport_trace.h"
#include "fbe/fbe_api_trace.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe_cli_rdgen_service.h"

/*!*******************************************************************
 * @enum fbe_rdgen_display_mode_t
 *********************************************************************
 * @brief
 *
 *********************************************************************/
typedef enum fbe_rdgen_display_mode_e
{
    FBE_RDGEN_DISPLAY_MODE_INVALID = 0,
    FBE_RDGEN_DISPLAY_MODE_DEFAULT = 0,
    FBE_RDGEN_DISPLAY_MODE_SUMMARY,
    FBE_RDGEN_DISPLAY_MODE_PERFORMANCE,
    FBE_RDGEN_DISPLAY_MODE_ERRORS,
    FBE_RDGEN_DISPLAY_MODE_LAST,
}
fbe_rdgen_display_mode_t;

static fbe_status_t fbe_cli_rdgen_display(fbe_u32_t argc, char** argv, fbe_rdgen_display_mode_t display_mode);
static fbe_status_t fbe_cli_rdgen_stop(fbe_u32_t argc, char** argv, fbe_bool_t b_sync);
static fbe_status_t fbe_cli_rdgen_start(fbe_u32_t argc, char** argv);
static fbe_status_t fbe_cli_rdgen_cmi_perf_test(fbe_u32_t argc, char** argv, fbe_bool_t with_mask);
static fbe_status_t fbe_cli_rdgen_get_filter(fbe_u32_t argc, char** argv,
                                             fbe_rdgen_filter_t *filter_p,
                                             fbe_u32_t *args_processed_p);
static fbe_bool_t be_quiet = FBE_FALSE;
static fbe_bool_t b_initted = FBE_FALSE;
enum {
    /*! When doing random abort we abort I/Os within 5 sec. 
     */
    FBE_RDGEN_CLI_RANDOM_ABORT_MS = 1000,

};
/*!*************************************************
 * @typedef fbe_cli_rdgen_info_t
 ***************************************************
 * @brief
 * 
 ***************************************************/
typedef struct fbe_cli_rdgen_info_t
{
    fbe_bool_t b_initialized;
    fbe_queue_head_t context_queue;
    fbe_spinlock_t lock;
    fbe_api_rdgen_context_t stop_context;
}
fbe_cli_rdgen_info_t;

/*!*************************************************
 * @typedef fbe_cli_rdgen_context_t
 ***************************************************
 * @brief Element we use to keep track of an rdgen request.
 * 
 ***************************************************/
typedef struct fbe_cli_rdgen_context_t
{
    fbe_queue_element_t queue_element;
    fbe_api_rdgen_context_t context;
}
fbe_cli_rdgen_context_t;

/*!*******************************************************************
 * @var fbe_cli_rdgen_info
 *********************************************************************
 * @brief Global context for managing outstanding requests.
 *        We init to uninitialized.
 *
 *********************************************************************/
fbe_cli_rdgen_info_t fbe_cli_rdgen_info = {FBE_FALSE};

static void fbe_cli_rdgen_display_context(fbe_cli_rdgen_context_t *context_p,
                                          fbe_bool_t b_start);

void fbe_cli_rdgen_info_init(fbe_cli_rdgen_info_t *info_p)
{
    if (info_p->b_initialized == FBE_FALSE)
    {
        fbe_queue_init(&info_p->context_queue);
        fbe_spinlock_init(&info_p->lock);
        info_p->b_initialized = FBE_TRUE;
    }
    return;
}
void fbe_cli_rdgen_info_enqueue(fbe_cli_rdgen_context_t *context_p)
{
    fbe_cli_rdgen_info_init(&fbe_cli_rdgen_info);
    fbe_spinlock_lock(&fbe_cli_rdgen_info.lock);
    fbe_queue_push(&fbe_cli_rdgen_info.context_queue, &context_p->queue_element);
    fbe_spinlock_unlock(&fbe_cli_rdgen_info.lock);
    return;
}
void fbe_cli_rdgen_info_dequeue(fbe_cli_rdgen_context_t *context_p)
{
    if (fbe_queue_is_element_on_queue(&context_p->queue_element))
    {
        fbe_queue_remove(&context_p->queue_element);
    }
    return;
}
void fbe_cli_rdgen_display_usage(void)
{
    fbe_cli_printf("%s", RDGEN_SERVICE_USAGE);
    fbe_cli_printf("%s", RDGEN_SERVICE_USAGE_SWITCHES);
    fbe_cli_printf("%s", RDGEN_SERVICE_USAGE_EXAMPLES);
}

double fbe_time_us_to_ms(fbe_time_t microsec)
{
    return (double)microsec / (double)FBE_TIME_MILLISECONDS_PER_MICROSECOND;
}
double fbe_time_dus_to_ms(double microsec)
{
    return (double)microsec / (double)FBE_TIME_MILLISECONDS_PER_MICROSECOND;
}
/*!**************************************************************
 * fbe_cli_cmd_rdgen_service()
 ****************************************************************
 * @brief
 *   this is the entry point for the rdgen service cli command.
 *  
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  3/27/2010 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_cli_cmd_rdgen_service(int argc , char ** argv)
{
    fbe_status_t status;
    fbe_api_rdgen_get_stats_t stats;
    fbe_cli_rdgen_info_init(&fbe_cli_rdgen_info);
	be_quiet = FBE_FALSE;

    if (argc == 0)
    {
        fbe_cli_error("%s Not enough arguments to rdgen\n", __FUNCTION__);
        fbe_cli_rdgen_display_usage();
        return;
    }

    if (b_initted == FBE_FALSE)
    {
        fbe_rdgen_filter_t filter;
        fbe_zero_memory(&filter,sizeof(filter));
        status = fbe_api_rdgen_filter_init(&filter, 
                                           FBE_RDGEN_FILTER_TYPE_CLASS,
                                           FBE_OBJECT_ID_INVALID, FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, 0);
        /* Even if the filter is not initialize, it is OK to send this to rdgen. */
        status = fbe_api_rdgen_get_stats(&stats, &filter);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\n\n************** WARNING !!! PLEASE PAY ATTENTION !!! ******************\n");
            fbe_cli_printf("rdgen cannot access newneitpackage.  \n\nDid you already load the NewNeitPackage?\n");
            fbe_cli_printf("  If not, please exit fbecli by typing q\n\n");
            fbe_cli_printf("  Then type net start NewNeitPackage to start neit.\n\n");
            fbe_cli_printf("  It is very important to **exit and restart** fbecli after you start NewNeitPackage.\n");
            fbe_cli_printf("**********************************************************************\n\n");
            return;
        }
        b_initted = FBE_TRUE;
    }
    /* By default if we are not given any arguments we do a 
     * terse display of all object. 
     * By default we also have -all enabled. 
     */

    /*
     * Parse the command line.
     */
    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /* If they are asking for help, just display help and exit.
             */
            fbe_cli_rdgen_display_usage();
            return;
        }
        else if (strcmp(*argv, "-d") == 0)
        {
            argc--;
            argv++;
            status = fbe_cli_rdgen_display(argc, argv, FBE_RDGEN_DISPLAY_MODE_DEFAULT);
            return;
        }
        else if (strcmp(*argv, "-dp") == 0)
        {
            argc--;
            argv++;
            status = fbe_cli_rdgen_display(argc, argv, FBE_RDGEN_DISPLAY_MODE_PERFORMANCE);
            return;
        }
        else if ((strcmp(*argv, "-s") == 0) ||
                 (strcmp(*argv, "-summary") == 0))
        {
            argc--;
            argv++;
            status = fbe_cli_rdgen_display(argc, argv, FBE_RDGEN_DISPLAY_MODE_SUMMARY);
            return;
        }		
        else if (strcmp(*argv, "-dq") == 0)
        {
			be_quiet = FBE_TRUE;
            argc--;
            argv++;
            status = fbe_cli_rdgen_display(argc, argv, FBE_RDGEN_DISPLAY_MODE_DEFAULT);
            return;
        }
        else if ((strcmp(*argv, "-trace_level") == 0) ||
                 (strcmp(*argv, "-t") == 0))
        {
            fbe_u32_t trace_level;

            /* Get Trace Level argument.
             */
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("-trace_level, expected trace level argument. \n");
                return;
            }
            /* Context is the new trace level to set.
             */
            trace_level = (fbe_u32_t)strtoul(*argv, 0, 0);
            status = fbe_api_rdgen_set_trace_level(trace_level);

            if (status == FBE_STATUS_OK)
            {
                fbe_cli_printf("Successfully set trace level to %d\n", trace_level);
                return;
            }
            else
            {
                fbe_cli_error("set trace level to %d failed with status %d\n", trace_level, status);
                return;
            }
        }
        else if (strcmp(*argv, "-stop_sync") == 0)
        {
            argc--;
            argv++;
            fbe_cli_rdgen_stop(argc, argv, FBE_TRUE /* synchronous */);
            return;
        }
        else if ((strcmp(*argv, "-stop") == 0) ||
                 (strcmp(*argv, "-term") == 0))
        {
            argc--;
            argv++;
            fbe_cli_rdgen_stop(argc, argv, FBE_FALSE /* Not synchronous */);
            return;
        }
		else if ((strcmp(*argv, "-cmi_perf_test") == 0)) 
		{
			argc--;
            argv++;
            fbe_cli_rdgen_cmi_perf_test(argc, argv, FBE_FALSE);
            return;

		}

		else if ((strcmp(*argv, "-cmi_perf_test_with_mask") == 0)) 
		{
			argc--;
            argv++;
            fbe_cli_rdgen_cmi_perf_test(argc, argv, FBE_TRUE);
            return;

		}

		else if ((strcmp(*argv, "-init_dps") == 0) ||
                 (strcmp(*argv, "-init_dps_cmm") == 0)) 
		{
            fbe_u32_t memory_size;
            fbe_rdgen_memory_type_t memory_type = FBE_RDGEN_MEMORY_TYPE_CACHE;
            if (strcmp(*argv, "-init_dps_cmm") == 0)
            {
                memory_type = FBE_RDGEN_MEMORY_TYPE_CMM;
            }
            else
            {
                /* By default use cache.
                 */
                memory_type = FBE_RDGEN_MEMORY_TYPE_CACHE;
            }
			argc--;
            argv++;

            if(argc == 0)
            {
                memory_size = FBE_API_RDGEN_DEFAULT_MEM_SIZE_MB;
                fbe_cli_printf("init of dps no memory size specified, using default of %d MB\n", memory_size);
            }
            else
            {
                /* Context is the new trace level to set.
                 */
                memory_size = (fbe_u32_t)strtoul(*argv, 0, 0);
            }

            status = fbe_api_rdgen_init_dps_memory(memory_size, memory_type);

            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("rdgen:  error initializing dps with memory size: %d MB status: 0x%x type: %d\n", 
                               memory_size, status, memory_type);
            }
            else
            {
                fbe_cli_printf("rdgen: dps_memory is initialized with memory size %d MB type: %d\n", memory_size, memory_type);
            }
            return;
		}
        else if (!strcmp(*argv, "-release_dps"))
        {
            status = fbe_api_rdgen_release_dps_memory();
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("%s -release_memory failed\n", __FUNCTION__);
                return;
            }
            else
            {
                fbe_cli_printf("rdgen memory release successfully\n");
            }
            return;
        }
        else if (!strcmp(*argv, "-reset_stats"))
        {
            status = fbe_api_rdgen_reset_stats();
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("%s -reset_stats failed\n", __FUNCTION__);
                return;
            }
            else
            {
                fbe_cli_printf("rdgen statistics reset successfully\n");
            }
            return;
        }
        else if (!strcmp(*argv, "-default_timeout_msec"))
        {
            fbe_time_t timeout_ms;
            argc--;
            argv++;
            if (argc)
            {
                timeout_ms = (fbe_time_t)strtoul(*argv, 0, 0);

                status = fbe_api_rdgen_set_default_timeout(timeout_ms);

                if (status != FBE_STATUS_OK)
                {
                    fbe_cli_error("rdgen: status %d when setting timeout\n", status);
                    return;
                }
                else
                {
                    fbe_cli_printf("rdgen: Successfully set timeout msec to %d.\n", (int)timeout_ms);
                }
            }
            else
            {
                fbe_cli_error("%s no argument found for -default_timeout_msec argument\n", __FUNCTION__);
                return;
            }
        }
        else if (!strcmp(*argv, "-enable_sys_threads"))
        {
            fbe_u32_t enable;
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("No value provided.\n");
                return;
            }
            else
            {
                /* Context is the new trace level to set.
                 */
                enable = (fbe_u32_t)strtoul(*argv, 0, 0);
            }

            if (enable > 1)
            {
                fbe_cli_printf("Invalid parameter, must be 0 or 1. Provided: 0x%x\n", enable);
                return;
            }

            status = fbe_api_rdgen_configure_system_threads(enable);

            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("rdgen: error setting system thread behavior to 0x%x: status: 0x%x\n", enable, status);
            }
            else
            {
                fbe_cli_printf("rdgen: system thread behavior set to: 0x%x\n", enable);
            }
            return;
        }
        else
        {
            fbe_cli_rdgen_start(argc, argv);
            return;
        }

        argc--;
        argv++;

    }   /* end of while */
    return;
}
/**************************************
 * end fbe_cli_cmd_rdgen_service()
 **************************************/

void fbe_api_rdgen_get_memory_type_string(fbe_rdgen_memory_type_t memory_type, const fbe_char_t **string_p)
{
    switch (memory_type)
    {
        case FBE_RDGEN_MEMORY_TYPE_CACHE:
            *string_p = "SP Cache";
            break;
        case FBE_RDGEN_MEMORY_TYPE_CMM:
            *string_p = "CMM";
            break;
        default:
            *string_p = "unknown";
    };
}
void fbe_cli_rdgen_display_terse_info(fbe_api_rdgen_get_stats_t *stats_p)
{
    const fbe_char_t *mem_type_string_p = NULL;

    fbe_api_rdgen_get_memory_type_string(stats_p->memory_type, &mem_type_string_p);
    fbe_cli_printf("\r\n");

    fbe_cli_printf("memory size in MB : %d\r\n", stats_p->memory_size_mb );
    fbe_cli_printf("memory type       : %d(%s)\r\n", stats_p->memory_type, mem_type_string_p);
    fbe_cli_printf("io timeout msec   : %lld\r\n", (long long)stats_p->io_timeout_msecs);

    fbe_cli_printf("\r\nActive Thread Statistics (Totals for threads currently running).\r\n");
    fbe_cli_printf("--------------------\r\n");

    fbe_cli_printf("threads           : %u\r\n", stats_p->threads );
    fbe_cli_printf("objects           : %u\r\n", stats_p->objects );
    fbe_cli_printf("requests          : %u\r\n", stats_p->requests );
    fbe_cli_printf("io count          : %u \r\n", stats_p->io_count);
    fbe_cli_printf("pass count        : %u \r\n", stats_p->pass_count);
    fbe_cli_printf("errors            : %u \r\n", stats_p->error_count);
    fbe_cli_printf("media errors      : %u \r\n", stats_p->media_error_count);
    fbe_cli_printf("aborted errors         : %u \r\n", stats_p->aborted_error_count);
    fbe_cli_printf("invalidated block count: %u \r\n", stats_p->inv_blocks_count);
    fbe_cli_printf("bad crc blocks count   : %u \r\n", stats_p->bad_crc_blocks_count);
    fbe_cli_printf("io failure errors      : %u \r\n", stats_p->io_failure_error_count);
    fbe_cli_printf("congested errors       : %u \r\n", stats_p->congested_err_count);
    fbe_cli_printf("still congested errors : %u \r\n", stats_p->still_congested_err_count);

    fbe_cli_printf("\r\nHistorical Statistics (Totals for threads that have completed) Reset with rdgen -reset_stats.\r\n");
    fbe_cli_printf("--------------------\r\n");

    fbe_cli_printf("io count            : %u \r\n", stats_p->historical_stats.io_count);
    fbe_cli_printf("pass count          : %u \r\n", stats_p->historical_stats.pass_count);
    fbe_cli_printf("errors              : %u \r\n", stats_p->historical_stats.error_count);
    fbe_cli_printf("media errors        : %u \r\n", stats_p->historical_stats.media_error_count);
    fbe_cli_printf("aborted errors      : %u \r\n", stats_p->historical_stats.aborted_error_count);
    fbe_cli_printf("inv blocks count    : %u \r\n", stats_p->historical_stats.inv_blocks_count);
    fbe_cli_printf("bad crc blocks count   : %u \r\n", stats_p->historical_stats.bad_crc_blocks_count);
    fbe_cli_printf("io failure errors      : %u \r\n", stats_p->historical_stats.io_failure_error_count);
    fbe_cli_printf("congested errors       : %u \r\n", stats_p->historical_stats.congested_error_count);
    fbe_cli_printf("still congeted errors  : %u \r\n", stats_p->historical_stats.still_congested_error_count);
    fbe_cli_printf("invalid request errors : %u \r\n", stats_p->historical_stats.invalid_request_error_count);
    fbe_cli_printf("requests from peer     : %u \r\n", stats_p->historical_stats.requests_from_peer_count);
    fbe_cli_printf("requests to peer       : %u \r\n", stats_p->historical_stats.requests_to_peer_count);

    return;
}
fbe_api_rdgen_request_info_t* fbe_cli_find_request_info_by_handle(fbe_api_rdgen_get_request_info_t *request_info_p,
                                                                  fbe_api_rdgen_handle_t handle)
{
    fbe_u32_t request_index;

    for (request_index = 0; request_index < request_info_p->num_requests; request_index++)
    {
        if (request_info_p->request_info_array[request_index].request_handle == handle)
        {
            return &request_info_p->request_info_array[request_index];
        }
    }
    /* Not found, return NULL.
     */
    return NULL;
}


void fbe_cli_rdgen_display_thread(fbe_api_rdgen_thread_info_t *thread_info_p,
                                  fbe_api_rdgen_get_request_info_t *request_info_p,
                                  fbe_rdgen_display_mode_t display_mode)
{
    fbe_api_rdgen_request_info_t *request_p = NULL;

    /* First find the request information.
     */
    request_p = fbe_cli_find_request_info_by_handle(request_info_p,
                                                    thread_info_p->request_handle);
    if (request_p != NULL)
    {
        const fbe_char_t *operation_string_p = NULL;
        fbe_api_rdgen_get_operation_string(request_p->specification.operation,
                                           &operation_string_p);
        printf("%5s  |", operation_string_p);
        
        printf(" 0x%6llx |",
	       (unsigned long long)request_p->specification.max_blocks);

        if (request_p->specification.block_spec == FBE_RDGEN_BLOCK_SPEC_CONSTANT)
        {
            printf(" %8s |",  "constant");
        }
        else if (request_p->specification.alignment_blocks)
        {
            printf("%8s|", "align/rand");
        }
        else
        {
            printf(" %8s |", "random");
        }


        if (request_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_FIXED)
        {
            printf(" %8s |", "fixed");
        }
        else if (request_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_CATERPILLAR_INCREASING)
        {
            printf(" %8s |", "cat inc");
        }
        else if (request_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_CATERPILLAR_DECREASING)
        {
            printf(" %8s |", "cat dec");
        }
        else if (request_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING)
        {
            printf(" %8s |", "seq inc");
        }
        else if (request_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_SEQUENTIAL_DECREASING)
        {
            printf(" %8s |", "seq dec");
        }
        else if (request_p->specification.alignment_blocks)
        {
            printf(" %8s |", "align/rand");
        }
        else
        {
            printf(" %8s |", "random");
        }
        if (display_mode != FBE_RDGEN_DISPLAY_MODE_PERFORMANCE)
        {
            printf(" 0x%6x |", thread_info_p->pass_count);
        }

        {
            fbe_u32_t seconds = request_p->elapsed_seconds;
            fbe_u32_t minutes = (seconds / 60) % 60;
            fbe_u32_t hours = seconds / 3600;
            seconds = seconds % 60;
            
            printf(" %02d:%02d:%02d |", hours, minutes, seconds);
        }

        /*! @todo might need to add this code for when we are retrying errors in rdgen.
         */
        if (0) // && flts_p->timer_id != 0 && flts_p->status != VD_REQUEST_COMPLETE)
        {
            printf("*0x%4x  | 0x%4x |", thread_info_p->err_count, thread_info_p->abort_err_count );
        }
        else if (display_mode != FBE_RDGEN_DISPLAY_MODE_PERFORMANCE)
        {
            printf(" 0x%4x  | 0x%4x |", thread_info_p->err_count, thread_info_p->abort_err_count );
        }
        printf(" 0x%8x |", thread_info_p->io_count);
        if (request_p->elapsed_seconds)
        {
            double io_rate = ((double)(thread_info_p->io_count) / ((double)(thread_info_p->elapsed_milliseconds) / 1000.0));
            printf(" %8.2f |", io_rate);
        }
        if (display_mode == FBE_RDGEN_DISPLAY_MODE_PERFORMANCE)
        {
            double avg_resp_time;
            if (thread_info_p->total_responses == 0)
            {
                avg_resp_time = 0;
            }
            else
            {
                avg_resp_time = (double)(thread_info_p->cumulative_response_time_us) / ((double)(thread_info_p->total_responses));
            }
            printf(" %8.4f  |", fbe_time_dus_to_ms(avg_resp_time));
            printf(" %8.4f | %8.4f |", fbe_time_us_to_ms(thread_info_p->max_response_time), 
                   fbe_time_us_to_ms(thread_info_p->last_response_time));
        }
        
        printf("\r\n");
        if (request_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_FIXED)
        {
            printf("                             | 0x%llx |\r\n", request_p->specification.min_lba);
        }
#if 0
        if (flts_p->io_mode & FL_CHECK_SET_PATTERN)
        {
            fbe_u32_t percent_complete = (fbe_u32_t) ((flts_p->current_lba * 100) / flts_p->max_lba);
            
            printf(" current lba is 0x%llX of 0x%llX total.   Percent complete is %d.\r\n", flts_p->current_lba, flts_p->max_lba,
                   percent_complete);
        }
#endif
    }
    return;
}
fbe_status_t fbe_cli_rdgen_display_object_stats(fbe_api_rdgen_object_info_t *object_info_p,
                                                fbe_api_rdgen_get_request_info_t *request_info_p,
                                                fbe_api_rdgen_get_thread_info_t *thread_info_p)
{

    fbe_u32_t thread_index;
    fbe_api_rdgen_thread_info_t *current_thread_info_p = NULL;
    fbe_u32_t found_threads = 0;

    double io_per_sec_cumulative = 0;
    fbe_u64_t ios_cumulative = 0;
    fbe_u64_t passes_cumulative = 0;
    fbe_u64_t errors_cumulative = 0;
    fbe_u64_t max_response_time = 0;
    fbe_u64_t total_responses = 0;
    fbe_u64_t cumulative_response_time = 0;
    fbe_u64_t max_last_response_time = 0;
    fbe_u64_t min_last_response_time = FBE_U64_MAX;
    fbe_u64_t total_last_response_times = 0;
    fbe_u64_t cumulative_last_response_time = 0;
    double average_response_time;
    double average_last_response_time;

    double read_average_response_time = 0;
    double read_io_per_sec_cumulative = 0;
    fbe_u64_t read_max_response_time = 0;
    fbe_u64_t read_total_responses = 0;
    fbe_u64_t read_cumulative_response_time = 0;

    double write_average_response_time = 0;
    double write_io_per_sec_cumulative = 0;
    fbe_u64_t write_max_response_time = 0;
    fbe_u64_t write_total_responses = 0;
    fbe_u64_t write_cumulative_response_time = 0;

    fbe_api_rdgen_request_info_t *request_p = NULL;

    /* Loop over all the threads and stop when we have found the number we were 
     * looking for. 
     */
    for ( thread_index = 0; 
        ((thread_index < thread_info_p->num_threads) && (found_threads < object_info_p->active_ts_count)); 
        thread_index++)
    {
        current_thread_info_p = &thread_info_p->thread_info_array[thread_index];
        if (current_thread_info_p->object_handle == object_info_p->object_handle)
        {
            double io_rate = ((double)(current_thread_info_p->io_count) / ((double)(current_thread_info_p->elapsed_milliseconds) / 1000.0));
            io_per_sec_cumulative += io_rate;
            ios_cumulative += current_thread_info_p->io_count;
            passes_cumulative += current_thread_info_p->pass_count;
            errors_cumulative += current_thread_info_p->err_count;
            max_response_time = FBE_MAX(max_response_time, current_thread_info_p->max_response_time);
            cumulative_response_time += current_thread_info_p->cumulative_response_time_us;
            total_responses += current_thread_info_p->total_responses;
            cumulative_last_response_time += current_thread_info_p->last_response_time;
            total_last_response_times++;
            max_last_response_time = FBE_MAX(current_thread_info_p->last_response_time, 
                                             max_last_response_time);
            min_last_response_time = FBE_MIN(current_thread_info_p->last_response_time, 
                                             min_last_response_time);
            request_p = fbe_cli_find_request_info_by_handle(request_info_p,
                                                            current_thread_info_p->request_handle);
            if (request_p != NULL){
                if (request_p->specification.operation == FBE_RDGEN_OPERATION_READ_ONLY){
                    read_io_per_sec_cumulative += io_rate;
                    read_max_response_time = FBE_MAX(read_max_response_time, current_thread_info_p->max_response_time);
                    read_cumulative_response_time += current_thread_info_p->cumulative_response_time_us;
                    read_total_responses += current_thread_info_p->total_responses;
                }
                else {
                    write_io_per_sec_cumulative += io_rate;
                    write_max_response_time = FBE_MAX(write_max_response_time, current_thread_info_p->max_response_time);
                    write_cumulative_response_time += current_thread_info_p->cumulative_response_time_us;
                    write_total_responses += current_thread_info_p->total_responses;
                }
            }
            found_threads++;
        }
    }
    /* In sim our response times can be 0.
     */
    if (total_last_response_times == 0)
    {
        average_last_response_time = 0;
    }
    else
    {
        average_last_response_time = (double)cumulative_last_response_time / (double)total_last_response_times;
    }

    if (total_responses == 0)
    {
        average_response_time = 0;
    }
    else
    {
        average_response_time = (double)cumulative_response_time / (double)total_responses;
    }
    fbe_cli_printf("threads: %u ", object_info_p->active_ts_count);
    fbe_cli_printf("ios: 0x%llx ", ios_cumulative);
    fbe_cli_printf("passes: 0x%llx ", passes_cumulative);
    fbe_cli_printf("errors: %llu ", errors_cumulative);
    fbe_cli_printf("\n");
    fbe_cli_printf("rate: %.2f ", io_per_sec_cumulative);
    fbe_cli_printf("avg resp tme: %.4f ", fbe_time_dus_to_ms(average_response_time));
    fbe_cli_printf("max resp tme: %.4f ", fbe_time_us_to_ms(max_response_time));
    fbe_cli_printf("avg current resp tme: %.4f ", fbe_time_dus_to_ms(average_last_response_time));
    fbe_cli_printf("max current resp tme: %.4f ", fbe_time_us_to_ms(max_last_response_time));
    fbe_cli_printf("min current resp tme: %.4f ", fbe_time_us_to_ms(min_last_response_time));

    if (read_total_responses != 0 && write_total_responses != 0) {
        fbe_cli_printf("\n");
        read_average_response_time = (double)read_cumulative_response_time / (double)read_total_responses;
        fbe_cli_printf("read rate: %.2f ", read_io_per_sec_cumulative);
        fbe_cli_printf("read avg resp tme: %.4f ", fbe_time_dus_to_ms(read_average_response_time));
        fbe_cli_printf("read max resp tme: %.4f ", fbe_time_us_to_ms(read_max_response_time));

        write_average_response_time = (double)write_cumulative_response_time / (double)write_total_responses;
        fbe_cli_printf("write rate: %.2f ", write_io_per_sec_cumulative);
        fbe_cli_printf("write avg resp tme: %.4f ", fbe_time_dus_to_ms(write_average_response_time));
        fbe_cli_printf("write max resp tme: %.4f ", fbe_time_us_to_ms(write_max_response_time));
    }
    return FBE_STATUS_OK;
}
void fbe_cli_rdgen_display_cumulative_info(fbe_api_rdgen_get_stats_t *stats_p,
                                           fbe_api_rdgen_get_object_info_t *object_info_p,
                                           fbe_api_rdgen_get_request_info_t *request_info_p,
                                           fbe_api_rdgen_get_thread_info_t *thread_info_p,
                                           fbe_rdgen_display_mode_t display_mode)
{

    fbe_u32_t object_index;
    fbe_api_rdgen_object_info_t *current_object_info_p = NULL;
    fbe_u32_t thread_index;
    fbe_api_rdgen_thread_info_t *current_thread_info_p = NULL;
    fbe_u32_t found_threads;
    fbe_api_rdgen_request_info_t *request_p = NULL;

    double io_per_sec_cumulative = 0;
    double min_response_time = 0;
    fbe_u64_t max_response_time = 0;
    fbe_api_rdgen_object_info_t *min_resp_object_info_p = NULL;
    fbe_api_rdgen_object_info_t *max_resp_object_info_p = NULL;
    fbe_api_rdgen_thread_info_t *min_resp_thread_info_p = NULL;
    fbe_api_rdgen_thread_info_t *max_resp_thread_info_p = NULL;
    fbe_u64_t total_responses = 0;
    fbe_u64_t cumulative_response_time = 0;
    double average_response_time;
    
    for (object_index = 0; object_index < object_info_p->num_objects; object_index++)
    {
        current_object_info_p = &object_info_p->object_info_array[object_index];

        /* If we have a fldb, display it if there are outstanding threads.
         */
        if ( current_object_info_p->active_ts_count != 0 )
        {
            found_threads = 0;

            /* Loop over all the threads and stop when we have found the number we were 
             * looking for. 
             */
            for ( thread_index = 0; 
                  ((thread_index < thread_info_p->num_threads) && (found_threads < current_object_info_p->active_ts_count)); 
                  thread_index++)
            {
                current_thread_info_p = &thread_info_p->thread_info_array[thread_index];
                if (current_thread_info_p->object_handle == current_object_info_p->object_handle)
                {
                    double io_rate = ((double)(current_thread_info_p->io_count) / ((double)(current_thread_info_p->elapsed_milliseconds) / 1000.0));
                    double avg_resp_time;
                    if (current_thread_info_p->total_responses == 0)
                    {
                        avg_resp_time = 0;
                    }
                    else
                    {
                        avg_resp_time = 
                            ((double)current_thread_info_p->cumulative_response_time_us) / ((double) current_thread_info_p->total_responses); 
                    }
                    if (current_thread_info_p->max_response_time > max_response_time)
                    {
                        max_response_time = current_thread_info_p->max_response_time;
                        max_resp_object_info_p = current_object_info_p;
                        max_resp_thread_info_p = current_thread_info_p;
                    }
                    if ((min_response_time == 0) || (avg_resp_time < min_response_time))
                    {
                        min_response_time = avg_resp_time;
                        min_resp_object_info_p = current_object_info_p;
                        min_resp_thread_info_p = current_thread_info_p;
                    }
                    
                    io_per_sec_cumulative += io_rate;
                    cumulative_response_time += current_thread_info_p->cumulative_response_time_us;
                    total_responses += current_thread_info_p->total_responses;

                    found_threads++;
                }
            }
        }
    }
    if (total_responses == 0)
    {
        average_response_time = 0;
    }
    else
    {
        average_response_time = (double)cumulative_response_time / (double)total_responses;
    }

    fbe_cli_printf("\n");
    fbe_cli_printf("Performance totals across all threads (all objects)\n");
    fbe_cli_printf("-----------------------------------------------------------------------------\n");
    fbe_cli_printf("cumulative io rate (io/sec):                      %9.2f \n", io_per_sec_cumulative);
    fbe_cli_printf("average response time per thread (millisec):      %9.4f \n", 
                   fbe_time_dus_to_ms(average_response_time));

    if(min_resp_thread_info_p != NULL)
    {

        request_p = fbe_cli_find_request_info_by_handle(request_info_p,
                                                    min_resp_thread_info_p->request_handle);
        if(request_p != NULL)
        {
            if (request_p->specification.io_interface != FBE_RDGEN_INTERFACE_TYPE_PACKET)
            {
                fbe_cli_printf("best average response time per thread (millisec): %9.4f object_id: %d name: %s\n", 
                       fbe_time_dus_to_ms(min_response_time), min_resp_object_info_p->object_id, min_resp_object_info_p->driver_object_name);
            }
            else
            {
                fbe_cli_printf("best average response time per thread (millisec): %9.4f object_id: %d\n", 
                               fbe_time_dus_to_ms(min_response_time), min_resp_object_info_p->object_id);
            }
        }
    }
 
    if(max_resp_thread_info_p != NULL)
    {
        request_p = fbe_cli_find_request_info_by_handle(request_info_p,
                                                    max_resp_thread_info_p->request_handle);
        if(request_p != NULL)
        {
            if (request_p->specification.io_interface != FBE_RDGEN_INTERFACE_TYPE_PACKET)
            {
                fbe_cli_printf("max thread response time (millisec):              %9.4f object_id: %d name: %s\n", 
                       fbe_time_us_to_ms(max_response_time), max_resp_object_info_p->object_id, max_resp_object_info_p->driver_object_name);
            }
            else
            {
                fbe_cli_printf("max thread response time (millisec):              %9.4f object_id: %d\n", 
                               fbe_time_us_to_ms(max_response_time), max_resp_object_info_p->object_id);
            }
        }
    }

    fbe_cli_printf("\r\n");
}

void fbe_cli_rdgen_display_thread_info(fbe_api_rdgen_get_stats_t *stats_p,
                                       fbe_api_rdgen_get_object_info_t *object_info_p,
                                       fbe_api_rdgen_get_request_info_t *request_info_p,
                                       fbe_api_rdgen_get_thread_info_t *thread_info_p,
                                       fbe_rdgen_display_mode_t display_mode)
{
    fbe_u32_t object_index;
    fbe_api_rdgen_object_info_t *current_object_info_p = NULL;
    fbe_u32_t thread_index;
    fbe_api_rdgen_thread_info_t *current_thread_info_p = NULL;
    fbe_u32_t found_threads;
    fbe_api_rdgen_request_info_t *request_p = NULL;

    fbe_cli_rdgen_display_cumulative_info(stats_p, object_info_p, request_info_p, thread_info_p, display_mode);

    for (object_index = 0; object_index < object_info_p->num_objects; object_index++)
    {
        current_object_info_p = &object_info_p->object_info_array[object_index];

        /* If we have a fldb, display it if there are outstanding threads.
         */
        if ( current_object_info_p->active_ts_count != 0 )
        {        
            /* First find the request information.
             */
            if (thread_info_p->num_threads > 0)
            {
                const fbe_char_t *string_p = NULL;
                request_p = fbe_cli_find_request_info_by_handle(request_info_p,
                                                                thread_info_p->thread_info_array[0].request_handle);
                fbe_api_rdgen_get_interface_string(request_p->specification.io_interface, &string_p);
                if (request_p->specification.io_interface != FBE_RDGEN_INTERFACE_TYPE_PACKET)
                {
                    fbe_cli_printf("\r\nobject_id: %d interface: %s name: %s\n", 
                                   current_object_info_p->object_id, string_p, current_object_info_p->driver_object_name);
                }
                else
                {
                    fbe_cli_printf("\r\nobject_id: %d interface: %s ", current_object_info_p->object_id, string_p);
                }
            }
            else
            {
                fbe_cli_printf("\r\nobject_id: %d ", current_object_info_p->object_id);
            }
            fbe_cli_rdgen_display_object_stats(current_object_info_p, request_info_p, thread_info_p);

            if (display_mode == FBE_RDGEN_DISPLAY_MODE_SUMMARY)
            {
                fbe_cli_printf("\n");
                continue;
            }
			if(be_quiet){
				continue;
			}
            //fbe_cli_printf("running: %s ", (current_object_info_p->b_stop) ? "no" : "yes");
            fbe_cli_printf("\n");
            if (current_object_info_p->active_ts_count)
            {
                if (display_mode == FBE_RDGEN_DISPLAY_MODE_PERFORMANCE)
                {
                  fbe_cli_printf("opcode | max blks | blk size | lba mode | Time     | ios        |    rate  | resp time | max resp | last resp\r\n");
                  fbe_cli_printf("-------|----------|----------|----------|----------|------------|----------|-----------|----------|------------ \r\n");
                }
                else
                {
                    fbe_cli_printf("opcode | max blks | blk size | lba mode | pass     | Time     | errCnt  | aborts | ios        | rate\r\n");
                    fbe_cli_printf("-------|----------|----------|----------|----------|----------|---------|--------|------------|------ \r\n");
                }
            }
            found_threads = 0;

            /* Loop over all the threads and stop when we have found the number we were 
             * looking for. 
             */
            for ( thread_index = 0; 
                  ((thread_index < thread_info_p->num_threads) && (found_threads < current_object_info_p->active_ts_count)); 
                  thread_index++)
            {
                current_thread_info_p = &thread_info_p->thread_info_array[thread_index];
                if (current_thread_info_p->object_handle == current_object_info_p->object_handle)
                {
                    fbe_cli_rdgen_display_thread(current_thread_info_p, request_info_p, display_mode);
                    found_threads++;
                }
            }
        }
    }
	fbe_cli_printf("\r\n");

    return;
}

/*!**************************************************************
 * fbe_cli_rdgen_stop()
 ****************************************************************
 * @brief
 *  Send a stop for rdgen threads.
 *
 * @param b_sync - FBE_TRUE to wait for things to come back.
 * 
 * @return fbe_status_t
 *
 * @author
 *  3/27/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_cli_rdgen_stop(fbe_u32_t argc, char** argv,
                                       fbe_bool_t b_sync)
{
    fbe_status_t status;
    fbe_cli_rdgen_context_t *cli_context_p = NULL;
    fbe_api_rdgen_context_t *rdgen_context_p = NULL;
    fbe_rdgen_filter_t filter;
    fbe_u32_t args_processed;

    /* If the user wants to do a synchronous stop, then stop all the requests that we started.
     * We wait for these to come back. 
     */
    if (b_sync)
    {
        fbe_spinlock_lock(&fbe_cli_rdgen_info.lock);
        while (!fbe_queue_is_empty(&fbe_cli_rdgen_info.context_queue))
        {
            cli_context_p = (fbe_cli_rdgen_context_t*)fbe_queue_pop(&fbe_cli_rdgen_info.context_queue);
    
            rdgen_context_p = &cli_context_p->context;
            /* Use the spinlock to display the entire context to prevent garbled output
             * if requests are completing when we are doing this. 
             */
            fbe_cli_printf("\r\nrdgen stopping: context: %p\n", cli_context_p);
            fbe_cli_rdgen_display_context(cli_context_p, FBE_FALSE  /* Not start */);
            fbe_spinlock_unlock(&fbe_cli_rdgen_info.lock);
    
            status = fbe_api_rdgen_send_stop_packet(&rdgen_context_p->start_io.filter);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("%s stop of rdgen context %p failed status: 0x%x\n", 
                              __FUNCTION__, cli_context_p, status);
            }
            
            status = fbe_api_rdgen_wait_for_stop_operation(rdgen_context_p);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("%s wait for stop of context %p rdgen failed status: 0x%x\n", 
                              __FUNCTION__, cli_context_p, status);
            }
            else
            {
                fbe_api_rdgen_test_context_destroy(&cli_context_p->context);
                fbe_api_free_contiguous_memory(cli_context_p);
            }
    
            fbe_spinlock_lock(&fbe_cli_rdgen_info.lock);
        }
        fbe_spinlock_unlock(&fbe_cli_rdgen_info.lock);
    } /* End if syncronous */

    /* An invalid filter means to stop all objects.
     */
    filter.filter_type = FBE_RDGEN_FILTER_TYPE_INVALID;

    /* If the user selected a filter, then use that to perform the stop. 
     * If no filter was provided, then the below function returns error and we will try to stop everything.
     */
    status = fbe_cli_rdgen_get_filter(argc, argv, &filter, &args_processed);
    if (status != FBE_STATUS_OK)
    {
        /* A bad status is normal, it just means no filter was provided in the args.
         */
        filter.filter_type = FBE_RDGEN_FILTER_TYPE_INVALID;
    }

    status = fbe_api_rdgen_send_stop_packet(&filter);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("%s error stopping all threads test status 0x%x\n", __FUNCTION__, status);
    }

    return status;
}
/**************************************
 * end fbe_cli_rdgen_stop()
 **************************************/

/*!**************************************************************
 * fbe_cli_rdgen_get_filter()
 ****************************************************************
 * @brief
 *  Get a filter from arguments passed in.
 *
 * @param argc - count of arguments              
 * @param argv - vector of arguments
 * @param filter_p
 * @param args_processed_p - 
 * 
 * @return fbe_status_t   
 *
 * @author
 *  3/30/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_cli_rdgen_get_filter(fbe_u32_t argc, char** argv,
                                             fbe_rdgen_filter_t *filter_p,
                                             fbe_u32_t *args_processed_p)
{
    fbe_status_t status;

    if ((argc == 0) ||
        (*argv == NULL))
    {
        *args_processed_p = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Get all arguments.
     */
    *args_processed_p = 1;
    if (!strcmp(*argv, "-parity"))
    {
        fbe_api_rdgen_filter_init(filter_p, FBE_RDGEN_FILTER_TYPE_CLASS,
                                  FBE_OBJECT_ID_INVALID, FBE_CLASS_ID_PARITY,
                                  FBE_PACKAGE_ID_SEP_0, filter_p->edge_index);
    }
    else if (!strcmp(*argv, "-striper"))
    {
        fbe_api_rdgen_filter_init(filter_p, FBE_RDGEN_FILTER_TYPE_CLASS,
                                  FBE_OBJECT_ID_INVALID, FBE_CLASS_ID_STRIPER,
                                  FBE_PACKAGE_ID_SEP_0, filter_p->edge_index);
    }
    else if (!strcmp(*argv, "-mirror"))
    {
        fbe_api_rdgen_filter_init(filter_p, FBE_RDGEN_FILTER_TYPE_CLASS,
                                  FBE_OBJECT_ID_INVALID, FBE_CLASS_ID_MIRROR,
                                  FBE_PACKAGE_ID_SEP_0, filter_p->edge_index);
    }
    else if (!strcmp(*argv, "-vd"))
    {
        fbe_api_rdgen_filter_init(filter_p, FBE_RDGEN_FILTER_TYPE_CLASS,
                                  FBE_OBJECT_ID_INVALID, FBE_CLASS_ID_VIRTUAL_DRIVE,
                                  FBE_PACKAGE_ID_SEP_0, filter_p->edge_index);
    }
    else if (!strcmp(*argv, "-pvd"))
    {
        fbe_api_rdgen_filter_init(filter_p, FBE_RDGEN_FILTER_TYPE_CLASS,
                                  FBE_OBJECT_ID_INVALID, FBE_CLASS_ID_PROVISION_DRIVE,
                                  FBE_PACKAGE_ID_SEP_0, filter_p->edge_index);
    }
    else if (!strcmp(*argv, "-lun"))
    {
        fbe_api_rdgen_filter_init(filter_p, FBE_RDGEN_FILTER_TYPE_CLASS,
                                  FBE_OBJECT_ID_INVALID, FBE_CLASS_ID_LUN,
                                  FBE_PACKAGE_ID_SEP_0, filter_p->edge_index);
    }
    else if (!strcmp(*argv, "-ldo"))
    {
        fbe_api_rdgen_filter_init(filter_p, FBE_RDGEN_FILTER_TYPE_CLASS,
                                  FBE_OBJECT_ID_INVALID, FBE_CLASS_ID_LOGICAL_DRIVE,
                                  FBE_PACKAGE_ID_PHYSICAL, filter_p->edge_index);
    }
    else if (!strcmp(*argv, "-sas_pdo"))
    {
        fbe_api_rdgen_filter_init(filter_p, FBE_RDGEN_FILTER_TYPE_CLASS,
                                  FBE_OBJECT_ID_INVALID, FBE_CLASS_ID_SAS_PHYSICAL_DRIVE,
                                  FBE_PACKAGE_ID_PHYSICAL, filter_p->edge_index);
    }
    else if (!strcmp(*argv, "-sata_pdo"))
    {
        fbe_api_rdgen_filter_init(filter_p, FBE_RDGEN_FILTER_TYPE_CLASS,
                                  FBE_OBJECT_ID_INVALID, FBE_CLASS_ID_SATA_PHYSICAL_DRIVE,
                                  FBE_PACKAGE_ID_PHYSICAL, 0);
    }
    else if (!strncmp(*argv, "-playback", 9))
    {
        argc--;
        argv++;
        if (argc)
        {
            size_t length;
            (*args_processed_p)++;
            length = strnlen(*argv, FBE_RDGEN_DEVICE_NAME_CHARS);
            if (length == 0)
            {
                fbe_cli_error("%s no file name found for -playback argument\n", __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            if (length > FBE_RDGEN_DEVICE_NAME_CHARS)
            {
                fbe_cli_error("%s no file name %s too long for -playback argument %d chars found %d chars max\n", 
                              __FUNCTION__, *argv, (int)length, FBE_RDGEN_DEVICE_NAME_CHARS);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            
            fbe_cli_printf("-playback_found: %s\n", *argv);
            status = fbe_api_rdgen_filter_init_for_playback(filter_p, FBE_RDGEN_FILTER_TYPE_PLAYBACK_SEQUENCE, *argv);
        }
        else
        {
            fbe_cli_error("%s no file name found for -playback argument\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else if (!strncmp(*argv, "-bshim", 6))
    {
        argc--;
        argv++;
        if (argc)
        {
            size_t length;
            (*args_processed_p)++;
            length = strnlen(*argv, FBE_RDGEN_DEVICE_NAME_CHARS);
            if (length == 0)
            {
                fbe_cli_error("%s no device name found for -bshim argument\n", __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            if (length > FBE_RDGEN_DEVICE_NAME_CHARS)
            {
                fbe_cli_error("%s no device name %s too long for -bshim argument %d chars found %d chars max\n", 
                              __FUNCTION__, *argv, (int)length, FBE_RDGEN_DEVICE_NAME_CHARS);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            status = fbe_api_rdgen_filter_init_for_block_device(filter_p, FBE_RDGEN_FILTER_TYPE_BLOCK_DEVICE, *argv);
        }
        else
        {
            fbe_cli_error("%s no device name found for -bshim argument\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else if (!strncmp(*argv, "-device", 7) || !strncmp(*argv, "-dev", 4))
    {
        argc--;
        argv++;
        if (argc)
        {
            size_t length;
            (*args_processed_p)++;
            length = strnlen(*argv, FBE_RDGEN_DEVICE_NAME_CHARS);
            if (length == 0)
            {
                fbe_cli_error("%s no device name found for -device argument\n", __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            if (length > FBE_RDGEN_DEVICE_NAME_CHARS)
            {
                fbe_cli_error("%s no device name %s too long for -device argument %d chars found %d chars max\n", 
                              __FUNCTION__, *argv, (int)length, FBE_RDGEN_DEVICE_NAME_CHARS);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            status = fbe_api_rdgen_filter_init_for_driver(filter_p, FBE_RDGEN_FILTER_TYPE_DISK_INTERFACE,
                                                          *argv, FBE_RDGEN_INVALID_OBJECT_NUMBER /* so we will just use device name */);
        }
        else
        {
            fbe_cli_error("%s no device name found for -device argument\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else if (!strcmp(*argv, "-clar_lun"))
    {
        argc--;
        argv++;
        if (argc)
        {
            fbe_object_id_t object_id;
            (*args_processed_p)++;
            object_id = (fbe_object_id_t)strtoul(*argv, 0, 0);

            status = fbe_api_rdgen_filter_init_for_driver(filter_p, FBE_RDGEN_FILTER_TYPE_DISK_INTERFACE,
                                                          "CLARiiONdisk", object_id);
        }
        else
        {
            fbe_cli_error("%s no device found for -clar_lun argument\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else if (!strcmp(*argv, "-sep_lun"))
    {
        argc--;
        argv++;
        if (argc)
        {
            fbe_object_id_t object_id;
            (*args_processed_p)++;
            object_id = (fbe_object_id_t)strtoul(*argv, 0, 0);

            status = fbe_api_rdgen_filter_init_for_driver(filter_p, FBE_RDGEN_FILTER_TYPE_DISK_INTERFACE,
                                                          "FlareDisk", object_id);
        }
        else
        {
            fbe_cli_error("%s no lun number found for -sep_lun argument\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else if (!strcmp(*argv, "-object_id"))
    {
        argc--;
        argv++;
        if (argc)
        {
            fbe_object_id_t object_id;
            (*args_processed_p)++;
            object_id = (fbe_object_id_t)strtoul(*argv, 0, 0);

            if ((object_id == 0) && strcmp(*argv, "0") && strcmp(*argv, "0x0"))
            {
                /* strtoul returns 0 if you enter a hex value without 0x. */
                fbe_cli_error("unexpected argument found for -object_id argument. \n");
                fbe_cli_error("Enter either -object_id (decimal number) or -object_id 0x(hex number) \n");
                return FBE_STATUS_GENERIC_FAILURE;
            }

            fbe_api_rdgen_filter_init(filter_p, FBE_RDGEN_FILTER_TYPE_OBJECT,
                                      object_id, FBE_CLASS_ID_INVALID,
                                      filter_p->package_id, filter_p->edge_index);
        }
        else
        {
            fbe_cli_error("%s no object id found for -object_id argument\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else if (!strcmp(*argv, "-rg_object_id"))
    {
        argc--;
        argv++;
        if (argc)
        {
            fbe_object_id_t object_id;
            (*args_processed_p)++;
            object_id = (fbe_object_id_t)strtoul(*argv, 0, 0);

            if ((object_id == 0) && strcmp(*argv, "0") && strcmp(*argv, "0x0"))
            {
                /* strtoul returns 0 if you enter a hex value without 0x. */
                fbe_cli_error("unexpected argument found for -rg_object_id argument. \n");
                fbe_cli_error("Enter either -rg_object_id (decimal number) or -rg_object_id 0x(hex number) \n");
                return FBE_STATUS_GENERIC_FAILURE;
            }

            fbe_api_rdgen_filter_init(filter_p, FBE_RDGEN_FILTER_TYPE_RAID_GROUP,
                                      object_id, FBE_CLASS_ID_INVALID,
                                      filter_p->package_id, filter_p->edge_index);
        }
        else
        {
            fbe_cli_error("%s no object id found for -object_id argument\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else if (!strcmp(*argv, "-package_id"))
    {
        argc--;
        argv++;
        if (argc)
        {
            (*args_processed_p)++;
            if (!strcmp(*argv, "physical"))
            {
                filter_p->package_id = FBE_PACKAGE_ID_PHYSICAL;
            }
            else if (!strcmp(*argv, "sep"))
            {
                filter_p->package_id = FBE_PACKAGE_ID_SEP_0;
            }
            else
            {
                (*args_processed_p) = 0;
                fbe_cli_error("%s unknown package id found for -package_id (%s) argument\n", __FUNCTION__, *argv);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        else
        {
            (*args_processed_p) = 0;
            fbe_cli_error("%s no package id found for -package_id argument\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        (*args_processed_p) = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_cli_rdgen_get_filter()
 **************************************/


/*!**************************************************************
 * fbe_cli_rdgen_get_operation_from_char()
 ****************************************************************
 * @brief
 *  Return the opcode we should use from a char that was entered
 *  on the command line.
 *
 * @param op_ch - char entered on command line
 * @param op_p - operation that is being returned.
 *
 * @return FBE_STATUS_OK on success.
 *         FBE_STATUS_GENERIC_FAILURE if input char not valid.
 *
 * @author
 *  4/14/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_cli_rdgen_get_operation_from_char(fbe_u8_t op_ch, fbe_rdgen_operation_t *op_p)
{
    switch (op_ch)
    {
        case 'c':
            *op_p = FBE_RDGEN_OPERATION_WRITE_READ_CHECK;
            break;
        case 'd':
            *op_p = FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY;
            break;
        case 'e':
            *op_p = FBE_RDGEN_OPERATION_VERIFY_WRITE;
            break;
        case 'h':
            *op_p = FBE_RDGEN_OPERATION_READ_CHECK;
            break;
        case 'i':
            *op_p = FBE_RDGEN_OPERATION_VERIFY_WRITE_READ_CHECK;
            break;
        case 'o':
            *op_p = FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK;
            break;
        case 'r':
            *op_p = FBE_RDGEN_OPERATION_READ_ONLY;
            break;
        case 'v':
            *op_p = FBE_RDGEN_OPERATION_VERIFY;
            break;
        case 'w':
            *op_p = FBE_RDGEN_OPERATION_WRITE_ONLY;
            break;
        case 's':
            *op_p = FBE_RDGEN_OPERATION_WRITE_SAME;
            break;
        case 't':
            *op_p = FBE_RDGEN_OPERATION_WRITE_SAME_READ_CHECK;
            break;
        case 'u':
            *op_p = FBE_RDGEN_OPERATION_UNMAP;
            break;
        case 'x':
            *op_p = FBE_RDGEN_OPERATION_CORRUPT_DATA_READ_CHECK;
            break;
        case 'y':
            *op_p = FBE_RDGEN_OPERATION_ZERO_READ_CHECK;
            break;
        case 'z':
            *op_p = FBE_RDGEN_OPERATION_ZERO_ONLY;
            break;
        default:
            *op_p = FBE_RDGEN_OPERATION_INVALID;
            return FBE_STATUS_GENERIC_FAILURE;
            break;
    };
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cli_rdgen_get_operation_from_char()
 ******************************************/
/*!**************************************************************
 * fbe_cli_rdgen_display_context()
 ****************************************************************
 * @brief
 *  Display details on rdgen context.
 *
 * @param context_p -     
 * @param b_start - FBE_TRUE if context is starting.
 *                 FBE_FALSE if context is complete.          
 *
 * @return None.   
 *
 * @author
 *  10/23/2012 - Created. Rob Foley
 *
 ****************************************************************/

static void fbe_cli_rdgen_display_context(fbe_cli_rdgen_context_t *context_p,
                                          fbe_bool_t b_start)
{
    const fbe_char_t *operation_string_p = NULL;
    const fbe_char_t *rdgen_status_string_p = NULL;
    const fbe_char_t *interface_string_p = NULL;
    const fbe_char_t *lba_spec_p = NULL;
    const fbe_char_t *block_spec_p = NULL;
    double io_rate;
    fbe_api_rdgen_get_operation_string(context_p->context.start_io.specification.operation,
                                       &operation_string_p);
    fbe_api_rdgen_get_status_string(context_p->context.start_io.status.rdgen_status,
                                          &rdgen_status_string_p);
    fbe_api_rdgen_get_block_spec_string(context_p->context.start_io.specification.block_spec,
                                          &block_spec_p);
    fbe_api_rdgen_get_lba_spec_string(context_p->context.start_io.specification.lba_spec,
                                      &lba_spec_p);
    fbe_api_rdgen_get_interface_string(context_p->context.start_io.specification.io_interface, &interface_string_p);
    if (b_start) {
        fbe_cli_printf("context: %p rdgen_operation: 0x%x (%s) Starting Rdgen Operation\n",
                       context_p, context_p->context.start_io.specification.operation, operation_string_p);
    } else {
        fbe_cli_printf("context: %p rdgen_operation: 0x%x (%s) rdgen_status: 0x%x (%s)\n",
                       context_p, context_p->context.start_io.specification.operation, operation_string_p,
                       context_p->context.start_io.status.rdgen_status, rdgen_status_string_p);
    }
    if (context_p->context.start_io.specification.io_interface != FBE_RDGEN_INTERFACE_TYPE_PACKET)
    {
        fbe_cli_printf("context: %p object_id: %d interface: %s name: %s\n", 
                       context_p, context_p->context.start_io.filter.object_id, interface_string_p, 
                       context_p->context.start_io.filter.device_name);
    }
    else
    {
        fbe_cli_printf("context: %p object_id: 0x%x class_id: 0x%x package_id: 0x%x interface: %s\n", 
                       context_p, context_p->context.start_io.filter.object_id, 
                       context_p->context.start_io.filter.class_id,
                       context_p->context.start_io.filter.package_id, interface_string_p);
    }

    fbe_cli_printf("context: %p num_ios_to_run %x, num_msec_to_run %x, threads %x pattern: 0x%x\n", 
                   context_p,
                   context_p->context.start_io.specification.max_number_of_ios,
                   (unsigned int)context_p->context.start_io.specification.msec_to_run,
                   context_p->context.start_io.specification.threads,
                   context_p->context.start_io.specification.pattern);

	fbe_cli_printf("context: %p lba_spec %x (%s) start_lba %llX, min_lba %llX, max_lba %llX inc_lba %llx\n",
                   context_p,
                   context_p->context.start_io.specification.lba_spec, lba_spec_p,
                   context_p->context.start_io.specification.start_lba,
                   context_p->context.start_io.specification.min_lba,
                   context_p->context.start_io.specification.max_lba, 
                   context_p->context.start_io.specification.inc_lba);

	fbe_cli_printf("context: %p block_spec %x (%s) min_blocks %llX, io_size_blocks %llX inc_blocks %llx\n",
                   context_p,
                   context_p->context.start_io.specification.block_spec, block_spec_p,
                   context_p->context.start_io.specification.min_blocks,
                   context_p->context.start_io.specification.max_blocks,
                   context_p->context.start_io.specification.inc_blocks);

	fbe_cli_printf("context: %p options %llx align_blocks: %d io_interface: %d peer_options: %d priority: %d\n",
                   context_p,
                   context_p->context.start_io.specification.options, 
                   context_p->context.start_io.specification.alignment_blocks,
                   context_p->context.start_io.specification.io_interface,
                   context_p->context.start_io.specification.peer_options,
                   context_p->context.start_io.specification.priority);

    fbe_cli_printf("context: %p pass_count: 0x%x io_count: 0x%x error_count: 0x%x\n",
                   context_p,
                   context_p->context.start_io.statistics.pass_count,
                   context_p->context.start_io.statistics.io_count,
                   context_p->context.start_io.statistics.error_count);
    if (!b_start) {
        
        fbe_cli_printf("context: %p abort_err: 0x%x invalid_request: 0x%x io_failures: 0x%x media: 0x%x\n",
                       context_p,
                       context_p->context.start_io.statistics.aborted_error_count,
                       context_p->context.start_io.statistics.invalid_request_err_count,
                       context_p->context.start_io.statistics.io_failure_error_count,
                       context_p->context.start_io.statistics.media_error_count);
        if (context_p->context.start_io.statistics.elapsed_msec < 1000) {
            io_rate = 0;
        } else {
            io_rate = ((double)(context_p->context.start_io.statistics.io_count) / 
                       ((double)(context_p->context.start_io.statistics.elapsed_msec) / 1000.0));
        }
        fbe_cli_printf("context: %p ios/second: %9.2f ios: %u elapsed msec: %llu\n",
                       context_p, io_rate, context_p->context.start_io.statistics.io_count,
                       context_p->context.start_io.statistics.elapsed_msec);
    }
}
/******************************************
 * end fbe_cli_rdgen_display_context()
 ******************************************/
/*!**************************************************************
 * fbe_cli_rdgen_test_io_completion()
 ****************************************************************
 * @brief
 *  The completion function for sending tests to rdgen.
 *
 * @param packet_p
 * @param context
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  10/22/2012 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t 
fbe_cli_rdgen_test_io_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_cli_rdgen_context_t *context_p = (fbe_cli_rdgen_context_t *)context;
    fbe_bool_t b_is_on_queue = FBE_FALSE;

    /* The spinlock ensures that we only display one thread completion at a time. 
     * This prevents the output from looking garbled if multiple requests complete at once.
     */
    fbe_spinlock_lock(&fbe_cli_rdgen_info.lock);
    b_is_on_queue = fbe_queue_is_element_on_queue(&context_p->queue_element);
    if (b_is_on_queue)
    {
        fbe_cli_rdgen_info_dequeue(context_p);
    }
	fbe_spinlock_unlock(&fbe_cli_rdgen_info.lock);

    fbe_cli_printf("\r\nrdgen operation complete: context: %p\n", context_p);
    fbe_cli_rdgen_display_context(context_p, FBE_FALSE /* Not start */);
    

    fbe_api_rdgen_test_io_completion(packet_p, &context_p->context);

    /* If we found it on the queue, then we know another thread is not waiting for it, we can 
     * destroy it and free the memory. 
     */
    if (b_is_on_queue)
    {
        fbe_api_rdgen_test_context_destroy(&context_p->context);
        fbe_api_free_contiguous_memory(context_p);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cli_rdgen_test_io_completion()
 ******************************************/

/*!**************************************************************
 * fbe_cli_rdgen_start()
 ****************************************************************
 * @brief
 *  Send a start for rdgen threads.
 *
 * @param argc - count of arguments              
 * @param argv - vector of arguments
 * 
 * @return FBE_STATUS_OK - Started OK.
 *         FBE_STATUS_GENERIC_FAILURE - Some error occurred,
 *         error details output to console.
 *
 * @author
 *  3/27/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_cli_rdgen_start(fbe_u32_t argc, char** argv)
{
    char *argv_p = NULL;
    fbe_status_t status;
    fbe_cli_rdgen_context_t *context_p = NULL;
    fbe_rdgen_block_specification_t block_spec = FBE_RDGEN_BLOCK_SPEC_RANDOM;
    fbe_rdgen_lba_specification_t lba_spec = FBE_RDGEN_LBA_SPEC_RANDOM;
    fbe_rdgen_operation_t rdgen_operation;
    fbe_rdgen_options_t options = FBE_RDGEN_OPTIONS_INVALID;
    fbe_rdgen_extra_options_t extra_options = FBE_RDGEN_EXTRA_OPTIONS_INVALID;
    fbe_u32_t msec_delay = 0;
    fbe_rdgen_data_pattern_flags_t data_pattern_flags = FBE_RDGEN_DATA_PATTERN_FLAGS_INVALID;
    fbe_rdgen_pattern_t pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    fbe_u32_t args_processed = 0;
    fbe_rdgen_filter_t filter;
    fbe_lba_t start_lba = 0;
    fbe_lba_t min_lba = 0;
    fbe_lba_t max_lba = FBE_LBA_INVALID;
    fbe_u32_t threads = 1;
    fbe_block_count_t min_blocks = 1;
    fbe_block_count_t io_size_blocks = 1;
    fbe_u8_t rdgen_op_char = 0;
    fbe_bool_t b_filter_initted = FBE_FALSE;
    const fbe_char_t *operation_string_p = NULL;
    fbe_u32_t num_ios = 0;
    fbe_time_t num_msec = 0;
    fbe_rdgen_affinity_t affinity = FBE_RDGEN_AFFINITY_NONE;
    fbe_u32_t core = FBE_U32_MAX;
	fbe_u32_t pass_count = 0;
    fbe_bool_t b_align = FBE_FALSE;
    fbe_rdgen_interface_type_t io_interface = FBE_RDGEN_INTERFACE_TYPE_PACKET;
    fbe_api_rdgen_peer_options_t  peer_options = FBE_API_RDGEN_PEER_OPTIONS_INVALID;
    fbe_block_count_t inc_lba = 0;
    fbe_block_count_t inc_blocks = 0;
    fbe_u32_t abort_ms = 0;
    fbe_u32_t sequence_number;
    fbe_u32_t random_seed = 0;
    fbe_bool_t b_wait_reply = FBE_FALSE;
    fbe_packet_priority_t priority = FBE_PACKET_PRIORITY_INVALID;
    fbe_bool_t b_sync = FBE_FALSE;
	be_quiet = FBE_FALSE;

    fbe_api_rdgen_filter_init(&filter, FBE_RDGEN_FILTER_TYPE_CLASS,
                              FBE_OBJECT_ID_INVALID, FBE_CLASS_ID_LUN,
                              FBE_PACKAGE_ID_SEP_0, 0);

    context_p = (fbe_cli_rdgen_context_t*) fbe_api_allocate_contiguous_memory(sizeof(fbe_cli_rdgen_context_t));

    if (context_p == NULL)
    {
        fbe_api_free_contiguous_memory(context_p);
        fbe_cli_error("%s unable to allocate rdgen context\n", __FUNCTION__);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    
    /* Get all arguments.
     */
    while (argc && **argv == '-')
    {
        if (fbe_cli_rdgen_get_filter(argc, 
                                     argv, 
                                     &filter, 
                                     &args_processed) == FBE_STATUS_OK)
        {
            argc -= args_processed;
            argv += args_processed;
            b_filter_initted = FBE_TRUE;
        }
        else if (!strcmp(*argv, "-seq") || !strcmp(*argv, "-inc"))
        {
            /* Start a sequential I/O Thread.
             */
            lba_spec = FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING;
            *argv++;
            argc--;
        }
        else if (!strcmp(*argv, "-quiet"))
        {
            /* Start a sequential I/O Thread.
             */
            be_quiet = FBE_TRUE;
            *argv++;
            argc--;
        }
        else if (!strncmp(*argv, "-sync", 5))
        {
            b_sync = FBE_TRUE;
            *argv++;
            argc--;
        }		
        else if (!strcmp(*argv, "-cat_inc"))
        {
            /* Start a sequential increasing caterpillar I/O Thread.
             */
            lba_spec = FBE_RDGEN_LBA_SPEC_CATERPILLAR_INCREASING;
            *argv++;
            argc--;
        }
        else if (!strcmp(*argv, "-cat_dec"))
        {
            /* Start a sequential decreasing caterpillar I/O Thread.
             */
            lba_spec = FBE_RDGEN_LBA_SPEC_CATERPILLAR_DECREASING;
            *argv++;
            argc--;
        }
        else if (!strcmp(*argv, "-constant"))
        {
            /* Start a fixed size I/O Thread.
             */
            block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;
            *argv++;
            argc--;
        }
        else if (!strcmp(*argv, "-repeat_lba"))
        {
            /* Start a fixed size I/O Thread.
             */
            lba_spec = FBE_RDGEN_LBA_SPEC_FIXED;
            *argv++;
            argc--;
        }
        else if (!strcmp(*argv, "-inc_lba"))
        {
            argc--;
            argv++;
            if (argc)
            {
                status = fbe_atoh64(*argv, &inc_lba);

                if (status != FBE_STATUS_OK)
                {
                    fbe_api_free_contiguous_memory(context_p);
                    fbe_cli_error("rdgen: -inc_lba argument invalid.\n");
                    return FBE_STATUS_ATTRIBUTE_NOT_FOUND;
                }
                argc--;
                argv++;
            }
            else
            {
                fbe_api_free_contiguous_memory(context_p);
                fbe_cli_error("%s no argument found for -inc_lba argument\n", __FUNCTION__);
                return FBE_STATUS_ATTRIBUTE_NOT_FOUND;
            }
        }
        else if (!strcmp(*argv, "-inc_blocks"))
        {
            argc--;
            argv++;
            if (argc)
            {
                status = fbe_atoh64(*argv, &inc_blocks);

                if (status != FBE_STATUS_OK)
                {
                    fbe_api_free_contiguous_memory(context_p);
                    fbe_cli_error("rdgen: -inc_blocks argument invalid.\n");
                    return FBE_STATUS_ATTRIBUTE_NOT_FOUND;
                }
                block_spec = FBE_RDGEN_BLOCK_SPEC_INCREASING;
                argc--;
                argv++;
            }
            else
            {
                fbe_api_free_contiguous_memory(context_p);
                fbe_cli_error("%s no argument found for -inc_blocks argument\n", __FUNCTION__);
                return FBE_STATUS_ATTRIBUTE_NOT_FOUND;
            }
        }
        else if (!strcmp(*argv, "-min_blocks"))
        {
            argc--;
            argv++;
            if (argc)
            {
                status = fbe_atoh64(*argv, &min_blocks);

                if (status != FBE_STATUS_OK)
                {
                    fbe_api_free_contiguous_memory(context_p);
                    fbe_cli_error("rdgen: -min_blocks argument invalid.\n");
                    return FBE_STATUS_ATTRIBUTE_NOT_FOUND;
                }
                argc--;
                argv++;
            }
            else
            {
                fbe_api_free_contiguous_memory(context_p);
                fbe_cli_error("%s no argument found for -min_blocks argument\n", __FUNCTION__);
                return FBE_STATUS_ATTRIBUTE_NOT_FOUND;
            }
        }
        else if (!strcmp(*argv, "-max_lba"))
        {
            argc--;
            argv++;
            if (argc)
            {
                status = fbe_atoh64(*argv, &max_lba);

                if (status != FBE_STATUS_OK)
                {
                    fbe_api_free_contiguous_memory(context_p);
                    fbe_cli_error("rdgen: -max_lba argument invalid.\n");
                    return FBE_STATUS_ATTRIBUTE_NOT_FOUND;
                }
                argc--;
                argv++;
            }
            else
            {
                fbe_api_free_contiguous_memory(context_p);
                fbe_cli_error("%s no argument found for -max_lba argument\n", __FUNCTION__);
                return FBE_STATUS_ATTRIBUTE_NOT_FOUND;
            }
        }
        else if (!strcmp(*argv, "-min_lba"))
        {
            argc--;
            argv++;
            if (argc)
            {
                status = fbe_atoh64(*argv, &min_lba);

                if (status != FBE_STATUS_OK)
                {
                    fbe_api_free_contiguous_memory(context_p);
                    fbe_cli_error("rdgen: -min_lba argument invalid.\n");
                    return FBE_STATUS_ATTRIBUTE_NOT_FOUND;
                }
                argc--;
                argv++;
            }
            else
            {
                fbe_api_free_contiguous_memory(context_p);
                fbe_cli_error("%s no argument found for -min_lba argument\n", __FUNCTION__);
                return FBE_STATUS_ATTRIBUTE_NOT_FOUND;
            }
        }
        else if (!strcmp(*argv, "-start_lba"))
        {
            argc--;
            argv++;
            if (argc)
            {
                status = fbe_atoh64(*argv, &start_lba);

                if (status != FBE_STATUS_OK)
                {
                    fbe_api_free_contiguous_memory(context_p);
                    fbe_cli_error("rdgen: -start_lba argument invalid.\n");
                    return FBE_STATUS_ATTRIBUTE_NOT_FOUND;
                }
                argc--;
                argv++;
            }
            else
            {
                fbe_api_free_contiguous_memory(context_p);
                fbe_cli_error("%s no argument found for -start_lba argument\n", __FUNCTION__);
                return FBE_STATUS_ATTRIBUTE_NOT_FOUND;
            }
        }
        else if (!strcmp(*argv, "-align"))
        {
            /* Start a fixed size I/O Thread.
             */
            b_align = FBE_TRUE;
            *argv++;
            argc--;
        }
        else if (!strcmp(*argv, "-dec"))
        {
            /* Start a sequential decreasing thread.
             */
            lba_spec = FBE_RDGEN_LBA_SPEC_SEQUENTIAL_DECREASING;
            *argv++;
            argc--;
        } 
        else if (!strcmp(*argv, "-fixed_sequence"))
        {
            /* Make sure the sequence number does not increment.
             */
            data_pattern_flags |= FBE_RDGEN_DATA_PATTERN_FLAGS_FIXED_SEQUENCE_NUMBER;
            *argv++;
            argc--;
        }  
        else if (!strcmp(*argv, "-sequence_number"))
        {
            /* Use the same sequence number.
             */
            options |= FBE_RDGEN_OPTIONS_USE_SEQUENCE_COUNT_SEED;
            argc--;
            argv++;
            if (argc)
            {
                sequence_number = (fbe_u32_t)strtoul(*argv, 0, 0);

                if ((sequence_number == 0) && strcmp(*argv, "0") && strcmp(*argv, "0x0"))
                {
                    /* strtoul returns 0 if you enter a hex value without 0x. */
                    fbe_api_free_contiguous_memory(context_p);
                    fbe_cli_error("unexpected argument found for -sequence_number argument. \n");
                    fbe_cli_error("Enter either -sequence_number (decimal number) or -sequence_number 0x(hex number) \n");
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                argc--;
                argv++;
            }
            else
            {
                fbe_api_free_contiguous_memory(context_p);
                fbe_cli_error("%s no argument found for -sequence_number argument\n", __FUNCTION__);
                return FBE_STATUS_ATTRIBUTE_NOT_FOUND;
            }
        }  
        else if (!strcmp(*argv, "-msec_delay"))
        {
            /* Use a specific delay.
             */
            argc--;
            argv++;
            if (argc)
            {
                msec_delay = (fbe_u32_t)strtoul(*argv, 0, 0);

                if ((msec_delay == 0) && strcmp(*argv, "0") && strcmp(*argv, "0x0"))
                {
                    /* strtoul returns 0 if you enter a hex value without 0x. */
                    fbe_api_free_contiguous_memory(context_p);
                    fbe_cli_error("unexpected argument found for -msec_delay argument. \n");
                    fbe_cli_error("Enter either -msec_delay (decimal number) or -msec_delay 0x(hex number) \n");
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                argc--;
                argv++;
            }
            else
            {
                fbe_api_free_contiguous_memory(context_p);
                fbe_cli_error("%s no argument found for -msec_delay argument\n", __FUNCTION__);
                return FBE_STATUS_ATTRIBUTE_NOT_FOUND;
            }
        }    
        else if (!strcmp(*argv, "-random_delay"))
        {
            extra_options |= FBE_RDGEN_EXTRA_OPTIONS_RANDOM_DELAY;
            /* Use a random delay.
             */
            argc--;
            argv++;
            if (argc)
            {
                msec_delay = (fbe_u32_t)strtoul(*argv, 0, 0);

                if ((msec_delay == 0) && strcmp(*argv, "0") && strcmp(*argv, "0x0"))
                {
                    /* strtoul returns 0 if you enter a hex value without 0x. */
                    fbe_api_free_contiguous_memory(context_p);
                    fbe_cli_error("unexpected argument found for -msec_delay argument. \n");
                    fbe_cli_error("Enter either -msec_delay (decimal number) or -msec_delay 0x(hex number) \n");
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                argc--;
                argv++;
            }
            else
            {
                fbe_api_free_contiguous_memory(context_p);
                fbe_cli_error("%s no argument found for -msec_delay argument\n", __FUNCTION__);
                return FBE_STATUS_ATTRIBUTE_NOT_FOUND;
            }
        }  
        else if (!strcmp(*argv, "-fixed_random_seed"))
        {
            /* Generate fixed random numbers that can be repeated run to run.
             */
            options |= FBE_RDGEN_OPTIONS_FIXED_RANDOM_NUMBERS;
            argc--;
            argv++;
            if (argc)
            {
                random_seed = (fbe_u32_t)strtoul(*argv, 0, 0);
                if ((random_seed == 0) && strcmp(*argv, "0") && strcmp(*argv, "0x0"))
                {
                    /* strtoul returns 0 if you enter a hex value without 0x. */
                    fbe_api_free_contiguous_memory(context_p);
                    fbe_cli_error("unexpected argument found for -fixed_random_seed argument. \n");
                    fbe_cli_error("Enter either -fixed_random_seed (decimal number) or -fixed_random_seed 0x(hex number) \n");
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                argc--;
                argv++;
            }
            else
            {
                fbe_api_free_contiguous_memory(context_p);
                fbe_cli_error("%s no argument found for -fixed_random_seed argument\n", __FUNCTION__);
                return FBE_STATUS_ATTRIBUTE_NOT_FOUND;
            }
        } 
        else if (!strcmp(*argv, "-abort"))
        {
            /* Set abort flag so we will abort I/Os randomly with a
             * fixed abort time. 
             */
            options |= FBE_RDGEN_OPTIONS_RANDOM_ABORT;
            options |= FBE_RDGEN_OPTIONS_CONTINUE_ON_ERROR;
            abort_ms = FBE_RDGEN_CLI_RANDOM_ABORT_MS;
            *argv++;
            argc--;
        }
        else if (!strcmp(*argv, "-continue_on_error"))
        {
            options |= FBE_RDGEN_OPTIONS_CONTINUE_ON_ERROR;
            *argv++;
            argc--;
        }
        else if (!strcmp(*argv, "-continue_on_all_errors"))
        {
            options |= FBE_RDGEN_OPTIONS_CONTINUE_ON_ALL_ERRORS;
            *argv++;
            argc--;
        }
        else if (!strcmp(*argv, "-panic_on_all_errors"))
        {
            options |= FBE_RDGEN_OPTIONS_PANIC_ON_ALL_ERRORS;
            *argv++;
            argc--;
        }
        else if (!strcmp(*argv, "-random_start_lba"))
        {
            options |= FBE_RDGEN_OPTIONS_RANDOM_START_LBA;
            *argv++;
            argc--;
        }
        else if (!strcmp(*argv, "-no_panic_on_dmc") || !strcmp(*argv, "-no_panic"))
        {
            options |= FBE_RDGEN_OPTIONS_DO_NOT_PANIC_ON_DATA_MISCOMPARE;
            *argv++;
            argc--;
        }
        else if (!strcmp(*argv, "-check_crc"))
        {
            data_pattern_flags |= FBE_RDGEN_DATA_PATTERN_FLAGS_CHECK_CRC;
            *argv++;
            argc--;
        }
        else if (!strcmp(*argv, "-check_lba_stamp"))
        {
            data_pattern_flags |= FBE_RDGEN_DATA_PATTERN_FLAGS_CHECK_LBA_STAMP;
            *argv++;
            argc--;
        }
        else if (!strcmp(*argv, "-non_cached"))
        {
            options |= FBE_RDGEN_OPTIONS_NON_CACHED;
            *argv++;
            argc--;
        }
        else if (!strcmp(*argv, "-random_abort"))
        {
            /* Set abort flag so we will abort I/Os also.
             */
            options |= FBE_RDGEN_OPTIONS_RANDOM_ABORT;
            *argv++;
            argc--;
            if (argc)
            {
                abort_ms = (fbe_u32_t)strtoul(*argv, 0, 0);

                if (abort_ms == 0)
                {
                    /* strtoul returns 0 if you enter a hex value without 0x. */
                    fbe_api_free_contiguous_memory(context_p);
                    fbe_cli_error("unexpected argument found for -random_abort argument. \n");
                    fbe_cli_error("Enter either -random_abort (decimal number) or -random_abort 0x(hex number) \n");
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                argc--;
                argv++;
            }
            else
            {
                fbe_api_free_contiguous_memory(context_p);
                fbe_cli_error("%s no argument found for -random_abort argument\n", __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        else if (!strcmp(*argv, "-abort_ms"))
        {
            argc--;
            argv++;
            if (argc)
            {
                abort_ms = (fbe_u32_t)strtoul(*argv, 0, 0);

                if (abort_ms == 0)
                {
                    /* strtoul returns 0 if you enter a hex value without 0x. */
                    fbe_api_free_contiguous_memory(context_p);
                    fbe_cli_error("unexpected argument found for -abort_ms argument. \n");
                    fbe_cli_error("Enter either -abort_ms (decimal number) or -abort_ms 0x(hex number) \n");
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                argc--;
                argv++;
            }
            else
            {
                fbe_api_free_contiguous_memory(context_p);
                fbe_cli_error("%s no argument found for -abort_ms argument\n", __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        else if (!strcmp(*argv, "-cp"))
        {
            /* Check pattern.  Read the entire LUN, expecting a fixed pattern. This will only do one pass count. 
             */
            rdgen_operation = FBE_RDGEN_OPERATION_READ_CHECK;
            block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;
            lba_spec = FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING;
            pass_count = 1; /* Run 1 pass only */
            *argv++;
            argc--;
        }
        else if (!strcmp(*argv, "-sp"))
        {
            /* Write the entire LUN with a given pattern.  This will only do one pass count.
             */
            rdgen_operation = FBE_RDGEN_OPERATION_WRITE_ONLY;
            block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;
            lba_spec = FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING;
            pass_count = 1; /* Run 1 pass only */
            *argv++;
            argc--;
        }
        else if (!strcmp(*argv, "-num_ios"))
        {
            argc--;
            argv++;
            if (argc)
            {
                num_ios = (fbe_u32_t)strtoul(*argv, 0, 0);

                if (num_ios == 0)
                {
                    /* strtoul returns 0 if you enter a hex value without 0x. */
                    fbe_api_free_contiguous_memory(context_p);
                    fbe_cli_error("unexpected argument found for -num_ios argument. \n");
                    fbe_cli_error("Enter either -num_ios (decimal number) or -num_ios 0x(hex number) \n");
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                argc--;
                argv++;
            }
            else
            {
                fbe_api_free_contiguous_memory(context_p);
                fbe_cli_error("%s no argument found for -num_ios argument\n", __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        else if (!strcmp(*argv, "-clear"))
        {
            argc--;
            argv++;
            pattern = FBE_RDGEN_PATTERN_CLEAR;
        }
        else if (!strcmp(*argv, "-zeros"))
        {
            argc--;
            argv++;
            pattern = FBE_RDGEN_PATTERN_ZEROS;
        }
        else if (!strcmp(*argv, "-perf") || !strcmp(*argv, "-performance"))
        {
            argc--;
            argv++;
            options |= FBE_RDGEN_OPTIONS_PERFORMANCE | FBE_RDGEN_OPTIONS_DO_NOT_CHECK_CRC;
        }
        else if (!strcmp(*argv, "-perf1"))
        {
            argc--;
            argv++;
            options |= FBE_RDGEN_OPTIONS_PERFORMANCE | FBE_RDGEN_OPTIONS_DO_NOT_CHECK_CRC | FBE_RDGEN_OPTIONS_OPTIMAL_RANDOM;
        }
        else if (!strcmp(*argv, "-allow_fail_congestion") || !strcmp(*argv, "-afc"))
        {
            argc--;
            argv++;
            options |= FBE_RDGEN_OPTIONS_ALLOW_FAIL_CONGESTION;
        }
        else if (!strcmp(*argv, "-num_msec"))
        {
            argc--;
            argv++;
            if (argc)
            {
                num_msec = (fbe_u32_t)strtoul(*argv, 0, 0);
                if (num_msec == 0)
                {
                    /* strtoul returns 0 if you enter a hex value without 0x. */
                    fbe_api_free_contiguous_memory(context_p);
                    fbe_cli_error("unexpected argument found for -num_msec argument. \n");
                    fbe_cli_error("Enter either -num_msec (decimal number) or -num_msec 0x(hex number) \n");
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                argc--;
                argv++;
            }
            else
            {
                fbe_api_free_contiguous_memory(context_p);
                fbe_cli_error("%s no argument found for -num_ios argument\n", __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        else if (!strcmp(*argv, "-affine"))
        {
            argc--;
            argv++;
            if (argc)
            {
                affinity = FBE_RDGEN_AFFINITY_FIXED;
                core = (fbe_u32_t)strtoul(*argv, 0, 0);

                if ((core == 0) && strcmp(*argv, "0") && strcmp(*argv, "0x0"))
                {
                    /* strtoul returns 0 if you enter a hex value without 0x. */
                    fbe_api_free_contiguous_memory(context_p);
                    fbe_cli_error("unexpected argument found for -affine argument. \n");
                    fbe_cli_error("Enter either -affine (decimal number) or -affine 0x(hex number) \n");
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                argc--;
                argv++;
            }
            else
            {
                fbe_api_free_contiguous_memory(context_p);
                fbe_cli_error("%s no argument found for -core argument\n", __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        else if (!strcmp(*argv, "-pass_count"))
        {
            argc--;
            argv++;
            if (argc)
            {
                pass_count = (fbe_u32_t)strtoul(*argv, 0, 0);
                if (pass_count == 0)
                {
                    /* strtoul returns 0 if you enter a hex value without 0x. */
                    fbe_api_free_contiguous_memory(context_p);
                    fbe_cli_error("unexpected argument found for -pass_count argument. \n");
                    fbe_cli_error("Enter either -pass_count (decimal number) or -pass_count 0x(hex number) \n");
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                argc--;
                argv++;
            }
            else
            {
                fbe_api_free_contiguous_memory(context_p);
                fbe_cli_error("%s no argument found for -pass_count argument\n", __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        else if (!strcmp(*argv, "-affinity_spread") || !strcmp(*argv, "-as"))
        {
            argc--;
            argv++;
            affinity = FBE_RDGEN_AFFINITY_SPREAD;
        }
        else if (!strcmp(*argv, "-irp_dca"))
        {
            argc--;
            argv++;
            io_interface = FBE_RDGEN_INTERFACE_TYPE_IRP_DCA;
        }
        else if (!strcmp(*argv, "-irp_sgl"))
        {
            argc--;
            argv++;
            io_interface = FBE_RDGEN_INTERFACE_TYPE_IRP_SGL;
        }
        else if (!strcmp(*argv, "-irp_mj"))
        {
            argc--;
            argv++;
            io_interface = FBE_RDGEN_INTERFACE_TYPE_IRP_MJ;
        }
        else if (!strcmp(*argv, "-peer_memory_init"))
        {
            argc--;
            argv++;
            status = fbe_api_rdgen_allocate_peer_memory();

            if (status != FBE_STATUS_OK)
            {
                fbe_api_free_contiguous_memory(context_p);
                fbe_cli_error("%s -peer_memory_init failed\n", __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        else if (!strcmp(*argv, "-peer_options"))
        {
            argc--;
            argv++;
            if (argc)
            {
                if (!strcmp(*argv, "peer_random"))
                {
                    peer_options = FBE_API_RDGEN_PEER_OPTIONS_RANDOM;
                } 
                else if (!strcmp(*argv, "load_balance"))
                {
                    peer_options = FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE;
                }
                else if (!strcmp(*argv, "peer_only"))
                {
                    peer_options = FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER;
                }
                argc--;
                argv++;
            }
            else
            {
                fbe_cli_error("%s no argument found for -peer_options argument\n", __FUNCTION__);
            }
        }
        else if (!strcmp(*argv, "-wait_reply"))
        {
            argc--;
            argv++;

            b_wait_reply = FBE_TRUE;
        }
        else if (!strcmp(*argv, "-priority"))
        {
            argc--;
            argv++;
            if (argc)
            {
                priority = (fbe_u32_t)strtoul(*argv, 0, 0);
                if ((priority == FBE_PACKET_PRIORITY_INVALID) ||
                    (priority >= FBE_PACKET_PRIORITY_LAST)){
                    fbe_api_free_contiguous_memory(context_p);
                    fbe_cli_error("%s priority argument %d is invalid priority must be between %d and %d inclusive.\n",
                                  __FUNCTION__, priority, FBE_PACKET_PRIORITY_INVALID + 1, FBE_PACKET_PRIORITY_LAST - 1);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                argc--;
                argv++;
            }
            else
            {
                fbe_api_free_contiguous_memory(context_p);
                fbe_cli_error("%s no argument found for -priority argument\n", __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        else if (!strcmp(*argv, "-options"))
        {
            argc--;
            argv++;
            if (argc)
            {
                options |=(fbe_u32_t)strtoul(*argv, 0, 0);
                argc--;
                argv++;
            }
            else
            {
                fbe_api_free_contiguous_memory(context_p);
                fbe_cli_error("%s no argument found for -options argument\n", __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE;
            }

        }
        else 
        {
            fbe_api_free_contiguous_memory(context_p);
            fbe_cli_error("%s rdgen switch %s is not a valid\n", __FUNCTION__, *argv);
            fbe_cli_rdgen_display_usage();
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    if (filter.filter_type == FBE_RDGEN_FILTER_TYPE_PLAYBACK_SEQUENCE)
    {
        /* Always want to read from a file when there is a playback.
         */
        options |= FBE_RDGEN_OPTIONS_PERFORMANCE | FBE_RDGEN_OPTIONS_FILE;
        /* Initialize and send out the context.
         */ 
        status = fbe_api_rdgen_test_context_init(&context_p->context, 
                                                 0x1, 
                                                 filter.class_id,
                                                 filter.package_id,
                                                 FBE_RDGEN_OPERATION_WRITE_ONLY,
                                                 pattern,
                                                 pass_count,    //0, /* Pass count means run until stopped. */
                                                 num_ios,
                                                 num_msec,
                                                 1,
                                                 FBE_RDGEN_LBA_SPEC_FIXED,
                                                 start_lba,
                                                 min_lba,
                                                 max_lba,
                                                 block_spec,
                                                 min_blocks,
                                                 io_size_blocks    /* Max blocks */ );
        if (status != FBE_STATUS_OK)
        {
            fbe_api_free_contiguous_memory(context_p);
            fbe_cli_error("%s unable to init the rdgen context.  status: 0x%x\n", __FUNCTION__, status);
            return status; 
        }
        context_p->context.start_io.filter = filter;

        status = fbe_api_rdgen_io_specification_set_options(&context_p->context.start_io.specification, options);
        if (status != FBE_STATUS_OK) { 
            fbe_api_free_contiguous_memory(context_p);
            fbe_cli_error("%s unable to set the options flags.  status: 0x%x\n", __FUNCTION__, status);
            return status; 
        }
        fbe_api_rdgen_set_context(&context_p->context, fbe_cli_rdgen_test_io_completion, context_p);
        if (!b_sync){
            fbe_cli_rdgen_info_enqueue(context_p);
        }
        else {
            if (context_p->context.start_io.specification.max_passes == 0){
                context_p->context.start_io.specification.max_passes = 1;
            }
            fbe_cli_rdgen_info_init(&fbe_cli_rdgen_info);
            fbe_queue_element_init(&context_p->queue_element);
        }
        status = fbe_api_rdgen_start_tests(&context_p->context, FBE_PACKAGE_ID_NEIT, 1);
        if (b_sync){
            fbe_bool_t b_done = FBE_FALSE;
            fbe_cli_printf("Waiting for rdgen playback to finish\n");
            while (!b_done){
                status = fbe_semaphore_wait_ms(&context_p->context.semaphore, (fbe_u32_t)0);

                if (status == FBE_STATUS_TIMEOUT){
                    fbe_cli_printf("rdgen playback still not done\n");
                    status = fbe_cli_rdgen_display(0, NULL, FBE_RDGEN_DISPLAY_MODE_DEFAULT);
                }
                else if (status != FBE_STATUS_OK){
                    fbe_cli_error("%s wait for rdgen failed\n", __FUNCTION__);
                    return status;
                }
                else {
                    b_done = FBE_TRUE;
                    fbe_api_rdgen_test_context_destroy(&context_p->context);
                    fbe_api_free_contiguous_memory(context_p);
                }
            }
        }
        return FBE_STATUS_OK;
    }
    if (argc < 3)
    {
        fbe_api_free_contiguous_memory(context_p);
        fbe_cli_error("%s Not enough rdgen parameters.\n", __FUNCTION__);
        fbe_cli_rdgen_display_usage();
        return FBE_STATUS_GENERIC_FAILURE;
    }


    /* The first argument is a single character representing the desired RDT op.
     */
    rdgen_op_char = **argv;
    status = fbe_cli_rdgen_get_operation_from_char(rdgen_op_char, &rdgen_operation);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_api_free_contiguous_memory(context_p);
        fbe_cli_error("%s rdgen opcode char %c is not a valid operation status: 0x%x\n", __FUNCTION__, rdgen_op_char, status);
        fbe_cli_rdgen_display_usage();
        return status; 
    }
    argv++;

    /* If the filter was not already initted above, then get the lun number.
     */
    if (!b_filter_initted)
    {
        /* The next argument is the "LUN Number" (interpreted in DECIMAL).
         */
        if (!strcmp(*argv, "*"))
        {
            /* Start on all LUNs.  Filter already set to LUNs.
             */
        }
        else
        {
            fbe_lun_number_t lun_id;
            lun_id = strtoul(*argv, 0, 0);
            if ((lun_id == 0) && strcmp(*argv, "0") && strcmp(*argv, "0x0"))
            {
                /* strtoul returns 0 if you enter a hex value without 0x. */
                fbe_api_free_contiguous_memory(context_p);
                fbe_cli_error("unexpected argument found for LUN. \n");
                fbe_cli_error("Enter either (decimal number) or 0x(hex number)\n");
                return FBE_STATUS_GENERIC_FAILURE;
            }
            filter.filter_type = FBE_RDGEN_FILTER_TYPE_OBJECT;
            status = fbe_api_database_lookup_lun_by_number(lun_id, &filter.object_id);
            if (status != FBE_STATUS_OK)
            {
                fbe_api_free_contiguous_memory(context_p);
                fbe_cli_error("%s rdgen lun %d not valid status: 0x%x\n", __FUNCTION__, lun_id, status);
                return status; 
            }
        }
    }
    argv++;

    /* The next argument is the thread value (always interpreted in HEX).
     */
    argv_p = *argv;
    if ((*argv_p == '0') && (*(argv_p + 1) == 'x' || *(argv_p + 1) == 'X'))
    {
        argv_p += 2;
    }
    threads = fbe_atoh(argv_p);
    if (threads == FBE_U32_MAX)
    {
        fbe_api_free_contiguous_memory(context_p);
        fbe_cli_error("rdgen: ERROR: Invalid argument (threads).\n");
        fbe_cli_rdgen_display_usage();
        return FBE_STATUS_GENERIC_FAILURE;
    }
    argv++;

    /* The next argument is the block count (interpreted in HEX).
     */
    argv_p = *argv;
    if (argv_p && *argv_p)
    {
        if ((*argv_p == '0') && (*(argv_p + 1) == 'x' || *(argv_p + 1) == 'X'))
        {
            argv_p += 2;
        }
        io_size_blocks = fbe_atoh(argv_p);
    }
    fbe_api_rdgen_get_operation_string(rdgen_operation, &operation_string_p);

    if (block_spec == FBE_RDGEN_BLOCK_SPEC_CONSTANT)
    {
        min_blocks = io_size_blocks;
        fbe_cli_printf("choosing constant block count of 0x%llx\n", min_blocks);
    }
    if (lba_spec == FBE_RDGEN_LBA_SPEC_FIXED)
    {
        /* When we issue a fixed lba operation we will use the "thread" number 
         * for the lba, and we will only start a single thread.
         */
        start_lba = threads;
        min_lba = threads;
        threads = 1;
        fbe_cli_printf("choosing fixed lba of 0x%llx\n", min_lba);
    }

    /* Initialize and send out the context.
     */
    status = fbe_api_rdgen_test_context_init(&context_p->context, 
                                             filter.object_id, 
                                             filter.class_id,
                                             filter.package_id,
                                             rdgen_operation,
                                             pattern,
                                             pass_count, //0, /* Pass count means run until stopped. */
                                             num_ios,
                                             num_msec,
                                             threads,
                                             lba_spec,
                                             start_lba,
                                             min_lba,
                                             max_lba,
                                             block_spec,
                                             min_blocks,
                                             io_size_blocks  /* Max blocks */ );
    if (status != FBE_STATUS_OK) 
    { 
        fbe_api_free_contiguous_memory(context_p);
        fbe_cli_error("%s unable to init the rdgen context.  status: 0x%x\n", __FUNCTION__, status);
        return status; 
    }
    context_p->context.start_io.filter = filter;

    if (filter.filter_type == FBE_RDGEN_FILTER_TYPE_DISK_INTERFACE)
    {
        /* By default, use the dca interface, otherwise use what the user specified.
         */
        if (io_interface == FBE_RDGEN_INTERFACE_TYPE_PACKET)
        {
            io_interface = FBE_RDGEN_INTERFACE_TYPE_IRP_DCA;
        }
        status = fbe_api_rdgen_io_specification_set_interface( &context_p->context.start_io.specification, 
                                                               io_interface );
        if (status != FBE_STATUS_OK)
        {
            fbe_api_free_contiguous_memory(context_p);
            fbe_cli_error("%s unable to init the rdgen interface.  status: 0x%x\n", __FUNCTION__, status);
            return status; 
        }
    }
    if (filter.filter_type == FBE_RDGEN_FILTER_TYPE_BLOCK_DEVICE)
    {
        status = fbe_api_rdgen_io_specification_set_interface( &context_p->context.start_io.specification, 
                                                               FBE_RDGEN_INTERFACE_TYPE_SYNC_IO);
        if (status != FBE_STATUS_OK)
        {
            fbe_api_free_contiguous_memory(context_p);
            fbe_cli_error("%s unable to init the rdgen interface.  status: 0x%x\n", __FUNCTION__, status);
            return status; 
        }
    }
    if ( ((filter.filter_type == FBE_RDGEN_FILTER_TYPE_OBJECT) ||
          (filter.filter_type == FBE_RDGEN_FILTER_TYPE_CLASS) ||
          (filter.filter_type == FBE_RDGEN_FILTER_TYPE_RAID_GROUP)) &&
         (io_interface != FBE_RDGEN_INTERFACE_TYPE_PACKET) )
    {
        const fbe_char_t *interface_string_p = NULL;
        fbe_api_rdgen_get_interface_string(io_interface, &interface_string_p);
        fbe_cli_error("%s irp interface selected (%s) but device name is missing.\n", 
                      __FUNCTION__, interface_string_p);
        fbe_cli_error("Please use -device or -clar_lun or -sep_lun to specify device name.\n");

        fbe_api_free_contiguous_memory(context_p);
        return status; 
    }

    if (b_align)
    {
        if (min_blocks > FBE_U32_MAX)
        {
            /* Block count is too big.
             */
            fbe_api_free_contiguous_memory(context_p);
            fbe_cli_error("%s align block_count > FBE_U32_MAX 0x%llx.  status: 0x%x\n", __FUNCTION__, min_blocks, status);
            return status; 
        }
        status = fbe_api_rdgen_io_specification_set_alignment_size(&context_p->context.start_io.specification, (fbe_u32_t)min_blocks);
        if (status != FBE_STATUS_OK)
        {
            fbe_api_free_contiguous_memory(context_p);
            fbe_cli_error("%s unable to set the alignment size 0x%llx.  status: 0x%x\n", __FUNCTION__, min_blocks, status);
            return status; 
        }
    }

    status = fbe_api_rdgen_io_specification_set_options(&context_p->context.start_io.specification, options);

    if (status != FBE_STATUS_OK) 
    { 
        fbe_api_free_contiguous_memory(context_p);
        fbe_cli_error("%s unable to set the options flags.  status: 0x%x\n", __FUNCTION__, status);
        return status; 
    }

    status = fbe_api_rdgen_io_specification_set_dp_flags(&context_p->context.start_io.specification, data_pattern_flags);

    if (status != FBE_STATUS_OK) 
    { 
        fbe_api_free_contiguous_memory(context_p);
        fbe_cli_error("%s unable to set the data pattern flags.  status: 0x%x\n", __FUNCTION__, status);
        return status; 
    }

    status = fbe_api_rdgen_io_specification_set_affinity(&context_p->context.start_io.specification, affinity, core);

    if (status != FBE_STATUS_OK) 
    { 
        fbe_api_free_contiguous_memory(context_p);
        fbe_cli_error("%s unable to set the affinity: %d core: %d.  status: 0x%x\n", 
                      __FUNCTION__, affinity, core, status);
        return status; 
    }

    status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->context.start_io.specification, peer_options);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_api_free_contiguous_memory(context_p);
        fbe_cli_error("%s unable to set peer options %d.  status: 0x%x\n", __FUNCTION__, peer_options, status);
        return status; 
    }

    status = fbe_api_rdgen_io_specification_set_inc_lba_blocks(&context_p->context.start_io.specification,
                                                               inc_blocks, inc_lba);

    if (status != FBE_STATUS_OK) { 
        fbe_api_free_contiguous_memory(context_p);
        fbe_cli_error("%s unable to set inc lba blocks.  status: 0x%x\n", __FUNCTION__, status);
        return status; 
    }

    status = fbe_api_rdgen_io_specification_set_msecs_to_abort(&context_p->context.start_io.specification, abort_ms);
    if (status != FBE_STATUS_OK) { 
        fbe_api_free_contiguous_memory(context_p);
        fbe_cli_error("%s unable to set abort ms.  status: 0x%x\n", __FUNCTION__, status);
        return status; 
    }

    if (options & FBE_RDGEN_OPTIONS_USE_SEQUENCE_COUNT_SEED)
    {
        status = fbe_api_rdgen_set_sequence_count_seed(&context_p->context.start_io.specification, sequence_number);
        if (status != FBE_STATUS_OK) { 
            fbe_api_free_contiguous_memory(context_p);
            fbe_cli_error("%s unable to set seq count seed.  status: 0x%x\n", __FUNCTION__, status);
            return status; 
        }
    }
    if (options & FBE_RDGEN_OPTIONS_FIXED_RANDOM_NUMBERS)
    {
        status = fbe_api_rdgen_set_random_seed(&context_p->context.start_io.specification, random_seed);
        if (status != FBE_STATUS_OK) { 
            fbe_api_free_contiguous_memory(context_p);
            fbe_cli_error("%s unable to set random seed.  status: 0x%x\n", __FUNCTION__, status);
            return status; 
        }
    }
    if (priority != FBE_PACKET_PRIORITY_INVALID)
    {
        status = fbe_api_rdgen_set_priority(&context_p->context.start_io.specification, priority);
        if (status != FBE_STATUS_OK) { 
            fbe_api_free_contiguous_memory(context_p);
            fbe_cli_error("%s unable to set priority.  status: 0x%x\n", __FUNCTION__, status);
            return status; 
        }
    }
    if (msec_delay != 0) {
        status = fbe_api_rdgen_set_msec_delay(&context_p->context.start_io.specification, msec_delay);
        if (status != FBE_STATUS_OK) { 
            fbe_api_free_contiguous_memory(context_p);
            fbe_cli_error("%s unable to set msec_delay.  status: 0x%x\n", __FUNCTION__, status);
            return status; 
        }
    }
    if (extra_options != FBE_RDGEN_EXTRA_OPTIONS_INVALID) {
        status = fbe_api_rdgen_io_specification_set_extra_options(&context_p->context.start_io.specification, extra_options);

        if (status != FBE_STATUS_OK) {
            fbe_api_free_contiguous_memory(context_p);
            fbe_cli_error("%s unable to set the options flags.  status: 0x%x\n", __FUNCTION__, status);
            return status; 
        }
    }

    /* Add callback so that we will release the memory.
     */
    fbe_cli_rdgen_info_enqueue(context_p);
    fbe_api_rdgen_set_context(&context_p->context, fbe_cli_rdgen_test_io_completion, context_p);
    if (!fbe_cli_is_simulation() && (b_wait_reply == FBE_FALSE))
    {
        status = fbe_api_cli_rdgen_start_tests(&context_p->context, FBE_PACKAGE_ID_NEIT, 1);
    }
    else
    {
        status = fbe_api_rdgen_start_tests(&context_p->context, FBE_PACKAGE_ID_NEIT, 1);
    }

    if (status != FBE_STATUS_OK) 
    { 
        fbe_cli_error("%s unable to start tests.  status: 0x%x\n", __FUNCTION__, status);
        return status; 
    }

	if(be_quiet){
		return status;
	}

    fbe_cli_printf("rdgen started context: %p\n", context_p);
    fbe_cli_rdgen_display_context(context_p, FBE_TRUE /* start */);

    return status;
}
/**************************************
 * end fbe_cli_rdgen_start()
 **************************************/
/*!**************************************************************
 * fbe_cli_rdgen_display()
 ****************************************************************
 * @brief
 *  Display the information on rdgen threads.
 *
 * @param argc - count of arguments              
 * @param argv - vector of arguments
 * 
 * @return -   
 *
 * @author
 *  3/27/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_cli_rdgen_display(fbe_u32_t argc, char** argv, fbe_rdgen_display_mode_t display_mode)
{
    fbe_status_t status;
    fbe_api_rdgen_get_stats_t stats;
    fbe_rdgen_filter_t filter;
    fbe_api_rdgen_get_object_info_t *object_info_p = NULL;
    fbe_api_rdgen_get_request_info_t *request_info_p = NULL;
    fbe_api_rdgen_get_thread_info_t *thread_info_p = NULL;

    /* Get the stats, then allocate memory.
     */
    fbe_api_rdgen_filter_init(&filter, 
                              FBE_RDGEN_FILTER_TYPE_INVALID, 
                              FBE_OBJECT_ID_INVALID, 
                              FBE_CLASS_ID_INVALID, 
                              FBE_PACKAGE_ID_INVALID,
                              0    /* edge index */);

    status = fbe_api_rdgen_get_stats(&stats, &filter);

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("rdgen display: cannot get stats\n");
        return status;
    }
	if(!be_quiet){
		fbe_cli_rdgen_display_terse_info(&stats);
	}

    if ((stats.objects > 0) && (stats.requests > 0))
    {
        /* Allocate memory to get objects, requests and threads.
         */
        object_info_p = (fbe_api_rdgen_get_object_info_t*)malloc(sizeof(fbe_api_rdgen_get_object_info_t) * stats.objects);
        if (object_info_p == NULL)
        {
            fbe_cli_error("%s unable to allocate object memory\n", __FUNCTION__);
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
        request_info_p = (fbe_api_rdgen_get_request_info_t*)malloc(sizeof(fbe_api_rdgen_get_request_info_t) * stats.requests);
        if (request_info_p == NULL)
        {
            free(object_info_p);
            fbe_cli_error("%s unable to allocate request memory\n", __FUNCTION__);
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
        thread_info_p = (fbe_api_rdgen_get_thread_info_t*)malloc(sizeof(fbe_api_rdgen_get_thread_info_t) * stats.threads);
        if (thread_info_p == NULL)
        {
            free(object_info_p);
            free(request_info_p);
            fbe_cli_error("%s unable to allocate thread memory\n", __FUNCTION__);
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
        /* Fetch info on objects, requests, and threads.
         */
        status = fbe_api_rdgen_get_object_info(object_info_p, stats.objects);
        if (status != FBE_STATUS_OK)
        {
            free(object_info_p);
            free(request_info_p);
            free(thread_info_p);
            fbe_cli_error("rdgen display: cannot get object info status: 0x%x\n", status);
            return status;
        }
        status = fbe_api_rdgen_get_request_info(request_info_p, stats.requests);
        if (status != FBE_STATUS_OK)
        {
            free(object_info_p);
            free(request_info_p);
            free(thread_info_p);
            fbe_cli_error("rdgen display: cannot get request info status: 0x%x\n", status);
            return status;
        }
        status = fbe_api_rdgen_get_thread_info(thread_info_p, stats.threads);
        if (status != FBE_STATUS_OK)
        {
            free(object_info_p);
            free(request_info_p);
            free(thread_info_p);
            fbe_cli_error("rdgen display: cannot get thread info status: 0x%x\n", status);
            return status;
        }

        if (status == FBE_STATUS_OK)
        {
            fbe_cli_rdgen_display_thread_info(&stats, object_info_p, request_info_p, thread_info_p, display_mode); 
        }
        free(object_info_p);
        free(request_info_p);
        free(thread_info_p);
    }
    return status;
}
/******************************************
 * end fbe_cli_rdgen_display()
 ******************************************/

/*!**************************************************************
 * fbe_cli_rdgen_cmi_perf_test()
 ****************************************************************
 * @brief
 *  run the cmi performance test
 *
 * @param argc - count of arguments              
 * @param argv - vector of arguments
 * 
 * @return -   
 *
 * @author
 *  9/7/2011 - Created. Shay Harel
 *
 ****************************************************************/
static fbe_status_t fbe_cli_rdgen_cmi_perf_test(fbe_u32_t argc, char** argv, fbe_bool_t with_mask)
{

	fbe_status_t					status;
	fbe_rdgen_control_cmi_perf_t	cmi_perf;
	fbe_u32_t						core = 0;
	
	/*argument order should be: message size, number or messages, number of threads*/
	if (argc < 3) {
		 fbe_cli_error("need 3 args: msg size (in bytes), # of msg, # of threads\n");
         return FBE_STATUS_GENERIC_FAILURE;
	}

	cmi_perf.bytes_per_message = (fbe_u32_t)strtoul(*argv, 0, 0);
	argv++;
	cmi_perf.number_of_messages = (fbe_u32_t)strtoul(*argv, 0, 0);
	argv++;
	cmi_perf.thread_count = (fbe_u32_t)strtoul(*argv, 0, 0);

	cmi_perf.with_mask = with_mask;

	fbe_cli_printf("\nUser request:Msg Size: %d Bytes, Msg count:%d, Thread count:%d\nMay take a few seconds to execute, depends on size\n\n", 
				   cmi_perf.bytes_per_message, cmi_perf.number_of_messages, cmi_perf.thread_count);

	status = fbe_api_rdgen_run_cmi_perf_test(&cmi_perf);
	if (status != FBE_STATUS_OK) {
		if (FBE_STATUS_INSUFFICIENT_RESOURCES == status) {
			fbe_cli_error("Make sure peer is alive and # of cores is correct\n");
			return status;
		}else{
			fbe_cli_error("test return error: %d\n", status);
			return status;
		}
	}

	/*display the results*/
	fbe_cli_printf("Results:\n\n");
    for (core = 0; core < FBE_CPU_ID_MAX; core ++) {
		if (cmi_perf.run_time_in_msec[core] != 0) {
			fbe_cli_printf("Core #%d: %d msec.\n", core, cmi_perf.run_time_in_msec[core]);
		}
	}

	fbe_cli_printf("\n");

	return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cli_rdgen_cmi_perf_test()
 ******************************************/
/*************************
 * end file fbe_cli_src_rdgen_service_cmds.c
 *************************/
