#ifndef BD_INTERFACE_PRIVATE_H
#define BD_INTERFACE_PRIVATE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_bvd_interface_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the private defines for the Basic Volume Driver (BVD) 
 *  object.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe_base_config_private.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_lun.h"
#include "fbe/fbe_bvd_interface.h"


/************************************************************
 *  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS
 ************************************************************/

extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(bvd_interface);

typedef enum fbe_bvd_interface_lifecycle_cond_id_e{
   FBE_BVD_INTERFACE_LIFECYCLE_COND_INVALID = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_BVD_INTERFACE),
   FBE_BVD_INTERFACE_LIFECYCLE_COND_LAST
} fbe_bvd_interface_lifecycle_cond_id_t;

typedef struct fbe_bvd_interface_metadata_memory_s{
    fbe_base_config_metadata_memory_t base_config_metadata_memory; /* MUST be first */
}fbe_bvd_interface_metadata_memory_t;

typedef struct fbe_bvd_interface_s{
    fbe_base_config_t   base_config;/*must be first*/
    fbe_queue_head_t    server_queue_head;
	fbe_queue_head_t    cache_notification_queue_head;/*this is where we queue notifications to cache and process them at a different context*/
	fbe_thread_t		cache_notification_thread;
	fbe_semaphore_t		cache_notification_semaphore;
	fbe_spinlock_t		cache_notification_queue_lock;
	fbe_bool_t			cache_notification_therad_run;
	fbe_bvd_interface_metadata_memory_t	bvd_metadata_memory;
	fbe_bvd_interface_metadata_memory_t	bvd_metadata_memory_peer;

   /* Lifecycle defines.*/
    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_BVD_INTERFACE_LIFECYCLE_COND_LAST));
    
}fbe_bvd_interface_t;

/* Queueing related functions */
static __forceinline os_device_info_t * bvd_interface_queue_element_to_os_device(fbe_queue_element_t * queue_element)
{
    os_device_info_t * os_device;
    os_device = (os_device_info_t  *)((fbe_u8_t *)queue_element - (fbe_u8_t *)(&((os_device_info_t  *)0)->queue_element));
    return os_device;
}

static __forceinline os_device_info_t * bvd_interface_block_edge_to_os_device(fbe_block_edge_t * block_edge)
{
    os_device_info_t * os_device;
    os_device = (os_device_info_t  *)((fbe_u8_t *)block_edge - (fbe_u8_t *)(&((os_device_info_t  *)0)->block_edge));
    return os_device;
}

/************************************************************
 *  FUNCTION PROTOTYPES
 ************************************************************/

fbe_status_t fbe_bvd_interface_load(void);
fbe_status_t fbe_bvd_interface_unload(void);
fbe_status_t fbe_bvd_interface_init (fbe_bvd_interface_t *bvd_object_p);/*to allow uniqu implementation for simulation and kernel*/
fbe_status_t fbe_bvd_interface_destroy (fbe_bvd_interface_t *bvd_object_p);
fbe_status_t fbe_bvd_interface_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_bvd_interface_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_bvd_interface_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p);
fbe_status_t fbe_bvd_interface_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_bvd_interface_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);
fbe_status_t fbe_bvd_interface_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_bvd_interface_block_transport_entry(fbe_transport_entry_context_t context, fbe_packet_t * packet);
fbe_status_t fbe_bvd_interface_export_os_device(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *   os_device_info);
fbe_status_t fbe_bvd_interface_unexport_os_device(fbe_bvd_interface_t *bvd_object_p, os_device_info_t * os_device_info);
fbe_status_t fbe_bvd_interface_export_os_special_device(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *os_device_info,
                                                        csx_puchar_t dev_name, csx_puchar_t link_name);
fbe_status_t fbe_bvd_interface_unexport_os_special_device(fbe_bvd_interface_t *bvd_object_p, os_device_info_t * os_device_info);
fbe_status_t fbe_bvd_interface_add_os_device(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *  os_device_info);
fbe_status_t fbe_bvd_interface_remove_os_device(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *os_device_info);
fbe_status_t fbe_bvd_interface_destroy_edge_ptr(fbe_bvd_interface_t *bvd_object_p, fbe_block_edge_t *edge_p);
fbe_status_t fbe_bvd_interface_export_lun(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *  os_device_info);
fbe_status_t fbe_bvd_interface_unexport_lun(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *  os_device_info);

fbe_status_t fbe_bvd_interface_monitor_load_verify(void);

void bvd_interface_send_notification_to_cache(fbe_bvd_interface_t *bvd_object_p, os_device_info_t * bvd_os_info);
void bvd_interface_report_attribute_change(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *bvd_os_info, fbe_bool_t immediate);

#endif /* BD_INTERFACE_PRIVATE_H */

