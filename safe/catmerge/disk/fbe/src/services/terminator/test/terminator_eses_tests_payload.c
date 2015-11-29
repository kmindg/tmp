/***************************************************************************
 *  terminator_eses_tests_payload.c
 ***************************************************************************
 *
 *  Description
 *      This has Payload functions that are used
 *      by encl eses tests.
 *      
 *
 *  History:
 *     Oct08   Created
 *    
 ***************************************************************************/

#include "terminator_test.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe_terminator_miniport_interface.h"
#include "fbe_terminator.h"

#include "fbe/fbe_eses.h"
#include "fbe_scsi.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_topology_interface.h"
#include "terminator_eses_test.h"

/**********************************/
/*        local functions         */
/**********************************/
fbe_status_t terminator_cdb_operation_build_eses_cdb(
    fbe_payload_cdb_operation_t *payload_cdb_operation,
    fbe_scsi_command_t scsi_cmd,
    PVOID scsi_cmd_input_param);

fbe_status_t terminator_miniport_api_issue_eses_cmd (fbe_u32_t port_number, 
                                                    fbe_terminator_api_device_handle_t device_id, 
                                                    fbe_scsi_command_t scsi_cmd,
                                                    PVOID scsi_cmd_input_param,
    fbe_sg_element_t *sg_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t *payload;
    fbe_payload_cdb_operation_t *payload_cdb_operation;
    fbe_miniport_device_id_t cpd_device_id;
    
    payload = (fbe_payload_ex_t *) malloc(sizeof(fbe_payload_ex_t));
    memset(payload, 0, sizeof(fbe_payload_ex_t));
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);
    MUT_ASSERT_TRUE(payload_cdb_operation != NULL);
    status = fbe_payload_ex_increment_cdb_operation_level(payload);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = terminator_cdb_operation_build_eses_cdb(payload_cdb_operation, 
        scsi_cmd, scsi_cmd_input_param);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);;

    if (sg_p == NULL)
    {
        status = FBE_STATUS_INSUFFICIENT_RESOURCES;
        return status;
    }
    status = fbe_payload_ex_set_sg_list(payload, sg_p, 1);

    // Get the CPD device ID for the object (ID used by the terminator miniport interface)
    status = fbe_terminator_api_get_device_cpd_device_id(port_number, device_id, &cpd_device_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Now issue payload io to the miniport API */
    status = fbe_terminator_miniport_api_send_payload(port_number, (fbe_cpd_device_id_t)cpd_device_id, payload);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return(status);  
}

fbe_status_t terminator_cdb_operation_build_eses_cdb(fbe_payload_cdb_operation_t *payload_cdb_operation,
                                                    fbe_scsi_command_t scsi_cmd,
                                                    PVOID scsi_cmd_input_param)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_eses_receive_diagnostic_results_cdb_t *receive_diag_cmd;
    fbe_eses_send_diagnostic_cdb_t *send_diag_cmd;
    ses_pg_code_enum *page_code;
    fbe_eses_read_buf_cdb_t *read_buf_cmd;
    fbe_eses_write_buf_cdb_t *write_buf_cmd;
    fbe_u8_t *cdb;
    read_buf_test_cdb_param_t *read_buf_param;
    write_buf_test_cdb_param_t *write_buf_param;
    mode_cmds_test_mode_sense_cdb_param_t *mode_sense_cdb_param;
    mode_cmds_test_mode_select_cdb_param_t *mode_select_cdb_param;
    fbe_eses_mode_sense_10_cdb_t *mode_sense_cdb;
    fbe_eses_mode_select_10_cdb_t *mode_select_cdb;

    fbe_payload_cdb_operation_get_cdb(payload_cdb_operation, &cdb);

    cdb[0] = scsi_cmd;

    switch(scsi_cmd)
    {
    case FBE_SCSI_RECEIVE_DIAGNOSTIC:
        receive_diag_cmd = (fbe_eses_receive_diagnostic_results_cdb_t *)(cdb);
        receive_diag_cmd->page_code_valid = 1;
        receive_diag_cmd->control = 0;
        receive_diag_cmd->operation_code = FBE_SCSI_RECEIVE_DIAGNOSTIC;
        page_code = (ses_pg_code_enum *)(scsi_cmd_input_param);
        receive_diag_cmd->page_code = *page_code;
        break;

    case FBE_SCSI_SEND_DIAGNOSTIC:
        send_diag_cmd = (fbe_eses_send_diagnostic_cdb_t *)(cdb);
        send_diag_cmd->self_test_code = 0;
        send_diag_cmd->control = 0;
        send_diag_cmd->page_format = 1;
        page_code = (ses_pg_code_enum *)(scsi_cmd_input_param);
        send_diag_cmd->operation_code = FBE_SCSI_SEND_DIAGNOSTIC;
        break;

    case FBE_SCSI_READ_BUFFER:
        read_buf_param = (read_buf_test_cdb_param_t *)(scsi_cmd_input_param);
        read_buf_cmd = (fbe_eses_read_buf_cdb_t *)(cdb);
        read_buf_cmd->buf_id = (read_buf_param->buf_id);
        read_buf_cmd->mode = FBE_ESES_READ_BUF_MODE_DATA;
        read_buf_cmd->alloc_length_lsbyte =  ((read_buf_param->alloc_len) & 0xFF);
        read_buf_cmd->alloc_length_midbyte = ((read_buf_param->alloc_len) & 0xFF00) >> 8;
        read_buf_cmd->alloc_length_msbyte =  ((read_buf_param->alloc_len) & 0xFF0000) >> 16;
        read_buf_cmd->buf_offset_lsbyte =  ((read_buf_param->buf_offset) & 0xFF);
        read_buf_cmd->buf_offset_midbyte = ((read_buf_param->buf_offset) & 0xFF00) >> 8;
        read_buf_cmd->buf_offset_msbyte =  ((read_buf_param->buf_offset) & 0xFF0000) >> 16;
        break;

    case FBE_SCSI_WRITE_BUFFER:
        write_buf_param = (write_buf_test_cdb_param_t *)(scsi_cmd_input_param);
        write_buf_cmd = (fbe_eses_write_buf_cdb_t *)(cdb);
        write_buf_cmd->buf_id = (write_buf_param->buf_id);
        write_buf_cmd->mode = FBE_ESES_WRITE_BUF_MODE_DATA;
        write_buf_cmd->param_list_length_lsbyte =  ((write_buf_param->param_list_len) & 0xFF);
        write_buf_cmd->param_list_length_midbyte = ((write_buf_param->param_list_len) & 0xFF00) >> 8;
        write_buf_cmd->param_list_length_msbyte =  ((write_buf_param->param_list_len) & 0xFF0000) >> 16;
        write_buf_cmd->buf_offset_lsbyte =  ((write_buf_param->buf_offset) & 0xFF);
        write_buf_cmd->buf_offset_midbyte = ((write_buf_param->buf_offset) & 0xFF00) >> 8;
        write_buf_cmd->buf_offset_msbyte =  ((write_buf_param->buf_offset) & 0xFF0000) >> 16;
        break;

    case  FBE_SCSI_MODE_SENSE_10:
        mode_sense_cdb_param = (mode_cmds_test_mode_sense_cdb_param_t *)(scsi_cmd_input_param);
        mode_sense_cdb = (fbe_eses_mode_sense_10_cdb_t *)(cdb);
        memset(mode_sense_cdb, 0, sizeof(fbe_eses_mode_sense_10_cdb_t));
        mode_sense_cdb->operation_code = FBE_SCSI_MODE_SENSE_10;
        mode_sense_cdb->pc = mode_sense_cdb_param->pc;
        mode_sense_cdb->pg_code = mode_sense_cdb_param->pg_code;
        mode_sense_cdb->subpg_code = mode_sense_cdb_param->subpg_code;
        break;

    case  FBE_SCSI_MODE_SELECT_10:
        mode_select_cdb_param = (mode_cmds_test_mode_select_cdb_param_t *)(scsi_cmd_input_param);
        mode_select_cdb = (fbe_eses_mode_select_10_cdb_t *)(cdb);
        memset(mode_select_cdb, 0, sizeof(fbe_eses_mode_select_10_cdb_t));
        mode_select_cdb->operation_code = FBE_SCSI_MODE_SELECT_10;
        mode_select_cdb->param_list_length_lsbyte = (mode_select_cdb_param->param_list_len & 0xFF);
        mode_select_cdb->param_list_length_msbyte = ((mode_select_cdb_param->param_list_len & 0xFF00) >> 8);
        break;

    default:
        break;
    }
    return(status);
}