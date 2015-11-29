/***************************************************************************
 *  terminator_encl_eses_plugin_emc_statistics_pages.c
 ***************************************************************************
 *
 *  DESCRIPTION:
 *     This contains the funtions related to processing the EMC statistics 
 *      Status Diagnostic Page & EMC statistics control diagnostic page.(11h)
 *
 *  NOTES:
 *      
 *  HISTROY:
 *      Sep-01-2009	Rajesh V. Created.
 *    
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "terminator_sas_io_api.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_eses.h"
#include "fbe_scsi.h"
#include "fbe_terminator.h"
#include "terminator_encl_eses_plugin.h"
#include "terminator_virtual_phy.h"

/**********************************/
/*        LOCAL DEFINES           */
/**********************************/


/**********************************/
/*        LOCAL FUNCTIONS         */
/**********************************/
static fbe_status_t 
emc_statistics_stat_page_build_device_slot_stats(fbe_u8_t *device_slot_stats_start_ptr, 
                                                 fbe_u8_t **device_slot_stats_end_ptr,
                                                 fbe_terminator_device_ptr_t virtual_phy_handle);




/*********************************************************************
*           build_emc_statistics_status_diagnostic_page ()
*********************************************************************
*
*  DESCRIPTION:
*   This builds the EMC statistics status diagnostic page.(11h) 
*
*  PARAMETERS:
*   scatter gather list, virtual phy device handle
*
*  RETURN VALUES/ERRORS:
*   success or failure
* 
*  NOTES:
* 
*  HISTORY:
*    Sep-01-2009: Rajesh V. Created
*    
*********************************************************************/
fbe_status_t 
build_emc_statistics_status_diagnostic_page(fbe_sg_element_t * sg_list, 
                                            fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_common_pg_hdr_struct *emc_statistics_stat_page_hdr;
    fbe_u8_t *elem_stats_start_ptr = NULL; 
    fbe_u8_t *elem_stats_end_ptr = NULL;
    fbe_sas_enclosure_type_t  encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    RETURN_ON_ERROR_STATUS;

    emc_statistics_stat_page_hdr = 
        (ses_common_pg_hdr_struct *)fbe_sg_element_address(sg_list);
    memset(emc_statistics_stat_page_hdr, 0, sizeof(ses_common_pg_hdr_struct));

    emc_statistics_stat_page_hdr->pg_code = SES_PG_CODE_EMC_STATS_STAT;

    // Gen code returned in bigEndian format.
    status = config_page_get_gen_code(virtual_phy_handle , 
                                      &emc_statistics_stat_page_hdr->gen_code);
    emc_statistics_stat_page_hdr->gen_code = 
        BYTE_SWAP_32(emc_statistics_stat_page_hdr->gen_code);


    elem_stats_start_ptr = (fbe_u8_t *)(emc_statistics_stat_page_hdr + 1);
    status = emc_statistics_stat_page_build_device_slot_stats(elem_stats_start_ptr, 
                                                              &elem_stats_end_ptr, 
                                                              virtual_phy_handle);
    RETURN_ON_ERROR_STATUS;

    emc_statistics_stat_page_hdr->pg_len = 
        (fbe_u16_t)((elem_stats_end_ptr - ((fbe_u8_t *)emc_statistics_stat_page_hdr)) - 4);
    // Always return page length in big endian format as actual expander firmware does that.
    emc_statistics_stat_page_hdr->pg_len = BYTE_SWAP_16(emc_statistics_stat_page_hdr->pg_len);

    return(status);     
}

/*********************************************************************
*         emc_statistics_stat_page_build_device_slot_stats ()
*********************************************************************
*
*  DESCRIPTION:
*   This builds the Array Device slot element statistics in
*   EMC statistics status diagnostic page. 
*
*  PARAMETERS:
*   device_slot_stats_start_ptr - Array device slot element
*       statistics start address.
*   device_slot_stats_end_ptr - End address of Array device
*       slot element statistics to be returned.
*   virtual phy handle
*
*  RETURN VALUES/ERRORS:
*   success or failure
* 
*  NOTES:
* 
*  HISTORY:
*    Sep-02-2009: Rajesh V. Created
*    
*********************************************************************/
static fbe_status_t 
emc_statistics_stat_page_build_device_slot_stats(fbe_u8_t *device_slot_stats_start_ptr, 
                                                 fbe_u8_t **device_slot_stats_end_ptr,
                                                 fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t                 status = FBE_STATUS_GENERIC_FAILURE;
    fbe_eses_device_slot_stats_t *overall_device_slot_stats = NULL;
    fbe_eses_device_slot_stats_t *individual_device_slot_stats = NULL;
    fbe_u16_t                    overall_device_slot_elem_addr_offset = 0;
    fbe_u8_t                     individual_device_slot_elem_offset = 0;
    fbe_u8_t                     overall_device_slot_elem_offset = 0;
    fbe_u8_t                     max_drive_slots = 0;
    fbe_u32_t                    i, j;
    fbe_u8_t                     device_slot_elem_stats_len = 0;    
    fbe_sas_enclosure_type_t     encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    terminator_sp_id_t spid;

    fbe_eses_exp_phy_stats_t     *overall_exp_phy_stats = NULL;
    fbe_eses_exp_phy_stats_t     *individual_exp_phy_stats = NULL;
    fbe_u16_t                     overall_exp_phy_elem_addr_offset = 0;
    fbe_u8_t                     individual_exp_phy_elem_offset = 0;
    fbe_u8_t                     overall_exp_phy_elem_offset = 0;
    fbe_u8_t                     exp_phy_elem_stats_len = 0;
    fbe_u8_t                     *source_ptr = NULL;
    fbe_u8_t                     max_phys = 0;
    fbe_u8_t                     mapped_phy_id = 127;

    fbe_u32_t   invalid_dword_count = 0;                
    fbe_u32_t   disparity_error_count = 0;             
    fbe_u32_t   loss_dword_sync_count = 0;            
    fbe_u32_t   phy_reset_fail_count = 0;            
    fbe_u32_t   code_violation_count = 0;           
    fbe_u8_t    phy_change_count = 0;              
    fbe_u16_t   crc_pmon_accum_count = 0;         
    fbe_u16_t   in_connect_crc_count = 0;        

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    RETURN_ON_ERROR_STATUS;
    status =  fbe_terminator_api_get_sp_id(&spid);

    status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_LCC,
                                                            spid, 
                                                            SES_ELEM_TYPE_ARRAY_DEV_SLOT, 
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &overall_device_slot_elem_addr_offset);
    RETURN_ON_ERROR_STATUS;

    overall_device_slot_elem_offset = 
        (fbe_u8_t)((overall_device_slot_elem_addr_offset - FBE_ESES_PAGE_HEADER_SIZE) / 
                   (FBE_ESES_CTRL_STAT_ELEM_SIZE));

    device_slot_elem_stats_len = 
        sizeof(fbe_eses_device_slot_stats_t) - sizeof(ses_common_statistics_field_t);

    overall_device_slot_stats = (fbe_eses_device_slot_stats_t *)device_slot_stats_start_ptr; 
    memset (overall_device_slot_stats, 0, sizeof(fbe_eses_device_slot_stats_t));
    overall_device_slot_stats->common_stats.elem_offset = overall_device_slot_elem_offset;
    overall_device_slot_stats->common_stats.stats_len = device_slot_elem_stats_len;      
    //Ignore statistics in overall array device slot element

    individual_device_slot_stats = overall_device_slot_stats; 
    individual_device_slot_elem_offset = overall_device_slot_elem_offset;

    status = terminator_max_drive_slots(encl_type, &max_drive_slots);
    RETURN_ON_ERROR_STATUS;

    for(i=0; i<max_drive_slots; i++)
    {
        individual_device_slot_stats ++; 
        individual_device_slot_elem_offset ++;

        memset(individual_device_slot_stats, 0, sizeof(fbe_eses_device_slot_stats_t));
        individual_device_slot_stats->common_stats.elem_offset = 
            individual_device_slot_elem_offset;
        individual_device_slot_stats->common_stats.stats_len = 
            device_slot_elem_stats_len;
        status = terminator_get_drive_slot_insert_count(virtual_phy_handle, 
                                                        i,
                                                        &individual_device_slot_stats->insert_count);
        RETURN_ON_ERROR_STATUS;
        status = terminator_get_drive_power_down_count(virtual_phy_handle, 
                                                        i,
                                                        &individual_device_slot_stats->power_down_count);
        RETURN_ON_ERROR_STATUS;
                                                        
    }
    *device_slot_stats_end_ptr = (fbe_u8_t *)(individual_device_slot_stats + 1);   

    //start pop phy statistics info
    source_ptr = *device_slot_stats_end_ptr;
    status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_LCC,
                                                            spid,
                                                            SES_ELEM_TYPE_EXP_PHY,
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &overall_exp_phy_elem_addr_offset);
    RETURN_ON_ERROR_STATUS;

    overall_exp_phy_elem_offset =
        (fbe_u8_t)((overall_exp_phy_elem_addr_offset - FBE_ESES_PAGE_HEADER_SIZE) /
                   (FBE_ESES_CTRL_STAT_ELEM_SIZE));

    exp_phy_elem_stats_len = sizeof(fbe_eses_exp_phy_stats_t) - sizeof(ses_common_statistics_field_t);

    overall_exp_phy_stats = (fbe_eses_exp_phy_stats_t *) source_ptr;
    memset(overall_exp_phy_stats, 0, sizeof(fbe_eses_exp_phy_stats_t));
    overall_exp_phy_stats->common_stats.elem_offset = overall_exp_phy_elem_offset;
    overall_exp_phy_stats->common_stats.stats_len = exp_phy_elem_stats_len;

    individual_exp_phy_elem_offset = overall_exp_phy_elem_offset;
    individual_exp_phy_stats = overall_exp_phy_stats;

    status = terminator_max_phys(encl_type, &max_phys);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: get max phys failed.\n",
                         __FUNCTION__);

        return status;
    }

    for(i=0; i<max_phys; i++)
    {
        individual_exp_phy_stats ++;
        individual_exp_phy_elem_offset ++;

        memset(individual_exp_phy_stats, 0, sizeof(fbe_eses_exp_phy_stats_t));
        individual_exp_phy_stats->common_stats.elem_offset =
                individual_exp_phy_elem_offset;
        individual_exp_phy_stats->common_stats.stats_len =
                exp_phy_elem_stats_len;
        mapped_phy_id = 255;
    
    	//start inject fake count info
	//for slot phy and connector phy we inject fake data differently just for fun
        for( j = 0; j < max_drive_slots; j++ )
        {
            // Get the corresponding PHY status
            status = sas_virtual_phy_get_drive_slot_to_phy_mapping(j, &mapped_phy_id, encl_type);
            if (status != FBE_STATUS_OK)
            {
                terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s sas_virtual_phy_get_drive_slot_to_phy_mapping failed!\n", __FUNCTION__);
                return status;
            }
            if(mapped_phy_id == i) // it is drive slot phy
            {
                individual_exp_phy_stats->invalid_dword = swap32(j++); // we swap the byte order to simulate big endian which is used by expander
                individual_exp_phy_stats->disparity_error = swap32(j++);
                individual_exp_phy_stats->loss_dword_sync = swap32(j++);
                individual_exp_phy_stats->phy_reset_fail = swap32(j++);
                individual_exp_phy_stats->code_violation = swap32(j++);
                individual_exp_phy_stats->phy_change = j++;
                individual_exp_phy_stats->crc_pmon_accum = swap16(j++);
                individual_exp_phy_stats->in_connect_crc = swap16(j++);
                break;
            }
        }

	// it is connector phy
        if(mapped_phy_id != i)
        {
            individual_exp_phy_stats->invalid_dword = swap32(invalid_dword_count++);
            individual_exp_phy_stats->disparity_error = swap32(disparity_error_count++);
            individual_exp_phy_stats->loss_dword_sync = swap32(loss_dword_sync_count++);
            individual_exp_phy_stats->phy_reset_fail = swap32(phy_reset_fail_count++);
            individual_exp_phy_stats->code_violation = swap32(code_violation_count++);
            individual_exp_phy_stats->phy_change = phy_change_count++;
            individual_exp_phy_stats->crc_pmon_accum = swap16(crc_pmon_accum_count++);
            individual_exp_phy_stats->in_connect_crc = swap16(in_connect_crc_count++);
        }
        //end inject fake count info

    }

    *device_slot_stats_end_ptr = (fbe_u8_t *)(individual_exp_phy_stats + 1);

    return(FBE_STATUS_OK);
}
