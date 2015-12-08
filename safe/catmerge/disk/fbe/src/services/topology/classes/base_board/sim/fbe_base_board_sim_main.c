#include "fbe/fbe_winddk.h"
#include "base_board_private.h"
#include "fbe/fbe_terminator_api.h"
#include "specl_interface.h"


static fbe_object_mgmt_attributes_t board_sim_mgmt_attributes = 0; /* Default. */
static fbe_terminator_api_get_board_info_function_t terminator_api_get_board_info_function = NULL;
static fbe_terminator_api_get_sp_id_function_t terminator_api_get_sp_id_function = NULL;
static fbe_terminator_api_is_single_sp_system_function_t terminator_api_is_single_sp_system_function = NULL;
static fbe_terminator_api_process_specl_sfi_mask_data_queue_function_t terminator_api_process_specl_sfi_mask_data_queue_function = NULL;

fbe_status_t
fbe_base_board_set_mgmt_attributes(fbe_object_mgmt_attributes_t mgmt_attributes)
{
    board_sim_mgmt_attributes = mgmt_attributes;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_board_get_mgmt_attributes(fbe_object_mgmt_attributes_t * mgmt_attributes)
{
    *mgmt_attributes = board_sim_mgmt_attributes;
    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_board_get_platform_info
 **************************************************************************
 *  @brief
 *      This function gets the platform info for terminator.
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
fbe_status_t 
fbe_base_board_get_platform_info(SPID_PLATFORM_INFO * platform_info)
{    
    fbe_terminator_board_info_t   board_info;

    terminator_sp_id_t terminator_sp_id;
    SP_ID              sp_id = SP_INVALID;
    fbe_status_t       status = FBE_STATUS_GENERIC_FAILURE;
    DWORD              specl_status = SPECL_SFI_COMMAND_EXECUTED;
    fbe_bool_t         is_single_sp = FBE_FALSE;


    status = terminator_api_get_board_info_function(&board_info);

    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    switch(board_info.platform_type)
    {
        case SPID_ENTERPRISE_HW_TYPE:
            platform_info->platformType = board_info.platform_type; 
            platform_info->cpuModule = TRITON_ERB_CPU_MODULE;
            platform_info->enclosureType = XPE_ENCLOSURE_TYPE;
            platform_info->midplaneType = PROTEUS_MIDPLANE;
            break;

        case SPID_PROMETHEUS_M1_HW_TYPE:
        case SPID_PROMETHEUS_S1_HW_TYPE:
            platform_info->platformType = board_info.platform_type; 
            platform_info->cpuModule = MEGATRON_CPU_MODULE;
            platform_info->enclosureType = XPE_ENCLOSURE_TYPE;
            platform_info->midplaneType = RATCHET_MIDPLANE;
            break;

        case SPID_DEFIANT_M1_HW_TYPE:
        case SPID_DEFIANT_M2_HW_TYPE:
        case SPID_DEFIANT_M3_HW_TYPE:
        case SPID_DEFIANT_M4_HW_TYPE:
        case SPID_DEFIANT_M5_HW_TYPE:
        case SPID_DEFIANT_S1_HW_TYPE:
        case SPID_DEFIANT_S4_HW_TYPE:
        case SPID_DEFIANT_K2_HW_TYPE:
        case SPID_DEFIANT_K3_HW_TYPE:
            platform_info->platformType = board_info.platform_type; 
            platform_info->cpuModule = JETFIRE_CPU_MODULE;
            platform_info->enclosureType = DPE_ENCLOSURE_TYPE;
            platform_info->midplaneType = STRATOSPHERE_MIDPLANE;
            break;

        case SPID_NOVA1_HW_TYPE:
        case SPID_NOVA3_HW_TYPE:
        case SPID_NOVA3_XM_HW_TYPE:
        case SPID_NOVA_S1_HW_TYPE:
        case SPID_BEARCAT_HW_TYPE:
            platform_info->platformType = board_info.platform_type; 
            platform_info->cpuModule = BEACHCOMBER_CPU_MODULE;
            platform_info->enclosureType = DPE_ENCLOSURE_TYPE;
            platform_info->midplaneType = RAMHORN_MIDPLANE;  
            break;
        
        case SPID_MERIDIAN_ESX_HW_TYPE:
            platform_info->platformType = board_info.platform_type; 
            platform_info->cpuModule = MERIDIAN_CPU_MODULE;
            platform_info->enclosureType = VPE_ENCLOSURE_TYPE;
            platform_info->midplaneType = MERIDIAN_MIDPLANE;
            break;
        case SPID_TUNGSTEN_HW_TYPE:
            platform_info->platformType = board_info.platform_type;
            platform_info->cpuModule = TUNGSTEN_CPU_MODULE;
            platform_info->enclosureType = VPE_ENCLOSURE_TYPE;
            platform_info->midplaneType = MERIDIAN_MIDPLANE;
            break;

// Moons
        case SPID_OBERON_1_HW_TYPE:
        case SPID_OBERON_2_HW_TYPE:
        case SPID_OBERON_3_HW_TYPE:
        case SPID_OBERON_4_HW_TYPE:
        case SPID_OBERON_S1_HW_TYPE:
            platform_info->platformType = board_info.platform_type; 
            platform_info->cpuModule = OBERON_CPU_MODULE;
            platform_info->enclosureType = DPE_ENCLOSURE_TYPE;
            platform_info->midplaneType = MIRANDA_MIDPLANE;  
            break;
        case SPID_HYPERION_1_HW_TYPE:
        case SPID_HYPERION_2_HW_TYPE:
        case SPID_HYPERION_3_HW_TYPE:
            platform_info->platformType = board_info.platform_type; 
            platform_info->cpuModule = HYPERION_CPU_MODULE;
            platform_info->enclosureType = DPE_ENCLOSURE_TYPE;
            platform_info->midplaneType = SINOPE_MIDPLANE;  
            break;
        case SPID_TRITON_1_HW_TYPE:
            platform_info->platformType = board_info.platform_type; 
            platform_info->cpuModule = TRITON_CPU_MODULE;
            platform_info->enclosureType = XPE_ENCLOSURE_TYPE;
            platform_info->midplaneType = TELESTO_MIDPLANE;  
            break;

        default:
            return FBE_STATUS_GENERIC_FAILURE;
    }

    status = terminator_api_get_sp_id_function(&terminator_sp_id);

    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    switch(terminator_sp_id) {
        case TERMINATOR_SP_A:
            sp_id = SP_A;
            break;
        case TERMINATOR_SP_B:
            sp_id = SP_B;
            break;
        default:
            return FBE_STATUS_GENERIC_FAILURE;
    }

    status = terminator_api_is_single_sp_system_function(&is_single_sp);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    platform_info->isSingleSP = (BOOL)is_single_sp;
    /*  There is now a `simulation' version of SPECL
     */
    specl_status = speclInitializeArrayAndCacheData(sp_id, *platform_info);

    if(specl_status != SPECL_SFI_COMMAND_EXECUTED)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = terminator_api_process_specl_sfi_mask_data_queue_function();

    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_board_sim_set_terminator_api_get_board_info_function(fbe_terminator_api_get_board_info_function_t function)
{
    terminator_api_get_board_info_function = function;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_board_sim_set_terminator_api_get_sp_id_function(fbe_terminator_api_get_sp_id_function_t function)
{
    terminator_api_get_sp_id_function = function;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_board_sim_set_terminator_api_is_single_sp_system_function(fbe_terminator_api_is_single_sp_system_function_t function)
{
    terminator_api_is_single_sp_system_function = function;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_board_sim_set_terminator_api_process_specl_sfi_mask_data_queue_function(fbe_terminator_api_process_specl_sfi_mask_data_queue_function_t function)
{
    terminator_api_process_specl_sfi_mask_data_queue_function = function;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_board_is_mfgMode(fbe_bool_t *mfgModePtr)
{
	/*since this is simulation we return TRUE*/
    *mfgModePtr = TRUE;
    return FBE_STATUS_OK;
}

fbe_bool_t fbe_base_board_is_simulated_hw(void)
{
	/*since this is simulation we return TRUE*/
	return FBE_TRUE;
}


/*!*************************************************************************
 *  @fn fbe_base_board_set_FlushFilesAndReg
 **************************************************************************
 *  @brief
 *      This function calls the appropriate SPID call to set flush File and
 *      Registry IO to disk
 *
 *  @param sp_id - SP id.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    28-Dec-2010: Brion P - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_set_FlushFilesAndReg(SP_ID sp_id)
{
    return FBE_STATUS_OK;
}


/*!*************************************************************************
 *  @fn fbe_base_board_check_local_sp_reboot
 **************************************************************************
 *  @brief
 *      This function returns successful status for an SP reboot request
 *      in simulation.
 *
 *  @param none
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    25-Feb-2011: Brion P - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_check_local_sp_reboot(fbe_base_board_t *base_board)
{
    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_board_check_peer_sp_reboot
 **************************************************************************
 *  @brief
 *      This function returns successful status for an SP reboot request
 *      in simulation.
 *
 *  @param none
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    25-Feb-2011: Brion P - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_check_peer_sp_reboot(fbe_base_board_t *base_board)
{
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_board_get_ssd_self_test_passed(fbe_base_board_t *base_board, fbe_bool_t *self_test_passed)
{
    *self_test_passed = FBE_TRUE;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_board_get_ssd_spare_blocks(fbe_base_board_t *base_board, fbe_u32_t *spare_blocks)
{
    *spare_blocks = 10;// good state
    //*spare_blocks = 2;// bad state
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_board_get_ssd_life_used(fbe_base_board_t *base_board, fbe_u32_t *life_used)
{
    *life_used = 10;// good state
    //*life_used = 90;// bad state

    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_board_get_ssd_temperature(fbe_base_board_t *base_board, fbe_u32_t *pSsdTemperature)
{
    *pSsdTemperature = 30;      // good state

    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_board_get_ssd_count(fbe_base_board_t *base_board, fbe_u32_t *ssd_count)
{
    *ssd_count = 1;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_board_get_ssd_serial_number(fbe_base_board_t *base_board, fbe_char_t *pSsdSerialNumber)
{
    fbe_zero_memory(pSsdSerialNumber, FBE_SSD_SERIAL_NUMBER_SIZE);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_board_get_ssd_part_number(fbe_base_board_t *base_board, fbe_char_t *pSsdPartNumber)
{
    fbe_zero_memory(pSsdPartNumber, FBE_SSD_PART_NUMBER_SIZE);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_board_get_ssd_assembly_name(fbe_base_board_t *base_board, fbe_char_t *pSsdAssemblyName)
{
    fbe_zero_memory(pSsdAssemblyName, FBE_SSD_ASSEMBLY_NAME_SIZE);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_board_get_ssd_firmware_revision(fbe_base_board_t *base_board, fbe_char_t *pSsdFirmwareRevision)
{
    fbe_zero_memory(pSsdFirmwareRevision, FBE_SSD_FIRMWARE_REVISION_SIZE);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_board_logSsdTemperatureToPmp(fbe_base_board_t *base_board, 
                                                   fbe_base_board_ssdTempLogType ssdTempLogType,
                                                   fbe_u32_t ssdTemperature)
{
    // NO OP
    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_board_is_single_sp_system
 **************************************************************************
 *  @brief
 *      This function gets the single sp setting from SPID.
 *
 *  @param singleSpPtr- The pointer to single sp mode value.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    30-Jan-2013: Rui Chang - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_is_single_sp_system(fbe_bool_t *singleSpPtr)
{
    return terminator_api_is_single_sp_system_function(singleSpPtr);
}

fbe_status_t fbe_base_board_get_serial_num_from_pci(fbe_base_board_t * base_board,
                                                    fbe_u32_t bus, fbe_u32_t function, fbe_u32_t slot,
                                                    fbe_u32_t phy_map, fbe_u8_t *serial_num, fbe_u32_t buffer_size)
{
    fbe_copy_memory(serial_num,FBE_DUMMY_PORT_SERIAL_NUM, buffer_size);
    return FBE_STATUS_OK;
}

