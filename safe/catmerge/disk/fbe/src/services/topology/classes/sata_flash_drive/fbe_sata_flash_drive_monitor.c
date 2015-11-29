/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_sata_flash_drive_monitor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the sata flash drive object lifecycle code.
 * 
 * HISTORY
 *   2/22/2010:  Created. chenl6
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_memory.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"

#include "fbe_transport_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"

#include "fbe_sas_port.h"
#include "sata_flash_drive_private.h"

/*!***************************************************************
 * fbe_sata_flash_drive_monitor_entry()
 ****************************************************************
 * @brief
 *  Entry routine for the sata flash drive's monitor.
 *
 * @param object_handle - This is the object handle, or in our case the pdo.
 * @param packet_p - The packet arriving from the monitor scheduler.
 *
 * @return fbe_status_t - Status of monitor operation.             
 *
 * @author
 *   2/22/2010:  Created. chenl6
 *
 ****************************************************************/
fbe_status_t 
fbe_sata_flash_drive_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_flash_drive_t * sata_flash_drive = NULL;

    sata_flash_drive = (fbe_sata_flash_drive_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace(  (fbe_base_object_t*)sata_flash_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry.\n", __FUNCTION__);

    status = fbe_lifecycle_crank_object(&fbe_sata_flash_drive_lifecycle_const, (fbe_base_object_t*)sata_flash_drive, packet);

    return status;
}

/*!**************************************************************
 * fbe_sata_flash_drive_monitor_load_verify()
 ****************************************************************
 * @brief
 *  This function simply validates the constant lifecycle data
 *  that is associated with the drive object.
 *
 * @param  - None.               
 *
 * @return fbe_status_t - The status.
 *
 * @author
 *   2/22/2010:  Created. chenl6
 *
 ****************************************************************/
fbe_status_t 
fbe_sata_flash_drive_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(sata_flash_drive));
}

/*--- local function prototypes --------------------------------------------------------*/
static fbe_lifecycle_status_t sata_flash_drive_self_init_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);


/*--- lifecycle callback functions -----------------------------------------------------*/
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(sata_flash_drive);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(sata_flash_drive);

static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(sata_flash_drive) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        sata_flash_drive,
        FBE_LIFECYCLE_NULL_FUNC,       /* online function */
        FBE_LIFECYCLE_NULL_FUNC)         /* no pending function */
};

/*--- constant derived condition entries -----------------------------------------------*/
static fbe_lifecycle_const_cond_t sata_flash_drive_self_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_OBJECT_LIFECYCLE_COND_SELF_INIT,
        sata_flash_drive_self_init_cond_function)
};


/*--- constant base condition entries --------------------------------------------------*/
#if 0
static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(sata_flash_drive)[] = {
};

FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(sata_flash_drive);
#endif
FBE_LIFECYCLE_DEF_CONST_NULL_BASE_CONDITIONS(sata_flash_drive);


static fbe_lifecycle_const_rotary_cond_t sata_flash_drive_specialize_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sata_flash_drive_self_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    /* Base conditions */   

};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(sata_flash_drive)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, sata_flash_drive_specialize_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(sata_flash_drive);

/*--- global sata drive lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sata_flash_drive) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        sata_flash_drive,
        FBE_CLASS_ID_SATA_FLASH_DRIVE,                  /* This class */
        FBE_LIFECYCLE_CONST_DATA(sata_physical_drive))    /* The super class */
};

/*--- Local Functions ------------------------------------------------------------------*/


/*--- Condition Functions --------------------------------------------------------------*/

/*!**************************************************************
 * sata_flash_drive_self_init_cond_function()
 ****************************************************************
 * @brief
 *  This function initializes the drive object.
 *
 * @param p_object - pointer to object.               
 * @param p_packet - pointer to packet.               
 *
 * @return fbe_lifecycle_status_t - The status.
 *
 * @author
 *   2/22/2010:  Created. chenl6
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
sata_flash_drive_self_init_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t status;
    fbe_sata_flash_drive_t * sata_flash_drive = (fbe_sata_flash_drive_t*)p_object;

    fbe_base_object_trace((fbe_base_object_t*)sata_flash_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

     
    fbe_sata_flash_drive_init(sata_flash_drive);

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sata_flash_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)sata_flash_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
