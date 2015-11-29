/***************************************************************************
* Copyright (C) EMC Corporation 2012
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_persist_debug.c
***************************************************************************
*
* @brief
*  This file contains the implementaion of hook logic for persist service
*
* @version
*    11/23/2012 - Created. Zhipeng Hu
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_persist_debug.h"
#include "fbe_panic_sp.h"
#include "fbe_persist_private.h"
#include "fbe_persist_transaction.h"



static void persist_hook_shrink_journal_and_wait(fbe_packet_t* packet, void* hook_elem, void* context);
static void persist_hook_shrink_live_transaction_and_wait(fbe_packet_t* packet, void* hook_elem, void* context);
static void persist_hook_panic_when_writing_journal_region(fbe_packet_t* packet, void* hook_elem, void* context);
static void persist_hook_panic_when_writing_live_data_region(fbe_packet_t* packet, void* hook_elem, void* context);
static void persist_hook_wait_before_invalidate_header(fbe_packet_t* packet, void* hook_elem, void* context);
static void persist_hook_wait_before_mark_header_valid(fbe_packet_t* packet, void* hook_elem, void* context);
static void persist_hook_return_failed_transaction(fbe_packet_t* packet, void* hook_elem, void* context);

static fbe_u32_t persist_hook_array_size = 0;

/*list all types of hooks in this array*/
static fbe_persist_hook_t persist_hook_array[] = 
{
    {
        FBE_PERSIST_HOOK_TYPE_SHRINK_JOURNAL_AND_WAIT,
        persist_hook_shrink_journal_and_wait, /*hook function*/
        0,  /*whether this hook has been by esp package*/
        0,  /*whether this hook has been by sep package*/
        0,  /*whether set hook for esp package*/
        0   /*whether set hook for sep package*/
    },
    {
        FBE_PERSIST_HOOK_TYPE_WAIT_BEFORE_MARK_JOURNAL_HEADER_VALID,
        persist_hook_wait_before_mark_header_valid, /*hook function*/
        0,  /*whether this hook has been by esp package*/
        0,  /*whether this hook has been by sep package*/
        0,  /*hooks for esp package*/
        0   /*hooks for sep package*/
    },
    {
        FBE_PERSIST_HOOK_TYPE_SHRINK_LIVE_TRANSACTION_AND_WAIT,
        persist_hook_shrink_live_transaction_and_wait, /*hook function*/
        0,  /*whether this hook has been by esp package*/
        0,  /*whether this hook has been by sep package*/
        0,  /*hooks for esp package*/
        0   /*hooks for sep package*/
    },
    {
        FBE_PERSIST_HOOK_TYPE_WAIT_BEFORE_INVALIDATE_JOURNAL_HEADER ,
        persist_hook_wait_before_invalidate_header, /*hook function*/
        0,  /*whether this hook has been by esp package*/
        0,  /*whether this hook has been by sep package*/
        0,  /*hooks for esp package*/
        0   /*hooks for sep package*/
    },
    {
        FBE_PERSIST_HOOK_TYPE_PANIC_WHEN_WRITING_JOURNAL_REGION,
        persist_hook_panic_when_writing_journal_region, /*hook function*/
        0,  /*whether this hook has been by esp package*/
        0,  /*whether this hook has been by sep package*/
        0,  /*hooks for esp package*/
        0   /*hooks for sep package*/
    },
    {
        FBE_PERSIST_HOOK_TYPE_PANIC_WHEN_WRITING_LIVE_DATA_REGION,
        persist_hook_panic_when_writing_live_data_region, /*hook function*/
        0,  /*whether this hook has been by esp package*/
        0,  /*whether this hook has been by sep package*/
        0,  /*hooks for esp package*/
        0   /*hooks for sep package*/
    },
    {
        FBE_PERSIST_HOOK_TYPE_RETURN_FAILED_TRANSACTION,
        persist_hook_return_failed_transaction, /*hook function*/
        0,  /*whether this hook has been by esp package*/
        0,  /*whether this hook has been by sep package*/
        0,  /*hooks for esp package*/
        0   /*hooks for sep package*/
    },
    {
        /*terminator element*/
        FBE_PERSIST_HOOK_TYPE_INVALID,
        NULL,
        0,  /*whether this hook has been by esp package*/
        0,  /*whether this hook has been by sep package*/
        0,  /*hooks for esp package*/
        0   /*hooks for sep package*/
    },
};

/*!**************************************************************************************************
@fn persist_hook_init
*****************************************************************************************************
* @brief
*  This function do initialization for hook logic
*
* @param none
*
* @return  none
*
* @version
*   11/23/2012:  created. Zhipeng Hu
*
*****************************************************************************************************/
void persist_hook_init(void)
{
    fbe_u32_t count = 0;
    fbe_persist_hook_t* hook_elem = &persist_hook_array[0];

    while(hook_elem->hook_type != FBE_PERSIST_HOOK_TYPE_INVALID)
    {
        hook_elem->esp_triggered = 0;
        hook_elem->sep_triggered = 0;
        hook_elem->esp_hook = 0;
        hook_elem->sep_hook = 0;
        count++;
        hook_elem++;
    }

    persist_hook_array_size = count;
    
}

/*!**************************************************************************************************
@fn persist_set_hook
*****************************************************************************************************
* @brief
*  Set a specific hook in persist service
*
* @param hook_type - the type of hook we want to set
*
* @return  FBE_STATUS_OK if no error
*
* @version
*   11/23/2012:  created. Zhipeng Hu
*
*****************************************************************************************************/
fbe_status_t  persist_set_hook(fbe_persist_hook_type_t hook_type, fbe_bool_t is_set_esp,
                               fbe_bool_t is_set_sep)
{
    fbe_persist_hook_t* hook_elem = &persist_hook_array[0];

    while(hook_elem->hook_type != FBE_PERSIST_HOOK_TYPE_INVALID)
    {
        if(hook_elem->hook_type == hook_type)
            break;

        hook_elem++;
    }

    /*if not found the hook type, return ERROR*/
    if(hook_elem->hook_type == FBE_PERSIST_HOOK_TYPE_INVALID)
    {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s failed to get hook type %d\n", __FUNCTION__, hook_type);
        return FBE_STATUS_NO_OBJECT;
    }

    if (is_set_esp == FBE_TRUE)
    {
        hook_elem->esp_hook++;
		
        fbe_persist_trace(FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s add hook %d for package %d, hook count %d\n", 
                          __FUNCTION__, hook_type, FBE_PACKAGE_ID_ESP,
                          hook_elem->esp_hook);
    }
    
    if (is_set_sep == FBE_TRUE)
    {
        hook_elem->sep_hook++;
  
        fbe_persist_trace(FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s add hook %d for package %d, hook count %d\n", 
                          __FUNCTION__, hook_type, FBE_PACKAGE_ID_SEP_0,
                          hook_elem->sep_hook);
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************************************************
@fn persist_remove_hook
*****************************************************************************************************
* @brief
*  Remove a specific hook in persist service
*
* @param hook_type - the type of hook we want to remove
*
* @return  FBE_STATUS_OK if no error
*
* @version
*   11/23/2012:  created. Zhipeng Hu
*
*****************************************************************************************************/
fbe_status_t  persist_remove_hook(fbe_persist_hook_type_t hook_type, fbe_bool_t is_set_esp,
                                  fbe_bool_t is_set_sep)
{
    fbe_persist_hook_t* hook_elem = &persist_hook_array[0];

    while(hook_elem->hook_type != FBE_PERSIST_HOOK_TYPE_INVALID)
    {
        if(hook_elem->hook_type == hook_type)
            break;

        hook_elem++;
    }

    /*if not found the hook type, return ERROR*/
    if(hook_elem->hook_type == FBE_PERSIST_HOOK_TYPE_INVALID)
    {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s failed to get hook type %d\n", __FUNCTION__, hook_type);
        return FBE_STATUS_NO_OBJECT;
    }

    if (is_set_esp == FBE_TRUE)
    {
        hook_elem->esp_hook = 0;
        hook_elem->esp_triggered = 0;
  
        fbe_persist_trace(FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s minus hook %d for package %d, hook count %d\n", 
                          __FUNCTION__, hook_type, FBE_PACKAGE_ID_ESP,
                          hook_elem->esp_hook);
    }
    
    if (is_set_sep == FBE_TRUE)
    {
        hook_elem->sep_hook = 0;
        hook_elem->sep_triggered = 0;
  
        fbe_persist_trace(FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s minus hook %d for package %d, hook count %d\n", 
                          __FUNCTION__, hook_type, FBE_PACKAGE_ID_SEP_0,
                          hook_elem->sep_hook);
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************************************************
@fn persist_get_hook_state
*****************************************************************************************************
* @brief
*  Get a specific hook' state in persist service
*  including whether it has been set and whether it is triggered by now
*
* @param[in]    hook_type - the type of hook we want to get state for
* @param[out]   is_set - whether the hook is set by user
* @param[out]   is_triggered - whether the hook is triggered by now
*
* @return  FBE_STATUS_OK if no error
*
* @version
*   11/23/2012:  created. Zhipeng Hu
*
*****************************************************************************************************/
fbe_status_t  persist_get_hook_state(fbe_persist_hook_type_t hook_type,
                                     fbe_bool_t *is_triggered, fbe_bool_t *is_set_esp,
                                     fbe_bool_t *is_set_sep)
{
    fbe_persist_hook_t* hook_elem = &persist_hook_array[0];

    while(hook_elem->hook_type != FBE_PERSIST_HOOK_TYPE_INVALID)
    {
        if(hook_elem->hook_type == hook_type)
            break;

        hook_elem++;
    }

    /*if not found the hook type, return ERROR*/
    if(hook_elem->hook_type == FBE_PERSIST_HOOK_TYPE_INVALID)
    {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s failed to get hook type %d\n", __FUNCTION__, hook_type);
        return FBE_STATUS_NO_OBJECT;
    }


    if(is_set_esp != NULL)
    {
        *is_set_esp = hook_elem->esp_hook> 0?FBE_TRUE:FBE_FALSE;
        if(is_triggered != NULL && *is_set_esp)
        {
            *is_triggered = hook_elem->esp_triggered> 0?FBE_TRUE:FBE_FALSE;
        }
    }

    if(is_set_sep != NULL)
    {
        *is_set_sep = hook_elem->sep_hook> 0?FBE_TRUE:FBE_FALSE;
        if(is_triggered != NULL && *is_set_sep)
        {
            *is_triggered = hook_elem->sep_triggered> 0?FBE_TRUE:FBE_FALSE;
        }
    }


    return FBE_STATUS_OK;
}

/*!**************************************************************************************************
@fn persist_check_and_run_hook
*****************************************************************************************************
* @brief
*  Check whether a specific hook is set. If yes, then run the corresponding hook function
*
* @param[in]    packet - the request packet
* @param[in]    hook_type - the type of hook we want to check and run
* @param[in]    context - passed in context with the packet
*
* @return  FBE_STATUS_OK if no error
*
* @version
*   11/23/2012:  created. Zhipeng Hu
*
*****************************************************************************************************/
fbe_status_t persist_check_and_run_hook(fbe_packet_t* packet, fbe_persist_hook_type_t hook_type, 
                                        void* context, fbe_bool_t *esp_hook_is_set,
                                        fbe_bool_t *sep_hook_is_set)
{
    fbe_persist_hook_t* hook_elem = &persist_hook_array[0];
    
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;
    fbe_persist_control_transaction_t * control_tran;
    fbe_u32_t control_tran_length;

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_tran = NULL;
    fbe_payload_control_get_buffer(control_operation, &control_tran);
    if (control_tran == NULL){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_tran_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &control_tran_length);
    if (control_tran_length != sizeof(fbe_persist_control_transaction_t)) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    while(hook_elem->hook_type != FBE_PERSIST_HOOK_TYPE_INVALID)
    {
        if(hook_elem->hook_type == hook_type)
            break;

        hook_elem++;
    }

    /*if not found the hook type, return ERROR*/
    if(hook_elem->hook_type == FBE_PERSIST_HOOK_TYPE_INVALID)
    {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s failed to get hook type %d\n", __FUNCTION__, hook_type);
        return FBE_STATUS_NO_OBJECT;
    }

    fbe_persist_trace(FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s hook for package:%d\n", __FUNCTION__, packet->package_id);

    if(hook_elem->esp_hook <= 0)
    {
        if(esp_hook_is_set != NULL)
            *esp_hook_is_set = FBE_FALSE;
    }
    else
    {
        if(esp_hook_is_set != NULL)
            *esp_hook_is_set = FBE_TRUE;
        
        if(control_tran->caller_package == FBE_PACKAGE_ID_ESP)
        {
            hook_elem->esp_triggered++;
            fbe_persist_trace(FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s esp hook %d triggered for %d time\n", __FUNCTION__, 
                              hook_type, hook_elem->esp_triggered);
            
            hook_elem->hook_function(packet, (void*)hook_elem, context);
        }
    }

    if(hook_elem->sep_hook <= 0)
    {
        if(sep_hook_is_set != NULL)
            *sep_hook_is_set = FBE_FALSE;
    }
    else
    {
        if(sep_hook_is_set != NULL)
            *sep_hook_is_set = FBE_TRUE;
        
        if(control_tran->caller_package == FBE_PACKAGE_ID_SEP_0)
        {
            hook_elem->sep_triggered++;
            fbe_persist_trace(FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s sep hook %d triggered for %d time\n", __FUNCTION__, 
                              hook_type, hook_elem->sep_triggered);
            
            hook_elem->hook_function(packet, (void*)hook_elem, context);
        }
    }
        
    return FBE_STATUS_OK;
    
}

/*!**************************************************************************************************
@fn persist_hook_shrink_journal_and_wait
*****************************************************************************************************
* @brief
*  Hook function for FBE_PERSIST_HOOK_TYPE_SHRINK_JOURNAL_AND_WAIT hook type
*  It will hang the persist progress when writing the journal region
*
* @param[in]    packet - the request packet
* @param[in]    hook_elem - the hook instance that record the state of the hook
* @param[in]    context - passed in context with the packet
*
* @return  FBE_STATUS_OK if no error
*
* @version
*   11/23/2012:  created. Zhipeng Hu
*
*****************************************************************************************************/
static void persist_hook_shrink_journal_and_wait(fbe_packet_t* packet, void* hook_elem, void* context)
{
    fbe_persist_hook_t* hook_element = (fbe_persist_hook_t*)hook_elem;
    fbe_persist_tran_header_t*   tran_header = (fbe_persist_tran_header_t*)context;

    if(hook_element == NULL || tran_header == NULL)
        return;

    /*before writing journal region, we shrink the element count to the journal*/
    if(tran_header->tran_state == FBE_PERSIST_TRAN_STATE_JOURNAL)
    {
        if(tran_header->tran_elem_count >= 2)
        {
            tran_header->tran_elem_count = tran_header->tran_elem_count/2;
        }
        
        return;
    }

    /*journal write finished, so wait here*/
    if(hook_element->esp_triggered> 0 || hook_element->sep_triggered> 0)
    {   
        fbe_persist_trace(FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s stopped by hook type:%d\n", __FUNCTION__, 
                        hook_element->hook_type);
        /*should not sleep because this hook is triggered in DPC context
        hook_element = (fbe_persist_hook_t*)hook_elem;*/
        while(FBE_TRUE)
        {
            ;
        }
    }
    
}

/*!**************************************************************************************************
@fn persist_hook_shrink_live_transaction_and_wait
*****************************************************************************************************
* @brief
*  Hook function for FBE_PERSIST_HOOK_TYPE_SHRINK_LIVE_TRANSACTION_AND_WAIT hook type
*  It will hang the persist progress when writing the live data region
*
* @param[in]    packet - the request packet
* @param[in]    hook_elem - the hook instance that record the state of the hook
* @param[in]    context - passed in context with the packet
*
* @return  FBE_STATUS_OK if no error
*
* @version
*   11/23/2012:  created. Zhipeng Hu
*
*****************************************************************************************************/
static void persist_hook_shrink_live_transaction_and_wait(fbe_packet_t* packet, void* hook_elem, void* context)
{
    fbe_persist_hook_t* hook_element = (fbe_persist_hook_t*)hook_elem;
    fbe_persist_tran_header_t  *tran_header = (fbe_persist_tran_header_t*)context;

    if(hook_element == NULL || tran_header == NULL)
        return;

    /*before writing live data region, we shrink the element count to the live data region*/
    if(tran_header->tran_state == FBE_PERSIST_TRAN_STATE_COMMIT)
    {
        if(tran_header->tran_elem_count >= 2)
        {
            tran_header->tran_elem_count = tran_header->tran_elem_count/2;
        }
        
        return;
    }

    if(hook_element->esp_triggered> 0 || hook_element->sep_triggered> 0)
    {   
        fbe_persist_trace(FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s stopped by hook type:%d\n", __FUNCTION__, 
                        hook_element->hook_type);
        /*should not sleep because this hook is triggered in DPC context
        hook_element = (fbe_persist_hook_t*)hook_elem;*/
        while(FBE_TRUE)
        {
            ;
        }
    }

}

/*!**************************************************************************************************
@fn persist_hook_panic_when_writing_journal_region
*****************************************************************************************************
* @brief
*  Hook function for FBE_PERSIST_HOOK_TYPE_PANIC_WHEN_WRITING_JOURNAL_REGION hook type
*  It will panic the SP when writing the journal region
*
* @param[in]    packet - the request packet
* @param[in]    hook_elem - the hook instance that record the state of the hook
* @param[in]    context - passed in context with the packet
*
* @return  FBE_STATUS_OK if no error
*
* @version
*   11/23/2012:  created. Zhipeng Hu
*
*****************************************************************************************************/
static void persist_hook_panic_when_writing_journal_region(fbe_packet_t* packet, void* hook_elem, void* context)
{
    fbe_persist_tran_header_t  *tran_header = (fbe_persist_tran_header_t*)context;

    if(tran_header == NULL)
        return;

    /*before writing journal region, we shrink the element count to the journal region*/
    if(tran_header->tran_state == FBE_PERSIST_TRAN_STATE_JOURNAL)
    {
        if(tran_header->tran_elem_count >= 2)
        {
            tran_header->tran_elem_count = tran_header->tran_elem_count/2;
        }
        
        return;
    }


    fbe_panic_sp(FBE_PANIC_SP_BASE, FBE_STATUS_OK);
}

/*!**************************************************************************************************
@fn persist_hook_panic_when_writing_live_data_region
*****************************************************************************************************
* @brief
*  Hook function for FBE_PERSIST_HOOK_TYPE_PANIC_WHEN_WRITING_LIVE_DATA_REGION hook type
*  It will panic the SP when writing the live data region
*
* @param[in]    packet - the request packet
* @param[in]    hook_elem - the hook instance that record the state of the hook
* @param[in]    context - passed in context with the packet
*
* @return  FBE_STATUS_OK if no error
*
* @version
*   11/23/2012:  created. Zhipeng Hu
*
*****************************************************************************************************/
static void persist_hook_panic_when_writing_live_data_region(fbe_packet_t* packet, void* hook_elem, void* context)
{
    fbe_persist_tran_header_t  *tran_header = (fbe_persist_tran_header_t*)context;

    if(tran_header == NULL)
        return;

    /*before writing live data region, we shrink the element count to the live data region*/
    if(tran_header->tran_state == FBE_PERSIST_TRAN_STATE_COMMIT)
    {
        if(tran_header->tran_elem_count >= 2)
        {
            tran_header->tran_elem_count = tran_header->tran_elem_count/2;
        }
        
        return;
    }
    
    fbe_panic_sp(FBE_PANIC_SP_BASE, FBE_STATUS_OK);
}

/*!**************************************************************************************************
@fn persist_hook_wait_before_mark_header_valid
*****************************************************************************************************
* @brief
*  Hook function for FBE_PERSIST_HOOK_TYPE_WAIT_BEFORE_MARK_JOURNAL_HEADER_VALID hook type
*  It will hang the persist progress after writing journal but before making header valid
*
* @param[in]    packet - the request packet
* @param[in]    hook_elem - the hook instance that record the state of the hook
* @param[in]    context - passed in context with the packet
*
* @return  FBE_STATUS_OK if no error
*
* @version
*   11/23/2012:  created. Zhipeng Hu
*
*****************************************************************************************************/
static void persist_hook_wait_before_mark_header_valid(fbe_packet_t* packet, void* hook_elem, void* context)
{
    fbe_persist_hook_t* hook_element = (fbe_persist_hook_t*)hook_elem;

    if(hook_element == NULL)
        return;


    if(hook_element->esp_triggered> 0 || hook_element->sep_triggered> 0)
    {   
        fbe_persist_trace(FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s stopped by hook type:%d\n", __FUNCTION__, 
                        hook_element->hook_type);
        /*should not sleep because this hook is triggered in DPC context
        hook_element = (fbe_persist_hook_t*)hook_elem;*/
        while(FBE_TRUE)
        {
            ;
        }
    }
  

}

/*!**************************************************************************************************
@fn persist_hook_wait_before_invalidate_header
*****************************************************************************************************
* @brief
*  Hook function for FBE_PERSIST_HOOK_TYPE_WAIT_BEFORE_INVALIDATE_JOURNAL_HEADER hook type
*  It will hang the persist progress after writing live data but before making header invalid
*
* @param[in]    packet - the request packet
* @param[in]    hook_elem - the hook instance that record the state of the hook
* @param[in]    context - passed in context with the packet
*
* @return  FBE_STATUS_OK if no error
*
* @version
*   11/23/2012:  created. Zhipeng Hu
*
*****************************************************************************************************/
static void persist_hook_wait_before_invalidate_header(fbe_packet_t* packet, void* hook_elem, void* context)
{
    fbe_persist_hook_t* hook_element = (fbe_persist_hook_t*)hook_elem;

    if(hook_element == NULL)
        return;

	if(hook_element->esp_triggered> 0 || hook_element->sep_triggered> 0)
    {	
		fbe_persist_trace(FBE_TRACE_LEVEL_INFO,
						FBE_TRACE_MESSAGE_ID_INFO,
						"%s stopped by hook type:%d\n", __FUNCTION__, 
						hook_element->hook_type);
        /*should not sleep because this hook is triggered in DPC context
        hook_element = (fbe_persist_hook_t*)hook_elem;*/
        while(FBE_TRUE)
        {
            ;
        }
    }


}

/*!**************************************************************************************************
@fn persist_hook_wait_before_invalidate_header
*****************************************************************************************************
* @brief
*  Hook function for FBE_PERSIST_HOOK_TYPE_RETURN_FAILED_TRANSACTION hook type
*  It will just return FAILED status for the caller to open persist transaction
*
* @param[in]    packet - the request packet
* @param[in]    hook_elem - the hook instance that record the state of the hook
* @param[in]    context - passed in context with the packet
*
* @return  FBE_STATUS_OK if no error
*
* @version
*   11/23/2012:  created. Zhipeng Hu
*
*****************************************************************************************************/
static void persist_hook_return_failed_transaction(fbe_packet_t* packet, void* hook_elem, void* context)
{
    return;
}
