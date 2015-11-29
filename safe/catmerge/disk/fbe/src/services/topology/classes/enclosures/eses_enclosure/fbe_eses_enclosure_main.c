/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_eses_enclosure_main.c
 ***************************************************************************
 *
 *  @brief
 *  This file contains the utility and initialization functions for maintaining
 *  the eses enclosure.
 *
 * @ingroup fbe_enclosure_class
 * 
 * HISTORY
 *   8/21/2008:  Created - bphilbin
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"
#include "fbe_transport_memory.h"
#include "fbe_eses_enclosure_private.h"
#include "fbe_eses_enclosure_config.h"
#include "edal_eses_enclosure_data.h"

/*
 * Local function prototypes.
 */

/* Export class methods  */
fbe_class_methods_t fbe_eses_enclosure_class_methods = {FBE_CLASS_ID_ESES_ENCLOSURE,
                                                        fbe_eses_enclosure_load,
                                                        fbe_eses_enclosure_unload,
                                                        fbe_eses_enclosure_create_object,
                                                        fbe_eses_enclosure_destroy_object,
                                                        fbe_eses_enclosure_control_entry,
                                                        fbe_eses_enclosure_event_entry,
                                                        fbe_eses_enclosure_io_entry,
                                                        fbe_eses_enclosure_monitor_entry};

/*!*************************************************************************
 * @fn fbe_eses_enclosure_init(fbe_eses_enclosure_t *eses_enclosure)
 ***************************************************************************
 *
 * @brief
 *  This function initializes the eses enclosure.  
 *
 * @param eses_enclosure - Pointer to our object context
 *
 * @return  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  8/21/08 - created. bphilbin
 *
 ****************************************************************/
fbe_status_t
fbe_eses_enclosure_init(fbe_eses_enclosure_t *eses_enclosure)
{
    fbe_zero_memory(eses_enclosure->encl_serial_number, FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE+1);
    // Now copy in a default value other than 0.  This should never be
    // passed up to the client unless enclosure object fails to read
    // the serial number form the expander.
    fbe_copy_memory(eses_enclosure->encl_serial_number, FBE_ESES_ENCL_SN_DEFAULT, sizeof(FBE_ESES_ENCL_SN_DEFAULT));
    eses_enclosure->day = 0;
    eses_enclosure->mode_select_needed = 0;
    eses_enclosure->test_mode_rqstd = FBE_ESES_ENCLOSURE_TEST_MODE_DEFAULT; // Set the test mode to the default value.
    eses_enclosure->test_mode_status = FBE_ESES_ENCLOSURE_TEST_MODE_DEFAULT; // Set the test mode to the default value.
    eses_enclosure->outstanding_write = 0;
    eses_enclosure->emc_encl_ctrl_outstanding_write = 0;

    fbe_eses_enclosure_set_outstanding_scsi_request_opcode(eses_enclosure, FBE_ESES_CTRL_OPCODE_INVALID);
    eses_enclosure->outstanding_scsi_request_count = 0;
    eses_enclosure->outstanding_scsi_request_max = FBE_ESES_ENCLOSURE_OUTSTANDING_SCSI_REQUEST_MAX;

    eses_enclosure->enclosureConfigBeingUpdated = FALSE;
    eses_enclosure->inform_fw_activating = FALSE;
    eses_enclosure->active_state_to_update_cond = 0;
    eses_enclosure->update_lifecycle_cond = 0;

    fbe_eses_enclosure_set_number_of_command_retry_max(eses_enclosure, MAX_CMD_RETRY);
    fbe_eses_enclosure_set_number_of_power_supply_subelem(eses_enclosure, 0);
    fbe_eses_enclosure_set_number_of_power_supplies_per_side(eses_enclosure, 0);
    fbe_eses_enclosure_set_ps_resume_format_special(eses_enclosure, 0);
    fbe_eses_enclosure_set_ps_overall_elem_saved(eses_enclosure, 0);

    eses_enclosure->mode_sense_retry_cnt = 0;
    eses_enclosure->reset_reason = 0;
    eses_enclosure->poll_count = 0;
    eses_enclosure->reset_shutdown_timer = FALSE;
    fbe_eses_enclosure_init_enclCurrentFupInfo(eses_enclosure);
    // initialize spin lock for enclosure Read/Status data
    fbe_spinlock_init(&eses_enclosure->enclGetInfoLock);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_eses_enclosure_load(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_ESES_ENCLOSURE,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "EsesEncl:%s entry\n", __FUNCTION__);
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_eses_enclosure_t) < FBE_MEMORY_CHUNK_SIZE);
    fbe_topology_class_trace(FBE_CLASS_ID_ESES_ENCLOSURE,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "EsesEncl:%s object size %llu", __FUNCTION__, (unsigned long long)sizeof(fbe_eses_enclosure_t));

    return fbe_eses_enclosure_monitor_load_verify();
}

fbe_status_t 
fbe_eses_enclosure_unload(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_ESES_ENCLOSURE,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "EsesEncl:%s entry\n", __FUNCTION__);
    return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_create_object(                  
*                    fbe_packet_t * packet, 
*                    fbe_object_handle_t object_handle)                  
***************************************************************************
*
* @brief
*       This is the constructor for the ESES enclosure class.
*
* @param      packet - The pointer to the fbe_packet_t.
* @param      object_handle - object handle.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   10-Oct-2008 NC - Added header.
*
***************************************************************************/
fbe_status_t 
fbe_eses_enclosure_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
    fbe_eses_enclosure_t * eses_enclosure;
    fbe_status_t status;

    fbe_topology_class_trace(FBE_CLASS_ID_ESES_ENCLOSURE,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "EsesEncl:%s entry\n", __FUNCTION__);

    /* Call parent constructor */
    status = fbe_base_enclosure_create_object(packet, object_handle);
    if(status != FBE_STATUS_OK){
        return status;
    }

    /* Set class id */
    eses_enclosure = (fbe_eses_enclosure_t *)fbe_base_handle_to_pointer(*object_handle);
    fbe_base_object_set_class_id((fbe_base_object_t *) eses_enclosure, FBE_CLASS_ID_ESES_ENCLOSURE);  
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, enclosure created successfully\n", 
                             __FUNCTION__  );

    /* Initialize the eses enclosure attibutes. */
    fbe_eses_enclosure_init(eses_enclosure);

    status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                    (fbe_base_object_t*)eses_enclosure, 
                                    FBE_SAS_ENCLOSURE_LIFECYCLE_COND_INQUIRY);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, can't set INQUIRY condition, status: 0x%x.\n",
                                __FUNCTION__, status);
    }

    return status;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_destroy_object(                  
*                    fbe_object_handle_t object_handle)                  
***************************************************************************
*
* @brief
*       This is the destructor for the ESES enclosure class.
*
* @param      object_handle - object handle.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   10-Oct-2008 NC - Added header.
*
***************************************************************************/
fbe_status_t 
fbe_eses_enclosure_destroy_object( fbe_object_handle_t object_handle)
{
    fbe_status_t status;
    fbe_eses_enclosure_t * eses_enclosure;

    eses_enclosure = (fbe_eses_enclosure_t *)fbe_base_handle_to_pointer(object_handle);
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "%s entry \n", __FUNCTION__ );

    fbe_spinlock_destroy(&eses_enclosure->enclGetInfoLock); /* SAFEBUG - needs destroy */

    /* no memory release is needed, as memory are allocated and destroyed from viper enclosure */
    if (eses_enclosure->enclCurrentFupInfo.enclFupImagePointer != NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "%s Releasing enclFupImagePointer %p.\n", __FUNCTION__,
                           eses_enclosure->enclCurrentFupInfo.enclFupImagePointer);

        fbe_memory_ex_release(eses_enclosure->enclCurrentFupInfo.enclFupImagePointer);

        eses_enclosure->enclCurrentFupInfo.enclFupImagePointer = NULL;
    }


    /* Call parent destructor */
    status = fbe_sas_enclosure_destroy_object(object_handle);
    return status;
}


/*!*************************************************************************
 *  @fn fbe_eses_enclosure_init_enclCurrentFupInfo(fbe_eses_enclosure_t * fbe_status_t)
 **************************************************************************
 *  @brief
 *      This function initializes the CURRENT firmware upgrade operation info.
 *
 *  @param    pEsesEncl - 
 *
 *  @return   fbe_status_t
 *
 *  @note
 *
 *  HISTORY:
 *    05-Jan-2011: PHE - Created.
 *
 **************************************************************************/
fbe_status_t fbe_eses_enclosure_init_enclCurrentFupInfo(fbe_eses_enclosure_t * pEsesEncl)
{
    pEsesEncl->enclCurrentFupInfo.enclFupImagePointer = NULL;
    pEsesEncl->enclCurrentFupInfo.enclFupImageSize = 0;
    pEsesEncl->enclCurrentFupInfo.enclFupComponent = FBE_FW_TARGET_INVALID;
    pEsesEncl->enclCurrentFupInfo.enclFupBytesTransferred = 0;
    pEsesEncl->enclCurrentFupInfo.enclFupOperationRetryCnt = 0;
    pEsesEncl->enclCurrentFupInfo.enclFupOperation = FBE_ENCLOSURE_FIRMWARE_OP_NONE;
    pEsesEncl->enclCurrentFupInfo.enclFupUseTunnelling = FALSE;
    pEsesEncl->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_NONE;
    pEsesEncl->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_NONE;
    pEsesEncl->enclCurrentFupInfo.enclFupCurrTransferSize = 0;
    pEsesEncl->enclCurrentFupInfo.enclFupComponentSide = FBE_ESES_SUBENCL_SIDE_ID_INVALID;
    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_eses_enclosure_init_enclFwInfo(fbe_eses_enclosure_t * pEsesEncl)
 **************************************************************************
 *  @brief
 *      This function initialize the enclosure fup info for all the devices. 
 *
 *  @param    pEsesEncl - 
 *
 *  @return   fbe_status_t
 *
 *  @note It should be called after the enclosure is specialized because the memory
 *        is allocated after the enclosure is specialized.
 *        This function needs to be revisited if fbe_eses_enclosure_get_fw_target_addr
 *        gets updated.
 *
 *  HISTORY:
 *    05-Jan-2011: PHE - Created.
 *
 **************************************************************************/
fbe_status_t fbe_eses_enclosure_init_enclFwInfo(fbe_eses_enclosure_t * pEsesEncl)
{
    fbe_u32_t index = 0;

    fbe_zero_memory(pEsesEncl->enclFwInfo, sizeof(fbe_eses_encl_fw_info_t));

    for(index = 0; index < FBE_FW_TARGET_MAX; index ++)
    {
        switch(index)
        {
            case FBE_FW_TARGET_LCC_EXPANDER:
            case FBE_FW_TARGET_LCC_BOOT_LOADER:
            case FBE_FW_TARGET_LCC_INIT_STRING:
            case FBE_FW_TARGET_LCC_FPGA:
            case FBE_FW_TARGET_LCC_MAIN:
                pEsesEncl->enclFwInfo->subencl_fw_info[index].fwPriority = FBE_ESES_ENCLOSURE_FW_PRIORITY_HIGH;
                break;

            default:
                pEsesEncl->enclFwInfo->subencl_fw_info[index].fwPriority = FBE_ESES_ENCLOSURE_FW_PRIORITY_LOW;
                break;
        }
    }
  
    return FBE_STATUS_OK;
}


