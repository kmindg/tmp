/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_sas_flash_drive_monitor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the sas flash drive object lifecycle code.
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
#include "sas_flash_drive_private.h"


/*!***************************************************************
 * fbe_sas_flash_drive_monitor_entry()
 ****************************************************************
 * @brief
 *  Entry routine for the sas flash drive's monitor.
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
fbe_sas_flash_drive_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_flash_drive_t * sas_flash_drive = NULL;

    sas_flash_drive = (fbe_sas_flash_drive_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace(  (fbe_base_object_t*)sas_flash_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry.\n", __FUNCTION__);

    status = fbe_lifecycle_crank_object(&fbe_sas_flash_drive_lifecycle_const, (fbe_base_object_t*)sas_flash_drive, packet);

    return status;
}

/*!**************************************************************
 * fbe_sas_flash_drive_monitor_load_verify()
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
fbe_status_t fbe_sas_flash_drive_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(sas_flash_drive));
}

/*--- local function prototypes --------------------------------------------------------*/
static fbe_lifecycle_status_t sas_flash_drive_self_init_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);
static fbe_lifecycle_status_t sas_flash_drive_vpd_inquiry_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);
static fbe_status_t sas_flash_drive_vpd_inquiry_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);


/*--- lifecycle callback functions -----------------------------------------------------*/
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(sas_flash_drive);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(sas_flash_drive);

static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(sas_flash_drive) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        sas_flash_drive,
        FBE_LIFECYCLE_NULL_FUNC,       /* online function */
        FBE_LIFECYCLE_NULL_FUNC)         /* no pending function */
};

/*--- constant derived condition entries -----------------------------------------------*/
static fbe_lifecycle_const_cond_t sas_flash_drive_self_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_OBJECT_LIFECYCLE_COND_SELF_INIT,
        sas_flash_drive_self_init_cond_function)
};


/*--- constant base condition entries --------------------------------------------------*/
/* FBE_SAS_FLASH_DRIVE_LIFECYCLE_COND_INQUIRY condition */
static fbe_lifecycle_const_base_cond_t sas_flash_drive_vpd_inquiry_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_flash_drive_vpd_inquiry_cond",
        FBE_SAS_FLASH_DRIVE_LIFECYCLE_COND_VPD_INQUIRY,
        sas_flash_drive_vpd_inquiry_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(sas_flash_drive)[] = {
    &sas_flash_drive_vpd_inquiry_cond,
};

FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(sas_flash_drive);


static fbe_lifecycle_const_rotary_cond_t sas_flash_drive_specialize_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_flash_drive_self_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    /* Base conditions */   
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_flash_drive_vpd_inquiry_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

};

static fbe_lifecycle_const_rotary_cond_t sas_flash_drive_activate_rotary[] = {
    /* Derived conditions */

    /* Base conditions */   
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_flash_drive_vpd_inquiry_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(sas_flash_drive)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, sas_flash_drive_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, sas_flash_drive_activate_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(sas_flash_drive);

/*--- global sas drive lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sas_flash_drive) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        sas_flash_drive,
        FBE_CLASS_ID_SAS_FLASH_DRIVE,                  /* This class */
        FBE_LIFECYCLE_CONST_DATA(sas_physical_drive))    /* The super class */
};

/*--- Local Functions ------------------------------------------------------------------*/


/*--- Condition Functions --------------------------------------------------------------*/

/*!**************************************************************
 * sas_flash_drive_self_init_cond_function()
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
sas_flash_drive_self_init_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t status;
    fbe_sas_flash_drive_t * sas_flash_drive = (fbe_sas_flash_drive_t*)p_object;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive,
                                               FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

     
    fbe_sas_flash_drive_init(sas_flash_drive);

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_flash_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!**************************************************************
 * sas_flash_drive_vpd_inquiry_cond_function()
 ****************************************************************
 * @brief
 *  This is the condition function where to process vpd inquiry.
 *
 * @param object - pointer to object.               
 * @param packet - pointer to packet.               
 *
 * @return fbe_lifecycle_status_t - The status.
 *
 * @author
 *   2/22/2010:  Created. chenl6
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
sas_flash_drive_vpd_inquiry_cond_function(fbe_base_object_t * object, fbe_packet_t * packet)
{
	fbe_status_t status;
    fbe_sas_flash_drive_t * sas_flash_drive = (fbe_sas_flash_drive_t*)object;
#if 0
    fbe_base_physical_drive_info_t * info_ptr = & ((fbe_base_physical_drive_t *)sas_flash_drive)->drive_info;
#endif

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* we want to read the VPD page for all Solid State Drives to determine the number of drive writes
     * per day.  Not that not all legacy drives will have this data available but in those cases
     * the valid bit will not be set.
     */
#if 0    
    /* Vpd_inquiry condition is done only when drive has a bridge card (so we can read the Paddle
       Card FW and HW revision) or the drive is an MLC drive so that we can read the number of
       Drive Writes / Day from page 0xC0. */
    if ( !( fbe_equal_memory(info_ptr->vendor_id, "SATA-", 5) || ((info_ptr->drive_parameter_flags & FBE_PDO_FLAG_MLC_FLASH_DRIVE) > 0) ) )
    {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_flash_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive,
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }
#endif

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_flash_drive_vpd_inquiry_cond_completion, sas_flash_drive);

    /* Call the executer function to do actual job */
    status = fbe_sas_flash_drive_send_vpd_inquiry(sas_flash_drive, packet);

    return FBE_LIFECYCLE_STATUS_PENDING;
}

/*!**************************************************************
 * sas_flash_drive_vpd_inquiry_cond_completion()
 ****************************************************************
 * @brief
 *  This is the completion function for VPD inquiry condition.
 *
 * @param packet - pointer to packet.               
 * @param context - completion context.               
 *
 * @return fbe_status_t - The status.
 *
 * @author
 *   2/22/2010:  Created. chenl6
 *
 ****************************************************************/
static fbe_status_t 
sas_flash_drive_vpd_inquiry_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_flash_drive_t * sas_flash_drive = (fbe_sas_flash_drive_t *)context;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t control_status_qualifier = FBE_SAS_DRIVE_STATUS_OK;
    
    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive,
            FBE_TRACE_LEVEL_DEBUG_HIGH,
            "%s entry\n", __FUNCTION__);    
    
    /* Get control status and qualifier */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);

    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    if (status != FBE_STATUS_OK) {
        /* We can't get the inquiry string, retry again */
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        
        return FBE_STATUS_OK; 
    }

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        if (control_status_qualifier == (fbe_payload_control_status_qualifier_t)FBE_SAS_DRIVE_STATUS_NEED_MORE_PROCESSING) {
            status = fbe_lifecycle_reschedule(&fbe_sas_physical_drive_lifecycle_const, 
                                             (fbe_base_object_t*)sas_flash_drive,  
                                              0);

            return FBE_STATUS_OK;
        }

        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_flash_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive,
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
        /* We are done with VPD inquiry */
        sas_flash_drive->vpd_pages_info.vpd_page_counter = 0;
        sas_flash_drive->vpd_pages_info.vpd_max_supported_pages = 0;

        /* There is no requirement for SSDs to support EQ.  Clear bit so we don't go into maintenance mode*/
        fbe_pdo_maintenance_mode_clear_flag(&sas_flash_drive->sas_physical_drive.base_physical_drive.maintenance_mode_bitmap, 
                                            FBE_PDO_MAINTENANCE_FLAG_EQ_NOT_SUPPORTED);
    }
    else
    {
        if (control_status_qualifier == (fbe_payload_control_status_qualifier_t)FBE_SAS_DRIVE_STATUS_NEED_RESCHEDULE) {
            status = fbe_lifecycle_reschedule(&fbe_sas_physical_drive_lifecycle_const, 
                                             (fbe_base_object_t*)sas_flash_drive,  
                                              0);

        }
    }

    return FBE_STATUS_OK;
}
