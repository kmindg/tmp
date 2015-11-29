/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_virtual_drive_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_virtual_drive interface for the storage extent package.
 *
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_virtual_drive_interface
 *
 * @version
 *   11/05/2009:  Created. guov
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_spare.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_physical_drive.h"

/**************************************
                Local variables
**************************************/
#define FBE_API_SET_PS_TRIGGER_TIMEOUT 30000

/*******************************************
                Local functions
********************************************/


/*!***************************************************************
 * @fn fbe_api_virtual_drive_get_spare_info(
 *       fbe_object_id_t vd_object_id, 
 *       fbe_spare_drive_info_t * spare_drive_info_p)
 ****************************************************************
 * @brief
 *  This function get spare_info with the given vd.
 *
 * @param vd_object_id - vd object ID
 * @param spare_drive_info_p - pointer to the spare drive info
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/05/09 - Created. guov
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_virtual_drive_get_spare_info(fbe_object_id_t vd_object_id, 
                                     fbe_spare_drive_info_t * spare_drive_info_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    if (spare_drive_info_p == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    spare_drive_info_p->drive_type = FBE_DRIVE_TYPE_INVALID;
    spare_drive_info_p->port_number = FBE_PORT_NUMBER_INVALID;

    status = fbe_api_common_send_control_packet (FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_SPARE_INFO,
                                                 spare_drive_info_p,
                                                 sizeof(fbe_spare_drive_info_t),
                                                 vd_object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/**************************************
 * end fbe_api_virtual_drive_get_spare_info()
 **************************************/


/*!***************************************************************
 * @fn fbe_api_virtual_drive_calculate_capacity(
 *        fbe_api_virtual_drive_calculate_capacity_info_t * capacity_info_p)
 ****************************************************************
 * @brief
 *  This function calculates capacity information based on input values.
 *
 * @param capacity_info_p - pointer to the vd capacity info structure
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/20/09 - Created. Peter Puhov
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_virtual_drive_calculate_capacity(fbe_api_virtual_drive_calculate_capacity_info_t * capacity_info_p)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_virtual_drive_control_class_calculate_capacity_t calculate_capacity;

    calculate_capacity.imported_capacity = capacity_info_p->imported_capacity;
    calculate_capacity.block_edge_geometry = capacity_info_p->block_edge_geometry;
    calculate_capacity.exported_capacity = 0;

    status = fbe_api_common_send_control_packet_to_class (FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_CALCULATE_CAPACITY,
                                                          &calculate_capacity,
                                                          sizeof(fbe_virtual_drive_control_class_calculate_capacity_t),
                                                          FBE_CLASS_ID_VIRTUAL_DRIVE,
                                                          FBE_PACKET_FLAG_NO_ATTRIB,
                                                          &status_info,
                                                          FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    capacity_info_p->exported_capacity = calculate_capacity.exported_capacity;
    return status;
}

/*!***************************************************************
 * @fn fbe_api_virtual_drive_get_configuration(
 *          fbe_api_virtual_drive_get_configuration_t * get_configuration_p)
 ****************************************************************
 * @brief
 *  This function is used to get the configuration of the virtual
 *  drive bject.
 *
 * @param configuration_mode_p - Pointer to the configuration mode.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/20/09 - Created. Peter Puhov
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_virtual_drive_get_configuration(fbe_object_id_t vd_object_id,
                                        fbe_api_virtual_drive_get_configuration_t * get_configuration_p)
{
    fbe_status_t                                                status;
    fbe_api_control_operation_status_info_t                     status_info;
    fbe_virtual_drive_configuration_t                       get_configuration;

    get_configuration.configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;
    status = fbe_api_common_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_CONFIGURATION,
                                                &get_configuration,
                                                sizeof(fbe_virtual_drive_configuration_t),
                                                vd_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    get_configuration_p->configuration_mode = get_configuration.configuration_mode;
    return status;
}

/*!***************************************************************
 * @fn fbe_api_virtual_drive_get_unused_drive_as_spare_flag(fbe_object_id_t vd_object_id,
 *                                         fbe_bool_t * get_unsued_drive_as_spare_flag_p)
 ****************************************************************
 * @brief
 *  This function is used to get the unused_drive_as_spare flag of virtual
 *  drive bject.
 *
 * @param get_unsued_drive_as_spare_flag_p - Pointer to flag.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  05/05/11 - Created. Vishnu Sharma
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_virtual_drive_get_unused_drive_as_spare_flag(fbe_object_id_t vd_object_id,
                                        fbe_bool_t * get_unsued_drive_as_spare_flag_p)
{
    fbe_status_t                                                status;
    fbe_api_control_operation_status_info_t                     status_info;
    fbe_api_virtual_drive_unused_drive_as_spare_flag_t          get_unused_drive_as_spare_flag;

    get_unused_drive_as_spare_flag.unused_drive_as_spare_flag = 0;
    
    status = fbe_api_common_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_UNUSED_DRIVE_AS_SPARE_FLAG,
                                                &get_unused_drive_as_spare_flag,
                                                sizeof(fbe_api_virtual_drive_unused_drive_as_spare_flag_t),
                                                vd_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *get_unsued_drive_as_spare_flag_p = get_unused_drive_as_spare_flag.unused_drive_as_spare_flag;
    return status;
}

/*!***************************************************************
 * @fn fbe_api_virtual_drive_set_unused_drive_as_spare_flag(fbe_object_id_t vd_object_id,
 *                                         fbe_bool_t set_unsued_drive_as_spare_flag)
 ****************************************************************
 * @brief
 *  This function is used to set the unused_drive_as_spare flag of virtual
 *  drive bject.
 *
 * @param set_unsued_drive_as_spare_flag - flag.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  05/05/11 - Created. Vishnu Sharma
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_virtual_drive_set_unused_drive_as_spare_flag(fbe_object_id_t vd_object_id,
                                        fbe_bool_t set_unsued_drive_as_spare_flag)
{
    fbe_status_t                                                status;
    fbe_api_control_operation_status_info_t                     status_info;
    fbe_api_virtual_drive_unused_drive_as_spare_flag_t          set_unused_drive_as_spare_flag;

    set_unused_drive_as_spare_flag.unused_drive_as_spare_flag = set_unsued_drive_as_spare_flag;
    
    status = fbe_api_common_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_SET_UNUSED_DRIVE_AS_SPARE_FLAG,
                                                &set_unused_drive_as_spare_flag,
                                                sizeof(fbe_api_virtual_drive_unused_drive_as_spare_flag_t),
                                                vd_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_virtual_drive_get_permanent_spare_trigger_time(
 *   fbe_api_virtual_drive_get_permanent_spare_trigger_time_t * spare_config_info)
 ****************************************************************
 * @brief
 *  This function gets the permanent spare trigger time in secs
 *  from virtual driver class
 *
 * @param spare_config_info - pointer to the permanent spare trigger time structure
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  10/5/2011 - Created. Harshal Wanjari
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_virtual_drive_get_permanent_spare_trigger_time(fbe_api_virtual_drive_permanent_spare_trigger_time_t * spare_config_info)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_virtual_drive_control_class_get_spare_config_t spare_config;

    status = fbe_api_common_send_control_packet_to_class (FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_GET_SPARE_CONFIG,
                                                          &spare_config,
                                                          sizeof(fbe_virtual_drive_control_class_get_spare_config_t),
                                                          FBE_CLASS_ID_VIRTUAL_DRIVE,
                                                          FBE_PACKET_FLAG_NO_ATTRIB,
                                                          &status_info,
                                                          FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:0x%x, packet qualifier:0x%x, payload error:0x%x, payload qualifier:0x%x\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    spare_config_info->permanent_spare_trigger_time= spare_config.system_spare_config_info.permanent_spare_trigger_time;
    return status;
}
/*************************************************************
 * end fbe_api_virtual_drive_get_permanent_spare_trigger_time
 *************************************************************/
/*!***************************************************************
 * @fn fbe_api_virtual_drive_class_set_unused_as_spare_flag()
 ****************************************************************
 * @brief
 *  This function is used to set the unused drive as spare flag to false 
 *  for Virtual Drive CLASS. This API needs to be called when you would like
 *  to configure VD&PVD as hot spare. This API needs to be called BEFORE
 *  a VD is created so it's good to call it in a sytem level before any RG
 *  are created. If the VD was already created, you will need to use the
 *  fbe_api_virtual_drive_set_unused_drive_as_spare_flag API to explicityly set 
 *  a specific VD.
 *
 * @param None.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/14/12 - Created. Vera Wang
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_virtual_drive_class_set_unused_as_spare_flag(fbe_bool_t enable)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_virtual_drive_control_class_set_unused_drive_as_spare_t set_flag;

    set_flag.enable = enable;

    status = fbe_api_common_send_control_packet_to_class(FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_SET_UNUSED_AS_SPARE_FLAG,
                                                &set_flag,
                                                sizeof(fbe_virtual_drive_control_class_set_unused_drive_as_spare_t),
                                                FBE_CLASS_ID_VIRTUAL_DRIVE,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                NULL,
                                                FBE_PACKAGE_ID_SEP_0);

     if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d.\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}


/*!***************************************************************************
 * fbe_api_virtual_drive_set_all_virtual_drive_debug_flags()
 *****************************************************************************
 *
 * @brief   This method sends the control_code passed to ALL the virtual drive
 *          objects:
 *              o FBE_CLASS_ID_VIRTUAL_DRIVE
 *
 * @param   control_code - virtual drive control code to send
 * @param   in_debug_flags - Debug flags to set
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/15/2010  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t FBE_API_CALL
fbe_api_virtual_drive_set_all_virtual_drive_debug_flags(fbe_virtual_drive_control_code_t control_code,
                                                        fbe_u32_t in_debug_flags)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_u32_t                               set_debug_flags = in_debug_flags;

    /* Validate that control structures are a single enum.
     */
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_virtual_drive_debug_payload_t) == sizeof(fbe_u32_t));

    /* Validate that the control code is supported.
     */
    switch (control_code)
    {
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_SET_DEBUG_FLAGS:
            break;

        default:
            /* Unsupported control code.
             */
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                         "%s: Unsupported control code: 0x%x \n",
                          __FUNCTION__, control_code);
            return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* First send the class control code to the raid group class.  This changes
     * the default debug flags for any new raid groups created.
     */
    status = fbe_api_common_send_control_packet_to_class(control_code,
                                                         &set_debug_flags,
                                                         sizeof(fbe_u32_t),
                                                         /* There is no rg class
                                                          * instances so we need to send 
                                                          * it to one of the leaf 
                                                          * classes. 
                                                          */
                                                         FBE_CLASS_ID_VIRTUAL_DRIVE,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         &status_info,
                                                         FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                       "%s: line: %d packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__, __LINE__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Always return the execution status.
     */
    return(status);
}
/***************************************************************
 * end fbe_api_virtual_drive_set_all_virtual_drive_debug_flags()
 ***************************************************************/

/*!***************************************************************************
 * fbe_api_virtual_drive_set_debug_flags()       
 *****************************************************************************
 *
 * @brief   This method set the virtual drive debug flags (which are included
 *          in the raid group object) to the value specified.  If the raid group
 *          object id is FBE_OBJECT_ID_INVALID, it indicates that the flags
 *          should be set for ALL classes that inherit the raid group object.
 *
 * @param   vd_object_id - FBE_OBJECT_ID_INVALID - Set the virtual drive debug
 *                                  flags for all virtual drive objects.
 *                         Other - Set the virtual drive debug flags for the
 *                                  virtual drive specified.
 * @param   virtual_drive_debug_flags - Value to set raid group debug flags to. 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/15/2010  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_virtual_drive_set_debug_flags(fbe_object_id_t vd_object_id, 
                                      fbe_virtual_drive_debug_flags_t virtual_drive_debug_flags)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_virtual_drive_debug_payload_t       set_debug_flags; 

    /* Populate the control structure.
     */
    set_debug_flags.virtual_drive_debug_flags = virtual_drive_debug_flags;

    /* If the object id is invalid it indicates that we should set default and
     * the current raid library debug flags for ALL raid groups to the value
     * indicated.
     */
    if (vd_object_id == FBE_OBJECT_ID_INVALID)
    {
        /* Change the raid library debug flags for ALL raid groups.  In addition any new
         * raid groups created will also use the new raid library debug flags.
         */
        status = fbe_api_virtual_drive_set_all_virtual_drive_debug_flags(FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_SET_DEBUG_FLAGS,
                                                                         (fbe_u32_t)virtual_drive_debug_flags);
    }
    else
    {
        /* Else set the raid library debug flags for the raid group specified.
         */
        status = fbe_api_common_send_control_packet (FBE_VIRTUAL_DRIVE_CONTROL_CODE_SET_DEBUG_FLAGS,
                                                     &set_debug_flags,
                                                     sizeof(fbe_virtual_drive_debug_payload_t),
                                                     vd_object_id,
                                                     FBE_PACKET_FLAG_TRAVERSE,
                                                     &status_info,
                                                     FBE_PACKAGE_ID_SEP_0);
        if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
        {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                          "%s: line: %d packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                          __FUNCTION__, __LINE__,
                          status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Always return the execution status.
     */
    return(status);
}
/************************************************
 * end fbe_api_virtual_drive_set_debug_flags()
 ************************************************/

/*!***************************************************************************
 * fbe_api_control_automatic_hot_spare(fbe_bool_t enable)       
 *****************************************************************************
 *
 * @brief   This method sets up the system to enable or disbale the hot spare feature
 *
 * @param   enable - if FBE_TRUE is passed, the system will automatically swap in HS based no policy
 *                   if FBE_FALSE is passed, the system will need drives to be marked as hot spare to swap them
 *   	 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_control_automatic_hot_spare(fbe_bool_t enable)
{
    fbe_status_t 		status;
    fbe_u32_t			virtual_drive_count;
    fbe_object_id_t	*	object_array = NULL;

    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s: Need to turn AHS %s\n", __FUNCTION__, (enable == FBE_TRUE) ? "On" : "Off" );

    /*let's tell the topology service what to do based on the flag*/
    status  = fbe_api_provision_drive_set_config_hs_flag(!enable);
    if (status != FBE_STATUS_OK) {
         fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to change toplogy HS selection flag\n", __FUNCTION__);
         return status;
    }

    /*now, let's turn off all future VDs that will be created by controlling the class level*/
    status = fbe_api_virtual_drive_class_set_unused_as_spare_flag(enable);
    if (status != FBE_STATUS_OK) {
         fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to change VD class level HS selection flag\n", __FUNCTION__);
         return status;
    }

    /*and now let's do all the existing VD, we have to call them explicitly*/
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_VIRTUAL_DRIVE, FBE_PACKAGE_ID_SEP_0, &virtual_drive_count );
    if (status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to get total VDs in the system\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (virtual_drive_count)
    {
        status = fbe_api_enumerate_class(FBE_CLASS_ID_VIRTUAL_DRIVE, FBE_PACKAGE_ID_SEP_0, &object_array, &virtual_drive_count);
        if (status != FBE_STATUS_OK) {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to get a list of VDs\n", __FUNCTION__);
            if(object_array != NULL)
            {
                fbe_api_free_memory(object_array);
            }
            return FBE_STATUS_GENERIC_FAILURE;
        }
    
        for (;virtual_drive_count > 0; virtual_drive_count--) {
            status = fbe_api_virtual_drive_set_unused_drive_as_spare_flag(object_array[virtual_drive_count - 1], enable);
            if (status != FBE_STATUS_OK) {
                fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to set vd ID 0x%X unused_drive_as_spare to:%s\n", 
                              __FUNCTION__, object_array[virtual_drive_count - 1], enable ? "True" : "False");
            }
    
        }

        fbe_api_free_memory(object_array);
    }
    return FBE_STATUS_OK;

}
/************************************************
 * end fbe_api_virtual_drive_set_debug_flags()
 ************************************************/

/*!***************************************************************************
 * fbe_api_update_permanent_spare_swap_in_time(fbe_u64_t swap_in_time_in_sec)      
 *****************************************************************************
 *
 * @brief   This method changes the default 5 minues it takes for a permanent spare to swap in.
 *          if you wish to completly disable auto swap, don't just usea big time.
 *          but rather call fbe_api_control_automatic_hot_spare
 *
 * @param   swap_in_time_in_sec - How long it seconds does it take to swap in a spare
 *   	 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_update_permanent_spare_swap_in_time(fbe_u64_t swap_in_time_in_sec)
{
    fbe_status_t 										status;
    fbe_api_job_service_update_permanent_spare_timer_t  update_permanent_spare_timer;
    fbe_job_service_error_type_t                		job_error_code;
    fbe_status_t                                		job_status;

    /* Cannot set permanent spare swap time to 0 seconds !!
     */
    if (swap_in_time_in_sec == 0)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: invalid swap in time: 0x%llx\n", 
                      __FUNCTION__, (unsigned long long)swap_in_time_in_sec);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* configure the permanent spare trigger timer. */
    update_permanent_spare_timer.permanent_spare_trigger_timer = swap_in_time_in_sec;

    /* update the permanent spare trigger timer. */
    status = fbe_api_job_service_set_permanent_spare_trigger_timer(&update_permanent_spare_timer);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s:failed to start job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
     fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s: wait for job: 0x%llx\n", 
                      __FUNCTION__, update_permanent_spare_timer.job_number);
    /* We have submitted the job and received a job number. 
     * Now wait for this job to complete and then check the status. 
     */
    status = fbe_api_common_wait_for_job(update_permanent_spare_timer.job_number,
                                         FBE_API_SET_PS_TRIGGER_TIMEOUT,
                                         &job_error_code,
                                         &job_status,
                                         NULL);

    if (status == FBE_STATUS_TIMEOUT) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s:timed out after %d msec waiting for job\n", __FUNCTION__, FBE_API_SET_PS_TRIGGER_TIMEOUT);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (job_error_code != FBE_JOB_SERVICE_ERROR_NO_ERROR ||
        job_status != FBE_STATUS_OK ||
        status != FBE_STATUS_OK) {

        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s:failed to change permanent spare swap in time\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
/************************************************
 * end fbe_api_update_permanent_spare_swap_in_time()
 ************************************************/

/*!***************************************************************
 * @fn fbe_api_virtual_drive_get_info(
 *        fbe_object_id_t vd_object_id, fbe_api_virtual_drive_server_info_t* vd_check_point_p)
 ****************************************************************
 * @brief
 *  This function get virtual drive information
 *
 * @param vd_get_info_p - pointer to the vd drive info structure
 *        vd_object_id - Vd object id.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  10/16/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_virtual_drive_get_info(fbe_object_id_t vd_object_id, fbe_api_virtual_drive_get_info_t *vd_get_info_p)
{
    fbe_status_t                            status;
    fbe_api_control_operation_status_info_t status_info;
    fbe_virtual_drive_get_info_t            virtual_drive_info;

    fbe_zero_memory(&virtual_drive_info, sizeof(fbe_virtual_drive_get_info_t));
    status = fbe_api_common_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_INFO,
                                                &virtual_drive_info,
                                                sizeof(fbe_virtual_drive_get_info_t),
                                                vd_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* copy from result to requested structure*/
    vd_get_info_p->vd_checkpoint = virtual_drive_info.vd_checkpoint;
    vd_get_info_p->configuration_mode = virtual_drive_info.configuration_mode;
    vd_get_info_p->swap_edge_index = virtual_drive_info.swap_edge_index;
    vd_get_info_p->original_pvd_object_id = virtual_drive_info.original_pvd_object_id;
    vd_get_info_p->spare_pvd_object_id = virtual_drive_info.spare_pvd_object_id;
    vd_get_info_p->b_request_in_progress = virtual_drive_info.b_request_in_progress;
    vd_get_info_p->b_is_copy_complete = virtual_drive_info.b_is_copy_complete;

    return status;
}
/************************************************
 * end fbe_api_virtual_drive_get_info()
 ************************************************/

/*!***************************************************************
 * @fn fbe_api_virtual_drive_get_checkpoint(
 *        fbe_object_id_t vd_object_id, fbe_api_virtual_drive_server_info_t* vd_check_point_p)
 ****************************************************************
 * @brief
 *  This function get vd check point.
 *
 * @param vd_check_point_p - pointer to the vd drive server info structure
 *        vd_object_id - Vd object id.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  7/11/2012 - Created. Harshal Wanjari
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_virtual_drive_get_checkpoint(fbe_object_id_t vd_object_id, fbe_lba_t *vd_check_point_p)
{
    fbe_status_t                        status;
    fbe_api_virtual_drive_get_info_t    vd_info;

    /* Initialize return to 0.
     */
    *vd_check_point_p = 0;

    /* Get the virtual drive info.
     */
    status = fbe_api_virtual_drive_get_info(vd_object_id, &vd_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                       "%s: get vd info for vd obj: 0x%x failed - status: 0x%x\n", 
                       __FUNCTION__, vd_object_id, status);
    }
    else
    {
        *vd_check_point_p = vd_info.vd_checkpoint;
    }

    return status;
}
/************************************************
 * end fbe_api_virtual_drive_get_checkpoint()
 ************************************************/

/*!***************************************************************************
 * @fn fbe_api_virtual_drive_get_performance_tier(
 *      fbe_api_virtual_drive_get_performance_tier_t *get_performance_tier_p)
 *   
 ***************************************************************************** 
 * 
 * @brief   Get the performance tier information.
 *
 * @param   api_get_performance_tier_p - Pointer to get performance tier request
 *
 * @return  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  07/07/2015  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_virtual_drive_get_performance_tier(fbe_api_virtual_drive_get_performance_tier_t *api_get_performance_tier_p)
{
    fbe_status_t                                        status;
    fbe_api_control_operation_status_info_t             status_info;
    fbe_virtual_drive_control_get_performance_tier_t    get_performance_tier;
    fbe_u32_t                                           drive_type_index;

    fbe_zero_memory(api_get_performance_tier_p, sizeof(*api_get_performance_tier_p));
    fbe_zero_memory(&get_performance_tier, sizeof(fbe_virtual_drive_control_get_performance_tier_t));
    status = fbe_api_common_send_control_packet_to_class (FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_GET_PERFORMANCE_TIER,
                                                          &get_performance_tier,
                                                          sizeof(fbe_virtual_drive_control_get_performance_tier_t),
                                                          FBE_CLASS_ID_VIRTUAL_DRIVE,
                                                          FBE_PACKET_FLAG_NO_ATTRIB,
                                                          &status_info,
                                                          FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:0x%x, packet qualifier:0x%x, payload error:0x%x, payload qualifier:0x%x\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Copy the results to the API structure.
     */
    api_get_performance_tier_p->last_valid_drive_type_plus_one = get_performance_tier.drive_type_last;
    for (drive_type_index = 0; drive_type_index < get_performance_tier.drive_type_last; drive_type_index++)
    {
        api_get_performance_tier_p->performance_tier[drive_type_index] = get_performance_tier.performance_tier[drive_type_index];
    }
    return status;
}
/*************************************************************
 * end fbe_api_virtual_drive_get_performance_tier()
 *************************************************************/

/*********************************************
 * end file fbe_api_virtual_drive_interface.c
 ********************************************/
