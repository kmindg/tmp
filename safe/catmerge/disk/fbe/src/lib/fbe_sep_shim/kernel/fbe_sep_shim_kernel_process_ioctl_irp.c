/***************************************************************************
* Copyright (C) EMC Corporation 2001-2009
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************
*  fbe_sep_shim_kernel_process_ioctl_irp.c
***************************************************************************
*
*  Description
*      Kernel implementation for the SEP shim to process IOCTL_IRP
*      
*
*  History:
*      06/24/09    sharel - Created
*    
***************************************************************************/
#include <ntddk.h>
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_library_interface.h"
#include "fbe_service_manager.h"
#include "fbe_topology.h"
#include "fbe/fbe_queue.h"
#include "fbe_sep_shim_private_interface.h"
#include "fbe/fbe_sep.h"
#include "flare_export_ioctls.h"
#include "fbe/fbe_lun.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_bvd_interface.h"
#include "fbe/fbe_base_config.h"
#include "flare_ioctls.h"
#include "fbe/fbe_lun.h"
#include "fbe_private_space_layout.h"
#include "IdmInterface.h"
#include "idm.h"
#include "core_config_runtime.h"

/**************************************
            Local variables
**************************************/
static PEMCPAL_DEVICE_OBJECT   idmDevicePtr = NULL;
static PEMCPAL_FILE_OBJECT     idmFilePtr = NULL;
/*******************************************
            Local functions
********************************************/
static fbe_status_t fbe_sep_shim_kernel_convert_fbe_lun_raid_info(FLARE_RAID_INFO *raid_info, 
                                                                  fbe_database_get_sep_shim_raid_info_t *lun_info);

static fbe_status_t fbe_sep_shim_kernel_map_volume_state_info(FLARE_VOLUME_STATE *volume_state_info, 
                                                              os_device_info_t * dev_info_p);

static fbe_status_t fbe_sep_shim_send_command_to_lun(os_device_info_t* device_info,
                                                     fbe_sep_shim_io_struct_t *	io_struct,
                                                     fbe_payload_control_operation_opcode_t control_code,
                                                     fbe_payload_control_buffer_t buffer,
                                                     fbe_payload_control_buffer_length_t buffer_length);

static fbe_status_t fbe_sep_shim_send_command_to_downstream_object(os_device_info_t* device_info,
                                                            fbe_sep_shim_io_struct_t *	io_struct,
                                                            fbe_payload_control_operation_opcode_t control_code,
                                                            fbe_payload_control_buffer_t buffer,
                                                            fbe_payload_control_buffer_length_t buffer_length,
                                                            fbe_object_id_t	object_id);

static fbe_status_t fbe_sep_shim_send_command_to_service_with_sg_list(fbe_sep_shim_io_struct_t *	io_struct,
                                                                      fbe_service_id_t service_id,
                                                                      fbe_payload_control_operation_opcode_t control_code,
                                                                      fbe_payload_control_buffer_t buffer,
                                                                      fbe_payload_control_buffer_length_t buffer_length,
                                                                      fbe_sg_element_t *sg_list,
                                                                      fbe_u32_t sg_list_count);

static fbe_status_t fbe_sep_shim_send_command_to_database_service(fbe_sep_shim_io_struct_t *	io_struct,
                                                                  fbe_payload_control_operation_opcode_t control_code,
                                                                  fbe_payload_control_buffer_t buffer,
                                                                  fbe_payload_control_buffer_length_t buffer_length);

static fbe_status_t fbe_sep_shim_process_sgl_corrupt_data_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_sep_shim_process_corrupt_data_allocation_completion(fbe_memory_request_t * memory_request, 
                                                                            fbe_memory_completion_context_t context);

static fbe_u8_t fbe_sep_shim_convert_raid_type(fbe_raid_group_type_t fbe_raid_type);

static void fbe_sep_shim_handle_cancel_registration_irp_function(PEMCPAL_DEVICE_OBJECT DeviceObject, PEMCPAL_IRP Irp);

extern fbe_status_t fbe_sep_shim_translate_status(fbe_packet_t *packet, fbe_payload_block_operation_t * block_operation, PEMCPAL_IRP PIrp, fbe_status_t packet_status, fbe_bool_t *io_failed);

static fbe_status_t fbe_sep_shim_enumerate_lun_class(fbe_sep_shim_io_struct_t *io_struct,
                                                     fbe_object_id_t ** obj_array_p, 
                                                     fbe_u32_t *num_objects_p);

static fbe_status_t sep_shim_convert_flun_to_lun(os_device_info_t *dev_info_p, fbe_assigned_wwid_t wwn, fbe_lun_number_t *idm_lun);

static fbe_status_t sep_shim_build_idm_request(IdmRequest *request,
                                               fbe_u32_t requestCode,
                                               fbe_u64_t dataBuffer,
                                               fbe_u32_t dataBufferSize,
                                               BufferType dataBufferType);

static EMCPAL_STATUS sep_shim_send_idm_ioctl(IdmRequest *idmRequestPtr);
static fbe_status_t fbe_sep_shim_kernel_open_idm(void);

static void fbe_sep_shim_kernel_map_lowest_drive_tier(FLARE_VOLUME_STATE *volume_state_info, fbe_u16_t drive_tier);
static void fbe_sep_shim_kernel_map_wear_level_info(FLARE_VOLUME_STATE *volume_state_info, 
                                                     fbe_raid_group_get_wear_level_t *wear_level_p);

/*********************************************************************
*            fbe_sep_shim_eject_media_ioctl ()
*********************************************************************
*
*  Description: puts the lun into power save mode
*
*	Inputs: PDeviceObject  - pointer to the device object
*			PIRP - IRP from top driver
*
*  Return Value: 
*	success or failure
*    
*********************************************************************/
EMCPAL_STATUS fbe_sep_shim_eject_media_ioctl(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp, fbe_sep_shim_io_struct_t *io_struct)
{

    /*fbe_status_t status;*/
    #if 0 /*not supported at this stage*/
    os_device_info_t * dev_info_p = (os_device_info_t *)EmcpalDeviceGetExtension(PDeviceObject);

    status = fbe_sep_shim_send_command_to_lun(dev_info_p, 
                                              FBE_BLOCK_TRANSPORT_CONTROL_CODE_CLIENT_HIBERNATING,
                                              NULL, 
                                              0);


    if(status != FBE_STATUS_OK) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: call to power save failed:%d\n",
                        __FUNCTION__, status);
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }
    #else
    fbe_sep_shim_trace(FBE_TRACE_LEVEL_WARNING, "%s: IOCTL_STORAGE_EJECT_MEDIA Not supported, will be ignored\n",__FUNCTION__);
    #endif

    fbe_sep_shim_return_io_structure(io_struct);
    //clean up context for cancellation
    EmcpalIrpCancelRoutineSet (PIrp, NULL);
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, 0);
    EmcpalIrpCompleteRequest(PIrp);

    return EMCPAL_STATUS_SUCCESS;
}

/*********************************************************************
*            fbe_sep_shim_load_media_ioctl ()
*********************************************************************
*
*  Description: takes the lun out of power save mode
*
*	Inputs: PDeviceObject  - pointer to the device object
*			PIRP - IRP from top driver
*
*  Return Value: 
*	success or failure
*    
*********************************************************************/
EMCPAL_STATUS fbe_sep_shim_load_media_ioctl(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp, fbe_sep_shim_io_struct_t *io_struct)
{
    fbe_status_t status;
    os_device_info_t * dev_info_p = (os_device_info_t *)EmcpalDeviceGetExtension(PDeviceObject);

    fbe_path_state_t path_state;

    fbe_block_transport_get_path_state(&dev_info_p->block_edge, &path_state);

    /*make sure host side calls us only once*/
    if (!(dev_info_p->current_attributes & VOL_ATTR_BECOMING_ACTIVE) && 
        ((path_state == FBE_PATH_STATE_SLUMBER) ||
         (dev_info_p->current_attributes & VOL_ATTR_IDLE_TIME_MET))) {

        dev_info_p->current_attributes &= ~ VOL_ATTR_STANDBY;
        dev_info_p->current_attributes &= ~VOL_ATTR_STANDBY_TIME_MET;
        /* we may enter this code when LUN is in the ready state, 2 minutes prior to going hibernation,
         * and we don't need to set becoming active for this case.
         */
        if (path_state == FBE_PATH_STATE_SLUMBER)
        {
            dev_info_p->current_attributes |= VOL_ATTR_BECOMING_ACTIVE;
        }

        fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "IOCTL_STORAGE_LOAD_MEDIA issued to LUN:%d, current attrib:0x%X, get LU out of Hibernation\n",
                        dev_info_p->lun_number, dev_info_p->current_attributes);

        status = fbe_sep_shim_send_command_to_lun(dev_info_p, 
                                              io_struct,
                                              FBE_BLOCK_TRANSPORT_CONTROL_CODE_EXIT_HIBERNATION,
                                              NULL, 
                                              0);

        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    

        if(status != FBE_STATUS_OK) {
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: call to get out of power save mode failed:%d\n",
                            __FUNCTION__, status);
            fbe_sep_shim_return_io_structure(io_struct);
            EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
            EmcpalExtIrpInformationFieldSet(PIrp, 0);
            EmcpalIrpCompleteRequest(PIrp);
            return EMCPAL_STATUS_UNSUCCESSFUL;
        }
    }else {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "IOCTL_STORAGE_LOAD_MEDIA issued to LUN:%d, current attrib:0x%X\n, path state:%d ",
                        dev_info_p->lun_number, dev_info_p->current_attributes, path_state);
    }

    fbe_sep_shim_return_io_structure(io_struct);
    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, 0);
    EmcpalIrpCompleteRequest(PIrp);

    return EMCPAL_STATUS_SUCCESS;
}


/*********************************************************************
*            fbe_sep_shim_trespass_ownership_loss_ioctl ()
*********************************************************************
*
*  Description: informs a LU that a decision has been made to trespass the LU from the SP
*
*	Inputs: PDeviceObject  - pointer to the device object
*			PIRP - IRP from top driver
*
*  Return Value: 
*	success or failure
*    
*********************************************************************/
EMCPAL_STATUS fbe_sep_shim_trespass_ownership_loss_ioctl(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp,fbe_sep_shim_io_struct_t *io_struct)
{
    /* @TODO: Shay will replace this with fixed code later. 02-07-11 */
    fbe_sep_shim_return_io_structure(io_struct);
    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, 0);
    //clean up context for cancellation
    EmcpalIrpCancelRoutineSet (PIrp, NULL);
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    

    EmcpalIrpCompleteRequest(PIrp);

    return EMCPAL_STATUS_SUCCESS;
}

/*********************************************************************
*            fbe_sep_shim_trespass_execute_ioctl ()
*********************************************************************
*
*  Description: informs a LU than a decision has been made to trespass the LU.
*               It is issued on the SP that is the new owner
*
*	Inputs: PDeviceObject  - pointer to the device object
*			PIRP - IRP from top driver
*
*  Return Value: 
*	success or failure
*    
*********************************************************************/
EMCPAL_STATUS fbe_sep_shim_trespass_execute_ioctl(PEMCPAL_DEVICE_OBJECT  PDeviceObject, PEMCPAL_IRP	PIrp, fbe_sep_shim_io_struct_t *io_struct)
{

    fbe_sep_shim_return_io_structure(io_struct);
    //clean up context for cancellation
    EmcpalIrpCancelRoutineSet (PIrp, NULL);
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
    
    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, 0);
    EmcpalIrpCompleteRequest(PIrp);

    return EMCPAL_STATUS_SUCCESS;
}

/*********************************************************************
 *            fbe_sep_shim_describe_extents_ioctl ()
 *********************************************************************
 *
 *  Description: finds disk extents(s) at and beyond the requested byte offset, and describes their characteristics
 *
 *	Inputs: PDeviceObject  - pointer to the device object
 *			PIRP - IRP from top driver
 *
 *  Return Value: 
 *	success or failure
 *    
 *********************************************************************/
EMCPAL_STATUS fbe_sep_shim_describe_extents_ioctl(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp, fbe_sep_shim_io_struct_t *io_struct)
{

    fbe_sep_shim_return_io_structure(io_struct);
    //clean up context for cancellation
    EmcpalIrpCancelRoutineSet (PIrp, NULL);
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, 0);
    EmcpalIrpCompleteRequest(PIrp);

     return EMCPAL_STATUS_SUCCESS;
}

/*********************************************************************
 *            fbe_sep_shim_register_for_change_notification_ioctl ()
 *********************************************************************
 *
 *  Description: this IRP will be completed with success if the volume state reported
 *               by IOCTL_FLARE_GET_VOLUME_STATE changed since the last IOCTL_FLARE_REGISTER_FOR_CHANGE_NOTIFICATION was returned
 *
 *	Inputs: PDeviceObject  - pointer to the device object
 *			PIRP - IRP from top driver
 *
 *  Return Value: 
 *	success or failure
 *    
 *********************************************************************/
EMCPAL_STATUS fbe_sep_shim_register_for_change_notification_ioctl(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp, fbe_sep_shim_io_struct_t *io_struct)
{
    os_device_info_t * 	dev_info_p = (os_device_info_t *)EmcpalDeviceGetExtension(PDeviceObject);
    PEMCPAL_IRP_CANCEL_FUNCTION      old_cancel_routine = NULL;

    fbe_sep_shim_return_io_structure(io_struct);/*didn't really needed it*/

    /*if there was a state change since last time, we will return the IRP*/
    if ((dev_info_p->previous_attributes != dev_info_p->current_attributes) || (dev_info_p->ready_state != FBE_TRI_STATE_INVALID)) {

        fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "sep_shim send notify to Cache,LU:#%d, Prev 0x:%X, Curr:0x%X, Rdy:0x%x\n",dev_info_p->lun_number, dev_info_p->previous_attributes,dev_info_p->current_attributes,dev_info_p->ready_state);

        dev_info_p->previous_attributes = dev_info_p->current_attributes;
        /* reset the ready state to Invalid state */
        dev_info_p->ready_state = FBE_TRI_STATE_INVALID;

        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
    }else{
        /*make sure we have a valid cancel routine*/
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "sep_shim reg. from upper layer,LU:#%d,Prev 0x:%X, Curr:0x%X, Rdy:0x%x\n",dev_info_p->lun_number,dev_info_p->previous_attributes,dev_info_p->current_attributes,dev_info_p->ready_state );
        dev_info_p->cancelled = 0;
        old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, fbe_sep_shim_handle_cancel_registration_irp_function);
        if (EmcpalIrpIsCancelInProgress(PIrp)){
          old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, NULL);
          if (old_cancel_routine) {
             EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_CANCELLED); 
             //clean up context for cancellation
             EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
             EmcpalExtIrpInformationFieldSet(PIrp, 0);
             EmcpalIrpCompleteRequest(PIrp);
             return EMCPAL_STATUS_CANCELLED;
    
          }
        }

        dev_info_p->state_change_notify_ptr = (void *)PIrp;/*hold it for later*/
    }

     return EMCPAL_STATUS_SUCCESS;
}

/*********************************************************************
 *            fbe_sep_shim_set_power_saving_policy_ioctl ()
 *********************************************************************
 *
 *  Description: set policy for power saving
 *
 *	Inputs: PDeviceObject  - pointer to the device object
 *			PIRP - IRP from top driver
 *
 *  Return Value: 
 *	success or failure
 *    
 *********************************************************************/
EMCPAL_STATUS fbe_sep_shim_set_power_saving_policy_ioctl(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp, fbe_sep_shim_io_struct_t *io_struct)
{

    /*fbe_status_t status;*/
    /*fbe_lun_set_power_saving_policy_t	fbe_ps_policy;*/
    FLARE_POWER_SAVING_POLICY	*		flare_policy;

    flare_policy = EmcpalExtIrpSystemBufferGet(PIrp);

    #if 0 /*currently not supported in Flare and will not be supported in LUN level until otherwise decided*/
    os_device_info_t * 					dev_info_p = (os_device_info_t *)EmcpalDeviceGetExtension(PDeviceObject);

    fbe_ps_policy.max_latency_allowed_in_100ns = flare_policy->MaxSpinUpTimeIn100NanoSeconds;/*a 0 value here will basically disable spin down*/
    fbe_ps_policy.lun_delay_before_hibernate_in_sec = flare_policy->IdleSecondsBeforeReportingNotReady; /*a 0xFFFFFFFF value here means we don't spin down*/
    
    status = fbe_sep_shim_send_command_to_lun(dev_info_p, 
                                              FBE_LUN_CONTROL_CODE_SET_POWER_SAVING_POLICY,
                                              &fbe_ps_policy, 
                                              sizeof(fbe_lun_set_power_saving_policy_t));


    if(status != FBE_STATUS_OK) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: call to power save failed:%d\n",
                        __FUNCTION__, status);
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(io_struct);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "PoweSave policy set:Max latency:%d 100ns, wait before save:%d 100ns \n",
                       flare_policy->MaxSpinUpTimeIn100NanoSeconds,
                       flare_policy->IdleSecondsBeforeReportingNotReady);

    #endif
    fbe_sep_shim_return_io_structure(io_struct);
    //clean up context for cancellation
    EmcpalIrpCancelRoutineSet (PIrp, NULL);
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, 0);
    EmcpalIrpCompleteRequest(PIrp);

     return EMCPAL_STATUS_SUCCESS;
}

/*********************************************************************
 *            fbe_sep_shim_capabilities_query_ioctl ()
 *********************************************************************
 *
 *  Description: Queries down the device stack for the set of capabilities that are supported
 *
 *	Inputs: PDeviceObject  - pointer to the device object
 *			PIRP - IRP from top driver
 *
 *  Return Value: 
 *	success or failure
 *    
 *********************************************************************/
EMCPAL_STATUS fbe_sep_shim_capabilities_query_ioctl(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp, fbe_sep_shim_io_struct_t *io_struct)
{
    PFLARE_CAPABILITIES_QUERY  FlareCapabilitiesPtr;

    FlareCapabilitiesPtr = (PFLARE_CAPABILITIES_QUERY) EmcpalExtIrpSystemBufferGet(PIrp);

    FlareCapabilitiesPtr->support  = FLARE_CAPABILITY_ZERO_FILL_SUPPORTED | 
                            FLARE_SGL_BOTH_SUPPORTED | FLARE_DCA_BOTH_SUPPORTED;

    fbe_sep_shim_return_io_structure(io_struct);
    //clean up context for cancellation
    EmcpalIrpCancelRoutineSet (PIrp, NULL);
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, 0);
    EmcpalIrpCompleteRequest(PIrp);

     return EMCPAL_STATUS_SUCCESS;
}

/*********************************************************************
 *            fbe_sep_shim_get_volume_state_ioctl ()
 *********************************************************************
 *
 *  Description: This request queries the volume state of the device stack, returning the current state
 *
 *	Inputs: PDeviceObject  - pointer to the device object
 *			PIRP - IRP from top driver
 *
 *  Return Value: 
 *	success or failure
 *    
 *********************************************************************/
EMCPAL_STATUS fbe_sep_shim_get_volume_state_ioctl(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp, fbe_sep_shim_io_struct_t *io_struct)
{

    FLARE_VOLUME_STATE *volume_state_info;
    fbe_status_t status;
    os_device_info_t * dev_info_p = (os_device_info_t *)EmcpalDeviceGetExtension(PDeviceObject);
    fbe_get_lowest_drive_tier_t drive_tier;
    fbe_raid_group_get_wear_level_t wear_level;
    fbe_database_get_sep_shim_raid_info_t	sep_shim_lun_info;

    volume_state_info = EmcpalExtIrpSystemBufferGet(PIrp);

    if(volume_state_info == NULL) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: Input Buffer NULL\n",
                        __FUNCTION__);
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(io_struct);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    fbe_zero_memory(volume_state_info,sizeof(FLARE_VOLUME_STATE));

    if (dev_info_p->lun_number == FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VAULT) {   
        fbe_transport_set_packet_attr(io_struct->packet, FBE_PACKET_FLAG_TRAVERSE);
        status = fbe_sep_shim_send_command_to_downstream_object(dev_info_p, 
                                                                io_struct,
                                                                FBE_RAID_GROUP_CONTROL_CODE_GET_LOWEST_DRIVE_TIER,
                                                                &drive_tier, 
                                                                sizeof(fbe_get_lowest_drive_tier_t),
                                                                FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG);
        if(status != FBE_STATUS_OK) {
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: Drive tier conversion failed:%d: %X\n",
                            __FUNCTION__, status, (unsigned int)status);
            //clean up context for cancellation
            EmcpalIrpCancelRoutineSet (PIrp, NULL);
            EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;  
            EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
            EmcpalExtIrpInformationFieldSet(PIrp, 0);
            EmcpalIrpCompleteRequest(PIrp);
            fbe_sep_shim_return_io_structure(io_struct);
            return EMCPAL_STATUS_UNSUCCESSFUL;
        }
        fbe_sep_shim_kernel_map_lowest_drive_tier(volume_state_info, drive_tier.drive_tier);
    }

    //clean up context for cancellation
    EmcpalIrpCancelRoutineSet (PIrp, NULL);
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;  

    
    status = fbe_sep_shim_kernel_map_volume_state_info(volume_state_info, dev_info_p);
    if(status != FBE_STATUS_OK) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: Conversion failed:%d: %X\n",
                        __FUNCTION__, status, (unsigned int)status);
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(io_struct);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    /* Get the lun object id */
    fbe_block_transport_get_server_id(&dev_info_p->block_edge, &sep_shim_lun_info.lun_object_id);
    status = fbe_database_get_sep_shim_raid_info(&sep_shim_lun_info);
    if(status != FBE_STATUS_OK) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: API call to get raid info failed:%X\n",
                        __FUNCTION__, status);
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(io_struct);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    // fetch the wear level information for this lun
    fbe_transport_set_packet_attr(io_struct->packet, FBE_PACKET_FLAG_TRAVERSE); 
    status = fbe_sep_shim_send_command_to_downstream_object(dev_info_p, 
                                                            io_struct,
                                                            FBE_RAID_GROUP_CONTROL_CODE_GET_WEAR_LEVEL,
                                                            &wear_level, 
                                                            sizeof(fbe_raid_group_get_wear_level_t),
                                                            sep_shim_lun_info.raid_group_id);
    if(status != FBE_STATUS_OK) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: rg 0x%x wear level failed: %d: %X\n",
                           __FUNCTION__, sep_shim_lun_info.raid_group_id, status, (unsigned int)status);
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(io_struct);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }
    fbe_sep_shim_kernel_map_wear_level_info(volume_state_info, &wear_level);


    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, sizeof(FLARE_VOLUME_STATE));
    EmcpalIrpCompleteRequest(PIrp);
    fbe_sep_shim_return_io_structure(io_struct);

    return EMCPAL_STATUS_SUCCESS;
}

/*********************************************************************
 *            fbe_sep_shim_get_raid_info_ioctl ()
 *********************************************************************
 *
 *  Description: This request allows the caller to get information about
 *  the Raid organization of the backend device
 *
 *	Inputs: PDeviceObject  - pointer to the device object
 *			PIRP - IRP from top driver
 *
 *  Return Value: 
 *	success or failure
 * 
 *   11/17/09 - Created - Ashok Tamilarasan
 *********************************************************************/
EMCPAL_STATUS fbe_sep_shim_get_raid_info_ioctl(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp, fbe_sep_shim_io_struct_t *sep_shim_io_struct)
{
    FLARE_RAID_INFO *						raid_info;
    fbe_status_t 							status;
    os_device_info_t * 						dev_info_p = (os_device_info_t *)EmcpalDeviceGetExtension(PDeviceObject);
    fbe_u32_t 								idm_lun_number = 0;
    fbe_database_get_sep_shim_raid_info_t	sep_shim_lun_info;

    raid_info = EmcpalExtIrpSystemBufferGet(PIrp);

    if(raid_info == NULL) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: Input Buffer NULL\n",
                        __FUNCTION__);
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    fbe_zero_memory(raid_info, sizeof(FLARE_RAID_INFO));

    /* Get the lun object id */
    fbe_block_transport_get_server_id(&dev_info_p->block_edge, &sep_shim_lun_info.lun_object_id);

    /*direct call to DB, no packets needed. This one needs to get back very fast*/
    status = fbe_database_get_sep_shim_raid_info(&sep_shim_lun_info);
    if(status != FBE_STATUS_OK) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: API call to get info failed:%X\n",
                        __FUNCTION__, status);
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    fbe_sep_shim_return_io_structure(sep_shim_io_struct);

    /*clean up context for cancellation*/

    EmcpalIrpCancelRoutineSet (PIrp, NULL);
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    

    /*convert the Flare number to whatever IDM holds for mapping*/
    status = sep_shim_convert_flun_to_lun(dev_info_p, sep_shim_lun_info.world_wide_name, &idm_lun_number);
    if(status != FBE_STATUS_OK) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: Can't map FLUN:%d to IDM\n",
                        __FUNCTION__, dev_info_p->lun_number);
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    raid_info->Lun = idm_lun_number;
    raid_info->Flun = dev_info_p->lun_number;
    raid_info->GroupId = sep_shim_lun_info.raid_group_number;

    status = fbe_sep_shim_kernel_convert_fbe_lun_raid_info(raid_info, &sep_shim_lun_info);
    if(status != FBE_STATUS_OK) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: Conversion failed:%d\n",
                        __FUNCTION__, status);
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "sep_shim_raid_info:FLUN:%d,IDM:%d,rgid:%d,str:0x%X,rr:%d,rType:%d,luch:%d,elm:0x%X\n", 
                       raid_info->Flun, raid_info->Lun, raid_info->GroupId, raid_info->StripeCount,raid_info->RotationalRate, raid_info->RaidType,raid_info->LunCharacteristics,raid_info->BytesPerElement);

    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, sizeof(FLARE_RAID_INFO));
    EmcpalIrpCompleteRequest(PIrp);

    return EMCPAL_STATUS_SUCCESS;
}

/*********************************************************************
 *            fbe_sep_shim_send_command_to_lun ()
 *********************************************************************
 *
 *  Description: This functions sends the control operation to
 *  the LUN object
 *
 *	Inputs: 
 *          device_info - Pointer to the OS device information
 *          io_struct - The io structure.
 *          control_code - Control Code for the operation
 *          buffer - Pointer to the buffer having control info
 *          buffer_length - Buffer Length
 *          
 *
 *  Return Value: 
 *	success or failure
 * 
 *   11/17/09 - Created - Ashok Tamilarasan
 *********************************************************************/
static fbe_status_t fbe_sep_shim_send_command_to_lun(os_device_info_t* device_info,
                                                     fbe_sep_shim_io_struct_t *	io_struct,
                                                     fbe_payload_control_operation_opcode_t control_code,
                                                     fbe_payload_control_buffer_t buffer,
                                                     fbe_payload_control_buffer_length_t buffer_length)
{
    fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t * 						packet = NULL;
    fbe_payload_ex_t *					sep_payload = NULL;
    fbe_payload_control_operation_t *	control_operation = NULL;
    fbe_object_id_t						server_id;
    
    packet = io_struct->packet;

    /*! @todo Need to get the `io_stamp' from the IRP and call 
     *        fbe_transport_set_io_stamp()
     */

    sep_payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(sep_payload);

    fbe_payload_control_build_operation (control_operation,
                                         control_code,
                                         buffer,
                                         buffer_length);

    /* get the Object ID of the LUN and send the command to it
     * directly
     */
    fbe_block_transport_get_server_id(&device_info->block_edge, &server_id);

    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              server_id); 

    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    packet->base_edge = (fbe_base_edge_t *)(&device_info->block_edge);
    status = fbe_service_manager_send_control_packet(packet);

    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        /* Only trace when it is an error we do not expect.*/
         fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, status:%d\n", __FUNCTION__, status);
         fbe_payload_ex_release_control_operation(sep_payload, control_operation);  
         return status;

    }

    /* Wait for the callback.  Our completion function is always called.*/
    fbe_transport_wait_completion(packet);

    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);
    fbe_payload_ex_release_control_operation(sep_payload, control_operation);

    return status;
}

/*********************************************************************
 *            fbe_sep_shim_send_command_to_downstream_object()
 *********************************************************************
 *
 *  Description: This functions sends the control operation to
 *  a downstream object connected to the lun
 *
 *	Inputs: 
 *          device_info - Pointer to the OS device information
 *          io_struct - The io structure.
 *          control_code - Control Code for the operation
 *          buffer - Pointer to the buffer having control info
 *          buffer_length - Buffer Length
 *          
 *
 *  Return Value: 
 *	success or failure
 * 
 *   11/17/09 - Created - Ashok Tamilarasan
 *   02/24/2014 - Modified - Deanna Heng
 *********************************************************************/
static fbe_status_t fbe_sep_shim_send_command_to_downstream_object(os_device_info_t* device_info,
                                                            fbe_sep_shim_io_struct_t *	io_struct,
                                                            fbe_payload_control_operation_opcode_t control_code,
                                                            fbe_payload_control_buffer_t buffer,
                                                            fbe_payload_control_buffer_length_t buffer_length,
                                                            fbe_object_id_t	object_id)
{
    fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t * 						packet = NULL;
    fbe_payload_ex_t *					sep_payload = NULL;
    fbe_payload_control_operation_t *	control_operation = NULL;
    
    packet = io_struct->packet;

    /*! @todo Need to get the `io_stamp' from the IRP and call 
     *        fbe_transport_set_io_stamp()
     */

    sep_payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(sep_payload);

    fbe_payload_control_build_operation (control_operation,
                                         control_code,
                                         buffer,
                                         buffer_length);

    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 

    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    packet->base_edge = (fbe_base_edge_t *)(&device_info->block_edge);
    status = fbe_service_manager_send_control_packet(packet);

    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        /* Only trace when it is an error we do not expect.*/
         fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, status:%d\n", __FUNCTION__, status);
         fbe_payload_ex_release_control_operation(sep_payload, control_operation);  
         return status;

    }

    /* Wait for the callback.  Our completion function is always called.*/
    fbe_transport_wait_completion(packet);

    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);
    fbe_payload_ex_release_control_operation(sep_payload, control_operation);

    return status;
}


/*********************************************************************
 *            fbe_sep_shim_send_command_to_service_with_sg_list()
 *********************************************************************
 *
 *  Description: This functions sends the control operation to
 *  the service with sg list
 *
 *	Inputs: 
 *          io_struct - The io structure.
 *          service_id - Service ID which indicates the command send to
 *          control_code - Control Code for the operation
 *          buffer - Pointer to the buffer having control info
 *          buffer_length - Buffer Length
 *          sg_list - Pointer to the sg list
 *          sg_list_count - sg list count
 *          
 *
 *  Return Value: 
 *	success or failure
 * 
 *   04/09/2012 - Created - Vera Wang
 *********************************************************************/
static fbe_status_t fbe_sep_shim_send_command_to_service_with_sg_list(fbe_sep_shim_io_struct_t *	io_struct,
                                                                      fbe_service_id_t service_id,
                                                                      fbe_payload_control_operation_opcode_t control_code,
                                                                      fbe_payload_control_buffer_t buffer,
                                                                      fbe_payload_control_buffer_length_t buffer_length,
                                                                      fbe_sg_element_t *sg_list,
                                                                      fbe_u32_t sg_list_count)
{
    fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t * 						packet = NULL;
    fbe_payload_ex_t *					sep_payload = NULL;
    fbe_payload_control_operation_t *	control_operation = NULL;
    
    packet = io_struct->packet;
    sep_payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(sep_payload);
    /*set sg list*/
    fbe_payload_ex_set_sg_list(sep_payload, sg_list, sg_list_count);

    fbe_payload_control_build_operation (control_operation,
                                         control_code,
                                         buffer,
                                         buffer_length);

    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_SEP_0,
                              service_id,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID); 

    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    status = fbe_service_manager_send_control_packet(packet);

    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        /* Only trace when it is an error we do not expect.*/
         fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, status:%d\n", __FUNCTION__, status);  
         fbe_payload_ex_release_control_operation(sep_payload, control_operation);
         return status;

    }

    /* Wait for the callback.  Our completion function is always called.*/
    fbe_transport_wait_completion(packet);

    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);
    fbe_payload_ex_release_control_operation(sep_payload, control_operation);
    return status;
}

/*********************************************************************
 *            fbe_sep_shim_send_command_to_database_service ()
 *********************************************************************
 *
 *  Description: This functions sends the control operation to
 *  the LUN object
 *
 *	Inputs: 
 *          io_struct - The io structure.
 *          control_code - Control Code for the operation
 *          buffer - Pointer to the buffer having control info
 *          buffer_length - Buffer Length
 *          
 *
 *  Return Value: 
 *	success or failure
 * 
 *   11/17/09 - Created - Ashok Tamilarasan
 *********************************************************************/
static fbe_status_t fbe_sep_shim_send_command_to_database_service(fbe_sep_shim_io_struct_t *io_struct,
                                                                  fbe_payload_control_operation_opcode_t control_code,
                                                                  fbe_payload_control_buffer_t buffer,
                                                                  fbe_payload_control_buffer_length_t buffer_length)
{
    fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t * 						packet = NULL;
    fbe_payload_ex_t *					sep_payload = NULL;
    fbe_payload_control_operation_t *	control_operation = NULL;
    
    packet = io_struct->packet;
    sep_payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(sep_payload);

    fbe_payload_control_build_operation (control_operation,
                                         control_code,
                                         buffer,
                                         buffer_length);
    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_DATABASE,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID); 
    
    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    
    status = fbe_service_manager_send_control_packet(packet);

    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        /* Only trace when it is an error we do not expect.*/
         fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, status:%d\n", __FUNCTION__, status);  
         fbe_payload_ex_release_control_operation(sep_payload, control_operation);
         return status;

    }

    /* Wait for the callback.  Our completion function is always called.*/
    fbe_transport_wait_completion(packet);

    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);
    fbe_payload_ex_release_control_operation(sep_payload, control_operation);
    return status;
}


/*********************************************************************
 *            fbe_sep_shim_convert_raid_type ()
 *********************************************************************
 *
 *  Description: This converts raid type from fbe format to flare format
 *
 *	Inputs: fbe_raid_type  - raid type in fbe format
 *
 *  Return Value: 
 *	raid type in flare format
 * 
 *   12/22/10 - Created - NCHIU
 *********************************************************************/
static fbe_u8_t fbe_sep_shim_convert_raid_type(fbe_raid_group_type_t fbe_raid_type)
{
    fbe_u8_t flare_raid_type;

    switch(fbe_raid_type)
    {
    case FBE_RAID_GROUP_TYPE_RAID1:
        flare_raid_type = FLARE_RAID_TYPE_RAID1;
        break;
    case FBE_RAID_GROUP_TYPE_RAID10:
        flare_raid_type = FLARE_RAID_TYPE_RAID10;
        break;
    case FBE_RAID_GROUP_TYPE_RAID3:
        flare_raid_type = FLARE_RAID_TYPE_RAID3;
        break;
    case FBE_RAID_GROUP_TYPE_RAID0:
        flare_raid_type = FLARE_RAID_TYPE_RAID0;
        break;
    case FBE_RAID_GROUP_TYPE_RAID5:
        flare_raid_type = FLARE_RAID_TYPE_RAID5;
        break;
    case FBE_RAID_GROUP_TYPE_RAID6:
        flare_raid_type = FLARE_RAID_TYPE_RAID6;
        break;
    case FBE_RAID_GROUP_TYPE_SPARE:
    default:
        flare_raid_type = FLARE_RAID_TYPE_NONE;
        break;
    }

    return flare_raid_type;
}


/*********************************************************************
 *            fbe_sep_shim_kernel_convert_fbe_lun_info ()
 *********************************************************************
 *
 *  Description: This request allows the caller to get information about
 *  the Raid organization of the backend device
 *
 *	Inputs: PDeviceObject  - pointer to the device object
 *			PIRP - IRP from top driver
 *
 *  Return Value: 
 *	success or failure
 * 
 *   11/17/09 - Created - Ashok Tamilarasan
 *********************************************************************/
static fbe_status_t fbe_sep_shim_kernel_convert_fbe_lun_raid_info(FLARE_RAID_INFO *raid_info, 
                                                                  fbe_database_get_sep_shim_raid_info_t *lun_info)
{
    raid_info->RaidType = fbe_sep_shim_convert_raid_type(lun_info->raid_type);

    raid_info->FlareMajorRevision = 0;
    raid_info->FlareMinorRevision = 0;
    raid_info->LunCharacteristics = lun_info->lun_characteristics;
    raid_info->RotationalRate = lun_info->rotational_rate;
    raid_info->BytesPerSector = lun_info->exported_block_size;
    raid_info->DisksPerStripe = lun_info->width;
    raid_info->SectorsPerStripe = (ULONG)(lun_info->sectors_per_stripe);
    raid_info->BytesPerElement = (ULONG)(lun_info->element_size * FBE_BYTES_PER_BLOCK);

    /* Calculate the stripe count for LUN based on LUN exported block size and RG stripe size */
    raid_info->StripeCount = (ULONG)(lun_info->capacity / lun_info->sectors_per_stripe);

    fbe_copy_memory(&raid_info->WorldWideName, &lun_info->world_wide_name, sizeof(raid_info->WorldWideName));
    fbe_copy_memory(&raid_info->UserDefinedName, &lun_info->user_defined_name, sizeof(raid_info->UserDefinedName));

    /* Set for Inquiry 0xB0 page. Note: This needs to be set so they can be consistent with Inyo. */
    raid_info->OptimalTransferLengthGranularity = 0;
    raid_info->OptimalUnmapGranularity = 0;
    raid_info->UnmapGranularityAlignment = 0;
    raid_info->MaxUnmapLbaCount = 0;

    /* raid_info->BindTime Need to be set for history purpose.*/
    raid_info->BindTime = (ULONG)(lun_info->bind_time);

    /* R3/R5/R6 will have 8 elements per parity stripe.  The other RAID types should have 0.  However, because
     * SPCache is expecting the non-zeroed elements per parity stripe, MCR needs to set 0 to 1.
     */
    raid_info->ElementsPerParityStripe = (ULONG)((lun_info->elements_per_parity_stripe > 0) ? lun_info->elements_per_parity_stripe : 1);

    raid_info->MaxQueueDepth = lun_info->max_queue_depth;
    return FBE_STATUS_OK;
}
/*********************************************************************
 *            fbe_sep_shim_kernel_map_volume_state_info ()
 *********************************************************************
 *
 *  Description: This request allows the caller to get information about
 *  the volume state
 *
 *
 *  Return Value: 
 *	success or failure
 * 
 *********************************************************************/
static fbe_status_t fbe_sep_shim_kernel_map_volume_state_info(FLARE_VOLUME_STATE *volume_state_info, 
                                                              os_device_info_t * dev_info_p)
{
    fbe_status_t				status;
    fbe_path_state_t 			path_state;
    fbe_block_count_t			cache_zero_bit_map_size = 0;
    fbe_lba_t					lun_capacity = dev_info_p->block_edge.capacity;

    status = fbe_block_transport_get_path_state(&dev_info_p->block_edge, &path_state);
    if (status != FBE_STATUS_OK){
       fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: can't get path state\n", __FUNCTION__);
       return FBE_STATUS_GENERIC_FAILURE;
    }

    volume_state_info->geometry.BytesPerSector = FBE_BYTES_PER_BLOCK;/*always export 512*/

	/*do some capacity magic so we return only the user exported capacity*/
	fbe_lun_calculate_cache_zero_bit_map_size_to_remove(lun_capacity, &cache_zero_bit_map_size);
    volume_state_info->capacity = lun_capacity - cache_zero_bit_map_size;

    /* Hardcoding some values for now.*/
    volume_state_info->geometry.SectorsPerTrack   = 1;
    volume_state_info->geometry.TracksPerCylinder = 1;

    volume_state_info->VolumeAttributes = dev_info_p->current_attributes;
    
    volume_state_info->geometry.Cylinders.QuadPart = volume_state_info->capacity;

    if (dev_info_p->current_attributes & VOL_ATTR_BECOMING_ACTIVE) {
        volume_state_info->NotReady = 0;
    }
    else
    {
        /* set up NotReady value based on the Path State */
        volume_state_info->NotReady = (((path_state == FBE_PATH_STATE_ENABLED)||(path_state == FBE_PATH_STATE_SLUMBER)) ? 0 : 1);
    }

    fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "sep_shim_vol_state:LU#:%d, V_Capactiy:0x%llx, NotReady:%d,attr:0x%X\n",
                       dev_info_p->lun_number,
               (unsigned long long)volume_state_info->capacity, 
                       volume_state_info->NotReady, volume_state_info->VolumeAttributes);

    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_sep_shim_kernel_map_lowest_drive_tier ()
 *********************************************************************
 *
 *  Description: This request allows the caller to get information about
 *  the volume state
 *
 *
 *  Return Value: 
 *	
 * 
 *********************************************************************/
static void fbe_sep_shim_kernel_map_lowest_drive_tier(FLARE_VOLUME_STATE *volume_state_info, 
                                                      fbe_u16_t drive_tier)
{
#ifdef VNX2_ONLY_VAULT_FEATURE
//  ***** NOTE: content relevant to VNX2 Vault-behavior only. 
//  ***** Do not merge and remove ifdef and included code if found in other stream
    switch (drive_tier) 
    {
        case FBE_DRIVE_PERFORMANCE_TIER_TYPE_7K:
            volume_state_info->LunMinPerformanceTier = HDD_MIN_LUN_PERF_TIER_SAS_NL;
            break;
        case FBE_DRIVE_PERFORMANCE_TIER_TYPE_10K:
            volume_state_info->LunMinPerformanceTier =  HDD_MIN_LUN_PERF_TIER_10K_SAS;
            break;
        case FBE_DRIVE_PERFORMANCE_TIER_TYPE_15K:
            volume_state_info->LunMinPerformanceTier = HDD_MIN_LUN_PERF_TIER_15K_SAS;
            break;
        default:
            volume_state_info->LunMinPerformanceTier = LUN_MIN_PERF_TIER_UNDEFINED;
            break;
    }

    fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "sep_shim_vol_state:vault drive_tier: 0x%x\n",
                       volume_state_info->LunMinPerformanceTier);
#else
    volume_state_info->SpareByte0 = 0;
#endif  // VNX2_ONLY_VAULT_FEATURE

}

/*********************************************************************
 *            fbe_sep_shim_kernel_map_wear_level_info ()
 *********************************************************************
 *
 *  Description: This request allows the caller to get information about
 *  the volume state
 *
 *
 *  Return Value: 
 *	
 * 
 *********************************************************************/
static void fbe_sep_shim_kernel_map_wear_level_info(FLARE_VOLUME_STATE *volume_state_info, 
                                                    fbe_raid_group_get_wear_level_t *wear_level_p)
{
    volume_state_info->MaxPECycle = wear_level_p->max_pe_cycle;
    volume_state_info->CurrentPECycle = wear_level_p->current_pe_cycle;
    volume_state_info->TotalPowerOnHours = wear_level_p->power_on_hours;

    fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "sep_shim_vol_state: max_pe_cycle: 0x%llx current_pe_cycle: 0x%llx\n",
                       volume_state_info->MaxPECycle, volume_state_info->CurrentPECycle);

}

/*********************************************************************
 *            fbe_sep_shim_write_buffer_ioctl ()
 *********************************************************************
 *
 *  Description: ????????????????????????
 *
 *	Inputs: PDeviceObject  - pointer to the device object
 *			PIRP - IRP from top driver
 *
 *  Return Value: 
 *	success or failure
 *    
 *********************************************************************/
EMCPAL_STATUS fbe_sep_shim_write_buffer_ioctl(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp, fbe_sep_shim_io_struct_t *sep_shim_io_struct)
{
    RW_BUFFER_INVALIDATE_DATA *         invalidateInfoPtr;
    EMCPAL_STATUS ntstatus =            EMCPAL_STATUS_SUCCESS;
    fbe_u32_t                           number_of_chunks;
    fbe_status_t                        fbe_status;

    /* SystemBuffer contains the invalidate data structure.
     */
    invalidateInfoPtr = (RW_BUFFER_INVALIDATE_DATA *)EmcpalExtIrpSystemBufferGet(PIrp);

    /* Validate that it's a write buffer request that we support.
     */
    switch (invalidateInfoPtr->header.BufferID)
    {
        case RW_BUFFER_ID_INVALIDATE:

            // Check the revision.
            switch (invalidateInfoPtr->invalidate.revision)
            {
                case INVALIDATE_DATA_REVISION:
                    /* The memory service will be asked for chunks containing 64 blocks, calculate how
                     * many chunks that is.
                     */
                    number_of_chunks = (invalidateInfoPtr->invalidate.numberOfBlocks + FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64 - 1) / 
                                           FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64;

                    /* Allocate the memory for the operation. Add one more chunk for the SG list.
                     */
                    fbe_memory_request_init(&sep_shim_io_struct->buffer);
                    fbe_memory_build_chunk_request(&sep_shim_io_struct->buffer, 
                                                   FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO,
                                                   number_of_chunks + 1,
                                                   0, /* Lowest resource priority */
                                                   FBE_MEMORY_REQUEST_IO_STAMP_INVALID,
                                                   (fbe_memory_completion_function_t)fbe_sep_shim_process_corrupt_data_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                                   sep_shim_io_struct);

                    fbe_status = fbe_memory_request_entry(&sep_shim_io_struct->buffer);
                    if ((fbe_status != FBE_STATUS_OK) && (fbe_status != FBE_STATUS_PENDING))
                    {
                        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: Unexpected Error 0x%x allocating memory.\n",
                                           __FUNCTION__, fbe_status);
                        //clean up context for cancellation
                        EmcpalIrpCancelRoutineSet (PIrp, NULL);
                        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
                        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
                        EmcpalExtIrpInformationFieldSet(PIrp, 0);
                        EmcpalIrpCompleteRequest(PIrp);
                        ntstatus = EMCPAL_STATUS_UNSUCCESSFUL;
                        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
                        break;
                    }

                    ntstatus = EMCPAL_STATUS_PENDING;
                    break;

                default:
                    // Not a revision that we support.
                    fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: Unsuported invalidate revision.\n",
                                       __FUNCTION__);
                    //clean up context for cancellation
                    EmcpalIrpCancelRoutineSet (PIrp, NULL);
                    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
                    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
                    EmcpalExtIrpInformationFieldSet(PIrp, 0);
                    EmcpalIrpCompleteRequest(PIrp);
                    ntstatus = EMCPAL_STATUS_UNSUCCESSFUL;
                    fbe_sep_shim_return_io_structure(sep_shim_io_struct);
                    break;
            }
            break;

        default:
            // Not one that we support.
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: Unsuported WriteBuffer request.\n",
                              __FUNCTION__);
            //clean up context for cancellation
            EmcpalIrpCancelRoutineSet (PIrp, NULL);
            EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
            EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
            EmcpalExtIrpInformationFieldSet(PIrp, 0);
            EmcpalIrpCompleteRequest(PIrp);
            ntstatus = EMCPAL_STATUS_UNSUCCESSFUL;
            fbe_sep_shim_return_io_structure(sep_shim_io_struct);
            break;

    } // end switch (bufferInfoPtr->BufferID)

    return ntstatus;
}

static fbe_status_t fbe_sep_shim_process_corrupt_data_allocation_completion(fbe_memory_request_t * memory_request_p, 
                                                                            fbe_memory_completion_context_t context)
{
    fbe_packet_t * 				        packet_p = NULL;
    fbe_memory_header_t *	 	        master_memory_header,  * current_memory_header;
    fbe_sep_shim_io_struct_t *	        sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)context;
    PEMCPAL_IRP 						PIrp;
    fbe_payload_ex_t *                 	sep_payload_p = NULL;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    PEMCPAL_DEVICE_OBJECT  		        PDeviceObject;
    os_device_info_t *                  dev_info_p;
    fbe_block_edge_geometry_t           block_edge_geometry;
    fbe_block_size_t                    exp_block_size = 0, imp_block_size = 0;
    fbe_status_t                        fbe_status = FBE_STATUS_GENERIC_FAILURE;
    fbe_xor_invalidate_sector_t         xor_invalidate;
    fbe_sg_element_t *                  sg_p = NULL;
    fbe_u32_t                           blocks_remaining;
    fbe_u32_t                           number_of_used_chunks;
    RW_BUFFER_INVALIDATE_DATA *         invalidateInfoPtr;
    PEMCPAL_IRP_CANCEL_FUNCTION         old_cancel_routine = NULL;

    PIrp = (PEMCPAL_IRP)sep_shim_io_struct->associated_io;
    PDeviceObject = (PEMCPAL_DEVICE_OBJECT)sep_shim_io_struct->associated_device;
    dev_info_p = (os_device_info_t *)EmcpalDeviceGetExtension(PDeviceObject);

    /* If memory request is null then return error. */
    if(memory_request_p == NULL)
    {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: Unexpected NULL memory request.\n",
                           __FUNCTION__);
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Handle case where allocation fails */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR,
                           "%s: Memory allocation failed. request state: %d \n",
                           __FUNCTION__, memory_request_p->request_state);
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Get the memory header */
    master_memory_header = current_memory_header = memory_request_p->ptr;

    /* SystemBuffer contains the invalidate data structure.
     */
    invalidateInfoPtr = (RW_BUFFER_INVALIDATE_DATA *)EmcpalExtIrpSystemBufferGet(PIrp);

    packet_p = sep_shim_io_struct->packet;
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);
    fbe_block_transport_edge_get_geometry(&dev_info_p->block_edge, &block_edge_geometry);
    fbe_block_transport_get_exported_block_size(block_edge_geometry, &exp_block_size);
    fbe_block_transport_get_physical_block_size(block_edge_geometry, &imp_block_size);

    /* Build the block operation.
     */
    fbe_status = fbe_payload_block_build_operation(block_operation_p,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA,
                                                   invalidateInfoPtr->invalidate.startingLBA,
                                                   invalidateInfoPtr->invalidate.numberOfBlocks,
                                                   exp_block_size,
                                                   imp_block_size,
                                                   NULL/*HACK for now, we don't need it*/);

    /* Clear the check crc flag since we are going to create a buffer full of
     * invalidated blocks.
     */
    fbe_payload_block_clear_flag(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM);

    /* The first chunk is the one to be used for the SG.
     */
    sg_p = (fbe_sg_element_t *) current_memory_header->data;

    /* Advance to the next chunk, set that one chunk has been used, 
     * and set the number of blocks for this transfer.
     */
    current_memory_header = current_memory_header->u.hptr.next_header;
    number_of_used_chunks = 1;
    blocks_remaining = invalidateInfoPtr->invalidate.numberOfBlocks;

    while (blocks_remaining != 0)
    {
        /* Set the number of blocks for this chunk to be either the size of the chunk, 64
         * or the number of blocks remaining in the request, whichever is smaller. 
         */
        fbe_u32_t current_block_count = min(blocks_remaining, FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64);

        if((number_of_used_chunks) < master_memory_header->number_of_chunks)
        {
            /* Set this chunk as the next entry in the SGL. 
             */
            fbe_sg_element_init(&sg_p[number_of_used_chunks - 1], 
                                current_block_count * exp_block_size,
                                current_memory_header->data);

            /* Advance to the next chunk
             */
            number_of_used_chunks++;
            current_memory_header = current_memory_header->u.hptr.next_header;
            blocks_remaining -= current_block_count;  
        }
        else
        {
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: Unexpectedly ran out of allocated memory.\n",
                               __FUNCTION__);
            EmcpalIrpCancelRoutineSet (PIrp, NULL);
            EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
            EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
            EmcpalExtIrpInformationFieldSet(PIrp, 0);
            EmcpalIrpCompleteRequest(PIrp);

            /* Free allocated memory
             */
            //fbe_memory_free_entry(sep_shim_io_struct->buffer.ptr);

            fbe_sep_shim_return_io_structure(sep_shim_io_struct);
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
         }
    }

    /* Terminate the last SG entry.
     */
    fbe_sg_element_terminate(&sg_p[number_of_used_chunks-1]);

    /* Assign the sg pointer to the payload.
     */
    fbe_payload_ex_set_sg_list(sep_payload_p, sg_p, 0);

    /* Setup the invalidation interface structure.
     */
    xor_invalidate.fru[0].sg_p = sg_p;
    xor_invalidate.fru[0].seed = invalidateInfoPtr->invalidate.startingLBA;
    xor_invalidate.fru[0].count = invalidateInfoPtr->invalidate.numberOfBlocks;
    xor_invalidate.fru[0].offset = 0;
            
    /* There is only one 'disk' for this since we are only invalidating one SGL.
     */
    xor_invalidate.data_disks = 1;

    /* Reason is `corrupt data' and who is client.
     */
    xor_invalidate.invalidated_reason = XORLIB_SECTOR_INVALID_REASON_CORRUPT_DATA;
    xor_invalidate.invalidated_by = XORLIB_SECTOR_INVALID_WHO_CLIENT;

    /* Invalidate the data.
     */
    fbe_status = fbe_xor_lib_execute_invalidate_sector(&xor_invalidate);
    if (fbe_status != FBE_STATUS_OK)
    {
        /* We don't expect any errors invalidating.
         */
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: Unexpected error 0x%x invalidating for corrupt data.\n",
                           __FUNCTION__, fbe_status);
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);

        /* Free allocated memory
         */
        //fbe_memory_free_entry(sep_shim_io_struct->buffer.ptr);

        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return fbe_status;
    }

    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              dev_info_p->bvd_object_id); 

    /* Make sure the packet has our edge.
     */
    packet_p->base_edge = &dev_info_p->block_edge.base_edge;

    //Store context for cancellation
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = (PVOID)packet_p;    
    
    old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, sep_handle_cancel_irp_function);
    if (EmcpalIrpIsCancelInProgress(PIrp)){
      old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, NULL);
      EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_CANCELLED); 
      EmcpalExtIrpInformationFieldSet(PIrp, 0);
      EmcpalIrpCompleteRequest(PIrp);
      fbe_sep_shim_return_io_structure(sep_shim_io_struct);
      return EMCPAL_STATUS_CANCELLED;
    }

    /* Set the completion function.
     */
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_sep_shim_process_sgl_corrupt_data_completion, 
                                          sep_shim_io_struct);

    /* Send the IO.
     */
    fbe_topology_send_io_packet(packet_p);
    return fbe_status;
}

static fbe_status_t fbe_sep_shim_process_sgl_corrupt_data_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sep_shim_io_struct_t *				sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)context;
    PEMCPAL_IRP                            	PIrp = sep_shim_io_struct->associated_io;
    fbe_payload_ex_t *                     	payload = NULL;
    fbe_payload_block_operation_t *         block_operation = NULL;
    fbe_status_t							status;
    fbe_bool_t								io_failed;

    payload = fbe_transport_get_payload_ex (packet);
    block_operation = fbe_payload_ex_get_block_operation(payload);

    /* Translate the fbe status to an IRP status.
     */
    status = fbe_transport_get_status_code(packet);
    fbe_sep_shim_translate_status(packet, block_operation, PIrp, status, &io_failed);

    //clean up context for cancellation
    EmcpalIrpCancelRoutineSet (PIrp, NULL);
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    

    EmcpalIrpCompleteRequest(PIrp);
    fbe_payload_ex_release_block_operation(payload, block_operation);
    
    /* Free allocated memory
     */
    //fbe_memory_free_entry(sep_shim_io_struct->buffer.ptr);

    fbe_sep_shim_return_io_structure(sep_shim_io_struct);
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_sep_shim_read_buffer_ioctl ()
 *********************************************************************
 *
 *  Description: ???????????????
 *
 *	Inputs: PDeviceObject  - pointer to the device object
 *			PIRP - IRP from top driver
 *
 *  Return Value: 
 *	success or failure
 *    
 *********************************************************************/
EMCPAL_STATUS fbe_sep_shim_read_buffer_ioctl(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp, fbe_sep_shim_io_struct_t *io_struct)
{
    fbe_sep_shim_return_io_structure(io_struct);
    EmcpalIrpCancelRoutineSet (PIrp, NULL);
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, sizeof(FLARE_MODIFY_CAPACITY_INFO));
    EmcpalIrpCompleteRequest(PIrp);
    return EMCPAL_STATUS_SUCCESS;
}

EMCPAL_STATUS fbe_sep_shim_process_modify_capacity(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP PIrp, fbe_sep_shim_io_struct_t *io_struct)
{
    fbe_sep_shim_return_io_structure(io_struct);
    //clean up context for cancellation
    EmcpalIrpCancelRoutineSet (PIrp, NULL);
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, sizeof(FLARE_MODIFY_CAPACITY_INFO));
    EmcpalIrpCompleteRequest(PIrp);
    return EMCPAL_STATUS_SUCCESS;

}

/*********************************************************************
 *            fbe_sep_shim_handle_cancel_registration_irp_function ()
 *********************************************************************
 *
 *  Description: cancel routine to return the registration IRP without an actucal event
 *
 *	Inputs: PDeviceObject  - pointer to the device object
 *			PIRP - IRP from top driver
 *
 *  Return Value: None
 *    
 *********************************************************************/
static void fbe_sep_shim_handle_cancel_registration_irp_function(PEMCPAL_DEVICE_OBJECT DeviceObject, PEMCPAL_IRP Irp)
{
    EMCPAL_KIRQL             	CancelIrql;
    os_device_info_t * 	dev_info_p = (os_device_info_t *)EmcpalDeviceGetExtension(DeviceObject);
    
    CancelIrql = EmcpalExtIrpCancelIrqlGet(Irp);

    EmcpalIrpCancelLockRelease (CancelIrql);
    
	if (!fbe_atomic_exchange(&dev_info_p->cancelled, 1)){
        dev_info_p->state_change_notify_ptr = NULL;/*we don't have an IRP to retun since it was canclled.*/
    
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "%s:#:%d IRP:0x%llX\n",__FUNCTION__,dev_info_p->lun_number, (unsigned long long)Irp);

		EmcpalExtIrpStatusFieldSet(Irp, EMCPAL_STATUS_CANCELLED); 
		EmcpalIrpCancelRoutineSet (Irp, NULL);
        EmcpalExtIrpInformationFieldSet(Irp, 0);
        EmcpalIrpCompleteRequest(Irp);
    }
    else{
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "%s:#:%d already in completion, IRP:0x%llX \n",__FUNCTION__,dev_info_p->lun_number, (unsigned long long)Irp);
    }


    return;

}

/*!***************************************************************
 * @fn fbe_sep_shim_enumerate_class(PEMCPAL_IRP PIrp, 
 *                                  fbe_sep_shim_io_struct_t *io_struct,
 *                                  fbe_object_id_t ** obj_array_p, 
 *                                  fbe_u32_t *num_objects_p}
 *****************************************************************
 * @brief
 *   This function returns a list of all objects in the system - the caller must free the memory allocated for the objects
 *
 * @param io_struct         - IO Structure
 * @param obj_array_p       - array object to fill
 * @param num_objects_p     - pointer to actual number of objects
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *  04/09/2012: Vera Wang created
 *
 ****************************************************************/
static fbe_status_t fbe_sep_shim_enumerate_lun_class(fbe_sep_shim_io_struct_t *io_struct,
                                                     fbe_object_id_t ** obj_array_p, 
                                                     fbe_u32_t *num_objects_p)
{
    fbe_status_t                                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                                           alloc_size;
    fbe_topology_control_get_total_objects_of_class_t   total_objects;
    fbe_topology_control_enumerate_class_t              enumerate_class;/*structure is small enough to be on the stack*/
    
    /* one for the entry and one for the NULL termination.
     */
    fbe_sg_element_t *                      sg_list = NULL;
    fbe_sg_element_t *                      temp_sg_list = NULL;

    total_objects.total_objects = 0;
    total_objects.class_id = FBE_CLASS_ID_LUN;

    /* upon completion, the user memory will be filled with the data 
     */
    status = fbe_sep_shim_send_command_to_service_with_sg_list(io_struct,
                                                               FBE_SERVICE_ID_TOPOLOGY,
                                                               FBE_TOPOLOGY_CONTROL_CODE_GET_TOTAL_OBJECTS_OF_CLASS,
                                                               &total_objects, 
                                                               sizeof(fbe_topology_control_get_total_objects_of_class_t),
                                                               NULL,
                                                               0);

    if (status != FBE_STATUS_OK ) {
        fbe_sep_shim_trace (FBE_TRACE_LEVEL_ERROR, "%s:sep_shim_send_command_to_topology failed, can't get total objects of lun class.\n", __FUNCTION__);
        return status;
    }

    *num_objects_p = total_objects.total_objects;
    if ( *num_objects_p == 0 ){
        *obj_array_p  = NULL;
        return status;
    }

    sg_list = fbe_memory_ex_allocate(sizeof(fbe_sg_element_t) * FBE_TOPLOGY_CONTROL_ENUMERATE_CLASS_SG_COUNT);
    if (sg_list == NULL) {
        fbe_sep_shim_trace (FBE_TRACE_LEVEL_ERROR, "%s:out of memory\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    temp_sg_list = sg_list;

    /* Allocate memory for the objects */
    alloc_size = sizeof (**obj_array_p) * *num_objects_p;
    *obj_array_p = fbe_memory_ex_allocate (alloc_size);
    if (*obj_array_p == NULL) {
        fbe_sep_shim_trace (FBE_TRACE_LEVEL_ERROR, "%s:out of memory\n", __FUNCTION__);
        fbe_memory_ex_release(sg_list);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    /*set up the sg list to point to the list of objects the user wants to get*/
    temp_sg_list->address = (fbe_u8_t *)*obj_array_p;
    temp_sg_list->count = alloc_size;
    temp_sg_list ++;
    temp_sg_list->address = NULL;
    temp_sg_list->count = 0;

    enumerate_class.number_of_objects = *num_objects_p;
    enumerate_class.class_id = FBE_CLASS_ID_LUN;
    enumerate_class.number_of_objects_copied = 0;

    fbe_transport_reuse_packet(io_struct->packet);
    /* upon completion, the user memory will be filled with the data
     */
    status = fbe_sep_shim_send_command_to_service_with_sg_list(io_struct,
                                                               FBE_SERVICE_ID_TOPOLOGY,
                                                               FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_CLASS,
                                                               &enumerate_class, 
                                                               sizeof (fbe_topology_control_enumerate_class_t),
                                                               sg_list,
                                                               FBE_TOPLOGY_CONTROL_ENUMERATE_CLASS_SG_COUNT);

    if (status != FBE_STATUS_OK) {
        fbe_sep_shim_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, can't enumerate class.\n", 
                            __FUNCTION__, status);
        fbe_memory_ex_release(sg_list);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ( enumerate_class.number_of_objects_copied < *num_objects_p )
    {
        *num_objects_p = enumerate_class.number_of_objects_copied;
    }
    
    fbe_memory_ex_release(sg_list);
    return FBE_STATUS_OK;

}

/*********************************************************************
 *            fbe_sep_shim_process_get_volume_cache_config ()
 *********************************************************************
 *
 *  Description: used by SP cache when running with system that had no cache
 *
 *	Inputs: PDeviceObject  - pointer to the device object
 *			PIRP - IRP from top driver
 *
 *  Return Value: 
 *	success or failure
 *********************************************************************/
EMCPAL_STATUS fbe_sep_shim_process_get_volume_cache_config(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP PIrp, fbe_sep_shim_io_struct_t *io_struct)
{
    FLARE_GET_VOLUME_CACHE_CONFIG *	cache_config;
    fbe_database_lun_info_t 		fbe_lun_info;
    fbe_status_t 					status;
    os_device_info_t * 				dev_info_p = (os_device_info_t *)EmcpalDeviceGetExtension(PDeviceObject);
    fbe_private_space_layout_lun_info_t psl_lun_info;

    
    cache_config = EmcpalExtIrpSystemBufferGet(PIrp);

    if(cache_config == NULL) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: Input Buffer NULL\n",
                        __FUNCTION__);
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(io_struct);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    fbe_zero_memory(cache_config, sizeof(FLARE_GET_VOLUME_CACHE_CONFIG));

    /* Get the lun object id */
    fbe_block_transport_get_server_id(&dev_info_p->block_edge, &fbe_lun_info.lun_object_id);
    
    status = fbe_sep_shim_send_command_to_database_service(io_struct,
                                                                FBE_DATABASE_CONTROL_CODE_GET_LUN_INFO,
                                                                &fbe_lun_info, 
                                                                sizeof(fbe_database_lun_info_t));

    fbe_sep_shim_return_io_structure(io_struct);
    //clean up context for cancellation
    EmcpalIrpCancelRoutineSet (PIrp, NULL);
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
    
    if(status != FBE_STATUS_OK) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: API call to get info failed:%X\n",
                        __FUNCTION__, status);
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    cache_config->user_capacity = fbe_lun_info.capacity;
    cache_config->internal_capacity = fbe_lun_info.overall_capacity;

    if(fbe_private_space_layout_object_id_is_system_lun(fbe_lun_info.lun_object_id)) {
        status = fbe_private_space_layout_get_lun_by_object_id(fbe_lun_info.lun_object_id, &psl_lun_info);
        if(status == FBE_STATUS_OK) {
            if(psl_lun_info.flags & FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ) {
                /* LU_READ_CACHE_ENABLED | LU_MULTIPLICATIVE_PREFETCH */
                cache_config->unit_cache_params.cache_config_flags |= 0x0001 | 0x0008;
            }
            if(psl_lun_info.flags & FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE) {
                /* LU_WRITE_CACHE_ENABLED */
                cache_config->unit_cache_params.cache_config_flags |= 0x0002;
            }
        }
        else {
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: PSL get_lun_by_object_id failed: 0x%X 0x%X\n",
                               __FUNCTION__, fbe_lun_info.lun_object_id, status);
        }
    }
    else {
        /* LU_READ_CACHE_ENABLED | LU_WRITE_CACHE_ENABLED | LU_MULTIPLICATIVE_PREFETCH*/
        cache_config->unit_cache_params.cache_config_flags |= (0x0001 | 0x0002 | 0x0008);
    }

    cache_config->unit_cache_params.cache_write_aside = 2048/*LU_CACHE_WRITE_ASIDE_BLOCK_COUNT*/;
    cache_config->unit_cache_params.cache_idle_delay = 20/*LU_CACHE_IDLE_DELAY*/;
    cache_config->unit_cache_params.prefetch_disable_length = 1024;
    cache_config->unit_cache_params.prefetch_total_length = 1024;
    cache_config->unit_cache_params.read_retention_priority = 0x3/*LU_HOST_PREFETCH_EQUAL_PRIORITY*/;

    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, sizeof(FLARE_GET_VOLUME_CACHE_CONFIG));
    EmcpalIrpCompleteRequest(PIrp);

    return EMCPAL_STATUS_SUCCESS;

}


/*********************************************************************
 *            fbe_sep_shim_process_disk_is_writable ()
 *********************************************************************
 *
 *  Description: used by upper layered drivers
 *
 *	Inputs: PDeviceObject  - pointer to the device object
 *			PIRP - IRP from top driver
 *
 *  Return Value: 
 *	success or failure
 *********************************************************************/
EMCPAL_STATUS fbe_sep_shim_process_disk_is_writable(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP PIrp, fbe_sep_shim_io_struct_t *io_struct)
{
    fbe_sep_shim_return_io_structure(io_struct);

    //clean up context for cancellation
    EmcpalIrpCancelRoutineSet (PIrp, NULL);
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, 0);
    EmcpalIrpCompleteRequest(PIrp);

    return EMCPAL_STATUS_SUCCESS;
}

static fbe_status_t sep_shim_convert_flun_to_lun(os_device_info_t *dev_info_p, fbe_assigned_wwid_t wwn, fbe_lun_number_t *idm_lun)
{
    INT_32 					request_status;
    IDMSTATUS 				idm_status;
    IDMLOOKUP *				idm_lookup_ptr;
    fbe_status_t 			fbe_status;
    fbe_lun_number_t 		flare_lun = dev_info_p->lun_number;

    /* First check to see this is a user lun.  If not there
       will be no associated IDM id.*/
    if (flare_lun >= PLATFORM_HOST_ACCESSIBLE_FLARE_LUNS){
        *idm_lun = flare_lun;
        return FBE_STATUS_OK;
    }

    /*unfortunatelly, we have no context to open IDM because it must start only after PSM
    that starts after us, way after us. This means, that just like Flare, we have to open IDM
    on the context of the first IOCTL which is ugly and risky but since FCT is the first one sending
    IOs to us in a loop, this should work*/
    if (idmDevicePtr == NULL) {
        fbe_status = fbe_sep_shim_kernel_open_idm();

        // Failed to open IDM
        if (fbe_status != FBE_STATUS_OK) {

            fbe_sep_shim_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s: Failed to open IDM device.\n",__FUNCTION__);

            return fbe_status;
        }

        // IDM open
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "%s: IDM first open:ptr: 0x%llX\n",__FUNCTION__, (unsigned long long)idmDevicePtr);
    }
   
    /* Fill in IDM lookup information*/
    idm_lookup_ptr = &dev_info_p->idm_lookup;
    
    fbe_copy_memory(&idm_lookup_ptr->wwid, wwn.bytes, FBE_WWN_BYTES);
    idm_lookup_ptr->status = IDM_NO_STATUS;

    /* Build a request to be issued to the IDM layer, this will fill up dev_info_p->idm_request
     */
    fbe_status = sep_shim_build_idm_request(&dev_info_p->idm_request,
                                            IOCTL_IDM_LOOK_UP_BY_WWN,
                                            (fbe_u64_t)idm_lookup_ptr,
                                            sizeof(IDMLOOKUP),
                                            BufferTypeOut);

    if (fbe_status != FBE_STATUS_OK){
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: can't build request for FLUN:%d\n",__FUNCTION__, flare_lun);
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "..........with wwn: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x \n",
                           wwn.bytes[0],
                           wwn.bytes[1],
                           wwn.bytes[2],
                           wwn.bytes[3],
                           wwn.bytes[4],
                           wwn.bytes[5],
                           wwn.bytes[6],
                           wwn.bytes[7],
                           wwn.bytes[8],
                           wwn.bytes[9],
                           wwn.bytes[10],
                           wwn.bytes[11],
                           wwn.bytes[12],
                           wwn.bytes[13],
                           wwn.bytes[14],
                           wwn.bytes[15]);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Issue a synchronous request to the IDM layer
     */
    request_status = sep_shim_send_idm_ioctl(&dev_info_p->idm_request);
    if (request_status == EMCPAL_STATUS_PENDING){
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to idmControl for wwn: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", 
                           __FUNCTION__, 
                           wwn.bytes[0],
                           wwn.bytes[1],
                           wwn.bytes[2],
                           wwn.bytes[3],
                           wwn.bytes[4],
                           wwn.bytes[5],
                           wwn.bytes[6],
                           wwn.bytes[7],
                           wwn.bytes[8],
                           wwn.bytes[9],
                           wwn.bytes[10],
                           wwn.bytes[11],
                           wwn.bytes[12],
                           wwn.bytes[13],
                           wwn.bytes[14],
                           wwn.bytes[15]);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    idm_status = idm_lookup_ptr->status;

    if ((request_status == 0) &&
        (idm_status == IDM_LOOKUP_BY_WWN_SUCCESS)){
        /* Good status was returned by the IDM driver
         */
        *idm_lun = idm_lookup_ptr->lunid;
        fbe_status = FBE_STATUS_OK;
    }else{
        /* Could not get the IDM id for the given WWN*/
        fbe_status = FBE_STATUS_GENERIC_FAILURE;
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s:Failed: request_status:%d, idm_status:%d\n",__FUNCTION__, request_status, idm_status);
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "......FLUN:%d with wwn: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", 
                           flare_lun,
                           wwn.bytes[0],
                           wwn.bytes[1],
                           wwn.bytes[2],
                           wwn.bytes[3],
                           wwn.bytes[4],
                           wwn.bytes[5],
                           wwn.bytes[6],
                           wwn.bytes[7],
                           wwn.bytes[8],
                           wwn.bytes[9],
                           wwn.bytes[10],
                           wwn.bytes[11],
                           wwn.bytes[12],
                           wwn.bytes[13],
                           wwn.bytes[14],
                           wwn.bytes[15]);
    }

    return fbe_status;
}

static fbe_status_t fbe_sep_shim_kernel_open_idm(void)
{
    EMCPAL_STATUS 			status;
    
    status = EmcpalExtDeviceOpen(IdmDeviceString,
                                 FILE_ALL_ACCESS,
                                 &idmFilePtr,
                                 &idmDevicePtr);

    if (!EMCPAL_IS_SUCCESS(status)){
      
       fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: can't open IDM\n",__FUNCTION__);
       return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

static fbe_status_t sep_shim_build_idm_request(IdmRequest *request,
                                               fbe_u32_t requestCode,
                                               fbe_u64_t dataBuffer,
                                               fbe_u32_t dataBufferSize,
                                               BufferType dataBufferType)
{
    IdmRequest *idmRequestPtr = request;
    
    idmRequestPtr->bufferPtr = dataBuffer;
    idmRequestPtr->outBufferSize = 0;
    idmRequestPtr->inBufferSize = 0;
    
    switch (dataBufferType)
    {
    case BufferTypeIn:
        /* Buffer is input buffer
         */
        idmRequestPtr->inBufferSize = dataBufferSize;
        break;
    case BufferTypeOut:
        /* Buffer is output buffer 
         */
        idmRequestPtr->outBufferSize = dataBufferSize;
        break;
    default:
       fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: invalid dataBufferType:%d\n",__FUNCTION__, dataBufferType);
       return FBE_STATUS_GENERIC_FAILURE;
    }

    idmRequestPtr->ctlCode = requestCode;
    idmRequestPtr->contextPtr = (fbe_u64_t)NULL;
    idmRequestPtr->msgCode = 0;
    idmRequestPtr->pipeId = -1;

    return FBE_STATUS_OK;
}

static EMCPAL_STATUS sep_shim_send_idm_ioctl(IdmRequest *idmRequestPtr)
{
    EMCPAL_IRP_STATUS_BLOCK ioStatus;
    EMCPAL_RENDEZVOUS_EVENT event;
    EMCPAL_STATUS status;
    PEMCPAL_IRP irpPtr;

    EmcpalRendezvousEventCreate(
                     EmcpalDriverGetCurrentClientObject(),
                     &event,
                     "idmIssueIoctl",
                     FALSE);

    /* Build the IRP, buffer info will be setup below
     */
    irpPtr = EmcpalExtIrpBuildIoctl(idmRequestPtr->ctlCode,
                                           idmDevicePtr,
                                           NULL,
                                           0,
                                           NULL,
                                           0,
                                           FALSE,
                                           &event,
                                           &ioStatus);

    if (irpPtr != NULL)
    {
        PEMCPAL_IO_STACK_LOCATION ioStackPtr;

        EmcpalExtIrpSystemBufferSet(irpPtr, (VOID *)idmRequestPtr->bufferPtr);

        /* Setup stack location for next driver
         */
        ioStackPtr = EmcpalIrpGetNextStackLocation(irpPtr);
        EmcpalExtIrpStackParamIoctlInputSizeSet(ioStackPtr, idmRequestPtr->inBufferSize);
        EmcpalExtIrpStackParamIoctlOutputSizeSet(ioStackPtr, idmRequestPtr->outBufferSize);

        /* Send this IRP down to the IDM driver
         */
        status = EmcpalExtIrpSendAsync(idmDevicePtr, irpPtr);

        if (status == EMCPAL_STATUS_PENDING)
        {
            status = EmcpalRendezvousEventWait(&event, EMCPAL_TIMEOUT_INFINITE_WAIT);

            if (EMCPAL_IS_SUCCESS(status))
            {
                status = EmcpalExtIrpStatusBlockStatusFieldGet(&ioStatus);
            }                             
        }
    }
    else
    {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR,"%s: Could not allocate IRP for IOCTL\n", __FUNCTION__);
        status = EMCPAL_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Set status for this request
     */
    idmRequestPtr->requestStatus = status;

    EmcpalRendezvousEventDestroy(&event);

    return (status);

}
/*!**************************************************************
 * fbe_sep_shim_send_read_pin_index()
 ****************************************************************
 * @brief
 *  Handle the send read and pin index by stashing the index in the LUN.
 *
 * @param PDeviceObject
 * @param PIrp
 * @param io_struct
 * 
 * @return EMCPAL_STATUS
 *
 * @author
 *  12/3/2013 - Created. Rob Foley
 *
 ****************************************************************/

EMCPAL_STATUS fbe_sep_shim_send_read_pin_index(PEMCPAL_DEVICE_OBJECT PDeviceObject, PEMCPAL_IRP PIrp, fbe_sep_shim_io_struct_t *io_struct)
{
    fbe_status_t status;
    os_device_info_t * dev_info_p = (os_device_info_t *)EmcpalDeviceGetExtension(PDeviceObject);
    fbe_lun_set_read_and_pin_index_t set_read_and_pin_index;
    LUN_INDEX *lun_index_p = NULL;
    lun_index_p = EmcpalExtIrpSystemBufferGet(PIrp);

    if(lun_index_p == NULL) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: Input Buffer NULL\n", __FUNCTION__);
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(io_struct);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "IOCTL_CACHE_READ_AND_PIN_DATA issued to LUN:%d, current attrib:0x%x index: 0x%llx\n",
                       dev_info_p->lun_number, dev_info_p->current_attributes, lun_index_p->Index64);;
    
    set_read_and_pin_index.index = *lun_index_p;
    status = fbe_sep_shim_send_command_to_lun(dev_info_p, 
                                              io_struct,
                                              FBE_LUN_CONTROL_CODE_SET_READ_AND_PIN_INDEX,
                                              &set_read_and_pin_index, 
                                              sizeof(fbe_lun_set_read_and_pin_index_t));

    //clean up context for cancellation
    EmcpalIrpCancelRoutineSet(PIrp, NULL);
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    

    if (status != FBE_STATUS_OK) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: call to send Read Pin index failed:%d\n",
                           __FUNCTION__, status);
        fbe_sep_shim_return_io_structure(io_struct);
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    fbe_sep_shim_return_io_structure(io_struct);
    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, 0);
    EmcpalIrpCompleteRequest(PIrp);

    return EMCPAL_STATUS_SUCCESS;
}
/******************************************
 * end fbe_sep_shim_send_read_pin_index()
 ******************************************/
