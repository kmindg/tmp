/**************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 **************************************************************************/

/**********************************/
/*        include files           */
/**********************************/
#include "fbe_terminator.h"
#include "fbe_terminator_device_registry.h"
#include "fbe_terminator_common.h"

/**********************************/
/*        local variables         */
/**********************************/

typedef struct _tag_terminator_registry_s
{
    fbe_terminator_device_registry_entry_t * registry;
    tdr_u32_t capacity;
    tdr_u32_t initialized;
    tdr_u32_t device_counter;
} terminator_registry_t;

static terminator_registry_t fbe_terminator_registry = EmcpalEmpty;

/* private function to extract the index from the device handle*/
static tdr_u32_t tdr_extract_registry_index_from_handle(tdr_device_handle_t device_handle);
static tdr_u32_t tdr_extract_generation_number_from_handle(tdr_device_handle_t device_handle);
static tdr_device_handle_t tdr_construct_device_handle(tdr_generation_number_t gen_number, tdr_u32_t index);

//init
tdr_status_t fbe_terminator_device_registry_init(tdr_u32_t size)
{
    tdr_u32_t i;
    if(fbe_terminator_registry.initialized)
    {
        return TDR_STATUS_GENERIC_FAILURE;
    }
    fbe_terminator_registry.registry = fbe_terminator_allocate_memory(size * sizeof(fbe_terminator_device_registry_entry_t));
    if(fbe_terminator_registry.registry == NULL)
    {
        return TDR_STATUS_GENERIC_FAILURE;
    }
    fbe_terminator_registry.capacity = size;
    for(i = 0; i < fbe_terminator_registry.capacity; i++)
    {
        fbe_terminator_registry.registry[i].generation_num = TDR_INITIAL_GEN_NUM;
        fbe_terminator_registry.registry[i].device_type = TDR_DEVICE_TYPE_INVALID;
        fbe_terminator_registry.registry[i].device_ptr = TDR_INVALID_PTR;
    }
    fbe_terminator_registry.device_counter = 0;
    fbe_terminator_registry.initialized = 1;
    return TDR_STATUS_OK;
}

//destroy
tdr_status_t fbe_terminator_device_registry_destroy()
{
    tdr_u32_t i;
    if(!fbe_terminator_registry.initialized)
    {
        return TDR_STATUS_GENERIC_FAILURE;
    }
    for(i = 0; i < fbe_terminator_registry.capacity; i++)
    {
        fbe_terminator_registry.registry[i].generation_num = TDR_INITIAL_GEN_NUM;
        fbe_terminator_registry.registry[i].device_type = TDR_DEVICE_TYPE_INVALID;
        fbe_terminator_registry.registry[i].device_ptr = TDR_INVALID_PTR;
    }
    fbe_terminator_free_memory(fbe_terminator_registry.registry);
    fbe_terminator_registry.device_counter = 0;
    fbe_terminator_registry.initialized = 0;
    return TDR_STATUS_OK;
}

//add
tdr_status_t fbe_terminator_device_registry_add_device(tdr_device_type_t device_type,
                                                       tdr_device_ptr_t device_ptr,
                                                       tdr_device_handle_t *device_handle)
{
    tdr_u32_t i;
    *device_handle = TDR_INVALID_HANDLE;
    if(!fbe_terminator_registry.initialized)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: terminator is not initialized. Type is %llu ptr is %p\n",
	    __FUNCTION__, (unsigned long long)device_type, device_ptr);
        return TDR_STATUS_GENERIC_FAILURE;
    }
    if( fbe_terminator_registry.device_counter >= fbe_terminator_registry.capacity)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: no free slots in registry. Type is %llu ptr is %p\n",
	    __FUNCTION__, (unsigned long long)device_type, device_ptr);
        return TDR_STATUS_GENERIC_FAILURE;
    }
    /* check parameters */
    if(device_type == TDR_DEVICE_TYPE_INVALID || device_ptr == TDR_INVALID_PTR)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: invalid parameters. Type is %llu ptr is %p\n",
	    __FUNCTION__, (unsigned long long)device_type, device_ptr);
        return TDR_STATUS_GENERIC_FAILURE;
    }
    for(i = 0; i < fbe_terminator_registry.capacity; i++)
    {
        /* check that device is not present already in the list */
        if(fbe_terminator_registry.registry[i].device_ptr == device_ptr)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: ptr is already present at position %d (type %llu). Type is %llu ptr is %p\n",
                __FUNCTION__, i,
		(unsigned long long)fbe_terminator_registry.registry[i].device_type,
		(unsigned long long)device_type, device_ptr);
            return TDR_STATUS_GENERIC_FAILURE;
        }
    }
    for(i = 0; i < fbe_terminator_registry.capacity; i++)
    {
        /* find first unused slot */
        if(fbe_terminator_registry.registry[i].device_ptr == TDR_INVALID_PTR)
        {
            fbe_terminator_registry.registry[i].device_type = device_type;
            fbe_terminator_registry.registry[i].device_ptr = device_ptr;
            fbe_terminator_registry.registry[i].generation_num++;
            fbe_terminator_registry.device_counter++;
            *device_handle = tdr_construct_device_handle(fbe_terminator_registry.registry[i].generation_num, i);
            return TDR_STATUS_OK;
        }
    }
    terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
        "%s: internal registry error. Capacity is %d, device_counter is %d. Type is %llu ptr is %p\n",
        __FUNCTION__, fbe_terminator_registry.capacity, fbe_terminator_registry.device_counter,
        (unsigned long long)device_type, device_ptr);
    /* something has gone wrong, and we have not found empty slot */
    return TDR_STATUS_GENERIC_FAILURE;
}

//remove
tdr_status_t fbe_terminator_device_registry_remove_device_by_handle(tdr_device_handle_t device_handle)
{
    tdr_u32_t index;
    tdr_generation_number_t gen_num;
    /* check registry state */
    if(!fbe_terminator_registry.initialized || fbe_terminator_registry.device_counter == 0)
    {
        return TDR_STATUS_GENERIC_FAILURE;
    }
    index = tdr_extract_registry_index_from_handle(device_handle);
    gen_num = tdr_extract_generation_number_from_handle(device_handle);
    if(index >= fbe_terminator_registry.capacity)
    {
        return TDR_STATUS_GENERIC_FAILURE;
    }
    /* validate the type and generation_number in registry */
    /* A valid device type is expected here.
     * Also the generation number should match.
     * If generation number does not match,
     * that means a new device is reusing this
     * slot and it should not be removed */
    if((fbe_terminator_registry.registry[index].device_type != TDR_DEVICE_TYPE_INVALID)
       &&(fbe_terminator_registry.registry[index].generation_num == gen_num))
    {
        fbe_terminator_registry.registry[index].device_type = TDR_DEVICE_TYPE_INVALID;
        fbe_terminator_registry.registry[index].device_ptr = TDR_INVALID_PTR;
        fbe_terminator_registry.device_counter--;
        return TDR_STATUS_OK;
    }
    /* we have not found record */
    return TDR_STATUS_GENERIC_FAILURE;
}

//remove
tdr_status_t fbe_terminator_device_registry_remove_device_by_ptr(tdr_device_ptr_t device_ptr)
{
    tdr_u32_t i;
    /* check registry state */
    if(!fbe_terminator_registry.initialized || fbe_terminator_registry.device_counter == 0)
    {
        return TDR_STATUS_GENERIC_FAILURE;
    }
    /* check parameters */
    if(device_ptr == TDR_INVALID_PTR)
    {
        return TDR_STATUS_GENERIC_FAILURE;
    }
    for(i = 0; i < fbe_terminator_registry.capacity; i++)
    {
        /* find handle in registry */
        if(fbe_terminator_registry.registry[i].device_ptr == device_ptr)
        {
            fbe_terminator_registry.registry[i].device_type = TDR_DEVICE_TYPE_INVALID;
            fbe_terminator_registry.registry[i].device_ptr = TDR_INVALID_PTR;
            fbe_terminator_registry.device_counter--;
            return TDR_STATUS_OK;
        }
    }
    /* we have not found record */
    return TDR_STATUS_GENERIC_FAILURE;
}


//enumerate
tdr_status_t fbe_terminator_device_registry_enumerate_all_devices(tdr_device_handle_t * device_handles,
                                                              tdr_u32_t device_handle_counter)
{
    tdr_u32_t i;
    tdr_u32_t handle_counter = 0;
    /* check registry state */
    if(!fbe_terminator_registry.initialized)
    {
        return TDR_STATUS_GENERIC_FAILURE;
    }
    /* check parameters */
    if(device_handles == NULL || device_handle_counter < fbe_terminator_registry.device_counter)
    {
        return TDR_STATUS_GENERIC_FAILURE;
    }
    for(i = 0; i < fbe_terminator_registry.capacity; i++)
    {
        if(fbe_terminator_registry.registry[i].device_type != TDR_DEVICE_TYPE_INVALID &&
            fbe_terminator_registry.registry[i].device_ptr != TDR_INVALID_PTR)
        {
            device_handles[handle_counter++] = tdr_construct_device_handle(fbe_terminator_registry.registry[i].generation_num, i);
            /* something has gone wrong */
            if(handle_counter > device_handle_counter)
            {
                return TDR_STATUS_GENERIC_FAILURE;
            }
        }
        else
        {
            device_handles[handle_counter++] = TDR_INVALID_HANDLE;
        }
    }
    return TDR_STATUS_OK;
}


tdr_status_t fbe_terminator_device_registry_enumerate_devices_by_type(tdr_device_handle_t * device_handles,
                                                                      tdr_u32_t device_handle_counter,
                                                                      tdr_device_type_t device_type)
{
    tdr_u32_t i;
    tdr_u32_t handle_counter = 0;
    /* check registry state */
    if(!fbe_terminator_registry.initialized)
    {
        return TDR_STATUS_GENERIC_FAILURE;
    }
    /* check parameters */
    if(device_handles == NULL ||
        device_type == TDR_DEVICE_TYPE_INVALID || /* Will they fit? */
        device_handle_counter < fbe_terminator_device_registry_get_device_count_by_type(device_type) )
    {
        return TDR_STATUS_GENERIC_FAILURE;
    }
    for(i = 0; i < fbe_terminator_registry.capacity; i++)
    {
        if(fbe_terminator_registry.registry[i].device_type == device_type &&
            fbe_terminator_registry.registry[i].device_ptr != TDR_INVALID_PTR)
        {
            device_handles[handle_counter++] = tdr_construct_device_handle(fbe_terminator_registry.registry[i].generation_num, i);
            /* something has gone wrong? */
            if(handle_counter > device_handle_counter)
            {
                return TDR_STATUS_GENERIC_FAILURE;
            }
        }
    }
    return TDR_STATUS_OK;
}


/*! @fn     fbe_terminator_device_registry_get_device_count()
 *  @brief  Function that returs total number of devices in registry
 *  @return Total number of devices in registry
 */
tdr_u32_t fbe_terminator_device_registry_get_device_count()
{
    return ( fbe_terminator_registry.initialized ) ? fbe_terminator_registry.device_counter : 0;
}

/*! @fn     fbe_terminator_device_registry_get_device_count_by_type()
 *  @brief  Function that returs number of devices with given type in registry
 *  @param  device_type - type of devices to count. IN
 *  @return Total number of devices with specified type in registry
 */
tdr_u32_t fbe_terminator_device_registry_get_device_count_by_type(tdr_device_type_t device_type)
{
    tdr_u32_t i;
    tdr_u32_t devs = 0;
    //there is no devices of type invalid, this value is reserved for internal usage
    if(device_type == TDR_DEVICE_TYPE_INVALID) {
        return 0;
    }
    for(i = 0; i < fbe_terminator_registry.capacity; i++)
    {
        if(fbe_terminator_registry.registry[i].device_type == device_type &&
           fbe_terminator_registry.registry[i].device_ptr != TDR_INVALID_PTR)
        {
            devs++;
        }
    }
    return devs;
}

//lookup
/*! @fn fbe_terminator_device_registry_get_device_type(tdr_device_handle_t device_handle,
 *                                                     tdr_device_type_t * device_type)
 *
 *  @brief Function that returns type of device by its handle
 *  @param device_handle - handle of device for which we want to obtain type. IN
 *  @param device_type - address where type of device would be stored. OUT
 *  @return the status of the call.
 */
tdr_status_t fbe_terminator_device_registry_get_device_type(tdr_device_handle_t device_handle,
                                                            tdr_device_type_t * device_type)
{
    tdr_u32_t index;
    tdr_generation_number_t gen_num;
    /* check parameters */
    if(device_type == NULL)
    {
        return TDR_STATUS_GENERIC_FAILURE;
    }
    gen_num = tdr_extract_generation_number_from_handle(device_handle);
    index = tdr_extract_registry_index_from_handle(device_handle);
    if(index >= fbe_terminator_registry.capacity)
    {
        return TDR_STATUS_GENERIC_FAILURE;
    }
    if((fbe_terminator_registry.registry[index].device_ptr != TDR_INVALID_PTR)
       && (fbe_terminator_registry.registry[index].device_type != TDR_DEVICE_TYPE_INVALID)
       && (fbe_terminator_registry.registry[index].generation_num == gen_num))
    {
        *device_type = fbe_terminator_registry.registry[index].device_type;
        return TDR_STATUS_OK;
    }
    return TDR_STATUS_GENERIC_FAILURE;
}

//lookup
/*! @fn fbe_terminator_device_registry_get_device_ptr(tdr_device_handle_t device_handle,
 *                                                    tdr_device_ptr_t * device_ptr)
 *
 *  @brief Function that returns type of device by its handle
 *  @param device_handle - handle of device for which we want to obtain type. IN
 *  @param device_ptr - pointer of device object would be stored. OUT
 *  @return the status of the call.
 */
tdr_status_t fbe_terminator_device_registry_get_device_ptr(tdr_device_handle_t device_handle, tdr_device_ptr_t *device_ptr)
{
    tdr_u32_t index;
    tdr_generation_number_t gen_num;
    /* check registry state */
    if(!fbe_terminator_registry.initialized || fbe_terminator_registry.device_counter == 0)
    {
        return TDR_STATUS_GENERIC_FAILURE;
    }
    index = tdr_extract_registry_index_from_handle(device_handle);
    gen_num = tdr_extract_generation_number_from_handle(device_handle);
    if(index >= fbe_terminator_registry.capacity)
    {
        return TDR_STATUS_GENERIC_FAILURE;
    }
    /* validate the type and generation_number in registry */
    /* A valid device type is expected here.
     * Also the generation number should match.
     * If generation number does not match,
     * that means a new device is reusing this
     * slot and it's ptr should not be returned */
    if((fbe_terminator_registry.registry[index].device_type != TDR_DEVICE_TYPE_INVALID)
       &&(fbe_terminator_registry.registry[index].generation_num == gen_num))
    {
        *device_ptr = fbe_terminator_registry.registry[index].device_ptr;
        return TDR_STATUS_OK;
    }
    /* we have not found record */
    return TDR_STATUS_GENERIC_FAILURE;
}

//lookup
/*! @fn fbe_terminator_device_registry_get_device_handle(tdr_device_handle_t device_handle,
 *                                                    tdr_device_ptr_t * device_ptr)
 *
 *  @brief Function that returns device ptr by its handle
 *  @param device_ptr - pointer of device which we want to lookup for. IN
 *  @param device_handle - handle of device. OUT
 *  @return the status of the call.
 */
tdr_status_t fbe_terminator_device_registry_get_device_handle(tdr_device_ptr_t device_ptr, tdr_device_handle_t *device_handle)
{
    tdr_u32_t i;
    *device_handle = TDR_INVALID_HANDLE;
    /* check registry state */
    if(!fbe_terminator_registry.initialized || fbe_terminator_registry.device_counter == 0)
    {
        return TDR_STATUS_GENERIC_FAILURE;
    }
    /* check parameters */
    if((device_ptr == TDR_INVALID_PTR)||(device_handle == NULL))
    {
        return TDR_STATUS_GENERIC_FAILURE;
    }
    for(i = 0; i < fbe_terminator_registry.capacity; i++)
    {
        /* find handle in registry */
        if(fbe_terminator_registry.registry[i].device_ptr == device_ptr)
        {
            *device_handle = tdr_construct_device_handle(fbe_terminator_registry.registry[i].generation_num, i);
            return TDR_STATUS_OK;
        }
    }
    /* we have not found record */
    return TDR_STATUS_GENERIC_FAILURE;
}


/* private functions*/
static tdr_u32_t tdr_extract_registry_index_from_handle(tdr_device_handle_t device_handle)
{
    return (tdr_u32_t)(device_handle & TDR_REG_INDEX_MASK);
}

static tdr_generation_number_t tdr_extract_generation_number_from_handle(tdr_device_handle_t device_handle)
{
    return ((tdr_generation_number_t)(device_handle >> TDR_GEN_NUM_BIT_SHIFT));
}

static tdr_device_handle_t tdr_construct_device_handle(tdr_generation_number_t gen_number, tdr_u32_t index)
{
    tdr_device_handle_t device_handle = gen_number;
    device_handle = (device_handle << TDR_GEN_NUM_BIT_SHIFT) + (index & TDR_REG_INDEX_MASK);

    return device_handle;
}

