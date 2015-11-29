#ifndef FBE_BVD_INTERFACE_H
#define FBE_BVD_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_bvd_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the public defines for the Basic Volume Driver (BVD) 
 *  object.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_object.h"
#include "fbe/fbe_lun.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_sep_shim.h"
#include "IdmInterface.h"
#include "idm.h"
#include "core_config_runtime.h"


/************************************************************
 *  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS
 ************************************************************/

#define FBE_OS_OPAQUE_DEV_INFO_SIZE      (68 + 2*(128*sizeof(csx_char_t)))


typedef enum fbe_bvd_interface_control_code_e{
    FBE_BVD_INTERFACE_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_BVD_INTERFACE),
    FBE_BVD_INTERFACE_CONTROL_CODE_REGISTER_LUN,
    /*! it is used to enable the SP performance statistics for the BVD object.
     */
    FBE_BVD_INTERFACE_CONTROL_CODE_ENABLE_PERFORMANCE_STATS,

    /*! it is used to disable the SP performance statistics for the BVD object.
     */
    FBE_BVD_INTERFACE_CONTROL_CODE_DISABLE_PERFORMANCE_STATS,
    /*! it is used to enable the SP performance statistics for the BVD object.
     */
    FBE_BVD_INTERFACE_CONTROL_CODE_GET_PERFORMANCE_STATS,
    /*! it is used to clear the SP performance statistics for the BVD object.
     */
    FBE_BVD_INTERFACE_CONTROL_CODE_CLEAR_PERFORMANCE_STATS,

    FBE_BVD_INTERFACE_CONTROL_CODE_GET_ATTR,

    FBE_BVD_INTERFACE_CONTROL_CODE_ENABLE_ASYNC_IO,
    FBE_BVD_INTERFACE_CONTROL_CODE_DISABLE_ASYNC_IO,

    FBE_BVD_INTERFACE_CONTROL_CODE_ENABLE_ASYNC_IO_COMPL,
    FBE_BVD_INTERFACE_CONTROL_CODE_DISABLE_ASYNC_IO_COMPL,

    FBE_BVD_INTERFACE_CONTROL_CODE_SET_RQ_METHOD,

    FBE_BVD_INTERFACE_CONTROL_CODE_ENABLE_GROUP_PRIORITY,
    FBE_BVD_INTERFACE_CONTROL_CODE_DISABLE_GROUP_PRIORITY,

    FBE_BVD_INTERFACE_CONTROL_CODE_ENABLE_PP_GROUP_PRIORITY,
    FBE_BVD_INTERFACE_CONTROL_CODE_DISABLE_PP_GROUP_PRIORITY,

    FBE_BVD_INTERFACE_CONTROL_CODE_SET_ALERT_TIME,
    FBE_BVD_INTERFACE_CONTROL_CODE_SHUTDOWN,

    FBE_BVD_INTERFACE_CONTROL_CODE_UNEXPORT_LUN,
    /* Insert new control codes above*/
    FBE_BVD_INTERFACE_CONTROL_CODE_LAST
}fbe_bvd_interface_control_code_t;


/*FBE_BVD_INTERFACE_CONTROL_CODE_REGISTER_LUN*/
/*and FBE_BVD_INTERFACE_CONTROL_CODE_REGISTER_PSM_LUN */
typedef struct fbe_bvd_interface_register_lun_s{
    fbe_object_id_t   server_id;        /* object id of the LUN */
    fbe_lba_t capacity;                 /* Capacity if the lun */
    fbe_lun_number_t    lun_number;     /* used for creating device name */
    fbe_bool_t    export_lun_b;         /* export by blkshim */
}fbe_bvd_interface_register_lun_t;

/*FBE_BVD_INTERFACE_CONTROL_CODE_UNREGISTER_LUN*/
/* and FBE_BVD_INTERFACE_CONTROL_CODE_UNREGISTER_PSM_LUN */
typedef struct fbe_bvd_interface_unregister_lun_s{
    fbe_edge_index_t    client_index;
    fbe_lun_number_t    lun_number;
}fbe_bvd_interface_unregister_lun_t;

typedef fbe_u32_t fbe_volume_attributes_flags;

/*FBE_BVD_INTERFACE_CONTROL_CODE_GET_ATTR*/
typedef struct fbe_bvd_interface_get_downstream_attr_s{
    fbe_volume_attributes_flags     attr;
    fbe_object_id_t                 lun_object_id;
    fbe_block_edge_t				block_edge;
    fbe_u64_t						opaque_edge_ptr;
}fbe_bvd_interface_get_downstream_attr_t;

typedef struct os_device_info_s{
    fbe_queue_element_t         queue_element;
    fbe_queue_element_t         cache_event_queue_element;/*we use that to queue the information that we had an event from the lun*/
    fbe_block_edge_t            block_edge;
    fbe_u8_t                    opaque_dev_info[FBE_OS_OPAQUE_DEV_INFO_SIZE];/*we use this memory later in kernel to store windows stuff*/
    fbe_lun_number_t            lun_number;
    fbe_object_id_t             bvd_object_id;
    void *                      state_change_notify_ptr;
    fbe_volume_attributes_flags current_attributes;
    fbe_volume_attributes_flags previous_attributes;
    fbe_path_attr_t             previous_path_attributes;/*do not confuse with the other attributes, these are teh FBE Bits*/
    fbe_bool_t                  cache_event_signaled;
    fbe_path_state_t            path_state;
    fbe_atomic_t                cancelled;
    FBE_TRI_STATE               ready_state;
    IDMLOOKUP					idm_lookup;
	IdmRequest					idm_request;
    fbe_bool_t                  export_lun; /*whether we need to export lun*/
    void *                      export_device;
}os_device_info_t;


#endif /*FBE_BVD_INTERFACE_H*/
