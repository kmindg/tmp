/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_cmi_service_io_cancel_thread.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions that process IO cancellation for 
 *  SEP IOs.
 * 
 * 
 * @version
 *   06/04/2012:  Created. Lili Chen
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_service.h"
#include "fbe/fbe_cmi_interface.h"
#include "fbe_cmi_private.h"
#include "fbe_cmi.h"



#define FBE_CMI_IO_CANCEL_THREAD_MAX_QUEUE_DEPTH 0x7FFFFFFF

typedef enum fbe_cmi_io_cancel_thread_flag_e{
    FBE_CMI_IO_CANCEL_THREAD_FLAG_RUN,
    FBE_CMI_IO_CANCEL_THREAD_FLAG_STOP,
    FBE_CMI_IO_CANCEL_THREAD_FLAG_DONE
} fbe_cmi_io_cancel_thread_flag_t;

/*************************
 *   FORWAD DECLARATIONS
 *************************/
static fbe_semaphore_t fbe_cmi_io_cancel_thread_semaphore;
static fbe_thread_t	fbe_cmi_io_cancel_thread_handle;
static fbe_cmi_io_cancel_thread_flag_t fbe_cmi_io_cancel_thread_flag;
static void fbe_cmi_io_cancel_thread_func(void * context);

/*!**************************************************************
 * fbe_cmi_io_cancel_thread_init()
 ****************************************************************
 * @brief
 *  This function initializes cancel thread.
 *
 * @param None
 *
 * @return fbe_status_t
 * 
 * @author
 *  06/04/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_cancel_thread_init(void)
{
    EMCPAL_STATUS       		 nt_status;

    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,"%s: entry\n", __FUNCTION__);

    fbe_semaphore_init(&fbe_cmi_io_cancel_thread_semaphore, 0, FBE_CMI_IO_CANCEL_THREAD_MAX_QUEUE_DEPTH);

    /* Start thread */
    fbe_cmi_io_cancel_thread_flag = FBE_CMI_IO_CANCEL_THREAD_FLAG_RUN;
    nt_status = fbe_thread_init(&fbe_cmi_io_cancel_thread_handle, "fbe_cmi_cancel", fbe_cmi_io_cancel_thread_func, (void*)(fbe_ptrhld_t)0);
    if (nt_status != EMCPAL_STATUS_SUCCESS) {
        fbe_cmi_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't start cancel thread, ntstatus:%X\n", __FUNCTION__,  nt_status); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_cancel_thread_init()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_cancel_thread_destroy()
 ****************************************************************
 * @brief
 *  This function destroys cancel thread.
 *
 * @param None
 *
 * @return fbe_status_t
 * 
 * @author
 *  06/04/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_cancel_thread_destroy(void)
{
    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,"%s: entry\n", __FUNCTION__);

	/* Stop thread */
    fbe_cmi_io_cancel_thread_flag = FBE_CMI_IO_CANCEL_THREAD_FLAG_STOP; 

    /* notify thread
     */
    fbe_semaphore_release(&fbe_cmi_io_cancel_thread_semaphore, 0, 1, FALSE); 
    fbe_thread_wait(&fbe_cmi_io_cancel_thread_handle);
    fbe_thread_destroy(&fbe_cmi_io_cancel_thread_handle);

    fbe_semaphore_destroy(&fbe_cmi_io_cancel_thread_semaphore);

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_cancel_thread_destroy()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_cancel_thread_func()
 ****************************************************************
 * @brief
 *  This is the cancel thread function.
 *
 * @param context - context
 *
 * @return fbe_status_t
 * 
 * @author
 *  06/04/2012 - Created. Lili Chen
 *
 ****************************************************************/
static void fbe_cmi_io_cancel_thread_func(void * context)
{
    FBE_UNREFERENCED_PARAMETER(context);

    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,"%s: entry\n", __FUNCTION__);

    while(1)    
    {
        fbe_semaphore_wait(&fbe_cmi_io_cancel_thread_semaphore, 0);
        if(fbe_cmi_io_cancel_thread_flag == FBE_CMI_IO_CANCEL_THREAD_FLAG_RUN) {

            /* Scan all packets in queue.  If any have aborted requests, 
             * abort them.
             */
            fbe_cmi_io_scan_for_cancelled_packets();
        } else {
            break;
        }
    }

    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s done\n", __FUNCTION__);

    fbe_cmi_io_cancel_thread_flag = FBE_CMI_IO_CANCEL_THREAD_FLAG_DONE;
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}
/****************************************************************
 * end fbe_cmi_io_cancel_thread_func()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_cancel_thread_notify()
 ****************************************************************
 * @brief
 *  This function invokes cancel thread when a packet is to be
 *  aborted.
 *
 * @param None
 *
 * @return fbe_status_t
 * 
 * @author
 *  06/04/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_cancel_thread_notify(void)
{
    fbe_semaphore_release(&fbe_cmi_io_cancel_thread_semaphore, 0, 1, FALSE); 
    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_cancel_thread_notify()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_cancel_function()
 ****************************************************************
 * @brief
 *  This is the cancel function called when a packet is to be aborted.
 *
 * @param context - context
 *
 * @return fbe_status_t
 * 
 * @author
 *  06/04/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_cancel_function(fbe_packet_completion_context_t context)
{
    fbe_cmi_io_cancel_thread_notify();
    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_cancel_function()
 ****************************************************************/
