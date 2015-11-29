/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_drive_configuration_service_parameters.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the tunable parameters which will control the
 *  behavior of all the physical drive objects.
 *
 * @version
 *   11/09/2012:  Wayne Garrett - Created.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_transport_memory.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_service_manager_interface.h"
#include "fbe_drive_configuration_service_private.h"
#include "../../topology/classes/sas_physical_drive/sas_physical_drive_config.h"  /*TODO: move MS defines to here or common interface. then remove this include.*/


/************************ 
 *  PROTOTYPES
 ************************/
static fbe_status_t fbe_dcs_set_pdo_service_time(fbe_object_id_t pdo_obj, fbe_time_t *set_service_time);



fbe_vendor_page_t fbe_sam_sas_vendor_tbl[] =
{
/*
 *  Samsung SATA FLASH Drive Table
 *
 *  This table is used for Samsung FLASH drive settings.
 *
 *  mask indicates which bits are okay to clear at destination.  any bit on is
 * cleared at destination.
 *
 *   page            offset                               mask                       value
 */
    /**
     * AR624691 and AR621469:
     * We need to override the IT Nexus/IRT timers to 2/4.5 sec respectively for Samsung drives
     */
    {FBE_MS_FIBRE_PG19, 4, 0xFF, 0x07},  /* bytes 4,5 is IT Nexus timeout in msec. set to 2 sec*/
    {FBE_MS_FIBRE_PG19, 5, 0xFF, 0xd0},
    {FBE_MS_FIBRE_PG19, 6, 0xFF, 0x11},  /* bytes 6,7 is IRT timeout in msec. set to 4.5 sec */
    {FBE_MS_FIBRE_PG19, 7, 0xFF, 0x94},

    {FBE_MS_REPORT_PG1C, FBE_PG1C_MRIE_OFFSET, FBE_PG1C_MRIE_MASK, FBE_PG1C_MRIE_BYTE},
};

const fbe_u16_t FBE_MS_MAX_SAM_SAS_TBL_ENTRIES_COUNT = sizeof(fbe_sam_sas_vendor_tbl)/sizeof(fbe_sam_sas_vendor_tbl[0]);


/*
 *  Mode pages that will be retrieved in long 
 */
fbe_u8_t fbe_sas_attr_pages[] = {FBE_MS_FIBRE_PG19, FBE_MS_GEOMETRY_PG04};

const fbe_u8_t FBE_MS_MAX_SAS_ATTR_PAGES_ENTRIES_COUNT = sizeof(fbe_sas_attr_pages)/sizeof(fbe_u8_t);


fbe_vendor_page_t fbe_def_sas_vendor_tbl[] =
{
/*
 *  Default Drive Table
 *
 *  This table is used for default drive settings.  The default drive at this
 * time is the Seagate drive.
 *
 *
 *   page            offset                               mask                       value
 */
   /* WD recommended turning on AWRE and ARRE.  All USD groups
    *  were polled and there were no objections.  Also Seagate's
    *  background testing works more effectively if AWRE and ARRE
    *  are on.  Setting Page 1 Byte 2 masking to don't care for bits
    *  6 and 7 so we will not override the drive default mode page.
    *  Changed in R33 AR 539446.
    */
    /* NOTE, page/byte overrides must come in order, except for page 0,
       which must be last. */
    {FBE_MS_RECOV_PG01, 2, 0x3F, 0x04},
    {FBE_MS_RECOV_PG01, 10, 0xFF, 0x0C},
    {FBE_MS_RECOV_PG01, 11, 0xFF, 0x00},
    {FBE_MS_DISCO_PG02, 2, 0xFF, 0x00},
    {FBE_MS_VERRECOV_PG07, 2, 0xFF, 0x04},
    {FBE_MS_VERRECOV_PG07, 10, 0xFF, 0x0C},
    {FBE_MS_VERRECOV_PG07, 11, 0xFF, 0x00},
    {FBE_MS_CACHING_PG08, 2, 0xFF, 0x10},
    {FBE_MS_CACHING_PG08, 12, 0x0F, 0x00},
    {FBE_MS_CONTROL_PG0A, 3, 0xFF, 0x10},    
    {FBE_MS_CONTROL_PG0A, 10, 0xFF, 0x00}, /* bytes 10-11, High Priority EQ = 600ms. must match PG00 bytes 16-17. in 100ms increments */
    {FBE_MS_CONTROL_PG0A, 11, 0xFF, 0x06}, 
    {FBE_MS_FIBRE_PG19, 2, 0xFF, 0x06},
    {FBE_MS_FIBRE_PG19, 4, 0xFF, 0x07},  /* bytes 4,5 is IT Nexus timeout in msec. set to 2 sec*/
    {FBE_MS_FIBRE_PG19, 5, 0xFF, 0xd0},
    {FBE_MS_FIBRE_PG19, 6, 0xFF, 0x11},  /* bytes 6,7 is IRT timeout in msec. set to 4.5 sec */    
    {FBE_MS_FIBRE_PG19, 7, 0xFF, 0x94},
    {FBE_MS_POWER_PG1A, 3, 0x03, 0x00},
    {FBE_MS_REPORT_PG1C, 2, 0xFF, 0x00},
    {FBE_MS_REPORT_PG1C, 3, 0x0F, 0x04},
    {FBE_MS_REPORT_PG1C, 8, 0xFF, 0x00},
    {FBE_MS_REPORT_PG1C, 9, 0xFF, 0x00},
    {FBE_MS_REPORT_PG1C, 10, 0xFF, 0x00},
    {FBE_MS_REPORT_PG1C, 11, 0xFF, 0x01},
    /* page 0 must be last */
    {FBE_MS_VENDOR_PG00, 16, 0xFF, 0x00}, /* High Priority EQ. byte 16-17. 50ms increments. 0x000C = 600 ms */
    {FBE_MS_VENDOR_PG00, 17, 0xFF, 0x0C},  
    {FBE_MS_VENDOR_PG00, 18, 0xFF, 0x00}, /* Low Priority EQ. byte 18-19.  50ms increments. 0x0006 = 300 ms */
    {FBE_MS_VENDOR_PG00, 19, 0xFF, 0x06},  
};

const fbe_u16_t FBE_MS_MAX_DEF_SAS_TBL_ENTRIES_COUNT = sizeof(fbe_def_sas_vendor_tbl)/sizeof(fbe_def_sas_vendor_tbl[0]);


/*******************************************
 * SAS and NL SAS Drive Mode Page Table    *
 *******************************************/
fbe_vendor_page_t fbe_minimal_sas_vendor_tbl[] =
{
    /*
    *  Drive Table
    *
    *  This table is used for SAS and NL SAS drives with.
    *    SMBT sets the default mode page settings to the values that we want
    *    for operation during the qualification of the drive.  As a result we
    *    don't want to change any values unless a problem develops in the field.
    *
    *   A value of 1 in the mask indicates which bits are okay to change. The bit(s) will be
    *    changed to those in the value parameter
    *
    *  We use the default setting now and will update when it needs.
    *
    *   page                 offset                    mask                    value
    */
    {FBE_MS_FIBRE_PG19, 4, 0xFF, 0x07},  /* bytes 4,5 is IT Nexus timeout in msec. set to 2 sec*/
    {FBE_MS_FIBRE_PG19, 5, 0xFF, 0xd0},
    {FBE_MS_FIBRE_PG19, 6, 0xFF, 0x11},  /* bytes 6,7 is IRT timeout in msec. set to 4.5 sec */
    {FBE_MS_FIBRE_PG19, 7, 0xFF, 0x94},

    {FBE_MS_REPORT_PG1C, FBE_PG1C_MRIE_OFFSET, FBE_PG1C_MRIE_MASK, FBE_PG1C_MRIE_BYTE},
};

const fbe_u16_t FBE_MS_MAX_MINIMAL_SAS_TBL_ENTRIES_COUNT = sizeof(fbe_minimal_sas_vendor_tbl)/sizeof(fbe_minimal_sas_vendor_tbl[0]);


/* Additional Override table which is can be set by fbecli 
*/
fbe_vendor_page_t fbe_cli_mode_page_override_tbl[FBE_MS_MAX_SAS_TBL_ENTRIES_COUNT];
fbe_u16_t fbe_cli_mode_page_override_entries = 0;



/*!**************************************************************
 * @fn fbe_drive_configuration_init_paramters
 ****************************************************************
 * @brief
 *  Initializes the physical drive tunable parameters with
 *  default values.
 * 
 * @return fbe_status_t
 *
 * @author
 *   11/09/2012:  Wayne Garrett - Created.
 *
 ****************************************************************/
void fbe_drive_configuration_init_paramters(void)
{
    fbe_zero_memory(&drive_configuration_service.pdo_params, sizeof(drive_configuration_service.pdo_params));

    drive_configuration_service.pdo_params.control_flags = 
          ( FBE_DCS_HEALTH_CHECK_ENABLED |
            FBE_DCS_REMAP_RETRIES_ENABLED |
            FBE_DCS_REMAP_HC_FOR_NON_RETRYABLE |
            //FBE_DCS_FWDL_FAIL_AFTER_MAX_RETRIES |
            FBE_DCS_ENFORCE_ENHANCED_QUEUING_DRIVES |
            //FBE_DCS_DRIVE_LOCKUP_RECOVERY | /*removed because agressive resets were causing issues with raid*/
            FBE_DCS_DRIVE_TIMEOUT_QUIESCE_HANDLING |
            FBE_DCS_PFA_HANDLING |
            FBE_DCS_MODE_SELECT_CAPABILITY |
            // FBE_DCS_BREAK_UP_FW_IMAGE |
            FBE_DCS_4K_ENABLED |
            //FBE_DCS_IGNORE_INVALID_IDENTITY | /*By default we want this unset. This could be set with fbecli dcssrvc command*/
            0
          );    

    drive_configuration_service.pdo_params.service_time_limit_ms          = FBE_DCS_DEFAULT_SERVICE_TIME_MS;
    drive_configuration_service.pdo_params.remap_service_time_limit_ms    = FBE_DCS_DEFAULT_REMAP_SERVICE_TIME_MS;
    drive_configuration_service.pdo_params.emeh_service_time_limit_ms     = FBE_DCS_DEFAULT_EMEH_SERVICE_TIME_MS;
    drive_configuration_service.pdo_params.fw_image_chunk_size            = FBE_DCS_DEFAULT_FW_IMAGE_CHUNK_SIZE;
}


/*!**************************************************************
 * @fn fbe_drive_configuration_set_parameters
 ****************************************************************
 * @brief
 *  Sets the physical drive tunable parameters.
 * 
 * @param packet - contains control info
 * 
 * @return fbe_status_t
 * 
 * @note this should only be called for testing or as part of
 *       a customer recovery procedure
 * 
 * @author
 *   11/09/2012:  Wayne Garrett - Created.
 *
 ****************************************************************/
fbe_status_t fbe_drive_configuration_control_set_parameters(fbe_packet_t * packet)
{
    fbe_payload_control_buffer_length_t                 len = 0;
    fbe_payload_ex_t *                                  payload = NULL;
    fbe_payload_control_operation_t *                   control_operation = NULL;
    fbe_dcs_tunable_params_t *                          params = NULL;
    fbe_u32_t                                           object_idx;
    fbe_topology_control_get_physical_drive_objects_t   * get_drive_objects;
    fbe_status_t                                        status = FBE_STATUS_GENERIC_FAILURE;
   
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_dcs_tunable_params_t)){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid buffer_len. expected:%d actual:%d \n", __FUNCTION__, sizeof(fbe_dcs_tunable_params_t), len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &params); 
    if(params == NULL){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: fbe_payload_control_get_buffer fail\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    get_drive_objects = (fbe_topology_control_get_physical_drive_objects_t *)fbe_memory_native_allocate(sizeof(fbe_topology_control_get_physical_drive_objects_t));
    if (get_drive_objects == NULL) {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: failed to allocate memory\n",  __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    /*now let's set the params
    */

    drive_configuration_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                              "DCS set_param: cntrl_flag:0x%x->0x%x, svc:%d->%d, rm_svc:%d->%d, emeh_svc:%d->%d, fw_chnk:%d->%d\n",
                              drive_configuration_service.pdo_params.control_flags, params->control_flags,
                              drive_configuration_service.pdo_params.service_time_limit_ms, params->service_time_limit_ms,
                              drive_configuration_service.pdo_params.remap_service_time_limit_ms, params->remap_service_time_limit_ms,
                              drive_configuration_service.pdo_params.emeh_service_time_limit_ms, params->emeh_service_time_limit_ms,
                              drive_configuration_service.pdo_params.fw_image_chunk_size, params->fw_image_chunk_size);

    /* Modify params passed in if a special code was set */
    if (params->service_time_limit_ms == FBE_DCS_PARAM_USE_DEFAULT){
        params->service_time_limit_ms = FBE_DCS_DEFAULT_SERVICE_TIME_MS;
    }
    else if (params->service_time_limit_ms == FBE_DCS_PARAM_USE_CURRENT)
    {
        params->service_time_limit_ms = drive_configuration_service.pdo_params.service_time_limit_ms;
    }

    if (params->remap_service_time_limit_ms == FBE_DCS_PARAM_USE_DEFAULT){
        params->remap_service_time_limit_ms = FBE_DCS_DEFAULT_REMAP_SERVICE_TIME_MS;
    }
    else if (params->remap_service_time_limit_ms == FBE_DCS_PARAM_USE_CURRENT)
    {
        params->remap_service_time_limit_ms = drive_configuration_service.pdo_params.remap_service_time_limit_ms;
    }

    if (params->emeh_service_time_limit_ms == FBE_DCS_PARAM_USE_DEFAULT){
        params->emeh_service_time_limit_ms = FBE_DCS_DEFAULT_EMEH_SERVICE_TIME_MS;
    }
    else if (params->emeh_service_time_limit_ms == FBE_DCS_PARAM_USE_CURRENT)
    {
        params->emeh_service_time_limit_ms = drive_configuration_service.pdo_params.emeh_service_time_limit_ms;
    }

    drive_configuration_service.pdo_params = *params;



    /* Notify all PDOs about instance-specific parameters. Note, someday this might be based
       on drive type */

    status = fbe_drive_configuration_get_physical_drive_objects(get_drive_objects); // get all PDOs

    if (FBE_STATUS_OK != status)
    {    
            drive_configuration_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: get_physial_drive_objects failed. status:%d\n",  __FUNCTION__, status);

            fbe_memory_native_release(get_drive_objects);

            fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
    }

    drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: number_of_objects=%d to check if qualified\n",  __FUNCTION__, get_drive_objects->number_of_objects);

    /* For each object, set the instance specific service time */
    for (object_idx = 0; object_idx < get_drive_objects->number_of_objects; object_idx++)
    {
        fbe_dcs_set_pdo_service_time(get_drive_objects->pdo_list[object_idx], &params->service_time_limit_ms);
    }
    
    fbe_memory_native_release(get_drive_objects);

    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;

}

/*!**************************************************************
 * @fn fbe_drive_configuration_control_get_mode_page_overrides
 ****************************************************************
 * @brief
 *  Return the mode select override table.
 * 
 * @param packet - contains control info
 * 
 * @return fbe_status_t
 *
 * @author
 *   03/29/2013:  Wayne Garrett - Created.
 *
 ****************************************************************/
fbe_status_t fbe_drive_configuration_control_get_mode_page_overrides(fbe_packet_t * packet)
{
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t *                      payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_dcs_mode_select_override_info_t *   return_info_p = NULL;
    fbe_vendor_page_t *                     override_table_p;
    fbe_u16_t                               entries;
    fbe_u16_t                               i;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_dcs_mode_select_override_info_t)){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid buffer_len. expected:%d actual:%d \n", __FUNCTION__, sizeof(fbe_dcs_mode_select_override_info_t), len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &return_info_p); 
    if(return_info_p == NULL){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: fbe_payload_control_get_buffer fail\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Copy override table into control operation return value 
    */

    switch(return_info_p->table_id)
    {
        case FBE_DCS_MP_OVERRIDE_TABLE_ID_DEFAULT:
            override_table_p = fbe_def_sas_vendor_tbl;
            entries = FBE_MS_MAX_DEF_SAS_TBL_ENTRIES_COUNT;
            break;
    
        case FBE_DCS_MP_OVERRIDE_TABLE_ID_SAMSUNG:
            override_table_p = fbe_sam_sas_vendor_tbl;
            entries = FBE_MS_MAX_SAM_SAS_TBL_ENTRIES_COUNT;
            break;
    
        case FBE_DCS_MP_OVERRIDE_TABLE_ID_MINIMAL:
            override_table_p = fbe_minimal_sas_vendor_tbl;
            entries = FBE_MS_MAX_MINIMAL_SAS_TBL_ENTRIES_COUNT;
            break;
    
        case FBE_DCS_MP_OVERRIDE_TABLE_ID_ADDITIONAL:
            override_table_p = fbe_cli_mode_page_override_tbl;
            entries = fbe_cli_mode_page_override_entries;
            break;
    
        default:
            drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s: Invalid table ID %d\n", __FUNCTION__, return_info_p->table_id);
    
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    return_info_p->num_entries = entries;
    for (i=0; i<entries; i++)
    {
        return_info_p->table[i].page = override_table_p->page;
        return_info_p->table[i].byte_offset = override_table_p->offset;
        return_info_p->table[i].mask = override_table_p->mask;
        return_info_p->table[i].value = override_table_p->value;
        override_table_p++;
    }

    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_drive_configuration_control_set_mode_page_byte
 ****************************************************************
 * @brief
 *  Modify a mode page byte in the override table.  Note, the
 *  mode page setting will not take affect until PDO transitions
 *  through the activate state, such as done by a reset.
 * 
 * @param packet - contains control info
 * 
 * @return fbe_status_t
 *
 * @author
 *   03/29/2013:  Wayne Garrett - Created.
 *
 ****************************************************************/
fbe_status_t fbe_drive_configuration_control_set_mode_page_byte(fbe_packet_t * packet)
{
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_dcs_set_mode_page_override_table_entry_t *  mode_page_byte_p = NULL;
    fbe_vendor_page_t *                 override_table_p;
    fbe_u16_t                           entries;
    fbe_u16_t                           i;
    fbe_bool_t                          is_found = FBE_FALSE;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_dcs_set_mode_page_override_table_entry_t)){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid buffer_len. expected:%d actual:%d \n", __FUNCTION__, sizeof(fbe_dcs_set_mode_page_override_table_entry_t), len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &mode_page_byte_p); 
    if(mode_page_byte_p == NULL){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: fbe_payload_control_get_buffer fail\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Modify the override table */

    switch(mode_page_byte_p->table_id)
    {
        case FBE_DCS_MP_OVERRIDE_TABLE_ID_DEFAULT:
            override_table_p = fbe_def_sas_vendor_tbl;
            entries = FBE_MS_MAX_DEF_SAS_TBL_ENTRIES_COUNT;
            break;

        case FBE_DCS_MP_OVERRIDE_TABLE_ID_SAMSUNG:
            override_table_p = fbe_sam_sas_vendor_tbl;
            entries = FBE_MS_MAX_SAM_SAS_TBL_ENTRIES_COUNT;
            break;

        case FBE_DCS_MP_OVERRIDE_TABLE_ID_MINIMAL:
            override_table_p = fbe_minimal_sas_vendor_tbl;
            entries = FBE_MS_MAX_MINIMAL_SAS_TBL_ENTRIES_COUNT;
            break;

        case FBE_DCS_MP_OVERRIDE_TABLE_ID_ADDITIONAL:
            override_table_p = fbe_cli_mode_page_override_tbl;
            entries = fbe_cli_mode_page_override_entries;
            break;

        default:
            drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s: Invalid table ID %d\n", __FUNCTION__, mode_page_byte_p->table_id);

            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* For the Additional Override table,  it's expected to be empty at first and caller will add to it.
       For the other tables, which are already populated at system init, it's expected that caller will 
       override these values.  This other tables are fixed-size and cannot grow.  
       So we will handle the Additional override table differently. */

    if (FBE_DCS_MP_OVERRIDE_TABLE_ID_ADDITIONAL == mode_page_byte_p->table_id)
    {
        if (fbe_cli_mode_page_override_entries >= FBE_MS_MAX_SAS_TBL_ENTRIES_COUNT)
        {
            drive_configuration_trace(FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s: table full. max entries=%d\n", __FUNCTION__, FBE_MS_MAX_SAS_TBL_ENTRIES_COUNT);
    
            fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
    
        fbe_cli_mode_page_override_tbl[fbe_cli_mode_page_override_entries].page = mode_page_byte_p->table_entry.page;
        fbe_cli_mode_page_override_tbl[fbe_cli_mode_page_override_entries].offset = mode_page_byte_p->table_entry.byte_offset;
        fbe_cli_mode_page_override_tbl[fbe_cli_mode_page_override_entries].mask = mode_page_byte_p->table_entry.mask;
        fbe_cli_mode_page_override_tbl[fbe_cli_mode_page_override_entries].value = mode_page_byte_p->table_entry.value;
        fbe_cli_mode_page_override_entries++;
    }
    else
    {
        /* overridding existing values 
        */
        for (i=0; i<entries; i++)
        {
            if (override_table_p[i].page == mode_page_byte_p->table_entry.page)
            {
                if (override_table_p[i].offset == mode_page_byte_p->table_entry.byte_offset)
                {
                    override_table_p[i].mask = mode_page_byte_p->table_entry.mask;
                    override_table_p[i].value = mode_page_byte_p->table_entry.value;
                    is_found = FBE_TRUE;
                    break;
                }
            }
        }
    }

    if (is_found)
    {
        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    else
    {
        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn fbe_drive_configuration_control_mp_addl_override_clear
 ****************************************************************
 * @brief
 *  Clear the additional override table.  Note, the
 *  mode page setting will not take affect until PDO is told to do
 *  a mode select.
 * 
 * @param packet - contains control info
 * 
 * @return fbe_status_t
 *
 * @author
 *   05/22/2014:  Wayne Garrett - Created.
 *
 ****************************************************************/
fbe_status_t fbe_drive_configuration_control_mp_addl_override_clear(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    fbe_cli_mode_page_override_entries = 0;


    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_drive_configuration_get_parameters
 ****************************************************************
 * @brief
 *  Get the physical drive tunable parameters.
 * 
 * @param packet - contains control info
 * 
 * @return fbe_status_t
 *
 * @author
 *   11/09/2012:  Wayne Garrett - Created.
 *
 ****************************************************************/
fbe_status_t fbe_drive_configuration_control_get_parameters(fbe_packet_t * packet)
{
    fbe_payload_control_buffer_length_t   len = 0;
    fbe_payload_ex_t *                    payload = NULL;
    fbe_payload_control_operation_t *     control_operation = NULL;
    fbe_dcs_tunable_params_t *            params = NULL;

    drive_configuration_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_dcs_tunable_params_t)){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid buffer_len. expected:%d actual:%d \n", __FUNCTION__, sizeof(fbe_dcs_tunable_params_t), len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &params); 
    if(params == NULL){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: fbe_payload_control_get_buffer fail\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*now let's copy the params*/

    *params = drive_configuration_service.pdo_params;

    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;

}


/*!**************************************************************
 * @fn fbe_dcs_set_pdo_service_time
 ****************************************************************
 * @brief
 *  Set the PDO instance-specific service time.
 * 
 * @param pdo_obj - Object ID for PDO
 * @param set_service_time - service time to set.
 * 
 * @return fbe_status_t
 *
 * @author
 *   11/09/2012:  Wayne Garrett - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_dcs_set_pdo_service_time(fbe_object_id_t pdo_obj, fbe_time_t *set_service_time )
{
    fbe_status_t                                            status;
    fbe_packet_t *                                          packet_p = NULL;
    fbe_payload_ex_t *                                      payload_p = NULL;
    fbe_payload_control_operation_t *                       control_p = NULL;

    /* Allocate packet.
     */
    packet_p = fbe_transport_allocate_packet();
    if (packet_p == NULL) 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_packet(packet_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_allocate_control_operation(payload_p);

    fbe_payload_control_build_operation(control_p,
                                        FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_SERVICE_TIME,
                                        set_service_time,
                                        sizeof(fbe_physical_drive_control_set_service_time_t));
    /* Set packet address.
     */
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              pdo_obj);
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_NO_ATTRIB);
    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    fbe_service_manager_send_control_packet(packet_p);

    /* Wait for completion.
     * The packet is always completed so the status above need not be checked.
     */
    fbe_transport_wait_completion(packet_p);

    /* Save status before we release packet.
     */
    status = fbe_transport_get_status_code(packet_p);

    if (FBE_STATUS_OK != status)
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: Failed. status:%d\n",  __FUNCTION__, status);
    }

    fbe_transport_release_packet(packet_p);

    return status;
}


/*!*************************************************************************
* @fn fbe_dcs_param_is_enabled()
*
* @brief    Return true if a DCS tunable parameter is enabled.  If caller
*           passes more than one flag, then function will test that all
*           flags are enabled.
*
* @return   fbe_bool_t
*
* @author   11/08/2012  Wayne Garrett  - Created.
*
***************************************************************************/
fbe_bool_t fbe_dcs_param_is_enabled(fbe_dcs_control_flags_t flag)
{
    return ((drive_configuration_service.pdo_params.control_flags & flag) == flag)? FBE_TRUE : FBE_FALSE;
}


/*!*************************************************************************
* @fn fbe_dcs_is_healthcheck_enabled()
*
* @brief    Return true if healthcheck drive action is enabled.
*
* @return   fbe_bool_t
*
* @author   11/08/2012  Wayne Garrett  - Created.
*
***************************************************************************/
fbe_bool_t fbe_dcs_is_healthcheck_enabled(void)
{
    return (drive_configuration_service.pdo_params.control_flags & FBE_DCS_HEALTH_CHECK_ENABLED)? FBE_TRUE : FBE_FALSE;
}

/*!*************************************************************************
* @fn fbe_dcs_is_remap_retries_enabled()
*
* @brief    Return true if retries on failed remaps is enabled.
*
* @return   fbe_bool_t
*
* @author   11/08/2012  Wayne Garrett  - Created.
*
***************************************************************************/
fbe_bool_t fbe_dcs_is_remap_retries_enabled(void)
{
    return (drive_configuration_service.pdo_params.control_flags & FBE_DCS_REMAP_RETRIES_ENABLED)? FBE_TRUE : FBE_FALSE;
}


/*!*************************************************************************
* @fn fbe_dcs_use_hc_for_remap_non_retryable()
*
* @brief    Returns true if health check recovery action is used for
*           remap handling when a non-retryable error is being returned.
*           This depends on on health check being enabled.
*
* @return   fbe_bool_t
*
* @author   11/08/2012  Wayne Garrett  - Created.
*
***************************************************************************/
fbe_bool_t fbe_dcs_use_hc_for_remap_non_retryable(void)
{
    if (fbe_dcs_is_healthcheck_enabled())
    {
        return (drive_configuration_service.pdo_params.control_flags & FBE_DCS_REMAP_HC_FOR_NON_RETRYABLE)? FBE_TRUE : FBE_FALSE;
    }

    return FBE_FALSE;
}

/*!*************************************************************************
* @fn fbe_dcs_get_svc_time_limit()
*
* @brief    Returns the service time limit in ms.
*
* @return   fbe_time_t - milliseconds
*
* @author   11/08/2012  Wayne Garrett  - Created.
*
***************************************************************************/
fbe_time_t fbe_dcs_get_svc_time_limit(void)
{
    return drive_configuration_service.pdo_params.service_time_limit_ms;
}


/*!*************************************************************************
* @fn fbe_dcs_get_remap_svc_time_limit()
*
* @brief    Returns the remap service time limit in ms.
*
* @return   fbe_time_t - milliseconds
*
* @author   11/08/2012  Wayne Garrett  - Created.
*
***************************************************************************/
fbe_time_t fbe_dcs_get_remap_svc_time_limit(void)
{
    return drive_configuration_service.pdo_params.remap_service_time_limit_ms;
}

/*!*************************************************************************
* @fn fbe_dcs_get_emeh_svc_time_limit()
*
* @brief    Returns service time limit for drive in a degraded RG.
*
* @return   fbe_time_t - milliseconds
*
* @author   6/18/2015  Wayne Garrett  - Created.
*
***************************************************************************/
fbe_time_t fbe_dcs_get_emeh_svc_time_limit(void)
{
    return drive_configuration_service.pdo_params.emeh_service_time_limit_ms;
}


/*!*************************************************************************
* @fn fbe_dcs_get_fw_image_chunk_size()
*
* @brief    Returns the sg element chunk size for breaking up fw image.
*
* @return   fbe_u32_t - chunk size in bytes
*
* @author   07/24/2013  Wayne Garrett  - Created.
*
***************************************************************************/
fbe_u32_t fbe_dcs_get_fw_image_chunk_size(void)
{
    return drive_configuration_service.pdo_params.fw_image_chunk_size;
}
