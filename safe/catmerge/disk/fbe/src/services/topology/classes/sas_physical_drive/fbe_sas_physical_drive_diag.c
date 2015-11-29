/**********************************************************************
 *  Copyright (C)  EMC Corporation 2009
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_sas_physical_drive_utils.c
 ***************************************************************************
 *
 * @brief
 *  The routines in this file contains the diagnostic related functions.
 *
 * NOTE: 
 *
 * HISTORY
 *   1-Sept-2009 chenl6 - Created. 
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_transport.h"
#include "fbe_transport_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"

#include "base_physical_drive_private.h"
#include "sas_physical_drive_private.h"
#include "swap_exports.h"


#define RECEIVE_DIAG_OFFSET_OF_PAGE_LENGTH		4
#define RECEIVE_DIAG_VENDOR_ID_SIZE				8
#define RECEIVE_DIAG_PROD_REV_SIZE				4

/* #define RECEIVE_DIAG_PG82_CODE                  0x82 move this to sas_physical_drive_private.h */
#define RECEIVE_DIAG_PG82_FEAT_COUNT			1
#define RECEIVE_DIAG_PG82_FEAT0_ID				0
#define RECEIVE_DIAG_PG82_FEAT0_SUB_ID			0
#define RECEIVE_DIAG_PG82_FEAT0_PARAM_COUNT		11
#define RECEIVE_DIAG_PG82_PARAM_PORT_A	        0x0000
#define RECEIVE_DIAG_PG82_PARAM_PORT_B	        0x0001

#define	INITIATE_PORT_ID					    0x0000
#define	INITIATE_PORT_SUB_ID				    0x0000
#define INVALID_DWORD_COUNT			            0x0001
#define LOSS_SYNC_COUNT					        0x0002
#define PHY_RESET_COUNT							0x0003
#define INVALID_CRC_COUNT                       0x0004
#define DISPARITY_ERROR_COUNT                   0x0005


/* Definitions */
typedef struct fbe_scsi_receive_diag_page_header_s
{
    fbe_u8_t page_code;
    fbe_u8_t page_code_specific;
    fbe_u8_t page_length;
    fbe_u8_t page_length_low;
}fbe_scsi_receive_diag_page_header_t;

typedef struct fbe_scsi_receive_diag_pg82_s
{
    fbe_scsi_receive_diag_page_header_t header;
    fbe_u8_t vendor_id[RECEIVE_DIAG_VENDOR_ID_SIZE];
    fbe_u8_t product_rev[RECEIVE_DIAG_PROD_REV_SIZE];
    fbe_u8_t feature_count; 
    fbe_u8_t feature_count_low;
}fbe_scsi_receive_diag_pg82_t;

typedef struct fbe_scsi_receive_diag_feature_s
{
    fbe_u8_t feature_id;
    fbe_u8_t feature_id_low;
    fbe_u8_t feature_sub_id;
    fbe_u8_t feature_sub_id_low;
    fbe_u8_t reserved;
    fbe_u8_t param_count;
    fbe_u8_t param_count_low;
    fbe_u8_t param_size;
}fbe_scsi_receive_diag_feature_t;

typedef struct fbe_scsi_receive_diag_param_s
{
    fbe_u8_t param_id;
    fbe_u8_t param_id_low;
    fbe_u8_t param_sub_id;
    fbe_u8_t param_sub_id_low;
    fbe_u8_t param_value_0;
    fbe_u8_t param_value_1;
    fbe_u8_t param_value_2;
    fbe_u8_t param_value_3;
}fbe_scsi_receive_diag_param_t;

/* Forward declaration */
fbe_status_t fbe_sas_physical_drive_process_receive_diag_pg82(fbe_sas_physical_drive_t * sas_physical_drive,
												              fbe_packet_t * packet);


/*!*************************************************************************
 * @fn fbe_sas_physical_drive_process_receive_diag_page
 *                    fbe_sas_physical_drive_t * sas_physical_drive,
 *                    fbe_packet_t * packet)
 **************************************************************************
 *
 *  @brief
 *      This function will take a control code & return a string for
 *      display use.
 *
 *  @param    sas_physical_drive
 *  @param    pointer to packet.
 *  @param    if DC is set, DC_flag = FBE_TRUE.
 *
 *  @return    fbe_status_t.
 *
 *  NOTES:
 *
 *  HISTORY:
 *    01-Sep-2009: chenl6 - Created
 *
 **************************************************************************/
fbe_status_t
fbe_sas_physical_drive_process_receive_diag_page(fbe_sas_physical_drive_t * sas_physical_drive,
                                             fbe_packet_t * packet)
{
    fbe_scsi_receive_diag_page_header_t *page_header_ptr;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_sg_element_t  * sg_list = NULL; 
    fbe_u32_t transferred_count;     
    fbe_u16_t page_length;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
	fbe_payload_cdb_get_transferred_count(payload_cdb_operation, &transferred_count);

    /* Point to the page header */
    page_header_ptr = (fbe_scsi_receive_diag_page_header_t *)sg_list[0].address;
    if (page_header_ptr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s NULL page\n",
                              __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;    
    }

    /* Get Page Length */
    page_length = swap16(*(fbe_u16_t *)&(page_header_ptr->page_length));
    if(transferred_count != (page_length + RECEIVE_DIAG_OFFSET_OF_PAGE_LENGTH))
    {
        fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Invalid page size: 0x%X, trasferred count 0x%X\n",
                              __FUNCTION__, page_length, transferred_count);
        return FBE_STATUS_GENERIC_FAILURE;    
    }

    switch(page_header_ptr->page_code)
    {
        case RECEIVE_DIAG_PG82_CODE:
            status = fbe_sas_physical_drive_process_receive_diag_pg82(sas_physical_drive, packet);
            break;
            
        default:
            fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s Invalid page code: 0x%X\n",
                                  __FUNCTION__, page_header_ptr->page_code);
            return FBE_STATUS_GENERIC_FAILURE;    
    }

    return status;    
}

/*!*************************************************************************
 * @fn fbe_sas_physical_drive_process_receive_diag_pg82
 *                    fbe_sas_physical_drive_t * sas_physical_drive,
 *                    fbe_packet_t * packet)
 **************************************************************************
 *
 *  @brief
 *      This function will take a control code & return a string for
 *      display use.
 *
 *  @param    sas_physical_drive
 *  @param    pointer to packet.
 *
 *  @return    fbe_status_t.
 *
 *  NOTES:
 *
 *  HISTORY:
 *    01-Sep-2009: chenl6 - Created
 *
 **************************************************************************/
fbe_status_t
fbe_sas_physical_drive_process_receive_diag_pg82(fbe_sas_physical_drive_t * sas_physical_drive,
												 fbe_packet_t * packet)
{
    fbe_u16_t page_length;
    fbe_u16_t feature_count;
    fbe_u16_t feature_id;
    fbe_u16_t feature_sub_id;
    fbe_u16_t param_count;
    fbe_u16_t param_id;
    fbe_u16_t param_sub_id;
    fbe_u32_t param_value;
    fbe_scsi_receive_diag_pg82_t *page_ptr;
    fbe_scsi_receive_diag_feature_t *feature_ptr;
    fbe_scsi_receive_diag_param_t *param_ptr;
    fbe_u8_t *bptr;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    fbe_physical_drive_mgmt_receive_diag_page_t * receive_diag_info = NULL;
    fbe_physical_drive_receive_diag_pg82_t * pg82_info;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &receive_diag_info);
    
    if (receive_diag_info == NULL)
    {
        /* There is no control buffer for Disk Collect. Handle Disk Collect data. */
        status = fbe_sas_physical_drive_fill_receive_diag_info(sas_physical_drive, sg_list[0].address,
                                        RECEIVE_DIAG_PG82_SIZE);
    
        return status;
    }
    
    pg82_info = &(receive_diag_info->page_info.pg82);

    /* Point to the page format */
    bptr = sg_list[0].address;
    page_ptr = (fbe_scsi_receive_diag_pg82_t *)bptr;
    page_length = swap16(*(fbe_u16_t *)&(page_ptr->header.page_length));

    /* Validate the feature descriptor count */
    feature_count = swap16(*(fbe_u16_t *)&(page_ptr->feature_count));
    if(feature_count != RECEIVE_DIAG_PG82_FEAT_COUNT)
    {
        fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Invalid feature count: 0x%X\n",
                              __FUNCTION__, feature_count);
      
        return FBE_STATUS_GENERIC_FAILURE;    
    }

    page_length -= (sizeof(fbe_scsi_receive_diag_pg82_t) - 4);

    /* Validate the features */
    bptr += sizeof(fbe_scsi_receive_diag_pg82_t);
    feature_ptr = (fbe_scsi_receive_diag_feature_t *)bptr;

    for (; feature_count > 0; feature_count--)
    {
        page_length -= sizeof(fbe_scsi_receive_diag_feature_t);
        feature_id = swap16(*(fbe_u16_t *)&(feature_ptr->feature_id));

        switch(feature_id)
        {
            case RECEIVE_DIAG_PG82_FEAT0_ID:
                /* feature 0 sub ID */
                feature_sub_id = swap16(*(fbe_u16_t *)&(feature_ptr->feature_sub_id));
                /* parameter count */
                param_count = swap16(*(fbe_u16_t *)&(feature_ptr->param_count));
                bptr = (fbe_u8_t *)feature_ptr;
                bptr += sizeof(fbe_scsi_receive_diag_feature_t);

                /* Validate the parameters */
                param_ptr = (fbe_scsi_receive_diag_param_t *)bptr;
                for (; param_count > 0; param_ptr++, param_count--)
                {
                    page_length -= sizeof(fbe_scsi_receive_diag_param_t);
                    param_id = swap16(*(fbe_u16_t *)&(param_ptr->param_id));
                    param_sub_id = swap16(*(fbe_u16_t *)&(param_ptr->param_sub_id));
                    param_value = swap32(*(fbe_u32_t *)&(param_ptr->param_value_0));
                    /* Now check each parameter */
                    switch(param_id)
                    {	
                        case INITIATE_PORT_ID:
                            if(param_sub_id == INITIATE_PORT_SUB_ID)
                            {
                                pg82_info->initiate_port = param_value;
                            }
                            break;
		
                        case INVALID_DWORD_COUNT:
                            if((param_sub_id == RECEIVE_DIAG_PG82_PARAM_PORT_A) || 
                               (param_sub_id == RECEIVE_DIAG_PG82_PARAM_PORT_B))
                            {
                                pg82_info->port[param_sub_id].invalid_dword_count = param_value;
                            }
                            break;	

                        case LOSS_SYNC_COUNT:
                            if((param_sub_id == RECEIVE_DIAG_PG82_PARAM_PORT_A) || 
                               (param_sub_id == RECEIVE_DIAG_PG82_PARAM_PORT_B))
                            {
                                pg82_info->port[param_sub_id].loss_sync_count = param_value;
                            }
                            break;				

                        case PHY_RESET_COUNT:
                            if((param_sub_id == RECEIVE_DIAG_PG82_PARAM_PORT_A) || 
                               (param_sub_id == RECEIVE_DIAG_PG82_PARAM_PORT_B))
                            {
                                pg82_info->port[param_sub_id].phy_reset_count = param_value;
                            }
                            break;				

                        case INVALID_CRC_COUNT:
                            if((param_sub_id == RECEIVE_DIAG_PG82_PARAM_PORT_A) || 
                               (param_sub_id == RECEIVE_DIAG_PG82_PARAM_PORT_B))
                            {
                                pg82_info->port[param_sub_id].invalid_crc_count = param_value;
                            }
                            break;				

                        case DISPARITY_ERROR_COUNT:
                            if((param_sub_id == RECEIVE_DIAG_PG82_PARAM_PORT_A) || 
                               (param_sub_id == RECEIVE_DIAG_PG82_PARAM_PORT_B))
                            {
                                pg82_info->port[param_sub_id].disparity_error_count = param_value;
                            }
                            break;				
						
                        default:
                            fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive,
                                                  FBE_TRACE_LEVEL_INFO,
                                                  FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s Invalid param id: 0x%X\n",
                                                  __FUNCTION__, param_id);
      
                            break;    
                    }
                }
                break;				

            default:
                fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s Invalid feature id: 0x%X\n",
                                      __FUNCTION__, feature_id);
      
                return FBE_STATUS_GENERIC_FAILURE;

        }
        /* Point to next feature if no error */
        param_count = swap16(*(fbe_u16_t *)&(feature_ptr->param_count));
        bptr += (param_count * sizeof(fbe_scsi_receive_diag_param_t));
        feature_ptr = (fbe_scsi_receive_diag_feature_t *) bptr;
    }

    /* Should have used exactly the number of bytes specified */
    if (page_length != 0)
    {
        fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Invalid feature id: 0x%X\n",
                              __FUNCTION__, feature_id);
      
        return FBE_STATUS_GENERIC_FAILURE;    
    }

    return FBE_STATUS_OK;
}