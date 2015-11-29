/***************************************************************************
 *  fbe_terminator_sas_enclosure_io_api.c
 ***************************************************************************
 *
 *  Description
 *      APIs to emulate the ESES functionality
 *      
 *
 *  History:
 *      06/30/08    sharel  Created
 *    
 ***************************************************************************/

#include "fbe/fbe_types.h"
#include "terminator_sas_io_api.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_eses.h"
#include "fbe_scsi.h"
#include "fbe_terminator.h"
#include "terminator_encl_eses_plugin.h"

/**********************************/
/*        local variables         */
/**********************************/

#define SWP16(d) ((((d) & 0xFF) << 8) | (((d) >> 8) & 0xFF))

terminator_sas_encl_inq_data_t  encl_table [] = {
  // encl_type                          board_type                                        type_str    vendor      product_id          unique_id           eses_version     platform_type
  // 4 bytes,                           2 bytes,                                          8 bytes,    8 bytes,    16 bytes,           16 bytes,           1, 1, 1 bytes    1, 1 bytes
    {FBE_SAS_ENCLOSURE_TYPE_BULLET,     SWP16(0),                                        "Simulate", "EMC     ", "ESES Enclosure  ", "0000000000000000",  0, 1, 0,         0xff, 0xfe},
    {FBE_SAS_ENCLOSURE_TYPE_VIPER,      SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_VIPER),      "SimViper", "EMC     ", "ESES Enclosure  ", "1234",              0, 1, 0,         0, 1   },
    {FBE_SAS_ENCLOSURE_TYPE_MAGNUM,     SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_MAGNUM),     "Sim-MAGN", "EMC     ", "ESES Enclosure  ", "2345",              0, 1, 0,         0, 3   },
    {FBE_SAS_ENCLOSURE_TYPE_BUNKER,     SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_SENTRY),     "Sim-BUNK", "EMC     ", "ESES Enclosure  ", "3456",              0, 1, 0,         0, 4   },
    {FBE_SAS_ENCLOSURE_TYPE_CITADEL,    SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_SENTRY),     "Sim-CTDL", "EMC     ", "ESES Enclosure  ", "5678",              0, 1, 0,         0, 5   },
    {FBE_SAS_ENCLOSURE_TYPE_DERRINGER,  SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_DERRINGER),  "Sim-DRNG", "EMC     ", "ESES Enclosure  ", "4567",              0, 1, 0,         0, 2   },
    {FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM,SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_VOYAGER_ICM),"Sim-VICM", "EMC     ", "ESES Enclosure  ", "6789",              0, 1, 0,         0, 0xA },
    {FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE, SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_VOYAGER_EE), "Sim-VYEE", "EMC     ", "ESES Enclosure  ", "7891",              0, 1, 0,         0, 0xA },

  // From the inquiry page, we would not be able to differentiate the IOSXP and DRVSXP.
    {FBE_SAS_ENCLOSURE_TYPE_VIKING,     SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_VIKING),     "Sim-VIKI", "EMC     ", "ESES Enclosure  ", "5566",              0, 1, 0,         0, 0xB },
    {FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP,     SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_VIKING),     "Sim-VIKI", "EMC     ", "ESES Enclosure  ", "5566",              0, 1, 0,         0, 0xB },
    {FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP,     SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_VIKING),     "Sim-VIKI", "EMC     ", "ESES Enclosure  ", "5566",              0, 1, 0,         0, 0xB },
  // From the inquiry page, we would not be able to differentiate the IOSXP and DRVSXP.
    {FBE_SAS_ENCLOSURE_TYPE_CAYENNE,     SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_CAYENNE),     "Sim-CAYE", "EMC     ", "ESES Enclosure  ", "3366",              0, 2, 0,         0, 0x15 },
    {FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP, SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_CAYENNE),   "Sim-CAYE", "EMC     ", "ESES Enclosure  ", "3366",              0, 2, 0,         0, 0x15 },
    {FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP, SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_CAYENNE),  "Sim-CAYE", "EMC     ", "ESES Enclosure  ", "3366",              0, 2, 0,         0, 0x15 },
    {FBE_SAS_ENCLOSURE_TYPE_NAGA,     SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_NAGA),            "Sim-NAGA", "EMC     ", "ESES Enclosure  ", "6655",              0, 2, 0,         0, 0x1A },
    {FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP,     SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_NAGA),      "Sim-NAGA", "EMC     ", "ESES Enclosure  ", "6655",              0, 2, 0,         0, 0x1A },
    {FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP,     SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_NAGA),     "Sim-NAGA", "EMC     ", "ESES Enclosure  ", "6655",              0, 2, 0,         0, 0x1A },
  // platform_type is get from fbe_eses_enclosure_platform_type_t 
    {FBE_SAS_ENCLOSURE_TYPE_FALLBACK, SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_JETFIRE_BEM),    "Sim-FLBK", "EMC     ", "ESES Enclosure  ", "4321",              0, 1, 0,         0, 0xD  },
    {FBE_SAS_ENCLOSURE_TYPE_BOXWOOD,  SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_SILVERBOLT),     "Sim-BXWD", "EMC     ", "ESES Enclosure  ", "5432",              0, 1, 0,         0, 0x12  },
    {FBE_SAS_ENCLOSURE_TYPE_KNOT,     SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_SILVERBOLT),     "Sim-KNOT", "EMC     ", "ESES Enclosure  ", "6543",              0, 1, 0,         0, 0x13  },
    {FBE_SAS_ENCLOSURE_TYPE_PINECONE, SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_PINECONE),       "Sim-PICN", "EMC     ", "ESES Enclosure  ", "7654",              0, 1, 0,         0, 0x8  },
    {FBE_SAS_ENCLOSURE_TYPE_STEELJAW, SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_BEACHCOMBER),    "Sim-STJW", "EMC     ", "ESES Enclosure  ", "8765",              0, 1, 0,         0, 0x10  },
    {FBE_SAS_ENCLOSURE_TYPE_RAMHORN,  SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_BEACHCOMBER),    "Sim-RMHN", "EMC     ", "ESES Enclosure  ", "9876",              0, 1, 0,         0, 0x11  },
    {FBE_SAS_ENCLOSURE_TYPE_ANCHO,      SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_ANCHO),        "Sim-ANCH", "EMC     ", "ESES Enclosure  ", "0666",              0, 2, 0,         0, 0x16 }, 
    {FBE_SAS_ENCLOSURE_TYPE_RHEA,        SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_OBERON),      "Sim-RHEA", "EMC     ", "ESES Enclosure  ", "2468",              0, 2, 0,         0, 0x19  },
    {FBE_SAS_ENCLOSURE_TYPE_MIRANDA,     SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_OBERON),      "Sim-MIRA", "EMC     ", "ESES Enclosure  ", "4680",              0, 2, 0,         0, 0x18  },
    {FBE_SAS_ENCLOSURE_TYPE_CALYPSO,     SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_LAPETUS),     "Sim-CALY", "EMC     ", "ESES Enclosure  ", "6802",              0, 2, 0,         0, 0x1B  },
    {FBE_SAS_ENCLOSURE_TYPE_TABASCO,    SWP16(FBE_ESES_ENCLOSURE_BOARD_TYPE_TABASCO),      "Sim-TBSC", "EMC     ", "ESES Enclosure  ", "0987",              0, 2, 0,         0, 0x17  },
    /*add stuff only above*/
    {FBE_SAS_ENCLOSURE_TYPE_LAST}
};



/******************************/
/*     local functions        */
/******************************/
static fbe_status_t sas_enclosure_process_payload_inquiry(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t virtual_phy_handle);

static fbe_status_t sas_enclosure_get_inq_data (fbe_sas_enclosure_type_t encl_type, terminator_sas_encl_inq_data_t **inq_data);

static fbe_status_t process_payload_receive_diagnostic_results(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t virtual_phy_handle);

static fbe_status_t sanity_check_cdb_receive_diagnostic_results(fbe_u8_t *cdb_page_ptr, fbe_u8_t *page_code);

/* Send diag command related function*/
static fbe_status_t 
sas_enclosure_process_payload_inquiry(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_u8_t *                          b_ptr = NULL;
    terminator_sas_encl_inq_data_t *    inq_data = NULL;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_cdb_operation_t       * payload_cdb_operation = NULL;
    fbe_sg_element_t                  * sg_list;
    fbe_sas_enclosure_type_t            encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    fbe_u8_t                            *uid = NULL;

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    /* get the enclosure uid thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_uid(virtual_phy_handle, &uid);

    /* Get cdb operation */
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);

    /*get the correct inq datd for this encl type*/
    status = sas_enclosure_get_inq_data (encl_type, &inq_data);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: Error !! Illegal encl type passed in: %d\n", __FUNCTION__, encl_type);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_INVALID_REQUEST);
        return status;
    }

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    b_ptr = (fbe_u8_t *)sg_list->address;
    if (sg_list->count == 0){
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: sg list count is 0, can't do inqury\n", __FUNCTION__);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_INVALID_REQUEST);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*Place the vendor, product and firmware rev fields for simulation disks in the buffer*/
    fbe_copy_memory (b_ptr, inq_data->type_str, 8);
    fbe_copy_memory (b_ptr + FBE_SCSI_INQUIRY_VENDOR_ID_OFFSET, inq_data->vendor, FBE_SCSI_INQUIRY_VENDOR_ID_SIZE);
    fbe_copy_memory (b_ptr + FBE_SCSI_INQUIRY_PRODUCT_ID_OFFSET, inq_data->product_id, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE);
    fbe_copy_memory (b_ptr + FBE_SCSI_INQUIRY_BOARD_TYPE_OFFSET, &(inq_data->board_type), FBE_SCSI_INQUIRY_BOARD_TYPE_SIZE);
    fbe_copy_memory (b_ptr + FBE_SCSI_INQUIRY_ENCLOSURE_PLATFORM_TYPE_OFFSET, inq_data->enclosure_platform_type, FBE_SCSI_INQUIRY_ENCLOSURE_PLATFORM_TYPE_SIZE);
    fbe_copy_memory (b_ptr + FBE_SCSI_INQUIRY_ESES_VERSION_DESCRIPTOR_OFFSET, inq_data->eses_version_descriptor, FBE_SCSI_INQUIRY_ESES_VERSION_DESCRIPTOR_SIZE);
    fbe_copy_memory (b_ptr + FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_OFFSET, uid, FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE);
    fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_SUCCESS);
    fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD);

    return FBE_STATUS_OK;
}

static fbe_status_t sas_enclosure_get_inq_data (fbe_sas_enclosure_type_t encl_type, terminator_sas_encl_inq_data_t **inq_data)
{
    *inq_data = encl_table;

    while ((*inq_data)->encl_type != FBE_SAS_ENCLOSURE_TYPE_LAST) {
        if ((*inq_data)->encl_type == encl_type) {
            return FBE_STATUS_OK;
        }

        (*inq_data)++;
    }

    /*we did not find anything*/
    return FBE_STATUS_GENERIC_FAILURE;

}

fbe_status_t fbe_terminator_sas_enclosure_get_eses_version_desc(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                                fbe_u16_t * eses_version_desc_ptr)
{
    fbe_sas_enclosure_type_t encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    terminator_sas_encl_inq_data_t  *inq_data = NULL;
    fbe_u16_t eses_version_desc_big_endian = 0;

    /* get the enclosure type thru the virtual_phy_handle */
    terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    
    /*get the correct inq datd for this encl type*/
    sas_enclosure_get_inq_data (encl_type, &inq_data);

    fbe_copy_memory(&eses_version_desc_big_endian, 
        &inq_data->eses_version_descriptor[0], 
        FBE_SCSI_INQUIRY_ESES_VERSION_DESCRIPTOR_SIZE);   
         
    *eses_version_desc_ptr = swap16(eses_version_desc_big_endian);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_terminator_sas_virtual_phy_payload_io(fbe_payload_ex_t *payload, fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_u8_t * cdb_opcode;
    fbe_u8_t *sense_buffer;
    fbe_u8_t unit_attention = 0;

    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    /* make sure we have the right operation */
    if (payload_cdb_operation == NULL) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: Error ! fbe_payload_ex_get_cdb_operation failed \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_payload_cdb_operation_get_sense_buffer(payload_cdb_operation, &sense_buffer);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s fbe_payload_cdb_operation_get_sense_buffer failed\n", __FUNCTION__);
        return status;
    }
    // Initialize the sense buffer
    build_sense_info_buffer(sense_buffer, 
                            FBE_SCSI_SENSE_KEY_NO_SENSE, 
                            FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO,
                            FBE_SCSI_ASCQ_NO_ADDITIONAL_SENSE_INFO);

    terminator_virtual_phy_get_unit_attention(virtual_phy_handle, &unit_attention);
    if(unit_attention)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s terminator_virtual_phy_get_unit_attention failed\n", __FUNCTION__);

        terminator_virtual_phy_set_unit_attention(virtual_phy_handle, 0);
        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_UNIT_ATTENTION, 
                                FBE_SCSI_ASC_TARGET_OPERATING_CONDITIONS_HAVE_CHANGED,
                                FBE_SCSI_ASCQ_TARGET_OPERATING_CONDITIONS_HAVE_CHANGED);
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_ERROR);
        return(FBE_STATUS_OK);
    }
    
    cdb_opcode = (fbe_u8_t *)(payload_cdb_operation->cdb);
	
    switch(*cdb_opcode)
    {
        case FBE_SCSI_INQUIRY:
            status = sas_enclosure_process_payload_inquiry(payload, virtual_phy_handle);
            break;
        case FBE_SCSI_RECEIVE_DIAGNOSTIC:
            status = process_payload_receive_diagnostic_results(payload, virtual_phy_handle);
            break;
        case FBE_SCSI_SEND_DIAGNOSTIC:
            status = process_payload_send_diagnostic(payload, virtual_phy_handle);
            break;
        case FBE_SCSI_READ_BUFFER:
            status = process_payload_read_buffer(payload, virtual_phy_handle);
            break;
        case FBE_SCSI_WRITE_BUFFER:
            status = process_payload_write_buffer(payload, virtual_phy_handle);
            break;
        case FBE_SCSI_MODE_SENSE_10:
            status = process_payload_mode_sense_10(payload, virtual_phy_handle);
            break;
        case FBE_SCSI_MODE_SELECT_10:
            status = process_payload_mode_select_10(payload, virtual_phy_handle);
            break;
            // continue with other commands
        default:
            build_sense_info_buffer(sense_buffer, 
                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                FBE_SCSI_ASC_INVALID_COMMAND_OPERATION_CODE,
                FBE_SCSI_ASCQ_INVALID_COMMAND_OPERATION_CODE);
            fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
            fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_ERROR);
            break;
    }

    return(FBE_STATUS_OK);
}

static fbe_status_t
process_payload_receive_diagnostic_results(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t receive_diagnostic_page_code;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_sas_enclosure_type_t        encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    fbe_sg_element_t * sg_list = NULL;
    fbe_u8_t *sense_buffer;
    fbe_bool_t not_ready = FBE_TRUE;
    fbe_u32_t                       transferred_count;
    ses_common_pg_hdr_struct        *common_pg_hdr;

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s terminator_virtual_phy_get_enclosure_type failed\n", __FUNCTION__);
        return status;
    }
	
    status = terminator_check_sas_enclosure_type(encl_type);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s terminator_check_sas_enclosure_type failed\n", __FUNCTION__);
        return status;
    }

    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    if(payload_cdb_operation == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s fbe_payload_ex_get_cdb_operation failed\n", __FUNCTION__);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    status = fbe_payload_cdb_operation_get_sense_buffer(payload_cdb_operation, &sense_buffer);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s fbe_payload_cdb_operation_get_sense_buffer failed\n", __FUNCTION__);
        return status;
    }

    status = sanity_check_cdb_receive_diagnostic_results((fbe_u8_t *)(&payload_cdb_operation->cdb), &receive_diagnostic_page_code);
    if(status != FBE_STATUS_OK) {
        build_sense_info_buffer(sense_buffer, 
            FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
            FBE_SCSI_ASC_INVALID_FIELD_IN_CDB,
            FBE_SCSI_ASCQ_INVALID_FIELD_IN_CDB);
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_ERROR);
        return status;
    }

    // Check if the user marked the page as unsupported
    if(!terminator_is_eses_page_supported(FBE_SCSI_RECEIVE_DIAGNOSTIC, receive_diagnostic_page_code))
    {
        //set up sense data here.
        build_sense_info_buffer(sense_buffer, 
            FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
            FBE_SCSI_ASC_UNSUPPORTED_ENCLOSURE_FUNCTION,
            FBE_SCSI_ASCQ_UNSUPPORTED_ENCLOSURE_FUNCTION);
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_ERROR);
        return(FBE_STATUS_OK);
    }

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    if((sg_list == NULL) || (sg_list->count == 0))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: sglist is zero or sglist count is 0, can't return the requested page\n", __FUNCTION__);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_INVALID_REQUEST);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch(receive_diagnostic_page_code)
    {
        case SES_PG_CODE_ENCL_STAT:
            status = build_enclosure_status_diagnostic_page(sg_list, virtual_phy_handle);
            break;
        case SES_PG_CODE_ADDL_ELEM_STAT:
            status = build_additional_element_status_diagnostic_page(sg_list, virtual_phy_handle);
            break;
        case SES_PG_CODE_CONFIG:
            status = build_configuration_diagnostic_page(sg_list, virtual_phy_handle);
            break;
        case SES_PG_CODE_EMC_ENCL_STAT:
            status = build_emc_enclosure_status_diagnostic_page(sg_list, virtual_phy_handle);
            break;
        case SES_PG_CODE_DOWNLOAD_MICROCODE_STAT:
            status = build_download_microcode_status_diagnostic_page(sg_list, virtual_phy_handle);
            break;
        case SES_PG_CODE_EMC_STATS_STAT:
            status = build_emc_statistics_status_diagnostic_page(sg_list, virtual_phy_handle);
            break;
        default:
        //set up sense data here.
            build_sense_info_buffer(sense_buffer, 
                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                FBE_SCSI_ASC_UNSUPPORTED_ENCLOSURE_FUNCTION,
                FBE_SCSI_ASCQ_UNSUPPORTED_ENCLOSURE_FUNCTION);
            status = FBE_STATUS_GENERIC_FAILURE;
            not_ready = FBE_FALSE;
            break;        
    }

    if (status != FBE_STATUS_OK)
    {
        if(not_ready)
        {
            // As the status page building failed, this indicates some bug 
            // (possible) in code. So put "NOT READY" in  sense data and return.
            ema_not_ready(sense_buffer);
            terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s Building status pages failed, possible BUG in terminator?\n", __FUNCTION__);
        }
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_ERROR);
    }
    else
    {
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_SUCCESS);
        common_pg_hdr = (ses_common_pg_hdr_struct *)fbe_sg_element_address(sg_list);          
        transferred_count = (fbe_u16_t)(swap16(common_pg_hdr->pg_len) + 4);
        fbe_payload_cdb_set_transferred_count(payload_cdb_operation, transferred_count);
    }
    return FBE_STATUS_OK;
}

fbe_status_t sanity_check_cdb_receive_diagnostic_results(fbe_u8_t *cdb_page_ptr, fbe_u8_t *page_code)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_eses_receive_diagnostic_results_cdb_t *cdb_page;

    cdb_page = (fbe_eses_receive_diagnostic_results_cdb_t *)(cdb_page_ptr);
    if ((cdb_page->page_code_valid != 1) || 
        (cdb_page->control != 0 ))
    {
        return(status);
    }
    
    *page_code = cdb_page->page_code;

    status = FBE_STATUS_OK;
    return(status);

}

// This is dummy for now that returns NOT INSTALLED every Nth time and installed other times
// this is temparary. OUTDATED.
ses_elem_stat_code_enum sas_virtual_phy_get_drive_slot_state(void)
{
    // return one of the opcodes from pg 152 of ESES spec.

    static fbe_u8_t flag = 0;
    const fbe_u8_t alternate_nth = 3;

    if(flag == (alternate_nth-1))
    {
        flag = 0;
        return(SES_STAT_CODE_NOT_INSTALLED);
    }

    flag++;
    return(SES_STAT_CODE_OK);
}


/*********************************************************************
*  terminator_sas_enclosure_io_api_init ()
**********************************************************************
*
*  Description: 
*   Terminator ESES IO Module Initialization interface.
*
*  Inputs: 
*   None
*
*  Return Value: success or failure
*
*  Notes: 
*
*  History:
*    Sep08  created
*    
*********************************************************************/
fbe_status_t terminator_sas_enclosure_io_api_init(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = config_page_init_type_desc_hdr_structs();
    RETURN_ON_ERROR_STATUS;

    status=FBE_STATUS_OK;
    return(status);
}

/*********************************************************************
* build_sense_info_buffer()
*********************************************************************
*
*  Description: This builds sense buffer information.
*
*  Inputs: 
*   pointer to sense buffer, sense key, ASC code, ASCQ code values 
*       to fill.
*
*  Return Value: success or failure
*
*  Notes: 
*
*  History:
*    Oct08  Rajesh V. created
*    
*********************************************************************/
void build_sense_info_buffer(
    fbe_u8_t *sense_info_buffer, 
    fbe_scsi_sense_key_t sense_key, 
    fbe_scsi_additional_sense_code_t ASC,
    fbe_scsi_additional_sense_code_qualifier_t ASCQ)
{
    memset(sense_info_buffer, 0, (FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE * sizeof(fbe_u8_t)));
    sense_info_buffer[FBE_SCSI_SENSE_DATA_RESPONSE_CODE_OFFSET] = 0x70;
    sense_info_buffer[FBE_SCSI_SENSE_DATA_SENSE_KEY_OFFSET] = sense_key;
    sense_info_buffer[FBE_SCSI_SENSE_DATA_ADDL_SENSE_LENGTH_OFFSET] = 10;
    sense_info_buffer[FBE_SCSI_SENSE_DATA_ASC_OFFSET] = ASC;
    sense_info_buffer[FBE_SCSI_SENSE_DATA_ASCQ_OFFSET] = ASCQ;

    return;
}

/* When there is a possible bug in ESES/Terminator code we call this
 * function to fill in the sense data to EMA NOT READY--etc.
 */
void ema_not_ready(fbe_u8_t *sense_info_buffer)
{
    terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "%s: Possible bug in termiantor?\n",
                     __FUNCTION__);

    build_sense_info_buffer(sense_info_buffer,
                            FBE_SCSI_SENSE_KEY_NOT_READY,
                            FBE_SCSI_ASC_ENCLOSURE_SERVICES_UNAVAILABLE,
                            FBE_SCSI_ASCQ_ENCLOSURE_SERVICES_UNAVAILABLE);

}

/* The below functions starting with "eses" are interface for terminator into
 * ESES Page routines or vice-versa.
 */
fbe_status_t eses_initialize_eses_page_info(fbe_sas_enclosure_type_t encl_type, terminator_vp_eses_page_info_t *eses_page_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = config_page_initialize_config_page_info(encl_type, &eses_page_info->vp_config_diag_page_info);
    RETURN_ON_ERROR_STATUS;

    status = mode_page_initialize_mode_page_info(&eses_page_info->vp_mode_page_info);
    RETURN_ON_ERROR_STATUS;

    status = download_ctrl_page_initialize_page_info(
        &eses_page_info->vp_download_microcode_ctrl_diag_page_info);
    RETURN_ON_ERROR_STATUS;

    status = download_stat_page_initialize_page_info(
        &eses_page_info->vp_download_micrcode_stat_diag_page_info);
    RETURN_ON_ERROR_STATUS;

    return(FBE_STATUS_OK);
}

fbe_status_t eses_get_config_diag_page_info(fbe_terminator_device_ptr_t virtual_phy_handle,
                                            vp_config_diag_page_info_t **config_diag_page_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_vp_eses_page_info_t *vp_eses_page_info;

    status = terminator_get_eses_page_info(virtual_phy_handle, &vp_eses_page_info);
    RETURN_ON_ERROR_STATUS;

    *config_diag_page_info = &vp_eses_page_info->vp_config_diag_page_info;
    return(FBE_STATUS_OK);
}

fbe_status_t eses_get_mode_page_info(fbe_terminator_device_ptr_t virtual_phy_handle,
                                     terminator_eses_vp_mode_page_info_t **vp_mode_page_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_vp_eses_page_info_t *vp_eses_page_info;

    status = terminator_get_eses_page_info(virtual_phy_handle, &vp_eses_page_info);
    RETURN_ON_ERROR_STATUS;

    *vp_mode_page_info = &vp_eses_page_info->vp_mode_page_info;
    return(FBE_STATUS_OK);
}
                                            
fbe_status_t eses_get_subencl_id(terminator_vp_eses_page_info_t *vp_eses_page_info,
                                ses_subencl_type_enum subencl_type,
                                terminator_eses_subencl_side side,
                                fbe_u8_t *subencl_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = config_page_get_subencl_id(vp_eses_page_info->vp_config_diag_page_info,
                                        subencl_type,
                                        side,
                                        subencl_id) ; 

    return(status);
}

fbe_status_t eses_get_subencl_ver_desc_position(terminator_vp_eses_page_info_t *eses_page_info,
                                                ses_subencl_type_enum subencl_type,
                                                terminator_eses_subencl_side side,
                                                fbe_u8_t *ver_desc_start_index,
                                                fbe_u8_t *num_ver_descs)
{
    
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = config_page_info_get_subencl_ver_desc_position(eses_page_info->vp_config_diag_page_info.config_page_info,
                                                            subencl_type, 
                                                            side, 
                                                            ver_desc_start_index, 
                                                            num_ver_descs);
    return(status);
}

fbe_status_t eses_get_buf_id_by_subencl_info(terminator_vp_eses_page_info_t *eses_page_info,
                                            ses_subencl_type_enum subencl_type,
                                            fbe_u8_t side,
                                            ses_buf_type_enum buf_type,
                                            fbe_bool_t consider_writable,
                                            fbe_u8_t writable,
                                            fbe_bool_t consider_buf_index,
                                            fbe_u8_t buf_index,
                                            fbe_bool_t consider_buf_spec_info,
                                            fbe_u8_t buf_spec_info,
                                            fbe_u8_t *buf_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = config_page_info_get_buf_id_by_subencl_info(eses_page_info->vp_config_diag_page_info,
                                                         subencl_type,
                                                         side,
                                                         buf_type,
                                                         consider_writable,
                                                         writable,
                                                         consider_buf_index,
                                                         buf_index,
                                                         consider_buf_spec_info,
                                                         buf_spec_info,
                                                         buf_id);
    return(status);
}

fbe_status_t eses_get_buf_info_by_buf_id(fbe_terminator_device_ptr_t virtual_phy_handle,
                                        fbe_u8_t buf_id,
                                        terminator_eses_buf_info_t **buf_info)
{
    return(terminator_eses_get_buf_info_by_buf_id(virtual_phy_handle, buf_id, buf_info));
}

/*********************************************************************
* build_download_microcode_status_diagnostic_page()
*********************************************************************
*
*  Description: 
*       This builds the Download microcode Status diagnostic 
*       page.
*
*  Inputs: 
*   Sg List and Virtual Phy object handle.
*
*  Return Value: success or failure
*
*  Notes: 
*
*  History:
*    Oct-19-2008    Rajesh V. created
*    
*********************************************************************/
fbe_status_t build_download_microcode_status_diagnostic_page(fbe_sg_element_t * sg_list, fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_eses_download_status_page_t *download_status_page;

    download_status_page = (fbe_eses_download_status_page_t *)fbe_sg_element_address(sg_list);
    memset(download_status_page, 0, sizeof(fbe_eses_download_status_page_t));
    download_status_page->page_code = SES_PG_CODE_DOWNLOAD_MICROCODE_STAT;
    status = config_page_get_num_of_sec_subencls(virtual_phy_handle, &download_status_page->number_of_subencls);
    RETURN_ON_ERROR_STATUS;

    // Gen code returned in bigendian format.
    status = config_page_get_gen_code(virtual_phy_handle, &download_status_page->gen_code);
    RETURN_ON_ERROR_STATUS;
    download_status_page->gen_code = BYTE_SWAP_32(download_status_page->gen_code);

    download_status_page->pg_len = FBE_ESES_MICROCODE_STATUS_PAGE_MAX_SIZE - 3;
    download_status_page->pg_len = BYTE_SWAP_16(download_status_page->pg_len);

    status = terminator_eses_get_download_microcode_stat_page_stat_desc(virtual_phy_handle,
                                                                        &download_status_page->dl_status);
    RETURN_ON_ERROR_STATUS;

    status = FBE_STATUS_OK;
    return(status); 
}

/**************************************************************************
 *  download_stat_page_initialize_page_info()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function initializes the download status page structures in Virtual
 *    Phy ESES information structure.
 *
 *  PARAMETERS:
 *     pointer to Virtual Phy download status page information
 *
 *  RETURN VALUES/ERRORS:
 *     success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *     Oct-5-2010: Rajesh V created.
 **************************************************************************/
fbe_status_t 
download_stat_page_initialize_page_info(
    vp_download_microcode_stat_diag_page_info_t *vp_download_microcode_stat_diag_page_info)
{
    fbe_zero_memory(vp_download_microcode_stat_diag_page_info, 
                    sizeof(vp_download_microcode_stat_diag_page_info_t));
    return FBE_STATUS_OK;
}

/**************************************************************************
 *  fbe_eses_copy_sg_list_into_contiguous_buffer()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function copies the sg_list memory into a contiguous
 *    virtual memory buffer
 *
 *  PARAMETERS:
 *     pointer to Virtual Phy download status page information
 *
 *  RETURN VALUES/ERRORS:
 *     success or failure.
 *
 *  NOTES:
 *   The absolute time comlexity can be reduced by using realloc and traversing
 *    the sg_list only once instead of twice. But that again has the overhead 
 *    of copying memory contents by realloc whenever a mem block is allocated in
 *    a new location. So its a tradeoff.
 *
 *  HISTORY:
 *     Nov-19-2010: Rajesh V created.
 **************************************************************************/
fbe_u8_t *
fbe_eses_copy_sg_list_into_contiguous_buffer(fbe_sg_element_t *sg_list)
{
    fbe_u32_t buffer_size=0;
    fbe_u8_t *buffer = NULL;
    fbe_u8_t *buffer_p;
    fbe_sg_element_t *sg_list_p = sg_list;

    //First caluclate the buffer size required
    sg_list_p = sg_list;
    while(sg_list_p->count != 0)
    {
        buffer_size += sg_list_p->count;
        sg_list_p ++;
    }

    buffer = (fbe_u8_t *)fbe_terminator_allocate_memory(buffer_size * sizeof(fbe_u8_t));
    if(!buffer)
    {
        return(buffer);
    }

    //Copy from the sg_list into our buffer
    sg_list_p = sg_list;
    buffer_p = buffer;
    while(sg_list_p->count != 0)
    {
        fbe_copy_memory(buffer_p, sg_list_p->address, sg_list_p->count);
        buffer_p += sg_list_p->count;
        sg_list_p ++;
    }

    return(buffer);
}
