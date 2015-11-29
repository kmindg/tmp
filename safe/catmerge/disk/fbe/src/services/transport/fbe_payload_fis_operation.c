#include "fbe/fbe_transport.h"
#include "fbe/fbe_payload_ex.h"
#include "fbe/fbe_sata_interface.h"
#include "fbe/fbe_payload_cdb_fis.h"
#include "fbe/fbe_library_interface.h"

static fbe_status_t 
fbe_payload_fis_process_fis_response(fbe_payload_fis_operation_t * fis_operation, /* IN */
                                     fbe_drive_configuration_handle_t drive_configuration_handle, /* IN */ 
                                     fbe_payload_cdb_fis_io_status_t * io_status, /* OUT */
                                     fbe_payload_cdb_fis_error_t * error_type, /* OUT */
                                     fbe_lba_t * bad_lba); /* OUT */

static fbe_status_t 
fbe_payload_fis_process_data_underrun(fbe_payload_fis_operation_t * fis_operation, /* IN */
										fbe_drive_configuration_handle_t drive_configuration_handle, /* IN */
										fbe_payload_cdb_fis_io_status_t * io_status, /* OUT */
										fbe_payload_cdb_fis_error_t * error_type); /* OUT */

static fbe_status_t 
fbe_payload_fis_process_NCQ_error (fbe_payload_fis_operation_t * fis_operation, /* IN */
                                        fbe_drive_configuration_handle_t drive_configuration_handle, /* IN */
                                        fbe_payload_cdb_fis_io_status_t * io_status, /* OUT */
                                        fbe_payload_cdb_fis_error_t * error_type, /* OUT */
                                        fbe_lba_t * bad_lba); /* OUT */

fbe_status_t 
fbe_payload_fis_build_read_fpdma_queued(fbe_payload_fis_operation_t * fis_operation,
								fbe_lba_t lba,
								fbe_block_count_t block_count,
								fbe_time_t timeout)
{

	fis_operation->payload_fis_task_attribute = FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_NCQ;
	fis_operation->payload_fis_flags = FBE_PAYLOAD_FIS_FLAGS_DATA_IN;
	fis_operation->timeout = timeout;
	fis_operation->fis_length = FBE_PAYLOAD_FIS_SIZE;

	fbe_zero_memory(fis_operation->fis, FBE_PAYLOAD_FIS_SIZE);
	fis_operation->fis[FBE_SATA_COMMAND_FIS_TYPE_OFFSET] = FBE_SATA_FIS_TYPE_REGISTER_HOST_TO_DEVICE;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_CCR_OFFESET] = 0x80; /* C=1 (Command register), PM port = 0. */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET] = FBE_SATA_READ_FPDMA_QUEUED; 
	fis_operation->fis[FBE_SATA_COMMAND_FIS_FEATURES_OFFSET] = (fbe_u8_t)(block_count & 0xFF); /* Sector count 7:0 */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_LOW_OFFSET] = (fbe_u8_t)(lba & 0xFF); /* LBA Low 7:0 */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_MID_OFFSET] = (fbe_u8_t)((lba >> 8) & 0xFF); /* LBA Mid 15:8 */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_HIGH_OFFSET] = (fbe_u8_t)((lba >> 16) & 0xFF); /* LBA High 23:16 */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_DEVICE_OFFSET] = 0x40; /* FUA=0 - data from storage media & buffers/cache */

	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_LOW_EXT_OFFSET] = (fbe_u8_t)((lba >> 24) & 0xFF); /* LBA Low Exp 31:24 */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_MID_EXT_OFFSET] = (fbe_u8_t)((lba >> 32) & 0xFF); /* LBA Mid Exp 39:32 */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_HIGH_EXT_OFFSET] = (fbe_u8_t)((lba >> 40) & 0xFF); /* LBA High Exp 47:40 */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_FEATURES_EXT_OFFSET] = (fbe_u8_t)((block_count >> 8) & 0xFF); /* Sector count 15:8 */

	fis_operation->fis[FBE_SATA_COMMAND_FIS_COUNT_OFFSET] = 0x00; /* TAG */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_COUNT_EXT_OFFSET] = 0x00; /* PRIO=0 - Normal priority */

	fis_operation->fis[FBE_SATA_COMMAND_FIS_CONTROL_OFFSET] = 0x00; /* Device Control register */

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_build_write_fpdma_queued(fbe_payload_fis_operation_t * fis_operation,
								fbe_lba_t lba,
								fbe_block_count_t block_count,
								fbe_time_t timeout)
{

	fis_operation->payload_fis_task_attribute = FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_NCQ;
	fis_operation->payload_fis_flags = FBE_PAYLOAD_FIS_FLAGS_DATA_OUT;
	fis_operation->timeout = timeout;
	fis_operation->fis_length = FBE_PAYLOAD_FIS_SIZE;

	fbe_zero_memory(fis_operation->fis, FBE_PAYLOAD_FIS_SIZE);
	fis_operation->fis[FBE_SATA_COMMAND_FIS_TYPE_OFFSET] = FBE_SATA_FIS_TYPE_REGISTER_HOST_TO_DEVICE;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_CCR_OFFESET] = 0x80; /* C=1 (Command register), PM port = 0. */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET] = FBE_SATA_WRITE_FPDMA_QUEUED; 
	fis_operation->fis[FBE_SATA_COMMAND_FIS_FEATURES_OFFSET] = (fbe_u8_t)(block_count & 0xFF); /* Sector count 7:0 */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_LOW_OFFSET] = (fbe_u8_t)(lba & 0xFF); /* LBA Low 7:0 */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_MID_OFFSET] = (fbe_u8_t)((lba >> 8) & 0xFF); /* LBA Mid 15:8 */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_HIGH_OFFSET] = (fbe_u8_t)((lba >> 16) & 0xFF); /* LBA High 23:16 */
    fis_operation->fis[FBE_SATA_COMMAND_FIS_DEVICE_OFFSET] = 0x40; /* FUA=0 Forced unit access turned off to take advantage of NCQ*/

	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_LOW_EXT_OFFSET] = (fbe_u8_t)((lba >> 24) & 0xFF); /* LBA Low Exp 31:24 */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_MID_EXT_OFFSET] = (fbe_u8_t)((lba >> 32) & 0xFF); /* LBA Mid Exp 39:32 */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_HIGH_EXT_OFFSET] = (fbe_u8_t)((lba >> 40) & 0xFF); /* LBA High Exp 47:40 */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_FEATURES_EXT_OFFSET] = (fbe_u8_t)((block_count >> 8) & 0xFF); /* Sector count 15:8 */

	fis_operation->fis[FBE_SATA_COMMAND_FIS_COUNT_OFFSET] = 0x00; /* TAG */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_COUNT_EXT_OFFSET] = 0x00; /* PRIO=0 - Normal priority */

	fis_operation->fis[FBE_SATA_COMMAND_FIS_CONTROL_OFFSET] = 0x00; /* Device Control register */

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_set_tag(fbe_payload_fis_operation_t * fis_operation, fbe_u8_t tag)
{
	fis_operation->fis[FBE_SATA_COMMAND_FIS_COUNT_OFFSET] = ((tag << 3) & 0xF8); /* TAG */
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_get_tag(fbe_payload_fis_operation_t * fis_operation, fbe_u8_t * tag)
{
	* tag = ((fis_operation->fis[FBE_SATA_COMMAND_FIS_COUNT_OFFSET] >> 3) & 0x1F );
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_build_identify_device(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout)
{

	fis_operation->payload_fis_task_attribute = FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_PIO;
	fis_operation->payload_fis_flags = FBE_PAYLOAD_FIS_FLAGS_DATA_IN;
	fis_operation->timeout = timeout;

	fis_operation->fis_length = FBE_PAYLOAD_FIS_SIZE;

	fbe_zero_memory(fis_operation->fis, FBE_PAYLOAD_FIS_SIZE);
	fis_operation->fis[FBE_SATA_COMMAND_FIS_TYPE_OFFSET] = FBE_SATA_FIS_TYPE_REGISTER_HOST_TO_DEVICE;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_CCR_OFFESET] = 0x80; /* C=1 (Command register), PM port = 0. */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET] = FBE_SATA_PIO_COMMAND_IDENTIFY_DEVICE; 

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_build_read_native_max_address_ext(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout)
{

	fis_operation->payload_fis_task_attribute = FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_NON_DATA;
	fis_operation->payload_fis_flags = FBE_PAYLOAD_FIS_FLAGS_NO_DATA_TRANSFER;
	fis_operation->timeout = timeout;

	fis_operation->fis_length = FBE_PAYLOAD_FIS_SIZE;

	fbe_zero_memory(fis_operation->fis, FBE_PAYLOAD_FIS_SIZE);
	fis_operation->fis[FBE_SATA_COMMAND_FIS_TYPE_OFFSET] = FBE_SATA_FIS_TYPE_REGISTER_HOST_TO_DEVICE;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_CCR_OFFESET] = 0x80; /* C=1 (Command register), PM port = 0. */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET] = FBE_SATA_PIO_COMMAND_READ_NATIVE_MAX_ADDRESS_EXT; 

	fis_operation->fis[FBE_SATA_COMMAND_FIS_DEVICE_OFFSET] = 0x40; /* Device field bit 6 shall be set to one */

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_build_read_inscription(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout, fbe_u64_t lba)
{
	fis_operation->payload_fis_task_attribute = FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_PIO;
	fis_operation->payload_fis_flags = FBE_PAYLOAD_FIS_FLAGS_DATA_IN;
	fis_operation->timeout = timeout;

	fis_operation->fis_length = FBE_PAYLOAD_FIS_SIZE;

	fbe_zero_memory(fis_operation->fis, FBE_PAYLOAD_FIS_SIZE);
	fis_operation->fis[FBE_SATA_COMMAND_FIS_TYPE_OFFSET] = FBE_SATA_FIS_TYPE_REGISTER_HOST_TO_DEVICE;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_CCR_OFFESET] = 0x80; /* C=1 (Command register), PM port = 0. */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET] = FBE_SATA_PIO_COMMAND_READ_SECTORS_EXT;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_LOW_OFFSET] = (fbe_u8_t)(lba & 0xFF); /* LBA Low 7:0 */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_MID_OFFSET] = (fbe_u8_t)((lba >> 8) & 0xFF); /* LBA Mid 15:8 */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_HIGH_OFFSET] = (fbe_u8_t)((lba >> 16) & 0xFF); /* LBA High 23:16 */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_LOW_EXT_OFFSET] = (fbe_u8_t)((lba >> 24) & 0xFF); /* LBA Low Exp 31:24 */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_MID_EXT_OFFSET] = (fbe_u8_t)((lba >> 32) & 0xFF); /* LBA Mid Exp 39:32 */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_HIGH_EXT_OFFSET] = (fbe_u8_t)((lba >> 40) & 0xFF); /* LBA High Exp 47:40 */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_COUNT_OFFSET] = FBE_SATA_DISK_DESIGNATOR_BLOCK_SECTOR_COUNT; 

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_build_check_power_mode(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout)
{
	fis_operation->payload_fis_task_attribute = FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_NON_DATA;
	fis_operation->payload_fis_flags = FBE_PAYLOAD_FIS_FLAGS_NO_DATA_TRANSFER;
	fis_operation->timeout = timeout;

	fis_operation->fis_length = FBE_PAYLOAD_FIS_SIZE;

	fbe_zero_memory(fis_operation->fis, FBE_PAYLOAD_FIS_SIZE);
	fis_operation->fis[FBE_SATA_COMMAND_FIS_TYPE_OFFSET] = FBE_SATA_FIS_TYPE_REGISTER_HOST_TO_DEVICE;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_CCR_OFFESET] = 0x80; /* C=1 (Command register), PM port = 0. */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET] = FBE_SATA_PIO_COMMAND_CHECK_POWER_MODE; 
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_build_execute_device_diag(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout)
{
	fis_operation->payload_fis_task_attribute = FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_NON_DATA;
	fis_operation->payload_fis_flags = FBE_PAYLOAD_FIS_FLAGS_NO_DATA_TRANSFER;
	fis_operation->timeout = timeout;

	fis_operation->fis_length = FBE_PAYLOAD_FIS_SIZE;

	fbe_zero_memory(fis_operation->fis, FBE_PAYLOAD_FIS_SIZE);
	fis_operation->fis[FBE_SATA_COMMAND_FIS_TYPE_OFFSET] = FBE_SATA_FIS_TYPE_REGISTER_HOST_TO_DEVICE;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_CCR_OFFESET] = 0x80; /* C=1 (Command register), PM port = 0. */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET] = FBE_SATA_PIO_COMMAND_EXECUTE_DEVICE_DIAGNOSTIC; 
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_build_flush_write_cache(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout)
{

	fis_operation->payload_fis_task_attribute = FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_NON_DATA;
	fis_operation->payload_fis_flags = FBE_PAYLOAD_FIS_FLAGS_NO_DATA_TRANSFER;
	fis_operation->timeout = timeout;

	fis_operation->fis_length = FBE_PAYLOAD_FIS_SIZE;

	fbe_zero_memory(fis_operation->fis, FBE_PAYLOAD_FIS_SIZE);
	fis_operation->fis[FBE_SATA_COMMAND_FIS_TYPE_OFFSET] = FBE_SATA_FIS_TYPE_REGISTER_HOST_TO_DEVICE;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_CCR_OFFESET] = 0x80; /* C=1 (Command register), PM port = 0. */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET] = FBE_SATA_PIO_COMMAND_FLUSH_CACHE_EXT; 
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_build_disable_write_cache(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout)
{

	fis_operation->payload_fis_task_attribute = FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_NON_DATA;
	fis_operation->payload_fis_flags = FBE_PAYLOAD_FIS_FLAGS_NO_DATA_TRANSFER;
	fis_operation->timeout = timeout;

	fis_operation->fis_length = FBE_PAYLOAD_FIS_SIZE;

	fbe_zero_memory(fis_operation->fis, FBE_PAYLOAD_FIS_SIZE);
	fis_operation->fis[FBE_SATA_COMMAND_FIS_TYPE_OFFSET] = FBE_SATA_FIS_TYPE_REGISTER_HOST_TO_DEVICE;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_CCR_OFFESET] = 0x80; /* C=1 (Command register), PM port = 0. */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET] = FBE_SATA_PIO_COMMAND_SET_FEATURES; 
    fis_operation->fis[FBE_SATA_COMMAND_FIS_FEATURES_OFFSET] = FBE_SATA_SET_FEATURES_DISABLE_WCACHE;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_build_enable_rla(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout)
{

	fis_operation->payload_fis_task_attribute = FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_NON_DATA;
	fis_operation->payload_fis_flags = FBE_PAYLOAD_FIS_FLAGS_NO_DATA_TRANSFER;
	fis_operation->timeout = timeout;

	fis_operation->fis_length = FBE_PAYLOAD_FIS_SIZE;

	fbe_zero_memory(fis_operation->fis, FBE_PAYLOAD_FIS_SIZE);
	fis_operation->fis[FBE_SATA_COMMAND_FIS_TYPE_OFFSET] = FBE_SATA_FIS_TYPE_REGISTER_HOST_TO_DEVICE;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_CCR_OFFESET] = 0x80; /* C=1 (Command register), PM port = 0. */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET] = FBE_SATA_PIO_COMMAND_SET_FEATURES; 
    fis_operation->fis[FBE_SATA_COMMAND_FIS_FEATURES_OFFSET] = FBE_SATA_SET_FEATURES_ENABLE_RLA;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_build_set_pio_mode(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout, fbe_u8_t PIOMode)
{

	fis_operation->payload_fis_task_attribute = FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_NON_DATA;
	fis_operation->payload_fis_flags = FBE_PAYLOAD_FIS_FLAGS_NO_DATA_TRANSFER;
	fis_operation->timeout = timeout;

	fis_operation->fis_length = FBE_PAYLOAD_FIS_SIZE;

	fbe_zero_memory(fis_operation->fis, FBE_PAYLOAD_FIS_SIZE);
	fis_operation->fis[FBE_SATA_COMMAND_FIS_TYPE_OFFSET] = FBE_SATA_FIS_TYPE_REGISTER_HOST_TO_DEVICE;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_CCR_OFFESET] = 0x80; /* C=1 (Command register), PM port = 0. */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET] = FBE_SATA_PIO_COMMAND_SET_FEATURES; 
    fis_operation->fis[FBE_SATA_COMMAND_FIS_FEATURES_OFFSET] = FBE_SATA_SET_FEATURES_TRANSFER; 
    //Set PIO mode from Identify Data
    fis_operation->fis[FBE_SATA_COMMAND_FIS_COUNT_OFFSET] = PIOMode;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_build_set_udma_mode(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout, fbe_u8_t UDMAMode)
{

	fis_operation->payload_fis_task_attribute = FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_NON_DATA;
	fis_operation->payload_fis_flags = FBE_PAYLOAD_FIS_FLAGS_NO_DATA_TRANSFER;
	fis_operation->timeout = timeout;

	fis_operation->fis_length = FBE_PAYLOAD_FIS_SIZE;

	fbe_zero_memory(fis_operation->fis, FBE_PAYLOAD_FIS_SIZE);
	fis_operation->fis[FBE_SATA_COMMAND_FIS_TYPE_OFFSET] = FBE_SATA_FIS_TYPE_REGISTER_HOST_TO_DEVICE;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_CCR_OFFESET] = 0x80; /* C=1 (Command register), PM port = 0. */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET] = FBE_SATA_PIO_COMMAND_SET_FEATURES; 
    fis_operation->fis[FBE_SATA_COMMAND_FIS_FEATURES_OFFSET] = FBE_SATA_SET_FEATURES_TRANSFER; 
    //Set UDMA mode from Identify Data
    fis_operation->fis[FBE_SATA_COMMAND_FIS_COUNT_OFFSET] = UDMAMode;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_build_enable_smart(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout)
{

	fis_operation->payload_fis_task_attribute = FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_NON_DATA;
	fis_operation->payload_fis_flags = FBE_PAYLOAD_FIS_FLAGS_NO_DATA_TRANSFER;
	fis_operation->timeout = timeout;

	fis_operation->fis_length = FBE_PAYLOAD_FIS_SIZE;

	fbe_zero_memory(fis_operation->fis, FBE_PAYLOAD_FIS_SIZE);
	fis_operation->fis[FBE_SATA_COMMAND_FIS_TYPE_OFFSET] = FBE_SATA_FIS_TYPE_REGISTER_HOST_TO_DEVICE;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_CCR_OFFESET] = 0x80; /* C=1 (Command register), PM port = 0. */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET] = FBE_SATA_PIO_COMMAND_SMART; 
    fis_operation->fis[FBE_SATA_COMMAND_FIS_FEATURES_OFFSET] = FBE_SATA_SMART_ENABLE_OPERATIONS;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_MID_OFFSET] = FBE_SATA_SMART_LBA_MID; 
    fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_HIGH_OFFSET] = FBE_SATA_SMART_LBA_HI;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_build_enable_smart_autosave(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout)
{

	fis_operation->payload_fis_task_attribute = FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_NON_DATA;
	fis_operation->payload_fis_flags = FBE_PAYLOAD_FIS_FLAGS_NO_DATA_TRANSFER;
	fis_operation->timeout = timeout;

	fis_operation->fis_length = FBE_PAYLOAD_FIS_SIZE;

	fbe_zero_memory(fis_operation->fis, FBE_PAYLOAD_FIS_SIZE);
	fis_operation->fis[FBE_SATA_COMMAND_FIS_TYPE_OFFSET] = FBE_SATA_FIS_TYPE_REGISTER_HOST_TO_DEVICE;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_CCR_OFFESET] = 0x80; /* C=1 (Command register), PM port = 0. */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET] = FBE_SATA_PIO_COMMAND_SMART; 
    fis_operation->fis[FBE_SATA_COMMAND_FIS_FEATURES_OFFSET] = FBE_SATA_SMART_ATTRIB_AUTOSAVE;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_MID_OFFSET] = FBE_SATA_SMART_LBA_MID; 
    fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_HIGH_OFFSET] = FBE_SATA_SMART_LBA_HI;
    fis_operation->fis[FBE_SATA_COMMAND_FIS_COUNT_OFFSET] = FBE_SATA_SMART_ATTRIB_AUTOSAVE_ENABLE;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_build_smart_return_status(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout)
{

	fis_operation->payload_fis_task_attribute = FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_NON_DATA;
	fis_operation->payload_fis_flags = FBE_PAYLOAD_FIS_FLAGS_NO_DATA_TRANSFER;
	fis_operation->timeout = timeout;

	fis_operation->fis_length = FBE_PAYLOAD_FIS_SIZE;

	fbe_zero_memory(fis_operation->fis, FBE_PAYLOAD_FIS_SIZE);
	fis_operation->fis[FBE_SATA_COMMAND_FIS_TYPE_OFFSET] = FBE_SATA_FIS_TYPE_REGISTER_HOST_TO_DEVICE;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_CCR_OFFESET] = 0x80; /* C=1 (Command register), PM port = 0. */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET] = FBE_SATA_PIO_COMMAND_SMART; 
    fis_operation->fis[FBE_SATA_COMMAND_FIS_FEATURES_OFFSET] = FBE_SATA_SMART_RETURN_STATUS;
	fis_operation->fis[5] = FBE_SATA_SMART_LBA_MID; 
    fis_operation->fis[6] = FBE_SATA_SMART_LBA_HI;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_build_smart_read_attributes(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout)
{
	fis_operation->payload_fis_task_attribute = FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_PIO;
	fis_operation->payload_fis_flags = FBE_PAYLOAD_FIS_FLAGS_DATA_IN;
	fis_operation->timeout = timeout;

	fis_operation->fis_length = FBE_PAYLOAD_FIS_SIZE;

	fbe_zero_memory(fis_operation->fis, FBE_PAYLOAD_FIS_SIZE);
	fis_operation->fis[FBE_SATA_COMMAND_FIS_TYPE_OFFSET] = FBE_SATA_FIS_TYPE_REGISTER_HOST_TO_DEVICE;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_CCR_OFFESET] = 0x80; /* C=1 (Command register), PM port = 0. */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET] = FBE_SATA_PIO_COMMAND_SMART; 
    fis_operation->fis[FBE_SATA_COMMAND_FIS_FEATURES_OFFSET] = FBE_SATA_SMART_READ_DATA_SUBCMD;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_MID_OFFSET] = FBE_SATA_SMART_LBA_MID; 
    fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_HIGH_OFFSET] = FBE_SATA_SMART_LBA_HI;
    fis_operation->fis[FBE_SATA_COMMAND_FIS_COUNT_OFFSET] = FBE_SATA_SMART_ATTRIBUTES_SECTOR_COUNT;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_build_sct_set_timer(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout)
{
	fis_operation->payload_fis_task_attribute = FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_PIO;
	fis_operation->payload_fis_flags = FBE_PAYLOAD_FIS_FLAGS_DATA_OUT;
	fis_operation->timeout = timeout;

	fis_operation->fis_length = FBE_PAYLOAD_FIS_SIZE;

	fbe_zero_memory(fis_operation->fis, FBE_PAYLOAD_FIS_SIZE);
	fis_operation->fis[FBE_SATA_COMMAND_FIS_TYPE_OFFSET] = FBE_SATA_FIS_TYPE_REGISTER_HOST_TO_DEVICE;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_CCR_OFFESET] = 0x80; /* C=1 (Command register), PM port = 0. */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET] = FBE_SATA_PIO_COMMAND_SMART; 
    fis_operation->fis[FBE_SATA_COMMAND_FIS_FEATURES_OFFSET] = FBE_SATA_SMART_WRITE_LOG_SUBCMD;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_LOW_OFFSET] = FBE_SATA_SMART_LBA_LOW; 
	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_MID_OFFSET] = FBE_SATA_SMART_LBA_MID; 
    fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_HIGH_OFFSET] = FBE_SATA_SMART_LBA_HI;
    fis_operation->fis[FBE_SATA_COMMAND_FIS_COUNT_OFFSET] = FBE_SATA_SMART_SECTOR_COUNT;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_build_firmware_download(fbe_payload_fis_operation_t * fis_operation, fbe_block_count_t block_count, fbe_time_t timeout)
{
    fis_operation->payload_fis_task_attribute = FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_PIO;
    fis_operation->payload_fis_flags = FBE_PAYLOAD_FIS_FLAGS_DATA_OUT | FBE_PAYLOAD_FIS_FLAGS_DRIVE_FW_UPGRADE;
	fis_operation->timeout = timeout;

	fis_operation->fis_length = FBE_PAYLOAD_FIS_SIZE;

	fbe_zero_memory(fis_operation->fis, FBE_PAYLOAD_FIS_SIZE);
	fis_operation->fis[FBE_SATA_COMMAND_FIS_TYPE_OFFSET] = FBE_SATA_FIS_TYPE_REGISTER_HOST_TO_DEVICE;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_CCR_OFFESET] = 0x80; /* C=1 (Command register), PM port = 0. */
    fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET] = FBE_SATA_PIO_COMMAND_DOWNLOAD_MICROCODE;
    
    /* lower 8 bits of sector count (bits 0-7) go into sector count 
       Higher 8 bits of sector count (bits 8-15) go into cylinder low*/            
	fis_operation->fis[FBE_SATA_COMMAND_FIS_COUNT_OFFSET] = (fbe_u8_t)(block_count & 0x00ff); 
    fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_LOW_OFFSET] = (fbe_u8_t)((block_count >> 8)& 0x00ff); 
    fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_MID_OFFSET] = 0;
    fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_HIGH_OFFSET] = 0;

    /* Set the feature feild to 7 - Download and save microcode for immediate and future use.*/
    fis_operation->fis[FBE_SATA_COMMAND_FIS_FEATURES_OFFSET] = FBE_SATA_SET_FEATURES_FIRMWARE_DOWNLOAD;;

	return FBE_STATUS_OK;
}
fbe_status_t 
fbe_payload_fis_build_standby(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout)
{
	fis_operation->payload_fis_task_attribute = FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_NON_DATA;
	fis_operation->payload_fis_flags = FBE_PAYLOAD_FIS_FLAGS_NO_DATA_TRANSFER;
	fis_operation->timeout = timeout;

	fis_operation->fis_length = FBE_PAYLOAD_FIS_SIZE;

	fbe_zero_memory(fis_operation->fis, FBE_PAYLOAD_FIS_SIZE);
	fis_operation->fis[FBE_SATA_COMMAND_FIS_TYPE_OFFSET] = FBE_SATA_FIS_TYPE_REGISTER_HOST_TO_DEVICE;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_CCR_OFFESET] = 0x80; /* C=1 (Command register), PM port = 0. */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET] = FBE_SATA_PIO_COMMAND_STANDBY_IMMEDIATE; 
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_build_write_uncorrectable(fbe_payload_fis_operation_t * fis_operation, fbe_lba_t lba, fbe_time_t timeout)
{
	fis_operation->payload_fis_task_attribute = FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_NON_DATA;
	fis_operation->payload_fis_flags = FBE_PAYLOAD_FIS_FLAGS_NO_DATA_TRANSFER;
	fis_operation->timeout = timeout;

	fis_operation->fis_length = FBE_PAYLOAD_FIS_SIZE;

	fbe_zero_memory(fis_operation->fis, FBE_PAYLOAD_FIS_SIZE);
	fis_operation->fis[FBE_SATA_COMMAND_FIS_TYPE_OFFSET] = FBE_SATA_FIS_TYPE_REGISTER_HOST_TO_DEVICE;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_CCR_OFFESET] = 0x80; /* C=1 (Command register), PM port = 0. */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET] = FBE_SATA_COMMAND_FIS_WRITE_UNCORRECTABLE; 
    fis_operation->fis[FBE_SATA_COMMAND_FIS_FEATURES_OFFSET] = FBE_SATA_FLAGGED_ERROR_WO_LOGGING;
	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_LOW_OFFSET] = (fbe_u8_t)(lba & 0xFF); /* LBA Low 7:0 */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_MID_OFFSET] = (fbe_u8_t)((lba >> 8) & 0xFF); /* LBA Mid 15:8 */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_HIGH_OFFSET] = (fbe_u8_t)((lba >> 16) & 0xFF); /* LBA High 23:16 */
	fis_operation->fis[FBE_SATA_COMMAND_FIS_DEVICE_OFFSET] = 0x40; /* Device field bit 6 shall be set to one */
    fis_operation->fis[FBE_SATA_COMMAND_FIS_COUNT_OFFSET] = 0x1; /* TODO: Can add more later */
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_set_transfer_count(fbe_payload_fis_operation_t * fis_operation, fbe_u32_t transfer_count)
{
	fis_operation->transfer_count = transfer_count;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_get_transfer_count(fbe_payload_fis_operation_t * fis_operation, fbe_u32_t * transfer_count)
{
    *transfer_count = fis_operation->transfer_count;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_set_transferred_count(fbe_payload_fis_operation_t * fis_operation, fbe_u32_t transferred_count)
{
	fis_operation->transferred_count = transferred_count;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_get_transferred_count(fbe_payload_fis_operation_t * fis_operation, fbe_u32_t * transferred_count)
{
    *transferred_count = fis_operation->transferred_count;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_set_response_buffer_length(fbe_payload_fis_operation_t * fis_operation, fbe_u32_t response_buffer_length)
{
    fis_operation->response_buffer_length = response_buffer_length;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_get_response_buffer_length(fbe_payload_fis_operation_t * fis_operation, fbe_u32_t * response_buffer_length)
{
    *response_buffer_length = fis_operation->response_buffer_length;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_get_lba(fbe_payload_fis_operation_t * fis_operation, fbe_lba_t * lba)
{
	*lba = 0;

	*lba = fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_LOW_OFFSET]; /* LBA Low 7:0 */
	*lba += fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_MID_OFFSET] << 8; /* LBA Mid 15:8 */
	*lba += fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_HIGH_OFFSET] << 16; /* LBA High 23:16 */

	*lba += ((fbe_lba_t)(fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_LOW_EXT_OFFSET])) << 24; /* LBA Low Exp 31:24 */
	*lba += ((fbe_lba_t)(fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_MID_EXT_OFFSET])) << 32; /* LBA Mid Exp 39:32 */
	*lba += ((fbe_lba_t)(fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_HIGH_EXT_OFFSET])) << 40; /* LBA High Exp 47:40 */

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_get_block_count(fbe_payload_fis_operation_t * fis_operation, fbe_block_count_t * block_count)
{
	*block_count = 0;
	*block_count = fis_operation->fis[FBE_SATA_COMMAND_FIS_FEATURES_OFFSET]; /* Sector count 7:0 */
	*block_count += fis_operation->fis[11] << 8; /* Sector count 15:8 */

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_set_request_status(fbe_payload_fis_operation_t * fis_operation, fbe_port_request_status_t request_status)
{
	fis_operation->port_request_status = request_status;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_get_request_status(fbe_payload_fis_operation_t * fis_operation, fbe_port_request_status_t * request_status)
{
	*request_status = fis_operation->port_request_status;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_set_recovery_status(fbe_payload_fis_operation_t * fis_operation, fbe_port_recovery_status_t recovery_status)
{
    fis_operation->port_recovery_status = recovery_status;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_fis_get_recovery_status(fbe_payload_fis_operation_t * fis_operation, fbe_port_recovery_status_t * recovery_status)
{
    *recovery_status = fis_operation->port_recovery_status;
    return FBE_STATUS_OK;
}


fbe_status_t 
fbe_payload_fis_completion(fbe_payload_fis_operation_t * fis_operation, /* IN */
							fbe_drive_configuration_handle_t drive_configuration_handle, /* IN */
							fbe_payload_cdb_fis_io_status_t * io_status, /* OUT */
							fbe_payload_cdb_fis_error_t * error_type, /* OUT */
							fbe_lba_t * bad_lba) /* OUT */
{
    fbe_port_request_status_t fis_request_status;
	fbe_status_t status = FBE_STATUS_OK;

	*bad_lba = FBE_LBA_INVALID;
	*error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;

    fbe_payload_fis_get_request_status(fis_operation, &fis_request_status);

    switch (fis_request_status)
    {
        case FBE_PORT_REQUEST_STATUS_SUCCESS:
            status = fbe_payload_fis_process_fis_response(fis_operation, drive_configuration_handle, io_status, error_type, bad_lba); 
            break;
            
        case FBE_PORT_REQUEST_STATUS_INVALID_REQUEST:
			*io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
			*error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE;
			break;
            
        case FBE_PORT_REQUEST_STATUS_INSUFFICIENT_RESOURCES:
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
			*error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;
            break;
            
        case FBE_PORT_REQUEST_STATUS_DEVICE_NOT_LOGGED_IN:
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
			*error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;
            break;
            
        case FBE_PORT_REQUEST_STATUS_BUSY:
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
			*error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;
            break;
            
        case FBE_PORT_REQUEST_STATUS_PROTOCOL_ERROR:
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
			*error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE;
            break;
            
        case FBE_PORT_REQUEST_STATUS_ABORT_TIMEOUT:
        case FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT:            
        case FBE_PORT_REQUEST_STATUS_TRANSIENT_ERROR:
			/* Check port and enclosure error stat and make a decision */
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
			*error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_LINK;
            break;
            
        case FBE_PORT_REQUEST_STATUS_DATA_OVERRUN:
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
			*error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE;
            break;

        case FBE_PORT_REQUEST_STATUS_DATA_UNDERRUN:
            status = fbe_payload_fis_process_data_underrun(fis_operation, drive_configuration_handle, io_status, error_type); 
            break;

        case FBE_PORT_REQUEST_STATUS_ABORTED_BY_SOFTWARE:        
        case FBE_PORT_REQUEST_STATUS_INCIDENTAL_ABORT:   /*aborted by miniport due to a severe error*/
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
			*error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;
            break;

        case FBE_PORT_REQUEST_STATUS_ABORTED_BY_DEVICE:        
			/* Check port and enclosure error stat and make a decision */
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
			*error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_LINK;
            break;

        case FBE_PORT_REQUEST_STATUS_SATA_NCQ_ERROR:
			/* Check port and enclosure error stat and make a decision */
            status = fbe_payload_fis_process_NCQ_error(fis_operation, drive_configuration_handle, io_status, error_type, bad_lba);
            
            break;

        default:
            status = FBE_STATUS_GENERIC_FAILURE;
    }

	return status;
}

static fbe_status_t 
fbe_payload_fis_process_fis_response(fbe_payload_fis_operation_t * fis_operation, /* IN */
                                     fbe_drive_configuration_handle_t drive_configuration_handle, /* IN */ 
                                     fbe_payload_cdb_fis_io_status_t * io_status, /* OUT */
                                     fbe_payload_cdb_fis_error_t * error_type, /* OUT */
                                     fbe_lba_t * bad_lba) /* OUT */
{
   
	fbe_status_t status = FBE_STATUS_OK;

    if(fis_operation->response_buffer[FBE_SATA_RESPONSE_STATUS] & FBE_SATA_RESPONSE_STATUS_ERR_BIT)
    {
        *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
	    *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE;
    }
    else
    {
		*io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_OK;
		*error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;
    }
    return status;
}			

static fbe_status_t 
fbe_payload_fis_process_data_underrun(fbe_payload_fis_operation_t * fis_operation, /* IN */
										fbe_drive_configuration_handle_t drive_configuration_handle, /* IN */
										fbe_payload_cdb_fis_io_status_t * io_status, /* OUT */
										fbe_payload_cdb_fis_error_t * error_type) /* OUT */
{

	return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_payload_fis_process_NCQ_error (fbe_payload_fis_operation_t * fis_operation, /* IN */
                                        fbe_drive_configuration_handle_t drive_configuration_handle, /* IN */
                                        fbe_payload_cdb_fis_io_status_t * io_status, /* OUT */
                                        fbe_payload_cdb_fis_error_t * error_type, /* OUT */
                                        fbe_lba_t * bad_lba) /* OUT */
{
    fbe_status_t status = FBE_STATUS_OK;
                
    switch(fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_ERROR_OFFSET])
    {
        case FBE_SATA_RESPONSE_FIS_ERROR_UNC:

            if(fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET] == FBE_SATA_READ_FPDMA_QUEUED)
            {
                *bad_lba = (fbe_lba_t)(fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_HIGH_EXT_OFFSET]) << 40 |
                           (fbe_lba_t)(fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_MID_EXT_OFFSET]) << 32 |
                           (fbe_lba_t)(fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_LOW_EXT_OFFSET]) << 24 |
                           (fbe_lba_t)(fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_HIGH_OFFSET]) << 16 |
                           (fbe_lba_t)(fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_MID_OFFSET]) << 8 |
                           (fbe_lba_t)(fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_LOW_OFFSET]);

                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_NO_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_MEDIA; 
            }
            else
            {
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_LINK;
            }
            break;
        case FBE_SATA_RESPONSE_FIS_ERROR_ICRC:
                 
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE;  
            
            break;

        case FBE_SATA_RESPONSE_FIS_ERROR_ABRT:

                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_LINK;
            break;

        case FBE_SATA_RESPONSE_FIS_ERROR_IDNF:
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;
            break;
    }

    return status;
}


void
fbe_payload_fis_trace_fis_buff (fbe_payload_fis_operation_t * fis_operation)
{        
    fbe_base_library_trace (FBE_LIBRARY_ID_FIS,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "FIS Buff: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n", 
                            fis_operation->fis[FBE_SATA_COMMAND_FIS_TYPE_OFFSET],
                            fis_operation->fis[FBE_SATA_COMMAND_FIS_CCR_OFFESET],
                            fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET],
                            fis_operation->fis[FBE_SATA_COMMAND_FIS_FEATURES_OFFSET],
                            fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_LOW_OFFSET],
                            fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_MID_OFFSET],
                            fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_HIGH_OFFSET],
                            fis_operation->fis[FBE_SATA_COMMAND_FIS_DEVICE_OFFSET],
                            fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_LOW_EXT_OFFSET],
                            fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_MID_EXT_OFFSET],
                            fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_HIGH_EXT_OFFSET],
                            fis_operation->fis[FBE_SATA_COMMAND_FIS_FEATURES_EXT_OFFSET],
                            fis_operation->fis[FBE_SATA_COMMAND_FIS_COUNT_OFFSET],
                            fis_operation->fis[FBE_SATA_COMMAND_FIS_COUNT_EXT_OFFSET],
                            fis_operation->fis[FBE_SATA_COMMAND_FIS_CONTROL_OFFSET]
                            );
}

void 
fbe_payload_fis_trace_response_buff(fbe_payload_fis_operation_t * fis_operation)
{
    fbe_base_library_trace (FBE_LIBRARY_ID_FIS,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Resp Buff: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n", 
                            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_TYPE_OFFSET],
                            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_STATUS_OFFSET],
                            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_ERROR_OFFSET],
                            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_LOW_OFFSET],
                            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_MID_OFFSET],
                            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_HIGH_OFFSET],
                            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_DEVICE_OFFSET],
                            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_LOW_EXT_OFFSET],
                            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_MID_EXT_OFFSET],
                            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_HIGH_EXT_OFFSET]
                            );
}
