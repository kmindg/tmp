/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_terminator_persistent_memory.c
 ***************************************************************************
 *
 * @brief This file implements simulated persistent memory equivalent to
 * what is provided by NVRAM/BIOS and CMM on hardware.
 *  
 *
 * @version
 *   2011-10-31 - Created. Matthew Ferson
 *
 ***************************************************************************/

#include "fbe_terminator_persistent_memory.h"

/*! 
 * @var fbe_terminator_persistent_memory_global_data 
 * @brief Global data required for the Terminator's persistent memory module. 
 */
fbe_terminator_persistent_memory_global_data_t fbe_terminator_persistent_memory_global_data;

/*! 
 * @brief Initialize the Terminator's persistent memory module, getting 
 * memory from the OS and adding it to an internal SGL. 
 * 
 * @version
 *   2011-10-31 - Created. Matthew Ferson
 */
fbe_status_t fbe_terminator_persistent_memory_init()
{
    fbe_terminator_persistent_memory_global_data_t * global_data;
    void * persistent_memory_ptr;
    SGL * sgl_p;

    global_data = &fbe_terminator_persistent_memory_global_data;

    /* If the memory state is already `initialized' we cannot proceed.
     */
    if (global_data->memory_state == FBE_TERMINATOR_PERSISTENT_MEMORY_STATE_INITIALIZED)
    {
        terminator_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                         "%s Memory state: %d is already initialized. Possible memory leak.\n",
                         __FUNCTION__, global_data->memory_state);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sgl_p = &global_data->sgl[0];

    /*! @todo Generate non-trivial SGLs */

    persistent_memory_ptr = fbe_terminator_allocate_memory(FBE_TERMINATOR_PERSISTENT_MEMORY_SIZE_BYTES);

    if(persistent_memory_ptr != NULL)
    {
        fbe_zero_memory(persistent_memory_ptr, FBE_TERMINATOR_PERSISTENT_MEMORY_SIZE_BYTES);
        sgl_p->PhysAddress = CSX_CAST_PTR_TO_PTRMAX(persistent_memory_ptr);
        sgl_p->PhysLength = FBE_TERMINATOR_PERSISTENT_MEMORY_SIZE_BYTES;
    }

    sgl_p++;
    sgl_p->PhysAddress  = 0;
    sgl_p->PhysLength   = 0;

    global_data->persistence_request    = FBE_FALSE;
    global_data->persistence_status     = FBE_CMS_MEMORY_PERSIST_STATUS_NORMAL_BOOT;
    global_data->memory_state           = FBE_TERMINATOR_PERSISTENT_MEMORY_STATE_INITIALIZED;

    return(FBE_STATUS_OK);
}


/*! 
 * @brief Prepare for tear-down of the Terminator's persistent memory module, 
 * releasing memory that we've allocated back to the OS. 
 * 
 * @version
 *   2011-10-31 - Created. Matthew Ferson
 */
fbe_status_t fbe_terminator_persistent_memory_destroy()
{
    fbe_terminator_persistent_memory_global_data_t * global_data;
    fbe_u32_t i = 0;
    SGL * sgl_p = NULL;

    global_data = &fbe_terminator_persistent_memory_global_data;

    /* Trace a warning but continue if we are not in the initialed state.
     */
    if (global_data->memory_state != FBE_TERMINATOR_PERSISTENT_MEMORY_STATE_INITIALIZED)
    {
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                         "%s Invoking destroy in non-initialized state: %d\n",
                         __FUNCTION__, global_data->memory_state);
    }

    sgl_p = &global_data->sgl[0];

    for(i = 0; i < FBE_CMS_MAXIMUM_SYSTEM_SGL_LENGTH; i++)
    {
        if(sgl_p->PhysLength == 0)
        {
            /* Validate address is 0 
             */
            if (sgl_p->PhysAddress != 0)
            {
                terminator_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s Unexpected non-null address: 0x%llx for index: %d\n",
                                 __FUNCTION__,
				 (unsigned long long)sgl_p->PhysAddress, i);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Continue */
        }
        else
        {
            /* If we found a valid allocation and we are not in the initialized 
             * state trace a critical error since some cleanup did not occur.
             */
            if (global_data->memory_state != FBE_TERMINATOR_PERSISTENT_MEMORY_STATE_INITIALIZED)
            {
                terminator_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s Cannot destroy allocated memory index: %d in state: %d\n",
                                 __FUNCTION__, i, global_data->memory_state);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Else free the memory and mark the slot as available.
             */
            fbe_terminator_free_memory((void *)(fbe_ptrhld_t)sgl_p->PhysAddress);
            sgl_p->PhysAddress = 0;
            sgl_p->PhysLength = 0;
        }

        /* Goto next
         */
        sgl_p++;
    }

    /* Now set the memory state to `Destroyed'
     */
    global_data->memory_state = FBE_TERMINATOR_PERSISTENT_MEMORY_STATE_DESTROYED;

    return(FBE_STATUS_OK);
}

/*! 
 * @brief Set a global flag indicating whether or not the Terminator 
 * should persist memory across an SP reboot. Synchronization is the 
 * responsibility of the caller, as it is on hardware. 
 * 
 * @version
 *   2011-10-31 - Created. Matthew Ferson
 */
fbe_status_t fbe_terminator_persistent_memory_set_persistence_request(fbe_bool_t request)
{
    fbe_terminator_persistent_memory_global_data_t * global_data;

    global_data = &fbe_terminator_persistent_memory_global_data;

    terminator_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                     "%s Memory Persistence Request was %d, is now %d.\n",
                     __FUNCTION__, global_data->persistence_request, request);

    global_data->persistence_request = request;

    if(request == FBE_TRUE)
    {
        global_data->persistence_status = FBE_CMS_MEMORY_PERSIST_STATUS_SUCCESSFUL;
    }
    else
    {
        global_data->persistence_status = FBE_CMS_MEMORY_PERSIST_STATUS_NORMAL_BOOT;
    }
    return(FBE_STATUS_OK);
}

/*! 
 * @brief Get the value of the global flag indicating whether or not the 
 * Terminator should persist memory across an SP reboot. Synchronization
 * is the responsibility of the caller, as it is on hardware. 
 * 
 * @version
 *   2011-10-31 - Created. Matthew Ferson
 */
fbe_status_t fbe_terminator_persistent_memory_get_persistence_request(fbe_bool_t * request)
{
    fbe_terminator_persistent_memory_global_data_t * global_data;

    global_data = &fbe_terminator_persistent_memory_global_data;

    *request = global_data->persistence_request;

    terminator_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                     "%s Memory Persistence Request is %d. \n",
                     __FUNCTION__, *request);

    return(FBE_STATUS_OK);
}

/*! 
 * @brief Set a global flag indicating whether or not the Terminator 
 * has successfully persisted memory. This is primarily a test hook 
 * to be called by FBE tests. 
 * 
 * @version
 *   2011-10-31 - Created. Matthew Ferson
 */
fbe_status_t fbe_terminator_persistent_memory_set_persistence_status(fbe_cms_memory_persist_status_t status)
{
    fbe_terminator_persistent_memory_global_data_t * global_data;

    global_data = &fbe_terminator_persistent_memory_global_data;

    terminator_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                     "%s Memory Persistence Status was 0x%X, is now 0x%X.\n",
                     __FUNCTION__, global_data->persistence_status, status);

    global_data->persistence_status = status;

    return(FBE_STATUS_OK);
}

/*! 
 * @brief Get the value of the global flag indicating whether or not the 
 * Terminator has successfully persisted memory across an SP reboot.
 * 
 * @version
 *   2011-10-31 - Created. Matthew Ferson
 */
fbe_status_t fbe_terminator_persistent_memory_get_persistence_status(fbe_cms_memory_persist_status_t * status)
{
    fbe_terminator_persistent_memory_global_data_t * global_data;

    global_data = &fbe_terminator_persistent_memory_global_data;

    terminator_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                     "%s Memory Persistence Status is 0x%X.\n",
                     __FUNCTION__, global_data->persistence_status);

    *status = global_data->persistence_status;

    return(FBE_STATUS_OK);
}

/*! 
 * @brief Fills out a caller-provided SGL describing the Terminator's 
 * persistent memory. This is equivalent to cmmGetPoolSGL() on hardware. 
 * 
 * @version
 *   2011-10-31 - Created. Matthew Ferson
 */
fbe_status_t fbe_terminator_persistent_memory_get_sgl(SGL * caller_sgl_p)
{
    fbe_terminator_persistent_memory_global_data_t * global_data;
    SGL * sgl_p;
    fbe_u32_t i;

    global_data = &fbe_terminator_persistent_memory_global_data;

    sgl_p = &global_data->sgl[0];

    for(i = 0; i < FBE_CMS_MAXIMUM_SYSTEM_SGL_LENGTH; i++)
    {
        caller_sgl_p->PhysAddress  = sgl_p->PhysAddress;
        caller_sgl_p->PhysLength   = sgl_p->PhysLength;

        if(sgl_p->PhysLength == 0)
        {
            break;
        }
        else
        {
            sgl_p++;
            caller_sgl_p++;
        }
    }

    return(FBE_STATUS_OK);
}

/*! 
 * @brief Zero the contents of the Terminator's persistent memory 
 * to simulate a case where the contents of memory have been lost. 
 * This is primarily a test hook to be called by FBE tests. 
 * 
 * @version
 *   2011-10-31 - Created. Matthew Ferson
 */
fbe_status_t fbe_terminator_persistent_memory_zero_memory()
{
    fbe_terminator_persistent_memory_global_data_t * global_data;
    SGL * sgl_p;
    fbe_u32_t i = 0;

    global_data = &fbe_terminator_persistent_memory_global_data;

    sgl_p = &global_data->sgl[0];
    for(i = 0; i < FBE_CMS_MAXIMUM_SYSTEM_SGL_LENGTH; i++)
    {
        if(sgl_p->PhysLength == 0)
        {
            break;
        }
        else
        {
            fbe_zero_memory((void *)(fbe_ptrhld_t)sgl_p->PhysAddress, sgl_p->PhysLength);
            sgl_p++;
        }
    }
    return(FBE_STATUS_OK);
}

/* end of fbe_terminator_persistent_memory.c */
