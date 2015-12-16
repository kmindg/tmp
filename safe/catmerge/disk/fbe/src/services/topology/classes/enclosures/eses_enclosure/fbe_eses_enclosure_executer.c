#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_transport_memory.h"
#include "fbe_scsi.h"
#include "fbe_sas_port.h"
#include "fbe_eses_enclosure_private.h"
#include "fbe_eses_enclosure_config.h"
#include "edal_eses_enclosure_data.h"
#include "fbe/fbe_eses.h"
#include "fbe_sas_enclosure_debug.h"
#include "fbe_eses_enclosure_debug.h"

#ifdef C4_INTEGRATED
#include "specl_types.h"
#include "specl_interface.h"
#endif /* C4_INTEGRATED - C4HW */

//#include "fbe_enclosure.h"

/* We force hardcode phy slot mapping for now, 
 * To turn it off, we need to remove this
 */
//#define ESES_FORCE_HARDCODE_VALUE 1

static fbe_u32_t fbe_eses_enclosure_spsCmdToken = 0;

/* Forward declarations */
static fbe_status_t fbe_eses_enclosure_send_get_element_list_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_eses_enclosure_cleanup_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                                   fbe_packet_t * packet,
                                                   fbe_payload_control_status_t control_status,
                                                   fbe_payload_control_status_qualifier_t control_status_qualifier);
static fbe_status_t fbe_eses_enclosure_setup_inquiry_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                                        fbe_packet_t * packet,
                                                        fbe_payload_cdb_operation_t * payload_cdb_operation);   
static fbe_status_t fbe_eses_enclosure_setup_receive_diag_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                                            fbe_packet_t * packet,
                                                            fbe_payload_cdb_operation_t * payload_cdb_operation);                                                                                 
static fbe_status_t fbe_eses_enclosure_setup_send_diag_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                                            fbe_packet_t * packet,
                                                            fbe_payload_cdb_operation_t * payload_cdb_operation);    
static fbe_status_t fbe_eses_enclosure_setup_read_buf_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                                            fbe_packet_t * packet,
                                                            fbe_payload_cdb_operation_t * payload_cdb_operation);   
static fbe_status_t fbe_eses_enclosure_setup_raw_inquiry_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                                            fbe_packet_t * packet,
                                                            fbe_payload_cdb_operation_t * payload_cdb_operation);   
static fbe_status_t fbe_eses_enclosure_setup_raw_rcv_diag_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                                            fbe_packet_t * packet,
                                                            fbe_payload_cdb_operation_t * payload_cdb_operation);   
static fbe_status_t fbe_eses_enclosure_setup_write_resume_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                                            fbe_packet_t * packet,
                                                            fbe_payload_cdb_operation_t * payload_cdb_operation);
static fbe_status_t fbe_eses_enclosure_setup_sps_in_buf_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                               fbe_packet_t * packet,
                                               fbe_payload_cdb_operation_t * payload_cdb_operation);
static fbe_status_t fbe_eses_enclosure_setup_sps_eeprom_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                                            fbe_packet_t * packet,
                                                            fbe_payload_cdb_operation_t * payload_cdb_operation);
static fbe_status_t fbe_eses_enclosure_setup_mode_sense_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                                            fbe_packet_t * packet,
                                                            fbe_payload_cdb_operation_t * payload_cdb_operation);
static fbe_status_t fbe_eses_enclosure_setup_mode_select_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                                            fbe_packet_t * packet,
                                                            fbe_payload_cdb_operation_t * payload_cdb_operation);
static fbe_status_t fbe_eses_enclosure_setup_get_rp_size_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                        fbe_packet_t * packet,
                                        fbe_payload_cdb_operation_t * payload_cdb_operation);  
static fbe_status_t fbe_eses_enclosure_send_scsi_cmd_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_enclosure_status_t fbe_eses_enclosure_handle_get_inquiry_data_response(fbe_eses_enclosure_t * eses_enclosure, fbe_sg_element_t * sg_list);
static fbe_enclosure_status_t fbe_eses_enclosure_handle_validate_identity_response(fbe_eses_enclosure_t * eses_enclosure, fbe_sg_element_t * sg_list);
static fbe_enclosure_status_t fbe_eses_enclosure_handle_get_config_response(fbe_eses_enclosure_t * eses_enclosure, fbe_sg_element_t * sg_list);
static fbe_enclosure_status_t fbe_eses_enclosure_handle_get_sas_encl_type_response(fbe_eses_enclosure_t * eses_enclosure, fbe_sg_element_t * sg_list);
static fbe_enclosure_status_t fbe_eses_enclosure_handle_get_addl_status_response(fbe_eses_enclosure_t * eses_enclosure, fbe_sg_element_t * sg_list);
static fbe_enclosure_status_t fbe_eses_enclosure_handle_get_emc_specific_status_response(fbe_eses_enclosure_t * eses_enclosure, fbe_sg_element_t * sg_list);
static fbe_enclosure_status_t fbe_eses_enclosure_handle_get_status_response(fbe_eses_enclosure_t * eses_enclosure, fbe_sg_element_t * sg_list);
static fbe_enclosure_status_t fbe_eses_enclosure_handle_get_rp_size_response(fbe_eses_enclosure_t * eses_enclosure, fbe_sg_element_t * sg_list);
static fbe_enclosure_status_t 
fbe_eses_enclosure_handle_mode_sense_response(fbe_eses_enclosure_t * eses_enclosure, 
                                              fbe_packet_t * packet,
                                              fbe_sg_element_t * sg_list);
static fbe_enclosure_status_t fbe_eses_enclosure_handle_get_download_status_response(fbe_eses_enclosure_t * eses_enclosure, 
                                                                                   fbe_sg_element_t * sg_list, 
                                                                                   fbe_enclosure_status_t encl_status); 
static fbe_enclosure_status_t 
fbe_eses_enclosure_handle_get_emc_statistics_status_response(fbe_eses_enclosure_t * eses_enclosure, 
                                                             fbe_sg_element_t * sg_list);
static fbe_enclosure_status_t fbe_eses_enclosure_handle_emc_statistics_status_cmd_response(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet);
static fbe_enclosure_status_t fbe_eses_enclosure_handle_threshold_in_response(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet);
static fbe_enclosure_status_t fbe_eses_enclosure_handle_get_trace_buf_info_status_response(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet);
static fbe_enclosure_status_t fbe_eses_enclosure_handle_read_buf_response(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet);                                                                                           
static fbe_enclosure_status_t fbe_eses_enclosure_handle_raw_rcv_diag_response(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet);                                                                                           
static fbe_enclosure_status_t fbe_eses_enclosure_handle_raw_inquiry_response(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet);

/*static fbe_enclosure_status_t fbe_eses_enclosure_handle_scsi_cmd_response(fbe_eses_enclosure_t * eses_enclosure, 
                                                   fbe_packet_t * packet,
                                                   fbe_eses_ctrl_opcode_t opcode,
                                                   fbe_scsi_error_code_t * scsi_error_code_p);*/
static fbe_scsi_error_code_t fbe_eses_enclosure_get_scsi_error_code(fbe_eses_enclosure_t * eses_enclosure, fbe_u8_t * sense_data);
static fbe_enclosure_status_t fbe_eses_enclosure_handle_scsi_error_code(fbe_eses_enclosure_t * eses_enclosure,
                                                                fbe_eses_ctrl_opcode_t opcode,
                                                                fbe_scsi_error_code_t scsi_error_code);
static fbe_enclosure_status_t fbe_eses_enclosure_handle_unsupported_emc_specific_status_page(fbe_eses_enclosure_t * eses_enclosure);


static fbe_status_t eses_enclosure_discovery_transport_get_protocol_address(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet);
static fbe_status_t eses_enclosure_discovery_transport_get_slot_info(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet);
static fbe_status_t eses_enclosure_discovery_transport_record_fw_activation_status(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet);
static fbe_status_t eses_enclosure_discovery_transport_get_server_info(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet);
fbe_bool_t fbe_eses_enclosure_is_activate_timeout(fbe_eses_enclosure_t * eses_enclosure);

static fbe_status_t fbe_eses_enclosure_check_duplicate_ESN(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet);
static fbe_status_t fbe_eses_enclosure_check_encl_SN_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
fbe_packet_t *fbe_eses_enclosure_build_encl_SN_packet(fbe_eses_enclosure_t *eses_enclosure, fbe_packet_t *packet);
void fbe_eses_printSendControlPage(fbe_eses_enclosure_t * eses_enclosure, fbe_u8_t *controlPagePtr);

static fbe_status_t fbe_eses_enclosure_notify_activation_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t 
fbe_eses_enclosure_discovery_transport_drive_power_ctrl(fbe_eses_enclosure_t *eses_enclosure, 
                                                 fbe_packet_t *packet);
static fbe_status_t 
fbe_eses_enclosure_discovery_transport_drive_power_cycle(fbe_eses_enclosure_t *eses_enclosure, 
                                        fbe_packet_t *packet);
static fbe_status_t 
fbe_eses_enclosure_discovery_transport_reset_drive_slot(fbe_eses_enclosure_t *eses_enclosure, 
                                        fbe_packet_t *packet);
static fbe_status_t 
fbe_eses_enclosure_discoveryUnbypass(fbe_eses_enclosure_t *eses_enclosure, 
                                         fbe_packet_t *packet);
static fbe_status_t fbe_eses_enclosure_unbypass_drive(fbe_eses_enclosure_t * eses_enclosure, fbe_u8_t slotIndex);
static fbe_status_t fbe_eses_enclosure_unbypass_expansion_port(fbe_eses_enclosure_t * eses_enclosure, fbe_edge_index_t server_index);

static fbe_enclosure_status_t fbe_eses_enclosure_receive_diag_validate_page(fbe_eses_enclosure_t *eses_enclosure,
                                                            ses_pg_code_enum expected_page_code,
                                                            fbe_payload_ex_t * payload);
static fbe_packet_t * 
fbe_eses_enclosure_build_reset_drive_slot_packet(fbe_eses_enclosure_t * eses_enclosure, 
                                                   fbe_packet_t * packet,
                                                   fbe_u32_t phy_id);
static fbe_status_t 
fbe_eses_enclosure_send_reset_drive_slot_completion(fbe_packet_t * packet, 
                                                     fbe_packet_completion_context_t context);

static __forceinline fbe_status_t 
fbe_eses_enclosure_increment_outstanding_scsi_request_count(fbe_eses_enclosure_t * eses_enclosure);

static __forceinline fbe_status_t 
fbe_eses_enclosure_decrement_outstanding_scsi_request_count(fbe_eses_enclosure_t * eses_enclosure);

static __forceinline fbe_status_t 
fbe_eses_enclosure_check_outstanding_scsi_request_count(fbe_eses_enclosure_t * eses_enclosure);


fbe_status_t 
fbe_eses_enclosure_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_eses_enclosure_t * eses_enclosure = NULL;
    fbe_transport_id_t transport_id;
    fbe_status_t status;

    eses_enclosure = (fbe_eses_enclosure_t *)fbe_base_handle_to_pointer(object_handle);
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__);

    /* Fisrt we need to figure out to what transport this packet belong */
    fbe_transport_get_transport_id(packet, &transport_id);
    switch(transport_id) {
        case FBE_TRANSPORT_ID_DISCOVERY:
            /* The server part of fbe_discovery transport is a member of discovering class.
                Even more than that, we do not expect to receive discovery protocol packets
                for "non discovering" objects 
            */
            status = fbe_base_discovering_discovery_bouncer_entry((fbe_base_discovering_t *) eses_enclosure,
                                                                        (fbe_transport_entry_t)fbe_eses_enclosure_discovery_transport_entry,
                                                                        packet);
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}


/*!*************************************************************************
* @fn fbe_eses_enclosure_handle_get_inquiry_data_response(fbe_eses_enclosure_t * eses_enclosure, 
*                                                   fbe_sg_element_t * sg_list)                  
***************************************************************************
* @brief
*       This function handles the response got from the "inquiry" command 
*       with the "get inquiry data" opcode. It gets the following data.
*       (1) The enclosure platform type;
*       (2) The enclosure UID(will be removed);
*       (3) The enclosure serial number.
*
* @param eses_enclosure - The pointer to the ESES enclosure.
* @param sg_list - the pointer to the sg list allocated while sending the command.
*
* @return  fbe_enclosure_status_t.
*      Always returns FBE_ENCLOSURE_STATUS_OK.
*
* NOTES
*
* HISTORY:
*  8/15/08 - add comment, NCHIU
*  12-Dec-2008 PHE - Changed eses_enclosure_send_inquiry_completion to
*                    fbe_eses_enclosure_handle_get_inquiry_data_response
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_handle_get_inquiry_data_response(fbe_eses_enclosure_t * eses_enclosure, 
                                                    fbe_sg_element_t * sg_list)
{
    fbe_u16_t        enclosure_platform_type = 0;
    fbe_u16_t        enclosure_platform_type_big_endian = 0;
    fbe_u16_t        board_type = 0;
    fbe_u16_t        board_type_big_endian = 0;
    fbe_enclosure_status_t     encl_status = FBE_ENCLOSURE_STATUS_OK;
    #ifdef C4_INTEGRATED
    fbe_enclosure_number_t  enclosure_number = FBE_ENCLOSURE_VALUE_INVALID;
    #endif /* C4_INTEGRATED - C4HW */

    /********
     * BEGIN
     ********/

    fbe_copy_memory(&enclosure_platform_type_big_endian, ((fbe_u8_t *)(sg_list[0].address)) + FBE_SCSI_INQUIRY_ENCLOSURE_PLATFORM_TYPE_OFFSET, FBE_SCSI_INQUIRY_ENCLOSURE_PLATFORM_TYPE_SIZE);        
    enclosure_platform_type = swap16(enclosure_platform_type_big_endian);
        
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "%s, big_endian 0x%x, enclosure_platform_type = 0x%x.\n",
                           __FUNCTION__, enclosure_platform_type_big_endian, enclosure_platform_type);

    /* Get FFID */ 	
    fbe_copy_memory(&board_type_big_endian, ((fbe_u8_t *)(sg_list[0].address)) + FBE_SCSI_INQUIRY_BOARD_TYPE_OFFSET, FBE_SCSI_INQUIRY_BOARD_TYPE_SIZE);  
    board_type = swap16(board_type_big_endian);

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_INFO,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "handle_get_inquiry_data_response, big_endian = 0x%x, board_type = 0x%x\n",
                           board_type_big_endian, board_type);
    
#if (MOCKUP_VOYAGER)
    // HACK FOR MOCKUP VOYAGER
    // This is a hack to force the VIPERs that are being used as edge
    // expanders for the Voyager to show as though they are EEs The
    // ICM will have the correct type, but we will override the Vipers
    // with a component id of non-zero.  These we will pretend are
    // Voyagers.
    if (((fbe_base_enclosure_t *) eses_enclosure)->component_id != 0)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_INFO,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                               "handle_get_inquiry_data_response, force nonzero component_id encl to be a Voyager\n");
        enclosure_platform_type = FBE_ESES_ENCLOSURE_PLATFORM_TYPE_VOYAGER;
    }
#endif

    switch(enclosure_platform_type)
    {
        case FBE_ESES_ENCLOSURE_PLATFORM_TYPE_VIPER:          
            eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
            break; 
        case FBE_ESES_ENCLOSURE_PLATFORM_TYPE_PINECONE:          
            eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_PINECONE;
            break;			
        case FBE_ESES_ENCLOSURE_PLATFORM_TYPE_ANCHO:		  
                eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_ANCHO;
            break; 
        case FBE_ESES_ENCLOSURE_PLATFORM_TYPE_MAGNUM:          
            eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_MAGNUM;
            break; 
       case FBE_ESES_ENCLOSURE_PLATFORM_TYPE_DERRINGER:          
            eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_DERRINGER;
            break;                    
       case FBE_ESES_ENCLOSURE_PLATFORM_TYPE_TABASCO:          
            eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_TABASCO;
            break; 
        case FBE_ESES_ENCLOSURE_PLATFORM_TYPE_SENTRY15:
            eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_BUNKER;
            break;     
        case FBE_ESES_ENCLOSURE_PLATFORM_TYPE_SENTRY25:
            eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_CITADEL;    
            break;     
        case FBE_ESES_ENCLOSURE_PLATFORM_TYPE_JETFIRE25:
            eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_FALLBACK;
            break;     

        case FBE_ESES_ENCLOSURE_PLATFORM_TYPE_EVERGREEN12:
        case FBE_ESES_ENCLOSURE_PLATFORM_TYPE_BEACHCOMBER12:
            eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_STEELJAW;
            break;     

        case FBE_ESES_ENCLOSURE_PLATFORM_TYPE_BEACHCOMBER25:
            eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_RAMHORN;
            break;     

        case FBE_ESES_ENCLOSURE_PLATFORM_TYPE_SILVERBOLT12:
            eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_BOXWOOD;
            break;     

        case FBE_ESES_ENCLOSURE_PLATFORM_TYPE_EVERGREEN25:
        case FBE_ESES_ENCLOSURE_PLATFORM_TYPE_SILVERBOLT25:
            eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_KNOT;
            break;     

        case FBE_ESES_ENCLOSURE_PLATFORM_TYPE_VOYAGER:
            if(board_type == FBE_ESES_ENCLOSURE_BOARD_TYPE_VOYAGER_ICM)
            {
                eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM;
            }
            else if(board_type == FBE_ESES_ENCLOSURE_BOARD_TYPE_VOYAGER_EE
#if (MOCKUP_VOYAGER)
                 // We need to handle the case where we are using Vipers in place of
                 // edge expanders.  In this case the board type shows up as 0. So
                 // use this to force the enclosure type to the EE.
                      || board_type == 0
#endif
                   )
            {
                eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE;      
            }
            else
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "handle_get_inquiry_data_response, board_type (0x%x) != to (icm 0x%x or ee 0x%x)\n",
                           board_type, FBE_ESES_ENCLOSURE_BOARD_TYPE_VOYAGER_ICM, FBE_ESES_ENCLOSURE_BOARD_TYPE_VOYAGER_EE);
                eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
                encl_status =  FBE_ENCLOSURE_STATUS_DATA_ILLEGAL;
            }
            break;

        case FBE_ESES_ENCLOSURE_PLATFORM_TYPE_VIKING:
            eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_VIKING;
            break;

        case FBE_ESES_ENCLOSURE_PLATFORM_TYPE_CAYENNE:
            eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_CAYENNE;
            break;

        case FBE_ESES_ENCLOSURE_PLATFORM_TYPE_NAGA:
            eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_NAGA;
            break;

        case FBE_ESES_ENCLOSURE_PLATFORM_TYPE_OBERON12:
            eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_RHEA;
            break;     

        case FBE_ESES_ENCLOSURE_PLATFORM_TYPE_OBERSON25:
            eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_MIRANDA;
            break;     

        case FBE_ESES_ENCLOSURE_PLATFORM_TYPE_HYPERION:
            eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_CALYPSO;
            break;     

        case FBE_ESES_ENCLOSURE_PLATFORM_TYPE_UNKNOWN:
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                        "handle_get_inquiry_data_response, enclosure platform type is unknown. Retry later\n");

            eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_UNKNOWN;
            /* Leave the current condition set, so we will retry later */        
            encl_status =  FBE_ENCLOSURE_STATUS_BUSY;
            break;

        case FBE_ESES_ENCLOSURE_PLATFORM_TYPE_INVALID:
        default:            
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                        "%s, unsupported enclosure platform type: 0x%x.\n",
                        __FUNCTION__, enclosure_platform_type);

            eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
            break; 
 
    }

//  // Get enclosure UID(will be removed)
//  fbe_copy_memory(&(eses_enclosure->unique_id),
//                ((fbe_u8_t *)(sg_list[0].address)) + FBE_SCSI_INQUIRY_ENCLOSURE_UNIQUE_ID_OFFSET,
//                FBE_SCSI_INQUIRY_ENCLOSURE_UNIQUE_ID_SIZE);
       
    // Get enclosure serial number.
    fbe_copy_memory(eses_enclosure->encl_serial_number, 
                  ((fbe_u8_t *)(sg_list[0].address)) + FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_OFFSET, 
                  FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE); 

    if (fbe_equal_memory(eses_enclosure->encl_serial_number, "UNKNOWN", 7))
    {
        /* Leave the current condition set, so we will retry later */
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, had unknown SN.  Retry later.\n",
                                __FUNCTION__ );
        
        
        encl_status =  FBE_ENCLOSURE_STATUS_BUSY;
    }

    #ifdef C4_INTEGRATED
    //
    //  If INVALID is returned, then the Inquiry response data does not
    //  the System SN.  So attempt to read it from the resume ourselves.
    //
    fbe_base_enclosure_get_enclosure_number((fbe_base_enclosure_t *)eses_enclosure, &enclosure_number);

    if ((encl_status != FBE_ENCLOSURE_STATUS_BUSY ) && 
        (enclosure_number == 0 ) &&
        (fbe_equal_memory(eses_enclosure->encl_serial_number, "INVALID", 7)) )
    {
        SPECL_RESUME_DATA                         resume_data;
        fbe_zero_memory(&resume_data, sizeof(SPECL_RESUME_DATA));

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, had invalid SN.  Retrieving from the Resume.\n",
                                __FUNCTION__ );

        speclGetResume(CHASSIS_SERIAL_EEPROM, &resume_data);

        //
        //  If the resume read was successful, copy the SN out of
        //  out of the resume data.
        //
        if (resume_data.transactionStatus == EMCPAL_STATUS_SUCCESS)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, Setting encl SN to %s from Resume.\n",
                                    __FUNCTION__ ,
                                    resume_data.resumePromData.product_serial_number);

            // Get enclosure serial number.
            fbe_copy_memory(eses_enclosure->encl_serial_number, 
                            resume_data.resumePromData.product_serial_number,
                            FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE); 
        }
        else
        {
            encl_status =  FBE_ENCLOSURE_STATUS_BUSY;
        }
    }
    #endif /* C4_INTEGRATED - C4HW */

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                            "handle_get_inquiry_data_response, %s enclosure is added. SN:%s\n",                            
                            fbe_sas_enclosure_print_sasEnclosureType(eses_enclosure->sas_enclosure_type, NULL, NULL),
                            eses_enclosure->encl_serial_number);

    return encl_status;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_handle_validate_identity_response(fbe_eses_enclosure_t * eses_enclosure, 
*                                                   fbe_sg_element_t * sg_list)                  
***************************************************************************
* @brief
*       This function handles the response got from the "inquiry" command 
*       with the "validate identity" opcode. 
*       It compares enclosure serial number from the response with the 
*       enclosure serial number saved in the enclosure object which got from 
*       "get inquiry data" command to see whether this is the same enclosure 
*       as before if the enclosure goes through the reset. If they mismatch,
*       set the enclosure to the destroy state.
*
* @param eses_enclosure - The pointer to the ESES enclosure.
* @param sg_list - the pointer to the sg list allocated while sending the command.
*
* @return  fbe_enclosure_status_t.
*      Always returns FBE_ENCLOSURE_STATUS_OK.
*
* NOTES
*
* HISTORY:
*  8/15/08 - add comment, NCHIU
*  12-Dec-2008 PHE - Changed eses_enclosure_validata_encl_SN_completion to
*                    fbe_eses_enclosure_handle_validate_identity_response
****************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_handle_validate_identity_response(fbe_eses_enclosure_t * eses_enclosure, 
                                                    fbe_sg_element_t * sg_list)
{
    fbe_u8_t                enclosure_SN[FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE+1];
    fbe_u16_t               enclosure_platform_type = 0;
    fbe_u16_t               enclosure_platform_type_big_endian = 0, eses_version_desc_big_endian = 0;
    fbe_enclosure_status_t  status = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_number_t  enclosure_number = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_port_number_t port_number;
    fbe_bool_t              old_SN_invalid, new_SN_invalid;
    fbe_enclosure_component_id_t component_id;

    fbe_zero_memory(enclosure_SN,(FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE+1));    

    fbe_copy_memory(enclosure_SN, 
                  ((fbe_u8_t *)(sg_list[0].address)) + FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_OFFSET, 
                  FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE);    
     
#if 0

    /* check against INVALID UID is disabled.  AEN will do the necessary check */
    if (fbe_equal_memory(enclosure_SN, "INVALID", 7))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                    "%s, Invalid SN, fail the enclosure.\n", __FUNCTION__);
        
        // update fault information and fail the enclosure
        status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *) eses_enclosure, 
                                        0,
                                        0,
                                        FBE_ENCL_FLTSYMPT_INVALID_SN,
                                        enclosure_SN);

        // trace out the error if we can't handle the error
        if(!ENCL_STAT_OK(status))
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "%s can't set BO:FAIL condition, status: 0x%X\n",
                                __FUNCTION__, status);
        }

        /* Set the status to BUSY so that we won't try to change the class id 
         * in function eses_enclosure_inquiry_data_not_exist_cond_completion.
         * The command won't be retried because the enclosure will go to fail state.
         */
        return FBE_ENCLOSURE_STATUS_BUSY;
    }
#endif

    if (fbe_equal_memory(enclosure_SN, "UNKNOWN", 7))
    {
        /* Leave the current condition set, so we will retry later */
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, enclosure had unknown SN.  Retry later.\n",
                                __FUNCTION__ );
        
        
        return  FBE_ENCLOSURE_STATUS_BUSY;
    }

#ifdef C4_INTEGRATED
    //
    //  If INVALID is returned, then the Inquiry response data does not
    //  the System SN.  So attempt to read it from the resume ourselves.
    //
    fbe_base_enclosure_get_enclosure_number((fbe_base_enclosure_t *)eses_enclosure, &enclosure_number);

    if ((enclosure_number == 0 ) &&
        (fbe_equal_memory(enclosure_SN, "INVALID", 7)) )
    {
        SPECL_RESUME_DATA                         resume_data;
        fbe_zero_memory(&resume_data, sizeof(SPECL_RESUME_DATA));

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, had invalid SN.  Retrieving from the Resume.\n",
                                __FUNCTION__ );

        speclGetResume(CHASSIS_SERIAL_EEPROM, &resume_data);

        //
        //  If the resume read was successful, copy the SN out of
        //  out of the resume data.
        //
        if (resume_data.transactionStatus == EMCPAL_STATUS_SUCCESS)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, Setting encl SN to %s from Resume.\n",
                                    __FUNCTION__ ,
                                    resume_data.resumePromData.product_serial_number);

            // Get enclosure serial number.
            fbe_copy_memory(enclosure_SN, 
                            resume_data.resumePromData.product_serial_number,
                            FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE); 
        }
        else
        {
            return FBE_ENCLOSURE_STATUS_BUSY;
        }
    }
#endif /* C4_INTEGRATED - C4HW */

    /* We need to destroy the object if SN mismatch. */
    if (!fbe_equal_memory(eses_enclosure->encl_serial_number, 
                          enclosure_SN, 
                          FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE))
    {
        // port number
        fbe_base_enclosure_get_port_number((fbe_base_enclosure_t *)eses_enclosure, &port_number);
        // Enclosure Position
        fbe_base_enclosure_get_enclosure_number((fbe_base_enclosure_t *)eses_enclosure, &enclosure_number);

        new_SN_invalid = fbe_equal_memory(enclosure_SN, "INVALID", 7);
        old_SN_invalid = fbe_equal_memory(eses_enclosure->encl_serial_number, "INVALID", 7);

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "enclosure SN mismatch, old  = %s, new = %s\n",
                                    eses_enclosure->encl_serial_number,
                                    enclosure_SN);

        component_id = fbe_base_enclosure_get_component_id((fbe_base_enclosure_t *)eses_enclosure);

        /* Only check wether the destroy is needed when the object is not the component object, 
         * i.e. the component id is zero. 
         */
        if (component_id == 0)
        {
            
            /* If one of the serial number is INVALID and it is the boot enclosure, 
             * We are not going to destroy the boot enclosure, let AEN to do more check on it.
             * Otherwise, destroy the enclosure.
         */
        if (!(((port_number==0) && (enclosure_number==0)) &&
              (new_SN_invalid || old_SN_invalid)))
        {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_INFO,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "%s destroying enclosure.\n",
                                            __FUNCTION__);
    
            status = fbe_base_enclosure_set_lifecycle_cond(
                                            &fbe_eses_enclosure_lifecycle_const,
                                            (fbe_base_enclosure_t*)eses_enclosure, 
                                            FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);
            if(!ENCL_STAT_OK(status))            
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s can't set BO:DESTROY condition, status: 0x%X\n",
                                        __FUNCTION__, status);

                return FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED;
            }
        }
    }
    }

    fbe_copy_memory(&enclosure_platform_type_big_endian, 
                        ((fbe_u8_t *)(sg_list[0].address)) + FBE_SCSI_INQUIRY_ENCLOSURE_PLATFORM_TYPE_OFFSET, 
                        FBE_SCSI_INQUIRY_ENCLOSURE_PLATFORM_TYPE_SIZE);        
    enclosure_platform_type = swap16(enclosure_platform_type_big_endian);

    // enclosure platform type is unknown
    if ((enclosure_platform_type == FBE_ESES_ENCLOSURE_PLATFORM_TYPE_UNKNOWN) ||
        (enclosure_platform_type == FBE_ESES_ENCLOSURE_PLATFORM_TYPE_INVALID))
    {
        /* Leave the current condition set, so we will retry later */
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                        "%s, enclosure platform type is unknown. Retry later.\n", __FUNCTION__);
        
        return  FBE_ENCLOSURE_STATUS_BUSY;
    }

    // eses version descriptor
    fbe_copy_memory(&eses_version_desc_big_endian, 
        ((fbe_u8_t *)(sg_list[0].address)) + FBE_SCSI_INQUIRY_ESES_VERSION_DESCRIPTOR_OFFSET, 
        FBE_SCSI_INQUIRY_ESES_VERSION_DESCRIPTOR_SIZE);        
    eses_enclosure->eses_version_desc = swap16(eses_version_desc_big_endian);
        
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, eses_version_desc = 0x%x.\n",
                            __FUNCTION__, eses_enclosure->eses_version_desc);

    if(eses_enclosure->eses_version_desc > MAX_SUPPORTED_ESES_VENDOR_DESCRIPTOR)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
        FBE_TRACE_LEVEL_INFO,
        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
        "%s, Invalid ESES VERSION DESCRIPTOR:0x%x, fail the enclosure.\n", __FUNCTION__, eses_enclosure->eses_version_desc);
    
        // update fault information and fail the enclosure
        status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *) eses_enclosure, 
                                        0,
                                        0,
                                        FBE_ENCL_FLTSYMPT_INVALID_ESES_VERSION_DESCRIPTOR,
                                        eses_enclosure->eses_version_desc);

        // trace out the error if we can't handle the error
        if(!ENCL_STAT_OK(status))
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "%s can't set FAIL condition, status: 0x%X\n",
                                __FUNCTION__, status);
        }
    }

    return FBE_ENCLOSURE_STATUS_OK;
}

/****************************************************************
 * fbe_eses_enclosure_send_get_element_list_command()
 ****************************************************************
 * DESCRIPTION:
 *  This function generates get_element_list request
 * and sends it down.
 *
 * PARAMETERS:
 *  p_object - our object context
 *  packet - request packet
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  9/9/08 - Add comment. nchiu
 *  11/25/08 - Switch to use SMP edge.  nchiu
 *
 ****************************************************************/
fbe_status_t 
fbe_eses_enclosure_send_get_element_list_command(fbe_eses_enclosure_t * eses_enclosure, 
                                                      fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_packet_t * new_packet = NULL;

    new_packet = fbe_sas_enclosure_build_get_element_list_packet((fbe_sas_enclosure_t *)eses_enclosure, packet);
    if (new_packet == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_transport_allocate_packet failed\n",
                                __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* The above call already set up a call back into fbe_sas_enclosure_send_get_element_list_completion()
     * Below just push additional completion context on the packet stack.
     * So we don't need to release packet in our completion routine.
     */
    status = fbe_transport_set_completion_function(new_packet, fbe_eses_enclosure_send_get_element_list_completion, eses_enclosure);

    /* Note we can not send the packet right away.
        It is possible that blocking is set or queue depth is exeded.
        So, we should "submit" "send" the packet to the bouncer for evaluation.
        Right now we have no bouncer implemented, but as we will have it we will use it.
    */
    /* We are sending control packet via smp edge */
    status = fbe_sas_enclosure_send_smp_functional_packet((fbe_sas_enclosure_t *) eses_enclosure, new_packet);

    return status;
}

/****************************************************************
 * fbe_eses_enclosure_send_get_element_list_completion()
 ****************************************************************
 * DESCRIPTION:
 *  This function handles the completion of get_element_list.
 *  It records client addresses from element_list.
 *  Because this function is piggy back on  
 *  fbe_sas_enclosure_send_get_element_list_completion, which
 *  does the work of releasing the packet, we don't have to
 *  do it here.
 *
 * PARAMETERS:
 *  packet - io packet pointer
 *  context - completion conext
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK 
 *                   - no error.
 *
 * HISTORY:
 *  8/15/08 - add comment, NCHIU
 *  11/25/08 - Switch to use SMP edge.  nchiu
 *
 ****************************************************************/
static fbe_status_t 
fbe_eses_enclosure_send_get_element_list_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_eses_enclosure_t * eses_enclosure = NULL;
    fbe_sg_element_t  * sg_list = NULL; 
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_smp_operation_t * payload_smp_operation = NULL;
    fbe_payload_smp_get_element_list_data_t * payload_smp_get_element_list_data = NULL;
    fbe_u32_t i;
    fbe_status_t status;
    fbe_edal_status_t   edalStatus;
    fbe_sas_address_t expander_sas_address = FBE_SAS_ADDRESS_INVALID;
    fbe_u8_t phy_number = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u8_t dev_index = FBE_ENCLOSURE_VALUE_INVALID;    
    LARGE_INTEGER sas_addr;
    fbe_edal_general_comp_handle_t generalDataPtr = NULL;
    fbe_u8_t expander_elem_index = SES_ELEM_INDEX_NONE;
    fbe_u8_t phy_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u8_t expansion_port_entire_connector_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u8_t expansion_port_connector_id = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_bool_t devLoggedIn = FALSE;
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;

    eses_enclosure = (fbe_eses_enclosure_t *)context;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry  \n", __FUNCTION__ ); 

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s: NULL payload.  \n", __FUNCTION__ ); 

        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload_smp_operation = fbe_payload_ex_get_smp_operation(payload);
    if (payload_smp_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s: NULL smp operation.  \n", __FUNCTION__ ); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    if (sg_list == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s: NULL sg_list.  \n", __FUNCTION__ ); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload_smp_get_element_list_data = (fbe_payload_smp_get_element_list_data_t *)sg_list[0].address;
    
    /* in case the element_list is bad, let's bail out.
     * and fbe_sas_enclosure_send_get_element_list_completion will do 
     * the tracing.
     * We should only have # of drives + # of expansion + our own virtual phy.
     * If we have more than that many, it's a problem.
     */
    if (payload_smp_get_element_list_data->number_of_elements >
            (fbe_u32_t)(fbe_eses_enclosure_get_number_of_slots(eses_enclosure) + 
            fbe_eses_enclosure_get_number_of_expansion_ports(eses_enclosure) + 1) )
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "eses get element list, too many children (%d) for Enclosure.\n",
                                payload_smp_get_element_list_data->number_of_elements);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We need to clean up the stale sas addresses.
     */
    for(i = 0 ; i < fbe_eses_enclosure_get_number_of_slots(eses_enclosure); i ++)
    {
        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_DRIVE_LOGGED_IN,  // Attribute.
                                                    FBE_ENCL_DRIVE_SLOT,         // Component type.
                                                    i,     // Component index.
                                                    FALSE);
        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    for(i = 0 ; i < fbe_eses_enclosure_get_number_of_expansion_ports(eses_enclosure); i ++)
    {
        encl_status = fbe_eses_enclosure_get_connector_entire_connector_index(eses_enclosure, 
                                                i,
                                                &expansion_port_entire_connector_index);

        if(encl_status != FBE_ENCLOSURE_STATUS_OK)       
        {
            continue;  // we can ignore it if expander didn't describe the port
        }

        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_CONNECTOR_DEV_LOGGED_IN,  // Attribute.
                                                    FBE_ENCL_CONNECTOR,         // Component type.
                                                    expansion_port_entire_connector_index,     // Component index.
                                                    FALSE);

        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    for(i = 0 ; i < payload_smp_get_element_list_data->number_of_elements; i++)
    {
        /* Let's look if we got a SAS target */
        if((payload_smp_get_element_list_data->element_list[i].element_type == FBE_PAYLOAD_SMP_ELEMENT_TYPE_SSP) ||
           (payload_smp_get_element_list_data->element_list[i].element_type == FBE_PAYLOAD_SMP_ELEMENT_TYPE_STP) ||
           (payload_smp_get_element_list_data->element_list[i].element_type == FBE_PAYLOAD_SMP_ELEMENT_TYPE_EXPANDER)) 
        {

            phy_number = payload_smp_get_element_list_data->element_list[i].phy_number;
            
            /* 
             * We need to find the drive slot number or expansion port id. 
             * It needs to go through the following steps in order:
             * (1)Find the expander sas address that the get element list command was sent to.
             * (2)Find the expander element index from the expander sas address.
             * (3)Find the phy index using the expander element index and phy id (i.e. phy number).
             * (4)Find the drive slot number using the phy index, or the expansion port connector id
             *
             * If any step above failed, we could not find the drive slot number. We will trace it 
             * and continue with the next element.
             */

            /* (1)Find the expander sas address that the get element list command was sent to. */
            fbe_payload_smp_get_children_list_command_sas_address(payload_smp_operation, &expander_sas_address);

            /* (2)Find the expander element index from the expander sas address. */
            if(fbe_eses_enclosure_expander_sas_address_to_elem_index(eses_enclosure,
                 expander_sas_address, &expander_elem_index) != FBE_ENCLOSURE_STATUS_OK)
            {
                sas_addr.QuadPart = expander_sas_address;
                
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "eses get element list, invalid expander_sas_address 0x%8.8x_0x%8.8x.\n",
                                        sas_addr.HighPart, sas_addr.LowPart);
                
                continue;
            
            }

            /* (3)Find the phy index using the expander element index and phy id (i.e. phy number). */
            if(fbe_eses_enclosure_exp_elem_index_phy_id_to_phy_index(eses_enclosure, 
                     expander_elem_index, phy_number, &phy_index) != FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "eses get element list,, Failed to find phy index, expander_elem_index: %d, phy_id: %d.\n",
                                        expander_elem_index, phy_number);
                
                continue;
            }

            if (payload_smp_get_element_list_data->element_list[i].element_type == FBE_PAYLOAD_SMP_ELEMENT_TYPE_EXPANDER)
            {
                /* (4)Find the expansion port number using the phy index.*/
                if (fbe_eses_enclosure_phy_index_to_connector_index(eses_enclosure, 
                        phy_index, &dev_index) != FBE_ENCLOSURE_STATUS_OK)
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_WARNING,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "eses get element list, Failed to find connector index, phy_id: %d, phy_index: %d.\n",
                                            phy_number, phy_index);
                    
                    continue;
                }
                // get the connector id
                if (fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_CONNECTOR_ID,
                                                    FBE_ENCL_CONNECTOR,
                                                    dev_index,
                                                    &expansion_port_connector_id)
                            != FBE_ENCLOSURE_STATUS_OK)
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_WARNING,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "eses get element list, Failed to find connector id, phy_id: %d, dev_index: %d.\n",
                                            phy_number, dev_index);
                    
                    continue;
                }
                // Get the entire connector index of the connector.
                if (fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_CONTAINER_INDEX,
                                                    FBE_ENCL_CONNECTOR,
                                                    dev_index,
                                                    &expansion_port_entire_connector_index)
                            != FBE_ENCLOSURE_STATUS_OK)
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_WARNING,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "eses get element list, Failed to find whole connector index, phy_id: %d, dev_index: %d, connector id %d.\n",
                                            phy_number, dev_index, expansion_port_connector_id);
                    
                    continue;
                }
                encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_CONNECTOR_DEV_LOGGED_IN,  // Attribute.
                                                    FBE_ENCL_CONNECTOR,         // Component type.
                                                    expansion_port_entire_connector_index,     // Component index.
                                                    TRUE);
                if (encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                /* We need to refresh our status through ESES if SMP discovery indicates something changed.
                */
                if ((eses_enclosure->expansion_port_info[expansion_port_connector_id].sas_address   != 
                        payload_smp_get_element_list_data->element_list[i].address.sas_address) ||
                    (eses_enclosure->expansion_port_info[expansion_port_connector_id].generation_code != 
                        payload_smp_get_element_list_data->element_list[i].generation_code) ||
                    (eses_enclosure->expansion_port_info[expansion_port_connector_id].element_type != 
                        (fbe_payload_discovery_element_type_t)payload_smp_get_element_list_data->element_list[i].element_type) ||
                    (eses_enclosure->expansion_port_info[expansion_port_connector_id].chain_depth != 
                        payload_smp_get_element_list_data->element_list[i].enclosure_chain_depth))    
                {
                    status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                (fbe_base_object_t*)eses_enclosure, 
                                FBE_ESES_ENCLOSURE_LIFECYCLE_COND_STATUS_UNKNOWN);

                    if (status != FBE_STATUS_OK) 
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                FBE_TRACE_LEVEL_ERROR,
                                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                "%s, can't set STATUS_UNKNOWN condition, status: 0x%x.\n",
                                                __FUNCTION__, status);

                        return FBE_STATUS_OK;
                    }
                    /* We will assume that we found new element or lost some old element.
                    In this case we should triger discovery update condition to take care of discovery edges.
                    */

                    status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                    (fbe_base_object_t*)eses_enclosure, 
                                    FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE);

                    if (status != FBE_STATUS_OK) {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                FBE_TRACE_LEVEL_ERROR,
                                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                "%s can't set discovery update condition, status: 0x%X",
                                                __FUNCTION__,  status);
                        
                    }
                }

                eses_enclosure->expansion_port_info[expansion_port_connector_id].sas_address = 
                            payload_smp_get_element_list_data->element_list[i].address.sas_address;
                eses_enclosure->expansion_port_info[expansion_port_connector_id].generation_code = 
                            payload_smp_get_element_list_data->element_list[i].generation_code; 
                eses_enclosure->expansion_port_info[expansion_port_connector_id].element_type = 
                            payload_smp_get_element_list_data->element_list[i].element_type; 
                eses_enclosure->expansion_port_info[expansion_port_connector_id].chain_depth = 
                            payload_smp_get_element_list_data->element_list[i].enclosure_chain_depth; 

            }
            else
            {
                /* (4)Find the drive slot number (index) using the phy index.*/
                if(fbe_eses_enclosure_phy_index_to_drive_slot_index(eses_enclosure, 
                        phy_index, &dev_index) != FBE_ENCLOSURE_STATUS_OK)
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_WARNING,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "eses get element list, Failed to find drive slot num, phy_id: %d, phy_index: %d.\n",
                                            phy_number, phy_index);
                
                continue;
            }

            /* drive log in */
            status = fbe_base_enclosure_access_specific_component((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_DRIVE_SLOT,
                                                            dev_index,
                                                        &generalDataPtr);
            edalStatus = eses_enclosure_access_setBool(generalDataPtr,
                                                        FBE_ENCL_DRIVE_LOGGED_IN,
                                                        FBE_ENCL_DRIVE_SLOT,
                                                        TRUE);
            if (edalStatus != FBE_EDAL_STATUS_OK)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }

            edalStatus = eses_enclosure_access_setU8(generalDataPtr,
                                                      FBE_ENCL_COMP_TYPE,
                                                      FBE_ENCL_DRIVE_SLOT,
                                                      (fbe_u8_t) payload_smp_get_element_list_data->element_list[i].element_type);
            if (edalStatus != FBE_EDAL_STATUS_OK)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }

                /* We need to refresh our status through ESES if SMP discovery indicates something changed */
                    if ((eses_enclosure->drive_info[dev_index].sas_address != 
                            payload_smp_get_element_list_data->element_list[i].address.sas_address) ||
                        (eses_enclosure->drive_info[dev_index].generation_code != 
                            payload_smp_get_element_list_data->element_list[i].generation_code) ||
                        (eses_enclosure->drive_info[dev_index].element_type != 
                        (fbe_payload_discovery_element_type_t)payload_smp_get_element_list_data->element_list[i].element_type))
                {
                    status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                (fbe_base_object_t*)eses_enclosure, 
                                FBE_ESES_ENCLOSURE_LIFECYCLE_COND_STATUS_UNKNOWN);

                    if (status != FBE_STATUS_OK) 
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                FBE_TRACE_LEVEL_ERROR,
                                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                "%s, can't set STATUS_UNKNOWN condition, status: 0x%x.\n",
                                                __FUNCTION__, status);

                        return FBE_STATUS_OK;
                    }
                    /* We will assume that we found new element or lost some old element.
                    In this case we should triger discovery update condition to take care of discovery edges.
                    */

                    status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                    (fbe_base_object_t*)eses_enclosure, 
                                    FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE);

                        if (status != FBE_STATUS_OK) {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                FBE_TRACE_LEVEL_ERROR,
                                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                "%s can't set discovery update condition, status: 0x%X",
                                                __FUNCTION__,  status);
                            
                    }
                }

            /* Save the new information */
                eses_enclosure->drive_info[dev_index].sas_address = payload_smp_get_element_list_data->element_list[i].address.sas_address;
                eses_enclosure->drive_info[dev_index].generation_code = payload_smp_get_element_list_data->element_list[i].generation_code; 
                eses_enclosure->drive_info[dev_index].element_type = payload_smp_get_element_list_data->element_list[i].element_type; 
            }
            
            sas_addr.QuadPart = payload_smp_get_element_list_data->element_list[i].address.sas_address;
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "eses get element list, element type %d, index %d, slot %d sas_address 0x%8.8x_0x%8.8x.\n",
                                    payload_smp_get_element_list_data->element_list[i].element_type,
                                    i, dev_index, sas_addr.HighPart, sas_addr.LowPart);
                
        }
        else if(payload_smp_get_element_list_data->element_list[i].element_type != FBE_PAYLOAD_SMP_ELEMENT_TYPE_VIRTUAL)
        {
            /* virtual port is picked up by sas enclosure */
            sas_addr.QuadPart = payload_smp_get_element_list_data->element_list[i].address.sas_address;
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "ESES index %d, unknown type %d, sas_address 0x%8.8x_0x%8.8x.\n",
                                    i, 
                                    payload_smp_get_element_list_data->element_list[i].element_type,
                                    sas_addr.HighPart,
                                    sas_addr.LowPart);
            
        }
    }
    
    /* We need to clean up stale data */
    for(i = 0 ; i < fbe_eses_enclosure_get_number_of_slots(eses_enclosure); i ++)
    {
        /* check drive log in */
        status = fbe_base_enclosure_access_specific_component((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_DRIVE_SLOT,
                                                    i,
                                                    &generalDataPtr);
        edalStatus = eses_enclosure_access_getBool(generalDataPtr,
                                                    FBE_ENCL_DRIVE_LOGGED_IN,
                                                    FBE_ENCL_DRIVE_SLOT,
                                                    &devLoggedIn);
        if ((edalStatus != FBE_EDAL_STATUS_OK)||(!devLoggedIn))
        {
            /* We need to refresh our status through ESES if SMP discovery indicates something changed */
            if (eses_enclosure->drive_info[i].sas_address != FBE_SAS_ADDRESS_INVALID)
            {
                // Update the component state change when drive state chnages (either logs in or out).
                edalStatus = eses_enclosure_access_setBool(generalDataPtr,
                                                    FBE_ENCL_COMP_STATE_CHANGE,
                                                    FBE_ENCL_DRIVE_SLOT,
                                                    TRUE);

                if (edalStatus != FBE_EDAL_STATUS_OK)
                {
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                            (fbe_base_object_t*)eses_enclosure, 
                            FBE_ESES_ENCLOSURE_LIFECYCLE_COND_STATUS_UNKNOWN);

                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "%s, can't set STATUS_UNKNOWN condition, status: 0x%x.\n",
                                            __FUNCTION__, status);

                    return FBE_STATUS_OK;
                }
                /* We will assume that we found new element or lost some old element.
                In this case we should triger discovery update condition to take care of discovery edges.
                */

                status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                (fbe_base_object_t*)eses_enclosure, 
                FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE);

                if (status != FBE_STATUS_OK) {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "%s can't set discovery update condition, status: 0x%X",
                                            __FUNCTION__,  status);
        
                }
            }
            eses_enclosure->drive_info[i].sas_address = FBE_SAS_ADDRESS_INVALID;
            eses_enclosure->drive_info[i].generation_code = 0;
            eses_enclosure->drive_info[i].element_type = FBE_PAYLOAD_SMP_ELEMENT_TYPE_INVALID;
        }
    }
    for(i = 0 ; i < fbe_eses_enclosure_get_number_of_expansion_ports(eses_enclosure); i ++)
    {
        encl_status = fbe_eses_enclosure_get_connector_entire_connector_index(eses_enclosure, 
                                                i,
                                                &expansion_port_entire_connector_index);

        if(encl_status != FBE_ENCLOSURE_STATUS_OK)       
        {
            continue;  // we can ignore it if expander didn't describe the port
        }

        encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_CONNECTOR_DEV_LOGGED_IN,  // Attribute.
                                                    FBE_ENCL_CONNECTOR,         // Component type.
                                                    expansion_port_entire_connector_index,     // Component index.
                                                    &devLoggedIn);

        if ((encl_status != FBE_ENCLOSURE_STATUS_OK) || (!devLoggedIn))
        {
            /* We need to refresh our status through ESES if SMP discovery indicates something changed */
            if (eses_enclosure->expansion_port_info[i].sas_address != FBE_SAS_ADDRESS_INVALID)
            {
                status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                            (fbe_base_object_t*)eses_enclosure, 
                            FBE_ESES_ENCLOSURE_LIFECYCLE_COND_STATUS_UNKNOWN);

                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "%s, can't set STATUS_UNKNOWN condition, status: 0x%x.\n",
                                            __FUNCTION__, status);

                    return FBE_STATUS_OK;
                }
                /* We will assume that we found new element or lost some old element.
                In this case we should triger discovery update condition to take care of discovery edges.
                */

                status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                (fbe_base_object_t*)eses_enclosure, 
                                FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE);

                if (status != FBE_STATUS_OK) {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "%s can't set discovery update condition, status: 0x%X",
                                            __FUNCTION__,  status);
                    
                }
            }
            eses_enclosure->expansion_port_info[i].sas_address = FBE_SAS_ADDRESS_INVALID;
            eses_enclosure->expansion_port_info[i].generation_code = 0;
            eses_enclosure->expansion_port_info[i].element_type = FBE_PAYLOAD_SMP_ELEMENT_TYPE_INVALID;
            eses_enclosure->expansion_port_info[i].chain_depth = 0;
        }
    }

    return FBE_STATUS_OK;
}

/****************************************************************
 * fbe_eses_enclosure_event_entry()
 ****************************************************************
 * DESCRIPTION:
 *  This function handles events to the ESES enclosure object.
 *
 * PARAMETERS:
 *  object_handle - The object receiving the event
 *  event_type - The type of event.
 *  event_context - The context for the event.
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  8/5/08 - updated. bphilbin
 *
 ****************************************************************/
fbe_status_t 
fbe_eses_enclosure_event_entry(fbe_object_handle_t object_handle, 
                                    fbe_event_type_t event_type, 
                                    fbe_event_context_t event_context)
{
    fbe_eses_enclosure_t * eses_enclosure = NULL;
    fbe_status_t status;

    eses_enclosure = (fbe_eses_enclosure_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry, event_type %d context 0x%p\n",
                            __FUNCTION__, event_type, event_context);
    /*
     * This class did not handle the event, just go to the superclass
     * to handle it.
     */        
    status = fbe_sas_enclosure_event_entry(object_handle, event_type, event_context);

    return status;

}


/****************************************************************
 * fbe_eses_enclosure_discovery_transport_entry()
 ****************************************************************
 * DESCRIPTION:
 *  This function handles incoming requests on our discovery edge.
 *  This is used to answer queries from the drive and other enclosure
 *  objects.
 *
 * PARAMETERS:
 *  p_object - our object context
 *  packet - request packet
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  7/31/08 - updated. bphilbin
 *
 ****************************************************************/
fbe_status_t 
fbe_eses_enclosure_discovery_transport_entry(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_eses_enclosure_t * eses_enclosure = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_payload_discovery_opcode_t discovery_opcode;
    fbe_status_t status;

    eses_enclosure = (fbe_eses_enclosure_t *)base_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry \n", __FUNCTION__);
        
    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
    fbe_payload_discovery_get_opcode(payload_discovery_operation, &discovery_opcode);

     switch(discovery_opcode){
        case FBE_PAYLOAD_DISCOVERY_OPCODE_GET_PROTOCOL_ADDRESS:
            status = eses_enclosure_discovery_transport_get_protocol_address(eses_enclosure, packet);
            break;
        case FBE_PAYLOAD_DISCOVERY_OPCODE_GET_SLOT_INFO:
            status = eses_enclosure_discovery_transport_get_slot_info(eses_enclosure, packet);
            break;
        case FBE_PAYLOAD_DISCOVERY_OPCODE_GET_SERVER_INFO:
            status = eses_enclosure_discovery_transport_get_server_info(eses_enclosure, packet);
            break;            
        case FBE_PAYLOAD_DISCOVERY_OPCODE_NOTIFY_FW_ACTIVATION_STATUS:
            status = eses_enclosure_discovery_transport_record_fw_activation_status(eses_enclosure, packet);
            break;
        case FBE_PAYLOAD_DISCOVERY_OPCODE_CHECK_DUP_ENCL_SN:
            status = fbe_eses_enclosure_check_duplicate_ESN(eses_enclosure, packet);
            break;
        case FBE_PAYLOAD_DISCOVERY_OPCODE_POWER_ON:
        case FBE_PAYLOAD_DISCOVERY_OPCODE_POWER_SAVE_ON:
        case FBE_PAYLOAD_DISCOVERY_OPCODE_POWER_SAVE_ON_PASSIVE:
            status = fbe_eses_enclosure_discovery_transport_drive_power_ctrl(eses_enclosure, packet);
            break;

        case FBE_PAYLOAD_DISCOVERY_OPCODE_POWER_CYCLE:
            status = fbe_eses_enclosure_discovery_transport_drive_power_cycle(eses_enclosure, packet);
            break;

        case FBE_PAYLOAD_DISCOVERY_OPCODE_UNBYPASS:
            status = fbe_eses_enclosure_discoveryUnbypass(eses_enclosure, packet);
            break;

        case FBE_PAYLOAD_DISCOVERY_OPCODE_RESET_SLOT:
            status = fbe_eses_enclosure_discovery_transport_reset_drive_slot(eses_enclosure, packet);
            break;
        
        default:
            status = fbe_sas_enclosure_discovery_transport_entry((fbe_sas_enclosure_t *)eses_enclosure, packet);
            break;
    }

    fbe_base_enclosure_record_discovery_transport_opcode((fbe_base_enclosure_t *)eses_enclosure,discovery_opcode);

    return status;
}

static fbe_status_t 
eses_enclosure_discovery_transport_get_protocol_address(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_payload_discovery_get_protocol_address_data_t * payload_discovery_get_protocol_address_data = NULL;
    fbe_edge_index_t server_index;
    fbe_status_t status;
    fbe_enclosure_number_t encl_number = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_lifecycle_state_t lifecycle_state;
    fbe_u8_t connector_id;

    /* Check for the lifecycle state before starting any operation in this function. 
     * If the Object is not in READY state, fail the packet to avoid accessing 
     * protocol address data frequently. 
     */
    status = fbe_lifecycle_get_state(&fbe_eses_enclosure_lifecycle_const,
                            (fbe_base_object_t*)eses_enclosure,
                            &lifecycle_state);

    if((status == FBE_STATUS_OK) && (lifecycle_state != FBE_LIFECYCLE_STATE_READY))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                            "Enclosure is not in READY state. Current state: 0x%x\n", lifecycle_state);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    payload_discovery_get_protocol_address_data = (fbe_payload_discovery_get_protocol_address_data_t *)sg_list[0].address;

    /* As a for now we have one client only with this particular id */
    status = fbe_base_discovering_get_server_index_by_client_id((fbe_base_discovering_t *) eses_enclosure, 
                                                        payload_discovery_operation->command.common_command.client_id,
                                                        &server_index);

    if(status != FBE_STATUS_OK) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "fbe_base_discovering_get_server_index_by_client_id fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Check if server_index is an index of the drive */
    if(server_index < fbe_eses_enclosure_get_number_of_slots(eses_enclosure)) {
        /* TO DO: these expansion port info will be moved to discovery edge */
        payload_discovery_get_protocol_address_data->address.sas_address = eses_enclosure->drive_info[server_index].sas_address;
        payload_discovery_get_protocol_address_data->generation_code = eses_enclosure->drive_info[server_index].generation_code; 
        payload_discovery_get_protocol_address_data->element_type = eses_enclosure->drive_info[server_index].element_type; 

        fbe_base_enclosure_get_enclosure_number ((fbe_base_enclosure_t *)eses_enclosure, &encl_number);
        payload_discovery_get_protocol_address_data->chain_depth = encl_number; 
        status = FBE_STATUS_OK;
    } else {
        connector_id = server_index - fbe_eses_enclosure_get_first_expansion_port_index(eses_enclosure);
        if(connector_id < fbe_eses_enclosure_get_number_of_expansion_ports(eses_enclosure) ) {
            payload_discovery_get_protocol_address_data->address.sas_address = 
                            eses_enclosure->expansion_port_info[connector_id].sas_address;
            payload_discovery_get_protocol_address_data->generation_code = 
                            eses_enclosure->expansion_port_info[connector_id].generation_code; 
            payload_discovery_get_protocol_address_data->element_type = 
                            eses_enclosure->expansion_port_info[connector_id].element_type; 
            payload_discovery_get_protocol_address_data->chain_depth = 
                            eses_enclosure->expansion_port_info[connector_id].chain_depth; 
        status = FBE_STATUS_OK;
    } else {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s Invalid server_index 0x%X",
                                __FUNCTION__, server_index);

        status = FBE_STATUS_GENERIC_FAILURE;
    }
    }
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}


/****************************************************************
 * eses_enclosure_discovery_transport_get_slot_info()
 ****************************************************************
 * DESCRIPTION:
 *  This function handles incoming requests on our discovery to get
 *  information about a particular slot.
 *
 * PARAMETERS:
 *  p_object - our object context
 *  packet - request packet
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  7/31/08 - created. bphilbin
 *
 ****************************************************************/
static fbe_status_t
eses_enclosure_discovery_transport_get_slot_info(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_edge_index_t server_index;
    fbe_status_t status, savedStatus = FBE_STATUS_OK;
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t     encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_object_id_t client_id;
    fbe_payload_discovery_get_slot_info_data_t * payload_discovery_get_slot_info_data;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_edal_general_comp_handle_t generalDataPtr;
    // temp
    fbe_bool_t powered_off = FALSE;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);

    /* Find out which slot they want some info about */
    client_id = payload_discovery_operation->command.common_command.client_id;

    /* Convert the client ID to a server_index to a slot */
    status = fbe_base_discovering_get_server_index_by_client_id((fbe_base_discovering_t *) eses_enclosure, 
                                                        payload_discovery_operation->command.common_command.client_id,
                                                        &server_index);

    if(status != FBE_STATUS_OK) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "fbe_base_discovering_get_server_index_by_client_id fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
            
    status = fbe_base_enclosure_access_specific_component((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_DRIVE_SLOT,
                                                        server_index,
                                                        &generalDataPtr);

    /*  We need to put the data into the sg_list inside the payload, grab the pointer.  */
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    payload_discovery_get_slot_info_data = (fbe_payload_discovery_get_slot_info_data_t *) sg_list[0].address;

    /* Fill in the slot info (inserted/powered off/bypassed/lcc_side_id) in the struct */
    edalStatus = eses_enclosure_access_getBool(generalDataPtr,
                                                FBE_ENCL_COMP_INSERTED,
                                                FBE_ENCL_DRIVE_SLOT,
                                                &(payload_discovery_get_slot_info_data->inserted));
    if (edalStatus != FBE_EDAL_STATUS_OK)
    {
        savedStatus = FBE_STATUS_GENERIC_FAILURE;
    }
    edalStatus = eses_enclosure_access_getBool(generalDataPtr,
                                                FBE_ENCL_COMP_POWERED_OFF,
                                                FBE_ENCL_DRIVE_SLOT,
                                                &powered_off);

    payload_discovery_get_slot_info_data->powered_off = powered_off;

    if (edalStatus != FBE_EDAL_STATUS_OK)
    {
        savedStatus = FBE_STATUS_GENERIC_FAILURE;
    }
    edalStatus = eses_enclosure_access_getBool(generalDataPtr,
                                                FBE_ENCL_DRIVE_BYPASSED,
                                                FBE_ENCL_DRIVE_SLOT,
                                                &(payload_discovery_get_slot_info_data->bypassed));
    if (edalStatus != FBE_EDAL_STATUS_OK)
    {
        savedStatus = FBE_STATUS_GENERIC_FAILURE;
    }

    encl_status = fbe_eses_enclosure_get_local_lcc_side_id(eses_enclosure, 
                                      &(payload_discovery_get_slot_info_data->local_lcc_side_id));

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        savedStatus = FBE_STATUS_GENERIC_FAILURE;
    }  

    status = savedStatus;   

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status; 

}

/****************************************************************
 * eses_enclosure_discovery_transport_record_fw_activation_status()
 ****************************************************************
 * DESCRIPTION:
 *  This function records firmware activation of attached enclosure
 *
 * PARAMETERS:
 *  eses_enclosure - our object context
 *  packet - request packet
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  15-Nov-2009 NCHIU - created. 
 *
 ****************************************************************/
static fbe_status_t 
eses_enclosure_discovery_transport_record_fw_activation_status(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet)
{
    fbe_enclosure_status_t   encl_status;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_status_t status = FBE_STATUS_OK;     
    fbe_u8_t            port;
    fbe_u8_t            connector_id[FBE_ENCLOSURE_MAX_EXPANSION_PORT_COUNT];

    /**********
     * BEGINE
     **********/   
    
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry.\n", 
                            __FUNCTION__);
    
    payload = fbe_transport_get_payload_ex(packet);

    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);

    // init the array to invalid
    for(port = 0; port < FBE_ENCLOSURE_MAX_EXPANSION_PORT_COUNT; port++){connector_id[port] = FBE_ENCLOSURE_VALUE_INVALID;}
    // get a list of connector_ids
    encl_status = fbe_eses_enclosure_get_expansion_port_connector_id(eses_enclosure, &connector_id[0], FBE_ENCLOSURE_MAX_EXPANSION_PORT_COUNT);
    if (encl_status != FBE_ENCLOSURE_STATUS_OK)
    {


        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for(port = 0; port < FBE_ENCLOSURE_MAX_EXPANSION_PORT_COUNT; port++)
    {
        if(connector_id[port] != FBE_ENCLOSURE_VALUE_INVALID)
        {
            if (connector_id[port] >= fbe_eses_enclosure_get_number_of_expansion_ports(eses_enclosure)) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "record_fw_activation_status, connector id %d of max %d.\n",
                                        connector_id[port], fbe_eses_enclosure_get_number_of_expansion_ports(eses_enclosure));
        
                fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            eses_enclosure->expansion_port_info[connector_id[port]].attached_encl_fw_activating = 
                            payload_discovery_operation->command.notify_fw_status_command.activation_in_progress;
        }
    }

    fbe_payload_discovery_set_status(payload_discovery_operation,FBE_PAYLOAD_DISCOVERY_STATUS_OK);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

/****************************************************************
 * eses_enclosure_discovery_transport_get_server_info()
 ****************************************************************
 * DESCRIPTION:
 *  This function handles incoming requests on our discovery to get
 *  the server address.
 *
 * PARAMETERS:
 *  eses_enclosure - our object context
 *  packet - request packet
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  15-Aug-2008 PHE - created. 
 *
 ****************************************************************/
static fbe_status_t 
eses_enclosure_discovery_transport_get_server_info(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_payload_discovery_get_server_info_data_t * payload_discovery_get_server_info_data = NULL;
    fbe_sas_address_t expander_sas_address = FBE_SAS_ADDRESS_INVALID;
    fbe_u8_t expansion_port_entire_connector_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_status_t status = FBE_STATUS_OK;     
    fbe_enclosure_status_t   enclStatus;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_edge_index_t server_index = 0;
    fbe_u8_t expansion_port_id = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u8_t connectorType = FBE_ESES_CONN_TYPE_UNKNOWN;

    /**********
     * BEGINE
     **********/   
    
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry. \n", 
                            __FUNCTION__);
    
    payload = fbe_transport_get_payload_ex(packet);

    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    payload_discovery_get_server_info_data = (fbe_payload_discovery_get_server_info_data_t *)sg_list[0].address;
   
    status = fbe_base_discovering_get_server_index_by_client_id((fbe_base_discovering_t *) eses_enclosure, 
                                    payload_discovery_operation->command.get_server_info_command.client_id,
                                    &server_index);

    if(status != FBE_STATUS_OK) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "get_server_info, get_server_index_by_client_id fail, status: 0x%x.\n",
                                status);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    expansion_port_id = server_index - fbe_eses_enclosure_get_first_expansion_port_index(eses_enclosure);
    if (expansion_port_id >= fbe_eses_enclosure_get_number_of_expansion_ports(eses_enclosure)) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "get_server_info, invalid server index %d, connector id %d.\n",
                                server_index, expansion_port_id);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    enclStatus = fbe_eses_enclosure_get_connector_type(eses_enclosure, 
                                                       expansion_port_id, 
                                                       FBE_TRUE, /*local connector*/ 
                                                       &connectorType);

    if(enclStatus != FBE_ENCLOSURE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "get_server_info, can not get connector type for connector id %d.\n",
                                expansion_port_id);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(fbe_eses_enclosure_get_expansion_port_entire_connector_index(eses_enclosure, 
                                    expansion_port_id,
                                    &expansion_port_entire_connector_index)
        == FBE_ENCLOSURE_STATUS_OK)
    {
        enclStatus = fbe_base_enclosure_edal_getU64_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_CONNECTOR_EXP_SAS_ADDRESS,  // Attribute
                                                FBE_ENCL_CONNECTOR,    // Component type
                                                expansion_port_entire_connector_index,  //Index of the component 
                                                &expander_sas_address); // The pointer to the return value
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    /* status will be shown in packet status */
    payload_discovery_get_server_info_data->address.sas_address = expander_sas_address;
    fbe_base_enclosure_get_port_number((fbe_base_enclosure_t *)eses_enclosure, 
                                        &(payload_discovery_get_server_info_data->port_number));

    fbe_base_enclosure_get_enclosure_number((fbe_base_enclosure_t *)eses_enclosure, 
                                        &(payload_discovery_get_server_info_data->position));

    if(connectorType != FBE_ESES_CONN_TYPE_INTERNAL_CONN) 
    {
        // downstream enclosure
        payload_discovery_get_server_info_data->position++;
        payload_discovery_get_server_info_data->component_id = 0;
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "get_server_info, downstream enclosure connector type 0x%x.\n", connectorType);
    }
    else
    {
        // internal edge expander
        /* internal connection reports connector id as component id */
        payload_discovery_get_server_info_data->component_id = expansion_port_id;
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "get_server_info, internal connector component_id %d.\n", expansion_port_id);
    }
    
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_send_scsi_cmd()
***************************************************************************
* @brief
*   This function calls the corresponding function to sets up the SCSI command 
*   based on the control operation opcode and then sends down the SCSI command. 
*
* @param   eses_enclosure - The pointer to the eses enclosure.
* @param   packet - The pointer to the fbe_packet_t.
*
* @return   fbe_status_t.
*        FBE_STATUS_OK  - no error found.
*        otherwise - error is found.
*
* NOTES
*
* HISTORY
*   18-Aug-2008 GB - Created.
*   30-Sept-2008 NC - use payload
*   09-Dec-2008 PHE - Used the control operation. 
***************************************************************************/
fbe_status_t fbe_eses_enclosure_send_scsi_cmd(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t *packet)
{
    fbe_u8_t            *buffer = NULL;
    fbe_sg_element_t    *sg_list = NULL;
    fbe_payload_ex_t       *payload = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_payload_control_operation_opcode_t opcode;
    fbe_payload_cdb_operation_t *payload_cdb_operation = NULL;
    fbe_status_t        status = FBE_STATUS_OK;

 
    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry \n", __FUNCTION__ );

    /* Set the packet status to FBE_STATUS_OK to start with. */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);

    /* Make control operation allocated in monitor or usurper as the current operation. */
    status = fbe_payload_ex_increment_control_operation_level(payload);

    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }
    /* Get the current control operation which is allocated in either monitor or usurper.*/  
    payload_control_operation = fbe_payload_ex_get_control_operation(payload);

    if(payload_control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_WARNING,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, NULL control operation, packet: %p.\n",
                                __FUNCTION__,  packet);

        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PACKET_FAILED);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    /* Initialize the output values. */
    fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_LAST);
    fbe_payload_control_set_status_qualifier(payload_control_operation, FBE_ENCLOSURE_STATUS_INVALID);

    /* Get the control operation opcode. */
    fbe_payload_control_get_opcode(payload_control_operation, &opcode);

    /* Allocate the cdb operation, cdb operation becomes the next operation. */
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);

    if(payload_cdb_operation == NULL) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_payload_ex_allocate_cdb_operation fail, opcode: 0x%x.\n",
                                __FUNCTION__,  opcode);

        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PACKET_FAILED);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }
    
    /* We need to increment first and then check the outstanding scsi request count.
     * If we check the outstanding scsi request count first, it is possible that
     * too threads increment the count and send down two requests.
     */
    fbe_eses_enclosure_increment_outstanding_scsi_request_count(eses_enclosure);

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "sendScsiCmd,%s(0x%x),Outstanding request count %lld, max %d.\n",
                            fbe_eses_enclosure_print_scsi_opcode(opcode), 
                            opcode,
                            eses_enclosure->outstanding_scsi_request_count,
                            eses_enclosure->outstanding_scsi_request_max);
    
    status = fbe_eses_enclosure_check_outstanding_scsi_request_count(eses_enclosure);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "sendScsiCmd,%s(0x%x),Exceeds max requests, count %lld, max %d.\n",
                                fbe_eses_enclosure_print_scsi_opcode(opcode), 
                                opcode,
                                eses_enclosure->outstanding_scsi_request_count,
                                eses_enclosure->outstanding_scsi_request_max);
        
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "sendScsiCmd,outstanding request: %s(0x%x).\n",
                                fbe_eses_enclosure_print_scsi_opcode(eses_enclosure->outstanding_scsi_request_opcode), 
                                eses_enclosure->outstanding_scsi_request_opcode);

        fbe_eses_enclosure_decrement_outstanding_scsi_request_count(eses_enclosure);
        /* Set the control operation status and status qualifier. */
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_payload_control_set_status_qualifier(payload_control_operation, FBE_ENCLOSURE_STATUS_BUSY);
        /* Complete the packet.*/
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    // Allocate one chunk of memory.
    buffer = fbe_transport_allocate_buffer(); 
    if (buffer == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_transport_allocate_buffer failed, opcode: 0x%x.\n",
                                __FUNCTION__,  opcode);
       
        /*  
        * Set the previous control operation status to FBE_PAYLOAD_CONTROL_STATUS_FAILURE
        * and clean up the current cdb operation and allocated buffer if any. 
        */
        fbe_eses_enclosure_decrement_outstanding_scsi_request_count(eses_enclosure);
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_MEM_ALLOC_FAILED);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    // zero the allocated memory
    // Temp solution.-- PHE
    fbe_zero_memory(buffer, FBE_MEMORY_CHUNK_SIZE);
    sg_list = (fbe_sg_element_t *)buffer;
    // Set sg_list for the payload and initialize the sg_count to 1. It will be updated as needed.
    fbe_payload_ex_set_sg_list(payload, sg_list, 1);
        
    switch(opcode)
    {
        /*
        * We have to pass payload_cdb_opertion as one of the paremters. 
        * we are not able to get the payload_cdb_operation from payload
        * because it is the next opertion not the current operation.
        */
        case FBE_ESES_CTRL_OPCODE_GET_INQUIRY_DATA:
        case FBE_ESES_CTRL_OPCODE_VALIDATE_IDENTITY:
            status = fbe_eses_enclosure_setup_inquiry_cmd(eses_enclosure, packet, payload_cdb_operation);
            break;

        case FBE_ESES_CTRL_OPCODE_GET_CONFIGURATION: 
        case FBE_ESES_CTRL_OPCODE_GET_SAS_ENCL_TYPE:
        case FBE_ESES_CTRL_OPCODE_GET_ENCL_STATUS:
        case FBE_ESES_CTRL_OPCODE_GET_ADDITIONAL_STATUS: 
        case FBE_ESES_CTRL_OPCODE_GET_EMC_SPECIFIC_STATUS:
        case FBE_ESES_CTRL_OPCODE_GET_TRACE_BUF_INFO_STATUS:
        case FBE_ESES_CTRL_OPCODE_GET_DOWNLOAD_STATUS:
        case FBE_ESES_CTRL_OPCODE_EMC_STATISTICS_STATUS_CMD:
        case FBE_ESES_CTRL_OPCODE_GET_EMC_STATISTICS_STATUS:
        case FBE_ESES_CTRL_OPCODE_THRESHOLD_IN_CMD:
        case FBE_ESES_CTRL_OPCODE_GET_TUNNEL_CMD_STATUS:
            status = fbe_eses_enclosure_setup_receive_diag_cmd(eses_enclosure, packet, payload_cdb_operation);
            break;

        case FBE_ESES_CTRL_OPCODE_DOWNLOAD_FIRMWARE:
        case FBE_ESES_CTRL_OPCODE_SET_EMC_SPECIFIC_CTRL: 
        case FBE_ESES_CTRL_OPCODE_SET_TRACE_BUF_INFO_CTRL: 
        case FBE_ESES_CTRL_OPCODE_SET_ENCL_CTRL: 
        case FBE_ESES_CTRL_OPCODE_STRING_OUT_CMD:
        case FBE_ESES_CTRL_OPCODE_THRESHOLD_OUT_CMD:    
        case FBE_ESES_CTRL_OPCODE_TUNNEL_GET_CONFIGURATION:
        case FBE_ESES_CTRL_OPCODE_TUNNEL_DOWNLOAD_FIRMWARE:
        case FBE_ESES_CTRL_OPCODE_TUNNEL_GET_DOWNLOAD_STATUS:
            status = fbe_eses_enclosure_setup_send_diag_cmd(eses_enclosure, packet, payload_cdb_operation);
            break;

        case FBE_ESES_CTRL_OPCODE_MODE_SENSE:            
            status = fbe_eses_enclosure_setup_mode_sense_cmd(eses_enclosure, packet, payload_cdb_operation);
            break;

        case FBE_ESES_CTRL_OPCODE_MODE_SELECT:
            status = fbe_eses_enclosure_setup_mode_select_cmd(eses_enclosure, packet, payload_cdb_operation);
            break;

        case FBE_ESES_CTRL_OPCODE_READ_BUF:
        case FBE_ESES_CTRL_OPCODE_READ_RESUME:
            status = fbe_eses_enclosure_setup_read_buf_cmd(eses_enclosure, packet, payload_cdb_operation);
            break;

        case FBE_ESES_CTRL_OPCODE_RAW_RCV_DIAG:
            status = fbe_eses_enclosure_setup_raw_rcv_diag_cmd(eses_enclosure, 
                                                               packet, 
                                                               payload_cdb_operation);
            break;

        case FBE_ESES_CTRL_OPCODE_RAW_INQUIRY:
            status = fbe_eses_enclosure_setup_raw_inquiry_cmd(eses_enclosure, packet, payload_cdb_operation);
            break; 

        case FBE_ESES_CTRL_OPCODE_WRITE_RESUME:
            status = fbe_eses_enclosure_setup_write_resume_cmd(eses_enclosure, packet, payload_cdb_operation);
            break;

        case FBE_ESES_CTRL_OPCODE_SPS_IN_BUF_CMD:
            status = fbe_eses_enclosure_setup_sps_in_buf_cmd(eses_enclosure, packet, payload_cdb_operation);
            break;

        case FBE_ESES_CTRL_OPCODE_SPS_EEPROM_CMD:
            status = fbe_eses_enclosure_setup_sps_eeprom_cmd(eses_enclosure, packet, payload_cdb_operation);
            break;

        case FBE_ESES_CTRL_OPCODE_GET_RP_SIZE:
            status = fbe_eses_enclosure_setup_get_rp_size_cmd(eses_enclosure, packet, payload_cdb_operation);
            break;

        default:
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s,  invalid page opcode: 0x%x. Fail request\n",
                                    __FUNCTION__,  payload_control_operation->opcode);
            
            /* since we don't know how to handle the completion, we should
             * fail the request here, so that we don't accidentally have
             * the caller wait for the response forever.
             */      
            fbe_eses_enclosure_decrement_outstanding_scsi_request_count(eses_enclosure);
            fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_CTRL_CODE_UNSUPPORTED);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
    }// End of - switch(opcode)

    if(status == FBE_STATUS_OK)
    {
        fbe_eses_enclosure_set_outstanding_scsi_request_opcode(eses_enclosure, opcode);

        /* setup completion routine */
        status = fbe_transport_set_completion_function(packet, 
                                            fbe_eses_enclosure_send_scsi_cmd_completion, 
                                            eses_enclosure);

        /* Send the command via the ssp edge. It will make the cdb operation as the current operation. 
         * fbe_sas_enclosure_send_ssp_functional_packet calls fbe_ssp_transport_send_functional_packet which
         * calls fbe_payload_ex_increment_cdb_operation_level to make the cdb operation the current operation.
         */
        status = fbe_sas_enclosure_send_ssp_functional_packet((fbe_sas_enclosure_t *)eses_enclosure, packet);     
    }
    else
    {
        fbe_eses_enclosure_decrement_outstanding_scsi_request_count(eses_enclosure);
    }

    return status;

} // End of function fbe_eses_enclosure_send_scsi_cmd

/*!*************************************************************************
* @fn fbe_eses_enclosure_setup_inquiry_cmd(fbe_eses_enclosure_t * eses_enclosure, 
*                                           fbe_packet_t * packet,
*                                           fbe_payload_cdb_operation_t * payload_cdb_operation)                  
***************************************************************************
* @brief
*       This function sets up the "inquiry" command.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
* @param     payload_cdb_operation - The pointer to the payload cdb operation.
*
* @return   fbe_status_t.
*        FBE_STATUS_OK  - no error found.
*        otherwise - error is found.
*
* NOTES
*   The function fbe_sas_enclosure_build_inquiry_packet includes cdb packet allocation,
*   buffer (for sg_list allocation) and set the completion function. Since these steps
*   has been done in fbe_eses_enclosure_send_scsi_cmd., we created this function.
*
* HISTORY
*   12-Dec-2008 PHE - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_setup_inquiry_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                            fbe_packet_t * packet,
                                            fbe_payload_cdb_operation_t * payload_cdb_operation)   
{
    fbe_payload_ex_t * payload = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    /********
     * BEGIN
     ********/


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry \n", 
                            __FUNCTION__ );
    
    payload = fbe_transport_get_payload_ex(packet);    
    
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    sg_list[0].count = FBE_SCSI_INQUIRY_DATA_SIZE;
    sg_list[0].address = (fbe_u8_t *)sg_list + (2 * sizeof(fbe_sg_element_t));

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    // Update the sg_count.
    fbe_payload_ex_set_sg_list(payload, sg_list, 1);
    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, FBE_SCSI_INQUIRY_DATA_SIZE);

    // Build cdb operation with the details of the cdb.
    status = fbe_payload_cdb_build_inquiry(payload_cdb_operation, FBE_ESES_ENCLOSURE_ESES_PAGE_TIMEOUT);

    return status;
} // End of fbe_eses_enclosure_setup_inquiry_cmd


/*!*************************************************************************
* @fn fbe_eses_enclosure_setup_receive_diag_cmd(fbe_eses_enclosure_t * eses_enclosure, 
*                                           fbe_packet_t * packet,
*                                           fbe_payload_cdb_operation_t * payload_cdb_operation)                  
***************************************************************************
* @brief
*       This function sets up the "receive diag results" command.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
* @param     payload_cdb_operation - The pointer to the payload cdb operation.
*
* @return   fbe_status_t.
*        FBE_STATUS_OK  - no error found.
*        otherwise - error is found.
*
* NOTES
*
* HISTORY
*   27-Jul-2008 PHE - Created.
*   30-Sept-2008 NC - use payload
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_setup_receive_diag_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                            fbe_packet_t * packet,
                                            fbe_payload_cdb_operation_t * payload_cdb_operation)   
{
    fbe_payload_ex_t * payload = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_u8_t * cdb = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_operation_opcode_t opcode;
    fbe_payload_control_buffer_t control_buffer = NULL;
    fbe_enclosure_mgmt_ctrl_op_t * eses_page_op = NULL;
    fbe_enclosure_mgmt_trace_buf_cmd_t * trace_buf_cmd_p = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    ses_pg_code_enum page_code = SES_PG_CODE_INVALID; 

    /********
     * BEGIN
     ********/


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry \n", __FUNCTION__ );
   
    payload = fbe_transport_get_payload_ex(packet);

    /* Get the control operation which is allocated in either monitor or usurper.*/  
    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, get_control_operation failed, NULL pointer.\n",
                                __FUNCTION__ );
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PACKET_FAILED);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    
    }

    fbe_payload_control_get_opcode(control_operation, &opcode);    
  
    // Set the page code based on the opcode.
    switch(opcode)
    { 
        case FBE_ESES_CTRL_OPCODE_GET_CONFIGURATION:
        case FBE_ESES_CTRL_OPCODE_GET_SAS_ENCL_TYPE:     
            page_code = SES_PG_CODE_CONFIG;  // Configuration diagnostic page.    
            break;

        case FBE_ESES_CTRL_OPCODE_GET_ENCL_STATUS:
            page_code = SES_PG_CODE_ENCL_STAT;  // Status diagnostic page.    
            break;

        case FBE_ESES_CTRL_OPCODE_GET_ADDITIONAL_STATUS: 
            page_code = SES_PG_CODE_ADDL_ELEM_STAT;  // Additional status diagnostic page.       
            break;

        case FBE_ESES_CTRL_OPCODE_GET_EMC_SPECIFIC_STATUS: 
            page_code = SES_PG_CODE_EMC_ENCL_STAT;  // EMC specific status diagnostic page.
            break;

        case FBE_ESES_CTRL_OPCODE_GET_TRACE_BUF_INFO_STATUS:
            fbe_payload_control_get_buffer(control_operation, &control_buffer);
            if( control_buffer == NULL )
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "opcode: 0x%x, setup_receive_diag_cmd, get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                                        opcode, status, control_buffer);

                fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PACKET_FAILED);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            eses_page_op = (fbe_enclosure_mgmt_ctrl_op_t *)control_buffer;
            trace_buf_cmd_p = &(eses_page_op->cmd_buf.trace_buf_cmd);

            if((trace_buf_cmd_p->buf_op != FBE_ENCLOSURE_TRACE_BUF_OP_GET_NUM) && 
                (trace_buf_cmd_p->buf_op != FBE_ENCLOSURE_TRACE_BUF_OP_GET_STATUS)) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s, opcode: 0x%x, invalid buf_op: 0x%x.\n",
                                        __FUNCTION__,  opcode, trace_buf_cmd_p->buf_op);

                fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, 
                               FBE_ENCLOSURE_STATUS_PARAMETER_INVALID);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            page_code = SES_PG_CODE_EMC_ENCL_STAT;  // EMC specific status diagnostic page.
            break;      

        case FBE_ESES_CTRL_OPCODE_GET_DOWNLOAD_STATUS:
            page_code = SES_PG_CODE_DOWNLOAD_MICROCODE_STAT; // firmware download status page            
            break;

        case FBE_ESES_CTRL_OPCODE_GET_TUNNEL_CMD_STATUS:
            page_code = SES_PG_CODE_TUNNEL_DIAG_STATUS; // tunnel command status page            
            break;

        case FBE_ESES_CTRL_OPCODE_EMC_STATISTICS_STATUS_CMD:
        case FBE_ESES_CTRL_OPCODE_GET_EMC_STATISTICS_STATUS:
            page_code = SES_PG_CODE_EMC_STATS_STAT;     // EMC Statistics status diagnostic page.
            break;

        case FBE_ESES_CTRL_OPCODE_THRESHOLD_IN_CMD:         
            page_code = SES_PG_CODE_THRESHOLD_IN;     // EMC Threshold In diagnostic page.
            break;
         
        default:
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, invalid opcode: 0x%x. Fail request\n",
                                    __FUNCTION__, opcode);
            
            /* since we don't know how to handle the completion, we should
             * fail the request here, so that we don't accidentally have
             * the caller wait for the response forever.
             */           
            fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_CTRL_CODE_UNSUPPORTED);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;

            break;
    }     
    
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    if ( sg_list != NULL )
    {
        sg_list[0].count = FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE;
        sg_list[0].address = (fbe_u8_t *)sg_list + (2 * sizeof(fbe_sg_element_t));
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, SG_LIST pointer is NULL\n", __FUNCTION__);
        
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PACKET_FAILED);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    // Update the sg_count.
    fbe_payload_ex_set_sg_list(payload, sg_list, 1);
    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE);

    /* cdb_operation setup */
    status = fbe_payload_cdb_set_parameters(payload_cdb_operation,
                                   FBE_ESES_ENCLOSURE_ESES_PAGE_TIMEOUT,
                                   FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE, //attribute
                                   FBE_PAYLOAD_CDB_FLAGS_DATA_IN,  // flag
                                   FBE_SCSI_RECEIVE_DIAGNOSTIC_RESULTS_CDB_SIZE); // The cdb length

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_payload_cdb_set_parameters failed, opcode: 0x%x.\n",
                                __FUNCTION__, opcode);
    
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED);
        fbe_transport_complete_packet(packet);
        return status;
    } 

    status = fbe_payload_cdb_operation_get_cdb(payload_cdb_operation, &cdb);
    
    /* details of the CDB belongs to eses library */
    status = fbe_eses_build_receive_diagnostic_results_cdb(cdb, 
                                              FBE_SCSI_RECEIVE_DIAGNOSTIC_RESULTS_CDB_SIZE, // size of CDB.
                                              FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE, // the maximum number of bytes to be returned.
                                              page_code);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, build_receive_diagnostic_results_cdb failed, opcode: 0x%x.\n",
                                __FUNCTION__, opcode);
    
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED);
        fbe_transport_complete_packet(packet);
        return status;
    } 

    return status;
} // End of fbe_eses_enclosure_setup_receive_diag_cmd

/*!*************************************************************************
* @fn fbe_eses_enclosure_setup_send_diag_cmd(fbe_eses_enclosure_t * eses_enclosure, 
*                                        fbe_packet_t * packet,
*                                        fbe_payload_cdb_operation_t * payload_cdb_operation)
***************************************************************************
* @brief
*   This function sets up the "send diag" command.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
* @param     payload_cdb_operation - The pointer to the payload cdb operation.
*
* @return   fbe_status_t.
*        FBE_STATUS_OK  - no error found.
*        otherwise - error is found.
*
* NOTES
*
* HISTORY
*   18-Aug-2008 GB - Created.
*   30-Sept-2008 NC - use payload
*   25-Sep-2008 PHE - Changed the function name from fbe_eses_enclosure_send_firmware_download_page
*                     and modify the function so that it can be used for all the pages sent by 
*                     the " SCSI send diagnostic" command.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_setup_send_diag_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                        fbe_packet_t * packet,
                                        fbe_payload_cdb_operation_t * payload_cdb_operation)
{
    fbe_sg_element_t    *sg_list = NULL;
    fbe_payload_ex_t       *payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_t control_buffer = NULL;    
    fbe_payload_control_operation_opcode_t  opcode;
    fbe_u8_t            *cdb = NULL;
    fbe_u16_t           parameter_length = 0; // a field in the send diag command cdb
    fbe_u32_t           sg_count = 0; 
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_time_t          timeout = 0;

    fbe_enclosure_mgmt_ctrl_op_t * eses_page_op = NULL;
    fbe_enclosure_mgmt_trace_buf_cmd_t *trace_buf_cmd = NULL;

    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry \n", 
                            __FUNCTION__ );
    
    payload = fbe_transport_get_payload_ex(packet);

    /* 
     * Get the control operation which is allocated in either monitor or usurper
     * which is the current operation.
     */  
    control_operation = fbe_payload_ex_get_control_operation(payload);

    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, get_control_operation failed, NULL pointer.\n",
                                __FUNCTION__ );
        
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PACKET_FAILED);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    
    }

    fbe_payload_control_get_opcode(control_operation, &opcode);
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    // Build the corresponding page based on the opcode.
    switch(opcode)
    {
        case FBE_ESES_CTRL_OPCODE_SET_TRACE_BUF_INFO_CTRL: 
        case FBE_ESES_CTRL_OPCODE_STRING_OUT_CMD:
        case FBE_ESES_CTRL_OPCODE_THRESHOLD_OUT_CMD:    
            // Get the control buffer for these ops which need a control buffer.
            fbe_payload_control_get_buffer(control_operation, &control_buffer);
            if( control_buffer == NULL )
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_WARNING,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "opcode: 0x%x, fbe_payload_control_get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                                         opcode, status, control_buffer);
                fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            break;

        default:
            break;
    }

    switch(opcode)
    {
        case FBE_ESES_CTRL_OPCODE_DOWNLOAD_FIRMWARE:            
            // Build the download control page and set up the image pointer. 
            status = fbe_eses_ucode_build_download_or_activate(eses_enclosure, 
                                                           sg_list, 
                                                           &parameter_length,
                                                           &sg_count);

            /* The new payload method does not include the last termination element */
            sg_count --;
           
            break;

        case FBE_ESES_CTRL_OPCODE_TUNNEL_GET_CONFIGURATION:
            // Build the get configuration page. 
            status = fbe_eses_ucode_build_tunnel_receive_diag_results_cdb(eses_enclosure,
                                                                          SES_PG_CODE_CONFIG,
                                                                          sg_list,
                                                                          &parameter_length,
                                                                          &sg_count);
            // The new payload method does not include the last termination element
            sg_count --;

            break;
        case FBE_ESES_CTRL_OPCODE_TUNNEL_DOWNLOAD_FIRMWARE:
            // Build the download control page and set up the image pointer. 
            status = fbe_eses_ucode_build_tunnel_download_or_activate(eses_enclosure,
                                                                      sg_list,
                                                                      &parameter_length,
                                                                      &sg_count);
            // The new payload method does not include the last termination element
            sg_count --;

            break;
        case FBE_ESES_CTRL_OPCODE_TUNNEL_GET_DOWNLOAD_STATUS:
            // Build the download control page and set up the image pointer. 
            status = fbe_eses_ucode_build_tunnel_receive_diag_results_cdb(eses_enclosure,
                                                                          SES_PG_CODE_DOWNLOAD_MICROCODE_STAT,
                                                                          sg_list,
                                                                          &parameter_length,
                                                                          &sg_count);
            // The new payload method does not include the last termination element
            sg_count --;

            break;

        case FBE_ESES_CTRL_OPCODE_SET_EMC_SPECIFIC_CTRL: 
            /* Encl time element is in EMC specific control page. */
            status = fbe_eses_enclosure_build_emc_specific_ctrl_page(eses_enclosure,
                                                                    NULL,
                                                                    sg_list, 
                                                                    &parameter_length, // parameter_length is the page size.
                                                                    &sg_count); 
            
            /* The new payload method does not include the last termination element */
            sg_count --;  
           
            break;

        case FBE_ESES_CTRL_OPCODE_SET_TRACE_BUF_INFO_CTRL:                 
            eses_page_op = (fbe_enclosure_mgmt_ctrl_op_t *)control_buffer;
            trace_buf_cmd = &eses_page_op->cmd_buf.trace_buf_cmd;

            /* Trace buffer info is in EMC specific control page. */
            status = fbe_eses_enclosure_build_emc_specific_ctrl_page(eses_enclosure,
                                                                    trace_buf_cmd,                                                                    
                                                                    sg_list, 
                                                                    &parameter_length, // parameter_length is the page size.
                                                                    &sg_count); 
            
            /* The new payload method does not include the last termination element */
            sg_count --;  
            
            break;

        case FBE_ESES_CTRL_OPCODE_SET_ENCL_CTRL: 
            status = fbe_eses_enclosure_buildControlPage(eses_enclosure,// enclosure object
                                                         sg_list, // sg list
                                                         &parameter_length, // parameter_length is the page size.
                                                         &sg_count);             

            eses_enclosure->outstanding_write ++;
            
            //fbe_eses_printSendControlPage(eses_enclosure, (fbe_u8_t *)sg_list->address);

            /* The new payload method does not include the last termination element */
            sg_count --;  
            
            break;

        case FBE_ESES_CTRL_OPCODE_STRING_OUT_CMD:
            /* string out diagnostic control page. */  
            eses_page_op = (fbe_enclosure_mgmt_ctrl_op_t *)control_buffer;
            fbe_eses_enclosure_buildStringOutPage(eses_enclosure, 
                                                  eses_page_op, 
                                                  &parameter_length,
                                                  sg_list, 
                                                  &sg_count);

            /* The new payload method does not include the last termination element */
            sg_count --;  
            break;

        case FBE_ESES_CTRL_OPCODE_THRESHOLD_OUT_CMD:
            /* threshold out diagnostic control page. */
            
            eses_page_op = (fbe_enclosure_mgmt_ctrl_op_t *)control_buffer;

            fbe_eses_enclosure_buildThresholdOutPage(eses_enclosure, 
                                                  eses_page_op, 
                                                  &parameter_length,
                                                  sg_list, 
                                                  &sg_count);            
                        
            /* The new payload method does not include the last termination element */
            sg_count --;  
            break;
        default:
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, unhandled opcode: 0x%x.\n",
                                    __FUNCTION__, opcode);
            
            /* since we don't know how to handle the completion, we should
             * fail the request here, so that we don't accidentally have
             * the caller wait for the response forever.
             */           
            fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_CTRL_CODE_UNSUPPORTED);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
            break;
    } // End of - switch(page_code)    
  
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "send_diag_cmd, build page failed, opcode: 0x%x, status: 0x%x.\n",
                                opcode, status);
       
    
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_BUILD_PAGE_FAILED);
        fbe_transport_complete_packet(packet);
        return status;    
    }

    // Update the sg_count.    
    status = fbe_payload_ex_set_sg_list(payload, sg_list, sg_count);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_payload_ex_set_sg_list failed, opcode: 0x%x, status: 0x%x.\n",
                                __FUNCTION__, opcode, status);
    
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PACKET_FAILED);
        fbe_transport_complete_packet(packet);
        return status;
    } 

    /* cdb_operation setup */
    if(opcode == FBE_ESES_CTRL_OPCODE_DOWNLOAD_FIRMWARE)
    {
        /* While downloading the expander cdes_rom.bin, 
         * it always failed with timeout when setting the timeout value as 10 seconds
         * OPT 460295 tracks this issue. For a workaround, the timeout is changed to 60 seconds.
         */
        timeout = FBE_ESES_ENCLOSURE_ESES_PAGE_DOWNLOAD_FIRMWARE;
    }
    else
    {
        timeout = FBE_ESES_ENCLOSURE_ESES_PAGE_TIMEOUT;
    }

    status = fbe_payload_cdb_set_parameters(payload_cdb_operation,
                                            timeout,
                                            FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE, //attribute
                                            FBE_PAYLOAD_CDB_FLAGS_DATA_OUT,  // flag
                                            FBE_SCSI_SEND_DIAGNOSTIC_CDB_SIZE); // The cdb length

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_payload_cdb_set_parameters failed,  opcode: 0x%x.\n",
                                __FUNCTION__, opcode);
   
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED);
        fbe_transport_complete_packet(packet);
        return status;
    } 

    status = fbe_payload_cdb_operation_get_cdb(payload_cdb_operation, &cdb);
    /* details of the CDB belongs to eses library */
    status = fbe_eses_build_send_diagnostic_cdb(cdb, FBE_SCSI_SEND_DIAGNOSTIC_CDB_SIZE, parameter_length);

    status = fbe_payload_cdb_set_transfer_count(payload_cdb_operation, parameter_length);    

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_eses_build_send_diagnostic_cdb failed, opcode: 0x%x.\n",
                                __FUNCTION__,  opcode);
    
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED);
        fbe_transport_complete_packet(packet);
        return status;
    }

    return status;
} // fbe_eses_enclosure_setup_send_diag_cmd

/*!*************************************************************************
* @fn fbe_eses_enclosure_setup_read_buf_cmd(fbe_eses_enclosure_t * eses_enclosure, 
*                                        fbe_packet_t * packet,
*                                        fbe_payload_cdb_operation_t * payload_cdb_operation)   
***************************************************************************
* @brief
*   This function sets up the "read buffer" command.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
* @param     payload_cdb_operation - The pointer to the payload cdb operation.
*
* @return   fbe_status_t.
*        FBE_STATUS_OK  - no error found.
*        otherwise - error is found.
*
* NOTES
*
* HISTORY
*   20-Nov-2008 PHE - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_setup_read_buf_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                        fbe_packet_t * packet,
                                        fbe_payload_cdb_operation_t * payload_cdb_operation)      
{
    fbe_payload_ex_t * payload = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_u8_t * cdb = NULL;    
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_operation_opcode_t opcode;
    fbe_payload_control_buffer_t control_buffer = NULL;
    fbe_enclosure_mgmt_ctrl_op_t * eses_page_op = NULL;
    fbe_enclosure_mgmt_read_buf_cmd_t * read_buf_cmd_p = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry \n", 
                            __FUNCTION__ );

    payload = fbe_transport_get_payload_ex(packet);     
    /* 
     * Get the control operation which is allocated in either monitor or usurper.
     * which is the current operation. 
     */  
    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s,  get_control_operation failed, NULL pointer.\n",
                                __FUNCTION__ );
        
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PACKET_FAILED);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    
    }

     /* Get the control operation opcode. */
    fbe_payload_control_get_opcode(control_operation, &opcode);

    fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( control_buffer == NULL )
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "opcode: 0x%x, setup_read_buf_cmd, get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                                opcode, status, control_buffer);

        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PACKET_FAILED);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    eses_page_op = (fbe_enclosure_mgmt_ctrl_op_t *)control_buffer;

    // get the pointer to the client operation, the location depends on the opcode
    if (opcode == FBE_ESES_CTRL_OPCODE_READ_RESUME)
    {
        read_buf_cmd_p = &eses_page_op->cmd_buf.resume_read.read_buf;
    }
    else
    {
        read_buf_cmd_p = &eses_page_op->cmd_buf.read_buf;
    }

    if((read_buf_cmd_p->mode != FBE_ESES_READ_BUF_MODE_DESC) &&
        (read_buf_cmd_p->mode != FBE_ESES_READ_BUF_MODE_DATA))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_WARNING,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, invalid mode: 0x%x.\n",
                                __FUNCTION__, read_buf_cmd_p->mode);
    
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, 
                                       FBE_ENCLOSURE_STATUS_PARAMETER_INVALID);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
        
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    sg_list[0].count = eses_page_op->response_buf_size; //The number of bytes allocated for the response.
    sg_list[0].address = (fbe_u8_t *)sg_list + (2 * sizeof(fbe_sg_element_t));  // Points to the response buffer allocated by the client.
    
    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    // Update the count of sg elements.
    fbe_payload_ex_set_sg_list(payload, sg_list, 1);
    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, eses_page_op->response_buf_size);

    /* cdb_operation setup */
    status = fbe_payload_cdb_set_parameters(payload_cdb_operation,  
                                   FBE_ESES_ENCLOSURE_ESES_PAGE_TIMEOUT,  // timeout
                                   FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE, //attribute
                                   FBE_PAYLOAD_CDB_FLAGS_DATA_IN,  // flag
                                   FBE_SCSI_READ_BUFFER_CDB_SIZE); // The cdb length

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_payload_cdb_set_parameters failed.\n",
                                __FUNCTION__ );
    
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED);
        fbe_transport_complete_packet(packet);
        return status;
    } 

    status = fbe_payload_cdb_operation_get_cdb(payload_cdb_operation, &cdb);    
   
    /* details of the CDB belongs to eses library */
    status = fbe_eses_build_read_buf_cdb(cdb, 
            FBE_SCSI_READ_BUFFER_CDB_SIZE, // The size of CDB.
            read_buf_cmd_p->mode, // The mode of the read buffer command.
            read_buf_cmd_p->buf_id, // The id of the buffer to be read.
            read_buf_cmd_p->buf_offset, // The byte offset inot a buffer described by buffer id. not used in all modes. 
            eses_page_op->response_buf_size);   // The number of bytes the client allocated for the response.  

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_eses_build_read_buf_cdb failed.\n",
                                __FUNCTION__ );
    
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED);
        fbe_transport_complete_packet(packet);
        return status;
    } 

    return status;

} // fbe_eses_enclosure_setup_read_buf_cmd

/*!*************************************************************************
* @fn fbe_eses_enclosure_setup_raw_inquiry_cmd(fbe_eses_enclosure_t * eses_enclosure, 
*                                           fbe_packet_t * packet,
*                                           fbe_payload_cdb_operation_t * payload_cdb_operation)                  
***************************************************************************
* @brief
*       This function sets up the "inquiry" command.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
* @param     payload_cdb_operation - The pointer to the payload cdb operation.
*
* @return   fbe_status_t.
*        FBE_STATUS_OK  - no error found.
*        otherwise - error is found.
*
* NOTES
*   The function fbe_sas_enclosure_build_inquiry_packet includes cdb packet allocation,
*   buffer (for sg_list allocation) and set the completion function.
*
* HISTORY
*   20-Mar-2009 AS - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_setup_raw_inquiry_cmd(fbe_eses_enclosure_t *eses_enclosure, 
                                            fbe_packet_t *packet,
                                            fbe_payload_cdb_operation_t *payload_cdb_operation)   
{
    fbe_payload_ex_t *payload = NULL;
    fbe_sg_element_t  *sg_list = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    fbe_enclosure_mgmt_ctrl_op_t *eses_page_op = NULL;
    fbe_payload_control_buffer_t control_buffer = NULL;

    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_scsi_inquiry_vpd_pg_enum page_code = FBE_SCSI_INQUIRY_VPD_PG_INVALID; 
    
    /********
     * BEGIN
     ********/


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__ );
    
    payload = fbe_transport_get_payload_ex(packet);    
    
    /* Get the control operation which is allocated in either monitor or usurper.
     * which is the current operation. 
     */  
    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, get_control_operation failed, NULL pointer.\n",
                                __FUNCTION__);
        
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PACKET_FAILED);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    
    }

    fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( control_buffer == NULL )
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_WARNING,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s,  fbe_payload_control_get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                                __FUNCTION__, status, control_buffer);

        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PACKET_FAILED);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    eses_page_op = (fbe_enclosure_mgmt_ctrl_op_t *)control_buffer;

    if ( eses_page_op->cmd_buf.raw_scsi.raw_inquiry.evpd )
    {
        /* Set the page code based on the opcode. */
        switch(eses_page_op->cmd_buf.raw_scsi.raw_inquiry.page_code)
        { 
            case FBE_ENCLOSURE_RAW_INQUIRY_VPD_PG_SUPPORTED:     
                page_code = FBE_SCSI_INQUIRY_VPD_PG_SUPPORTED;  // Supported.    
                break;

            case FBE_ENCLOSURE_RAW_INQUIRY_VPD_PG_UNIT_SERIALNO:
                page_code = FBE_SCSI_INQUIRY_VPD_PG_UNIT_SERIALNO;  // Unit Serial No.    
                break;

            case FBE_ENCLOSURE_RAW_INQUIRY_VPD_PG_DEVICE_ID: 
                page_code = FBE_SCSI_INQUIRY_VPD_PG_DEVICE_ID;  // Device Id.       
                break;

            case FBE_ENCLOSURE_RAW_INQUIRY_VPD_PG_EXTENDED: 
                page_code = FBE_SCSI_INQUIRY_VPD_PG_EXTENDED;  // Extended.
                break;

            case FBE_ENCLOSURE_RAW_INQUIRY_VPD_PG_SUBENCL: 
                page_code = FBE_SCSI_INQUIRY_VPD_PG_SUB_ENCL;  // Sub enclosure.
                break;

            default:
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s, invalid opcode: 0x%x. Fail request\n",
                                        __FUNCTION__, 
                                        eses_page_op->cmd_buf.raw_scsi.raw_inquiry.page_code);
                
                /* since we don't know how to handle the completion, we should
                * fail the request here, so that we don't accidentally have
                * the caller wait for the response forever.
                */           
                fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PARAMETER_INVALID);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;

                break;
        }     
    }
    
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    sg_list[0].count = eses_page_op->response_buf_size; //The number of bytes allocated for the response.
    sg_list[0].address = eses_page_op->response_buf_p;  // Points to the response buffer allocated by the client.

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);    
    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, eses_page_op->response_buf_size);

    if ( eses_page_op->cmd_buf.raw_scsi.raw_inquiry.evpd )
    {
        // Build cdb operation with the details of the cdb for all SCSI VPD Inquiry Pages.
        status = fbe_payload_cdb_build_vpd_inquiry(payload_cdb_operation, 
                                            FBE_ESES_ENCLOSURE_ESES_PAGE_TIMEOUT,
                                            eses_page_op->cmd_buf.raw_scsi.raw_inquiry.page_code);
    }
    else
    {
        // Build cdb operation with the details of the cdb for standard Inquiry Page.
        status = fbe_payload_cdb_build_inquiry(payload_cdb_operation, 
                                                FBE_ESES_ENCLOSURE_ESES_PAGE_TIMEOUT);
    }

    // the fbe_payload_cbd_ functions above only return success, therfore we should never get bad status
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_eses_enclosure_setup_raw_inquiry_cmd failed.\n",
                                __FUNCTION__);
    
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED);
        fbe_transport_complete_packet(packet);
        return status;
    } 

    return status;
} // End of fbe_eses_enclosure_setup_raw_inquiry_cmd

/*!*************************************************************************
* @fn fbe_eses_enclosure_setup_raw_rcv_diag_cmd(fbe_eses_enclosure_t * eses_enclosure, 
*                                        fbe_packet_t * packet,
*                                        fbe_payload_cdb_operation_t * payload_cdb_operation)   
***************************************************************************
* @brief
*   This function sets up the pass-throu receive diag command.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
* @param     payload_cdb_operation - The pointer to the payload cdb operation.
*
* @return   fbe_status_t.
*        FBE_STATUS_OK  - no error found.
*        otherwise - error is found.
*
* NOTES
*
* HISTORY
*   2-Mar-2009 NC - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_setup_raw_rcv_diag_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                        fbe_packet_t * packet,
                                        fbe_payload_cdb_operation_t * payload_cdb_operation)      
{
    fbe_payload_ex_t * payload = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_u8_t * cdb = NULL;    
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_t control_buffer = NULL;

    fbe_enclosure_mgmt_ctrl_op_t * eses_page_op = NULL;
    ses_pg_code_enum page_code = SES_PG_CODE_INVALID; 

    fbe_status_t status = FBE_STATUS_OK;

    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n",__FUNCTION__ );

    payload = fbe_transport_get_payload_ex(packet);     
    /* 
     * Get the control operation which is allocated in either monitor or usurper.
     * which is the current operation. 
     */  
    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, get_control_operation failed, NULL pointer.\n",
                                __FUNCTION__);
        
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PACKET_FAILED);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    
    }

    fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( control_buffer == NULL )
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_WARNING,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_payload_control_get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                                __FUNCTION__, status, control_buffer);
        
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PACKET_FAILED);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    eses_page_op = (fbe_enclosure_mgmt_ctrl_op_t *)control_buffer;

    // Set the page code based on the opcode.
    switch(eses_page_op->cmd_buf.raw_rcv_diag_page.rcv_diag_page)
    { 
        case FBE_ENCLOSURE_RAW_RCV_DIAG_PG_CFG:     
            page_code = SES_PG_CODE_CONFIG;  // Configuration diagnostic page.    
            break;

        case FBE_ENCLOSURE_RAW_RCV_DIAG_PG_STATUS:
            page_code = SES_PG_CODE_ENCL_STAT;  // Status diagnostic page.    
            break;

        case FBE_ENCLOSURE_RAW_RCV_DIAG_PG_ADDL_STATUS: 
            page_code = SES_PG_CODE_ADDL_ELEM_STAT;  // Additional status diagnostic page.       
            break;

        case FBE_ENCLOSURE_RAW_RCV_DIAG_PG_EMC_ENCL_STATUS: 
            page_code = SES_PG_CODE_EMC_ENCL_STAT;  // EMC specific status diagnostic page.
            break;

        case FBE_ENCLOSURE_RAW_RCV_DIAG_PG_STRING_IN:
            page_code = SES_PG_CODE_STR_IN;
            break;

        case FBE_ENCLOSURE_RAW_RCV_DIAG_PG_STATISTICS:
            page_code = SES_PG_CODE_EMC_STATS_STAT;
            break;

        case FBE_ENCLOSURE_RAW_RCV_DIAG_PG_THRESHOLD_IN:
            page_code = SES_PG_CODE_THRESHOLD_IN;
            break;
        default:
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, invalid opcode: 0x%x. Fail request\n",
                                    __FUNCTION__, 
                                    eses_page_op->cmd_buf.raw_rcv_diag_page.rcv_diag_page);
            
            /* since we don't know how to handle the completion, we should
             * fail the request here, so that we don't accidentally have
             * the caller wait for the response forever.
             */           
            fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PARAMETER_INVALID);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;

            break;
    }     
    
        
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    sg_list[0].count = eses_page_op->response_buf_size; //The number of bytes allocated for the response.
    sg_list[0].address = eses_page_op->response_buf_p;  // Points to the response buffer allocated by the client.

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);    
    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, eses_page_op->response_buf_size);

    /* cdb_operation setup */
    status = fbe_payload_cdb_set_parameters(payload_cdb_operation,  
                                   FBE_ESES_ENCLOSURE_ESES_PAGE_TIMEOUT,  // timeout
                                   FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE, //attribute
                                   FBE_PAYLOAD_CDB_FLAGS_DATA_IN,  // flag
                                   FBE_SCSI_RECEIVE_DIAGNOSTIC_RESULTS_CDB_SIZE); // The cdb length

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_payload_cdb_set_parameters failed \n",
                                __FUNCTION__);
    
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED);
        fbe_transport_complete_packet(packet);
        return status;
    } 

    status = fbe_payload_cdb_operation_get_cdb(payload_cdb_operation, &cdb);
    
    /* details of the CDB belongs to eses library */
    status = fbe_eses_build_receive_diagnostic_results_cdb(cdb, 
                                              FBE_SCSI_RECEIVE_DIAGNOSTIC_RESULTS_CDB_SIZE, // size of CDB.
                                              eses_page_op->response_buf_size, // the maximum number of bytes to be returned.
                                              page_code);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, build_receive_diagnostic_results_cdb failed.\n",
                                __FUNCTION__);
    
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED);
        fbe_transport_complete_packet(packet);
        return status;
    } 

    return status;

} // fbe_eses_enclosure_setup_raw_rcv_diag_cmd

/*!*************************************************************************
 * @fn fbe_eses_enclosure_setup_write_resume_cmd(fbe_eses_enclosure_t * eses_enclosure, 
 *                                        fbe_packet_t * packet,
 *                                        fbe_payload_cdb_operation_t * payload_cdb_operation)   
 ***************************************************************************
 * @brief
 *   This function sets up the "Write Resume Prom" command.
 *
 * @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
 * @param     packet - The pointer to the fbe_packet_t.
 * @param     payload_cdb_operation - The pointer to the payload cdb operation.
 *
 * @return   fbe_status_t.
 *        FBE_STATUS_OK  - no error found.
 *        otherwise - error is found.
 *
 * NOTES
 *
 * HISTORY
 *   02-Feb-2009 AS - Created.
 *   16-Oct-2009 Prasenjeet - Added request type in fbe_eses_enclosure_prom_id_to_buf_id
 ***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_setup_write_resume_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                        fbe_packet_t * packet,
                                        fbe_payload_cdb_operation_t * payload_cdb_operation)      
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u8_t buf_id = 0; 
    fbe_u8_t *cdb = NULL;    
    fbe_payload_ex_t *payload = NULL;
    fbe_sg_element_t  *sg_list = NULL;
    fbe_payload_control_buffer_t control_buffer = NULL;
    fbe_enclosure_mgmt_ctrl_op_t *eses_page_op = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_enclosure_mgmt_resume_write_cmd_t *write_buf_cmd_p = NULL;

    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);     

    /* Get the control operation which is allocated in either monitor or usurper.
     * which is the current operation. 
     */  
    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, get_control_operation failed, NULL pointer.\n",
                                __FUNCTION__);
        
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PACKET_FAILED);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( control_buffer == NULL )
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_WARNING,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_payload_control_get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                                __FUNCTION__,  status, control_buffer);

        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PACKET_FAILED);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    eses_page_op = (fbe_enclosure_mgmt_ctrl_op_t *)control_buffer;

    write_buf_cmd_p = &eses_page_op->cmd_buf.resume_write;

    /* Get the buffer id for the component for the resume write operation */
    status = fbe_eses_enclosure_get_buf_id(eses_enclosure, 
                                           write_buf_cmd_p->deviceType,
                                           &write_buf_cmd_p->deviceLocation,
                                           &buf_id,
                                           FBE_ENCLOSURE_WRITE_RESUME_PROM);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, get_buf_id failed.\n",
                                __FUNCTION__);
    
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PARAMETER_INVALID);
        fbe_transport_complete_packet(packet);
        return status;
    } 
    else if(buf_id == FBE_ENCLOSURE_VALUE_INVALID) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, invalid buf id %d.\n",
                                __FUNCTION__, buf_id);
    
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_COMPONENT_NOT_FOUND);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    sg_list[0].count = write_buf_cmd_p->data_buf_size; //The number of bytes allocated for the response.
    sg_list[0].address = write_buf_cmd_p->data_buf_p;  // Points to the response buffer allocated by the client.

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);    

    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, write_buf_cmd_p->data_buf_size);
        
    /* cdb_operation setup */
    status = fbe_payload_cdb_set_parameters(payload_cdb_operation,  
                                   FBE_ESES_ENCLOSURE_ESES_PAGE_TIMEOUT,  // timeout
                                   FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE, //attribute
                                   FBE_PAYLOAD_CDB_FLAGS_DATA_OUT,  // flag
                                   FBE_SCSI_WRITE_BUFFER_CDB_SIZE); // The cdb length

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                 "%s, fbe_payload_cdb_set_parameters failed.\n",
                                __FUNCTION__);
   
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED);
        fbe_transport_complete_packet(packet);
        return status;
    } 

    status = fbe_payload_cdb_operation_get_cdb(payload_cdb_operation, &cdb);
    
    /* Details of the CDB belongs to eses library */
    status = fbe_eses_build_write_buf_cdb(cdb, 
            FBE_SCSI_WRITE_BUFFER_CDB_SIZE, // The size of CDB.
            FBE_ESES_WRITE_BUF_MODE_DATA, // (write_buf_cmd_p->mode,) The mode of the read buffer command.
            buf_id, // The id of the buffer to be written.
            write_buf_cmd_p->data_offset, // The byte offset is not a buffer described by buffer id. not used in all modes. 
            write_buf_cmd_p->data_buf_size);   // data buffer size.  

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_eses_build_write_buf_cdb failed.\n",
                                __FUNCTION__);
    
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED);
        fbe_transport_complete_packet(packet);
        return status;
    } 

    return status;
} // fbe_eses_enclosure_setup_write_resume_cmd

/*!*************************************************************************
 * @fn fbe_eses_enclosure_setup_sps_in_buf_cmd(fbe_eses_enclosure_t * eses_enclosure, 
 *                                             fbe_packet_t * packet,
 *                                             fbe_payload_cdb_operation_t * payload_cdb_operation)   
 ***************************************************************************
 * @brief
 *   This function sets up the "SPS In Buffer" command.
 *
 * @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
 * @param     packet - The pointer to the fbe_packet_t.
 * @param     payload_cdb_operation - The pointer to the payload cdb operation.
 *
 * @return   fbe_status_t.
 *        FBE_STATUS_OK  - no error found.
 *        otherwise - error is found.
 *
 * NOTES
 *
 * HISTORY
 *   04-Sep-2012    Joe Perry - Created.
 ***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_setup_sps_in_buf_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                        fbe_packet_t * packet,
                                        fbe_payload_cdb_operation_t * payload_cdb_operation)      
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_u8_t                            buf_id = 0; 
    fbe_u8_t                            *cdb = NULL;    
    fbe_payload_ex_t                    *payload = NULL;
    fbe_sg_element_t                    *sg_list = NULL;
    fbe_payload_control_buffer_t        control_buffer = NULL;
    fbe_enclosure_mgmt_ctrl_op_t        *eses_page_op;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_u8_t                            cmdLength = 0;
    dev_sps_in_buffer_struct_t          *sps_in_buffer_p = NULL;
    fbe_u8_t                            *bufferPtr;

    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);     

    /* Get the control operation which is allocated in either monitor or usurper.
     * which is the current operation. 
     */  
    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, get_control_operation failed, NULL pointer.\n",
                                __FUNCTION__);
        
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PACKET_FAILED);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( control_buffer == NULL )
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_WARNING,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_payload_control_get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                                __FUNCTION__,  status, control_buffer);

        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PACKET_FAILED);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    eses_page_op = (fbe_enclosure_mgmt_ctrl_op_t *)control_buffer;

    if ((eses_page_op->cmd_buf.spsInBufferCmd.spsAction!= FBE_SPS_ACTION_START_TEST) &&
        (eses_page_op->cmd_buf.spsInBufferCmd.spsAction!=  FBE_SPS_ACTION_STOP_TEST) &&
        (eses_page_op->cmd_buf.spsInBufferCmd.spsAction!=  FBE_SPS_ACTION_SHUTDOWN))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_ERROR,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                           "%s, Invalid spsAction %d.\n",
                                           __FUNCTION__, eses_page_op->cmd_buf.spsInBufferCmd.spsAction);
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PARAMETER_INVALID);
        fbe_transport_complete_packet(packet);
        return status;
    }

    /* Get the buffer id for the component for the resume write operation */
    status = fbe_eses_enclosure_get_buf_id(eses_enclosure, 
                                           FBE_DEVICE_TYPE_SPS,
                                           NULL,
                                           &buf_id,
                                           FBE_ENCLOSURE_WRITE_ACTIVE_RAM);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_payload_cdb_set_parameters failed.\n",
                                __FUNCTION__);
    
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PARAMETER_INVALID);
        fbe_transport_complete_packet(packet);
        return status;
    } 

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    /*
     * Format SPS In Buffer command
     */
    bufferPtr = fbe_transport_allocate_buffer(); 
    fbe_set_memory(bufferPtr, 0, FBE_MEMORY_CHUNK_SIZE);

    sps_in_buffer_p = (dev_sps_in_buffer_struct_t *)bufferPtr;
    sps_in_buffer_p->version = 0;
    sps_in_buffer_p->cmd_token = swap16(++fbe_eses_enclosure_spsCmdToken);
    switch (eses_page_op->cmd_buf.spsInBufferCmd.spsAction)
    {
        case FBE_SPS_ACTION_START_TEST:
            // format the command to start SPS Test
            cmdLength = FBE_HW_SPS_BATTEST_CMD_LEN;
            fbe_copy_memory(&sps_in_buffer_p->command, 
                            FBE_HW_SPS_BATTEST_CMD,
                            FBE_HW_SPS_BATTEST_CMD_LEN);
            break;
        case FBE_SPS_ACTION_STOP_TEST:
        case FBE_SPS_ACTION_SHUTDOWN:
            cmdLength = FBE_HW_SPS_STOP_CMD_LEN;
            fbe_copy_memory(&sps_in_buffer_p->command, 
                            FBE_HW_SPS_STOP_CMD,
                            FBE_HW_SPS_STOP_CMD_LEN);
            break;
        default:
            break;
    }
    sps_in_buffer_p->command_len = cmdLength;

    sg_list[0].count = cmdLength + SES_SPS_IN_BUF_HEADER_LEN;       // add size of header to command length
    sg_list[0].address = (fbe_u8_t *) sps_in_buffer_p;

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);    

    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, sg_list[0].count);
        
    /* cdb_operation setup */
    status = fbe_payload_cdb_set_parameters(payload_cdb_operation,  
                                            FBE_ESES_ENCLOSURE_ESES_PAGE_TIMEOUT,  // timeout
                                            FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE, //attribute
                                            FBE_PAYLOAD_CDB_FLAGS_DATA_OUT,  // flag
                                            FBE_SCSI_WRITE_BUFFER_CDB_SIZE); // The cdb length

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_ERROR,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                           "%s, fbe_payload_cdb_set_parameters failed.\n",
                                           __FUNCTION__);
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED);
        fbe_transport_complete_packet(packet);
        return status;
    } 

    status = fbe_payload_cdb_operation_get_cdb(payload_cdb_operation, &cdb);
    
    /* Details of the CDB belongs to eses library */
    status = fbe_eses_build_write_buf_cdb(cdb, 
                                          FBE_SCSI_WRITE_BUFFER_CDB_SIZE, // The size of CDB.
                                          FBE_ESES_WRITE_BUF_MODE_DATA, // (write_buf_cmd_p->mode,) The mode of the read buffer command.
                                          buf_id, // The id of the buffer to be written.
                                          0,
                                          sg_list[0].count);   // data buffer size.  

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_ERROR,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                           "%s, fbe_eses_build_write_buf_cdb failed.\n",
                                           __FUNCTION__);
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED);
        fbe_transport_complete_packet(packet);
        return status;
    } 

    return status;
} // fbe_eses_enclosure_setup_sps_in_buf_cmd


/*!*************************************************************************
 * @fn fbe_eses_enclosure_setup_sps_eeprom_cmd(fbe_eses_enclosure_t * eses_enclosure, 
 *                                             fbe_packet_t * packet,
 *                                             fbe_payload_cdb_operation_t * payload_cdb_operation)   
 ***************************************************************************
 * @brief
 *   This function sets up the "SPS EEPROM" command (get Mfg Info).
 *
 * @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
 * @param     packet - The pointer to the fbe_packet_t.
 * @param     payload_cdb_operation - The pointer to the payload cdb operation.
 *
 * @return   fbe_status_t.
 *        FBE_STATUS_OK  - no error found.
 *        otherwise - error is found.
 *
 * NOTES
 *
 * HISTORY
 *   21-Sep-2012    Joe Perry - Created.
 ***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_setup_sps_eeprom_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                        fbe_packet_t * packet,
                                        fbe_payload_cdb_operation_t * payload_cdb_operation)      
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_u8_t                            buf_id = 0; 
    fbe_u8_t                            *cdb = NULL;    
    fbe_payload_ex_t                    *payload = NULL;
    fbe_sg_element_t                    *sg_list = NULL;
    fbe_payload_control_buffer_t        control_buffer = NULL;
    fbe_enclosure_mgmt_ctrl_op_t        *eses_page_op;
    fbe_payload_control_operation_t     *control_operation = NULL;

    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);     

    /* Get the control operation which is allocated in either monitor or usurper.
     * which is the current operation. 
     */  
    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, get_control_operation failed, NULL pointer.\n",
                                __FUNCTION__);
        
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PACKET_FAILED);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( control_buffer == NULL )
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_WARNING,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_payload_control_get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                                __FUNCTION__,  status, control_buffer);

        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PACKET_FAILED);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    eses_page_op = (fbe_enclosure_mgmt_ctrl_op_t *)control_buffer;

    /* Get the buffer id for the component for the resume write operation */
    status = fbe_eses_enclosure_get_buf_id(eses_enclosure, 
                                           FBE_DEVICE_TYPE_SPS,
                                           NULL,
                                           &buf_id,
                                           FBE_ENCLOSURE_READ_RESUME_PROM);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_payload_cdb_set_parameters failed.\n",
                                __FUNCTION__);
    
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PARAMETER_INVALID);
        fbe_transport_complete_packet(packet);
        return status;
    } 

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    sg_list[0].count = sizeof(sps_eeprom_info_struct);
    sg_list[0].address = (fbe_u8_t *)sg_list + (2 * sizeof(fbe_sg_element_t));

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);    

    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, sg_list[0].count);
        
    /* cdb_operation setup */
    status = fbe_payload_cdb_set_parameters(payload_cdb_operation,  
                                            FBE_ESES_ENCLOSURE_ESES_PAGE_TIMEOUT,  // timeout
                                            FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE, //attribute
                                            FBE_PAYLOAD_CDB_FLAGS_DATA_IN,  // flag
                                            FBE_SCSI_READ_BUFFER_CDB_SIZE); // The cdb length

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_ERROR,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                           "%s, fbe_payload_cdb_set_parameters failed.\n",
                                           __FUNCTION__);
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED);
        fbe_transport_complete_packet(packet);
        return status;
    } 

    status = fbe_payload_cdb_operation_get_cdb(payload_cdb_operation, &cdb);
    
    /* Details of the CDB belongs to eses library */
    status = fbe_eses_build_read_buf_cdb(cdb, 
                                         FBE_SCSI_READ_BUFFER_CDB_SIZE, // The size of CDB.
                                         FBE_ESES_READ_BUF_MODE_DATA,   //read_buf_cmd_p->mode, // The mode of the read buffer command.
                                         buf_id,    //read_buf_cmd_p->buf_id, // The id of the buffer to be read.
                                         0,         //read_buf_cmd_p->buf_offset, // The byte offset inot a buffer described by buffer id. not used in all modes. 
                                         sizeof(sps_eeprom_info_struct));   // The number of bytes the client allocated for the response.
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_ERROR,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                           "%s, fbe_eses_build_write_buf_cdb failed.\n",
                                           __FUNCTION__);
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED);
        fbe_transport_complete_packet(packet);
        return status;
    } 

    return status;
} // fbe_eses_enclosure_setup_sps_eeprom_cmd

/*!*************************************************************************
* @fn fbe_eses_enclosure_setup_mode_sense_cmd(fbe_eses_enclosure_t * eses_enclosure, 
*                                       fbe_packet_t * packet,
*                                       fbe_payload_cdb_operation_t * payload_cdb_operation)                                                                                
***************************************************************************
* @brief
*       This function sets up the "mode sense (10 bytes)" command.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
* @param     payload_cdb_operation - The pointer to the payload cdb operation.
*
* @return   fbe_status_t.
*        FBE_STATUS_OK  - no error found.
*        otherwise - error is found.
*
* NOTES
*
* HISTORY
*   02-Dec-2008 PHE - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_setup_mode_sense_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                        fbe_packet_t * packet,
                                        fbe_payload_cdb_operation_t * payload_cdb_operation)                                                             
{
    fbe_payload_ex_t * payload = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_u8_t * cdb = NULL;
    fbe_status_t status;

    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n",__FUNCTION__);
    
    payload = fbe_transport_get_payload_ex(packet); 
   
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    sg_list[0].count = FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE;
    sg_list[0].address = (fbe_u8_t *)sg_list + (2 * sizeof(fbe_sg_element_t));

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    // Update the sg_count.
    fbe_payload_ex_set_sg_list(payload, sg_list, 1);
    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE);

    /* cdb_operation setup */
    status = fbe_payload_cdb_set_parameters(payload_cdb_operation,
                                   FBE_ESES_ENCLOSURE_ESES_PAGE_TIMEOUT,  // timeout
                                   FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE, //attribute
                                   FBE_PAYLOAD_CDB_FLAGS_DATA_IN,  // flag
                                   FBE_SCSI_MODE_SENSE_10_CDB_SIZE); // The cdb length

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_payload_cdb_set_parameters failed,  status: 0x%x.\n",
                                __FUNCTION__, status);
    
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED);
        fbe_transport_complete_packet(packet);
        return status;
    } 

    status = fbe_payload_cdb_operation_get_cdb(payload_cdb_operation, &cdb);
    
    /* details of the CDB belongs to eses library */
    status = fbe_eses_build_mode_sense_10_cdb(cdb, 
                                              FBE_SCSI_MODE_SENSE_10_CDB_SIZE, // size of CDB.
                                              FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE, // the maximum number of bytes to be returned.
                                              SES_PG_CODE_ALL_SUPPORTED_MODE_PGS);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                 "%s, build_mode_sense_10_cdb failed, status: 0x%x.\n",
                                __FUNCTION__,  status);
    
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED);
        fbe_transport_complete_packet(packet);
        return status;
    }  

    return status;
} // End of function fbe_eses_enclosure_setup_mode_sense_cmd

/*!*************************************************************************
* @fn fbe_eses_enclosure_setup_mode_select_cmd(fbe_eses_enclosure_t * eses_enclosure, 
*                                        fbe_packet_t * packet,
*                                        fbe_payload_cdb_operation_t * payload_cdb_operation)
***************************************************************************
* @brief
*   This function sets up the "mode select" command.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
* @param     payload_cdb_operation - The pointer to the payload cdb operation.
*
* @return   fbe_status_t.
*        FBE_STATUS_OK  - no error found.
*        otherwise - error is found.
*
* NOTES
*
* HISTORY
*   03-Dec-2008 PHE - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_setup_mode_select_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                        fbe_packet_t * packet,
                                        fbe_payload_cdb_operation_t * payload_cdb_operation)
{
    fbe_payload_ex_t       *payload = NULL;
    fbe_sg_element_t    *sg_list = NULL;
    fbe_u8_t            *cdb = NULL;
    fbe_u16_t           parameter_length = 0; // a field in the send diag command cdb
    fbe_u32_t           sg_count = 0; 
    fbe_status_t        status;

    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__ );    
    
    payload = fbe_transport_get_payload_ex(packet);
    
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    /* Build the mode param list. */
    fbe_eses_enclosure_build_mode_param_list(eses_enclosure,
                                                      sg_list, 
                                                      &parameter_length, // parameter_length is the page size.
                                                      &sg_count); 
    
    /* The new payload method does not include the last termination element */
    sg_count --;   
   
    // Update the sg_counet.
    status = fbe_payload_ex_set_sg_list(payload, sg_list, sg_count);
    
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_payload_ex_set_sg_list failed.\n",
                                __FUNCTION__ );
    
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PACKET_FAILED);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    /* cdb_operation setup */
    status = fbe_payload_cdb_set_parameters(payload_cdb_operation,
                                   FBE_ESES_ENCLOSURE_ESES_PAGE_TIMEOUT, // timeout
                                   FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE, //attribute
                                   FBE_PAYLOAD_CDB_FLAGS_DATA_OUT,  // flag
                                   FBE_SCSI_MODE_SELECT_10_CDB_SIZE); // The cdb length

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_payload_cdb_set_parameters failed, status: 0x%x.\n",
                                __FUNCTION__, status);
    
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED);
        fbe_transport_complete_packet(packet);
        return status;
    } 

    status = fbe_payload_cdb_operation_get_cdb(payload_cdb_operation, &cdb);
    
    /* details of the CDB belongs to eses library */
    status = fbe_eses_build_mode_select_10_cdb(cdb, FBE_SCSI_MODE_SELECT_10_CDB_SIZE, parameter_length);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, build_mode_select_10_cdb failed, status: 0x%x.\n",
                                __FUNCTION__,  status);
    
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED);
        fbe_transport_complete_packet(packet);
        return status;
    } 

    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, parameter_length);

    return status;

} // End of function fbe_eses_enclosure_setup_mode_select_cmd

/**************************************************************************
* fbe_eses_enclosure_handle_scsi_cmd_response()                  
***************************************************************************
*
* DESCRIPTION
*       This function checks the following status and handle the error status of the SCSI command.
*       (1) the IO request status
*       (2) the SCSI command status 
*       It can only be called when the packet status is OK.
*       
* PARAMETERS
*       eses_enclosure - The pointer to the ESES enclosure.
*       packet - The pointer to the packet.
*       page_opcode - The ESES page code.
*       scsi_error_code_p - The pointer to the scsi error code.
*                           If the SCSI status code of the response is check condition or reservation conflict 
*                           which has the sense data, the scsi_error_code is the sense data code(scsi_cc_code).
*                           Otherwise, the returned scsi_error_code is FBE_SCSI_CC_NOERR.
*
* RETURN VALUES
*       fbe_enclosure_status_t -
*       FBE_ENCLOSURE_STATUS_OK - The command completed successfully.
*       otherwise, return other status. 
*           
* NOTES
*      Because the RECEIVE DIAGNOSTIC RESULTS command and SEND DIAGNOSTIC command could have the same page code,
*      we need the scsi commmand and page code to know which page to be handled.
*
* HISTORY
*   17-Sep-2008 PHE - Created.
*   30-Sept-2008 NC - use payload
***************************************************************************/
fbe_enclosure_status_t
fbe_eses_enclosure_handle_scsi_cmd_response(fbe_eses_enclosure_t * eses_enclosure, 
                                                   fbe_packet_t * packet,
                                                   fbe_eses_ctrl_opcode_t opcode,
                                                   fbe_scsi_error_code_t * scsi_error_code_p)
{
    fbe_status_t                     status = FBE_STATUS_OK;
    fbe_enclosure_status_t           encl_status = FBE_ENCLOSURE_STATUS_OK;    
    fbe_payload_cdb_scsi_status_t    scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD;        
    fbe_port_request_status_t payload_request_status = FBE_PORT_REQUEST_STATUS_ERROR;
    fbe_u8_t                       * sense_data = NULL;
    fbe_payload_ex_t                  * payload = NULL;
    fbe_payload_cdb_operation_t    * payload_cdb_operation = NULL;
    fbe_scsi_error_code_t            scsi_error_code = FBE_SCSI_CC_NOERR;

    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure, 
                                FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "%s  entry, opcode: %s (0x%x). \n",
                                __FUNCTION__, 
                                fbe_eses_enclosure_print_scsi_opcode(opcode),
                                opcode);

    /* Check the SCSI status. */
    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);    
    fbe_payload_cdb_get_scsi_status(payload_cdb_operation, &scsi_status);

#ifdef ESES_FORCE_HARDCODE_VALUE
    // used for debugging the unsupported page. -- PINGHE
    if((opcode == FBE_ESES_CTRL_OPCODE_GET_ADDITIONAL_STATUS) || (opcode == FBE_ESES_CTRL_OPCODE_GET_EMC_SPECIFIC_STATUS))
    {
        scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION;    
    }
#endif
    if(scsi_status != FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD) 
    {
        /* Handle SCSI status */
        switch (scsi_status)
        {
            case FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION:
            case FBE_PAYLOAD_CDB_SCSI_STATUS_RESERVATION_CONFLICT:
                /* retrieve sense data buffer */
                fbe_payload_cdb_operation_get_sense_buffer(payload_cdb_operation, &sense_data);   
                
#ifdef ESES_FORCE_HARDCODE_VALUE
                // used for debugging the unsupported page. -- PINGHE
                if((opcode == FBE_ESES_CTRL_OPCODE_GET_ADDITIONAL_STATUS) || (opcode == FBE_ESES_CTRL_OPCODE_GET_EMC_SPECIFIC_STATUS))
                {
                    scsi_error_code = FBE_SCSI_CC_UNSUPPORTED_ENCL_FUNC;    
                }
                else
                {
                    /* Get the SCSI error code from the sense data. */
                    scsi_error_code = fbe_eses_enclosure_get_scsi_error_code(eses_enclosure, sense_data);  
                }
#else
                /* Get the SCSI error code from the sense data. */
                scsi_error_code = fbe_eses_enclosure_get_scsi_error_code(eses_enclosure, sense_data);  
#endif

                break;
            
            case FBE_PAYLOAD_CDB_SCSI_STATUS_BUSY:
            case FBE_PAYLOAD_CDB_SCSI_STATUS_TASK_SET_FULL:
            default:
                /* These status codes do not have the sense data. Set scsi_error_code to FBE_SCSI_DEVICE_BUSY. */
                scsi_error_code = FBE_SCSI_DEVICE_BUSY;
                break;
        } // End of - switch (scsi_status)

        /* Take the appropriate action according to the SCSI error code. */
        encl_status = fbe_eses_enclosure_handle_scsi_error_code(eses_enclosure, 
                                                                     opcode, 
                                                                     scsi_error_code);

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure, 
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                    "handleScsiCmdResp, op:%s (0x%x), badScsiStat:0x%x, scsiErr:0x%x\n",
                    fbe_eses_enclosure_print_scsi_opcode(opcode), 
                    opcode, scsi_status, scsi_error_code);

    } // End if - if(scsi_status != FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD) 
    else 
    {
        /* 
         * Check the payload cdb request status. 
         */
        status = fbe_payload_cdb_get_request_status(payload_cdb_operation, &payload_request_status);

        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "scsiCmdOp:%s (0x%x), getReqStat failed 0x%x.\n", 
                                fbe_eses_enclosure_print_scsi_opcode(opcode), opcode, status); 

            encl_status =  FBE_ENCLOSURE_STATUS_CDB_REQUEST_FAILED;            
        }
        else
        {
            switch (payload_request_status)
            {
            case FBE_PORT_REQUEST_STATUS_SUCCESS:
            case FBE_PORT_REQUEST_STATUS_DATA_UNDERRUN:
                // ignore underrun
                encl_status = FBE_ENCLOSURE_STATUS_OK;
                break;
            case FBE_PORT_REQUEST_STATUS_BUSY:
                encl_status = FBE_ENCLOSURE_STATUS_BUSY;
                break;
            default:
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "scsiCmdOp: %s (0x%x) failed, payloadStat 0x%x.\n", 
                                fbe_eses_enclosure_print_scsi_opcode(opcode), opcode, payload_request_status);

                encl_status =  FBE_ENCLOSURE_STATUS_CDB_REQUEST_FAILED;            
                break;
            }
        }
    }

    /* Update the SCSI error info when scsi_status or encl_status is not good */
    if((scsi_status != FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD) ||
        (encl_status != FBE_ENCLOSURE_STATUS_OK))
    {
        fbe_base_enclosure_set_scsi_error_info((fbe_base_enclosure_t *) eses_enclosure,
                                                                    scsi_error_code,
                                                                    scsi_status,
                                                                    payload_request_status,
                                                                    sense_data);
    }
   
    /* 
     * Check if scsi_cc_code_p is NULL pointer. If it is, the call does not need the scsi_cc_code.
     * Otherwise, need to return the scsi_cc_code.
     */
    if(scsi_error_code_p != NULL)
    {
        *scsi_error_code_p = scsi_error_code;
    }
    
    return encl_status;
} // End of - fbe_eses_enclosure_handle_scsi_cmd_response

/**************************************************************************
* fbe_eses_enclosure_get_scsi_error_code()                  
***************************************************************************
*
* DESCRIPTION
*       This function gets called when the SCSI status is check condition or reservation conflict.
*       It returns the SCSI error code based on the sense key, 
*       the SCSI additional sense code(ASC) and the SCSI additional sense code qualifier(ASCQ).
*       
* PARAMETERS
*       eses_enclosure - The pointer to the ESES enclosure.
*       sense_data - The pointer to the sense data block.
*
* RETURN VALUES
*       fbe_scsi_error_code_t - The SCSI check condition error code. 
*
* NOTES
*
* HISTORY
*   20-Oct-2008 PHE - Created.
***************************************************************************/
static fbe_scsi_error_code_t 
fbe_eses_enclosure_get_scsi_error_code(fbe_eses_enclosure_t * eses_enclosure, fbe_u8_t * sense_data)
{
    fbe_scsi_error_code_t  scsi_error_code = FBE_SCSI_CC_NOERR;
    fbe_scsi_sense_key_t sense_key = FBE_SCSI_SENSE_KEY_NO_SENSE; 
    fbe_scsi_additional_sense_code_t asc = FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO; 
    fbe_scsi_additional_sense_code_qualifier_t ascq = FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO;

    /********
     * BEGIN
     ********/

    /* Check if the response code is 70h or 71h. */
    if (((sense_data[0] & 0x7E) != 0x70))  
    {
        /* 
         * The response code is not 70h or 71h. 
         * The reponse is invalid. Do not continue to process the sense data.
         */
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                    "get_scsi_error_code, recv resp_code:0x%x, exp resp_code:0x70 or 0x71\n",
                                    sense_data[0]);
        return FBE_SCSI_CC_NOERR;
    }

    /* 
     * Extract the "Sense key", the "Additional Sense Code" (ASC), and the
     * "Additional Sense Code Qualifier" (ASCQ) from the sense data block.
     * The sense key is the last 4 least significant bits from the containing byte.
     * So get the sense key from the byte which contains it by masking out the unwanted bits. 
     */ 
    sense_key = sense_data[FBE_SCSI_SENSE_DATA_SENSE_KEY_OFFSET] & FBE_SCSI_SENSE_DATA_SENSE_KEY_MASK;
    asc = sense_data[FBE_SCSI_SENSE_DATA_ASC_OFFSET];
    ascq = sense_data[FBE_SCSI_SENSE_DATA_ASCQ_OFFSET];

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, sense_key: 0x%x, ASC/ASCQ: 0x%x/0x%x.\n",
                            __FUNCTION__,  sense_key, asc, ascq);
    
    switch(sense_key)
    {
        case FBE_SCSI_SENSE_KEY_NO_SENSE:
            scsi_error_code = FBE_SCSI_CC_NOERR;     
            break;
        
        case FBE_SCSI_SENSE_KEY_UNIT_ATTENTION:
            if((asc == FBE_SCSI_ASC_TARGET_OPERATING_CONDITIONS_HAVE_CHANGED) &&
                (ascq == FBE_SCSI_ASCQ_TARGET_OPERATING_CONDITIONS_HAVE_CHANGED))
            {
                /* 
                 * We got a unit attention with an ASC/ASCQ of 
                 * TARGET OPERATING CONDITIONS HAVE CHANGED.
                 */
                scsi_error_code = FBE_SCSI_CC_TARGET_OPERATING_COND_CHANGED;
            }
            else if((asc == FBE_SCSI_ASC_POWER_ON_RESET_BUS_DEVICE_RESET_OCCURRED) &&
                    (ascq == FBE_SCSI_ASCQ_POWER_ON_RESET_BUS_DEVICE_RESET_OCCURRED))
            {
                scsi_error_code = FBE_SCSI_CC_DEV_RESET;
            }
            else if((asc == FBE_SCSI_ASC_MODE_PARAM_CHANGED) &&
                    (ascq == FBE_SCSI_ASCQ_MODE_PARAM_CHANGED))
            {
                scsi_error_code = FBE_SCSI_CC_MODE_PARAM_CHANGED;
            }
            else
            {
                scsi_error_code = FBE_SCSI_CC_UNIT_ATTENTION;
            }
            break;

        case FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST:
            if((asc == FBE_SCSI_ASC_UNSUPPORTED_ENCLOSURE_FUNCTION) &&
               (ascq == FBE_SCSI_ASCQ_UNSUPPORTED_ENCLOSURE_FUNCTION))
            {
                scsi_error_code = FBE_SCSI_CC_UNSUPPORTED_ENCL_FUNC;
            }
            else if((asc == FBE_SCSI_ASC_ENCLOSURE_SERVICES_TRANSFER_REFUSED) &&
                    (ascq == FBE_SCSI_ASCQ_ENCLOSURE_SERVICES_TRANSFER_REFUSED))
            {
                scsi_error_code = FBE_SCSI_CC_ENCLOSURE_SERVICES_TRANSFER_REFUSED;
            }
            else
            {
                scsi_error_code = FBE_SCSI_CC_ILLEGAL_REQUEST;
            }

            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, illegalRequest senseData[15-17]:0x%X, 0x%X, 0x%X.\n",
                            __FUNCTION__,  
                            sense_data[FBE_ESES_SENSE_DATA_ILLEGAL_REQUEST_START_OFFSET],
                            sense_data[FBE_ESES_SENSE_DATA_ILLEGAL_REQUEST_FILED_POINTER_FIRST_BYTE_OFFSET],
                            sense_data[FBE_ESES_SENSE_DATA_ILLEGAL_REQUEST_FILED_POINTER_SECOND_BYTE_OFFSET]);
            break;

        case FBE_SCSI_SENSE_KEY_RECOVERED_ERROR:
            if ((asc == FBE_SCSI_ASC_SCSI_INTFC_PARITY_ERR) &&
                (ascq == FBE_SCSI_ASCQ_BECOMING_READY))
            {
                scsi_error_code = FBE_SCSI_CC_HARDWARE_ERROR_PARITY;
            }
            else
            {
                scsi_error_code = FBE_SCSI_CC_HARDWARE_ERROR;
            }
            break;

        case FBE_SCSI_SENSE_KEY_NOT_READY: 
            scsi_error_code = FBE_SCSI_CC_NOT_READY;
            break;

        case FBE_SCSI_SENSE_KEY_HARDWARE_ERROR:  
            scsi_error_code = FBE_SCSI_CC_HARDWARE_ERROR;
            break;

        case FBE_SCSI_SENSE_KEY_ABORTED_CMD:
            scsi_error_code = FBE_SCSI_CC_ABORTED_CMD;
            break;

        default:
            scsi_error_code = FBE_SCSI_CC_UNEXPECTED_SENSE_KEY;
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_WARNING,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, unexpected sense_key: 0x%x, ASC/ASCQ: 0x%x/0x%x.\n",
                                    __FUNCTION__, sense_key, asc, ascq);
            
            break;
    }// End of - switch(sense_key)

    return scsi_error_code;
} // end fbe_eses_enclosure_get_scsi_error_code

/**************************************************************************
* fbe_eses_enclosure_handle_scsi_error_code()                  
***************************************************************************
*
* DESCRIPTION
*       This function handles the SCSI check condition code.
*       
* PARAMETERS
*       eses_enclosure - The pointer to the ESES enclosure.
*       opcode - The ESES control page code.
*       scsi_cc_code - The SCSI error code.
*
* RETURN VALUES
*       fbe_enclosure_status_t. 
*           FBE_ENCLOSURE_STATUS_ENCL_FUNC_UNSUPPORTED - unsupported enclosure function.
*           FBE_ENCLOSURE_STATUS_PARAMETER_INVALID - The invalid parameter.  
*           FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED - The lifecycle related failure.
*           FBE_ENCLOSURE_STATUS_CMD_FAILED  - Other failure.
*
* NOTES
*
* HISTORY
*   22-Sep-2008 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_handle_scsi_error_code(fbe_eses_enclosure_t * eses_enclosure,
                                            fbe_eses_ctrl_opcode_t opcode,
                                            fbe_scsi_error_code_t scsi_error_code)
{
    /* 
     * Initialize the encl_status to FBE_ENCLOSURE_STATUS_CMD_FAILED.
     * This function is only called when the SCSI status is check condition or reservation conflict.
     */
    fbe_enclosure_status_t      encl_status = FBE_ENCLOSURE_STATUS_CMD_FAILED;
    fbe_enclosure_status_t      tempEnclStatus;
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;   
    fbe_lifecycle_state_t lifecycle_state;

    /********
     * BEGIN
     ********/
  
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, opcode: 0x%x, scsi_error_code: 0x%x.\n",
                            __FUNCTION__,  opcode, scsi_error_code);

    switch(scsi_error_code)
    {
        case FBE_SCSI_CC_TARGET_OPERATING_COND_CHANGED:
        case FBE_SCSI_CC_DEV_RESET:
        case FBE_SCSI_CC_ENCLOSURE_SERVICES_TRANSFER_REFUSED:
           /* 
            * The configuration of the enclosure has changed or a reset has occurred.
            * The only way for the client to clear the unit attention on that I_T nexus is to request
            * another Configuration diagnostic page.
            * Do not process the status page. Set the configuration unknown condition
            * to read the configuration page.
            */

            status = fbe_lifecycle_get_state(&fbe_eses_enclosure_lifecycle_const,
                                    (fbe_base_object_t*)eses_enclosure,
                                    &lifecycle_state);

            if((status == FBE_STATUS_OK) && (lifecycle_state == FBE_LIFECYCLE_STATE_READY))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                    "Changing state from READY to ACTIVATE. Reason:Configuration changed or reset occurred.\n");
            }

            /* We don't need to set the Additional Status page and EMC specific page here. 
             * It will be set when the configuration page successfully completes.
             */
            encl_status = fbe_base_enclosure_set_lifecycle_cond(
                                &fbe_eses_enclosure_lifecycle_const,
                                (fbe_base_enclosure_t *)eses_enclosure,
                                FBE_ESES_ENCLOSURE_LIFECYCLE_COND_CONFIGURATION_UNKNOWN);

            if(!ENCL_STAT_OK(encl_status))            
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                    "%s, can't set configuration unknown condition, status: 0x%x.\n",
                                    __FUNCTION__, encl_status);

                encl_status = FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED;
            }  
            else
            {
                encl_status = FBE_ENCLOSURE_STATUS_CMD_FAILED;
            }
            break;

        case FBE_SCSI_CC_MODE_PARAM_CHANGED:
            /* 
             * Set the condition to do mode sense. 
             * The mode unselect condition will be set in the mode sense completion routine 
             * if mode sense command finds the mode select command is needed. 
             */
            encl_status = fbe_base_enclosure_set_lifecycle_cond(
                                &fbe_eses_enclosure_lifecycle_const,
                                (fbe_base_enclosure_t *)eses_enclosure,
                                FBE_ESES_ENCLOSURE_LIFECYCLE_COND_MODE_UNSENSED);

            if(!ENCL_STAT_OK(encl_status))            
            {
                /* Failed to set the mode unsensed condition. */
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                        "%s, can't set mode unsensed condition, status: 0x%x.\n",
                                        __FUNCTION__, encl_status);

                encl_status = FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED;
            }  
            else
            {
                encl_status = FBE_ENCLOSURE_STATUS_CMD_FAILED;
            }
            break;

        case FBE_SCSI_CC_UNSUPPORTED_ENCL_FUNC:
            /*
            * At the early hardware integration, the hardware does not support additional status and EMC specific status page.
            * We will use the hardcode page.
            */
            encl_status = FBE_ENCLOSURE_STATUS_ENCL_FUNC_UNSUPPORTED;    
            if(opcode == FBE_ESES_CTRL_OPCODE_GET_ADDITIONAL_STATUS)
            {
                /* 
                 * Handle the unsupported additional status diagnostic page.
                 * Whether it is handled successfully or not, it should not change the returning encl_status in this function.
                 */
                encl_status = fbe_eses_enclosure_handle_unsupported_additional_status_page(eses_enclosure);
                if (encl_status != FBE_ENCLOSURE_STATUS_UNSUPPORTED_PAGE_HANDLED) 
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "handle_unsupported_additional_status_page failed, encl_status: 0x%x.\n",
                                            encl_status);
                    
                }    
            }
            else if(opcode == FBE_ESES_CTRL_OPCODE_GET_EMC_SPECIFIC_STATUS)
            {
                /*
                 * Handle the unsupported EMC specific status diagnostic page.
                 * Whether it is handled successfully or not, it should not change the returning encl_status in this function.
                 */
                encl_status = fbe_eses_enclosure_handle_unsupported_emc_specific_status_page(eses_enclosure);
                if (encl_status != FBE_ENCLOSURE_STATUS_UNSUPPORTED_PAGE_HANDLED) 
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "handle_unsupported_additional_status_page failed, encl_status: 0x%x.\n",
                                            encl_status);
                    
                } 
                else
                {
                    if(eses_enclosure->enclosureConfigBeingUpdated == TRUE)
                    {

                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                     FBE_TRACE_LEVEL_INFO,
                                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                                     "handle_scsi_error_code, End of mapping.\n");

                        eses_enclosure->enclosureConfigBeingUpdated = FALSE;

                        /* set active_edal_valid = TRUE*/                       
                       fbe_base_enclosure_set_activeEdalValid((fbe_base_enclosure_t *) eses_enclosure, TRUE);
                        
                        tempEnclStatus = 
                            fbe_base_enclosure_edal_incrementStateChangeCount((fbe_base_enclosure_t *)eses_enclosure);
                        if(tempEnclStatus != FBE_ENCLOSURE_STATUS_OK)
                        {
                            fbe_base_object_customizable_trace(
                                (fbe_base_object_t*)eses_enclosure, 
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "HndlScsiErrCd, stateChangeCount increment failed, tempEnclStatus: 0x%x.\n",
                                tempEnclStatus);
                            return encl_status;
                        }        
                    }
                }
            }
            else if(opcode == FBE_ESES_CTRL_OPCODE_MODE_SENSE)
            {
                // Assume the enclosure default mode page is what we want.
                // So we do not need to do any processing here.
                /* Set the page unsupported */
                encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_ENCL_MODE_SENSE_UNSUPPORTED, // Attribute.
                                                                FBE_ENCL_ENCLOSURE,  // component type.
                                                                0, // only 1 enclosure
                                                                TRUE);  // The value to be set to.

                if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    return encl_status;
                }        

                /* Whether it is successful or not to set the edal attribute, it should be affect the 
                 * return value; 
                 */
                encl_status = FBE_ENCLOSURE_STATUS_UNSUPPORTED_PAGE_HANDLED;
                
            }
            else if(opcode == FBE_ESES_CTRL_OPCODE_MODE_SELECT)
            {
                // Assume the enclosure default mode page is what we want.
                // So we do not need to do any processing here.
                /* Set the page unsupported */
                encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_ENCL_MODE_SELECT_UNSUPPORTED, // Attribute.
                                                                FBE_ENCL_ENCLOSURE,  // component type.
                                                                0, // only 1 enclosure
                                                                TRUE);  // The value to be set to.
                if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    return encl_status;
                }        

                /* Whether it is successful or not to set the edal attribute, it should be affect the 
                 * return value; 
                 */
                encl_status = FBE_ENCLOSURE_STATUS_UNSUPPORTED_PAGE_HANDLED;
            
            }
            break;

        case FBE_SCSI_CC_ILLEGAL_REQUEST:
            /* Set the encl_status to FBE_ENCLOSURE_STATUS_ILLEGAL_REQUEST to indicate the reason that the command failed. */
            encl_status = FBE_ENCLOSURE_STATUS_ILLEGAL_REQUEST;
            break;

        case FBE_SCSI_CC_HARDWARE_ERROR:
            encl_status = FBE_ENCLOSURE_STATUS_HARDWARE_ERROR;
            break;

        case FBE_SCSI_CC_HARDWARE_ERROR_PARITY:
            encl_status = FBE_ENCLOSURE_STATUS_CHECKSUM_ERROR;
            break;

        case FBE_SCSI_DEVICE_BUSY:
            encl_status = FBE_ENCLOSURE_STATUS_BUSY;
            break;

        case FBE_SCSI_CC_NOT_READY:
        case FBE_SCSI_CC_ABORTED_CMD:
        case FBE_SCSI_CC_UNIT_ATTENTION:
        case FBE_SCSI_CC_NOERR:  // invalid response code or no sense key.
        default: 
            encl_status = FBE_ENCLOSURE_STATUS_CMD_FAILED;
            break;
    } // End of - switch(scsi_cc_code)

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure, 
                           FBE_TRACE_LEVEL_INFO,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                           "handle_scsi_error_code,opcode 0x%x,scsiErrorCode 0x%x,enclStatus 0x%x.\n",
                           opcode, scsi_error_code, encl_status);


    return encl_status ;
}// End of function fbe_eses_enclosure_handle_scsi_error_code

/*!*************************************************************************
* @fn fbe_eses_enclosure_handle_unsupported_additional_status_page(
*                                           fbe_eses_enclosure_t * eses_enclosure)                 
***************************************************************************
*
* @brief
*       This function is called when the addition page functionality is not supported.
*       It hard-codes the configuration/mapping information which is supposed to be provided 
*       by the additional status page.
*
* @param   eses_enclosure - The pointer to the eses enclosure.
*
* @return  fbe_enclosure_status_t.
*
* NOTES
*
* HISTORY
*   17-Sep-2008 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_handle_unsupported_additional_status_page(fbe_eses_enclosure_t * eses_enclosure)
{
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_edal_status_t   edalStatus;

    fbe_u8_t     drive_elem_index = SES_ELEM_INDEX_NONE;
    fbe_u8_t     local_expander_elem_index = SES_ELEM_INDEX_NONE;
    fbe_u8_t     connector_elem_index = SES_ELEM_INDEX_NONE;
    fbe_u8_t     local_connector_elem_index = SES_ELEM_INDEX_NONE;
    fbe_u8_t     peer_connector_elem_index = SES_ELEM_INDEX_NONE;
    fbe_u8_t     local_lcc_side_id = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_sas_address_t expander_sas_address = FBE_SAS_ADDRESS_INVALID;
    fbe_sas_address_t local_expander_sas_address = FBE_SAS_ADDRESS_INVALID;
    ses_elem_type_enum elem_type = SES_ELEM_TYPE_INVALID;
    fbe_u8_t    first_elem_index = SES_ELEM_INDEX_NONE;

    fbe_u8_t     group_id = 0;
    fbe_u8_t     i = 0;  // for the loops
    fbe_eses_elem_group_t * elem_group = NULL;
    fbe_edal_general_comp_handle_t specific_component_data_ptr = NULL;
#ifndef ESES_FORCE_HARDCODE_VALUE
    fbe_enclosure_component_block_t * encl_component_block = NULL;
#endif
    fbe_bool_t   is_local_connector = FALSE;
    fbe_bool_t   is_entire_connector = FALSE;
    fbe_u8_t     number_of_connectors_per_lcc = 0;
    fbe_u8_t     number_of_lanes_per_entire_connector = 0;
    fbe_u8_t     entire_connector_index = FBE_ENCLOSURE_VALUE_INVALID;  // The index in EDAL.
    fbe_u8_t     container_index = FBE_ENCLOSURE_VALUE_INVALID;  // The index in EDAL.
    fbe_u8_t     phy_index = FBE_ENCLOSURE_VALUE_INVALID; 

    /***********
     * BEGIN
     ***********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__);

    /* Set the page unsupported */
    encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                        FBE_ENCL_ADDITIONAL_STATUS_PAGE_UNSUPPORTED, // Attribute.
                                        FBE_ENCL_ENCLOSURE,  // component type.
                                        0, // only 1 enclosure
                                        TRUE);  // The value to be set to.

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);

    for(group_id = 0; group_id < fbe_eses_enclosure_get_number_of_actual_elem_groups(eses_enclosure); group_id ++ )
    {  
        elem_type = fbe_eses_elem_group_get_elem_type(elem_group, group_id);
        first_elem_index = fbe_eses_elem_group_get_first_elem_index(elem_group, group_id);
        
        switch(elem_type)
        {
            case SES_ELEM_TYPE_ARRAY_DEV_SLOT:
                if(fbe_eses_elem_group_get_subencl_id(elem_group, group_id) == SES_SUBENCL_ID_PRIMARY)
                {
                    // Get first drive element index. There is only one array device device slot(drive slot) group on local LCC for ESES enclosure.
                    drive_elem_index = first_elem_index; 
                }
                break;

            case SES_ELEM_TYPE_SAS_EXP:  
                if(fbe_eses_elem_group_get_subencl_id(elem_group, group_id) == SES_SUBENCL_ID_PRIMARY)
                {
                    //Get the expander element index. There is only one expander on local LCC for ESES enclosure.
                    local_expander_elem_index = first_elem_index; 
                }
                break;

            case SES_ELEM_TYPE_SAS_CONN:   
                if(fbe_eses_elem_group_get_subencl_id(elem_group, group_id) == SES_SUBENCL_ID_PRIMARY)
                {
                    // Get the first connector element index. There is only one connector group on local LCC for ESES enclosure. 
                    local_connector_elem_index = first_elem_index;  
                }
                else
                {
                    peer_connector_elem_index = first_elem_index;
                }
                break;

            default:
                break;
        }// End of - switch(elem_type)
       
    }// End of - for(group_id = 0; group_id < fbe_eses_enclosure_get_number_of_actual_elem_groups(eses_enclosure); group_id ++ ).

    if((drive_elem_index == SES_ELEM_INDEX_NONE) /*||      
       (local_connector_elem_index == SES_ELEM_INDEX_NONE)*/)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, invalid config.\n", 
                                __FUNCTION__);  
        

        /* One or more of these element indexes is invalid. We can not continue to hardcode other config info. */ 
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "drive_elem_index: %d, local_connector_elem_index: %d.\n", 
                                drive_elem_index, local_connector_elem_index);  
       
        return FBE_ENCLOSURE_STATUS_CONFIG_INVALID;    
    }

    // Get the side id of the local LCC.
    encl_status = fbe_eses_enclosure_get_local_lcc_side_id(eses_enclosure, 
                                                          &local_lcc_side_id);
       
    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }   

    // DRIVE: Hardcode the drive element index, drive slot to phy index mapping for drive info.
    for(i = 0; i< fbe_eses_enclosure_get_number_of_slots(eses_enclosure); i++)
    {
        status = fbe_base_enclosure_access_specific_component((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_DRIVE_SLOT, // component type
                                                    i,  //Index of the component 
                                                    &specific_component_data_ptr);

        if(status != FBE_STATUS_OK)
        {
            return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
        }

        // Assume the drive element index is in the order of the drive slot. 
        edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                                  FBE_ENCL_COMP_ELEM_INDEX,  // Attribute
                                                  FBE_ENCL_DRIVE_SLOT,    // Component type
                                                  drive_elem_index + i); // The value to be set to   
        if(edalStatus != FBE_EDAL_STATUS_OK)
        {
            return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
        }

        // Assume the drive element index is in the order of the drive slot. 
        // TO DO: we need to fill in the correct slot for voyager.
        edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                                  FBE_ENCL_DRIVE_SLOT_NUMBER,  // Attribute
                                                  FBE_ENCL_DRIVE_SLOT,    // Component type
                                                  i); // The value to be set to   
        if(edalStatus != FBE_EDAL_STATUS_OK)
        {
            return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
        }

        edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                                  FBE_ENCL_DRIVE_PHY_INDEX,  // Attribute
                                                  FBE_ENCL_DRIVE_SLOT,    // Component type
                                                  fbe_eses_enclosure_get_drive_slot_to_phy_index_mapping(eses_enclosure,local_lcc_side_id,i)); // The value to be set to 
        if(edalStatus != FBE_EDAL_STATUS_OK)
        {
            return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
        }
    }
   
    // PHY: Hardcode the element index of the expander which contains the phy and the phy id of the phy.
    for(i = 0; i< fbe_eses_enclosure_get_number_of_expander_phys(eses_enclosure); i++)
    {
        status = fbe_base_enclosure_access_specific_component((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_EXPANDER_PHY,  // component type
                                                        i,  //Index of the component 
                                                        &specific_component_data_ptr);

        if(status != FBE_STATUS_OK)
        {
            return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
        }
            
        edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                                  FBE_ENCL_EXP_PHY_EXPANDER_ELEM_INDEX,  // Attribute
                                                  FBE_ENCL_EXPANDER_PHY,    // Component type
                                                  local_expander_elem_index); // The value to be set to

        if(edalStatus != FBE_EDAL_STATUS_OK)
        {
            return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
        }

        edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                                  FBE_ENCL_EXP_PHY_ID,  // Attribute
                                                  FBE_ENCL_EXPANDER_PHY,    // Component type
                                                  fbe_eses_enclosure_get_phy_index_to_phy_id_mapping(eses_enclosure,i)); // The value to be set to

        if(edalStatus != FBE_EDAL_STATUS_OK)
        {
            return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
        }       
    }
 
    // Get the expander sas address on local LCC.
    // There is only one expander per LCC for the current configuraiton.
    // It might change in the future.
    status = fbe_sas_enclosure_get_sas_address((fbe_sas_enclosure_t *)eses_enclosure, &local_expander_sas_address);

    // CONNECTOR: Hardcode the connector element index and containing expander sas address for the connectors. 
    number_of_connectors_per_lcc = fbe_eses_enclosure_get_number_of_connectors_per_lcc(eses_enclosure);
    number_of_lanes_per_entire_connector = fbe_eses_enclosure_get_number_of_lanes_per_entire_connector(eses_enclosure);
    for(i = 0; i< fbe_eses_enclosure_get_number_of_connectors(eses_enclosure); i++)
    {  
        if(((i< number_of_connectors_per_lcc) && 
            (local_lcc_side_id == FBE_ESES_ENCLOSURE_LCC_A)) ||
           ((i>= number_of_connectors_per_lcc) && 
            (local_lcc_side_id == FBE_ESES_ENCLOSURE_LCC_B))) 
        {        
            // The connector is on local LCC. 
            is_local_connector = TRUE;
            connector_elem_index = local_connector_elem_index + (i % number_of_connectors_per_lcc);
            expander_sas_address = local_expander_sas_address;  
            // phy_index is only valid for local connectors.
            if(local_lcc_side_id == FBE_ESES_ENCLOSURE_LCC_A)
            {
                phy_index = fbe_eses_enclosure_get_connector_index_to_phy_index_mapping(eses_enclosure,i);
            }
            else
            {
                phy_index = fbe_eses_enclosure_get_connector_index_to_phy_index_mapping(eses_enclosure,
                                                                        (i - number_of_connectors_per_lcc));
            }
        }
        else
        {                
            // The connector is on peer LCC. We don't care the info for the connector on peer LCC.
            is_local_connector = FALSE;
            connector_elem_index = peer_connector_elem_index + (i % number_of_connectors_per_lcc);
            expander_sas_address = FBE_SAS_ADDRESS_INVALID;  
            // phy_index is only valid for local connectors.
            phy_index = FBE_ENCLOSURE_VALUE_INVALID; 
        }          

        status = fbe_base_enclosure_access_specific_component((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_CONNECTOR,  // component type
                                                    i,  //Index of the component 
                                                    &specific_component_data_ptr);

        if(status != FBE_STATUS_OK)
        {
            return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
        } 

        /* Set the local/peer flag.*/
        edalStatus = eses_enclosure_access_setBool(specific_component_data_ptr,
                                                    FBE_ENCL_COMP_IS_LOCAL,  // Attribute
                                                    FBE_ENCL_CONNECTOR,    // Component type
                                                    is_local_connector); // The value to be set to
        
        if(edalStatus != FBE_EDAL_STATUS_OK)
        {
            return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
        }

        /* Set the element index for the connector. */
        edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                                    FBE_ENCL_COMP_ELEM_INDEX,  // Attribute
                                                    FBE_ENCL_CONNECTOR,    // Component type
                                                    connector_elem_index); // The value to be set to

        if(edalStatus != FBE_EDAL_STATUS_OK)
        {
            return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
        }

        /* Set the expander sas address for the connector. */
        edalStatus = eses_enclosure_access_setU64(specific_component_data_ptr,
                                                    FBE_ENCL_CONNECTOR_EXP_SAS_ADDRESS,  // Attribute
                                                    FBE_ENCL_CONNECTOR,    // Component type
                                                    expander_sas_address); // The value to be set to

        if(edalStatus != FBE_EDAL_STATUS_OK)
        {
            return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
        }  

        // Assume the 1st and the 6th elements in the group are the elements of the entire connectors.
        if((i%number_of_connectors_per_lcc) <= number_of_lanes_per_entire_connector)
        {  
            entire_connector_index = (i < number_of_connectors_per_lcc) ? 0 : number_of_connectors_per_lcc;
        }
        else
        {
            entire_connector_index = (i < number_of_connectors_per_lcc) ? 
                                      (1 + number_of_lanes_per_entire_connector) : 
                                      (1 + number_of_lanes_per_entire_connector + number_of_connectors_per_lcc);
        }

        if(i == entire_connector_index)
        {
            is_entire_connector = TRUE;
            container_index = FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED;
        }
        else
        {
            is_entire_connector = FALSE;
            container_index = entire_connector_index;
        }

        /* Set FBE_ENCL_COMP_CONTAINER_INDEX attribute for the connector. */
        edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                                    FBE_ENCL_COMP_CONTAINER_INDEX,  // Attribute
                                                    FBE_ENCL_CONNECTOR,    // Component type
                                                    container_index); // The value to be set to

        if(edalStatus != FBE_EDAL_STATUS_OK)
        {
            return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
        }
                  
        /* Set the is_entire_connector attribute for the connector. */                        
        edalStatus = eses_enclosure_access_setBool(specific_component_data_ptr,
                                                FBE_ENCL_CONNECTOR_IS_ENTIRE_CONNECTOR,  // Attribute
                                                FBE_ENCL_CONNECTOR,    // Component type
                                                is_entire_connector); // The value to be set to

        if(edalStatus != FBE_EDAL_STATUS_OK)
        {
            return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
        }   

      
        edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                                  FBE_ENCL_CONNECTOR_PHY_INDEX,  // Attribute
                                                  FBE_ENCL_CONNECTOR,    // Component type
                                                  phy_index); // The value to be set to 
     
        if(edalStatus != FBE_EDAL_STATUS_OK)
        {
            return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
        }        
    }

    // EXPANDER: There is only one expander on local LCC for ESES???
    status = fbe_sas_enclosure_get_sas_address((fbe_sas_enclosure_t *)eses_enclosure, &expander_sas_address);
    
    status = fbe_base_enclosure_access_specific_component((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_EXPANDER,  // component type
                                                    local_lcc_side_id,  // Index of the component 
                                                    &specific_component_data_ptr);

    edalStatus = eses_enclosure_access_setU64(specific_component_data_ptr,
                                               FBE_ENCL_EXP_SAS_ADDRESS,  // Attribute
                                               FBE_ENCL_EXPANDER,    // Component type
                                               local_expander_sas_address); // The value to be set to

    if(edalStatus != FBE_EDAL_STATUS_OK)
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    } 

    //The expander element index is set while processing the configuration page. So we don't need to 
    //set it here.

#ifndef ESES_FORCE_HARDCODE_VALUE
    fbe_base_enclosure_get_component_block_ptr((fbe_base_enclosure_t *)eses_enclosure, &encl_component_block);
    fbe_edal_printAllComponentData(encl_component_block, fbe_edal_trace_func);
#endif

    return FBE_ENCLOSURE_STATUS_UNSUPPORTED_PAGE_HANDLED;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_handle_unsupported_emc_specific_status_page(
*                                      fbe_eses_enclosure_t * eses_enclosure)                  
***************************************************************************
*
* @brief
*       This function is called when the EMC specific status page functionality 
*       is not supported. It hard-codes the configure/mapping information 
*       which is supposed to be provided by the EMC specific status page.
*
* @param   eses_enclosure - The pointer to the eses enclosure.
*
* @return  fbe_enclosure_status_t.
*
* NOTES
*
* HISTORY
*   17-Sep-2008 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_handle_unsupported_emc_specific_status_page(fbe_eses_enclosure_t * eses_enclosure)
{
    fbe_u8_t local_lcc_side_id = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t number_of_connectors_per_lcc = 0;
    fbe_u8_t number_of_lanes_per_entire_connector = 0;
    fbe_u8_t connector_index = 0;
    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__ );
    
    /* Set the page unsupported */
    encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                        FBE_ENCL_EMC_SPECIFIC_STATUS_PAGE_UNSUPPORTED, // Attribute.
                                        FBE_ENCL_ENCLOSURE,  // component type.
                                        0, // only 1 enclosure
                                        TRUE);  // The value to be set to.

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    // Get the enclosure local lcc side id.
    encl_status = fbe_eses_enclosure_get_local_lcc_side_id(eses_enclosure, 
                                                          &local_lcc_side_id);
   
    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }
  
    number_of_connectors_per_lcc = fbe_eses_enclosure_get_number_of_connectors_per_lcc(eses_enclosure);
    number_of_lanes_per_entire_connector = fbe_eses_enclosure_get_number_of_lanes_per_entire_connector(eses_enclosure);
    if(local_lcc_side_id == FBE_ESES_ENCLOSURE_LCC_A)
    {
        // LCC A ( side_id = 0) is the local LCC.
        // The phys attached to the primary port connector are described 
        // after those phys attached to the expansion port connector in the additional status page.
        eses_enclosure->primary_port_entire_connector_index[0] = 1 + number_of_lanes_per_entire_connector;
    }
    else
    {
        // LCC B ( side_id = 1) is the local LCC.
        // The phys attached to the primary port connector are described 
        // after those phys attached to the expansion port connector in the additional status page.
        eses_enclosure->primary_port_entire_connector_index[0] = 1 + number_of_lanes_per_entire_connector + number_of_connectors_per_lcc;
    }

    encl_status = fbe_eses_enclosure_populate_connector_primary_port(eses_enclosure);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "populate_primary_port failed, primary port entire conn index:%d, encl_status: 0x%x.\n", 
                                eses_enclosure->primary_port_entire_connector_index[0],
                                encl_status);

        return encl_status;
    }

    for(connector_index = 0; 
          connector_index < fbe_eses_enclosure_get_number_of_connectors(eses_enclosure);
          connector_index ++)
    {
        encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_CONNECTOR_ATTACHED_SUB_ENCL_ID,  // Attribute
                                            FBE_ENCL_CONNECTOR,    // Component type
                                            connector_index,  //Index of the component 
                                            FBE_ESES_SUBENCL_SIDE_ID_INVALID); // The pointer to the return value
                                            
        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
             fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                FBE_TRACE_LEVEL_ERROR,
                                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                "Failed to set connector_sub_encl_id, connector_index:%d, encl_status: 0x%x.\n", 
                                                connector_index,
                                                encl_status);
             return encl_status;
        }       
    }
    return FBE_ENCLOSURE_STATUS_UNSUPPORTED_PAGE_HANDLED;
} // End of function fbe_eses_enclosure_handle_unsupported_emc_specific_status_page

/*!*************************************************************************
* @fn fbe_eses_enclosure_handle_get_config_response(fbe_eses_enclosure_t * eses_enclosure, 
*                                                   fbe_sg_element_t * sg_list)                  
***************************************************************************
* @brief
*       This function handles the response got from the "receive diag" command 
*       with the "get configuration" opcode.
*       It calls the function to parse the configuration page and set the 
*       conditions if needed.
*
* @param eses_enclosure - The pointer to the ESES enclosure.
* @param sg_list - the pointer to the sg list allocated while sending the command.
*
* @return  fbe_enclosure_status_t.
*      FBE_ENCLOSURE_STATUS_OK - no error.
*      otherwise - error is found.
*    
* NOTES
*
* HISTORY
*   28-Aug-2008 PHE - Created.
*   30-Sept-2008 NC - use payload
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_handle_get_config_response(fbe_eses_enclosure_t * eses_enclosure, 
                                              fbe_sg_element_t * sg_list)
{
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__ );  
    
    /* we don't have to set these conditions because we expect an enclosure will 
     * be in activate state when it's doing a config page, and the other conditions 
     * are preset for activate state.
     */
     
    encl_status = fbe_eses_enclosure_process_configuration_page(eses_enclosure, 
                                                (fbe_u8_t *)sg_list[0].address);
    

    if(!ENCL_STAT_OK(encl_status))
    {
        // Processing configuration page failed.
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "process_configuration_page failed, encl_status: 0x%x.\n",
                                encl_status); 
        
    }

    return encl_status;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_handle_get_sas_encl_type_response(fbe_eses_enclosure_t * eses_enclosure, 
*                                                   fbe_sg_element_t * sg_list)                  
***************************************************************************
* @brief
*       This function handles the response got from the "receive diag" command 
*       with the "get sas encl type" opcode.
*       It calls the function to parse the configuration page to get the
*       primary subencl product id.
*
* @param eses_enclosure - The pointer to the ESES enclosure.
* @param sg_list - the pointer to the sg list allocated while sending the command.
*
* @return  fbe_enclosure_status_t.
*      FBE_ENCLOSURE_STATUS_OK - no error.
*      otherwise - error is found.
*    
* NOTES
*
* HISTORY
*   23-Nov-2013 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_handle_get_sas_encl_type_response(fbe_eses_enclosure_t * eses_enclosure, 
                                              fbe_sg_element_t * sg_list)
{
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__ );  
    
    /* we don't have to set these conditions because we expect an enclosure will 
     * be in activate state when it's doing a config page, and the other conditions 
     * are preset for activate state.
     */
     
    encl_status = fbe_eses_enclosure_get_sas_encl_type_from_config_page(eses_enclosure, 
                                                (ses_pg_config_struct *)sg_list[0].address);
    

    if(!ENCL_STAT_OK(encl_status))
    {
        // Processing configuration page failed.
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "get_sas_encl_type_from_config_page failed, encl_status: 0x%x.\n",
                                encl_status); 
        
    }

    return encl_status;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_handle_get_status_response(fbe_eses_enclosure_t * eses_enclosure, 
*                                                   fbe_sg_element_t * sg_list)                  
***************************************************************************
* @brief
*       This function handles the response got from the "receive diag" command 
*       with the "get status" opcode.
*       It calls the function to parse the status page and set the 
*       conditions if needed.
*
* @param eses_enclosure - The pointer to the ESES enclosure.
* @param sg_list - the pointer to the sg list allocated while sending the command.
*
* @return  fbe_enclosure_status_t.
*      FBE_ENCLOSURE_STATUS_OK - no error.
*      otherwise - error is found.
*
* NOTES
*
* HISTORY
*   27-Jul-2008 PHE - Created.
*   30-Sept-2008 NC - use payload
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_handle_get_status_response(fbe_eses_enclosure_t * eses_enclosure, 
                                              fbe_sg_element_t * sg_list)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u64_t               timeOfLastGoodStatusPage;

    
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry \n", __FUNCTION__ );

    /*
     * use spin lock when updating Status to ensure we return consistent data (not partial updated)
     */
    fbe_spinlock_lock(&eses_enclosure->enclGetInfoLock);
    encl_status = fbe_eses_enclosure_process_status_page(eses_enclosure, 
                                                         (fbe_u8_t *)sg_list[0].address);
    fbe_spinlock_unlock(&eses_enclosure->enclGetInfoLock);
     
    /* Clean up the memeory allocated and set the packet status. */
    if(encl_status == FBE_ENCLOSURE_STATUS_OK)    
    {
        /*
         * Processing the status page succeeded.
         * Set the condition FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE
         */
        eses_enclosure->status_page_requiered = FALSE;

        // set timestamp for last successfully processed Status Page
        timeOfLastGoodStatusPage = fbe_get_time();
        encl_status = fbe_base_enclosure_edal_setU64((fbe_base_enclosure_t *)eses_enclosure,
                                                     FBE_ENCL_TIME_OF_LAST_GOOD_STATUS_PAGE,
                                                     FBE_ENCL_ENCLOSURE,
                                                     0,
                                                     timeOfLastGoodStatusPage);

        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            // trace failure, but continue processing
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s can't set timeOfLastGoodStatusPage, encl_status: 0x%X",
                                    __FUNCTION__, encl_status);
            
        }    

        /* We will assume that we found new element or lost some old element.
        In this case we should triger discovery update condition to take care of discovery edges.
        */

        status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                        (fbe_base_object_t*)eses_enclosure, 
                                        FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE);

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s can't set discovery update condition, status: 0x%X",
                                    __FUNCTION__, status);
             
            encl_status = FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED;
        }
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_WARNING,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s process_status_page failed, encl_status: 0x%x.\n",
                                __FUNCTION__, encl_status);
        
    }
    
    return encl_status;
} // End of - fbe_eses_enclosure_handle_get_status_response


/*!*************************************************************************
* @fn fbe_eses_enclosure_handle_get_addl_status_response(fbe_eses_enclosure_t * eses_enclosure, 
*                                                        fbe_sg_element_t * sg_list)                  
***************************************************************************
* @brief
*       This function handles the response got from the "receive diag" command 
*       with the "get additional status" opcode.
*       It calls the function to parse the additional status page.
*
* @param eses_enclosure - The pointer to the ESES enclosure.
* @param sg_list - the pointer to the sg list allocated while sending the command.
*
* @return  fbe_enclosure_status_t.
*      FBE_ENCLOSURE_STATUS_OK - no error.
*      otherwise - error is found.
*
* NOTES
*
* HISTORY
*   06-Aug-2008 PHE - Created.
*   30-Sept-2008 NC - use payload
***************************************************************************/
static fbe_enclosure_status_t fbe_eses_enclosure_handle_get_addl_status_response(fbe_eses_enclosure_t * eses_enclosure, 
                                                                                   fbe_sg_element_t * sg_list)
{
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__);
     
    encl_status = fbe_eses_enclosure_process_addl_status_page(eses_enclosure, 
                                                (fbe_u8_t *)sg_list[0].address);
    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s process_addl_status_page failed, encl_status: 0x%x.\n",
                                __FUNCTION__, encl_status);

        
        /* We may need to set other condition to recover this error. */
    }       
    
    return encl_status;
}


/*!*************************************************************************
* @fn fbe_eses_enclosure_handle_get_emc_specific_status_response(fbe_eses_enclosure_t * eses_enclosure, 
 *                                                        fbe_sg_element_t * sg_list)                 
***************************************************************************
* @brief
*       This function handles the response got from the "receive diag" command 
*       with the "get EMC specific status" opcode.
*       It calls the function to parse the EMC specific status page.
*
* @param eses_enclosure - The pointer to the ESES enclosure.
* @param sg_list - the pointer to the sg list allocated while sending the command.
*
* @return  fbe_enclosure_status_t.
*      FBE_ENCLOSURE_STATUS_OK - no error.
*      otherwise - error is found.
*
* NOTES
*
* HISTORY
*   06-Aug-2008 PHE - Created.
*   30-Sept-2008 NC - use payload
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_handle_get_emc_specific_status_response(fbe_eses_enclosure_t * eses_enclosure, 
                                                          fbe_sg_element_t * sg_list)
{
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__ );
 
    encl_status = fbe_eses_enclosure_process_emc_specific_page(eses_enclosure, 
                                                (ses_pg_emc_encl_stat_struct *)sg_list[0].address);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s fbe_eses_enclosure_process_emc_specific_status_page failed, encl_status: 0x%x.\n",
                                __FUNCTION__, encl_status); 
    

    }
    else
    {
        // if expander hasn't reported the upstream connector attached sas address, let's wait.
        if (eses_enclosure->primary_port_entire_connector_index[0] == FBE_ENCLOSURE_VALUE_INVALID)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_INFO,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                           "handle_get_emc_spec_status_resp: no attached upstream enclosure reported by expander.\n");
            return FBE_ENCLOSURE_STATUS_BUSY;
        }

        if(eses_enclosure->enclosureConfigBeingUpdated == TRUE)
        {

            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_INFO,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                           "handle_get_emc_spec_status_resp End of mapping.\n");

            eses_enclosure->enclosureConfigBeingUpdated = FALSE;
            
            /* set active_edal_valid = TRUE*/          
            fbe_base_enclosure_set_activeEdalValid((fbe_base_enclosure_t *) eses_enclosure, TRUE);
            encl_status = 
                 fbe_base_enclosure_edal_incrementStateChangeCount((fbe_base_enclosure_t *)eses_enclosure);
            if(encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_base_object_customizable_trace(
                    (fbe_base_object_t*)eses_enclosure, 
                    FBE_TRACE_LEVEL_ERROR,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                    "HndlGetEmcSpecStatRsp, stateChangeCount increment failed, encl_status: 0x%x.\n",
                    encl_status);
                return encl_status;
            }
        }
    }
    return encl_status;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_cleanup_cmd(fbe_eses_enclosure_t * eses_enclosure, 
*                                    fbe_packet_t * packet,
*                                    fbe_payload_control_status_t control_status,
*                                    fbe_payload_control_status_qualifier_t control_status_qualifier)                  
***************************************************************************
* @brief
*       This function does the cleanup when the command complets. It releases 
*       the memory buffer allocated for the IO, set the previous control operation
*       status and release the current cdb operation.
*
* @param   eses_enclosure - The pointer to the ESES enclosure object.
* @param   packet - The pointer to the fbe_packet_t.
* @param   control_status - The previous control operation status that will be set to.
*
* @return   fbe_status_t.
*       Always returns FBE_STATUS_OK.
*
* NOTES
*       
* HISTORY
*   28-Aug-2008 PHE - Created.
*   30-Sept-2008 NC - use payload
***************************************************************************/
static fbe_status_t fbe_eses_enclosure_cleanup_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                                   fbe_packet_t * packet,
                                                   fbe_payload_control_status_t control_status,
                                                   fbe_payload_control_status_qualifier_t control_status_qualifier)
{
    fbe_sg_element_t  * sg_list = NULL; 
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_payload_control_operation_t * prev_control_operation = NULL;
    fbe_payload_control_operation_opcode_t opcode;


    payload = fbe_transport_get_payload_ex(packet);

    if (payload != NULL)
    {
        fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

        if(sg_list != NULL)
        {
            // for download operations the sg list can contain
            // additional allcoated buffers, so check for them
            // and release them before releasing the sg list buffer
            // The control operation is the previous operation and the cdb operation is the current one.
            prev_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
            if (prev_control_operation != NULL)
            {
                fbe_payload_control_get_opcode(prev_control_operation, &opcode);
                if((FBE_ESES_CTRL_OPCODE_DOWNLOAD_FIRMWARE == opcode) ||
                   (FBE_ESES_CTRL_OPCODE_TUNNEL_DOWNLOAD_FIRMWARE == opcode))
                {
                    fbe_eses_enclosure_dl_release_buffers(eses_enclosure,
                                                          (fbe_u8_t *)sg_list, 
                                                          &sg_list[1]);
                }
            }
            /* Release the memory chunk allocated for the sg_list. */        
            fbe_transport_release_buffer(sg_list);
            /* Set the SG_LIST to NULL */
            fbe_payload_ex_set_sg_list(payload, NULL, 0);
        }

        /* The cdb operation is the current operation and the control operation is the previous operation. */ 
        payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);

        /* release cdb operation */
        if (payload_cdb_operation != NULL)
        {
            fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
        }
        
        /* Afer releasing the cdb operation, the control operation becomes the current operation. */
        payload_control_operation = fbe_payload_ex_get_control_operation(payload);

        if(payload_control_operation != NULL)
        {
            fbe_payload_control_get_opcode(payload_control_operation, &opcode);
            if(FBE_ESES_CTRL_OPCODE_SET_ENCL_CTRL == opcode)
            {
                eses_enclosure->outstanding_write --;
            }

            /* Set the control operation status and status qualifier. */
            fbe_payload_control_set_status(payload_control_operation, control_status);
            fbe_payload_control_set_status_qualifier(payload_control_operation, control_status_qualifier);
        }
    }

    return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_handle_get_download_status_response(fbe_eses_enclosure_t * eses_enclosure, 
*                                                       fbe_sg_element_t * sg_list, 
*                                                       fbe_enclosure_status_t encl_status)                  
***************************************************************************
* @brief
*       This function handles the response got from the "receive diag" command 
*       with the "get download status" opcode.
*       It calls the function to parse the download status status page.
*       The input encl_status will be updated to become the return value.
*
* @param eses_enclosure - The pointer to the ESES enclosure.
* @param sg_list - the pointer to the sg list allocated while sending the command.
* @param encl_status - the status of handling the scsi command response to indicate 
*                      whether the content the sg_list points to is valid or not.
*
* @return  fbe_enclosure_status_t.
*      FBE_ENCLOSURE_STATUS_OK - no error.
*      otherwise - error is found.
*
* NOTES
*
* HISTORY
*   18-Aug-2008 GB - Created.
*   30-Sept-2008 NC - use payload
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_handle_get_download_status_response(fbe_eses_enclosure_t * eses_enclosure, 
                                                       fbe_sg_element_t * sg_list, 
                                                       fbe_enclosure_status_t encl_status)
{
    
    /*******
     * BEGIN
     *******/


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__);

    if(encl_status == FBE_ENCLOSURE_STATUS_OK)
    {
        // process the download status
        encl_status = fbe_eses_enclosure_process_download_ucode_status(eses_enclosure,
                                                        (fbe_eses_download_status_page_t *)sg_list[0].address);   
    }
    else
    {
        // error, don't request more status, change the ext status to "unknown"
        eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_UNKNOWN;//FBE_ENCLOSURE_FW_EXT_STATUS_NONE;
    }
    // if activation in progress check for rev change or timeout exceeded
    if ((eses_enclosure->enclCurrentFupInfo.enclFupOperation == FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE) &&
        (eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus == FBE_ENCLOSURE_FW_STATUS_IN_PROGRESS))
    {
        if (fbe_eses_enclosure_is_rev_change(eses_enclosure))
        {
            eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_EXT_STATUS_NONE;
            eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_NONE;
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s,REV CHANGED, SET STATUS TO NONE. \n",
                            __FUNCTION__);
        }
        else if (fbe_eses_enclosure_is_activate_timeout(eses_enclosure))
        {
            eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_FAIL;
            eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_ERR_TO;
        }
    }

    return encl_status;
} // fbe_eses_enclosure_handle_get_download_status_response


/*!*************************************************************************
* @fn fbe_eses_enclosure_handle_get_trace_buf_info_status_response(fbe_eses_enclosure_t * eses_enclosure, 
*                                                             fbe_packet_t * packet)                
***************************************************************************
* @brief
*       This function handles the response got from the "receive diag" command 
*       with the "get trace buffer info status" opcode.
*       It gets the previous control buffer and compares the allocated response 
*       buffer size with the size of the trace buffer info status. The data
*       of the trace buffer info status are copied to the response bufffer if it is 
*       large enough.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return  fbe_enclosure_status_t.
*      FBE_ENCLOSURE_STATUS_OK - no error.
*      otherwise - error is found.
*
* NOTES
*
* HISTORY
*   06-Oct-2008 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_handle_get_trace_buf_info_status_response(fbe_eses_enclosure_t * eses_enclosure, 
                                                             fbe_packet_t * packet)
{    
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_eses_info_elem_group_hdr_t * trace_buf_info_elem_group_hdr_p = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * prev_control_operation = NULL;
    fbe_payload_control_buffer_t control_buffer = NULL;
    fbe_enclosure_mgmt_ctrl_op_t * eses_page_op = NULL;
    fbe_enclosure_mgmt_trace_buf_cmd_t * trace_buf_cmd_p = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_u32_t bytes_to_be_copied = 0;

    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__ );
   
    payload = fbe_transport_get_payload_ex(packet);
    /* Get the pervious control operation */  
    prev_control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    fbe_payload_control_get_buffer(prev_control_operation, &control_buffer);
    if( control_buffer == NULL )
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_WARNING,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_payload_control_get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                                __FUNCTION__, status, control_buffer);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_ENCLOSURE_STATUS_PACKET_FAILED;
    }
    
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    encl_status = fbe_eses_enclosure_get_info_elem_group_hdr_p(eses_enclosure, 
                                                (ses_pg_emc_encl_stat_struct *)sg_list[0].address,
                                                FBE_ESES_INFO_ELEM_TYPE_TRACE_BUF,
                                                &trace_buf_info_elem_group_hdr_p);

    eses_page_op = (fbe_enclosure_mgmt_ctrl_op_t *)control_buffer;
    trace_buf_cmd_p = &(eses_page_op->cmd_buf.trace_buf_cmd);
    
    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, get trace buf failed, encl_status: 0x%x.\n",
                                __FUNCTION__,  encl_status);
                
    }
    else 
    {
        if(trace_buf_cmd_p->buf_op == FBE_ENCLOSURE_TRACE_BUF_OP_GET_NUM)
        {  
            eses_page_op->required_response_buf_size = 1;
            *(eses_page_op->response_buf_p) = trace_buf_info_elem_group_hdr_p->num_info_elems;
        }
        else if(trace_buf_cmd_p->buf_op == FBE_ENCLOSURE_TRACE_BUF_OP_GET_STATUS) 
        {
            
            eses_page_op->required_response_buf_size = 1 + 
                (trace_buf_info_elem_group_hdr_p->num_info_elems * trace_buf_info_elem_group_hdr_p->info_elem_size);

            /* Zero out the memory allocated. */
            fbe_zero_memory(eses_page_op->response_buf_p, eses_page_op->response_buf_size);
            /* The first byte is the number of the trace buffers, followed by the status of each trace buffer. */
            *(eses_page_op->response_buf_p) = trace_buf_info_elem_group_hdr_p->num_info_elems;
            

           /* Check the size. If the user allocated buffer size is less than required, 
            * set the encl_status to MEMORY INSUFFICIENT so that the requestor can tell there is 
            * less memory alloted 
            */
            if(eses_page_op->response_buf_size < eses_page_op->required_response_buf_size)
            {            
                bytes_to_be_copied = eses_page_op->response_buf_size - 1;
                             
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_INFO,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s, not enought memory allocated. allocated: %d, required: %d.\n",
                                        __FUNCTION__, eses_page_op->response_buf_size, 
                                        eses_page_op->required_response_buf_size); 
                
                // Update the enclosure status.
                encl_status =  FBE_ENCLOSURE_STATUS_ALLOCATED_MEMORY_INSUFFICIENT;
            }
            else
            {
                bytes_to_be_copied = eses_page_op->required_response_buf_size - 1; 
            }

            /* Copy the status over to the response buffer. */
            fbe_copy_memory((fbe_u8_t *)eses_page_op->response_buf_p + 1, 
                            (fbe_u8_t *)trace_buf_info_elem_group_hdr_p + FBE_ESES_EMC_CTRL_STAT_INFO_ELEM_GROUP_HEADER_SIZE, 
                            bytes_to_be_copied);
        }
        else
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, unhandled buf_op: %d.\n",
                                    __FUNCTION__, trace_buf_cmd_p->buf_op); 

            return FBE_ENCLOSURE_STATUS_PARAMETER_INVALID;
        }
    } 

    return encl_status;
} // End of - fbe_eses_enclosure_handle_get_trace_buf_info_status_response


/*!*************************************************************************
* @fn fbe_eses_enclosure_handle_read_buf_response(fbe_eses_enclosure_t * eses_enclosure, 
*                                                 fbe_packet_t * packet)             
***************************************************************************
* @brief
*       This function handles the response got from the "read buffer" command.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return  fbe_enclosure_status_t.
*      FBE_ENCLOSURE_STATUS_OK - no error.
*      otherwise - error is found.
*
* NOTES
*
* HISTORY
*   02-Feb-2009 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t fbe_eses_enclosure_handle_read_buf_response(fbe_eses_enclosure_t * eses_enclosure, 
                                                                          fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t   * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * prev_control_operation = NULL;
    fbe_payload_control_operation_opcode_t opcode;
    fbe_u32_t transfer_count = 0;
    fbe_payload_control_buffer_t control_buffer = NULL;
    fbe_enclosure_mgmt_ctrl_op_t * eses_page_op = NULL;
    fbe_enclosure_mgmt_read_buf_cmd_t * read_buf_cmd_p = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_bool_t resumeValid = FBE_FALSE;
  

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    if (sg_list == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s: NULL sg_list.  \n", __FUNCTION__ ); 
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_ENCLOSURE_STATUS_PACKET_FAILED;
    }

    /* Get the pervious control operation */  
    prev_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    
    fbe_payload_control_get_buffer(prev_control_operation, &control_buffer);
    if(control_buffer == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_WARNING,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_payload_control_get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                                __FUNCTION__, status, control_buffer);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_ENCLOSURE_STATUS_PACKET_FAILED;
    }

    eses_page_op = (fbe_enclosure_mgmt_ctrl_op_t *)control_buffer;

    fbe_payload_control_get_opcode(prev_control_operation, &opcode);

    switch(opcode)
    {
        case FBE_ESES_CTRL_OPCODE_READ_BUF:
            read_buf_cmd_p = &(eses_page_op->cmd_buf.read_buf);
            break;

        case FBE_ESES_CTRL_OPCODE_READ_RESUME:
            read_buf_cmd_p = &(eses_page_op->cmd_buf.resume_read.read_buf);
            break;

        default:
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, unhandled opcode : 0x%x.\n",
                                    __FUNCTION__,  opcode);

            return FBE_ENCLOSURE_STATUS_PARAMETER_INVALID;
            break;
    }

    if(read_buf_cmd_p->mode == FBE_ESES_READ_BUF_MODE_DESC)
    {
        eses_page_op->required_response_buf_size = sizeof(fbe_eses_read_buf_desc_t);
    }
    else if(read_buf_cmd_p->mode == FBE_ESES_READ_BUF_MODE_DATA) 
    {
        payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
        fbe_payload_cdb_get_transferred_count(payload_cdb_operation, &transfer_count);
        eses_page_op->required_response_buf_size = transfer_count;
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, opcode: 0x%x, unhandled mode: %d.\n",
                                __FUNCTION__, opcode, read_buf_cmd_p->mode); 

        return FBE_ENCLOSURE_STATUS_PARAMETER_INVALID;
    }

    if((opcode == FBE_ESES_CTRL_OPCODE_READ_RESUME) &&
       (eses_page_op->cmd_buf.resume_read.deviceType == FBE_DEVICE_TYPE_PS) &&
        fbe_eses_enclosure_is_ps_resume_format_special(eses_enclosure)) 
    { 
        fbe_eses_enclosure_reformat_resume(eses_enclosure,
                                          (RESUME_PROM_STRUCTURE *)eses_page_op->response_buf_p,
                                          (dev_eeprom_rev0_info_struct *)sg_list[0].address);

        fbe_eses_enclosure_is_eeprom_resume_valid(eses_enclosure, 
                                              (dev_eeprom_rev0_info_struct *)sg_list[0].address,
                                              &resumeValid);
        if(!resumeValid) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_WARNING,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "handleReadBufResp, opcode: %s, PS eeprom resume invalid. The client can retry. \n",
                                    fbe_eses_enclosure_print_scsi_opcode(opcode));

            return FBE_ENCLOSURE_STATUS_BUSY;
        }
    }
    else
    {
       fbe_copy_memory(eses_page_op->response_buf_p, 
                        (fbe_u8_t *)sg_list[0].address,
                        ((eses_page_op->response_buf_size > eses_page_op->required_response_buf_size) ?
                        eses_page_op->required_response_buf_size : 
                        eses_page_op->response_buf_size));
    }

    if((eses_page_op->required_response_buf_size != FBE_ENCLOSURE_VALUE_INVALID) &&
       (eses_page_op->response_buf_size < eses_page_op->required_response_buf_size))
    {            
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, opcode: 0x%x.\n",
                                __FUNCTION__,  opcode);

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, not enought memory allocated, allocated: %d, required: %d.\n",
                                __FUNCTION__, eses_page_op->response_buf_size, 
                                eses_page_op->required_response_buf_size); 

        // Update the enclosure status.
        encl_status =  FBE_ENCLOSURE_STATUS_ALLOCATED_MEMORY_INSUFFICIENT;
    }

    /* 
     * Do not need to copy memory from sg_list[0].address to eses_page_op->response_buf_p because 
     * sg_list[0].address points to the same memory that eses_page_op->response_buf_p points to.
     */

    return encl_status;
} // End of - fbe_eses_enclosure_handle_read_buf_response

/*!*************************************************************************
* @fn fbe_eses_enclosure_handle_raw_rcv_diag_response(fbe_eses_enclosure_t * eses_enclosure, 
*                                                 fbe_packet_t * packet)             
***************************************************************************
* @brief
*       This function handles the response got from the pass-throu rcv diag command.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return  fbe_enclosure_status_t.
*      FBE_ENCLOSURE_STATUS_OK - no error.
*      otherwise - error is found.
*
* NOTES
*
* HISTORY
*   02-Mar-2009 NC - Created.
***************************************************************************/
static fbe_enclosure_status_t fbe_eses_enclosure_handle_raw_rcv_diag_response(fbe_eses_enclosure_t * eses_enclosure, 
                                                                          fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * prev_control_operation = NULL;
    fbe_payload_control_buffer_t control_buffer = NULL;
    fbe_enclosure_mgmt_ctrl_op_t * eses_page_op = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_sg_element_t  * sg_list = NULL; 
  

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                             "%s entry \n", __FUNCTION__ );

    payload = fbe_transport_get_payload_ex(packet);
    /* Get the pervious control operation */  
    prev_control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    fbe_payload_control_get_buffer(prev_control_operation, &control_buffer);
    if( control_buffer == NULL )
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_WARNING,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                 "%s, fbe_payload_control_get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                                __FUNCTION__,  status, control_buffer);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_ENCLOSURE_STATUS_PACKET_FAILED;
    }

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    eses_page_op = (fbe_enclosure_mgmt_ctrl_op_t *)control_buffer;

    eses_page_op->required_response_buf_size = 
        fbe_eses_get_pg_size((ses_common_pg_hdr_struct *)sg_list[0].address);

    if (eses_page_op->response_buf_size < eses_page_op->required_response_buf_size)
    {            
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, not enought memory allocated. allocated: %d, required: %d.\n",
                                __FUNCTION__, eses_page_op->response_buf_size, 
                                eses_page_op->required_response_buf_size); 
        
        // Update the enclosure status.
        encl_status =  FBE_ENCLOSURE_STATUS_ALLOCATED_MEMORY_INSUFFICIENT;
    }

    /* 
     * Do not need to copy memory from sg_list[0].address to eses_page_op->response_buf_p because 
     * sg_list[0].address points to the same memory that eses_page_op->response_buf_p points to.
     */

    return encl_status;
} // End of - fbe_eses_enclosure_handle_raw_rcv_diag_response


/*!*************************************************************************
* @fn fbe_eses_enclosure_handle_mode_sense_response(fbe_eses_enclosure_t * eses_enclosure, 
*                                                   fbe_sg_element_t * sg_list)                
***************************************************************************
* @brief
*       This function handles the response got from the "mode sense" command, 
*       with the "mode sense" opcode.
*       It calls the function to parse the mode param list.
*
* @param       eses_enclosure - The pointer to the ESES enclosure.
* @param       packet - pointer to the fbe_packet_t
* @param       sg_list - the pointer to the sg list allocated while sending the command.
*
* @return  fbe_enclosure_status_t.
*      FBE_ENCLOSURE_STATUS_OK - no error.
*      otherwise - error is found.
*
* NOTES
*
* HISTORY
*   03-Dec-2008 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_handle_mode_sense_response(fbe_eses_enclosure_t * eses_enclosure, 
                                              fbe_packet_t * packet,
                                              fbe_sg_element_t * sg_list)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * prev_control_operation = NULL;
    fbe_payload_control_buffer_t control_buffer = NULL;
    fbe_enclosure_mgmt_ctrl_op_t * eses_page_op = NULL;
    fbe_status_t status = FBE_STATUS_OK;  
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
  

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__);     
     
    encl_status = fbe_eses_enclosure_process_mode_param_list(eses_enclosure, 
                                    (fbe_eses_mode_param_list_t *)sg_list[0].address);

    if(encl_status == FBE_ENCLOSURE_STATUS_OK)
    {
        /*
         * Mode Sense may be generated internally or from outside (via Usurper command)
         * If there is a previous operation, fill in status
         */
        payload = fbe_transport_get_payload_ex(packet);

        /* Get the pervious control operation */  
        prev_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
        if (prev_control_operation != NULL)
        {
            status = fbe_payload_control_get_buffer(prev_control_operation, &control_buffer);
            if ((status == FBE_STATUS_OK) && (control_buffer != NULL))
            {
                eses_page_op = (fbe_enclosure_mgmt_ctrl_op_t *)control_buffer;
                eses_page_op->cmd_buf.testModeInfo.testModeStatus = eses_enclosure->test_mode_status;
            }
        }

        if(eses_enclosure->mode_select_needed != 0)
        {
            status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                        (fbe_base_object_t*)eses_enclosure, 
                                        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_MODE_UNSELECTED);

            if(status != FBE_STATUS_OK)            
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s can't set discovery update condition, status: 0x%X",
                                         __FUNCTION__,  status);
            }
        }
    }
    else
    {
        // process_mode_param_list failed.
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "process_mode_param_list failed, encl_status: 0x%X",
                                encl_status); 
        
    }
      
    return encl_status;
} // End of function fbe_eses_enclosure_handle_mode_sense_response


/*!*************************************************************************
* @fn fbe_eses_enclosure_handle_sps_eeprom_response(fbe_eses_enclosure_t * eses_enclosure, 
*                                                   fbe_sg_element_t * sg_list)                
***************************************************************************
* @brief
*       This function handles the response got from the "SPS EEPROM" command.
*
* @param       eses_enclosure - The pointer to the ESES enclosure.
* @param       packet - pointer to the fbe_packet_t
* @param       sg_list - the pointer to the sg list allocated while sending the command.
*
* @return  fbe_enclosure_status_t.
*      FBE_ENCLOSURE_STATUS_OK - no error.
*      otherwise - error is found.
*
* NOTES
*
* HISTORY
*   21-Sep-2012     Joe Perry - Created.
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_handle_sps_eeprom_response(fbe_eses_enclosure_t * eses_enclosure, 
                                              fbe_packet_t * packet,
                                              fbe_sg_element_t * sg_list)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * prev_control_operation = NULL;
    fbe_payload_control_buffer_t control_buffer = NULL;
    sps_eeprom_info_struct     *spsEepromBufPtr = NULL;
    fbe_enclosure_mgmt_ctrl_op_t * eses_page_op = NULL;
    fbe_sps_manuf_info_t        *returnSpsManufInfo = NULL;
    fbe_status_t status = FBE_STATUS_OK;  
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_CMD_FAILED;
  
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__);     

    payload = fbe_transport_get_payload_ex(packet);

    /* Get the pervious control operation */  
    prev_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    if (prev_control_operation != NULL)
    {
        status = fbe_payload_control_get_buffer(prev_control_operation, &control_buffer);
        if ((status == FBE_STATUS_OK) && (control_buffer != NULL))
        {
            eses_page_op = (fbe_enclosure_mgmt_ctrl_op_t *)control_buffer;

            if(eses_page_op->response_buf_p != NULL)
            {
                spsEepromBufPtr = (sps_eeprom_info_struct *)sg_list[0].address;
                if (spsEepromBufPtr != NULL)
                {
                    returnSpsManufInfo = (fbe_sps_manuf_info_t *)eses_page_op->response_buf_p;
                    if (spsEepromBufPtr->spsEepromRev0.version == 0)
                    {

                        // copy data to pass back
                        fbe_copy_memory(&returnSpsManufInfo->spsModuleManufInfo.spsSerialNumber,
                                        &spsEepromBufPtr->spsEepromRev0.emc_serialnum,
                                        FBE_SPS_SERIAL_NUM_REVSION_SIZE);
                        fbe_copy_memory(&returnSpsManufInfo->spsModuleManufInfo.spsPartNumber,
                                        &spsEepromBufPtr->spsEepromRev0.emc_partnum,
                                        FBE_SPS_PART_NUM_REVSION_SIZE);
                        fbe_copy_memory(returnSpsManufInfo->spsModuleManufInfo.spsFirmwareRevision,
                                        spsEepromBufPtr->spsEepromRev0.per_dev.sps.primary_firmware_rev,
                                        5);
                        fbe_copy_memory(returnSpsManufInfo->spsModuleManufInfo.spsSecondaryFirmwareRevision,
                                        spsEepromBufPtr->spsEepromRev0.per_dev.sps.secondary_firmware_rev,
                                        5);
                        fbe_copy_memory(&returnSpsManufInfo->spsModuleManufInfo.spsModelString,
                                        &spsEepromBufPtr->spsEepromRev0.mfg_model_number,
                                        FBE_SPS_MODEL_STRING_SIZE);
                        fbe_copy_memory(&returnSpsManufInfo->spsModuleManufInfo.spsPartNumRevision,
                                        &spsEepromBufPtr->spsEepromRev0.emc_revision,
                                        3);
                        fbe_copy_memory(&returnSpsManufInfo->spsModuleManufInfo.spsVendor,
                                        &spsEepromBufPtr->spsEepromRev0.mfg_name,
                                        32);
                        fbe_copy_memory(&returnSpsManufInfo->spsModuleManufInfo.spsVendorModelNumber,
                                        &spsEepromBufPtr->spsEepromRev0.mfg_model_number,
                                        16);

                        fbe_copy_memory(&returnSpsManufInfo->spsBatteryManufInfo.spsSerialNumber,
                                        &spsEepromBufPtr->spsEepromRev0.per_dev.sps.batt_emc_serialnum,
                                        FBE_SPS_SERIAL_NUM_REVSION_SIZE);
                        fbe_copy_memory(&returnSpsManufInfo->spsBatteryManufInfo.spsPartNumber,
                                        &spsEepromBufPtr->spsEepromRev0.per_dev.sps.batt_emc_partnum,
                                        FBE_SPS_PART_NUM_REVSION_SIZE);
                        fbe_copy_memory(returnSpsManufInfo->spsBatteryManufInfo.spsFirmwareRevision,
                                        spsEepromBufPtr->spsEepromRev0.per_dev.sps.batt_firmware_rev,
                                        5);
                        fbe_copy_memory(&returnSpsManufInfo->spsBatteryManufInfo.spsModelString,
                                        &spsEepromBufPtr->spsEepromRev0.per_dev.sps.batt_model_number,
                                        9);
                        fbe_copy_memory(&returnSpsManufInfo->spsBatteryManufInfo.spsPartNumRevision,
                                        &spsEepromBufPtr->spsEepromRev0.per_dev.sps.batt_emc_revision,
                                        3);
                        fbe_copy_memory(&returnSpsManufInfo->spsBatteryManufInfo.spsVendor,
                                        &spsEepromBufPtr->spsEepromRev0.per_dev.sps.batt_mfg_name,
                                        32);
                        fbe_copy_memory(&returnSpsManufInfo->spsBatteryManufInfo.spsVendorModelNumber,
                                        &spsEepromBufPtr->spsEepromRev0.per_dev.sps.batt_model_number,
                                        9);
                        // save FFID info to EDAL
                        encl_status = fbe_base_enclosure_edal_setU32((fbe_base_enclosure_t *)eses_enclosure,
                                                                      FBE_SPS_FFID,
                                                                      FBE_ENCL_SPS,
                                                                      0,
                                                                      swap32(spsEepromBufPtr->spsEepromRev0.ffid));
                        if(!ENCL_STAT_OK(encl_status))
                        {
                            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                               FBE_TRACE_LEVEL_ERROR,
                                                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                               "%s, failed setting SpsFfid, encl_status %d\n", 
                                                               __FUNCTION__, encl_status);
                            return encl_status;
                        }
                        encl_status = fbe_base_enclosure_edal_setU32((fbe_base_enclosure_t *)eses_enclosure,
                                                                      FBE_SPS_BATTERY_FFID,
                                                                      FBE_ENCL_SPS,
                                                                      0,
                                                                      swap32(spsEepromBufPtr->spsEepromRev0.per_dev.sps.batt_ffid));
                        if(!ENCL_STAT_OK(encl_status))
                        {
                            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                               FBE_TRACE_LEVEL_ERROR,
                                                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                               "%s, failed setting SpsBatteryFfid, encl_status %d\n", 
                                                               __FUNCTION__, encl_status);
                            return encl_status;
                        }
                    }
                    else
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                           FBE_TRACE_LEVEL_WARNING,
                                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                           "%s, SPS EEPROM revision not supported, rev %d",
                                                           __FUNCTION__,
                                                           spsEepromBufPtr->spsEepromRev0.version);
                    }
                }
            }
        }
    }
      
    return encl_status;
} // End of function fbe_eses_enclosure_handle_sps_eeprom_response


/*!**************************************************************************
* @fn fbe_eses_enclosure_check_duplicate_ESN((fbe_eses_enclosure_t *)(
*      fbe_eses_enclosure_t *eses_enclosure, 
*      fbe_packet_t *packet)
***************************************************************************
*
* @brief
*       Function to check for duplicate enclosure Serial Number.
* 
* @param
*      eses_enclosure - The object receiving the event
*      packet - The pointer to the fbe_packet_t.
*
* @return
*       fbe_status_t.
*
* NOTES
*
* HISTORY
*   16-Oct-2008 - Created. Arunkumar Selvapillai
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_check_duplicate_ESN(fbe_eses_enclosure_t *eses_enclosure, fbe_packet_t *packet)
{
    fbe_bool_t              is_encl_SN_equal = FALSE;



    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t           *payload = NULL;
    fbe_sg_element_t        *sg_list = NULL;
    
    fbe_enclosure_number_t          enclosure_number = 0;

    fbe_payload_discovery_check_dup_encl_SN_data_t     *payload_discovery_get_encl_sn_data = NULL;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__);

    fbe_base_enclosure_get_enclosure_number((fbe_base_enclosure_t *)eses_enclosure, &enclosure_number);

    payload = fbe_transport_get_payload_ex(packet);

    /*  We need to put the data into the sg_list inside the payload, grab the pointer.  */
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    payload_discovery_get_encl_sn_data = (fbe_payload_discovery_check_dup_encl_SN_data_t *) sg_list[0].address;

    /* If the size is different, we can't compare the two, let's fail the request */
    if (payload_discovery_get_encl_sn_data->serial_number_length != FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                 "%s invalid SN size %d\n", 
                                __FUNCTION__,
                                payload_discovery_get_encl_sn_data->serial_number_length);
        
        status = FBE_STATUS_GENERIC_FAILURE;
        payload_discovery_get_encl_sn_data->encl_pos = FBE_ESES_ENCLOSURE_INVALID_POS;
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

#ifdef C4_INTEGRATED
    //
    //  If INVALID is returned, then the Inquiry response data does not
    //  the System SN.  So attempt to read it from the resume ourselves.
    //
    if (enclosure_number == 0 && 
        fbe_equal_memory(payload_discovery_get_encl_sn_data->encl_SN, "INVALID", 7))
    {
        SPECL_RESUME_DATA                         resume_data;
        fbe_zero_memory(&resume_data, sizeof(SPECL_RESUME_DATA));

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, had invalid SN.  Retrieving from the Resume.\n",
                                __FUNCTION__ );

        speclGetResume(CHASSIS_SERIAL_EEPROM, &resume_data);

        //
        //  If the resume read was successful, copy the SN out of
        //  out of the resume data.
        //
        if (resume_data.transactionStatus == EMCPAL_STATUS_SUCCESS)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, Setting encl SN to %s from Resume.\n",
                                    __FUNCTION__ ,
                                    resume_data.resumePromData.product_serial_number);

            // Get enclosure serial number.
            fbe_copy_memory(payload_discovery_get_encl_sn_data->encl_SN, 
                            resume_data.resumePromData.product_serial_number,
                            FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE); 
        }
        // no else.  This function handles enclosure 0 below.
    }
#endif /* C4_INTEGRATED - C4HW */


    /* Compare the ESN's of the payload and what eses has and return the appropriate status. */
    is_encl_SN_equal = fbe_equal_memory(eses_enclosure->encl_serial_number, 
                                        payload_discovery_get_encl_sn_data->encl_SN, 
                                        FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE);

    if ( is_encl_SN_equal )
    {
        payload_discovery_get_encl_sn_data->encl_pos = enclosure_number;
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        status = FBE_STATUS_OK;
    }
    else
    {
        if ( enclosure_number == 0 )
        {
            /* Return OK and complete the packet if the UID are equal. 
             * We are not going to do any more operation if it is enclosure 0.
             */
            payload_discovery_get_encl_sn_data->encl_pos = FBE_ESES_ENCLOSURE_INVALID_POS;
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet);
            status = FBE_STATUS_OK;
        }
        else
        {
            /* We are not done yet. We need to compare the enclosure UID with 
             * all the other available enclosures in the chain. 
             */
            status = fbe_sas_enclosure_discovery_transport_entry((fbe_sas_enclosure_t *)eses_enclosure, packet);
        }
    }

    return status;
}

/*!***************************************************************
* @fn fbe_eses_enclosure_notify_upstream_fw_activation(
*          fbe_eses_enclosure_t *eses_enclosure, 
*          fbe_packet_t *packet)
****************************************************************
* @brief:
*  This function sends notification to upstream object about 
* ongoing firmware activation.
*
* @param
*  eses_enclosure - The object receiving the event
*  packet - io packet pointer
*
* @return
*  fbe_status_t - FBE_STATUS_OK 
*                   - no error.
*                 FBE_STATUS_MORE_PROCESSING_REQUIRED
*                   - imply the event needs more handling.  
*
* HISTORY:
*  11/18/09 - Created. NChiu
*
****************************************************************/
fbe_status_t 
fbe_eses_enclosure_notify_upstream_fw_activation(fbe_eses_enclosure_t *eses_enclosure, fbe_packet_t *packet)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t           *payload = NULL;
    fbe_object_id_t         my_object_id;
    fbe_payload_discovery_operation_t   *payload_discovery_operation = NULL;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                             "%s entry\n", __FUNCTION__);

    /* Get the payload and allocate the discovery operation */
    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s NULL payload", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload_discovery_operation = fbe_payload_ex_allocate_discovery_operation(payload);

    /* Validate the payload discovery operation */
    if( payload_discovery_operation == NULL ) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s fbe_payload_ex_allocate_discovery_operation fail", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_object_get_object_id((fbe_base_object_t *)eses_enclosure, &my_object_id);

    status = fbe_payload_discovery_build_notify_fw_activation_status(payload_discovery_operation,  
                                                                     eses_enclosure->inform_fw_activating,
                                                                     my_object_id);

    if( status == FBE_STATUS_GENERIC_FAILURE ) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                 "%s build_notify_fw_activation_status fail", __FUNCTION__);

        fbe_payload_ex_release_discovery_operation(payload, payload_discovery_operation);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_transport_set_completion_function(packet, 
                                                   fbe_eses_enclosure_notify_activation_completion, 
                                                   eses_enclosure);

    /* We are sending discovery packet via discovery edge */
    status = fbe_base_discovered_send_functional_packet((fbe_base_discovered_t *)eses_enclosure, packet);

    return status;
}

/*!***************************************************************
 * @fn fbe_eses_enclosure_notify_activation_completion(
 *              fbe_packet_t * packet, 
 *              fbe_packet_completion_context_t context)
 ****************************************************************
 * @brief:
 *  This function handles the completion of notify fw activation.
 *
 * @param
 *  packet - io packet pointer
 *  context - completion conext
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK 
 *                   - no error.
 *
 * HISTORY:
 *  11/18/09 - Created. NChiu
 *
 ****************************************************************/
static fbe_status_t 
fbe_eses_enclosure_notify_activation_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context)
{
    fbe_status_t                status;
    fbe_payload_ex_t               *payload = NULL;
    fbe_eses_enclosure_t        *eses_enclosure = NULL;

    fbe_payload_discovery_operation_t                   *payload_discovery_operation = NULL;

    eses_enclosure = (fbe_eses_enclosure_t *)context;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);

    if(status != FBE_STATUS_OK)
    { 
        /* Something bad happen to child, just release packet */
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s fbe_transport_get_status_code failed, status: 0x%x.\n", 
                                __FUNCTION__, status);  
        
        return status;
    }
   
    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);

    fbe_payload_ex_release_discovery_operation(payload, payload_discovery_operation);

    /* reschedule ourself */
    fbe_lifecycle_reschedule(&fbe_eses_enclosure_lifecycle_const, 
                            (fbe_base_object_t*)eses_enclosure,  
                            0);

    return FBE_STATUS_OK;
}

/*!***************************************************************
* @fn fbe_eses_enclosure_build_encl_SN_packet(
*          fbe_eses_enclosure_t *eses_enclosure, 
*          fbe_packet_t *packet)
****************************************************************
* @brief:
*  This function builds the packet for checking encl SN.
*
* @param
*  eses_enclosure - The object receiving the event
*  packet - io packet pointer
*
* @return
*  fbe_status_t - FBE_STATUS_OK 
*                   - no error.
*                 FBE_STATUS_MORE_PROCESSING_REQUIRED
*                   - imply the event needs more handling.  
*
* HISTORY:
*  10/20/08 - Created. Arunkumar Selvapillai
*  04/27/09 - This function is disabled, and 
*             the functionality is moved up to AEN.  -NChiu
*
****************************************************************/
fbe_packet_t * 
fbe_eses_enclosure_build_encl_SN_packet(fbe_eses_enclosure_t *eses_enclosure, fbe_packet_t *packet)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t                *buffer = NULL;
    fbe_payload_ex_t           *payload = NULL;
    fbe_sg_element_t        *sg_list = NULL;

    fbe_payload_discovery_operation_t   *payload_discovery_operation = NULL;
    fbe_payload_discovery_check_dup_encl_SN_data_t     *payload_discovery_get_encl_sn_data = NULL;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                             "%s entry\n", __FUNCTION__);

    /* Allocate the buffer */
    buffer = fbe_transport_allocate_buffer();

    /* Validate the buffer */
    if( buffer == NULL ) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s fbe_transport_allocate_buffer fail", __FUNCTION__);

        return NULL;
    }

    /* Get the payload and allocate the discovery operation */
    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_allocate_discovery_operation(payload);

    /* Validate the payload discovery operation */
    if( payload_discovery_operation == NULL ) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s fbe_payload_ex_allocate_discovery_operation fail", __FUNCTION__);

        fbe_transport_release_buffer(buffer);
        return NULL;
    }

    sg_list = (fbe_sg_element_t *)buffer;
    sg_list[0].count = sizeof(fbe_payload_discovery_check_dup_encl_SN_data_t);
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);

    /* Build the encl SN data*/
    payload_discovery_get_encl_sn_data = (fbe_payload_discovery_check_dup_encl_SN_data_t *)sg_list[0].address;

    status = fbe_payload_discovery_build_encl_SN(payload_discovery_operation,  
                                                 payload_discovery_get_encl_sn_data,
                                                 eses_enclosure->encl_serial_number,
                                                 FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE);

    if( status == FBE_STATUS_GENERIC_FAILURE ) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                 "%s fbe_payload_discovery_build_encl_SN fail", __FUNCTION__);

        fbe_transport_release_buffer(buffer);
        fbe_payload_ex_release_discovery_operation(payload, payload_discovery_operation);
        return NULL;
    }

    return packet;
}

/*!***************************************************************
 * @fn fbe_eses_enclosure_check_dup_encl_SN(
 *              fbe_eses_enclosure_t *eses_enclosure, 
 *              fbe_packet_t *packet)
 ****************************************************************
 * @brief:
 *  This function generates the packet for encl Serial Number and 
 *  sends it down.
 *
 * @param
 *  eses_enclosure - The object receiving the event
 *  packet - io packet pointer
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK 
 *                   - no error.
 *
 * HISTORY:
 *  10/20/08 - Created. Arunkumar Selvapillai
 *
 ****************************************************************/
fbe_status_t 
fbe_eses_enclosure_check_dup_encl_SN(fbe_eses_enclosure_t *eses_enclosure, 
                                            fbe_packet_t *packet)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_packet_t            *new_packet = NULL;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__);

    new_packet = fbe_eses_enclosure_build_encl_SN_packet(eses_enclosure, packet);

    if ( new_packet == NULL )
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s fbe_transport_allocate_buffer fail", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_transport_set_completion_function(new_packet, 
                                                   fbe_eses_enclosure_check_encl_SN_completion, 
                                                   eses_enclosure);

    /* We are sending discovery packet via discovery edge */
    status = fbe_base_discovered_send_functional_packet((fbe_base_discovered_t *)eses_enclosure, new_packet);

    return status;
}

/*!***************************************************************
 * @fn fbe_eses_enclosure_check_encl_SN_completion(
 *              fbe_packet_t * packet, 
 *              fbe_packet_completion_context_t context)
 ****************************************************************
 * @brief:
 *  This function handles the completion of encl Serial Number.
 *
 * @param
 *  packet - io packet pointer
 *  context - completion conext
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK 
 *                   - no error.
 *
 * HISTORY:
 *  10/20/08 - Created. Arunkumar Selvapillai
 *
 ****************************************************************/
static fbe_status_t 
fbe_eses_enclosure_check_encl_SN_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context)
{
    fbe_status_t                status;
    fbe_payload_ex_t               *payload_packet = NULL;
    fbe_sg_element_t            *sg_list = NULL; 
    fbe_eses_enclosure_t        *eses_enclosure = NULL;

    fbe_payload_discovery_operation_t                   *payload_discovery_operation = NULL;
    fbe_payload_discovery_check_dup_encl_SN_data_t     *payload_discovery_get_encl_sn_data = NULL;

    eses_enclosure = (fbe_eses_enclosure_t *)context;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);

    if(status != FBE_STATUS_OK)
    { 
        /* Something bad happen to child, just release packet */
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s fbe_transport_get_status_code failed, status: 0x%x.\n", 
                                __FUNCTION__, status);  
        
        return status;
    }
   
    payload_packet = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload_packet);

    fbe_payload_ex_get_sg_list(payload_packet, &sg_list, NULL);

    payload_discovery_get_encl_sn_data = (fbe_payload_discovery_check_dup_encl_SN_data_t *)sg_list[0].address;

    /* Throw out the enclosure position where we find the duplicate enclosure UID */
    if (payload_discovery_get_encl_sn_data->encl_pos != FBE_ESES_ENCLOSURE_INVALID_POS)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s entry. Duplicate Enclosure Serial Number Found at position: %d\n", 
                                __FUNCTION__, payload_discovery_get_encl_sn_data->encl_pos);

        // update fault information and fail the enclosure
        status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *) eses_enclosure, 
                                        0,
                                        0,
                                        FBE_ENCL_FLTSYMPT_DUPLICATE_UID,
                                        payload_discovery_get_encl_sn_data->encl_pos);

        // trace out the error if we can't handle the error
        if(!ENCL_STAT_OK(status))
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                            "%s can't set FAIL condition, status: 0x%X\n",
                            __FUNCTION__, status);
        }
   }

    fbe_transport_release_buffer(sg_list);
    fbe_payload_ex_release_discovery_operation(payload_packet, payload_discovery_operation);

    return FBE_STATUS_OK;
}


/*!**************************************************************************
 * @fn fbe_eses_enclosure_discovery_transport_drive_power_ctrl(
 *      fbe_eses_enclosure_t *eses_enclosure, 
 *      fbe_packet_t *packet)
 ***************************************************************************
 *
 * @brief
 *       Function to power on/off a drive slot.
 * 
 * @param
 *      eses_enclosure - The object receiving the event
 *      packet - The pointer to the fbe_packet_t.
 *
 * @return
 *       fbe_status_t.
 *
 * NOTES
 *
 * HISTORY
 *   19-Dec-2008 - Created. Joe Perry
 *   02-Apr-2009 PHE - Added drive power off.
 ***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_discovery_transport_drive_power_ctrl(fbe_eses_enclosure_t *eses_enclosure, 
                                        fbe_packet_t *packet)
{
    fbe_payload_ex_t                       *payload = NULL;
    fbe_payload_discovery_operation_t   *payload_discovery_operation = NULL;
    fbe_edge_index_t                    slotIndex = 0;
    fbe_bool_t                          device_off = FALSE;
    fbe_u8_t                            device_off_reason = FBE_ENCL_POWER_OFF_REASON_INVALID;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_enclosure_status_t              encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_payload_discovery_opcode_t      discovery_opcode = FBE_PAYLOAD_DISCOVERY_OPCODE_INVALID;    
   
    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);

    /* As a for now we have one client only with this particular id */
    status = fbe_base_discovering_get_server_index_by_client_id(
        (fbe_base_discovering_t *) eses_enclosure, 
        payload_discovery_operation->command.common_command.client_id,
        &slotIndex);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, get_server_index_by_client_id fail, client_id %d.\n",
                                __FUNCTION__, payload_discovery_operation->command.common_command.client_id);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Check if server_index is an index of the drive */
    if(slotIndex >= fbe_eses_enclosure_get_number_of_slots(eses_enclosure)) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s: Invalid slotIndex %d\n", __FUNCTION__, slotIndex);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_discovery_get_opcode(payload_discovery_operation, &discovery_opcode);
    
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, discovery_opcode 0x%x, slot %d.\n", 
                            __FUNCTION__, discovery_opcode, slotIndex);

    switch(discovery_opcode)
    {
        case FBE_PAYLOAD_DISCOVERY_OPCODE_POWER_ON:
            device_off = FALSE;
            break;

        case FBE_PAYLOAD_DISCOVERY_OPCODE_POWER_SAVE_ON:
            device_off_reason = FBE_ENCL_POWER_OFF_REASON_POWER_SAVE;
            device_off = TRUE; 
            break;

        case FBE_PAYLOAD_DISCOVERY_OPCODE_POWER_SAVE_ON_PASSIVE:
            device_off_reason = FBE_ENCL_POWER_OFF_REASON_POWER_SAVE_PASSIVE;
            break;

        default:
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, slot: %d, unhandled discovery opcode: 0x%x.\n",
                                    __FUNCTION__, slotIndex, discovery_opcode);

            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_transport_set_status(packet, status, 0);
            fbe_transport_complete_packet(packet);
            return status;
            break;
    }// End of - switch(discovery_opcode)


    if(device_off_reason != FBE_ENCL_POWER_OFF_REASON_INVALID)
    {
        // Update the device off reason.
        encl_status = fbe_base_enclosure_edal_setU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                    FBE_ENCL_DRIVE_DEVICE_OFF_REASON,  //Attribute
                                    FBE_ENCL_DRIVE_SLOT, // Component type
                                    slotIndex, // Component index
                                    device_off_reason);

        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_transport_set_status(packet, status, 0);
            fbe_transport_complete_packet(packet);
            return status;
        }    
    }

    /* If the opcode is power save passive, no need to do anything here. */
    if(device_off_reason != FBE_ENCL_POWER_OFF_REASON_POWER_SAVE_PASSIVE)
    {
        // Power On/off this drive slot. power save passive does not need to do anything here.
        encl_status = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_DRIVE_DEVICE_OFF,
                                                    FBE_ENCL_DRIVE_SLOT,
                                                    slotIndex,
                                                    device_off);

        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_transport_set_status(packet, status, 0);
            fbe_transport_complete_packet(packet);
            return status;
        } 
   
        /* Set condition to read the status page for the drive slot status. 
         * The latest status of the power off attribute will be used while building
         * the enclosure control page for drive slot.
         */
        status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                    (fbe_base_object_t*)eses_enclosure, 
                                    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_STATUS_UNKNOWN);  

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, can't set STATUS_UNKNOWN condition, status: 0x%x.\n",
                                    __FUNCTION__,  status);

            fbe_transport_set_status(packet, status, 0);
            fbe_transport_complete_packet(packet);
            return status;
        }
     
        /*
        * Set condition to write out ESES Control
        */
        status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                        (fbe_base_object_t*)eses_enclosure, 
                                        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EXPANDER_CONTROL_NEEDED);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, can't set ControlNeeded condition, status: 0x%x.\n",
                                    __FUNCTION__,  status);
            
        }
    }// End of - if(device_off_reason != FBE_ENCL_POWER_OFF_REASON_POWER_SAVE_PASSIVE)
  
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return status;

}   // end of fbe_eses_enclosure_discovery_transport_drive_power_ctrl

/*!**************************************************************************
 * @fn fbe_eses_enclosure_discovery_transport_drive_power_cycle(
 *      fbe_eses_enclosure_t *eses_enclosure, 
 *      fbe_packet_t *packet)
 ***************************************************************************
 *
 * @brief
 *       Function to power cycle a drive slot.
 * 
 * @param
 *      eses_enclosure - The object receiving the event
 *      packet - The pointer to the fbe_packet_t.
 *
 * @return
 *       fbe_status_t.
 *
 * NOTES
 *
 * HISTORY
 *   01-Sep-2009 PHE - Created.
 ***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_discovery_transport_drive_power_cycle(fbe_eses_enclosure_t *eses_enclosure, 
                                        fbe_packet_t *packet)
{
    fbe_payload_ex_t                       *payload = NULL;
    fbe_payload_discovery_operation_t   *payload_discovery_operation = NULL;
    fbe_edge_index_t                    slot_index = 0;
    fbe_bool_t                          power_cycle_comp_ack = FALSE;
    fbe_u32_t                           power_cycle_duration = 0;
    fbe_bool_t                          slot_powered_off = FALSE;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_enclosure_status_t              encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_bool_t                          power_cycle_pending = FALSE;

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);

    /* As a for now we have one client only with this particular id */
    status = fbe_base_discovering_get_server_index_by_client_id(
        (fbe_base_discovering_t *) eses_enclosure, 
        payload_discovery_operation->command.power_cycle_command.client_id,
        &slot_index);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "discovery_transport_drive_power_cycle, get_server_index_by_client_id failed, "
                                "client id %d.\n", payload_discovery_operation->command.power_cycle_command.client_id);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Check if server_index is an index of the drive */
    if(slot_index >= fbe_eses_enclosure_get_number_of_slots(eses_enclosure)) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "discovery_transport_drive_power_cycle, Invalid slot %d.\n", slot_index);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    power_cycle_comp_ack = payload_discovery_operation->command.power_cycle_command.completed;
    power_cycle_duration = payload_discovery_operation->command.power_cycle_command.duration;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "discovery_transport_drive_power_cycle, slot %d, compAck %d, duration %d.\n", 
                            slot_index,
                            power_cycle_comp_ack,
                            power_cycle_duration);
        
    if(power_cycle_comp_ack)
    {
        /* The drive object has acknowledged the power cycle completion. 
        * Clear the EDAL attribute FBE_ENCL_COMP_POWER_CYCLE_COMPLETED 
        * so that the discovery path attribute FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_COMPLETE
        * can be cleared.
        */
        encl_status = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                FBE_ENCL_COMP_POWER_CYCLE_COMPLETED,
                                FBE_ENCL_DRIVE_SLOT,
                                slot_index,
                                FALSE); 

        if (!ENCL_STAT_OK(encl_status))
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }   
    }
    else
    {
        /* The drive object asks to power cycle the drive slot. */  
        if(power_cycle_duration > FBE_ESES_DRIVE_POWER_CYCLE_DURATION_MAX)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t *) eses_enclosure, 
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                            "discovery_transport_drive_power_cycle, slot %d, invalid duration %d, expected <= %d.\n",
                            slot_index, 
                            power_cycle_duration,
                            FBE_ESES_DRIVE_POWER_CYCLE_DURATION_MAX);

            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_transport_set_status(packet, status, 0);
            fbe_transport_complete_packet(packet);
            return status;
        }

        /* If the slot is already powered off, return failure. */
        encl_status = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                FBE_ENCL_COMP_POWERED_OFF,
                                FBE_ENCL_DRIVE_SLOT,
                                slot_index,
                                &slot_powered_off); 

        if (!ENCL_STAT_OK(encl_status) || slot_powered_off)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_transport_set_status(packet, status, 0);
            fbe_transport_complete_packet(packet);
            return status;
        }           

        encl_status = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                    FBE_ENCL_COMP_POWER_CYCLE_PENDING,
                                    FBE_ENCL_DRIVE_SLOT,
                                    slot_index,
                                    &power_cycle_pending);
        if (!ENCL_STAT_OK(encl_status))
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_transport_set_status(packet, status, 0);
            fbe_transport_complete_packet(packet);
            return status;
        }    

        /* If the power cycle pending is already set, it means there is already a power cycle command ongoing.
         *
         * It will be a no-op for the expander when it receives the power cycle command while the drive slot is in the powered
         * off state due to the first power cycle command. 
         * If we read the drive slot power down count before sending the second power cycle command, 
         * it could be possible that we read the drive slot power down count when the drive is powered off
         * due to the first power cycle command. We may not see the drive slot power down count get increased. 
         * So we don't want to send the power cycle command again.
         *
         * For the case that the expander gets reset before the power cycle is actually executed, it will be 
         * handled by the function fbe_eses_enclosure_ride_through_handle_slot_power_cycle. That function clears
         * the pending flag so that the drive object will send the power cycle command again after the expander reset.
         */
      
        if(power_cycle_pending)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_WARNING,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s, power cycle for drive slot %d is in progress, do not send the cmd again.\n",
                                        __FUNCTION__,  slot_index);
        }
        else
        {
            fbe_eses_enclosure_set_drive_power_cycle_duration(eses_enclosure, 
                                                (fbe_u8_t)slot_index,
                                                power_cycle_duration);
            fbe_eses_enclosure_set_drive_power_cycle_request(eses_enclosure, 
                                                (fbe_u8_t)slot_index,
                                                TRUE);

           /* Set the FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA attribute to TRUE. The function also
            * sets FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA_SENT to FALSE.
            */
            encl_status = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA,
                                                        FBE_ENCL_DRIVE_SLOT,
                                                        slot_index,
                                                        TRUE);

            if (!ENCL_STAT_OK(encl_status))
            {
                status = FBE_STATUS_GENERIC_FAILURE;
                fbe_transport_set_status(packet, status, 0);
                fbe_transport_complete_packet(packet);
                return status;
            }            

            encl_status = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                    FBE_ENCL_COMP_POWER_CYCLE_PENDING,
                                    FBE_ENCL_DRIVE_SLOT,
                                    slot_index,
                                    TRUE);

            if (!ENCL_STAT_OK(encl_status))
            {
                status = FBE_STATUS_GENERIC_FAILURE;
                fbe_transport_set_status(packet, status, 0);
                fbe_transport_complete_packet(packet);
                return status;
            }       

            /* Set condition to read the EMC statistics status page for the drive slot power down count. */
            status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                        (fbe_base_object_t*)eses_enclosure, 
                                        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EMC_STATISTICS_STATUS_UNKNOWN);  

            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s, can't set EMC_STATISTICS_STATUS_UNKNOWN condition, status: 0x%x.\n",
                                        __FUNCTION__,  status);

                fbe_transport_set_status(packet, status, 0);
                fbe_transport_complete_packet(packet);
                return status;
                
            }
            
            /* Set condition to write out EMC enclosure control page. */
            status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                        (fbe_base_object_t*)eses_enclosure, 
                                        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EMC_SPECIFIC_CONTROL_NEEDED);  

            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s, can't set EMC_SPECIFIC_CONTROL_NEEDED condition, status: 0x%x.\n",
                                        __FUNCTION__,  status);
                
            }
        } // End of - else - if(power_cycle_pending)
    } // End of - else - if(power_cycle_comp_ack)
  
    
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return status;

}   // end of fbe_eses_enclosure_discovery_transport_drive_power_cycle

/*!**************************************************************************
 * @fn fbe_eses_enclosure_discovery_transport_reset_drive_slot(
 *      fbe_eses_enclosure_t *eses_enclosure, 
 *      fbe_packet_t *packet)
 ***************************************************************************
 *
 * @brief
 *       Function to reset the drive slot.
 * 
 * @param
 *      eses_enclosure - The object receiving the event
 *      packet - The pointer to the fbe_packet_t.
 *
 * @return
 *       fbe_status_t.
 *
 * NOTES
 *
 * HISTORY
 *   23-Sep-2010 PHE - Created.
 ***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_discovery_transport_reset_drive_slot(fbe_eses_enclosure_t *eses_enclosure, 
                                        fbe_packet_t *packet)
{
    fbe_packet_t                       * new_packet = NULL;
    fbe_payload_ex_t                      * payload = NULL;
    fbe_payload_discovery_operation_t  * payload_discovery_operation = NULL;
    fbe_u32_t                            slot_num = 0;
    fbe_edge_index_t                     server_index;
    fbe_u8_t                             phy_index;
    fbe_u8_t                             phy_id;
    fbe_bool_t                           phy_link_ready;
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_enclosure_status_t               encl_status = FBE_ENCLOSURE_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);

    /* As a for now we have one client only with this particular id */
    status = fbe_base_discovering_get_server_index_by_client_id(
        (fbe_base_discovering_t *) eses_enclosure, 
        payload_discovery_operation->command.common_command.client_id,
        &server_index);

    slot_num = (fbe_u32_t)server_index;

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "discovery_transport_drive_reset_slot, get_server_index_by_client_id failed, "
                                "client id %d.\n", payload_discovery_operation->command.common_command.client_id);

        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Check if server_index is an index of the drive */
    if(slot_num >= fbe_eses_enclosure_get_number_of_slots(eses_enclosure)) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "discovery_transport_drive_reset_slot, Invalid slot %d.\n", slot_num);

        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the phy Index for the slot. */
    encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                               FBE_ENCL_DRIVE_PHY_INDEX,
                                               FBE_ENCL_DRIVE_SLOT,
                                               slot_num,
                                               &phy_index);
    if (encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the phy Id for the slot. */
    encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                               FBE_ENCL_EXP_PHY_ID,
                                               FBE_ENCL_EXPANDER_PHY,
                                               phy_index,
                                               &phy_id);
    if (encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the phy link ready attribute. */
    encl_status = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                               FBE_ENCL_EXP_PHY_LINK_READY,
                                               FBE_ENCL_EXPANDER_PHY,
                                               phy_index,
                                               &phy_link_ready);
    if (encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If the phy link is NOT ready, return the failure. */
    if(!phy_link_ready)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "discovery_transport_reset_drive_slot, phyLinkReady FALSE, do not send down cmd, slot %d, phyIndex %d, phyId %d.\n", 
                            slot_num,
                            phy_index,
                            phy_id);
        
        fbe_payload_discovery_set_status(payload_discovery_operation, FBE_PAYLOAD_DISCOVERY_STATUS_FAILURE);

        status = FBE_STATUS_OK;
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Build the new packet for the SMP operation. */
    new_packet = fbe_eses_enclosure_build_reset_drive_slot_packet(eses_enclosure, packet, phy_id);
    if (new_packet == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_transport_allocate_packet failed\n",
                                __FUNCTION__);

        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the completion routine for the new packet. */
    status = fbe_transport_set_completion_function(new_packet, 
                                                   fbe_eses_enclosure_send_reset_drive_slot_completion, 
                                                   eses_enclosure);

    /* We are sending the new packet via smp edge */
    status = fbe_sas_enclosure_send_smp_functional_packet((fbe_sas_enclosure_t *) eses_enclosure, new_packet);

    return status;
} // end of fbe_eses_enclosure_discovery_transport_reset_drive_slot

/*!***************************************************************
 * @fn fbe_eses_enclosure_build_reset_drive_slot_packet(
 *                    fbe_eses_enclosure_t * eses_enclosure, 
 *                    fbe_packet_t * packet,
 *                    fbe_u8_t phy_id)                  
 ****************************************************************
 * @brief
 *  This function builds the new packet used to send down the command 
 *  to the port object to reset the expander phy through SMP edge.
 *
 * @param  eses_enclosure - our object context
 * @param  packet - request packet
 * @param  phy_id - The phy id of the drive slot to be reset.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  23-Sep-2010 PHE - Created.
 *
 ****************************************************************/
fbe_packet_t * 
fbe_eses_enclosure_build_reset_drive_slot_packet(fbe_eses_enclosure_t * eses_enclosure, 
                                                   fbe_packet_t * packet,
                                                   fbe_u32_t phy_id)
{
    fbe_packet_t                  * new_packet = NULL;
    fbe_payload_ex_t                 * payload = NULL;
    fbe_payload_smp_operation_t   * payload_smp_operation = NULL;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "%s entry \n", __FUNCTION__ );

    /* Allocate packet */
    new_packet = fbe_transport_allocate_packet();
    if(new_packet == NULL) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s fbe_transport_allocate_packet fail\n", __FUNCTION__ );

        return NULL;
    }
   
    fbe_transport_initialize_packet(new_packet);

    payload = fbe_transport_get_payload_ex(new_packet);
    payload_smp_operation = fbe_payload_ex_allocate_smp_operation(payload);
  
    fbe_payload_smp_build_reset_end_device(payload_smp_operation, phy_id);

    /* No need for the sg list. */
    fbe_payload_ex_set_sg_list(payload, NULL, 0);

    /* We need to associate the newly allocated packet with the original one. */
    fbe_transport_add_subpacket(packet, new_packet);

    fbe_transport_set_cancel_function(packet, 
                                      fbe_base_object_packet_cancel_function, 
                                      (fbe_base_object_t *)eses_enclosure);

    fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)eses_enclosure, packet);

    return new_packet;
}


/****************************************************************
 * fbe_eses_enclosure_send_reset_drive_slot_completion()
 ****************************************************************
 * DESCRIPTION:
 *  This function handles the completion of rreset_drive_slot.
 *
 * PARAMETERS:
 *  packet - io packet pointer
 *  context - completion conext
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK 
 *                   - no error.
 *
 * HISTORY:
 *  23-Sep-2010 PHE - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_eses_enclosure_send_reset_drive_slot_completion(fbe_packet_t * packet, 
                                                     fbe_packet_completion_context_t context)
{
    fbe_eses_enclosure_t * eses_enclosure = NULL;
    fbe_packet_t * master_packet = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_ex_t * master_packet_payload = NULL;
    fbe_payload_smp_operation_t * payload_smp_operation = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_status_t status;
    fbe_payload_smp_status_t smp_status;
    fbe_payload_discovery_status_t master_packet_discovery_status = FBE_PAYLOAD_DISCOVERY_STATUS_OK;

    eses_enclosure = (fbe_eses_enclosure_t *)context;

    master_packet = (fbe_packet_t *)fbe_transport_get_master_packet(packet);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);

    payload = fbe_transport_get_payload_ex(packet);
    if (payload != NULL)
    {
        payload_smp_operation = fbe_payload_ex_get_smp_operation(payload);
        if (payload_smp_operation == NULL)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    if (status == FBE_STATUS_OK)
    {
        fbe_payload_smp_get_status(payload_smp_operation, &smp_status);
        
        if(smp_status == FBE_PAYLOAD_SMP_STATUS_OK)
        {
            master_packet_discovery_status = FBE_PAYLOAD_DISCOVERY_STATUS_OK;
        }
        else
        {
            master_packet_discovery_status = FBE_PAYLOAD_DISCOVERY_STATUS_FAILURE;  
        }
    }
            
    /* We need to remove packet from master queue */
    fbe_transport_remove_subpacket(packet);

    if (payload_smp_operation != NULL)
    {
        fbe_payload_ex_release_smp_operation(payload, payload_smp_operation);
    }

    fbe_transport_release_packet(packet);

    /* Remove master packet from the termination queue. */
    fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)eses_enclosure, master_packet);

    master_packet_payload = fbe_transport_get_payload_ex(master_packet);
    if (master_packet_payload != NULL)
    {
        payload_discovery_operation = fbe_payload_ex_get_discovery_operation(master_packet_payload);
        fbe_payload_discovery_set_status(payload_discovery_operation, master_packet_discovery_status);
    }
    else
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_set_status(master_packet, status, 0);
    fbe_transport_complete_packet(master_packet);

    return FBE_STATUS_OK;
}


/*!**************************************************************************
 * @fn fbe_eses_enclosure_discoveryUnbypass((fbe_eses_enclosure_t *)(
 *      fbe_eses_enclosure_t *eses_enclosure, 
 *      fbe_packet_t *packet)
 ***************************************************************************
 *
 * @brief
 *       Function to unbypass a drive slot.
 * 
 * @param
 *      eses_enclosure - The object receiving the event
 *      packet - The pointer to the fbe_packet_t.
 *
 * @return
 *       fbe_status_t.
 *
 * NOTES
 *
 * HISTORY
 *   19-Dec-2008 - Created. Joe Perry
 ***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_discoveryUnbypass(fbe_eses_enclosure_t *eses_enclosure, 
                                         fbe_packet_t *packet)
{
    fbe_payload_ex_t                       *payload = NULL;
    fbe_payload_discovery_operation_t   *payload_discovery_operation = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_edge_index_t                    server_index = 0;
    fbe_payload_discovery_status_t      discovery_status;

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);

    /* As a for now we have one client only with this particular id */
    status = fbe_base_discovering_get_server_index_by_client_id((fbe_base_discovering_t *) eses_enclosure, 
                                                        payload_discovery_operation->command.common_command.client_id,
                                                        &server_index);

    if(status != FBE_STATUS_OK) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "discoveryUnbypass, get_server_index_by_client_id fail, status: 0x%x.\n",
                                status);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, server_index %d.\n", __FUNCTION__, server_index);

    /* Check if server_index is an index of the drive */
    if(server_index < fbe_eses_enclosure_get_number_of_slots(eses_enclosure)) 
    {
        //This command is from the drive.
        status = fbe_eses_enclosure_unbypass_drive(eses_enclosure, (fbe_u8_t)server_index);
    }
    else if(server_index < fbe_eses_enclosure_get_first_expansion_port_index(eses_enclosure) + 
                           fbe_eses_enclosure_get_number_of_expansion_ports(eses_enclosure)) 
    {
        // This command is from the enclosure.
        status = fbe_eses_enclosure_unbypass_expansion_port(eses_enclosure, (fbe_u8_t)server_index);   
    }
    else 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s entry. discoveryUnbypass, invalid server_index 0x%x.\n",
                                __FUNCTION__, server_index);

        status = FBE_STATUS_GENERIC_FAILURE;
    }

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "discoveryUnbypass failed, client_id: %d, status: 0x%x.\n",
                                payload_discovery_operation->command.common_command.client_id,
                                status);

        discovery_status = FBE_PAYLOAD_DISCOVERY_STATUS_FAILURE;
    }
    else
    {
        discovery_status = FBE_PAYLOAD_DISCOVERY_STATUS_OK;
    }
    
    fbe_payload_discovery_set_status(payload_discovery_operation, discovery_status);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;  
}
    

/*!**************************************************************************
* @fn fbe_eses_enclosure_unbypass_drive(fbe_eses_enclosure_t * eses_enclosure, fbe_u8_t drive_slot_number)
***************************************************************************
* @brief
*       Function to unbypass a drive slot.
* 
* @param  eses_enclosure - The object receiving the event
* @param  slotIndex - The slot number of the drive to be unbypassed.
*
* @return
*       fbe_status_t.
*
* NOTES
*
* HISTORY
*   19-Dec-2008 - Created. Joe Perry
*   15-Jan-2008 - Changed the input parameters. PHE
******************************************************************************/
static fbe_status_t fbe_eses_enclosure_unbypass_drive(fbe_eses_enclosure_t * eses_enclosure, fbe_u8_t slotIndex)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_enclosure_status_t      enclStatus = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                    phyIndex = 0;
    fbe_u8_t                    slotNumber = 0;
     
    if(slotIndex >= fbe_eses_enclosure_get_number_of_slots(eses_enclosure)) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s: Invalid slotIndex: %d.\n",
                                __FUNCTION__, slotIndex);   

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                               FBE_ENCL_DRIVE_SLOT_NUMBER,
                                               FBE_ENCL_DRIVE_SLOT,
                                               slotIndex,
                                               &slotNumber);
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, slotNumber %d, slotIndex %d.\n", 
                            __FUNCTION__, slotNumber, slotIndex);

    enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                               FBE_ENCL_DRIVE_PHY_INDEX,
                                               FBE_ENCL_DRIVE_SLOT,
                                               slotIndex,
                                               &phyIndex);
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Enable this drive slot's PHY
    enclStatus = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                 FBE_ENCL_EXP_PHY_DISABLE,
                                                 FBE_ENCL_EXPANDER_PHY,
                                                 phyIndex,
                                                 FALSE);

    if(enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;   
    }    

    /*
    * Set condition to write out ESES Control
    */
    
    status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                    (fbe_base_object_t*)eses_enclosure, 
                                    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EXPANDER_CONTROL_NEEDED);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, can't set ControlNeeded condition, status: 0x%x.\n",
                                __FUNCTION__, status);
                
        return status;
    }
        
    return FBE_STATUS_OK;
   
}// End of function fbe_eses_enclosure_unbypass_drive

/*!**************************************************************************
* @fn fbe_eses_enclosure_unbypass_expansion_port(fbe_eses_enclosure_t * eses_enclosure, fbe_edge_index_t server_index)
***************************************************************************
* @brief
*       Function to unbypass a downstream enclosure.
* 
* @param  eses_enclosure - The object receiving the event.
* @param  server_index - The server index of the expansion port.
*
* @return
*       fbe_status_t.
*
* NOTES
*       We currently have only one expansion port. If later on it comes a fanout which means it will have multiple expansion port,
*       we should have server_index to indicate which expansion port. That is why the server_index is one of the input parameters.  
*
* HISTORY
*   15-Jan-2008 PHE - Created. 
******************************************************************************/
static fbe_status_t fbe_eses_enclosure_unbypass_expansion_port(fbe_eses_enclosure_t * eses_enclosure, fbe_edge_index_t server_index)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_enclosure_status_t      encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                    component_index = 0;
    fbe_bool_t                  is_local_connector = FALSE;
    fbe_u8_t                    phy_index;
    fbe_u8_t                    first_expansion_port_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u8_t                    connector_id_rqst = 0;
    fbe_u8_t                    connector_id = 0; 
     
    /* Check if the server_index is an index of the expansion port. */
    if((server_index >= fbe_eses_enclosure_get_first_expansion_port_index(eses_enclosure) + 
                       fbe_eses_enclosure_get_number_of_expansion_ports(eses_enclosure)) ||
       (server_index < fbe_eses_enclosure_get_first_expansion_port_index(eses_enclosure)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "unbypass_expansion_port, invalid server_index %d.\n",
                                 server_index);   

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Please see fbe_eses_enclosure_send_get_element_list_completion
     * and fbe_eses_enclosure_expansion_port_discovery_update for 
     * how the enclosure was instantiated.
     */
    first_expansion_port_index = fbe_eses_enclosure_get_first_expansion_port_index(eses_enclosure);
    connector_id_rqst = server_index - first_expansion_port_index;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "unbypass_expansion_port, server_index %d, connector_id_rqst %d.\n",
                                 server_index, connector_id_rqst);    

    for(component_index = 0; component_index < fbe_eses_enclosure_get_number_of_connectors(eses_enclosure); component_index ++)
    {
        /* Save the local flag in EDAL for the LCC. */              
        encl_status = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_COMP_IS_LOCAL, // Attribute.
                                            FBE_ENCL_CONNECTOR,  // component type.
                                            component_index,
                                            &is_local_connector);  // The value to be set to.

        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if(is_local_connector)
        {
            /* This is local connector. 
             * Continue to ge the attached sas address.
             */            
            encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_CONNECTOR_ID, // Attribute.
                                            FBE_ENCL_CONNECTOR,  // component type.
                                            component_index,
                                            &connector_id);  // The value to be set to.

            if(encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }

            if(connector_id == connector_id_rqst) 
            {
                encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_CONNECTOR_PHY_INDEX, // Attribute.
                                            FBE_ENCL_CONNECTOR,  // component type.
                                            component_index,
                                            &phy_index);  // The value to be set to.
                if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                // There is no phy associated with the entire connector. so we don't need to enable the phy for it.
                // The phy index for the entire connector is set to FBE_ENCLOSURE_VALUE_INVALID. 
                // So need to check the phy index here.
                if(phy_index != FBE_ENCLOSURE_VALUE_INVALID)
                {  
                    //Enable the phy whether it is disabled or enabled. If it is already enabled, the state will not be affected in any way.
                    encl_status = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_EXP_PHY_DISABLE,
                                            FBE_ENCL_EXPANDER_PHY,
                                            phy_index,
                                            FALSE);

                    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                    {
                        return FBE_STATUS_GENERIC_FAILURE;   
                    }
                }// End of - if(phy_index != FBE_ENCLOSURE_VALUE_INVALID)
            }// End of - if(!is_primary_port)
        }// End of - if(is_local_connector)  
    }// End of the for loop

    /*
    * Set condition to write out ESES Control
    */
    
    status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                    (fbe_base_object_t*)eses_enclosure, 
                                    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EXPANDER_CONTROL_NEEDED);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s unbypass_expansion_port, can't set ControlNeeded condition, status: 0x%x.\n",
                                __FUNCTION__,  status);
        
        return status;
    }

    return FBE_STATUS_OK;
}// End of function fbe_eses_enclosure_unbypass_enclosure


/*!*************************************************************************
* @fn fbe_eses_enclosure_is_rev_change
****************************************************************************
* @brief
*   This function checks if the firmware rev changed for the
*   firmware component being that is being activated.
*
* @param      eses_enclosure - pointer to fbe_eses_enclosure_t
*
* @return     fbe_bool_t.
*
* HISTORY
*   27-Oct-2008 GB - Created.
***************************************************************************/
fbe_bool_t fbe_eses_enclosure_is_rev_change(fbe_eses_enclosure_t *eses_enclosure_p)
{
    fbe_u32_t               status = FBE_STATUS_OK;
    fbe_u8_t                index = 0;
    fbe_edal_general_comp_handle_t generalDataPtr=NULL;
    fbe_enclosure_component_types_t fw_comp_type; 
    fbe_base_enclosure_attributes_t fw_comp_attr;
    fbe_edal_fw_info_t              fw_info;
    fbe_eses_encl_fw_component_info_t *eses_encl_fw_comp_info_p = NULL;
    fbe_eses_enclosure_format_fw_rev_t fwRevAfterFormatting = {0};

    // get the rev info
    fbe_edal_get_fw_target_component_type_attr(eses_enclosure_p->enclCurrentFupInfo.enclFupComponent,
                                               &fw_comp_type,
                                               &fw_comp_attr);

    status = (fbe_u32_t)fbe_eses_enclosure_map_fw_target_type_to_component_index(eses_enclosure_p, 
                                    fw_comp_type, 
                                    eses_enclosure_p->enclCurrentFupInfo.enclFupComponentSide, 
                                    0,  // Overall rev.
                                    &index);

    if (status != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                       FBE_TRACE_LEVEL_ERROR,
                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                       "is_rev_change: Failed to get component index for %s, side %d.\n", 
                       fbe_eses_enclosure_fw_targ_to_text(eses_enclosure_p->enclCurrentFupInfo.enclFupComponent),
                       eses_enclosure_p->enclCurrentFupInfo.enclFupComponentSide);

        return FBE_FALSE;
    }
    // the revision and identifier are in the public area, so need different access method
    status = (fbe_u32_t)fbe_base_enclosure_access_specific_component((fbe_base_enclosure_t *)eses_enclosure_p,
                                                           fw_comp_type,
                                                           index,
                                                           &generalDataPtr);
    if (status == FBE_STATUS_OK)
    {
        // copy the fw revision
        status = (fbe_u32_t)eses_enclosure_access_getStr(generalDataPtr,
                                      fw_comp_attr,
                                      fw_comp_type,
                                      sizeof(fw_info),
                                      (char *)&fw_info);    // dest of the copy

        if ((status == FBE_EDAL_STATUS_OK)&& (fw_info.valid))
        {
            // get a pointer to the eses enclosure FwInfo for this fbe component
            status = fbe_eses_enclosure_get_fw_target_addr(eses_enclosure_p,
                                                  eses_enclosure_p->enclCurrentFupInfo.enclFupComponent, 
                                                  eses_enclosure_p->enclCurrentFupInfo.enclFupComponentSide,
                                                  &eses_encl_fw_comp_info_p);
            // Compare the rev saved from the parsed config pages with the rev at the time of the upgrade.
            // They should be the same until the new image is activated and the new config pages are parsed.
            // A change indicates we're done.
            // Using memcmp in case null char encountered

            fbe_eses_enclosure_format_fw_rev(eses_enclosure_p, &fwRevAfterFormatting, &fw_info.fwRevision[0], FBE_EDAL_FW_REV_STR_SIZE);
            
            if((fwRevAfterFormatting.majorNumber == 0) && 
               (fwRevAfterFormatting.minorNumber == 0) &&
               (fwRevAfterFormatting.patchNumber == 0))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                       FBE_TRACE_LEVEL_ERROR,
                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                       "is_rev_change: REV CHANGED to 0, new rev %.16s, old rev %.16s, ignore rev change.\n", 
                       &fw_info.fwRevision[0],
                       &eses_encl_fw_comp_info_p->fwEsesFwRevision[0]);

                return FBE_FALSE;
            }
            else if (!fbe_equal_memory(fw_info.fwRevision, 
                                 eses_encl_fw_comp_info_p->fwEsesFwRevision, //eses_enclosure_p->enclFwInfo->subencl_fw_info[fw_target+side_id].fwEsesFwRevision, 
                                 FBE_EDAL_FW_REV_STR_SIZE))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                       FBE_TRACE_LEVEL_ERROR,
                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                       "is_rev_change: REV CHANGED, new rev %.16s, old rev %.16s.\n", 
                       &fw_info.fwRevision[0],
                       &eses_encl_fw_comp_info_p->fwEsesFwRevision[0]);

                return FBE_TRUE;
            }
        }
    }

    return FBE_FALSE;

} // end fbe_eses_enclosure_is_rev_change

/*!*************************************************************************
* @fn fbe_eses_enclosure_is_activate_timeout
***************************************************************************
*
* @brief
*   This function checks if the firmware download operation time has
*   exceeded the limit. The start time is referenced from 
*   fbe_eses_enclosure_t and the limit is currently hard-coded.
*
* @param      eses_enclosure - pointer to fbe_eses_enclosure_t
*
* @return     fbe_bool_t.
*
* HISTORY
*   27-Oct-2008 GB - Created.
***************************************************************************/
fbe_bool_t fbe_eses_enclosure_is_activate_timeout(fbe_eses_enclosure_t * eses_enclosure)
{
    fbe_u32_t   activation_period;
    fbe_u32_t   timeout_limit;

    // activation_period = current - start
    activation_period = fbe_get_elapsed_seconds(eses_enclosure->enclCurrentFupInfo.enclFupStartTime);

    if((eses_enclosure->eses_version_desc == ESES_REVISION_1_0) &&
       (eses_enclosure->enclCurrentFupInfo.enclFupComponent == FBE_FW_TARGET_LCC_MAIN)) 
    {
        timeout_limit = FBE_ESES_FW_ACTIVATE_AND_RESET_LIMIT;
    }
    else if((eses_enclosure->eses_version_desc == ESES_REVISION_2_0) &&
            ((eses_enclosure->enclCurrentFupInfo.enclFupComponent == FBE_FW_TARGET_LCC_MAIN) ||
             (eses_enclosure->enclCurrentFupInfo.enclFupComponent == FBE_FW_TARGET_LCC_FPGA) ||
             (eses_enclosure->enclCurrentFupInfo.enclFupComponent == FBE_FW_TARGET_LCC_INIT_STRING) ||
             (eses_enclosure->enclCurrentFupInfo.enclFupComponent == FBE_FW_TARGET_LCC_EXPANDER) ||
             (eses_enclosure->enclCurrentFupInfo.enclFupComponent == FBE_FW_TARGET_LCC_EXPANDER)))
    {
        timeout_limit = FBE_ESES_CDES2_FW_ACTIVATE_AND_RESET_LIMIT;
    }
    else if ((eses_enclosure->eses_version_desc == ESES_REVISION_2_0) &&
             (eses_enclosure->enclCurrentFupInfo.enclFupComponent == FBE_FW_TARGET_PS))
    {
        timeout_limit = FBE_ESES_CDES2_PS_CM_FW_ACTIVATE_AND_RESET_LIMIT;
    } else 
    {
        /* For PS, Cooling module and SPS */
        timeout_limit = FBE_ESES_PS_CM_FW_ACTIVATE_AND_RESET_LIMIT;
    }

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                       FBE_TRACE_LEVEL_INFO,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                       "%s, elapsed:%ds limit:%ds\n", 
                                       __FUNCTION__,  
                                       activation_period,
                                       timeout_limit);

    if ( activation_period > timeout_limit )
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
} // end fbe_eses_enclosure_is_activate_timeout
 
/*!*************************************************************************
* @fn  fbe_eses_enclosure_receive_diag_validate_page(fbe_eses_enclosure_t *eses_enclosure,
*                                                           ses_common_pg_hdr_struct * page_hdr,
*                                                           ses_pg_code_enum required_page_code)                
***************************************************************************
* @brief
*       This function performs the sanity check for the ESES page read by the 
*       recieve diag command. These pages have the common header struct.
*
* @param  eses_enclosure - The pointer to the enclosure.
* @param  pg_hdr - The pointer to the EMC ESES page header.
* @param  expected_page_code - The expected ESES page code.
*
* @return fbe_enclosure_status_t 
*     FBE_ENCLOSURE_STATUS_OK -- no error.
*     otherwise - error is found.
*   
* NOTES
*
* HISTORY
*   18-Nov-2008 PHE - Created.
*   8-Jul-2009 Prasenjeet - added the support to validate the cdb transferred count.
***************************************************************************/
static fbe_enclosure_status_t fbe_eses_enclosure_receive_diag_validate_page(fbe_eses_enclosure_t *eses_enclosure,
                                                            ses_pg_code_enum expected_pg_code,
                                                            fbe_payload_ex_t * payload)
                                                            
{
    fbe_u32_t                         page_size = 0;
    fbe_u16_t                         gen_code = 0;
    fbe_status_t                      status = FBE_STATUS_OK;
    fbe_enclosure_status_t      encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_lifecycle_state_t          lifecycle_state;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_u32_t                                transferred_count;     
    fbe_sg_element_t *                  sg_list;
    fbe_u8_t *                               status_page;   
    ses_common_pg_hdr_struct *   pg_hdr;
    fbe_enclosure_number_t enclosure_number = FBE_ENCLOSURE_VALUE_INVALID;
    
    /********
     * BEGIN
     ********/
    fbe_base_enclosure_get_enclosure_number((fbe_base_enclosure_t *)eses_enclosure, &enclosure_number);     
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);   

    status_page = (fbe_u8_t *)sg_list[0].address;
    pg_hdr = (ses_common_pg_hdr_struct *)status_page; 

    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    
    // Check page pointer.
    if(pg_hdr == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "%s,  NULL pointer for expected page code: 0x%x.\n", 
                           __FUNCTION__, expected_pg_code);  


        return FBE_ENCLOSURE_STATUS_ESES_PAGE_INVALID;
    }    
   
    // Check the page code.
    if(pg_hdr->pg_code != expected_pg_code)
    {
        if(pg_hdr->pg_code == SES_PG_CODE_ENCL_BUSY)
        {
            return FBE_ENCLOSURE_STATUS_BUSY;
        }
        else
        {
            // This is an invalid page.
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_ERROR,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                               "invalid page_code: 0x%X.\n", 
                                pg_hdr->pg_code);

            
            return FBE_ENCLOSURE_STATUS_ESES_PAGE_INVALID;
        }
    }

    // Check page size.
    page_size = fbe_eses_get_pg_size(pg_hdr);
   
    /*
     * FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE = FBE_MEMORY_CHUNK_SIZE - 2 * sizeof(fbe_sg_element_t)
     * This is limited to the memory size that the framework allocates.
     */
    if(page_size > FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE) 
    {
       fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                         FBE_TRACE_LEVEL_ERROR,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                         "not enough memory allocated,page_code 0x%x,page_size %d,allocated %llu.\n",
                          pg_hdr->pg_code, page_size, (unsigned long long)FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE);  
     
        // The allocated memory is less than the actual page size.
        // So this page is not a complete page. 
        return FBE_ENCLOSURE_STATUS_ALLOCATED_MEMORY_INSUFFICIENT;    
    }

    fbe_payload_cdb_get_transferred_count(payload_cdb_operation, &transferred_count);
    if( transferred_count != page_size)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                         FBE_TRACE_LEVEL_INFO,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                         "validate config page:xfer(0x%x) != pg size(0x%x),page code:0x%x, alloc:%llu\n",
                         payload_cdb_operation->transferred_count,
                         page_size,
                         pg_hdr->pg_code,
                         (unsigned long long)FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE);
        /////////////////kludge temporary workaround for expander returning wrong page size
        if ((pg_hdr->pg_code == SES_PG_CODE_DOWNLOAD_MICROCODE_STAT) &&
            ((transferred_count+4) == page_size))
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                         FBE_TRACE_LEVEL_INFO,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                         "allow failed validate check page code:0x%x  transferred count:%d\n",
                         pg_hdr->pg_code,
                         payload_cdb_operation->transferred_count);
        }
        else
        {
         // The transferred bytes don't match the page size
         return FBE_ENCLOSURE_STATUS_ESES_PAGE_INVALID;    
        }
    }

    fbe_base_object_trace((fbe_base_object_t*)eses_enclosure,
                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "esesEncl %d,transfered count is equal to page size ,transferred_count 0x%x,page_code 0x%x,page_size %d,allocated %llu.\n", 
                    payload_cdb_operation->transferred_count, enclosure_number, pg_hdr->pg_code, page_size, (unsigned long long)FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE);  
        

    /* Check the generation code if it is not the configuration page. 
     * It should be the same as the eses_generation_code.
     * The eses_generation_code is updated while processing the configuration page. 
     */
    gen_code = fbe_eses_get_pg_gen_code(pg_hdr);

    // microcode status page has been problematic, so add some tracing
    if (pg_hdr->pg_code == SES_PG_CODE_DOWNLOAD_MICROCODE_STAT)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                         FBE_TRACE_LEVEL_INFO,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                         "Validate page code 0x%x, gen code rcvd:%d gencode eses_encl:%d\n", 
                         pg_hdr->pg_code, gen_code, eses_enclosure->eses_generation_code);
    }

    if((pg_hdr->pg_code != SES_PG_CODE_CONFIG) && 
        (gen_code != eses_enclosure->eses_generation_code))
    {
        /* The config has been changed but we did not get the check condition with unit attention from EMA.
         * There might be other client talking to EMA directly.
         * In order to recover from this situation, we want to issue the command to read the configuration 
         * page. So set the condition 
         */
        
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                         FBE_TRACE_LEVEL_INFO,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "Validate page code 0x%x, gen code(0x%x) not equal to expected(0x%x).\n", 
                         pg_hdr->pg_code, gen_code, eses_enclosure->eses_generation_code);  

        status = fbe_lifecycle_get_state(&fbe_eses_enclosure_lifecycle_const,
                                    (fbe_base_object_t*)eses_enclosure,
                                    &lifecycle_state);

        if((status == FBE_STATUS_OK) && (lifecycle_state == FBE_LIFECYCLE_STATE_READY))
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "Changing state from READY to ACTIVATE. Reason:generation_code:%d is changed from:%d.\n", 
                                gen_code, eses_enclosure->eses_generation_code);
        }

        /* We don't need to set the Additional Status page and EMC specific page here. 
         * It will be set when the configuration page successfully completes.
         */
        encl_status = fbe_base_enclosure_set_lifecycle_cond(
                            &fbe_eses_enclosure_lifecycle_const,
                            (fbe_base_enclosure_t *)eses_enclosure,
                            FBE_ESES_ENCLOSURE_LIFECYCLE_COND_CONFIGURATION_UNKNOWN);

        if(!ENCL_STAT_OK(encl_status))            
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "%s, can't set configuration unknown condition, status: 0x%x.\n",
                                __FUNCTION__, encl_status);

            encl_status = FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED;
        }             

        encl_status =  FBE_ENCLOSURE_STATUS_CMD_FAILED;
    }

    return encl_status;
}
    

/*!*************************************************************************
* @fn fbe_eses_enclosure_send_scsi_cmd_completion()                  
***************************************************************************
* @brief
*       It is the completion function for "send SCSI command".
*
* @param   packet - The pointer to the fbe_packet_t.
* @param   context - The completion context.
*
* @return   fbe_status_t. 
*         Alway returns FBE_STATUS_OK
*
* NOTES
*
* HISTORY
*   08-Dec-2008 PHE - Created.
*   8-July-2009 Prasenjeet - Added page validation check for Read diag command.
***************************************************************************/
static fbe_status_t fbe_eses_enclosure_send_scsi_cmd_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_eses_enclosure_t * eses_enclosure = (fbe_eses_enclosure_t *)context;
    fbe_sg_element_t  * sg_list = NULL; 
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * prev_control_operation = NULL;
    fbe_payload_control_operation_opcode_t opcode;
    fbe_scsi_error_code_t scsi_error_code = FBE_SCSI_CC_NOERR;
    fbe_time_t                      glitch_start_time = 0;


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "EsesEnclSendScsi entry\n");

    fbe_eses_enclosure_decrement_outstanding_scsi_request_count(eses_enclosure);
    fbe_eses_enclosure_set_outstanding_scsi_request_opcode(eses_enclosure, FBE_ESES_CTRL_OPCODE_INVALID);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    
    if(status != FBE_STATUS_OK)
    { 
        /* Something bad happen to child */
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "EsesEnclSendScsi, transport_get_stat_code fail,stat: 0x%x.\n", status); 

        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PACKET_FAILED);
        return FBE_STATUS_OK;  
    }

    payload = fbe_transport_get_payload_ex(packet);
    /* Get the pervious control operation, the current operation is the cdb operation. */  
    prev_control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    fbe_payload_control_get_opcode(prev_control_operation, &opcode);
    /*
     * Check IO request status and SCSI command status.
     * and handle the error status if needed.
     */
    encl_status = fbe_eses_enclosure_handle_scsi_cmd_response(eses_enclosure, 
                                       packet, 
                                       opcode,              // page opcode
                                       &scsi_error_code);   // Used to return the SCSI error code.
    
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    
    switch(opcode)
    {
        case FBE_ESES_CTRL_OPCODE_GET_INQUIRY_DATA:  // Inquiry command.
            if(encl_status == FBE_ENCLOSURE_STATUS_OK)
            {
                encl_status = fbe_eses_enclosure_handle_get_inquiry_data_response(eses_enclosure, sg_list);
            }
            break;
        
        case FBE_ESES_CTRL_OPCODE_VALIDATE_IDENTITY:  // Inquiry command.
            if(encl_status == FBE_ENCLOSURE_STATUS_OK)
            {
                encl_status = fbe_eses_enclosure_handle_validate_identity_response(eses_enclosure, sg_list);
            }
            break;            
    
        case FBE_ESES_CTRL_OPCODE_GET_CONFIGURATION: // Receive diag command.
            if(encl_status == FBE_ENCLOSURE_STATUS_OK)
            {
                encl_status = fbe_eses_enclosure_receive_diag_validate_page(eses_enclosure, SES_PG_CODE_CONFIG,payload);
                if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                      FBE_TRACE_LEVEL_INFO,
                                      fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                      "EsesEnclSendScsi,%s,rcvDiagValPg fail,enclStat:0x%x.\n", 
                                      fbe_eses_enclosure_print_scsi_opcode(opcode),encl_status);
                }
                else
                {
                    encl_status = fbe_eses_enclosure_handle_get_config_response(eses_enclosure, sg_list);
                }
            }
            break;

        case FBE_ESES_CTRL_OPCODE_GET_SAS_ENCL_TYPE: // Receive diag command.
            if(encl_status == FBE_ENCLOSURE_STATUS_OK)
            {
                encl_status = fbe_eses_enclosure_receive_diag_validate_page(eses_enclosure, SES_PG_CODE_CONFIG,payload);
                if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                      FBE_TRACE_LEVEL_INFO,
                                      fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                      "EsesEnclSendScsi,%s,rcvDiagValPg fail,enclStat:0x%x.\n", 
                                      fbe_eses_enclosure_print_scsi_opcode(opcode),encl_status);
                }
                else
                {
                    encl_status = fbe_eses_enclosure_handle_get_sas_encl_type_response(eses_enclosure, sg_list);
                }
            }
            break;

        case FBE_ESES_CTRL_OPCODE_GET_ENCL_STATUS: // Receive diag command.
            if(encl_status == FBE_ENCLOSURE_STATUS_OK)
            {
                encl_status = fbe_eses_enclosure_receive_diag_validate_page(eses_enclosure, SES_PG_CODE_ENCL_STAT,payload);
                if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                     // The allocated memory is less than the actual page size.
                     // So this page is not a complete page. 
                     fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                      FBE_TRACE_LEVEL_INFO,
                                      fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                      "EsesEnclSendScsi,%s,rcvDiagValPg fail,enclStat:0x%x.\n", 
                                      fbe_eses_enclosure_print_scsi_opcode(opcode),encl_status);
                }
                else
                {
                    encl_status = fbe_eses_enclosure_handle_get_status_response(eses_enclosure, sg_list); 
                }
            }
            break;

        case FBE_ESES_CTRL_OPCODE_GET_ADDITIONAL_STATUS: // Receive diag command.
            if(encl_status == FBE_ENCLOSURE_STATUS_OK)
            {
                encl_status = fbe_eses_enclosure_receive_diag_validate_page(eses_enclosure,SES_PG_CODE_ADDL_ELEM_STAT,payload);
                if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_INFO,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                     "EsesEnclSendScsi,%s,rcvDiagValPg fail,enclStat:0x%x.\n", 
                                     fbe_eses_enclosure_print_scsi_opcode(opcode),encl_status);
                }
                else
                {
                    encl_status = fbe_eses_enclosure_handle_get_addl_status_response(eses_enclosure, sg_list);
                }
            }
            break;

        case FBE_ESES_CTRL_OPCODE_GET_EMC_SPECIFIC_STATUS:// Receive diag command.
            if(encl_status == FBE_ENCLOSURE_STATUS_OK)
            {
                encl_status = fbe_eses_enclosure_receive_diag_validate_page(eses_enclosure,SES_PG_CODE_EMC_ENCL_STAT,payload);
                
                if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    // This is not a valid EMC specific page.
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_INFO,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                     "EsesEnclSendScsi,%s,rcvDiagValPg fail,enclStat:0x%x.\n", 
                                     fbe_eses_enclosure_print_scsi_opcode(opcode),encl_status);
                }
                else
                {
                    encl_status = fbe_eses_enclosure_handle_get_emc_specific_status_response(eses_enclosure, sg_list);
                }
            }
            break;        

        case FBE_ESES_CTRL_OPCODE_GET_TRACE_BUF_INFO_STATUS: // Receive diag command.
            if(encl_status == FBE_ENCLOSURE_STATUS_OK)
            {
                encl_status = fbe_eses_enclosure_receive_diag_validate_page(eses_enclosure, SES_PG_CODE_EMC_ENCL_STAT,payload);
                
                if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    // This is not a valid EMC specific page.
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_INFO,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                     "EsesEnclSendScsi,%s,rcvDiagValPg fail,enclStat:0x%x.\n", 
                                     fbe_eses_enclosure_print_scsi_opcode(opcode),encl_status);
                           
                } 
                else
                {
                    encl_status = fbe_eses_enclosure_handle_get_trace_buf_info_status_response(eses_enclosure, packet);
                }
            }
            break;

        case FBE_ESES_CTRL_OPCODE_RAW_RCV_DIAG:                 // Read buffer command.
            if(encl_status == FBE_ENCLOSURE_STATUS_OK)
            {
                encl_status = fbe_eses_enclosure_handle_raw_rcv_diag_response(eses_enclosure, packet);
            }
            break;

        case FBE_ESES_CTRL_OPCODE_RAW_INQUIRY:                 // Inquiry command.
            if(encl_status == FBE_ENCLOSURE_STATUS_OK)
            {
                encl_status = fbe_eses_enclosure_handle_raw_inquiry_response(eses_enclosure, packet);
            }
            break;
        
        case FBE_ESES_CTRL_OPCODE_READ_BUF:                 // Read buffer command.
        case FBE_ESES_CTRL_OPCODE_READ_RESUME:              // read resume prom.
            if(encl_status == FBE_ENCLOSURE_STATUS_OK)
            {
                encl_status = fbe_eses_enclosure_handle_read_buf_response(eses_enclosure, packet);
            }
            break;

        case FBE_ESES_CTRL_OPCODE_GET_DOWNLOAD_STATUS: // Receive diag command.
            if(encl_status == FBE_ENCLOSURE_STATUS_OK)
            {
                encl_status = fbe_eses_enclosure_receive_diag_validate_page(eses_enclosure, SES_PG_CODE_DOWNLOAD_MICROCODE_STAT,payload);
                
                if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    // This is an invalid page.
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "EsesEnclSendScsi,%s,rcvDiagValPg fail,enclStat:0x%x.\n", 
                                    fbe_eses_enclosure_print_scsi_opcode(opcode),encl_status);  
                } 
            }
                
            /* The function fbe_eses_enclosure_handle_get_download_status_response, 
             * processes some stuff based on the encl_status
             * So we did not put it inside of if(encl_status == FBE_ENCLOSURE_STATUS_OK) block.
             */
            encl_status = fbe_eses_enclosure_handle_get_download_status_response(eses_enclosure, sg_list, encl_status);
                
            
            break;

        case FBE_ESES_CTRL_OPCODE_GET_EMC_STATISTICS_STATUS:// Receive diag command.
            if(encl_status == FBE_ENCLOSURE_STATUS_OK)
            {
                encl_status = fbe_eses_enclosure_receive_diag_validate_page(eses_enclosure,SES_PG_CODE_EMC_STATS_STAT,payload);
               
                if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    // This is an invalid page.
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "EsesEnclSendScsi,%s,rcvDiagValPg fail,enclStat:0x%x.\n", 
                                    fbe_eses_enclosure_print_scsi_opcode(opcode),encl_status);
                }
                else
                {
                       
                    encl_status = fbe_eses_enclosure_handle_get_emc_statistics_status_response(eses_enclosure, sg_list);
                }
            }
            break;

        case FBE_ESES_CTRL_OPCODE_EMC_STATISTICS_STATUS_CMD:    // Receive diag command
            if(encl_status == FBE_ENCLOSURE_STATUS_OK)
            {
                encl_status = fbe_eses_enclosure_receive_diag_validate_page(eses_enclosure,SES_PG_CODE_EMC_STATS_STAT,payload);
               
                if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    // This is an invalid page.
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "EsesEnclSendScsi,%s,rcvDiagValPg fail,enclStat:0x%x.\n", 
                                    fbe_eses_enclosure_print_scsi_opcode(opcode),encl_status);
                }
                else
                {
                       
                    encl_status = fbe_eses_enclosure_handle_emc_statistics_status_cmd_response(eses_enclosure, packet);
                }
            }
            break;
            
        case FBE_ESES_CTRL_OPCODE_THRESHOLD_IN_CMD:    // send diag command
            if(encl_status == FBE_ENCLOSURE_STATUS_OK)
            {
                encl_status = fbe_eses_enclosure_handle_threshold_in_response(eses_enclosure, packet);
            }
            break;          

        case FBE_ESES_CTRL_OPCODE_SET_ENCL_CTRL: // send diag command.   
            if(encl_status == FBE_ENCLOSURE_STATUS_OK)
            {
                encl_status = fbe_eses_control_page_write_complete(eses_enclosure);         
            }
            break;

        case FBE_ESES_CTRL_OPCODE_SET_EMC_SPECIFIC_CTRL:    // send diag command.
            if(encl_status == FBE_ENCLOSURE_STATUS_OK)
            {
                encl_status = fbe_eses_emc_encl_ctrl_page_write_complete(eses_enclosure); 
            }
            break;

        case FBE_ESES_CTRL_OPCODE_DOWNLOAD_FIRMWARE:  // send diag command.
            if ((encl_status == FBE_ENCLOSURE_STATUS_CMD_FAILED) || 
                (encl_status == FBE_ENCLOSURE_STATUS_ILLEGAL_REQUEST))
            {
                // Set fail and request status if this is not the first page 
                // or a retry has already been done.
                if ((eses_enclosure->enclCurrentFupInfo.enclFupOperationRetryCnt != 0) || 
                    (eses_enclosure->enclCurrentFupInfo.enclFupBytesTransferred != 0))
                {
                    // We won't do a retry on this error.
                    // Need to issue receive diag in order to get the ucode download status 
                    // page which will provide the fault details. Set the operation status
                    // to tell the monitor completion function what to do.
                    eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_FAIL;
                    eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_REQ;

                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "EsesEnclSendScsi,DOWNLOAD_FIRMWARE encl_stat:0x%x sts/extsts:0x%x/0x%x\n", 
                                    encl_status,eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus,
                                    eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus);                    
                    // Set packet status to failure - for download this will trigger get 
                    // download status, the other operations don't care.
                    // Since we have changed the enclFupOperationStatus and enclFupOperationAdditionalStatus,
                    // Why do we need to manipulate the packet status? -- PINGHE
                    // we don't need to set transport status, in fact the monitor completion will not check it
                    // and encl_status will be passed back, and used by monitor completion.
                    // fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                }
            }
            else if ((encl_status != FBE_ENCLOSURE_STATUS_BUSY) && (encl_status != FBE_ENCLOSURE_STATUS_CDB_REQUEST_FAILED))
            {
                // retry when busy or failed cdb request, otherwise clear the retry count
                eses_enclosure->enclCurrentFupInfo.enclFupOperationRetryCnt = 0;

                // If image activation was just sent with good status returned, set a special 
                // glitch ride through period to cover long enclosure delays(for lcc case). Not done 
                // for power supply or cooling module fup.
                if ((eses_enclosure->enclCurrentFupInfo.enclFupOperation == FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE) &&
                     ((eses_enclosure->enclCurrentFupInfo.enclFupComponent == FBE_FW_TARGET_LCC_MAIN) ||
                     (eses_enclosure->enclCurrentFupInfo.enclFupComponent == FBE_FW_TARGET_LCC_EXPANDER) ||
                     (eses_enclosure->enclCurrentFupInfo.enclFupComponent == FBE_FW_TARGET_LCC_FPGA) ||
                     (eses_enclosure->enclCurrentFupInfo.enclFupComponent == FBE_FW_TARGET_LCC_INIT_STRING) ||
                     (eses_enclosure->enclCurrentFupInfo.enclFupComponent == FBE_FW_TARGET_LCC_BOOT_LOADER)))
                {
                    // If the no glitch in progress, set a special glitch 
                    // ride through period to cover potential long enclosure reset delay
                    // a new image activation.
                    fbe_base_enclosure_get_time_ride_through_start((fbe_base_enclosure_t*)eses_enclosure, 
                                                    &glitch_start_time);
                    if (glitch_start_time == 0)
                    {
                        fbe_base_enclosure_set_expected_ride_through_period(
                                    (fbe_base_enclosure_t *)eses_enclosure, 
                                    FBE_ESES_FW_RESET_TIME_LIMIT);
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                    "scsi_cmd_completion, new glitch period:%d\n", 
                                    FBE_ESES_FW_RESET_TIME_LIMIT);  
                    }
                }
            }
            break;

        case FBE_ESES_CTRL_OPCODE_TUNNEL_GET_CONFIGURATION:  // tunnel receive configuration command.
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_INFO,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                           "FBE FUP scsi_cmd_completion, tunnel receive configuration, encl_status 0x%x scsi_error_code 0x%x\n",
                                           encl_status, scsi_error_code);
            // Set the event for the next cycle of the eses_enclosure_firmware_download_cond_function function. 
            fbe_eses_tunnel_fup_context_set_event(&(eses_enclosure->tunnel_fup_context_p),
                                                  fbe_eses_tunnel_fup_encl_status_to_event(encl_status));
            break;

        case FBE_ESES_CTRL_OPCODE_TUNNEL_DOWNLOAD_FIRMWARE:  // tunnel send diag command.
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                           "FBE FUP scsi_cmd_completion, tunnel download, encl_status 0x%x scsi_error_code 0x%x\n",
                                           encl_status, scsi_error_code);
            // Set the event for the next cycle of the eses_enclosure_firmware_download_cond_function function. 
            fbe_eses_tunnel_fup_context_set_event(&(eses_enclosure->tunnel_fup_context_p),
                                                  fbe_eses_tunnel_fup_encl_status_to_event(encl_status));
            if ((FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE == eses_enclosure->enclCurrentFupInfo.enclFupOperation) &&
                (eses_enclosure->enclCurrentFupInfo.enclFupUseTunnelling == TRUE)&&
                (FBE_ENCLOSURE_STATUS_OK == encl_status)) 
            {
               // Image activation was jut sent with good status returned, set a special glitch ride
               // through period to cover long enclosure delays if the no glitch in progress.
               fbe_base_enclosure_get_time_ride_through_start((fbe_base_enclosure_t*)eses_enclosure, &glitch_start_time);
               if (0 == glitch_start_time)
               {
                  fbe_base_enclosure_set_expected_ride_through_period((fbe_base_enclosure_t *)eses_enclosure,
                                                                       FBE_ESES_FW_RESET_TIME_LIMIT);
                  fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_INFO,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                           "FBE FUP activate command completed. New glitch period %d seconds.\n",
                                           FBE_ESES_FW_RESET_TIME_LIMIT);
                }
            }
            break;

        case FBE_ESES_CTRL_OPCODE_TUNNEL_GET_DOWNLOAD_STATUS:  // tunnel receive diag command.
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                           "FBE FUP scsi_cmd_completion, tunnel get download status, encl_status 0x%x scsi_error_code 0x%x\n",
                                           encl_status, scsi_error_code);
            // Set the event for the next cycle of the eses_enclosure_firmware_download_cond_function function. 
            fbe_eses_tunnel_fup_context_set_event(&(eses_enclosure->tunnel_fup_context_p),
                                                  fbe_eses_tunnel_fup_encl_status_to_event(encl_status));
            break;

        case FBE_ESES_CTRL_OPCODE_GET_TUNNEL_CMD_STATUS:  // receive diag status page 0x83.
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                           "FBE FUP scsi_cmd_completion, get tunnel cmd status, encl_status 0x%x scsi_error_code 0x%x\n",
                                           encl_status, scsi_error_code);
            if (encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
               eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_UNKNOWN;
            }
            else 
            {
               // We have gotten a status page.
               encl_status = fbe_eses_enclosure_handle_status_page_83h(eses_enclosure, sg_list);
            }
            // Set the event for the next cycle of the eses_enclosure_firmware_download_cond_function function. 
            fbe_eses_tunnel_fup_context_set_event(&(eses_enclosure->tunnel_fup_context_p),
                                                  fbe_eses_tunnel_fup_encl_status_to_event(encl_status));
            break;

        case FBE_ESES_CTRL_OPCODE_MODE_SENSE:  // Mode sense command.
            if(encl_status == FBE_ENCLOSURE_STATUS_OK)
            {
                encl_status = fbe_eses_enclosure_handle_mode_sense_response(eses_enclosure, 
                                                                            packet,
                                                                            sg_list);
            }
            break;

        case FBE_ESES_CTRL_OPCODE_MODE_SELECT:              // Mode select command.
            if(encl_status == FBE_ENCLOSURE_STATUS_OK)
            {
                eses_enclosure->test_mode_status = eses_enclosure->test_mode_rqstd;
            }
            break;

        case FBE_ESES_CTRL_OPCODE_SET_TRACE_BUF_INFO_CTRL:  //send diag command.
        case FBE_ESES_CTRL_OPCODE_STRING_OUT_CMD:           //send diag command.
        case FBE_ESES_CTRL_OPCODE_WRITE_RESUME:             // write resume prom.
        case FBE_ESES_CTRL_OPCODE_THRESHOLD_OUT_CMD:        //send diag command
            // No further processing needed.
            break;          

        case FBE_ESES_CTRL_OPCODE_SPS_IN_BUF_CMD:           //send diag command
            if (sg_list->address != NULL)
            {
                fbe_transport_release_buffer(sg_list->address);
            }
            break;

        case FBE_ESES_CTRL_OPCODE_SPS_EEPROM_CMD:
            encl_status = fbe_eses_enclosure_handle_sps_eeprom_response(eses_enclosure, 
                                                                        packet,
                                                                        sg_list);
            break;

        case FBE_ESES_CTRL_OPCODE_GET_RP_SIZE:
            encl_status = fbe_eses_enclosure_handle_get_rp_size_response(eses_enclosure, sg_list);
            break;

        default:
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "EsesEnclSendScsi, invalid opcode: 0x%x.\n", opcode); 
            
            encl_status = FBE_ENCLOSURE_STATUS_PARAMETER_INVALID;
            break;
    } // End of - switch(page_code)    

    /* Set the status and status qualifier for the control operaton and clean up the cdb operation and the sg list.
     * The caller will decide if the command needs to be retried based on the status and status qualifier.
     */
    fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_OK, encl_status);

    return FBE_STATUS_OK;

} // End of - fbe_eses_enclosure_send_scsi_cmd_completion

/*!*************************************************************************
*   @fn fbe_eses_control_page_write_complete()                  
***************************************************************************
*
* DESCRIPTION
*       @brief
*       This function will handle control page completion.  It's only 
* called when the control page completed successfully.
*
* PARAMETERS
*       @param eses_enclosure - The pointer to the fbe_eses_enclosure_t.
*
* RETURN VALUES
*       @return fbe_enclosure_status_t.
*
* NOTES
*
* HISTORY
*   18-Dec-2008     NC - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_control_page_write_complete(fbe_eses_enclosure_t * eses_enclosure)
{
    fbe_enclosure_status_t                  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_status_t                            status;
    fbe_u32_t                               index;
    fbe_bool_t                              writeDataFound, writeDataSent, clearOverallStatus;
    fbe_u32_t                               componentOverallStatus;
    fbe_u8_t                                maxNumberOfComponents;
    fbe_enclosure_component_types_t         componentType;
    fbe_bool_t                              more_processing_required = FALSE;


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, outstanding write %d\n", 
                            __FUNCTION__,  eses_enclosure->outstanding_write);

    /* looping through all types, edal will find out which block it's in */
    for (componentType = 0; componentType < FBE_ESES_ENCLOSURE_MAX_COMPONENT_TYPE; componentType++)
    {
        clearOverallStatus = TRUE;
        // check if there is any Write/Control data for this component type
        encl_status = fbe_base_enclosure_edal_getOverallStatus((fbe_base_enclosure_t *)eses_enclosure,
                                                                componentType,
                                                                &componentOverallStatus);

        // it's not an error if this particular component is not available for this platform
        if (encl_status == FBE_ENCLOSURE_STATUS_COMPONENT_NOT_FOUND)
        {
            encl_status = FBE_ENCLOSURE_STATUS_OK; 
            continue;  // not interested in this type, move on
        }

        if ((encl_status == FBE_ENCLOSURE_STATUS_OK) &&
            (componentOverallStatus == FBE_ENCL_COMPONENT_TYPE_STATUS_WRITE_NEEDED))
        {
            // loop through all components
            status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)eses_enclosure,
                                                            componentType,
                                                            &maxNumberOfComponents);
            if (status != FBE_STATUS_OK)
            {
                continue;
            }

            for (index = 0; index < maxNumberOfComponents; index++)
            {
                // check if this component has sent Write/Control data
                encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_WRITE_DATA_SENT,
                                                            componentType,
                                                            index,
                                                            &writeDataSent);
                /* check if this component has sent Write/Control data
                    * We need to check this, in case of a WRITE_DATA is set and
                    * WRITE_DATA_SENT not set, we should not clear the overallStatus
                    */
                encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_WRITE_DATA,
                                                            componentType,
                                                            index,
                                                            &writeDataFound);
                if (encl_status == FBE_ENCLOSURE_STATUS_OK) 
                {
                    if (writeDataSent!=writeDataFound)
                    {
                        clearOverallStatus = FALSE;
                    }

                    // if we have sent
                    if (writeDataSent)
                    {
                        // The function clears both FBE_ENCL_COMP_WRITE_DATA and FBE_ENCL_COMP_WRITE_DATA_SENT flags
                        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                                        FBE_ENCL_COMP_WRITE_DATA,
                                                                        componentType,
                                                                        index,
                                                                        FALSE);
                        if (encl_status != FBE_ENCLOSURE_STATUS_OK) 
                        {
                            return encl_status;
                        }
                        /* We don't have per component hook, so we are clearing drive related bits directly */
                        switch(componentType)
                        {
                            case FBE_ENCL_CONNECTOR:
                                 /* clear the connector swap bit */
                                 encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                                            FBE_ENCL_COMP_CLEAR_SWAP,
                                                                            componentType,
                                                                            index,
                                                                            FALSE);
                                if(!ENCL_STAT_OK(encl_status))
                                {
                                     return encl_status;
                                 }
                                 break ;

                            default:
                                break;
                        }
                                             
                    }
                }
                else
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "%s, can't access component %s index %d, enclStatus: 0x%x.\n",
                                            __FUNCTION__,  
                                            enclosure_access_printComponentType(componentType), 
                                            index, encl_status);
                    
                }

            }   // for index

            if (clearOverallStatus)
            {
                // clear the overall status if needed
                encl_status = fbe_base_enclosure_edal_setOverallStatus((fbe_base_enclosure_t *)eses_enclosure,
                                                        componentType,
                                                        FBE_ENCL_COMPONENT_TYPE_STATUS_OK);
                if (encl_status != FBE_ENCLOSURE_STATUS_OK) 
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "%s, can't set overall status for component %s, enclStatus: 0x%x.\n",
                                            __FUNCTION__,  
                                            enclosure_access_printComponentType(componentType), 
                                            encl_status);
                }
            }
            else
            {
                more_processing_required = TRUE;
            }
        }// if write needed
    } // all types

    if(more_processing_required)
    {
        return FBE_ENCLOSURE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        return FBE_ENCLOSURE_STATUS_OK;
    }
}// End of - fbe_eses_control_page_write_complete

/*!*************************************************************************
*   @fn fbe_eses_emc_encl_ctrl_page_write_complete()                  
***************************************************************************
* @brief
*       This function will handle EMC enclosure control page completion.  
*       It's only called when the EMC enclosure control page completed successfully.
*
* @param eses_enclosure - The pointer to the fbe_eses_enclosure_t.
*
* @return fbe_enclosure_status_t.
*
* NOTES
*
* HISTORY
*   20-May-2009     PHE - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_emc_encl_ctrl_page_write_complete(fbe_eses_enclosure_t * eses_enclosure)
{  
    fbe_enclosure_component_types_t         componentType;
    fbe_u8_t                                maxNumberOfComponents;
    fbe_u8_t                                index;
    fbe_bool_t                              writeDataFound;
    fbe_bool_t                              writeDataSent;    
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_enclosure_status_t                  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_bool_t                              more_processing_required = FALSE;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "emc_encl_ctrl_page_write_complete, outstanding_write: %d.\n", 
                             eses_enclosure->emc_encl_ctrl_outstanding_write);

    /* looping through all xomponent types. */
    for (componentType = 0; componentType < FBE_ESES_ENCLOSURE_MAX_COMPONENT_TYPE; componentType++)
    {            
        switch(componentType)
        {
            case FBE_ENCL_CONNECTOR: // for sas connector info element group in EMC encl control page.
            case FBE_ENCL_ENCLOSURE: // for encl time info elem group in EMC encl control page.
            case FBE_ENCL_DRIVE_SLOT:  // for general info elem group in EMC encl control page.

                status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)eses_enclosure,
                                                                componentType,
                                                                &maxNumberOfComponents);
                if (status != FBE_STATUS_OK)
                {
                    return FBE_ENCLOSURE_STATUS_DATA_ACCESS_FAILED;
                }

                // loop through all components
                for (index = 0; index < maxNumberOfComponents; index++)
                {
                    /* Check both WRITE_DATA and WRITE_DATA_SENT attributes.
                    * If WRITE_DATA is set but WRITE_DATA_SENT not set such as 
                    * a new write request comes after the first request was sent and before this function is called,
                    * we should send out the write request again i. e. the WRITE_DATA attribute can not be cleared.
                    */
                    encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA,
                                                                componentType,
                                                                index,
                                                                &writeDataFound);

                    if (!ENCL_STAT_OK(encl_status))
                    {
                        return encl_status;
                    }

                    encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA_SENT,
                                                                componentType,
                                                                index,
                                                                &writeDataSent);

                    if (!ENCL_STAT_OK(encl_status))
                    {
                        return encl_status;
                    }

                    
                    if (writeDataSent)
                    {
                        /* The write request has been sent.
                        * The function clears both FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA
                        * and FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA_SENT attributes.
                        */ 
                        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                                        FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA,
                                                                        componentType,
                                                                        index,
                                                                        FALSE);
                        if (!ENCL_STAT_OK(encl_status))
                        {
                            return encl_status;
                        }
                        /* For enclosure, after we clear the reset reason, we need to read it again to make 
                         * sure it really is cleaned up.
                         */
                        if (componentType == FBE_ENCL_ENCLOSURE)
                        {
                            if (eses_enclosure->reset_reason == FBE_ESES_RESET_REASON_EXTERNAL)
                            {
                                encl_status = fbe_base_enclosure_set_lifecycle_cond(
                                                &fbe_eses_enclosure_lifecycle_const,
                                                (fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EMC_SPECIFIC_STATUS_UNKNOWN);

                                if(!ENCL_STAT_OK(encl_status))            
                                {
                                    /* Failed to set the emc specific status condition. */
                                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                        FBE_TRACE_LEVEL_ERROR,
                                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                        "write completion, can't set emc specific status unknown condition, status: 0x%x.\n",
                                                        encl_status);
                                }

                            }
                        }
                    }
                    else if(writeDataFound)
                    {
                        /* There is a new request coming down before we reach this point. 
                         * The condition should not be cleared. 
                         */
                        more_processing_required = TRUE; 
                    }

                } // End of - for (index = 0; index < maxNumberOfComponents; index++)  
                break;

            default:
                break;
        }// End of - switch(componentType)
    } // End of - for (componentType = 0; componentType < FBE_ESES_ENCLOSURE_MAX_COMPONENT_TYPE; componentType++)

    if(more_processing_required)
    {
        return FBE_ENCLOSURE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        return FBE_ENCLOSURE_STATUS_OK;
    }
}// End of - fbe_eses_emc_encl_ctrl_page_write_complete


/*!*************************************************************************
* @fn fbe_eses_enclosure_handle_raw_inquiry_response(fbe_eses_enclosure_t * eses_enclosure, 
*                                                 fbe_packet_t * packet)             
***************************************************************************
* @brief
*       This function handles the response got from the SCSI Inquiry command.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return  fbe_enclosure_status_t.
*      FBE_ENCLOSURE_STATUS_OK - no error.
*      otherwise - error is found.
*
* NOTES
*
* HISTORY
*   26-Mar-2009 AS - Created.
***************************************************************************/
static fbe_enclosure_status_t fbe_eses_enclosure_handle_raw_inquiry_response(fbe_eses_enclosure_t * eses_enclosure, 
                                                                          fbe_packet_t * packet)
{
    fbe_payload_ex_t *payload = NULL;
    fbe_sg_element_t  *sg_list = NULL; 
    fbe_status_t status = FBE_STATUS_OK;

    fbe_payload_control_buffer_t control_buffer = NULL;
    fbe_enclosure_mgmt_ctrl_op_t *eses_page_op = NULL;

    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_payload_control_operation_t *prev_control_operation = NULL;
  

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry \n", __FUNCTION__ );
   
    payload = fbe_transport_get_payload_ex(packet);
    /* Get the pervious control operation */  
    prev_control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    fbe_payload_control_get_buffer(prev_control_operation, &control_buffer);
    if( control_buffer == NULL )
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_WARNING,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fbe_payload_control_get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                                __FUNCTION__, status, control_buffer);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_ENCLOSURE_STATUS_PACKET_FAILED;
    }

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    eses_page_op = (fbe_enclosure_mgmt_ctrl_op_t *)control_buffer;

    /* Check for the EVPD value. If 0, then call get_inquiry_pg_size to get 
     * required response buffer size for the standard inquiry page, else, a 
     * regular call to get_pg_size will give the corresponding EVPD page size.
     */
    if ( eses_page_op->cmd_buf.raw_scsi.raw_inquiry.evpd )
    {
        eses_page_op->required_response_buf_size = fbe_scsi_get_vpd_inquiry_pg_size(sg_list[0].address);
    }
    else
    {
        eses_page_op->required_response_buf_size = fbe_scsi_get_standard_inquiry_pg_size(sg_list[0].address);
    }

    /* Check the size. If the user allocated buffer size is less than required, then the 
     * page should be truncated at this length.
     */
    if (eses_page_op->response_buf_size < eses_page_op->required_response_buf_size)
    {            
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, not enought memory allocated. allocated: %d, required: %d.\n",
                                __FUNCTION__, eses_page_op->response_buf_size, 
                                eses_page_op->required_response_buf_size); 
        
        // Update the enclosure status.
        encl_status =  FBE_ENCLOSURE_STATUS_ALLOCATED_MEMORY_INSUFFICIENT;
    }

    /* 
     * Do not need to copy memory from sg_list[0].address to eses_page_op->response_buf_p because 
     * sg_list[0].address points to the same memory that eses_page_op->response_buf_p points to.
     */

    return encl_status;
} // End of - fbe_eses_enclosure_handle_raw_inquiry_response

/*!*************************************************************************
 * @fn fbe_eses_enclosure_handle_emc_statistics_status_cmd_response(fbe_eses_enclosure_t * eses_enclosure, 
 *                                                        fbe_packet_t * packet)                 
 ***************************************************************************
 * @brief
 *       This function handles the response got from the "receive diag" command 
 *       with the "FBE_ESES_CTRL_OPCODE_EMC_STATISTICS_STATUS_CMD" opcode.
 *       It calls the function to parse the EMC Statistics status page.
 *
 *       Get the previous control buffer and compare the allocated response 
 *       buffer size with the size of the trace buffer info status. The data
 *       returned from the recieve diag command are copied to the response bufffer.
 *
 * @param eses_enclosure - The pointer to the ESES enclosure.
 * @param sg_list - the pointer to the sg list allocated while sending the command.
 *
 * @return  fbe_enclosure_status_t.
 *      FBE_ENCLOSURE_STATUS_OK - no error.
 *      otherwise - error is found.
 *
 * NOTES
 *
 * HISTORY
 *   21-Apr-2009 AS - Created.
 ***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_handle_emc_statistics_status_cmd_response(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_enclosure_status_t          encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_number_t          enclosure_number = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_payload_ex_t *                 payload = NULL;
    fbe_payload_control_operation_t * prev_control_operation = NULL;
    fbe_payload_control_buffer_t    control_buffer = NULL;
    fbe_enclosure_mgmt_ctrl_op_t    * eses_page_op = NULL;
    fbe_sg_element_t                * sg_list = NULL;


    fbe_base_enclosure_get_enclosure_number((fbe_base_enclosure_t *)eses_enclosure, &enclosure_number);

    fbe_base_object_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry, Encl: %d.\n", __FUNCTION__, enclosure_number);
    
    payload = fbe_transport_get_payload_ex(packet);
    /* Get the previous control operation */  
    prev_control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    fbe_payload_control_get_buffer(prev_control_operation, &control_buffer);
    if( control_buffer == NULL )
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_WARNING,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "fbe_payload_control_get_buffer returned fail: status:0x%x, control_buffer: %64p.\n",
                                status, 
                                control_buffer);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_ENCLOSURE_STATUS_PACKET_FAILED;
    }

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    eses_page_op = (fbe_enclosure_mgmt_ctrl_op_t *)control_buffer;
    //statistics_cmd_p = &(eses_page_op->cmd_buf.element_statistics); ----use eses_page_op

    // response_buf_size is the user allocated number of bytes
    // requires_response_buf_size is the actual page number of bytes returned from eses
    eses_page_op->required_response_buf_size = 
        fbe_eses_get_pg_size((ses_common_pg_hdr_struct *)sg_list[0].address);

    /* Zero out the memory allocated. */
    fbe_zero_memory(eses_page_op->response_buf_p, eses_page_op->response_buf_size);

    // parse the statistics page: go through the page data and 
    // copy the targeted data into the control op response buffer
    encl_status = fbe_eses_parse_statistics_page(eses_enclosure,
                                               eses_page_op,    // details of the command (element type, slot, etc)
                                               (ses_pg_emc_statistics_struct*)sg_list[0].address);   // (source) data to parse (from eses)

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "fbe_eses_parse_statistics_page failed-encl_status: 0x%x\n",
                                encl_status);
    
        encl_status = FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED;
    }

    return encl_status;
} //fbe_eses_enclosure_handle_emc_statistics_status_cmd_response

/*!*************************************************************************
* @fn fbe_eses_enclosure_handle_get_emc_statistics_status_response(fbe_eses_enclosure_t * eses_enclosure, 
*                                                        fbe_sg_element_t * sg_list)                  
***************************************************************************
* @brief
*       This function handles the response got from the "receive diag" command 
*       with the "FBE_ESES_CTRL_OPCODE_GET_EMC_STATISTICS_STATUS" opcode.
*       It calls the function to parse the EMC statistics status page.
*
* @param eses_enclosure - The pointer to the ESES enclosure.
* @param sg_list - the pointer to the sg list allocated while sending the command.
*
* @return  fbe_enclosure_status_t.
*      FBE_ENCLOSURE_STATUS_OK - no error.
*      otherwise - error is found.
*
* NOTES
*
* HISTORY
*   01-Sep-2009 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_handle_get_emc_statistics_status_response(fbe_eses_enclosure_t * eses_enclosure, 
                                                             fbe_sg_element_t * sg_list)
{
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__);
     
    encl_status = fbe_eses_enclosure_process_emc_statistics_status_page(eses_enclosure, 
                                                (ses_pg_emc_statistics_struct *)sg_list[0].address);
    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s process_emc_statistics_status_page failed, encl_status: 0x%x.\n",
                                __FUNCTION__, encl_status);

    }       
    
    return encl_status;
}


/*!*************************************************************************
 * @fn fbe_eses_enclosure_threshold_in_response(fbe_eses_enclosure_t * eses_enclosure, 
 *                                                        fbe_packet_t * packet)                 
 ***************************************************************************
 * @brief
 *       This function handles the response got from the "receive diag" command 
 *       with the "get EMC Threshold In status" opcode.
 *       It calls the function to parse the EMC Threshold In status page.
 *
 *       Get the previous control buffer and compare the allocated response 
 *       buffer size with the size of the trace buffer info status. The data
 *       returned from the recieve diag command are copied to the response bufffer.
 *
 * @param eses_enclosure - The pointer to the ESES enclosure.
 * @param sg_list - the pointer to the sg list allocated while sending the command.
 *
 * @return  fbe_enclosure_status_t.
 *      FBE_ENCLOSURE_STATUS_OK - no error.
 *      otherwise - error is found.
 *
 * NOTES
 *
 * HISTORY
 *   17-Sep-2009 Prasenjeet - Created.
 ***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_handle_threshold_in_response(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet)
{
    fbe_status_t                          status = FBE_STATUS_OK;
    fbe_enclosure_status_t          encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_number_t        enclosure_number = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t * prev_control_operation = NULL;
    fbe_payload_control_buffer_t    control_buffer = NULL;
    fbe_enclosure_mgmt_ctrl_op_t    * eses_page_op = NULL;
    fbe_sg_element_t                * sg_list = NULL;

    fbe_base_enclosure_get_enclosure_number((fbe_base_enclosure_t *)eses_enclosure, &enclosure_number);

    fbe_base_object_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry, Encl: %d.\n", __FUNCTION__, enclosure_number);
    
    payload = fbe_transport_get_payload_ex(packet);
    /* Get the previous control operation */  
    prev_control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    status = fbe_payload_control_get_buffer(prev_control_operation, &control_buffer);

    if((status != FBE_STATUS_OK)||(control_buffer == NULL))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                       FBE_TRACE_LEVEL_WARNING,
                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                       "fbe_payload_control_get_buffer returned fail: status:0x%x, control_buffer: %64p.\n",
                       status, 
                       control_buffer);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_ENCLOSURE_STATUS_PACKET_FAILED;
    }

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    eses_page_op = (fbe_enclosure_mgmt_ctrl_op_t *)control_buffer;

    // response_buf_size is the user allocated number of bytes
    // requires_response_buf_size is the actual page number of bytes returned from eses
    eses_page_op->required_response_buf_size =   sizeof(fbe_enclosure_mgmt_exp_threshold_info_t);

    /* Zero out the memory allocated. */
    fbe_zero_memory(eses_page_op->response_buf_p, eses_page_op->response_buf_size);

    // parse the threshold page: go through the page data and 
    // copy the targeted data into the control op response buffer
    encl_status = fbe_eses_parse_threshold_page(eses_enclosure,
                                               eses_page_op,    // details of the command (element type, overall thresh stat elem)
                                               sg_list[0].address);   // (source) data to parse (from eses)


    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "fbe_eses_enclosure_process_emc_statistics_status_page failed-encl_status: 0x%x\n",
                                encl_status);
    
        encl_status = FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED;
    }

    return encl_status;
} //fbe_eses_enclosure_handle_get_emc_statistics_status_response


/*!**************************************************************
 * @fn fbe_eses_enclosure_increment_outstanding_scsi_request_count
 ****************************************************************
 * @brief
 *  This function should be called before the SCSI request is sent.
 *  It will increment outstanding scsi request count.
 *
 * @param eses_enclosure - 
 *
 * @return FBE_STATUS_OK
 * 
 * @author
 *  15-Mar-2013: PHE - Created.
 ****************************************************************/
static __forceinline fbe_status_t 
fbe_eses_enclosure_increment_outstanding_scsi_request_count(fbe_eses_enclosure_t * eses_enclosure)
{
    fbe_atomic_increment(&eses_enclosure->outstanding_scsi_request_count);
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_eses_enclosure_decrement_outstanding_scsi_request_count
 ****************************************************************
 * @brief
 *  This function should be called after the SCSI request is completed.
 *  It will decrement outstanding scsi request count.
 *
 * @param eses_enclosure - 
 *
 * @return FBE_STATUS_OK
 * 
 * @author
 *  15-Mar-2013: PHE - Created.
 ****************************************************************/
static __forceinline fbe_status_t 
fbe_eses_enclosure_decrement_outstanding_scsi_request_count(fbe_eses_enclosure_t * eses_enclosure)
{
    fbe_atomic_decrement(&eses_enclosure->outstanding_scsi_request_count);
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_eses_enclosure_check_outstanding_scsi_request_count
 ****************************************************************
 * @brief
 *  This function checks whether the maximum number of the outstanding scsi
 *  requests is exceeded or not. If it is exceeded, return FBE_STATUS_BUSY.
 *  Otherwise, return FBE_STATUS_OK.
 *
 * @param eses_enclosure - 
 *
 * @return FBE_STATUS_BUSY - the maximum number of the outstanding scsi
 *  requests is exceeded
 *         FBE_STATUS_OK - the maximum number of the outstanding scsi
 *  requests is NOT exceeded
 * 
 * @author
 *  15-Mar-2013: PHE - Created.
 ****************************************************************/
static __forceinline fbe_status_t fbe_eses_enclosure_check_outstanding_scsi_request_count(
                       fbe_eses_enclosure_t * eses_enclosure)
{
    if(eses_enclosure->outstanding_scsi_request_count > eses_enclosure->outstanding_scsi_request_max) 
    {
        return FBE_STATUS_BUSY;
    }
    else
    {
        return FBE_STATUS_OK;
    }
}

/*!**************************************************************
 * @fn fbe_eses_enclosure_set_outstanding_scsi_request_opcode
 ****************************************************************
 * @brief
 *  This function should be called t set the outstanding scsi request opcode.
 *
 * @param eses_enclosure - 
 *
 * @return FBE_STATUS_OK
 * 
 * @author
 *  08-Apr-2013: PHE - Created.
 ****************************************************************/
fbe_status_t 
fbe_eses_enclosure_set_outstanding_scsi_request_opcode(fbe_eses_enclosure_t * eses_enclosure,
                                                          fbe_eses_ctrl_opcode_t opcode)
{
    eses_enclosure->outstanding_scsi_request_opcode = opcode;
    return FBE_STATUS_OK;
}

/*!*************************************************************************
 * @fn fbe_eses_enclosure_setup_get_rp_size_cmd(fbe_eses_enclosure_t * eses_enclosure, 
 *                                             fbe_packet_t * packet,
 *                                             fbe_payload_cdb_operation_t * payload_cdb_operation)   
 ***************************************************************************
 * @brief
 *   This function sets up the "GET RP SIZE" command.
 *
 * @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
 * @param     packet - The pointer to the fbe_packet_t.
 * @param     payload_cdb_operation - The pointer to the payload cdb operation.
 *
 * @return   fbe_status_t.
 *        FBE_STATUS_OK  - no error found.
 *        otherwise - error is found.
 *
 * NOTES:
 *       
 *
 * HISTORY
 *   11-Dec-2013    PHE  - Created.
 ***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_setup_get_rp_size_cmd(fbe_eses_enclosure_t * eses_enclosure, 
                                        fbe_packet_t * packet,
                                        fbe_payload_cdb_operation_t * payload_cdb_operation)      
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_u8_t                            *cdb = NULL;    
    fbe_payload_ex_t                    *payload = NULL;
    fbe_sg_element_t                    *sg_list = NULL;
    fbe_eses_enclosure_buf_info_t       bufInfo = {0};

    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);     

    /* Get the buffer id which needs to get the buffer size for */
    status = fbe_eses_enclosure_get_buf_id_with_unavail_buf_size(eses_enclosure, &bufInfo);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, failed to get buf_id, status 0x%X.\n",
                                __FUNCTION__, status);
    
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_PARAMETER_INVALID);
        fbe_transport_complete_packet(packet);
        return status;
    } 

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    fbe_copy_memory((fbe_u8_t *)sg_list + (2 * sizeof(fbe_sg_element_t)),
                    &bufInfo,
                    sizeof(fbe_eses_enclosure_buf_info_t));

    sg_list[0].count = FBE_ESES_READ_BUF_DESC_SIZE;
    sg_list[0].address = (fbe_u8_t *)sg_list + (2 * sizeof(fbe_sg_element_t))+ sizeof(fbe_eses_enclosure_buf_info_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);    

    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, sg_list[0].count);
        
    /* cdb_operation setup */
    status = fbe_payload_cdb_set_parameters(payload_cdb_operation,  
                                            FBE_ESES_ENCLOSURE_ESES_PAGE_TIMEOUT,  // timeout
                                            FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE, //attribute
                                            FBE_PAYLOAD_CDB_FLAGS_DATA_IN,  // flag
                                            FBE_SCSI_READ_BUFFER_CDB_SIZE); // The cdb length

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_ERROR,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                           "%s, fbe_payload_cdb_set_parameters failed.\n",
                                           __FUNCTION__);
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED);
        fbe_transport_complete_packet(packet);
        return status;
    } 

    status = fbe_payload_cdb_operation_get_cdb(payload_cdb_operation, &cdb);
    
    /* Details of the CDB belongs to eses library */
    status = fbe_eses_build_read_buf_cdb(cdb, 
                                         FBE_SCSI_READ_BUFFER_CDB_SIZE, // The size of CDB.
                                         FBE_ESES_READ_BUF_MODE_DESC,   // The mode of the read buffer command.
                                         bufInfo.bufId,    // The id of the buffer to be read.
                                         0,         // The byte offset inot a buffer described by buffer id. not used in all modes. 
                                         FBE_ESES_READ_BUF_DESC_SIZE);   // The number of bytes allocated for the response.
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_ERROR,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                           "%s, fbe_eses_build_write_buf_cdb failed.\n",
                                           __FUNCTION__);
        fbe_eses_enclosure_cleanup_cmd(eses_enclosure, packet, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED);
        fbe_transport_complete_packet(packet);
        return status;
    } 

    return status;
} // fbe_eses_enclosure_setup_get_rp_size_cmd


/*!*************************************************************************
* @fn fbe_eses_enclosure_handle_get_rp_size_response(fbe_eses_enclosure_t * eses_enclosure, 
*                                                   fbe_sg_element_t * sg_list)                
***************************************************************************
* @brief
*       This function handles the response got from the "GET RP SIZE" command.
*
* @param       eses_enclosure - The pointer to the ESES enclosure.
* @param       packet - pointer to the fbe_packet_t
* @param       sg_list - the pointer to the sg list allocated while sending the command.
*
* @return  fbe_enclosure_status_t.
*      FBE_ENCLOSURE_STATUS_OK - no error.
*      otherwise - error is found.
*
* NOTES
*
* HISTORY
*   10-Dec-2013     PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_handle_get_rp_size_response(fbe_eses_enclosure_t * eses_enclosure,
                                              fbe_sg_element_t * sg_list)
{
    fbe_eses_enclosure_buf_info_t * buf_info_p = NULL;
    fbe_eses_read_buf_desc_t * read_buf_desc_p = NULL;
    fbe_u32_t buf_size = 0;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry \n", __FUNCTION__ );

    buf_info_p = (fbe_eses_enclosure_buf_info_t *)((fbe_u8_t *)sg_list + (2 * sizeof(fbe_sg_element_t)));
    read_buf_desc_p = (fbe_eses_read_buf_desc_t *)(sg_list[0].address);
    buf_size = fbe_eses_get_buf_capacity(read_buf_desc_p);


    encl_status = fbe_base_enclosure_edal_setU32_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                 FBE_ENCL_BD_BUFFER_SIZE,
                                                 buf_info_p->componentType, 
                                                 buf_info_p->componentIndex,
                                                 buf_size);

    if (encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                FBE_TRACE_LEVEL_ERROR,
                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                "%s, %s, index %d, failed to set RP size, enclStatus 0x%X.\n", 
                __FUNCTION__, 
                enclosure_access_printComponentType(buf_info_p->componentType),
                buf_info_p->componentIndex,
                encl_status);
    }

    return encl_status;
} // End of - fbe_eses_enclosure_handle_get_rp_size_response


//End of file fbe_eses_enclosure_executer.c
