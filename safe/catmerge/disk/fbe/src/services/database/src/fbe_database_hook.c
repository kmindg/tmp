/***************************************************************************
* Copyright (C) EMC Corporation 2012
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_database_hook.c
***************************************************************************
*
* @brief
*  This file contains the implementaion of hook logic for database service
*
* @version
*    11/25/2012 - Created. Zhipeng Hu
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_database_hook.h"
#include "fbe_panic_sp.h"
#include "fbe_database_private.h"



static void database_hook_wait_in_update_transaction(fbe_packet_t* packet, void* hook_elem, void* context);
static void database_hook_wait_before_transaction_persist(fbe_packet_t* packet, void* hook_elem, void* context);
static void database_hook_panic_in_update_transaction(fbe_packet_t* packet, void* hook_elem, void* context);
static void database_hook_panic_before_transaction_persist(fbe_packet_t* packet, void* hook_elem, void* context);

static fbe_u32_t database_hook_array_size = 0;

/*list all types of hooks in this array*/
static fbe_database_hook_t database_hook_array[] = 
{
    {
        FBE_DATABASE_HOOK_TYPE_WAIT_IN_UPDATE_TRANSACTION,
        database_hook_wait_in_update_transaction, /*hook function*/
        0,  /*whether tester set this hook*/
        0   /*whether this hook has been triggered*/
    },
    {
        FBE_DATABASE_HOOK_TYPE_WAIT_BEFORE_TRANSACTION_PERSIST,
        database_hook_wait_before_transaction_persist, /*hook function*/
        0, /*whether tester set this hook*/
        0  /*whether this hook has been triggered*/
    },
    {
        FBE_DATABASE_HOOK_TYPE_PANIC_IN_UPDATE_TRANSACTION,
        database_hook_panic_in_update_transaction, /*hook function*/
        0, /*whether tester set this hook*/
        0  /*whether this hook has been triggered*/
    },
    {
        FBE_DATABASE_HOOK_TYPE_PANIC_BEFORE_TRANSACTION_PERSIST,
        database_hook_panic_before_transaction_persist, /*hook function*/
        0, /*whether tester set this hook*/
        0  /*whether this hook has been triggered*/
    },
    {
        FBE_DATABASE_HOOK_TYPE_WAIT_IN_UPDATE_ROLLBACK_TRANSACTION,
        database_hook_wait_in_update_transaction, /*hook function*/
        0,  /*whether tester set this hook*/
        0   /*whether this hook has been triggered*/
    },
    {
        FBE_DATABASE_HOOK_TYPE_WAIT_BEFORE_ROLLBACK_TRANSACTION_PERSIST,
        database_hook_wait_before_transaction_persist, /*hook function*/
        0, /*whether tester set this hook*/
        0  /*whether this hook has been triggered*/
    },
    {
        FBE_DATABASE_HOOK_TYPE_PANIC_IN_UPDATE_ROLLBACK_TRANSACTION,
        database_hook_panic_in_update_transaction, /*hook function*/
        0, /*whether tester set this hook*/
        0  /*whether this hook has been triggered*/
    },
    {
        FBE_DATABASE_HOOK_TYPE_PANIC_BEFORE_ROLLBACK_TRANSACTION_PERSIST,
        database_hook_panic_before_transaction_persist, /*hook function*/
        0, /*whether tester set this hook*/
        0  /*whether this hook has been triggered*/
    },
    {
        /*terminator element*/
        FBE_DATABASE_HOOK_TYPE_INVALID,
        NULL, /*hook function*/
        0, /*whether tester set this hook*/
        0  /*whether this hook has been triggered*/
    },
};

/*!**************************************************************************************************
@fn database_hook_init
*****************************************************************************************************
* @brief
*  This function do initialization for hook logic
*
* @param none
*
* @return  none
*
* @version
*   11/25/2012:  created. Zhipeng Hu
*
*****************************************************************************************************/
void database_hook_init(void)
{
    fbe_u32_t count = 0;
    fbe_database_hook_t* hook_elem = &database_hook_array[0];

    while(hook_elem->hook_type != FBE_DATABASE_HOOK_TYPE_INVALID)
    {
        hook_elem->hook_set = 0;
        hook_elem->hook_triggered = 0;
        count++;
        hook_elem++;
    }

    database_hook_array_size = count;
    
}

/*!**************************************************************************************************
@fn database_set_hook
*****************************************************************************************************
* @brief
*  Set a specific hook in database service
*
* @param hook_type - the type of hook we want to set
*
* @return  FBE_STATUS_OK if no error
*
* @version
*   11/25/2012:  created. Zhipeng Hu
*
*****************************************************************************************************/
fbe_status_t  database_set_hook(fbe_database_hook_type_t hook_type)
{
    fbe_database_hook_t* hook_elem = &database_hook_array[0];

    while(hook_elem->hook_type != FBE_DATABASE_HOOK_TYPE_INVALID)
    {
        if(hook_elem->hook_type == hook_type)
            break;

        hook_elem++;
    }

    /*if not found the hook type, return ERROR*/
    if(hook_elem->hook_type == FBE_DATABASE_HOOK_TYPE_INVALID)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s failed to get hook type %d\n", __FUNCTION__, hook_type);
        return FBE_STATUS_NO_OBJECT;
    }

    hook_elem->hook_set++;

    database_trace(FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s set hook %d\n", __FUNCTION__, hook_type);

    return FBE_STATUS_OK;
}

/*!**************************************************************************************************
@fn database_remove_hook
*****************************************************************************************************
* @brief
*  Remove a specific hook in database service
*
* @param hook_type - the type of hook we want to remove
*
* @return  FBE_STATUS_OK if no error
*
* @version
*   11/25/2012:  created. Zhipeng Hu
*
*****************************************************************************************************/
fbe_status_t  database_remove_hook(fbe_database_hook_type_t hook_type)
{
    fbe_database_hook_t* hook_elem = &database_hook_array[0];

    while(hook_elem->hook_type != FBE_DATABASE_HOOK_TYPE_INVALID)
    {
        if(hook_elem->hook_type == hook_type)
            break;

        hook_elem++;
    }

    /*if not found the hook type, return ERROR*/
    if(hook_elem->hook_type == FBE_DATABASE_HOOK_TYPE_INVALID)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s failed to get hook type %d\n", __FUNCTION__, hook_type);
        return FBE_STATUS_NO_OBJECT;
    }

    hook_elem->hook_set = 0;
    hook_elem->hook_triggered = 0;

    database_trace(FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s set hook %d\n", __FUNCTION__, hook_type);

    return FBE_STATUS_OK;
}

/*!**************************************************************************************************
@fn database_get_hook_state
*****************************************************************************************************
* @brief
*  Get a specific hook' state in database service
*  including whether it has been set and whether it is triggered by now
*
* @param[in]    hook_type - the type of hook we want to get state for
* @param[out]   is_set - whether the hook is set by user
* @param[out]   is_triggered - whether the hook is triggered by now
*
* @return  FBE_STATUS_OK if no error
*
* @version
*   11/25/2012:  created. Zhipeng Hu
*
*****************************************************************************************************/
fbe_status_t  database_get_hook_state(fbe_database_hook_type_t hook_type, fbe_bool_t *is_set, fbe_bool_t *is_triggered)
{
    fbe_database_hook_t* hook_elem = &database_hook_array[0];

    while(hook_elem->hook_type != FBE_DATABASE_HOOK_TYPE_INVALID)
    {
        if(hook_elem->hook_type == hook_type)
            break;

        hook_elem++;
    }

    /*if not found the hook type, return ERROR*/
    if(hook_elem->hook_type == FBE_DATABASE_HOOK_TYPE_INVALID)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s failed to get hook type %d\n", __FUNCTION__, hook_type);
        return FBE_STATUS_NO_OBJECT;
    }

    if(is_set != NULL)
    {
        *is_set = hook_elem->hook_set > 0?FBE_TRUE:FBE_FALSE;
    }

    if(is_triggered != NULL)
    {
        *is_triggered = hook_elem->hook_triggered > 0?FBE_TRUE:FBE_FALSE;
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************************************************
@fn database_check_and_run_hook
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
*   11/25/2012:  created. Zhipeng Hu
*
*****************************************************************************************************/
fbe_status_t database_check_and_run_hook(fbe_packet_t* packet, fbe_database_hook_type_t hook_type, void* context)
{
    fbe_database_hook_t* hook_elem = &database_hook_array[0];

    while(hook_elem->hook_type != FBE_DATABASE_HOOK_TYPE_INVALID)
    {
        if(hook_elem->hook_type == hook_type)
            break;

        hook_elem++;
    }

    /*if not found the hook type, return ERROR*/
    if(hook_elem->hook_type == FBE_DATABASE_HOOK_TYPE_INVALID)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s failed to get hook type %d\n", __FUNCTION__, hook_type);
        return FBE_STATUS_NO_OBJECT;
    }

    if(hook_elem->hook_set == 0)
    {
        /*hook not set, just return*/
        return FBE_STATUS_OK;
    }

    hook_elem->hook_triggered++;
    database_trace(FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s hook %d triggered for %d time\n", __FUNCTION__, hook_type, hook_elem->hook_triggered);

    hook_elem->hook_function(packet, (void*)hook_elem, context);

    return FBE_STATUS_OK;
    
}

/*!**************************************************************************************************
@fn database_hook_wait_in_update_transaction
*****************************************************************************************************
* @brief
*  Hook function for FBE_DATABASE_HOOK_TYPE_WAIT_IN_UPDATE_TRANSACTION hook type
*  It will hang the transaction when the transaction is updated by the job
*
* @param[in]    packet - the request packet
* @param[in]    hook_elem - the hook instance that record the state of the hook
* @param[in]    context - passed in context with the packet
*
* @return  FBE_STATUS_OK if no error
*
* @version
*   11/25/2012:  created. Zhipeng Hu
*
*****************************************************************************************************/
static void database_hook_wait_in_update_transaction(fbe_packet_t* packet, void* hook_elem, void* context)
{
    fbe_database_hook_t* hook_element = (fbe_database_hook_t*)hook_elem;

    if(hook_element == NULL)
        return;

    while(hook_element->hook_set > 0)
    {
        fbe_thread_delay(500);
    }
    
}

/*!**************************************************************************************************
@fn database_hook_wait_before_transaction_persist
*****************************************************************************************************
* @brief
*  Hook function for FBE_DATABASE_HOOK_TYPE_WAIT_BEFORE_TRANSACTION_PERSIST hook type
*  It will hang the transaction just before the database transaction is persisted
*
* @param[in]    packet - the request packet
* @param[in]    hook_elem - the hook instance that record the state of the hook
* @param[in]    context - passed in context with the packet
*
* @return  FBE_STATUS_OK if no error
*
* @version
*   11/25/2012:  created. Zhipeng Hu
*
*****************************************************************************************************/
static void database_hook_wait_before_transaction_persist(fbe_packet_t* packet, void* hook_elem, void* context)
{
    fbe_database_hook_t* hook_element = (fbe_database_hook_t*)hook_elem;

    if(hook_element == NULL)
        return;

    while(hook_element->hook_set > 0)
    {
        fbe_thread_delay(100);
    }   

}

/*!**************************************************************************************************
@fn database_hook_panic_in_update_transaction
*****************************************************************************************************
* @brief
*  Hook function for FBE_DATABASE_HOOK_TYPE_PANIC_IN_UPDATE_TRANSACTION hook type
*  It will panic the SP when the database transaction is updated
*
* @param[in]    packet - the request packet
* @param[in]    hook_elem - the hook instance that record the state of the hook
* @param[in]    context - passed in context with the packet
*
* @return  FBE_STATUS_OK if no error
*
* @version
*   11/25/2012:  created. Zhipeng Hu
*
*****************************************************************************************************/
static void database_hook_panic_in_update_transaction(fbe_packet_t* packet, void* hook_elem, void* context)
{
    fbe_panic_sp(FBE_PANIC_SP_BASE, FBE_STATUS_OK);
}

/*!**************************************************************************************************
@fn database_hook_panic_before_transaction_persist
*****************************************************************************************************
* @brief
*  Hook function for FBE_DATABASE_HOOK_TYPE_PANIC_BEFORE_TRANSACTION_PERSIST hook type
*  It will panic the SP just before the database transaction is persisted
*
* @param[in]    packet - the request packet
* @param[in]    hook_elem - the hook instance that record the state of the hook
* @param[in]    context - passed in context with the packet
*
* @return  FBE_STATUS_OK if no error
*
* @version
*   11/25/2012:  created. Zhipeng Hu
*
*****************************************************************************************************/
static void database_hook_panic_before_transaction_persist(fbe_packet_t* packet, void* hook_elem, void* context)
{
    fbe_panic_sp(FBE_PANIC_SP_BASE, FBE_STATUS_OK);
}

