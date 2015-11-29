/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_sas_magnum_enclosure_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains monitor functions for sas magnum enclosure.
 *
 * @ingroup fbe_enclosure_class
 *
 * HISTORY
 *   10/13/2008:  Created file header. NC
 *   03/02/2009:  Created from fbe_sas_viper_enclosure_main.c
 *
 ***************************************************************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"
#include "fbe_transport_memory.h"
#include "fbe_eses_enclosure_config.h"
#include "sas_magnum_enclosure_private.h"
#include "edal_eses_enclosure_data.h"

/* Export class methods  */
fbe_class_methods_t fbe_sas_magnum_enclosure_class_methods = {FBE_CLASS_ID_SAS_MAGNUM_ENCLOSURE,
                                                        fbe_sas_magnum_enclosure_load,
                                                        fbe_sas_magnum_enclosure_unload,
                                                        fbe_sas_magnum_enclosure_create_object,
                                                        fbe_sas_magnum_enclosure_destroy_object,
                                                        fbe_sas_magnum_enclosure_control_entry,
                                                        fbe_sas_magnum_enclosure_event_entry,
                                                        fbe_sas_magnum_enclosure_io_entry,
                                                        fbe_sas_magnum_enclosure_monitor_entry};

/*
 * Initialization data for Magnum components. It has to be in the same order as fbe_enclosure_component_types_t
 */
fbe_u32_t   fbe_sas_magnum_init_data[FBE_SAS_MAGNUM_ENCLOSURE_MAX_COMPONENT_TYPE]=
{
    FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_ENCL,
    FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_LCCS,
    FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_POWER_SUPPLIES,
    FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_COOLING_COMPONENTS,
    FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_SLOTS,
    FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_TEMP_SENSORS,
    FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_CONNECTORS,
    FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_EXPANDERS,
    FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_EXPANDER_PHYS,
    FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_DISPLAYS,
};

fbe_u32_t   fbe_sas_magnum_init_size_data[FBE_SAS_MAGNUM_ENCLOSURE_MAX_COMPONENT_TYPE]=
{
    sizeof(fbe_eses_enclosure_info_t),
    sizeof(fbe_eses_lcc_info_t ),
    sizeof(fbe_eses_power_supply_info_t),
    sizeof(fbe_eses_cooling_info_t),
    sizeof(fbe_eses_encl_drive_info_t),
    sizeof(fbe_eses_temp_sensor_info_t),
    sizeof(fbe_eses_encl_connector_info_t),
    sizeof(fbe_eses_encl_expander_info_t),
    sizeof(fbe_eses_expander_phy_info_t),
    sizeof(fbe_eses_display_info_t),
};

/**************************
 * GLOBALS 
 **************************/

/* PINGHE - hardcoded for early hardware integration. 
 * BRION - hacking in the sizes since it is not worth it to dynamically allocate these for debug code
 */
fbe_u8_t   fbe_sas_magnum_enclosure_drive_slot_to_phy_index[ESES_SUPPORTED_SIDES][FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_SLOTS]=
//0  1  2  3  4  5  6   7   8   9   10  11  12  13  14
{{4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18},
 {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18}};

fbe_u8_t   fbe_sas_magnum_enclosure_local_connector_index_to_phy_index[FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_CONNECTORS_PER_LCC]=
{0xFF, 0, 1, 2, 3};

fbe_u8_t   fbe_sas_magnum_enclosure_phy_index_to_phy_id[FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_EXPANDER_PHYS]=
{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18};


/* Forward declaration */
static fbe_status_t fbe_sas_magnum_enclosure_get_encl_device_details(fbe_sas_magnum_enclosure_t * sas_magnum_enclosure, fbe_packet_t *packet);

static fbe_status_t fbe_sas_magnum_enclosure_get_encl_device_details_completion(fbe_packet_t * sub_packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_sas_magnum_enclosure_get_device_info_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_sas_magnum_enclosure_get_device_info(fbe_sas_magnum_enclosure_t * sas_magnum_enclosure, fbe_packet_t *packet);

static fbe_status_t sas_magnum_enclosure_slot_monitor(fbe_sas_magnum_enclosure_t * sas_magnum_enclosure, fbe_u8_t *resp_buffer);

static fbe_status_t fbe_sas_magnum_enclosure_turn_fault_led_on(fbe_sas_magnum_enclosure_t * sas_magnum_enclosure, fbe_packet_t *packet, fbe_u32_t slot_id);

static fbe_status_t fbe_sas_magnum_enclosure_turn_fault_led_on_completion(fbe_packet_t * sub_packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_ses_build_encl_control_data(fbe_u8_t *cmd, fbe_u32_t cmd_size);

static fbe_status_t fbe_sas_magnum_enclosure_set_drive_fault_led(fbe_sas_magnum_enclosure_t * sas_magnum_enclosure, fbe_packet_t * packet);

static fbe_status_t fbe_sas_magnum_release_encl_data_block(fbe_enclosure_component_block_t   *component_block);

static fbe_status_t
fbe_sas_magnum_fit_component(fbe_enclosure_component_block_t *sas_magnum_component_block,
                            fbe_enclosure_component_types_t componentType,
                            fbe_u32_t number_elements,
                            fbe_u32_t componentTypeSize);

/* fbe_bool_t first_time = TRUE; */
fbe_status_t 
fbe_sas_magnum_enclosure_load(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_SAS_MAGNUM_ENCLOSURE,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "MagnumEncl:%s entry\n", __FUNCTION__);
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_sas_magnum_enclosure_t) < FBE_MEMORY_CHUNK_SIZE);
    fbe_topology_class_trace(FBE_CLASS_ID_SAS_MAGNUM_ENCLOSURE,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "MagnumEncl:%s object size %llu\n", __FUNCTION__, (unsigned long long)sizeof(fbe_sas_magnum_enclosure_t));

    return fbe_sas_magnum_enclosure_monitor_load_verify();
}

fbe_status_t 
fbe_sas_magnum_enclosure_unload(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_SAS_MAGNUM_ENCLOSURE,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "MagnumEncl:%s entry\n", __FUNCTION__);
    return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn fbe_sas_magnum_enclosure_create_object(                  
*                    fbe_packet_t * packet, 
*                    fbe_object_handle_t object_handle)                  
***************************************************************************
*
* @brief
*       This is the constructor for magnum class.
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
*   02-Mar-2009 BP - Created from fbe_sas_viper_enclosure_create_object.
*
***************************************************************************/
fbe_status_t 
fbe_sas_magnum_enclosure_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
    fbe_sas_magnum_enclosure_t * sas_magnum_enclosure;
    fbe_status_t status;

    fbe_topology_class_trace(FBE_CLASS_ID_SAS_MAGNUM_ENCLOSURE,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "MagnumEncl:%s entry\n", __FUNCTION__);

    /* Call parent constructor */
    status = fbe_base_enclosure_create_object(packet, object_handle);
    if(status != FBE_STATUS_OK){
        return status;
    }

    /* Set class id */
    sas_magnum_enclosure = (fbe_sas_magnum_enclosure_t *)fbe_base_handle_to_pointer(*object_handle);
    fbe_base_object_set_class_id((fbe_base_object_t *) sas_magnum_enclosure, FBE_CLASS_ID_SAS_MAGNUM_ENCLOSURE);  
    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_magnum_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_magnum_enclosure),
                            "%s, enclosure created successfully\n", 
                            __FUNCTION__);

    fbe_sas_magnum_enclosure_init(sas_magnum_enclosure);

    return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn fbe_sas_magnum_enclosure_destroy_object(                  
*                    fbe_object_handle_t object_handle)                  
***************************************************************************
*
* @brief
*       This is the destructor for magnum class.
*
* @param      object_handle - object handle.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   10-Oct-2008 NC - Added header.
*   02-Mar-2009 BP - Created from fbe_sas_viper_enclosure_destroy_object.
*
***************************************************************************/
fbe_status_t 
fbe_sas_magnum_enclosure_destroy_object( fbe_object_handle_t object_handle)
{
    fbe_status_t status;
    fbe_sas_magnum_enclosure_t * sas_magnum_enclosure;
    fbe_enclosure_component_block_t   *sas_magnum_component_block;
    fbe_enclosure_component_block_t   *sas_magnum_component_backUp_block;

    sas_magnum_enclosure = (fbe_sas_magnum_enclosure_t *)fbe_base_handle_to_pointer(object_handle);
    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_magnum_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_magnum_enclosure),
                            "%s entry \n", 
                            __FUNCTION__ );

    /* release memory for internal data */
    fbe_base_enclosure_get_component_block_ptr((fbe_base_enclosure_t *)sas_magnum_enclosure, 
                                                &sas_magnum_component_block);
    fbe_sas_magnum_release_encl_data_block(sas_magnum_component_block);
    /* clean up */
    fbe_base_enclosure_set_component_block_ptr((fbe_base_enclosure_t *)sas_magnum_enclosure, NULL);
    /* release memory for BackUp data */
    fbe_base_enclosure_get_component_block_backup_ptr((fbe_base_enclosure_t *)sas_magnum_enclosure, 
                                                &sas_magnum_component_backUp_block);
    fbe_sas_magnum_release_encl_data_block(sas_magnum_component_backUp_block);
    /* clean up */
    fbe_base_enclosure_set_component_block_backup_ptr((fbe_base_enclosure_t *)sas_magnum_enclosure, NULL);

    /* 
     * Element group and element list use one chunk of memory.
     * While releasing the buffer for element group, it will 
     * release the chunk of memory for both element group and element list.
     */  
    if (fbe_eses_enclosure_get_drive_info_ptr((fbe_eses_enclosure_t *)sas_magnum_enclosure))
    {
        fbe_transport_release_buffer(fbe_eses_enclosure_get_drive_info_ptr((fbe_eses_enclosure_t *)sas_magnum_enclosure));
        /* clean up.  All 3 coming off the same block, should be NULL'ed once the block is released */
        fbe_eses_enclosure_set_drive_info_ptr((fbe_eses_enclosure_t *) sas_magnum_enclosure, NULL);    
        fbe_eses_enclosure_set_enclFwInfo_ptr((fbe_eses_enclosure_t *) sas_magnum_enclosure, NULL);
        fbe_eses_enclosure_set_expansion_port_info_ptr((fbe_eses_enclosure_t *) sas_magnum_enclosure, NULL);
    }

    if (fbe_eses_enclosure_get_elem_group_ptr((fbe_eses_enclosure_t *)sas_magnum_enclosure))
    {
        fbe_memory_ex_release(fbe_eses_enclosure_get_elem_group_ptr((fbe_eses_enclosure_t *)sas_magnum_enclosure));
        fbe_eses_enclosure_set_elem_group((fbe_eses_enclosure_t *)sas_magnum_enclosure, NULL);
    }

    /* Call parent destructor */
    status = fbe_eses_enclosure_destroy_object(object_handle);
    return status;
}


/*!*************************************************************************
 *  @fn fbe_sas_magnum_enclosure_init_eses_parameters(
 *                   fbe_sas_magnum_enclosure_t *sas_magnum_enclosure)
 **************************************************************************
 *
 *  @brief
 *      This function will initialize eses class parameters for the magnum
 *      enclosure type
 *
 *  @param    sas_magnum_enclosure - pointer to the Magnum enclosure object
 *
 *  @return   void
 *
 *  NOTES:
 *
 *  HISTORY:
 *    29-Oct-2008: Brion Philbin - Created
 *    02-Mar-2009: Brion Philbin - Created from fbe_sas_viper_enclosure_init_eses_parameters.
 *
 **************************************************************************/
void
fbe_sas_magnum_enclosure_init_eses_parameters(fbe_sas_magnum_enclosure_t *sas_magnum_enclosure)
{
    fbe_eses_enclosure_t *eses_enclosure;

    eses_enclosure = &sas_magnum_enclosure->eses_enclosure;    

    fbe_eses_enclosure_set_drive_slot_to_phy_index_mapping(eses_enclosure, 0, fbe_sas_magnum_enclosure_drive_slot_to_phy_index[0]);
    fbe_eses_enclosure_set_drive_slot_to_phy_index_mapping(eses_enclosure, 1, fbe_sas_magnum_enclosure_drive_slot_to_phy_index[1]);
    fbe_eses_enclosure_set_connector_index_to_phy_index_mapping(eses_enclosure, fbe_sas_magnum_enclosure_local_connector_index_to_phy_index);
    fbe_eses_enclosure_set_phy_index_to_phy_id_mapping(eses_enclosure, fbe_sas_magnum_enclosure_phy_index_to_phy_id);

    fbe_eses_enclosure_set_number_of_comp_types(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_COMP_TYPES);
    fbe_eses_enclosure_set_number_of_power_supplies(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_POWER_SUPPLIES);
    fbe_eses_enclosure_set_number_of_cooling_components(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_COOLING_COMPONENTS);
    fbe_eses_enclosure_set_number_of_cooling_components_per_ps(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_COOLING_COMPONENTS_PER_PS);    
    fbe_eses_enclosure_set_number_of_cooling_components_on_chassis(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_COOLING_COMPONENTS_ON_CHASSIS); 
    fbe_eses_enclosure_set_number_of_cooling_components_external(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_COOLING_COMPONENTS_EXTERNAL); 
    fbe_eses_enclosure_set_number_of_slots(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_SLOTS);
    fbe_eses_enclosure_set_max_encl_slots(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_MAX_ENCL_SLOTS);
    fbe_eses_enclosure_set_number_of_temp_sensors(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_TEMP_SENSORS);
    fbe_eses_enclosure_set_number_of_temp_sensors_per_lcc(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_TEMP_SENSORS_PER_LCC);
    fbe_eses_enclosure_set_number_of_temp_sensors_on_chassis(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_TEMP_SENSORS_ON_CHASSIS);
    fbe_eses_enclosure_set_number_of_lccs_with_temp_sensor(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_LCCS);
    fbe_eses_enclosure_set_number_of_expanders(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_EXPANDERS);
    fbe_eses_enclosure_set_number_of_expanders_per_lcc(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_EXPANDERS_PER_LCC);
    fbe_eses_enclosure_set_number_of_expander_phys(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_EXPANDER_PHYS);
    fbe_eses_enclosure_set_number_of_encl(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_ENCL);
    fbe_eses_enclosure_set_number_of_connectors(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_CONNECTORS);
    fbe_eses_enclosure_set_number_of_connectors_per_lcc(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_CONNECTORS_PER_LCC);
    fbe_eses_enclosure_set_number_of_lanes_per_entire_connector(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_LANES_PER_ENTIRE_CONNECTOR);
    fbe_eses_enclosure_set_number_of_lccs(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_LCCS);
    fbe_eses_enclosure_set_number_of_related_expanders(eses_enclosure, 0);
    fbe_eses_enclosure_set_first_slot_index(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_FIRST_SLOT_INDEX);
    fbe_eses_enclosure_set_number_of_expansion_ports(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_EXPANSION_PORTS);
    fbe_eses_enclosure_set_first_expansion_port_index(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_FIRST_EXPANSION_PORT_INDEX);
    fbe_eses_enclosure_set_number_of_display_chars(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_DISPLAYS);
    fbe_eses_enclosure_set_number_of_two_digit_displays(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_TWO_DIGIT_DISPLAYS);
    fbe_eses_enclosure_set_number_of_one_digit_displays(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_ONE_DIGIT_DISPLAYS);
    fbe_eses_enclosure_set_number_of_actual_elem_groups(eses_enclosure, 0);//will be updated while processing the config page in eses enclosure.
    fbe_eses_enclosure_set_number_of_drive_midplane(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_DRIVE_MIDPLANE);

    fbe_eses_enclosure_set_enclosure_fw_dl_retry_max(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_FW_DL_RETRY_MAX);
    fbe_eses_enclosure_set_fw_num_expander_elements(eses_enclosure, FBE_MAGNUM_FW_NUM_EXPANDER_ELEMENTS);
    fbe_eses_enclosure_set_fw_num_subencl_sides(eses_enclosure, FBE_MAGNUM_FW_NUM_SUBENCL_SIDES);
    fbe_eses_enclosure_set_fw_lcc_version_descs(eses_enclosure, FBE_MAGNUM_FW_LCC_VERSION_DESCS);
    fbe_eses_enclosure_set_fw_ps_version_descs(eses_enclosure, FBE_MAGNUM_FW_PS_VERSION_DESCS);
    fbe_eses_enclosure_set_default_ride_through_period(eses_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_DEFAULT_RIDE_THROUGH_PERIOD);    


    return;
}


/*!*************************************************************************
 * @fn fbe_sas_magnum_format_encl_data
 *           (fbe_enclosure_component_block_t   *sas_magnum_component_block)
 **************************************************************************
 *
 *  @brief
 *      This function will initialize enclosure object component data.
 *
 *  @param    pointer to component block
 *
 *  @return   fbe_status_t
 *
 *  NOTES:
 *
 *  HISTORY:
 *    25-Jul-2008: Joe Perry - Created
 *    02-Mar-2009: Brion Philbin - Created from fbe_sas_viper_format_encl_data
 *
 **************************************************************************/
fbe_status_t
fbe_sas_magnum_format_encl_data(fbe_enclosure_component_block_t   *sas_magnum_component_block)
{
    fbe_u32_t                   componentTypeIndex;
    fbe_u32_t                   componentTypeSize;
    fbe_status_t                status = FBE_STATUS_OK;

    /* Initialize Component Info for a Magnum enclosure */
    // loop through all components and initialize data & allocate locations
    for (componentTypeIndex = 0; 
         componentTypeIndex < FBE_SAS_MAGNUM_ENCLOSURE_MAX_COMPONENT_TYPE; 
         componentTypeIndex++)
    {
        /* calculate the size needed for this type of component */
        componentTypeSize = fbe_sas_magnum_init_data[componentTypeIndex] *
                                fbe_sas_magnum_init_size_data[componentTypeIndex];
        if ((componentTypeSize +            
                (sizeof(fbe_enclosure_component_t) * sas_magnum_component_block->maxNumberComponentTypes) +
                (sizeof(fbe_enclosure_component_block_t))) > FBE_MEMORY_CHUNK_SIZE)
        {
            fbe_topology_class_trace(FBE_CLASS_ID_SAS_MAGNUM_ENCLOSURE,
                                        FBE_TRACE_LEVEL_ERROR, 
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "  MagnumEncl, component index %d need too much memory %d\n",
                                        componentTypeIndex, componentTypeSize);
            return FBE_STATUS_NOT_INITIALIZED;
        }

        status = fbe_sas_magnum_fit_component(sas_magnum_component_block, // block
                                    componentTypeIndex,        // component type
                                    fbe_sas_magnum_init_data[componentTypeIndex], // # of element
                                    fbe_sas_magnum_init_size_data[componentTypeIndex]); // individual size


        if (status != FBE_STATUS_OK)
        {
            fbe_topology_class_trace(FBE_CLASS_ID_SAS_MAGNUM_ENCLOSURE,
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "MagnumEncl:%s, Component Data too large!!!\n",
                                    __FUNCTION__);
            status = FBE_STATUS_FAILED;
            break;
        }
    }   // end of for loop

    if (status == FBE_STATUS_OK)
    {
        fbe_edal_printAllComponentBlockInfo((void *)sas_magnum_component_block);
    }

    return status;

}   // end of fbe_sas_magnum_format_encl_data


/*!*************************************************************************
 * @fn fbe_sas_magnum_enclosure_init
 *              (fbe_sas_magnum_enclosure_t * sas_magnum_enclosure)
 **************************************************************************
 *
 *  @brief
 *      This function will initialize the enclosure object.
 *
 *  @param    sas_magnum_enclosure - pointer to the Magnum enclosure object
 *
 *  @return   fbe_status_t - Status of the initialization.
 *
 *  NOTES:
 *
 *  HISTORY:
 *    25-Jul-2008: Joe Perry - Created function header 
 *                             (function already created)
 *    02-Mar-2009: Brion Philbin - Created from fbe_sas_viper_enclosure_init.
 *
 **************************************************************************/
fbe_status_t 
fbe_sas_magnum_enclosure_init(fbe_sas_magnum_enclosure_t * sas_magnum_enclosure)
{
    fbe_eses_elem_group_t *     elem_group = NULL;
    fbe_eses_encl_fw_info_t *   enclFwInfo = NULL;
    fbe_enclosure_component_block_t   *sas_magnum_component_block;
    fbe_enclosure_component_block_t   *sas_magnum_component_block_backup;
    fbe_enclosure_number_t      enclosure_number = 0;  
    fbe_address_t               server_address;
    fbe_edal_status_t           edalStatus;
    fbe_enclosure_status_t      encl_status;
    fbe_port_number_t           port_number;    
    fbe_eses_enclosure_expansion_port_info_t *expansion_port_info;
    fbe_eses_enclosure_drive_info_t *drive_info;
    fbe_u8_t encl_sn[FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE];
    fbe_u8_t                    enclMaxSlot;

    fbe_sas_magnum_enclosure_init_eses_parameters(sas_magnum_enclosure);


    fbe_base_enclosure_get_enclosure_number((fbe_base_enclosure_t *)sas_magnum_enclosure, &enclosure_number);

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_magnum_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_magnum_enclosure),
                            "%s entry \n", 
                            __FUNCTION__ );

   
    fbe_base_enclosure_set_lcc_device_type((fbe_base_enclosure_t *)sas_magnum_enclosure, FBE_DEVICE_TYPE_SP);
    fbe_base_enclosure_set_enclosure_type((fbe_base_enclosure_t *)sas_magnum_enclosure, FBE_ENCLOSURE_TYPE_SAS);
    fbe_base_enclosure_set_power_save_supported((fbe_base_enclosure_t *)sas_magnum_enclosure, TRUE);
    fbe_base_enclosure_set_number_of_slots((fbe_base_enclosure_t *)sas_magnum_enclosure, FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_SLOTS);
    fbe_base_enclosure_set_max_number_of_drives_spinningup((fbe_base_enclosure_t *)sas_magnum_enclosure, 
                             FBE_SAS_MAGNUM_ENCLOSURE_MAX_NUMBER_OF_DRIVES_SPINNINGUP);
    fbe_sas_enclosure_set_enclosure_type((fbe_sas_enclosure_t *)sas_magnum_enclosure, FBE_SAS_ENCLOSURE_TYPE_MAGNUM);  

     /*
     * Initialize Component Info for a Magnum enclosure
     */    
    sas_magnum_component_block = 
        fbe_base_enclosure_allocate_encl_data_block(FBE_ENCL_SAS_MAGNUM,
                                fbe_base_enclosure_get_component_id((fbe_base_enclosure_t *)sas_magnum_enclosure),
                                FBE_SAS_MAGNUM_ENCLOSURE_MAX_COMPONENT_TYPE);
            
    if (sas_magnum_component_block == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_magnum_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_magnum_enclosure),
                                "%s, could not allocate ComponentDataBuffer \n", 
                                __FUNCTION__ );
        
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* 
    fbe_base_object_trace(
        (fbe_base_object_t *)sas_magnum_enclosure,
        FBE_TRACE_LEVEL_INFO,
        FBE_TRACE_MESSAGE_ID_INFO,
        "MagnumEncl:%s, ComponentDataBuffer for encl 0x%x created (port %d, encl %d\n", 
        __FUNCTION__, 
        sas_magnum_enclosure,
        port_number,
        enclosure_number);
        */
    if (fbe_sas_magnum_format_encl_data(sas_magnum_component_block) != FBE_STATUS_OK)
    {
        fbe_sas_magnum_release_encl_data_block(sas_magnum_component_block);
        return FBE_STATUS_NOT_INITIALIZED;
    }
    fbe_base_enclosure_set_component_block_ptr((fbe_base_enclosure_t *)sas_magnum_enclosure, 
                                                sas_magnum_component_block);
    /*
     * Initialize Component block backup Info for a Magnum enclosure
     */    
    sas_magnum_component_block_backup = 
        fbe_base_enclosure_allocate_encl_data_block(FBE_ENCL_SAS_MAGNUM,
                                fbe_base_enclosure_get_component_id((fbe_base_enclosure_t *)sas_magnum_enclosure),
                                FBE_SAS_MAGNUM_ENCLOSURE_MAX_COMPONENT_TYPE);
            
    if (sas_magnum_component_block_backup == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_magnum_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_magnum_enclosure),
                                "%s, could not allocate ComponentDataBackup \n", 
                                __FUNCTION__ );
        
        /* Release memory for active EDAL also */
        fbe_sas_magnum_release_encl_data_block(sas_magnum_component_block); 
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    if (fbe_sas_magnum_format_encl_data(sas_magnum_component_block_backup) != FBE_STATUS_OK)
    {
        fbe_sas_magnum_release_encl_data_block(sas_magnum_component_block_backup);
        /* Release memory for active EDAL also */
        fbe_sas_magnum_release_encl_data_block(sas_magnum_component_block); 
        return FBE_STATUS_NOT_INITIALIZED;
    }
    fbe_base_enclosure_set_component_block_backup_ptr((fbe_base_enclosure_t *)sas_magnum_enclosure, 
                                                sas_magnum_component_block_backup);
    

    /* We need to store this in enclosure database */
    fbe_base_enclosure_get_server_address((fbe_base_enclosure_t *)sas_magnum_enclosure,
                                       &server_address);
    fbe_base_enclosure_get_port_number((fbe_base_enclosure_t *)sas_magnum_enclosure,
                                       &port_number);

    /* We use sas address off server address */

    edalStatus = fbe_base_enclosure_edal_setU64((fbe_base_enclosure_t *) sas_magnum_enclosure,
                                            FBE_ENCL_SERVER_SAS_ADDRESS,
                                            FBE_ENCL_ENCLOSURE,
                                                0,
                                            server_address.sas_address);

    edalStatus = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *) sas_magnum_enclosure,
                                            FBE_ENCL_PORT_NUMBER,
                                            FBE_ENCL_ENCLOSURE,
                                            0,
                                            (fbe_u8_t)port_number);

    fbe_eses_enclosure_get_serial_number((fbe_eses_enclosure_t *)sas_magnum_enclosure,
                                        FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE,
                                        encl_sn);
    edalStatus = fbe_base_enclosure_edal_setStr((fbe_base_enclosure_t *) sas_magnum_enclosure,
                                            FBE_ENCL_SERIAL_NUMBER,
                                            FBE_ENCL_ENCLOSURE,
                                            0,
                                            FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE,
                                            encl_sn);

    edalStatus = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *) sas_magnum_enclosure,
                                            FBE_ENCL_POSITION,
                                            FBE_ENCL_ENCLOSURE,
                                            0,
                                            enclosure_number);

    edalStatus = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *) sas_magnum_enclosure,
                                            FBE_ENCL_COMP_SIDE_ID,
                                            FBE_ENCL_ENCLOSURE,
                                            0,
                                            FBE_ENCLOSURE_VALUE_INVALID);

    enclMaxSlot = fbe_eses_enclosure_get_max_encl_slots((fbe_eses_enclosure_t *)sas_magnum_enclosure);

    edalStatus = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)sas_magnum_enclosure,
                                            FBE_ENCL_MAX_SLOTS,
                                            FBE_ENCL_ENCLOSURE,
                                            0,
                                            enclMaxSlot);


    // Allocate the buffer for the element group
    // and initialize it to all 0s.
    // Will move to the eses enclose object and do not need to allocate 1K for it.
    FBE_ASSERT_AT_COMPILE_TIME(((FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_SLOTS * sizeof(fbe_eses_enclosure_drive_info_t)) +
                                (FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_EXPANSION_PORTS * sizeof(fbe_eses_enclosure_expansion_port_info_t)) +
                                (sizeof(fbe_eses_encl_fw_info_t)))
                               < FBE_MEMORY_CHUNK_SIZE);
    
    drive_info = (fbe_eses_enclosure_drive_info_t *)fbe_transport_allocate_buffer();

    fbe_eses_enclosure_set_drive_info_ptr((fbe_eses_enclosure_t *) sas_magnum_enclosure, drive_info);
    if(drive_info == NULL){
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_magnum_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_magnum_enclosure),
                            "%s, failed allocating drive info\n",
                            __FUNCTION__);

        fbe_sas_magnum_release_encl_data_block(sas_magnum_component_block);
        fbe_base_enclosure_set_component_block_ptr((fbe_base_enclosure_t *)sas_magnum_enclosure, NULL);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    /* Zero the entire memory chunk. */
    fbe_zero_memory(drive_info, FBE_MEMORY_CHUNK_SIZE);

    fbe_eses_enclosure_init_drive_info((fbe_eses_enclosure_t *) sas_magnum_enclosure);

    expansion_port_info = (fbe_eses_enclosure_expansion_port_info_t *)((fbe_u8_t *)drive_info +
                                        (FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_SLOTS * sizeof(fbe_eses_enclosure_drive_info_t)));
    fbe_eses_enclosure_set_expansion_port_info_ptr((fbe_eses_enclosure_t *) sas_magnum_enclosure, expansion_port_info);
    fbe_eses_enclosure_init_expansion_port_info((fbe_eses_enclosure_t *) sas_magnum_enclosure);

    enclFwInfo = (fbe_eses_encl_fw_info_t *)((fbe_u8_t *)drive_info +
                                        (FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_SLOTS * sizeof(fbe_eses_enclosure_drive_info_t)) +
                                        (FBE_SAS_MAGNUM_ENCLOSURE_NUMBER_OF_EXPANSION_PORTS * sizeof(fbe_eses_enclosure_expansion_port_info_t)));

    fbe_eses_enclosure_set_enclFwInfo_ptr((fbe_eses_enclosure_t *) sas_magnum_enclosure, enclFwInfo);
    fbe_eses_enclosure_init_enclFwInfo((fbe_eses_enclosure_t *) sas_magnum_enclosure);

    if((encl_status = fbe_eses_enclosure_init_config_info((fbe_eses_enclosure_t *)sas_magnum_enclosure)) != FBE_ENCLOSURE_STATUS_OK)
    {
#if TRUE
        fbe_edal_printAllComponentData((void *)sas_magnum_component_block, fbe_edal_trace_func);
#endif
        // Fail to initialize the configuration and mapping info. 
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_magnum_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_magnum_enclosure),
                                "%s,failed to init config and mapping info, encl_status: 0x%x.\n", 
                                __FUNCTION__, encl_status);

        fbe_sas_magnum_release_encl_data_block(sas_magnum_component_block);
        fbe_base_enclosure_set_component_block_ptr((fbe_base_enclosure_t *)sas_magnum_enclosure, NULL);
        fbe_transport_release_buffer(elem_group);
        /* clean up.  All 3 coming off the same block, should be NULL'ed once the block is released */
        fbe_eses_enclosure_set_elem_group((fbe_eses_enclosure_t *)sas_magnum_enclosure, NULL);
        fbe_eses_enclosure_set_drive_info_ptr((fbe_eses_enclosure_t *) sas_magnum_enclosure, NULL);    
        fbe_eses_enclosure_set_enclFwInfo_ptr((fbe_eses_enclosure_t *) sas_magnum_enclosure, NULL);
        return FBE_STATUS_NOT_INITIALIZED;
    };   

#if FALSE
    fbe_edal_printAllComponentData((void *)sas_magnum_component_block, fbe_edal_trace_func);
#endif

    fbe_base_enclosure_set_default_ride_through_period(
        (fbe_base_enclosure_t *)sas_magnum_enclosure, 
        FBE_SAS_MAGNUM_ENCLOSURE_DEFAULT_RIDE_THROUGH_PERIOD);

    return FBE_STATUS_OK;
}


/*!*************************************************************************
 * @fn fbe_sas_magnum_fit_component
 **************************************************************************
 *
 *  @brief
 *      This function tries to fit a component into a block, if it can't fit,
 *  it will try to allocate a new block.
 *
 *  @param    sas_magnum_component_block - pointer to first block
 *  @param    componentType             - component type
 *  @param    number_elements           - number of element for this component type
 *  @param    componentTypeSize         - individual size
 *
 *  @return   fbe_status_t - Status of the fit.
 *
 *  NOTES:
 *
 *  HISTORY:
 *    18-Dec-2008: NC - Created
 *    02-Mar-2009: BP - Created from fbe_sas_viper_fit_component.
 *
 **************************************************************************/
static fbe_status_t
fbe_sas_magnum_fit_component(fbe_enclosure_component_block_t *sas_magnum_component_block,
                            fbe_enclosure_component_types_t componentType,
                            fbe_u32_t number_elements,
                            fbe_u32_t componentTypeSize)
{
    fbe_enclosure_component_block_t *local_block;
    fbe_status_t status;
    fbe_edal_status_t edal_status;
    fbe_u32_t totalSize;
    fbe_u8_t edalLocale = 0;
    
    edal_status = fbe_edal_getEdalLocale(sas_magnum_component_block, &edalLocale);

    totalSize = number_elements * componentTypeSize;

    edal_status = fbe_edal_fit_component(sas_magnum_component_block,
                                         componentType,
                                         number_elements,
                                         componentTypeSize);

    /* if it doesn't fit, we need to create a new block,
     * appended to edal, and try again.
     */
    if (edal_status != FBE_EDAL_STATUS_OK)
    {
        local_block = 
            fbe_base_enclosure_allocate_encl_data_block(FBE_ENCL_SAS_MAGNUM,
                                                edalLocale,
                                                FBE_SAS_MAGNUM_ENCLOSURE_MAX_COMPONENT_TYPE);
        fbe_edal_append_block(sas_magnum_component_block, local_block);
        /* Let's try again */
        edal_status = fbe_edal_fit_component(sas_magnum_component_block,
                                            componentType,
                                            number_elements,
                                            componentTypeSize);
    }

    /* in case the latest allocation failed */
    if (edal_status == FBE_EDAL_STATUS_OK)
    {
        status = FBE_STATUS_OK;
    }
    else
    {
        fbe_topology_class_trace(FBE_CLASS_ID_SAS_MAGNUM_ENCLOSURE,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "MagnumEncl:%s, component %s require %d space!!!\n", 
                                __FUNCTION__, 
                                enclosure_access_printComponentType(componentType),
                                totalSize);
        status = FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    return (status);
}

/*!*************************************************************************
* @fn fbe_sas_magnum_release_encl_data_block(                  
*                    fbe_enclosure_component_block_t   *component_block)                  
***************************************************************************
*
* @brief
*       This function release a chain of blocks.
*
* @param      component_block - beginning of a chain.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   10-Oct-2008 NC - Added header.
*    02-Mar-2009: BP - Created from fbe_sas_viper_release_encl_data_block.
*
***************************************************************************/
static fbe_status_t 
fbe_sas_magnum_release_encl_data_block(fbe_enclosure_component_block_t   *component_block)
{
    fbe_enclosure_component_block_t *sas_magnum_component_block;

    sas_magnum_component_block = fbe_edal_drop_the_tail_block(component_block);

    while (sas_magnum_component_block!=NULL)
    {
        /* release the memory */
        fbe_transport_release_buffer(sas_magnum_component_block);
        /* drop the next one */
        sas_magnum_component_block = fbe_edal_drop_the_tail_block(component_block);
    }

    if(component_block != NULL) 
    {
        fbe_transport_release_buffer(component_block);
    }

    return (FBE_STATUS_OK);
}

// End of file fbe_sas_magnum_enclosure_main.c
