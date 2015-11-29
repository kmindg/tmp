#include "fbe/fbe_winddk.h"
#include "fbe/fbe_peer_boot_utils.h"
#include "base_board_private.h"
#include "base_board_pe_private.h"
#include "generic_utils_lib.h"
#include "spid_types.h" 
#include "spid_kernel_interface.h" 

static fbe_object_mgmt_attributes_t base_board_mgmt_attributes = 0; /* Default. */

fbe_status_t
fbe_base_board_set_mgmt_attributes(fbe_object_mgmt_attributes_t mgmt_attributes)
{
    base_board_mgmt_attributes = mgmt_attributes;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_board_get_mgmt_attributes(fbe_object_mgmt_attributes_t * mgmt_attributes)
{
    *mgmt_attributes = base_board_mgmt_attributes;
    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_board_pe_get_platform_info
 **************************************************************************
 *  @brief
 *      This function gets the platform info from SPID.
 *
 *  @param pSpeclIoSum- The pointer to SPID_PLATFORM_INFO.
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
fbe_status_t fbe_base_board_get_platform_info(SPID_PLATFORM_INFO * platform_info)
{
    if(spidGetHwTypeEx(platform_info) == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
    
    return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t 
fbe_base_board_get_board_info(fbe_terminator_board_info_t * board_info)
{
    EMCPAL_STATUS nt_status;
    SPID_HW_TYPE spid_hw_type;

    board_info->board_type = FBE_BOARD_TYPE_INVALID;

    nt_status = spidGetHwType(&spid_hw_type);
    if(nt_status != EMCPAL_STATUS_SUCCESS){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch(spid_hw_type) {
    case SPID_ENTERPRISE_HW_TYPE:/* HACKS!!! */

    case SPID_PROMETHEUS_M1_HW_TYPE:/* HACKS!!! */
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

        board_info->board_type = FBE_BOARD_TYPE_ARMADA;
        break;

    /* Virtual Platfrom; TODO: Revisit */
    case SPID_MERIDIAN_ESX_HW_TYPE:
    case SPID_TUNGSTEN_HW_TYPE:
        board_info->board_type = FBE_BOARD_TYPE_ARMADA;
        break;
    
    case SPID_UNKNOWN_HW_TYPE:   
    default:
        /* Uknown or unsupported type */

        break;
    }

    if(board_info->board_type == FBE_BOARD_TYPE_INVALID){
        return FBE_STATUS_GENERIC_FAILURE;
    } else {
        return FBE_STATUS_OK;
    }
}

/*!*************************************************************************
 *  @fn fbe_base_board_is_mfgMode
 **************************************************************************
 *  @brief
 *      This function gets the MfgMode setting from SPID.
 *
 *  @param mfgModePtr- The pointer to MfgMode value.
 *
 *  @return fbe_status_t - status of the operation
 *             FEB_STATUS_OK - no error.
 *             otherwise - error encountered.
 *  @note
 *
 *  @version
 *    11-Nov-2011: Joe Perry - Created.
 *    09-Dec-2014: Joe Ash - Hardware no longer supports, just return false.
 *
 **************************************************************************/
fbe_status_t fbe_base_board_is_mfgMode(fbe_bool_t *mfgModePtr)
{
    *mfgModePtr = FALSE;
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
    EMCPAL_STATUS nt_status;
    BOOLEAN  singleSP = FALSE;

    *singleSpPtr = FALSE;

    nt_status = spidIsSingleSPSystem(&singleSP);
    if(nt_status != EMCPAL_STATUS_SUCCESS){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (singleSP)
    {
        *singleSpPtr = TRUE;
    }

    return FBE_STATUS_OK;
}

fbe_bool_t fbe_base_board_is_simulated_hw(void)
{
	/*since this is kernel we return FALSE*/
	return FBE_FALSE;
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
    if(spidFlushFilesAndReg() == EMCPAL_STATUS_SUCCESS)
    {
        return FBE_STATUS_OK;
    }
    
    return FBE_STATUS_GENERIC_FAILURE;
}


/*!*************************************************************************
 *  @fn fbe_base_board_check_local_sp_reboot
 **************************************************************************
 *  @brief
 *      If this function is even called, the request to reboot the local sp
 *      has not been successful.
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
    // if we haven't rebooted by the time we check, its a failure

    fbe_base_object_trace((fbe_base_object_t *)base_board,
                    FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s, Reboot of Local SP requested but SP has not yet rebooted.\n",
                    __FUNCTION__);


    return FBE_STATUS_GENERIC_FAILURE;
}


/*!*************************************************************************
 *  @fn fbe_base_board_check_peer_sp_reboot
 **************************************************************************
 *  @brief
 *      If this function is even called, the request to reboot the local sp
 *      has not been successful.
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
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t new_fault_reg_status = FBE_PEER_STATUS_UNKNOW;

    new_fault_reg_status = fbe_base_board_pe_get_fault_reg_status(base_board);
    if(new_fault_reg_status != FBE_PEER_STATUS_UNKNOW)
    {
        if( (new_fault_reg_status == CSR_OS_RUNNING) ||
            (new_fault_reg_status == CSR_OS_APPLICATION_RUNNING))
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            status = FBE_STATUS_OK;
        }
    }
    return status;
}

fbe_status_t fbe_base_board_get_serial_num_from_pci(fbe_base_board_t * base_board,
                                                    fbe_u32_t bus, fbe_u32_t function, fbe_u32_t slot,
                                                    fbe_u32_t phy_map, fbe_u8_t *serial_num, fbe_u32_t buffer_size)
{
    fbe_status_t                            status;
    SMB_DEVICE smb_device;
    fbe_u32_t offset;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s Resume PROM Read PCI:%d.%d.%d map:0x%x\n",
                          __FUNCTION__, bus, function, 
                          slot, phy_map);

    /* Get the SMB Device from the PCI information */
    status = fbe_base_board_get_smb_device_from_pci(bus, 
                                                    function, 
                                                    slot, 
                                                    phy_map,
                                                    &smb_device);

    if ((status != FBE_STATUS_OK) || smb_device == SMB_DEVICE_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s unknown device. PCI:%d.%d.%d map:0x%x. Status:%d \n",
                              __FUNCTION__, bus, function, 
                              slot, phy_map, status);

        return status;
    }

    offset = getResumeFieldOffset(RESUME_PROM_EMC_TLA_SERIAL_NUM);

    status =  fbe_base_board_get_resume_field_data(base_board,
                                                   smb_device,
                                                   serial_num,
                                                   offset,
                                                   buffer_size);
    return status;
}   // end of base_board_get_platform_info
#define SSD_SELF_TEST_CMD_STRING "smartctl -a /dev/ssd|egrep 'SMART overall-health self-assessment test result' | awk '{print $6}'"
#define SSD_LIFE_REMAIN_CMD_STRING "smartctl -a /dev/ssd|egrep 'SSD_Life_Remain' | awk '{print $10}'"
#define SSD_LIFE_USED_CMD_STRING "smartctl -a /dev/ssd|egrep 'Pct_Drive_Life_Used' | awk '{print $10}'"
#define SSD_REMAINING_SPARE_BLOCKS_CMD_STRING "smartctl -a /dev/ssd|egrep 'Remaining_Spare_Blks' | awk '{print $10}'"

#define SSD_SERIAL_NUMBER_CMD_STRING "smartctl -a /dev/ssd|egrep 'Serial Number' | awk '{print $3}'"
#define SSD_PART_NUMBER_CMD_STRING "smartctl -a /dev/ssd|egrep 'Device Model' | awk 'BEGIN {FS = \":\"} {print $2}'"
#define SSD_ASSEMBLY_NAME_CMD_STRING "smartctl -a /dev/ssd|egrep 'Model Family' | awk 'BEGIN {FS = \":\"} {print $2}'"
#define SSD_FIRMWARE_REVISION_CMD_STRING "smartctl -a /dev/ssd|egrep 'Firmware Version' | awk '{print $3}'"

fbe_status_t fbe_base_board_get_ssd_self_test_passed(fbe_base_board_t *base_board, fbe_bool_t *self_test_passed)
{
    fbe_char_t output_buffer[255] = {0};
    fbe_u32_t return_status = 0;

#ifndef ALAMOSA_WINDOWS_ENV
    csx_p_native_process_drive_from_helper(SSD_SELF_TEST_CMD_STRING,CSX_NULL,output_buffer,255,CSX_NULL,&return_status);

    if (strncmp(output_buffer, "PASSED", strlen("PASSED")) == 0) 
    {
        *self_test_passed = FBE_TRUE;
    }
    else
    {
        *self_test_passed = FBE_FALSE;
    }
    fbe_base_object_trace((fbe_base_object_t *)base_board,
                    FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s, string returned %s.\n",
                    __FUNCTION__, output_buffer);
#endif
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_board_get_ssd_life_used(fbe_base_board_t *base_board, fbe_u32_t *life_used)
{
    fbe_char_t output_buffer[255] = {0};
    fbe_u32_t return_status = 0;

#ifndef ALAMOSA_WINDOWS_ENV
    csx_p_native_process_drive_from_helper(SSD_LIFE_USED_CMD_STRING,CSX_NULL,output_buffer,255,CSX_NULL,&return_status);

    *life_used = atoi(output_buffer);

    fbe_base_object_trace((fbe_base_object_t *)base_board,
                    FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s, string returned %s.\n",
                    __FUNCTION__, output_buffer);
#else
    *life_used = 10;
#endif
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_board_get_ssd_spare_blocks(fbe_base_board_t *base_board, fbe_u32_t *spare_blocks)
{
    fbe_char_t output_buffer[255] = {0};
    fbe_u32_t return_status = 0;

#ifndef ALAMOSA_WINDOWS_ENV
    csx_p_native_process_drive_from_helper(SSD_REMAINING_SPARE_BLOCKS_CMD_STRING,CSX_NULL,output_buffer,255,CSX_NULL,&return_status);

    *spare_blocks = atoi(output_buffer);

    fbe_base_object_trace((fbe_base_object_t *)base_board,
                    FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s, string returned %s.\n",
                    __FUNCTION__, output_buffer);

#else
    *spare_blocks = 10;// good state
    //*spare_blocks = 2;// bad state
#endif
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_board_get_ssd_count(fbe_base_board_t *base_board, fbe_u32_t *ssd_count)
{
    fbe_char_t output_buffer[255] = {0};
    fbe_u32_t return_status = 0;

#ifndef ALAMOSA_WINDOWS_ENV
    csx_p_native_process_drive_from_helper(SSD_REMAINING_SPARE_BLOCKS_CMD_STRING,CSX_NULL,output_buffer,255,CSX_NULL,&return_status);

    fbe_base_object_trace((fbe_base_object_t *)base_board,
                    FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s, string returned %s.\n",
                    __FUNCTION__, output_buffer);
#endif
    *ssd_count = 1; // hard coded for now, need to see the output to figure out how to count these things

    return FBE_STATUS_OK;

}

fbe_status_t fbe_base_board_get_ssd_serial_number(fbe_base_board_t *base_board, fbe_char_t *pSsdSerialNumber)
{
    fbe_char_t output_buffer[255] = {0};
    fbe_u32_t return_status = 0;

#ifndef ALAMOSA_WINDOWS_ENV
    csx_p_native_process_drive_from_helper(SSD_SERIAL_NUMBER_CMD_STRING,CSX_NULL,output_buffer,255,CSX_NULL,&return_status);

    if(strlen(&output_buffer[0]) == 0)
    {
        return FBE_STATUS_OK;
    }
    else if (strlen(&output_buffer[0]) < FBE_SSD_SERIAL_NUMBER_SIZE) 
    {
        /* There is a return charactor at the end of the string. It does not need to be copied over.
         * So we use strlen(&output_buffer[0])-1.
         */
        fbe_copy_memory(pSsdSerialNumber, &output_buffer[0], strlen(&output_buffer[0])-1);
    }
    else
    {
        fbe_copy_memory(pSsdSerialNumber, &output_buffer[0], FBE_SSD_SERIAL_NUMBER_SIZE);
    }

    fbe_base_object_trace((fbe_base_object_t *)base_board,
                    FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s, string returned %s.\n",
                    __FUNCTION__, output_buffer);
#endif
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_board_get_ssd_part_number(fbe_base_board_t *base_board, fbe_char_t *pSsdPartNumber)
{
    fbe_char_t output_buffer[255] = {0};
    fbe_u32_t return_status = 0;

#ifndef ALAMOSA_WINDOWS_ENV
    csx_p_native_process_drive_from_helper(SSD_PART_NUMBER_CMD_STRING,CSX_NULL,output_buffer,255,CSX_NULL,&return_status);

    if(strlen(&output_buffer[0]) == 0)
    {
        return FBE_STATUS_OK;
    }
    else if (strlen(&output_buffer[0]) < FBE_SSD_PART_NUMBER_SIZE) 
    {
        /* There is a return charactor at the end of the string. It does not need to be copied over.
         * So we use strlen(&output_buffer[0])-1.
         */
        fbe_copy_memory(pSsdPartNumber, &output_buffer[0], strlen(&output_buffer[0])-1); 
    }
    else
    {
        fbe_copy_memory(pSsdPartNumber, &output_buffer[0], FBE_SSD_PART_NUMBER_SIZE);
    }

    fbe_base_object_trace((fbe_base_object_t *)base_board,
                    FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s, string returned %s.\n",
                    __FUNCTION__, output_buffer);
#endif
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_board_get_ssd_assembly_name(fbe_base_board_t *base_board, fbe_char_t *pSsdAssemblyName)
{
    fbe_char_t output_buffer[255] = {0};
    fbe_u32_t return_status = 0;

#ifndef ALAMOSA_WINDOWS_ENV
    csx_p_native_process_drive_from_helper(SSD_ASSEMBLY_NAME_CMD_STRING,CSX_NULL,output_buffer,255,CSX_NULL,&return_status);

    if(strlen(&output_buffer[0]) == 0)
    {
        return FBE_STATUS_OK;
    }
    else if (strlen(&output_buffer[0]) < FBE_SSD_ASSEMBLY_NAME_SIZE) 
    {
        /* There is a return charactor at the end of the string. It does not need to be copied over.
         * So we use strlen(&output_buffer[0])-1.
         */
        fbe_copy_memory(pSsdAssemblyName, &output_buffer[0], strlen(&output_buffer[0])-1);
    }
    else
    {
        fbe_copy_memory(pSsdAssemblyName, &output_buffer[0], FBE_SSD_ASSEMBLY_NAME_SIZE);
    }

    fbe_base_object_trace((fbe_base_object_t *)base_board,
                    FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s, string returned %s.\n",
                    __FUNCTION__, output_buffer);
#endif
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_board_get_ssd_firmware_revision(fbe_base_board_t *base_board, fbe_char_t *pSsdFirmwareRevision)
{
    fbe_char_t output_buffer[255] = {0};
    fbe_u32_t return_status = 0;

#ifndef ALAMOSA_WINDOWS_ENV
    csx_p_native_process_drive_from_helper(SSD_FIRMWARE_REVISION_CMD_STRING,CSX_NULL,output_buffer,255,CSX_NULL,&return_status);

    if(strlen(&output_buffer[0]) == 0)
    {
        return FBE_STATUS_OK;
    }
    else if (strlen(&output_buffer[0]) < FBE_SSD_FIRMWARE_REVISION_SIZE) 
    {
        /* There is a return charactor at the end of the string. It does not need to be copied over.
         * So we use strlen(&output_buffer[0])-1.
         */
        fbe_copy_memory(pSsdFirmwareRevision, &output_buffer[0], strlen(&output_buffer[0])-1);
    }
    else
    {
        fbe_copy_memory(pSsdFirmwareRevision, &output_buffer[0], FBE_SSD_FIRMWARE_REVISION_SIZE);
    }

    fbe_base_object_trace((fbe_base_object_t *)base_board,
                    FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s, string returned %s.\n",
                    __FUNCTION__, output_buffer);
#endif
    return FBE_STATUS_OK;
}
