/***************************************************************************
 * Copyright (C) EMC Corporation  2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 * Unpublished - all rights reserved under the copyright laws
 * of the United States
 ***************************************************************************/

/*!**************************************************************************
 *  @file edal_processor_enclosure_data_get.c
 ***************************************************************************
 * @brief
 *      This module contains functions to retrieve data for the processor 
 *      enclosure component.
 *
 *  @ingroup fbe_edal
 *
 *  @version
 *      23-Feb-2010: PHE - Created.
 *
 *
 **************************************************************************/
#include "fbe/fbe_winddk.h"
#include "edal_processor_enclosure_data.h"
#include "fbe_enclosure_data_access_private.h"
#include "fbe_pe_types.h"

/*!*************************************************************************
 *  @fn processor_enclosure_get_component_size
 **************************************************************************
 * @brief
 *      This function returns the component size for the specified 
 *      enclosure type and component type.
 *
 * @param component_type(Input) - the specified component type.
 * @param component_size_p(Output) - the pointer to the returned component size.
 *
 * @return  fbe_edal_status_t    
 *
 *  NOTES:
 *
 *  HISTORY:
 *    03-Mar-2010: PHE - Created
 *
 **************************************************************************/
fbe_edal_status_t
processor_enclosure_get_component_size(fbe_enclosure_component_types_t component_type,
                                      fbe_u32_t * component_size_p)
{
    fbe_edal_status_t  status = FBE_EDAL_STATUS_OK;
        
    switch(component_type)
    {
        case FBE_ENCL_IO_MODULE:
        case FBE_ENCL_BEM:
        case FBE_ENCL_MEZZANINE:
            *component_size_p = sizeof(edal_pe_io_comp_info_t);
            break;

        case FBE_ENCL_IO_PORT:
            *component_size_p = sizeof(edal_pe_io_port_info_t);
            break;

        case FBE_ENCL_PLATFORM:
            *component_size_p = sizeof(edal_pe_platform_info_t);
            break;

        case FBE_ENCL_MISC:
            *component_size_p = sizeof(edal_pe_misc_info_t);
            break;      

        case FBE_ENCL_POWER_SUPPLY:
            *component_size_p = sizeof(edal_pe_power_supply_info_t);
            break;

        case FBE_ENCL_MGMT_MODULE:
            *component_size_p = sizeof(edal_pe_mgmt_module_info_t);
            break;

        case FBE_ENCL_COOLING_COMPONENT:
            *component_size_p = sizeof(edal_pe_cooling_info_t);
            break;

        case FBE_ENCL_FLT_REG:
            *component_size_p = sizeof(edal_pe_flt_exp_info_t);
            break;

        case FBE_ENCL_SLAVE_PORT:
            *component_size_p = sizeof(edal_pe_slave_port_info_t);
            break;

        case FBE_ENCL_SUITCASE:
            *component_size_p = sizeof(edal_pe_suitcase_info_t);
            break;

        case FBE_ENCL_BMC:
            *component_size_p = sizeof(edal_pe_bmc_info_t);
            break;

        case FBE_ENCL_RESUME_PROM:
            *component_size_p = sizeof(edal_pe_resume_prom_info_t);
            break;

        case FBE_ENCL_TEMPERATURE:
            *component_size_p = sizeof(edal_pe_temperature_info_t);
            break;

        case FBE_ENCL_SPS:
            *component_size_p = sizeof(edal_pe_sps_info_t);
            break;

        case FBE_ENCL_BATTERY:
            *component_size_p = sizeof(edal_pe_battery_info_t);
            break;

        case FBE_ENCL_CACHE_CARD:
            *component_size_p = sizeof(edal_pe_cache_card_info_t);
            break;

        case FBE_ENCL_DIMM:
            *component_size_p = sizeof(edal_pe_dimm_info_t);
            break;

        case FBE_ENCL_SSD:
            *component_size_p = sizeof(edal_pe_ssd_info_t);
            break;

        default:
            enclosure_access_log_error("EDAL:%s, unhandled component type %s.\n", 
                __FUNCTION__, 
                enclosure_access_printComponentType(component_type));

            status = FBE_EDAL_STATUS_COMP_TYPE_UNSUPPORTED;
            break;
    }
    
    return status;
}

/*!*************************************************************************
 *                         @fn processor_enclosure_is_component_type_supported
 **************************************************************************
 *  @brief
 *      This function checks whether the processor enclosure component type is valid. 
 *      for the processor enclosure component.
 *
 *  @param componentType - The component type 
 *
 *  @return TRUE - supported, FALSE - unsupported.
 *
 *  @version
 *    04-May-2010: PHE - Created.
 *
 **************************************************************************/
fbe_bool_t processor_enclosure_is_component_type_supported(fbe_enclosure_component_types_t componentType)
{
    fbe_bool_t componentTypeValid = FALSE;
        
    switch (componentType)
    {
        case FBE_ENCL_IO_MODULE:
        case FBE_ENCL_IO_PORT:
        case FBE_ENCL_BEM:
        case FBE_ENCL_MEZZANINE:
        case FBE_ENCL_POWER_SUPPLY:        
        case FBE_ENCL_COOLING_COMPONENT:
        case FBE_ENCL_MGMT_MODULE:        
        case FBE_ENCL_MISC:
        case FBE_ENCL_PLATFORM:
        case FBE_ENCL_FLT_REG:
        case FBE_ENCL_SLAVE_PORT:
        case FBE_ENCL_SUITCASE:
        case FBE_ENCL_BMC:
        case FBE_ENCL_RESUME_PROM:
        case FBE_ENCL_TEMPERATURE:
        case FBE_ENCL_SPS:
        case FBE_ENCL_BATTERY:
        case FBE_ENCL_CACHE_CARD:
        case FBE_ENCL_DIMM:
        case FBE_ENCL_SSD:
            componentTypeValid = TRUE;
            break;
        default:
            componentTypeValid = FALSE;
            break;
    }

    return componentTypeValid;
}
/*!*************************************************************************
 *  @fn processor_enclosure_access_getBuffer
 **************************************************************************
 *  @brief
 *      This function gets a block of data for the processor enclosure component.
 *
 *  @param generalDataPtr - pointer to the component data
 *  @param attribute - enum for attribute to get
 *  @param component - type of Component 
 *  @param returnDataPtr - pointer to the boolean return value
 *
 *  @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  @version
 *    17-Apr-2010: PHE - Created.
 *
 **************************************************************************/
fbe_edal_status_t 
processor_enclosure_access_getBuffer(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t componentType,
                              fbe_u32_t buffer_length,
                              fbe_u8_t *buffer_p)
{
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;
    fbe_encl_component_general_info_t   *genComponentDataPtr = NULL;
    fbe_encl_component_general_status_info_t    *genStatusDataPtr = NULL;
    edal_pe_io_comp_info_t * peIoCompInfoPtr = NULL;
    edal_pe_io_port_info_t * peIoPortInfoPtr = NULL;
    edal_pe_power_supply_info_t * powerSupplyInfoPtr = NULL;
    edal_pe_cooling_info_t * coolingInfoPtr = NULL;
    edal_pe_mgmt_module_info_t * mgmtModuleInfoPtr = NULL;

    edal_pe_platform_info_t * platformInfoPtr = NULL;
    edal_pe_misc_info_t * miscInfoPtr = NULL;   
    edal_pe_resume_prom_info_t * rpInfoPtr = NULL;   
    edal_pe_flt_exp_info_t * fltExpInfoPtr = NULL;
    edal_pe_slave_port_info_t * slavePortInfoPtr = NULL; 
    edal_pe_suitcase_info_t * suitcaseInfoPtr = NULL;
    edal_pe_bmc_info_t * bmcInfoPtr = NULL;
    edal_pe_temperature_info_t * tempInfoPtr = NULL; 
    edal_pe_sps_info_t * spsInfoPtr = NULL; 
    edal_pe_battery_info_t * batteryInfoPtr = NULL; 
    edal_pe_cache_card_info_t * cacheCardInfoPtr = NULL; 
    edal_pe_dimm_info_t * dimmInfoPtr = NULL; 
    fbe_board_mgmt_ssd_info_t *ssdInfoPtr = NULL;

    /* validate pointer */
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }

    /*
     * Get the General Component Data and status data based on component type
     */

    status = base_enclosure_access_component_status_data_ptr(generalDataPtr,
                                                componentType,
                                                &genComponentDataPtr,
                                                &genStatusDataPtr);

    if(status != FBE_EDAL_STATUS_OK)
    {
        return(status);
    }

    /* validate buffer */
    if (genStatusDataPtr->componentCanary != EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, comp %d, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            componentType,
            EDAL_COMPONENT_CANARY_VALUE, 
            genStatusDataPtr->componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }
    
    switch (attribute)
    {
        case FBE_ENCL_PE_PLATFORM_INFO:
            platformInfoPtr = (edal_pe_platform_info_t *)generalDataPtr;
            if(buffer_length >= sizeof(fbe_board_mgmt_platform_info_t)) 
            {
                buffer_length = sizeof(fbe_board_mgmt_platform_info_t);
            
                fbe_copy_memory(buffer_p, 
                        &(platformInfoPtr->pePlatformSubInfo), 
                        buffer_length);
                status = FBE_EDAL_STATUS_OK;
            }
            else
            {
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            break;
        
        case FBE_ENCL_PE_MISC_INFO:
            miscInfoPtr = (edal_pe_misc_info_t *)generalDataPtr;
            if(buffer_length >= sizeof(fbe_board_mgmt_misc_info_t)) 
            {
                buffer_length = sizeof(fbe_board_mgmt_misc_info_t);
            
                fbe_copy_memory(buffer_p, 
                        &(miscInfoPtr->peMiscSubInfo), 
                        buffer_length);
                status = FBE_EDAL_STATUS_OK;
            }
            else
            {
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            break;
        
        case FBE_ENCL_PE_RESUME_PROM_INFO:
            rpInfoPtr = (edal_pe_resume_prom_info_t *)generalDataPtr;
            if(buffer_length >= sizeof(edal_pe_resume_prom_sub_info_t)) 
            {
                buffer_length = sizeof(edal_pe_resume_prom_sub_info_t);
    
                fbe_copy_memory(buffer_p, 
                        &(rpInfoPtr->peResumePromSubInfo), 
                        buffer_length);
                status = FBE_EDAL_STATUS_OK;
            }
            else
            {
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            break;

        case FBE_ENCL_PE_IO_COMP_INFO:
            peIoCompInfoPtr = (edal_pe_io_comp_info_t *)generalDataPtr;
            if(buffer_length >= sizeof(edal_pe_io_comp_sub_info_t)) 
            {
                buffer_length = sizeof(edal_pe_io_comp_sub_info_t);
            
                fbe_copy_memory(buffer_p, 
                        &(peIoCompInfoPtr->peIoCompSubInfo), 
                        buffer_length);
                status = FBE_EDAL_STATUS_OK;
            }
            else
            {
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            break;

        case FBE_ENCL_PE_IO_PORT_INFO:
            peIoPortInfoPtr = (edal_pe_io_port_info_t *)generalDataPtr;
            if(buffer_length >= sizeof(edal_pe_io_port_sub_info_t)) 
            {
                buffer_length = sizeof(edal_pe_io_port_sub_info_t);
            
                fbe_copy_memory(buffer_p, 
                        &(peIoPortInfoPtr->peIoPortSubInfo), 
                        buffer_length);
                status = FBE_EDAL_STATUS_OK;
            }
            else
            {
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            break;

        case FBE_ENCL_PE_POWER_SUPPLY_INFO:
            powerSupplyInfoPtr = (edal_pe_power_supply_info_t *)generalDataPtr;
            if(buffer_length >= sizeof(edal_pe_power_supply_sub_info_t)) 
            {
                buffer_length = sizeof(edal_pe_power_supply_sub_info_t);
            
                fbe_copy_memory(buffer_p, 
                        &(powerSupplyInfoPtr->pePowerSupplySubInfo), 
                        buffer_length);
                status = FBE_EDAL_STATUS_OK;
            }
            else
            {
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            break;

        case FBE_ENCL_PE_COOLING_INFO:
            coolingInfoPtr = (edal_pe_cooling_info_t *)generalDataPtr;
            if(buffer_length >= sizeof(edal_pe_cooling_sub_info_t)) 
            {
                buffer_length = sizeof(edal_pe_cooling_sub_info_t);
           
                fbe_copy_memory(buffer_p, 
                        &(coolingInfoPtr->peCoolingSubInfo), 
                        buffer_length);
                status = FBE_EDAL_STATUS_OK;
            }
            else
            {
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            break;


        case FBE_ENCL_PE_MGMT_MODULE_INFO:
            mgmtModuleInfoPtr = (edal_pe_mgmt_module_info_t *)generalDataPtr;
            if(buffer_length >= sizeof(edal_pe_mgmt_module_sub_info_t)) 
            {
                buffer_length = sizeof(edal_pe_mgmt_module_sub_info_t);
            
                fbe_copy_memory(buffer_p, 
                        &(mgmtModuleInfoPtr->peMgmtModuleSubInfo), 
                        buffer_length);
                status = FBE_EDAL_STATUS_OK;
            }
            else
            {
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            break;             

        case FBE_ENCL_PE_FLT_REG_INFO:
            fltExpInfoPtr = (edal_pe_flt_exp_info_t *)generalDataPtr;
            if(buffer_length >= sizeof(edal_pe_flt_exp_sub_info_t)) 
            {
                buffer_length = sizeof(edal_pe_flt_exp_sub_info_t);
            
                fbe_copy_memory(buffer_p, 
                        &(fltExpInfoPtr->peFltExpSubInfo), 
                        buffer_length);
                status = FBE_EDAL_STATUS_OK;
            }
            else
            {
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            break;

        case FBE_ENCL_PE_SLAVE_PORT_INFO:
            slavePortInfoPtr = (edal_pe_slave_port_info_t *)generalDataPtr;
            if(buffer_length >= sizeof(edal_pe_slave_port_sub_info_t)) 
            {
                buffer_length = sizeof(edal_pe_slave_port_sub_info_t);
            
                fbe_copy_memory(buffer_p, 
                        &(slavePortInfoPtr->peSlavePortSubInfo), 
                        buffer_length);
                status = FBE_EDAL_STATUS_OK;
            }
            else
            {
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            break;
        
        case FBE_ENCL_PE_SUITCASE_INFO:
            suitcaseInfoPtr = (edal_pe_suitcase_info_t *)generalDataPtr;
            if(buffer_length >= sizeof(edal_pe_suitcase_sub_info_t)) 
            {
                buffer_length = sizeof(edal_pe_suitcase_sub_info_t);
            
                fbe_copy_memory(buffer_p, 
                        &(suitcaseInfoPtr->peSuitcaseSubInfo), 
                        buffer_length);
                status = FBE_EDAL_STATUS_OK;
            }
            else
            {
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            break;

        case FBE_ENCL_PE_BMC_INFO:
            bmcInfoPtr = (edal_pe_bmc_info_t *)generalDataPtr;
            if(buffer_length >= sizeof(edal_pe_bmc_sub_info_t)) 
            {
                buffer_length = sizeof(edal_pe_bmc_sub_info_t);
            
                fbe_copy_memory(buffer_p, 
                        &(bmcInfoPtr->peBmcSubInfo), 
                        buffer_length);
                status = FBE_EDAL_STATUS_OK;
            }
            else
            {
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            break;

        case FBE_ENCL_PE_TEMPERATURE_INFO:
            tempInfoPtr = (edal_pe_temperature_info_t *)generalDataPtr;
            if(buffer_length >= sizeof(edal_pe_temperature_sub_info_t)) 
            {
                buffer_length = sizeof(edal_pe_temperature_sub_info_t);

                fbe_copy_memory(buffer_p, 
                                &(tempInfoPtr->peTempSubInfo), 
                                buffer_length);
                status = FBE_EDAL_STATUS_OK;
            }
            else
            {
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            break;

        case FBE_ENCL_PE_SPS_INFO:
            spsInfoPtr = (edal_pe_sps_info_t *)generalDataPtr;
            if(buffer_length >= sizeof(edal_pe_sps_sub_info_t)) 
            {
                buffer_length = sizeof(edal_pe_sps_sub_info_t);

                fbe_copy_memory(buffer_p, 
                                &(spsInfoPtr->peSpsSubInfo), 
                                buffer_length);
                status = FBE_EDAL_STATUS_OK;
            }
            else
            {
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            break;

        case FBE_ENCL_PE_BATTERY_INFO:
            batteryInfoPtr = (edal_pe_battery_info_t *)generalDataPtr;
            if(buffer_length >= sizeof(edal_pe_battery_sub_info_t)) 
            {
                buffer_length = sizeof(edal_pe_battery_sub_info_t);

                fbe_copy_memory(buffer_p, 
                                &(batteryInfoPtr->peBatterySubInfo), 
                                buffer_length);
                status = FBE_EDAL_STATUS_OK;
            }
            else
            {
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            break;

        case FBE_ENCL_PE_CACHE_CARD_INFO:
            cacheCardInfoPtr = (edal_pe_cache_card_info_t *)generalDataPtr;
            if(buffer_length >= sizeof(edal_pe_cache_card_sub_info_t)) 
            {
                buffer_length = sizeof(edal_pe_cache_card_sub_info_t);

                fbe_copy_memory(buffer_p, 
                                &(cacheCardInfoPtr->peCacheCardSubInfo), 
                                buffer_length);
                status = FBE_EDAL_STATUS_OK;
            }
            else
            {
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            break;
        
        case FBE_ENCL_PE_DIMM_INFO:
            dimmInfoPtr = (edal_pe_dimm_info_t *)generalDataPtr;
            if(buffer_length >= sizeof(edal_pe_dimm_sub_info_t)) 
            {
                buffer_length = sizeof(edal_pe_dimm_sub_info_t);

                fbe_copy_memory(buffer_p, 
                                &(dimmInfoPtr->peDimmSubInfo), 
                                buffer_length);
                status = FBE_EDAL_STATUS_OK;
            }
            else
            {
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            break;

        case FBE_ENCL_PE_SSD_INFO:
            ssdInfoPtr = &((edal_pe_ssd_info_t *)generalDataPtr)->ssdInfo;
            if(buffer_length >= sizeof(fbe_board_mgmt_ssd_info_t))
            {
                buffer_length = sizeof(fbe_board_mgmt_ssd_info_t);

                fbe_copy_memory(buffer_p, 
                                ssdInfoPtr, 
                                buffer_length);
                status = FBE_EDAL_STATUS_OK;
            }
            else
            {
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            break;
        
        default:
            status = FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
            break;

    }

    return status;
}   // end of processor_enclosure_access_getBuffer


/*!*************************************************************************
 *  @fn processor_enclosure_access_getBool
 **************************************************************************
 *  @brief
 *      This function gets Boolean type data for the processor enclosure component.
 *
 *  @param generalDataPtr - pointer to the component data
 *  @param attribute - enum for attribute to get
 *  @param component - type of Component 
 *  @param returnDataPtr - pointer to the boolean return value
 *
 *  @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  @version
 *    23-Feb-2010: PHE - Created.
 *
 **************************************************************************/
fbe_edal_status_t 
processor_enclosure_access_getBool(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t componentType,
                              fbe_bool_t *returnDataPtr)
{
    fbe_encl_component_general_info_t   *genComponentDataPtr = NULL;
    fbe_encl_component_general_status_info_t    *genStatusDataPtr = NULL;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    /* validate pointer */
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }

    /*
     * Get the General Component Data and status data based on component type
     */

    status = base_enclosure_access_component_status_data_ptr(generalDataPtr,
                                                componentType,
                                                &genComponentDataPtr,
                                                &genStatusDataPtr);

    if(status != FBE_EDAL_STATUS_OK)
    {
        return(status);
    }

    /* validate buffer */
    if (genStatusDataPtr->componentCanary != EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, comp %d, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            componentType,
            EDAL_COMPONENT_CANARY_VALUE, 
            genStatusDataPtr->componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    if(processor_enclosure_is_component_type_supported(componentType))
    {
        // check if attribute defined in super enclosure
        status = base_enclosure_access_getBool(generalDataPtr,
                                                attribute,
                                                componentType,
                                                returnDataPtr);                  
    }
    else
    {
        status = FBE_EDAL_STATUS_COMP_TYPE_UNSUPPORTED;           
    }

    return status;
}   // end of processor_enclosure_access_getBool


/*!*************************************************************************
 *  @fn processor_enclosure_access_getU8
 **************************************************************************
 *  @brief
 *      This function gets fbe_u8_t type data 
 *      for the processor enclosure component.
 *
 *  @param generalDataPtr - pointer to the component data
 *  @param attribute - enum for attribute to get
 *  @param component - type of Component 
 *  @param returnDataPtr - pointer to the fbe_u8_t return value
 *
 *  @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  @version
 *    23-Feb-2010: PHE - Created.
 *
 **************************************************************************/
fbe_edal_status_t 
processor_enclosure_access_getU8(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_enclosure_component_types_t componentType,
                            fbe_u8_t *returnDataPtr)
{
    fbe_encl_component_general_info_t   *genComponentDataPtr = NULL;
    fbe_encl_component_general_status_info_t    *genStatusDataPtr = NULL;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    /* validate pointer */
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }

    /*
     * Get the General Component Data and status data based on component type
     */

    status = base_enclosure_access_component_status_data_ptr(generalDataPtr,
                                                componentType,
                                                &genComponentDataPtr,
                                                &genStatusDataPtr);

    if(status != FBE_EDAL_STATUS_OK)
    {
        return(status);
    }

    if(processor_enclosure_is_component_type_supported(componentType))
    {
        // check if attribute defined in super enclosure
        status = base_enclosure_access_getU8(generalDataPtr,
                                                attribute,
                                                componentType,
                                                returnDataPtr);                  
    }
    else
    {
        status = FBE_EDAL_STATUS_COMP_TYPE_UNSUPPORTED;           
    }   

    return status;

}   // end of processor_enclosure_access_getU8



/*!*************************************************************************
 *  @fn processor_enclosure_access_getU32
 **************************************************************************
 *  @brief
 *      This function gets fbe_u32_t type data 
 *      for the processor enclosure component.
 *
 *  @param generalDataPtr - pointer to the component data
 *  @param attribute - enum for attribute to get
 *  @param component - type of Component 
 *  @param returnDataPtr - pointer to the fbe_u32_t return value
 *
 *  @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  @version
 *    23-Feb-2010: PHE - Created.
 *
 **************************************************************************/
fbe_edal_status_t 
processor_enclosure_access_getU32(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_enclosure_component_types_t componentType,
                            fbe_u32_t *returnDataPtr)
{
    fbe_encl_component_general_info_t   *genComponentDataPtr = NULL;
    fbe_encl_component_general_status_info_t    *genStatusDataPtr = NULL;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    /* validate pointer */
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }

    /*
     * Get the General Component Data and status data based on component type
     */

    status = base_enclosure_access_component_status_data_ptr(generalDataPtr,
                                                componentType,
                                                &genComponentDataPtr,
                                                &genStatusDataPtr);

    if(status != FBE_EDAL_STATUS_OK)
    {
        return(status);
    }

    /* validate buffer */
    if (genStatusDataPtr->componentCanary != EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, comp %d, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            componentType,
            EDAL_COMPONENT_CANARY_VALUE, 
            genStatusDataPtr->componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    if(processor_enclosure_is_component_type_supported(componentType))
    {
        // check if attribute defined in super enclosure
        status = base_enclosure_access_getU32(generalDataPtr,
                                                attribute,
                                                componentType,
                                                returnDataPtr);                  
    }
    else
    {
        status = FBE_EDAL_STATUS_COMP_TYPE_UNSUPPORTED;           
    }   

    return status;
}   // end of processor_enclosure_access_getU32



/*!*************************************************************************
 *  @fn processor_enclosure_access_getU64
 **************************************************************************
 *  @brief
 *      This function gets fbe_u64_t type data 
 *      for the processor enclosure component.
 *
 *  @param generalDataPtr - pointer to the component data
 *  @param attribute - enum for attribute to get
 *  @param component - type of Component 
 *  @param returnDataPtr - pointer to the fbe_u64_t return value
 *
 *  @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  @version
 *    23-Feb-2010: PHE - Created.
 *
 **************************************************************************/
fbe_edal_status_t 
processor_enclosure_access_getU64(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_enclosure_component_types_t componentType,
                            fbe_u64_t *returnDataPtr)
{
    fbe_encl_component_general_info_t   *genComponentDataPtr = NULL;
    fbe_encl_component_general_status_info_t    *genStatusDataPtr = NULL;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    /* validate pointer */
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }

    /*
     * Get the General Component Data and status data based on component type
     */

    status = base_enclosure_access_component_status_data_ptr(generalDataPtr,
                                                componentType,
                                                &genComponentDataPtr,
                                                &genStatusDataPtr);

    if(status != FBE_EDAL_STATUS_OK)
    {
        return(status);
    }

    /* validate buffer */
    if (genStatusDataPtr->componentCanary != EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, comp %d, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            componentType,
            EDAL_COMPONENT_CANARY_VALUE, 
            genStatusDataPtr->componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    if(processor_enclosure_is_component_type_supported(componentType))
    {
        // check if attribute defined in super enclosure
        status = base_enclosure_access_getU64(generalDataPtr,
                                                attribute,
                                                componentType,
                                                returnDataPtr);                  
    }
    else
    {
        status = FBE_EDAL_STATUS_COMP_TYPE_UNSUPPORTED;           
    }   

    return status;
}   // end of processor_enclosure_access_getU64

/*!*************************************************************************
 *  @fn processor_enclosure_access_getStr
 **************************************************************************
 *  @brief
 *      This function gets string 
 *      for the processor enclosure component.
 *
 *  @param generalDataPtr - pointer to a component
 *  @param attribute - enum for attribute to get
 *  @param componentType - type of Component (PS, Drive, ...)
 *  @param returnDataPtr - pointer to the String return value
 *  @param length - length of string
 *
 *  @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  @version
 *    23-Feb-2010: PHE - Created.
 *
 **************************************************************************/
fbe_edal_status_t 
processor_enclosure_access_getStr(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t componentType,
                              fbe_u32_t length,
                              char *returnStringPtr)
{
    fbe_encl_component_general_info_t   *genComponentDataPtr = NULL;
    fbe_encl_component_general_status_info_t    *genStatusDataPtr = NULL;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    /* validate pointer */
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }

    /*
     * Get the General Component Data and status data based on component type
     */

    status = base_enclosure_access_component_status_data_ptr(generalDataPtr,
                                                componentType,
                                                &genComponentDataPtr,
                                                &genStatusDataPtr);

    if(status != FBE_EDAL_STATUS_OK)
    {
        return(status);
    }

    /* validate buffer */
    if (genStatusDataPtr->componentCanary != EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, comp %d, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            componentType,
            EDAL_COMPONENT_CANARY_VALUE, 
            genStatusDataPtr->componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    if(processor_enclosure_is_component_type_supported(componentType))
    {
        // check if attribute defined in super enclosure
        status = base_enclosure_access_getStr(generalDataPtr,
                                                attribute,
                                                componentType,
                                                length,
                                                returnStringPtr);                  
    }
    else
    {
        status = FBE_EDAL_STATUS_COMP_TYPE_UNSUPPORTED;           
    }   

    return status;
}   // end of processor_enclosure_access_getStr

