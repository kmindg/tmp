#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_ses_internal.h"
#include "fbe_ses.h"

static fbe_status_t fbe_ses_build_receive_diagnostic_page(fbe_u8_t *cdb, 
                                                          fbe_u32_t cdb_buffer_size, 
                                                          fbe_u32_t response_buffer_size,
                                                          fbe_u8_t page_code);

static fbe_status_t fbe_ses_build_send_diagnostic_page(fbe_u8_t *cdb, 
                                                       fbe_u32_t cdb_buffer_size, 
                                                       fbe_u32_t cmd_buffer_size,
                                                       fbe_u8_t page_code);

static fbe_status_t fbe_ses_build_receive_diagnostic_page(fbe_u8_t *cdb, 
                                                          fbe_u32_t cdb_buffer_size, 
                                                          fbe_u32_t response_buffer_size,
                                                          fbe_u8_t page_code)
{
    fbe_ses_receive_diagnostic_results_cdb_t *cdb_page;

    if(cdb_buffer_size < FBE_SES_RECEIVE_DIAGNOSTIC_RESULTS_CDB_SIZE) {
        return (FBE_STATUS_GENERIC_FAILURE);
    }
   
    cdb_page = (fbe_ses_receive_diagnostic_results_cdb_t *)cdb;

    cdb_page->operation_code = FBE_SES_RECEIVE_DIAGNOSTIC_RESULTS_OPCODE;
    cdb_page->page_code_valid = 1;
    cdb_page->rsvd = 0;
    cdb_page->page_code = page_code;
    cdb_page->alloc_length_msbyte = ((response_buffer_size & 0xFF00) >> 8);
    cdb_page->alloc_length_lsbyte = (response_buffer_size & 0xFF);
    cdb_page->control = 0;

   return FBE_STATUS_OK;
}

static fbe_status_t fbe_ses_build_send_diagnostic_page(fbe_u8_t *cdb, 
                                                       fbe_u32_t cdb_buffer_size, 
                                                       fbe_u32_t cmd_buffer_size,
                                                       fbe_u8_t page_code)
{
    fbe_ses_send_diagnostic_results_cdb_t *cdb_page;

    if(cdb_buffer_size < FBE_SES_SEND_DIAGNOSTIC_RESULTS_CDB_SIZE) {
        return (FBE_STATUS_GENERIC_FAILURE);
    }

    cdb_page = (fbe_ses_send_diagnostic_results_cdb_t *)cdb;

    cdb_page->operation_code = FBE_SES_SEND_DIAGNOSTIC_RESULTS_OPCODE;
    cdb_page->page_format = 1;
    cdb_page->unit_offline = 0;
    cdb_page->device_offline = 0;
    cdb_page->self_test = 0;
    cdb_page->self_test_code = 0;
    cdb_page->rsvd = 0;
    cdb_page->rsvd1 = 0;
    cdb_page->param_list_length_msbyte = ((cmd_buffer_size & 0xFF00) >> 8);
    cdb_page->param_list_length_lsbyte = (cmd_buffer_size & 0xFF);
    cdb_page->control = 0;

   return FBE_STATUS_OK;
}

fbe_status_t fbe_ses_build_encl_config_page(fbe_u8_t *cdb, 
                                            fbe_u32_t cdb_buffer_size, 
                                            fbe_u32_t response_buffer_size)
{
    fbe_status_t status;

    status = fbe_ses_build_receive_diagnostic_page(cdb, cdb_buffer_size, response_buffer_size, FBE_SES_ENCLOSURE_CONFIGURATION_PAGE);
    return status;
}


fbe_status_t fbe_ses_build_encl_status_page(fbe_u8_t *cdb, 
                                            fbe_u32_t cdb_buffer_size, 
                                            fbe_u32_t response_buffer_size)
{
    fbe_status_t status;

    status = fbe_ses_build_receive_diagnostic_page(cdb,
                                                   cdb_buffer_size, 
                                                   response_buffer_size,
                                                   FBE_SES_ENCLOSURE_STATUS_PAGE);
    return status;
}

fbe_status_t fbe_ses_build_additional_status_page(fbe_u8_t *cdb, 
                                                  fbe_u32_t cdb_buffer_size, 
                                                  fbe_u32_t response_buffer_size)
{
    fbe_status_t status;

    status = fbe_ses_build_receive_diagnostic_page(cdb,
                                                   cdb_buffer_size, 
                                                   response_buffer_size,
                                                   FBE_SES_ADDITIONAL_ELEMENT_STATUS_PAGE);
    return status;
}
 
fbe_status_t fbe_ses_build_encl_control_page(fbe_u8_t *cdb, 
                                             fbe_u32_t cdb_buffer_size, 
                                             fbe_u32_t cmd_buffer_size)
{
    fbe_status_t status;

    status = fbe_ses_build_send_diagnostic_page(cdb,
                                                cdb_buffer_size, 
                                                cmd_buffer_size,
                                                FBE_SES_ENCLOSURE_CONTROL_PAGE);
    return status;
}

fbe_status_t fbe_ses_build_encl_control_cmd(fbe_u8_t *cmd, 
                                            fbe_u32_t cmd_buffer_size)
{
    fbe_ses_send_diagnostic_page_header_t *header;
    
    if(cmd_buffer_size < FBE_SES_ENCLOSURE_CONTROL_DATA_SIZE)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    header = (fbe_ses_send_diagnostic_page_header_t *) cmd;
    header->page_code = FBE_SES_ENCLOSURE_CONTROL_PAGE;
    header->rsvd = 0;
    header->page_length_msbyte = ((FBE_SES_ENCLOSURE_CONTROL_DATA_SIZE & 0xFF00) >> 8);
    header->page_length_lsbyte = (FBE_SES_ENCLOSURE_CONTROL_DATA_SIZE & 0xFF);
    header->generation_code = 0;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_ses_drive_control_fault_led_onoff(fbe_u8_t slot_id,
                                                   fbe_u8_t *cmd_buffer,
                                                   fbe_u32_t cmd_buffer_size,
                                                   fbe_bool_t fault_led_on)
{
    fbe_u32_t offset;
    fbe_ses_enclosure_control_device_control_word_t *drive_control_word;
    
    /* Calculate the offset for the drive control for a particular slot */ 
    offset = FBE_SES_ENCLOSURE_CONTROL_DEVICE_CONTROL_OFFSET + 
            (FBE_SES_ENCLOSURE_CONTROL_DEVICE_CONTROL_WORD_LENGTH * slot_id);
    
    if(offset > cmd_buffer_size)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    drive_control_word = (fbe_ses_enclosure_control_device_control_word_t *) &cmd_buffer[offset];
    drive_control_word->select_bit = 1;
    drive_control_word->rqst_flt = fault_led_on;
    
    return FBE_STATUS_OK;
}

fbe_status_t fbe_ses_drive_control_fault_led_flash_onoff(fbe_u8_t slot_id,
                                                         fbe_u8_t *cmd_buffer,
                                                         fbe_u32_t cmd_buffer_size,
                                                         fbe_bool_t flash_on)
{
    fbe_u32_t offset;
    fbe_ses_enclosure_control_device_control_word_t *drive_control_word;
    
    /* Calculate the offset for the drive control for a particular slot */ 
    offset = FBE_SES_ENCLOSURE_CONTROL_DEVICE_CONTROL_OFFSET + 
            (FBE_SES_ENCLOSURE_CONTROL_DEVICE_CONTROL_WORD_LENGTH * slot_id);

    if(offset > cmd_buffer_size)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    drive_control_word = (fbe_ses_enclosure_control_device_control_word_t *) &cmd_buffer[offset];
    drive_control_word->select_bit = 1;
    drive_control_word->rqst_ident = flash_on;
    
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_ses_get_drive_status(fbe_u8_t *resp_buffer, fbe_u32_t slot_id, fbe_u32_t *drive_status)
{
    fbe_ses_slot_status_element_t *drive_info;
    fbe_u32_t offset;

    offset = FBE_SES_ENCLOSURE_STATUS_DEVICE_STATUS_OFFSET + 
            (sizeof(fbe_ses_slot_status_element_t) * slot_id);

    drive_info = (fbe_ses_slot_status_element_t *) &resp_buffer[offset];
	KvTrace("Slot:%d, Status Code:%d, Swap:%d, Slot Id:%d\n",
             slot_id, drive_info->status_code, drive_info->swap, drive_info->slot_address);

    *drive_status = drive_info->status_code;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_ses_get_status_element(fbe_u8_t *resp_buffer, fbe_u32_t index, fbe_ses_status_element_t * status_element)
{
    fbe_ses_status_element_t * elem;
    fbe_u32_t offset;
	fbe_u8_t * buf = NULL;

    offset = FBE_SES_ENCLOSURE_STATUS_DEVICE_STATUS_OFFSET + (sizeof(fbe_ses_status_element_t) * index);

    elem = (fbe_ses_status_element_t *) &resp_buffer[offset];

	fbe_copy_memory(status_element, elem, sizeof(fbe_ses_status_element_t));

	buf = (fbe_u8_t *)status_element;
	KvTrace("%s index %d status_element: %02X %02X %02X %02X\n", __FUNCTION__, index,  buf[0] , buf[1], buf[2], buf[3]);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_ses_get_drive_slot_address(fbe_u8_t *resp_buffer, fbe_u32_t device_num, fbe_u32_t *slot_address)
{
    fbe_ses_slot_status_element_t *drive_info;
    fbe_u32_t offset;

    offset = FBE_SES_ENCLOSURE_STATUS_DEVICE_STATUS_OFFSET + 
            (sizeof(fbe_ses_slot_status_element_t) * device_num);

    drive_info = (fbe_ses_slot_status_element_t *) &resp_buffer[offset];
    *slot_address = drive_info->slot_address;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_ses_get_drive_swap_info(fbe_u8_t *resp_buffer, fbe_u32_t slot_id, fbe_u32_t *swap)
{
    fbe_ses_slot_status_element_t *drive_info;
    fbe_u32_t offset;

    offset = FBE_SES_ENCLOSURE_STATUS_DEVICE_STATUS_OFFSET + 
            (sizeof(fbe_ses_slot_status_element_t) * slot_id);

    drive_info = (fbe_ses_slot_status_element_t *) &resp_buffer[offset];
    *swap = drive_info->swap;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_ses_get_drive_sas_address(fbe_u8_t *resp_buffer, fbe_u32_t slot_id, fbe_sas_address_t *sas_address)
{
    fbe_ses_additional_status_device_info_t *device_additional_info;
    fbe_u32_t offset;

    offset = FBE_SES_ADDITIONAL_ELEMENT_DEVICE_STATUS_OFFSET + 
            (sizeof(fbe_ses_additional_status_device_info_t) * slot_id);

    device_additional_info = (fbe_ses_additional_status_device_info_t *) &resp_buffer[offset];
    *sas_address = device_additional_info->sas_address;
    return FBE_STATUS_OK;
}