/***************************************************************************
 * Copyright (C) EMC Corporation  2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 * Unpublished - all rights reserved under the copyright laws
 * of the United States
 ***************************************************************************/

/*!**************************************************************************
 *  @file edal_processor_enclosure_data_set.c
 ***************************************************************************
 * @brief
 *      This module contains functions to update data for the processor 
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

static fbe_edal_status_t 
processor_enclosure_access_setCompBuffer(fbe_edal_general_comp_handle_t generalDataPtr,
                                         fbe_enclosure_component_types_t componentType,
                                         fbe_u32_t newInfoLength,
                                         fbe_u32_t infoSize,
                                         fbe_u32_t externalInfoSize,
                                         fbe_u8_t * newInfoPtr,
                                         fbe_u8_t * oldInfoPtr,
                                         fbe_u8_t * newExternalInfoPtr, 
                                         fbe_u8_t * oldExternalInfoPtr);

/*!*************************************************************************
 *  @fn processor_enclosure_access_setBuffer
 **************************************************************************
 *  @brief
 *      This function sets a block of data for the processor enclosure component.
 *
 *  @param generalDataPtr - pointer to the component data
 *  @param attribute - enum for attribute to get
 *  @param componentType - type of Component 
 *  @param buffer_length - input buffer length
 *  @param buffer_length - input buffer length
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
processor_enclosure_access_setBuffer(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t componentType,
                              fbe_u32_t buffer_length,
                              fbe_u8_t *buffer_p)
{
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;
    fbe_encl_component_general_info_t   *genComponentDataPtr = NULL;
    fbe_encl_component_general_status_info_t    *genStatusDataPtr = NULL;
    fbe_u32_t subInfoSize = 0;
    fbe_u32_t externalInfoSize = 0;

    edal_pe_platform_info_t     * oldPePlatformInfoPtr = NULL;    
    SPID_PLATFORM_INFO * oldPePlatformSubInfoPtr = NULL;
    SPID_PLATFORM_INFO * newPePlatformSubInfoPtr = NULL;

    edal_pe_misc_info_t     * oldPeMiscInfoPtr = NULL;    
    fbe_board_mgmt_misc_info_t * oldPeMiscSubInfoPtr = NULL;
    fbe_board_mgmt_misc_info_t * newPeMiscSubInfoPtr = NULL;

    edal_pe_io_comp_info_t     * oldPeIoCompInfoPtr = NULL;    
    edal_pe_io_comp_sub_info_t * oldPeIoCompSubInfoPtr = NULL;
    edal_pe_io_comp_sub_info_t * newPeIoCompSubInfoPtr = NULL;

    edal_pe_io_port_info_t     * oldPeIoPortInfoPtr = NULL;    
    edal_pe_io_port_sub_info_t * oldPeIoPortSubInfoPtr = NULL;
    edal_pe_io_port_sub_info_t * newPeIoPortSubInfoPtr = NULL;

    edal_pe_power_supply_info_t     * oldPePowerSupplyInfoPtr = NULL;    
    edal_pe_power_supply_sub_info_t * oldPePowerSupplySubInfoPtr = NULL;
    edal_pe_power_supply_sub_info_t * newPePowerSupplySubInfoPtr = NULL;  
    /* 
     * tempNewExternalPowerSupplyInfo and tempOldExternalPowerSupplyInfo
     * are just used for masking off some fields we don't want to 
     * trigger state changes for.
     */
    fbe_power_supply_info_t           tempNewExternalPowerSupplyInfo = {0}; 
    fbe_power_supply_info_t           tempOldExternalPowerSupplyInfo = {0}; 

    edal_pe_cooling_info_t     * oldPeCoolingInfoPtr = NULL;    
    edal_pe_cooling_sub_info_t * oldPeCoolingSubInfoPtr = NULL;
    edal_pe_cooling_sub_info_t * newPeCoolingSubInfoPtr = NULL;

    edal_pe_mgmt_module_info_t     * oldPeMgmtModuleInfoPtr = NULL;    
    edal_pe_mgmt_module_sub_info_t * oldPeMgmtModuleSubInfoPtr = NULL;
    edal_pe_mgmt_module_sub_info_t * newPeMgmtModuleSubInfoPtr = NULL;

    edal_pe_flt_exp_info_t     * oldPeFltExpInfoPtr = NULL;    
    edal_pe_flt_exp_sub_info_t * oldPeFltExpSubInfoPtr = NULL;
    edal_pe_flt_exp_sub_info_t * newPeFltExpSubInfoPtr = NULL;

    edal_pe_slave_port_info_t     * oldPeSlavePortInfoPtr = NULL;    
    edal_pe_slave_port_sub_info_t * oldPeSlavePortSubInfoPtr = NULL;
    edal_pe_slave_port_sub_info_t * newPeSlavePortSubInfoPtr = NULL;

    edal_pe_suitcase_info_t     * oldPeSuitcaseInfoPtr = NULL;    
    edal_pe_suitcase_sub_info_t * oldPeSuitcaseSubInfoPtr = NULL;
    edal_pe_suitcase_sub_info_t * newPeSuitcaseSubInfoPtr = NULL;

    edal_pe_bmc_info_t     * oldPeBmcInfoPtr = NULL;
    edal_pe_bmc_sub_info_t * oldPeBmcSubInfoPtr = NULL;
    edal_pe_bmc_sub_info_t * newPeBmcSubInfoPtr = NULL;

    edal_pe_temperature_info_t     * oldPeTempInfoPtr = NULL;    
    edal_pe_temperature_sub_info_t * oldPeTempSubInfoPtr = NULL;
    edal_pe_temperature_sub_info_t * newPeTempSubInfoPtr = NULL;

    edal_pe_sps_info_t     * oldPeSpsInfoPtr = NULL;    
    edal_pe_sps_sub_info_t * oldPeSpsSubInfoPtr = NULL;
    edal_pe_sps_sub_info_t * newPeSpsSubInfoPtr = NULL;

    edal_pe_battery_info_t     * oldPeBatteryInfoPtr = NULL;    
    edal_pe_battery_sub_info_t * oldPeBatterySubInfoPtr = NULL;
    edal_pe_battery_sub_info_t * newPeBatterySubInfoPtr = NULL;

    edal_pe_resume_prom_info_t      * oldPeResumePromInfoPtr = NULL;    
    edal_pe_resume_prom_sub_info_t  * oldPeResumePromSubInfoPtr = NULL;
    edal_pe_resume_prom_sub_info_t  * newPeResumePromSubInfoPtr = NULL;

    edal_pe_cache_card_info_t     * oldPeCacheCardInfoPtr = NULL;    
    edal_pe_cache_card_sub_info_t * oldPeCacheCardSubInfoPtr = NULL;
    edal_pe_cache_card_sub_info_t * newPeCacheCardSubInfoPtr = NULL;

    edal_pe_dimm_info_t     * oldPeDimmInfoPtr = NULL;    
    edal_pe_dimm_sub_info_t * oldPeDimmSubInfoPtr = NULL;
    edal_pe_dimm_sub_info_t * newPeDimmSubInfoPtr = NULL;

    edal_pe_ssd_info_t * oldPeSSDInfoPtr = NULL;
    fbe_board_mgmt_ssd_info_t *oldPeSSDSubInfoPtr = NULL;
    fbe_board_mgmt_ssd_info_t *newPeSSDSubInfoPtr = NULL;

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
            subInfoSize = sizeof(SPID_PLATFORM_INFO);
            externalInfoSize = sizeof(SPID_PLATFORM_INFO);

            newPePlatformSubInfoPtr = (SPID_PLATFORM_INFO *)buffer_p;
            oldPePlatformInfoPtr = (edal_pe_platform_info_t *)generalDataPtr;
            oldPePlatformSubInfoPtr = &oldPePlatformInfoPtr->pePlatformSubInfo.hw_platform_info;
           
            status = processor_enclosure_access_setCompBuffer(generalDataPtr,
                                         componentType,                                         
                                         buffer_length,
                                         subInfoSize,
                                         externalInfoSize,
                                         (fbe_u8_t *)newPePlatformSubInfoPtr,
                                         (fbe_u8_t *)oldPePlatformSubInfoPtr,                                         
                                         (fbe_u8_t *)newPePlatformSubInfoPtr,                                         
                                         (fbe_u8_t *)oldPePlatformSubInfoPtr);
            break;

        case FBE_ENCL_PE_MISC_INFO:
            subInfoSize = sizeof(fbe_board_mgmt_misc_info_t);
            externalInfoSize = sizeof(fbe_board_mgmt_misc_info_t);

            newPeMiscSubInfoPtr = (fbe_board_mgmt_misc_info_t *)buffer_p;
            oldPeMiscInfoPtr = (edal_pe_misc_info_t *)generalDataPtr;
            oldPeMiscSubInfoPtr = &oldPeMiscInfoPtr->peMiscSubInfo;
           
            status = processor_enclosure_access_setCompBuffer(generalDataPtr,
                                         componentType,                                         
                                         buffer_length,
                                         subInfoSize,
                                         externalInfoSize,
                                         (fbe_u8_t *)newPeMiscSubInfoPtr,
                                         (fbe_u8_t *)oldPeMiscSubInfoPtr,                                         
                                         (fbe_u8_t *)newPeMiscSubInfoPtr,                                         
                                         (fbe_u8_t *)oldPeMiscSubInfoPtr);
            break;        
        
        case FBE_ENCL_PE_RESUME_PROM_INFO:
            subInfoSize = sizeof(edal_pe_resume_prom_sub_info_t);
            externalInfoSize = sizeof(fbe_board_mgmt_resume_prom_info_t);

            newPeResumePromSubInfoPtr = (edal_pe_resume_prom_sub_info_t *)buffer_p;
            oldPeResumePromInfoPtr = (edal_pe_resume_prom_info_t *)generalDataPtr;
            oldPeResumePromSubInfoPtr = &oldPeResumePromInfoPtr->peResumePromSubInfo;

            status = processor_enclosure_access_setCompBuffer(generalDataPtr,
                                         componentType,                                         
                                         buffer_length,
                                         subInfoSize,
                                         externalInfoSize,
                                         (fbe_u8_t *)newPeResumePromSubInfoPtr,
                                         (fbe_u8_t *)oldPeResumePromSubInfoPtr,                                         
                                         (fbe_u8_t *)&newPeResumePromSubInfoPtr->externalResumePromInfo,  
                                         (fbe_u8_t *)&oldPeResumePromSubInfoPtr->externalResumePromInfo);
            break;        

        case FBE_ENCL_PE_IO_COMP_INFO:
            subInfoSize = sizeof(edal_pe_io_comp_sub_info_t);
            externalInfoSize = sizeof(fbe_board_mgmt_io_comp_info_t);

            newPeIoCompSubInfoPtr = (edal_pe_io_comp_sub_info_t *)buffer_p;
            oldPeIoCompInfoPtr = (edal_pe_io_comp_info_t *)generalDataPtr;
            oldPeIoCompSubInfoPtr = &oldPeIoCompInfoPtr->peIoCompSubInfo;
           
            status = processor_enclosure_access_setCompBuffer(generalDataPtr,
                                         componentType,                                         
                                         buffer_length,
                                         subInfoSize,
                                         externalInfoSize,
                                         (fbe_u8_t *)newPeIoCompSubInfoPtr,
                                         (fbe_u8_t *)oldPeIoCompSubInfoPtr,                                         
                                         (fbe_u8_t *)&newPeIoCompSubInfoPtr->externalIoCompInfo,                                         
                                         (fbe_u8_t *)&oldPeIoCompSubInfoPtr->externalIoCompInfo);
            
        
            break;

        case FBE_ENCL_PE_IO_PORT_INFO:
            subInfoSize = sizeof(edal_pe_io_port_sub_info_t);
            externalInfoSize = sizeof(fbe_board_mgmt_io_port_info_t);

            newPeIoPortSubInfoPtr = (edal_pe_io_port_sub_info_t *)buffer_p;
            oldPeIoPortInfoPtr = (edal_pe_io_port_info_t *)generalDataPtr;
            oldPeIoPortSubInfoPtr = &oldPeIoPortInfoPtr->peIoPortSubInfo;

            status = processor_enclosure_access_setCompBuffer(generalDataPtr,
                                         componentType,                                         
                                         buffer_length,
                                         subInfoSize,
                                         externalInfoSize,
                                         (fbe_u8_t *)newPeIoPortSubInfoPtr,
                                         (fbe_u8_t *)oldPeIoPortSubInfoPtr,                                         
                                         (fbe_u8_t *)&newPeIoPortSubInfoPtr->externalIoPortInfo,                                         
                                         (fbe_u8_t *)&oldPeIoPortSubInfoPtr->externalIoPortInfo); 
            
        
            break;

        case FBE_ENCL_PE_POWER_SUPPLY_INFO:
            subInfoSize = sizeof(edal_pe_power_supply_sub_info_t);
            externalInfoSize = sizeof(fbe_power_supply_info_t);

            newPePowerSupplySubInfoPtr = (edal_pe_power_supply_sub_info_t *)buffer_p;
            oldPePowerSupplyInfoPtr = (edal_pe_power_supply_info_t *)generalDataPtr;
            oldPePowerSupplySubInfoPtr = &oldPePowerSupplyInfoPtr->pePowerSupplySubInfo;
            /* Do not want to set the state_change to notify the clients when the input power changes.
             * the input power changes very oftern.
             * Copy to the new location.
             */
            fbe_copy_memory(&tempNewExternalPowerSupplyInfo, 
                            &newPePowerSupplySubInfoPtr->externalPowerSupplyInfo,
                            externalInfoSize);
            fbe_copy_memory(&tempOldExternalPowerSupplyInfo, 
                            &oldPePowerSupplySubInfoPtr->externalPowerSupplyInfo,
                            externalInfoSize);

            /* Zero out the fields that we don't want to generate the state change for. */
            tempNewExternalPowerSupplyInfo.inputPower = 0;
            tempNewExternalPowerSupplyInfo.inputPowerStatus = 0;
            tempNewExternalPowerSupplyInfo.psInputPower = 0;

            tempOldExternalPowerSupplyInfo.inputPower = 0;
            tempOldExternalPowerSupplyInfo.inputPowerStatus = 0;
            tempOldExternalPowerSupplyInfo.psInputPower = 0;
            

            status = processor_enclosure_access_setCompBuffer(generalDataPtr,
                                         componentType,                                         
                                         buffer_length,
                                         subInfoSize,
                                         externalInfoSize,
                                         (fbe_u8_t *)newPePowerSupplySubInfoPtr,
                                         (fbe_u8_t *)oldPePowerSupplySubInfoPtr,                                         
                                         (fbe_u8_t *)&tempNewExternalPowerSupplyInfo,                                         
                                         (fbe_u8_t *)&tempOldExternalPowerSupplyInfo); 
      
            break;

        case FBE_ENCL_PE_COOLING_INFO:
            subInfoSize = sizeof(edal_pe_cooling_sub_info_t);
            externalInfoSize = sizeof(fbe_cooling_info_t);

            newPeCoolingSubInfoPtr = (edal_pe_cooling_sub_info_t *)buffer_p;
            oldPeCoolingInfoPtr = (edal_pe_cooling_info_t *)generalDataPtr;
            oldPeCoolingSubInfoPtr = &oldPeCoolingInfoPtr->peCoolingSubInfo;

            status = processor_enclosure_access_setCompBuffer(generalDataPtr,
                                         componentType,                                         
                                         buffer_length,
                                         subInfoSize,
                                         externalInfoSize,
                                         (fbe_u8_t *)newPeCoolingSubInfoPtr,
                                         (fbe_u8_t *)oldPeCoolingSubInfoPtr,                                         
                                         (fbe_u8_t *)&newPeCoolingSubInfoPtr->externalCoolingInfo,                                         
                                         (fbe_u8_t *)&oldPeCoolingSubInfoPtr->externalCoolingInfo); 
            break;

        case FBE_ENCL_PE_MGMT_MODULE_INFO:
            subInfoSize = sizeof(edal_pe_mgmt_module_sub_info_t);
            externalInfoSize = sizeof(fbe_board_mgmt_mgmt_module_info_t);

            newPeMgmtModuleSubInfoPtr = (edal_pe_mgmt_module_sub_info_t *)buffer_p;
            oldPeMgmtModuleInfoPtr = (edal_pe_mgmt_module_info_t *)generalDataPtr;
            oldPeMgmtModuleSubInfoPtr = &oldPeMgmtModuleInfoPtr->peMgmtModuleSubInfo;

            status = processor_enclosure_access_setCompBuffer(generalDataPtr,
                                         componentType,                                         
                                         buffer_length,
                                         subInfoSize,
                                         externalInfoSize,
                                         (fbe_u8_t *)newPeMgmtModuleSubInfoPtr,
                                         (fbe_u8_t *)oldPeMgmtModuleSubInfoPtr,                                         
                                         (fbe_u8_t *)&newPeMgmtModuleSubInfoPtr->externalMgmtModuleInfo,                                         
                                         (fbe_u8_t *)&oldPeMgmtModuleSubInfoPtr->externalMgmtModuleInfo); 
            break;        

        case FBE_ENCL_PE_FLT_REG_INFO:
            subInfoSize = sizeof(edal_pe_flt_exp_sub_info_t);
            externalInfoSize = sizeof(fbe_peer_boot_flt_exp_info_t);

            newPeFltExpSubInfoPtr = (edal_pe_flt_exp_sub_info_t *)buffer_p;
            oldPeFltExpInfoPtr = (edal_pe_flt_exp_info_t *)generalDataPtr;
            oldPeFltExpSubInfoPtr = &oldPeFltExpInfoPtr->peFltExpSubInfo;

            status = processor_enclosure_access_setCompBuffer(generalDataPtr,
                                         componentType,                                         
                                         buffer_length,
                                         subInfoSize,
                                         externalInfoSize,
                                         (fbe_u8_t *)newPeFltExpSubInfoPtr,
                                         (fbe_u8_t *)oldPeFltExpSubInfoPtr,                                         
                                         (fbe_u8_t *)&newPeFltExpSubInfoPtr->externalFltExpInfo,                                         
                                         (fbe_u8_t *)&oldPeFltExpSubInfoPtr->externalFltExpInfo); 
            
            break;

         case FBE_ENCL_PE_SLAVE_PORT_INFO:
            subInfoSize = sizeof(edal_pe_slave_port_sub_info_t);
            externalInfoSize = sizeof(fbe_board_mgmt_slave_port_info_t);

            newPeSlavePortSubInfoPtr = (edal_pe_slave_port_sub_info_t *)buffer_p;
            oldPeSlavePortInfoPtr = (edal_pe_slave_port_info_t *)generalDataPtr;
            oldPeSlavePortSubInfoPtr = &oldPeSlavePortInfoPtr->peSlavePortSubInfo;

            status = processor_enclosure_access_setCompBuffer(generalDataPtr,
                                         componentType,                                         
                                         buffer_length,
                                         subInfoSize,
                                         externalInfoSize,
                                         (fbe_u8_t *)newPeSlavePortSubInfoPtr,
                                         (fbe_u8_t *)oldPeSlavePortSubInfoPtr,                                         
                                         (fbe_u8_t *)&newPeSlavePortSubInfoPtr->externalSlavePortInfo,                                         
                                         (fbe_u8_t *)&oldPeSlavePortSubInfoPtr->externalSlavePortInfo); 
            break;
        
         case FBE_ENCL_PE_SUITCASE_INFO:
            subInfoSize = sizeof(edal_pe_suitcase_sub_info_t);
            externalInfoSize = sizeof(fbe_board_mgmt_suitcase_info_t);

            newPeSuitcaseSubInfoPtr = (edal_pe_suitcase_sub_info_t *)buffer_p;
            oldPeSuitcaseInfoPtr = (edal_pe_suitcase_info_t *)generalDataPtr;
            oldPeSuitcaseSubInfoPtr = &oldPeSuitcaseInfoPtr->peSuitcaseSubInfo;

            status = processor_enclosure_access_setCompBuffer(generalDataPtr,
                                         componentType,                                         
                                         buffer_length,
                                         subInfoSize,
                                         externalInfoSize,
                                         (fbe_u8_t *)newPeSuitcaseSubInfoPtr,
                                         (fbe_u8_t *)oldPeSuitcaseSubInfoPtr,                                         
                                         (fbe_u8_t *)&newPeSuitcaseSubInfoPtr->externalSuitcaseInfo,                                         
                                         (fbe_u8_t *)&oldPeSuitcaseSubInfoPtr->externalSuitcaseInfo); 
            break;          

         case FBE_ENCL_PE_BMC_INFO:
            subInfoSize = sizeof(edal_pe_bmc_sub_info_t);
            externalInfoSize = sizeof(fbe_board_mgmt_bmc_info_t);

            newPeBmcSubInfoPtr = (edal_pe_bmc_sub_info_t *)buffer_p;
            oldPeBmcInfoPtr = (edal_pe_bmc_info_t *)generalDataPtr;
            oldPeBmcSubInfoPtr = &oldPeBmcInfoPtr->peBmcSubInfo;

            status = processor_enclosure_access_setCompBuffer(generalDataPtr,
                                         componentType,
                                         buffer_length,
                                         subInfoSize,
                                         externalInfoSize,
                                         (fbe_u8_t *)newPeBmcSubInfoPtr,
                                         (fbe_u8_t *)oldPeBmcSubInfoPtr,
                                         (fbe_u8_t *)&newPeBmcSubInfoPtr->externalBmcInfo,
                                         (fbe_u8_t *)&oldPeBmcSubInfoPtr->externalBmcInfo); 
            break;

        case FBE_ENCL_PE_TEMPERATURE_INFO:
            subInfoSize = sizeof(edal_pe_temperature_sub_info_t);
            externalInfoSize = sizeof(fbe_eir_temp_sample_t);

            newPeTempSubInfoPtr = (edal_pe_temperature_sub_info_t *)buffer_p;
            oldPeTempInfoPtr = (edal_pe_temperature_info_t *)generalDataPtr;
            oldPeTempSubInfoPtr = &oldPeTempInfoPtr->peTempSubInfo;

            status = processor_enclosure_access_setCompBuffer(generalDataPtr,
                                                              componentType,                                         
                                                              buffer_length,
                                                              subInfoSize,
                                                              externalInfoSize,
                                                              (fbe_u8_t *)newPeTempSubInfoPtr,
                                                              (fbe_u8_t *)oldPeTempSubInfoPtr,                                         
                                                              (fbe_u8_t *)&newPeTempSubInfoPtr->externalTempInfo,                                         
                                                              (fbe_u8_t *)&oldPeTempSubInfoPtr->externalTempInfo); 
            break;          
                    
        case FBE_ENCL_PE_SPS_INFO:
            subInfoSize = sizeof(edal_pe_sps_sub_info_t);
            externalInfoSize = sizeof(fbe_base_sps_info_t);

            newPeSpsSubInfoPtr = (edal_pe_sps_sub_info_t *)buffer_p;
            oldPeSpsInfoPtr = (edal_pe_sps_info_t *)generalDataPtr;
            oldPeSpsSubInfoPtr = &oldPeSpsInfoPtr->peSpsSubInfo;

            status = processor_enclosure_access_setCompBuffer(generalDataPtr,
                                                              componentType,                                         
                                                              buffer_length,
                                                              subInfoSize,
                                                              externalInfoSize,
                                                              (fbe_u8_t *)newPeSpsSubInfoPtr,
                                                              (fbe_u8_t *)oldPeSpsSubInfoPtr,                                         
                                                              (fbe_u8_t *)&newPeSpsSubInfoPtr->externalSpsInfo,                                         
                                                              (fbe_u8_t *)&oldPeSpsSubInfoPtr->externalSpsInfo); 
            break;          

        case FBE_ENCL_PE_BATTERY_INFO:
            subInfoSize = sizeof(edal_pe_battery_sub_info_t);
            externalInfoSize = sizeof(fbe_base_battery_info_t);

            newPeBatterySubInfoPtr = (edal_pe_battery_sub_info_t *)buffer_p;
            oldPeBatteryInfoPtr = (edal_pe_battery_info_t *)generalDataPtr;
            oldPeBatterySubInfoPtr = &oldPeBatteryInfoPtr->peBatterySubInfo;

            status = processor_enclosure_access_setCompBuffer(generalDataPtr,
                                                              componentType,                                         
                                                              buffer_length,
                                                              subInfoSize,
                                                              externalInfoSize,
                                                              (fbe_u8_t *)newPeBatterySubInfoPtr,
                                                              (fbe_u8_t *)oldPeBatterySubInfoPtr,                                         
                                                              (fbe_u8_t *)&newPeBatterySubInfoPtr->externalBatteryInfo,                                         
                                                              (fbe_u8_t *)&oldPeBatterySubInfoPtr->externalBatteryInfo); 
            break;          
  
        case FBE_ENCL_PE_CACHE_CARD_INFO:
            subInfoSize = sizeof(edal_pe_cache_card_sub_info_t);
            externalInfoSize = sizeof(fbe_board_mgmt_cache_card_info_t);

            newPeCacheCardSubInfoPtr = (edal_pe_cache_card_sub_info_t *)buffer_p;
            oldPeCacheCardInfoPtr = (edal_pe_cache_card_info_t *)generalDataPtr;
            oldPeCacheCardSubInfoPtr = &oldPeCacheCardInfoPtr->peCacheCardSubInfo;

            status = processor_enclosure_access_setCompBuffer(generalDataPtr,
                                                              componentType,                                         
                                                              buffer_length,
                                                              subInfoSize,
                                                              externalInfoSize,
                                                              (fbe_u8_t *)newPeCacheCardSubInfoPtr,
                                                              (fbe_u8_t *)oldPeCacheCardSubInfoPtr,                                         
                                                              (fbe_u8_t *)&newPeCacheCardSubInfoPtr->externalCacheCardInfo,                                         
                                                              (fbe_u8_t *)&oldPeCacheCardSubInfoPtr->externalCacheCardInfo); 
            break;          
                                   
        case FBE_ENCL_PE_DIMM_INFO:
            subInfoSize = sizeof(edal_pe_dimm_sub_info_t);
            externalInfoSize = sizeof(fbe_board_mgmt_dimm_info_t);

            newPeDimmSubInfoPtr = (edal_pe_dimm_sub_info_t *)buffer_p;
            oldPeDimmInfoPtr = (edal_pe_dimm_info_t *)generalDataPtr;
            oldPeDimmSubInfoPtr = &oldPeDimmInfoPtr->peDimmSubInfo;
            

            status = processor_enclosure_access_setCompBuffer(generalDataPtr,
                                                              componentType,                                         
                                                              buffer_length,
                                                              subInfoSize,
                                                              externalInfoSize,
                                                              (fbe_u8_t *)newPeDimmSubInfoPtr,
                                                              (fbe_u8_t *)oldPeDimmSubInfoPtr,                                         
                                                              (fbe_u8_t *)&newPeDimmSubInfoPtr->externalDimmInfo,                                         
                                                              (fbe_u8_t *)&oldPeDimmSubInfoPtr->externalDimmInfo); 
            break; 
            
        case FBE_ENCL_PE_SSD_INFO:
            subInfoSize = sizeof(fbe_board_mgmt_ssd_info_t);
            newPeSSDSubInfoPtr = (fbe_board_mgmt_ssd_info_t *)buffer_p;
            oldPeSSDInfoPtr = (edal_pe_ssd_info_t *)generalDataPtr;
            oldPeSSDSubInfoPtr = &oldPeSSDInfoPtr->ssdInfo;
    
            status = processor_enclosure_access_setCompBuffer(generalDataPtr,
                                                              componentType,                                         
                                                              buffer_length,
                                                              subInfoSize,
                                                              0,
                                                              (fbe_u8_t *)newPeSSDSubInfoPtr,
                                                              (fbe_u8_t *)oldPeSSDSubInfoPtr,
                                                              (fbe_u8_t *)newPeSSDSubInfoPtr,
                                                              (fbe_u8_t *)oldPeSSDSubInfoPtr);
    
            break;
                

        default:
            status = FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
            break;

    }

    return status;
}   // end of processor_enclosure_access_setBuffer

/*!*************************************************************************
 *  @fn processor_enclosure_access_setCompBuffer
 **************************************************************************
 *  @brief
 *      This function sets a block of data for the processor enclosure component if needed
 *      and sets the state change attribute if the external info gets changed.
 *
 *  @param generalDataPtr - pointer to the component data
 *  @param componentType - The component type.
 *  @param newInfoLength - The length of new info.
 *  @param infoSize - The actual size of info.
 *  @param externalInfoSize - The actual size of external info.
 *  @param newInfoPtr - The pointer to the new info.
 *  @param oldInfoPtr - The pointer to the old info.
 *  @param newExternalInfoPtr - The pointer to the new external info.
 *  @param oldExternalInfoPtr - The pointer to the old external info.
 *
 *  @return fbe_edal_status_t - status of the operation
 *
 *
 *  @version
 *    04-May-2010: PHE - Created.
 *
 **************************************************************************/
static fbe_edal_status_t 
processor_enclosure_access_setCompBuffer(fbe_edal_general_comp_handle_t generalDataPtr,
                                         fbe_enclosure_component_types_t componentType,
                                         fbe_u32_t newInfoLength,
                                         fbe_u32_t infoSize,
                                         fbe_u32_t externalInfoSize,
                                         fbe_u8_t * newInfoPtr,
                                         fbe_u8_t * oldInfoPtr,
                                         fbe_u8_t * newExternalInfoPtr, 
                                         fbe_u8_t * oldExternalInfoPtr)
{
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    if(newInfoLength != infoSize) 
    {
        status = FBE_EDAL_STATUS_SIZE_MISMATCH;
    }
    else 
    {
        /* Have to check the externalInfo state change before copying the entire subInfo to EDAL.
         * Otherwise we won't see any state change for externalInfo. 
         */
        if (!fbe_equal_memory(oldExternalInfoPtr, newExternalInfoPtr, externalInfoSize))
        {            
            status = processor_enclosure_access_setBool(generalDataPtr,
                                                FBE_ENCL_COMP_STATE_CHANGE,
                                                componentType,
                                                TRUE);  
            if (!EDAL_STAT_OK(status))
            {
                return status;
            }
        }
        
        if (!fbe_equal_memory(oldInfoPtr, newInfoPtr, infoSize))
        {
        
            fbe_copy_memory(oldInfoPtr, newInfoPtr, infoSize);
            status = FBE_EDAL_STATUS_OK_VAL_CHANGED;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;   
        }
    }

    return status;
}


/*!*************************************************************************
 *                         @fn processor_enclosure_access_setBool
 **************************************************************************
 *  @brief
 *      This function updates the Boolean type data 
 *      for the processor enclosure component.
 *
 *  @param generalDataPtr - pointer to the component data
 *  @param attribute - enum for attribute to get
 *  @param component - enum for the component type
 *  @param newValue - new boolean value
 *
 *  @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status 
 *
 *  @version
 *    23-Feb-2010: PHE - Created.
 *
 **************************************************************************/
fbe_edal_status_t 
processor_enclosure_access_setBool(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t componentType,
                              fbe_bool_t newValue)
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
        status = base_enclosure_access_setBool(generalDataPtr,
                                            attribute,
                                            componentType,
                                            newValue);
    }
    else
    {
        status = FBE_EDAL_STATUS_COMP_TYPE_UNSUPPORTED;           
    }

    return status;
}   // end of processor_enclosure_access_setBool


/*!*************************************************************************
 *                         @fn processor_enclosure_access_setU8
 **************************************************************************
 *  @brief
 *      This function updates the fbe_u8_t type data 
 *      for the processor enclosure component.
 *
 *  @param generalDataPtr - pointer to the component data
 *  @param attribute - enum for attribute to get
 *  @param component - enum for the component type
 *  @param newValue - 
 *
 *  @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status 
 *
 *  @version
 *    23-Feb-2010: PHE - Created.
 *
 **************************************************************************/
fbe_edal_status_t 
processor_enclosure_access_setU8(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t componentType,
                              fbe_u8_t newValue)
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
        status = base_enclosure_access_setU8(generalDataPtr,
                                                attribute,
                                                componentType,
                                                newValue);
    }
    else
    {
        status = FBE_EDAL_STATUS_COMP_TYPE_UNSUPPORTED;           
    }

    return status;

}   // end of processor_enclosure_access_setU8



/*!*************************************************************************
 *                         @fn processor_enclosure_access_setU32
 **************************************************************************
 *  @brief
 *      This function updates the fbe_u32_t type data 
 *      for the processor enclosure component.
 *
 *  @param generalDataPtr - pointer to the component data
 *  @param attribute - enum for attribute to get
 *  @param component - enum for the component type
 *  @param newValue - 
 *
 *  @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status 
 *
 *  @version
 *    23-Feb-2010: PHE - Created.
 *
 **************************************************************************/
fbe_edal_status_t 
processor_enclosure_access_setU32(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t componentType,
                              fbe_u32_t newValue)
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
        status = base_enclosure_access_setU32(generalDataPtr,
                                                attribute,
                                                componentType,
                                                newValue);                  
    }
    else
    {
        status = FBE_EDAL_STATUS_COMP_TYPE_UNSUPPORTED;           
    }


    return status;
}   // end of processor_enclosure_access_setU32

/*!*************************************************************************
 *                         @fn processor_enclosure_access_setU64
 **************************************************************************
 *  @brief
 *      This function updates the fbe_u64_t type data 
 *      for the processor enclosure component.
 *
 *  @param generalDataPtr - pointer to the component data
 *  @param attribute - enum for attribute to get
 *  @param component - enum for the component type
 *  @param newValue - 
 *
 *  @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status 
 *
 *  @version
 *    23-Feb-2010: PHE - Created.
 *
 **************************************************************************/
fbe_edal_status_t 
processor_enclosure_access_setU64(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t componentType,
                              fbe_u64_t newValue)
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
        status = base_enclosure_access_setU64(generalDataPtr,
                                                attribute,
                                                componentType,
                                                newValue);                  
    }
    else
    {
        status = FBE_EDAL_STATUS_COMP_TYPE_UNSUPPORTED;           
    }

    return status;
}   // end of processor_enclosure_access_setU64

/*!*************************************************************************
 *                         @fn processor_enclosure_access_setU64
 **************************************************************************
 *  @brief
 *      This function updates the string data 
 *      for the processor enclosure component.
 *
 *  @param generalDataPtr - pointer to an object
 *  @param attribute - enum for attribute to get
 *  @param componentType - type of Component (PS, Drive, ...)
 *  @param newValue - pointer to the string to write
 *  @param length - string length to copy
 *
 *  @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status 
 *
 *  @version
 *    23-Feb-2010: PHE - Created.
 *
 **************************************************************************/
fbe_edal_status_t 
processor_enclosure_access_setStr(fbe_edal_general_comp_handle_t generalDataPtr,
                                    fbe_base_enclosure_attributes_t attribute,
                                    fbe_enclosure_component_types_t componentType,
                                    fbe_u32_t length,
                                    char *newValue)
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
        status = base_enclosure_access_setStr(generalDataPtr,
                                                attribute,
                                                componentType,
                                                length,
                                                newValue);                  
    }
    else
    {
        status = FBE_EDAL_STATUS_COMP_TYPE_UNSUPPORTED;           
    }

    return status;
}   // end of processor_enclosure_access_setStr
