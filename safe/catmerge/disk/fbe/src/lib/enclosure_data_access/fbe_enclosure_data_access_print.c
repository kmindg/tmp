/***************************************************************************
 * Copyright (C) EMC Corporation  2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 * Unpublished - all rights reserved under the copyright laws
 * of the United States
 ***************************************************************************/

/*!**************************************************************************
 *  @file fbe_enclosure_data_access_print.c
 ***************************************************************************
 *
 * DESCRIPTION:
 * @brief
 *      This module is a library that provides access functions (print)
 *      for Enclosure Object data.  The enclosure object has multiple
 *      classes where the data may reside.
 *
 *  @ingroup fbe_edal
 *
 *  FUNCTIONS:
 *
 *  LOCAL FUNCTIONS:
 *
 *  NOTES:
 *
 *  HISTORY:
 *      06-Aug-08   Joe Perry - Created
 *
 *
 **************************************************************************/

// Includes
#include "fbe_enclosure_data_access_public.h"
#include "fbe_enclosure_data_access_private.h"
#include "edal_eses_enclosure_data.h"
#include "edal_processor_enclosure_data.h"
#include "fbe_enclosure.h"
#include "fbe_base.h"
#include "fbe_trace.h"
#include "fbe_library_interface.h"
#include "generic_utils_lib.h"
#include "generic_utils.h"
#include "fbe_pe_types.h"
#include "fbe_peer_boot_utils.h"

// EDAL Globals
fbe_trace_level_t   edal_traceLevel = FBE_TRACE_LEVEL_INFO;
fbe_bool_t          edal_cliContext = FALSE;

#ifdef C4_INTEGRATED
fbe_bool_t          print_for_neo_fbecli = FALSE;
#endif /* C4_INTEGRATED - C4BUG - whatever is being done here probably should be runtime-config-all-cases-code */


//***********************  Local Prototypes ***********************************
void
enclosure_access_printComponentTypeData(fbe_enclosure_component_t *enclosureComponentTypeDataPtr, fbe_edal_trace_func_t trace_func);
void 
enclosure_access_printGeneralComponentInfo (fbe_encl_component_general_info_t *generalInfoPtr,
                                            fbe_bool_t printFullData,
                                            fbe_edal_trace_func_t trace_func);
void 
enclosure_access_printGeneralComponentStatus (fbe_encl_component_general_status_info_t *generalStatusPtr,
                                              fbe_edal_trace_func_t trace_func);
void 
enclosure_access_printGeneralPowerSupplyInfo (fbe_encl_power_supply_info_t *psInfoPtr, fbe_edal_trace_func_t trace_func);
void 
enclosure_access_printGeneralDriveInfo (fbe_encl_drive_slot_info_t *driveInfoPtr, fbe_edal_trace_func_t trace_func);
void 
enclosure_access_printGeneralCoolingInfo (fbe_encl_cooling_info_t *coolingInfoPtr, fbe_edal_trace_func_t trace_func);
void 
enclosure_access_printGeneralTempSensorInfo (fbe_encl_temp_sensor_info_t *tempSensorInfoPtr, fbe_edal_trace_func_t trace_func);
void 
enclosure_access_printSasConnectorInfo (fbe_eses_encl_connector_info_t *connectorInfoPtr, fbe_edal_trace_func_t trace_func);
void 
enclosure_access_printSasExpanderInfo (fbe_encl_component_general_info_t *generalInfoPtr, fbe_edal_trace_func_t trace_func);
void 
enclosure_access_printSasExpanderPhyInfo (fbe_eses_expander_phy_info_t *phyInfoPtr, fbe_edal_trace_func_t trace_func);
void
enclosure_access_printEsesTempSensorInfo (fbe_eses_temp_sensor_info_t *tempSensorInfoPtr, fbe_edal_trace_func_t trace_func);
void
enclosure_access_printEsesCoolingInfo (fbe_eses_cooling_info_t *coolingInfoPtr, fbe_edal_trace_func_t trace_func);
void 
enclosure_access_printBaseEnclosureInfo (fbe_base_enclosure_info_t *baseEnclDataPtr, fbe_edal_trace_func_t trace_func);
void 
enclosure_access_printSasEnclosureInfo (fbe_sas_enclosure_info_t *enclosureDataPtr, fbe_edal_trace_func_t trace_func);
void 
enclosure_access_printEsesEnclosureInfo (fbe_eses_enclosure_info_t *enclosureDataPtr,
                                         fbe_bool_t printFullData, fbe_edal_trace_func_t trace_func);
void 
enclosure_access_printEsesPowerSupplyInfo (fbe_eses_power_supply_info_t *psInfoPtr, fbe_edal_trace_func_t trace_func);
void 
enclosure_access_printEsesSpsInfo (fbe_eses_sps_info_t *spsInfoPtr, fbe_edal_trace_func_t trace_func);
void
enclosure_access_printPowerSupplyData(fbe_enclosure_component_t   *componentTypeDataPtr,
                                      fbe_enclosure_types_t enclosureType,
                                      fbe_u32_t componentIndex,
                                      fbe_bool_t printFullData, 
                                      fbe_edal_trace_func_t trace_func);
void
enclosure_access_printDriveData(fbe_enclosure_component_t   *componentTypeDataPtr,
                                fbe_enclosure_types_t enclosureType,
                                fbe_u32_t componentIndex,
                                fbe_bool_t printFullData, 
                                fbe_edal_trace_func_t trace_func);
void
enclosure_access_printCoolingData(fbe_enclosure_component_t   *componentTypeDataPtr,
                                  fbe_enclosure_types_t enclosureType,
                                  fbe_u32_t componentIndex,
                                  fbe_bool_t printFullData, 
                                  fbe_edal_trace_func_t trace_func);
void
enclosure_access_printTempSensorData(fbe_enclosure_component_t   *componentTypeDataPtr,
                                     fbe_enclosure_types_t enclosureType,
                                     fbe_u32_t componentIndex,
                                     fbe_bool_t printFullData, 
                                     fbe_edal_trace_func_t trace_func);
void
enclosure_access_printConnectorData(fbe_enclosure_component_t   *componentTypeDataPtr,
                                    fbe_enclosure_types_t enclosureType,
                                    fbe_u32_t componentIndex,
                                    fbe_bool_t printFullData, 
                                    fbe_edal_trace_func_t trace_func);
void
enclosure_access_printExpanderData(fbe_enclosure_component_t   *componentTypeDataPtr,
                                   fbe_enclosure_types_t enclosureType,
                                   fbe_u32_t componentIndex,
                                   fbe_bool_t printFullData, 
                                   fbe_edal_trace_func_t trace_func);
void
enclosure_access_printExpanderPhyData(fbe_enclosure_component_t   *componentTypeDataPtr,
                                      fbe_enclosure_types_t enclosureType,
                                      fbe_u32_t componentIndex,
                                      fbe_bool_t printFullData, 
                                      fbe_edal_trace_func_t trace_func);
void
enclosure_access_printEnclosureData(fbe_enclosure_component_t   *componentTypeDataPtr,
                                    fbe_enclosure_types_t enclosureType,
                                    fbe_u32_t componentIndex,
                                    fbe_bool_t printFullData, 
                                    fbe_edal_trace_func_t trace_func);
char*
enclosure_access_printComponentType(fbe_enclosure_component_types_t componentType);

void 
enclosure_access_printFwRevInfo(fbe_edal_fw_info_t *fw_rev_p, fbe_edal_trace_func_t trace_func);

void 
enclosure_access_printBaseLccData (fbe_base_lcc_info_t *baseLccDataPtr, fbe_edal_trace_func_t trace_func);
void 
enclosure_access_printEsesLccData (fbe_eses_lcc_info_t *lccDataPtr,
                                   fbe_bool_t printFullData, 
                                   fbe_edal_trace_func_t trace_func);
void
enclosure_access_printLccData(fbe_enclosure_component_t   *componentTypeDataPtr,
                              fbe_enclosure_types_t enclosureType,
                              fbe_u32_t componentIndex,
                              fbe_bool_t printFullData, 
                              fbe_edal_trace_func_t trace_func);
void
enclosure_access_printDisplayData(fbe_enclosure_component_t   *componentTypeDataPtr,
                                  fbe_enclosure_types_t enclosureType,
                                  fbe_u32_t componentIndex,
                                  fbe_bool_t printFullData, 
                                  fbe_edal_trace_func_t trace_func);

void
enclosure_access_printSasDriveInfo (fbe_eses_encl_drive_info_t *driveInfoPtr, fbe_edal_trace_func_t trace_func);

void 
enclosure_access_printEsesDisplayData (fbe_eses_display_info_t  *displayDataPtr, fbe_edal_trace_func_t trace_func);

void 
enclosure_access_printIoCompInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                  fbe_enclosure_types_t enclosureType,
                                  fbe_u32_t componentIndex,
                                  fbe_bool_t printFullData, 
                                  fbe_edal_trace_func_t trace_func);

void 
enclosure_access_printPeIoCompInfo(edal_pe_io_comp_info_t *peIoCompInfoPtr, fbe_edal_trace_func_t trace_func);

void
enclosure_access_printIoPortInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                     fbe_enclosure_types_t enclosureType,
                                     fbe_u32_t componentIndex,
                                     fbe_bool_t printFullData, 
                                     fbe_edal_trace_func_t trace_func);
void 
enclosure_access_printPeIoPortInfo(edal_pe_io_port_info_t *peIoPortInfoPtr, fbe_edal_trace_func_t trace_func);

void
enclosure_access_printMezzanineInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                     fbe_enclosure_types_t enclosureType,
                                     fbe_u32_t componentIndex,
                                     fbe_bool_t printFullData, 
                                     fbe_edal_trace_func_t trace_func);

void
enclosure_access_printCacheCardInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                     fbe_enclosure_types_t enclosureType,
                                     fbe_u32_t componentIndex,
                                     fbe_bool_t printFullData, 
                                     fbe_edal_trace_func_t trace_func);
void 
enclosure_access_printPeCacheCardInfo(edal_pe_cache_card_info_t *peCacheCardInfoPtr, 
                                     fbe_edal_trace_func_t trace_func);
void
enclosure_access_printDimmInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                     fbe_enclosure_types_t enclosureType,
                                     fbe_u32_t componentIndex,
                                     fbe_bool_t printFullData, 
                                     fbe_edal_trace_func_t trace_func);
void 
enclosure_access_printPeDimmInfo(edal_pe_dimm_info_t *peDimmInfoPtr, 
                                     fbe_edal_trace_func_t trace_func);

void
enclosure_access_printSSDInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                     fbe_enclosure_types_t enclosureType,
                                     fbe_u32_t componentIndex,
                                     fbe_bool_t printFullData, 
                                     fbe_edal_trace_func_t trace_func);
void 
enclosure_access_printPeSSDInfo(edal_pe_ssd_info_t *peSSDInfoPtr, 
                                     fbe_edal_trace_func_t trace_func);
  
void
enclosure_access_printPlatformInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                     fbe_enclosure_types_t enclosureType,
                                     fbe_u32_t componentIndex,
                                     fbe_bool_t printFullData, 
                                     fbe_edal_trace_func_t trace_func);

void 
enclosure_access_printPePlatformInfo(edal_pe_platform_info_t  *pePlatformInfoPtr, fbe_edal_trace_func_t trace_func);

void
enclosure_access_printMiscInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                     fbe_enclosure_types_t enclosureType,
                                     fbe_u32_t componentIndex,
                                     fbe_bool_t printFullData, 
                                     fbe_edal_trace_func_t trace_func);

void 
enclosure_access_printPeMiscInfo(edal_pe_misc_info_t  *peMiscInfoPtr, fbe_edal_trace_func_t trace_func);

void 
enclosure_access_printPePowerSupplyInfo (edal_pe_power_supply_info_t *pePowerSupplyInfoPtr, fbe_edal_trace_func_t trace_func);

void
enclosure_access_printPeCoolingInfo (edal_pe_cooling_info_t *coolingInfoPtr, fbe_edal_trace_func_t trace_func);

void 
enclosure_access_printMgmtModuleInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                  fbe_enclosure_types_t enclosureType,
                                  fbe_u32_t componentIndex,
                                  fbe_bool_t printFullData, 
                                  fbe_edal_trace_func_t trace_func);

void 
enclosure_access_printPeMgmtModuleInfo(edal_pe_mgmt_module_info_t *peMgmtModuleInfoPtr, fbe_edal_trace_func_t trace_func);

void 
enclosure_access_printFltRegInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                  fbe_enclosure_types_t enclosureType,
                                  fbe_u32_t componentIndex,
                                  fbe_bool_t printFullData, 
                                  fbe_edal_trace_func_t trace_func);

void
enclosure_access_printSuitcaseData(fbe_enclosure_component_t   *componentTypeDataPtr,
                                      fbe_enclosure_types_t enclosureType,
                                      fbe_u32_t componentIndex,
                                      fbe_bool_t printFullData, 
                                      fbe_edal_trace_func_t trace_func);

void 
enclosure_access_printPeSuitcaseInfo(edal_pe_suitcase_info_t *peSuitcaseInfoPtr, fbe_edal_trace_func_t trace_func);

void
enclosure_access_printBmcData(fbe_enclosure_component_t   *componentTypeDataPtr,
                                     fbe_enclosure_types_t enclosureType,
                                     fbe_u32_t componentIndex,
                                     fbe_bool_t printFullData, 
                                     fbe_edal_trace_func_t trace_func);

void 
enclosure_access_printPeBmcInfo(edal_pe_bmc_info_t *peBmcSubPtr, fbe_edal_trace_func_t trace_func);

void 
enclosure_access_printPeFltRegInfo(edal_pe_flt_exp_info_t *peFltExpInfoPtr, fbe_edal_trace_func_t trace_func);

void 
enclosure_access_printSlavePortInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                  fbe_enclosure_types_t enclosureType,
                                  fbe_u32_t componentIndex,
                                  fbe_bool_t printFullData, 
                                  fbe_edal_trace_func_t trace_func);
void 
enclosure_access_printTemperatureInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                      fbe_enclosure_types_t enclosureType,
                                      fbe_u32_t componentIndex,
                                      fbe_bool_t printFullData, 
                                      fbe_edal_trace_func_t trace_func);
void 
enclosure_access_printSpsInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                              fbe_enclosure_types_t enclosureType,
                              fbe_u32_t componentIndex,
                              fbe_bool_t printFullData, 
                              fbe_edal_trace_func_t trace_func);
void 
enclosure_access_printPeBatteryInfo(edal_pe_battery_info_t *peBatteryPtr, 
                                    fbe_edal_trace_func_t trace_func);
void
enclosure_access_printBatteryInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                  fbe_enclosure_types_t enclosureType,
                                  fbe_u32_t componentIndex,
                                  fbe_bool_t printFullData, 
                                  fbe_edal_trace_func_t trace_func);

void 
enclosure_access_printPeSlavePortInfo(edal_pe_slave_port_info_t *peSlavePortInfoPtr, fbe_edal_trace_func_t trace_func);
void enclosure_access_printPeTemperatureInfo(edal_pe_temperature_info_t *peTempPtr, fbe_edal_trace_func_t trace_func);
void enclosure_access_printPeSpsInfo(edal_pe_sps_info_t *peSpsPtr, fbe_edal_trace_func_t trace_func);

char * fbe_edal_print_AdditionalStatus(fbe_u8_t additionalStatus);
void
enclosure_access_printSscInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                    fbe_enclosure_types_t enclosureType,
                                    fbe_u32_t componentIndex,
                                    fbe_bool_t printFullData, 
                                    fbe_edal_trace_func_t trace_func);

void 
enclosure_access_printEsesSscInfo (fbe_eses_encl_ssc_info_t *sscInfoPtr, 
                                          fbe_edal_trace_func_t trace_func);


void fbe_edal_trace_func(fbe_edal_trace_context_t trace_context, const char * fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    if (edal_traceLevel == FBE_TRACE_LEVEL_INVALID)
    {
        edal_traceLevel = fbe_trace_get_default_trace_level();
    }
    if (FBE_TRACE_LEVEL_INFO <= edal_traceLevel)
    {
        fbe_trace_report(FBE_COMPONENT_TYPE_LIBRARY,
                         FBE_LIBRARY_ID_ENCL_DATA_ACCESS,
                         FBE_TRACE_LEVEL_INFO,
                         0, /* message_id */
                         fmt, 
                         args);
    }
    va_end(args);
}


/*!*************************************************************************
 *                         @fn fbe_edal_printAllComponentData
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace some or all of the Enclosure Object's
 *      Component data.
 *
 *  PARAMETERS:
 *  @param  compBlockPtr - The pointer to the first EDAL block.
 *  @param  trace_func - trace function.     
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *    16-Mar-2010: PHE - Modified for the component type which is allocated in multiple blocks.
 *
 **************************************************************************/
void
fbe_edal_printAllComponentData(void *compBlockPtr, fbe_edal_trace_func_t trace_func)
{
    fbe_u32_t componentType = 0;
   
    fbe_edal_printAllComponentBlockInfo(compBlockPtr);

    /* Loop through all the possible component types. */
    for (componentType = 0; componentType < FBE_ENCL_MAX_COMPONENT_TYPE; componentType ++)
    {
        fbe_edal_printSpecificComponentData(compBlockPtr,
                                        componentType,
                                        FBE_ENCL_COMPONENT_INDEX_ALL,
                                        TRUE,
                                        trace_func);
    }

    return;
}   // end of fbe_edal_printAllComponentData


/*!*************************************************************************
 *                         @fn fbe_edal_printFaultedComponentData
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace any faulted Enclosure Object's
 *      Component data.
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    10-Sep-2010: Joe Perry - Created
 *
 **************************************************************************/
void
fbe_edal_printFaultedComponentData(void *compBlockPtr, fbe_edal_trace_func_t trace_func)
{
    fbe_enclosure_component_block_t *baseCompBlockPtr;
    fbe_enclosure_component_t       *componentTypeDataPtr = NULL;
    fbe_u32_t                       componentIndex, index, componentCount;
    fbe_edal_status_t               edalStatus, faultedEdalStatus, addStatusEdalStatus;
    fbe_bool_t                      faulted;
    fbe_u8_t                        additionalStatus;

    baseCompBlockPtr = (fbe_enclosure_component_block_t *)compBlockPtr;

    // potentially loop through all the components
    for (componentIndex = 0; componentIndex < baseCompBlockPtr->numberComponentTypes; componentIndex++)
    {

        enclosure_access_getEnclosureComponentTypeData(baseCompBlockPtr, 
                                                    &componentTypeDataPtr);

        // loop through all of this component type
        edalStatus = fbe_edal_getSpecificComponentCount(baseCompBlockPtr,
                                                    componentIndex,
                                                    &componentCount);
        if (edalStatus != FBE_EDAL_STATUS_OK)
        {
            continue;
        }
        for (index = 0; index < componentCount; index++)
        {
            /*
             * Check for Faults
             */
            // check the faulted attribute
            faultedEdalStatus = fbe_edal_getBool(compBlockPtr,
                                      FBE_ENCL_COMP_FAULTED,
                                      componentIndex,
                                      index,
                                      &faulted);
            // check Additional Status
            addStatusEdalStatus = fbe_edal_getU8(compBlockPtr,
                                      FBE_ENCL_COMP_ADDL_STATUS,
                                      componentIndex,
                                      index,
                                      &additionalStatus);
            if ((faultedEdalStatus == FBE_EDAL_STATUS_OK) && 
                (addStatusEdalStatus == FBE_EDAL_STATUS_OK))
            {
                // check if component is faulted
                if ((additionalStatus != SES_STAT_CODE_OK) &&
                   (additionalStatus != SES_STAT_CODE_NOT_INSTALLED))
                {
                    trace_func(NULL, "\t%s %d, Faulted %d, Additional Status : %s\n",
                               enclosure_access_printComponentType(componentIndex),
                               index,
                               faulted,
                               fbe_edal_print_AdditionalStatus(additionalStatus));
                }
    
            }
        }   // end of for index

        // go to next component type
        componentTypeDataPtr++;

    }   // end of for componentIndex


}   // end of fbe_edal_printFaultedComponentData


/*!*************************************************************************
 *                         @fn fbe_edal_printSpecificComponentData
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace some or all of the Enclosure Object's
 *      Component data.
 *
 *  PARAMETERS:
 *      @param  compBlockPtr   - pointer to Component Block Data
 *      @param  componentType  - type of component
 *      @param  componentIndex - index to a specific component
 *      @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                               updated component data
 *      @param  trace_func     - EDAL trace function
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    10-Feb-2009: Joe Perry - Created
 *    16-Mar-2010: PHE - Modified for the component type which is allocated in multiple blocks.
 *
 **************************************************************************/
void
fbe_edal_printSpecificComponentData(void *compBlockPtr,
                                    fbe_enclosure_component_types_t componentType,
                                    fbe_u32_t componentIndex,
                                    fbe_bool_t printFullData,
                                    fbe_edal_trace_func_t trace_func)
{
    fbe_enclosure_component_block_t *baseCompBlockPtr;
    fbe_u32_t component_count = 0;
    fbe_edal_status_t edal_status = FBE_EDAL_STATUS_OK;

    baseCompBlockPtr = (fbe_enclosure_component_block_t *)compBlockPtr;

    if(componentType >= FBE_ENCL_MAX_COMPONENT_TYPE)
    {
        trace_func (NULL,"Error - invalid component type 0x%x.\n", componentType);
    }
    else 
    {
        edal_status = fbe_edal_getSpecificComponentCount(baseCompBlockPtr, componentType, &component_count);

        if((edal_status == FBE_EDAL_STATUS_OK) && (component_count > 0))
        {
            if((componentIndex < component_count)||
               (componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL))
            {       
                enclosure_access_printSpecificComponentDataInAllBlocks(baseCompBlockPtr,
                                                    componentType,                                                  
                                                    componentIndex,
                                                    printFullData,
                                                    trace_func);
            }
            else
            { 
                enclosure_access_log_warning("%s, comptype %d compIndex %d out of range (max index %d)\n", 
                    __FUNCTION__, componentType, componentIndex, component_count - 1);

            }
        }
    }
  
    return;

}   // end of fbe_edal_printSpecificComponentData


/*!*************************************************************************
 *                         @fn fbe_edal_printAllComponentBlockInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function generates reports for edal blocks
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    23-Dec-2008: NC - Created
 *
 **************************************************************************/
void fbe_edal_printAllComponentBlockInfo(void *compBlockPtr)
{
    fbe_enclosure_component_block_t *local_block;

    local_block = (fbe_enclosure_component_block_t *)compBlockPtr;
    while (local_block != NULL)
    {
        enclosure_access_log_debug("EDAL block 0x%p, used %d entries, %d bytes left, block size %d bytes, generationCount: %d\n", 
                                local_block, 
                                local_block->numberComponentTypes, 
                                local_block->availableDataSpace,
                                local_block->blockSize,
                                local_block->generation_count);
        local_block = local_block->pNextBlock;
    }
} // end of fbe_edal_printAllComponentBlockInfo

// jap
#if FALSE
/**************************************************************************
 *                         @fn enclosure_access_printUpdatedComponentData
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace the Enclosure Object's Component data
 *      for components that have been updated.
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
void
enclosure_access_printUpdatedComponentData(fbe_enclosure_component_block_t *enclosureComponentTypeBlock,
                                           fbe_enclosure_types_t enclosureType,
                                           fbe_u32_t currentChangeCount,
                                           fbe_edal_trace_func_t trace_func)
{
    fbe_status_t                status;
    fbe_u32_t                   componentIndex;
    fbe_u32_t                   updatedComponentCount;
    fbe_bool_t                  stateChangeDetected;
    fbe_enclosure_component_t   *component_data;
    fbe_enclosure_component_block_t *localCompBlockPtr;

    /* caller of this function should log the enclosure number first */
    localCompBlockPtr = enclosureComponentTypeBlock; 
    while (localCompBlockPtr!=NULL)
    {
        status = enclosure_access_stateChangeDetected(localCompBlockPtr,
                                                    currentChangeCount,
                                                    &updatedComponentCount);
        if (status == FBE_EDAL_STATUS_OK)
        {
            if (updatedComponentCount > 0)
            {
                trace_func(NULL, "%d components have been updated in enclosure \n",
                    updatedComponentCount);

                // potentially loop through all the updated components
                for (componentIndex = 0,
                    component_data = edal_calculateComponentTypeDataStart(localCompBlockPtr);
                    componentIndex < localCompBlockPtr->numberComponentTypes; 
                    componentIndex++ , component_data ++)
                {
                    status = enclosure_access_componentStateChangeDetected(component_data,
                                                                        currentChangeCount,
                                                                        &stateChangeDetected);
                    if ((status == FBE_EDAL_STATUS_OK) && stateChangeDetected)
                    {
                        enclosure_access_printSpecificComponentData(component_data, enclosureType, trace_func);
                        --updatedComponentCount;
                    }

                    // see if all updated components processed
                    if (updatedComponentCount <= 0)
                    {
                        break;
                    }
                }   // end of for loop
            }
            else
            {
                trace_func(NULL, "No components have been updated\n");
            }
        }
        else
        {
            trace_func(NULL, "Cannot find updated components, bad status %d\n", status);
        }

        // check if there's a next block
        localCompBlockPtr = localCompBlockPtr->pNextBlock;
    }
}   // end of enclosure_access_printUpdatedComponentData
#endif
// jap

/*!*************************************************************************
 *  @fn enclosure_access_printSpecificComponentDataInAllBlocks
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace the specified component(s) in all the EDAL block chains.
 *
 *  PARAMETERS:
 *      @param  baseCompBlockPtr  - pointer to the first EDAL block in the chain.
 *      @param  componentType  - type of component
 *      @param  componentIndex - index to a specific component
 *      @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                               updated component data
 *      @param  trace_func     - EDAL trace function
 *      
 *
 *  RETURN VALUES/ERRORS:
 *       none
 *      
 *
 *  NOTES:
 *       caller of this function should log the enclosure number first.
 *
 *  HISTORY:
 *    11-Mar-2010: PHE - Created.
 *
 **************************************************************************/
void
enclosure_access_printSpecificComponentDataInAllBlocks(fbe_enclosure_component_block_t   *baseCompBlockPtr,
                                            fbe_enclosure_component_types_t componentType,
                                            fbe_u32_t componentIndex,
                                            fbe_bool_t printFullData,
                                            fbe_edal_trace_func_t trace_func)
{
    fbe_enclosure_component_block_t   * localCompBlockPtr = NULL;
    fbe_enclosure_component_t         * componentTypeDataPtr = NULL;
    fbe_edal_status_t                   edal_status = FBE_EDAL_STATUS_OK;
    
    localCompBlockPtr = baseCompBlockPtr;

    while (localCompBlockPtr!=NULL)
    {
        if(componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL)
        {
            edal_status = enclosure_access_getSpecifiedComponentTypeDataInLocalBlock(localCompBlockPtr,
                                                       &componentTypeDataPtr,
                                                       componentType);
            
        }
        else
        {
            edal_status = enclosure_access_getSpecifiedComponentTypeDataWithIndex(localCompBlockPtr,
                                                       &componentTypeDataPtr,
                                                       componentType,
                                                       componentIndex);

        }

        if((edal_status == FBE_EDAL_STATUS_OK) && 
            (componentTypeDataPtr->maxNumberOfComponents > 0))
        {
            enclosure_access_printSpecificComponentData(componentTypeDataPtr,
                                            localCompBlockPtr->enclosureType,
                                            componentIndex,
                                            printFullData,
                                            trace_func);
        }

        // check if there's a next block
        localCompBlockPtr = localCompBlockPtr->pNextBlock;
    }// end of while loop

    return;

}   // end of enclosure_access_printSpecificComponentDataInAllBlocks

/*!*************************************************************************
 *                         @fn enclosure_access_printSpecificComponentData
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace the specified component(s) in the local block.
 *      Component.
 *
 *  PARAMETERS:
 *      @param  componentTypeDataPtr  - pointer to Component Data
 *      @param  enclosureType  - type of enclosure
 *      @param  componentIndex - index to a specific component
 *      @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                               updated component data
 *      @param  trace_func     - EDAL trace function
 *      
 *
 *  RETURN VALUES/ERRORS:
 *       none
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
void
enclosure_access_printSpecificComponentData(fbe_enclosure_component_t   *componentTypeDataPtr,
                                            fbe_enclosure_types_t enclosureType,
                                            fbe_u32_t componentIndex,
                                            fbe_bool_t printFullData,
                                            fbe_edal_trace_func_t trace_func)
{
    if ((componentIndex != FBE_ENCL_COMPONENT_INDEX_ALL) && 
            ((componentIndex < componentTypeDataPtr->firstComponentIndex)  ||
            (componentIndex >= componentTypeDataPtr->maxNumberOfComponents)))
    {
        enclosure_access_log_debug("EDAL: comp type %d, invalid component index %d , first index %d, max %d.\n", 
                                componentTypeDataPtr->componentType, 
                                componentIndex, 
                                componentTypeDataPtr->firstComponentIndex,
                                componentTypeDataPtr->maxNumberOfComponents);   
        return;
    }

    if (componentTypeDataPtr != NULL )
    {
        // TO DO: move to caller
//        trace_func(NULL, "%s component data for enclosure 0x%x\n",
//            enclosure_access_printComponentType(componentType), enclosurePtr);
        if (printFullData)
        {
            enclosure_access_printComponentTypeData(componentTypeDataPtr, trace_func);
        }

        switch (componentTypeDataPtr->componentType)
        {
        case FBE_ENCL_POWER_SUPPLY:
            enclosure_access_printPowerSupplyData(componentTypeDataPtr, 
                                                  enclosureType,
                                                  componentIndex,
                                                  printFullData,
                                                  trace_func);
            break;
        case FBE_ENCL_COOLING_COMPONENT:
            enclosure_access_printCoolingData(componentTypeDataPtr, 
                                              enclosureType,
                                              componentIndex,
                                              printFullData,
                                              trace_func);
            break;
        case FBE_ENCL_DRIVE_SLOT:
            enclosure_access_printDriveData(componentTypeDataPtr, 
                                            enclosureType,
                                            componentIndex,
                                            printFullData,
                                            trace_func);
            break;
        case FBE_ENCL_TEMP_SENSOR:
            enclosure_access_printTempSensorData(componentTypeDataPtr, 
                                                 enclosureType,
                                                 componentIndex,
                                                 printFullData,
                                                 trace_func);
            break;
        case FBE_ENCL_CONNECTOR:
            enclosure_access_printConnectorData(componentTypeDataPtr, 
                                                enclosureType,
                                                componentIndex,
                                                printFullData,
                                                trace_func);
            break;
        case FBE_ENCL_EXPANDER:
            enclosure_access_printExpanderData(componentTypeDataPtr, 
                                               enclosureType,
                                               componentIndex,
                                               printFullData,
                                               trace_func);
            break;
        case FBE_ENCL_EXPANDER_PHY:
            enclosure_access_printExpanderPhyData(componentTypeDataPtr, 
                                                  enclosureType,
                                                  componentIndex,
                                                  printFullData,
                                                  trace_func);
            break;
        case FBE_ENCL_ENCLOSURE:
            enclosure_access_printEnclosureData(componentTypeDataPtr, 
                                                enclosureType,
                                                componentIndex,
                                                printFullData,
                                                trace_func);
            break;
        case FBE_ENCL_LCC:
            enclosure_access_printLccData(componentTypeDataPtr, 
                                          enclosureType,
                                          componentIndex,
                                          printFullData,
                                          trace_func);
            break;
        case FBE_ENCL_DISPLAY:
            enclosure_access_printDisplayData(componentTypeDataPtr, 
                                              enclosureType,
                                              componentIndex,
                                              printFullData,
                                              trace_func);
            break;

        case FBE_ENCL_IO_MODULE:
        case FBE_ENCL_BEM:
        case FBE_ENCL_MEZZANINE:
            enclosure_access_printIoCompInfo(componentTypeDataPtr, 
                                              enclosureType,
                                              componentIndex,
                                              printFullData,
                                              trace_func);
            break;

        case FBE_ENCL_IO_PORT:
            enclosure_access_printIoPortInfo(componentTypeDataPtr, 
                                              enclosureType,
                                              componentIndex,
                                              printFullData,
                                              trace_func);
            break;

        case FBE_ENCL_PLATFORM:
            enclosure_access_printPlatformInfo(componentTypeDataPtr, 
                                              enclosureType,
                                              componentIndex,
                                              printFullData,
                                              trace_func);
            break;

        case FBE_ENCL_CACHE_CARD:
            enclosure_access_printCacheCardInfo(componentTypeDataPtr, 
                                              enclosureType,
                                              componentIndex,
                                              printFullData,
                                              trace_func);
            break;

        case FBE_ENCL_DIMM:
            enclosure_access_printDimmInfo(componentTypeDataPtr, 
                                              enclosureType,
                                              componentIndex,
                                              printFullData,
                                              trace_func);
            break;
        case FBE_ENCL_SSD:
            enclosure_access_printSSDInfo(componentTypeDataPtr, 
                                              enclosureType,
                                              componentIndex,
                                              printFullData,
                                              trace_func);
            break;

        case FBE_ENCL_MISC:
            enclosure_access_printMiscInfo(componentTypeDataPtr, 
                                              enclosureType,
                                              componentIndex,
                                              printFullData,
                                              trace_func);
            break;

        case FBE_ENCL_MGMT_MODULE:
            enclosure_access_printMgmtModuleInfo(componentTypeDataPtr, 
                                              enclosureType,
                                              componentIndex,
                                              printFullData,
                                              trace_func);
            break;

        case FBE_ENCL_SUITCASE:
            enclosure_access_printSuitcaseData(componentTypeDataPtr, 
                                              enclosureType,
                                              componentIndex,
                                              printFullData,
                                              trace_func);
            break;

        case FBE_ENCL_BMC:
            enclosure_access_printBmcData(componentTypeDataPtr, 
                                              enclosureType,
                                              componentIndex,
                                              printFullData,
                                              trace_func);
            break;

        case FBE_ENCL_FLT_REG:
            enclosure_access_printFltRegInfo(componentTypeDataPtr, 
                                              enclosureType,
                                              componentIndex,
                                              printFullData,
                                              trace_func);
            break;

        case FBE_ENCL_SLAVE_PORT:
            enclosure_access_printSlavePortInfo(componentTypeDataPtr, 
                                              enclosureType,
                                              componentIndex,
                                              printFullData,
                                              trace_func);
            break;
        case FBE_ENCL_TEMPERATURE:
            enclosure_access_printTemperatureInfo(componentTypeDataPtr, 
                                                  enclosureType,
                                                  componentIndex,
                                                  printFullData,
                                                  trace_func);
            break;
        case FBE_ENCL_SPS:
            enclosure_access_printSpsInfo(componentTypeDataPtr, 
                                          enclosureType,
                                          componentIndex,
                                          printFullData,
                                          trace_func);
            break;
        case FBE_ENCL_BATTERY:
            enclosure_access_printBatteryInfo(componentTypeDataPtr, 
                                             enclosureType,
                                             componentIndex,
                                             printFullData,
                                             trace_func);
            break;
        case FBE_ENCL_SSC:
            enclosure_access_printSscInfo(componentTypeDataPtr, 
                                             enclosureType,
                                             componentIndex,
                                             printFullData,
                                             trace_func);
            break;

        case FBE_ENCL_INVALID_COMPONENT_TYPE:
            break;

        default:
            break;
        }
    }
    else
    {
        trace_func (NULL,"Error - NULL componentTypeDataPtr for this enclosure 0x%x\n", enclosureType);
    }

}   // end of enclosure_access_printSpecificComponentData

/*!*************************************************************************
 *                         @fn enclosure_access_printComponentTypeData
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will 
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
void
enclosure_access_printComponentTypeData(fbe_enclosure_component_t *enclosureComponentTypeDataPtr, fbe_edal_trace_func_t trace_func)
{
    trace_func(NULL, "%s Component Type Data:\n", 
        enclosure_access_printComponentType(enclosureComponentTypeDataPtr->componentType));
    trace_func(NULL, "   Num : %d, CompSize : %d, 1stCompOff : %d (0x%x)\n", 
        enclosureComponentTypeDataPtr->maxNumberOfComponents,
        enclosureComponentTypeDataPtr->componentSize,
        enclosureComponentTypeDataPtr->firstComponentStartOffset,
        enclosureComponentTypeDataPtr->firstComponentStartOffset);
    trace_func(NULL, "  1stCompIndex: %d, OverallStatus : %d, OverallStChgCnt : %d\n", 
        enclosureComponentTypeDataPtr->firstComponentIndex,
        enclosureComponentTypeDataPtr->componentOverallStatus,
        enclosureComponentTypeDataPtr->componentOverallStateChangeCount);
}   // end of enclosure_access_printComponentTypeData


/*!*************************************************************************
 *                         @fn enclosure_access_printGeneralComponentInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace some or all of the Enclosure Object's
 *      Component data.
 *
 *  PARAMETERS:
 *      @param  generalInfoPtr - pointer to general Component Data
 *      @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                               updated component data
 *      @param  trace_func     - EDAL trace function
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
void 
enclosure_access_printGeneralComponentInfo (fbe_encl_component_general_info_t *generalInfoPtr,
                                            fbe_bool_t printFullData, 
                                            fbe_edal_trace_func_t trace_func)
{
#ifdef C4_INTEGRATED
    if (print_for_neo_fbecli)
    {
        if (!generalInfoPtr->generalFlags.componentInserted) 
            trace_func((fbe_edal_trace_context_t)0x01234567, "REMOVED");
        else if (generalInfoPtr->generalFlags.componentFaulted)
            trace_func((fbe_edal_trace_context_t)0x01234567, "FAULTED");
        else
            trace_func((fbe_edal_trace_context_t)0x01234567, "OK");
        return;
    }
#endif /* C4_INTEGRATED - C4BUG - whatever is being done here probably should be runtime-config-all-cases-code */
    trace_func(NULL, "  General Component Flags\n");
    trace_func(NULL, "    Insrtd : %d, InsrtedPrior %d, Fltd : %d, PwrdOff : %d\n", 
        generalInfoPtr->generalFlags.componentInserted,
        generalInfoPtr->generalFlags.componentInsertedPriorConfig,
        generalInfoPtr->generalFlags.componentFaulted,
        generalInfoPtr->generalFlags.componentPoweredOff);
    trace_func(NULL, "    FltLEDOn : %d, Mrkd : %d, TrnOnFltLED(W) : %d, Mrk(W) : %d\n", 
        generalInfoPtr->generalFlags.componentFaultLedOn,
        generalInfoPtr->generalFlags.componentMarked,
        generalInfoPtr->generalFlags.turnComponentFaultLedOn,
        generalInfoPtr->generalFlags.markComponent);
   
        enclosure_access_printGeneralComponentStatus(&(generalInfoPtr->generalStatus), trace_func);
 
}   // end of enclosure_access_printGeneralComponentInfo

/*!*************************************************************************
 *                         @fn enclosure_access_printGeneralComponentStatus
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace the Enclosure Component's General Status.
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    17-Sep-2008: Joe Perry - Created
 *
 **************************************************************************/
void 
enclosure_access_printGeneralComponentStatus (fbe_encl_component_general_status_info_t *generalStatusPtr, 
                                              fbe_edal_trace_func_t trace_func)
{
    trace_func(NULL, "  General Component Status\n");
    trace_func(NULL, "    Canary : 0x%x, StChgCnt : %d, WrUpd : %d, WrSnt : %d\n", 
        generalStatusPtr->componentCanary,
        generalStatusPtr->readWriteFlags.componentStateChange,
        generalStatusPtr->readWriteFlags.writeDataUpdate,
        generalStatusPtr->readWriteFlags.writeDataSent);
    trace_func(NULL, "     emcWrUpd : %d, emcWrSnt : %d\n", 
        generalStatusPtr->readWriteFlags.emcEnclCtrlWriteDataUpdate,
        generalStatusPtr->readWriteFlags.emcEnclCtrlWriteDataSent);
    trace_func(NULL, "    AddStatus : %s, CompStatValid : 0x%x\n", 
        fbe_edal_print_AdditionalStatus(generalStatusPtr->addlComponentStatus),
        generalStatusPtr->readWriteFlags.componentStatusValid);

}   // end of enclosure_access_printGeneralComponentStatus

/*!*************************************************************************
 *  @fn enclosure_access_printPeGeneralComponentInfo
 **************************************************************************
 *  @brief
 *      This function prints out the processor enclosure general component info.
 *
 *  @param  generalInfoPtr - pointer to general Component Data
 *  @param  printFullData
 *  @param  trace_func     - EDAL trace function
 *
 *  @return none      
 *
 *  NOTES:
 *
 *  @version
 *    23-Feb-2010: PHE - Created.
 *
 **************************************************************************/
void 
enclosure_access_printPeGeneralComponentInfo(fbe_encl_component_general_info_t *generalInfoPtr, 
                                             fbe_bool_t printFullData,
                                             fbe_edal_trace_func_t trace_func)
{
    fbe_encl_component_general_status_info_t *generalStatusPtr = NULL;
        
    if (printFullData)
    {
        generalStatusPtr = &generalInfoPtr->generalStatus;
        
        trace_func(NULL, "  General Component Status\n");
        trace_func(NULL, "    Canary : 0x%x, componentStateChange : %d\n", 
            generalStatusPtr->componentCanary,
            generalStatusPtr->readWriteFlags.componentStateChange);
    }

    return;
}   // end of enclosure_access_printPeGeneralComponentInfo

/*!*************************************************************************
 *                         @fn enclosure_access_printSuitcaseData
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace PE suitcase component data.
 *
 *  PARAMETERS:
 *      @param  componentTypeDataPtr  - pointer to Component Data
 *      @param  enclosureType  - type of enclosure
 *      @param  componentIndex - index to a specific component
 *      @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                               updated component data
 *      @param  trace_func     - EDAL trace function
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    15-Apr-2010: Arun S - Created
 *
 **************************************************************************/
void
enclosure_access_printSuitcaseData(fbe_enclosure_component_t   *componentTypeDataPtr,
                                      fbe_enclosure_types_t enclosureType,
                                      fbe_u32_t componentIndex,
                                      fbe_bool_t printFullData, 
                                      fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t  generalDataPtr = NULL;
    edal_base_suitcase_info_t       *baseSuitcaseInfoPtr = NULL;
    edal_pe_suitcase_info_t         *peSuitcaseInfoPtr = NULL;
    fbe_u32_t                       index = 0;

    if (componentTypeDataPtr->componentType != FBE_ENCL_SUITCASE)
    {
        return;
    }

    trace_func(NULL, "\n");

    // loop through suitcase
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, "  Suitcase %d\n", index);
            }
            else
            {
                trace_func(NULL, "State Change to Suitcase %d\n", index);
            }
            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);

            baseSuitcaseInfoPtr = (edal_base_suitcase_info_t *)generalDataPtr;
            enclosure_access_printGeneralComponentInfo(&(baseSuitcaseInfoPtr->generalComponentInfo),
                                                       printFullData, trace_func);

            peSuitcaseInfoPtr = (edal_pe_suitcase_info_t *)generalDataPtr;
            enclosure_access_printPeSuitcaseInfo(peSuitcaseInfoPtr, trace_func);         
        }        
    }

    return;
}   // end of enclosure_access_printSuitcaseData

/*!*************************************************************************
 *                @fn enclosure_access_printPeSuitcaseInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace some or all of the Suitcase data.
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    15-Apr-2010: Arun S - Created
 *
 **************************************************************************/
void 
enclosure_access_printPeSuitcaseInfo(edal_pe_suitcase_info_t *peSuitcaseInfoPtr, 
                                          fbe_edal_trace_func_t trace_func)
{
    edal_pe_suitcase_sub_info_t * pPeSuitcaseSubInfo = NULL;
    fbe_board_mgmt_suitcase_info_t * pExtSuitcaseInfo = NULL;

    pPeSuitcaseSubInfo = &peSuitcaseInfoPtr->peSuitcaseSubInfo;
    pExtSuitcaseInfo = &pPeSuitcaseSubInfo->externalSuitcaseInfo;

    trace_func(NULL, "  Processor Encl Suitcase Info\n");
    trace_func(NULL, "    associatedSp : %d, suitcaseID : %d, isLocalFru : %d\n", 
        pExtSuitcaseInfo->associatedSp,
        pExtSuitcaseInfo->suitcaseID,
        pExtSuitcaseInfo->isLocalFru);

    trace_func(NULL, "    xactStatus : 0x%x, envInterfaceStatus : 0x%x\n", 
        pExtSuitcaseInfo->transactionStatus,
        pExtSuitcaseInfo->envInterfaceStatus);

    trace_func(NULL, "    TempSensor : %d, Tap12Vmissing : %d\n", 
        pExtSuitcaseInfo->tempSensor,
        pExtSuitcaseInfo->Tap12VMissing);

    trace_func(NULL, "    ShtdnWarn : %d, FanPackFailure : %d\n", 
        pExtSuitcaseInfo->shutdownWarning,
        pExtSuitcaseInfo->fanPackFailure);

    trace_func(NULL, "    AmbientOvertempFault: %d, ambientOvertempWarning: %d\n", 
        pExtSuitcaseInfo->ambientOvertempFault,
        pExtSuitcaseInfo->ambientOvertempWarning);
    
    trace_func(NULL, "    isCPUFaultRegFail : %s, hwErrMonFault : %s\n", 
               BOOLEAN_TO_TEXT(pExtSuitcaseInfo->isCPUFaultRegFail),
               BOOLEAN_TO_TEXT(pExtSuitcaseInfo->hwErrMonFault));  
   
    return;
}   // end of enclosure_access_printPeSuitcaseInfo

/*!*************************************************************************
 *                         @fn enclosure_access_printGeneralPowerSupplyInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace some or all of the Enclosure Object's
 *      Component data.
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
void 
enclosure_access_printGeneralPowerSupplyInfo (fbe_encl_power_supply_info_t *psInfoPtr, 
                                              fbe_edal_trace_func_t trace_func)
{
    trace_func(NULL, "  General Power Supply Info\n");
    trace_func(NULL, "    AcFail : %d, DcDtctd : %d\n",
        psInfoPtr->psFlags.acFail,
        psInfoPtr->psFlags.dcDetected);
    trace_func(NULL, "    psSupportd : %d, psSideId %d\n", 
        psInfoPtr->psFlags.psSupported,
        psInfoPtr->psData.psSideId);
}   // end of enclosure_access_printGeneralPowerSupplyInfo


/*!*************************************************************************
 *                         @fn enclosure_access_printGeneralDriveInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace some or all of the Enclosure Object's
 *      Component data.
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
void 
enclosure_access_printGeneralDriveInfo (fbe_encl_drive_slot_info_t *driveInfoPtr, fbe_edal_trace_func_t trace_func)
{
    trace_func(NULL, "  General Drive Info\n");
    trace_func(NULL, "    DrvType : %d, InsrtMskd  : %d, Bypassed : %d, LoggedIn : %d\n", 
        driveInfoPtr->enclDriveType,
        driveInfoPtr->driveFlags.enclDriveInsertMasked,
        driveInfoPtr->driveFlags.enclDriveBypassed,
        driveInfoPtr->driveFlags.enclDriveLoggedIn);
    trace_func(NULL, "    Bypass(W) : %d, DevOff(W) : %d, DevOffReason: %d\n", 
        driveInfoPtr->driveFlags.bypassEnclDrive,
        driveInfoPtr->driveFlags.driveDeviceOff,
        driveInfoPtr->driveDeviceOffReason);
    trace_func(NULL, "    PowerCyclePending : %d, PowerCycleCompleted : %d\n", 
        driveInfoPtr->driveFlags.drivePowerCyclePending,
        driveInfoPtr->driveFlags.drivePowerCycleCompleted);
    trace_func(NULL, "    BatteryBacked : %d\n", 
        driveInfoPtr->driveFlags.driveBatteryBacked);
}   // end of enclosure_access_printGeneralDriveInfo


/*!*************************************************************************
 *                         @fn enclosure_access_printGeneralCoolingInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace some or all of the Enclosure Object's
 *      Component data.
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    15-Sep-2008: Joe Perry - Created
 *
 **************************************************************************/
void 
enclosure_access_printGeneralCoolingInfo (fbe_encl_cooling_info_t *coolingInfoPtr, fbe_edal_trace_func_t trace_func)
{
    trace_func(NULL, "  General Cooling Info\n");
    trace_func(NULL, "    MultiFanFlt : %d, coolSideId : %d, coolSubtype : %d\n", 
        coolingInfoPtr->coolingFlags.multiFanFault,
        coolingInfoPtr->coolingData.coolingSideId,
        coolingInfoPtr->coolingData.coolingSubtype);
    return;

}   // end of enclosure_access_printGeneralCoolingInfo


/*!*************************************************************************
 *                         @fn enclosure_access_printGeneralTempSensorInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace some or all of the Enclosure Object's
 *      Component data.
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    15-Sep-2008: Joe Perry - Created
 *
 **************************************************************************/
void 
enclosure_access_printGeneralTempSensorInfo (fbe_encl_temp_sensor_info_t *tempSensorInfoPtr, 
                                             fbe_edal_trace_func_t trace_func)
{
    trace_func(NULL, "  General Temperature Sensor Info\n");
    trace_func(NULL, "    OTFail : %d, OTWarn : %d, SideId : %d, Subtype : %d, \n", 
        tempSensorInfoPtr->tempSensorFlags.overTempFailure,
        tempSensorInfoPtr->tempSensorFlags.overTempWarning,
        tempSensorInfoPtr->tempSensorData.tempSensorSideId,
        tempSensorInfoPtr->tempSensorData.tempSensorSubtype);

    if((tempSensorInfoPtr->generalComponentInfo.generalStatus.readWriteFlags.componentStatusValid)
       &&
       (tempSensorInfoPtr->generalComponentInfo.generalFlags.componentInserted))
    {
#ifdef C4_INTEGRATED
    if (print_for_neo_fbecli)
    {
        if (tempSensorInfoPtr->tempSensorFlags.tempValid)
            trace_func((fbe_edal_trace_context_t)0x01234567, "%d", tempSensorInfoPtr->tempSensorData.temp / 100);
        else
            trace_func((fbe_edal_trace_context_t)0x01234567, "00");
    }
#endif /* C4_INTEGRATED - C4BUG - whatever is being done here probably should be runtime-config-all-cases-code */
        trace_func(NULL, "  TempValid: %d, Temp(2s complement): %d, \n", 
           tempSensorInfoPtr->tempSensorFlags.tempValid, tempSensorInfoPtr->tempSensorData.temp);
    }
#ifdef C4_INTEGRATED
    else
    {
        if (print_for_neo_fbecli)
        {
            trace_func((fbe_edal_trace_context_t)0x01234567, "00");
        }
    }
#endif /* C4_INTEGRATED - C4BUG - whatever is being done here probably should be runtime-config-all-cases-code */
    return;

}   // end of enclosure_access_printGeneralTempSensorInfo


/*!*************************************************************************
 *                         @fn enclosure_access_printSasConnectorInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace some or all of the Enclosure Object's
 *      Component data.
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    15-Sep-2008: Joe Perry - Created
 *
 **************************************************************************/
void 
enclosure_access_printSasConnectorInfo (fbe_eses_encl_connector_info_t *connectorInfoPtr, 
                                        fbe_edal_trace_func_t trace_func)
{
    trace_func(NULL, "  SAS Connector Info\n");
    trace_func(NULL, "    InsrtMskd  : %d, Disabled : %d, Degraded : %d, isEntire : %d, isLocal : %d, loggedIn : %d illegal : %d\n",
        connectorInfoPtr->connectorFlags.connectorInsertMasked,
        connectorInfoPtr->connectorFlags.connectorDisabled,
        connectorInfoPtr->connectorFlags.connectorDegraded,
        connectorInfoPtr->connectorFlags.isEntireConnector,
        connectorInfoPtr->connectorFlags.isLocalConnector,
        connectorInfoPtr->connectorFlags.deviceLoggedIn,
        connectorInfoPtr->connectorFlags.illegalCable); 
    trace_func(NULL, "    SasAddr Contain:0x%llX, Attach:0x%llX\n", 
        (unsigned long long)connectorInfoPtr->expanderSasAddress,
        (unsigned long long)connectorInfoPtr->attachedSasAddress);
    trace_func(NULL, "    PrimPort : %d, ConnCompType : %d\n", 
        connectorInfoPtr->connectorFlags.primaryPort,
        connectorInfoPtr->connectedComponentType);
    trace_func(NULL, "    ElemIndx : %d, PhyIndx : %d, ContainIndx : %d\n", 
        connectorInfoPtr->connectorElementIndex,
        connectorInfoPtr->connectorPhyIndex,
        connectorInfoPtr->connectorContainerIndex);

    trace_func(NULL, "    ConnectorId : %d, connectorType : 0x%X, attachedSubEnclId : %d, location : %d\n",
        connectorInfoPtr->ConnectorId,
        connectorInfoPtr->connectorType,
        connectorInfoPtr->attachedSubEncId,
        connectorInfoPtr->connectorLocation);

}   // end of enclosure_access_printSasConnectorInfo


/*!*************************************************************************
 *                         @fn enclosure_access_printSasExpanderInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace some or all of the Enclosure Object's
 *      Component data.
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    15-Sep-2008: Joe Perry - Created
 *
 **************************************************************************/
void 
enclosure_access_printSasExpanderInfo (fbe_encl_component_general_info_t *generalInfoPtr, 
                                       fbe_edal_trace_func_t trace_func)
{

    fbe_eses_encl_expander_info_t    *expanderDataPtr;

    expanderDataPtr = (fbe_eses_encl_expander_info_t *)generalInfoPtr;
	
    enclosure_access_printGeneralComponentInfo(generalInfoPtr,TRUE,trace_func); 	
   
    trace_func(NULL, "  SAS Expander Info\n");
    trace_func(NULL, "    SasAddr : 0x%llX, ElemIndx : %d, SideId : %d\n", 
        (unsigned long long)expanderDataPtr->expanderSasAddress,
        expanderDataPtr->expanderElementIndex,
        expanderDataPtr->expanderSideId);


   return;

}   // end of enclosure_access_printSasExpanderInfo


/*!*************************************************************************
 *                         @fn enclosure_access_printSasExpanderPhyInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace some or all of the Enclosure Object's
 *      Component data.
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    15-Sep-2008: Joe Perry - Created
 *
 **************************************************************************/
void 
enclosure_access_printSasExpanderPhyInfo (fbe_eses_expander_phy_info_t *phyInfoPtr, 
                                          fbe_edal_trace_func_t trace_func)
{
    trace_func(NULL, "  SAS Expander PHY Info\n");
    trace_func(NULL, "    ElemIndx : %d, Ready : %d, Disabled : %d, LinkReady : %d\n", 
        phyInfoPtr->phyElementIndex,
        phyInfoPtr->phyFlags.phyReady,
        phyInfoPtr->phyFlags.phyDisabled,
        phyInfoPtr->phyFlags.phyLinkReady);
    trace_func(NULL, "    CarrDtct : %d, FrcDis : %d, SataSUHld : %d, SUEnbld : %d\n", 
        phyInfoPtr->phyFlags.phyCarrierDetected,
        phyInfoPtr->phyFlags.phyForceDisabled,
        phyInfoPtr->phyFlags.phySataSpinupHold,
        phyInfoPtr->phyFlags.phySpinupEnabled);
    trace_func(NULL, "    PhyIdent : %d, ElemIndx: %d, Disable(W): %d, DisReason: %d\n", 
        phyInfoPtr->phyIdentifier,
        phyInfoPtr->expanderElementIndex,
        phyInfoPtr->phyFlags.disable,
        phyInfoPtr->phyDisableReason);
   
}   // end of enclosure_access_printSasExpanderPhyInfo

/*!*************************************************************************
 *                         @fn enclosure_access_printEsesTempSensorInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace some or all of the ESES temp sensor data.
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    25-Feb-2009: PHE - Created
 *
 **************************************************************************/
void 
enclosure_access_printEsesTempSensorInfo (fbe_eses_temp_sensor_info_t *tempSensorInfoPtr, 
                                          fbe_edal_trace_func_t trace_func)
{
    trace_func(NULL, "  ESES temp sensor Info\n");
    trace_func(NULL, "    TsElemIndx : %d, TsContainerIndx: %d, TsMaxTemp : %d\n", 
        tempSensorInfoPtr->tempSensorElementIndex,
        tempSensorInfoPtr->tempSensorContainerIndex,
        tempSensorInfoPtr->tempSensorMaxTemperature);
    
    return;
}   // end of enclosure_access_printEsesTempSensorInfo

/*!*************************************************************************
 *                         @fn enclosure_access_printEsesCoolingInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace some or all of the ESES temp sensor data.
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    25-Feb-2009: PHE - Created
 *
 **************************************************************************/
void 
enclosure_access_printEsesCoolingInfo(fbe_eses_cooling_info_t * coolingInfoPtr, 
                                      fbe_edal_trace_func_t trace_func)
{
    trace_func(NULL, "  ESES Cooling Info\n");
    trace_func(NULL, "    coolElemIndx  : %d, SubEnclId : %d, coolContainerIndx: %d\n", 
        coolingInfoPtr->coolingElementIndex,
        coolingInfoPtr->coolingSubEnclId,
        coolingInfoPtr->coolingContainerIndex);
    /* only valid if this is a subenclosure */
    if (coolingInfoPtr->coolingSubEnclId != FBE_ENCLOSURE_VALUE_INVALID)
    {
        trace_func(NULL, "    BufferDesc Id : %d, Type : %d, Indx : %d, Writable : %d\n", 
            coolingInfoPtr->bufferDescriptorInfo.bufferDescBufferId,
            coolingInfoPtr->bufferDescriptorInfo.bufferDescBufferType,
            coolingInfoPtr->bufferDescriptorInfo.bufferDescBufferIndex,
            coolingInfoPtr->bufferDescriptorInfo.bufferDescBufferIdWriteable);
        enclosure_access_printFwRevInfo(&coolingInfoPtr->fwInfo, trace_func);

        trace_func(NULL, "    CM SN : %.16s\n", coolingInfoPtr->coolingSerialNumber);
        trace_func(NULL, "    CM Product Id : %.16s\n", coolingInfoPtr->coolingProductId);
    }
    
    return;
}   // end of enclosure_access_printEsesCoolingInfo

/*!*************************************************************************
 *                  @fn enclosure_access_printPeCoolingInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace some or all of the Processor Enclosure
 *           Cooling data.
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    07-Apr-2010: Arun S - Created
 *
 **************************************************************************/
void 
enclosure_access_printPeCoolingInfo(edal_pe_cooling_info_t *peCoolingInfoPtr, 
                                      fbe_edal_trace_func_t trace_func)
{
    edal_pe_cooling_sub_info_t * peCoolingSubInfoPtr = NULL;
    fbe_cooling_info_t * extCoolingInfoPtr = NULL;

    peCoolingSubInfoPtr = &peCoolingInfoPtr->peCoolingSubInfo;
    extCoolingInfoPtr = &peCoolingSubInfoPtr->externalCoolingInfo;

    trace_func(NULL, "  Processor Encl Cooling Info\n");

    trace_func(NULL, "    associatedSp : %s, slotNumOnSpBlade: %d, xactStatus : 0x%x, envInterfaceStatus : %d\n", 
        (extCoolingInfoPtr->associatedSp == SP_A)?"SPA":"SPB",
        extCoolingInfoPtr->slotNumOnSpBlade,
        extCoolingInfoPtr->transactionStatus,
        extCoolingInfoPtr->envInterfaceStatus);   
    
    trace_func(NULL, "    fanModuleFault: %d, multiFanModuleFault: %d, persistentMultiFanModuleFault: %d\n", 
        extCoolingInfoPtr->fanFaulted,
        extCoolingInfoPtr->multiFanModuleFault,
        extCoolingInfoPtr->persistentMultiFanModuleFault);

    trace_func(NULL, "    isFaultRegFail:%s\n", BOOLEAN_TO_TEXT(extCoolingInfoPtr->isFaultRegFail));
    return;
}   // end of enclosure_access_printPeCoolingInfo

/*!*************************************************************************
 *                         @fn enclosure_access_printBaseEnclosureInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace some or all of the Enclosure Object's
 *      Component data.
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    15-Sep-2008: Joe Perry - Created
 *
 **************************************************************************/
void 
enclosure_access_printBaseEnclosureInfo(fbe_base_enclosure_info_t * baseEnclDataPtr, 
                                        fbe_edal_trace_func_t trace_func)
{
    trace_func(NULL, "  Base Enclosure Info\n");
    trace_func(NULL, "    Pos : %d, Addr : %d, Port : %d, ResetRTCnt : %d, SideId : %d, MaxSlot : %d\n", 
        baseEnclDataPtr->enclPosition,
        baseEnclDataPtr->enclAddress,
        baseEnclDataPtr->enclPortNumber,
        baseEnclDataPtr->enclResetRideThruCount,
        baseEnclDataPtr->enclSideId,
        baseEnclDataPtr->enclMaxSlot);
    trace_func(NULL, "    enclFltLedReason : 0x%llX\n", 
        baseEnclDataPtr->enclFaultLedReason);
}   // end of enclosure_access_printBaseEnclosureInfo

/*!*************************************************************************
 *                         @fn enclosure_access_printSasEnclosureInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace some or all of the Enclosure Object's
 *      Component data.
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    15-Sep-2008: Joe Perry - Created
 *
 **************************************************************************/
void 
enclosure_access_printSasEnclosureInfo (fbe_sas_enclosure_info_t *enclosureDataPtr, 
                                        fbe_edal_trace_func_t trace_func)
{
    trace_func(NULL, "  SAS Enclosure Info\n");
    trace_func(NULL, "    SasAddr Ses : 0x%llX, Smp : 0x%llX\n", 
        (unsigned long long)enclosureDataPtr->sesSasAddress,
        (unsigned long long)enclosureDataPtr->smpSasAddress);
    trace_func(NULL, "    ServerSasAddr : 0x%llX, UniqueId : 0x%x\n", 
        (unsigned long long)enclosureDataPtr->serverSasAddress,
        enclosureDataPtr->uniqueId);

}   // end of enclosure_access_printSasEnclosureInfo

/*!*************************************************************************
 *                         @fn enclosure_access_printEsesEnclosureInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace some or all of the Enclosure Object's
 *      Component data.
 *
 *  PARAMETERS:
 *      @param  enclosureDataPtr - pointer to Component Data
 *      @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                               updated component data
 *      @param  trace_func     - EDAL trace function
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    15-Sep-2008: Joe Perry - Created
 *
 **************************************************************************/
void 
enclosure_access_printEsesEnclosureInfo (fbe_eses_enclosure_info_t *enclosureDataPtr,
                                         fbe_bool_t printFullData, 
                                         fbe_edal_trace_func_t trace_func)
{
    trace_func(NULL, "  ESES Enclosure Info\n");
    trace_func(NULL, "    FailInd : %d, FailRqstd : %d, RqstFail(W) : %d\n", 
        enclosureDataPtr->esesEnclosureFlags.enclFailureIndication,
        enclosureDataPtr->esesEnclosureFlags.enclFailureRequested,
        enclosureDataPtr->esesEnclosureFlags.enclRequestFailure);
    trace_func(NULL, "    WarnInd : %d, WarnRqst : %d, RqstWarn(W) : %d\n", 
        enclosureDataPtr->esesEnclosureFlags.enclWarningIndication,
        enclosureDataPtr->esesEnclosureFlags.enclWarningRequested,
        enclosureDataPtr->esesEnclosureFlags.enclRequestWarning);
    trace_func(NULL, "    EmcSN : %.16s\n", 
        enclosureDataPtr->enclSerialNumber);    
    trace_func(NULL, "    PwrCycle Delay : %d, Rqst : %d, Duration : %d\n", 
        enclosureDataPtr->esesEnclosureData.enclPowerCycleDelay,
        enclosureDataPtr->esesEnclosureData.enclPowerCycleRequest,
        enclosureDataPtr->esesEnclosureData.enclPowerOffDuration);
    trace_func(NULL, "    SSC Insert : %s, Fault : %s\n", 
            enclosureDataPtr->esesStatusFlags.sscInsert == 1 ? "Y" : "N",
            enclosureDataPtr->esesStatusFlags.sscFault == 1 ? "Y" : "N");
    if (printFullData)
    {
        trace_func(NULL, "    ElemIndx : %d, SubEnclId : %d, TracePresent : %s\n", 
            enclosureDataPtr->enclElementIndex,
            enclosureDataPtr->enclSubEnclId,
            enclosureDataPtr->esesStatusFlags.tracePresent == 1 ? "Y" : "N"); 
        trace_func(NULL, "    BufferDesc Id : %d, Type : %d, Indx : %d, Writable : %d\n", 
            enclosureDataPtr->esesEnclosureData.bufferDescriptorInfo.bufferDescBufferId,
            enclosureDataPtr->esesEnclosureData.bufferDescriptorInfo.bufferDescBufferType,
            enclosureDataPtr->esesEnclosureData.bufferDescriptorInfo.bufferDescBufferIndex,
            enclosureDataPtr->esesEnclosureData.bufferDescriptorInfo.bufferDescBufferIdWriteable);
        trace_func(NULL, "    ELog BufferDesc Id : %d, Active Trace Buffer Id : %d\n", 
            enclosureDataPtr->esesEnclosureData.bufferDescriptorInfo.bufferDescElogBufferID,
            enclosureDataPtr->esesEnclosureData.bufferDescriptorInfo.bufferDescActBufferID);
        trace_func(NULL, "    Unsupport AddStat : %s, EmcSpec : %s, MdSns : %s, MdSel : %s\n", 
            enclosureDataPtr->esesStatusFlags.addl_status_unsupported == 1 ? "Y" : "N",
            enclosureDataPtr->esesStatusFlags.emc_specific_unsupported == 1 ? "Y" : "N",
            enclosureDataPtr->esesStatusFlags.mode_sense_unsupported == 1 ? "Y" : "N",
            enclosureDataPtr->esesStatusFlags.mode_select_unsupported == 1 ? "Y" : "N");
        trace_func(NULL, "    TimeLastGoodStat : 0x%llX, SDReason : 0x%x, PartialSD : %s\n", 
            (unsigned long long)enclosureDataPtr->enclTimeOfLastGoodStatusPage,
            enclosureDataPtr->enclShutdownReason,
            enclosureDataPtr->esesStatusFlags.partialShutDown == 1 ? "Y" : "N");
    }

    return;
}   // end of enclosure_access_printEsesEnclosureInfo

/*!*************************************************************************
 *                         @fn enclosure_access_printSasDriveInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace some or all of the Enclosure Object's
 *      Component data.
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
void 
enclosure_access_printSasDriveInfo (fbe_eses_encl_drive_info_t *driveInfoPtr, 
                                    fbe_edal_trace_func_t trace_func)
{
    trace_func(NULL, "  SAS Drive Info\n");
    trace_func(NULL, "    PhyIndx : %d, ElemIndx : %d, powerDownCount: %d\n", 
        driveInfoPtr->drivePhyIndex,
        driveInfoPtr->driveElementIndex,
        driveInfoPtr->drivePowerDownCount);

}   // end of enclosure_access_printSasDriveInfo

/*!*************************************************************************
 *                         @fn enclosure_access_printEsesPowerSupplyInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace some or all of the Enclosure Object's
 *      Component data.
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    15-Sep-2008: Joe Perry - Created
 *    21-Sep-2012: PHE - Changed the function name to indicate this is ESES level.
 *
 **************************************************************************/
void 
enclosure_access_printEsesPowerSupplyInfo (fbe_eses_power_supply_info_t *psInfoPtr, 
                                          fbe_edal_trace_func_t trace_func)
{
    trace_func(NULL, "  ESES Power Supply Info\n");
    trace_func(NULL, "    ElemIndx : %d, SubEnclId : %d, psContainerIndx: %d\n",
        psInfoPtr->psElementIndex,
        psInfoPtr->psSubEnclId,
        psInfoPtr->psContainerIndex);
    trace_func(NULL, "    BufferDesc Id : %d, Type : %d, Indx: %d, Writable : %d, bufferSize %d\n", 
        psInfoPtr->bufferDescriptorInfo.bufferDescBufferId,
        psInfoPtr->bufferDescriptorInfo.bufferDescBufferType,
        psInfoPtr->bufferDescriptorInfo.bufferDescBufferIndex,
        psInfoPtr->bufferDescriptorInfo.bufferDescBufferIdWriteable,
        psInfoPtr->bufferDescriptorInfo.bufferSize);

    enclosure_access_printFwRevInfo(&psInfoPtr->fwInfo, trace_func);

    trace_func(NULL, "    PS SN : %.16s\n", psInfoPtr->PSSerialNumber);
    trace_func(NULL, "    PS Product Id : %.16s\n", psInfoPtr->psProductId);

    // Input power
    if(psInfoPtr->generalPowerSupplyInfo.psData.inputPowerStatus != 
       FBE_EDAL_ATTRIBUTE_UNSUPPORTED)
    {
        trace_func(NULL, " InputPowerStatus:%d, InputPower:%d\n", 
            psInfoPtr->generalPowerSupplyInfo.psData.inputPowerStatus,
            psInfoPtr->generalPowerSupplyInfo.psData.inputPower);
#ifdef C4_INTEGRATED
    if (print_for_neo_fbecli)
    {
        if (psInfoPtr->generalPowerSupplyInfo.psData.inputPowerStatus == 1)
        {
            trace_func((fbe_edal_trace_context_t)0x01234567, " %d", psInfoPtr->generalPowerSupplyInfo.psData.inputPower / 10);
        }
        else
        {
            trace_func((fbe_edal_trace_context_t)0x01234567, " 000");
        }
    }
#endif /* C4_INTEGRATED - C4BUG - whatever is being done here probably should be runtime-config-all-cases-code */
        trace_func(NULL, " PsMarginTestMode Stat:%d, Cntrl:%d, PsMarginTestResults:%d\n", 
            psInfoPtr->generalPowerSupplyInfo.psData.psMarginTestMode,
            psInfoPtr->generalPowerSupplyInfo.psData.psMarginTestModeControl,
            psInfoPtr->generalPowerSupplyInfo.psData.psMarginTestResults);
    }

    return;
}   // end of enclosure_access_printEsesPowerSupplyInfo

/*!*************************************************************************
 *                         @fn enclosure_access_printPePowerSupplyInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace some or all of the Processor Enclosure
 *      Power Supply data.
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    07-Apr-2010: Arun S - Created
 *
 **************************************************************************/
void 
enclosure_access_printPePowerSupplyInfo (edal_pe_power_supply_info_t *pePowerSupplyInfoPtr, 
                                         fbe_edal_trace_func_t trace_func)
{
    
    edal_pe_power_supply_sub_info_t * pePowerSupplySubInfoPtr = NULL;
    fbe_power_supply_info_t * extPowerSupplyInfoPtr = NULL;

    pePowerSupplySubInfoPtr = &pePowerSupplyInfoPtr->pePowerSupplySubInfo;
    extPowerSupplyInfoPtr = &pePowerSupplySubInfoPtr->externalPowerSupplyInfo;
        
    trace_func(NULL, "  Processor Encl Power Supply Info\n");   

    trace_func(NULL, "    associatedSp : %d, slotNumOnEncl : %d, isLocalFru %d, associatedSps : %d\n", 
        extPowerSupplyInfoPtr->associatedSp,
        extPowerSupplyInfoPtr->slotNumOnEncl,
        extPowerSupplyInfoPtr->isLocalFru,
        extPowerSupplyInfoPtr->associatedSps);

    trace_func(NULL, "    xactionStatus : %d, envInterfaceStatus : %d, peerEnvInterfaceStatus : %d\n", 
        extPowerSupplyInfoPtr->transactionStatus,
        extPowerSupplyInfoPtr->envInterfaceStatus,
        pePowerSupplySubInfoPtr->peerEnvInterfaceStatus);

    trace_func(NULL, "    inserted : %d, generalFault %d, ACFail : %d, ACDCInput : %d, isFaultRegFail:%s\n", 
        extPowerSupplyInfoPtr->bInserted,
        extPowerSupplyInfoPtr->generalFault,
        extPowerSupplyInfoPtr->ACFail,
        extPowerSupplyInfoPtr->ACDCInput,
        BOOLEAN_TO_TEXT(extPowerSupplyInfoPtr->isFaultRegFail));    

    trace_func(NULL, "    InternalFanFault : %d\n", 
        extPowerSupplyInfoPtr->internalFanFault);

    trace_func(NULL, "    ambientOvertempFault : %d\n", 
        extPowerSupplyInfoPtr->ambientOvertempFault);

    trace_func(NULL, "    firmwareRevValid : %d, FirmwareRev : %s\n", 
        extPowerSupplyInfoPtr->firmwareRevValid,
        extPowerSupplyInfoPtr->firmwareRev);

    trace_func(NULL, "    inputPower : %d, inputPowerStatus : 0x%x\n", 
        extPowerSupplyInfoPtr->inputPower,
        extPowerSupplyInfoPtr->inputPowerStatus);

    return;

}   // end of enclosure_access_printPePowerSupplyInfo

/*!*************************************************************************
 *                         @fn enclosure_access_printPowerSupplyData
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace Power Supply component data.
 *
 *  PARAMETERS:
 *      @param  componentTypeDataPtr  - pointer to Component Data
 *      @param  enclosureType  - type of enclosure
 *      @param  componentIndex - index to a specific component
 *      @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                               updated component data
 *      @param  trace_func     - EDAL trace function
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
void
enclosure_access_printPowerSupplyData(fbe_enclosure_component_t   *componentTypeDataPtr,
                                      fbe_enclosure_types_t enclosureType,
                                      fbe_u32_t componentIndex,
                                      fbe_bool_t printFullData, 
                                      fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t generalDataPtr;
    fbe_encl_power_supply_info_t    *powerSupplyDataPtr;
    edal_pe_power_supply_info_t     *pePowerSupplyInfoPtr;
    fbe_u32_t                       index;


    if (componentTypeDataPtr->componentType != FBE_ENCL_POWER_SUPPLY)
    {
        return;
    }

    // loop through all Power Supplies
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, "  Power Supply %d\n", index);
            }
            else
            {
                trace_func(NULL, "State Change to Power Supply %d\n", index);
            }

            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);

            powerSupplyDataPtr = (fbe_encl_power_supply_info_t *)generalDataPtr;
            
#ifdef C4_INTEGRATED
            if (print_for_neo_fbecli)
            {
                if (!powerSupplyDataPtr->generalComponentInfo.generalFlags.componentInserted)
                    trace_func((fbe_edal_trace_context_t)0x01234567, "REMOVED");
                else if (powerSupplyDataPtr->psFlags.acFail)
                    trace_func((fbe_edal_trace_context_t)0x01234567, "FAULTED ACLOSS");
                else if (powerSupplyDataPtr->generalComponentInfo.generalFlags.componentFaulted)
                    trace_func((fbe_edal_trace_context_t)0x01234567, "FAULTED");
                else
                    trace_func((fbe_edal_trace_context_t)0x01234567, "OK");
            }
#endif /* C4_INTEGRATED - C4BUG - whatever is being done here probably should be runtime-config-all-cases-code */

            if (enclosureType != FBE_ENCL_PE)
            {
#ifdef C4_INTEGRATED
                if(!print_for_neo_fbecli)
#endif /* C4_INTEGRATED - C4BUG - whatever is being done here probably should be runtime-config-all-cases-code */
                enclosure_access_printGeneralComponentInfo(&(powerSupplyDataPtr->generalComponentInfo),
                                                         printFullData, trace_func);
            
                enclosure_access_printGeneralPowerSupplyInfo(powerSupplyDataPtr, trace_func);

                if(IS_VALID_ENCL(enclosureType)) 
                {
                    if (printFullData)
                    {
                        enclosure_access_printEsesPowerSupplyInfo((fbe_eses_power_supply_info_t *)powerSupplyDataPtr, trace_func);
                    }
                }
            }
            else
            {                
                enclosure_access_printPeGeneralComponentInfo(&(powerSupplyDataPtr->generalComponentInfo),
                                                                  printFullData, trace_func);

                pePowerSupplyInfoPtr = (edal_pe_power_supply_info_t *)generalDataPtr;

                enclosure_access_printPePowerSupplyInfo(pePowerSupplyInfoPtr, trace_func);         
            }
        }
    }

}   // end of enclosure_access_printPowerSupplyData


/*!*************************************************************************
 *                         @fn enclosure_access_printDriveData
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace Drive/Slot component data.
 *
 *  PARAMETERS:
 *      @param  componentTypeDataPtr  - pointer to Component Data
 *      @param  enclosureType  - type of enclosure
 *      @param  componentIndex - index to a specific component
 *      @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                               updated component data
 *      @param  trace_func     - EDAL trace function
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
void
enclosure_access_printDriveData(fbe_enclosure_component_t   *componentTypeDataPtr,
                                fbe_enclosure_types_t enclosureType,
                                fbe_u32_t componentIndex,
                                fbe_bool_t printFullData, 
                                fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t generalDataPtr;
    fbe_encl_drive_slot_info_t      *driveDataPtr;
    fbe_u32_t                       index;
        
    if (componentTypeDataPtr->componentType != FBE_ENCL_DRIVE_SLOT)
    {
        return;
    }

    // loop through all Drive/Slots
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);
            driveDataPtr = (fbe_encl_drive_slot_info_t *)generalDataPtr;

            if (printFullData)
            {
                trace_func(NULL, "  Drive-index(Slot) %d(%d)\n", 
                           index, driveDataPtr->driveSlotNumber);
            }
            else
            {
                trace_func(NULL, "State Change to Drive-index(Slot) %d(%d)\n", 
                           index, driveDataPtr->driveSlotNumber);
            }

            enclosure_access_printGeneralComponentInfo(&(driveDataPtr->generalComponentInfo),
                                                       printFullData, trace_func);

            enclosure_access_printGeneralDriveInfo(driveDataPtr, trace_func);

            if(IS_VALID_ENCL(enclosureType)) 
            {
                enclosure_access_printSasDriveInfo((fbe_eses_encl_drive_info_t *)driveDataPtr, trace_func);
            }
        }
    }

}   // end of enclosure_access_printDriveData


/*!*************************************************************************
 *                         @fn enclosure_access_printCoolingData
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace Fan/Cooling component data.
 *
 *  PARAMETERS:
 *      @param  componentTypeDataPtr  - pointer to Component Data
 *      @param  componentIndex - index to a specific component
 *      @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                               updated component data
 *      @param  trace_func     - EDAL trace function
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    15-Sep-2008: Joe Perry - Created
 *    25-Feb-2009: PHE - Added the input parameter enclosureType.
 *
 **************************************************************************/
void
enclosure_access_printCoolingData(fbe_enclosure_component_t   *componentTypeDataPtr,
                                  fbe_enclosure_types_t enclosureType,
                                  fbe_u32_t componentIndex,
                                  fbe_bool_t printFullData, 
                                  fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t  generalDataPtr = NULL;
    fbe_encl_cooling_info_t         *coolingDataPtr;
    edal_pe_cooling_info_t          *peCoolingInfoPtr;
    fbe_u32_t                       index;

    if (componentTypeDataPtr->componentType != FBE_ENCL_COOLING_COMPONENT)
    {
        return;
    }

    // loop through all Fans
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, "Cooling %d\n", index);
            }
            else
            {
                trace_func(NULL, "State Change to Cooling %d\n", index);
            }
            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);

            coolingDataPtr = (fbe_encl_cooling_info_t *)generalDataPtr;            

            if(enclosureType != FBE_ENCL_PE)
            {
                enclosure_access_printGeneralComponentInfo(&(coolingDataPtr->generalComponentInfo),
                                                       printFullData, trace_func);

                enclosure_access_printGeneralCoolingInfo(coolingDataPtr, trace_func);
            
                if(IS_VALID_ENCL(enclosureType)) 
                {
                    enclosure_access_printEsesCoolingInfo((fbe_eses_cooling_info_t *)coolingDataPtr, trace_func);
                }
            }
            else
            {                
                enclosure_access_printPeGeneralComponentInfo(&(coolingDataPtr->generalComponentInfo),
                                                                printFullData, trace_func);

                peCoolingInfoPtr = (edal_pe_cooling_info_t *)generalDataPtr;

                enclosure_access_printPeCoolingInfo(peCoolingInfoPtr, trace_func);         
            }
        }
    }

}   // end of enclosure_access_printCoolingData


/*!*************************************************************************
 *                         @fn enclosure_access_printTempSensorData
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace Temperature Sensor component data.
 *
 *  PARAMETERS:
 *      @param  componentTypeDataPtr  - pointer to Component Data
 *      @param  componentIndex - index to a specific component
 *      @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                               updated component data
 *      @param  trace_func     - EDAL trace function
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    15-Sep-2008: Joe Perry - Created
 *    25-Feb-2009: PHE - Added the input parameter enclosureType.
 *
 **************************************************************************/
void
enclosure_access_printTempSensorData(fbe_enclosure_component_t   *componentTypeDataPtr,
                                     fbe_enclosure_types_t enclosureType,
                                     fbe_u32_t componentIndex,
                                     fbe_bool_t printFullData, 
                                     fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t  generalDataPtr = NULL;
    fbe_encl_temp_sensor_info_t     *tempSensorDataPtr;
    fbe_u32_t                       index;

    if (componentTypeDataPtr->componentType != FBE_ENCL_TEMP_SENSOR)
    {
        return;
    }

    // loop through all Temp Sensors
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, "  Temperature Sensor %d\n", index);
            }
            else
            {
                trace_func(NULL, "State Change to Temperature Sensor %d\n", index);
            }
            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);
            tempSensorDataPtr = (fbe_encl_temp_sensor_info_t *)generalDataPtr;

            enclosure_access_printGeneralComponentInfo(&(tempSensorDataPtr->generalComponentInfo),
                                                       printFullData, trace_func);

            enclosure_access_printGeneralTempSensorInfo(tempSensorDataPtr, trace_func);

            if(IS_VALID_ENCL(enclosureType)) 
            {
                enclosure_access_printEsesTempSensorInfo((fbe_eses_temp_sensor_info_t *)tempSensorDataPtr, 
                                                            trace_func);
            }
        }        
    }

}   // end of enclosure_access_printTempSensorData


/*!*************************************************************************
 *                         @fn enclosure_access_printConnectorData
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace Connector component data.
 *
 *  PARAMETERS:
 *      @param  componentTypeDataPtr  - pointer to Component Data
 *      @param  enclosureType  - type of enclosure
 *      @param  componentIndex - index to a specific component
 *      @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                               updated component data
 *      @param  trace_func     - EDAL trace function
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    15-Sep-2008: Joe Perry - Created
 *
 **************************************************************************/
void
enclosure_access_printConnectorData(fbe_enclosure_component_t   *componentTypeDataPtr,
                                    fbe_enclosure_types_t enclosureType,
                                    fbe_u32_t componentIndex,
                                    fbe_bool_t printFullData, 
                                    fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t  generalDataPtr;
    fbe_eses_encl_connector_info_t  *connectorDataPtr;
    fbe_u32_t                       index;

    if (componentTypeDataPtr->componentType != FBE_ENCL_CONNECTOR)
    {
        return;
    }

    // loop through all Connectors
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, "  Connector %d\n", index);
            }
            else
            {
                trace_func(NULL, "State Change to Connector %d\n", index);
            }
            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);
            connectorDataPtr = (fbe_eses_encl_connector_info_t *)generalDataPtr;

            enclosure_access_printGeneralComponentInfo(&(connectorDataPtr->baseConnectorInfo.generalComponentInfo),
                                                       printFullData, trace_func);
//            enclosure_access_printGeneralConnectorInfo(connectorDataPtr);

            if(IS_VALID_ENCL(enclosureType)) 
            {
                enclosure_access_printSasConnectorInfo(connectorDataPtr, trace_func);
            }
        }
    }

}   // end of enclosure_access_printConnectorData


/*!*************************************************************************
 *                         @fn enclosure_access_printExpanderData
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace Expander component data.
 *
 *  PARAMETERS:
 *      @param  componentTypeDataPtr  - pointer to Component Data
 *      @param  enclosureType  - type of enclosure
 *      @param  componentIndex - index to a specific component
 *      @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                               updated component data
 *      @param  trace_func     - EDAL trace function
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    15-Sep-2008: Joe Perry - Created
 *
 **************************************************************************/
void
enclosure_access_printExpanderData(fbe_enclosure_component_t   *componentTypeDataPtr,
                                   fbe_enclosure_types_t enclosureType,
                                   fbe_u32_t componentIndex,
                                   fbe_bool_t printFullData, 
                                   fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t generalDataPtr;

    fbe_u32_t                       index;

    if (componentTypeDataPtr->componentType != FBE_ENCL_EXPANDER)
    {
        return;
    }

    // loop through all Expanders
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, "  Expander %d\n", index);
            }
            else
            {
                trace_func(NULL, "State Change to Expander %d\n", index);
            }
            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);

            if(IS_VALID_ENCL(enclosureType)) 
            {
                enclosure_access_printSasExpanderInfo(generalDataPtr, trace_func);
            }
        }
    }

}   // end of enclosure_access_printExpanderData


/*!*************************************************************************
 *                         @fn enclosure_access_printExpanderPhyData
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace Expander PHY component data.
 *
 *  PARAMETERS:
 *      componentTypeDataPtr - pointer to expander PHY component
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    15-Sep-2008: Joe Perry - Created
 *
 **************************************************************************/
void
enclosure_access_printExpanderPhyData(fbe_enclosure_component_t   *componentTypeDataPtr,
                                      fbe_enclosure_types_t enclosureType,
                                      fbe_u32_t componentIndex,
                                      fbe_bool_t printFullData, 
                                      fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t generalDataPtr;
    fbe_sas_expander_phy_info_t    *phyDataPtr;
    fbe_u32_t                       index;

    if (componentTypeDataPtr->componentType != FBE_ENCL_EXPANDER_PHY)
    {
        return;
    }

    // loop through all Expander PHY's
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, "  Expander PHY %d\n", index);
            }
            else
            {
                trace_func(NULL, "State Change to Expander PHY %d\n", index);
            }

            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);
            
            if(IS_VALID_ENCL(enclosureType)) 
            {
                phyDataPtr = (fbe_sas_expander_phy_info_t *)generalDataPtr;

                enclosure_access_printGeneralComponentInfo(&(phyDataPtr->generalExpanderPhyInfo),
                                                        printFullData, trace_func);   
                
                enclosure_access_printSasExpanderPhyInfo((fbe_eses_expander_phy_info_t *)phyDataPtr,
                                                        trace_func);
            }
        }
    }

}   // end of enclosure_access_printExpanderPhyData


/*!*************************************************************************
 *                         @fn enclosure_access_printEnclosureData
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace Enclosure component data.
 *
 *  PARAMETERS:
 *      @param  componentTypeDataPtr  - pointer to Component Data
 *      @param  enclosureType  - type of enclosure
 *      @param  componentIndex - index to a specific component
 *      @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                               updated component data
 *      @param  trace_func     - EDAL trace function
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    15-Sep-2008: Joe Perry - Created
 *
 **************************************************************************/
void
enclosure_access_printEnclosureData(fbe_enclosure_component_t   *componentTypeDataPtr,
                                    fbe_enclosure_types_t enclosureType,
                                    fbe_u32_t componentIndex,
                                    fbe_bool_t printFullData, 
                                    fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t generalDataPtr;
    fbe_base_enclosure_info_t       *enclosureDataPtr;
    fbe_u32_t                       index;

    if (componentTypeDataPtr->componentType != FBE_ENCL_ENCLOSURE)
    {
        return;
    }

    // loop through all Enclosures
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, "  Enclosure %d\n", index);
            }
            else
            {
                trace_func(NULL, "State Change to Enclosure %d\n", index);
            }
            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);
            enclosureDataPtr = (fbe_base_enclosure_info_t *)generalDataPtr;

            enclosure_access_printGeneralComponentInfo(&(enclosureDataPtr->generalEnclosureInfo),
                                                       printFullData, trace_func);   

            enclosure_access_printBaseEnclosureInfo(enclosureDataPtr, trace_func);

            if(IS_VALID_ENCL(enclosureType)) 
            {
                enclosure_access_printSasEnclosureInfo((fbe_sas_enclosure_info_t *)enclosureDataPtr,
                                                        trace_func);
            }
        }
        
        if(IS_VALID_ENCL(enclosureType)) 
        {
            enclosure_access_printEsesEnclosureInfo((fbe_eses_enclosure_info_t *)enclosureDataPtr,
                                                    printFullData, trace_func);
        }
    }

}   // end of enclosure_access_printEnclosureData

/*!*************************************************************************
 *                         @fn enclosure_access_printFwRevInfo
 **************************************************************************
 * DESCRIPTION:
 * @brief
 *  This function displays firmware revision 
 *  characters. The characters are copied into a local array int order 
 *  to append a teminating 0 and display a string format.
 *
 * PARAMETERS:
 * @param fw_rev_p - pointer to the firmware revision ifnormation
 * @param  trace_func - EDAL trace function
 *
 * @return void
 * 
 * HISTORY:
 *    13-Oct-2008: GB - Created
 *    29-Apr-2010: NCHIU - modified to print from fw_info structure.
 *
 **************************************************************************/
void enclosure_access_printFwRevInfo(fbe_edal_fw_info_t *fw_rev_p, 
                                     fbe_edal_trace_func_t trace_func)
{
    // only print when there's valid info, 
    // some fans, power supplies do not have firmware rev.
    if (fw_rev_p->valid)
    {
#ifdef C4_INTEGRATED
        if (print_for_neo_fbecli)
        {
            if (fw_rev_p->fwRevision[0] == '\0')
            {
               trace_func((fbe_edal_trace_context_t)0x01234567, " 00.00");
            }
            else
            {
                trace_func((fbe_edal_trace_context_t)0x01234567, "%.16s", fw_rev_p->fwRevision);
            }
            return;
        }
        else
#endif /* C4_INTEGRATED - C4BUG - whatever is being done here probably should be runtime-config-all-cases-code */
        trace_func(NULL, "    Rev     : %.16s\n", fw_rev_p->fwRevision);
    }
    else
    {
        trace_func(NULL, "    Rev Not Valid\n");
    }
} // end enclosure_access_printFwRevInfo


/*!*************************************************************************
 *                         @fn enclosure_access_printIoCompInfo
 **************************************************************************
 *  @brief
 *      This function prints out the IO module/IO annex info.
 *
 *  @param  componentTypeDataPtr  - pointer to Component Data
 *  @param  componentIndex - index to a specific component
 *  @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                              updated component data
 *  @param  trace_func     - EDAL trace function
 *
 *  @return
 *         none.      
 *
 *  @note
 *
 *  @version
 *    10-Mar-2010: PHE - Created.
 *
 **************************************************************************/
void
enclosure_access_printIoCompInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                     fbe_enclosure_types_t enclosureType,
                                     fbe_u32_t componentIndex,
                                     fbe_bool_t printFullData, 
                                     fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t  generalDataPtr = NULL;
    edal_base_io_comp_info_t    * baseIoCompInfoPtr = NULL;
    edal_pe_io_comp_info_t      * peIoCompInfoPtr = NULL;
    fbe_u32_t                       index = 0;

    if((componentTypeDataPtr->componentType != FBE_ENCL_IO_MODULE) &&
       (componentTypeDataPtr->componentType != FBE_ENCL_BEM) &&
       (componentTypeDataPtr->componentType != FBE_ENCL_MEZZANINE))
    {
        return;
    }

    trace_func(NULL, "\n");

    // loop through all IO modules
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, " %s %d\n", 
                    (componentTypeDataPtr->componentType == FBE_ENCL_IO_MODULE) ? "IO Module" : 
                    (componentTypeDataPtr->componentType == FBE_ENCL_BEM) ? "BEM" : "Mezzanine", index);
            }
            else
            {
                trace_func(NULL, "State Change to %s %d\n", 
                    (componentTypeDataPtr->componentType == FBE_ENCL_IO_MODULE) ? "IO Module" : 
                    (componentTypeDataPtr->componentType == FBE_ENCL_BEM) ? "BEM" : "Mezzanine", index);
            }
            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);
            baseIoCompInfoPtr = (edal_base_io_comp_info_t *)generalDataPtr;

            enclosure_access_printPeGeneralComponentInfo(&(baseIoCompInfoPtr->generalComponentInfo),
                                                       printFullData, trace_func);

            peIoCompInfoPtr = (edal_pe_io_comp_info_t *)generalDataPtr;

            enclosure_access_printPeIoCompInfo(peIoCompInfoPtr, trace_func);         
        }        
    }

    return;

}   // end of enclosure_access_printIoCompInfo


/*!*************************************************************************
 *                         @fn enclosure_access_printPeIoCompInfo
 **************************************************************************
 *  @brief
 *      This function prints out the processor enclosure IO module/IO annex info.
 *
 *  @param     peIoCompInfoPtr
 *  @param     trace_func
 *
 *  @return
 *      None.
 *
 *  @note
 *
 *  @version
 *    10-Mar-2010: PHE - Created
 *
 **************************************************************************/
void 
enclosure_access_printPeIoCompInfo(edal_pe_io_comp_info_t *peIoCompInfoPtr, 
                                          fbe_edal_trace_func_t trace_func)
{
    edal_pe_io_comp_sub_info_t * peIoCompSubInfoPtr = NULL;
    fbe_board_mgmt_io_comp_info_t * extIoCompInfoPtr = NULL;

    peIoCompSubInfoPtr = &peIoCompInfoPtr->peIoCompSubInfo;
    extIoCompInfoPtr = &peIoCompInfoPtr->peIoCompSubInfo.externalIoCompInfo;

    trace_func(NULL, "  Processor Encl IO Comp Info\n");
     
    trace_func(NULL, "    associatedSp : %d, slotNumOnBlade : %d\n",
        extIoCompInfoPtr->associatedSp,
        extIoCompInfoPtr->slotNumOnBlade);
    
    trace_func(NULL, "    xactStatus : 0x%x, envInterfaceStatus : %d\n", 
        extIoCompInfoPtr->transactionStatus,
        extIoCompInfoPtr->envInterfaceStatus);     

    trace_func(NULL, "    inserted : %d, uniqueID %d, faultLEDStatus : %d, powerStatus : %d\n",
        extIoCompInfoPtr->inserted,
        extIoCompInfoPtr->uniqueID,
        extIoCompInfoPtr->faultLEDStatus,
        extIoCompInfoPtr->powerStatus);    
    
    trace_func(NULL, "    ioPortCount : %d,  location: %d, isLocalFru : %d\n", 
        extIoCompInfoPtr->ioPortCount,
        extIoCompInfoPtr->location,
        extIoCompInfoPtr->isLocalFru);   

    trace_func(NULL, "    isFaultRegFail : %s, hwErrMonFault : %s\n", 
        BOOLEAN_TO_TEXT(extIoCompInfoPtr->isFaultRegFail), 
        BOOLEAN_TO_TEXT(extIoCompInfoPtr->hwErrMonFault));   

    return;
}   // end of enclosure_access_printPeIoCompInfo


/*!*************************************************************************
 *                         @fn enclosure_access_printIoPortInfo
 **************************************************************************
 *  @brief
 *      This function prints out the IO Port info.
 *
 *  @param  componentTypeDataPtr  - pointer to Component Data
 *  @param  componentIndex - index to a specific component
 *  @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                              updated component data
 *  @param  trace_func     - EDAL trace function
 *
 *  @return
 *         none.      
 *
 *  @note
 *
 *  @version
 *    10-Mar-2010: PHE - Created.
 *
 **************************************************************************/
void
enclosure_access_printIoPortInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                     fbe_enclosure_types_t enclosureType,
                                     fbe_u32_t componentIndex,
                                     fbe_bool_t printFullData, 
                                     fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t  generalDataPtr = NULL;
    edal_base_io_port_info_t      * baseIoPortInfoPtr = NULL;
    edal_pe_io_port_info_t        * peIoPortInfoPtr = NULL;
    fbe_u32_t                       index = 0;

    if(componentTypeDataPtr->componentType != FBE_ENCL_IO_PORT)
    {
        return;
    }

    trace_func(NULL, "\n");

    // loop through all IO modules
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, " IO Port %d\n", index);
            }
            else
            {
                trace_func(NULL, "State Change to IO Port %d\n", index);
            }
            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);
            baseIoPortInfoPtr = (edal_base_io_port_info_t *)generalDataPtr;

            enclosure_access_printPeGeneralComponentInfo(&(baseIoPortInfoPtr->generalComponentInfo),
                                                       printFullData, trace_func);

            peIoPortInfoPtr = (edal_pe_io_port_info_t *)generalDataPtr;

            enclosure_access_printPeIoPortInfo(peIoPortInfoPtr, trace_func);         
        }        
    }

    return;

}   // end of enclosure_access_printIoPortInfo

/*!*************************************************************************
 *                         @fn enclosure_access_printPeIoPortInfo
 **************************************************************************
 *  @brief
 *      This function prints out the processor enclosure IO Port info.
 *
 *  @param     peIoCompInfoPtr
 *  @param     trace_func
 *
 *  @return
 *      None.
 *
 *  @note
 *
 *  @version
 *    10-Mar-2010: PHE - Created
 *
 **************************************************************************/
void 
enclosure_access_printPeIoPortInfo(edal_pe_io_port_info_t *peIoPortInfoPtr, 
                                          fbe_edal_trace_func_t trace_func)
{
    edal_pe_io_port_sub_info_t * peIoPortSubInfoPtr = NULL;
    fbe_board_mgmt_io_port_info_t * extIoPortInfoPtr = NULL;

    peIoPortSubInfoPtr = &peIoPortInfoPtr->peIoPortSubInfo;
    extIoPortInfoPtr = &peIoPortSubInfoPtr->externalIoPortInfo;

    trace_func(NULL, "  Processor Encl IO Comp Info\n");
     
    trace_func(NULL, "    associatedSp : %d, slotNumOnBlade : %d, portNumOnModule : %d\n",
        extIoPortInfoPtr->associatedSp,
        extIoPortInfoPtr->slotNumOnBlade,
        extIoPortInfoPtr->portNumOnModule);  

    trace_func(NULL, "    isLocalFru : %d,  deviceType: %lld, present : %d\n",  
        extIoPortInfoPtr->isLocalFru,
        (fbe_u64_t)extIoPortInfoPtr->deviceType,
        extIoPortInfoPtr->present); 

    trace_func(NULL, "    protocol: %d,  powerStatus: %d,  supportedSpeeds: %d\n", 
        extIoPortInfoPtr->protocol,
        extIoPortInfoPtr->powerStatus,
        extIoPortInfoPtr->supportedSpeeds); 

    trace_func(NULL, "    ioPortLED: %d, ioPortLEDColor: %d\n", 
        extIoPortInfoPtr->ioPortLED,
        extIoPortInfoPtr->ioPortLEDColor); 

    trace_func(NULL, "    SFPcapable : %d,  pciDataBus: %d,  pciDataDevice: %d,  pciDataFunction: %d\n", 
        extIoPortInfoPtr->SFPcapable,
        extIoPortInfoPtr->pciData[0].pciData.bus,
        extIoPortInfoPtr->pciData[0].pciData.device,
        extIoPortInfoPtr->pciData[0].pciData.function);  

    return;
}   // end of enclosure_access_printPeIoPortInfo



/*!*************************************************************************
 *                         @fn enclosure_access_printPlatformInfo
 **************************************************************************
 *  @brief
 *      This function prints out the platform info.
 *
 *  @param  componentTypeDataPtr  - pointer to Component Data
 *  @param  componentIndex - index to a specific component
 *  @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                              updated component data
 *  @param  trace_func     - EDAL trace function
 *
 *  @return
 *         none.      
 *
 *  @note
 *
 *  @version
 *    07-Apr-2010: PHE - Created
 *
 **************************************************************************/
void
enclosure_access_printPlatformInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                     fbe_enclosure_types_t enclosureType,
                                     fbe_u32_t componentIndex,
                                     fbe_bool_t printFullData, 
                                     fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t  generalDataPtr = NULL;
    edal_base_platform_info_t     * basePlatformInfoPtr = NULL;
    edal_pe_platform_info_t       * pePlatformInfoPtr = NULL;
    fbe_u32_t                       index = 0;

    trace_func(NULL, "\n");

    // loop through all IO modules
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, " Platform %d\n", index);
            }
            else
            {
                trace_func(NULL, "State Change to Platform %d\n", index);
            }
            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);
            basePlatformInfoPtr = (edal_base_platform_info_t *)generalDataPtr;

            enclosure_access_printPeGeneralComponentInfo(&(basePlatformInfoPtr->generalComponentInfo),
                                                       printFullData, trace_func);

            pePlatformInfoPtr = (edal_pe_platform_info_t *)generalDataPtr;

            enclosure_access_printPePlatformInfo(pePlatformInfoPtr, trace_func);         
        }        
    }

    return;

}   // end of enclosure_access_printPlatformInfo

/*!*************************************************************************
 *                         @fn enclosure_access_printPePlatformInfo
 **************************************************************************
 *  @brief
 *      This function prints out the processor enclosure platform info.
 *
 *  @param     pePlatformInfoPtr
 *  @param     trace_func
 *
 *  @return
 *      None.
 *
 *  @note
 *
 *  @version
 *    07-Apr-2010: PHE - Created
 *
 **************************************************************************/
void 
enclosure_access_printPePlatformInfo(edal_pe_platform_info_t *pePlatformInfoPtr, 
                                     fbe_edal_trace_func_t trace_func)
{
    fbe_board_mgmt_platform_info_t * pePlatformInfoSubPtr = NULL;
    pePlatformInfoSubPtr = &pePlatformInfoPtr->pePlatformSubInfo;
        
    trace_func(NULL, "  Processor Encl Platform Info\n");
    trace_func(NULL, "    platformType : %d, cpuModule : %d, enclosureType : %d, midplaneType : %d\n", 
        pePlatformInfoSubPtr->hw_platform_info.platformType,
        pePlatformInfoSubPtr->hw_platform_info.cpuModule,
        pePlatformInfoSubPtr->hw_platform_info.enclosureType,
        pePlatformInfoSubPtr->hw_platform_info.midplaneType);
    
    return;
}   // end of enclosure_access_printPePlatformInfo


/*!*************************************************************************
 *                         @fn enclosure_access_printCacheCardInfo
 **************************************************************************
 *  @brief
 *      This function prints out the Cache Card info.
 *
 *  @param  componentTypeDataPtr  - pointer to Component Data
 *  @param  componentIndex - index to a specific component
 *  @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                              updated component data
 *  @param  trace_func     - EDAL trace function
 *
 *  @return
 *         none.      
 *
 *  @note
 *
 *  @version
 *    31-Jan-2013: Rui Chang - Created
 *
 **************************************************************************/
void
enclosure_access_printCacheCardInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                     fbe_enclosure_types_t enclosureType,
                                     fbe_u32_t componentIndex,
                                     fbe_bool_t printFullData, 
                                     fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t  generalDataPtr = NULL;
    edal_base_cache_card_info_t     * cacheCardInfoPtr = NULL;
    edal_pe_cache_card_info_t       * peCacheCardInfoPtr = NULL;
    fbe_u32_t                       index = 0;

    trace_func(NULL, "\n");

    // loop through all IO modules
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, " Cache Card %d\n", index);
            }
            else
            {
                trace_func(NULL, "State Change to Cache Card %d\n", index);
            }
            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);
            cacheCardInfoPtr = (edal_base_cache_card_info_t *)generalDataPtr;

            enclosure_access_printPeGeneralComponentInfo(&(cacheCardInfoPtr->generalComponentInfo),
                                                       printFullData, trace_func);

            peCacheCardInfoPtr = (edal_pe_cache_card_info_t *)generalDataPtr;

            enclosure_access_printPeCacheCardInfo(peCacheCardInfoPtr, trace_func);         
        }        
    }

    return;

}   // end of enclosure_access_printCacheCardInfo

/*!*************************************************************************
 *                         @fn enclosure_access_printPeCacheCardInfo
 **************************************************************************
 *  @brief
 *      This function prints out the processor enclosure CacheCard info.
 *
 *  @param     peCacheCardInfoPtr
 *  @param     trace_func
 *
 *  @return
 *      None.
 *
 *  @note
 *
 *  @version
 *    31-Jan-2013: Rui Chang - Created
 *
 **************************************************************************/
void 
enclosure_access_printPeCacheCardInfo(edal_pe_cache_card_info_t *peCacheCardInfoPtr, 
                                     fbe_edal_trace_func_t trace_func)
{
    fbe_board_mgmt_cache_card_info_t * peCacheCardInfoSubPtr = NULL;
    peCacheCardInfoSubPtr = &peCacheCardInfoPtr->peCacheCardSubInfo.externalCacheCardInfo;
        
    trace_func(NULL, "  Processor Encl Cache Card Info\n");
    trace_func(NULL, "    associatedSp : %d, cacheCardID : %d, isLocalFru : %d, envInterfaceStatus : %d\n",         
        peCacheCardInfoSubPtr->associatedSp,
        peCacheCardInfoSubPtr->cacheCardID,
        peCacheCardInfoSubPtr->isLocalFru,
        peCacheCardInfoSubPtr->envInterfaceStatus);

    trace_func(NULL, "    inserted : %d, uniqueID: %d, faultLEDStatus : %d, reset : %d\n",         
        peCacheCardInfoSubPtr->inserted,
        peCacheCardInfoSubPtr->uniqueID,
        peCacheCardInfoSubPtr->faultLEDStatus,
        peCacheCardInfoSubPtr->reset);

    trace_func(NULL, "    powerEnable : %d, powerGood : %d, powerUpEnable : %d, powerUpFailure : %d\n",         
        peCacheCardInfoSubPtr->powerEnable,
        peCacheCardInfoSubPtr->powerGood,
        peCacheCardInfoSubPtr->powerUpEnable,
        peCacheCardInfoSubPtr->powerUpFailure);

    return;
}   // end of enclosure_access_printPeCacheCardInfo

/*!*************************************************************************
 *                         @fn enclosure_access_printDimmInfo
 **************************************************************************
 *  @brief
 *      This function prints out the DIMM info.
 *
 *  @param  componentTypeDataPtr  - pointer to Component Data
 *  @param  componentIndex - index to a specific component
 *  @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                              updated component data
 *  @param  trace_func     - EDAL trace function
 *
 *  @return
 *         none.      
 *
 *  @note
 *
 *  @version
 *    06-May-2013: Rui Chang - Created
 *
 **************************************************************************/
void
enclosure_access_printDimmInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                     fbe_enclosure_types_t enclosureType,
                                     fbe_u32_t componentIndex,
                                     fbe_bool_t printFullData, 
                                     fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t  generalDataPtr = NULL;
    edal_base_dimm_info_t     * dimmInfoPtr = NULL;
    edal_pe_dimm_info_t       * peDimmInfoPtr = NULL;
    fbe_u32_t                       index = 0;

    trace_func(NULL, "\n");

    // loop through all IO modules
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, " DIMM %d\n", index);
            }
            else
            {
                trace_func(NULL, "State Change to DIMM %d\n", index);
            }
            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);
            dimmInfoPtr = (edal_base_dimm_info_t *)generalDataPtr;

            enclosure_access_printPeGeneralComponentInfo(&(dimmInfoPtr->generalComponentInfo),
                                                       printFullData, trace_func);

            peDimmInfoPtr = (edal_pe_dimm_info_t *)generalDataPtr;

            enclosure_access_printPeDimmInfo(peDimmInfoPtr, trace_func);         
        }        
    }

    return;

}   // end of enclosure_access_printDimmInfo

/*!*************************************************************************
 *                         @fn enclosure_access_printPeDimmInfo
 **************************************************************************
 *  @brief
 *      This function prints out the processor enclosure DIMM info.
 *
 *  @param     peCacheCardInfoPtr
 *  @param     trace_func
 *
 *  @return
 *      None.
 *
 *  @note
 *
 *  @version
 *    06-May-2013: Rui Chang - Created
 *
 **************************************************************************/
void 
enclosure_access_printPeDimmInfo(edal_pe_dimm_info_t *peDimmInfoPtr, 
                                     fbe_edal_trace_func_t trace_func)
{
    fbe_u32_t                  index=0;
    fbe_board_mgmt_dimm_info_t * peDimmInfoSubPtr = NULL;
    peDimmInfoSubPtr = &peDimmInfoPtr->peDimmSubInfo.externalDimmInfo;

        
    trace_func(NULL, "  Processor Encl DIMM Info\n");
    trace_func(NULL, "    associatedSp : %d, dimmID : %d, isLocalFru : %d, envInterfaceStatus : %d\n",         
        peDimmInfoSubPtr->associatedSp,
        peDimmInfoSubPtr->dimmID,
        peDimmInfoSubPtr->isLocalFru,
        peDimmInfoSubPtr->envInterfaceStatus);

    trace_func(NULL, "    inserted : %d, dimmType: 0x%X, dimmRevision : 0x%X, dimmDensity : %d\n",         
        peDimmInfoSubPtr->inserted,
        peDimmInfoSubPtr->dimmType,
        peDimmInfoSubPtr->dimmRevision,
        peDimmInfoSubPtr->dimmDensity);

    trace_func(NULL, "    numOfBanks : %d, deviceWidth : %d, numberOfRanks : %d, faulted : %d\n",         
        peDimmInfoSubPtr->numOfBanks,
        peDimmInfoSubPtr->deviceWidth,
        peDimmInfoSubPtr->numberOfRanks,
        peDimmInfoSubPtr->faulted);

    trace_func(NULL, "    vendorRegionValid : %d, jedecIdCode : 0x%X, manufacturingLocation : %d\n",         
        peDimmInfoSubPtr->vendorRegionValid,
        peDimmInfoSubPtr->jedecIdCode[0],
        peDimmInfoSubPtr->manufacturingLocation);

    trace_func(NULL, "    state : %d\n",         
        peDimmInfoSubPtr->state);

    trace_func(NULL, "    faultRegFault : %s, peerFaultRegFault : %s, hwErrMonFault : %s\n", 
               BOOLEAN_TO_TEXT(peDimmInfoSubPtr->faultRegFault),
               BOOLEAN_TO_TEXT(peDimmInfoSubPtr->peerFaultRegFault),
               BOOLEAN_TO_TEXT(peDimmInfoSubPtr->hwErrMonFault));  
   
    trace_func(NULL, "    manufacturingDate :");

    for (index = 0; index < MANUFACTURING_DATE_SIZE; index++)
    {
        trace_func(NULL, " 0x%02X", peDimmInfoSubPtr->manufacturingDate[index]);
    }

    trace_func(NULL, ", moduleSerialNumber : ");

    for (index = 0; index < MODULE_SERIAL_NUMBER_SIZE; index++)
    {
        trace_func(NULL, "%02X", peDimmInfoSubPtr->moduleSerialNumber[index]);
    }
    trace_func(NULL, "\n");


    trace_func(NULL, "    partNumber : %s, ", peDimmInfoSubPtr->partNumber);

    trace_func(NULL, "OldEMCDimmSerialNumber : ");
    for (index = 0; index < OLD_EMC_DIMM_SERIAL_NUMBER_SIZE; index++)
    {
        trace_func(NULL, "%02X", peDimmInfoSubPtr->OldEMCDimmSerialNumber[index]);
    }
    trace_func(NULL, "\n");

    trace_func(NULL, "    EMCDimmPartNumber : %s", peDimmInfoSubPtr->EMCDimmPartNumber);

    trace_func(NULL, ", EMCDimmSerialNumber : ");        
    for (index = 0; index < EMC_DIMM_SERIAL_NUMBER_SIZE; index++)
    {
        trace_func(NULL, "%02X", peDimmInfoSubPtr->EMCDimmSerialNumber[index]);
    }
    trace_func(NULL, "\n");        

    return;
}   // end of enclosure_access_printPeDimmInfo



/*!*************************************************************************
 *                         @fn enclosure_access_printSSDInfo
 **************************************************************************
 *  @brief
 *      This function prints out the SSD info.
 *
 *  @param  componentTypeDataPtr  - pointer to Component Data
 *  @param  componentIndex - index to a specific component
 *  @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                              updated component data
 *  @param  trace_func     - EDAL trace function
 *
 *  @return
 *         none.      
 *
 *  @note
 *
 *  @version
 *    29-Sep-2014: bphilbin - Created
 *
 **************************************************************************/
void
enclosure_access_printSSDInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                     fbe_enclosure_types_t enclosureType,
                                     fbe_u32_t componentIndex,
                                     fbe_bool_t printFullData, 
                                     fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t  generalDataPtr = NULL;
    edal_base_ssd_info_t     * ssdInfoPtr = NULL;
    edal_pe_ssd_info_t       * peSSDInfoPtr = NULL;
    fbe_u32_t                       index = 0;

    trace_func(NULL, "\n");

    // loop through all SSDs
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, " SSD %d\n", index);
            }
            else
            {
                trace_func(NULL, "State Change to SSD %d\n", index);
            }
            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);
            ssdInfoPtr = (edal_base_ssd_info_t *)generalDataPtr;

            enclosure_access_printPeGeneralComponentInfo(&(ssdInfoPtr->generalComponentInfo),
                                                       printFullData, trace_func);

            peSSDInfoPtr = (edal_pe_ssd_info_t *)generalDataPtr;

            enclosure_access_printPeSSDInfo(peSSDInfoPtr, trace_func);         
        }        
    }

    return;

}   // end of enclosure_access_printSSDInfo

/*!*************************************************************************
 *                         @fn enclosure_access_printPeSSDInfo
 **************************************************************************
 *  @brief
 *      This function prints out the processor enclosure DIMM info.
 *
 *  @param     peCacheCardInfoPtr
 *  @param     trace_func
 *
 *  @return
 *      None.
 *
 *  @note
 *
 *  @version
 *    29-Sep-20143: bphilbin - Created
 *
 **************************************************************************/
void 
enclosure_access_printPeSSDInfo(edal_pe_ssd_info_t *peSSDInfoPtr, 
                                     fbe_edal_trace_func_t trace_func)
{
    fbe_board_mgmt_ssd_info_t * peSSDInfoSubPtr = NULL;
    peSSDInfoSubPtr = &peSSDInfoPtr->ssdInfo;

        
    trace_func(NULL, "  Processor Encl SSD Info\n");
    trace_func(NULL, "  slot : %d\n", 
        peSSDInfoSubPtr->slot);

    trace_func(NULL, "    remainingSpareBlkCount : %d, ssdLiveUsed: %d, ssdSelfTestPassed : %d\n",         
        peSSDInfoSubPtr->remainingSpareBlkCount,
        peSSDInfoSubPtr->ssdLifeUsed,
        peSSDInfoSubPtr->ssdSelfTestPassed);      

    trace_func(NULL, "    serialNo : %s, partNo : %s\n",         
        &peSSDInfoSubPtr->ssdSerialNumber[0],
        &peSSDInfoSubPtr->ssdPartNumber[0]);      

    trace_func(NULL, "    assemblyName : %s, firmwareRev: %s\n",         
        &peSSDInfoSubPtr->ssdAssemblyName[0],
        &peSSDInfoSubPtr->ssdFirmwareRevision[0]); 
    trace_func(NULL, "    isPeerFaultRegFail : %d\n",         
        peSSDInfoSubPtr->isPeerFaultRegFail);     

    return;
}   // end of enclosure_access_printPeSSDInfo

/*!*************************************************************************
 *                         @fn enclosure_access_printMiscInfo
 **************************************************************************
 *  @brief
 *      This function prints out the miscellaneous info.
 *
 *  @param  componentTypeDataPtr  - pointer to Component Data
 *  @param  componentIndex - index to a specific component
 *  @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                              updated component data
 *  @param  trace_func     - EDAL trace function
 *
 *  @return
 *         none.      
 *
 *  @note
 *
 *  @version
 *    07-Apr-2010: PHE - Created
 *
 **************************************************************************/
void
enclosure_access_printMiscInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                     fbe_enclosure_types_t enclosureType,
                                     fbe_u32_t componentIndex,
                                     fbe_bool_t printFullData, 
                                     fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t  generalDataPtr = NULL;
    edal_base_misc_info_t         * baseMiscInfoPtr = NULL;
    edal_pe_misc_info_t           * peMiscInfoPtr = NULL;
    fbe_u32_t                       index = 0;

    trace_func(NULL, "\n");

    // loop through all IO modules
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, " Misc %d\n", index);
            }
            else
            {
                trace_func(NULL, "State Change to Misc %d\n", index);
            }
            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);
            baseMiscInfoPtr = (edal_base_misc_info_t *)generalDataPtr;

            enclosure_access_printPeGeneralComponentInfo(&(baseMiscInfoPtr->generalComponentInfo),
                                                       printFullData, trace_func);

            peMiscInfoPtr = (edal_pe_misc_info_t *)generalDataPtr;

            enclosure_access_printPeMiscInfo(peMiscInfoPtr, trace_func);         
        }        
    }

    return;

}   // end of enclosure_access_printMiscInfo

/*!*************************************************************************
 *                         @fn enclosure_access_printPeMiscInfo
 **************************************************************************
 *  @brief
 *      This function prints out the processor enclosure miscellaneous info.
 *
 *  @param     pePlatformInfoPtr
 *  @param     trace_func
 *
 *  @return
 *      None.
 *
 *  @note
 *
 *  @version
 *    07-Apr-2010: PHE - Created
 *
 **************************************************************************/
void 
enclosure_access_printPeMiscInfo(edal_pe_misc_info_t *peMiscInfoPtr, 
                                          fbe_edal_trace_func_t trace_func)
{
    fbe_board_mgmt_misc_info_t *peMiscSubInfoPtr = NULL;

    peMiscSubInfoPtr = &peMiscInfoPtr->peMiscSubInfo;
    
    trace_func(NULL, "  Processor Encl Misc Info\n");

    trace_func(NULL, "    spid : %d, engineIdFault : %d, peerInserted : %d, lowBattery : %d, internalCableStat: %d, smbSelectBitsFailed : %d\n", 
        peMiscSubInfoPtr->spid,
        peMiscSubInfoPtr->engineIdFault,
        peMiscSubInfoPtr->peerInserted,
        peMiscSubInfoPtr->lowBattery,
        peMiscSubInfoPtr->fbeInternalCablePort1,
        peMiscSubInfoPtr->smbSelectBitsFailed);
   
    trace_func(NULL, "    SPFaultLED : %d, UnsafeToRemoveLED : %d, EnclosureFaultLED : %d, SPFaultLEDColor : %d\n", 
        peMiscSubInfoPtr->SPFaultLED,
        peMiscSubInfoPtr->UnsafeToRemoveLED,
        peMiscSubInfoPtr->EnclosureFaultLED,
        peMiscSubInfoPtr->SPFaultLEDColor);

    trace_func(NULL, "    ManagementPortLED : %d, ServicePortLED : %d, enclFaultLedReason: 0x%llX\n", 
        peMiscSubInfoPtr->ManagementPortLED,
        peMiscSubInfoPtr->ServicePortLED,
        peMiscSubInfoPtr->enclFaultLedReason);
    
    return;
}   // end of enclosure_access_printPeMiscInfo


/*!*************************************************************************
 *                   @fn enclosure_access_printMgmtModuleInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace Management module data.
 *
 *  PARAMETERS:
 *      @param  componentTypeDataPtr  - pointer to Component Data
 *      @param  componentIndex - index to a specific component
 *      @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                               updated component data
 *      @param  trace_func     - EDAL trace function
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    07-Apr-2010: Arun S - Created.
 *
 **************************************************************************/
void
enclosure_access_printMgmtModuleInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                     fbe_enclosure_types_t enclosureType,
                                     fbe_u32_t componentIndex,
                                     fbe_bool_t printFullData, 
                                     fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t  generalDataPtr = NULL;
    edal_base_mgmt_module_info_t    *baseMgmtModuleInfoPtr = NULL;
    edal_pe_mgmt_module_info_t      *peMgmtModuleInfoPtr = NULL;
    fbe_u32_t                       index = 0;

    if (componentTypeDataPtr->componentType != FBE_ENCL_MGMT_MODULE)
    {
        return;
    }

    trace_func(NULL, "\n");

    // loop through all Management modules
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, "  Mgmt Module %d\n", index);
            }
            else
            {
                trace_func(NULL, "State Change to Mgmt Module %d\n", index);
            }
            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);

            baseMgmtModuleInfoPtr = (edal_base_mgmt_module_info_t *)generalDataPtr;
            enclosure_access_printPeGeneralComponentInfo(&(baseMgmtModuleInfoPtr->generalComponentInfo),
                                                       printFullData, trace_func);

            peMgmtModuleInfoPtr = (edal_pe_mgmt_module_info_t *)generalDataPtr;            

            enclosure_access_printPeMgmtModuleInfo(peMgmtModuleInfoPtr, trace_func);         
        }        
    }

    return;

}   // end of enclosure_access_printMgmtModuleInfo

/*!*************************************************************************
 *                 @fn enclosure_access_printPeMgmtModuleInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace some or all of the Processor Enclosure
 *       Management module data.
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    07-Apr-2010: Arun S - Created
 *
 **************************************************************************/
void 
enclosure_access_printPeMgmtModuleInfo(edal_pe_mgmt_module_info_t *peMgmtModuleInfoPtr, 
                                          fbe_edal_trace_func_t trace_func)
{
    edal_pe_mgmt_module_sub_info_t * peMgmtModuleSubInfoPtr = NULL;
    fbe_board_mgmt_mgmt_module_info_t * extMgmtModuleInfoPtr = NULL;
    PORT_STS_CONFIG * portConfigPtr = NULL;

    peMgmtModuleSubInfoPtr = &peMgmtModuleInfoPtr->peMgmtModuleSubInfo;
    extMgmtModuleInfoPtr = &peMgmtModuleSubInfoPtr->externalMgmtModuleInfo;
        
    trace_func(NULL, "  Processor Encl Management module Info\n");

    trace_func(NULL, "    associatedSp : %d, mgmtID : %d, isLocalFru %d\n",         
        extMgmtModuleInfoPtr->associatedSp,
        extMgmtModuleInfoPtr->mgmtID,
        extMgmtModuleInfoPtr->isLocalFru);

    trace_func(NULL, "    xactStatus : 0x%x, envInterfaceStatus : %d\n", 
        extMgmtModuleInfoPtr->transactionStatus,
        extMgmtModuleInfoPtr->envInterfaceStatus); 

    trace_func(NULL, "    inserted : %d, uniqueID : %d, generalFault : %d, faultInfoStatus : 0x%x\n", 
        extMgmtModuleInfoPtr->bInserted,
        extMgmtModuleInfoPtr->uniqueID,
        extMgmtModuleInfoPtr->generalFault,
        extMgmtModuleInfoPtr->faultInfoStatus);

    trace_func(NULL, "    faultLEDStatus : %d, vLanConfigMode : %d\n", 
        extMgmtModuleInfoPtr->faultLEDStatus,
        extMgmtModuleInfoPtr->vLanConfigMode);

    trace_func(NULL, "    firmwareRevMajor : %d, firmwareRevMinor : %d\n", 
        extMgmtModuleInfoPtr->firmwareRevMajor,
        extMgmtModuleInfoPtr->firmwareRevMinor);
    
    trace_func(NULL, "    isFaultRegFail : %s\n", 
        BOOLEAN_TO_TEXT(extMgmtModuleInfoPtr->isFaultRegFail));   

    trace_func(NULL, "    mgmgPort - linkStatus : %d, portSpeed : %d, portDuplex : %d, autoNegotiate : 0x%x\n", 
        extMgmtModuleInfoPtr->managementPort.linkStatus,
        extMgmtModuleInfoPtr->managementPort.mgmtPortSpeed,
        extMgmtModuleInfoPtr->managementPort.mgmtPortDuplex,
        extMgmtModuleInfoPtr->managementPort.mgmtPortAutoNeg);

    portConfigPtr = &extMgmtModuleInfoPtr->servicePort;

    trace_func(NULL, "    servicePort - linkStatus : %d, portSpeed : %d, portDuplex : %d, autoNegotiate : 0x%x\n", 
        portConfigPtr->linkStatus,
        portConfigPtr->portSpeed,
        portConfigPtr->portDuplex,
        portConfigPtr->autoNegotiate);
    
    return;
}   // end of enclosure_access_printPeMgmtModuleInfo

/*!*************************************************************************
 *                   @fn enclosure_access_printBmcData
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace BMC data.
 *
 *  PARAMETERS:
 *      @param  componentTypeDataPtr  - pointer to Component Data
 *      @param  componentIndex - index to a specific component
 *      @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                               updated component data
 *      @param  trace_func     - EDAL trace function
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    13-Sep-2012: Eric Zhou - Created.
 *
 **************************************************************************/
void
enclosure_access_printBmcData(fbe_enclosure_component_t   *componentTypeDataPtr,
                                     fbe_enclosure_types_t enclosureType,
                                     fbe_u32_t componentIndex,
                                     fbe_bool_t printFullData, 
                                     fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t  generalDataPtr = NULL;
    edal_base_bmc_info_t            *baseBmcInfoPtr = NULL;
    edal_pe_bmc_info_t              *peBmcInfoPtr = NULL;
    fbe_u32_t                       index = 0;

    if (componentTypeDataPtr->componentType != FBE_ENCL_BMC)
    {
        return;
    }

    trace_func(NULL, "\n");

    // loop through all BMCs
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, "  BMC %d\n", index);
            }
            else
            {
                trace_func(NULL, "State Change to BMC %d\n", index);
            }
            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);

            baseBmcInfoPtr = (edal_base_bmc_info_t *)generalDataPtr;
            enclosure_access_printPeGeneralComponentInfo(&(baseBmcInfoPtr->generalComponentInfo),
                                                       printFullData, trace_func);

            peBmcInfoPtr = (edal_pe_bmc_info_t *)generalDataPtr;

            enclosure_access_printPeBmcInfo(peBmcInfoPtr, trace_func);
        }        
    }

    return;
}

/*!*************************************************************************
 *                 @fn enclosure_access_printPeBmcInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace some or all of the Processor Enclosure
 *       BMC data.
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    13-Sep-2012: Eric Zhou - Created
 *
 **************************************************************************/
void 
enclosure_access_printPeBmcInfo(edal_pe_bmc_info_t *peBmcInfoPtr, 
                                          fbe_edal_trace_func_t trace_func)
{
    edal_pe_bmc_sub_info_t      * peBmcSubInfoPtr = NULL;
    fbe_board_mgmt_bmc_info_t   * extPeBmcInfoPtr = NULL;
    fbe_u32_t                   index;

    peBmcSubInfoPtr = &peBmcInfoPtr->peBmcSubInfo;
    extPeBmcInfoPtr = &peBmcSubInfoPtr->externalBmcInfo;

    trace_func(NULL, "  Processor Encl BMC Info\n");

    trace_func(NULL, "    associatedSp                  : %s\n", decodeSpId(extPeBmcInfoPtr->associatedSp));
    trace_func(NULL, "    bmcID                         : %d\n", extPeBmcInfoPtr->bmcID);
    trace_func(NULL, "    isLocalFru                    : %s\n", BOOLEAN_TO_TEXT(extPeBmcInfoPtr->isLocalFru));

    trace_func(NULL, "    transactionStatus             : %d\n", extPeBmcInfoPtr->transactionStatus);
    trace_func(NULL, "    envInterfaceStatus            : %d\n", extPeBmcInfoPtr->envInterfaceStatus);

    if (extPeBmcInfoPtr->transactionStatus != EMCPAL_STATUS_SUCCESS)
    {
        return;
    }

    trace_func(NULL, "    shutdownWarning               : %s\n", BOOLEAN_TO_TEXT(extPeBmcInfoPtr->shutdownWarning));
    trace_func(NULL, "    _shutdownReason_\n");
    trace_func(NULL, "      embedIOOverTemp             : %s\n", BOOLEAN_TO_TEXT(extPeBmcInfoPtr->shutdownReason.embedIOOverTemp));
    trace_func(NULL, "      fanFault                    : %s\n", BOOLEAN_TO_TEXT(extPeBmcInfoPtr->shutdownReason.fanFault));
    trace_func(NULL, "      slicOverTemp                : %s\n", BOOLEAN_TO_TEXT(extPeBmcInfoPtr->shutdownReason.slicOverTemp));
    trace_func(NULL, "      diskOverTemp                : %s\n", BOOLEAN_TO_TEXT(extPeBmcInfoPtr->shutdownReason.diskOverTemp));
    trace_func(NULL, "      psOverTemp                  : %s\n", BOOLEAN_TO_TEXT(extPeBmcInfoPtr->shutdownReason.psOverTemp));
    trace_func(NULL, "      memoryOverTemp              : %s\n", BOOLEAN_TO_TEXT(extPeBmcInfoPtr->shutdownReason.memoryOverTemp));
    trace_func(NULL, "      cpuOverTemp                 : %s\n", BOOLEAN_TO_TEXT(extPeBmcInfoPtr->shutdownReason.cpuOverTemp));
    trace_func(NULL, "      ambientOverTemp             : %s\n", BOOLEAN_TO_TEXT(extPeBmcInfoPtr->shutdownReason.ambientOverTemp));
    trace_func(NULL, "      sspHang                     : %s\n", BOOLEAN_TO_TEXT(extPeBmcInfoPtr->shutdownReason.sspHang));

    trace_func(NULL, "    shutdownTimeRemaining         : %d\n", extPeBmcInfoPtr->shutdownTimeRemaining);

    if (extPeBmcInfoPtr->rideThroughTimeSupported)
    {
        trace_func(NULL, "    lowPowerMode                  : %s\n", BOOLEAN_TO_TEXT(extPeBmcInfoPtr->lowPowerMode)); 
        trace_func(NULL, "    rideThroughTime               : %d\n", extPeBmcInfoPtr->rideThroughTime);
        trace_func(NULL, "    vaultTimeout                  : %d\n", extPeBmcInfoPtr->vaultTimeout);
    }

    // BIST attributes
    trace_func(NULL, "    _BIST Report_\n");
    trace_func(NULL, "    cpuTest                       : %s\n", decodeBMCBistResult(extPeBmcInfoPtr->bistReport.cpuTest));
    trace_func(NULL, "    dramTest                      : %s\n", decodeBMCBistResult(extPeBmcInfoPtr->bistReport.dramTest));
    trace_func(NULL, "    sramTest                      : %s\n", decodeBMCBistResult(extPeBmcInfoPtr->bistReport.sramTest));

    for (index = 0; index < I2C_TEST_MAX; index++)
    {
        trace_func(NULL, "    i2cTest %d                   : %s\n", index, decodeBMCBistResult(extPeBmcInfoPtr->bistReport.i2cTests[index])); 
    }

    for (index = 0; index < ARB_TEST_MAX; index++)
    {
        trace_func(NULL, "    arbTest %d                    : %s\n", index, decodeBMCBistResult(extPeBmcInfoPtr->bistReport.arbTests[index]));
    }

    for (index = 0; index < FAN_TEST_MAX; index++)
    {
        trace_func(NULL, "    fanTest %d                    : %s\n", index, decodeBMCBistResult(extPeBmcInfoPtr->bistReport.fanTests[index]));
    }

    for (index = 0; index < BBU_TEST_MAX; index++)
    {
        trace_func(NULL, "    bbuTest %d                    : %s\n", index, decodeBMCBistResult(extPeBmcInfoPtr->bistReport.bbuTests[index]));
    }

    for (index = 0; index < UART_TEST_MAX; index++)
    {
        trace_func(NULL, "    uartTest %d                   : %s\n", index, decodeBMCBistResult(extPeBmcInfoPtr->bistReport.uartTests[index]));
    }

    trace_func(NULL, "    sspTest                       : %s\n", decodeBMCBistResult(extPeBmcInfoPtr->bistReport.sspTest));

    trace_func(NULL, "    nvramTest                     : %s\n", decodeBMCBistResult(extPeBmcInfoPtr->bistReport.nvramTest));
    trace_func(NULL, "    sgpioTest                     : %s\n", decodeBMCBistResult(extPeBmcInfoPtr->bistReport.sgpioTest));
    trace_func(NULL, "    ncsiLanTest                   : %s\n", decodeBMCBistResult(extPeBmcInfoPtr->bistReport.ncsiLanTest));
    trace_func(NULL, "    dedicatedLanTest              : %s\n", decodeBMCBistResult(extPeBmcInfoPtr->bistReport.dedicatedLanTest));

    trace_func(NULL, "    _BMC App Main Firmware Status_\n");
    trace_func(NULL, "      updateStatus                : %s\n", decodeFirmwareUpdateStatus(extPeBmcInfoPtr->firmwareStatus.bmcAppMain.updateStatus));
    trace_func(NULL, "      Firmware Revision           : %d.%d\n",
        extPeBmcInfoPtr->firmwareStatus.bmcAppMain.revisionMajor,
        extPeBmcInfoPtr->firmwareStatus.bmcAppMain.revisionMinor);

    trace_func(NULL, "    _BMC App Redundant Firmware Status_\n");
    trace_func(NULL, "      updateStatus                : %s\n", decodeFirmwareUpdateStatus(extPeBmcInfoPtr->firmwareStatus.bmcAppRedundant.updateStatus));
    trace_func(NULL, "      Firmware Revision           : %d.%d\n",
        extPeBmcInfoPtr->firmwareStatus.bmcAppRedundant.revisionMajor,
        extPeBmcInfoPtr->firmwareStatus.bmcAppRedundant.revisionMinor);

    trace_func(NULL, "    _BMC SSP Firmware Status_\n");
    trace_func(NULL, "      updateStatus                : %s\n", decodeFirmwareUpdateStatus(extPeBmcInfoPtr->firmwareStatus.ssp.updateStatus));
    trace_func(NULL, "      Firmware Revision           : %d.%d\n",
        extPeBmcInfoPtr->firmwareStatus.ssp.revisionMajor,
        extPeBmcInfoPtr->firmwareStatus.ssp.revisionMinor);

    trace_func(NULL, "    _BMC Boot Block Firmware Status_\n");
    trace_func(NULL, "      updateStatus                : %s\n", decodeFirmwareUpdateStatus(extPeBmcInfoPtr->firmwareStatus.bootBlock.updateStatus));
    trace_func(NULL, "      Firmware Revision           : %d.%d\n",
        extPeBmcInfoPtr->firmwareStatus.bootBlock.revisionMajor,
        extPeBmcInfoPtr->firmwareStatus.bootBlock.revisionMinor);

    return;

}

/*!*************************************************************************
 *                   @fn enclosure_access_printFltRegInfo
 **************************************************************************
 *  @brief
 *      This function will trace fault register data.
 *
 *  @param  componentTypeDataPtr  - pointer to Component Data
 *  @param  componentIndex - index to a specific component
 *  @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                               updated component data
 *  @param  trace_func     - EDAL trace function
 *
 *  @return none.
 *
 *  @note
 *
 *  @version
 *    17-July-2012: Chengkai Hu - Created.
 *
 **************************************************************************/
void
enclosure_access_printFltRegInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                  fbe_enclosure_types_t enclosureType,
                                  fbe_u32_t componentIndex,
                                  fbe_bool_t printFullData, 
                                  fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t  generalDataPtr = NULL;
    edal_base_flt_exp_info_t       *baseFltExpInfoPtr = NULL;
    edal_pe_flt_exp_info_t         *peFltExpInfoPtr = NULL;
    fbe_u32_t                       index = 0;

    if (componentTypeDataPtr->componentType != FBE_ENCL_FLT_REG)
    {
        return;
    }

    trace_func(NULL, "\n");

    // Loop through all fault registers.
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // Check if we should print this component index.
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, "  Fault Register %d\n", index);
            }
            else
            {
                trace_func(NULL, "State Change to Fault Register %d\n", index);
            }
            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);

            baseFltExpInfoPtr = (edal_base_flt_exp_info_t *)generalDataPtr;
            enclosure_access_printPeGeneralComponentInfo(&(baseFltExpInfoPtr->generalComponentInfo),
                                                       printFullData, trace_func);

            peFltExpInfoPtr = (edal_pe_flt_exp_info_t *)generalDataPtr;            

            enclosure_access_printPeFltRegInfo(peFltExpInfoPtr, trace_func);         
        }        
    }

    return;

}   // end of enclosure_access_printFltRegInfo

/*!*************************************************************************
 *  @fn enclosure_access_printPeFltRegInfo(edal_pe_flt_exp_info_t *peFltExpInfoPtr, 
 *                                         fbe_edal_trace_func_t trace_func)
 **************************************************************************
 *  @brief
 *      This function will trace some or all of the Processor Enclosure
 *       Fault Register data.
 *
 *  @param  peFltExpInfoPtr - fault register handle in PE
 *  @param  trace_func
 *
 *  @return none.
 *      
 *  @note
 *
 *  @version
 *    17-July-2012: Chengkai Hu - Created.
 **************************************************************************/
void 
enclosure_access_printPeFltRegInfo(edal_pe_flt_exp_info_t *peFltExpInfoPtr, 
                                          fbe_edal_trace_func_t trace_func)
{
    edal_pe_flt_exp_sub_info_t    * peFltExpSubInfoPtr = NULL;
    fbe_peer_boot_flt_exp_info_t  * extFltRegInfoPtr = NULL;
    SPECL_FAULT_REGISTER_STATUS   * fltRegStatusPtr = NULL;
    fbe_u32_t                       eachFault = 0;
    fbe_u32_t                       width = 25;

    peFltExpSubInfoPtr = &peFltExpInfoPtr->peFltExpSubInfo;
    extFltRegInfoPtr = &peFltExpSubInfoPtr->externalFltExpInfo;
    fltRegStatusPtr = &extFltRegInfoPtr->faultRegisterStatus;

    trace_func(NULL, "    associatedSp : %d, fltExpID : %d, xactionStatus : 0x%x\n",          
        extFltRegInfoPtr->associatedSp,
        extFltRegInfoPtr->fltExpID,
        extFltRegInfoPtr->transactionStatus);

    trace_func(NULL, "  Processor Encl Fault Register Info\n\n");
    
    if (fltRegStatusPtr->anyFaultsFound)
    {
        for (eachFault = 0; eachFault < MAX_DIMM_FAULT; eachFault++)
        {
            if (fltRegStatusPtr->dimmFault[eachFault])
            {
                trace_func(NULL, "\t %-*s %2d : %s\n",    width - 3,  "Failure DIMM",     eachFault,
                                                                                BOOLEAN_TO_TEXT(fltRegStatusPtr->dimmFault[eachFault]));
            }
        }

        for (eachFault = 0; eachFault < MAX_DRIVE_FAULT; eachFault++)
        {
            if (fltRegStatusPtr->driveFault[eachFault])
            {
                trace_func(NULL, "\t %-*s %2d : %s\n",    width - 3,  "Failure Drive",    eachFault,
                                                                                BOOLEAN_TO_TEXT(fltRegStatusPtr->driveFault[eachFault]));
            }
        }

        for (eachFault = 0; eachFault < MAX_SLIC_FAULT; eachFault++)
        {
            if (fltRegStatusPtr->slicFault[eachFault])
            {
                trace_func(NULL, "\t %-*s %2d : %s\n",    width - 3,  "Failure SLIC",     eachFault,
                                                                                BOOLEAN_TO_TEXT(fltRegStatusPtr->slicFault[eachFault]));
            }
        }

        for (eachFault = 0; eachFault < MAX_POWER_FAULT; eachFault++)
        {
            if (fltRegStatusPtr->powerFault[eachFault])
            {
                trace_func(NULL, "\t %-*s %d : %s\n",     width - 3,  "Failure Power",    eachFault,
                                                                                BOOLEAN_TO_TEXT(fltRegStatusPtr->powerFault[eachFault]));
            }
        }

        for (eachFault = 0; eachFault < MAX_BATTERY_FAULT; eachFault++)
        {
            if (fltRegStatusPtr->batteryFault[eachFault])
            {
                trace_func(NULL, "\t %-*s %d : %s\n",     width - 3,  "Failure Battery",    eachFault,
                                                                                BOOLEAN_TO_TEXT(fltRegStatusPtr->batteryFault[eachFault]));
            }
        }

        if (fltRegStatusPtr->superCapFault)
        {
            trace_func(NULL, "\t %-*s : %s\n",            width,      "SuperCap Failure", BOOLEAN_TO_TEXT(fltRegStatusPtr->superCapFault));
        }

        for (eachFault = 0; eachFault < MAX_FAN_FAULT; eachFault++)
        {
            if (fltRegStatusPtr->fanFault[eachFault])
            {
                trace_func(NULL, "\t %-*s %d : %s\n",     width - 2,  "Failure Fan",      eachFault,
                                                                                BOOLEAN_TO_TEXT(fltRegStatusPtr->fanFault[eachFault]));
            }
        }

        for (eachFault = 0; eachFault < MAX_I2C_FAULT; eachFault++)
        {
            if (fltRegStatusPtr->i2cFault[eachFault])
            {
                trace_func(NULL, "\t %-*s %d : %s\n",     width - 2,  "Failure I2C Bus",  eachFault,
                                                                                BOOLEAN_TO_TEXT(fltRegStatusPtr->i2cFault[eachFault]));
            }
        }

        if (fltRegStatusPtr->cpuFault)
        {
            trace_func(NULL, "\t %-*s : %s\n",            width,      "CPU Failure",      BOOLEAN_TO_TEXT(fltRegStatusPtr->cpuFault));
        }

        if (fltRegStatusPtr->mgmtFault)
        {
            trace_func(NULL, "\t %-*s : %s\n",            width,      "Mgmt Failure",     BOOLEAN_TO_TEXT(fltRegStatusPtr->mgmtFault));
        }

        if (fltRegStatusPtr->bemFault)
        {
            trace_func(NULL, "\t %-*s : %s\n",            width,      "BEM Failure",      BOOLEAN_TO_TEXT(fltRegStatusPtr->bemFault));
        }

        if (fltRegStatusPtr->eFlashFault)
        {
            trace_func(NULL, "\t %-*s : %s\n",            width,      "eFlash Failure",   BOOLEAN_TO_TEXT(fltRegStatusPtr->eFlashFault));
        }

        if (fltRegStatusPtr->cacheCardFault)
        {
            trace_func(NULL, "\t %-*s : %s\n",            width,      "Cache Card Failure",  BOOLEAN_TO_TEXT(fltRegStatusPtr->cacheCardFault));
        }

        if (fltRegStatusPtr->midplaneFault)
        {
            trace_func(NULL, "\t %-*s : %s\n",            width,      "Midplane Failure", BOOLEAN_TO_TEXT(fltRegStatusPtr->midplaneFault));
        }

        if (fltRegStatusPtr->cmiFault)
        {
            trace_func(NULL, "\t %-*s : %s\n",            width,      "CMI Failure",      BOOLEAN_TO_TEXT(fltRegStatusPtr->cmiFault));
        }

        if (fltRegStatusPtr->allFrusFault)
        {
            trace_func(NULL, "\t %-*s : %s\n",            width,      "All Frus Faulted", BOOLEAN_TO_TEXT(fltRegStatusPtr->allFrusFault));
        }
    }
    trace_func(NULL, "\t %-*s : %s\n",            width,  "Any Faults Found",         BOOLEAN_TO_TEXT(fltRegStatusPtr->anyFaultsFound));

    trace_func(NULL, "\t %-*s : 0x%08X (%s)\n",   width,  "CPU Status Register",      fltRegStatusPtr->cpuStatusRegister,
                                                                            decodeCpuStatusRegister(fltRegStatusPtr->cpuStatusRegister));
    
    return;
}   // end of enclosure_access_printPeFltRegInfo

/*!*************************************************************************
 *                   @fn enclosure_access_printSlavePortInfo
 **************************************************************************
 *  @brief
 *      This function will trace slave port data.
 *
 *  @param  componentTypeDataPtr  - pointer to Component Data
 *  @param  componentIndex - index to a specific component
 *  @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                               updated component data
 *  @param  trace_func     - EDAL trace function
 *
 *  @return none.
 *
 *  @note
 *
 *  @version
 *    20-Apr-2010: PHE - Created.
 *
 **************************************************************************/
void
enclosure_access_printSlavePortInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                  fbe_enclosure_types_t enclosureType,
                                  fbe_u32_t componentIndex,
                                  fbe_bool_t printFullData, 
                                  fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t  generalDataPtr = NULL;
    edal_base_slave_port_info_t    *baseSlavePortInfoPtr = NULL;
    edal_pe_slave_port_info_t      *peSlavePortInfoPtr = NULL;
    fbe_u32_t                       index = 0;

    if (componentTypeDataPtr->componentType != FBE_ENCL_SLAVE_PORT)
    {
        return;
    }

    trace_func(NULL, "\n");

    // Loop through all slave ports.
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // Check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, "  Slave Port %d\n", index);
            }
            else
            {
                trace_func(NULL, "State Change to Slave Port %d\n", index);
            }
            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);

            baseSlavePortInfoPtr = (edal_base_slave_port_info_t *)generalDataPtr;
            enclosure_access_printPeGeneralComponentInfo(&(baseSlavePortInfoPtr->generalComponentInfo),
                                                       printFullData, trace_func);

            peSlavePortInfoPtr = (edal_pe_slave_port_info_t *)generalDataPtr;            

            enclosure_access_printPeSlavePortInfo(peSlavePortInfoPtr, trace_func);         
        }        
    }

    return;

}   // end of enclosure_access_printSlavePortInfo

/*!*************************************************************************
 *  @fn enclosure_access_printPeSlavePortInfo(edal_pe_slave_port_info_t *peSlavePortPtr, 
 *                                         fbe_edal_trace_func_t trace_func)
 **************************************************************************
 *  @brief
 *      This function will trace some or all of the Processor Enclosure
 *       Slave Port data.
 *
 *  @param  peSlavePortPtr
 *  @param  trace_func
 *
 *  @return none.
 *      
 *  @note
 *
 *  @version
 *    20-Apr-2010: PHE - Created.
 *
 **************************************************************************/
void 
enclosure_access_printPeSlavePortInfo(edal_pe_slave_port_info_t *peSlavePortInfoPtr, 
                                          fbe_edal_trace_func_t trace_func)
{
    edal_pe_slave_port_sub_info_t * peSlavePortSubInfoPtr = NULL;
    fbe_board_mgmt_slave_port_info_t * extSlavePortInfoPtr = NULL;

    peSlavePortSubInfoPtr = &peSlavePortInfoPtr->peSlavePortSubInfo;
    extSlavePortInfoPtr = &peSlavePortSubInfoPtr->externalSlavePortInfo;
    
    trace_func(NULL, "  Processor Encl Slave Port Info\n");

    trace_func(NULL, "    associatedSp : %d, slavePortID : %d, isLocalFru : %d, xactionStatus : 0x%x\n",         
        extSlavePortInfoPtr->associatedSp,
        extSlavePortInfoPtr->slavePortID,
        extSlavePortInfoPtr->isLocalFru,
        extSlavePortInfoPtr->transactionStatus);

    trace_func(NULL, "    envInterfaceStatus : %d, generalStatus : 0x%x, componentStatus : 0x%x, componentExtStatus : 0x%x\n",         
        extSlavePortInfoPtr->envInterfaceStatus,
        extSlavePortInfoPtr->generalStatus,
        extSlavePortInfoPtr->componentStatus,
        extSlavePortInfoPtr->componentExtStatus);

    return;
}   // end of enclosure_access_printPeSlavePortInfo

/*!*************************************************************************
 *                   @fn enclosure_access_printTemperatureInfo
 **************************************************************************
 *  @brief
 *      This function will trace Temperature data.
 *
 *  @param  componentTypeDataPtr  - pointer to Component Data
 *  @param  componentIndex - index to a specific component
 *  @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                               updated component data
 *  @param  trace_func     - EDAL trace function
 *
 *  @return none.
 *
 *  @note
 *
 *  @version
 *    23-Feb-2011: Joe Perry - Created.
 *
 **************************************************************************/
void
enclosure_access_printTemperatureInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                      fbe_enclosure_types_t enclosureType,
                                      fbe_u32_t componentIndex,
                                      fbe_bool_t printFullData, 
                                      fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t  generalDataPtr = NULL;
    fbe_encl_temp_sensor_info_t     *baseTempInfoPtr = NULL;
    edal_pe_temperature_info_t      *peTempInfoPtr = NULL;
    fbe_u32_t                       index = 0;

    if (componentTypeDataPtr->componentType != FBE_ENCL_TEMPERATURE)
    {
        return;
    }

    trace_func(NULL, "\n");

    // Loop through all Temperatures
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // Check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, "  Temperature %d\n", index);
            }
            else
            {
                trace_func(NULL, "State Change to Temperature %d\n", index);
            }
            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);

            baseTempInfoPtr = (fbe_encl_temp_sensor_info_t *)generalDataPtr;
            enclosure_access_printPeGeneralComponentInfo(&(baseTempInfoPtr->generalComponentInfo),
                                                       printFullData, trace_func);

            peTempInfoPtr = (edal_pe_temperature_info_t *)generalDataPtr;            

            enclosure_access_printPeTemperatureInfo(peTempInfoPtr, trace_func);         
        }        
    }

    return;

}   // end of enclosure_access_printTemperatureInfo

/*!*************************************************************************
 *  @fn enclosure_access_printPeTemperatureInfo(edal_pe_temperature_info_t *peTempPtr, 
 *                                              fbe_edal_trace_func_t trace_func)
 **************************************************************************
 *  @brief
 *      This function will trace some or all of the Processor Enclosure
 *       Temperature data.
 *
 *  @param  peTempPtr
 *  @param  trace_func
 *
 *  @return none.
 *      
 *  @note
 *
 *  @version
 *    23-Feb-2011: Joe Perry - Created.
 *
 **************************************************************************/
void 
enclosure_access_printPeTemperatureInfo(edal_pe_temperature_info_t *peTempPtr, 
                                        fbe_edal_trace_func_t trace_func)
{
    edal_pe_temperature_sub_info_t  * peTempSubInfoPtr = NULL;
    fbe_eir_temp_sample_t           * extTempInfoPtr = NULL;

    peTempSubInfoPtr = &peTempPtr->peTempSubInfo;
    extTempInfoPtr = &peTempSubInfoPtr->externalTempInfo;
    
    trace_func(NULL, "  Processor Encl Temperature Info\n");

    trace_func(NULL, "    xactionStatus : 0x%x, peerEnvIntStatus : 0x%x\n",         
        peTempSubInfoPtr->transactionStatus,
        peTempSubInfoPtr->peerEnvInterfaceStatus);

    trace_func(NULL, "    airInletTemp : %d, status : 0x%x\n",         
        extTempInfoPtr->airInletTemp,
        extTempInfoPtr->status);

    return;
}   // end of enclosure_access_printPeTemperatureInfo

/*!*************************************************************************
 *                   @fn enclosure_access_printSpsInfo
 **************************************************************************
 *  @brief
 *      This function will trace SPS data.
 *
 *  @param  componentTypeDataPtr  - pointer to Component Data
 *  @param  componentIndex - index to a specific component
 *  @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                               updated component data
 *  @param  trace_func     - EDAL trace function
 *
 *  @return none.
 *
 *  @note
 *
 *  @version
 *    23-Feb-2011: Joe Perry - Created.
 *
 **************************************************************************/
void
enclosure_access_printSpsInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                              fbe_enclosure_types_t enclosureType,
                              fbe_u32_t componentIndex,
                              fbe_bool_t printFullData, 
                              fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t  generalDataPtr = NULL;
    edal_pe_sps_info_t              *peSpsInfoPtr = NULL;
    fbe_eses_sps_info_t             *spsDataPtr = NULL;
    fbe_u32_t                       index = 0;

    if (componentTypeDataPtr->componentType != FBE_ENCL_SPS)
    {
        return;
    }

    trace_func(NULL, "\n");

    // Loop through all SPS's
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // Check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, "  SPS %d\n", index);
            }
            else
            {
                trace_func(NULL, "State Change to SPS %d\n", index);
            }
            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);

            if (enclosureType != FBE_ENCL_PE)
            {
                spsDataPtr = (fbe_eses_sps_info_t *)generalDataPtr;
                enclosure_access_printGeneralComponentInfo(&(spsDataPtr->baseSpsInfo.generalComponentInfo),
                                                         printFullData, trace_func);
            
                if(IS_VALID_ENCL(enclosureType)) 
                {
                    if (printFullData)
                    {
                        enclosure_access_printEsesSpsInfo(spsDataPtr, trace_func);
                    }
                }
            }
            else
            {
                peSpsInfoPtr = (edal_pe_sps_info_t *)generalDataPtr;
                enclosure_access_printPeGeneralComponentInfo(&(peSpsInfoPtr->baseSpsInfo.generalComponentInfo),
                                                             printFullData, trace_func);
                enclosure_access_printPeSpsInfo(peSpsInfoPtr, trace_func);         
            }
        }        
    }

    return;

}   // end of enclosure_access_printSpsInfo


/*!*************************************************************************
 *                         @fn enclosure_access_printEsesSpsInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace some or all of the Enclosure Object's
 *      Component data.
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    15-Aug-2012: Joe Perry - Created
 *    21-Sep-2012: PHe - Changed the function name to indicate this is ESES level.
 *
 **************************************************************************/
void 
enclosure_access_printEsesSpsInfo (fbe_eses_sps_info_t *spsInfoPtr, 
                                  fbe_edal_trace_func_t trace_func)
{
    trace_func(NULL, "  ESES SPS Info\n");
    trace_func(NULL, "    ElemIndx : %d, SidelId : %d, spsSubEnclId %d\n", 
               spsInfoPtr->spsElementIndex,
               spsInfoPtr->spsSideId,
               spsInfoPtr->spsSubEnclId);
    trace_func(NULL, "    buffId %d, buffIdWr %d\n", 
               spsInfoPtr->bufferDescriptorInfo.bufferDescBufferId,
               spsInfoPtr->bufferDescriptorInfo.bufferDescBufferIdWriteable);
    trace_func(NULL, "    actRamId %d, actRamIdWr %d\n", 
               spsInfoPtr->bufferDescriptorInfo.bufferDescActRamID,
               spsInfoPtr->bufferDescriptorInfo.bufferDescActRamIdWriteable);
    trace_func(NULL, "    SpsStatus : 0x%x, SpsBatTime : 0x%x\n",
               spsInfoPtr->spsStatus,
               spsInfoPtr->spsBatTime);
    trace_func(NULL, "    spsFfid : 0x%x, spsBatteryFfid : 0x%x\n",
               spsInfoPtr->spsFfid,
               spsInfoPtr->spsBatteryFfid);

    trace_func(NULL, "    SPS SN : %.16s\n", &spsInfoPtr->spsSerialNumber[0]);
    trace_func(NULL, "    SPS Product Id : %.16s\n", &spsInfoPtr->spsProductId[0]);

    trace_func(NULL, "    Primary");
    enclosure_access_printFwRevInfo(&spsInfoPtr->primaryFwInfo, trace_func);
    trace_func(NULL, "    Secondary");
    enclosure_access_printFwRevInfo(&spsInfoPtr->secondaryFwInfo, trace_func);
    trace_func(NULL, "    Battery");
    enclosure_access_printFwRevInfo(&spsInfoPtr->batteryFwInfo, trace_func);

    return;
}   // end of enclosure_access_printEsesSpsInfo

/*!*************************************************************************
 *  @fn enclosure_access_printPeSpsInfo(edal_pe_sps_info_t *peSpsPtr, 
 *                                      fbe_edal_trace_func_t trace_func)
 **************************************************************************
 *  @brief
 *      This function will trace some or all of the Processor Enclosure
 *       SPS data.
 *
 *  @param  peSpsPtr
 *  @param  trace_func
 *
 *  @return none.
 *      
 *  @note
 *
 *  @version
 *    23-Feb-2011: Joe Perry - Created.
 *    16-Jan-2013: Lin Lou - Modified.
 *
 **************************************************************************/
void 
enclosure_access_printPeSpsInfo(edal_pe_sps_info_t *peSpsPtr, 
                                        fbe_edal_trace_func_t trace_func)
{
    edal_pe_sps_sub_info_t  * peSpsSubInfoPtr = NULL;
    fbe_base_sps_info_t     * extSpsInfoPtr = NULL;
    fbe_sps_manuf_info_t    * extSpsManufPtr = NULL;

    peSpsSubInfoPtr = &peSpsPtr->peSpsSubInfo;
    extSpsInfoPtr = &peSpsSubInfoPtr->externalSpsInfo;
    extSpsManufPtr = &peSpsSubInfoPtr->externalSpsManufInfo;
    
    trace_func(NULL, "  Processor Encl SPS Info\n");

    trace_func(NULL, "    xactionStatus : 0x%x, peerEnvIntStatus : 0x%x\n",         
               extSpsInfoPtr->transactionStatus,
               peSpsSubInfoPtr->peerEnvInterfaceStatus);

    trace_func(NULL, "    status : %d,, ModPresent : %d, BattPresent : %d, model : %d\n",         
               extSpsInfoPtr->spsStatus,
               extSpsInfoPtr->spsModulePresent,
               extSpsInfoPtr->spsBatteryPresent,
               extSpsInfoPtr->spsModel);
    trace_func(NULL, "    EOL : %d, ChgFail : %d, IntFlt : %d, BatNotEngFlt : %d, CableConn : %d\n",         
               extSpsInfoPtr->spsFaultInfo.spsBatteryEOL,
               extSpsInfoPtr->spsFaultInfo.spsChargerFailure,
               extSpsInfoPtr->spsFaultInfo.spsInternalFault,
               extSpsInfoPtr->spsFaultInfo.spsBatteryNotEngagedFault,
               extSpsInfoPtr->spsFaultInfo.spsCableConnectivityFault);

    trace_func(NULL, "    spsFfid : 0x%x, spsBatteryFfid : 0x%x\n",
               extSpsInfoPtr->spsModuleFfid,
               extSpsInfoPtr->spsBatteryFfid);
    trace_func(NULL, "    SPS SN : %s, Battery SN : %s\n",
               extSpsManufPtr->spsModuleManufInfo.spsSerialNumber,
               extSpsManufPtr->spsBatteryManufInfo.spsSerialNumber);
    trace_func(NULL, "    SPS Product Id : %s, Battery Product Id : %s\n",
               extSpsManufPtr->spsModuleManufInfo.spsModelString,
               extSpsManufPtr->spsBatteryManufInfo.spsModelString);

    trace_func(NULL, "    Primary Rev : %s\n    Secondary Rev : %s\n    Battery Rev : %s\n",
               extSpsInfoPtr->primaryFirmwareRev,
               extSpsInfoPtr->secondaryFirmwareRev,
               extSpsInfoPtr->batteryFirmwareRev);

    return;
}   // end of enclosure_access_printPeSpsInfo

/*!*************************************************************************
 *                   @fn enclosure_access_printBatteryInfo
 **************************************************************************
 *  @brief
 *      This function will trace Battery data.
 *
 *  @param  componentTypeDataPtr  - pointer to Component Data
 *  @param  componentIndex - index to a specific component
 *  @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                               updated component data
 *  @param  trace_func     - EDAL trace function
 *
 *  @return none.
 *
 *  @note
 *
 *  @version
 *    22-Mar-2012: Joe Perry - Created.
 *
 **************************************************************************/
void
enclosure_access_printBatteryInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                  fbe_enclosure_types_t enclosureType,
                                  fbe_u32_t componentIndex,
                                  fbe_bool_t printFullData, 
                                  fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t  generalDataPtr = NULL;
    edal_base_battery_info_t        *baseBatteryInfoPtr = NULL;
    edal_pe_battery_info_t          *peBatteryInfoPtr = NULL;
    fbe_u32_t                       index = 0;

    if (componentTypeDataPtr->componentType != FBE_ENCL_BATTERY)
    {
        return;
    }

    trace_func(NULL, "\n");

    // Loop through all Batteries
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // Check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, "  Battery %d\n", index);
            }
            else
            {
                trace_func(NULL, "State Change to Battery %d\n", index);
            }
            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);

            baseBatteryInfoPtr = (edal_base_battery_info_t *)generalDataPtr;
            enclosure_access_printPeGeneralComponentInfo(&(baseBatteryInfoPtr->generalComponentInfo),
                                                         printFullData, trace_func);
            peBatteryInfoPtr = (edal_pe_battery_info_t *)generalDataPtr;
            enclosure_access_printPeBatteryInfo(peBatteryInfoPtr, trace_func);         
        }        
    }

    return;

}   // end of enclosure_access_printBatteryInfo

/*!*************************************************************************
 *  @fn enclosure_access_printPeBatteryInfo(edal_pe_sps_info_t *peSpsPtr, 
 *                                          fbe_edal_trace_func_t trace_func)
 **************************************************************************
 *  @brief
 *      This function will trace some or all of the Processor Enclosure
 *       Battery data.
 *
 *  @param  peBatteryPtr
 *  @param  trace_func
 *
 *  @return none.
 *      
 *  @note
 *
 *  @version
 *    22-Mar-2012: Joe Perry - Created.
 *
 **************************************************************************/
void 
enclosure_access_printPeBatteryInfo(edal_pe_battery_info_t *peBatteryPtr, 
                                    fbe_edal_trace_func_t trace_func)
{
    edal_pe_battery_sub_info_t  * peBatterySubInfoPtr = NULL;
    fbe_base_battery_info_t     * extBatteryInfoPtr = NULL;

    peBatterySubInfoPtr = &peBatteryPtr->peBatterySubInfo;
    extBatteryInfoPtr = &peBatterySubInfoPtr->externalBatteryInfo;
    
    trace_func(NULL, "  Processor Encl Battery Info\n");

    trace_func(NULL, "    xactionStatus : 0x%x, peerEnvIntStatus : 0x%x\n",         
               extBatteryInfoPtr->transactionStatus,
               peBatterySubInfoPtr->peerEnvInterfaceStatus);

    trace_func(NULL, "    associatedSp : %d, slotNumOnSpBlade : %d, inserted : %d, enabled : %d\n",         
               extBatteryInfoPtr->associatedSp,
               extBatteryInfoPtr->slotNumOnSpBlade,
               extBatteryInfoPtr->batteryInserted,
               extBatteryInfoPtr->batteryEnabled);

    trace_func(NULL, "    state : %d, chargeState : %d, faults : %d\n",         
               extBatteryInfoPtr->batteryState,
               extBatteryInfoPtr->batteryChargeState,
               extBatteryInfoPtr->batteryFaults);

    trace_func(NULL, "    storageCapacity : %d, deliverableCapacity : %d, batteryTestStatus : %d\n",         
               extBatteryInfoPtr->batteryStorageCapacity,
               extBatteryInfoPtr->batteryDeliverableCapacity,
               extBatteryInfoPtr->batteryTestStatus);

    trace_func(NULL, "    firmwareRevMajor : %d, firmwareRevMinor : %02d\n",
               extBatteryInfoPtr->firmwareRevMajor,
               extBatteryInfoPtr->firmwareRevMinor);

    trace_func(NULL, "    energyRequirement : energy %d, maxLoad %d, power %d, time %d\n",
               extBatteryInfoPtr->energyRequirement.energy,
               extBatteryInfoPtr->energyRequirement.maxLoad,
               extBatteryInfoPtr->energyRequirement.power,
               extBatteryInfoPtr->energyRequirement.time);

    trace_func(NULL, "    isFaultRegFail : %s\n", 
        BOOLEAN_TO_TEXT(extBatteryInfoPtr->isFaultRegFail));   

/*
    trace_func(NULL, "    status : %d, faults : %d, present : %d, model : %d\n",         
               peBatterySubInfoPtr->externalBatteryInfo.,
               extSpsInfoPtr->spsFaults,
               extSpsInfoPtr->spsPresent,
               extSpsInfoPtr->spsModel);
*/
    return;
}   // end of enclosure_access_printPeBatteryInfo

/*!*************************************************************************
 *                         @fn enclosure_access_printComponentType
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will will convert an Enclosure Object Component Type
 *      to a string.
 *
 *  PARAMETERS:
 *      componentType - Enclosure Object Component Type
 *
 *  RETURN VALUES/ERRORS:
 *      pointer to the string
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created 
 *
 **************************************************************************/
char*
enclosure_access_printComponentType(fbe_enclosure_component_types_t componentType)
{
    char    *componentTypeString;

    switch (componentType)
    {
    case FBE_ENCL_POWER_SUPPLY:
        componentTypeString = "Power Supply";
        break;
    case FBE_ENCL_COOLING_COMPONENT:
        componentTypeString = "Cooling Component";
        break;
    case FBE_ENCL_MGMT_MODULE:
        componentTypeString = "Management Module";
        break;
    case FBE_ENCL_DRIVE_SLOT:
        componentTypeString = "Drive Slot";
        break;
    case FBE_ENCL_TEMP_SENSOR:
        componentTypeString = "Temp Sensor";
        break;
    case FBE_ENCL_CONNECTOR:
        componentTypeString = "Connector";
        break;
    case FBE_ENCL_EXPANDER:
        componentTypeString = "Expander";
        break;
    case FBE_ENCL_EXPANDER_PHY:
        componentTypeString = "Exp PHY";
        break;
    case FBE_ENCL_ENCLOSURE:
        componentTypeString = "Enclosure";
        break;
    case FBE_ENCL_LCC:
        componentTypeString = "LCC";
        break;
    case FBE_ENCL_DISPLAY:
        componentTypeString = "Display";
        break;
    case FBE_ENCL_IO_MODULE:
        componentTypeString = "IO Module";
        break;
    case FBE_ENCL_IO_PORT:
        componentTypeString = "IO Port";
        break;
    case FBE_ENCL_BEM:
        componentTypeString = "Base Module";
        break;
    case FBE_ENCL_MEZZANINE:
        componentTypeString = "Mezzanine";
        break;
    case FBE_ENCL_PLATFORM:
        componentTypeString = "Platform";
        break;
    case FBE_ENCL_MISC:
        componentTypeString = "Misc";
        break;
    case FBE_ENCL_FLT_REG:
        componentTypeString = "FltReg";
        break;
    case FBE_ENCL_SLAVE_PORT:
        componentTypeString = "SlavePort";
        break;
    case FBE_ENCL_SUITCASE:
        componentTypeString = "Suitcase";
        break;
    case FBE_ENCL_BMC:
        componentTypeString = "BMC";
        break;
    case FBE_ENCL_TEMPERATURE:
        componentTypeString = "Temperature";
        break;
    case FBE_ENCL_SPS:
        componentTypeString = "SPS";
        break;
    case FBE_ENCL_BATTERY:
        componentTypeString = "Battery";
        break;
    case FBE_ENCL_RESUME_PROM:
        componentTypeString = "ResumeProm";
        break;
    case FBE_ENCL_CACHE_CARD:
        componentTypeString = "Cache Card";
        break;
    case FBE_ENCL_DIMM:
        componentTypeString = "DIMM";
        break;
    case FBE_ENCL_SSC:
        componentTypeString = "SSC";
        break;
    case FBE_ENCL_SSD:
        componentTypeString = "SSD";
        break;

    case FBE_ENCL_INVALID_COMPONENT_TYPE:
        componentTypeString = "Invalid Component";
        break;
    default:
        componentTypeString = "Unknown Component";
        break;
    }
    
    return (componentTypeString);

}   // end of enclosure_access_printComponentType


/*!*************************************************************************
 *                         @fn enclosure_access_printAttribute
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will will convert an Enclosure Object Attribute
 *      to a string.
 *
 *  PARAMETERS:
 *      attribute - Enclosure Object attribute
 *
 *  RETURN VALUES/ERRORS:
 *      pointer to the string
 *
 *  NOTES:
 *
 *  HISTORY:
 *    10-Oct-2008: Joe Perry - Created 
 *
 **************************************************************************/
char*
enclosure_access_printAttribute(fbe_base_enclosure_attributes_t attribute)
{
    char    *attributeString;

    switch (attribute)
    {
    case FBE_ENCL_COMP_INSERTED:
        attributeString = "Inserted";
        break;
    case FBE_ENCL_COMP_INSERTED_PRIOR_CONFIG:
        attributeString = "InsertedPriorConfig";
        break;
    case FBE_ENCL_COMP_FAULTED:
        attributeString = "Faulted";
        break;
    case FBE_ENCL_COMP_POWERED_OFF:
        attributeString = "PoweredOff";
        break;
    case FBE_ENCL_COMP_TYPE:
        attributeString = "Type";
        break;
    case FBE_ENCL_COMP_ELEM_INDEX:
        attributeString = "ElementIndex";
        break;
    case FBE_ENCL_COMP_SUB_ENCL_ID:
        attributeString = "SubEnclId";
        break;   
    case FBE_ENCL_COMP_FAULT_LED_ON:
        attributeString = "FaultLedOn";
        break;
    case FBE_ENCL_COMP_MARKED:
        attributeString = "Marked";
        break;
    case FBE_ENCL_COMP_TURN_ON_FAULT_LED:
        attributeString = "TurnOnFltLed";
        break;
    case FBE_ENCL_COMP_MARK_COMPONENT:
        attributeString = "MarkComponent";
        break;
    case FBE_ENCL_COMP_FAULT_LED_REASON:
        attributeString = "FaultLedReason";
        break;
    case FBE_ENCL_COMP_STATE_CHANGE:
        attributeString = "StateChange";
        break;
    case FBE_ENCL_COMP_WRITE_DATA:
        attributeString = "WriteData";
        break;
    case FBE_ENCL_COMP_IS_LOCAL:
        attributeString = "IsLocalComponent";
        break;
    case FBE_ENCL_COMP_SUBTYPE:
        attributeString = "SubType";
        break;   
    case FBE_ENCL_COMP_STATUS_VALID:
        attributeString = "ComponentStatusValid";
        break;
    case FBE_ENCL_COMP_ADDL_STATUS:
        attributeString = "ComponentAddlStatus";
        break; 
    case FBE_ENCL_COMP_CONTAINER_INDEX:
        attributeString = "CompContainerIndex";
        break;
    case FBE_ENCL_COMP_SIDE_ID:
        attributeString = "CompSideId";
        break;
    case FBE_ENCL_COMP_POWER_CYCLE_PENDING:
        attributeString = "CompPowerCyclePending";
        break;
    case FBE_ENCL_COMP_POWER_CYCLE_COMPLETED:
        attributeString = "CompPowerCycleCompleted";
        break;
    case FBE_ENCL_COMP_POWER_DOWN_COUNT:
        attributeString = "CompPowerDownCount";
        break;
    case FBE_ENCL_PS_AC_FAIL:
        attributeString = "AcFail";
        break;
    case FBE_ENCL_PS_DC_DETECTED:
        attributeString = "PsDcDetected";
        break;
    case FBE_ENCL_PS_SUPPORTED:
        attributeString = "PsSupported";
        break;
    case FBE_ENCL_PS_INPUT_POWER:
        attributeString = "InputPower";
        break;
    case FBE_ENCL_PS_INPUT_POWER_STATUS:
        attributeString = "InputPowerStatus";
        break;
    case FBE_ENCL_PS_PEER_POWER_SUPPLY_ID:
        attributeString = "Peer Power Supply ID";
        break;
    case FBE_ENCL_PS_AC_DC_INPUT:
        attributeString = "AC DC Input";
        break;
    case FBE_ENCL_PS_ASSOCIATED_SPS:
        attributeString = "Associated SPS";
        break;
    case FBE_ENCL_PS_FIRMWARE_REV:
        attributeString = "Firmware Rev";
        break;
    case FBE_ENCL_PS_INTERNAL_FAN1_FAULT:
        attributeString = "Internal Fan 1 Fault";
        break;
    case FBE_ENCL_PS_INTERNAL_FAN2_FAULT:
        attributeString = "Internal Fan 2 Fault";
        break;
    case FBE_ENCL_PS_AMBIENT_OVERTEMP_FAULT:
        attributeString = "Ambient Overtemp Fault";
        break;
    case FBE_ENCL_PS_PEER_AVAILABLE:
        attributeString = "Peer Available";
        break;
    case FBE_ENCL_DRIVE_BYPASSED:
        attributeString = "DriveBypassed";
        break;
    case FBE_ENCL_BYPASS_DRIVE:
        attributeString = "BypassDrive";
        break;
    case FBE_ENCL_DRIVE_DEVICE_OFF:
        attributeString = "DriveDeviceOff";
        break;
    case FBE_ENCL_DRIVE_DEVICE_OFF_REASON:
        attributeString = "DriveDeviceOffReason";
        break;
    case FBE_ENCL_DRIVE_LOGGED_IN:
        attributeString = "DriveLoggedIn";
        break;
    case FBE_ENCL_DRIVE_INSERT_MASKED:
        attributeString = "DriveInsertMasked";
        break;
    case FBE_ENCL_DRIVE_SAS_ADDRESS:
        attributeString = "DriveSasAddress";
        break;
    case FBE_ENCL_DRIVE_ESES_SAS_ADDRESS:
        attributeString = "DriveEsesSasAddress";
        break;
    case FBE_ENCL_DRIVE_PHY_INDEX:
        attributeString = "DrivePhyIndex";
        break;
    case FBE_ENCL_DRIVE_BATTERY_BACKED:
        attributeString = "DriveBatteryBacked";
        break;
    case FBE_ENCL_EXP_PHY_DISABLED:
        attributeString = "ExpPhyDisabled";
        break;
    case FBE_ENCL_EXP_PHY_READY:
        attributeString = "ExpPhyReady";
        break;
    case FBE_ENCL_EXP_PHY_LINK_READY:
        attributeString = "ExpPhyLinkReady";
        break;
    case FBE_ENCL_EXP_PHY_FORCE_DISABLED:
        attributeString = "ExpPhyForceDisabled";
        break;
    case FBE_ENCL_EXP_PHY_CARRIER_DETECTED:
        attributeString = "ExpPhyCarrierDetected";
        break;
    case FBE_ENCL_EXP_PHY_SPINUP_ENABLED:
        attributeString = "ExpPhySpinupEnabled";
        break;
    case FBE_ENCL_EXP_PHY_SATA_SPINUP_HOLD:
        attributeString = "ExpPhySataSpinupHold";
        break;
    case FBE_ENCL_EXP_PHY_EXPANDER_ELEM_INDEX:
        attributeString = "ExpPhyElemIndex";
        break;
    case FBE_ENCL_EXP_PHY_ID:
        attributeString = "ExpPhyId";
        break;
    case FBE_ENCL_EXP_PHY_DISABLE:
        attributeString = "ExpPhyDisable";
        break;
    case FBE_ENCL_EXP_PHY_DISABLE_REASON:
        attributeString = "ExpPhyDisableReason";
        break;
    case FBE_ENCL_CONNECTOR_INSERT_MASKED:
        attributeString = "ConnectorInsertMasked";
        break;
    case FBE_ENCL_CONNECTOR_DISABLED:
        attributeString = "ConnectorDisabled";
        break;
    case FBE_ENCL_CONNECTOR_DEGRADED:
        attributeString = "ConnectorDegraded";
        break;
    case FBE_ENCL_CONNECTOR_PRIMARY_PORT:
        attributeString = "ConnectorPrimaryPort";
        break;
    case FBE_ENCL_CONNECTOR_IS_ENTIRE_CONNECTOR:
        attributeString = "ConnectorIsEntireConnector";
        break;    
    case FBE_ENCL_CONNECTOR_ILLEGAL_CABLE:
        attributeString = "IllegalCableInerted";
        break;    
    case FBE_ENCL_CONNECTOR_PHY_INDEX:
        attributeString = "ConnectorPhyIndex";
        break;
    case FBE_ENCL_CONNECTOR_EXP_SAS_ADDRESS:
        attributeString = "ConnectorExpanderSasAddress";
        break;
    case FBE_ENCL_CONNECTOR_ATTACHED_SAS_ADDRESS:
        attributeString = "ConnectorAttachedSasAddress";
        break;
    case FBE_ENCL_CONNECTOR_ATTACHED_SUB_ENCL_ID:
        attributeString = "ConnectorAttachedSubEnclID";
        break;
    case FBE_ENCL_CONNECTOR_DEV_LOGGED_IN:
        attributeString = "ConnectorLoggedIn";
        break;
    case FBE_ENCL_EXP_SAS_ADDRESS:
        attributeString = "ExpanderSasAddress";
        break;
    case FBE_ENCL_SES_SAS_ADDRESS:
        attributeString = "SesSasAddress";
        break;
    case FBE_ENCL_SMP_SAS_ADDRESS:
        attributeString = "SmpSasAddress";
        break;
    case FBE_ENCL_SERVER_SAS_ADDRESS:
        attributeString = "ServerSasAddress";
        break;
    case FBE_ENCL_UNIQUE_ID:
        attributeString = "UniqueId";
        break;
    case FBE_ENCL_POSITION:
        attributeString = "EnclPosition";
        break;
    case FBE_ENCL_ADDRESS:
        attributeString = "EnclAddress";
        break;
    case FBE_ENCL_PORT_NUMBER:
        attributeString = "EnclPortNumber";
        break;    
    case FBE_ENCL_COOLING_MULTI_FAN_FAULT:
        attributeString = "MultiFanFault";
        break;
    case FBE_ENCL_TEMP_SENSOR_OT_WARNING:
        attributeString = "OverTempWaringing";
        break;
    case FBE_ENCL_TEMP_SENSOR_OT_FAILURE:
        attributeString = "OverTempFailure";
        break;
    case FBE_ENCL_TEMP_SENSOR_TEMPERATURE:
        attributeString = "Temperature";
        break;
    case FBE_ENCL_TRACE_PRESENT:
        attributeString = "TracePresent";
        break;   
    case FBE_ENCL_MODE_SENSE_UNSUPPORTED:
        attributeString = "ModeSenseUnsupported";
        break; 
    case FBE_ENCL_MODE_SELECT_UNSUPPORTED:
        attributeString = "ModeSelectUnsupported";
        break;
    case FBE_ENCL_ADDITIONAL_STATUS_PAGE_UNSUPPORTED:
        attributeString = "AddlStatusPageUnsupported";
        break;
    case FBE_ENCL_EMC_SPECIFIC_STATUS_PAGE_UNSUPPORTED:
        attributeString = "EmcSpecificStatusPageUnsupported";
        break;
    case FBE_ENCL_SERIAL_NUMBER:
        attributeString = "EmcSerialNumber";
        break;
    case FBE_ENCL_BD_BUFFER_ID:
        attributeString = "BufferDescriptorId";
        break;
    case FBE_ENCL_BD_BUFFER_ID_WRITEABLE:
        attributeString = "BufferDescriptorIdWriteable";
        break;      
    case FBE_ENCL_BD_BUFFER_TYPE:
        attributeString = "BufferDescriptorType";
        break;
    case FBE_ENCL_BD_BUFFER_INDEX:
        attributeString = "BufferDescriptorIndex";
        break;
    case FBE_ENCL_BD_ELOG_BUFFER_ID:
        attributeString = "BufferDescriptorELogIndex";
        break;      
    case FBE_ENCL_BD_ACT_TRC_BUFFER_ID:
        attributeString = "BufferDescriptorActiveTraceBuffer";
        break;      
    case FBE_ENCL_SHUTDOWN_REASON:
        attributeString = "ShutdownReason";
        break;
    case FBE_ENCL_PARTIAL_SHUTDOWN:
        attributeString = "PartialShutdown";
        break;
    case FBE_DISPLAY_MODE_STATUS:
        attributeString = "DispalyModeStatus";
        break;
    case FBE_DISPLAY_CHARACTER_STATUS:
        attributeString = "DispalyCharacterStatus";
        break;
    case FBE_DISPLAY_MODE:
        attributeString = "DisplayMode";
        break;
    case FBE_DISPLAY_CHARACTER:
        attributeString = "DisplayCharacter";
        break;
    case FBE_ENCL_LCC_POWER_CYCLE_REQUEST:
        attributeString = "PowerCycleRequest";
        break;
    case FBE_ENCL_LCC_POWER_CYCLE_DURATION:
        attributeString = "PowerCycleDuration";
        break;
    case FBE_ENCL_LCC_POWER_CYCLE_DELAY:
        attributeString = "PowerCycleDelay";
        break;
    case FBE_ENCL_RESET_RIDE_THRU_COUNT:
        attributeString = "EnclResetRideThruCount";
        break;
    case FBE_ENCL_TIME_OF_LAST_GOOD_STATUS_PAGE:
        attributeString = "EnclTimeOfLastGoodStatusPage";
        break;
    case FBE_ENCL_TEMP_SENSOR_MAX_TEMPERATURE:
        attributeString = "TempSensorMaxTemperature";
        break;
    case FBE_ENCL_SUBENCL_SERIAL_NUMBER:
        attributeString = "Sub Encl Serial number";
        break; 
    case FBE_ENCL_SUBENCL_PRODUCT_ID:
        attributeString = "Sub Encl Product id";
        break;  
    case FBE_ENCL_PE_IO_COMP_INFO:
        attributeString = "PeIoCompInfo";
        break; 
    case FBE_ENCL_PE_IO_PORT_INFO:
        attributeString = "PeIoCompInfo";
        break; 
    case FBE_ENCL_PE_POWER_SUPPLY_INFO:
        attributeString = "PePowerSupplyInfo";
        break;  
    case FBE_ENCL_PE_COOLING_INFO:
        attributeString = "PeCoolingInfo";
        break;
    case FBE_ENCL_PE_MGMT_MODULE_INFO:
        attributeString = "PeMgmtModuleInfo";
        break; 
    case FBE_ENCL_PE_PLATFORM_INFO:
        attributeString = "PePlatformInfoInfo";
        break;
    case FBE_ENCL_PE_MISC_INFO:
        attributeString = "PeMiscInfo";
        break;    
    case FBE_ENCL_PE_FLT_REG_INFO:
        attributeString = "PeFltRegInfo";
        break; 
    case FBE_ENCL_PE_SLAVE_PORT_INFO:
        attributeString = "PeSlavePortInfo";
        break; 
    case FBE_ENCL_PE_SUITCASE_INFO:
        attributeString = "PeSuitcaseInfo";
        break; 
    case FBE_ENCL_PE_BMC_INFO:
        attributeString = "PeBMCInfo";
        break; 
    case FBE_ENCL_PE_TEMPERATURE_INFO:
        attributeString = "PeTemperatureInfo";
        break; 
    case FBE_LCC_EXP_FW_INFO:
        attributeString = "Exp firmware rev";
        break;	
    case FBE_LCC_BOOT_FW_INFO:
        attributeString = "Boot loader rev";
        break;	
    case FBE_LCC_INIT_FW_INFO:
        attributeString = "Init string rev";
        break;	
    case FBE_LCC_FPGA_FW_INFO:
        attributeString = "FPGA rev";
        break;	
    case FBE_ENCL_PE_SPS_INFO:
        attributeString = "PeSpsInfo";
        break; 
    case FBE_ENCL_BD_BUFFER_SIZE:
        attributeString = "BufferSize";
        break;
    default:
        attributeString = "Unknown Attribute";
        break;
    }
    
    return (attributeString);

}   // end of enclosure_access_printAttribute


/*!*************************************************************************
 *                         @fn enclosure_access_log_info
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function trace at the INFO level for EDAL.
 *
 *  PARAMETERS:
 *      @param fmt - formatted string
 *
 *  RETURN VALUES/ERRORS:
 *      @return None
 *
 *  NOTES:
 *      None
 *
 *  HISTORY:
 *    09-Oct-2008: Joe Perry - Created
 *
 **************************************************************************/
void enclosure_access_log_info(const char * fmt, ...)
{
/* Disabling trace function within edal. Enable it for Debugging purpose. 
 * We should NOT be including non-debug libraries to build ppdbg macros.
 */
#if EDAL_DEBUG
    va_list args;

    va_start(args, fmt);
    if (edal_traceLevel == FBE_TRACE_LEVEL_INVALID)
    {
        edal_traceLevel = fbe_trace_get_default_trace_level();
    }
    if (FBE_TRACE_LEVEL_INFO <= edal_traceLevel)
    {
        fbe_trace_report(FBE_COMPONENT_TYPE_LIBRARY,
                         FBE_LIBRARY_ID_ENCL_DATA_ACCESS,
                         FBE_TRACE_LEVEL_INFO,
                         0, /* message_id */
                         fmt, 
                         args);
    }
    va_end(args);
#endif
}   // end of enclosure_access_log_info


/*!*************************************************************************
 *                         @fn enclosure_access_log_debug
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function trace at the DEBUG LOW level for EDAL.
 *
 *  PARAMETERS:
 *      @param fmt - formatted string
 *
 *  RETURN VALUES/ERRORS:
 *      @return None
 *
 *  NOTES:
 *      None
 *
 *  HISTORY:
 *    09-Oct-2008: Joe Perry - Created
 *
 **************************************************************************/
void enclosure_access_log_debug(const char * fmt, ...)
{
/* Disabling trace function within edal. Enable it for Debugging purpose. 
 * We should NOT be including non-debug libraries to build ppdbg macros.
 */
#if EDAL_DEBUG
    va_list args;

    va_start(args, fmt);
    if (edal_traceLevel == FBE_TRACE_LEVEL_INVALID)
    {
        edal_traceLevel = fbe_trace_get_default_trace_level();
    }
    if (FBE_TRACE_LEVEL_DEBUG_LOW <= edal_traceLevel)
    {
        fbe_trace_report(FBE_COMPONENT_TYPE_LIBRARY,
                         FBE_LIBRARY_ID_ENCL_DATA_ACCESS,
                         FBE_TRACE_LEVEL_DEBUG_LOW,
                         0, /* message_id */
                         fmt, 
                        args);
    }
    va_end(args);
#endif
}   // end of enclosure_access_log_debug


/*!*************************************************************************
 *                         @fn enclosure_access_log_error
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function trace at the ERROR level for EDAL.
 *
 *  PARAMETERS:
 *      @param fmt - formatted string
 *
 *  RETURN VALUES/ERRORS:
 *      @return None
 *
 *  NOTES:
 *      None
 *
 *  HISTORY:
 *    09-Oct-2008: Joe Perry - Created
 *
 **************************************************************************/
void enclosure_access_log_error(const char * fmt, ...)
{
/* Disabling trace function within edal. Enable it for Debugging purpose. 
 * We should NOT be including non-debug libraries to build ppdbg macros.
 */
#if EDAL_DEBUG
    va_list args;

    va_start(args, fmt);
    if (edal_traceLevel == FBE_TRACE_LEVEL_INVALID)
    {
        edal_traceLevel = fbe_trace_get_default_trace_level();
    }
    if (FBE_TRACE_LEVEL_ERROR <= edal_traceLevel)
    {
        fbe_trace_report(FBE_COMPONENT_TYPE_LIBRARY,
                         FBE_LIBRARY_ID_ENCL_DATA_ACCESS,
                         FBE_TRACE_LEVEL_ERROR,
                         0, /* message_id */
                         fmt, 
                         args);
    }
    va_end(args);
#endif
}   // end of enclosure_access_log_error


/*!*************************************************************************
 *                         @fn enclosure_access_log_warning
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function trace at the WARNING level for EDAL.
 *
 *  PARAMETERS:
 *      @param fmt - formatted string
 *
 *  RETURN VALUES/ERRORS:
 *      @return None
 *
 *  NOTES:
 *      None
 *
 *  HISTORY:
 *    09-Oct-2008: Joe Perry - Created
 *
 **************************************************************************/
void enclosure_access_log_warning(const char * fmt, ...)
{
/* Disabling trace function within edal. Enable it for Debugging purpose. 
 * We should NOT be including non-debug libraries to build ppdbg macros.
 */
#if EDAL_DEBUG
    va_list args;

    va_start(args, fmt);
    if (edal_traceLevel == FBE_TRACE_LEVEL_INVALID)
    {
        edal_traceLevel = fbe_trace_get_default_trace_level();
    }
    if (FBE_TRACE_LEVEL_WARNING <= edal_traceLevel)
    {
        fbe_trace_report(FBE_COMPONENT_TYPE_LIBRARY,
                         FBE_LIBRARY_ID_ENCL_DATA_ACCESS,
                         FBE_TRACE_LEVEL_WARNING,
                         0, /* message_id */
                         fmt, 
                         args);
    }
    va_end(args);
#endif
}   // end of enclosure_access_log_warning


/*!*************************************************************************
 *                         @fn enclosure_access_log_critical_error
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function trace at the CRITICAL ERROR level for EDAL.
 *
 *  PARAMETERS:
 *      @param fmt - formatted string
 *
 *  RETURN VALUES/ERRORS:
 *      @return None
 *
 *  NOTES:
 *      None
 *
 *  HISTORY:
 *    09-Oct-2008: Joe Perry - Created
 *
 **************************************************************************/
void enclosure_access_log_critical_error(const char * fmt, ...)
{
/* Disabling trace function within edal. Enable it for Debugging purpose. 
 * We should NOT be including non-debug libraries to build ppdbg macros.
 */
#if EDAL_DEBUG
    va_list args;

    va_start(args, fmt);
    if (edal_traceLevel == FBE_TRACE_LEVEL_INVALID)
    {
        edal_traceLevel = fbe_trace_get_default_trace_level();
    }
    if (FBE_TRACE_LEVEL_CRITICAL_ERROR <= edal_traceLevel)
    {
        fbe_trace_report(FBE_COMPONENT_TYPE_LIBRARY,
                         FBE_LIBRARY_ID_ENCL_DATA_ACCESS,
                         FBE_TRACE_LEVEL_CRITICAL_ERROR,
                         0, /* message_id */
                         fmt, 
                         args);
    }
    va_end(args);
#endif
}   // end of enclosure_access_log_critical_error
/*!*************************************************************************
 *  @fn enclosure_access_printBaseLccData(fbe_base_lcc_info_t *baseLccDataPtr)
 **************************************************************************
 *  @brief
 *      This function will trace some or all of the LCC component data.
 *
 *  @param baseLccDataPtr - The pointer to the base lcc data.
 *
 *  @return 
 *     None.
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Oct-2008: PHE - Created
 *
 **************************************************************************/
void enclosure_access_printBaseLccData(fbe_base_lcc_info_t *baseLccDataPtr, 
                                       fbe_edal_trace_func_t trace_func)
{
    trace_func(NULL, "  Base LCC Info\n");
    trace_func(NULL, "    IsLocal : %d, SideId : %d, Slot : %d\n", 
        baseLccDataPtr->lccFlags.isLocalLcc,
        baseLccDataPtr->lccData.lccSideId,
        baseLccDataPtr->lccData.lccLocation);
    return;

}// End of function enclosure_access_printBaseLccData


/*!*************************************************************************
 *  @fn enclosure_access_printEsesLccData(fbe_eses_lcc_info_t *lccDataPtr)
 **************************************************************************
 *  @brief
 *      This function will trace some or all of the Enclosure Object's
 *      Component data.
 *
 *      @param  lccDataPtr  - pointer to LCC Component Data
 *      @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                               updated component data
 *      @param  trace_func     - EDAL trace function
 *      
 *  @return 
 *      None.
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Oct-2008: PHE - Created.
 *
 **************************************************************************/
void enclosure_access_printEsesLccData (fbe_eses_lcc_info_t *lccDataPtr,
                                        fbe_bool_t printFullData, 
                                        fbe_edal_trace_func_t trace_func)
{
    trace_func(NULL, "  ESES LCC Info\n");
    trace_func(NULL, "    FailInd : %d, FailRqstd : %d, RqstFail : %d\n", 
        lccDataPtr->esesLccFlags.enclFailureIndication,
        lccDataPtr->esesLccFlags.enclFailureRequested,
        lccDataPtr->esesLccFlags.enclRequestFailure);
    trace_func(NULL, "    RqstWarn : %d, WarnInd : %d, WarnRqstd  : %d, ecbFault : %d\n", 
        lccDataPtr->esesLccFlags.enclRequestWarning,
        lccDataPtr->esesLccFlags.enclWarningIndication,
        lccDataPtr->esesLccFlags.enclWarningRequested,
        lccDataPtr->ecbFault);
    trace_func(NULL, "    PwrCycle Delay : %d, Rqst : %d, Duration : %d\n", 
        lccDataPtr->esesLccData.enclPowerCycleDelay,
        lccDataPtr->esesLccData.enclPowerCycleRequest,
        lccDataPtr->esesLccData.enclPowerOffDuration);

    trace_func(NULL, "    Lcc Product Id : %.16s\n", lccDataPtr->lccProductId);
    trace_func(NULL, "    LccSN : %.16s\n", lccDataPtr->lccSerialNumber);
    
    trace_func(NULL, "    FW");
    enclosure_access_printFwRevInfo(&lccDataPtr->fwInfo, trace_func);
    trace_func(NULL, "    (Exp");
    enclosure_access_printFwRevInfo(&lccDataPtr->fwInfoExp, trace_func);
    trace_func(NULL, "     Boot");
    enclosure_access_printFwRevInfo(&lccDataPtr->fwInfoBoot, trace_func);
    trace_func(NULL, "     InitStr");
    enclosure_access_printFwRevInfo(&lccDataPtr->fwInfoInit, trace_func);
    trace_func(NULL, "     FPGA");
    enclosure_access_printFwRevInfo(&lccDataPtr->fwInfoFPGA, trace_func);
    trace_func(NULL, "    )\n");

    if (printFullData)
    {
        trace_func(NULL, "    ElemIndx : %d, SubEnclId : %d\n", 
            lccDataPtr->lccElementIndex,
            lccDataPtr->lccSubEnclId);
        trace_func(NULL, "    BufferDesc Id : %d, Type : %d, Indx : %d, Writable : %d\n", 
            lccDataPtr->esesLccData.bufferDescriptorInfo.bufferDescBufferId,
            lccDataPtr->esesLccData.bufferDescriptorInfo.bufferDescBufferType,
            lccDataPtr->esesLccData.bufferDescriptorInfo.bufferDescBufferIndex,
            lccDataPtr->esesLccData.bufferDescriptorInfo.bufferDescBufferIdWriteable);
        trace_func(NULL, "    ELog BufferDesc Id : %d, Active Trace Buffer Id : %d\n", 
            lccDataPtr->esesLccData.bufferDescriptorInfo.bufferDescElogBufferID,
            lccDataPtr->esesLccData.bufferDescriptorInfo.bufferDescActBufferID);
		
    }
    return;
}// End of function enclosure_access_printEsesLccData


/*!*************************************************************************
 *  @fn enclosure_access_printEsesDisplayData(fbe_eses_lcc_info_t *lccDataPtr)
 **************************************************************************
 *  @brief
 *      This function will trace some or all of the Enclosure Object's
 *      Component data.
 *
 *  @param displayDataPtr
 *  @param  trace_func - EDAL trace function
 *      
 *  @return 
 *      None.
 *
 *  NOTES:
 *
 *  HISTORY:
 *    03-Nov-2008: Joe Perry - Created.
 *
 **************************************************************************/
void enclosure_access_printEsesDisplayData (fbe_eses_display_info_t *displayDataPtr, 
                                            fbe_edal_trace_func_t trace_func)
{
    trace_func(NULL, "  ESES Display Info\n");
    trace_func(NULL, "    ElemIndx : %d, Subtype : %d\n", 
        displayDataPtr->displayElementIndex,
        displayDataPtr->displaySubtype);
    trace_func(NULL, "    MdStat : %d, CharStat : %d (%c)\n", 
        displayDataPtr->displayModeStatus,
        displayDataPtr->displayCharacterStatus,
        displayDataPtr->displayCharacterStatus);
    trace_func(NULL, "    Md : %d, Char : %d (%c)\n", 
        displayDataPtr->displayMode,
        displayDataPtr->displayCharacter,
        displayDataPtr->displayCharacter);

    return;

}// End of function enclosure_access_printEsesDisplayData


/*!*************************************************************************
 *  @fn enclosure_access_printLccData(fbe_enclosure_component_t   *componentTypeDataPtr,
 *                                    fbe_enclosure_types_t enclosureType)
 **************************************************************************
 *  @brief
 *      This function will trace Enclosure component data.
 *
 *      @param  componentTypeDataPtr  - pointer to Component Data
 *      @param  enclosureType  - type of enclosure
 *      @param  componentIndex - index to a specific component
 *      @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                               updated component data
 *      @param  trace_func     - EDAL trace function
 *
 *  @return 
 *    None.      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Oct-2008: PHE - Created
 *
 **************************************************************************/
void enclosure_access_printLccData(fbe_enclosure_component_t   *componentTypeDataPtr,
                              fbe_enclosure_types_t enclosureType,
                              fbe_u32_t componentIndex,
                              fbe_bool_t printFullData, 
                              fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t generalDataPtr;
    fbe_base_lcc_info_t       *lccDataPtr;
    fbe_u32_t                  index = 0;

    if (componentTypeDataPtr->componentType != FBE_ENCL_LCC)
    {
        return;
    }

    // loop through all LCCs.
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, "  LCC %d\n", index);
            }
            else
            {
                trace_func(NULL, "State Change to LCC %d\n", index);
            }
            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);
            lccDataPtr = (fbe_base_lcc_info_t *)generalDataPtr;
        
            enclosure_access_printGeneralComponentInfo(&(lccDataPtr->generalComponentInfo),
                                                       printFullData, trace_func);        

            enclosure_access_printBaseLccData(lccDataPtr, trace_func);
      
            if(IS_VALID_ENCL(enclosureType)) 
            {
                enclosure_access_printEsesLccData((fbe_eses_lcc_info_t *)lccDataPtr, 
                                                    printFullData, trace_func);
            }
        }
    }

    return;

}// End of function enclosure_access_printLccData


/*!*************************************************************************
 *  @fn enclosure_access_printDisplayData(fbe_enclosure_component_t   *componentTypeDataPtr,
 *                                        fbe_enclosure_types_t enclosureType)
 **************************************************************************
 *  @brief
 *      This function will trace Enclosure component data.
 *
 *      @param  componentTypeDataPtr  - pointer to Component Data
 *      @param  enclosureType  - type of enclosure
 *      @param  componentIndex - index to a specific component
 *      @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                               updated component data
 *      @param  trace_func     - EDAL trace function
 *
 *  @return 
 *    None.      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Oct-2008: PHE - Created
 *
 **************************************************************************/
void enclosure_access_printDisplayData(fbe_enclosure_component_t   *componentTypeDataPtr,
                                       fbe_enclosure_types_t enclosureType,
                                       fbe_u32_t componentIndex,
                                       fbe_bool_t printFullData, 
                                       fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t generalDataPtr;
    fbe_eses_display_info_t        *displayDataPtr;
    fbe_u32_t                      index = 0;

    if (componentTypeDataPtr->componentType != FBE_ENCL_DISPLAY)
    {
        return;
    }

    // loop through all Displays.
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, "  Display %d\n", index);
            }
            else
            {
                trace_func(NULL, "State Change to Display %d\n", index);
            }
            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);
            displayDataPtr = (fbe_eses_display_info_t *)generalDataPtr;
        
            if (printFullData)
            {
                enclosure_access_printGeneralComponentInfo(&(displayDataPtr->generalDisplayInfo),
                                                                printFullData, trace_func);        
            }

            if(IS_VALID_ENCL(enclosureType)) 
            {
                enclosure_access_printEsesDisplayData(displayDataPtr, trace_func);
            }
        }
    }

    return;

}// End of function enclosure_access_printDisplayData

/*
 * Get/Set functions for EDAL globals
 */
void fbe_edal_setTraceLevel(fbe_trace_level_t traceLevel)
{
    edal_traceLevel = traceLevel;
}

fbe_trace_level_t fbe_edal_getTraceLevel(void)
{
    return edal_traceLevel;
}

void fbe_edal_setCliContext(fbe_bool_t cliContext)
{
    edal_cliContext = cliContext;
}

fbe_bool_t fbe_edal_getCliContext(void)
{
    return edal_cliContext;
}

/*!*************************************************************************
 * @fn fbe_edal_print_Enclosuretype
 **************************************************************************
 *
 *  @brief
 *      This function return enclosure type into text.
 *
 *  @param   fbe_edal_block_handle_t  *type
 *
 *  @return    char * - string
 *
 *  NOTES:
 *
 *  HISTORY:
 *    26-Feb-2009: Dipak Patel- created.
 *
 **************************************************************************/
 char * fbe_edal_print_Enclosuretype(fbe_u32_t enclosureType)
{
    char * enclTypeStr;
        
    switch(enclosureType)
    {
        case FBE_ENCL_BASE:
            enclTypeStr = (char *)("BASE_ENCLOSURE");
            break;
        case FBE_ENCL_FC:
            enclTypeStr = (char *)("FC_ENCLOSURE");
            break;
        case FBE_ENCL_SAS:
            enclTypeStr = (char *)("SAS_ENCLOSURE");
            break;
        case FBE_ENCL_ESES:
            enclTypeStr = (char *)("ESES_ENCLOSURE");
            break;
        case FBE_ENCL_SAS_VIPER:
            enclTypeStr = (char *)("SAS_VIPER_ENCLOSURE");
            break;
        case FBE_ENCL_SAS_MAGNUM:
            enclTypeStr = (char *)("SAS_MAGNUM_ENCLOSURE");
            break;
        case FBE_ENCL_SAS_DERRINGER:
            enclTypeStr = (char *)("SAS_DERRINGER_ENCLOSURE");
            break;
        case FBE_ENCL_SAS_CITADEL:
            enclTypeStr = (char *)("SAS_CITADEL_ENCLOSURE");
            break;
        case FBE_ENCL_SAS_BUNKER:
            enclTypeStr = (char *)("SAS_BUNKER_ENCLOSURE");
            break;
        case FBE_ENCL_SAS_VOYAGER_ICM:
            enclTypeStr = (char *)("SAS_VOYAGER_ICM_ENCLOSURE");
            break; 	
        case FBE_ENCL_SAS_VOYAGER_EE:
            enclTypeStr = (char *)("SAS_VOYAGER_EE_ENCLOSURE");
            break; 
        case FBE_ENCL_SAS_VIKING_IOSXP:
            enclTypeStr = (char *)("SAS_VIKING_IOSXP_ENCLOSURE");
            break;      
        case FBE_ENCL_SAS_VIKING_DRVSXP:
            enclTypeStr = (char *)("SAS_VIKING_DRVSXP_ENCLOSURE");
            break;  
        case FBE_ENCL_SAS_NAGA_IOSXP:
            enclTypeStr = (char *)("SAS_NAGA_IOSXP_ENCLOSURE");
            break;      
        case FBE_ENCL_SAS_CAYENNE_IOSXP:
            enclTypeStr = (char *)("SAS_CAYENNE_IOSXP_ENCLOSURE");
            break;      
        case FBE_ENCL_SAS_CAYENNE_DRVSXP:
            enclTypeStr = (char *)("SAS_CAYENNE_DRVSXP_ENCLOSURE");
            break;      
        case FBE_ENCL_SAS_NAGA_DRVSXP:
            enclTypeStr = (char *)("SAS_NAGA_DRVSXP_ENCLOSURE");
            break;    
        case FBE_ENCL_SAS_FALLBACK:
            enclTypeStr = (char *)("SAS_FALLBACK_ENCLOSURE");
            break;
        case FBE_ENCL_SAS_BOXWOOD:
            enclTypeStr = (char *)("SAS_BOXWOOD_ENCLOSURE");
            break;
        case FBE_ENCL_SAS_KNOT:
            enclTypeStr = (char *)("SAS_KNOT_ENCLOSURE");
            break;
        case FBE_ENCL_SAS_ANCHO:
            enclTypeStr = (char *)("SAS_ANCHO_ENCLOSURE");
            break;
        case FBE_ENCL_SAS_PINECONE:
            enclTypeStr = (char *)("SAS_PINECONE_ENCLOSURE");
            break;
        case FBE_ENCL_SAS_STEELJAW:
            enclTypeStr = (char *)("SAS_STEELJAW_ENCLOSURE");
            break;
        case FBE_ENCL_SAS_RAMHORN:
            enclTypeStr = (char *)("SAS_RAMHORN_ENCLOSURE");
            break;
        case FBE_ENCL_SAS_TABASCO:
            enclTypeStr = (char *)("SAS_TABASCO_ENCLOSURE");
            break;
        case FBE_ENCL_SAS_CALYPSO:
            enclTypeStr = (char *)("SAS_CALYPSO_ENCLOSURE");
            break;
        case FBE_ENCL_SAS_MIRANDA:
            enclTypeStr = (char *)("SAS_MIRANDA_ENCLOSURE");
            break;
        case FBE_ENCL_SAS_RHEA:
            enclTypeStr = (char *)("SAS_RHEA_ENCLOSURE");
            break;
        default:
            enclTypeStr = (char *)( "UNKNOWN_ENCLOSURE ");
        break;
        }
    return(enclTypeStr);
}


/*!*************************************************************************
 * @fn fbe_edal_print_AdditionalStatus
 **************************************************************************
 *
 *  @brief
 *      This function return enclosure type into text.
 *
 *  @param   fbe_edal_block_handle_t  *type
 *
 *  @return    char * - string
 *
 *  NOTES:
 *
 *  HISTORY:
 *    26-Feb-2009: Dipak Patel- created.
 *
 **************************************************************************/
 char * fbe_edal_print_AdditionalStatus(fbe_u8_t additionalStatus)
{
    char * additionalStatusStr;
        
    switch(additionalStatus)
    {
    case SES_STAT_CODE_UNSUPP:
        additionalStatusStr = (char *)("Unsupported");
        break;
    case SES_STAT_CODE_OK:
        additionalStatusStr = (char *)("OK");
        break;
    case SES_STAT_CODE_CRITICAL:
        additionalStatusStr = (char *)("Critical");
        break;
    case SES_STAT_CODE_NONCRITICAL:
        additionalStatusStr = (char *)("NonCritical");
        break;
    case SES_STAT_CODE_UNRECOV:
        additionalStatusStr = (char *)("Unrecovered");
        break;
    case SES_STAT_CODE_NOT_INSTALLED:
        additionalStatusStr = (char *)("Not Installed");
        break;
    case SES_STAT_CODE_UNKNOWN:
        additionalStatusStr = (char *)("Unknown");
        break;
    case SES_STAT_CODE_UNAVAILABLE:
        additionalStatusStr = (char *)("Unavailable");
        break;
    default:
        additionalStatusStr = (char *)("Unrecognized");
        break;
    }

    return(additionalStatusStr);
}

/*!*************************************************************************
 *                         @fn enclosure_access_printSscInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace SSC(Sytem Status Card) component data.
 *
 *  PARAMETERS:
 *      @param  componentTypeDataPtr  - pointer to Component Data
 *      @param  enclosureType  - type of enclosure
 *      @param  componentIndex - index to a specific component
 *      @param  printFullData  - flag to print all data (if not set, usually for tracing 
 *                               updated component data
 *      @param  trace_func     - EDAL trace function
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    10-Jan-2014: PHE - Created
 *
 **************************************************************************/
void
enclosure_access_printSscInfo(fbe_enclosure_component_t   *componentTypeDataPtr,
                                      fbe_enclosure_types_t enclosureType,
                                      fbe_u32_t componentIndex,
                                      fbe_bool_t printFullData, 
                                      fbe_edal_trace_func_t trace_func)
{
    fbe_edal_general_comp_handle_t  generalDataPtr;
    fbe_eses_encl_ssc_info_t       *sscDataPtr;
    fbe_u32_t                       index;


    if (componentTypeDataPtr->componentType != FBE_ENCL_SSC)
    {
        return;
    }

    // loop through all Connectors
    for (index = componentTypeDataPtr->firstComponentIndex; 
          index < componentTypeDataPtr->firstComponentIndex + componentTypeDataPtr->maxNumberOfComponents; 
          index++)
    {
        // check if we should print this component index
        if ((componentIndex == FBE_ENCL_COMPONENT_INDEX_ALL) || 
            (componentIndex == index))
        {
            if (printFullData)
            {
                trace_func(NULL, "  SSC %d\n", index);
            }
            else
            {
                trace_func(NULL, "State Change to SSC %d\n", index);
            }
            enclosure_access_getSpecifiedComponentData(componentTypeDataPtr,
                                                       index,
                                                       &generalDataPtr);
            sscDataPtr = (fbe_eses_encl_ssc_info_t *)generalDataPtr;

            enclosure_access_printGeneralComponentInfo(&(sscDataPtr->baseSscInfo.generalComponentInfo),
                                                       printFullData, trace_func);

            if(IS_VALID_ENCL(enclosureType)) 
            {
                enclosure_access_printEsesSscInfo(sscDataPtr, trace_func);
            }
        }
    }
}   // end of enclosure_access_printSscInfo


/*!*************************************************************************
 *                         @fn enclosure_access_printEsesSscInfo
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will print the SSC(System Status Card) Component data.
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    10-Jan-2014: PHE - Created
 *
 **************************************************************************/
void 
enclosure_access_printEsesSscInfo (fbe_eses_encl_ssc_info_t *sscInfoPtr, 
                                          fbe_edal_trace_func_t trace_func)
{
    trace_func(NULL, "  ESES SSC Info\n");
    trace_func(NULL, "    ElemIndx : %d\n",
        sscInfoPtr->sscElementIndex);
    return;
}   // end of enclosure_access_printEsesSscInfo

// End of file fbe_enclosure_data_access_print.c

