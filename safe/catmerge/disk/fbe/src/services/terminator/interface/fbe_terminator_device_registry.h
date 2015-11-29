/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/


/*!**************************************************************************
 * @file fbe_terminator_device_registry.h
 ***************************************************************************
 *
 * @brief
 *  This file contains public interface for the Terminator device registry module.
 *
 * @date 11/12/2008
 * @author VG
 *
 ***************************************************************************/

/**********************************/
/*        include files           */
/**********************************/
#include "fbe/fbe_types.h"

/* 32-bit unsigned */
typedef fbe_u32_t tdr_u32_t;

/* 64-bit unsigned */
typedef fbe_u64_t tdr_u64_t;

/* generation number */
typedef tdr_u32_t tdr_generation_number_t;

/* device type - we don't know anything about devices, so use 32-bit type instead of enumeration */
typedef tdr_u64_t tdr_device_type_t;

/* device handle consist of generation_num and index into the registry table */
typedef tdr_u64_t tdr_device_handle_t;

/* device ptr is the pointer to the device structure used by Terminator*/
typedef void * tdr_device_ptr_t;

typedef enum tdr_status_e 
{
    TDR_STATUS_OK = 1,
    TDR_STATUS_GENERIC_FAILURE
} tdr_status_t;

#define TDR_INVALID_HANDLE 0

#define TDR_INVALID_PTR NULL

#define TDR_DEVICE_TYPE_INVALID 0

#define TERMINATOR_MAX_NUMBER_OF_DEVICES 1024 /* make sure this is smaller than 0xFFFF FFFF.  
                                                 when the max device grows bigger than a 32-bit number, 
                                                 make sure TDR_REG_INDEX_MASK and TDR_GEN_NUM_BIT_SHIFT 
                                                 are updated to match */

#define TDR_INITIAL_GEN_NUM 0
#define TDR_GEN_NUM_BIT_SHIFT 32 /* the bit shift in the device handle */

#define TDR_REG_INDEX_MASK 0xFFFFFFFF /* assuming the TERMINATOR_MAX_NUMBER_OF_DEVICES does not exceed 32 bit.*/

/*! @struct fbe_terminator_device_registry_entry_t
 *  
 * @brief Terminator device registry entry structure. This holds device handle and type.
 * We can also add a unique id for each device created.  I don't see why we need it right now. VG 11/10/2008
 */
typedef struct fbe_terminator_device_registry_entry_s
{
    tdr_generation_number_t             generation_num;
    tdr_device_type_t                   device_type;
    tdr_device_ptr_t                    device_ptr;
}fbe_terminator_device_registry_entry_t;


/*! @fn tdr_status_t fbe_terminator_device_registry_init(tdr_u32_t size)
 *  
 *  @brief Function that initializes Terminator device registry.
 *          The memory for the registry table is allocated here
 *  @param  size - maximum number of devices can be stored in this registry, IN
 *  @return the status of the call.
 */
tdr_status_t fbe_terminator_device_registry_init(tdr_u32_t size);


/*! @fn tdr_status_t fbe_terminator_device_registry_destroy()
 *  
 *  @brief  Function that destroys Terminator device registry.
 *          The memory for the registry table is freed here.
 *          Devices are not destroyed and pointers to them remain valid.
 *  @return the status of the call.
 */
tdr_status_t fbe_terminator_device_registry_destroy(void);


/*! @fn fbe_terminator_device_registry_add_device(tdr_device_type_t device_type,
 *                                                tdr_device_handle_t device_handle)
 *  @brief Function that adds 
 *  @param device_type - type of device to add IN
 *  @param device_ptr - pointer to device to be added IN
 *  @return the status of the call.
 */
tdr_status_t fbe_terminator_device_registry_add_device(tdr_device_type_t device_type,
                                                       tdr_device_ptr_t device_ptr,
                                                       tdr_device_handle_t *device_handle);


/*! @fn fbe_terminator_device_registry_remove_device_by_handle(tdr_device_handle_t device_handle)
 *  @brief This function removes the registry entry of a device from the Terminator registry.
 *         The device is not destroyed, therefore the pointer remains valid.
 *  @param device_handle - handle of the device to be removed, IN
 *  @return the status of the call.
 */
tdr_status_t fbe_terminator_device_registry_remove_device_by_handle(tdr_device_handle_t device_handle);


/*! @fn fbe_terminator_device_registry_remove_device_by_ptr(tdr_device_ptr_t device_ptr)
 *  @brief This function removes the registry entry of a device from the Terminator registry.
 *         The device is not destroyed, therefore the pointer remains valid.
 *  @param device_ptr - pointer to device object to be removed, IN
 *  @return the status of the call.
 */
tdr_status_t fbe_terminator_device_registry_remove_device_by_ptr(tdr_device_ptr_t device_ptr);


/*! @fn fbe_terminator_device_registry_enumerate_all_devices(tdr_device_handle_t * device_handles,
 *                                                       tdr_u32_t device_handle_counter)
 *
 *  @brief Function that enumerates handles of devices in Terminator registry
 *  @param device_handles - array to store handles of devices. Caller must allocate this array himself.
 *          This array must be able to store all handles. Minimal size of this array can be determined
 *          using fbe_terminator_device_registry_get_device_count() function. IN, OUT
 *  @param device_handle_counter - size of array to store handles. IN
 *  @return the status of the call.
 */
tdr_status_t fbe_terminator_device_registry_enumerate_all_devices(tdr_device_handle_t * device_handles,
                                                                  tdr_u32_t device_handle_counter);


/*! @fn fbe_terminator_device_registry_enumerate_devices_by_type(tdr_device_handle_t * device_handles,
 *                                                              tdr_device_type_t device_type,
 *                                                              tdr_u32_t device_handle_counter)
 *
 *  @brief Function that enumerates handles of devices of given type in Terminator registry
 *  @param device_handles - array to store handles of devices. This array must be able to store all handles. OUT
 *  @param device_handle_counter - size of array to store handles. IN
 *  @param device_type - type of devices to enumerate. IN
 *  @return the status of the call.
 */
tdr_status_t fbe_terminator_device_registry_enumerate_devices_by_type(tdr_device_handle_t * device_handles,
                                                                      tdr_u32_t device_handle_counter,
                                                                      tdr_device_type_t device_type);


/*! @fn fbe_terminator_device_registry_get_device_type(tdr_device_handle_t device_handle,
 *                                                     tdr_device_type_t * device_type)
 *
 *  @brief Function that returns type of device by its handle
 *  @param device_handle - handle of device for which we want to obtain type. IN
 *  @param device_type - address where type of device would be stored. OUT
 *  @return the status of the call.
 */
tdr_status_t fbe_terminator_device_registry_get_device_type(tdr_device_handle_t device_handle,
                                                            tdr_device_type_t * device_type);


/*! @fn     fbe_terminator_device_registry_get_device_count()
 *  @brief  Function that returns total number of devices in registry
 *  @return Total number of devices in registry
 */
tdr_u32_t fbe_terminator_device_registry_get_device_count(void);


/*! @fn     fbe_terminator_device_registry_get_device_count_by_type(tdr_device_type_t device_type)
 *  @brief  Function that returns number of devices with given type in registry
 *  @param  device_type - type of devices to count. IN
 *  @return Total number of devices with specified type in registry
 */
tdr_u32_t fbe_terminator_device_registry_get_device_count_by_type(tdr_device_type_t device_type);

/*! @fn fbe_terminator_device_registry_get_device_ptr(tdr_device_handle_t device_handle,
 *                                                    tdr_device_ptr_t * device_ptr)
 *
 *  @brief Function that returns type of device by its handle
 *  @param device_handle - handle of device for which we want to obtain type. IN
 *  @param device_ptr - pointer of device object would be stored. OUT
 *  @return the status of the call.
 */
tdr_status_t fbe_terminator_device_registry_get_device_ptr(tdr_device_handle_t device_handle, tdr_device_ptr_t *device_ptr);

/*! @fn fbe_terminator_device_registry_get_device_handle(tdr_device_handle_t device_handle,
 *                                                    tdr_device_ptr_t * device_ptr)
 *
 *  @brief Function that returns device ptr by its handle
 *  @param device_ptr - pointer of device which we want to lookup for. IN
 *  @param device_handle - handle of device. OUT
 *  @return the status of the call.
 */
tdr_status_t fbe_terminator_device_registry_get_device_handle(tdr_device_ptr_t device_ptr, tdr_device_handle_t *device_handle);


