#include "fbe/fbe_winddk.h"
#include "base_board_private.h"

#include "spid_types.h" 
//#include "spid_user_interface.h" 

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

fbe_status_t 
fbe_base_board_get_board_info(SPID_PLATFORM_INFO * board_info)
{
  
	board_info->platformType = SPID_DEFIANT_M3_HW_TYPE;
	board_info->cpuModule = JETFIRE_CPU_MODULE;
	board_info->enclosureType = DPE_ENCLOSURE_TYPE;
	board_info->midplaneType = STRATOSPHERE_MIDPLANE;
    return FBE_STATUS_OK;

#if 0 /* AT - The foll. piece is commented out because the calling convention used by
       * physical package is stdcall and spid is __cdecl. Other utilities that uses
       * spid are __cdecl so we cannot change spid to use stdcall and for now we dont
       * want physical package to use __cdecl. So this piece if commented out */
	status = spidGetHwTypeEx(&board_info);
	if(status){
		return FBE_STATUS_GENERIC_FAILURE;
	}

	switch(spid_hw_type) {
	default:
		/* Uknown or unsupported type */

		break;
	}

	if(*board_type == FBE_BOARD_TYPE_INVALID){
		return FBE_STATUS_GENERIC_FAILURE;
	} else {
		return FBE_STATUS_OK;
	}
#endif
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

fbe_status_t fbe_base_board_get_serial_num_from_pci(fbe_base_board_t * base_board,
                                                    fbe_u32_t bus, fbe_u32_t function, fbe_u32_t slot,
                                                    fbe_u32_t phy_map, fbe_u8_t *serial_num, fbe_u32_t buffer_size)
{
    fbe_copy_memory(serial_num,FBE_DUMMY_PORT_SERIAL_NUM, buffer_size);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_board_get_ssd_self_test_passed(fbe_base_board_t *base_board, fbe_bool_t *self_test_passed)
{
    *self_test_passed = FBE_TRUE;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_board_get_ssd_spare_blocks(fbe_base_board_t *base_board, fbe_u32_t *spare_blocks)
{
    *spare_blocks = 5;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_board_get_ssd_life_used(fbe_base_board_t *base_board, fbe_u32_t *life_used)
{
    *life_used = 10;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_board_get_ssd_temperature(fbe_base_board_t *base_board, fbe_u32_t *pSsdTemperature)
{
    *pSsdTemperature = 30;
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

