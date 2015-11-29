/***************************************************************************
* Copyright (C) EMC Corporation 2011 
* All rights reserved. 
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!*************************************************************************
*                 @file fbe_cli_lib_database.c
****************************************************************************
*
* @brief
*  This file contains cli functions for the database related features in
*  FBE CLI.
*
* @ingroup fbe_cli
*
* @date
*  07/12/2011 - Created. Vera Wang
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe/fbe_winddk.h"
#include "fbe_err_sniff_call_sp_collector.h" 
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_types.h"


/*!**********************************************************************
*             fbe_err_sniff_call_sp_collector()               
*************************************************************************
*
*  @brief
*    fbe_err_sniff_call_sp_collector - Trigger spcollect
	
*  @param    
*  @param    
*  
*  @return
*			 fbe_status_t
*************************************************************************/

/*!************************
*   LOCAL FUNCTIONS
**************************/
static void fbe_err_sniff_spcollect_thread_func(void * context);
static fbe_status_t fbe_err_sniff_spcollect_handle_init(void);

/*!************************
*   LOCAL VARIABLES
**************************/
static fbe_bool_t   fbe_err_sniff_spcollect_run = FBE_TRUE;
static fbe_bool_t   fbe_err_sniff_spcollect_initialized;
static fbe_thread_t fbe_err_sniff_spcollect_thread_handle;

/*!**********************************************************************
*             fbe_err_sniff_spcollect_thread_func()               
*************************************************************************
*
*  @brief
*    fbe_err_sniff_spcollect_thread_func - the thread function for 
*        spcollect trigger
*
*  @param    None
*
*  @return
*			 None
*************************************************************************/
static void fbe_err_sniff_spcollect_thread_func(void * context)
{
    printf("\nSPCollect starts\n");
    system("start spcollect");
    printf("\nSPCollect will finish by itself after several minutes.\n");
    fbe_err_sniff_spcollect_initialized = FBE_FALSE;
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}
/***************************************************
 * end fbe_err_sniff_notification_thread_func()
 **************************************************/ 

fbe_status_t fbe_err_sniff_spcollect_handle_init(void)
{
    EMCPAL_STATUS       nt_status;

	/*start the thread that will execute updates from the queue*/
	nt_status = fbe_thread_init(&fbe_err_sniff_spcollect_thread_handle, "fbe_sniff_spcollect", fbe_err_sniff_spcollect_thread_func, NULL);
	if (nt_status != EMCPAL_STATUS_SUCCESS) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't start spcollect thread, ntstatus:%X\n", __FUNCTION__,  nt_status); 
		return FBE_STATUS_GENERIC_FAILURE;
	}
   
    fbe_err_sniff_spcollect_initialized = FBE_TRUE;
    return FBE_STATUS_OK;
}

fbe_thread_t * fbe_err_sniff_spcollect_get_handle(void)
{
	return &fbe_err_sniff_spcollect_thread_handle;
}

fbe_bool_t fbe_err_sniff_spcollect_is_initialized(void)
{
    return fbe_err_sniff_spcollect_initialized;
}

fbe_bool_t fbe_err_sniff_get_spcollect_run(void)
{
    return fbe_err_sniff_spcollect_run;
}

void fbe_err_sniff_set_spcollect_run(fbe_bool_t spcollect_run)
{
    fbe_err_sniff_spcollect_run = spcollect_run;
}

fbe_status_t fbe_err_sniff_call_sp_collector(void)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    status = fbe_err_sniff_spcollect_handle_init();

	return FBE_STATUS_OK;
}

/*************************
* end file fbe_err_sniff_call_sp_collector.c
*************************/
