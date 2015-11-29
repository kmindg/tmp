/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008 - 2010
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_base_board_pe_read.c
 ***************************************************************************
 * @brief
 *  The routines in this file read the specl data.   
 *  
 * HISTORY:
 *  10-Mar-2010 PHE - Created.                  
 ***************************************************************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_board.h"
#include "fbe_scheduler.h"

#include "base_discovering_private.h"
#include "base_board_private.h"
#include "base_board_pe_private.h"
#include "fbe_transport_memory.h"
#include "fbe_pe_types.h"
#include "speeds.h"
#include "fbe_notification_interface.h"
#include "fbe_notification.h"
#include "edal_processor_enclosure_data.h"
#include "fbe_eir_info.h"
#include "generic_utils_lib.h"


extern fbe_base_board_pe_init_data_t   pe_init_data[FBE_BASE_BOARD_PE_NUMBER_OF_COMPONENT_TYPES];

static fbe_status_t
fbe_base_board_platform_type_to_board_type(SPID_HW_TYPE platform_type, 
                                           fbe_board_type_t * board_type);

static fbe_status_t
fbe_base_board_init_io_module_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * io_module_count_p);
static fbe_status_t
fbe_base_board_init_mezzanine_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * mezzanine_count_p);
static fbe_status_t
fbe_base_board_init_local_io_module_count(fbe_base_board_t * pBaseBoard, 
                                         fbe_u32_t * pLocalIoModuleCount);

static fbe_status_t
fbe_base_board_init_local_io_port_count(fbe_base_board_t * pBaseBoard, 
                                      fbe_u32_t * pLocalIoPortCount);

static fbe_status_t
fbe_base_board_init_bem_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * bem_count_p);

static fbe_status_t
fbe_base_board_init_ps_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * ps_count_p);

static fbe_status_t 
fbe_base_board_translate_sps_status(fbe_base_board_t * base_board,
                                    edal_pe_sps_sub_info_t * pPeSpsSubInfo,     
                                    SPECL_SPS_SUMMARY * pSpeclSpsSum);

static fbe_status_t 
fbe_base_board_translate_sps_isp_state(fbe_base_board_t * base_board,
                                       edal_pe_sps_sub_info_t * pPeSpsSubInfo,
                                       SPECL_SPS_SUMMARY * pSpeclSpsSum, 
                                       SPS_ISP_STATE ispState);
static fbe_status_t 
fbe_base_board_translate_sps_resume(fbe_base_board_t * base_board,
                                    edal_pe_sps_sub_info_t * pPeSpsSubInfo,     
                                    SPECL_SPS_RESUME * pSpeclSpsResume);

static fbe_status_t
fbe_base_board_init_fan_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * fan_count_p);

static fbe_status_t
fbe_base_board_init_mgmt_module_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * mgmt_module_count_p);

static fbe_status_t
fbe_base_board_init_flt_reg_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * flt_reg_count_p);

static fbe_status_t
fbe_base_board_init_slave_port_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * slave_port_count_p);

static fbe_status_t
fbe_base_board_init_temperature_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * temperature_count_p);

static fbe_status_t
fbe_base_board_init_sps_count(fbe_base_board_t * base_board, 
                              fbe_u32_t * sps_count_p);

static fbe_status_t
fbe_base_board_init_battery_count(fbe_base_board_t * base_board, 
                                  fbe_u32_t * battery_count_p);

static fbe_status_t
fbe_base_board_init_bmc_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * bmc_count_p);

static fbe_status_t
fbe_base_board_init_cache_card_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * cache_card_count_p);

static fbe_status_t
fbe_base_board_init_dimm_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * dimm_count_p);

static fbe_status_t
fbe_base_board_init_ssd_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * ssd_count_p);

static fbe_status_t
fbe_base_board_init_suitcase_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * suitcase_count_p);

static fbe_status_t 
fbe_base_board_translate_misc_status(fbe_board_mgmt_misc_info_t *pPeMiscSubInfo, 
                                        SPECL_MISC_SUMMARY *pSpeclMiscSum);

static fbe_status_t 
fbe_base_board_translate_led_status(fbe_board_mgmt_misc_info_t *pPeMiscSubInfo, 
                                    SPECL_LED_SUMMARY *pSpeclLedcSum);

static fbe_status_t
fbe_base_board_read_io_module_status(fbe_base_board_t * base_board,
                                     SPECL_IO_SUMMARY * pSpeclIoSum);

static fbe_status_t  
fbe_base_board_read_io_port_status_on_io_comp(fbe_base_board_t * pBaseBoard,
                                               edal_pe_io_comp_sub_info_t * pPeIoCompSubInfo,
                                               fbe_u32_t ioControllerCount,
                                               SPECL_IO_CONTROLLER_STATUS speclIoControllerInfo[]);

static fbe_status_t
fbe_base_board_read_bem_status(fbe_base_board_t * base_board,
                               SPECL_IO_SUMMARY * pSpeclIoSum);

static fbe_status_t 
fbe_base_board_translate_io_status(fbe_base_board_t * pBaseBoard,
                                   edal_pe_io_comp_sub_info_t * pPeIoCompSubInfo,
                                   SPECL_IO_STATUS  * pSpeclIoStatus);

static fbe_status_t 
fbe_base_board_translate_io_port_status(edal_pe_io_port_sub_info_t * pPeIoPortSubInfo,
                                      edal_pe_io_comp_sub_info_t * pPeIoCompSubInfo,
                                      SPECL_IO_CONTROLLER_STATUS * pSpeclIoControllerInfo,
                                      SPECL_IO_PORT_DATA * pSpeclIoPortData);

static fbe_status_t 
fbe_base_board_translate_mezzanine_status(edal_pe_io_comp_sub_info_t * pMezzanineSubInfo,                                      
                                          SPECL_IO_STATUS  * pSpeclMezzanineStatus);

static fbe_status_t 
fbe_base_board_translate_power_supply_status(fbe_base_board_t * pBaseBoard,
                                                edal_pe_power_supply_sub_info_t * pPePowerSupplySubInfo,     
                                                SPECL_PS_SUMMARY * pSpeclPowerSupplySum,
                                                SPECL_PS_STATUS  * pSpeclPowerSupplyStatus);

static fbe_status_t 
fbe_base_board_translate_cooling_status(edal_pe_cooling_sub_info_t * pPeCooingSubInfo,                                      
                                             SPECL_FAN_SUMMARY  * pSpeclFanSum);

static fbe_status_t 
fbe_base_board_translate_mgmt_module_status(edal_pe_mgmt_module_sub_info_t * pMgmtModuleSubInfo,                                      
                                            SPECL_MGMT_STATUS  * pSpeclMgmtStatus);

static void 
fbe_base_board_translate_mgmt_port_settings(fbe_mgmt_port_status_t *mgmt_port, 
                                            PORT_STS_CONFIG *specl_mgmt_port);

static fbe_status_t 
fbe_base_board_translate_flt_reg_status(edal_pe_flt_exp_sub_info_t * peFltExpSubInfo,                                      
                                        SPECL_FAULT_REGISTER_STATUS  * pSpeclFltRegStatus);

static fbe_status_t 
fbe_base_board_translate_slave_port_status(edal_pe_slave_port_sub_info_t * peSlavePortSubInfo,                                      
                                           SPECL_SLAVE_PORT_STATUS  * pSpeclSlavePortStatus);

static fbe_status_t 
fbe_base_board_translate_suitcase_status(edal_pe_suitcase_sub_info_t * pSuitcaseSubInfo,                                      
                                             SPECL_SUITCASE_STATUS  * pSpeclSuitcaseStatus);

static fbe_status_t 
fbe_base_board_translate_bmc_status(edal_pe_bmc_sub_info_t * pBmcSubInfo,
                                             SPECL_BMC_STATUS  * pSpeclBmcStatus);
static fbe_status_t 
fbe_base_board_translate_cache_card_status(edal_pe_cache_card_sub_info_t * pCacheCardSubInfo,
                                             SPECL_CACHE_CARD_STATUS  * pSpeclCacheCardStatus);
static fbe_status_t 
fbe_base_board_translate_dimm_status(edal_pe_dimm_sub_info_t * pDimmSubInfo,
                                             SPECL_DIMM_STATUS  * pSpeclDimmStatus);
static fbe_status_t 
fbe_base_board_translate_temperature_status(edal_pe_temperature_sub_info_t *pPeTempSubInfo,
                                            SPECL_TEMPERATURE_STATUS *pSpeclTempStatus);
static fbe_status_t 
fbe_base_board_translate_resume_prom_status(edal_pe_resume_prom_sub_info_t * pPeResumePromSubInfo,                                      
                                            SPECL_RESUME_DATA  * pSpeclResumeData);
static fbe_status_t 
fbe_base_board_translate_battery_status(fbe_base_board_t * base_board,
                                        edal_pe_battery_sub_info_t * pPeBatterySubInfo,     
                                        SPECL_BATTERY_SUMMARY * pSpeclBatterySum,
                                        SPECL_BATTERY_STATUS * pSpeclBatteryStatus);

static void 
base_board_send_data_change_notification(fbe_base_board_t *pBaseBoard, 
                                         fbe_notification_data_changed_info_t * pDataChangeInfo);

static fbe_status_t
base_board_get_data_change_info(fbe_base_board_t * pBaseBoard,
                                fbe_enclosure_component_types_t componentType,
                                fbe_u32_t componentIndex,
                                fbe_notification_data_changed_info_t * pDataChangeInfo);

static fbe_status_t
fbe_base_board_check_and_get_fup_data_change_info(fbe_base_board_t * pBaseBoard,
                                fbe_enclosure_component_types_t componentType,
                                fbe_u32_t componentIndex,
                                fbe_notification_data_changed_info_t * pDataChangeInfo);

static fbe_status_t
fbe_base_board_check_env_interface_error(fbe_base_board_t *base_board,
                                          fbe_enclosure_component_types_t component_type,
                                          SPECL_ERROR transactionStatus,
                                          fbe_u64_t newTimeStamp,
                                          fbe_u64_t *pTimeStampForFirstBadStatus, 
                                          fbe_env_inferface_status_t *pEnvInterfaceStatus);

static fbe_status_t
fbe_base_board_get_component_max_status_timeout(fbe_enclosure_component_types_t component_type, 
                                            fbe_u32_t *max_status_timeout);

static fbe_status_t 
fbe_base_board_process_persistent_multi_fan_fault(fbe_base_board_t *base_board,
                                                  fbe_u64_t newTimeStamp, 
                                                  fbe_u32_t fan_fault_count);

static fbe_status_t 
fbe_base_board_set_persistent_multi_fan_module_fault(fbe_base_board_t *base_board, 
                                              fbe_mgmt_status_t persistentMultiFanModuleFault,
                                              fbe_u64_t timeStampForMultiFaultedStateStart);

static fbe_status_t
fbe_base_board_set_io_insert_bit(fbe_base_board_t * pBaseBoard, 
                                 edal_pe_io_comp_sub_info_t *pPeIoCompSubInfo, 
                                 SPECL_IO_STATUS  *newSpeclIoStatus);

static fbe_status_t
fbe_base_board_set_io_uniqueID(edal_pe_io_comp_sub_info_t *pPeIoCompSubInfo, 
                               SPECL_IO_STATUS  *newSpeclIoStatus);

static void 
fbe_base_board_setPsInputPower(SPECL_PS_STATUS  *pSpeclPowerSupplyStatus,
                               fbe_eir_input_power_sample_t *inputPowerPtr);

static void 
fbe_base_board_saveEirSample(fbe_base_board_t *base_board,
                             fbe_u32_t psIndex,
                             fbe_eir_input_power_sample_t *inputPowerPtr);
static void 
fbe_base_board_processEirSample(fbe_base_board_t *base_board,
                                fbe_u32_t psCount);

static fbe_dimm_state_t 
fbe_base_board_translate_dimm_state(fbe_board_mgmt_dimm_info_t *pDimmInfo);

static void fbe_base_board_initBmcBistStatus(fbe_board_mgmt_bist_info_t *pBistReport);


/*!*************************************************************************
 *  @fn fbe_base_board_get_platform_info_and_board_type
 **************************************************************************
 *  @brief
 *      This function saves the platform info to EDAL and 
 *        converts the platform type to the board type. 
 *
 *  @param base_board- The pointer to the base board object.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    08-Apr-2010: PHE - Created.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_board_get_platform_info_and_board_type(fbe_base_board_t * base_board,
                              fbe_board_type_t * board_type)
{
    fbe_status_t           status = FBE_STATUS_OK;   
    SPECL_MISC_SUMMARY     speclMiscSum = {0};
    SPECL_MISC_SUMMARY   * pSpeclMiscSum = &speclMiscSum;

    /* Read SPECL Platform info. */
    fbe_zero_memory(&base_board->platformInfo, sizeof(SPID_PLATFORM_INFO));
    /* Get the platform info. 
     * This function fbe_base_board_get_platform_info gets the platform info and 
     * also initializes the SPECL data in the simulation mode.
     * So we can get the component count from the SPECL data 
     * while initializing the base board.
     */
    status = fbe_base_board_get_platform_info(&base_board->platformInfo);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }   

    /* Read SPECL MISC info to get the spid. */
    fbe_zero_memory(pSpeclMiscSum, sizeof(SPECL_MISC_SUMMARY));
    status = fbe_base_board_pe_get_misc_status(pSpeclMiscSum);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) base_board,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: fbe_base_board_pe_get_misc_status() failed, status: 0x%x\n",
                              __FUNCTION__, status);
        return status;  
    }
    if(pSpeclMiscSum->spid == SP_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t *) base_board,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: fbe_base_board_pe_get_misc_status() returned SP_INVALID.\n",
                              __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* We need to get spid before init component size for local IO port. */
    base_board->spid = pSpeclMiscSum->spid;   

    status = fbe_base_board_platform_type_to_board_type(base_board->platformInfo.platformType, 
                                                           board_type);

    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    status = fbe_base_board_is_single_sp_system(&base_board->isSingleSpSystem);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    status = fbe_base_board_is_mfgMode(&base_board->mfgMode);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_board_platform_type_to_board_type
 **************************************************************************
 *  @brief
 *      This function converts the platform type to board type.
 *
 *  @param platform_type- The processor enclosure platform type.
 *  @param board_type - the pointer to the returned board type.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    08-Apr-2010: PHE - Created.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_board_platform_type_to_board_type(SPID_HW_TYPE platform_type, 
                                           fbe_board_type_t * board_type)
{
    switch(platform_type) 
    {
        case SPID_ENTERPRISE_HW_TYPE:/* HACK!!! */

        /* HACKS that were never unhacked!!! 
         * There should be a Transformers board type, logically*/
        case SPID_PROMETHEUS_M1_HW_TYPE:
        case SPID_PROMETHEUS_S1_HW_TYPE:
        case SPID_DEFIANT_M1_HW_TYPE:
        case SPID_DEFIANT_M2_HW_TYPE:
        case SPID_DEFIANT_M3_HW_TYPE:
        case SPID_DEFIANT_M4_HW_TYPE:
        case SPID_DEFIANT_M5_HW_TYPE:
        case SPID_DEFIANT_S1_HW_TYPE:
        case SPID_DEFIANT_S4_HW_TYPE:
        case SPID_DEFIANT_K2_HW_TYPE:
        case SPID_DEFIANT_K3_HW_TYPE:
        case SPID_NOVA1_HW_TYPE:
        case SPID_NOVA3_HW_TYPE:
        case SPID_NOVA3_XM_HW_TYPE:
        case SPID_NOVA_S1_HW_TYPE:
        case SPID_BEARCAT_HW_TYPE:
// Moons
        case SPID_OBERON_1_HW_TYPE:
        case SPID_OBERON_2_HW_TYPE:
        case SPID_OBERON_3_HW_TYPE:
        case SPID_OBERON_4_HW_TYPE:
        case SPID_OBERON_S1_HW_TYPE:
        case SPID_HYPERION_1_HW_TYPE:
        case SPID_HYPERION_2_HW_TYPE:
        case SPID_HYPERION_3_HW_TYPE:
        case SPID_TRITON_1_HW_TYPE:

            *board_type = FBE_BOARD_TYPE_ARMADA;
            break;

        case SPID_MERIDIAN_ESX_HW_TYPE:
        case SPID_TUNGSTEN_HW_TYPE:
            *board_type = FBE_BOARD_TYPE_ARMADA;
            break;
        
        case SPID_UNKNOWN_HW_TYPE:
        default:
            /* Uknown or unsupported type */

        break;
    }

    if(*board_type == FBE_BOARD_TYPE_INVALID){
        return FBE_STATUS_GENERIC_FAILURE;
    } else {
        return FBE_STATUS_OK;
    }
}


/*!*************************************************************************
 *  @fn fbe_base_board_init_component_size
 **************************************************************************
 *  @brief
 *      This function initializes the component size for 
 *      all the processor enclosure component types based on the EDAL data.
 *
 *  @param pe_init_data - pointer to the array which will save 
 *            the component size for all the processor enclosure component types.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  NOTES:
 *
 *  HISTORY:
 *    03-Mar-2010: PHE - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_init_component_size(fbe_base_board_pe_init_data_t pe_init_data[])
{
    fbe_u32_t           index = 0;
    fbe_u32_t           component_size = 0;
    fbe_u32_t           component_type = 0;
    fbe_edal_status_t   edal_status = FBE_EDAL_STATUS_OK;   

        
    for(index = 0; index < FBE_BASE_BOARD_PE_NUMBER_OF_COMPONENT_TYPES; index++)
    {
        component_type = pe_init_data[index].component_type;
        
        switch(component_type)
        {
            case FBE_ENCL_IO_MODULE:
            case FBE_ENCL_MEZZANINE:
            case FBE_ENCL_IO_PORT:
            case FBE_ENCL_BEM:     
            case FBE_ENCL_POWER_SUPPLY:
            case FBE_ENCL_COOLING_COMPONENT:
            case FBE_ENCL_MGMT_MODULE:                   
            case FBE_ENCL_PLATFORM:
            case FBE_ENCL_SUITCASE:
            case FBE_ENCL_BMC:
            case FBE_ENCL_MISC:
            case FBE_ENCL_FLT_REG:
            case FBE_ENCL_SLAVE_PORT:
            case FBE_ENCL_RESUME_PROM:
            case FBE_ENCL_TEMPERATURE:
            case FBE_ENCL_SPS:
            case FBE_ENCL_BATTERY:
            case FBE_ENCL_CACHE_CARD:
            case FBE_ENCL_DIMM:
            case FBE_ENCL_SSD:
                edal_status = fbe_edal_get_component_size(FBE_ENCL_PE, component_type, &component_size);                
                break;

            default:
                break;
        }

        if(edal_status == FBE_EDAL_STATUS_OK)
        {
            pe_init_data[index].component_size = component_size;
        }
        else
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }
    }

    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_get_required_buffer_size
 **************************************************************************
 *  @brief
 *      This function gets the component counts for all the processor enclosure component types.
 *
 *  @param pe_init_data - pointer to the array which will save 
 *                        the processor enclosure component init info.
 *  @param pe_buffer_size_p- The pointer to the required buffer size.
 *
 *  @return fbe_status_t - always return FEB_STATUS_OK 
 *  NOTES:
 *
 *  HISTORY:
 *    03-Mar-2010: PHE - Created.
 *
 **************************************************************************/
fbe_status_t
fbe_base_board_pe_get_required_buffer_size(fbe_base_board_pe_init_data_t pe_init_data[],
                                           fbe_u32_t *pe_buffer_size_p)
{
    fbe_u32_t index = 0;
    fbe_u32_t max_size = 0;

    *pe_buffer_size_p = 0;

    for(index = 0; index < FBE_BASE_BOARD_PE_NUMBER_OF_COMPONENT_TYPES; index++)
    {
        switch(pe_init_data[index].component_type)
        {
            case FBE_ENCL_PLATFORM:
                max_size = (max_size > sizeof(SPID_PLATFORM_INFO)) ? max_size : sizeof(SPID_PLATFORM_INFO);
                break;
            
            case FBE_ENCL_MISC:
                max_size = (max_size > sizeof(SPECL_LED_SUMMARY)) ? max_size : sizeof(SPECL_LED_SUMMARY);
                max_size = (max_size > sizeof(SPECL_MISC_SUMMARY)) ? max_size : sizeof(SPECL_MISC_SUMMARY);
                break;
            
            case FBE_ENCL_IO_MODULE:
            case FBE_ENCL_MEZZANINE:
            case FBE_ENCL_BEM: 
                /* IO port info will be availabe together with the IO component on which it resides.*/
                max_size = (max_size > sizeof(SPECL_IO_SUMMARY)) ? max_size : sizeof(SPECL_IO_SUMMARY);
                break;

            case FBE_ENCL_POWER_SUPPLY:
                max_size = (max_size > sizeof(SPECL_PS_SUMMARY)) ? max_size : sizeof(SPECL_PS_SUMMARY);
                break;

            case FBE_ENCL_COOLING_COMPONENT:
                max_size = (max_size > sizeof(SPECL_FAN_SUMMARY)) ? max_size : sizeof(SPECL_FAN_SUMMARY);
                break;

            case FBE_ENCL_MGMT_MODULE:
                max_size = (max_size > sizeof(SPECL_MGMT_SUMMARY)) ? max_size : sizeof(SPECL_MGMT_SUMMARY);
                break;

            case FBE_ENCL_FLT_REG:
                max_size = (max_size > sizeof(SPECL_FLT_EXP_SUMMARY)) ? max_size : sizeof(SPECL_FLT_EXP_SUMMARY);
                break;

            case FBE_ENCL_SLAVE_PORT:
                max_size = (max_size > sizeof(SPECL_SLAVE_PORT_SUMMARY)) ? max_size : sizeof(SPECL_SLAVE_PORT_SUMMARY);
                break;            

            case FBE_ENCL_SUITCASE:
                max_size = (max_size > sizeof(SPECL_SUITCASE_SUMMARY)) ? max_size : sizeof(SPECL_SUITCASE_SUMMARY);
                break;        

            case FBE_ENCL_BMC:
                max_size = (max_size > sizeof(SPECL_BMC_SUMMARY)) ? max_size : sizeof(SPECL_BMC_SUMMARY);
                break;

            case FBE_ENCL_RESUME_PROM:
                max_size = (max_size > sizeof(SPECL_RESUME_DATA)) ? max_size : sizeof(SPECL_RESUME_DATA);
                break;            

            case FBE_ENCL_TEMPERATURE:
                max_size = (max_size > sizeof(SPECL_TEMPERATURE_SUMMARY)) ? max_size : sizeof(SPECL_TEMPERATURE_SUMMARY);
                break;            

            case FBE_ENCL_SPS:
                max_size = (max_size > sizeof(SPECL_SPS_SUMMARY)) ? max_size : sizeof(SPECL_SPS_SUMMARY);
                break;            

            case FBE_ENCL_BATTERY:
                max_size = (max_size > sizeof(SPECL_BATTERY_SUMMARY)) ? max_size : sizeof(SPECL_BATTERY_SUMMARY);
                break;            

            case FBE_ENCL_CACHE_CARD:
                max_size = (max_size > sizeof(SPECL_CACHE_CARD_SUMMARY)) ? max_size : sizeof(SPECL_CACHE_CARD_SUMMARY);
                break;        
    
            case FBE_ENCL_DIMM:
                max_size = (max_size > sizeof(SPECL_DIMM_SUMMARY)) ? max_size : sizeof(SPECL_DIMM_SUMMARY);
                break;  
                
            case FBE_ENCL_SSD:
                max_size = (max_size > sizeof(fbe_board_mgmt_ssd_info_t)) ? max_size: sizeof(fbe_board_mgmt_ssd_info_t);
                break;

            default:
                break;
        }
    }// End of for loop

    *pe_buffer_size_p = max_size;

    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_board_init_component_count
 **************************************************************************
 *  @brief
 *      This function initializes the component count for 
 *      all the processor enclosure component types based on the SPECL data.
 *
 *  @param base_board- The pointer to the base board object.
 *  @param pe_init_data - pointer to the array which will save 
 *                        the processor enclosure component init info.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  NOTES:
 *
 *  HISTORY:
 *    03-Mar-2010: PHE - Created.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_board_init_component_count(fbe_base_board_t * base_board,
                                       fbe_base_board_pe_init_data_t pe_init_data[])
{
    fbe_u32_t           index = 0;
    fbe_u32_t           component_count = 0;
    fbe_status_t        status = FBE_STATUS_OK; 

    for(index = 0; index < FBE_BASE_BOARD_PE_NUMBER_OF_COMPONENT_TYPES; index ++ )
    {
        switch(pe_init_data[index].component_type)
        {
            case FBE_ENCL_RESUME_PROM:
                component_count = FBE_PE_MAX_RESUME_PROM_IDS;   
                status = FBE_STATUS_OK;
                break;

            case FBE_ENCL_PLATFORM:
            case FBE_ENCL_MISC:
                component_count = 1;   
                status = FBE_STATUS_OK;
                break;
            
            case FBE_ENCL_IO_MODULE:
                status = fbe_base_board_init_io_module_count(base_board, &component_count);               
                break;

            case FBE_ENCL_MEZZANINE:
                status = fbe_base_board_init_mezzanine_count(base_board, &component_count);               
                break;

            case FBE_ENCL_IO_PORT:
                status = fbe_base_board_init_local_io_port_count(base_board, &component_count);               
                break;

            case FBE_ENCL_BEM:
                status = fbe_base_board_init_bem_count(base_board, &component_count);               
                break;            

            case FBE_ENCL_POWER_SUPPLY:
                status = fbe_base_board_init_ps_count(base_board, &component_count);               
                break;

            case FBE_ENCL_COOLING_COMPONENT:
                status = fbe_base_board_init_fan_count(base_board, &component_count);               
                break;

            case FBE_ENCL_MGMT_MODULE:
                status = fbe_base_board_init_mgmt_module_count(base_board, &component_count);               
                break;    

            case FBE_ENCL_FLT_REG:
                status = fbe_base_board_init_flt_reg_count(base_board, &component_count);               
                break;

            case FBE_ENCL_SLAVE_PORT:
                status = fbe_base_board_init_slave_port_count(base_board, &component_count);               
                break;

            case FBE_ENCL_SUITCASE:
                status = fbe_base_board_init_suitcase_count(base_board, &component_count);               
                break;

            case FBE_ENCL_BMC:
                status = fbe_base_board_init_bmc_count(base_board, &component_count);
                break;

            case FBE_ENCL_TEMPERATURE:
                status = fbe_base_board_init_temperature_count(base_board, &component_count);               
                break;            

            case FBE_ENCL_SPS:
                status = fbe_base_board_init_sps_count(base_board, &component_count);               
                break;            

            case FBE_ENCL_BATTERY:
                status = fbe_base_board_init_battery_count(base_board, &component_count);               
                break;            

            case FBE_ENCL_CACHE_CARD:
                status = fbe_base_board_init_cache_card_count(base_board, &component_count);               
                break;    

            case FBE_ENCL_DIMM:
                status = fbe_base_board_init_dimm_count(base_board, &component_count);               
                break;    

            case FBE_ENCL_SSD:
                status = fbe_base_board_init_ssd_count(base_board, &component_count);
                break;

            default:
                break;
        }

        if(status == FBE_STATUS_OK)
        {
            pe_init_data[index].component_count = component_count;
        }
        else
        {
            return status;  
        }
    }

    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_board_init_io_module_count(fbe_base_board_t * base_board, 
 *                                    fbe_u32_t * io_module_count_p)
 **************************************************************************
 *  @brief
 *      This function gets the total IO module count.
 *
 *  @param base_board- The pointer to the base board object.
 *  @param io_module_count_p - pointer to the returned io module count.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  NOTES:
 *
 *  HISTORY:
 *    03-Mar-2010: PHE - Created.
 *
 **************************************************************************/
static fbe_status_t
fbe_base_board_init_io_module_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * io_module_count_p)
{
    SP_ID               sp;
    fbe_u32_t           ioModule;
    SPECL_IO_SUMMARY  * pSpeclIoSum = NULL;
    fbe_status_t        status = FBE_STATUS_OK; 

    *io_module_count_p = 0;

    pSpeclIoSum = (SPECL_IO_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclIoSum, sizeof(SPECL_IO_SUMMARY));

    status = fbe_base_board_pe_get_io_comp_status(pSpeclIoSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    FBE_FOR_ALL_IO_MODULES(sp, ioModule, pSpeclIoSum)
    {
        if ((pSpeclIoSum->ioSummary[sp].ioStatus[ioModule].moduleType == IO_MODULE_TYPE_SLIC_1_0) ||
            (pSpeclIoSum->ioSummary[sp].ioStatus[ioModule].moduleType == IO_MODULE_TYPE_SLIC_2_0))
        {
            *io_module_count_p += 1;
        }
    }

    return status;
}


/*!*************************************************************************
 *  @fn fbe_base_board_init_mezzanine_count(fbe_base_board_t * base_board, 
 *                                    fbe_u32_t * mezzanine_count_p)
 **************************************************************************
 *  @brief
 *      This function gets the total mezzanine count.
 *
 *  @param base_board- The pointer to the base board object.
 *  @param mezzanine_count_p - pointer to the returned mezzanine count.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    07-Apr-2010: PHE - Created.
 *
 **************************************************************************/
static fbe_status_t
fbe_base_board_init_mezzanine_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * mezzanine_count_p)
{
    SP_ID               sp;
    fbe_u32_t           ioModule;
    SPECL_IO_SUMMARY  * pSpeclIoSum = NULL;
    fbe_status_t        status = FBE_STATUS_OK; 

    *mezzanine_count_p = 0;

    pSpeclIoSum = (SPECL_IO_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclIoSum, sizeof(SPECL_IO_SUMMARY));

    status = fbe_base_board_pe_get_io_comp_status(pSpeclIoSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    FBE_FOR_ALL_IO_MODULES(sp, ioModule, pSpeclIoSum)
    {
        if ((pSpeclIoSum->ioSummary[sp].ioStatus[ioModule].moduleType == IO_MODULE_TYPE_MEZZANINE) ||
            (pSpeclIoSum->ioSummary[sp].ioStatus[ioModule].moduleType == IO_MODULE_TYPE_ONBOARD))
        {
            *mezzanine_count_p += 1;
        }
    }

    return status;
}

/*!*************************************************************************
 *  @fn fbe_base_board_init_local_io_module_count(fbe_base_board_t * pBase_board, 
 *                         fbe_u32_t * pLocalIoModuleCount)
 **************************************************************************
 *  @brief
 *      This function gets the local IO module count.
 *
 *  @param base_board- The pointer to the base board object.
 *  @param io_module_count_p - pointer to the returned io module count.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  NOTES:
 *    This function has to be called after reading SPECL misc status to get the spid.
 *
 *  HISTORY:
 *    07-May-2010: PHE - Created.
 *
 **************************************************************************/
static fbe_status_t
fbe_base_board_init_local_io_module_count(fbe_base_board_t * pBaseBoard, 
                                         fbe_u32_t * pLocalIoModuleCount)
{
    fbe_u32_t           ioModule;
    fbe_u32_t           localAltIoDevCount = 0;
    SPECL_IO_SUMMARY  * pSpeclIoSum = NULL;
    fbe_status_t        status = FBE_STATUS_OK; 

    *pLocalIoModuleCount = 0;

    pSpeclIoSum = (SPECL_IO_SUMMARY *)pBaseBoard->pe_buffer_handle;
    fbe_zero_memory(pSpeclIoSum, sizeof(SPECL_IO_SUMMARY));

    status = fbe_base_board_pe_get_io_comp_status(pSpeclIoSum);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    *pLocalIoModuleCount = pSpeclIoSum->ioSummary[pBaseBoard->spid].ioModuleCount;
    //Hack to allow NDU of changes to single array of IO devices in SPECL
    for (ioModule = 0; ioModule < pSpeclIoSum->ioSummary[pBaseBoard->spid].ioModuleCount; ioModule++)
    {
        if ((pSpeclIoSum->ioSummary[pBaseBoard->spid].ioStatus[ioModule].moduleType != IO_MODULE_TYPE_SLIC_1_0) &&
            (pSpeclIoSum->ioSummary[pBaseBoard->spid].ioStatus[ioModule].moduleType != IO_MODULE_TYPE_SLIC_2_0))
        {
            localAltIoDevCount += 1;
        }
    }


    pBaseBoard->localIoModuleCount = *pLocalIoModuleCount;
    pBaseBoard->localAltIoDevCount = localAltIoDevCount;
    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_board_init_local_io_port_count(fbe_base_board_t * base_board, 
 *                                    fbe_u32_t * io_module_count_p)
 **************************************************************************
 *  @brief
 *      This function gets the local IO Port count.
 *
 *  @param base_board- The pointer to the base board object.
 *  @param io_module_count_p - pointer to the returned io module count.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  NOTES:
 *
 *  HISTORY:
 *    03-Mar-2010: PHE - Created.
 *
 **************************************************************************/
static fbe_status_t
fbe_base_board_init_local_io_port_count(fbe_base_board_t * pBaseBoard, 
                                      fbe_u32_t * pLocalIoPortCount)
{
    fbe_u32_t        localIoModuleCount = 0;
    fbe_status_t     status = FBE_STATUS_OK;

    *pLocalIoPortCount = 0;

    status = fbe_base_board_init_local_io_module_count(pBaseBoard, &localIoModuleCount);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    *pLocalIoPortCount = localIoModuleCount * MAX_IO_PORTS_PER_MODULE;
 
    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_board_init_bem_count(fbe_base_board_t * base_board, 
 *                                    fbe_u32_t * bem_count_p)
 **************************************************************************
 *  @brief
 *      This function gets the total BEM count.
 *
 *  @param base_board- The pointer to the base board object.
 *  @param io_module_count_p - pointer to the returned bem count.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    07-Apr-2010: PHE - Created.
 *
 **************************************************************************/
static fbe_status_t
fbe_base_board_init_bem_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * bem_count_p)
{
    SP_ID               sp;
    fbe_u32_t           ioModule;
    SPECL_IO_SUMMARY  * pSpeclIoSum = NULL;
    fbe_status_t        status = FBE_STATUS_OK; 

    *bem_count_p = 0;
        
    pSpeclIoSum = (SPECL_IO_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclIoSum, sizeof(SPECL_IO_SUMMARY));

    status = fbe_base_board_pe_get_io_comp_status(pSpeclIoSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    FBE_FOR_ALL_IO_MODULES(sp, ioModule, pSpeclIoSum)
    {
        if (pSpeclIoSum->ioSummary[sp].ioStatus[ioModule].moduleType == IO_MODULE_TYPE_BEM)
        {
            *bem_count_p += 1;
        }
    }

    return status;
}

/*!*************************************************************************
 *  @fn fbe_base_board_init_mgmt_module_count
 **************************************************************************
 *  @brief
 *      This function gets the total Management module count.
 *
 *  @param base_board- The pointer to the base board object.
 *  @param mgmt_module_count_p - pointer to the returned io module count.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  NOTES:
 *
 *  HISTORY:
 *    06-Apr-2010: AS - Created.
 *
 **************************************************************************/
static fbe_status_t
fbe_base_board_init_mgmt_module_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * mgmt_module_count_p)
{
    SP_ID               sp;
    SPECL_MGMT_SUMMARY  *pSpeclMgmtSum = NULL;
    fbe_status_t        status = FBE_STATUS_OK; 

    *mgmt_module_count_p = 0;

    pSpeclMgmtSum = (SPECL_MGMT_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclMgmtSum, sizeof(SPECL_MGMT_SUMMARY));

    status = fbe_base_board_pe_get_mgmt_module_status(pSpeclMgmtSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    for(sp = 0; sp < pSpeclMgmtSum->bladeCount; sp ++)
    {
        *mgmt_module_count_p += pSpeclMgmtSum->mgmtSummary[sp].mgmtCount;
    }

    return status;
}
    
/*!*************************************************************************
 *  @fn fbe_base_board_init_fan_count
 **************************************************************************
 *  @brief
 *      This function gets the total fan count.
 *
 *  @param base_board- The pointer to the base board object.
 *  @param fan_count_p - pointer to the returned fan count.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  NOTES:
 *
 *  HISTORY:
 *    06-Apr-2010: AS - Created.
 *
 **************************************************************************/
static fbe_status_t
fbe_base_board_init_fan_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * fan_count_p)
{
    SPECL_FAN_SUMMARY      *pSpeclFanSum = NULL;
    fbe_status_t           status = FBE_STATUS_OK;
    SP_ID               sp;

    *fan_count_p = 0;

    pSpeclFanSum = (SPECL_FAN_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclFanSum, sizeof(SPECL_FAN_SUMMARY));

    status = fbe_base_board_pe_get_cooling_status(pSpeclFanSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    for (sp = 0; sp < pSpeclFanSum->bladeCount; sp++)
    {
        *fan_count_p += pSpeclFanSum->fanSummary[sp].fanPackCount;
    }

    return status;
}

/*!*************************************************************************
 *  @fn fbe_base_board_init_ps_count
 **************************************************************************
 *  @brief
 *      This function gets the total Power Supply count.
 *
 *  @param base_board- The pointer to the base board object.
 *  @param mgmt_module_count_p - pointer to the returned io module count.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  NOTES:
 *
 *  HISTORY:
 *    06-Apr-2010: AS - Created.
 *
 **************************************************************************/
static fbe_status_t
fbe_base_board_init_ps_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * ps_count_p)
{
    SP_ID               sp;
    SPECL_PS_SUMMARY    *pSpeclPsSum = NULL;
    fbe_status_t        status = FBE_STATUS_OK; 

    *ps_count_p = 0;
        
    pSpeclPsSum = (SPECL_PS_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclPsSum, sizeof(SPECL_PS_SUMMARY));

    status = fbe_base_board_pe_get_power_supply_status(pSpeclPsSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    for(sp = 0; sp < pSpeclPsSum->bladeCount; sp ++)
    {
        *ps_count_p += pSpeclPsSum->psSummary[sp].psCount;
    }

    base_board->psCountPerBlade = pSpeclPsSum->psSummary[base_board->spid].psCount;
    return status;
}

/*!*************************************************************************
 *  @fn fbe_base_board_init_suitcase_count
 **************************************************************************
 *  @brief
 *      This function gets the total Suitcase count.
 *
 *  @param base_board- The pointer to the base board object.
 *  @param sc_count_p - pointer to the returned suitcase count.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  NOTES:
 *
 *  HISTORY:
 *    06-Apr-2010: AS - Created.
 *
 **************************************************************************/
static fbe_status_t
fbe_base_board_init_suitcase_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * sc_count_p)
{
    SP_ID                     sp;
    SPECL_SUITCASE_SUMMARY    *pSpeclSuitcaseSum = NULL;
    fbe_status_t              status = FBE_STATUS_OK; 

    *sc_count_p = 0;

    pSpeclSuitcaseSum = (SPECL_SUITCASE_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclSuitcaseSum, sizeof(SPECL_SUITCASE_SUMMARY));

    status = fbe_base_board_pe_get_suitcase_status(pSpeclSuitcaseSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    for(sp = 0; sp < pSpeclSuitcaseSum->bladeCount; sp ++)
    {
        *sc_count_p += pSpeclSuitcaseSum->suitcaseSummary[sp].suitcaseCount;
    }

    return status;
}

/*!*************************************************************************
 *  @fn fbe_base_board_init_bmc_count
 **************************************************************************
 *  @brief
 *      This function gets the total BMC count.
 *
 *  @param base_board- The pointer to the base board object.
 *  @param sc_count_p - pointer to the returned bmc count.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  NOTES:
 *
 *  HISTORY:
 *    11-Sep-2012  Eric Zhou - Created.
 *
 **************************************************************************/
static fbe_status_t
fbe_base_board_init_bmc_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * bmc_count_p)
{
    SP_ID                 sp;
    SPECL_BMC_SUMMARY     *pSpeclBMCSum = NULL;
    fbe_status_t          status = FBE_STATUS_OK; 

    *bmc_count_p = 0;

    pSpeclBMCSum = (SPECL_BMC_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclBMCSum, sizeof(SPECL_BMC_SUMMARY));

    status = fbe_base_board_pe_get_bmc_status(pSpeclBMCSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    for(sp = 0; sp < pSpeclBMCSum->bladeCount; sp ++)
    {
        *bmc_count_p += pSpeclBMCSum->bmcSummary[sp].bmcCount;
    }

    return status;
}

/*!*************************************************************************
 *  @fn fbe_base_board_init_cache_card_count
 **************************************************************************
 *  @brief
 *      This function gets the total cache card count.
 *
 *  @param base_board- The pointer to the base board object.
 *  @param cache_card_count_p - pointer to the returned cache_card count.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  NOTES:
 *
 *  HISTORY:
 *    25-Jan-2013  Rui Chang - Created.
 *
 **************************************************************************/
static fbe_status_t
fbe_base_board_init_cache_card_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * cache_card_count_p)
{
    SPECL_CACHE_CARD_SUMMARY     *pSpeclCacheCardSum = NULL;
    fbe_status_t          status = FBE_STATUS_OK; 

    *cache_card_count_p = 0;

    pSpeclCacheCardSum = (SPECL_CACHE_CARD_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclCacheCardSum, sizeof(SPECL_CACHE_CARD_SUMMARY));

    status = fbe_base_board_pe_get_cache_card_status(pSpeclCacheCardSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    *cache_card_count_p = pSpeclCacheCardSum->cacheCardCount;

    return status;
}

/*!*************************************************************************
 *  @fn fbe_base_board_init_dimm_count
 **************************************************************************
 *  @brief
 *      This function gets the total DIMM count.
 *
 *  @param base_board- The pointer to the base board object.
 *  @param cache_card_count_p - pointer to the returned DIMM count.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  NOTES:
 *
 *  HISTORY:
 *    15-Apr-2013  Rui Chang - Created.
 *
 **************************************************************************/
static fbe_status_t
fbe_base_board_init_dimm_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * dimm_count_p)
{
    SPECL_DIMM_SUMMARY     *pSpeclDimmSum = NULL;
    fbe_status_t          status = FBE_STATUS_OK; 

    *dimm_count_p = 0;

    pSpeclDimmSum = (SPECL_DIMM_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclDimmSum, sizeof(SPECL_DIMM_SUMMARY));

    status = fbe_base_board_pe_get_dimm_status(pSpeclDimmSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    *dimm_count_p = pSpeclDimmSum->dimmCount;

    return status;
}

/*!*************************************************************************
 *  @fn fbe_base_board_init_ssd_count
 **************************************************************************
 *  @brief
 *      This function gets the total SSD count.
 *
 *  @param base_board- The pointer to the base board object.
 *  @param ssd_count_p - pointer to the returned SSD count.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  NOTES:
 *
 *  HISTORY:
 *    05-Oct-2014  bphilbin - Created.
 *
 **************************************************************************/
static fbe_status_t
fbe_base_board_init_ssd_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * ssd_count_p)
{
    fbe_status_t          status = FBE_STATUS_OK; 

    *ssd_count_p = 0;

    status =  fbe_base_board_get_ssd_count(base_board, ssd_count_p);


    return status;
}


/*!*************************************************************************
 *  @fn fbe_base_board_init_flt_reg_count(fbe_base_board_t * base_board, 
 *                                    fbe_u32_t * flt_reg_count_p)
 **************************************************************************
 *  @brief
 *      This function gets the total fault register count.
 *
 *  @param base_board- The pointer to the base board object.
 *  @param mezzanine_count_p - pointer to the returned fault register count.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    17-July-2012: Chengkai Hu - Created.
 *
 **************************************************************************/
static fbe_status_t
fbe_base_board_init_flt_reg_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * flt_reg_count_p)
{
    SP_ID               sp;
    SPECL_FLT_EXP_SUMMARY  * pSpeclFltExpSum = NULL;
    fbe_status_t        status = FBE_STATUS_OK; 

    *flt_reg_count_p = 0;

    pSpeclFltExpSum = (SPECL_FLT_EXP_SUMMARY *)base_board->pe_buffer_handle;

    fbe_zero_memory(pSpeclFltExpSum, sizeof(SPECL_FLT_EXP_SUMMARY));

    status = fbe_base_board_pe_get_flt_exp_status(pSpeclFltExpSum);

    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    for(sp = 0; sp < pSpeclFltExpSum->bladeCount; sp ++)
    {
        *flt_reg_count_p += pSpeclFltExpSum->fltExpSummary[sp].faultRegisterCount;
    }

    return status;
} // End of - fbe_base_board_init_flt_reg_count

/*!*************************************************************************
 *  @fn fbe_base_board_init_slave_port_count(fbe_base_board_t * base_board, 
 *                                    fbe_u32_t * slave_port_count_p)
 **************************************************************************
 *  @brief
 *      This function gets the total slave port count.
 *
 *  @param base_board- The pointer to the base board object.
 *  @param slave_port_count_p - pointer to the returned slave port count.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    07-Apr-2010: PHE - Created.
 *
 **************************************************************************/
static fbe_status_t
fbe_base_board_init_slave_port_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * slave_port_count_p)
{
    SP_ID               sp;
    SPECL_SLAVE_PORT_SUMMARY  * pSpeclSlavePortSum = NULL;
    fbe_status_t        status = FBE_STATUS_OK; 

    *slave_port_count_p = 0;

    pSpeclSlavePortSum = (SPECL_SLAVE_PORT_SUMMARY *)base_board->pe_buffer_handle;

    fbe_zero_memory(pSpeclSlavePortSum, sizeof(SPECL_SLAVE_PORT_SUMMARY));

    status = fbe_base_board_pe_get_slave_port_status(pSpeclSlavePortSum);

    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    for(sp = 0; sp < pSpeclSlavePortSum->bladeCount; sp ++)
    {
        *slave_port_count_p += pSpeclSlavePortSum->slavePortSummary[sp].slavePortCount;
    }

    return status;
}  // End of fbe_base_board_init_slave_port_count


/*!*************************************************************************
 *  @fn fbe_base_board_init_temperature_count(fbe_base_board_t * base_board, 
 *                                    fbe_u32_t * temperature_count_p)
 **************************************************************************
 *  @brief
 *      This function gets the total temperature count.
 *
 *  @param base_board- The pointer to the base board object.
 *  @param temperature_count_p - pointer to the returned temperature count.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    04-Jan-2011:  Joe Perry - Created.
 *
 **************************************************************************/
static fbe_status_t
fbe_base_board_init_temperature_count(fbe_base_board_t * base_board, 
                                      fbe_u32_t * temperature_count_p)
{
    SPECL_TEMPERATURE_SUMMARY   * pSpeclTemperatureSum = NULL;
    fbe_status_t                status = FBE_STATUS_OK; 

    *temperature_count_p = 0;

    pSpeclTemperatureSum = (SPECL_TEMPERATURE_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclTemperatureSum, sizeof(SPECL_TEMPERATURE_SUMMARY));

    status = fbe_base_board_pe_get_temperature_status(pSpeclTemperatureSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    *temperature_count_p = pSpeclTemperatureSum->temperatureCount;

    return status;

}   // end of fbe_base_board_init_temperature_count


/*!*************************************************************************
 *  @fn fbe_base_board_init_sps_count(fbe_base_board_t * base_board, 
 *                                    fbe_u32_t * sps_count_p)
 **************************************************************************
 *  @brief
 *      This function gets the total SPS count.
 *
 *  @param base_board- The pointer to the base board object.
 *  @param sps_count_p - pointer to the returned SPS count.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    04-Jan-2011:  Joe Perry - Created.
 *
 **************************************************************************/
static fbe_status_t
fbe_base_board_init_sps_count(fbe_base_board_t * base_board, 
                              fbe_u32_t * sps_count_p)
{
    SPECL_SPS_SUMMARY               speclSpsSum;
    fbe_status_t                    status;

    status = fbe_base_board_pe_get_sps_status(&speclSpsSum);
    if ((status != FBE_STATUS_OK) || 
        (speclSpsSum.transactionStatus == STATUS_SMB_DEVICE_NOT_VALID_FOR_PLATFORM))
    {
        *sps_count_p = 0;
    }
    else
    {
        *sps_count_p = 1;
    }

    return status;

}   // end of fbe_base_board_init_sps_count


/*!*************************************************************************
 *  @fn fbe_base_board_init_battery_count(fbe_base_board_t * base_board, 
 *                                        fbe_u32_t * battery_count_p)
 **************************************************************************
 *  @brief
 *      This function gets the total Battery count.
 *
 *  @param base_board- The pointer to the base board object.
 *  @param battery_count_p - pointer to the returned Battery count.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    22-Mar-2012:  Joe Perry - Created.
 *
 **************************************************************************/
static fbe_status_t
fbe_base_board_init_battery_count(fbe_base_board_t * base_board, 
                                  fbe_u32_t * battery_count_p)
{
    SP_ID                   sp;
    SPECL_BATTERY_SUMMARY   *pSpeclBatterySum = NULL;
    fbe_status_t            status = FBE_STATUS_OK; 

    *battery_count_p = 0;
        
    pSpeclBatterySum = (SPECL_BATTERY_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclBatterySum, sizeof(SPECL_BATTERY_SUMMARY));

    status = fbe_base_board_pe_get_battery_status(pSpeclBatterySum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    for(sp = 0; sp < pSpeclBatterySum->bladeCount; sp ++)
    {
        *battery_count_p += pSpeclBatterySum->batterySummary[sp].batteryCount;
    }

    return status;
}   // end of fbe_base_board_init_battery_count


/*!*************************************************************************
 *  @fn fbe_base_board_read_misc_and_led_status
 **************************************************************************
 *  @brief
 *      This function gets the miscellaneous info and 
 *      fills in EDAL with the info. 
 *      It includes the specl led summary and misc summary.
 *
 *  @param base_board- The pointer to the base board object.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *       This function has to bee called before reading other component info
 *       accept the platform because other component will need spid to populate EDAL.
 *  @version
 *    08-Apr-2010: PHE - Created.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_board_read_misc_and_led_status(fbe_base_board_t * base_board)
{
    SPECL_MISC_SUMMARY  * pSpeclMiscSum = NULL;
    SPECL_LED_SUMMARY   * pSpeclLedSum = NULL;
    fbe_board_mgmt_misc_info_t    peMiscSubInfo = {0};
    fbe_board_mgmt_misc_info_t  * pPeMiscSubInfo = &peMiscSubInfo; 
    fbe_edal_status_t   edalStatus = FBE_EDAL_STATUS_OK;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_edal_block_handle_t edalBlockHandle = NULL;   

    fbe_base_board_get_edal_block_handle(base_board, &edalBlockHandle);

    fbe_zero_memory(pPeMiscSubInfo, sizeof(fbe_board_mgmt_misc_info_t));        
    /* Get old Misc info from EDAL. */
    edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                    FBE_ENCL_PE_MISC_INFO,  //Attribute
                                    FBE_ENCL_MISC,  // component type
                                    0,  // component index
                                    sizeof(fbe_board_mgmt_misc_info_t),  // buffer length
                                    (fbe_u8_t *)pPeMiscSubInfo);  // buffer pointer

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    /* Read SPECL MISC info. */
    pSpeclMiscSum = (SPECL_MISC_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclMiscSum, sizeof(SPECL_MISC_SUMMARY));

    status = fbe_base_board_pe_get_misc_status(pSpeclMiscSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }   

    /* Save sp id to the board board for easy reference. */
    base_board->spid = pSpeclMiscSum->spid;
    fbe_base_board_translate_misc_status(pPeMiscSubInfo, pSpeclMiscSum);

    /* Read SPECL LED info. */
    pSpeclLedSum = (SPECL_LED_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclLedSum, sizeof(SPECL_LED_SUMMARY));
    
    status = fbe_base_board_pe_get_led_status(pSpeclLedSum);

    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    fbe_base_board_translate_led_status(pPeMiscSubInfo, pSpeclLedSum);

    /* Update EDAL with new Misc info. */
    edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_MISC_INFO,  //Attribute
                                        FBE_ENCL_MISC,  // component type
                                        0,  // component index
                                        sizeof(fbe_board_mgmt_misc_info_t),  // buffer length
                                        (fbe_u8_t *)pPeMiscSubInfo);  // buffer pointer
    

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_STATUS_GENERIC_FAILURE;  
    }  

    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_board_translate_misc_status
 **************************************************************************
 *  @brief
 *      This function translates the SPECL misc status.
 *
 *  @param pPeMiscInfo- The pointer to fbe_board_mgmt_misc_info_t.
 *  @param pSpeclMiscSum - The pointer to SPECL_MISC_SUMMARY.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    20-Apr-2010: PHE - Created.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_board_translate_misc_status(fbe_board_mgmt_misc_info_t *pPeMiscInfo, 
                                        SPECL_MISC_SUMMARY *pSpeclMiscSum)
{
    pPeMiscInfo->spid = pSpeclMiscSum->spid;
    pPeMiscInfo->engineIdFault = pSpeclMiscSum->engineIdFault;
    pPeMiscInfo->peerInserted = pSpeclMiscSum->peerInserted;
    pPeMiscInfo->lowBattery = pSpeclMiscSum->low_battery;
    pPeMiscInfo->peerHoldInReset = pSpeclMiscSum->peerHoldInReset;
    pPeMiscInfo->peerHoldInPost = pSpeclMiscSum->peerHoldInPost;
    pPeMiscInfo->localHoldInPost = pSpeclMiscSum->localHoldInPost;
    pPeMiscInfo->fbeInternalCablePort1 = FBE_CABLE_STATUS_UNKNOWN;
    pPeMiscInfo->localPowerECBFault = FALSE;
    pPeMiscInfo->peerPowerECBFault = FALSE;
    pPeMiscInfo->cnaMode = CNA_MODE_UNKNOWN;
    if (pSpeclMiscSum->type == 1) //Jetfire
    {
        pPeMiscInfo->localPowerECBFault = pSpeclMiscSum->type1.localPowerECBFault;
        pPeMiscInfo->peerPowerECBFault = pSpeclMiscSum->type1.peerPowerECBFault;
    }
    else if (pSpeclMiscSum->type == 2) //Oberon
    {
        pPeMiscInfo->cnaMode = pSpeclMiscSum->type2.localCnaMode;
        if (pSpeclMiscSum->type2.internalCableStatePort1 == INTERNAL_CABLE_STATE_INSERTED)
        {
            pPeMiscInfo->fbeInternalCablePort1 = FBE_CABLE_STATUS_GOOD;
        }
        else if (pSpeclMiscSum->type2.internalCableStatePort1 == INTERNAL_CABLE_STATE_MISSING)
        {
            pPeMiscInfo->fbeInternalCablePort1 = FBE_CABLE_STATUS_MISSING;
        }
        else if (pSpeclMiscSum->type2.internalCableStatePort1 == INTERNAL_CABLE_STATE_CROSSED)
        {
            pPeMiscInfo->fbeInternalCablePort1 = FBE_CABLE_STATUS_CROSSED;
        }
    }
    else if (pSpeclMiscSum->type == 3) //Bearcat
    {
        pPeMiscInfo->cnaMode = pSpeclMiscSum->type3.localCnaMode;
    }

    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_board_translate_led_status
 **************************************************************************
 *  @brief
 *      This function translates the SPECL led status.
 *
 *  @param pPeMiscInfo- The pointer to fbe_board_mgmt_misc_info_t.
 *  @param pSpeclLedcSum - The pointer to SPECL_LED_SUMMARY.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    20-Apr-2010: PHE - Created.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_board_translate_led_status(fbe_board_mgmt_misc_info_t *pPeMiscInfo, 
                                    SPECL_LED_SUMMARY *pSpeclLedSum)
{
    pPeMiscInfo->SPFaultLED = pSpeclLedSum->SPFaultLED;
    pPeMiscInfo->UnsafeToRemoveLED = pSpeclLedSum->UnsafeToRemoveLED;
    pPeMiscInfo->EnclosureFaultLED = pSpeclLedSum->EnclosureFaultLED;
    pPeMiscInfo->SPFaultLEDColor = pSpeclLedSum->SPFaultLEDColor;
    pPeMiscInfo->ManagementPortLED = pSpeclLedSum->ManagementPortLED;
    pPeMiscInfo->ServicePortLED = pSpeclLedSum->ServicePortLED;

    return FBE_STATUS_OK;
}


/*!*************************************************************************
 *  @fn fbe_base_board_read_io_status
 **************************************************************************
 *  @brief
 *      This function gets the IO modules, IO ports and IO annexes status and 
 *      fills in EDAL with the status.
 *
 *  @param base_board- The pointer to the base board object.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    03-Mar-2010: PHE - Created.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_board_read_io_status(fbe_base_board_t * base_board)
{
   
    SPECL_IO_SUMMARY  * pSpeclIoSum = NULL;   
    fbe_status_t        status = FBE_STATUS_OK;

    pSpeclIoSum = (SPECL_IO_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclIoSum, sizeof(SPECL_IO_SUMMARY));

    status = fbe_base_board_pe_get_io_comp_status(pSpeclIoSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }
   
    status = fbe_base_board_read_io_module_status(base_board, pSpeclIoSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    status = fbe_base_board_read_bem_status(base_board, pSpeclIoSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }
    
    return FBE_STATUS_OK;
}// End of - fbe_base_board_read_io_status
  
/*!*************************************************************************
 *  @fn fbe_base_board_read_io_module_status
 **************************************************************************
 *  @brief
 *      This function gets all the IO modules and their IO ports status and 
 *      fills in EDAL with the status.
 *
 *  @param base_board- The pointer to the base board object.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    07-May-2010: PHE - Created.
 *
 **************************************************************************/
static fbe_status_t
fbe_base_board_read_io_module_status(fbe_base_board_t * pBaseBoard,
                                     SPECL_IO_SUMMARY * pSpeclIoSum)
{
    SP_ID               sp;
    fbe_u32_t           ioModule;
    fbe_u32_t           componentIndex;
    fbe_u32_t           componentCount = 0;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_edal_status_t   edalStatus = FBE_EDAL_STATUS_OK;

    SPECL_IO_STATUS               * pSpeclIoStatus = NULL;
    edal_pe_io_comp_sub_info_t      peIoModuleSubInfo = {0}; 
    edal_pe_io_comp_sub_info_t    * pPeIoModuleSubInfo = &peIoModuleSubInfo;  
    fbe_board_mgmt_io_comp_info_t * pExtIoModuleInfo = NULL;
    fbe_edal_block_handle_t         edalBlockHandle = NULL;   

    fbe_base_board_get_edal_block_handle(pBaseBoard, &edalBlockHandle);
    
    /* Get the total count of IO modules. */
    edalStatus = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                                                     FBE_ENCL_IO_MODULE, 
                                                     &componentCount);

    if(edalStatus != FBE_EDAL_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    componentIndex = 0;
    
    FBE_FOR_ALL_IO_MODULES(sp, ioModule, pSpeclIoSum)
    {
        pSpeclIoStatus = &(pSpeclIoSum->ioSummary[sp].ioStatus[ioModule]);

        if ((pSpeclIoStatus->moduleType != IO_MODULE_TYPE_SLIC_1_0) &&
            (pSpeclIoStatus->moduleType != IO_MODULE_TYPE_SLIC_2_0))
            continue;

        fbe_zero_memory(pPeIoModuleSubInfo, sizeof(edal_pe_io_comp_sub_info_t));        
        /* Get old IO module info from EDAL. */
        edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_IO_COMP_INFO,  //Attribute
                                        FBE_ENCL_IO_MODULE,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_io_comp_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeIoModuleSubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edalStatus))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }    

        pExtIoModuleInfo = &pPeIoModuleSubInfo->externalIoCompInfo;

        pExtIoModuleInfo->associatedSp = sp;
        pExtIoModuleInfo->slotNumOnBlade = ioModule;  
        pExtIoModuleInfo->deviceType = FBE_DEVICE_TYPE_IOMODULE;
        pExtIoModuleInfo->isLocalFru = (pBaseBoard->spid == sp) ? TRUE : FALSE;
        
        status = fbe_base_board_get_resume_prom_id(pExtIoModuleInfo->associatedSp, 
                                                   pExtIoModuleInfo->slotNumOnBlade, 
                                                   pExtIoModuleInfo->deviceType, 
                                                   &pPeIoModuleSubInfo->resumePromId);

        /* Handle the Stale data and SMB errors */
        status = fbe_base_board_check_env_interface_error(pBaseBoard,
                                                           FBE_ENCL_IO_MODULE,
                                                           pSpeclIoStatus->transactionStatus,
                                                           pSpeclIoStatus->timeStamp.QuadPart,
                                                           &pPeIoModuleSubInfo->timeStampForFirstBadStatus,
                                                           &pExtIoModuleInfo->envInterfaceStatus);

        if(status != FBE_STATUS_OK)
        {
            return status;  
        }           

        fbe_base_board_translate_io_status(pBaseBoard, pPeIoModuleSubInfo, pSpeclIoStatus);
    
        /* Update EDAL with new IO module info. */
        edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_IO_COMP_INFO,  //Attribute
                                        FBE_ENCL_IO_MODULE,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_io_comp_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeIoModuleSubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edalStatus))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }   

        /* Read all the IO ports info on this IO module. */
        status = fbe_base_board_read_io_port_status_on_io_comp(pBaseBoard,
                                                pPeIoModuleSubInfo,
                                                pSpeclIoStatus->ioControllerCount,
                                                &pSpeclIoStatus->ioControllerInfo[0]);

        if(status != FBE_STATUS_OK)
        {
            return status;
        }

        componentIndex++;

        if(componentIndex >= componentCount)
        {
            break;
        }
    }

    return FBE_STATUS_OK;
}// End of - fbe_base_board_read_io_module_status

/*!*************************************************************************
 *  @fn fbe_base_board_read_io_port_status_on_io_comp
 **************************************************************************
 *  @brief
 *      This function reads all the IO port info on the IO component 
 *      and save the IO port info to EDAL.
 *
 *  @param pPeIoCompSubInfo- The pointer to edal_pe_io_comp_sub_info_t.
 *  @param ioControllerCount - The ioControllerCount on the module.
 *  @param speclIoControllerInfo - The array name which saves the io constroller info.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    04-May-2010: PHE - Created.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_board_read_io_port_status_on_io_comp(fbe_base_board_t * pBaseBoard,
                                               edal_pe_io_comp_sub_info_t * pPeIoCompSubInfo,
                                               fbe_u32_t ioControllerCount,
                                               SPECL_IO_CONTROLLER_STATUS speclIoControllerInfo[])
{
    fbe_board_mgmt_io_comp_info_t * pExtIoCompInfo = NULL;
    edal_pe_io_port_sub_info_t    * peIoPortSubInfo;
    edal_pe_io_port_sub_info_t    * pPeIoPortSubInfo = NULL;
    fbe_board_mgmt_io_port_info_t * pExtIoPortInfo = NULL;
    fbe_u32_t ioPort = 0;  /* controller based io port number */
    fbe_u32_t ioController = 0; /* module based io controller number */
    fbe_u32_t componentIndex = 0;
    fbe_edal_block_handle_t edalBlockHandle = NULL;   
    fbe_status_t status = FBE_STATUS_OK;
    fbe_edal_status_t edalStatus = FBE_EDAL_STATUS_OK;
    fbe_u32_t portNumOnModule = 0;

    fbe_base_board_get_edal_block_handle(pBaseBoard, &edalBlockHandle);

    pExtIoCompInfo = &pPeIoCompSubInfo->externalIoCompInfo;

    if(!pExtIoCompInfo->isLocalFru)
    {
        /* We currently don't save any non-local IO ports. 
         * So no need to proceed further.
         */
        return FBE_STATUS_OK;
    }

    peIoPortSubInfo = (edal_pe_io_port_sub_info_t *)fbe_memory_ex_allocate(sizeof(edal_pe_io_port_sub_info_t) * MAX_IO_PORTS_PER_MODULE);
    if (peIoPortSubInfo == NULL) {
        fbe_base_object_trace((fbe_base_object_t *) pBaseBoard,
                    FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "%s: fail to allocate memory for pe port sub info.\n",
                    __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_zero_memory(&peIoPortSubInfo[0], MAX_IO_PORTS_PER_MODULE*sizeof(edal_pe_io_port_sub_info_t));

    /* Get old IO port info and init some of the attributes to default values. */
    for(portNumOnModule = 0; portNumOnModule < MAX_IO_PORTS_PER_MODULE; portNumOnModule ++)
    {
        pPeIoPortSubInfo = &peIoPortSubInfo[portNumOnModule];
        pExtIoPortInfo = &pPeIoPortSubInfo->externalIoPortInfo;

        /* Get the EDAL component index for the IO port. */
        status = fbe_base_board_get_io_port_component_index(pBaseBoard, 
                                                            pExtIoCompInfo->associatedSp,
                                                            pExtIoCompInfo->slotNumOnBlade, 
                                                            portNumOnModule,
                                                            pExtIoCompInfo->deviceType,
                                                            &componentIndex);
        if(status != FBE_STATUS_OK)
        {
            fbe_memory_ex_release(peIoPortSubInfo);
            return status;  
        }  
            
        /* Get the old IO port info. */
        edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_IO_PORT_INFO,  //Attribute
                                        FBE_ENCL_IO_PORT,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_io_port_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeIoPortSubInfo);  // buffer pointer  

        if(!EDAL_STAT_OK(edalStatus))
        {
            fbe_memory_ex_release(peIoPortSubInfo);
            return FBE_STATUS_GENERIC_FAILURE;  
        }      
        
        /* Init some attributes of the IO port to default values. */       
        /* Populate the static attributes for the IO port. */
        pExtIoPortInfo->associatedSp = pExtIoCompInfo->associatedSp;
        pExtIoPortInfo->slotNumOnBlade = pExtIoCompInfo->slotNumOnBlade;
        pExtIoPortInfo->portNumOnModule = portNumOnModule; 

        pExtIoPortInfo->deviceType = pExtIoCompInfo->deviceType;
        pExtIoPortInfo->isLocalFru = pExtIoCompInfo->isLocalFru;
        
        /* Init these attributes to default values. It will be updated accordingly.*/
        pExtIoPortInfo->present = FBE_MGMT_STATUS_NA;
        pExtIoPortInfo->powerStatus = FBE_POWER_STATUS_NA;
        pExtIoPortInfo->protocol = IO_CONTROLLER_PROTOCOL_UNKNOWN; 
        pExtIoPortInfo->supportedSpeeds = SPEED_UNKNOWN; 
    }
      
    portNumOnModule = 0; 
    /* Update some attributes based on the speclIoControllerInfo. */
    for(ioController = 0; ioController < ioControllerCount; ioController++)
    {        
        for(ioPort = 0; ioPort < speclIoControllerInfo[ioController].ioPortCount; ioPort ++)
        {
            fbe_base_board_translate_io_port_status(&peIoPortSubInfo[portNumOnModule],
                                                pPeIoCompSubInfo, 
                                                &speclIoControllerInfo[ioController],
                                                &speclIoControllerInfo[ioController].ioPortInfo[ioPort]);
            peIoPortSubInfo[portNumOnModule].externalIoPortInfo.portal_number = ioPort;

            portNumOnModule ++;
        }
    }    

    /* Update derived info based on uniqueID. */
    if((pExtIoCompInfo->ioPortCount == 0) &&
       (pExtIoCompInfo->powerStatus == FBE_POWER_STATUS_POWER_OFF) &&
       (pExtIoCompInfo->uniqueID != NO_MODULE)) 
    {
         for(portNumOnModule = 0; portNumOnModule < MAX_IO_PORTS_PER_MODULE; portNumOnModule ++)
         {
             fbe_base_board_translate_io_port_status(&peIoPortSubInfo[portNumOnModule],
                                            pPeIoCompSubInfo, 
                                            NULL,
                                            NULL);    
         }
    }

    /* Finally update EDAL with the new IO Port info. */
    for(portNumOnModule = 0; portNumOnModule < MAX_IO_PORTS_PER_MODULE; portNumOnModule ++)
    {
        /* Get the EDAL component index for the IO port. */
        status = fbe_base_board_get_io_port_component_index(pBaseBoard, 
                                                            pExtIoCompInfo->associatedSp,
                                                            pExtIoCompInfo->slotNumOnBlade, 
                                                            portNumOnModule,
                                                            pExtIoCompInfo->deviceType,
                                                            &componentIndex);
        if(status != FBE_STATUS_OK)
        {
            fbe_memory_ex_release(peIoPortSubInfo);
            return status;  
        } 

        /* Update IO Port info with the possible change. */
        edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                    FBE_ENCL_PE_IO_PORT_INFO,  //Attribute
                                    FBE_ENCL_IO_PORT,  // component type
                                    componentIndex,  // component index
                                    sizeof(edal_pe_io_port_sub_info_t),  // buffer length
                                    (fbe_u8_t *)&peIoPortSubInfo[portNumOnModule]);  // buffer pointer    

        if(!EDAL_STAT_OK(edalStatus))
        {
            fbe_memory_ex_release(peIoPortSubInfo);
            return FBE_STATUS_GENERIC_FAILURE;  
        }          
    }

    fbe_memory_ex_release(peIoPortSubInfo);
    return FBE_STATUS_OK;
} // End of - fbe_base_board_read_io_port_status_on_io_comp


/*!*************************************************************************
 *  @fn fbe_base_board_get_io_port_component_index
 **************************************************************************
 *  @brief
 *      This function gets the IO port EDAL component index.
 *
 *  @param pBaseBoard- The pointer to the base board object.
 *  @param associatedSp-  The SP that the IO port is associated with.
 *  @param slotNumOnBlade - The slot number of the IO component that the IO port resides on.

 *  @param portNumOnModule - The port number on the module.
 *  @param deviceType - The device type.
 *  @param pComponentIndex- The pointer to the returned EDAL component index for the IO port.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    07-May-2010: PHE - Created.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_board_get_io_port_component_index(fbe_base_board_t * pBaseBoard,
                                           SP_ID associatedSp,
                                           fbe_u32_t slotNumOnBlade,
                                           fbe_u32_t portNumOnModule,
                                           fbe_u64_t deviceType, 
                                           fbe_u32_t * pComponentIndex)
{
    *pComponentIndex = 0;
       
    if(associatedSp == pBaseBoard->spid)
    {
        if(deviceType == FBE_DEVICE_TYPE_MEZZANINE )
        {
            if((slotNumOnBlade >= pBaseBoard->localAltIoDevCount) ||
               (portNumOnModule >= MAX_IO_PORTS_PER_MODULE))
            {
                fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, Mezzanine port, invalid slotNumOnBlade %d or portNumOnModule %d.\n",
                                __FUNCTION__, slotNumOnBlade, portNumOnModule);

                return FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                *pComponentIndex = slotNumOnBlade * MAX_IO_PORTS_PER_MODULE + portNumOnModule;
            }
        }
        else if(deviceType == FBE_DEVICE_TYPE_BACK_END_MODULE )
        {
            if((slotNumOnBlade >= pBaseBoard->localAltIoDevCount) ||
               (portNumOnModule >= MAX_IO_PORTS_PER_MODULE))
            {
                fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, BEM port, invalid slotNumOnBlade %d or portNumOnModule %d.\n",
                                __FUNCTION__, slotNumOnBlade, portNumOnModule);

                return FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                *pComponentIndex = slotNumOnBlade * MAX_IO_PORTS_PER_MODULE + portNumOnModule;
            }
        }
        else if(deviceType == FBE_DEVICE_TYPE_IOMODULE)
        {
            if((slotNumOnBlade >= (pBaseBoard->localIoModuleCount - pBaseBoard->localAltIoDevCount)) ||
               (portNumOnModule >= MAX_IO_PORTS_PER_MODULE))
            {
                fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, IO module port, invalid slotNumOnBlade %d or portNumOnModule %d.\n",
                                __FUNCTION__, slotNumOnBlade, portNumOnModule);

                return FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                //*pComponentIndex = slotNumOnBlade * MAX_IO_PORTS_PER_MODULE + portNumOnModule;
                //Hack to allow NDU of changes to single array of IO devices in SPECL
                *pComponentIndex = (slotNumOnBlade + pBaseBoard->localAltIoDevCount) * MAX_IO_PORTS_PER_MODULE + portNumOnModule;
            }
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, invalid deviceType %lld.\n",
                                __FUNCTION__,
                                deviceType);

            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, invalid associatedSp %d, expected %d.\n",
                                __FUNCTION__,
                                associatedSp, pBaseBoard->spid);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    return FBE_STATUS_OK;
} // End of - fbe_base_board_get_io_port_component_index


/*!*************************************************************************
 *  @fn fbe_base_board_read_bem_status
 **************************************************************************
 *  @brief
 *      This function gets all the BEMs status and 
 *      fills in EDAL with the status.
 *
 *  @param base_board- The pointer to the base board object.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    07-May-2010: PHE - Created.
 *
 **************************************************************************/
static fbe_status_t
fbe_base_board_read_bem_status(fbe_base_board_t * pBaseBoard,
                                     SPECL_IO_SUMMARY * pSpeclIoSum)
{
    SP_ID               sp;
    fbe_u32_t           bem;
    fbe_u32_t           bemIndex;
    fbe_u32_t           componentIndex;
    fbe_u32_t           componentCount = 0;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_edal_status_t   edal_status = FBE_EDAL_STATUS_OK;

    SPECL_IO_STATUS               * pSpeclIoStatus = NULL;
    edal_pe_io_comp_sub_info_t      peBemSubInfo = {0}; 
    edal_pe_io_comp_sub_info_t    * pPeBemSubInfo = &peBemSubInfo;
    fbe_board_mgmt_io_comp_info_t * pExtBemInfo = NULL;
    fbe_edal_block_handle_t         edalBlockHandle = NULL;   


    fbe_base_board_get_edal_block_handle(pBaseBoard, &edalBlockHandle);
    
    /* Get the total count of IO annexes. */
    edal_status = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                                                     FBE_ENCL_BEM, 
                                                     &componentCount);

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    componentIndex = 0;

    for(sp = 0; sp < pSpeclIoSum->bladeCount; sp++)
    {
      bemIndex = 0;
      for(bem = 0; bem < pSpeclIoSum->ioSummary[sp].ioModuleCount; bem++)
      {

        pSpeclIoStatus = &(pSpeclIoSum->ioSummary[sp].ioStatus[bem]);

        if (pSpeclIoStatus->moduleType != IO_MODULE_TYPE_BEM)
            continue;

        fbe_zero_memory(pPeBemSubInfo, sizeof(edal_pe_io_comp_sub_info_t));
        /* Get old IO Annex info from EDAL. */
        edal_status = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_IO_COMP_INFO,  //Attribute
                                        FBE_ENCL_BEM,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_io_comp_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeBemSubInfo);  // buffer pointer 

        if(!EDAL_STAT_OK(edal_status))
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }

        pExtBemInfo = &pPeBemSubInfo->externalIoCompInfo;

        pExtBemInfo->associatedSp = sp;
        //pExtBemInfo->slotNumOnBlade = bem;
        //Hack to allow NDU of changes to single array of IO devices in SPECL
        pExtBemInfo->slotNumOnBlade = bemIndex;
        pExtBemInfo->deviceType = FBE_DEVICE_TYPE_BACK_END_MODULE;
        pExtBemInfo->isLocalFru = (pBaseBoard->spid == sp) ? TRUE : FALSE;

        /* Handle the Stale data and SMB errors */
        status = fbe_base_board_check_env_interface_error(pBaseBoard, 
                                                           FBE_ENCL_BEM,
                                                           pSpeclIoStatus->transactionStatus,
                                                           pSpeclIoStatus->timeStamp.QuadPart,
                                                           &pPeBemSubInfo->timeStampForFirstBadStatus,
                                                           &pExtBemInfo->envInterfaceStatus);

        if(status != FBE_STATUS_OK)
        {
            return status;  
        }           

        fbe_base_board_translate_io_status(pBaseBoard, pPeBemSubInfo, pSpeclIoStatus);
    
        /* Update EDAL with new BEM info. */
        edal_status = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_IO_COMP_INFO,  //Attribute
                                        FBE_ENCL_BEM,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_io_comp_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeBemSubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edal_status))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }
        
         /* Read all the IO ports info on the Mezzanine. */
        status = fbe_base_board_read_io_port_status_on_io_comp(pBaseBoard,
                                                pPeBemSubInfo,
                                                pSpeclIoStatus->ioControllerCount,
                                                &pSpeclIoStatus->ioControllerInfo[0]);

        if(status != FBE_STATUS_OK)
        {
            return status;
        }           

        componentIndex++;
        bemIndex++;

        if(componentIndex >= componentCount)
        {
            break;
        }
      }
      if(componentIndex >= componentCount)
      {
          break;
      }
    }

    return FBE_STATUS_OK;
}// End of - fbe_base_board_read_bem_status


/*!*************************************************************************
 *  @fn fbe_base_board_translate_io_status
 **************************************************************************
 *  @brief
 *      This function translates the SPECL_IO_STATUS to edal_pe_io_comp_sub_info_t.
 *
 *  @param pBaseBoard - The pointer to fbe_base_board_t.
 *  @param pPeIoCompInfo- The pointer to edal_pe_io_comp_sub_info_t.
 *  @param pSpeclIoStatus - The pointer to SPECL_IO_STATUS.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    03-Mar-2010: PHE - Created.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_board_translate_io_status(fbe_base_board_t * pBaseBoard,
                                   edal_pe_io_comp_sub_info_t * pPeIoCompSubInfo,
                                   SPECL_IO_STATUS  * pSpeclIoStatus)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_board_mgmt_io_comp_info_t * pExtPeIoCompInfo = NULL;
    fbe_u32_t ioController = 0;

    pExtPeIoCompInfo = &pPeIoCompSubInfo->externalIoCompInfo;   

    if(pExtPeIoCompInfo->deviceType == FBE_DEVICE_TYPE_IOMODULE)
    {
        pExtPeIoCompInfo->location = FBE_IO_MODULE_LOC_ONBOARD_SLOT0 + pPeIoCompSubInfo->externalIoCompInfo.slotNumOnBlade;
    }
    else
    {
        pExtPeIoCompInfo->location = FBE_IO_MODULE_LOC_NA;
    }

    if(pSpeclIoStatus->transactionStatus == EMCPAL_STATUS_SUCCESS)
    {
        pExtPeIoCompInfo->transactionStatus = pSpeclIoStatus->transactionStatus;
        /* Set the Unique ID 
         * The Unique ID needs to be set before setting the insert bit.
         * Because we would check the Unique ID to decide whether the insert bit should be masked or not.
         */
        status = fbe_base_board_set_io_uniqueID(pPeIoCompSubInfo, pSpeclIoStatus);

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pBaseBoard, 
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                  "%s, Failed to set UniqueID for Component: %lld, Slot: %d, SP: %d.\n", 
                                  __FUNCTION__, 
                                  pPeIoCompSubInfo->externalIoCompInfo.deviceType,
                                  pPeIoCompSubInfo->externalIoCompInfo.slotNumOnBlade, 
                                  pPeIoCompSubInfo->externalIoCompInfo.associatedSp);
            return status;  
        }           
    
        /* Set the insert bit 
         * The inserted-ness needs to be set after the UniqueID.
         * Because we would check the Unique ID to decide whether the insert bit should be masked or not.
         */
        status = fbe_base_board_set_io_insert_bit(pBaseBoard, pPeIoCompSubInfo, pSpeclIoStatus);

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pBaseBoard, 
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                  "%s, Failed to set Insert bit for Component: %lld, Slot: %d, SP: %d.\n", 
                                  __FUNCTION__, 
                                  pPeIoCompSubInfo->externalIoCompInfo.deviceType,
                                  pPeIoCompSubInfo->externalIoCompInfo.slotNumOnBlade, 
                                  pPeIoCompSubInfo->externalIoCompInfo.associatedSp);
            return status;  
        }  

        if (pSpeclIoStatus->type == 1)
        {
            pExtPeIoCompInfo->faultLEDStatus = pSpeclIoStatus->type1.faultLEDStatus;
        }

        /* Translate powerStatus. */
        if((pSpeclIoStatus->type == 1) &&
           pSpeclIoStatus->type1.powerGood)
        {
            pExtPeIoCompInfo->powerStatus = FBE_POWER_STATUS_POWER_ON;
        }
        else if((pSpeclIoStatus->type == 1) &&
                pSpeclIoStatus->type1.powerUpFailure)
        {
            pExtPeIoCompInfo->powerStatus = FBE_POWER_STATUS_POWERUP_FAILED;
        }
        else
        {
            if(pSpeclIoStatus->inserted)
            {
                pExtPeIoCompInfo->powerStatus = FBE_POWER_STATUS_POWER_OFF;
            }
            else 
            {
                pExtPeIoCompInfo->powerStatus = FBE_POWER_STATUS_NA;
            }
        }
        if (pSpeclIoStatus->type == 1)
        {
            pExtPeIoCompInfo->powerEnabled = pSpeclIoStatus->type1.persistedPowerState;
        }

        /* Zero out ioPortCount and then add up all the ports for all controllers for this. */
        pExtPeIoCompInfo->ioPortCount = 0;
        for(ioController = 0; ioController < pSpeclIoStatus->ioControllerCount; ioController++)
        {
            pExtPeIoCompInfo->ioPortCount +=  pSpeclIoStatus->ioControllerInfo[ioController].ioPortCount;
        }            
    }
    else if(pSpeclIoStatus->transactionStatus == STATUS_SMB_DEVICE_NOT_PRESENT)
    {
        pExtPeIoCompInfo->transactionStatus = pSpeclIoStatus->transactionStatus;
        /* This is possible scenario when for ex: The IO Annex is removed along with 
        * its Transaction initiator(T-MCU) to IOM 4 & 5. As T-MCU is not present 
        * SPECL will send SMB_DEVICE_NOT_PRESENT. Data in other fields returned is
        * unreliable. DIMS 193162
        */
        pExtPeIoCompInfo->inserted = FBE_MGMT_STATUS_FALSE;
        pExtPeIoCompInfo->uniqueID = NO_MODULE;
        pExtPeIoCompInfo->faultLEDStatus = LED_BLINK_INVALID;
        pExtPeIoCompInfo->powerStatus = FBE_POWER_STATUS_NA;
        pExtPeIoCompInfo->ioPortCount = 0;
    }
    else if(pExtPeIoCompInfo->envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
    {
        pExtPeIoCompInfo->transactionStatus = pSpeclIoStatus->transactionStatus;
        /* env interface is no good. We are not able to get any status and the old values are also timed out.
        */
        pExtPeIoCompInfo->inserted = FBE_MGMT_STATUS_UNKNOWN;
        pExtPeIoCompInfo->uniqueID = NO_MODULE;
        pExtPeIoCompInfo->faultLEDStatus = LED_BLINK_INVALID;
        pExtPeIoCompInfo->powerStatus = FBE_POWER_STATUS_UNKNOWN; 
        pExtPeIoCompInfo->ioPortCount = 0;
    }
    else
    {
        /* envInterfaceStatus is FBE_ENV_INTERFACE_STATUS_GOOD.  
        * We can still continue using the old values.
        * So no need to update anything here.
        */
    }

    if((pSpeclIoStatus->uniqueID != NO_MODULE) && 
       (pSpeclIoStatus->transactionStatus != EMCPAL_STATUS_SUCCESS))
    { 
        /* transactionStatus failed. It means the inserted field
         * was not valid. But we got the IO module uniqueID as valid, 
         * we consider the IO module is inserted.
         */
        pExtPeIoCompInfo->inserted = FBE_MGMT_STATUS_TRUE;
        pExtPeIoCompInfo->uniqueID = pSpeclIoStatus->uniqueID;
        /* Zero out ioPortCount and then add up all the ports for all controllers for this. */
        pExtPeIoCompInfo->ioPortCount = 0;
        for(ioController = 0; ioController < pSpeclIoStatus->ioControllerCount; ioController++)
        {
            pExtPeIoCompInfo->ioPortCount +=  pSpeclIoStatus->ioControllerInfo[ioController].ioPortCount;
        }     
    } 

    if(pExtPeIoCompInfo->inserted == FBE_MGMT_STATUS_FALSE)
    {
        pExtPeIoCompInfo->faultLEDStatus = LED_BLINK_INVALID;
    }

    
    /* Save the new SpeclIoStatus as old Specl status */
    pPeIoCompSubInfo->uniqueID = pSpeclIoStatus->uniqueID;
    pPeIoCompSubInfo->inserted = pSpeclIoStatus->inserted;

    return FBE_STATUS_OK;
}// End of - fbe_base_board_translate_io_status


/*!*************************************************************************
 *  @fn fbe_base_board_translate_io_port_status
 **************************************************************************
 *  @brief
 *      This function translates the individual IO port info.
 *
 *  @param pPeIoPortSubInfo- The pointer to the individual IO port sub info.
 *  @param pPeIoCompSubInfo- The pointer to the sub info of the IO component that the IO port resides on.
 *  @param pSpeclIoControllerInfo - The pointer to the specl data of the IO controller that the IO port resides on.
 *  @param pSpeclIoPortData - The pointer to the specl data of the IO port IoPortData.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    04-May-2010: PHE - Created.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_board_translate_io_port_status(edal_pe_io_port_sub_info_t * pPeIoPortSubInfo,
                                      edal_pe_io_comp_sub_info_t * pPeIoCompSubInfo,
                                      SPECL_IO_CONTROLLER_STATUS * pSpeclIoControllerInfo,
                                      SPECL_IO_PORT_DATA * pSpeclIoPortData)
{
    fbe_board_mgmt_io_comp_info_t * pExtIoCompInfo = NULL;
    fbe_board_mgmt_io_port_info_t * pExtIoPortInfo = NULL;
    
    pExtIoCompInfo = &pPeIoCompSubInfo->externalIoCompInfo;
    pExtIoPortInfo = &pPeIoPortSubInfo->externalIoPortInfo;
    
    /* Update the default values to the real values. */
    pExtIoPortInfo->present     = pExtIoCompInfo->inserted;
    pExtIoPortInfo->powerStatus = pExtIoCompInfo->powerStatus;

    if((pSpeclIoControllerInfo != NULL) && 
       (pSpeclIoPortData != NULL))
    {
        pExtIoPortInfo->controller_protocol = pSpeclIoControllerInfo->protocol;
        pExtIoPortInfo->protocol            = pSpeclIoPortData->portPciData[0].protocol;
        pExtIoPortInfo->supportedSpeeds     = pSpeclIoControllerInfo->availableControllerLinkSpeeds;
        pExtIoPortInfo->boot_device         = pSpeclIoPortData->boot_device;
        pExtIoPortInfo->SFPcapable          = pSpeclIoPortData->SFPcapable;
        // PHYmapping from HardwareAttributesLib contains PHYPolarity bits in the upper 16bits - need to mask them out.
        pExtIoPortInfo->phyMapping          = (pSpeclIoPortData->PHYmapping & PHY_MAP_MASK);
        pExtIoPortInfo->pciDataCount        = pSpeclIoPortData->pciDataCount;
        fbe_copy_memory(&pExtIoPortInfo->pciData,   &pSpeclIoPortData->portPciData[0].pciData,  sizeof(SPECL_PCI_DATA)* PCI_MAX_FUNCTION);
        fbe_copy_memory(&pExtIoPortInfo->portPciData, &pSpeclIoPortData->portPciData, sizeof(SPECL_IO_PORT_PCI_DATA) * PCI_MAX_FUNCTION);
        fbe_copy_memory(&pExtIoPortInfo->sfpStatus, &pSpeclIoPortData->sfpStatus,               sizeof(SPECL_SFP_STATUS));
    }
    
    return FBE_STATUS_OK;   
}// End of - fbe_base_board_translate_io_port_status


/*!*************************************************************************
 *  @fn fbe_base_board_read_mezzanine_status
 **************************************************************************
 *  @brief
 *      This function gets the Mezzanine status and 
 *      fills in EDAL with the status.
 *
 *  @param base_board- The pointer to the base board object.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    03-Mar-2010: PHE - Created.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_board_read_mezzanine_status(fbe_base_board_t * pBaseBoard)
{
    SP_ID               sp;
    fbe_u32_t           mezzanine;
    fbe_u32_t           mezzanineIndex;
    fbe_u32_t           componentIndex;
    fbe_u32_t           componentCount = 0;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_edal_status_t   edalStatus = FBE_EDAL_STATUS_OK;

    SPECL_IO_SUMMARY              * pSpeclIoSum = NULL;
    SPECL_IO_STATUS               * pSpeclIoStatus = NULL;
    edal_pe_io_comp_sub_info_t      peMezzanineSubInfo = {0};        
    edal_pe_io_comp_sub_info_t    * pPeMezzanineSubInfo = &peMezzanineSubInfo;
    fbe_board_mgmt_io_comp_info_t * pExtMezzanineInfo = NULL;
    fbe_edal_block_handle_t         edalBlockHandle = NULL;   

    fbe_base_board_get_edal_block_handle(pBaseBoard, &edalBlockHandle);
    
    pSpeclIoSum = (SPECL_IO_SUMMARY *)pBaseBoard->pe_buffer_handle;
    fbe_zero_memory(pSpeclIoSum, sizeof(SPECL_IO_SUMMARY));

    status = fbe_base_board_pe_get_io_comp_status(pSpeclIoSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    /* Get the total count of Mezzanines. */
    edalStatus = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                                                     FBE_ENCL_MEZZANINE, 
                                                     &componentCount);

    if(edalStatus != FBE_EDAL_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    componentIndex = 0;

    for(sp = 0; sp < pSpeclIoSum->bladeCount; sp++)
    {
      mezzanineIndex = 0;
      for(mezzanine = 0; mezzanine < pSpeclIoSum->ioSummary[sp].ioModuleCount; mezzanine++)
      {
        pSpeclIoStatus = &(pSpeclIoSum->ioSummary[sp].ioStatus[mezzanine]);

        if ((pSpeclIoStatus->moduleType != IO_MODULE_TYPE_MEZZANINE) &&
            (pSpeclIoStatus->moduleType != IO_MODULE_TYPE_ONBOARD))
            continue;

        fbe_zero_memory(pPeMezzanineSubInfo, sizeof(edal_pe_io_comp_sub_info_t)); 

        /* Get old IO mezzanine info from EDAL. */
        edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_IO_COMP_INFO,  //Attribute
                                        FBE_ENCL_MEZZANINE,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_io_comp_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeMezzanineSubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edalStatus))
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }

        pExtMezzanineInfo = &pPeMezzanineSubInfo->externalIoCompInfo;

        pExtMezzanineInfo->associatedSp = sp;
        //pExtMezzanineInfo->slotNumOnBlade = mezzanine;
        //Hack to allow NDU of changes to single array of IO devices in SPECL
        pExtMezzanineInfo->slotNumOnBlade = mezzanineIndex;
        pExtMezzanineInfo->deviceType = FBE_DEVICE_TYPE_MEZZANINE;
        pExtMezzanineInfo->isLocalFru = (pBaseBoard->spid == sp) ? TRUE : FALSE;

        /* Handle the Stale data and SMB errors */
        status = fbe_base_board_check_env_interface_error(pBaseBoard,
                                                           FBE_ENCL_MEZZANINE,
                                                           pSpeclIoStatus->transactionStatus,
                                                           pSpeclIoStatus->timeStamp.QuadPart,
                                                           &pPeMezzanineSubInfo->timeStampForFirstBadStatus,
                                                           &pExtMezzanineInfo->envInterfaceStatus);

        if(status != FBE_STATUS_OK)
        {
            return status;  
        }           

        fbe_base_board_translate_mezzanine_status(pPeMezzanineSubInfo, pSpeclIoStatus);  
    
        /* Update EDAL with new Mezzanine info. */
        edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_IO_COMP_INFO,  //Attribute
                                        FBE_ENCL_MEZZANINE,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_io_comp_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeMezzanineSubInfo);  // buffer pointer        
        
        if(!EDAL_STAT_OK(edalStatus))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }  

         /* Read all the IO ports info on the Mezzanine. */
        status = fbe_base_board_read_io_port_status_on_io_comp(pBaseBoard,
                                                pPeMezzanineSubInfo,
                                                pSpeclIoStatus->ioControllerCount,
                                                &pSpeclIoStatus->ioControllerInfo[0]);

        if(status != FBE_STATUS_OK)
        {
            return status;
        }

        componentIndex++;

        if(componentIndex >= componentCount)
        {
            break;
        }
      }
      if(componentIndex >= componentCount)
      {
          break;
      }
    }

    return FBE_STATUS_OK;
}


/*!*************************************************************************
 *  @fn fbe_base_board_read_temperature_status
 **************************************************************************
 *  @brief
 *      This function gets the Temperature status and 
 *      fills in EDAL with the status.
 *
 *  @param base_board- The pointer to the base board object.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    29-Dec-2010:  Joe Perry - Created.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_board_read_temperature_status(fbe_base_board_t * pBaseBoard)
{
    fbe_u32_t           tempIndex, tempCount;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_edal_status_t   edalStatus = FBE_EDAL_STATUS_OK;

    SPECL_TEMPERATURE_SUMMARY       * pSpeclTempSum = NULL;
/*    SPECL_TEMPERATURE_STATUS        * pSpeclTempStatus = NULL; */
    edal_pe_temperature_sub_info_t  peTempSubInfo;        
    edal_pe_temperature_sub_info_t  *pPeTempSubInfo = &peTempSubInfo;
    fbe_eir_temp_sample_t           * pExtTempInfo = NULL;
    fbe_edal_block_handle_t         edalBlockHandle = NULL;   

    fbe_base_board_get_edal_block_handle(pBaseBoard, &edalBlockHandle);
    
    pSpeclTempSum = (SPECL_TEMPERATURE_SUMMARY *)pBaseBoard->pe_buffer_handle;
    fbe_zero_memory(pSpeclTempSum, sizeof(SPECL_TEMPERATURE_SUMMARY));

    status = fbe_base_board_pe_get_temperature_status(pSpeclTempSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    // Get the total count of Temperature components
    edalStatus = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                                                     FBE_ENCL_TEMPERATURE, 
                                                     &tempCount);

    if(edalStatus != FBE_EDAL_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    /*
     * Loop through all the Temperature components
     */
    for (tempIndex = 0; tempIndex < tempCount; tempIndex++)
    {
        pSpeclTempSum = (SPECL_TEMPERATURE_SUMMARY *)pBaseBoard->pe_buffer_handle;
        fbe_zero_memory(pSpeclTempSum, sizeof(SPECL_TEMPERATURE_SUMMARY));
    
        status = fbe_base_board_pe_get_temperature_status(pSpeclTempSum);
        if(status != FBE_STATUS_OK)
        {
            return status;  
        }
        
//        fbe_zero_memory(pPeTempInfo, sizeof(edal_pe_temperature_info_s)); 
    
        /* Get old IO mezzanine info from EDAL. */
        edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_TEMPERATURE_INFO,       //Attribute
                                        FBE_ENCL_TEMPERATURE,               // component type
                                        tempIndex,                          // component index
                                        sizeof(edal_pe_temperature_sub_info_t), // buffer length
                                        (fbe_u8_t *)pPeTempSubInfo);        // buffer pointer
        if(!EDAL_STAT_OK(edalStatus))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }  
    
        pExtTempInfo = &pPeTempSubInfo->externalTempInfo;
    
    
        /* Handle the Stale data and SMB errors */
/*
        status = fbe_base_board_check_env_interface_error(pBaseBoard,
                                                          FBE_ENCL_TEMPERATURE,
                                                          pSpeclTempStatus->transactionStatus,
                                                          pSpeclTempStatus->timeStamp.QuadPart,
                                                          &pPeTempSubInfo->timeStampForFirstBadStatus,
                                                          &pExtTempInfo->>envInterfaceStatus);
        if(status != FBE_STATUS_OK)
        {
            return status;  
        }           
*/
    
        status = fbe_base_board_translate_temperature_status(pPeTempSubInfo, 
                                                                 &(pSpeclTempSum->temperatureStatus[tempIndex]));
        if(status != FBE_STATUS_OK)
        {
            return status;
        }
        
        /* Update EDAL with new Mezzanine info. */
        edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_TEMPERATURE_INFO,           //Attribute
                                        FBE_ENCL_TEMPERATURE,                   // component type
                                        tempIndex,                              // component index
                                        sizeof(edal_pe_temperature_sub_info_t), // buffer length
                                        (fbe_u8_t *)pPeTempSubInfo);            // buffer pointer        
        if(!EDAL_STAT_OK(edalStatus))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }  
    
    }
    
    return FBE_STATUS_OK;
}   // end of fbe_base_board_read_temperature_status



/*!*************************************************************************
 *  @fn fbe_base_board_translate_temperature_status
 **************************************************************************
 *  @brief
 *      This function translates SPECL power supply status to physical package power supply info. 
 *
 *  @param base_board- The pointer to base board object.
 *  @param pPePowerSupplyInfo- The pointer to edal_pe_power_supply_sub_info_t. 
 *  @param pSpeclPowerSupplyStatus - The pointer to SPECL_PS_SUMMARY.
 *  @param pSpeclPowerSupplyStatus - The pointer to SPECL_PS_STATUS.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    03-Mar-2010: PHE - Created.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_board_translate_temperature_status(edal_pe_temperature_sub_info_t *pPeTempSubInfo,
                                            SPECL_TEMPERATURE_STATUS *pSpeclTempStatus)
{
    fbe_u16_t                   temperature = 0;

    // Initialize to 0.
    memset(pPeTempSubInfo, 0, sizeof(edal_pe_temperature_sub_info_t));

    /*
     * Check to see if Temperature is present on this platform
     */
    pPeTempSubInfo->transactionStatus = pSpeclTempStatus->transactionStatus;
    if (pSpeclTempStatus->transactionStatus == EMCPAL_STATUS_SUCCESS)
    {
    
        // convert Temperature value
        temperature = pSpeclTempStatus->TemperatureMSB * 10;
        if (pSpeclTempStatus->TemperatureLSB & BIT7)
        {
            temperature += 5;
        }
        pPeTempSubInfo->externalTempInfo.airInletTemp = temperature;
        pPeTempSubInfo->externalTempInfo.status = FBE_ENERGY_STAT_VALID;
    
    }
    else
    {
        pPeTempSubInfo->externalTempInfo.airInletTemp = 0;
        pPeTempSubInfo->externalTempInfo.status = FBE_ENERGY_STAT_FAILED;
    }
             
    return FBE_STATUS_OK;

}   // end of fbe_base_board_translate_temperature_status

/*!*************************************************************************
 *  @fn fbe_base_board_translate_mezzanine_status
 **************************************************************************
 *  @brief
 *      This function translates SPECL_IO_STATUS to edal_pe_io_comp_sub_info_t. 
 *
 *  @param pPeIoCompSubInfo- The pointer to edal_pe_io_comp_sub_info_t.
 *  @param pSpeclMezzanineStatus - The pointer to SPECL_IO_STATUS.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    03-Mar-2010: PHE - Created.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_board_translate_mezzanine_status(edal_pe_io_comp_sub_info_t * pPeIoCompSubInfo,                                      
                                          SPECL_IO_STATUS  * pSpeclMezzanineStatus)
{
    fbe_board_mgmt_io_comp_info_t * pExtPeIoCompInfo = NULL;
    fbe_u32_t ioController = 0;

    pExtPeIoCompInfo = &pPeIoCompSubInfo->externalIoCompInfo; 

    pExtPeIoCompInfo->uniqueID = pSpeclMezzanineStatus->uniqueID;    
   
    /* location is for IO module only. */
    pExtPeIoCompInfo->location = FBE_IO_MODULE_LOC_NA;

    /* Mezzanine does not have powerStatus. */
    pExtPeIoCompInfo->powerStatus = FBE_POWER_STATUS_NA;

    /* Mezzanine does not have faultLEDStatus. */
    pExtPeIoCompInfo->faultLEDStatus = LED_BLINK_INVALID;

    if(pSpeclMezzanineStatus->transactionStatus == EMCPAL_STATUS_SUCCESS)
    {
        pExtPeIoCompInfo->transactionStatus = pSpeclMezzanineStatus->transactionStatus;

        pExtPeIoCompInfo->inserted = (pSpeclMezzanineStatus->inserted == TRUE) ? 
                                        FBE_MGMT_STATUS_TRUE : FBE_MGMT_STATUS_FALSE;
        pExtPeIoCompInfo->uniqueID = pSpeclMezzanineStatus->uniqueID;

        pExtPeIoCompInfo->ioPortCount = 0;
        for(ioController = 0; ioController < pSpeclMezzanineStatus->ioControllerCount; ioController++)
        {
            pExtPeIoCompInfo->ioPortCount +=  pSpeclMezzanineStatus->ioControllerInfo[ioController].ioPortCount;
        }
    }
    else if(pExtPeIoCompInfo->envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
    {
        pExtPeIoCompInfo->transactionStatus = pSpeclMezzanineStatus->transactionStatus;
        /* env interface is no good. We are not able to get any status and the old values are also timed out.
         */
        pExtPeIoCompInfo->inserted = FBE_MGMT_STATUS_UNKNOWN;
        pExtPeIoCompInfo->uniqueID = NO_MODULE;
        pExtPeIoCompInfo->ioPortCount = 0;
    }
    else
    {
        /* envInterfaceStatus is still FBE_ENV_INTERFACE_STATUS_GOOD. 
        * We can continue to use the old values.
        * So no need to update anything here.
        */
    }

    if(pSpeclMezzanineStatus->uniqueID != NO_MODULE)
    {  
        pExtPeIoCompInfo->inserted = FBE_MGMT_STATUS_TRUE;
        pExtPeIoCompInfo->uniqueID = pSpeclMezzanineStatus->uniqueID; 
    }    
    
    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_board_read_mplxr_status
 **************************************************************************
 *  @brief
 *      This function gets SPECL mplxr status to populate the IO port led status in EDAL.
 *
 *  @param base_board- The pointer to the base board object.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    03-Mar-2010: PHE - Created.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_board_read_mplxr_status(fbe_base_board_t * base_board)
{
    fbe_u32_t           ioModule;
    fbe_u32_t           portNumOnModule;
    fbe_u32_t           componentIndex;
    fbe_u32_t           componentCount = 0;
    SPECL_MPLXR_SUMMARY  * pSpeclMplxrSum = NULL; 
    SPECL_IO_SUMMARY    * speclIoSum;
    edal_pe_io_port_sub_info_t   peIoPortSubInfo = {0};       
    edal_pe_io_port_sub_info_t * pPeIoPortSubInfo = &peIoPortSubInfo;
    fbe_board_mgmt_io_port_info_t  * pExtIoPortInfo = NULL;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_edal_status_t   edalStatus = FBE_EDAL_STATUS_OK;  
    fbe_edal_block_handle_t edalBlockHandle = NULL;   
        fbe_u32_t           portCountPerController = 0;
    fbe_u32_t           ioControllerIndex = 0;
    fbe_u32_t           ioPortIndex = 0;
        fbe_u32_t           controllerCountPerModule = 0;

    fbe_base_board_get_edal_block_handle(base_board, &edalBlockHandle);

    speclIoSum = (SPECL_IO_SUMMARY *) fbe_memory_ex_allocate(sizeof(SPECL_IO_SUMMARY));
    if (speclIoSum == NULL) {
        fbe_base_object_trace((fbe_base_object_t *) base_board,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: fail to allocate memory for specl IO summary.\n",
                              __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    pSpeclMplxrSum = (SPECL_MPLXR_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclMplxrSum, sizeof(SPECL_MPLXR_SUMMARY));

    status = fbe_base_board_pe_get_mplxr_status(pSpeclMplxrSum);
    if(status != FBE_STATUS_OK)
    {
        fbe_memory_ex_release(speclIoSum);
        return status;  
    }
   
    /* Get the total count of the io ports. */
    edalStatus = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                                                     FBE_ENCL_IO_PORT, 
                                                     &componentCount);

    if(edalStatus != FBE_EDAL_STATUS_OK)
    {
        fbe_memory_ex_release(speclIoSum);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    componentIndex = 0;

    FBE_FOR_ALL_MPLXR_IO_PORTS(ioModule, portNumOnModule, pSpeclMplxrSum)
    {
        fbe_zero_memory(pPeIoPortSubInfo, sizeof(edal_pe_io_port_sub_info_t));   

        /* Get old IO port info from EDAL. */
        edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_IO_PORT_INFO,  //Attribute
                                        FBE_ENCL_IO_PORT,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_io_port_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeIoPortSubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edalStatus))
        {
            fbe_memory_ex_release(speclIoSum);
            return FBE_STATUS_GENERIC_FAILURE;  
        }            

        pExtIoPortInfo = &pPeIoPortSubInfo->externalIoPortInfo;

        /* Get LED from IO module summary instead of Mplxr. */
        status = fbe_base_board_pe_get_io_comp_status(speclIoSum);
        if(status != FBE_STATUS_OK)
        {
            fbe_memory_ex_release(speclIoSum);
            return status;  
        }
                
                /*Map global portNumOnModule to local controller/port index*/
        ioPortIndex = portNumOnModule;
                controllerCountPerModule = speclIoSum->ioSummary[base_board->spid].ioStatus[ioModule].ioControllerCount;
                for (ioControllerIndex = 0; ioControllerIndex < controllerCountPerModule; ioControllerIndex++)
                {
                        portCountPerController = speclIoSum->ioSummary[base_board->spid].ioStatus[ioModule].ioControllerInfo[ioControllerIndex].ioPortCount;
                        if(ioPortIndex < portCountPerController)
                        {
                                pExtIoPortInfo->ioPortLED = 
                                        speclIoSum->ioSummary[base_board->spid].ioStatus[ioModule].ioControllerInfo[ioControllerIndex].ioPortInfo[ioPortIndex].LEDStatus.LEDBlinkRate;
                                pExtIoPortInfo->ioPortLEDColor = 
                                        speclIoSum->ioSummary[base_board->spid].ioStatus[ioModule].ioControllerInfo[ioControllerIndex].ioPortInfo[ioPortIndex].LEDStatus.LEDColor;
                                break ;
                        }
                        
                        ioPortIndex -= portCountPerController;                  
                }
  
        /* Update EDAL with new IO port info. */
        edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_IO_PORT_INFO,  //Attribute
                                        FBE_ENCL_IO_PORT,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_io_port_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeIoPortSubInfo);  // buffer pointer
        if(!EDAL_STAT_OK(edalStatus))
        {
            fbe_memory_ex_release(speclIoSum);
            return FBE_STATUS_GENERIC_FAILURE;  
        }  

        componentIndex++;

        if(componentIndex >= componentCount)
        {
            break;
        }
    }
    fbe_memory_ex_release(speclIoSum);
    return FBE_STATUS_OK;
}// End of - fbe_base_board_read_mplxr_status


/*!*************************************************************************
 *  @fn fbe_base_board_read_power_supply_status
 **************************************************************************
 *  @brief
 *      This function gets the power supply status and 
 *      fills in EDAL with the status.
 *
 *  @param base_board- The pointer to the base board object.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    03-Mar-2010: PHE - Created.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_board_read_power_supply_status(fbe_base_board_t * base_board)
{
    SP_ID               sp;
    POWER_SUPPLY_IDS    powerSupply;
    fbe_u32_t           componentIndex;
    fbe_u32_t           componentCount = 0;
    fbe_edal_status_t   edalStatus = FBE_EDAL_STATUS_OK;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_bool_t          power_failure = TRUE;
    fbe_u32_t           path_index = 0;
    fbe_u32_t           num_ports = 0;

    SPECL_PS_SUMMARY                * pSpeclPowerSupplySum = NULL;
    SPECL_PS_STATUS                 * pSpeclPowerSupplyStatus = NULL;
    edal_pe_power_supply_sub_info_t   pePowerSupplySubInfo = {0};       
    edal_pe_power_supply_sub_info_t * pPePowerSupplySubInfo = &pePowerSupplySubInfo;
    fbe_power_supply_info_t         * pExtPowerSupplyInfo = NULL;
    fbe_edal_block_handle_t           edalBlockHandle = NULL;   
    fbe_path_attr_t                   path_attr = 0;

    fbe_base_board_get_edal_block_handle(base_board, &edalBlockHandle);

    pSpeclPowerSupplySum = (SPECL_PS_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclPowerSupplySum, sizeof(SPECL_PS_SUMMARY));

    status = fbe_base_board_pe_get_power_supply_status(pSpeclPowerSupplySum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    /* Get the total count of the power supplies. */
    edalStatus = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                                                     FBE_ENCL_POWER_SUPPLY, 
                                                     &componentCount);

    if(edalStatus != FBE_EDAL_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    componentIndex = 0;

    FBE_FOR_ALL_POWER_SUPPLIES(sp, powerSupply, pSpeclPowerSupplySum)
    {
        fbe_zero_memory(pPePowerSupplySubInfo, sizeof(edal_pe_power_supply_sub_info_t));               

        /* Get old PS info from EDAL. */
        edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_POWER_SUPPLY_INFO,  //Attribute
                                        FBE_ENCL_POWER_SUPPLY,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_power_supply_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPePowerSupplySubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edalStatus))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }  

        pExtPowerSupplyInfo = &pPePowerSupplySubInfo->externalPowerSupplyInfo;
        pExtPowerSupplyInfo->associatedSp = sp;
        pExtPowerSupplyInfo->slotNumOnEncl = componentIndex;
        pExtPowerSupplyInfo->slotNumOnSpBlade = powerSupply;
        /* Processor enclosure power supply upgrade is control by POST. 
         * So we set bDownloadable to FALSE. 
         */
        pExtPowerSupplyInfo->bDownloadable = FALSE;

        pSpeclPowerSupplyStatus = &(pSpeclPowerSupplySum->psSummary[sp].psStatus[powerSupply]);  

        /* Handle the Stale data and SMB errors */
        status = fbe_base_board_check_env_interface_error(base_board,
                                                           FBE_ENCL_POWER_SUPPLY,
                                                           pSpeclPowerSupplyStatus->transactionStatus,
                                                           pSpeclPowerSupplyStatus->timeStamp.QuadPart,
                                                           &pPePowerSupplySubInfo->timeStampForFirstBadStatus,
                                                           &pExtPowerSupplyInfo->envInterfaceStatus);

        if(status != FBE_STATUS_OK)
        {
            return status;  
        }           

        fbe_base_board_translate_power_supply_status(base_board, 
                                                     pPePowerSupplySubInfo, 
                                                     pSpeclPowerSupplySum, 
                                                     pSpeclPowerSupplyStatus);
 
        /* Update EDAL with new PS info. */
        edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_POWER_SUPPLY_INFO,  //Attribute
                                        FBE_ENCL_POWER_SUPPLY,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_power_supply_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPePowerSupplySubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edalStatus))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }

        /*
         * Check if all the power supplies are experiencing ACFailures.
         * If they are then we notify the port object of the power failure.
         */
        
        if(pSpeclPowerSupplyStatus->ACFail == FBE_MGMT_STATUS_FALSE)
        {
            power_failure = FALSE;
        }

        componentIndex++;

        if(componentIndex >= componentCount)
        {
            break;
        }
    }

    fbe_base_discovering_get_number_of_clients((fbe_base_discovering_t *) base_board, &num_ports);

    if( (componentCount > 0) &&
        (power_failure == TRUE) )
    {
        // notify port object of the power failure
        for(path_index=0;path_index<num_ports;path_index++)
        {
            fbe_base_discovering_get_path_attr((fbe_base_discovering_t*) base_board, path_index,&path_attr);
            if(!(path_attr & FBE_DISCOVERY_PATH_ATTR_POWERFAIL_DETECTED))
            {
                fbe_base_discovering_set_path_attr((fbe_base_discovering_t*) base_board, path_index, FBE_DISCOVERY_PATH_ATTR_POWERFAIL_DETECTED);
            }
        }
    }
    else if( (componentCount > 0) &&
             (power_failure == FALSE) )
    {
        // notify port object of the power good
        for(path_index=0;path_index<num_ports;path_index++)
        {
            fbe_base_discovering_get_path_attr((fbe_base_discovering_t*) base_board, path_index,&path_attr);
            if(path_attr & FBE_DISCOVERY_PATH_ATTR_POWERFAIL_DETECTED)
            {
                fbe_base_discovering_clear_path_attr((fbe_base_discovering_t*) base_board, path_index, FBE_DISCOVERY_PATH_ATTR_POWERFAIL_DETECTED);
            }
        }
    }

    
    /*
     * Process the EIR info (if needed)
     */
    fbe_base_board_processEirSample(base_board, componentCount);

    return FBE_STATUS_OK;
}


/*!*************************************************************************
 *  @fn fbe_base_board_read_sps_status
 **************************************************************************
 *  @brief
 *      This function gets the SPS status and 
 *      fills in EDAL with the status.
 *
 *  @param base_board- The pointer to the base board object.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    04-Feb-2011: Joe Perry - Created.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_board_read_sps_status(fbe_base_board_t * base_board)
{
    fbe_u32_t           componentIndex;
    fbe_u32_t           componentCount = 0;
    fbe_edal_status_t   edalStatus = FBE_EDAL_STATUS_OK;
    fbe_status_t        status = FBE_STATUS_OK;
    PSPECL_SPS_SUMMARY               pSpeclSpsSum;
    SPECL_SPS_RESUME                speclSpsResume;
    edal_pe_sps_sub_info_t          peSpsSubInfo = {0};       
    edal_pe_sps_sub_info_t          * pPeSpsSubInfo = &peSpsSubInfo;
    fbe_base_sps_info_t             * pExtSpsInfo = NULL;
    fbe_edal_block_handle_t         edalBlockHandle = NULL;   

    fbe_base_board_get_edal_block_handle(base_board, &edalBlockHandle);

    pSpeclSpsSum = (SPECL_SPS_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclSpsSum, sizeof(SPECL_SPS_SUMMARY));

    status = fbe_base_board_pe_get_sps_status(pSpeclSpsSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    /* Get the total count of SPS's. */
    edalStatus = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                                                     FBE_ENCL_SPS, 
                                                     &componentCount);

    if(edalStatus != FBE_EDAL_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    for (componentIndex = 0; componentIndex < componentCount; componentIndex++)
    {
        fbe_zero_memory(pPeSpsSubInfo, sizeof(edal_pe_sps_sub_info_t));               
    
        /* Get old SPS info from EDAL. */
        edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_SPS_INFO,           //Attribute
                                        FBE_ENCL_SPS,                   // component type
                                        componentIndex,                 // component index
                                        sizeof(edal_pe_sps_sub_info_t), // buffer length
                                        (fbe_u8_t *)pPeSpsSubInfo);     // buffer pointer
        if(!EDAL_STAT_OK(edalStatus))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }  
    
        pExtSpsInfo = &pPeSpsSubInfo->externalSpsInfo;
        /* Handle the Stale data and SMB errors */
        status = fbe_base_board_check_env_interface_error(base_board,
                                                           FBE_ENCL_SPS,
                                                           pSpeclSpsSum->transactionStatus,
                                                           pSpeclSpsSum->timeStamp.QuadPart,
                                                           &pPeSpsSubInfo->timeStampForFirstBadStatus,
                                                           &pExtSpsInfo->envInterfaceStatus);
        if(status != FBE_STATUS_OK)
        {
            return status;  
        }

        /* Translate the sps resume before sps status.
         * The reason is that we need the resuem prom transaction status
         * to determine whether we could use the ispState when 
         * the SPS firmware upgrade is done.
         */
        status = fbe_base_board_pe_get_sps_resume(&speclSpsResume);
        if(status == FBE_STATUS_OK)
        {
            fbe_base_board_translate_sps_resume(base_board, 
                                                pPeSpsSubInfo, 
                                                &speclSpsResume);
        }

        fbe_base_board_translate_sps_status(base_board, 
                                            pPeSpsSubInfo, 
                                            pSpeclSpsSum);
     
        /* Update EDAL with new SPS info. */
        edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_SPS_INFO,           //Attribute
                                        FBE_ENCL_SPS,                   // component type
                                        componentIndex,                 // component index
                                        sizeof(edal_pe_sps_sub_info_t), // buffer length
                                        (fbe_u8_t *)pPeSpsSubInfo);     // buffer pointer
        if(!EDAL_STAT_OK(edalStatus))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }  
    }
           
    return FBE_STATUS_OK;
}   // end of fbe_base_board_read_sps_status

/*!*************************************************************************
 *  @fn fbe_base_board_read_battery_status
 **************************************************************************
 *  @brief
 *      This function gets the Battery status and 
 *      fills in EDAL with the status.
 *
 *  @param base_board- The pointer to the base board object.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    22-Mar-2012: Joe Perry - Created.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_board_read_battery_status(fbe_base_board_t * base_board)
{
    fbe_u32_t           componentIndex;
    fbe_u32_t           componentCount = 0;
    fbe_edal_status_t   edalStatus = FBE_EDAL_STATUS_OK;
    fbe_status_t        status = FBE_STATUS_OK;
    PSPECL_BATTERY_SUMMARY          pSpeclBatterySum;
    PSPECL_BATTERY_STATUS           pSpeclBatteryStatus;
//    SPECL_SPS_RESUME                speclSpsResume;
    edal_pe_battery_sub_info_t      peBatterySubInfo = {0};       
    edal_pe_battery_sub_info_t      * pPeBatterySubInfo = &peBatterySubInfo;
    fbe_base_battery_info_t         * pExtBatteryInfo = NULL;
    fbe_edal_block_handle_t         edalBlockHandle = NULL;   
    SP_ID                           sp;
    fbe_u32_t                       batteryIndex;

    fbe_base_board_get_edal_block_handle(base_board, &edalBlockHandle);

    pSpeclBatterySum = (SPECL_BATTERY_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclBatterySum, sizeof(SPECL_BATTERY_SUMMARY));

    status = fbe_base_board_pe_get_battery_status(pSpeclBatterySum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    edalStatus = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                                                    FBE_ENCL_BATTERY, 
                                                    &componentCount);
    if(edalStatus != FBE_EDAL_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;  
    } 

    componentIndex = 0;
    FBE_FOR_ALL_BATTERIES(sp, batteryIndex, pSpeclBatterySum)
    {
    
        fbe_zero_memory(pPeBatterySubInfo, sizeof(edal_pe_battery_sub_info_t));               
    
        /* Get old Battery info from EDAL. */
        edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_BATTERY_INFO,           //Attribute
                                        FBE_ENCL_BATTERY,                   // component type
                                        componentIndex,                     // component index
                                        sizeof(edal_pe_battery_sub_info_t), // buffer length
                                        (fbe_u8_t *)pPeBatterySubInfo);     // buffer pointer
        if(!EDAL_STAT_OK(edalStatus))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }  
    
        pExtBatteryInfo = &pPeBatterySubInfo->externalBatteryInfo;
        pSpeclBatteryStatus = &(pSpeclBatterySum->batterySummary[sp].batteryStatus[batteryIndex]);  
    
        pExtBatteryInfo->associatedSp = sp;
        pExtBatteryInfo->slotNumOnSpBlade = batteryIndex;

        /* Handle the Stale data and SMB errors */
        status = fbe_base_board_check_env_interface_error(base_board,
                                                           FBE_ENCL_BATTERY,
                                                           pSpeclBatteryStatus->transactionStatus,
                                                           pSpeclBatteryStatus->timeStamp.QuadPart,
                                                           &pPeBatterySubInfo->timeStampForFirstBadStatus,
                                                           &pExtBatteryInfo->envInterfaceStatus);

        if(status != FBE_STATUS_OK)
        {
            return status;  
        }           

        fbe_base_board_translate_battery_status(base_board, 
                                                pPeBatterySubInfo, 
                                                pSpeclBatterySum,
                                                pSpeclBatteryStatus);
     
// JAP_BOB - resume PROM is like other compnents, may not be needed             
/*             
        edalStatus = fbe_base_board_pe_get_battery_resume(&speclBatteryResume);
        if(EDAL_STAT_OK(edalStatus))
        {
            fbe_base_board_translate_battery_resume(base_board, 
                                                    pPeBatterySubInfo, 
                                                    &speclBatteryResume);
        }
*/

        /* Update EDAL with new Battery info. */
        edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_BATTERY_INFO,           //Attribute
                                        FBE_ENCL_BATTERY,                   // component type
                                        componentIndex,                     // component index
                                        sizeof(edal_pe_battery_sub_info_t), // buffer length
                                        (fbe_u8_t *)pPeBatterySubInfo);     // buffer pointer
        if(!EDAL_STAT_OK(edalStatus))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }  

        componentIndex++;

        if(componentIndex >= componentCount)
        {
            break;
        }
    }
           
    return FBE_STATUS_OK;
}   // end of fbe_base_board_read_battery_status


/*!*************************************************************************
 *  @fn fbe_base_board_translate_power_supply_status
 **************************************************************************
 *  @brief
 *      This function translates SPECL power supply status to physical package power supply info. 
 *
 *  @param base_board- The pointer to base board object.
 *  @param pPePowerSupplyInfo- The pointer to edal_pe_power_supply_sub_info_t. 
 *  @param pSpeclPowerSupplyStatus - The pointer to SPECL_PS_SUMMARY.
 *  @param pSpeclPowerSupplyStatus - The pointer to SPECL_PS_STATUS.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    03-Mar-2010: PHE - Created.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_board_translate_power_supply_status(fbe_base_board_t * base_board,
                                                edal_pe_power_supply_sub_info_t * pPePowerSupplySubInfo,     
                                                SPECL_PS_SUMMARY * pSpeclPowerSupplySum,
                                                SPECL_PS_STATUS  * pSpeclPowerSupplyStatus)
{
    SPID_PLATFORM_INFO * pPlatformInfo = NULL;
    fbe_power_supply_info_t * pExtPowerSupplyInfo = NULL;
    fbe_eir_input_power_sample_t    inputPower;

    pExtPowerSupplyInfo = &pPePowerSupplySubInfo->externalPowerSupplyInfo;    

    pExtPowerSupplyInfo->inProcessorEncl = TRUE;
    pExtPowerSupplyInfo->isLocalFru = pSpeclPowerSupplyStatus->localSupply;
    pExtPowerSupplyInfo->associatedSps = (pSpeclPowerSupplyStatus->localPSID == PS0) ? FBE_SPS_A : FBE_SPS_B;    
    pExtPowerSupplyInfo->bDownloadable = FALSE;
    pExtPowerSupplyInfo->uniqueId = pSpeclPowerSupplyStatus->uniqueID;
    pExtPowerSupplyInfo->uniqueIDFinal = pSpeclPowerSupplyStatus->uniqueIDFinal;

    if(pSpeclPowerSupplyStatus->transactionStatus == STATUS_SMB_DEVICE_NOT_PRESENT
            || pExtPowerSupplyInfo->envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
    {
        pExtPowerSupplyInfo->transactionStatus = pSpeclPowerSupplyStatus->transactionStatus;
        if(pSpeclPowerSupplyStatus->transactionStatus == STATUS_SMB_DEVICE_NOT_PRESENT)
        {
            pExtPowerSupplyInfo->bInserted = FBE_MGMT_STATUS_FALSE;
        }
        else
        {
            pExtPowerSupplyInfo->bInserted = FBE_MGMT_STATUS_UNKNOWN;
        }
        /* env interface is no good. We are not able to get any status and the old values are also timed out.
         */
        pExtPowerSupplyInfo->generalFault = FBE_MGMT_STATUS_UNKNOWN;
        pExtPowerSupplyInfo->internalFanFault = FBE_MGMT_STATUS_UNKNOWN;
        pExtPowerSupplyInfo->ambientOvertempFault = FBE_MGMT_STATUS_UNKNOWN;
        pExtPowerSupplyInfo->DCPresent = FBE_MGMT_STATUS_UNKNOWN;
        pExtPowerSupplyInfo->ACFail = FBE_MGMT_STATUS_UNKNOWN;
        pExtPowerSupplyInfo->ACDCInput = FBE_MGMT_STATUS_UNKNOWN;

        pExtPowerSupplyInfo->firmwareRevValid = FALSE;
        /* Need to convert to the string. */
        //pExtPowerSupplyInfo->firmwareRev = pSpeclPowerSupplyStatus->firmwareRevision;

        // set InputPower info
        inputPower.inputPower = 0;
        inputPower.status = FBE_ENERGY_STAT_FAILED;
        fbe_base_board_saveEirSample(base_board, pExtPowerSupplyInfo->slotNumOnEncl, &inputPower);
    }
    /* Check local PS env interface status. */
    else if(pSpeclPowerSupplyStatus->transactionStatus == EMCPAL_STATUS_SUCCESS)
    {
        pExtPowerSupplyInfo->transactionStatus = pSpeclPowerSupplyStatus->transactionStatus;
        pExtPowerSupplyInfo->bInserted = pSpeclPowerSupplyStatus->inserted;
        pExtPowerSupplyInfo->generalFault = (pSpeclPowerSupplyStatus->generalFault == TRUE) ? FBE_MGMT_STATUS_TRUE : FBE_MGMT_STATUS_FALSE;
        if (pSpeclPowerSupplyStatus->type == 4)
        {
            pExtPowerSupplyInfo->internalFanFault = (pSpeclPowerSupplyStatus->type4.internalFanFault == TRUE) ? FBE_MGMT_STATUS_TRUE : FBE_MGMT_STATUS_FALSE;
        }
        else
        {
            pExtPowerSupplyInfo->internalFanFault = FBE_MGMT_STATUS_FALSE;
        }
        pExtPowerSupplyInfo->ambientOvertempFault = (pSpeclPowerSupplyStatus->ambientOvertempFault == TRUE) ? FBE_MGMT_STATUS_TRUE : FBE_MGMT_STATUS_FALSE; 
        pExtPowerSupplyInfo->DCPresent = (pSpeclPowerSupplyStatus->DCPresent == TRUE) ? FBE_MGMT_STATUS_TRUE : FBE_MGMT_STATUS_FALSE; 
        pExtPowerSupplyInfo->ACFail = (pSpeclPowerSupplyStatus->ACFail == TRUE) ? FBE_MGMT_STATUS_TRUE : FBE_MGMT_STATUS_FALSE; 
        pExtPowerSupplyInfo->ACDCInput = pSpeclPowerSupplyStatus->ACDCInput;        

        // check for PS Margin Test faults (reporting through BIST)
        pExtPowerSupplyInfo->psMarginTestMode = FBE_ESES_MARGINING_TEST_MODE_STATUS_TEST_SUCCESSFUL;
        pExtPowerSupplyInfo->psMarginTestResults = FBE_ESES_MARGINING_TEST_RESULTS_SUCCESS;
        fbe_base_object_trace((fbe_base_object_t*)base_board,
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, psMarginTestMode %d, psMarginTestResults %d\n", 
                              __FUNCTION__,
                              pExtPowerSupplyInfo->psMarginTestMode,
                              pExtPowerSupplyInfo->psMarginTestResults);

        pPlatformInfo = &base_board->platformInfo;

        pExtPowerSupplyInfo->firmwareRevValid = TRUE;
        /* Convert to the string. */
        if (pSpeclPowerSupplyStatus->type == 3)
        {
            fbe_sprintf(pExtPowerSupplyInfo->firmwareRev,
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1,
                        "%02d%02d",
                        pSpeclPowerSupplyStatus->type3.firmwareRevMajor,
                        pSpeclPowerSupplyStatus->type3.firmwareRevMinor);
        }
        else if (pSpeclPowerSupplyStatus->type == 4)
        {
            fbe_sprintf(pExtPowerSupplyInfo->firmwareRev,
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1,
                        "%02d%02d",
                        pSpeclPowerSupplyStatus->type4.firmwareRevMajor,
                        pSpeclPowerSupplyStatus->type4.firmwareRevMinor);
        }
        else
        {
            fbe_sprintf(pExtPowerSupplyInfo->firmwareRev,
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1,
                        "0000");
        }

        // process InputPower info
        fbe_base_board_setPsInputPower(pSpeclPowerSupplyStatus, &inputPower);
        fbe_base_board_saveEirSample(base_board, pExtPowerSupplyInfo->slotNumOnEncl, &inputPower);

    }
    else
    {
        /* If envInterfaceStatus is FBE_ENV_INTERFACE_STATUS_GOOD, 
         * no need to do anything, it will continue to use the old values. 
         */     
        // copy previous sample to new sample
        inputPower.inputPower = 
            base_board->inputPowerSamples[pExtPowerSupplyInfo->slotNumOnEncl][base_board->sampleIndex].inputPower;
        inputPower.status = 
            base_board->inputPowerSamples[pExtPowerSupplyInfo->slotNumOnEncl][base_board->sampleIndex].status;
        fbe_base_board_saveEirSample(base_board, pExtPowerSupplyInfo->slotNumOnEncl, &inputPower);

    }

    return FBE_STATUS_OK;
}


/*!*************************************************************************
 *  @fn fbe_base_board_translate_sps_status
 **************************************************************************
 *  @brief
 *      This function translates SPECL SPS status to physical package SPS info. 
 *
 *  @param base_board- The pointer to base board object.
 *  @param pPeSpsSubInfo- The pointer to edal_pe_sps_sub_info_t. 
 *  @param pSpeclSpsSum - The pointer to SPECL_SPS_SUMMARY.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    23-Feb-2011: Joe Perry - Created.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_board_translate_sps_status(fbe_base_board_t * base_board,
                                    edal_pe_sps_sub_info_t * pPeSpsSubInfo,     
                                    SPECL_SPS_SUMMARY * pSpeclSpsSum)
{
    fbe_base_sps_info_t *pExtSpsInfo = NULL;
    fbe_s32_t            ispIndex = 0; 
    fbe_u32_t            ispIndexProcessed = 0;
    SPS_STATE            oldSpsStatus = SPS_STATE_UNKNOWN;

    pExtSpsInfo = &(pPeSpsSubInfo->externalSpsInfo);

    /* Save oldSpsStatus. It will be used later. */
    oldSpsStatus = pExtSpsInfo->spsStatus;

    pExtSpsInfo->spsStatus = pSpeclSpsSum->state;
    pExtSpsInfo->spsModulePresent = pSpeclSpsSum->inserted;
    pExtSpsInfo->spsModel = pSpeclSpsSum->type;
    pExtSpsInfo->spsBatteryTime = pSpeclSpsSum->batteryTime;
    pExtSpsInfo->transactionStatus = pSpeclSpsSum->transactionStatus;

    // process SPS Fault info
    pExtSpsInfo->spsFaultInfo.spsBatteryEOL = pSpeclSpsSum->batteryEOL;
    pExtSpsInfo->spsFaultInfo.spsChargerFailure = pSpeclSpsSum->chargeFail;
    if (pSpeclSpsSum->inserted)
    {
        pExtSpsInfo->spsBatteryPresent = pSpeclSpsSum->batteryStatus.inserted;
        pExtSpsInfo->spsFaultInfo.spsBatteryNotEngagedFault =
            pSpeclSpsSum->batteryPackModuleNotEngagedFault;
    }
    else
    {
        pExtSpsInfo->spsBatteryPresent = FALSE;
        pExtSpsInfo->spsFaultInfo.spsBatteryNotEngagedFault = TRUE;
    }

    // ignore Internal Fault if Battery removed
    if (pExtSpsInfo->spsBatteryPresent)
    {
        pExtSpsInfo->spsFaultInfo.spsInternalFault = pSpeclSpsSum->internalFaults;
    }
    pExtSpsInfo->spsFaultInfo.spsCableConnectivityFault = pSpeclSpsSum->cableConnectivity;

    pExtSpsInfo->spsModuleFfid = pSpeclSpsSum->uniqueID;
    pExtSpsInfo->spsBatteryFfid = pSpeclSpsSum->batteryStatus.batteryUniqueID;
    if(pSpeclSpsSum->uniqueID == LI_ION_SPS_2_2_KW_WITHOUT_BATTERY)
    {
        pExtSpsInfo->bSpsModuleDownloadable = FBE_TRUE;
        pExtSpsInfo->programmableCount = 2;
    }
    else
    {
        pExtSpsInfo->bSpsModuleDownloadable = FBE_FALSE;
        pExtSpsInfo->programmableCount = 0;
    }

    if((pSpeclSpsSum->batteryStatus.batteryUniqueID == LI_ION_2_42_KW_BATTERY_PACK_082) ||
       (pSpeclSpsSum->batteryStatus.batteryUniqueID == LI_ION_2_42_KW_BATTERY_PACK_115))
    {
        pExtSpsInfo->bSpsBatteryDownloadable = FBE_TRUE;
        pExtSpsInfo->programmableCount ++;
    }
    else
    {
        pExtSpsInfo->bSpsBatteryDownloadable = FBE_FALSE;
    }
   
    /* ispLog[0].state is the latest state. 
     * ispLog[MAX_SPS_ISP_STATE_LOG_ENTRY-1].state is the oldest state.
     * We will start with the oldest.
     * Comparing with the saved lastIspStateTimeStamp, if the state was processed before, skip it.
     * Otherwise, translate the ispState.
     * ispIndex to be signed so that it can get out of the for loop below. 
     */
    for (ispIndex = 0; ispIndex < MAX_SPS_ISP_STATE_LOG_ENTRY; ispIndex ++)
    {
        /* Some ipsState have the same timestamps. 
         * Even if the timestamps are the same, we need to process if the ispState is different.
         * We must process the lastest ispState with ispIndex 0.
         * The reason is that the we might skip the processing of the idle ispState while the spsStatus was SPS_STATE_ON_BATTERY.
         * The spsStatus could be changed and now we need to process the idle ispState.
         */
        if((pSpeclSpsSum->ispLog[ispIndex].timeStamp.QuadPart == pPeSpsSubInfo->lastIspStateTimeStamp.QuadPart) &&
           (pSpeclSpsSum->ispLog[ispIndex].state == pPeSpsSubInfo->ispState)) 
        {
            ispIndexProcessed = ispIndex;
            break;
        }
    }

    if(ispIndexProcessed == 0)
    {
        if((pSpeclSpsSum->ispLog[0].state == SPS_ISP_IDLE) && 
           (oldSpsStatus == SPS_STATE_ON_BATTERY) &&
           (oldSpsStatus != pExtSpsInfo->spsStatus))
        {
            /* Need to process the latest ispState even if it has been processed before. 
             * The reason is that spsState might changed from SPS_STATE_ON_BATTERY. 
             * We skipped translating the ispState when it was IDLE and spsState was SPS_STATE_ON_BATTERY
             */
            fbe_base_object_trace((fbe_base_object_t *)base_board,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "SPS ispState %s, spsStatus changed from %s to %s.\n",  
                                      decodeSpsIspState(SPS_ISP_IDLE),
                                      decodeSpsState(oldSpsStatus),
                                      decodeSpsState(pExtSpsInfo->spsStatus));

            fbe_base_board_translate_sps_isp_state(base_board, 
                                                   pPeSpsSubInfo, 
                                                   pSpeclSpsSum,
                                                   pSpeclSpsSum->ispLog[0].state);
        }
        else if((pSpeclSpsSum->ispLog[0].state == SPS_ISP_IDLE) && 
                (pPeSpsSubInfo->prevResumeTransactionStatus == STATUS_SPECL_TRANSACTION_PENDING) &&
                (pPeSpsSubInfo->resumeTransactionStatus != pPeSpsSubInfo->prevResumeTransactionStatus))
        {
            fbe_base_object_trace((fbe_base_object_t *)base_board,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "SPS ispState IDLE, resumeTrans changed from pending\n");
            fbe_base_board_translate_sps_isp_state(base_board, 
                                                   pPeSpsSubInfo, 
                                                   pSpeclSpsSum,
                                                   pSpeclSpsSum->ispLog[0].state);
        }
    }
    else 
    {
        /* ispIndexProcessed > 0 */
        for (ispIndex = ispIndexProcessed - 1; ispIndex >= 0; ispIndex--)
        { 
            fbe_base_object_trace((fbe_base_object_t *)base_board,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "SPS ispState changed from %s to %s.\n",  
                                      decodeSpsIspState(pPeSpsSubInfo->ispState),
                                      decodeSpsIspState(pSpeclSpsSum->ispLog[ispIndex].state));

            pPeSpsSubInfo->ispState = pSpeclSpsSum->ispLog[ispIndex].state;
            pPeSpsSubInfo->lastIspStateTimeStamp = pSpeclSpsSum->ispLog[ispIndex].timeStamp;

            fbe_base_board_translate_sps_isp_state(base_board, 
                                                   pPeSpsSubInfo, 
                                                   pSpeclSpsSum,
                                                   pSpeclSpsSum->ispLog[ispIndex].state);
        } 
    }

    return FBE_STATUS_OK;

}   // end of fbe_base_board_translate_sps_status


/*!*************************************************************************
 *  @fn fbe_base_board_translate_sps_isp_state
 **************************************************************************
 *  @brief
 *      This function translates SPECL SPS ispState. 
 *
 *  @param base_board- The pointer to base board object.
 *  @param pPeSpsSubInfo- The pointer to edal_pe_sps_sub_info_t. 
 *  @param ispState - SPS ispState to be translated.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    07-Feb-2011: PHE - Created.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_board_translate_sps_isp_state(fbe_base_board_t * base_board,
                                       edal_pe_sps_sub_info_t * pPeSpsSubInfo, 
                                       SPECL_SPS_SUMMARY * pSpeclSpsSum,
                                       SPS_ISP_STATE ispState)
{
    fbe_base_object_trace((fbe_base_object_t *)base_board,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s, entry.\n",  
                          __FUNCTION__);

    switch(ispState)
    {
        case SPS_ISP_UNKNOWN:
        case SPS_ISP_INIT:
        case SPS_ISP_READY_TO_RECEIVE:
            pPeSpsSubInfo->fupStatus = FBE_ENCLOSURE_FW_STATUS_UNKNOWN;
            pPeSpsSubInfo->fupExtStatus = FBE_ENCLOSURE_FW_EXT_STATUS_UNKNOWN;
            break;

        case SPS_ISP_ACTIVATED:
        case SPS_ISP_DOWNLOADING:
            pPeSpsSubInfo->fupStatus = FBE_ENCLOSURE_FW_STATUS_IN_PROGRESS;
            pPeSpsSubInfo->fupExtStatus = FBE_ENCLOSURE_FW_EXT_STATUS_NONE;
            break;

        case SPS_ISP_DOWNLOAD_COMPLETE:
            pPeSpsSubInfo->fupStatus = FBE_ENCLOSURE_FW_STATUS_NONE;
            pPeSpsSubInfo->fupExtStatus = FBE_ENCLOSURE_FW_EXT_STATUS_IMAGE_LOADED;
            break;

        case SPS_ISP_IDLE:
            if((pPeSpsSubInfo->fupStatus == FBE_ENCLOSURE_FW_STATUS_DOWNLOAD_FAILED) ||
               (pPeSpsSubInfo->fupStatus == FBE_ENCLOSURE_FW_STATUS_ACTIVATE_FAILED) ||
               (pPeSpsSubInfo->fupStatus == FBE_ENCLOSURE_FW_STATUS_ABORT))
            {
                fbe_base_object_trace((fbe_base_object_t *)base_board,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "SPS ispState %s, latch the fupStatus %d until next download.\n",
                              decodeSpsIspState(ispState),
                              pPeSpsSubInfo->fupStatus);
            }
            else if(pSpeclSpsSum->state == SPS_STATE_ON_BATTERY)
            {
                fbe_base_object_trace((fbe_base_object_t *)base_board,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "SPS ispState %s, spsStatus %s, do not update fupStatus.\n",
                              decodeSpsIspState(ispState),
                              decodeSpsState(pSpeclSpsSum->state));
            }
            else if(pPeSpsSubInfo->resumeTransactionStatus == STATUS_SPECL_TRANSACTION_PENDING)
            {
                fbe_base_object_trace((fbe_base_object_t *)base_board,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "SPS ispState %s, resumeXStatus PENDING, do not update fupStatus.\n",
                              decodeSpsIspState(ispState));
            }
            else
            {
                pPeSpsSubInfo->fupStatus = FBE_ENCLOSURE_FW_STATUS_NONE;
                pPeSpsSubInfo->fupExtStatus = FBE_ENCLOSURE_FW_EXT_STATUS_NONE;
            }
            break;

        case SPS_ISP_ACTIVATE_FAILED:
            /*AR566061  If ACTIVATE FAILED is caused by download failed, we ignore this ACTIVATE FAILED, so ESP will retry FUP.*/
            if (pPeSpsSubInfo->fupStatus != FBE_ENCLOSURE_FW_STATUS_DOWNLOAD_FAILED)
            {
                pPeSpsSubInfo->fupStatus = FBE_ENCLOSURE_FW_STATUS_ACTIVATE_FAILED;
                pPeSpsSubInfo->fupExtStatus = FBE_ENCLOSURE_FW_EXT_STATUS_NONE;
            }
            break;

        case SPS_ISP_ABORTED:
            /*AR566061  Currently ISP_ABORTED just mean download failed*/
            pPeSpsSubInfo->fupStatus = FBE_ENCLOSURE_FW_STATUS_DOWNLOAD_FAILED;
            pPeSpsSubInfo->fupExtStatus = FBE_ENCLOSURE_FW_EXT_STATUS_NONE;
            break;
            
        case SPS_ISP_NOT_READY:
            pPeSpsSubInfo->fupStatus = FBE_ENCLOSURE_FW_STATUS_DOWNLOAD_FAILED;
            pPeSpsSubInfo->fupExtStatus = FBE_ENCLOSURE_FW_EXT_STATUS_NONE;
            break;

        default:
            break;
    } // End of - switch(ispState)

    return FBE_STATUS_OK;
}
            
/*!*************************************************************************
 *  @fn fbe_base_board_translate_sps_resume
 **************************************************************************
 *  @brief
 *      This function translates SPECL SPS Resume PROM to physical package
 *      SPS Resume PROM info. 
 *
 *  @param base_board- The pointer to base board object.
 *  @param pPeSpsSubInfo- The pointer to edal_pe_sps_sub_info_t. 
 *  @param pSpeclSpsResume - The pointer to SPECL_SPS_RESUME.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    23-Feb-2011: Joe Perry - Created.
 *    06-May-2-13: PHE - Updated to check the spsResumeTransactionStatus and
 *                                 spsBatteryResumeTransactionStatus.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_board_translate_sps_resume(fbe_base_board_t * base_board,
                                    edal_pe_sps_sub_info_t * pPeSpsSubInfo,     
                                    SPECL_SPS_RESUME * pSpeclSpsResume)
{
    fbe_sps_manuf_info_t *pExtSpsManufInfo = &pPeSpsSubInfo->externalSpsManufInfo;

    pPeSpsSubInfo->prevResumeTransactionStatus = pPeSpsSubInfo->resumeTransactionStatus;
    pPeSpsSubInfo->resumeTransactionStatus = pSpeclSpsResume->transactionStatus;

    if(pSpeclSpsResume->transactionStatus == STATUS_SMB_DEVICE_NOT_PRESENT)
    {
        fbe_zero_memory(pExtSpsManufInfo, sizeof(fbe_sps_manuf_info_t));

        fbe_zero_memory(&pPeSpsSubInfo->externalSpsInfo.primaryFirmwareRev[0], 
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

        fbe_zero_memory(&pPeSpsSubInfo->externalSpsInfo.secondaryFirmwareRev[0],
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

        fbe_zero_memory(&pPeSpsSubInfo->externalSpsInfo.batteryFirmwareRev[0],
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);
    }
    else if(pSpeclSpsResume->transactionStatus == EMCPAL_STATUS_SUCCESS) 
    {
        strncpy(pExtSpsManufInfo->spsModuleManufInfo.spsSerialNumber,        pSpeclSpsResume->emcSerialNum, sizeof(pExtSpsManufInfo->spsModuleManufInfo.spsSerialNumber)-1);
        strncpy(pExtSpsManufInfo->spsModuleManufInfo.spsPartNumber,          pSpeclSpsResume->emcPartNum, sizeof(pExtSpsManufInfo->spsModuleManufInfo.spsPartNumber)-1);
        strncpy(pExtSpsManufInfo->spsModuleManufInfo.spsPartNumRevision,     pSpeclSpsResume->emcRevision, sizeof(pExtSpsManufInfo->spsModuleManufInfo.spsPartNumRevision)-1);
        strncpy(pExtSpsManufInfo->spsModuleManufInfo.spsVendor,              pSpeclSpsResume->supplier, sizeof(pExtSpsManufInfo->spsModuleManufInfo.spsVendor)-1);
        strncpy(pExtSpsManufInfo->spsModuleManufInfo.spsVendorModelNumber,   pSpeclSpsResume->supplierModelNum, sizeof(pExtSpsManufInfo->spsModuleManufInfo.spsVendorModelNumber)-1);
        strncpy(pExtSpsManufInfo->spsModuleManufInfo.spsFirmwareRevision,    pSpeclSpsResume->primaryFirmwareRev, sizeof(pExtSpsManufInfo->spsModuleManufInfo.spsFirmwareRevision)-1);
        strncpy(pExtSpsManufInfo->spsModuleManufInfo.spsSecondaryFirmwareRevision,    pSpeclSpsResume->secondaryFirmwareRev, sizeof(pExtSpsManufInfo->spsModuleManufInfo.spsSecondaryFirmwareRevision)-1);
        strncpy(pExtSpsManufInfo->spsModuleManufInfo.spsModelString,         pSpeclSpsResume->supplierModelNum, sizeof(pExtSpsManufInfo->spsModuleManufInfo.spsModelString)-1);

        fbe_copy_memory(&pPeSpsSubInfo->externalSpsInfo.primaryFirmwareRev[0],
                    &pSpeclSpsResume->primaryFirmwareRev[0],
                    FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

        fbe_copy_memory(&pPeSpsSubInfo->externalSpsInfo.secondaryFirmwareRev[0],
                        &pSpeclSpsResume->secondaryFirmwareRev[0],
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

        if(pSpeclSpsResume->batteryResume.transactionStatus == STATUS_SMB_DEVICE_NOT_PRESENT) 
        {
            fbe_zero_memory(&pExtSpsManufInfo->spsBatteryManufInfo, sizeof(fbe_sps_manuf_record_t));

            fbe_zero_memory(&pPeSpsSubInfo->externalSpsInfo.batteryFirmwareRev[0],
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);
        }
        else if(pSpeclSpsResume->batteryResume.transactionStatus == EMCPAL_STATUS_SUCCESS) 
        {
            strncpy(pExtSpsManufInfo->spsBatteryManufInfo.spsSerialNumber,       pSpeclSpsResume->batteryResume.batEmcSerialNum, sizeof(pExtSpsManufInfo->spsBatteryManufInfo.spsSerialNumber)-1);
            strncpy(pExtSpsManufInfo->spsBatteryManufInfo.spsPartNumber,         pSpeclSpsResume->batteryResume.batEmcPartNum, sizeof(pExtSpsManufInfo->spsBatteryManufInfo.spsPartNumber)-1);
            strncpy(pExtSpsManufInfo->spsBatteryManufInfo.spsPartNumRevision,    pSpeclSpsResume->batteryResume.batEmcRevision, sizeof(pExtSpsManufInfo->spsBatteryManufInfo.spsPartNumRevision)-1);
            strncpy(pExtSpsManufInfo->spsBatteryManufInfo.spsVendor,             pSpeclSpsResume->batteryResume.batSupplier, sizeof(pExtSpsManufInfo->spsBatteryManufInfo.spsVendor)-1);
            strncpy(pExtSpsManufInfo->spsBatteryManufInfo.spsVendorModelNumber,  pSpeclSpsResume->batteryResume.batSupplierModelNum, sizeof(pExtSpsManufInfo->spsBatteryManufInfo.spsVendorModelNumber)-1);
            strncpy(pExtSpsManufInfo->spsBatteryManufInfo.spsFirmwareRevision,   pSpeclSpsResume->batteryResume.batFirmwareRev, sizeof(pExtSpsManufInfo->spsBatteryManufInfo.spsFirmwareRevision)-1);
            strncpy(pExtSpsManufInfo->spsBatteryManufInfo.spsModelString,        pSpeclSpsResume->batteryResume.batSupplierModelNum, sizeof(pExtSpsManufInfo->spsBatteryManufInfo.spsModelString)-1);

            fbe_copy_memory(&pPeSpsSubInfo->externalSpsInfo.batteryFirmwareRev[0],
                        &pSpeclSpsResume->batteryResume.batFirmwareRev[0],
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);
        }
    }
    else
    {
        /* The transactionStatus is bad.
         * There is no need to do anything, it will continue to use the old values. 
         */
    }

    return FBE_STATUS_OK;

}   // end of fbe_base_board_translate_sps_resume


/*!*************************************************************************
 *  @fn fbe_base_board_translate_battery_status
 **************************************************************************
 *  @brief
 *      This function translates SPECL Battery status to physical package Battery info. 
 *
 *  @param base_board- The pointer to base board object.
 *  @param pPeSpsSubInfo- The pointer to edal_pe_sps_sub_info_t. 
 *  @param pSpeclSpsSum - The pointer to SPECL_SPS_SUMMARY.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    23-Feb-2011: Joe Perry - Created.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_board_translate_battery_status(fbe_base_board_t * base_board,
                                        edal_pe_battery_sub_info_t * pPeBatterySubInfo,     
                                        SPECL_BATTERY_SUMMARY * pSpeclBatterySum,
                                        SPECL_BATTERY_STATUS * pSpeclBatteryStatus)
{
    fbe_base_battery_info_t *pExtBatteryInfo = NULL;
    fbe_base_battery_info_t previousBatteryInfo;
    BMC_BIST_RESULT         tmpBatteryTestState = BMC_BIST_UNKNOWN;

    pExtBatteryInfo = &(pPeBatterySubInfo->externalBatteryInfo);
    pExtBatteryInfo->hasResumeProm = TRUE;

    /* Save the previous external battery info before it gets updated.
     * It will be used while logging the ktrace.
     */
    previousBatteryInfo = *pExtBatteryInfo;

    /* Determine testState of the battery */
    if (pSpeclBatteryStatus->type == 1)
    {
        tmpBatteryTestState = pSpeclBatteryStatus->type1.testState;
    }
    else if (pSpeclBatteryStatus->type == 3)
    {
        tmpBatteryTestState = pSpeclBatteryStatus->type3.testState;
    }
    else if (pSpeclBatteryStatus->type == 4)
    {
        tmpBatteryTestState = pSpeclBatteryStatus->type4.testState;
    }

    /* SPECL could set the inserted-ness to TRUE before it can be really acted upon
     * So we must check the transactionStatus before using the inserted-ness
     */
    if(previousBatteryInfo.batteryReady != pSpeclBatteryStatus->batteryReady)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, transSts:0x%x Ready change to:%d\n",
                              __FUNCTION__,
                              pSpeclBatteryStatus->transactionStatus,
                              pSpeclBatteryStatus->batteryReady);

        fbe_base_object_trace((fbe_base_object_t *)base_board,
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, enabl:%d genflt:%d intflt:%d lowChrg:%d onbatt:%d\n",
                              __FUNCTION__,
                              pSpeclBatteryStatus->enabled,
                              pSpeclBatteryStatus->generalFault,
                              pSpeclBatteryStatus->internalFailure,
                              pSpeclBatteryStatus->lowCharge,
                              pSpeclBatteryStatus->onBattery);
        if (pSpeclBatteryStatus->type != 2)
        {
            fbe_base_object_trace((fbe_base_object_t *)base_board,
                                  FBE_TRACE_LEVEL_DEBUG_LOW,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, tststate:0x%x\n",
                                  __FUNCTION__,
                                  tmpBatteryTestState);
        }
    }

    if(pSpeclBatteryStatus->transactionStatus == EMCPAL_STATUS_SUCCESS)
    {
        pExtBatteryInfo->transactionStatus = pSpeclBatteryStatus->transactionStatus;
        // set direct mapping Battery fields
        pExtBatteryInfo->batteryInserted = pSpeclBatteryStatus->inserted;
        pExtBatteryInfo->batteryEnabled = pSpeclBatteryStatus->enabled;
        pExtBatteryInfo->onBattery = pSpeclBatteryStatus->onBattery;
        pExtBatteryInfo->batteryReady = pSpeclBatteryStatus->batteryReady;
        pExtBatteryInfo->batteryChargeState = pSpeclBatteryStatus->chargeState;
        pExtBatteryInfo->firmwareRevMajor = pSpeclBatteryStatus->firmwareRevMajor;
        pExtBatteryInfo->firmwareRevMinor = pSpeclBatteryStatus->firmwareRevMinor;
        pExtBatteryInfo->batteryFfid = pSpeclBatteryStatus->uniqueID;
        if (pSpeclBatteryStatus->type == 1)
        {
            pExtBatteryInfo->batteryDeliverableCapacity = pSpeclBatteryStatus->type1.deliverableCapacity;
            pExtBatteryInfo->batteryStorageCapacity = pSpeclBatteryStatus->type1.storageCapacity;
            pExtBatteryInfo->energyRequirement.energy = pSpeclBatteryStatus->type1.energyRequirement;
            pExtBatteryInfo->energyRequirement.maxLoad = pSpeclBatteryStatus->type1.maxLoad;
        }
        else if (pSpeclBatteryStatus->type == 3)
        {
            pExtBatteryInfo->energyRequirement.energy = pSpeclBatteryStatus->type3.energyRequirement;
            pExtBatteryInfo->energyRequirement.maxLoad = pSpeclBatteryStatus->type3.maxLoad;
        }
        else if (pSpeclBatteryStatus->type == 4)
        {
            pExtBatteryInfo->energyRequirement.energy = pSpeclBatteryStatus->type4.energyRequirement;
            pExtBatteryInfo->energyRequirement.maxLoad = pSpeclBatteryStatus->type4.maxLoad;
        }
        else if (pSpeclBatteryStatus->type == 2)
        {
            pExtBatteryInfo->energyRequirement.power = pSpeclBatteryStatus->type2.dischargeRate;
            pExtBatteryInfo->energyRequirement.time = pSpeclBatteryStatus->type2.remainingTime;
        }


        // set Battery Status
        if (!pSpeclBatteryStatus->inserted)
        {
            pExtBatteryInfo->batteryState = FBE_BATTERY_STATUS_REMOVED;
        }
        else if (pSpeclBatteryStatus->onBattery)
        {
            pExtBatteryInfo->batteryState = FBE_BATTERY_STATUS_ON_BATTERY;
        }
        else if (pSpeclBatteryStatus->batteryReady)
        {
            pExtBatteryInfo->batteryState = FBE_BATTERY_STATUS_BATTERY_READY;
        }
        else if (pSpeclBatteryStatus->chargeState == BATTERY_CHARGING)
        {
            pExtBatteryInfo->batteryState = FBE_BATTERY_STATUS_CHARGING;
        }
        else if (pSpeclBatteryStatus->chargeState == BATTERY_FULLY_CHARGED)
        {
            pExtBatteryInfo->batteryState = FBE_BATTERY_STATUS_FULL_CHARGED;
        }
        else if (tmpBatteryTestState == BMC_BIST_RUNNING)
        {
            pExtBatteryInfo->batteryState = FBE_BATTERY_STATUS_TESTING;
        }
        else
        {
            pExtBatteryInfo->batteryState = FBE_BATTERY_STATUS_UNKNOWN;
        }
        
        // set Battery Test Status
        if ((tmpBatteryTestState == BMC_BIST_PASSED) &&
            (previousBatteryInfo.batteryTestStatus == FBE_BATTERY_TEST_STATUS_STARTED))
        {
            pExtBatteryInfo->batteryTestStatus = FBE_BATTERY_TEST_STATUS_COMPLETED;
            fbe_base_object_trace((fbe_base_object_t *)base_board,
                                  FBE_TRACE_LEVEL_DEBUG_HIGH,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, TestCompleted, testResults 0x%x, buckConv %d\n",
                                  __FUNCTION__,
                                  (pSpeclBatteryStatus->type == 1) ?
                                    pSpeclBatteryStatus->type1.testResult : 0,
                                  (pSpeclBatteryStatus->type == 1) ?
                                    pSpeclBatteryStatus->type1.extendedTestResult.fields.buck_converter_fault : 0);
        }
        else if (tmpBatteryTestState == BMC_BIST_FAILED)
        {
            /* Removed the following check. 
             * if (previousBatteryInfo.batteryTestStatus == FBE_BATTERY_TEST_STATUS_STARTED)
             * Because if SPB boots up after SPA finished the BBU self test,
             * SPB will not have this FBE_BATTERY_TEST_STATUS_STARTED. With this check, SPB could not 
             * report the BBUA0 test status and test fault correctly  -- PINGHE.
             */ 
            if ((pSpeclBatteryStatus->type == 1) &&
                (pSpeclBatteryStatus->type1.testResult == 0xAA))
            {
               if (pSpeclBatteryStatus->type1.extendedTestResult.fields.rail_lift_ckt_fault)
               {
                   if(pExtBatteryInfo->batteryTestStatus != FBE_BATTERY_TEST_STATUS_COMPLETED)
                   {
                       fbe_base_object_trace((fbe_base_object_t *)base_board,
                                         FBE_TRACE_LEVEL_INFO,
                                         FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s%d, type %d, RailLift failure - IGNORE (need new FW)\n",
                                         (pExtBatteryInfo->associatedSp == SP_A) ? "BBUA":"BBUB",
                                         pExtBatteryInfo->slotNumOnSpBlade,
                                         pSpeclBatteryStatus->type);

                       pExtBatteryInfo->batteryTestStatus = FBE_BATTERY_TEST_STATUS_COMPLETED;
                   }
               }
               else
               {
                   if(pExtBatteryInfo->batteryTestStatus != FBE_BATTERY_TEST_STATUS_FAILED)
                   {
                       fbe_base_object_trace((fbe_base_object_t *)base_board,
                                         FBE_TRACE_LEVEL_INFO,
                                         FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s%d, type %d, Mark test as failed, testResults 0x%x\n",
                                         (pExtBatteryInfo->associatedSp == SP_A) ? "BBUA":"BBUB",
                                         pExtBatteryInfo->slotNumOnSpBlade,
                                         pSpeclBatteryStatus->type,
                                         pSpeclBatteryStatus->type1.testResult);

                       pExtBatteryInfo->batteryTestStatus = FBE_BATTERY_TEST_STATUS_FAILED;
                   }
               }
            }
            else if ((pSpeclBatteryStatus->type == 4) &&
                (pSpeclBatteryStatus->type4.testState == BMC_BIST_FAILED))
            {
                if(pExtBatteryInfo->batteryTestStatus != FBE_BATTERY_TEST_STATUS_FAILED)
                {
                    fbe_base_object_trace((fbe_base_object_t *)base_board,
                                         FBE_TRACE_LEVEL_INFO,
                                         FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s%d, type %d, Mark test as failed, testState 0x%x\n",
                                         (pExtBatteryInfo->associatedSp == SP_A) ? "BBUA":"BBUB",
                                         pExtBatteryInfo->slotNumOnSpBlade,
                                         pSpeclBatteryStatus->type,
                                         pSpeclBatteryStatus->type4.testState);

                    pExtBatteryInfo->batteryTestStatus = FBE_BATTERY_TEST_STATUS_FAILED;
                }
            }
            else
            {
                if(pExtBatteryInfo->batteryTestStatus != FBE_BATTERY_TEST_STATUS_COMPLETED)
                {
                    fbe_base_object_trace((fbe_base_object_t *)base_board,
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s%d, type %d, Mark test as completed, testResults 0x%x\n",
                                          (pExtBatteryInfo->associatedSp == SP_A) ? "BBUA":"BBUB",
                                          pExtBatteryInfo->slotNumOnSpBlade,
                                          pSpeclBatteryStatus->type,
                                          (pSpeclBatteryStatus->type == 1) ?
                                            pSpeclBatteryStatus->type1.testResult : 0);

                    pExtBatteryInfo->batteryTestStatus = FBE_BATTERY_TEST_STATUS_COMPLETED;
                }
            }
        }
        else if (tmpBatteryTestState == BMC_BIST_RUNNING)
        {
            pExtBatteryInfo->batteryTestStatus = FBE_BATTERY_TEST_STATUS_STARTED;
        }
        else
        {
            pExtBatteryInfo->batteryTestStatus = FBE_BATTERY_TEST_STATUS_NONE;
        }

        // set Battery Faults
        if (pSpeclBatteryStatus->internalFailure)
        {
            pExtBatteryInfo->batteryFaults = FBE_BATTERY_FAULT_INTERNAL_FAILURE;
        }
        else if (pSpeclBatteryStatus->lowCharge)
        {
            pExtBatteryInfo->batteryFaults = FBE_BATTERY_FAULT_LOW_CHARGE;
        }
        else if (pExtBatteryInfo->batteryTestStatus == FBE_BATTERY_TEST_STATUS_FAILED)
        {
            pExtBatteryInfo->batteryFaults = FBE_BATTERY_FAULT_TEST_FAILED;
        }
        else
        {
            /* Do not set to FBE_BATTERY_FAULT_NO_FAULT. Let it keep the old value.
             * The reason is that if the BBU has self test fault and the SP sents another self test command,
             * We don't want to see the BBU changed to NO fault. 
             */
        }
    }
    else if(pSpeclBatteryStatus->transactionStatus == STATUS_SMB_DEVICE_NOT_PRESENT)
    {
        pExtBatteryInfo->transactionStatus = pSpeclBatteryStatus->transactionStatus;
        pExtBatteryInfo->batteryInserted = FALSE;
    }
    else
    {
        /* pSpeclBatteryStatus->transactionStatus is bad. 
         * no need to do anything, it will continue to use the old values. 
         */  
    }

// debug ktracing
    if (previousBatteryInfo.batteryInserted != pExtBatteryInfo->batteryInserted)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s%d, type %d, batteryInserted change %d to %d\n",
                              (pExtBatteryInfo->associatedSp == SP_A) ? "BBUA":"BBUB",
                              pExtBatteryInfo->slotNumOnSpBlade,
                              pSpeclBatteryStatus->type,
                              previousBatteryInfo.batteryInserted,
                              pExtBatteryInfo->batteryInserted);
    }
    if (previousBatteryInfo.batteryState != pExtBatteryInfo->batteryState)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s%d, type %d, batteryState change %d to %d\n",
                              (pExtBatteryInfo->associatedSp == SP_A) ? "BBUA":"BBUB",
                              pExtBatteryInfo->slotNumOnSpBlade,
                              pSpeclBatteryStatus->type,
                              previousBatteryInfo.batteryState,
                              pExtBatteryInfo->batteryState);
    }
    if (previousBatteryInfo.batteryChargeState != pExtBatteryInfo->batteryChargeState)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s%d, type %d,  batteryChargeState change %d to %d\n",
                              (pExtBatteryInfo->associatedSp == SP_A) ? "BBUA":"BBUB",
                              pExtBatteryInfo->slotNumOnSpBlade,
                              pSpeclBatteryStatus->type,
                              previousBatteryInfo.batteryChargeState,
                              pExtBatteryInfo->batteryChargeState);
    }
    if (previousBatteryInfo.batteryFaults != pExtBatteryInfo->batteryFaults)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s%d, type %d, batteryFaults change %d to %d\n",
                              (pExtBatteryInfo->associatedSp == SP_A) ? "BBUA":"BBUB",
                              pExtBatteryInfo->slotNumOnSpBlade,
                              pSpeclBatteryStatus->type,
                              previousBatteryInfo.batteryFaults,
                              pExtBatteryInfo->batteryFaults);
    }
    if (previousBatteryInfo.batteryTestStatus != pExtBatteryInfo->batteryTestStatus)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s%d, type %d, tmpBatTestState %d, prevTestStatus %d, currTestStatus %d, batFaults %d.\n",
                         (pExtBatteryInfo->associatedSp == SP_A) ? "BBUA":"BBUB",
                         pExtBatteryInfo->slotNumOnSpBlade,
                         pSpeclBatteryStatus->type,
                         tmpBatteryTestState,
                         previousBatteryInfo.batteryTestStatus,
                         pExtBatteryInfo->batteryTestStatus,
                         pExtBatteryInfo->batteryFaults);

    }
    
    return FBE_STATUS_OK;

}   // end of fbe_base_board_translate_battery_status


/*!*************************************************************************
 *  @fn fbe_base_board_translate_battery_resume
 **************************************************************************
 *  @brief
 *      This function translates SPECL SPS Resume PROM to physical package
 *      SPS Resume PROM info. 
 *
 *  @param base_board- The pointer to base board object.
 *  @param pPeSpsSubInfo- The pointer to edal_pe_sps_sub_info_t. 
 *  @param pSpeclSpsResume - The pointer to SPECL_SPS_RESUME.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    23-Feb-2011: Joe Perry - Created.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_board_translate_battery_resume(fbe_base_board_t * base_board,
                                        edal_pe_battery_sub_info_t * pPeBatterySubInfo,     
                                        SPECL_RESUME_DATA * pSpeclBatteryResume)
{
    fbe_base_battery_info_t *pExtBatteryInfo = NULL;

    pExtBatteryInfo = &(pPeBatterySubInfo->externalBatteryInfo);
// JAP_BOB - needed?
//    pExtBatteryInfo->batteryResumePromInfo = *pSpeclBatteryResume;

    return FBE_STATUS_OK;

}   // end of fbe_base_board_translate_battery_resume


/*!*************************************************************************
 *  @fn fbe_base_board_read_cooling_status
 **************************************************************************
 *  @brief
 *      This function gets the Cooling status and 
 *          fills in EDAL with the status.
 *
 *  @param base_board- The pointer to the base board object.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  NOTES:
 *
 *  HISTORY:
 *    21-Apr-2010: Arun S - Created.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_board_read_cooling_status(fbe_base_board_t * base_board)
{
    SP_ID                sp;
    fbe_u32_t           fanPack = 0;
    fbe_u32_t           componentIndex = 0;
    fbe_u32_t           componentCount = 0;
    fbe_u32_t           fan_fault_count = 0;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_edal_status_t   edalStatus = FBE_EDAL_STATUS_OK;


    SPECL_FAN_SUMMARY              * pSpeclFanSum = NULL;
    edal_pe_cooling_sub_info_t       peCoolingSubInfo = {0};      
    edal_pe_cooling_sub_info_t     * pPeCoolingSubInfo = &peCoolingSubInfo;
    fbe_edal_block_handle_t          edalBlockHandle = NULL;   

    fbe_base_board_get_edal_block_handle(base_board, &edalBlockHandle);

    /* Get the total count of the blowers. */
    edalStatus = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                                                     FBE_ENCL_COOLING_COMPONENT, 
                                                     &componentCount);

    if(edalStatus != FBE_EDAL_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    if(componentCount == 0)
    {
        return FBE_STATUS_OK;
    }

    pSpeclFanSum = (SPECL_FAN_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclFanSum, sizeof(SPECL_FAN_SUMMARY));

    status = fbe_base_board_pe_get_cooling_status(pSpeclFanSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }
    
    componentIndex = 0;

    FBE_FOR_ALL_FAN_PACKS(sp, fanPack, pSpeclFanSum)
    {
        fbe_zero_memory(pPeCoolingSubInfo, sizeof(edal_pe_cooling_sub_info_t)); 

        /* Get old Cooling info from EDAL. */
        edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_COOLING_INFO,  //Attribute
                                        FBE_ENCL_COOLING_COMPONENT,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_cooling_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeCoolingSubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edalStatus))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }  

        pPeCoolingSubInfo->externalCoolingInfo.slotNumOnEncl = componentIndex;
        pPeCoolingSubInfo->externalCoolingInfo.slotNumOnSpBlade = fanPack;
        pPeCoolingSubInfo->externalCoolingInfo.associatedSp = sp;

        /* Handle the Stale data and SMB errors */
        status = fbe_base_board_check_env_interface_error(base_board,
                                                           FBE_ENCL_COOLING_COMPONENT,
                                                           pSpeclFanSum->fanSummary[sp].fanPackStatus[fanPack].transactionStatus,
                                                           pSpeclFanSum->fanSummary[sp].fanPackStatus[fanPack].timeStamp.QuadPart,
                                                           &pPeCoolingSubInfo->timeStampForFirstBadStatus,
                                                           &pPeCoolingSubInfo->externalCoolingInfo.envInterfaceStatus);

        if(status != FBE_STATUS_OK)
        {
            return status;  
        }           

        fbe_base_board_translate_cooling_status(pPeCoolingSubInfo, pSpeclFanSum);

        /* See if the fanModuleFault bit is SET. If so, increment the counter 
         * to track the no. of faulted fans.
         */
        if (pPeCoolingSubInfo->externalCoolingInfo.fanFaulted)
        {
            fan_fault_count++;
        }

        /* Update EDAL with new Cooling info. */
        edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_COOLING_INFO,  //Attribute
                                        FBE_ENCL_COOLING_COMPONENT,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_cooling_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeCoolingSubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edalStatus))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }  

        componentIndex++;

        if(componentIndex >= componentCount)
        {
            break;
        }

    }
    
    return status;
}
    
/*!*************************************************************************
 *  @fn fbe_base_board_translate_cooling_status
 **************************************************************************
 *  @brief
 *      This function translates SPECL cooling status. 
 *
 *  @param pBlowerInfo- The pointer to edal_pe_cooling_sub_info_t.
 *  @param pSpeclFanSum - The pointer to SPECL_FAN_SUMMARY.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    21-Apr-2010: Arun S - Created.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_board_translate_cooling_status(edal_pe_cooling_sub_info_t * pPeCoolingSubInfo,                                      
                                             SPECL_FAN_SUMMARY  * pSpeclFanSum)
{
    fbe_cooling_info_t * pExtCoolingInfo = NULL;
    fbe_u8_t                 fanNum=0;

    pExtCoolingInfo = &pPeCoolingSubInfo->externalCoolingInfo;
    pExtCoolingInfo->inProcessorEncl = TRUE;
    pExtCoolingInfo->fanDegraded = FBE_MGMT_STATUS_FALSE;
    pExtCoolingInfo->bDownloadable = FALSE;

    pExtCoolingInfo->transactionStatus = pSpeclFanSum->fanSummary[pExtCoolingInfo->associatedSp].fanPackStatus[pExtCoolingInfo->slotNumOnSpBlade].transactionStatus;

    if ((pExtCoolingInfo->envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD) ||
        (pExtCoolingInfo->transactionStatus == STATUS_SMB_DEVICE_NOT_PRESENT))
    {
        /* env interface is no good. We are not able to get any status and the old values are also timed out.
         */
        if(pExtCoolingInfo->transactionStatus == STATUS_SMB_DEVICE_NOT_PRESENT)
        {
            pExtCoolingInfo->inserted = FBE_MGMT_STATUS_FALSE;
        }
        else
        {
            pExtCoolingInfo->inserted = FBE_MGMT_STATUS_UNKNOWN;
        }
        pExtCoolingInfo->fanFaulted = FBE_MGMT_STATUS_UNKNOWN;
        pExtCoolingInfo->multiFanModuleFault = FBE_MGMT_STATUS_UNKNOWN;
        pExtCoolingInfo->uniqueId = NO_MODULE;
        memset(pExtCoolingInfo->firmwareRev, 0, sizeof(pExtCoolingInfo->firmwareRev));
    }
    else if(pSpeclFanSum->fanSummary[pExtCoolingInfo->associatedSp].fanPackStatus[pExtCoolingInfo->slotNumOnSpBlade].transactionStatus == EMCPAL_STATUS_SUCCESS)
    {  
        pExtCoolingInfo->transactionStatus = pSpeclFanSum->fanSummary[pExtCoolingInfo->associatedSp].fanPackStatus[pExtCoolingInfo->slotNumOnSpBlade].transactionStatus;

        if (((pSpeclFanSum->fanSummary[pExtCoolingInfo->associatedSp].fanPackStatus[pExtCoolingInfo->slotNumOnSpBlade].type == 2) &&
             pSpeclFanSum->fanSummary[pExtCoolingInfo->associatedSp].fanPackStatus[pExtCoolingInfo->slotNumOnSpBlade].type2.inserted) ||
            ((pSpeclFanSum->fanSummary[pExtCoolingInfo->associatedSp].fanPackStatus[pExtCoolingInfo->slotNumOnSpBlade].type == 3) &&
             pSpeclFanSum->fanSummary[pExtCoolingInfo->associatedSp].fanPackStatus[pExtCoolingInfo->slotNumOnSpBlade].type3.inserted))
        {
            pExtCoolingInfo->inserted = FBE_MGMT_STATUS_TRUE;
        }
        else if (((pSpeclFanSum->fanSummary[pExtCoolingInfo->associatedSp].fanPackStatus[pExtCoolingInfo->slotNumOnSpBlade].type == 2) &&
                  !(pSpeclFanSum->fanSummary[pExtCoolingInfo->associatedSp].fanPackStatus[pExtCoolingInfo->slotNumOnSpBlade].type2.inserted)) ||
                 ((pSpeclFanSum->fanSummary[pExtCoolingInfo->associatedSp].fanPackStatus[pExtCoolingInfo->slotNumOnSpBlade].type == 3) &&
                  !(pSpeclFanSum->fanSummary[pExtCoolingInfo->associatedSp].fanPackStatus[pExtCoolingInfo->slotNumOnSpBlade].type3.inserted)))
        {
            pExtCoolingInfo->inserted = FBE_MGMT_STATUS_FALSE;
        }
        else
        {
            /* For some platforms, we don't know whether the Fan is inserted or NOT. Assume it is inserted. 
             * Otherwise, Navi will display the fan as faulted.
             */
            pExtCoolingInfo->inserted = FBE_MGMT_STATUS_TRUE;
        }

        /* 
         * Originally we OR'd inletUnderspeed, inletOverspeed powerUpFailure for fanFault.
         * However, inletOverspeed does not necessarily require the fan replacement.
         * SPECL sets fanFault based on FSR. We only need to get the fan fault from SPECL fanFault field.
         */    
        pExtCoolingInfo->fanFaulted = FBE_MGMT_STATUS_FALSE;
        for (fanNum = 0; fanNum < pSpeclFanSum->fanSummary[pExtCoolingInfo->associatedSp].fanPackStatus[pExtCoolingInfo->slotNumOnSpBlade].fanCount; fanNum++)
        {
            if (pSpeclFanSum->fanSummary[pExtCoolingInfo->associatedSp].fanPackStatus[pExtCoolingInfo->slotNumOnSpBlade].fanStatus[fanNum].fanFault)
            {
                pExtCoolingInfo->fanFaulted = FBE_MGMT_STATUS_TRUE;
                break;
            }
        }

        pExtCoolingInfo->multiFanModuleFault = (pSpeclFanSum->fanSummary[pExtCoolingInfo->associatedSp].multFanFault == TRUE) ? 
                                            FBE_MGMT_STATUS_TRUE : FBE_MGMT_STATUS_FALSE;


        if (pSpeclFanSum->fanSummary[pExtCoolingInfo->associatedSp].fanPackStatus[pExtCoolingInfo->slotNumOnSpBlade].type == 2)
        {
            sprintf(pExtCoolingInfo->firmwareRev, "%02d%02d", pSpeclFanSum->fanSummary[pExtCoolingInfo->associatedSp].fanPackStatus[pExtCoolingInfo->slotNumOnSpBlade].type2.firmwareRevMajor, 
                                                              pSpeclFanSum->fanSummary[pExtCoolingInfo->associatedSp].fanPackStatus[pExtCoolingInfo->slotNumOnSpBlade].type2.firmwareRevMinor);

            pExtCoolingInfo->uniqueId =  pSpeclFanSum->fanSummary[pExtCoolingInfo->associatedSp].fanPackStatus[pExtCoolingInfo->slotNumOnSpBlade].type2.uniqueID;

            pExtCoolingInfo->hasResumeProm = FBE_TRUE;
        }
        else
        {
            sprintf(pExtCoolingInfo->firmwareRev, "0000");
        }
    }
    else
    {
        /* If envInterfaceStatus is FBE_ENV_INTERFACE_STATUS_GOOD, 
         * no need to do anything, it will continue to use the old values. 
         */     
    }
 
    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_board_read_mgmt_module_status
 **************************************************************************
 *  @brief
 *      This function gets the Management module status and 
 *          fills in EDAL with the status.
 *
 *  @param base_board- The pointer to the base board object.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  NOTES:
 *
 *  HISTORY:
 *    08-Aor-2010: Arun S - Created.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_board_read_mgmt_module_status(fbe_base_board_t * base_board)
{
    SP_ID               sp;
    fbe_u32_t           mgmtModule;
    fbe_u32_t           componentIndex;
    fbe_u32_t           componentCount = 0;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_edal_status_t   edalStatus = FBE_EDAL_STATUS_OK;

    SPECL_MGMT_SUMMARY                * pSpeclMgmtSum = NULL;
    SPECL_MGMT_STATUS                 * pSpeclMgmtStatus = NULL;
    edal_pe_mgmt_module_sub_info_t      peMgmtModuleSubInfo = {0};      
    edal_pe_mgmt_module_sub_info_t    * pPeMgmtModuleSubInfo = &peMgmtModuleSubInfo;
    fbe_board_mgmt_mgmt_module_info_t * pExtMgmtModuleInfo = NULL;
    fbe_edal_block_handle_t             edalBlockHandle = NULL;   

    fbe_base_board_get_edal_block_handle(base_board, &edalBlockHandle);

    pSpeclMgmtSum = (SPECL_MGMT_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclMgmtSum, sizeof(SPECL_MGMT_SUMMARY));

    status = fbe_base_board_pe_get_mgmt_module_status(pSpeclMgmtSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    /* Get the total count of the management modules. */
    edalStatus = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                                                     FBE_ENCL_MGMT_MODULE, 
                                                     &componentCount);

    if(edalStatus != FBE_EDAL_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    componentIndex = 0;

    FBE_FOR_ALL_MGMT_MODULES(sp, mgmtModule, pSpeclMgmtSum)
    {
        fbe_zero_memory(pPeMgmtModuleSubInfo, sizeof(edal_pe_mgmt_module_sub_info_t)); 

        edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_MGMT_MODULE_INFO,  //Attribute
                                        FBE_ENCL_MGMT_MODULE,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_mgmt_module_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeMgmtModuleSubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edalStatus))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }  

        pExtMgmtModuleInfo = &pPeMgmtModuleSubInfo->externalMgmtModuleInfo;

        pExtMgmtModuleInfo->associatedSp = sp;
        pExtMgmtModuleInfo->mgmtID = mgmtModule;  
        pExtMgmtModuleInfo->isLocalFru = (base_board->spid == sp) ? TRUE : FALSE;  

        pSpeclMgmtStatus = &(pSpeclMgmtSum->mgmtSummary[sp].mgmtStatus[mgmtModule]);

        /* Handle the Stale data and SMB errors */
        status = fbe_base_board_check_env_interface_error(base_board,
                                                           FBE_ENCL_MGMT_MODULE,
                                                           pSpeclMgmtStatus->transactionStatus,
                                                           pSpeclMgmtStatus->timeStamp.QuadPart,
                                                           &pPeMgmtModuleSubInfo->timeStampForFirstBadStatus,
                                                           &pExtMgmtModuleInfo->envInterfaceStatus);

        if(status != FBE_STATUS_OK)
        {
            return status;  
        }           

        fbe_base_board_translate_mgmt_module_status(pPeMgmtModuleSubInfo, pSpeclMgmtStatus);        
    
        /* Update EDAL with new Mgmt info. */
        edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_MGMT_MODULE_INFO,  //Attribute
                                        FBE_ENCL_MGMT_MODULE,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_mgmt_module_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeMgmtModuleSubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edalStatus))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }  

        componentIndex++;

        if(componentIndex >= componentCount)
        {
            break;
        }
    }
    
    return FBE_STATUS_OK;
}
    
/*!*************************************************************************
 *  @fn fbe_base_board_translate_mgmt_module_status
 **************************************************************************
 *  @brief
 *      This function translates SPECL management module status. 
 *
 *  @param pMgmtModuleInfo- The pointer to edal_pe_mgmt_module_sub_info_t.
 *  @param pSpeclMgmtStatus - The pointer to SPECL_MGMT_STATUS.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    21-Apr-2010: Arun S - Created.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_board_translate_mgmt_module_status(edal_pe_mgmt_module_sub_info_t * pPeMgmtModuleSubInfo,                                      
                                            SPECL_MGMT_STATUS  * pSpeclMgmtStatus)
{
    fbe_board_mgmt_mgmt_module_info_t * pExtMgmtModuleInfo = NULL;

    pExtMgmtModuleInfo = &pPeMgmtModuleSubInfo->externalMgmtModuleInfo;
    
    pExtMgmtModuleInfo->uniqueID = pSpeclMgmtStatus->uniqueID;
    pExtMgmtModuleInfo->bInserted = pSpeclMgmtStatus->inserted;

    /* Check env interface status. */   
    if(pSpeclMgmtStatus->transactionStatus == EMCPAL_STATUS_SUCCESS)
    {   
        pExtMgmtModuleInfo->transactionStatus = pSpeclMgmtStatus->transactionStatus;
        pExtMgmtModuleInfo->generalFault = (pSpeclMgmtStatus->generalFault == TRUE) ? FBE_MGMT_STATUS_TRUE : FBE_MGMT_STATUS_FALSE;
        pExtMgmtModuleInfo->faultInfoStatus = (fbe_u8_t)pSpeclMgmtStatus->faultInfoStatus;
        pExtMgmtModuleInfo->faultLEDStatus = pSpeclMgmtStatus->faultLEDStatus;
        pExtMgmtModuleInfo->vLanConfigMode = pSpeclMgmtStatus->vLanConfigMode;  

        pExtMgmtModuleInfo->firmwareRevMinor = pSpeclMgmtStatus->firmwareRevMinor;  
        pExtMgmtModuleInfo->firmwareRevMajor = pSpeclMgmtStatus->firmwareRevMajor;  

        fbe_base_board_translate_mgmt_port_settings(&pExtMgmtModuleInfo->managementPort, &pSpeclMgmtStatus->portStatus[MANAGEMENT_PORT0]);
        pExtMgmtModuleInfo->servicePort = pSpeclMgmtStatus->portStatus[SERVICE_PORT0];    
    }
    else if(pExtMgmtModuleInfo->envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
    {
        pExtMgmtModuleInfo->transactionStatus = pSpeclMgmtStatus->transactionStatus;
        pExtMgmtModuleInfo->generalFault = FBE_MGMT_STATUS_UNKNOWN;
        pExtMgmtModuleInfo->faultInfoStatus = 0;
        pExtMgmtModuleInfo->faultLEDStatus = LED_BLINK_INVALID;
        pExtMgmtModuleInfo->vLanConfigMode = UNKNOWN_VLAN_MODE; 

        pExtMgmtModuleInfo->firmwareRevMinor = pSpeclMgmtStatus->firmwareRevMinor;  
        pExtMgmtModuleInfo->firmwareRevMajor = pSpeclMgmtStatus->firmwareRevMajor;
 
        fbe_base_board_translate_mgmt_port_settings(&pExtMgmtModuleInfo->managementPort, &pSpeclMgmtStatus->portStatus[MANAGEMENT_PORT0]); 
        pExtMgmtModuleInfo->servicePort = pSpeclMgmtStatus->portStatus[SERVICE_PORT0];       
    }
    else
    {
        /* If envInterfaceStatus is FBE_ENV_INTERFACE_STATUS_GOOD, 
         * no need to do anything, it will continue to use the old values. 
         */
    }

    return FBE_STATUS_OK;
}


/*!*************************************************************************
 *  @fn fbe_base_board_translate_mgmt_module_status
 **************************************************************************
 *  @brief
 *      This function translates SPECL management port settings
 *
 *  @param mgmt_port- The pointer to the fbe mgmt port info.
 *  @param specl_mgmt_port - The pointer to the specl mgmt port info.
 *
 *  @return none
 *  @note
 *
 *  @version
 *    25-March-2013: bphilbin - Created.
 *
 **************************************************************************/
void fbe_base_board_translate_mgmt_port_settings(fbe_mgmt_port_status_t *mgmt_port, PORT_STS_CONFIG *specl_mgmt_port)
{
    if(specl_mgmt_port->autoNegotiate)
    {
        mgmt_port->mgmtPortAutoNeg = FBE_PORT_AUTO_NEG_ON;
    }
    else
    {
        mgmt_port->mgmtPortAutoNeg = FBE_PORT_AUTO_NEG_OFF;

    }
    switch(specl_mgmt_port->portDuplex)
    {
        case HALF_DUPLEX:
            mgmt_port->mgmtPortDuplex = FBE_PORT_DUPLEX_MODE_HALF;
            break;
        case FULL_DUPLEX:
            mgmt_port->mgmtPortDuplex = FBE_PORT_DUPLEX_MODE_FULL;
            break;
        default:
            mgmt_port->mgmtPortDuplex = FBE_PORT_DUPLEX_MODE_INDETERMIN;
            break;
    }
    switch(specl_mgmt_port->portSpeed)
    {
        case SPEED_10_Mbps:
            mgmt_port->mgmtPortSpeed = FBE_MGMT_PORT_SPEED_10MBPS;
            break;
        case SPEED_100_Mbps:
            mgmt_port->mgmtPortSpeed = FBE_MGMT_PORT_SPEED_100MBPS;
            break;
        case SPEED_1000_Mbps:
            mgmt_port->mgmtPortSpeed = FBE_MGMT_PORT_SPEED_1000MBPS;
            break;
        default:
            mgmt_port->mgmtPortSpeed = FBE_MGMT_PORT_SPEED_INDETERMIN;
            break;
    }
    mgmt_port->linkStatus = specl_mgmt_port->linkStatus;
    return;
}

/*!*************************************************************************
 *  @fn fbe_base_board_read_flt_reg_status
 **************************************************************************
 *  @brief
 *      This function gets the fault register status and 
 *      fills in EDAL with the status.
 *
 *  @param base_board- The pointer to the base board object.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    23-July-2012: Chengkai Hu - Created.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_board_read_flt_reg_status(fbe_base_board_t * base_board)
{
    SP_ID               sp;
    fbe_u32_t           fltReg;
    fbe_u32_t           componentIndex;
    fbe_u32_t           componentCount = 0;
    SPECL_FLT_EXP_SUMMARY  * pSpeclFltExpSum = NULL;
    SPECL_FAULT_REGISTER_STATUS   * pSpeclFltRegStatus = NULL;
    edal_pe_flt_exp_sub_info_t      peFltExpSubInfo = {0};     
    edal_pe_flt_exp_sub_info_t    * pPeFltExpSubInfo = &peFltExpSubInfo;
    fbe_peer_boot_flt_exp_info_t  * pExtRegExpInfo = NULL;
    fbe_edal_status_t   edalStatus = FBE_EDAL_STATUS_OK;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_edal_block_handle_t edalBlockHandle = NULL;   

    fbe_base_board_get_edal_block_handle(base_board, &edalBlockHandle);

    pSpeclFltExpSum = (SPECL_FLT_EXP_SUMMARY *)base_board->pe_buffer_handle;

    fbe_zero_memory(pSpeclFltExpSum, sizeof(SPECL_FLT_EXP_SUMMARY));

    status = fbe_base_board_pe_get_flt_exp_status(pSpeclFltExpSum);

    if(status != FBE_STATUS_OK)
    {
        return status;  
    }
    
    /* Get the total count of fault expanders. */
    edalStatus = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                                                     FBE_ENCL_FLT_REG, 
                                                     &componentCount);

    if(edalStatus != FBE_EDAL_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    componentIndex = 0;

    FBE_FOR_ALL_FLT_REGS(sp, fltReg, pSpeclFltExpSum)
    {
        fbe_zero_memory(pPeFltExpSubInfo, sizeof(edal_pe_flt_exp_sub_info_t)); 

        /* Get Old info. */
        edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_FLT_REG_INFO,  //Attribute
                                        FBE_ENCL_FLT_REG,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_flt_exp_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeFltExpSubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edalStatus))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }          

        pExtRegExpInfo = &pPeFltExpSubInfo->externalFltExpInfo;

        pExtRegExpInfo->associatedSp = sp;
        pExtRegExpInfo->fltExpID = fltReg; 
        pExtRegExpInfo->fltHwType = FBE_DEVICE_TYPE_FLT_REG;

        pSpeclFltRegStatus = &(pSpeclFltExpSum->fltExpSummary[sp].faultRegisterStatus[fltReg]);

        /* Handle the Stale data and SMB errors */
        status = fbe_base_board_check_env_interface_error(base_board,
                                                           FBE_ENCL_FLT_REG,
                                                           pSpeclFltRegStatus->transactionStatus,
                                                           pSpeclFltRegStatus->timeStamp.QuadPart,
                                                           &pPeFltExpSubInfo->timeStampForFirstBadStatus,
                                                           &pExtRegExpInfo->envInterfaceStatus);
        if(status != FBE_STATUS_OK)
        {
            return status;
        }

        fbe_base_board_translate_flt_reg_status(pPeFltExpSubInfo, pSpeclFltRegStatus);

        /* Update EDAL with new info. */
        edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_FLT_REG_INFO,  //Attribute
                                        FBE_ENCL_FLT_REG,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_flt_exp_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeFltExpSubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edalStatus))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }  

        componentIndex++;

        if(componentIndex >= componentCount)
        {
            break;
        }
    }

    return FBE_STATUS_OK;
} // End of fbe_base_board_read_flt_reg_status

/*!*************************************************************************
 *  @fn fbe_base_board_translate_flt_reg_status
 **************************************************************************
 *  @brief
 *      This function translates SPECL fault register status. 
 *
 *  @param peFltExpInfo- The pointer to edal_pe_flt_exp_sub_info_t.
 *  @param pSpeclFltRegStatus - The pointer to SPECL_FLT_REGISTER_STATUS.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    16-July-2012: Chengkai Hu - Created.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_board_translate_flt_reg_status(edal_pe_flt_exp_sub_info_t * peFltExpSubInfo,                                      
                                        SPECL_FAULT_REGISTER_STATUS  * pSpeclFltRegStatus)
{
    fbe_peer_boot_flt_exp_info_t * pExtFltExpInfo = NULL;
    pExtFltExpInfo = &peFltExpSubInfo->externalFltExpInfo;

    if(pSpeclFltRegStatus->transactionStatus == EMCPAL_STATUS_SUCCESS)
    {
        pExtFltExpInfo->transactionStatus = pSpeclFltRegStatus->transactionStatus;
        fbe_copy_memory(&(pExtFltExpInfo->faultRegisterStatus),
            pSpeclFltRegStatus, sizeof(SPECL_FAULT_REGISTER_STATUS));
        /* no need store timestamp in EDAL */
        fbe_zero_memory(&pExtFltExpInfo->faultRegisterStatus.timeStamp,
            sizeof(pExtFltExpInfo->faultRegisterStatus.timeStamp));
    }
    else if(pExtFltExpInfo->envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD)
    {
        /* If envInterfaceStatus is FBE_ENV_INTERFACE_STATUS_GOOD, 
         * no need to do anything, it will continue to use the old values. 
         */        
    }

    return FBE_STATUS_OK;
}// End of fbe_base_board_translate_flt_reg_status

/*!*************************************************************************
 *  @fn fbe_base_board_read_slave_port_status
 **************************************************************************
 *  @brief
 *      This function gets the slave port status and 
 *      fills in EDAL with the status.
 *
 *  @param base_board- The pointer to the base board object.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    20-Apr-2010: PHE - Created.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_board_read_slave_port_status(fbe_base_board_t * base_board)
{
    SP_ID               sp;
    fbe_u32_t           slavePort;
    fbe_u32_t           componentIndex;
    fbe_u32_t           componentCount = 0;
    SPECL_SLAVE_PORT_SUMMARY  * pSpeclSlavePortSum = NULL;
    SPECL_SLAVE_PORT_STATUS   * pSpeclSlavePortStatus = NULL;
    edal_pe_slave_port_sub_info_t   peSlavePortSubInfo = {0};       
    edal_pe_slave_port_sub_info_t * pPeSlavePortSubInfo = &peSlavePortSubInfo;
    fbe_board_mgmt_slave_port_info_t * pExtSlavePortInfo = NULL;
    fbe_edal_status_t   edalStatus = FBE_EDAL_STATUS_OK;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_edal_block_handle_t edalBlockHandle = NULL;   

    fbe_base_board_get_edal_block_handle(base_board, &edalBlockHandle);

    pSpeclSlavePortSum = (SPECL_SLAVE_PORT_SUMMARY *)base_board->pe_buffer_handle;

    fbe_zero_memory(pSpeclSlavePortSum, sizeof(SPECL_SLAVE_PORT_SUMMARY));

    status = fbe_base_board_pe_get_slave_port_status(pSpeclSlavePortSum);

    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    /* Get the total count of slave ports. */
    edalStatus = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                                                     FBE_ENCL_SLAVE_PORT, 
                                                     &componentCount);

    if(edalStatus != FBE_EDAL_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    componentIndex = 0;

    FBE_FOR_ALL_SLAVE_PORTS(sp, slavePort, pSpeclSlavePortSum)
    {
        fbe_zero_memory(pPeSlavePortSubInfo, sizeof(edal_pe_slave_port_sub_info_t)); 
        /* Get Old slave port info. */
        edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_SLAVE_PORT_INFO,  //Attribute
                                        FBE_ENCL_SLAVE_PORT,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_slave_port_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeSlavePortSubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edalStatus))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }  

        pExtSlavePortInfo = &pPeSlavePortSubInfo->externalSlavePortInfo;
        
        pExtSlavePortInfo->associatedSp = sp;
        pExtSlavePortInfo->slavePortID = slavePort;   
        pExtSlavePortInfo->isLocalFru = (base_board->spid == sp) ? TRUE : FALSE; 

        pSpeclSlavePortStatus = &(pSpeclSlavePortSum->slavePortSummary[sp].slavePortStatus[slavePort]);

        /* Handle the Stale data and SMB errors */
        status = fbe_base_board_check_env_interface_error(base_board,
                                                           FBE_ENCL_SLAVE_PORT,
                                                           pSpeclSlavePortStatus->transactionStatus,
                                                           pSpeclSlavePortStatus->timeStamp.QuadPart,
                                                           &pPeSlavePortSubInfo->timeStampForFirstBadStatus,
                                                           &pExtSlavePortInfo->envInterfaceStatus);
        if(status != FBE_STATUS_OK)
        {
            return status;
        }

        fbe_base_board_translate_slave_port_status(pPeSlavePortSubInfo, pSpeclSlavePortStatus);

        /* Update EDAL with new slave port info. */
        edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_SLAVE_PORT_INFO,  //Attribute
                                        FBE_ENCL_SLAVE_PORT,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_slave_port_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeSlavePortSubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edalStatus))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }  

        componentIndex++;

        if(componentIndex >= componentCount)
        {
            break;
        }
    }
    
    return FBE_STATUS_OK;
} // End of fbe_base_board_read_slave_port_status

/*!*************************************************************************
 *  @fn fbe_base_board_translate_slave_port_status
 **************************************************************************
 *  @brief
 *      This function translates SPECL slave port status. 
 *
 *  @param peFltExpInfo- The pointer to edal_pe_slave_port_sub_info_t.
 *  @param pSpeclSlavePortStatus - The pointer to SPECL_SLAVE_PORT_STATUS.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    20-Apr-2010: PHE - Created.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_board_translate_slave_port_status(edal_pe_slave_port_sub_info_t * peSlavePortSubInfo,                                      
                                           SPECL_SLAVE_PORT_STATUS  * pSpeclSlavePortStatus)
{
    fbe_board_mgmt_slave_port_info_t * pExtSlavePortInfo = NULL;
    pExtSlavePortInfo = &peSlavePortSubInfo->externalSlavePortInfo;

    if(pSpeclSlavePortStatus->transactionStatus == EMCPAL_STATUS_SUCCESS)
    {
        pExtSlavePortInfo->transactionStatus = pSpeclSlavePortStatus->transactionStatus;
        pExtSlavePortInfo->generalStatus = pSpeclSlavePortStatus->generalStatus;
        pExtSlavePortInfo->componentStatus = pSpeclSlavePortStatus->componentStatus;
        pExtSlavePortInfo->componentExtStatus = pSpeclSlavePortStatus->componentExtStatus;
    }
    else if(pExtSlavePortInfo->envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD)
    {
        /* If envInterfaceStatus is FBE_ENV_INTERFACE_STATUS_GOOD, 
         * no need to do anything, it will continue to use the old values. 
         */
    }

    return FBE_STATUS_OK;
}// End of fbe_base_board_translate_slave_port_status



/*!*************************************************************************
 *  @fn fbe_base_board_read_suitcase_status
 **************************************************************************
 *  @brief
 *      This function gets the Suitcase status and 
 *          fills in EDAL with the status.
 *
 *  @param base_board- The pointer to the base board object.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  NOTES:
 *
 *  HISTORY:
 *    08-Apr-2010: Arun S - Created.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_board_read_suitcase_status(fbe_base_board_t * base_board)
{
    SP_ID               sp;
    fbe_u32_t           suitcase;
    fbe_u32_t           componentIndex;
    fbe_u32_t           componentCount = 0;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_edal_status_t   edalStatus = FBE_EDAL_STATUS_OK;

    SPECL_SUITCASE_SUMMARY          * pSpeclSuitcaseSum = NULL;
    SPECL_SUITCASE_STATUS           * pSpeclSuitcaseStatus = NULL;
    edal_pe_suitcase_sub_info_t       peSuitcaseSubInfo = {0};      
    edal_pe_suitcase_sub_info_t     * pPeSuitcaseSubInfo = &peSuitcaseSubInfo;
    fbe_board_mgmt_suitcase_info_t  * pExtSuitcaseInfo = NULL;
    fbe_edal_block_handle_t           edalBlockHandle = NULL;   

    fbe_base_board_get_edal_block_handle(base_board, &edalBlockHandle);

    pSpeclSuitcaseSum = (SPECL_SUITCASE_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclSuitcaseSum, sizeof(SPECL_SUITCASE_SUMMARY));

    status = fbe_base_board_pe_get_suitcase_status(pSpeclSuitcaseSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    /* Get the total count of the Suitcases. */
    edalStatus = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                                                     FBE_ENCL_SUITCASE, 
                                                     &componentCount);

    if(edalStatus != FBE_EDAL_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    componentIndex = 0;

    FBE_FOR_ALL_SUITCASES(sp, suitcase, pSpeclSuitcaseSum)
    {
        fbe_zero_memory(pPeSuitcaseSubInfo, sizeof(edal_pe_suitcase_sub_info_t)); 
        /* Get old suit case info. */
        edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_SUITCASE_INFO,  //Attribute
                                        FBE_ENCL_SUITCASE,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_suitcase_sub_info_t),  // buffer length
                                       (fbe_u8_t *)pPeSuitcaseSubInfo);  // buffer pointer 

        if(!EDAL_STAT_OK(edalStatus))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }  

        pExtSuitcaseInfo = &pPeSuitcaseSubInfo->externalSuitcaseInfo;

        pExtSuitcaseInfo->associatedSp = sp;
        pExtSuitcaseInfo->suitcaseID = suitcase;
        pExtSuitcaseInfo->isLocalFru = (base_board->spid == sp) ? TRUE : FALSE;

        pSpeclSuitcaseStatus = &(pSpeclSuitcaseSum->suitcaseSummary[sp].suitcaseStatus[suitcase]);

        /* Handle the Stale data and SMB errors */
        status = fbe_base_board_check_env_interface_error(base_board,
                                                           FBE_ENCL_SUITCASE,
                                                           pSpeclSuitcaseStatus->transactionStatus,
                                                           pSpeclSuitcaseStatus->timeStamp.QuadPart,
                                                           &pPeSuitcaseSubInfo->timeStampForFirstBadStatus,
                                                           &pExtSuitcaseInfo->envInterfaceStatus);

        if(status != FBE_STATUS_OK)
        {
            return status;  
        }           

        fbe_base_board_translate_suitcase_status(pPeSuitcaseSubInfo, pSpeclSuitcaseStatus);
    
        /* Update EDAL with new suitcase info. */
        edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_SUITCASE_INFO,  //Attribute
                                        FBE_ENCL_SUITCASE,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_suitcase_sub_info_t),  // buffer length
                                       (fbe_u8_t *)pPeSuitcaseSubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edalStatus))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }  

        componentIndex++;

        if(componentIndex >= componentCount)
        {
            break;
        }
    }
    
    return FBE_STATUS_OK;
}
    
/*!*************************************************************************
 *  @fn fbe_base_board_translate_suitcase_status
 **************************************************************************
 *  @brief
 *      This function translates SPECL Suitcase status. 
 *
 *  @param pSuitcaseInfo- The pointer to edal_pe_suitcase_sub_info_t.
 *  @param pSpeclSuitcaseStatus - The pointer to SPECL_SUITCASE_STATUS.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    21-Apr-2010: Arun S - Created.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_board_translate_suitcase_status(edal_pe_suitcase_sub_info_t * pSuitcaseSubInfo,                                      
                                             SPECL_SUITCASE_STATUS  * pSpeclSuitcaseStatus)
{
    fbe_board_mgmt_suitcase_info_t * pExtSuitcaseInfo = NULL;

    pExtSuitcaseInfo = &pSuitcaseSubInfo->externalSuitcaseInfo;

    if(pSpeclSuitcaseStatus->transactionStatus == EMCPAL_STATUS_SUCCESS)
    {
        pExtSuitcaseInfo->transactionStatus = pSpeclSuitcaseStatus->transactionStatus;
        pExtSuitcaseInfo->Tap12VMissing = (pSpeclSuitcaseStatus->Tap12VMissing == TRUE) ?
                                               FBE_MGMT_STATUS_TRUE : FBE_MGMT_STATUS_FALSE;
        pExtSuitcaseInfo->shutdownWarning = (pSpeclSuitcaseStatus->shutdownWarning == TRUE) ?
                                               FBE_MGMT_STATUS_TRUE : FBE_MGMT_STATUS_FALSE;    
        pExtSuitcaseInfo->ambientOvertempFault = (pSpeclSuitcaseStatus->ambientOvertempFault == TRUE) ?
                                               FBE_MGMT_STATUS_TRUE : FBE_MGMT_STATUS_FALSE;    
        pExtSuitcaseInfo->ambientOvertempWarning = (pSpeclSuitcaseStatus->ambientOvertempWarning == TRUE) ?
                                               FBE_MGMT_STATUS_TRUE : FBE_MGMT_STATUS_FALSE;    
        pExtSuitcaseInfo->fanPackFailure = (pSpeclSuitcaseStatus->fanPackFailure == TRUE) ?
                                               FBE_MGMT_STATUS_TRUE : FBE_MGMT_STATUS_FALSE;
        /*tempSensor is fbe_u8_t */
        pExtSuitcaseInfo->tempSensor = pSpeclSuitcaseStatus->tempSensor;    
    }
    else if(pExtSuitcaseInfo->envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
    {
        pExtSuitcaseInfo->transactionStatus = pSpeclSuitcaseStatus->transactionStatus;
        pExtSuitcaseInfo->Tap12VMissing = FBE_MGMT_STATUS_UNKNOWN;
        pExtSuitcaseInfo->shutdownWarning = FBE_MGMT_STATUS_UNKNOWN;    
        pExtSuitcaseInfo->ambientOvertempFault = FBE_MGMT_STATUS_UNKNOWN;   
        pExtSuitcaseInfo->fanPackFailure = FBE_MGMT_STATUS_UNKNOWN; 
        /*tempSensor is fbe_u8_t */
        pExtSuitcaseInfo->tempSensor = pSpeclSuitcaseStatus->tempSensor;            
    }
    else
    {
        /* If envInterfaceStatus is FBE_ENV_INTERFACE_STATUS_GOOD, 
         * no need to do anything, it will continue to use the old values. 
         */
    }

    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_board_read_bmc_status
 **************************************************************************
 *  @brief
 *      This function gets the BMC status and 
 *          fills in EDAL with the status.
 *
 *  @param base_board- The pointer to the base board object.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  NOTES:
 *
 *  HISTORY:
 *    11-Sep-2012  Eric Zhou - Created.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_board_read_bmc_status(fbe_base_board_t * base_board)
{
    SP_ID               sp;
    fbe_u32_t           bmc;
    fbe_u32_t           componentIndex;
    fbe_u32_t           componentCount = 0;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_edal_status_t   edal_status = FBE_EDAL_STATUS_OK;

    SPECL_BMC_SUMMARY         * pSpeclBmcSum = NULL;
    SPECL_BMC_STATUS          * pSpeclBmcStatus = NULL;
    edal_pe_bmc_sub_info_t      peBmcSubInfo = {0};     
    edal_pe_bmc_sub_info_t    * pPeBmcSubInfo = &peBmcSubInfo;
    fbe_board_mgmt_bmc_info_t * pExtBmcInfo = NULL;
    fbe_edal_block_handle_t     edalBlockHandle = NULL;   

    fbe_base_board_get_edal_block_handle(base_board, &edalBlockHandle);

    pSpeclBmcSum = (SPECL_BMC_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclBmcSum, sizeof(SPECL_BMC_SUMMARY));

    status = fbe_base_board_pe_get_bmc_status(pSpeclBmcSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    /* Get the total count of the BMCs. */
    edal_status = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                                                     FBE_ENCL_BMC, 
                                                     &componentCount);

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    componentIndex = 0;

    FBE_FOR_ALL_BMC(sp, bmc, pSpeclBmcSum)
    {
        fbe_zero_memory(pPeBmcSubInfo, sizeof(edal_pe_bmc_sub_info_t)); 

        /* Get old BMC info. */
        edal_status = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_BMC_INFO,  //Attribute
                                        FBE_ENCL_BMC,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_bmc_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeBmcSubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edal_status))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }    

        pExtBmcInfo = &pPeBmcSubInfo->externalBmcInfo;

        pExtBmcInfo->associatedSp = sp;
        pExtBmcInfo->bmcID = bmc;
        pExtBmcInfo->isLocalFru = (base_board->spid == sp) ? TRUE : FALSE;

        pSpeclBmcStatus = &(pSpeclBmcSum->bmcSummary[sp].bmcStatus[bmc]);

        /* Handle the Stale data and SMB errors */
        status = fbe_base_board_check_env_interface_error(base_board,
                                                           FBE_ENCL_BMC,
                                                           pSpeclBmcStatus->transactionStatus,
                                                           pSpeclBmcStatus->timeStamp.QuadPart,
                                                           &pPeBmcSubInfo->timeStampForFirstBadStatus,
                                                           &pExtBmcInfo->envInterfaceStatus);

        if(status != FBE_STATUS_OK)
        {
            return status;
        }

        fbe_base_board_translate_bmc_status(pPeBmcSubInfo, pSpeclBmcStatus);
    
        /* Get EDAL with new BMC info. */
        edal_status = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_BMC_INFO,  //Attribute
                                        FBE_ENCL_BMC,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_bmc_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeBmcSubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edal_status))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }  

        componentIndex++;

        if(componentIndex >= componentCount)
        {
            break;
        }
    }
    
    return FBE_STATUS_OK;

}

/*!*************************************************************************
 *  @fn fbe_base_board_translate_bmc_status
 **************************************************************************
 *  @brief
 *      This function translates SPECL BMC status. 
 *
 *  @param pBmcInfo- The pointer to edal_pe_bmc_sub_info_t.
 *  @param pSpeclBmcStatus - The pointer to SPECL_BMC_STATUS.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    11-Sep-2012  Eric Zhou - Created.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_board_translate_bmc_status(edal_pe_bmc_sub_info_t * pBmcSubInfo,
                                             SPECL_BMC_STATUS  * pSpeclBmcStatus)
{
    fbe_board_mgmt_bmc_info_t * pExtBmcInfo = NULL;
    fbe_u32_t                   index;

    pExtBmcInfo = &pBmcSubInfo->externalBmcInfo;

    if(pSpeclBmcStatus->transactionStatus == EMCPAL_STATUS_SUCCESS)
    {
        pExtBmcInfo->transactionStatus = pSpeclBmcStatus->transactionStatus;
        pExtBmcInfo->shutdownWarning = pSpeclBmcStatus->shutdownWarning;
        pExtBmcInfo->shutdownReason = pSpeclBmcStatus->shutdownReason;
        pExtBmcInfo->shutdownTimeRemaining = pSpeclBmcStatus->shutdownTimeRemaining;

        if (pSpeclBmcStatus->type == 1) 
        {
            pExtBmcInfo->rideThroughTimeSupported = FALSE;
            pExtBmcInfo->firmwareStatus = pSpeclBmcStatus->type1.firmwareStatus;
            // init BIST to PASSING (not applicable for this type)
            fbe_base_board_initBmcBistStatus(&pExtBmcInfo->bistReport);
        }
        else if (pSpeclBmcStatus->type == 2)
        {
            pExtBmcInfo->rideThroughTimeSupported = FALSE;
            pExtBmcInfo->firmwareStatus = pSpeclBmcStatus->type2.firmwareStatus;
            // init BIST to PASSING (not applicable for this type)
            fbe_base_board_initBmcBistStatus(&pExtBmcInfo->bistReport);
        }
        else if (pSpeclBmcStatus->type == 3)
        {
            pExtBmcInfo->rideThroughTimeSupported = TRUE;
            pExtBmcInfo->lowPowerMode = pSpeclBmcStatus->type3.lowPowerMode;
            pExtBmcInfo->rideThroughTime = pSpeclBmcStatus->type3.rideThroughTime;
            pExtBmcInfo->vaultTimeout = pSpeclBmcStatus->type3.vaultTimeout;

            pExtBmcInfo->bistReport.cpuTest = pSpeclBmcStatus->type3.bistReport.cpuTest.overallStatus;
            pExtBmcInfo->bistReport.dramTest = pSpeclBmcStatus->type3.bistReport.dramTest.overallStatus;
            pExtBmcInfo->bistReport.sramTest = pSpeclBmcStatus->type3.bistReport.sramTest.overallStatus;
            for (index = 0; index < I2C_TEST_MAX; index++)
            {
                pExtBmcInfo->bistReport.i2cTests[index] = 
                    pSpeclBmcStatus->type3.bistReport.i2cTests[index].overallStatus;
            }
            for (index = 0; index < UART_TEST_MAX; index++)
            {
                pExtBmcInfo->bistReport.uartTests[index] = 
                    pSpeclBmcStatus->type3.bistReport.uartTests[index].overallStatus;
            }
            pExtBmcInfo->bistReport.sspTest = pSpeclBmcStatus->type3.bistReport.sspTest.overallStatus;
            for (index = 0; index < BBU_TEST_MAX; index++)
            {
                pExtBmcInfo->bistReport.bbuTests[index] = 
                    pSpeclBmcStatus->type3.bistReport.bbuTests[index].overallStatus;
            }
            pExtBmcInfo->bistReport.nvramTest = pSpeclBmcStatus->type3.bistReport.nvramTest.overallStatus;
            pExtBmcInfo->bistReport.sgpioTest = pSpeclBmcStatus->type3.bistReport.sgpioTest.overallStatus;
            for (index = 0; index < FAN_TEST_MAX; index++)
            {
                pExtBmcInfo->bistReport.fanTests[index] = 
                    pSpeclBmcStatus->type3.bistReport.fanTests[index].overallStatus;
            }
            for (index = 0; index < ARB_TEST_MAX; index++)
            {
                pExtBmcInfo->bistReport.arbTests[index] = 
                    pSpeclBmcStatus->type3.bistReport.arbTests[index].overallStatus;
            }
            pExtBmcInfo->bistReport.dedicatedLanTest = pSpeclBmcStatus->type3.bistReport.dedicatedLanTest.overallStatus;
            pExtBmcInfo->bistReport.ncsiLanTest = pSpeclBmcStatus->type3.bistReport.ncsiLanTest.overallStatus;
        }

    }
    else
    {
        /* If envInterfaceStatus is FBE_ENV_INTERFACE_STATUS_GOOD, 
         * no need to do anything, it will continue to use the old values. 
         */
    }

    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_board_read_cache_card_status
 **************************************************************************
 *  @brief
 *      This function gets the cache_card status and 
 *          fills in EDAL with the status.
 *
 *  @param base_board- The pointer to the base board object.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  NOTES:
 *
 *  HISTORY:
 *    25-Jan-2013  Rui Chang - Created.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_board_read_cache_card_status(fbe_base_board_t * base_board)
{
    fbe_u32_t           cacheCardIndex;
    fbe_u32_t           componentIndex;
    fbe_u32_t           componentCount = 0;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_edal_status_t   edal_status = FBE_EDAL_STATUS_OK;

    SPECL_CACHE_CARD_SUMMARY         * pSpeclCacheCardSum = NULL;
    SPECL_CACHE_CARD_STATUS          * pSpeclCacheCardStatus = NULL;
    edal_pe_cache_card_sub_info_t      peCacheCardSubInfo = {0};     
    edal_pe_cache_card_sub_info_t    * pPeCacheCardSubInfo = &peCacheCardSubInfo;
    fbe_board_mgmt_cache_card_info_t * pExtCacheCardInfo = NULL;
    fbe_edal_block_handle_t     edalBlockHandle = NULL;   

    fbe_base_board_get_edal_block_handle(base_board, &edalBlockHandle);

    pSpeclCacheCardSum = (SPECL_CACHE_CARD_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclCacheCardSum, sizeof(SPECL_CACHE_CARD_SUMMARY));

    status = fbe_base_board_pe_get_cache_card_status(pSpeclCacheCardSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    /* Get the total count of the Cache Card. */
    edal_status = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                                                     FBE_ENCL_CACHE_CARD, 
                                                     &componentCount);

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    componentIndex = 0;

    FBE_FOR_ALL_CACHE_CARD(cacheCardIndex, pSpeclCacheCardSum)
    {
        fbe_zero_memory(pPeCacheCardSubInfo, sizeof(edal_pe_cache_card_sub_info_t)); 

        /* Get old cache card info. */
        edal_status = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_CACHE_CARD_INFO,  //Attribute
                                        FBE_ENCL_CACHE_CARD,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_cache_card_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeCacheCardSubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edal_status))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }    

        pExtCacheCardInfo = &pPeCacheCardSubInfo->externalCacheCardInfo;
        //currently, only beachcomber can have cache_card cache card, and it is only valid in single SP configuration.
        pExtCacheCardInfo->associatedSp = SP_A;
        pExtCacheCardInfo->cacheCardID = cacheCardIndex;
        pExtCacheCardInfo->isLocalFru = TRUE;

        pSpeclCacheCardStatus = &(pSpeclCacheCardSum->cacheCardSummary[cacheCardIndex]);

        /* Handle the Stale data and SMB errors */
        status = fbe_base_board_check_env_interface_error(base_board,
                                                           FBE_ENCL_CACHE_CARD,
                                                           pSpeclCacheCardStatus->transactionStatus,
                                                           pSpeclCacheCardStatus->timeStamp.QuadPart,
                                                           &pPeCacheCardSubInfo->timeStampForFirstBadStatus,
                                                           &pExtCacheCardInfo->envInterfaceStatus);

        if(status != FBE_STATUS_OK)
        {
            return status;
        }

        fbe_base_board_translate_cache_card_status(pPeCacheCardSubInfo, pSpeclCacheCardStatus);
    
        /* Get EDAL with new CacheCard info. */
        edal_status = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_CACHE_CARD_INFO,  //Attribute
                                        FBE_ENCL_CACHE_CARD,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_cache_card_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeCacheCardSubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edal_status))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }  

        componentIndex++;

        if(componentIndex >= componentCount)
        {
            break;
        }
    }
    
    return FBE_STATUS_OK;

}

/*!*************************************************************************
 *  @fn fbe_base_board_translate_cache_card_status
 **************************************************************************
 *  @brief
 *      This function translates SPECL CacheCard status. 
 *
 *  @param pCacheCardInfo- The pointer to edal_pe_cache_card_sub_info_t.
 *  @param pSpeclCacheCardStatus - The pointer to SPECL_CACHE_CARD_STATUS.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    25-Jan-2013  Rui Chang - Created.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_board_translate_cache_card_status(edal_pe_cache_card_sub_info_t * pCacheCardSubInfo,
                                             SPECL_CACHE_CARD_STATUS  * pSpeclCacheCardStatus)
{
    fbe_board_mgmt_cache_card_info_t * pExtCacheCardInfo = NULL;

    pExtCacheCardInfo = &pCacheCardSubInfo->externalCacheCardInfo;


    if(pSpeclCacheCardStatus->transactionStatus == EMCPAL_STATUS_SUCCESS)
    {
        pCacheCardSubInfo->transactionStatus = pSpeclCacheCardStatus->transactionStatus;
        pExtCacheCardInfo->uniqueID = pSpeclCacheCardStatus->uniqueID;
        pExtCacheCardInfo->inserted = pSpeclCacheCardStatus->inserted;
        pExtCacheCardInfo->faultLEDStatus = pSpeclCacheCardStatus->faultLEDStatus;
        pExtCacheCardInfo->powerGood = pSpeclCacheCardStatus->powerGood;
        pExtCacheCardInfo->powerUpFailure = pSpeclCacheCardStatus->powerUpFailure;
        pExtCacheCardInfo->powerUpEnable = pSpeclCacheCardStatus->powerUpEnable;
        pExtCacheCardInfo->powerEnable = pSpeclCacheCardStatus->powerEnable;
        pExtCacheCardInfo->reset = pSpeclCacheCardStatus->reset;
    }
    else if(pExtCacheCardInfo->envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD ||
           pSpeclCacheCardStatus->transactionStatus == STATUS_SMB_DEVICE_NOT_PRESENT)
    {
        pCacheCardSubInfo->transactionStatus = pSpeclCacheCardStatus->transactionStatus;
        //changr2 need check again when SPECL is finalized.
        if (pSpeclCacheCardStatus->transactionStatus == STATUS_SMB_DEVICE_NOT_PRESENT)
        {
            pExtCacheCardInfo->inserted = FBE_MGMT_STATUS_FALSE;
        }
        else
        {
            pExtCacheCardInfo->inserted = FBE_MGMT_STATUS_UNKNOWN;
        }
        pExtCacheCardInfo->faultLEDStatus = LED_BLINK_INVALID;
        pExtCacheCardInfo->powerGood = FBE_MGMT_STATUS_UNKNOWN;
        pExtCacheCardInfo->powerUpFailure = FBE_MGMT_STATUS_UNKNOWN;
        pExtCacheCardInfo->powerUpEnable = FBE_MGMT_STATUS_UNKNOWN;
        pExtCacheCardInfo->powerEnable = FBE_MGMT_STATUS_UNKNOWN;
        pExtCacheCardInfo->reset = FBE_MGMT_STATUS_UNKNOWN;
    }
    else
    {
        /* If envInterfaceStatus is FBE_ENV_INTERFACE_STATUS_GOOD, 
         * no need to do anything, it will continue to use the old values. 
         */
    }

    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_board_read_ssd_status
 **************************************************************************
 *  @brief
 *      This function gets the SSD status and 
 *          fills in EDAL with the status.
 *
 *  @param base_board- The pointer to the base board object.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  NOTES:
 *
 *  HISTORY:
 *    03-Oct-2014  bphilbin - Created.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_board_read_ssd_status(fbe_base_board_t * base_board)
{
    fbe_status_t status;
    fbe_bool_t self_test_passed;
    fbe_u32_t life_used=0, spare_blocks=0;
    fbe_edal_block_handle_t edalBlockHandle = NULL;
    fbe_edal_status_t edal_status;
    fbe_board_mgmt_ssd_info_t SSDInfo = {0};
    fbe_board_mgmt_ssd_info_t *pSSDInfo = &SSDInfo;

    fbe_base_object_trace((fbe_base_object_t *) base_board,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n",
                              __FUNCTION__);

    status = fbe_base_board_get_ssd_self_test_passed(base_board, &self_test_passed);

    status = fbe_base_board_get_ssd_life_used(base_board, &life_used);

    status = fbe_base_board_get_ssd_spare_blocks(base_board, &spare_blocks);

    edal_status = fbe_base_board_get_edal_block_handle(base_board, &edalBlockHandle);

    if(!EDAL_STAT_OK(edal_status))
    {
        fbe_base_object_trace((fbe_base_object_t *) base_board,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: failed, to get the edal block handle edal_status: 0x%x\n",
                              __FUNCTION__, edal_status);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    /* Get old ssd info. */
    edal_status = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_SSD_INFO,  //Attribute
                                        FBE_ENCL_SSD,  // component type
                                        0,  // component index
                                        sizeof(fbe_board_mgmt_ssd_info_t),  // buffer length
                                        (fbe_u8_t *)pSSDInfo);  // buffer pointer

    if(!EDAL_STAT_OK(edal_status))
    {
        fbe_base_object_trace((fbe_base_object_t *) base_board,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: failed, to get the edal buffer edal_status: 0x%x\n",
                              __FUNCTION__, edal_status);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    
    fbe_base_object_trace((fbe_base_object_t *)base_board,  
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s Setting SSD:0 SpBlkCnt:%d PctLifeUsd:%d SlfTstPssd:%d\n",
                           __FUNCTION__, 
                          spare_blocks, 
                          life_used,
                          self_test_passed);

    pSSDInfo->associatedSp = base_board->spid;
    pSSDInfo->remainingSpareBlkCount = spare_blocks;
    pSSDInfo->slot = 0;
    pSSDInfo->isLocalFru = FBE_TRUE;
    pSSDInfo->ssdLifeUsed = life_used;
    pSSDInfo->ssdSelfTestPassed = self_test_passed;

    fbe_base_board_get_ssd_serial_number(base_board, &pSSDInfo->ssdSerialNumber[0]);

    fbe_base_board_get_ssd_part_number(base_board, &pSSDInfo->ssdPartNumber[0]);

    fbe_base_board_get_ssd_assembly_name(base_board, &pSSDInfo->ssdAssemblyName[0]);

    fbe_base_board_get_ssd_firmware_revision(base_board, &pSSDInfo->ssdFirmwareRevision[0]);
    
    /* Set EDAL with new SSD info. */
    edal_status = fbe_edal_setBuffer(edalBlockHandle, 
                                    FBE_ENCL_PE_SSD_INFO,  //Attribute
                                    FBE_ENCL_SSD,  // component type
                                    0,  // component index
                                    sizeof(fbe_board_mgmt_ssd_info_t),  // buffer length
                                    (fbe_u8_t *)pSSDInfo);  // buffer pointer

    fbe_base_object_trace((fbe_base_object_t *)base_board,  
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s Sent to EDAL SSD:0 SpBlkCnt:%d PctLifeUsd:%d SlfTstPssd:%d\n",
                           __FUNCTION__, 
                          pSSDInfo->remainingSpareBlkCount, 
                          pSSDInfo->ssdLifeUsed,
                          pSSDInfo->ssdSelfTestPassed);

    if(!EDAL_STAT_OK(edal_status))
    {
        fbe_base_object_trace((fbe_base_object_t *) base_board,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: failed, to set the edal buffer edal_status: 0x%x\n",
                              __FUNCTION__, edal_status);
        return FBE_STATUS_GENERIC_FAILURE;  
    }  


    
    return FBE_STATUS_OK;

}

/*!*************************************************************************
 *  @fn fbe_base_board_read_dimm_status
 **************************************************************************
 *  @brief
 *      This function gets the DIMM status and 
 *          fills in EDAL with the status.
 *
 *  @param base_board- The pointer to the base board object.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  NOTES:
 *
 *  HISTORY:
 *    15-Apr-2013  Rui Chang - Created.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_board_read_dimm_status(fbe_base_board_t * base_board)
{
    fbe_u32_t           dimmIdex;
    fbe_u32_t           componentIndex;
    fbe_u32_t           componentCount = 0;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_edal_status_t   edal_status = FBE_EDAL_STATUS_OK;

    SPECL_DIMM_SUMMARY         * pSpeclDimmSum = NULL;
    SPECL_DIMM_STATUS          * pSpeclDimmStatus = NULL;
    edal_pe_dimm_sub_info_t      peDimmSubInfo = {0};     
    edal_pe_dimm_sub_info_t    * pPeDimmSubInfo = &peDimmSubInfo;
    fbe_board_mgmt_dimm_info_t * pExtDimmInfo = NULL;
    fbe_edal_block_handle_t     edalBlockHandle = NULL;   
    SPECL_MISC_SUMMARY  speclMiscSum;

    fbe_base_board_get_edal_block_handle(base_board, &edalBlockHandle);

    pSpeclDimmSum = (SPECL_DIMM_SUMMARY *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclDimmSum, sizeof(SPECL_DIMM_SUMMARY));

    status = fbe_base_board_pe_get_dimm_status(pSpeclDimmSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    status = fbe_base_board_pe_get_misc_status(&speclMiscSum);
    if(status != FBE_STATUS_OK)
    {
        return status;  
    }   

    /* Get the total count of the DIMM. */
    edal_status = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                                                     FBE_ENCL_DIMM, 
                                                     &componentCount);

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    componentIndex = 0;

    FBE_FOR_ALL_DIMM(dimmIdex, pSpeclDimmSum)
    {
        fbe_zero_memory(pPeDimmSubInfo, sizeof(edal_pe_dimm_sub_info_t)); 

        /* Get old dimm info. */
        edal_status = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_DIMM_INFO,  //Attribute
                                        FBE_ENCL_DIMM,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_dimm_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeDimmSubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edal_status))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }    

        pExtDimmInfo = &pPeDimmSubInfo->externalDimmInfo;
        pExtDimmInfo->associatedSp = speclMiscSum.spid;
        pExtDimmInfo->dimmID = dimmIdex;
        pExtDimmInfo->isLocalFru = TRUE;

        pSpeclDimmStatus = &(pSpeclDimmSum->dimmStatus[dimmIdex]);

        // set envInterfaceStatus based on transactionStatus (DIMM's are only read once & timestamps not updated)
        switch (pSpeclDimmStatus->transactionStatus)
        {
        case STATUS_SMB_DEV_ERR:
        case STATUS_SMB_BUS_ERR:
        case STATUS_SMB_ARB_ERR:
        case STATUS_SMB_FAILED_ERR:
            pExtDimmInfo->envInterfaceStatus = FBE_ENV_INTERFACE_STATUS_XACTION_FAIL;
            break;
        default:
            pExtDimmInfo->envInterfaceStatus = FBE_ENV_INTERFACE_STATUS_GOOD;
            break;
        }

        fbe_base_board_translate_dimm_status(pPeDimmSubInfo, pSpeclDimmStatus);
    
        /* Get EDAL with new DIMM info. */
        edal_status = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_DIMM_INFO,  //Attribute
                                        FBE_ENCL_DIMM,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_dimm_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeDimmSubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edal_status))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }  

        componentIndex++;

        if(componentIndex >= componentCount)
        {
            break;
        }
    }
    
    return FBE_STATUS_OK;

}

/*!***************************************************************
 * fbe_base_board_translate_dimm_state()
 ****************************************************************
 * @brief
 *  This function translated the dimm state.
 *
 * @param board_mgmt - The pointer to fbe_board_mgmt_dimm_info_t.
 *
 * @return fbe_dimm_state_t
 *
 * @author
 *  16-Jan-205: PHE - Create.
 *
 ****************************************************************/
static fbe_dimm_state_t 
fbe_base_board_translate_dimm_state(fbe_board_mgmt_dimm_info_t *pDimmInfo)
{
    fbe_dimm_state_t dimmState = FBE_DIMM_STATE_UNKNOWN;

    if(pDimmInfo->inserted == FBE_MGMT_STATUS_FALSE && 
       pDimmInfo->dimmDensity == 0)
    {
        dimmState = FBE_DIMM_STATE_EMPTY;
    }
    else if(pDimmInfo->inserted == FBE_MGMT_STATUS_FALSE && 
       pDimmInfo->dimmDensity != 0)
    {
        dimmState = FBE_DIMM_STATE_MISSING;
    }
    else if(pDimmInfo->inserted == FBE_MGMT_STATUS_TRUE && 
            pDimmInfo->dimmDensity == 0)
    {
        dimmState = FBE_DIMM_STATE_NEED_TO_REMOVE;
    }
    else if(pDimmInfo->inserted == FBE_MGMT_STATUS_TRUE && 
           pDimmInfo->dimmDensity != 0 &&
           pDimmInfo->faulted == FBE_MGMT_STATUS_FALSE)
    {
        dimmState = FBE_DIMM_STATE_OK;
    }
    else if(pDimmInfo->inserted == FBE_MGMT_STATUS_TRUE && 
           pDimmInfo->dimmDensity != 0 &&
           pDimmInfo->faulted == FBE_MGMT_STATUS_TRUE)
    {
        dimmState = FBE_DIMM_STATE_FAULTED;
    }
    else
    {
        dimmState = FBE_DIMM_STATE_UNKNOWN;
    }

    return dimmState;
}

/*!*************************************************************************
 *  @fn fbe_base_board_translate_dimm_status
 **************************************************************************
 *  @brief
 *      This function translates SPECL DIMM status. 
 *
 *  @param pDimmSubInfo - The pointer to edal_pe_dimm_sub_info_t.
 *  @param pSpeclDimmStatus - The pointer to SPECL_DIMM_STATUS.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    25-Jan-2013  Rui Chang - Created.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_board_translate_dimm_status(edal_pe_dimm_sub_info_t * pDimmSubInfo,
                                             SPECL_DIMM_STATUS  * pSpeclDimmStatus)
{
    fbe_board_mgmt_dimm_info_t * pExtDimmInfo = NULL;

    pExtDimmInfo = &pDimmSubInfo->externalDimmInfo;

    pExtDimmInfo->inserted = (pSpeclDimmStatus->inserted == TRUE) ? FBE_MGMT_STATUS_TRUE : FBE_MGMT_STATUS_FALSE;

    if(pSpeclDimmStatus->transactionStatus == EMCPAL_STATUS_SUCCESS)
    {
        pDimmSubInfo->transactionStatus = pSpeclDimmStatus->transactionStatus;
        pExtDimmInfo->dimmType = pSpeclDimmStatus->dimmType;
        pExtDimmInfo->dimmRevision = pSpeclDimmStatus->dimmRevision;
        pExtDimmInfo->dimmDensity = pSpeclDimmStatus->dimmDensity;
        pExtDimmInfo->numOfBanks = pSpeclDimmStatus->numOfBanks;
        pExtDimmInfo->deviceWidth = pSpeclDimmStatus->deviceWidth;
        pExtDimmInfo->numberOfRanks = pSpeclDimmStatus->numberOfRanks;
        pExtDimmInfo->vendorRegionValid = pSpeclDimmStatus->vendorRegionValid;
        pExtDimmInfo->manufacturingLocation = pSpeclDimmStatus->manufacturingLocation;
        pExtDimmInfo->faulted = FALSE;

        fbe_copy_memory(&pExtDimmInfo->jedecIdCode[0],
                        &pSpeclDimmStatus->jedecIdCode[0],
                        sizeof(pExtDimmInfo->jedecIdCode[0])*JEDEC_ID_CODE_SIZE);
         fbe_copy_memory(&pExtDimmInfo->manufacturingDate[0],
                        &pSpeclDimmStatus->manufacturingDate[0],
                        sizeof(pExtDimmInfo->manufacturingDate[0])*MANUFACTURING_DATE_SIZE);
        fbe_copy_memory(&pExtDimmInfo->moduleSerialNumber[0],
                        &pSpeclDimmStatus->moduleSerialNumber[0],
                        sizeof(pExtDimmInfo->moduleSerialNumber[0])*MODULE_SERIAL_NUMBER_SIZE);
        fbe_copy_memory(&pExtDimmInfo->partNumber[0],
                        &pSpeclDimmStatus->partNumber[0],
                        sizeof(pExtDimmInfo->partNumber[0])*PART_NUMBER_SIZE_2);
        fbe_copy_memory(&pExtDimmInfo->EMCDimmPartNumber[0],
                        &pSpeclDimmStatus->EMCDimmPartNumber[0],
                        sizeof(pExtDimmInfo->EMCDimmPartNumber[0])*EMC_DIMM_PART_NUMBER_SIZE);
        fbe_copy_memory(&pExtDimmInfo->EMCDimmSerialNumber[0],
                        &pSpeclDimmStatus->EMCDimmSerialNumber[0],
                        sizeof(pExtDimmInfo->EMCDimmSerialNumber[0])*EMC_DIMM_SERIAL_NUMBER_SIZE);
        fbe_copy_memory(&pExtDimmInfo->OldEMCDimmSerialNumber[0],
                        &pSpeclDimmStatus->OldEMCDimmSerialNumber[0],
                        sizeof(pExtDimmInfo->OldEMCDimmSerialNumber[0])*OLD_EMC_DIMM_SERIAL_NUMBER_SIZE);

    }
    else if(pExtDimmInfo->envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD ||
            pSpeclDimmStatus->transactionStatus == STATUS_SMB_DEVICE_NOT_PRESENT)
    {
        pDimmSubInfo->transactionStatus = pSpeclDimmStatus->transactionStatus;
        pExtDimmInfo->vendorRegionValid = FBE_MGMT_STATUS_UNKNOWN;
        pExtDimmInfo->faulted = FBE_MGMT_STATUS_UNKNOWN;

        fbe_copy_memory(&pExtDimmInfo->jedecIdCode[0],
                        &pSpeclDimmStatus->jedecIdCode[0],
                        sizeof(pExtDimmInfo->jedecIdCode));
         fbe_copy_memory(&pExtDimmInfo->manufacturingDate[0],
                        &pSpeclDimmStatus->manufacturingDate[0],
                        sizeof(pExtDimmInfo->manufacturingDate));
        fbe_copy_memory(&pExtDimmInfo->moduleSerialNumber[0],
                        &pSpeclDimmStatus->moduleSerialNumber[0],
                        sizeof(pExtDimmInfo->moduleSerialNumber));
        fbe_copy_memory(&pExtDimmInfo->partNumber[0],
                        &pSpeclDimmStatus->partNumber[0],
                        sizeof(pExtDimmInfo->partNumber));
        fbe_copy_memory(&pExtDimmInfo->EMCDimmPartNumber[0],
                        &pSpeclDimmStatus->EMCDimmPartNumber[0],
                        sizeof(pExtDimmInfo->EMCDimmPartNumber));
        fbe_copy_memory(&pExtDimmInfo->EMCDimmSerialNumber[0],
                        &pSpeclDimmStatus->EMCDimmSerialNumber[0],
                        sizeof(pExtDimmInfo->EMCDimmSerialNumber));
        fbe_copy_memory(&pExtDimmInfo->OldEMCDimmSerialNumber[0],
                        &pSpeclDimmStatus->OldEMCDimmSerialNumber[0],
                        sizeof(pExtDimmInfo->OldEMCDimmSerialNumber));
    }
    else
    {
        /* If envInterfaceStatus is FBE_ENV_INTERFACE_STATUS_GOOD, 
         * no need to do anything, it will continue to use the old values. 
         */
    }

    pExtDimmInfo->state = fbe_base_board_translate_dimm_state(pExtDimmInfo);

    return FBE_STATUS_OK;
}

/*!************************************************************************* 
 *  @fn fbe_base_board_read_resume_prom_status
 **************************************************************************
 *  @brief
 *      This function gets the Resume Prom status from Specl and 
 *          fills in EDAL with the status.
 *
 *  @param base_board- The pointer to the base board object.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  NOTES:
 *
 *  HISTORY:
 *    11-Oct-2010: Arun S - Created.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_board_read_resume_prom_status(fbe_base_board_t * base_board)
{
    fbe_u32_t           componentIndex;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_edal_status_t   edalStatus = FBE_EDAL_STATUS_OK;

    SMB_DEVICE                          smb_device = SMB_DEVICE_INVALID;
    SPECL_RESUME_DATA                 * pSpeclResumeData = NULL;
    edal_pe_resume_prom_sub_info_t      peResumePromSubInfo = {0};
    edal_pe_resume_prom_sub_info_t    * pPeResumePromSubInfo = &peResumePromSubInfo;
    fbe_edal_block_handle_t             edalBlockHandle = NULL;   
    fbe_u64_t                   device_type = FBE_DEVICE_TYPE_INVALID;
    fbe_device_physical_location_t      device_location = {0};


    fbe_base_board_get_edal_block_handle(base_board, &edalBlockHandle);

    pSpeclResumeData = (SPECL_RESUME_DATA *)base_board->pe_buffer_handle;
    fbe_zero_memory(pSpeclResumeData, sizeof(SPECL_RESUME_DATA));

    /* Loop through all the supported Resume Prom components on each SP */
    for(componentIndex = FBE_PE_MIDPLANE_RESUME_PROM; componentIndex < FBE_PE_MAX_RESUME_PROM_IDS; componentIndex++)
    {
        fbe_zero_memory(pPeResumePromSubInfo, sizeof(edal_pe_resume_prom_sub_info_t)); 

        edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_RESUME_PROM_INFO,  //Attribute
                                        FBE_ENCL_RESUME_PROM, // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_resume_prom_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeResumePromSubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edalStatus))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }

        /* For the very first time, the deviceType will be INVALID. So it is necessary
         * to get the device_type and smb_device explicitly and store in SubInfo structure.
         */
        if (pPeResumePromSubInfo->deviceType == FBE_DEVICE_TYPE_INVALID)
        {
            /* Get the sp, slot and device type. Device location carries the sp and slot. */
            status = fbe_base_board_get_resume_prom_device_type_and_location(base_board,
                                                                 componentIndex, 
                                                                 &device_location, 
                                                                 &device_type);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)base_board,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s: Invalid Resume Prom Component: %d.\n", 
                                      __FUNCTION__,
                                      componentIndex);

                return status;  
            }

            /* Get the SMB Device based on the device_type, slot and SP */
            status = fbe_base_board_get_resume_prom_smb_device(base_board, device_type, &device_location, &smb_device);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)base_board,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "Invalid SMB Device Type for Resume Prom Device: %s.\n", 
                                      fbe_base_board_decode_device_type(device_type));

                return status;  
            }

            pPeResumePromSubInfo->associatedSp = device_location.sp;

            /* For PS, device_location.slot is the slotNumOnEncl. 
             * For others, this is device_location.slot is the slotNumOnBlade. 
             */
            pPeResumePromSubInfo->slotNumOnBlade = device_location.slot;  
            
            pPeResumePromSubInfo->deviceType = device_type;
            pPeResumePromSubInfo->smbDevice = smb_device;
        }

        /* Get the Resume Prom information for the corresponding SMB device */
        status = fbe_base_board_get_resume(pPeResumePromSubInfo->smbDevice, pSpeclResumeData);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)base_board,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Get Resume Prom Failed for SMB Device: %d.\n", 
                                  __FUNCTION__,
                                  pPeResumePromSubInfo->smbDevice);

            return status;  
        }

        status = fbe_base_board_translate_resume_prom_status(pPeResumePromSubInfo, pSpeclResumeData); 
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)base_board,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Resume Prom status Translation failed. Xaction status: %d.\n", 
                                  __FUNCTION__,
                                  pSpeclResumeData->transactionStatus);

            return status;  
        }

        /* Update EDAL with new Resume Prom info. */
        edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_RESUME_PROM_INFO,  //Attribute
                                        FBE_ENCL_RESUME_PROM,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_resume_prom_sub_info_t),  // buffer length
                                        (fbe_u8_t *)pPeResumePromSubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edalStatus))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }  
    } /* For each component Index*/

    return FBE_STATUS_OK;
}
    
/*!*************************************************************************
 *  @fn fbe_base_board_translate_resume_prom_status
 **************************************************************************
 *  @brief
 *      This function translates SPECL resume prom status. 
 *
 *  @param pResumePromInfo- The pointer to edal_pe_mgmt_module_sub_info_t.
 *  @param pSpeclResumeData - The pointer to SPECL_RESUME_DATA.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    11-Oct-2010: Arun S - Created.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_board_translate_resume_prom_status(edal_pe_resume_prom_sub_info_t * pPeResumePromSubInfo,                                      
                                            SPECL_RESUME_DATA  * pSpeclResumeData)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_board_mgmt_resume_prom_info_t * pExtResumePromInfo = NULL;

    pExtResumePromInfo = &pPeResumePromSubInfo->externalResumePromInfo;

    status = fbe_base_board_pe_xlate_smb_status(pSpeclResumeData->transactionStatus, 
                                                &pExtResumePromInfo->resumePromStatus,
                                                pSpeclResumeData->retriesExhausted);

    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    // These fields are not used anywhere at this time. Don't update them because it can 
    // generate notification events.
#if 0
    pExtResumePromInfo->lastWriteStatus = pSpeclResumeData->lastWriteStatus;
    pExtResumePromInfo->retriesExhausted = pSpeclResumeData->retriesExhausted;
#endif

    return status;
}

/*!*************************************************************************
 *   @fn base_board_process_state_change(fbe_base_board_t * base_board, 
 *                             fbe_base_board_pe_init_data_t pe_init_data[])     
 **************************************************************************
 *  @brief
 *      This function will determine whether a StateChange has occurred
 *      in any component of an Enclosure Object
 *      This function goes through the chain.
 *
 *  @param base_board - pointer to base board object
 *
 *  @return fbe_status_t - status of process State Change operation
 *
 *  @version
 *    04-May-2010: PHE - Created.
 *
 **************************************************************************/
fbe_status_t
base_board_process_state_change(fbe_base_board_t * base_board, 
                                fbe_base_board_pe_init_data_t pe_init_data[])                           
{
    fbe_u32_t                       index = 0;
    fbe_enclosure_component_types_t componentType;
    fbe_u32_t                       componentIndex;
    fbe_u32_t                       componentCount;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_edal_status_t               edalStatus = FBE_EDAL_STATUS_OK;
    fbe_bool_t                      individualCompStateChange = FALSE;
    fbe_bool_t                      compTypeStateChange = FALSE;
    fbe_bool_t                      anyStateChange = FALSE;
    fbe_edal_block_handle_t         edalBlockHandle = NULL;
    fbe_notification_data_changed_info_t dataChangeInfo = {0};

    fbe_base_object_trace((fbe_base_object_t *)base_board,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry.\n", __FUNCTION__);

    fbe_base_board_get_edal_block_handle(base_board, &edalBlockHandle);

    for(index = 0; index < FBE_BASE_BOARD_PE_NUMBER_OF_COMPONENT_TYPES; index++)
    {       
        componentType = pe_init_data[index].component_type;
   
        edalStatus = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                                                            componentType,
                                                            &componentCount);
        /* If something bad happened, the ktrace would repeat every 3 seconds. 
         * It should never happen!!! 
         */
        if (edalStatus != FBE_EDAL_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)base_board,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, getSpecCompCnt for %s failed, index %d, edalStatus %d.\n", 
                                  __FUNCTION__,  
                                  enclosure_access_printComponentType(componentType),
                                  index,
                                  edalStatus);
            continue;
        }  

        // check if any of this component type changed
        compTypeStateChange = FALSE;

        for (componentIndex = 0; componentIndex < componentCount; componentIndex++)
        {
            individualCompStateChange = FALSE;
            edalStatus = fbe_edal_getBool(edalBlockHandle,
                                        FBE_ENCL_COMP_STATE_CHANGE,
                                        componentType,
                                        componentIndex,
                                        &individualCompStateChange);
            // if state change, record it & clear flag
            if ((edalStatus == FBE_EDAL_STATUS_OK) && individualCompStateChange)
            {
                anyStateChange = TRUE;
                compTypeStateChange = TRUE;
                edalStatus = fbe_edal_setBool(edalBlockHandle,
                                            FBE_ENCL_COMP_STATE_CHANGE,
                                            componentType,
                                            componentIndex,
                                            FALSE);

                // send notification. 
                status = base_board_get_data_change_info(base_board,
                                                         componentType, 
                                                         componentIndex,
                                                         &dataChangeInfo);
                if(status != FBE_STATUS_GENERIC_FAILURE)
                {
                    base_board_send_data_change_notification(base_board, &dataChangeInfo); 
                }
            } 
        } // End of - for (componentIndex = 0; componentIndex < componentCount; componentIndex++)

        // if state change to this component type, increment state change count for the component type
        if (compTypeStateChange)
        {
            fbe_edal_incrementComponentOverallStateChange(edalBlockHandle, componentType);
        }

    } //End of - for(index = 0; index < FBE_BASE_BOARD_PE_NUMBER_OF_COMPONENT_TYPES; index++)


    if(anyStateChange)
    {
        fbe_edal_incrementOverallStateChange(edalBlockHandle);
    }

    return FBE_STATUS_OK;

}   // end of base_board_process_state_change

/*!*************************************************************************
* @fn base_board_send_data_change_notification(fbe_base_board_t *pBaseBoard, 
                  fbe_notification_data_changed_info_t * pDataChangeInfo)             
***************************************************************************
*
* @brief
*       This functions sends the data change notification.
*
* @param      pBaseBoard - The pointer to the fbe_base_object_t.
* @param      pDataChangeInfo - The pointer to the data change info.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   06-May-2010     PHE - Created.
*
***************************************************************************/
static void 
base_board_send_data_change_notification(fbe_base_board_t *pBaseBoard, 
                                         fbe_notification_data_changed_info_t * pDataChangeInfo)
{
    fbe_notification_info_t     notification;
    fbe_object_id_t             objectId;
    
    fbe_base_object_get_object_id((fbe_base_object_t *)pBaseBoard, &objectId);

    notification.notification_type = FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED;
    notification.class_id = FBE_CLASS_ID_BASE_BOARD;
    notification.object_type = FBE_TOPOLOGY_OBJECT_TYPE_BOARD;
    notification.notification_data.data_change_info = *pDataChangeInfo;

    fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, bus %d, encl %d, slot %d, deviceMask 0x%llX.\n", 
                          __FUNCTION__, 
                          pDataChangeInfo->phys_location.bus,
                          pDataChangeInfo->phys_location.enclosure,
                          pDataChangeInfo->phys_location.slot,
                          pDataChangeInfo->device_mask);

    fbe_notification_send(objectId, notification);
    return;

}   // end of base_board_send_data_change_notification

/*!*************************************************************************
 *   @fn base_board_get_data_change_info(fbe_base_board_t * pBaseBoard,,
 *                             fbe_enclosure_component_types_t componentType,
 *                             fbe_u32_t componentIndex,
 *                             fbe_notification_data_changed_info_t * pDataChangeInfo)     
 **************************************************************************
 *  @brief
 *      This function will get the necessary data change info 
 *      for state change notification.
 *
 *  @param pBaseBoard - The pointer to the base board.
 *  @param componentType - The EDAL component type.
 *  @param componentIndex - The EDAL component index.
 *  @param pDataChangeInfo - The pointer to the data change info for data change notification.
 *
 *  @return fbe_status_t 
 *              FBE_STATUS_OK - successful, 
 *              otherwise - failed or no need to send data change notification for the component.
 *
 *  @version
 *    04-May-2010: PHE - Created.
 *
 **************************************************************************/
static fbe_status_t
base_board_get_data_change_info(fbe_base_board_t * pBaseBoard,
                                fbe_enclosure_component_types_t componentType,
                                fbe_u32_t componentIndex,
                                fbe_notification_data_changed_info_t * pDataChangeInfo) 
{
    fbe_status_t          status = FBE_STATUS_OK;
    fbe_edal_status_t     edalStatus = FBE_EDAL_STATUS_OK;
    fbe_board_mgmt_platform_info_t pePlatformSubInfo = {0};
    fbe_board_mgmt_misc_info_t     peMiscSubInfo = {0};
    edal_pe_io_comp_sub_info_t peIoCompSubInfo = {0};
    edal_pe_io_port_sub_info_t peIoPortSubInfo = {0};
    edal_pe_power_supply_sub_info_t  pePowerSupplySubInfo = {0};
    edal_pe_cooling_sub_info_t  peCoolingSubInfo = {0};
    edal_pe_mgmt_module_sub_info_t  peMgmtModuleSubInfo = {0};
    edal_pe_flt_exp_sub_info_t  peFltExpSubInfo = {0};
    edal_pe_slave_port_sub_info_t  peSlavePortSubInfo = {0};
    edal_pe_suitcase_sub_info_t  peSuitcaseSubInfo = {0};
    edal_pe_bmc_sub_info_t  peBmcSubInfo = {0};
    edal_pe_sps_sub_info_t peSpsSubInfo;
    edal_pe_battery_sub_info_t peBatterySubInfo;
    edal_pe_resume_prom_sub_info_t peResumePromSubInfo = {0};
    edal_pe_cache_card_sub_info_t peCacheCardSubInfo = {0};
    edal_pe_dimm_sub_info_t peDimmSubInfo = {0};
    fbe_board_mgmt_ssd_info_t peSSDInfo = {0};

    fbe_u32_t componentCount = 0;

    fbe_edal_block_handle_t edalBlockHandle = NULL;
        
    fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                          FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry, compType %s, index %d.\n",
                          __FUNCTION__,
                          enclosure_access_printComponentType(componentType),
                          componentIndex);

    fbe_base_board_get_edal_block_handle(pBaseBoard, &edalBlockHandle);

    fbe_zero_memory(pDataChangeInfo, sizeof(fbe_notification_data_changed_info_t));

    if (pBaseBoard->platformInfo.enclosureType == XPE_ENCLOSURE_TYPE)
    {
        pDataChangeInfo->phys_location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        pDataChangeInfo->phys_location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }
    else if (pBaseBoard->platformInfo.enclosureType == VPE_ENCLOSURE_TYPE)
    {
        pDataChangeInfo->phys_location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        pDataChangeInfo->phys_location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }
    else
    {
        pDataChangeInfo->phys_location.bus = 0;
        pDataChangeInfo->phys_location.enclosure = 0;
    }

    switch(componentType)
    {
        case FBE_ENCL_PLATFORM:
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_PLATFORM;
            pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_PLATFORM_INFO,  //Attribute
                                        FBE_ENCL_PLATFORM,  // component type
                                        componentIndex,  // component index
                                        sizeof(fbe_board_mgmt_platform_info_t),  // buffer length
                                        (fbe_u8_t *)&pePlatformSubInfo);  // buffer pointer

            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_GENERIC_FAILURE;  
            }        

            break;

        case FBE_ENCL_MISC:
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_MISC;
            pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_MISC_INFO,  //Attribute
                                        FBE_ENCL_MISC,  // component type
                                        componentIndex,  // component index
                                        sizeof(fbe_board_mgmt_misc_info_t),  // buffer length
                                        (fbe_u8_t *)&peMiscSubInfo);  // buffer pointer

            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_GENERIC_FAILURE;  
            }        
            break;
        
        case FBE_ENCL_IO_MODULE:
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_IOMODULE;
            pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_IO_COMP_INFO,  //Attribute
                                        FBE_ENCL_IO_MODULE,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_io_comp_sub_info_t),  // buffer length
                                        (fbe_u8_t *)&peIoCompSubInfo);  // buffer pointer

            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_GENERIC_FAILURE;  
            }        
            pDataChangeInfo->phys_location.sp = peIoCompSubInfo.externalIoCompInfo.associatedSp;
            pDataChangeInfo->phys_location.slot = peIoCompSubInfo.externalIoCompInfo.slotNumOnBlade;
            break;        

        case FBE_ENCL_BEM:
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_BACK_END_MODULE;
            pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_IO_COMP_INFO,  //Attribute
                                        FBE_ENCL_BEM,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_io_comp_sub_info_t),  // buffer length
                                        (fbe_u8_t *)&peIoCompSubInfo);  // buffer pointer

            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_GENERIC_FAILURE;  
            }        
            pDataChangeInfo->phys_location.sp = peIoCompSubInfo.externalIoCompInfo.associatedSp;
            pDataChangeInfo->phys_location.slot = peIoCompSubInfo.externalIoCompInfo.slotNumOnBlade;
            break;

        case FBE_ENCL_MEZZANINE:
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_MEZZANINE;
            pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_IO_COMP_INFO,  //Attribute
                                        FBE_ENCL_MEZZANINE,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_io_comp_sub_info_t),  // buffer length
                                        (fbe_u8_t *)&peIoCompSubInfo);  // buffer pointer

            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_GENERIC_FAILURE;  
            }        
            pDataChangeInfo->phys_location.sp = peIoCompSubInfo.externalIoCompInfo.associatedSp;
            pDataChangeInfo->phys_location.slot = peIoCompSubInfo.externalIoCompInfo.slotNumOnBlade;
            break;

        case FBE_ENCL_IO_PORT:
            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_IO_PORT_INFO,  //Attribute
                                        FBE_ENCL_IO_PORT,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_io_port_sub_info_t),  // buffer length
                                        (fbe_u8_t *)&peIoPortSubInfo);  // buffer pointer

            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_GENERIC_FAILURE;  
            }        
          
            pDataChangeInfo->device_mask = peIoPortSubInfo.externalIoPortInfo.deviceType;
            pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_PORT_INFO;
            pDataChangeInfo->phys_location.sp = peIoPortSubInfo.externalIoPortInfo.associatedSp;
            pDataChangeInfo->phys_location.slot = peIoPortSubInfo.externalIoPortInfo.slotNumOnBlade;
            pDataChangeInfo->phys_location.port = peIoPortSubInfo.externalIoPortInfo.portNumOnModule;
            break;

        case FBE_ENCL_POWER_SUPPLY:
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_PS;
            pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_POWER_SUPPLY_INFO,  //Attribute
                                        FBE_ENCL_POWER_SUPPLY,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_power_supply_sub_info_t),  // buffer length
                                        (fbe_u8_t *)&pePowerSupplySubInfo);  // buffer pointer

            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_GENERIC_FAILURE;  
            }        

            pDataChangeInfo->phys_location.slot = pePowerSupplySubInfo.externalPowerSupplyInfo.slotNumOnEncl;
            break;

        case FBE_ENCL_COOLING_COMPONENT:
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_FAN;
            pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_COOLING_INFO,  //Attribute
                                        FBE_ENCL_COOLING_COMPONENT,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_cooling_sub_info_t),  // buffer length
                                        (fbe_u8_t *)&peCoolingSubInfo);  // buffer pointer

            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_GENERIC_FAILURE;  
            }        

            pDataChangeInfo->phys_location.sp = peCoolingSubInfo.externalCoolingInfo.associatedSp;
            pDataChangeInfo->phys_location.slot = componentIndex;
            break;

        case FBE_ENCL_MGMT_MODULE:
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_MGMT_MODULE;
            pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_MGMT_MODULE_INFO,  //Attribute
                                        FBE_ENCL_MGMT_MODULE,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_mgmt_module_sub_info_t),  // buffer length
                                        (fbe_u8_t *)&peMgmtModuleSubInfo);  // buffer pointer

            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_GENERIC_FAILURE;  
            }        
            pDataChangeInfo->phys_location.sp = peMgmtModuleSubInfo.externalMgmtModuleInfo.associatedSp;
            pDataChangeInfo->phys_location.slot = peMgmtModuleSubInfo.externalMgmtModuleInfo.mgmtID;
            break;

        case FBE_ENCL_FLT_REG:
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_FLT_REG;
            pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_FLT_REG_INFO,  //Attribute
                                        FBE_ENCL_FLT_REG,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_flt_exp_sub_info_t),  // buffer length
                                        (fbe_u8_t *)&peFltExpSubInfo);  // buffer pointer

            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_GENERIC_FAILURE;  
            }
            pDataChangeInfo->phys_location.sp = peFltExpSubInfo.externalFltExpInfo.associatedSp;
            pDataChangeInfo->phys_location.slot = peFltExpSubInfo.externalFltExpInfo.fltExpID;

            /* Both local and peer fault register component will be updated in EDAL. 
             * Only peer FRU component will be updated with FaultReg fault in EDAL.
             */
            if ((peFltExpSubInfo.externalFltExpInfo.associatedSp != pBaseBoard->spid) && 
                /* Only handle primary fault register and ignore mirror in Rockies */
                (peFltExpSubInfo.externalFltExpInfo.fltExpID == PRIMARY_FLT_REG) &&
                (peFltExpSubInfo.externalFltExpInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD))
            {
                fbe_base_board_peerBoot_fltReg_checkFru(pBaseBoard, &(peFltExpSubInfo.externalFltExpInfo));
            }
            break;

        case FBE_ENCL_SLAVE_PORT:
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_SLAVE_PORT;
            pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_SLAVE_PORT_INFO,  //Attribute
                                        FBE_ENCL_SLAVE_PORT,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_slave_port_sub_info_t),  // buffer length
                                        (fbe_u8_t *)&peSlavePortSubInfo);  // buffer pointer

            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_GENERIC_FAILURE;  
            }        
            pDataChangeInfo->phys_location.sp = peSlavePortSubInfo.externalSlavePortInfo.associatedSp;
            pDataChangeInfo->phys_location.slot = peSlavePortSubInfo.externalSlavePortInfo.slavePortID;
            break;

        case FBE_ENCL_SUITCASE:
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_SUITCASE;
            pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_SUITCASE_INFO,  //Attribute
                                        FBE_ENCL_SUITCASE,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_suitcase_sub_info_t),  // buffer length
                                        (fbe_u8_t *)&peSuitcaseSubInfo);  // buffer pointer

            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_GENERIC_FAILURE;  
            }        
            pDataChangeInfo->phys_location.sp = peSuitcaseSubInfo.externalSuitcaseInfo.associatedSp;
            pDataChangeInfo->phys_location.slot = peSuitcaseSubInfo.externalSuitcaseInfo.suitcaseID;
            break;

        case FBE_ENCL_BMC:
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_BMC;
            pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_BMC_INFO,  //Attribute
                                        FBE_ENCL_BMC,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_bmc_sub_info_t),  // buffer length
                                        (fbe_u8_t *)&peBmcSubInfo);  // buffer pointer

            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_GENERIC_FAILURE;  
            }        
            pDataChangeInfo->phys_location.sp = peBmcSubInfo.externalBmcInfo.associatedSp;
            pDataChangeInfo->phys_location.slot = peBmcSubInfo.externalBmcInfo.bmcID;
            break;

        case FBE_ENCL_SPS:
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_SPS;
            pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                            FBE_ENCL_PE_SPS_INFO,               //Attribute
                                            FBE_ENCL_SPS,                       // component type
                                            componentIndex,                     // component index
                                            sizeof(edal_pe_sps_sub_info_t),     // buffer length
                                            (fbe_u8_t *)&peSpsSubInfo);         // buffer pointer

            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_GENERIC_FAILURE;  
            }        

            edalStatus = fbe_edal_getSpecificComponentCount(edalBlockHandle,
                                                            FBE_ENCL_SPS,
                                                            &componentCount);
            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_GENERIC_FAILURE;  
            }

            // SPSA & SPSB will both report same slot (ESP will differentiate properly)
            pDataChangeInfo->phys_location.slot = componentIndex;
            break;

        case FBE_ENCL_BATTERY:
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_BATTERY;
            pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                            FBE_ENCL_PE_BATTERY_INFO,           //Attribute
                                            FBE_ENCL_BATTERY,                   // component type
                                            componentIndex,                     // component index
                                            sizeof(edal_pe_battery_sub_info_t), // buffer length
                                            (fbe_u8_t *)&peBatterySubInfo);     // buffer pointer

            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_GENERIC_FAILURE;  
            }        

            edalStatus = fbe_edal_getSpecificComponentCount(edalBlockHandle,
                                                            FBE_ENCL_BATTERY,
                                                            &componentCount);
            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_GENERIC_FAILURE;  
            }

            pDataChangeInfo->phys_location.sp = peBatterySubInfo.externalBatteryInfo.associatedSp;
            if (pDataChangeInfo->phys_location.sp == SP_A)
            {
                pDataChangeInfo->phys_location.slot = componentIndex; 
            }
            else
            {
                pDataChangeInfo->phys_location.slot = componentIndex - (componentCount/2); 
            }
            break;

        case FBE_ENCL_RESUME_PROM:
            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_RESUME_PROM_INFO,  //Attribute
                                        FBE_ENCL_RESUME_PROM,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_resume_prom_sub_info_t),  // buffer length
                                        (fbe_u8_t *)&peResumePromSubInfo);  // buffer pointer

            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_GENERIC_FAILURE;  
            }        

            pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_RESUME_PROM_INFO;
            pDataChangeInfo->device_mask = peResumePromSubInfo.deviceType;
            pDataChangeInfo->phys_location.sp = peResumePromSubInfo.associatedSp;
            pDataChangeInfo->phys_location.slot = peResumePromSubInfo.slotNumOnBlade;
            break;

        case FBE_ENCL_CACHE_CARD:
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_CACHE_CARD;
            pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_CACHE_CARD_INFO,  //Attribute
                                        FBE_ENCL_CACHE_CARD,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_cache_card_sub_info_t),  // buffer length
                                        (fbe_u8_t *)&peCacheCardSubInfo);  // buffer pointer

            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_GENERIC_FAILURE;  
            }        

            pDataChangeInfo->phys_location.slot = componentIndex;
            pDataChangeInfo->phys_location.sp = peCacheCardSubInfo.externalCacheCardInfo.associatedSp;

            break;

        case FBE_ENCL_DIMM:
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_DIMM;
            pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_DIMM_INFO,  //Attribute
                                        FBE_ENCL_DIMM,  // component type
                                        componentIndex,  // component index
                                        sizeof(edal_pe_dimm_sub_info_t),  // buffer length
                                        (fbe_u8_t *)&peDimmSubInfo);  // buffer pointer

            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_GENERIC_FAILURE;  
            }        

            pDataChangeInfo->phys_location.slot = componentIndex;
            pDataChangeInfo->phys_location.sp = peDimmSubInfo.externalDimmInfo.associatedSp;

            break;


        case FBE_ENCL_SSD:
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_SSD;
            pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_SSD_INFO,  //Attribute
                                        FBE_ENCL_SSD,  // component type
                                        componentIndex,  // component index
                                        sizeof(fbe_board_mgmt_ssd_info_t),  // buffer length
                                        (fbe_u8_t *)&peSSDInfo);  // buffer pointer

            if(!EDAL_STAT_OK(edalStatus))
            {

                fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                          FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Failed to get buffer for SSD:%d info.\n",
                          __FUNCTION__,
                          componentIndex);
                return FBE_STATUS_GENERIC_FAILURE;  
            }        

            pDataChangeInfo->phys_location.slot = componentIndex;

            break;

        default:
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_INVALID;
            pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_INVALID;
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}

/*!*************************************************************************
 *  @fn fbe_base_board_process_fup_status_change(fbe_base_board_t * pBaseBoard)
 **************************************************************************
 *  @brief
 *      This function will determine whether a firmware download/activate status change
 *      has occurred. If yes, send the notification.
 *
 *  @param pBaseEncl - pointer to fbe_base_enclosure_t
 *
 *  @return fbe_status_t - status of process State Change operation
 *
 *  @note
 *
 *  @version
 *    24-Sep-2012: PHE - Created. 
 *
 **************************************************************************/
fbe_status_t 
fbe_base_board_process_fup_status_change(fbe_base_board_t * pBaseBoard)
{ 
    fbe_notification_data_changed_info_t dataChangeInfo = {0};
    fbe_u32_t                            componentCount = 0;
    fbe_u32_t                            componentIndex = 0;
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_edal_status_t                    edalStatus = FBE_EDAL_STATUS_OK;
    fbe_edal_block_handle_t              edalBlockHandle = NULL;

    fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s Entry.\n",
                        __FUNCTION__);      

    fbe_base_board_get_edal_block_handle(pBaseBoard, &edalBlockHandle);

    /* Get the total count of SPS's. */
    edalStatus = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                                                     FBE_ENCL_SPS, 
                                                     &componentCount);

    if(edalStatus != FBE_EDAL_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    for (componentIndex = 0; componentIndex < componentCount; componentIndex++)
    {
        status = fbe_base_board_check_and_get_fup_data_change_info(pBaseBoard,
                                                          FBE_ENCL_SPS,
                                                          componentIndex,
                                                          &dataChangeInfo);

        if((status == FBE_STATUS_OK) &&
           (dataChangeInfo.data_type != FBE_DEVICE_DATA_TYPE_INVALID ) &&
           (dataChangeInfo.device_mask != FBE_DEVICE_TYPE_INVALID))
        {
            base_board_send_data_change_notification(pBaseBoard, &dataChangeInfo); 
        }
    }
  
    return FBE_STATUS_OK;

} // End of function fbe_base_board_process_fup_status_change


/*!*************************************************************************
 *   @fn fbe_base_board_check_and_get_fup_data_change_info(fbe_base_board_t * pBaseBoard,
 *                               fbe_enclosure_component_types_t componentType,
 *                               fbe_u32_t componentIndex,
 *                               fbe_notification_data_changed_info_t * pDataChangeInfo)    
 **************************************************************************
 *  @brief
 *      This function will check whether the upgrade status has changed.
 *      If yes, get the necessary data change info 
 *      for firmware upgrade status change notification.
 *
 *  @param pBaseBoard - The pointer to the base board.
 *  @param componentType - The EDAL component type.
 *  @param componentIndex - The EDAL component index.
 *  @param pDataChangeInfo - The pointer to the data change info for data change notification.
 *
 *  @return fbe_status_t 
 *              FBE_STATUS_OK - successful, 
 *              otherwise - failed or no need to send data change notification for the component.
 *
 *  @version
 *    24-Sep-2012: PHE - Created.
 *
 **************************************************************************/
static fbe_status_t
fbe_base_board_check_and_get_fup_data_change_info(fbe_base_board_t * pBaseBoard,
                                fbe_enclosure_component_types_t componentType,
                                fbe_u32_t componentIndex,
                                fbe_notification_data_changed_info_t * pDataChangeInfo) 
{ 
    edal_pe_sps_sub_info_t               peSpsSubInfo = {0};
    fbe_edal_block_handle_t              edalBlockHandle = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_edal_status_t                    edalStatus = FBE_EDAL_STATUS_OK;
        
    fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                          FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry, compType %s, index %d.\n",
                          __FUNCTION__,
                          enclosure_access_printComponentType(componentType),
                          componentIndex);

    fbe_base_board_get_edal_block_handle(pBaseBoard, &edalBlockHandle);

    fbe_zero_memory(pDataChangeInfo, sizeof(fbe_notification_data_changed_info_t));
    pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_INVALID;
    pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_INVALID;

    if (pBaseBoard->platformInfo.enclosureType == XPE_ENCLOSURE_TYPE)
    {
        pDataChangeInfo->phys_location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        pDataChangeInfo->phys_location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }
    else if (pBaseBoard->platformInfo.enclosureType == VPE_ENCLOSURE_TYPE)
    {
        pDataChangeInfo->phys_location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        pDataChangeInfo->phys_location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }
    else
    {
        pDataChangeInfo->phys_location.bus = 0;
        pDataChangeInfo->phys_location.enclosure = 0;
    }

    switch(componentType)
    {
       case FBE_ENCL_SPS:
            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                            FBE_ENCL_PE_SPS_INFO,               //Attribute
                                            FBE_ENCL_SPS,                       // component type
                                            componentIndex,                     // component index
                                            sizeof(edal_pe_sps_sub_info_t),     // buffer length
                                            (fbe_u8_t *)&peSpsSubInfo);         // buffer pointer

            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_GENERIC_FAILURE;  
            }        

            if((peSpsSubInfo.fupStatus != peSpsSubInfo.prevFupStatus) || 
               (peSpsSubInfo.fupExtStatus != peSpsSubInfo.prevFupExtStatus))
            {
                fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                       "FUP: SPE SPS, fupStatus/fupExtStatus 0x%X/0x%X, prevStatus/prevExtStatus 0x%X/0x%X.\n",
                                       peSpsSubInfo.fupStatus,
                                       peSpsSubInfo.fupExtStatus,
                                       peSpsSubInfo.prevFupStatus,
                                       peSpsSubInfo.prevFupExtStatus); 

                peSpsSubInfo.prevFupStatus = peSpsSubInfo.fupStatus;
                peSpsSubInfo.prevFupExtStatus = peSpsSubInfo.fupExtStatus;
 
                /* Update EDAL with new SPS info. */
                edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                                FBE_ENCL_PE_SPS_INFO,           //Attribute
                                                FBE_ENCL_SPS,                   // component type
                                                componentIndex,                 // component index
                                                sizeof(edal_pe_sps_sub_info_t), // buffer length
                                                (fbe_u8_t *)&peSpsSubInfo);     // buffer pointer
                if(!EDAL_STAT_OK(edalStatus))
                {
                    return FBE_STATUS_GENERIC_FAILURE;  
                }  

                /* Need to send the notification.*/
                pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_SPS;
                pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_FUP_INFO;
                /* Will be implemented later. */
                //pDataChangeInfo->phys_location.slot = peSpsSubInfo.externalSpsInfo.slot; /* 0 for SPSA and 1 for SPSB*/
                pDataChangeInfo->phys_location.slot = 0;
            }
            
            break;

        default:
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_INVALID;
            pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_INVALID;
            status = FBE_STATUS_OK;
            break;
    }

    return status;
}// End of function fbe_base_board_check_and_get_fup_data_change_info

/*!*************************************************************************
 *   @fn fbe_base_board_check_env_interface_error(fbe_edal_block_handle_t edal_block_handle,
 *                                       fbe_component_type_t component_type,
 *                                       SPECL_ERROR transactionStatus,
 *                                       fbe_u64_t newTimeStamp,
 *                                       fbe_u64_t *pTimeStampForFirstBadStatus,
 *                                       fbe_env_interface_status_t *pEnvInterfaceStatus)
 **************************************************************************
 *  @brief
 *      This function handles the Stale data and SMB error for PE components.
 *
 *  @param edal_block_handle - Handle tofab get the component information from EDAL.
 *  @param component_type - The EDAL component type
 *  @param transactionStatus - The SPECL transaction Status.
 *  @param pTimeStampForFirstBadStatus - The Timestamp for the first bad status.
 *  @param pEnvInterfaceStatus - The pointer to the Interface Status notification.
 *
 *  @return fbe_status_t 
 *              FBE_STATUS_OK - successful, 
 *              otherwise - failed or no need to send data any notification for the component.
 *
 *  @version
 *    08-June-2010: Arun S - Created.
 *
 **************************************************************************/
static fbe_status_t fbe_base_board_check_env_interface_error(fbe_base_board_t *base_board,
                                                              fbe_enclosure_component_types_t component_type,
                                                              SPECL_ERROR transactionStatus,
                                                              fbe_u64_t newTimeStamp,
                                                              fbe_u64_t *pTimeStampForFirstBadStatus, 
                                                              fbe_env_inferface_status_t *pEnvInterfaceStatus)
{
    fbe_u32_t                   max_status_timeout = 0;
    fbe_u64_t                   elapsedTime = 0;
    fbe_u64_t                   timeStampForFirstBadStatus = 0;
    fbe_edal_status_t           edalStatus = FBE_EDAL_STATUS_OK;
    fbe_edal_block_handle_t     edal_block_handle = NULL;   
    fbe_board_mgmt_misc_info_t  misc_info = {0};

    fbe_base_object_trace((fbe_base_object_t *)base_board,
                          FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "base_brd_chk_env_interface_err: entry.Comp:%s\n", 
                          enclosure_access_printComponentType(component_type));

    /* Get the max status timeout for the component */
    fbe_base_board_get_component_max_status_timeout(component_type, &max_status_timeout);

    /* Get how long the Stale data has lasted. */
    fbe_base_board_get_elapsed_time_data(&elapsedTime, newTimeStamp, SPECL_1_SECOND_UNITS);

    /* Verify if it is stale data */
    if(elapsedTime >= max_status_timeout)  
    {
         if(*pEnvInterfaceStatus != FBE_ENV_INTERFACE_STATUS_DATA_STALE)
         {
             *pEnvInterfaceStatus = FBE_ENV_INTERFACE_STATUS_DATA_STALE;

             fbe_base_object_trace((fbe_base_object_t *)base_board,
                                   FBE_TRACE_LEVEL_INFO,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "base_brd_chk_env_interface_err:Stale Data.Max limit %d sec.EnvIntStat:%d.Comp:%s\n", 
                                   max_status_timeout,
                                   *pEnvInterfaceStatus,
                                   enclosure_access_printComponentType(component_type));
         }

         return FBE_STATUS_OK;
    }

    /* Verify if the SMB read succeeded */
    if ((transactionStatus == EMCPAL_STATUS_SUCCESS) || 
        (transactionStatus == STATUS_SMB_DEVICE_NOT_PRESENT) ||
        (transactionStatus == STATUS_SMB_DEVICE_POWERED_OFF))
    {
        /* Either the Read succeeded or the SMB device not present or 
         * SMB Device is powered off. Clear the Timestamp for First Bad Status 
         */
        *pTimeStampForFirstBadStatus = 0;

        if(*pEnvInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
        {
            *pEnvInterfaceStatus = FBE_ENV_INTERFACE_STATUS_GOOD;

            fbe_base_object_trace((fbe_base_object_t *)base_board,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "base_brd_chk_env_interface_err: Read succeed.EnvIntStat:%d,actionStat:0x%X.Comp:%s\n", 
                                  *pEnvInterfaceStatus,
                                  transactionStatus,
                                  enclosure_access_printComponentType(component_type));
        }
    }
    else
    {
        /* The transactionStatus is not successful/valid.
         * If the timeStampForFirstBadStatus is 0, it means this is 
         * the first bad transactionStatus.
         * Save the timestamp as timeStampForFirstBadStatus.
         */
        if (*pTimeStampForFirstBadStatus == 0) 
        {
            *pTimeStampForFirstBadStatus = newTimeStamp;
        }

        /* Get how long the bad status has lasted. */
        timeStampForFirstBadStatus = *pTimeStampForFirstBadStatus;
        fbe_base_board_get_elapsed_time_data(&elapsedTime, timeStampForFirstBadStatus, SPECL_1_SECOND_UNITS);

        if (elapsedTime < max_status_timeout)
        {
            /* Read Failed but not exceeded the max_limit */
           if(*pEnvInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
           {
               *pEnvInterfaceStatus = FBE_ENV_INTERFACE_STATUS_GOOD;

               fbe_base_object_trace((fbe_base_object_t *)base_board,
                                     FBE_TRACE_LEVEL_INFO,
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "base_brd_chk_env_interface_err:Read Fail not reach max %d sec.EnvIntStat:%d,actionStat:0x%X.Comp:%s\n", 
                                     max_status_timeout,
                                     *pEnvInterfaceStatus,
                                     transactionStatus,
                                     enclosure_access_printComponentType(component_type));
           }
        }
        else
        {
            if(*pEnvInterfaceStatus != FBE_ENV_INTERFACE_STATUS_XACTION_FAIL)
            {
                /* Read Failed and reached the max_limit */
                *pEnvInterfaceStatus = FBE_ENV_INTERFACE_STATUS_XACTION_FAIL;

                fbe_base_object_trace((fbe_base_object_t *)base_board,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "base_brd_chk_env_interface_err:Read Fail reach max %d sec.EnvIntStat:%d,actionStat:0x%X.Comp:%s\n",  
                                      max_status_timeout,
                                      *pEnvInterfaceStatus,
                                      transactionStatus,
                                      enclosure_access_printComponentType(component_type));
            }

            if(transactionStatus == STATUS_SMB_SELECTBITS_FAILED)
            {
                fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);

                /* Get Misc info from EDAL. */
                edalStatus = fbe_edal_getBuffer(edal_block_handle, 
                                                FBE_ENCL_PE_MISC_INFO,  //Attribute
                                                FBE_ENCL_MISC,  // component type
                                                0,  // component index
                                                sizeof(fbe_board_mgmt_misc_info_t),  // buffer length
                                                (fbe_u8_t *)&misc_info);  // buffer pointer

                if(!EDAL_STAT_OK(edalStatus))
                {
                    return FBE_STATUS_GENERIC_FAILURE;  
                }    

                if(!misc_info.smbSelectBitsFailed)                
                {
                    /* Set the SMSelectBitsFailed to TRUE */
                    misc_info.smbSelectBitsFailed = TRUE;

                    /* Update EDAL with new Misc info. */
                    edalStatus = fbe_edal_setBuffer(edal_block_handle, 
                                                    FBE_ENCL_PE_MISC_INFO,  //Attribute
                                                    FBE_ENCL_MISC,  // component type
                                                    0,  // component index
                                                    sizeof(fbe_board_mgmt_misc_info_t),  // buffer length
                                                   (fbe_u8_t *)&misc_info);  // buffer pointer

                    if(!EDAL_STAT_OK(edalStatus))
                    {
                        return FBE_STATUS_GENERIC_FAILURE;  
                    }  

                    fbe_base_object_trace((fbe_base_object_t *)base_board,
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "base_brd_chk_env_interface_err:SMB select bits Failed is SET.Comp:%s\n",
                                          enclosure_access_printComponentType(component_type));
                }
            }
        }
    }

    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_board_get_component_max_status_timeout
 **************************************************************************
 *  @brief
 *      This function gets the PE component max age for Bad status.
 *
 *  @param pe_init_data - pointer to the array which will save 
 *            the max timeout for few the processor enclosure component types.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  NOTES:
 *
 *  HISTORY:
 *    08-June-2010: Arun S - Created.
 *
 **************************************************************************/
static fbe_status_t fbe_base_board_get_component_max_status_timeout(fbe_enclosure_component_types_t component_type, 
                                                                fbe_u32_t *max_status_timeout)
{
    fbe_u32_t  index = 0;
        
    for(index = 0; index < FBE_BASE_BOARD_PE_NUMBER_OF_COMPONENT_TYPES; index ++ )
    {
        if (component_type == pe_init_data[index].component_type)
        {
            *max_status_timeout = pe_init_data[index].max_status_timeout;
            break;
        }
    }

    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_board_process_persistent_multi_fan_fault
 **************************************************************************
 *  @brief
 *      This function handles the persistent multi fan fault.
 *
 *  @param fan_fault_count - Fan fault count to determine the multi fan fault.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  NOTES:
 *
 *  HISTORY:
 *    14-June-2010: Arun S - Created.
 *
 **************************************************************************/
static fbe_status_t fbe_base_board_process_persistent_multi_fan_fault(fbe_base_board_t *base_board,
                                                                      fbe_u64_t newTimeStamp, 
                                                                      fbe_u32_t fan_fault_count)
{
    fbe_u64_t            elapsed_time = 0;
    fbe_u64_t            timeStampForFirstMultiFanModuleFault = 0;

    fbe_status_t         status = FBE_STATUS_OK;
    fbe_edal_status_t    edal_status = FBE_EDAL_STATUS_OK;
    fbe_mgmt_status_t    persistentMultiFanModuleFault = FBE_MGMT_STATUS_FALSE;

    fbe_edal_block_handle_t         edal_block_handle = NULL;   
    edal_pe_cooling_sub_info_t      peCoolingSubInfo = {0};      
    edal_pe_cooling_sub_info_t     *pPeCoolingSubInfo = &peCoolingSubInfo;

    fbe_base_object_trace((fbe_base_object_t *)base_board,
                          FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s: entry.\n", 
                          __FUNCTION__);

    /* Get Cooling info of Fan 0 from EDAL to check for envInterfaceStatus to
     * process the Persistent Multi Fan Fault. We dont need to get the envInterfaceStatus
     * of all the the other Fans (1, 2, 3) as those will also have the same envInterfaceStatus. 
     */
    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);

    fbe_zero_memory(pPeCoolingSubInfo, sizeof(edal_pe_cooling_sub_info_t)); 
    edal_status = fbe_edal_getBuffer(edal_block_handle, 
                                     FBE_ENCL_PE_COOLING_INFO,  //Attribute
                                     FBE_ENCL_COOLING_COMPONENT,  // component type
                                     0,  // component index
                                     sizeof(edal_pe_cooling_sub_info_t),  // buffer length
                                     (fbe_u8_t *)pPeCoolingSubInfo);  // buffer pointer

    if(!EDAL_STAT_OK(edal_status))
    {
        return FBE_STATUS_GENERIC_FAILURE;  
    }  

    /* Check for env status. If not GOOD, then return from here */
    if (pPeCoolingSubInfo->externalCoolingInfo.envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD) 
    {
        /* No need to proceed. */
        return FBE_STATUS_OK;
    }

    persistentMultiFanModuleFault = pPeCoolingSubInfo->externalCoolingInfo.persistentMultiFanModuleFault;

    if(fan_fault_count >= 2)
    {
        if(pPeCoolingSubInfo->timeStampForMultiFaultedStateStart == 0)
        {
            timeStampForFirstMultiFanModuleFault = newTimeStamp;
        }
    }
    else
    {
        /* The Fan could have recovered from the previous Multi Fan Fault.
         * So, clean up the timestamp and the persistentMultiFanModuleFault status.
         */
        timeStampForFirstMultiFanModuleFault = 0;

        if (persistentMultiFanModuleFault == FBE_MGMT_STATUS_TRUE) 
        {
            /* Clear the persistentMultiFanModuleFault. */
            persistentMultiFanModuleFault = FBE_MGMT_STATUS_FALSE;

            fbe_base_object_trace((fbe_base_object_t *)base_board,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: persistentMultiFanModuleFault is Cleared. No Multi Fan Fault observed.\n", 
                                  __FUNCTION__);
        }

        return FBE_STATUS_OK;
    }

    fbe_base_board_get_elapsed_time_data(&elapsed_time, timeStampForFirstMultiFanModuleFault, SPECL_1_SECOND_UNITS);
   
    /* Set the value persistentMultiFanModuleFault.  If multiFanFaultAge is greater than
     * or equal to FBE_MAX_MULTI_FAN_FAULT_TIMEOUT and there are exactly 2 fans 
     * faulted, then set persistentMultiFanModuleFault.  With exactly 2 fans faulted, the
     * system can run some indefinite amount of time, so delay and allow the multi
     * fan fault to possibly disappear.
     * If there exists a multi fan fault, and the number of fans faulted is not 2
     * (i.e. it may be unknown, or it may be 3 or 4), then assume the system could
     * heat up quickly enough to cause some components to fail quickly.  In this
     * case set persistentMultiFanModuleFault, so cache is dumped now.
     */
    if ( (elapsed_time >= FBE_MAX_MULTI_FAN_FAULT_TIMEOUT) &&
        (fan_fault_count == 2) )
    {
        if (persistentMultiFanModuleFault != FBE_MGMT_STATUS_TRUE)
        {
            /* Set the Persistent Multi Fan fault */
            persistentMultiFanModuleFault = FBE_MGMT_STATUS_TRUE;

            fbe_base_object_trace((fbe_base_object_t *)base_board,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: persistentMultiFanModuleFault is SET exceeding the max limit: %d sec with %d fans faulted.\n", 
                                  __FUNCTION__,
                                  FBE_MAX_MULTI_FAN_FAULT_TIMEOUT,
                                  fan_fault_count);
        }
    }
    else if ( (elapsed_time > 0) && (fan_fault_count > 2) )
    {
        if (persistentMultiFanModuleFault != FBE_MGMT_STATUS_TRUE)
        {
            /* Set the Persistent Multi Fan fault */
            persistentMultiFanModuleFault = FBE_MGMT_STATUS_TRUE;

            fbe_base_object_trace((fbe_base_object_t *)base_board,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: PersistentMultiFanModuleFault is SET: More than 2 (%d) fans faulted.\n", 
                                  __FUNCTION__,
                                  fan_fault_count);
        }
    }
    else
    {
        if (persistentMultiFanModuleFault != FBE_MGMT_STATUS_FALSE) 
        {
            /* Clear the persistentMultiFanModuleFault. */
            persistentMultiFanModuleFault = FBE_MGMT_STATUS_FALSE;

            fbe_base_object_trace((fbe_base_object_t *)base_board,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: persistentMultiFanModuleFault is Cleared.\n", 
                                  __FUNCTION__);
        }
    }

    status = fbe_base_board_set_persistent_multi_fan_module_fault(base_board, persistentMultiFanModuleFault, timeStampForFirstMultiFanModuleFault);

    if(status != FBE_STATUS_OK)
    {
        return status;  
    }

    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_board_set_persistent_multi_fan_module_fault
 **************************************************************************
 *  @brief
 *      This function records the Persistent Multi Fan fault and timestamp for
 *      Multi Fan faulted start state in all the Fans.
 *
 *  @param persistentMultiFanModuleFault - Status to be set. 
 *  @param timeStampForMultiFaultedStateStart - Timestamp to be set. 
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  NOTES:
 *
 *  HISTORY:
 *    14-June-2010: Arun S - Created.
 *
 **************************************************************************/
static fbe_status_t fbe_base_board_set_persistent_multi_fan_module_fault(fbe_base_board_t *base_board, 
                                                                  fbe_mgmt_status_t persistentMultiFanModuleFault,
                                                                  fbe_u64_t timeStampForMultiFaultedStateStart)
{
    fbe_u32_t                       fan;
    fbe_u32_t                       component_count = 0;
    fbe_edal_status_t               edal_status = FBE_EDAL_STATUS_OK;
    fbe_edal_block_handle_t         edal_block_handle = NULL;   

    edal_pe_cooling_sub_info_t      peCoolingSubInfo = {0};      
    edal_pe_cooling_sub_info_t     *pPeCoolingSubInfo = &peCoolingSubInfo;

    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);

    /* Get the total count of the blowers. */
    edal_status = fbe_edal_getSpecificComponentCount(edal_block_handle, 
                                                     FBE_ENCL_COOLING_COMPONENT, 
                                                     &component_count);

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    for(fan = 0; fan < component_count; fan++ )
    {
        fbe_zero_memory(pPeCoolingSubInfo, sizeof(edal_pe_cooling_sub_info_t)); 

        /* Get Cooling info from EDAL. */
        edal_status = fbe_edal_getBuffer(edal_block_handle, 
                                         FBE_ENCL_PE_COOLING_INFO,  //Attribute
                                         FBE_ENCL_COOLING_COMPONENT,  // component type
                                         fan,  // component index
                                         sizeof(edal_pe_cooling_sub_info_t),  // buffer length
                                         (fbe_u8_t *)pPeCoolingSubInfo);  // buffer pointer

        if(!EDAL_STAT_OK(edal_status))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }  

        /* Set the Persistent Multi Fan Fault */
        pPeCoolingSubInfo->externalCoolingInfo.persistentMultiFanModuleFault = persistentMultiFanModuleFault;

        /* Set the Timestamp for First Multi Fan Fault */
        pPeCoolingSubInfo->timeStampForMultiFaultedStateStart = timeStampForMultiFaultedStateStart;

        /* Update EDAL with new Cooling info. */
        edal_status = fbe_edal_setBuffer(edal_block_handle, 
                                         FBE_ENCL_PE_COOLING_INFO,  //Attribute
                                         FBE_ENCL_COOLING_COMPONENT,  // component type
                                         fan,  // component index
                                         sizeof(edal_pe_cooling_sub_info_t),  // buffer length
                                         (fbe_u8_t *)pPeCoolingSubInfo);  // buffer pointer
    
        if(!EDAL_STAT_OK(edal_status))
        {
            return FBE_STATUS_GENERIC_FAILURE;  
        }  
    }

    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *   @fn fbe_base_board_set_io_insert_bit(fbe_base_board_t * pbase_board, 
                                  edal_pe_io_comp_sub_info_t *pPeIoCompSubInfo, 
                                  SPECL_IO_STATUS  *newSpeclIoStatus)     
 **************************************************************************
 *  @brief
 *      This function checks the IOModule/IOAnnex status and sets the
 *      insert bit based on few checks.
 *
 *  @param pbase_board - The pointer to the fbe_base_board_t.
 *  @param pPeIoCompSubInfo - The pointer to the IO Module/IO Annex info.
 *  @param newSpeclIoStatus - The pointer to the Specl Status.
 *
 *  @return fbe_status_t 
 *              FBE_STATUS_OK - successful, 
 *              otherwise - failed.
 *
 *  @version
 *    19-Oct-2010: Arun S - Created.
 *
 **************************************************************************/

static fbe_status_t
fbe_base_board_set_io_insert_bit(fbe_base_board_t * pBaseBoard, 
                                  edal_pe_io_comp_sub_info_t *pPeIoCompSubInfo, 
                                  SPECL_IO_STATUS  *newSpeclIoStatus)
{
    fbe_edal_block_handle_t             edalBlockHandle = NULL;
    fbe_board_mgmt_io_comp_info_t     * pExtPeIoCompInfo = NULL;

    fbe_base_board_get_edal_block_handle(pBaseBoard, &edalBlockHandle);

    pExtPeIoCompInfo = &pPeIoCompSubInfo->externalIoCompInfo;

    if(newSpeclIoStatus->inserted)
    {
        if((pExtPeIoCompInfo->uniqueID == NO_MODULE)&& 
           (!newSpeclIoStatus->uniqueIDFinal)) 
        {
            /* Do not expose the inserted IO module/Base module if the uniqueID is not available yet
             * When the IO module/Base module just got inserted. 
             */
            pExtPeIoCompInfo->inserted = FBE_MGMT_STATUS_FALSE;
        }
        else
        {
            pExtPeIoCompInfo->inserted = FBE_MGMT_STATUS_TRUE;
        }
    }
    else
    {
        /* newSpeclIoStatus->inserted is false, there is no IOM  in this slot */
        /* IO Module/IO Annex is just removed */
        pExtPeIoCompInfo->inserted = FBE_MGMT_STATUS_FALSE;
    }

    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *   @fn fbe_base_board_set_io_uniqueID(SPECL_IO_STATUS  *newSpeclIoStatus,
 *                                  edal_pe_io_comp_sub_info_t *pPeIoCompInfo)     
 **************************************************************************
 *  @brief
 *      This function checks the IOModule/IOAnnex status and sets the uniqueID.
 *
 *  @param newSpeclIoStatus - The new Specl IO status data.
 *  @param pPeIoCompSubInfo - The new IOModule/IOAnnex information read from EDAL.
 *
 *  @return fbe_status_t 
 *              FBE_STATUS_OK - successful, 
 *              otherwise - failed.
 *
 *  @version
 *    25-Oct-2010: Arun S - Created.
 *
 **************************************************************************/

static fbe_status_t
fbe_base_board_set_io_uniqueID(edal_pe_io_comp_sub_info_t *pPeIoCompSubInfo, SPECL_IO_STATUS  *newSpeclIoStatus)
{  
    if(newSpeclIoStatus->uniqueID == NO_MODULE)
    {
        if((newSpeclIoStatus->inserted) && (pPeIoCompSubInfo->inserted))
        {
            /* Do not need to change pPeIoCompSubInfo->externalIoCompInfo.uniqueID
             * because the inserted-ness was not changed.
             */
            //Do nothing here.
        }
        else
        {
            /* For all the other cases, use the new Specl Unique ID data */
            pPeIoCompSubInfo->externalIoCompInfo.uniqueID = newSpeclIoStatus->uniqueID;
        }
    }
    else
    {
        /* For all the other cases, use the new Specl Unique ID data */
        pPeIoCompSubInfo->externalIoCompInfo.uniqueID = newSpeclIoStatus->uniqueID;
    }

    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_board_setPsInputPower
 **************************************************************************
 *  @brief
 *      This function checks various PS info and sets the InputPower &
 *      Status appropriately.
 *
 *  @param pSpeclPowerSupplyStatus - The pointer to SPECL_PS_STATUS.
 *  @param pExtPowerSupplyInfo - The pointer to fbe_power_supply_info_t
 *
 *  @return None
 * 
 *  @note
 *
 *  @version
 *    09-Dec-2010: Joe Perry - Created.
 *
 **************************************************************************/
static void 
fbe_base_board_setPsInputPower(SPECL_PS_STATUS  *pSpeclPowerSupplyStatus,
                               fbe_eir_input_power_sample_t *inputPowerPtr)
{

    inputPowerPtr->inputPower = 0;        
    inputPowerPtr->status = FBE_ENERGY_STAT_INVALID;

    if (!pSpeclPowerSupplyStatus->inserted)
    {
        inputPowerPtr->status = FBE_ENERGY_STAT_NOT_PRESENT;
    }
    else if (pSpeclPowerSupplyStatus->generalFault)
    {
        inputPowerPtr->status = FBE_ENERGY_STAT_FAILED;
    }
    else if (!(pSpeclPowerSupplyStatus->energyStarCompliant))
    {
        inputPowerPtr->status = FBE_ENERGY_STAT_UNSUPPORTED;
    }
    else
    {
        inputPowerPtr->status = FBE_ENERGY_STAT_VALID;
        inputPowerPtr->inputPower = pSpeclPowerSupplyStatus->inputPower;
    }


}   // end of fbe_base_board_setPsInputPower

/*!*************************************************************************
 *  @fn fbe_base_board_saveEirSample
 **************************************************************************
 *  @brief
 *      This function checks various PS info and sets the InputPower &
 *      Status appropriately.
 *
 *  @param base_board - The pointer to base board object
 *  @param inputPowerPtr - The pointer to InputPower sample
 *
 *  @return None
 * 
 *  @note
 *
 *  @version
 *    08-Feb-2011: Joe Perry - Created.
 *
 **************************************************************************/
static void 
fbe_base_board_saveEirSample(fbe_base_board_t *base_board,
                             fbe_u32_t psIndex,
                             fbe_eir_input_power_sample_t *inputPowerPtr)
{
    // save sample
    base_board->inputPowerSamples[psIndex][base_board->sampleIndex].inputPower = 
        inputPowerPtr->inputPower;
    base_board->inputPowerSamples[psIndex][base_board->sampleIndex].status = 
        inputPowerPtr->status;
    fbe_base_object_trace((fbe_base_object_t *)base_board,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, save sample, sampleIndex %d, ps %d, inputPower %d, status 0x%x\n", 
                          __FUNCTION__,
                          base_board->sampleIndex,
                          psIndex,
                          base_board->inputPowerSamples[psIndex][base_board->sampleIndex].inputPower,
                          base_board->inputPowerSamples[psIndex][base_board->sampleIndex].status);

}   // end of fbe_base_board_saveEirSample

/*!*************************************************************************
 *  @fn fbe_base_board_processEirSample
 **************************************************************************
 *  @brief
 *      This function performs calculation (average) on the PS InputPower
 *      samples
 *
 *  @param base_board - The pointer to base board object
 *  @param psCount - number of PS's
 *
 *  @return None
 * 
 *  @note
 *
 *  @version
 *    08-Feb-2011: Joe Perry - Created.
 *
 **************************************************************************/
static void 
fbe_base_board_processEirSample(fbe_base_board_t *base_board,
                                fbe_u32_t psCount)
{
    fbe_u32_t                   averageSum = 0, averageCount = 0;
    fbe_u32_t                   psIndex;
    fbe_u32_t                   sampleIndex;
    fbe_eir_input_power_sample_t *inputPowerPtr;

    if (base_board->sampleCount < FBE_BASE_BOARD_EIR_SAMPLE_COUNT)
    {
        base_board->sampleCount++; 
    }

    fbe_base_object_trace((fbe_base_object_t *)base_board,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, sampleCount %d, sampleIndex %d, EIR_SAMPLE_COUNT %d\n", 
                          __FUNCTION__,
                          base_board->sampleCount,
                          base_board->sampleIndex,
                          FBE_BASE_BOARD_EIR_SAMPLE_COUNT);

    if (base_board->sampleCount < FBE_BASE_BOARD_EIR_SAMPLE_COUNT)
    {
        base_board->inputPower.status = FBE_ENERGY_STAT_SAMPLE_TOO_SMALL;
        base_board->inputPower.inputPower = 0;
    }
    else if (++base_board->sampleIndex >= FBE_BASE_BOARD_EIR_SAMPLE_COUNT)
    {
        // calculate average
        base_board->inputPower.status = 0;
        for (sampleIndex = 0; sampleIndex < FBE_BASE_BOARD_EIR_SAMPLE_COUNT; sampleIndex++)
        {
            for (psIndex = 0; psIndex < psCount; psIndex++)
            {
                inputPowerPtr = &(base_board->inputPowerSamples[psIndex][sampleIndex]);
                fbe_base_object_trace((fbe_base_object_t *)base_board,
                                      FBE_TRACE_LEVEL_DEBUG_HIGH,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "  %s, sampleIndex %d, ps %d, inputPower %d, status 0x%x\n", 
                                      __FUNCTION__,
                                      sampleIndex,
                                      psIndex,
                                      inputPowerPtr->inputPower,
                                      inputPowerPtr->status);
                base_board->inputPower.status |= inputPowerPtr->status;
                if (inputPowerPtr->status == FBE_ENERGY_STAT_VALID)
                {
                    averageSum += inputPowerPtr->inputPower;
                }
                // clear sample now
                inputPowerPtr->inputPower = 0;
                inputPowerPtr->status = FBE_ENERGY_STAT_UNINITIALIZED;
            }
            averageCount++;
        }
        if (averageCount > 0)
        {
            base_board->inputPower.inputPower = averageSum / averageCount;
            fbe_base_object_trace((fbe_base_object_t *)base_board,
                                  FBE_TRACE_LEVEL_DEBUG_HIGH,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, sum %d, cnt %d, average %d, status 0x%x\n", 
                                  __FUNCTION__,
                                  averageSum, averageCount, 
                                  base_board->inputPower.inputPower,
                                  base_board->inputPower.status);
        }

        base_board->sampleIndex = 0;
    }

}   // end of fbe_base_board_processEirSample


/*!*************************************************************************
 *  @fn fbe_base_board_initBmcBistStatus
 **************************************************************************
 *  @brief
 *      This function sets BIST statuses to PASSED.
 *
 *  @param pBistReport - The pointer to BIST status
 *
 *  @return None
 * 
 *  @note
 *
 *  @version
 *    18-Aug-2015: Joe Perry - Created.
 *
 **************************************************************************/
static void fbe_base_board_initBmcBistStatus(fbe_board_mgmt_bist_info_t *pBistReport)
{
    fbe_u32_t       index;

    fbe_zero_memory(pBistReport, sizeof(fbe_board_mgmt_bist_info_t));
    pBistReport->cpuTest = BMC_BIST_PASSED;
    pBistReport->dramTest = BMC_BIST_PASSED;
    pBistReport->sramTest = BMC_BIST_PASSED;
    for (index = 0; index < I2C_TEST_MAX; index++)
    {
        pBistReport->i2cTests[index] = BMC_BIST_PASSED;
    }
    for (index = 0; index < UART_TEST_MAX; index++)
    {
        pBistReport->uartTests[index] = BMC_BIST_PASSED;
    }
    pBistReport->sspTest = BMC_BIST_PASSED;
    for (index = 0; index < BBU_TEST_MAX; index++)
    {
        pBistReport->bbuTests[index] = BMC_BIST_PASSED;
    }
    pBistReport->nvramTest = BMC_BIST_PASSED;
    pBistReport->sgpioTest = BMC_BIST_PASSED;
    for (index = 0; index < FAN_TEST_MAX; index++)
    {
        pBistReport->fanTests[index] = BMC_BIST_PASSED;
    }
    for (index = 0; index < ARB_TEST_MAX; index++)
    {
        pBistReport->arbTests[index] = BMC_BIST_PASSED;
    }
    pBistReport->dedicatedLanTest = BMC_BIST_PASSED;
    pBistReport->ncsiLanTest = BMC_BIST_PASSED;

}   // end of fbe_base_board_initBmcBistStatus


// End of file

