/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008 - 2010
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_base_board_pe_shim.c
 ***************************************************************************
 * @brief
 *  The routines in this file read the specl data.   
 *  
 * @version:
 *  22-Jul-2010 Arun S - Created.                  
 ***************************************************************************/

#include "fbe/fbe_winddk.h"
#include "fbe_topology.h"
#include "fbe_base_board.h"
#include "base_board_private.h"
#include "base_board_pe_private.h"
#include "edal_processor_enclosure_data.h"
#include "fbe_device_types.h"
#include "fbe_pe_types.h"

#include "specl_interface.h"

#ifndef ALAMOSA_WINDOWS_ENV
#if !defined(UMODE_ENV) && !defined(SIMMODE_ENV)
#include "spid_kernel_interface.h"
#endif // !defined(UMODE_ENV) && !defined(SIMMODE_ENV)
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - PCPC */



#if !defined(UMODE_ENV) && !defined(SIMMODE_ENV)
#include "ASEhwWatchDogApi.h"

static void fbe_base_board_hwErrMonCallbackFunction(PVOID context, 
                                                    ULONG value, 
                                                    HWERRMON_ERROR_DEFINE Error);
#endif // !defined(UMODE_ENV) && !defined(SIMMODE_ENV)


/*!*************************************************************************
 *  @fn fbe_base_board_pe_get_io_comp_status
 **************************************************************************
 *  @brief
 *      This function gets the IO module and IO annex status from SPECL.
 *
 *  @param pSpeclIoSum- The pointer to SPECL_IO_SUMMARY.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    12-Apr-2010: PHE - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_pe_get_io_comp_status(SPECL_IO_SUMMARY * pSpeclIoSum)
{
    if(speclGetIOModuleStatus(pSpeclIoSum) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
    
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_get_mezzanine_status
 **************************************************************************
 *  @brief
 *      This function gets the Mezzanine status from SPECL.
 *
 *  @param pSpeclMezzanineSum- The pointer to SPECL_IO_SUMMARY.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    12-Apr-2010: PHE - Created.
 *    30-May-2012: JY  - Modified.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_pe_get_mezzanine_status(SPECL_IO_SUMMARY * pSpeclIoSum)
{
    if(speclGetIOModuleStatus(pSpeclIoSum) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK ;
    }
    
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_get_temperature_status
 **************************************************************************
 *  @brief
 *      This function gets the Temperature status from SPECL.
 *
 *  @param pSpeclTempSum- The pointer to SPECL_TEMPERATURE_SUMMARY.
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
fbe_status_t fbe_base_board_pe_get_temperature_status(SPECL_TEMPERATURE_SUMMARY * pSpeclTempSum)
{
    if(speclGetTemperatureStatus(pSpeclTempSum) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK ;
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_get_mplxr_status
 **************************************************************************
 *  @brief
 *      This function gets the Mezzanine status from SPECL.
 *
 *  @param pSpeclMezzanineSum- The pointer to SPECL_MEZZANINE_SUMMARY.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    25-May-2010: PHE - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_pe_get_mplxr_status(SPECL_MPLXR_SUMMARY * pSpeclMplxrSum)
{
    if(speclGetMplxrStatus(pSpeclMplxrSum) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK ;
    }
    
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_get_led_status
 **************************************************************************
 *  @brief
 *      This function gets the LED status from SPECL.
 *
 *  @param pSpeclMezzanineSum- The pointer to SPECL_LED_SUMMARY.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    12-Apr-2010: PHE - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_pe_get_led_status(SPECL_LED_SUMMARY * pSpeclLedSum)
{
    if(speclGetLEDStatus(pSpeclLedSum) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
    
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_get_misc_status
 **************************************************************************
 *  @brief
 *      This function gets the Misc status from SPECL.
 *
 *  @param pSpeclMiscSum- The pointer to SPECL_MISC_SUMMARY.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    12-Apr-2010: PHE - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_pe_get_misc_status(SPECL_MISC_SUMMARY * pSpeclMiscSum)
{
    if(speclGetMiscStatus(pSpeclMiscSum) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK ;
    }
    
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_get_flt_exp_status
 **************************************************************************
 *  @brief
 *      This function gets the fault expander status from SPECL.
 *
 *  @param pSpeclMiscSum- The pointer to SPECL_FLT_EXP_SUMMARY.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    12-Apr-2010: PHE - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_pe_get_flt_exp_status(SPECL_FLT_EXP_SUMMARY * pSpeclFltExpSum)
{
    if(speclGetFaultExpanderStatus(pSpeclFltExpSum) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK ;
    }
    
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_get_slave_port_status
 **************************************************************************
 *  @brief
 *      This function gets the fault expander status from SPECL.
 *
 *  @param pSpeclMiscSum- The pointer to SPECL_FLT_EXP_SUMMARY.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    12-Apr-2010: PHE - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_pe_get_slave_port_status(SPECL_SLAVE_PORT_SUMMARY * pSpeclSlavePortSum)
{
    if(speclGetPeerSoftwareBootStatus(pSpeclSlavePortSum) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK ;
    }
    
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_get_mgmt_module_status
 **************************************************************************
 *  @brief
 *      This function gets the Mgmt module status from SPECL.
 *
 *  @param pSpeclMgmtSum- The pointer to SPECL_MGMT_SUMMARY.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    16-Apr-2010: Arun S - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_pe_get_mgmt_module_status(SPECL_MGMT_SUMMARY * pSpeclMgmtSum)
{
    if(speclGetMgmtStatus(pSpeclMgmtSum) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
	
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_get_cooling_status
 **************************************************************************
 *  @brief
 *      This function gets the Blower status from SPECL.
 *
 *  @param pSpeclFanSum - The pointer to SPECL_FAN_SUMMARY.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    16-Apr-2010: Arun S - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_pe_get_cooling_status(SPECL_FAN_SUMMARY * pSpeclFanSum)
{
    if(speclGetFanStatus(pSpeclFanSum) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_get_power_supply_status
 **************************************************************************
 *  @brief
 *      This function gets the Power Supply status from SPECL.
 *
 *  @param pSpeclPsSum - The pointer to SPECL_PS_SUMMARY.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    16-Apr-2010: Arun S - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_pe_get_power_supply_status(SPECL_PS_SUMMARY * pSpeclPsSum)
{
    if(speclGetPowerSupplyStatus(pSpeclPsSum) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
	
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_get_sps_status
 **************************************************************************
 *  @brief
 *      This function gets the SPS status from SPECL.
 *
 *  @param pSpeclPsSum - The pointer to SPECL_SPS_SUMMARY.
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
fbe_status_t fbe_base_board_pe_get_sps_status(SPECL_SPS_SUMMARY * pSpeclSpsSum)
{
    if(speclGetSpsStatus(pSpeclSpsSum) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
	
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_get_sps_resume
 **************************************************************************
 *  @brief
 *      This function gets the SPS Resume PROM from SPECL.
 *
 *  @param pSpeclSpsResume - The pointer to PSPECL_SPS_RESUME.
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
fbe_status_t fbe_base_board_pe_get_sps_resume(PSPECL_SPS_RESUME  pSpeclSpsResume)
{
    if(speclGetSpsResume(pSpeclSpsResume) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
	
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_get_battery_status
 **************************************************************************
 *  @brief
 *      This function gets the Battery status from SPECL.
 *
 *  @param pSpeclPsSum - The pointer to SPECL_BATTERY_SUMMARY.
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
fbe_status_t fbe_base_board_pe_get_battery_status(SPECL_BATTERY_SUMMARY * pSpeclBatterySum)
{
    if(speclGetBatteryStatus(pSpeclBatterySum) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
	
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_get_suitcase_status
 **************************************************************************
 *  @brief
 *      This function gets the Suitcase status from SPECL.
 *
 *  @param pSpeclSuitcaseSum - The pointer to SPECL_SUITCASE_SUMMARY.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    16-Apr-2010: Arun S - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_pe_get_suitcase_status(SPECL_SUITCASE_SUMMARY * pSpeclSuitcaseSum)
{
    if(speclGetSuitcaseStatus(pSpeclSuitcaseSum) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
	
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_get_bmc_status
 **************************************************************************
 *  @brief
 *      This function gets the BMC summary from SPECL.
 *
 *  @param pSpeclBmcSum - The pointer to SPECL_BMC_SUMMARY.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    11-Sep-2012: Eric Zhou - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_pe_get_bmc_status(SPECL_BMC_SUMMARY * pSpeclBmcSum)
{
    if(speclGetBMCStatus(pSpeclBmcSum) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_get_cache_card_status
 **************************************************************************
 *  @brief
 *      This function gets the cache card summary from SPECL.
 *
 *  @param pSpeclCacheCardSum - The pointer to SPECL_CACHE_CARD_SUMMARY.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    25-Jan-2013: Rui Chang - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_pe_get_cache_card_status(SPECL_CACHE_CARD_SUMMARY * pSpeclCacheCardSum)
{
    if(speclGetCacheCardStatus(pSpeclCacheCardSum) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_get_dimm_status
 **************************************************************************
 *  @brief
 *      This function gets the DIMM summary from SPECL.
 *
 *  @param pSpeclCacheCardSum - The pointer to SPECL_EHORNET_SUMMARY.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    15-Apr-2013: Rui Chang - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_pe_get_dimm_status(SPECL_DIMM_SUMMARY * pSpeclDimmSum)
{
    if(speclGetDimmStatus(pSpeclDimmSum) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_set_enclFaultLED
 **************************************************************************
 *  @brief
 *      This function sets the Enclosure Fault LED in PE.
 *
 *  @param blink_rate- The pointer to LED_BLINK_RATE.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    07-May-2010: Arun S - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_set_enclFaultLED(LED_BLINK_RATE blink_rate)
{
    if(speclSetEnclosureFaultLED(blink_rate) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
    
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_set_UnsafetoRemoveLED
 **************************************************************************
 *  @brief
 *      This function sets the unsafe to Remove LED in PE.
 *
 *  @param blink_rate- The pointer to LED_BLINK_RATE.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    07-May-2010: Arun S - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_set_UnsafetoRemoveLED(LED_BLINK_RATE blink_rate)
{
    if(speclSetUnsafeToRemoveLED(blink_rate) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
    
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_set_spFaultLED
 **************************************************************************
 *  @brief
 *      This function sets the SP fault LED in PE.
 *
 *  @param blink_rate - LED_BLINK_RATE.
 *  @param status_condition - status_condition.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    10-May-2010: Arun S - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_set_spFaultLED(LED_BLINK_RATE blink_rate,
                                           fbe_u32_t status_condition)
{
    if(speclSetSPFaultLED(blink_rate, status_condition) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
    
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_set_iomFaultLED
 **************************************************************************
 *  @brief
 *      This function sets the IO module fault LED in PE.
 *
 *  @param sp_id - SP id.
 *  @param io_module - IO module for which the fault LED task to be carried on.
 *  @param blink_rate - LED_BLINK_RATE.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    10-May-2010: Arun S - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_set_iomFaultLED(SP_ID sp_id, fbe_u32_t io_module, LED_BLINK_RATE blink_rate)
{
    if(speclSetIOModuleFaultLED(sp_id, io_module, blink_rate) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
    
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_set_iom_port_LED
 **************************************************************************
 *  @brief
 *      This function sets the IO module Port LED in PE.
 *
 *  @param sp_id - SP id.
 *  @param io_module - IO module for which the port belong to.
 *  @param io_port - Port for which the LED to be set.
 *  @param blink_rate - LED_BLINK_RATE.
 *  @param led_color - LED color.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    10-May-2010: Arun S - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_set_iom_port_LED(SP_ID sp_id, 
                                             fbe_u32_t io_module, 
                                             LED_BLINK_RATE blink_rate,
                                             fbe_u32_t io_port,
                                             LED_COLOR_TYPE led_color)
{
    if(speclSetIOModulePortLED(sp_id, io_module, io_port, blink_rate, led_color) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
    
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_set_mgmt_vlan_config_mode
 **************************************************************************
 *  @brief
 *      This function sets the Mgmt module Vlan config mode in PE.
 *
 *  @param sp_id - SP id.
 *  @param vlan_mode - vlan config mode for Mgmt module.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    10-May-2010: Arun S - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_set_mgmt_vlan_config_mode(SP_ID sp_id, VLAN_CONFIG_MODE vlan_mode)
{
    if(speclSetMgmtvLanConfig(sp_id, vlan_mode) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
    
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_set_mgmt_fault_LED
 **************************************************************************
 *  @brief
 *      This function sets the Mgmt module fault LED in PE.
 *
 *  @param sp_id - SP id.
 *  @param blink_rate - LED blink rate for Mgmt module fault.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    10-May-2010: Arun S - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_set_mgmt_fault_LED(SP_ID sp_id, LED_BLINK_RATE blink_rate)
{
    if(speclSetMgmtFaultLED(sp_id, blink_rate) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
    
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_set_mgmt_port
 **************************************************************************
 *  @brief
 *      This function calls the appropriate SPECL call to set the Mgmt port in PE.
 *
 *  @param sp_id - SP id.
 *  @param blink_rate - LED blink rate for Mgmt module fault.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    10-May-2010: Arun S - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_set_mgmt_port(fbe_base_board_t * base_board,
                                          SP_ID sp_id, PORT_ID_TYPE portIDType, 
                                          fbe_mgmt_port_auto_neg_t mgmtPortAutoNeg,
                                          fbe_mgmt_port_speed_t mgmtPortSpeed,
                                          fbe_mgmt_port_duplex_mode_t mgmtPortDuplex)
{
    fbe_bool_t autoNegotiate;
    PORT_SPEED portSpeed = SPEED_RESERVED_Mbps;
    PORT_DUPLEX portDuplex = FULL_DUPLEX;

    switch(mgmtPortAutoNeg)
    {
    case FBE_PORT_AUTO_NEG_OFF:
        autoNegotiate = FBE_FALSE;
        break;
        
    case FBE_PORT_AUTO_NEG_ON:
        autoNegotiate = FBE_TRUE;
        break;
        
    default:
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s invalid mgmtPortAutoNeg %d\n", 
                              __FUNCTION__, mgmtPortAutoNeg);
        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    switch(mgmtPortSpeed)
    {
    case FBE_MGMT_PORT_SPEED_1000MBPS:
        portSpeed = SPEED_1000_Mbps;
        break;
    case FBE_MGMT_PORT_SPEED_100MBPS:
        portSpeed = SPEED_100_Mbps;
        break;
    case FBE_MGMT_PORT_SPEED_10MBPS:
        portSpeed = SPEED_10_Mbps;
        break;
    default:
        if(autoNegotiate != FBE_TRUE)
        {
            fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s invalid mgmtPortSpeed %d\n", 
                                  __FUNCTION__, mgmtPortSpeed);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        
        break;
    }

    switch(mgmtPortDuplex)
    {
    case FBE_PORT_DUPLEX_MODE_HALF:
        portDuplex = HALF_DUPLEX;
        break;
    case FBE_PORT_DUPLEX_MODE_FULL:
        portDuplex = FULL_DUPLEX;
        break;
    default:
        if(autoNegotiate != FBE_TRUE)
        {
            fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s invalid mgmtPortDuplex %d\n", 
                                  __FUNCTION__, mgmtPortDuplex);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        break;
    }

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s with AutoNeg:%d Speed:0x%x Duplex:%d\n", 
                              __FUNCTION__,autoNegotiate, portSpeed, portDuplex);
    

    if(speclSetMgmtPort(sp_id, portIDType, autoNegotiate, portSpeed, portDuplex)== EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Got bad status from specl to set Mgmt Port with AutoNeg:%d Speed:0x%x Duplex:%d\n", 
                              __FUNCTION__,autoNegotiate, portSpeed, portDuplex);
    
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_set_PostAndOrReset
 **************************************************************************
 *  @brief
 *      This function calls the appropriate SPECL call to set the 
 *      Post and Reset status in PE.
 *
 *  @param sp_id - SP id.
 *  @param holdInPost - Boolean.
 *  @param holdInReset - Boolean.
 *  @param rebootBlade - Boolean.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    10-May-2010: Arun S - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_set_PostAndOrReset(SP_ID sp_id, 
                                               fbe_bool_t holdInPost,
                                               fbe_bool_t holdInReset,
                                               fbe_bool_t rebootBlade)
{

#ifndef ALAMOSA_WINDOWS_ENV
#if !defined(UMODE_ENV) && !defined(SIMMODE_ENV)
    SPID                        SpInfo = {};
    EMCPAL_STATUS               status = EMCPAL_STATUS_SUCCESS;
    status = spidGetSpid(&SpInfo);

    if ( ! EMCUTIL_SUCCESS( status ))
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //
    //  If we are only rebooting, then we need to actually exit()
    //  the flare process and not WARM reboot the SP.  The SP on
    //  Linux needs to shutdown cleanly without a HW reboot and without
    //  a core being generated.
    //
    //  Note:  This change is not being made in specl itself because the ioctl
    //         used for specl to WARM Reboot the SP still has legitimate 
    //         reboot cases where the SP needs to be WARM restarted.
    //         
    //         Also, the rebootPeer case is not being handled here because there's 
    //         a way currently to tell the peer to just "exit(0)" his flare process.
    //
    if ( rebootBlade && !holdInPost && !holdInReset )
    {
        //
        //  If the SP we are rebooting is our own, then exit().
        //
        if (sp_id == SpInfo.engine )
        { 
            //
            //  Wait a couple of seconds so that the ktrace message above gets written out to disk before we reboot.
            //
            fbe_thread_delay(2000);

            //
            //  This will terminate the Flare process on Linux without core dumping.
            //
            CSX_EXIT(1);
        }
    }

    //
    //  Fallthrough to the existing FBE logic.  This will allow this SP to still
    //  warm reboot the peer if that is required. Or it will allow this logic to use the 
    //  HoldInRest or HoldInPost logic; if required.
    //
#endif // #if !defined(UMODE_ENV) && !defined(SIMMODE_ENV)
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - PCPC */

    if(EMCPAL_STATUS_SUCCESS == speclSetClearHoldInPostAndOrReset(sp_id, 
                                                                  holdInPost, 
                                                                  holdInReset, 
                                                                  rebootBlade, 
                                                                  DRIVER_ESP, 
                                                                  REASON_FBE_BASE_REQUESTED,
                                                                  0))
    {
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_set_resume
 **************************************************************************
 *  @brief
 *      This function calls the appropriate SPECL call to set the 
 *      resume prom for a particular SMB Device in PE.
 *
 *  @param device - SMB Device for which the Resume info has to be set.
 *  @param resume_prom - Resume information.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    10-May-2010: Arun S - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_set_resume(SMB_DEVICE device, 
                                       RESUME_PROM_STRUCTURE * pResumePromStruture)
{
    if(speclSetResume(device, pResumePromStruture) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
    
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_get_resume
 **************************************************************************
 *  @brief
 *      This function calls the appropriate SPECL call to get the 
 *      resume prom data for a particular SMB Device in PE.
 *
 *  @param device - SMB Device for which the Resume info has to be get.
 *  @param resume_data - Resume information.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    20-May-2010: Arun S - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_get_resume(SMB_DEVICE smb_device, 
                                       SPECL_RESUME_DATA *resume_data)
{
    if(speclGetResume(smb_device, resume_data) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
    
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_get_elapsed_time_data
 **************************************************************************
 *  @brief
 *      This function gets the time converted in milliseconds.
 *
 *  @param out_elapsed_time - The pointer to the converted Time.
 *  @param in_elapsed_time - The Time to be converted.
 *  @param convert_time - Conversion unit.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    11-June-2010: Arun S - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_get_elapsed_time_data(fbe_u64_t *out_elapsed_time, 
                                                  fbe_u64_t in_elapsed_time, 
                                                  SPECL_TIMESCALE_UNIT convert_time)
{
    EMCPAL_LARGE_INTEGER inTime;
    EMCPAL_LARGE_INTEGER outTime;

    inTime.QuadPart = in_elapsed_time;
    outTime.QuadPart = *out_elapsed_time;

    if(speclGetElapsedTimeData(&outTime, inTime, convert_time) == EMCPAL_STATUS_SUCCESS)
    {
        *out_elapsed_time = outTime.QuadPart;
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_start_sps_testing
 **************************************************************************
 *  @brief
 *      This function starts an SPS test via SPECL.
 *
 *  @param
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
fbe_status_t fbe_base_board_pe_start_sps_testing(void)
{
    if(speclStartSpsTest() == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
	
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_stop_sps_testing
 **************************************************************************
 *  @brief
 *      This function stops an SPS test via SPECL.
 *
 *  @param
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
fbe_status_t fbe_base_board_pe_stop_sps_testing(void)
{
    if(speclStopSpsTest() == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
	
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_shutdown_sps
 **************************************************************************
 *  @brief
 *      This function stops an SPS test via SPECL.
 *
 *  @param
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
fbe_status_t fbe_base_board_pe_shutdown_sps(void)
{
    if(speclShutdownSps(FALSE) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
	
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_set_sps_present
 **************************************************************************
 *  @brief
 *      This function willl enable/disable SPS Present setting via SPECL.
 *
 *  @param
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    22-Sep-2012: Joe Perry - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_pe_set_sps_present(SP_ID spid, fbe_bool_t spsPresent)
{
    if(speclSetSpsPresent(spid, spsPresent) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_set_sps_fastSwitchover
 **************************************************************************
 *  @brief
 *      This function willl enable/disable SPS Fast Switchover setting via SPECL.
 *
 *  @param spsFastSwitchover - enable/disable setting
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    03-Mar-2013: Joe Perry - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_pe_set_sps_fastSwitchover(fbe_bool_t spsFastSwitchover)
{
    if(speclSetSpsFastSwitchover(spsFastSwitchover) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_start_battery_testing
 **************************************************************************
 *  @brief
 *      This function starts a Battery test via SPECL.
 *
 *  @param spId - SP ID
 *  @param slot - slot number of the battery
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
fbe_status_t fbe_base_board_pe_start_battery_testing(SP_ID spid, fbe_u8_t slot)
{
    if(speclSetBatterySelfTest(spid, slot, TRUE) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
	
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_stop_battery_testing
 **************************************************************************
 *  @brief
 *      This function stops a Battery test via SPECL.
 *
 *  @param spId - SP ID
 *  @param slot - slot number of the battery
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
fbe_status_t fbe_base_board_pe_stop_battery_testing(SP_ID spid, fbe_u8_t slot)
{
    if(speclSetBatterySelfTest(spid, slot, FALSE) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
	
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_enable_battery
 **************************************************************************
 *  @brief
 *      This function enable a Battery via SPECL.
 *
 *  @param spId - SP ID
 *  @param slot - slot number of the battery
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    30-Aug-2012: Rui Chang - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_pe_enable_battery(SP_ID spid, fbe_u8_t slot)
{
    if(speclSetBatteryEnableState(spid, TRUE) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
	
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_disable_battery
 **************************************************************************
 *  @brief
 *      This function disable a Battery via SPECL.
 *
 *  @param spId - SP ID
 *  @param slot - slot number of the battery
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    19-Sep-2012: Rui Chang - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_pe_disable_battery(SP_ID spid, fbe_u8_t slot)
{
    if(speclSetBatteryEnableState(spid, FALSE) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
	
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_set_battery_energy_req
 **************************************************************************
 *  @brief
 *      This function sets the Battery's Energy requirements via SPECL.
 *
 *  @param spId - SP ID
 *  @param slot - slot number of the battery
 *  @param batteryRequirements - energy  requirements
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    15-Nov-2012: Joe Perry - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_pe_set_battery_energy_req(SP_ID spid, fbe_u8_t slot, fbe_base_board_battery_action_data_t batteryRequirements)
{
    if(speclSetBatteryEnergyRequirements(spid, batteryRequirements.setPowerReqInfo.batteryEnergyRequirement) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_GENERIC_FAILURE;
}


/*!*************************************************************************
 *  @fn fbe_base_board_pe_enableRideThroughTimer
 **************************************************************************
 *  @brief
 *      This function enable the BMC Ride Through timer via SPECL.
 *
 *  @param spId - SP ID
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    30-Jun-2015: Joe Perry - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_pe_enableRideThroughTimer(SP_ID spid, fbe_bool_t lowPowerMode, fbe_u8_t vaultTimeout)
{
    if(speclSetVaultModeConfig(spid, lowPowerMode, FBE_BMC_RIDE_THROUGH_TIMER_ENABLE_VALUE, vaultTimeout) 
       == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
	
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_disableRideThroughTimer
 **************************************************************************
 *  @brief
 *      This function disable the BMC Ride Through timer via SPECL.
 *
 *  @param spId - SP ID
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    30-Jun-2015: Joe Perry - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_pe_disableRideThroughTimer(SP_ID spid, fbe_bool_t lowPowerMode, fbe_u8_t vaultTimeout)
{
    if(speclSetVaultModeConfig(spid, lowPowerMode, FBE_BMC_RIDE_THROUGH_TIMER_DISABLE_VALUE, vaultTimeout) 
       == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
	
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  fbe_base_board_set_local_software_boot_status
 **************************************************************************
 *  @brief
 *      This function is to set local software boot status.
 *
 *  @param generalStatus -
 *  @param componentStatus - 
 *  @param componentExtStatus -
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *      04-Aug-2011 PHE - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_set_local_software_boot_status(SW_GENERAL_STATUS_CODE  generalStatus,
                                                    BOOT_SW_COMPONENT_CODE  componentStatus,
                                                    SW_COMPONENT_EXT_CODE  componentExtStatus)
{
    if(speclSetLocalSoftwareBootStatus(generalStatus, componentStatus, componentExtStatus) 
              == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
    return FBE_STATUS_GENERIC_FAILURE;
}
/*************************************************
 *  end of fbe_base_board_set_local_software_boot_status
 ************************************************/


/*!*************************************************************************
 *  fbe_base_board_set_local_flt_exp_status
 **************************************************************************
 *  @brief
 *      This function is to set local fault expander status.
 *
 *  @param generalStatus -
 *  @param componentStatus - 
 *  @param componentExtStatus -
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *      04-Aug-2011 PHE - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_set_local_flt_exp_status(fbe_u8_t faultStatus)
{
    if(speclSetLocalFaultExpanderStatus(faultStatus) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
    return FBE_STATUS_GENERIC_FAILURE;
}
/*************************************************
 *  end of fbe_base_board_set_local_flt_exp_status
 ************************************************/

/*!*************************************************************************
 *  fbe_base_board_sps_download_command
 **************************************************************************
 *  @brief
 *  Wrapper function to send sps download command to specl.
 *
 *  @param imageType - the type of image
 *  @param pImageBuffer - pointer to the image
 *  @param imageLength - image length
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @version
 *  09-Sep-2012 GB - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_sps_download_command(SPS_FW_TYPE imageType, fbe_u8_t *pImageBuffer, fbe_u32_t imageLength)
{
    if(EMCPAL_STATUS_SUCCESS == speclSpsDownloadFirmware(imageType, pImageBuffer, imageLength))
    {
        return FBE_STATUS_OK;
    }
    return FBE_STATUS_GENERIC_FAILURE;
} //fbe_base_board_sps_download_command


/*!*************************************************************************
 *  fbe_base_board_set_io_module_persisted_power_state
 **************************************************************************
 *  @brief
 *  This function sends a request to control the persisted power state of an IO module 
 *  to causing specl to power it on/off on the next boot.
 *
 *  @param sp_id - SP ID
 *  @param io_module - IO module number
 *  @param powerup_enable - enable power T/F
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @version
 *  07-Feb-2013 BP - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_set_io_module_persisted_power_state(SP_ID sp_id, fbe_u32_t io_module, fbe_bool_t powerup_enabled)
{
    if(EMCPAL_STATUS_SUCCESS == speclSetIOModulePersistedPowerState(sp_id, io_module, powerup_enabled))
    {
        return FBE_STATUS_OK;
    }
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 *  fbe_base_board_set_cna_mode
 **************************************************************************
 *  @brief
 *  This function sends a request to set the CNA mode for the ports in this
 *  system.
 *
 *  @param cna_mode - desired cna mode setting
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @version
 *  07-Jan-2015 BP - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_set_cna_mode(SPECL_CNA_MODE cna_mode)
{
    SPECL_STATUS specl_status;
    specl_status = speclSetCnaMode(cna_mode);

    if(specl_status != EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        return FBE_STATUS_OK;
    }
}

/*!*************************************************************************
 *  fbe_base_board_initHwErrMonCallback
 **************************************************************************
 *  @brief
 *  This function initializes notification callback from HwErrMon.
 *
 *  @param base_board - pointer to Base Board object
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @version
 *  10-Feb-2015     Joe Perry - Created.
 *
 **************************************************************************/
#if !defined(UMODE_ENV) && !defined(SIMMODE_ENV)
fbe_status_t fbe_base_board_initHwErrMonCallback(fbe_base_board_t * base_board)
{
    HWERRMON_REG_CALLBACK_IOCTL_INFO    *hwErrMonCallbackInfo;

    hwErrMonCallbackInfo = (HWERRMON_REG_CALLBACK_IOCTL_INFO * )
        EmcpalAllocateUtilityPool(EmcpalNonPagedPool,
                                  sizeof(HWERRMON_REG_CALLBACK_IOCTL_INFO));

    hwErrMonCallbackInfo->Context = base_board;
    hwErrMonCallbackInfo->notify_for_this_device = HwErrMonValidCallBackClients;
    hwErrMonCallbackInfo->Error_Callback = fbe_base_board_hwErrMonCallbackFunction;
    RegisterClientWithHwErrMon(hwErrMonCallbackInfo);

    return FBE_STATUS_OK;

}   // end of fbe_base_board_initHwErrMonCallback


/*!*************************************************************************
 *  fbe_base_board_hwErrMonCallbackFunction
 **************************************************************************
 *  @brief
 *  This function is the notification callback from HwErrMon.  It will
 *  determine the faulted component & set stats accordingly.
 *
 *  @param context - void pointer set to Base Board object
 *  @param value -
 *  @param Error - structure defined by HwErrMon with fault details
 *
 *  @return void - no returned vaule
 *  @version
 *  10-Feb-2015     Joe Perry - Created.
 *
 **************************************************************************/
static void fbe_base_board_hwErrMonCallbackFunction(PVOID context, 
                                                    ULONG value, 
                                                    HWERRMON_ERROR_DEFINE Error)
{
    fbe_base_board_t                * base_board;
    fbe_edal_block_handle_t           edalBlockHandle = NULL;   
    edal_pe_suitcase_sub_info_t     peSuitcaseSubInfo = {0};      
    edal_pe_suitcase_sub_info_t     * pPeSuitcaseSubInfo = &peSuitcaseSubInfo;
    edal_pe_dimm_sub_info_t         peDimmSubInfo = {0};     
    edal_pe_dimm_sub_info_t         * pPeDimmSubInfo = &peDimmSubInfo;
    edal_pe_io_comp_sub_info_t      peIoModuleSubInfo = {0}; 
    edal_pe_io_comp_sub_info_t      * pPeIoModuleSubInfo = &peIoModuleSubInfo;  
    edal_pe_io_comp_sub_info_t      peBemSubInfo = {0}; 
    edal_pe_io_comp_sub_info_t      * pPeBemSubInfo = &peBemSubInfo;
    edal_pe_io_comp_sub_info_t      peMezzanineSubInfo = {0};        
    edal_pe_io_comp_sub_info_t      * pPeMezzanineSubInfo = &peMezzanineSubInfo;
    fbe_edal_status_t               edalStatus = FBE_EDAL_STATUS_OK;
    fbe_u32_t                       compIndex;

    compIndex = Error.FbeDeviceInfo.fbe_num;

    base_board = (fbe_base_board_t *)context;

    fbe_base_board_get_edal_block_handle(base_board, &edalBlockHandle);

    switch (Error.faulted_device_type)
    {
    case FBE_DEVICE_TYPE_SP:
    case FBE_DEVICE_TYPE_SUITCASE:
        // get specified Suitcase status
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_BOARD,
                                 FBE_TRACE_LEVEL_INFO, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                 "%s, fault on SP/SUITCASE, sp %d, compIndex %d\n", 
                                 __FUNCTION__,
                                 Error.sp_id, 
                                 compIndex);
        fbe_zero_memory(pPeSuitcaseSubInfo, sizeof(edal_pe_suitcase_sub_info_t)); 
        edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_SUITCASE_INFO,              //Attribute
                                        FBE_ENCL_SUITCASE,                      // component type
                                        Error.sp_id,                            // component index
                                        sizeof(edal_pe_suitcase_sub_info_t),    // buffer length
                                       (fbe_u8_t *)pPeSuitcaseSubInfo);         // buffer pointer 
        if(!EDAL_STAT_OK(edalStatus))
        {
            return;  
        }  
        // set fault & save
        pPeSuitcaseSubInfo->externalSuitcaseInfo.hwErrMonFault = TRUE;
        edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_SUITCASE_INFO,              //Attribute
                                        FBE_ENCL_SUITCASE,                      // component type
                                        Error.sp_id,                            // component index
                                        sizeof(edal_pe_suitcase_sub_info_t),    // buffer length
                                       (fbe_u8_t *)pPeSuitcaseSubInfo);         // buffer pointer

        if(!EDAL_STAT_OK(edalStatus))
        {
            return;  
        }  
        break;
    case FBE_DEVICE_TYPE_DIMM:
        // get specified DIMM status
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_BOARD,
                                 FBE_TRACE_LEVEL_INFO, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                 "%s, fault on DIMM, sp %d, compIndex %d\n", 
                                 __FUNCTION__,
                                 Error.sp_id, 
                                 compIndex);
        fbe_zero_memory(pPeDimmSubInfo, sizeof(edal_pe_dimm_sub_info_t)); 
        edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_DIMM_INFO,                  //Attribute
                                        FBE_ENCL_DIMM,                          // component type
                                        compIndex,                              // component index
                                        sizeof(edal_pe_dimm_sub_info_t),        // buffer length
                                       (fbe_u8_t *)pPeDimmSubInfo);             // buffer pointer 
        if(!EDAL_STAT_OK(edalStatus))
        {
            return;  
        }  
        // set fault & save
        pPeDimmSubInfo->externalDimmInfo.hwErrMonFault = TRUE;
        edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_DIMM_INFO,                  //Attribute
                                        FBE_ENCL_DIMM,                          // component type
                                        compIndex,                              // component index
                                        sizeof(edal_pe_dimm_sub_info_t),        // buffer length
                                       (fbe_u8_t *)pPeDimmSubInfo);             // buffer pointer 

        if(!EDAL_STAT_OK(edalStatus))
        {
            return;  
        }  
        break;
    case FBE_DEVICE_TYPE_IOMODULE:
        // get specified IoModule/BM status
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_BOARD,
                                 FBE_TRACE_LEVEL_INFO, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                 "%s, fault on IOMODULE, sp %d, compIndex %d\n", 
                                 __FUNCTION__,
                                 Error.sp_id, 
                                 compIndex);
        fbe_zero_memory(pPeIoModuleSubInfo, sizeof(edal_pe_io_comp_sub_info_t)); 
        edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_IO_COMP_INFO,               //Attribute
                                        FBE_ENCL_IO_MODULE,                     // component type
                                        compIndex,                              // component index
                                        sizeof(edal_pe_io_comp_sub_info_t),     // buffer length
                                       (fbe_u8_t *)pPeIoModuleSubInfo);         // buffer pointer 
        if(!EDAL_STAT_OK(edalStatus))
        {
            return;  
        }  
        // set fault & save
        pPeIoModuleSubInfo->externalIoCompInfo.hwErrMonFault = TRUE;
        edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_IO_COMP_INFO,               //Attribute
                                        FBE_ENCL_IO_MODULE,                     // component type
                                        compIndex,                              // component index
                                        sizeof(edal_pe_io_comp_sub_info_t),     // buffer length
                                       (fbe_u8_t *)pPeIoModuleSubInfo);         // buffer pointer 

        if(!EDAL_STAT_OK(edalStatus))
        {
            return;  
        }  
        break;
    case FBE_DEVICE_TYPE_BACK_END_MODULE:
        // get specified IoModule/BM status
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_BOARD,
                                 FBE_TRACE_LEVEL_INFO, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                 "%s, fault on BM, sp %d, compIndex %d\n", 
                                 __FUNCTION__,
                                 Error.sp_id, 
                                 compIndex);
        fbe_zero_memory(pPeBemSubInfo, sizeof(edal_pe_io_comp_sub_info_t)); 
        edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_IO_COMP_INFO,               //Attribute
                                        FBE_ENCL_BEM,                           // component type
                                        Error.sp_id,                            // component index
                                        sizeof(edal_pe_io_comp_sub_info_t),     // buffer length
                                       (fbe_u8_t *)pPeBemSubInfo);              // buffer pointer 
        if(!EDAL_STAT_OK(edalStatus))
        {
            return;  
        }  
        // set fault & save
        pPeBemSubInfo->externalIoCompInfo.hwErrMonFault = TRUE;
        edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_IO_COMP_INFO,               //Attribute
                                        FBE_ENCL_BEM,                           // component type
                                        Error.sp_id,                            // component index
                                        sizeof(edal_pe_io_comp_sub_info_t),     // buffer length
                                       (fbe_u8_t *)pPeBemSubInfo);              // buffer pointer 

        if(!EDAL_STAT_OK(edalStatus))
        {
            return;  
        }  
        break;
    case FBE_DEVICE_TYPE_MEZZANINE:
        // get specified Mezzanine status
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_BOARD,
                                 FBE_TRACE_LEVEL_INFO, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                 "%s, fault on MEZZANINE, sp %d, compIndex %d\n", 
                                 __FUNCTION__,
                                 Error.sp_id, 
                                 compIndex);
        fbe_zero_memory(pPeMezzanineSubInfo, sizeof(edal_pe_io_comp_sub_info_t)); 
        edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_MEZZANINE_INFO,             //Attribute
                                        FBE_ENCL_MEZZANINE,                     // component type
                                        Error.sp_id,                            // component index
                                        sizeof(edal_pe_io_comp_sub_info_t),     // buffer length
                                       (fbe_u8_t *)pPeMezzanineSubInfo);        // buffer pointer 
        if(!EDAL_STAT_OK(edalStatus))
        {
            return;  
        }  
        // set fault & save
        pPeMezzanineSubInfo->externalIoCompInfo.hwErrMonFault = TRUE;
        edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_MEZZANINE_INFO,             //Attribute
                                        FBE_ENCL_MEZZANINE,                     // component type
                                        Error.sp_id,                            // component index
                                        sizeof(edal_pe_io_comp_sub_info_t),     // buffer length
                                       (fbe_u8_t *)pPeMezzanineSubInfo);        // buffer pointer 
        if(!EDAL_STAT_OK(edalStatus))
        {
            return;  
        }  
        break;
    default:
        // Unknown device, so do nothing
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_BOARD,
                                 FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s, fault on unsupported device 0x%016llx\n", 
                                 __FUNCTION__,
                                 Error.faulted_device_type);
        break;
    }
}   // end of fbe_base_board_hwErrMonCallbackFunction
#endif  // !defined(UMODE_ENV) && !defined(SIMMODE_ENV)

/*!*************************************************************************
 *  fbe_base_board_set_local_veeprom_cpu_status
 **************************************************************************
 *  @brief
 *      This function is to set local veeprom cpu status.
 *
 *  @param cpuStatus -
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *      04-July-2015 MCR - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_set_local_veeprom_cpu_status(fbe_u32_t cpuStatus)
{
    if(speclSetLocalVeepromCpuStatus(cpuStatus) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************************
 *  end of fbe_base_board_set_local_veeprom_cpu_status 
 ******************************************************/

/*!*************************************************************************
 *  fbe_base_board_set_local_veeprom_cpu_status
 **************************************************************************
 *  @brief
 *      This function is to returns the SMB Device based on the PCI information
 *      that was passed.
 *
 *  *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *      04-Sept-2015 AT - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_get_smb_device_from_pci(fbe_u32_t pci_bus, fbe_u32_t pci_function, fbe_u32_t pci_slot, 
                                                    fbe_u32_t phy_map, SMB_DEVICE *smb_device)
{
    SPECL_PCI_TO_IO_MODULE_MAPPING    pci_mapping;
    SPECL_STATUS status;

    status = speclMapPCIBusDevFuncToIOModulePortNum(pci_bus, pci_function, pci_slot, phy_map, &pci_mapping);
    if(status == EMCPAL_STATUS_SUCCESS)
    {
        *smb_device = pci_mapping.resumeSmbDevice;
        return FBE_STATUS_OK;
    }
    else
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
}

// End of file

