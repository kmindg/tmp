/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*********************************************************************
 * @file fbe_eses_enclosure_build_pages.c
 **********************************************************************
 * @brief 
 *  The routines in this file process the ESES pages to populate the 
 *  configuration and mapping information.   
 *  
 * @ingroup fbe_enclosure_class
 *
 * HISTORY:
 *  05-Aug-2008 PHE - Created.                  
 ***********************************************************************/
#include "generic_types.h"
#include "fbe_eses_enclosure_private.h"
#include "fbe_eses_enclosure_config.h"
#include "edal_eses_enclosure_data.h"
#include "fbe_eses.h"

/* Forward declaration */
static fbe_status_t fbe_eses_enclosure_build_trace_buf_info_elem_group(fbe_eses_enclosure_t * eses_enclosure,
                                                         fbe_enclosure_mgmt_trace_buf_cmd_t * trace_buf_cmd_p,
                                                         ses_pg_emc_encl_ctrl_struct * emc_encl_ctrl_pg_p,
                                                         fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p,
                                                         fbe_u16_t * page_size_p);
static fbe_status_t fbe_eses_enclosure_build_sas_conn_info_elem_group(fbe_eses_enclosure_t * eses_enclosure,
                                                         ses_pg_emc_encl_ctrl_struct * emc_encl_ctrl_pg_p,
                                                         fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p,
                                                         fbe_u16_t * page_size_p);
static fbe_status_t fbe_eses_enclosure_build_encl_time_info_elem_group(fbe_eses_enclosure_t * eses_enclosure,
                                                         ses_pg_emc_encl_ctrl_struct * emc_encl_ctrl_pg_p,
                                                         fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p,
                                                         fbe_u16_t * page_size_p);
static fbe_status_t fbe_eses_enclosure_build_general_info_array_dev_slot(fbe_eses_enclosure_t * eses_enclosure,
                                                         ses_pg_emc_encl_ctrl_struct * emc_encl_ctrl_pg_p,
                                                         fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p,
                                                         fbe_u16_t * page_size_p);
static fbe_status_t 
fbe_eses_enclosure_build_ps_control_info_elem_group(fbe_eses_enclosure_t * eses_enclosure,
                                                    ses_pg_emc_encl_ctrl_struct * emc_encl_ctrl_pg_p,
                                                    fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p,
                                                    fbe_u16_t * page_size_p);
static fbe_status_t fbe_eses_enclosure_build_general_info_expander(fbe_eses_enclosure_t * eses_enclosure,
                                                         ses_pg_emc_encl_ctrl_struct * emc_encl_ctrl_pg_p,
                                                         fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p,
                                                         fbe_u16_t * page_size_p);
static fbe_status_t fbe_eses_enclosure_build_sas_conn_info_elem(fbe_eses_enclosure_t * eses_enclosure, 
                                                         ses_sas_conn_info_elem_struct * sas_conn_info_elem_p,
                                                         fbe_u8_t connector_index);
static fbe_status_t fbe_eses_enclosure_build_trace_buf_info_elem(fbe_eses_enclosure_t * eses_enclosure, 
                                                         fbe_enclosure_mgmt_trace_buf_cmd_t * trace_buf_cmd_p,
                                                         ses_trace_buf_info_elem_struct * trace_buf_info_elem_p);
static fbe_status_t fbe_eses_enclosure_build_encl_time_elem(fbe_eses_enclosure_t * eses_enclosure, 
                                                         ses_encl_time_elem_struct * encl_time_elem_p);
static fbe_status_t fbe_eses_enclosure_build_general_info_expander_element(fbe_eses_enclosure_t * eses_enclosure, 
                                                         ses_general_info_elem_expander_struct * expander_elem_p);
static fbe_status_t fbe_eses_enclosure_build_general_info_array_dev_slot_elem(fbe_eses_enclosure_t * eses_enclosure, 
                                                         ses_general_info_elem_array_dev_slot_struct * general_info_elem_p,
                                                         fbe_u8_t slot_num);

static fbe_status_t fbe_eses_fill_download_control_page(fbe_sg_element_t *sg_p, 
                                                 fbe_u32_t image_size, 
                                                 fbe_u32_t ucode_data_bytes, 
                                                 fbe_u8_t mode, 
                                                 fbe_u8_t subencl_id,
                                                 fbe_u32_t  generation_code,
                                                 fbe_u16_t *parameter_length,
                                                 fbe_u32_t  image_bytes_sent);

static fbe_enclosure_status_t fbe_eses_enclosure_debug_drive_remove_cmd(fbe_eses_enclosure_t * eses_enclosure,
                                                                 fbe_u8_t drive_slot_num,
                                                                 fbe_u8_t * cmd_str,
                                                                 fbe_u8_t * cmd_str_len_p);

static fbe_enclosure_status_t fbe_eses_enclosure_debug_drive_insert_cmd(fbe_eses_enclosure_t * eses_enclosure,
                                                                 fbe_u8_t drive_slot_num,
                                                                 fbe_u8_t * cmd_str,
                                                                 fbe_u8_t * cmd_str_len_p);

static fbe_enclosure_status_t fbe_eses_enclosure_build_eep_mode_page(fbe_eses_enclosure_t * eses_enclosure, 
                                                                     fbe_eses_pg_eep_mode_pg_t * eep_page_p);

static fbe_enclosure_status_t fbe_eses_enclosure_build_eenp_mode_page(fbe_eses_enclosure_t * eses_enclosure, 
                                                                      fbe_eses_pg_eenp_mode_pg_t * eenp_page_p);

void printElemGroupInfo(fbe_eses_enclosure_t *eses_enclosure);

static fbe_status_t fbe_eses_ucode_build_dl_sg_list(fbe_eses_enclosure_t *eses_enclosure,
                                            fbe_sg_element_t *sgp, 
                                            fbe_u16_t *request_size, 
                                            fbe_u32_t *sg_item_count,
                                            fbe_u32_t ucode_data_offset);

static fbe_status_t 
fbe_eses_fill_tunnel_download_control_page(fbe_sg_element_t *sg_p,
                                                 fbe_u32_t image_size,
                                                 fbe_u32_t ucode_data_bytes,
                                                 fbe_u8_t mode,
                                                 fbe_u8_t subencl_id,
                                                 fbe_u32_t  generation_code,
                                                 fbe_u16_t *parameter_length,
                                                 fbe_u32_t  image_bytes_sent);


/**************************
 * GLOBALS 
 **************************/
extern const fbe_eses_enclosure_comp_type_hdlr_t * fbe_eses_enclosure_comp_type_hdlr_table[]; 


/*!*************************************************************************
* @fn fbe_eses_enclosure_build_emc_specific_ctrl_page()                  
***************************************************************************
* @brief
*       This function builds the EMC specific control page. It includes 
*       the following blocks in addition to the page header.
*       (1) The SAS connector info elements block.
*       (2) The trace buffer info elements block.
*       (3) The encl time block.      
*
* @param   eses_enclosure (INPUT)- The pointer to the enclosure.
* @param   trace_buf_cmd_p (INPUT) - The pointer to the trace buffer command.
* @param   sg_list (INPUT)- The pointer to the sg list which contains the page.
* @param   page_size_ptr (OUTPUT) - The pointer to the page size.
* @param   sg_count (OUTPUT)- The pointer to the sg count.
*
* @return  fbe_status_t 
*
* NOTES
*   The EMC specific control page does not need to include all the elements reported 
*   in the EMC specific status page. 
*   It only needs to include those elements to be controlled. 
*
* HISTORY
*   24-Sep-2008 PHE - Created.
*   15-Dec-2008 PHE - Built the block for the sas connector info elements.
***************************************************************************/
fbe_status_t fbe_eses_enclosure_build_emc_specific_ctrl_page(fbe_eses_enclosure_t * eses_enclosure,
                                                         fbe_enclosure_mgmt_trace_buf_cmd_t * trace_buf_cmd_p,
                                                         fbe_sg_element_t * sg_list,
                                                         fbe_u16_t * page_size_ptr,
                                                         fbe_u32_t *sg_count)
{
    ses_pg_emc_encl_ctrl_struct * emc_encl_ctrl_pg = NULL;
    ses_common_pg_hdr_struct * pg_hdr = NULL;
    fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p = NULL;
    fbe_u16_t page_size = 0;
    fbe_enclosure_component_types_t  component_type;
    fbe_status_t status = FBE_STATUS_OK;

    /**********
     * BEGIN
     **********/
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                    "%s entry \n", __FUNCTION__); 

    *sg_count = 2;

    /* sg_list[0].count will be updated with the page size at the end of the function.*/
    fbe_sg_element_init(&sg_list[0], 
                       0,   // count
                       (fbe_u8_t *)&sg_list[0] + (*sg_count) * sizeof(fbe_sg_element_t)); // address

    fbe_sg_element_init(&sg_list[1], 
                       0,   // count
                       NULL); // address  

    emc_encl_ctrl_pg = (ses_pg_emc_encl_ctrl_struct *)sg_list[0].address;

    pg_hdr = &emc_encl_ctrl_pg->hdr;
    pg_hdr->pg_code = SES_PG_CODE_EMC_ENCL_CTRL;

    /* when used in control page, partial field is set to tell enclosure to reset shutdown timer*/ 
    if (eses_enclosure->reset_shutdown_timer == TRUE)
    {
        emc_encl_ctrl_pg->partial = 1;
        eses_enclosure->reset_shutdown_timer = FALSE;
    }
    else
    {
        emc_encl_ctrl_pg->partial = 0;
    }

    emc_encl_ctrl_pg->shutdown_reason = 0; // Reserved in a control page.
    //fbe_eses_set_pg_gen_code takes care of the byte swap.
    fbe_eses_set_pg_gen_code(pg_hdr, eses_enclosure->eses_generation_code);
   
    /* Init to 0. Will be updated later. */
    emc_encl_ctrl_pg->num_info_elem_groups = 0;
    
    page_size = FBE_ESES_EMC_CTRL_STAT_FIRST_INFO_ELEM_GROUP_OFFSET;

    if(trace_buf_cmd_p != NULL)
    {
        /* Need to build the trace buffer info element group. */
        info_elem_group_hdr_p = (fbe_eses_info_elem_group_hdr_t *)((fbe_u8_t *)emc_encl_ctrl_pg + page_size);
        // Init to 0. It will be updated by the build_xxx_info_elem_group functions.
        info_elem_group_hdr_p->num_info_elems = 0;

        status = fbe_eses_enclosure_build_trace_buf_info_elem_group(eses_enclosure, 
                                          trace_buf_cmd_p, 
                                          emc_encl_ctrl_pg, 
                                          info_elem_group_hdr_p, 
                                          &page_size);
        
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                        "build_trace_buf_info_elem_group failed, status: 0x%x.\n", 
                        status); 
            return status;
        }
        /* We do not send other control with the trace buffer info elem control. */
    }
    else
    {
        // loop through all components to see whether there is any write needed.
        for (component_type = 0; component_type < FBE_ESES_ENCLOSURE_MAX_COMPONENT_TYPE; component_type ++)
        {
            info_elem_group_hdr_p = (fbe_eses_info_elem_group_hdr_t *)((fbe_u8_t *)emc_encl_ctrl_pg + page_size);
            // Init to 0. It will be updated by the build_xxx_info_elem_group functions.
            info_elem_group_hdr_p->num_info_elems = 0;

            switch(component_type)
            {
                case FBE_ENCL_CONNECTOR:
                    /* Check if we need to build the sas connector info elem group. If yes, build it. */
                    status = fbe_eses_enclosure_build_sas_conn_info_elem_group(eses_enclosure, 
                                                                            emc_encl_ctrl_pg, 
                                                                            info_elem_group_hdr_p, 
                                                                            &page_size);
                    
                    if(status != FBE_STATUS_OK)
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                    "build_sas_conn_info_elem_group failed, status: 0x%x.\n", 
                                    status);  
                        return status;            
                    }

                    break;

                case FBE_ENCL_ENCLOSURE:

                    /* Check if we need to build the encl time info elem group. If yes, build it. */
                    status = fbe_eses_enclosure_build_encl_time_info_elem_group(eses_enclosure, 
                                                                            emc_encl_ctrl_pg, 
                                                                            info_elem_group_hdr_p, 
                                                                            &page_size);

                    if(status != FBE_STATUS_OK)
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                    "build_encl_time_info_elem_group failed, status: 0x%x.\n", 
                                    status);  
                        return status;            
                    }

                        /* need to clear reset reason if enclosure was power cycled */
                    if(eses_enclosure->reset_reason == FBE_ESES_RESET_REASON_EXTERNAL){
                        // move on to the next group
                        info_elem_group_hdr_p = (fbe_eses_info_elem_group_hdr_t *)((fbe_u8_t *)emc_encl_ctrl_pg + page_size);
                        // Init to 0. It will be updated by the build_general_info_expander functions.
                        info_elem_group_hdr_p->num_info_elems = 0;

                        status = fbe_eses_enclosure_build_general_info_expander(eses_enclosure, 
                                                                            emc_encl_ctrl_pg, 
                                                                            info_elem_group_hdr_p, 
                                                                            &page_size);

                        if(status != FBE_STATUS_OK)
                        {
                            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                        "build_general_info_elem_group failed, status: 0x%x.\n", 
                                        status);  
                            return status;            
                        }

                    }
                    break;

                case FBE_ENCL_DRIVE_SLOT:

                    status = fbe_eses_enclosure_build_general_info_array_dev_slot(eses_enclosure, 
                                                                            emc_encl_ctrl_pg, 
                                                                            info_elem_group_hdr_p, 
                                                                            &page_size);

                    if(status != FBE_STATUS_OK)
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                    "build_general_info_elem_group failed, status: 0x%x.\n", 
                                    status);  
                        return status;            
                    }

                    break;

// JAP - PS Margining Test Control needs more work
#if FALSE
                case FBE_ENCL_POWER_SUPPLY:

                    /* Check if we need to build the PS info elem group. If yes, build it. */
                    status = fbe_eses_enclosure_build_ps_control_info_elem_group(eses_enclosure, 
                                                                                 emc_encl_ctrl_pg, 
                                                                                 info_elem_group_hdr_p, 
                                                                                 &page_size);

                    if(status != FBE_STATUS_OK)
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                    "build_ps_control_info_elem_group failed, status: 0x%x.\n", 
                                    status);  
                        return status;            
                    }

                    break;
#endif

                default:
                    break;
            }// End of - switch(component_type)
        }// End of - for (component_type = 0; component_type < FBE_ESES_ENCLOSURE_MAX_COMPONENT_TYPE; component_type ++)    
    } // End of - else - if(trace_buf_cmd_p != NULL)
    
    //fbe_eses_set_pg_len takes care of the byte swap.
    fbe_eses_set_pg_len(pg_hdr, page_size);
    *page_size_ptr = page_size;

    /* Update sg_list[0].count with the page size. */
    fbe_sg_element_init(&sg_list[0], 
                       page_size,   // count
                       (fbe_u8_t *)&sg_list[0] + (*sg_count) * sizeof(fbe_sg_element_t)); // address

    return FBE_STATUS_OK;
} // End of function fbe_eses_enclosure_build_emc_specific_ctrl_page

/*!*************************************************************************
* @fn fbe_eses_enclosure_build_trace_buf_info_elem_group()                  
***************************************************************************
* @brief
*       This function builds the trace buffer info element group.
*
* @param   eses_enclosure (INPUT)- The pointer to the enclosure.
* @param   trace_buf_cmd_p (INPUT) - The pointer to the trace buffer command.
* @param   emc_encl_ctrl_pg_p (INPUT/OUTPUT)- The pointer to the EMC enclosure control page.
* @param   info_elem_group_hdr_p (INPUT/OUTPUT) - The pointer to the info element group. 
* @param   page_size_ptr (OUTPUT/OUTPUT) - The pointer to the page size.
*
* @return  fbe_status_t 
*
* NOTES
*
* HISTORY
*   20-May-2009 PHE - Created.
***************************************************************************/
static fbe_status_t fbe_eses_enclosure_build_trace_buf_info_elem_group(fbe_eses_enclosure_t * eses_enclosure,
                                                         fbe_enclosure_mgmt_trace_buf_cmd_t * trace_buf_cmd_p,
                                                         ses_pg_emc_encl_ctrl_struct * emc_encl_ctrl_pg_p,
                                                         fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p,
                                                         fbe_u16_t * page_size_p)
{    
    fbe_u32_t page_size = 0;
    fbe_status_t status = FBE_STATUS_OK;
    ses_trace_buf_info_elem_struct * trace_buf_info_elem_p = NULL;
    /**********
    * BEGIN
    **********/
    page_size = *page_size_p;

    info_elem_group_hdr_p->info_elem_type = FBE_ESES_INFO_ELEM_TYPE_TRACE_BUF;

    /* Set the number of trace buffer info elements to 1. 
    * We currently can only send one trace buffer info element.
    */
    info_elem_group_hdr_p->num_info_elems = 1;
    info_elem_group_hdr_p->info_elem_size = FBE_ESES_EMC_CONTROL_TRACE_BUF_INFO_ELEM_SIZE;

    page_size += FBE_ESES_EMC_CTRL_STAT_INFO_ELEM_GROUP_HEADER_SIZE;

    if(page_size + FBE_ESES_EMC_CONTROL_TRACE_BUF_INFO_ELEM_SIZE > FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE) 
    {
        /* The allocated buffer is not large enough for the page, trace the fault.*/
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                    FBE_TRACE_LEVEL_ERROR,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                    "not enough memory for trace buf info elem, rqst: %d, allocated: %llu.\n", 
                    page_size + FBE_ESES_EMC_CONTROL_TRACE_BUF_INFO_ELEM_SIZE,
                    (unsigned long long)FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE);  

        return FBE_STATUS_GENERIC_FAILURE;   
    }
    
    trace_buf_info_elem_p = (ses_trace_buf_info_elem_struct *)((fbe_u8_t *)emc_encl_ctrl_pg_p + page_size); 
    status = fbe_eses_enclosure_build_trace_buf_info_elem(eses_enclosure, trace_buf_cmd_p, trace_buf_info_elem_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                    FBE_TRACE_LEVEL_ERROR,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                    "build_trace_buf_info_elem failed, status: 0x%x.\n", 
                    status);  

        return status;            
    }
    page_size += FBE_ESES_EMC_CONTROL_TRACE_BUF_INFO_ELEM_SIZE;

    emc_encl_ctrl_pg_p->num_info_elem_groups ++;

    // Update the returned page size. 
    *page_size_p = page_size;

    return FBE_STATUS_OK;
}


/*!*************************************************************************
* @fn fbe_eses_enclosure_build_sas_conn_info_elem_group()                  
***************************************************************************
* @brief
*       This function builds the sas connector info element group.
*
* @param   eses_enclosure (INPUT)- The pointer to the enclosure.
* @param   emc_encl_ctrl_pg_p (INPUT/OUTPUT)- The pointer to the EMC enclosure control page.
* @param   info_elem_group_hdr_p (INPUT/OUTPUT) - The pointer to the info element group. 
* @param   page_size_ptr (OUTPUT/OUTPUT) - The pointer to the page size.
*
* @return  fbe_status_t 
*
* NOTES
*
* HISTORY
*   20-May-2009 PHE - Created.
***************************************************************************/
static fbe_status_t fbe_eses_enclosure_build_sas_conn_info_elem_group(fbe_eses_enclosure_t * eses_enclosure,
                                                         ses_pg_emc_encl_ctrl_struct * emc_encl_ctrl_pg_p,
                                                         fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p,
                                                         fbe_u16_t * page_size_p)
{    
    fbe_u32_t page_size = 0;
    fbe_u8_t index = 0;
    fbe_u8_t number_of_components = 0;
    fbe_bool_t write_data_found = FALSE;
    fbe_bool_t first_sas_conn_to_write = TRUE;
    ses_sas_conn_info_elem_struct * sas_conn_info_elem_p = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;

    /**********
    * BEGIN
    **********/
    page_size = *page_size_p;

    status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_CONNECTOR,
                                                        &number_of_components);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    for(index = 0; index < number_of_components; index++)
    {
        // check if this component has Write/Control data
        encl_status = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA,
                                                    FBE_ENCL_CONNECTOR,
                                                    index,
                                                    &write_data_found);

        if ((encl_status == FBE_ENCLOSURE_STATUS_OK) && write_data_found)
        {
            if(first_sas_conn_to_write)
            {
                page_size += FBE_ESES_EMC_CTRL_STAT_INFO_ELEM_GROUP_HEADER_SIZE;
                first_sas_conn_to_write = FALSE;
            }
            
            // The write is needed. Check whether enough memory is allocated for the page.
            if(page_size + FBE_ESES_EMC_CONTROL_SAS_CONN_INFO_ELEM_SIZE > FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE) 
            {
                /* The allocated buffer is not large enough for the page, trace the fault.*/
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "not enough memory for sas conn info elem, rqst: %d, allocated: %llu.\n", 
                                page_size + FBE_ESES_EMC_CONTROL_SAS_CONN_INFO_ELEM_SIZE,
                                (unsigned long long)FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE);  

                return FBE_STATUS_GENERIC_FAILURE;    
            }

            sas_conn_info_elem_p = (ses_sas_conn_info_elem_struct *)((fbe_u8_t *)emc_encl_ctrl_pg_p + page_size); 
            status = fbe_eses_enclosure_build_sas_conn_info_elem(eses_enclosure, sas_conn_info_elem_p, index);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                            "build_sas_conn_info_elem failed, status: 0x%x.\n", 
                            status);  
                return status;            
            }

            encl_status = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA_SENT,
                                                    FBE_ENCL_CONNECTOR,
                                                    index,
                                                    TRUE);
            if(!ENCL_STAT_OK(encl_status))
            {
                return FBE_STATUS_GENERIC_FAILURE;            
            }

            page_size += FBE_ESES_EMC_CONTROL_SAS_CONN_INFO_ELEM_SIZE;
            info_elem_group_hdr_p->num_info_elems ++;
        }
    } // End of - for(index = 0; index < number_of_components; index++)

    if(info_elem_group_hdr_p->num_info_elems > 0)
    {
        info_elem_group_hdr_p->info_elem_type = FBE_ESES_INFO_ELEM_TYPE_SAS_CONN;
        info_elem_group_hdr_p->info_elem_size = FBE_ESES_EMC_CONTROL_SAS_CONN_INFO_ELEM_SIZE;
        emc_encl_ctrl_pg_p->num_info_elem_groups ++;                
    }

    // Update the returned page size. 
    *page_size_p = page_size;

    return FBE_STATUS_OK;
}


/*!*************************************************************************
* @fn fbe_eses_enclosure_build_encl_time_info_elem_group()                  
***************************************************************************
* @brief
*       This function builds the encl time info element group.
*
* @param   eses_enclosure (INPUT)- The pointer to the enclosure.
* @param   emc_encl_ctrl_pg_p (INPUT/OUTPUT)- The pointer to the EMC enclosure control page.
* @param   info_elem_group_hdr_p (INPUT/OUTPUT) - The pointer to the info element group. 
* @param   page_size_ptr (OUTPUT/OUTPUT) - The pointer to the page size.
*
* @return  fbe_status_t 
*
* NOTES
*
* HISTORY
*   20-May-2009 PHE - Created.
***************************************************************************/
static fbe_status_t fbe_eses_enclosure_build_encl_time_info_elem_group(fbe_eses_enclosure_t * eses_enclosure,
                                                         ses_pg_emc_encl_ctrl_struct * emc_encl_ctrl_pg_p,
                                                         fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p,
                                                         fbe_u16_t * page_size_p)
{
    fbe_u32_t page_size = 0;
    fbe_bool_t write_data_found = FALSE;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    ses_encl_time_elem_struct * encl_time_elem_p = NULL;

    /**********
    * BEGIN
    **********/
    page_size = *page_size_p;

    // check if this component has Write/Control data
    encl_status = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA,  // attribute
                                                FBE_ENCL_ENCLOSURE,  // component type
                                                0,  // component index
                                                &write_data_found);

    if ((encl_status == FBE_ENCLOSURE_STATUS_OK) && write_data_found)
    { 
        page_size += FBE_ESES_EMC_CTRL_STAT_INFO_ELEM_GROUP_HEADER_SIZE;

        /* Need to write the enclosure time element. */
        if(page_size + FBE_ESES_EMC_CONTROL_ENCL_TIME_INFO_ELEM_SIZE > FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE) 
        {
            /* The allocated buffer is not large enough for the page, trace the fault.*/
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                            "not enough memory for encl time elem, rqst: %d, allocated: %llu.\n", 
                            page_size + FBE_ESES_EMC_CONTROL_ENCL_TIME_INFO_ELEM_SIZE, 
                            (unsigned long long)FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE); 

            return FBE_STATUS_GENERIC_FAILURE;   
        }
        
        encl_time_elem_p = (ses_encl_time_elem_struct *)((fbe_u8_t *)emc_encl_ctrl_pg_p + page_size); 
        status = fbe_eses_enclosure_build_encl_time_elem(eses_enclosure, encl_time_elem_p);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                        "build_encl_time_elem failed, status: 0x%x.\n", 
                        status);  
            return status;            
        }

        encl_status = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA_SENT,
                                                    FBE_ENCL_ENCLOSURE,
                                                    0,
                                                    TRUE);
        if(!ENCL_STAT_OK(encl_status))
        {
            return FBE_STATUS_GENERIC_FAILURE;            
        }

        page_size += FBE_ESES_EMC_CONTROL_ENCL_TIME_INFO_ELEM_SIZE;
        
        info_elem_group_hdr_p->num_info_elems = 1;
        info_elem_group_hdr_p->info_elem_type = FBE_ESES_INFO_ELEM_TYPE_ENCL_TIME;
        info_elem_group_hdr_p->info_elem_size = FBE_ESES_EMC_CONTROL_ENCL_TIME_INFO_ELEM_SIZE;   
        emc_encl_ctrl_pg_p->num_info_elem_groups ++;
    }

    // Update the returned page size. 
    *page_size_p = page_size;

    return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_build_general_info_expander()                  
***************************************************************************
* @brief
*       This function builds the general info element group for expander to clear the reset reason.
*
* @param   eses_enclosure (INPUT)- The pointer to the enclosure.
* @param   emc_encl_ctrl_pg_p (INPUT/OUTPUT)- The pointer to the EMC enclosure control page.
* @param   info_elem_group_hdr_p (INPUT/OUTPUT) - The pointer to the info element group. 
* @param   page_size_ptr (OUTPUT/OUTPUT) - The pointer to the page size.
*
* @return  fbe_status_t 
*
* NOTES
*
* HISTORY
*   12-Apr-2009 NCHIU - Created.
***************************************************************************/
static fbe_status_t fbe_eses_enclosure_build_general_info_expander(fbe_eses_enclosure_t * eses_enclosure,
                                                         ses_pg_emc_encl_ctrl_struct * emc_encl_ctrl_pg_p,
                                                         fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p,
                                                         fbe_u16_t * page_size_p)
{
    fbe_u32_t page_size = 0;
    ses_general_info_elem_expander_struct * general_info_elem_p = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    /**********
    * BEGIN
    **********/
    page_size = *page_size_p;
    
    if((page_size + FBE_ESES_EMC_CTRL_STAT_INFO_ELEM_GROUP_HEADER_SIZE +FBE_ESES_EMC_CONTROL_GENERAL_INFO_ELEM_SIZE) 
            > FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE) 
    {
        /* The allocated buffer is not large enough for the page, trace the fault.*/
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                        "not enough memory for expander elem, rqst: %d, allocated: %llu.\n",
                        page_size + FBE_ESES_EMC_CONTROL_GENERAL_INFO_ELEM_SIZE, 
                        (unsigned long long)FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE); 

        return FBE_STATUS_GENERIC_FAILURE;   
    }

    general_info_elem_p = (ses_general_info_elem_expander_struct *)((fbe_u8_t *)info_elem_group_hdr_p + FBE_ESES_EMC_CTRL_STAT_INFO_ELEM_GROUP_HEADER_SIZE); 
    status = fbe_eses_enclosure_build_general_info_expander_element(eses_enclosure, general_info_elem_p);
    if (status == FBE_STATUS_OK)
    {
        /* now we have a valid entry, update the page to reflect the change of size */
        page_size += FBE_ESES_EMC_CONTROL_GENERAL_INFO_ELEM_SIZE + FBE_ESES_EMC_CTRL_STAT_INFO_ELEM_GROUP_HEADER_SIZE;

        info_elem_group_hdr_p->num_info_elems = 1;
        info_elem_group_hdr_p->info_elem_type = FBE_ESES_INFO_ELEM_TYPE_GENERAL;
        info_elem_group_hdr_p->info_elem_size = FBE_ESES_EMC_CONTROL_GENERAL_INFO_ELEM_SIZE;
        emc_encl_ctrl_pg_p->num_info_elem_groups ++;    
    }

    // Update the returned page size. 
    *page_size_p = page_size;

    return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_build_general_info_array_dev_slot()                  
***************************************************************************
* @brief
*       This function builds the general info element group for drive power cycle command.
*
* @param   eses_enclosure (INPUT)- The pointer to the enclosure.
* @param   emc_encl_ctrl_pg_p (INPUT/OUTPUT)- The pointer to the EMC enclosure control page.
* @param   info_elem_group_hdr_p (INPUT/OUTPUT) - The pointer to the info element group. 
* @param   page_size_ptr (OUTPUT/OUTPUT) - The pointer to the page size.
*
* @return  fbe_status_t 
*
* NOTES
*
* HISTORY
*   20-May-2009 PHE - Created.
***************************************************************************/
static fbe_status_t fbe_eses_enclosure_build_general_info_array_dev_slot(fbe_eses_enclosure_t * eses_enclosure,
                                                         ses_pg_emc_encl_ctrl_struct * emc_encl_ctrl_pg_p,
                                                         fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p,
                                                         fbe_u16_t * page_size_p)
{
    fbe_u32_t page_size = 0;
    fbe_u8_t index = 0;
    fbe_u8_t number_of_components = 0;
    fbe_bool_t write_data_found = FALSE;
    fbe_bool_t first_drive_slot_to_write = TRUE;
    ses_general_info_elem_array_dev_slot_struct * general_info_elem_p = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;

    /**********
    * BEGIN
    **********/
    page_size = *page_size_p;

    status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_DRIVE_SLOT,
                                            &number_of_components);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    for(index = 0; index < number_of_components; index++)
    {
        // check if this component has Write/Control data
        encl_status = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA,
                                                    FBE_ENCL_DRIVE_SLOT,
                                                    index,
                                                    &write_data_found);

        if ((encl_status == FBE_ENCLOSURE_STATUS_OK) && write_data_found)
        {
            if(first_drive_slot_to_write)
            {
                page_size += FBE_ESES_EMC_CTRL_STAT_INFO_ELEM_GROUP_HEADER_SIZE;
                first_drive_slot_to_write = FALSE;
            }
            
            // The write is needed. Check whether enough memory is allocated for the page.
            if(page_size + FBE_ESES_EMC_CONTROL_GENERAL_INFO_ELEM_SIZE > FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE) 
            {
                /* The allocated buffer is not large enough for the page, trace the fault.*/
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "not enough memory for general info elem, rqst: %d, allocated: %llu.\n",
                                page_size + FBE_ESES_EMC_CONTROL_GENERAL_INFO_ELEM_SIZE, 
                                (unsigned long long)FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE);  

                return FBE_STATUS_GENERIC_FAILURE;    
            }

            general_info_elem_p = (ses_general_info_elem_array_dev_slot_struct *)((fbe_u8_t *)emc_encl_ctrl_pg_p + page_size); 
            status = fbe_eses_enclosure_build_general_info_array_dev_slot_elem(eses_enclosure, general_info_elem_p, index);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                            "build_sas_conn_info_elem failed, status: 0x%x.\n", 
                            status);  
                return status;            
            }

            encl_status = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA_SENT,
                                                    FBE_ENCL_DRIVE_SLOT,
                                                    index,
                                                    TRUE);
            if(!ENCL_STAT_OK(encl_status))
            {
                return FBE_STATUS_GENERIC_FAILURE;            
            }

            page_size += FBE_ESES_EMC_CONTROL_GENERAL_INFO_ELEM_SIZE;
            info_elem_group_hdr_p->num_info_elems ++;
        }
    } // End of - for(index = 0; index < number_of_components; index++)

    if(info_elem_group_hdr_p->num_info_elems > 0)
    {
        info_elem_group_hdr_p->info_elem_type = FBE_ESES_INFO_ELEM_TYPE_GENERAL;
        info_elem_group_hdr_p->info_elem_size = FBE_ESES_EMC_CONTROL_GENERAL_INFO_ELEM_SIZE;
        emc_encl_ctrl_pg_p->num_info_elem_groups ++;                
    }

    // Update the returned page size. 
    *page_size_p = page_size;

    return FBE_STATUS_OK;
}
/*!*************************************************************************
* @fn fbe_eses_enclosure_build_ps_control_info_elem_group()                  
***************************************************************************
* @brief
*       This function builds the PS info element group for PS Margin Test control command.
*
* @param   eses_enclosure (INPUT)- The pointer to the enclosure.
* @param   emc_encl_ctrl_pg_p (INPUT/OUTPUT)- The pointer to the EMC enclosure control page.
* @param   info_elem_group_hdr_p (INPUT/OUTPUT) - The pointer to the info element group. 
* @param   page_size_ptr (OUTPUT/OUTPUT) - The pointer to the page size.
*
* @return  fbe_status_t 
*
* NOTES
*
* HISTORY
*   18-Dec-2009 Joe Perry - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_build_ps_control_info_elem_group(fbe_eses_enclosure_t * eses_enclosure,
                                                    ses_pg_emc_encl_ctrl_struct * emc_encl_ctrl_pg_p,
                                                    fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p,
                                                    fbe_u16_t * page_size_p)
{
    fbe_u32_t page_size = 0;
    fbe_u8_t index = 0;
    fbe_u8_t number_of_components = 0;
    fbe_bool_t write_data_found = FALSE;
    fbe_bool_t first_ps_to_write = TRUE;
    ses_ps_info_elem_struct * general_info_elem_p = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;

    /**********
    * BEGIN
    **********/
    page_size = *page_size_p;

    status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_POWER_SUPPLY,
                                            &number_of_components);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    for(index = 0; index < number_of_components; index++)
    {
        // check if this component has Write/Control data
        encl_status = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                      FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA,
                                                      FBE_ENCL_POWER_SUPPLY,
                                                      index,
                                                      &write_data_found);

        if ((encl_status == FBE_ENCLOSURE_STATUS_OK) && write_data_found)
        {
            if(first_ps_to_write)
            {
                page_size += FBE_ESES_EMC_CTRL_STAT_INFO_ELEM_GROUP_HEADER_SIZE;
                first_ps_to_write = FALSE;
            }
            
            // The write is needed. Check whether enough memory is allocated for the page.
            if(page_size + FBE_ESES_EMC_CTRL_STAT_PS_INFO_ELEM_SIZE > FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE) 
            {
                /* The allocated buffer is not large enough for the page, trace the fault.*/
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "not enough memory for ps info elem, rqst: %d, allocated: %llu.\n",
                                page_size + FBE_ESES_EMC_CTRL_STAT_PS_INFO_ELEM_SIZE, 
                                (unsigned long long)FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE);  

                return FBE_STATUS_GENERIC_FAILURE;    
            }

            general_info_elem_p = (ses_ps_info_elem_struct *)((fbe_u8_t *)emc_encl_ctrl_pg_p + page_size); 
            encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_ELEM_INDEX,  //Attribute
                                                        FBE_ENCL_POWER_SUPPLY, // Component type
                                                        index, // Component index
                                                        &general_info_elem_p->ps_elem_index);

            if(!ENCL_STAT_OK(encl_status)) 
            {
                status = FBE_STATUS_GENERIC_FAILURE;
            }

            encl_status = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA_SENT,
                                                    FBE_ENCL_POWER_SUPPLY,
                                                    index,
                                                    TRUE);
            if(!ENCL_STAT_OK(encl_status))
            {
                return FBE_STATUS_GENERIC_FAILURE;            
            }

            page_size += FBE_ESES_EMC_CTRL_STAT_PS_INFO_ELEM_SIZE;
            info_elem_group_hdr_p->num_info_elems ++;
        }
    } // End of - for(index = 0; index < number_of_components; index++)

    if(info_elem_group_hdr_p->num_info_elems > 0)
    {
        info_elem_group_hdr_p->info_elem_type = FBE_ESES_INFO_ELEM_TYPE_DRIVE_POWER;
        info_elem_group_hdr_p->info_elem_size = FBE_ESES_EMC_CTRL_STAT_PS_INFO_ELEM_SIZE;
        emc_encl_ctrl_pg_p->num_info_elem_groups ++;                
    }

    // Update the returned page size. 
    *page_size_p = page_size;

    return FBE_STATUS_OK;

}   // end of fbe_eses_enclosure_build_ps_control_info_elem_group

/*!*************************************************************************
* @fn fbe_eses_enclosure_build_sas_conn_info_elem(fbe_eses_enclosure_t * eses_enclosure, 
*                                                ses_sas_conn_info_elem_struct * sas_conn_info_elem_p,
*                                                fbe_u8_t connector_index)
***************************************************************************
* @brief
*       This function builds the sas connector information element 
*       in the EMC specific control page. It sets:
*       (1) port use;
*       (2) wide port id;
*       (3) connector element index.
*
* @param   eses_enclosure - The pointer to the enclosure.
* @param   sas_conn_info_elem_p - The pointer which points to the sas connector
*                                  information element in the EMC specific control page.
* @param   connector_index - The connector index in EDAL.
*
* @return  fbe_status_t 
*
* NOTES
*   Support of connectors is supported only for local LCC connectors. 
*
* HISTORY
*   15-Dec-2008 PHE - Created.
***************************************************************************/
static fbe_status_t fbe_eses_enclosure_build_sas_conn_info_elem(fbe_eses_enclosure_t * eses_enclosure, 
                                                         ses_sas_conn_info_elem_struct * sas_conn_info_elem_p,
                                                         fbe_u8_t connector_index)
{
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_bool_t  is_primary_port = FALSE;
     
    /********
     * BEGIN
     ********/
    fbe_zero_memory(sas_conn_info_elem_p, FBE_ESES_EMC_CONTROL_SAS_CONN_INFO_ELEM_SIZE);

   /* Set the wide port identifier of 0, 
    * and this will assign all the lanes in the connector to the same wide port.
    */        
    sas_conn_info_elem_p->wide_port_id = 0;

    if(connector_index != FBE_ENCLOSURE_VALUE_INVALID)
    {
        // The matching connector index is found.
        // Get the value for the attribute FBE_ENCL_CONNECTOR_PRIMARY_PORT.
        encl_status = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_CONNECTOR_PRIMARY_PORT, // Attribute.
                                                        FBE_ENCL_CONNECTOR,  // Component type.
                                                        connector_index, // Component index.
                                                        &is_primary_port); // The value to be returned.

        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if(is_primary_port)
        {
            sas_conn_info_elem_p->port_use = FBE_ESES_EXP_PORT_UPSTREAM;
        }
        else 
        {
            sas_conn_info_elem_p->port_use = FBE_ESES_EXP_PORT_DOWNSTREAM;
        }
         

        // Get the corresponding element index for the connector found.
        encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_COMP_ELEM_INDEX, // attribute.
                                                FBE_ENCL_CONNECTOR,  // component type.
                                                connector_index, // component index.
                                                &sas_conn_info_elem_p->conn_elem_index); // the value to be returned.
        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }        
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                            "build_sas_conn_info_elem, mapping failed, invalid connector_index.\n"); 
   
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}// End of - fbe_eses_enclosure_build_sas_conn_info_elem

/*!*************************************************************************
 * @fn fbe_eses_enclosure_build_trace_buf_info_elem(fbe_eses_enclosure_t * eses_enclosure, 
 *                                                       fbe_enclosure_mgmt_trace_buf_cmd_t *trace_buf_cmd_p,
 *                                                       ses_trace_buf_info_elem_struct * trace_buf_info_elem_p)
 ***************************************************************************
 * @brief
 *       This function builds the trace buffer information element in the EMC
 *       specific control page.
 *
 * @param   eses_enclosure - The pointer to the enclosure.
 * @param   trace_buf_cmd_p - The pointer to the trace buffer commmand. 
 * @param   trace_buf_info_elem_p - The pointer which points to the trace buffer
 *                                  information element in the EMC specific control page.
 * @return  fbe_status_t 
 *
 * NOTES
 *
 * HISTORY
 *   24-Sep-2008 PHE - Created.
 ***************************************************************************/
static fbe_status_t fbe_eses_enclosure_build_trace_buf_info_elem(fbe_eses_enclosure_t * eses_enclosure, 
                                                         fbe_enclosure_mgmt_trace_buf_cmd_t * trace_buf_cmd_p,
                                                         ses_trace_buf_info_elem_struct * trace_buf_info_elem_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;     
    /********
     * BEGIN
     ********/
    fbe_zero_memory(trace_buf_info_elem_p, FBE_ESES_EMC_CONTROL_TRACE_BUF_INFO_ELEM_SIZE);
    
    switch(trace_buf_cmd_p->buf_op)
    {
        case FBE_ENCLOSURE_TRACE_BUF_OP_SAVE:
            trace_buf_info_elem_p->buf_action = FBE_ESES_TRACE_BUF_ACTION_CTRL_SAVE_BUF;     
            break;

        case FBE_ENCLOSURE_TRACE_BUF_OP_CLEAR:
            trace_buf_info_elem_p->buf_action = FBE_ESES_TRACE_BUF_ACTION_CTRL_CLEAR_BUF;
            break;

        default:
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                            "build_trace_buf_info_elem, unhandled cmd: 0x%x.\n", 
                            trace_buf_cmd_p->buf_op);  
   
            return FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    trace_buf_info_elem_p->buf_id = trace_buf_cmd_p->buf_id;

    // Need to specify the elem index which should be the elem index of the local sas expander. 
    encl_status = fbe_eses_enclosure_get_local_sas_exp_elem_index(eses_enclosure, &(trace_buf_info_elem_p->elem_index));

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_build_encl_time_elem(fbe_eses_enclosure_t * eses_enclosure, 
*                                             ses_encl_time_elem_struct * encl_time_elem_p)                  
***************************************************************************
* @brief
*       This function builds the enclosure time element in the EMC specific
*       control page.
*
* @param   eses_enclosure - The pointer to the enclosure.
* @param   encl_time_elem_p - The pointer which points to the enclosure time
*                            element in the EMC specific control page.
*
* @return  fbe_status_t 
*
* NOTES
*
* HISTORY
*   24-Sep-2008 PHE - Created.
***************************************************************************/
static fbe_status_t fbe_eses_enclosure_build_encl_time_elem(fbe_eses_enclosure_t * eses_enclosure, 
                                                         ses_encl_time_elem_struct * encl_time_elem_p)
{
    fbe_system_time_t system_time;
    fbe_u32_t         milliseconds = 0;

    fbe_get_system_time(&system_time);
    encl_time_elem_p->valid = 1;
    encl_time_elem_p->year = (fbe_u8_t)(system_time.year % 100);
    encl_time_elem_p->month = (fbe_u8_t)system_time.month;
    encl_time_elem_p->day = (fbe_u8_t)system_time.day;
    // The value to be written in time_zone needs to be re-evaluated.
    encl_time_elem_p->time_zone = FBE_ESES_EMC_CTRL_STAT_ENCL_TIME_ZONE_UNSPECIFIED;
    milliseconds = system_time.hour * 3600000 + system_time.minute * 60000 + system_time.milliseconds;
    encl_time_elem_p->milliseconds = swap32(milliseconds);

    return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_build_general_info_expander_element(fbe_eses_enclosure_t * eses_enclosure, 
*                                             ses_general_info_elem_expander_struct * expander_elem_p)                  
***************************************************************************
* @brief
*       This function builds the expander element in the EMC specific
*       control page, which clears the reset reason for the local expander.
*
* @param   eses_enclosure - The pointer to the enclosure.
* @param   expander_elem_p - The pointer which points to the expader 
*                            element in the EMC specific control page.
*
* @return  fbe_status_t 
*
* NOTES
*
* HISTORY
*   12-Apr-2010 NCHIU - Created.
***************************************************************************/
static fbe_status_t fbe_eses_enclosure_build_general_info_expander_element(fbe_eses_enclosure_t * eses_enclosure, 
                                                         ses_general_info_elem_expander_struct * expander_elem_p)
{
    fbe_enclosure_status_t              enclStatus;
    fbe_status_t                        retStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t                            enclSide = FBE_ESES_SUBENCL_SIDE_ID_INVALID;
    fbe_u8_t                            elemIndex;

    enclStatus = fbe_base_enclosure_edal_getEnclosureSide((fbe_base_enclosure_t *)eses_enclosure,
                                                          &enclSide);
    if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
    {
        /* expander element indexed by side ide */
        enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_COMP_ELEM_INDEX,  // Attribute
                                                FBE_ENCL_EXPANDER, 
                                                enclSide,
                                                &elemIndex); 
        if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
        {
            expander_elem_p->reset_reason = FBE_ESES_EXPANDER_ACTION_CLEAR_RESET;
            expander_elem_p->elem_index = elemIndex;
            retStatus = FBE_STATUS_OK;
        }
    }

    if (retStatus != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                        "build control for expander elem failed, encl status 0x%x, side: %d.\n", 
                        enclStatus,enclSide); 
    }
    return retStatus;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_build_general_info_array_dev_slot_elem(fbe_eses_enclosure_t * eses_enclosure, 
*                                             ses_general_info_elem_array_dev_slot_struct * general_info_elem_p,
*                                             fbe_u8_t slot_num)                  
***************************************************************************
* @brief
*       This function builds general info element for drive power cycle command
*       in the EMC specific control page.
*
* @param   eses_enclosure - The pointer to the enclosure.
* @param   general_info_elem_p - The pointer which points to the general info 
*                            element in the EMC specific control page.
* @param   slot_num - the drive slot number.
*
* @return  fbe_status_t 
*
* NOTES
*
* HISTORY
*   19-May-2009 PHE - Created.
***************************************************************************/
static fbe_status_t fbe_eses_enclosure_build_general_info_array_dev_slot_elem(fbe_eses_enclosure_t * eses_enclosure, 
                                                         ses_general_info_elem_array_dev_slot_struct * general_info_elem_p,
                                                         fbe_u8_t slot_num)
{
    fbe_u8_t                elem_index = 0;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
     
    /********
     * BEGIN
     ********/
    fbe_zero_memory(general_info_elem_p, sizeof(ses_general_info_elem_array_dev_slot_struct));

    encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                    FBE_ENCL_COMP_ELEM_INDEX,  //Attribute
                                    FBE_ENCL_DRIVE_SLOT, // Component type
                                    slot_num, // Component index
                                    &elem_index);

    if(!ENCL_STAT_OK(encl_status)) 
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    
    general_info_elem_p->elem_index = elem_index;
    if (eses_enclosure->drive_info[slot_num].drive_need_power_cycle == TRUE)
    {
        general_info_elem_p->power_cycle = 1;
        // The duration for the power cycle in 0.5 second increments.
        general_info_elem_p->duration = eses_enclosure->drive_info[slot_num].power_cycle_duration;
        eses_enclosure->drive_info[slot_num].drive_need_power_cycle = FALSE;
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                        "Build emc specific ctrl page to power cycle drive slot %d.\n", 
                        slot_num); 
    }        

    general_info_elem_p->battery_backed = eses_enclosure->drive_info[slot_num].drive_battery_backed;
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                        "Build emc specific ctrl page to set drive slot %d battery backed bit to %d.\n", 
                        slot_num, eses_enclosure->drive_info[slot_num].drive_battery_backed); 

    return FBE_STATUS_OK;
}

/**************************************************************************
* @fn fbe_eses_ucode_build_dl_sg_list()
***************************************************************************
* DESCRIPTION
*   @brief
*   Build the sg list to be used for firmware download. Configure the sg
*   list items with valid count and address fields. Allocate buffers as 
*   needed.
*
*   Note that we overload with the original fbe_eses_ucode_build_dl_sg_list
*   with the new input parameter, ucode_data_offset, so the function can 
*   be used for both download page and tunneled download page. The old
*   fbe_eses_ucode_build_dl_sg_list function can be deprecated.
*   
* PARAMETERS
*   @param sgp - poitner to the start of the sg list
*   @param request_size - pointer to the number of image bytes to transfer
*   @param sg_item_count - pointer to the number of sg items contain in the list
*   @param ucode_data_offset - number of bytes before the Microcode data 
*          (i.e. the image) field. 
*
* RETURN VALUES
*   @return - request_size is modified if the size gets truncated
*   @return - sg_item_count
*
* HISTORY
*   31-Mar-2009 GB - Created.
***************************************************************************/
static fbe_status_t fbe_eses_ucode_build_dl_sg_list(fbe_eses_enclosure_t * eses_enclosure,
                                            fbe_sg_element_t *sgp, 
                                            fbe_u16_t *request_size, 
                                            fbe_u32_t *sg_item_count,
                                            fbe_u32_t ucode_data_offset)
{
    fbe_u8_t            *buffer_endp;
    fbe_u32_t           bytes_left;
    fbe_u32_t           image_bytes;
    fbe_u32_t           sg_buffer_data_byte_count;
    fbe_u32_t           sglist_space;
    fbe_sg_element_t    *list_startp;
    fbe_enclosure_status_t got_buffer;
    
    // save pointer to the first element
    list_startp = sgp;

    // save the last address of this buffer, to be used as a sanity check
    buffer_endp = ((fbe_u8_t*)list_startp + FBE_MEMORY_CHUNK_SIZE);

    // fill in the byte count for the first item (diag pg info)
    sgp->count = ucode_data_offset;

    // sg_count starts with 2 items: snd_diag page and termination
    *sg_item_count = 2;
    sg_buffer_data_byte_count = sgp->count;
    image_bytes = 0;

    // use sg_count+1 to account for the item we want to add
    sglist_space = fbe_eses_enclosure_sg_list_bytes_left(*sg_item_count+1, sg_buffer_data_byte_count);

    // set up a counter for the allocated space needed
    bytes_left = *request_size;

    // if the request size won't fit in the sg list page, allocate a new mem chunk
    while(bytes_left > sglist_space)
    {
        // get the size needed for this page
        image_bytes = (bytes_left > FBE_MEMORY_CHUNK_SIZE) ? FBE_MEMORY_CHUNK_SIZE : bytes_left;

        // use the next sg item
        sgp++;

        // allocate the buffer and set the sg element fields
        got_buffer = fbe_eses_get_chunk_and_add_to_list(eses_enclosure, sgp, image_bytes);
        if(got_buffer != FBE_ENCLOSURE_STATUS_OK)
        {
            // no buffer available, adjust sgp and sg_item_count because and
            // transfer whatever fits in the buffer being used for the sg list
            sgp--;
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s: no buffer available to create sg list.\n", 
                                __FUNCTION__);  

            return FBE_STATUS_GENERIC_FAILURE;
        }
        
        // sgp was incremented above, now bump the count to match 
        (*sg_item_count)++;
        bytes_left -= image_bytes;

        // check the available space in the buffer used for the sg list (add 1 to
        // item_count to allow for space needed for the final element)
        sglist_space = fbe_eses_enclosure_sg_list_bytes_left(*sg_item_count + 1, sg_buffer_data_byte_count);

    } // end while


    if (bytes_left == 0)
    {
        // bump the item count for the terminator item that will be added
        //(*sg_item_count)++;

        // the send diag page data follows the terminator item
        list_startp->address = (fbe_u8_t *)&list_startp[*sg_item_count];
    }
    else
    {
        // use the sg list buffer to hold the remaining image data
        // item count is adjusted for this item and the terminating item
        *sg_item_count += 1;

        // the send diag page data follows the terminator item
        list_startp->address = (fbe_u8_t *)&list_startp[*sg_item_count];

        // next sg item - for the last image bytes
        sgp++;

        // if buffers were not available, bytes_left can be larger than the
        // remainder of the sg list buffer size
        if (bytes_left > sglist_space)
        {
            // since the request is truncated, modify the request size
            //   request_size - bytes_left = bytes allocated
            //   bytes allocated + sglist_space = adjusted bytes requested
            *request_size = *request_size - bytes_left + sglist_space;
            sgp->count = sglist_space;
        }
        else
        {
            sgp->count = bytes_left;
        }
        // calculate the start address of the image data
        // the image data follows the send diag page data (from the first item)
        sgp->address = list_startp->address + list_startp->count;
    }

    // terminate the list
    sgp++;
    fbe_sg_element_terminate(sgp);

    return FBE_STATUS_OK;
} // end fbe_eses_ucode_build_dl_sg_list


/**************************************************************************
* @fn fbe_eses_ucode_build_download_or_activate()
***************************************************************************
* DESCRIPTION
*   @brief
*   For download operation:
*     Build the sg list for the download control page and for the image data.
*     Fill the list entries and copy the image data.
*     Keep track of image bytes sent.
*
*   This function will update enclCurrentFupInfo.enclFupCurrTransferSize
*   
*   For activate operation:
*     Build the sg list for the activate control page only.
*
* PARAMETERS
*   @param eses_enclosure_p - The pointer to the eses enclosure
*   @param fbe_sg_element_t      - The pointer to the scatter gather list.
*   @param parameter_length_p    - returns the paramenter length, 
*                           to be reported in the cdb
*   @param sg_count              - returns sg_count.  This count includes 
*                           the last termination element.
*
* RETURN VALUES
*   @return fbe_u32_t number of image bytes being sent
*
* HISTORY
*   18-Aug-2008 GB - Created.
***************************************************************************/
fbe_status_t 
fbe_eses_ucode_build_download_or_activate(fbe_eses_enclosure_t * eses_enclosure_p, 
                                        fbe_sg_element_t *sg_p, 
                                        fbe_u16_t *parameter_length_p,
                                        fbe_u32_t *sg_count)
{
    fbe_u32_t       bytes_remaining;
    fbe_u32_t       sgl_size;
    fbe_u8_t        sub_encl_id;
    fbe_u8_t        download_mode;
    fbe_u32_t       generation_code;
    fbe_u16_t       sgl_bytes = 0;        // number of bytes copied to sg list and sent in this transfer
    fbe_u32_t       image_size = 0;
    fbe_u32_t       image_bytes_sent = 0; // the number of bytes downloaded to the enclosure, taken from base_encl_obj data
    fbe_u8_t        * image_p = NULL;
    fbe_eses_encl_fw_component_info_t *fw_info_p = NULL;
    fbe_u16_t       max_data_bytes;
    fbe_status_t    status;

    // get a pointer to the eses version data for this fbe specified component
    status = fbe_eses_enclosure_get_fw_target_addr(eses_enclosure_p,
                                          eses_enclosure_p->enclCurrentFupInfo.enclFupComponent, 
                                          eses_enclosure_p->enclCurrentFupInfo.enclFupComponentSide,
                                          &fw_info_p);

    if (status != FBE_STATUS_OK || fw_info_p == NULL)
    {
        // error - target not found
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sub_encl_id = fw_info_p->fwEsesComponentSubEnclId;
    generation_code = eses_enclosure_p->eses_generation_code;

    // image page max size is 4072 ans must be multiple of 4
    max_data_bytes = FBE_ESES_DL_CONTROL_MAX_UCODE_DATA_LENGTH & 0x0ff8;

    // first sg item
    sgl_size = 1;

    if (eses_enclosure_p->enclCurrentFupInfo.enclFupOperation == FBE_ENCLOSURE_FIRMWARE_OP_DOWNLOAD)
    {
        /* We are going to have 3 sg_list elements
        * 1. header for the download code, 24 bytes,
        * 2. points to image itself, with calculated bytes.
        * 3. terminator.
        *
        * If the image itself is described through sg list,
        * we may need to shift things down.
        */
        // get the accumulated bytes downloaded for this image
        image_bytes_sent = eses_enclosure_p->enclCurrentFupInfo.enclFupBytesTransferred;

        // get the image size
        image_size = eses_enclosure_p->enclCurrentFupInfo.enclFupImageSize;

        if (image_size > image_bytes_sent)
        {
            image_p = (fbe_u8_t *) eses_enclosure_p->enclCurrentFupInfo.enclFupImagePointer;

            if (image_p == NULL)
            {
                // error
                return FBE_STATUS_GENERIC_FAILURE;
            }

            bytes_remaining = image_size - image_bytes_sent;
            // set the number of image bytes to transfer
            // this count will also be used in the control page field ucode_data_length
            if (bytes_remaining < max_data_bytes)
            {
                sgl_bytes = bytes_remaining;
            }
            else
            {
                sgl_bytes = max_data_bytes;
            }

            // build the sg list for this transfer
            fbe_eses_ucode_build_dl_sg_list(eses_enclosure_p, sg_p, &sgl_bytes, sg_count, FBE_ESES_DL_CONTROL_UCODE_DATA_OFFSET);

            // next byte to copy/transfer
            image_p += image_bytes_sent;

            // sg_p points to the first item, which is the diag page data

            // copy the image item[1] into the sg list
            fbe_eses_enclosure_copy_data_to_sg(&sg_p[1], image_p);
        }

        download_mode = FBE_ESES_DL_CONTROL_MODE_DOWNLOAD;
    }  // end of adding image to sg_list for download
    else if (eses_enclosure_p->enclCurrentFupInfo.enclFupOperation == FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE)
    {
        download_mode = FBE_ESES_DL_CONTROL_MODE_ACTIVATE;
        /* We are going to have 2 sg_list elements
        * 1. header for the activate page, 24 bytes,
        * 2. terminator.
        */

        // Set the data size and address for the send diag page in sg_p[0].
        // There will be one more item added to the list so the address starts
        // after the last item. Index 3 is sgl_size+1 which is the 
        // starting address for the data.
        fbe_sg_element_init(sg_p,
                            FBE_ESES_DL_CONTROL_UCODE_DATA_OFFSET,
                            &sg_p[sgl_size+1]);

        // set the termination element
        fbe_sg_element_terminate(&sg_p[sgl_size]);
    }
    else
    {
        // let's do download but not sending any image out.
        download_mode = FBE_ESES_DL_CONTROL_MODE_DOWNLOAD;

        // get the image size
        image_size = eses_enclosure_p->enclCurrentFupInfo.enclFupImageSize;

        // Set the data size and address for the send diag page in sg_p[0].
        // There will be one more item added to the list so the address starts
        // after the last item. Index 3 is sgl_size+1 which is the 
        // starting address for the data.
        fbe_sg_element_init(sg_p,
                            FBE_ESES_DL_CONTROL_UCODE_DATA_OFFSET,
                            &sg_p[sgl_size+1]);

        // set the termination element
        fbe_sg_element_terminate(&sg_p[sgl_size]);
    }

     // set bytes 0-23, up to ucode data and parameter_length_p
    fbe_eses_fill_download_control_page(&sg_p[0], 
                                        image_size, 
                                        sgl_bytes, 
                                        download_mode,
                                        sub_encl_id,
                                        generation_code,
                                        parameter_length_p, 
                                        image_bytes_sent);

    eses_enclosure_p->enclCurrentFupInfo.enclFupCurrTransferSize = sgl_bytes;
    if (*parameter_length_p != (sgl_bytes+FBE_ESES_DL_CONTROL_UCODE_DATA_OFFSET))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                                "%s, esesFwTarget %s, bad pagesize exp:0x%x act:0x%x\n", 
                                __FUNCTION__, 
                                fbe_eses_enclosure_fw_targ_to_text(eses_enclosure_p->enclCurrentFupInfo.enclFupComponent),
                                *parameter_length_p, 
                                (sgl_bytes+FBE_ESES_DL_CONTROL_UCODE_DATA_OFFSET));  
        
    }
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                            "%s, esesFwTarget %s pagesize:0x%x pagelen:0x%x.\n", 
                            __FUNCTION__,
                            fbe_eses_enclosure_fw_targ_to_text(eses_enclosure_p->enclCurrentFupInfo.enclFupComponent),
                            *parameter_length_p, 
                            (sgl_bytes+FBE_ESES_DL_CONTROL_UCODE_DATA_LENGTH_OFFSET-FBE_ESES_PAGE_LENGTH_OFFSET));  

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                            "dataOffset:%d datalen:%d sent:%d downloadMode 0x%x subEnclId %d.\n", 
                            image_bytes_sent,
                            sgl_bytes, 
                            sgl_bytes+image_bytes_sent,
                            download_mode,
                            sub_encl_id);  
    

    return FBE_STATUS_OK;
} // fbe_eses_ucode_build_download_or_activate

/**************************************************************************
* @fn fbe_eses_fill_download_control_page()
***************************************************************************
* DESCRIPTION
*   @brief
*   Build the download control page up to the image data bytes. The image 
*   data is pointed to from the sg list.
*
* PARAMETERS
*   @param fbe_sg_element_t - The pointer to the scatter gather list.
*
* RETURN VALUES
*   @return fbe_u32_t number of image bytes being sent
*
* HISTORY
*   18-Aug-2008 GB - Created.
***************************************************************************/
static fbe_status_t fbe_eses_fill_download_control_page(fbe_sg_element_t *sg_p, 
                                                 fbe_u32_t image_size, 
                                                 fbe_u32_t ucode_data_bytes, 
                                                 fbe_u8_t mode, 
                                                 fbe_u8_t subencl_id,
                                                 fbe_u32_t  generation_code,
                                                 fbe_u16_t *parameter_length,
                                                 fbe_u32_t  image_bytes_sent)
{
    fbe_u16_t                                   page_length;
    fbe_eses_download_control_page_header_t    *page_p;    
    ses_common_pg_hdr_struct                   *hdr = NULL;

    // page_length = image_data_bytes + offset to the image data - offset to page length
    page_length = ucode_data_bytes + FBE_ESES_DL_CONTROL_UCODE_DATA_LENGTH_OFFSET - FBE_ESES_PAGE_LENGTH_OFFSET;
    // set the total page size
    *parameter_length =  page_length + FBE_ESES_PAGE_SIZE_ADJUST;

    // the buffer has been zeroed, so only fill the non-zero fields

    // start of the page
    page_p = (fbe_eses_download_control_page_header_t *)sg_p->address;
    page_p->page_code = SES_PG_CODE_DOWNLOAD_MICROCODE_CTRL;
    page_p->subenclosure_id = subencl_id;
    page_p->page_length_msb = (page_length >> 8) & 0xFF;
    page_p->page_length_lsb = page_length & 0xFF;
    hdr = (ses_common_pg_hdr_struct *)page_p;
    //fbe_eses_set_pg_gen_code takes care of the byte swap. 
    fbe_eses_set_pg_gen_code(hdr, generation_code);  
    page_p->mode = mode;
    page_p->buffer_offset = swap32(image_bytes_sent);
    // Use buffer_id 0 to indicate download to all MCU(s) located on the subEnclosure.
    page_p->buffer_id = 0;
    page_p->image_length = swap32(image_size);
    page_p->transfer_length = swap32(ucode_data_bytes);
    return FBE_STATUS_OK;
} // fbe_eses_fill_download_control_page

/**************************************************************************
* @fn fbe_eses_ucode_build_tunnel_download_or_activate()
***************************************************************************
* DESCRIPTION
*   @brief
*   For download operation:
*     Build the sg list for the tunnel download control page and for the image data.
*     Fill the list entries and copy the image data.
*     Keep track of image bytes sent.
*
*   This function will update enclCurrentFupInfo.enclFupCurrTransferSize
*   
*   For activate operation:
*     Build the sg list for the tunnel activate control page only.
*
* PARAMETERS
*   @param eses_enclosure_p - The pointer to the eses enclosure
*   @param fbe_sg_element_t      - The pointer to the scatter gather list.
*   @param parameter_length_p    - returns the paramenter length, 
*                           to be reported in the cdb
*   @param sg_count              - returns sg_count.  This count includes 
*                           the last termination element.
*
* RETURN VALUES
*   @return fbe_status_t
*
* HISTORY
*   23-Feb-2011 Kenny Huang - Created.
***************************************************************************/
fbe_status_t
fbe_eses_ucode_build_tunnel_download_or_activate(fbe_eses_enclosure_t * eses_enclosure_p,
                                                 fbe_sg_element_t *sg_p,
                                                 fbe_u16_t *parameter_length_p,
                                                 fbe_u32_t *sg_count)
{
    fbe_u32_t       bytes_remaining;
    fbe_u32_t       sgl_size;
    fbe_u8_t        sub_encl_id;
    fbe_u8_t        download_mode;
    fbe_u32_t       generation_code;
    fbe_u16_t       sgl_bytes = 0;        // number of bytes copied to sg list and sent in this transfer
    fbe_u32_t       image_size = 0;
    fbe_u32_t       image_bytes_sent = 0; // the number of bytes downloaded to the enclosure, taken from base_encl_obj data
    fbe_u8_t        * image_p = NULL;
    fbe_eses_encl_fw_component_info_t *fw_info_p = NULL;
    fbe_u16_t       max_data_bytes;
    fbe_status_t    status;

    status = fbe_eses_enclosure_get_fw_target_addr(eses_enclosure_p,
                                          eses_enclosure_p->enclCurrentFupInfo.enclFupComponent,
                                          eses_enclosure_p->enclCurrentFupInfo.enclFupComponentSide,
                                          &fw_info_p);

    if (status != FBE_STATUS_OK || fw_info_p == NULL)
    {
        // error - target not found
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sub_encl_id = fw_info_p->fwEsesComponentSubEnclId;
    generation_code = eses_enclosure_p->tunnel_fup_context_p.generation_code;

    // image page max size must be multiple of 4
    max_data_bytes = FBE_ESES_DL_CONTROL_MAX_TUNNEL_UCODE_DATA_LENGTH & 0x0ff8;

    // first sg item
    sgl_size = 1;

    if (eses_enclosure_p->enclCurrentFupInfo.enclFupOperation == FBE_ENCLOSURE_FIRMWARE_OP_DOWNLOAD &&
        eses_enclosure_p->enclCurrentFupInfo.enclFupUseTunnelling == TRUE)
    {
        /* We are going to have 3 sg_list elements
        * 1. header for the download code, 48 bytes,
        * 2. points to image itself, with calculated bytes.
        * 3. terminator.
        *
        * If the image itself is described through sg list,
        * we may need to shift things down.
        */
        // get the accumulated bytes downloaded for this image
        image_bytes_sent = eses_enclosure_p->enclCurrentFupInfo.enclFupBytesTransferred;

        // get the image size
        image_size = eses_enclosure_p->enclCurrentFupInfo.enclFupImageSize;

        if (image_size > image_bytes_sent)
        {
            image_p = (fbe_u8_t *) eses_enclosure_p->enclCurrentFupInfo.enclFupImagePointer;

            if (image_p == NULL)
            {
                // error
                return FBE_STATUS_GENERIC_FAILURE;
            }

            bytes_remaining = image_size - image_bytes_sent;
            // set the number of image bytes to transfer
            // this count will also be used in the control page field ucode_data_length
            if (bytes_remaining < max_data_bytes)
            {
                sgl_bytes = bytes_remaining;
            }
            else
            {
                sgl_bytes = max_data_bytes;
            }

            // build the sg list for this transfer
            fbe_eses_ucode_build_dl_sg_list(eses_enclosure_p, sg_p, &sgl_bytes, sg_count, FBE_ESES_DL_CONTROL_TUNNEL_UCODE_DATA_OFFSET);

            // next byte to copy/transfer
            image_p += image_bytes_sent;

            // sg_p points to the first item, which is the diag page data

            // copy the image item[1] into the sg list
            fbe_eses_enclosure_copy_data_to_sg(&sg_p[1], image_p);
        }

        download_mode = FBE_ESES_DL_CONTROL_MODE_DOWNLOAD;
    }  // end of adding image to sg_list for download
    else if (eses_enclosure_p->enclCurrentFupInfo.enclFupOperation == FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE &&
        eses_enclosure_p->enclCurrentFupInfo.enclFupUseTunnelling == TRUE)
    {
        download_mode = FBE_ESES_DL_CONTROL_MODE_ACTIVATE;
        /* We are going to have 2 sg_list elements
        * 1. header for the activate page.
        * 2. terminator.
        */

        // Set the data size and address for the send diag page in sg_p[0].
        // There will be one more item added to the list so the address starts
        // after the last item. Index 3 is sgl_size+1 which is the 
        // starting address for the data.
        fbe_sg_element_init(sg_p,
                            FBE_ESES_DL_CONTROL_TUNNEL_UCODE_DATA_OFFSET,
                            &sg_p[sgl_size+1]);

        // set the termination element
        fbe_sg_element_terminate(&sg_p[sgl_size]);
    }
    else
    {
        // let's do download but not sending any image out.
        download_mode = FBE_ESES_DL_CONTROL_MODE_DOWNLOAD;

        // Set the data size and address for the send diag page in sg_p[0].
        // There will be one more item added to the list so the address starts
        // after the last item. Index 3 is sgl_size+1 which is the 
        // starting address for the data.
        fbe_sg_element_init(sg_p,
                            FBE_ESES_DL_CONTROL_TUNNEL_UCODE_DATA_OFFSET,
                            &sg_p[sgl_size+1]);

        // set the termination element
        fbe_sg_element_terminate(&sg_p[sgl_size]);
    }

     // Set bytes 0-45, up to ucode data. The output, parameter_length_p, is the length of
     // the entire tunnel download control page.
    fbe_eses_fill_tunnel_download_control_page(&sg_p[0],
                                              image_size,
                                              sgl_bytes,
                                              download_mode,
                                              sub_encl_id,
                                              generation_code,
                                              parameter_length_p,
                                              image_bytes_sent);

    eses_enclosure_p->enclCurrentFupInfo.enclFupCurrTransferSize = sgl_bytes;
    if (*parameter_length_p != (sgl_bytes+FBE_ESES_DL_CONTROL_TUNNEL_UCODE_DATA_OFFSET))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                                "%s, bad pagesize exp:0x%x act:0x%x\n",
                                __FUNCTION__, *parameter_length_p, (sgl_bytes+FBE_ESES_DL_CONTROL_UCODE_DATA_OFFSET));

    }
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                            "%s, pagesize:0x%x pagelen:0x%x datalen:0x%x sent:%d\n",
                            __FUNCTION__,
                            *parameter_length_p,
                            (sgl_bytes+FBE_ESES_DL_CONTROL_UCODE_DATA_LENGTH_OFFSET-FBE_ESES_PAGE_LENGTH_OFFSET),
                            sgl_bytes, sgl_bytes+image_bytes_sent);


    return FBE_STATUS_OK;
} // fbe_eses_ucode_build_tunnel_download_or_activate

/**************************************************************************
* @fn fbe_eses_fill_tunnel_download_control_page()
***************************************************************************
* DESCRIPTION
*   @brief
*   Build the tunnel download control page up to the image data bytes. The
*   image data is pointed to from the sg list.
*
* PARAMETERS
*   @param fbe_sg_element_t - The pointer to the scatter gather list.
*
* RETURN VALUES
*   @return fbe_u32_t number of image bytes being sent
*
* HISTORY
*   24-Feb-2011 Kenny Huang - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_fill_tunnel_download_control_page(fbe_sg_element_t *sg_p,
                                           fbe_u32_t image_size,
                                           fbe_u32_t ucode_data_bytes,
                                           fbe_u8_t mode,
                                           fbe_u8_t subencl_id,
                                           fbe_u32_t  generation_code,
                                           fbe_u16_t *parameter_length,
                                           fbe_u32_t  image_bytes_sent)
{
    fbe_status_t status = FBE_STATUS_OK; 
    fbe_u16_t page_length;
    fbe_u16_t data_page_length;
    fbe_u16_t data_length;
    fbe_eses_tunnel_diag_ctrl_page_header_t *page_p;

    // page_length = image_data_bytes + offset to the image data - number of byte to page length 
    // set the total page size
    *parameter_length =  ucode_data_bytes + (fbe_u16_t)sizeof(fbe_eses_tunnel_diag_ctrl_page_header_t);
    page_length = *parameter_length - FBE_ESES_PAGE_SIZE_ADJUST;
    data_length = ucode_data_bytes + (fbe_u16_t)sizeof(fbe_eses_download_control_page_header_t);

    // the buffer has been zeroed, so only fill the non-zero fields

    // start of the page
    page_p = (fbe_eses_tunnel_diag_ctrl_page_header_t *)sg_p->address;
    page_p->page_code = SES_PG_CODE_TUNNEL_DIAG_CTRL;
    page_p->prot = FBE_ESES_TUNNEL_PROT_SCSI;
    page_p->rqst_type = FBE_ESES_TUNNEL_RQST_TYPE_LOCAL_PEER;
    page_p->pg_len_msb = (page_length >> 8) & 0xFF;
    page_p->pg_len_lsb = page_length  & 0xFF;
    page_p->cmd_len_msb = (sizeof(fbe_eses_send_diagnostic_cdb_t) >> 8) & 0xFF;
    page_p->cmd_len_lsb = sizeof(fbe_eses_send_diagnostic_cdb_t) & 0xFF;
    page_p->data_len_msb = (data_length >> 8) & 0xFF;
    page_p->data_len_lsb = data_length & 0xFF;

    // Set the command field
    status = fbe_eses_build_send_diagnostic_cdb((fbe_u8_t *)(&(page_p->cmd)), FBE_SCSI_SEND_DIAGNOSTIC_CDB_SIZE, data_length);
    if (FBE_STATUS_OK != status)
    {
        KvTracex(TRC_K_STD, "FBE: %s build cdb error.\n", __FUNCTION__);
        return status;
    }

    // The embedded data page_lengh field
    data_page_length = data_length - FBE_ESES_PAGE_SIZE_ADJUST;
    // Set the data header
    page_p->data.page_code = SES_PG_CODE_DOWNLOAD_MICROCODE_CTRL;
    page_p->data.subenclosure_id = subencl_id;
    page_p->data.page_length_msb = (data_page_length >> 8) & 0xFF;
    page_p->data.page_length_lsb = data_page_length & 0xFF;
    page_p->data.config_gen_code = swap32(generation_code);
    page_p->data.mode = mode;
    page_p->data.buffer_offset = swap32(image_bytes_sent);
    page_p->data.image_length = swap32(image_size);
    page_p->data.transfer_length = swap32(ucode_data_bytes);

    return status;
} // fbe_eses_fill_tunnel_download_control_page

/**************************************************************************
* @fn fbe_eses_ucode_build_tunnel_receive_diag_results_cdb()
***************************************************************************
* DESCRIPTION
*   @brief
*   Tunnel the receive diagnostic resultes cdb.
*
*
* PARAMETERS
*   @param eses_enclosure_p - The pointer to the eses enclosure
*   @param page_code - page code in the cdb 
*   @param fbe_sg_element_t - The pointer to the scatter gather list.
*   @param parameter_length_p    - returns the paramenter length, 
*                           to be reported in the cdb
*   @param sg_count              - returns sg_count.  This count includes 
*                           the last termination element.
*
* RETURN VALUES
*   @return fbe_status_t
*
* HISTORY
*   07-Mar-2011 Kenny Huang - Created.
***************************************************************************/
fbe_status_t
fbe_eses_ucode_build_tunnel_receive_diag_results_cdb(fbe_eses_enclosure_t * eses_enclosure_p,
                                                     ses_pg_code_enum page_code,
                                                     fbe_sg_element_t *sg_p,
                                                     fbe_u16_t *parameter_length_p,
                                                     fbe_u32_t *sg_count)
{
    fbe_status_t status;
    fbe_u32_t sgl_size;
    fbe_u16_t page_length; 
    fbe_eses_tunnel_diag_status_page_header_t* page_p;

    // We are going to have 2 sg_list elements
    //     1. header for the activate page.
    //     2. terminator.
    *sg_count = 2;
    sgl_size = 1;

    // Set the data size and address for the send diag page in sg_p[0].
    // There will be one more item added to the list so the address starts
    // after the last item. Index 3 is sgl_size+1 which is the 
    // starting address for the data.
    fbe_sg_element_init(sg_p, sizeof(fbe_eses_tunnel_diag_status_page_header_t), &sg_p[sgl_size+1]);

    // set the termination element
    fbe_sg_element_terminate(&sg_p[sgl_size]);

    // Set bytes 0-21. The buffer has been zeroed, so only fill the non-zero fields
    *parameter_length_p = sizeof(fbe_eses_tunnel_diag_status_page_header_t);
    page_length = *parameter_length_p - FBE_ESES_PAGE_SIZE_ADJUST; 

    // start of the page
    page_p = (fbe_eses_tunnel_diag_status_page_header_t *)(&sg_p[0])->address;
    page_p->page_code = SES_PG_CODE_TUNNEL_DIAG_STATUS;
    page_p->prot = FBE_ESES_TUNNEL_PROT_SCSI;
    page_p->rqst_type = FBE_ESES_TUNNEL_RQST_TYPE_LOCAL_PEER;
    page_p->pg_len_msb = (page_length >> 8) & 0xFF;
    page_p->pg_len_lsb = page_length  & 0xFF;
    page_p->cmd_len_msb = (sizeof(fbe_eses_receive_diagnostic_results_cdb_t) >> 8) & 0xFF;
    page_p->cmd_len_lsb = sizeof(fbe_eses_receive_diagnostic_results_cdb_t) & 0xFF;
    page_p->data_len_msb = 0;
    page_p->data_len_lsb = 0;

    // Set the command field
    status = fbe_eses_build_receive_diagnostic_results_cdb((fbe_u8_t *)&((page_p->cmd)), 
                 FBE_SCSI_RECEIVE_DIAGNOSTIC_RESULTS_CDB_SIZE, // size of CDB
                 FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE, // the maximum number of bytes to be returned.
                 page_code);

    return status;
} // fbe_eses_ucode_build_tunnel_receive_diag_results_cdb

/*!*************************************************************************
* @fn fbe_eses_enclosure_array_dev_slot_setup_control()                  
***************************************************************************
*
* DESCRIPTION
*       @brief
*       This function will format the Array Device Slot Control for the 
*       specified component.
*
* PARAMETERS
*       @param enclosure (INPUT)- The pointer to the enclosure.
*       @param generalDataPtr (INPUT) - handle to Drive component data
*       @param compType (INPUT) - EDAL component type 
*       @param ctrl_data (INPUT) - pointer to Control data
*
* RETURN VALUES
*      @return fbe_status_t FBE_STATUS_OK - the contol page was formatted without error.
*
* NOTES
*
* HISTORY
*  10-Nov-2008 Joe Perry - Created.
*
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_array_dev_slot_setup_control(void * enclosure,
                                                fbe_u32_t index,
                                                fbe_u32_t compType,
                                                fbe_u8_t *ctrl_data)
{   
    ses_ctrl_elem_array_dev_slot_struct     *deviceSlotControlPtr;
    fbe_bool_t                              turnOnFaultLed, markComponent, deviceOff;
    fbe_bool_t                              userRequest, devicePowerStatus;
    fbe_enclosure_status_t                  enclStatus;

    fbe_base_object_customizable_trace((fbe_base_object_t*)enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)enclosure),
                            "%s entry \n", __FUNCTION__ );  

    // set pointer to control structure & clear it
    deviceSlotControlPtr = (ses_ctrl_elem_array_dev_slot_struct *)ctrl_data;
    fbe_zero_memory(deviceSlotControlPtr, sizeof(ses_ctrl_elem_array_dev_slot_struct));

    /*
     *  Fill in the Device Slot info
     */
    deviceSlotControlPtr->cmn_ctrl.select = 1;      // must set Select

    // set the Fault LED control
    enclStatus = fbe_base_enclosure_edal_getBool(enclosure,
                                                 FBE_ENCL_COMP_TURN_ON_FAULT_LED,
                                                 FBE_ENCL_DRIVE_SLOT,
                                                 index,
                                                 &turnOnFaultLed);
    if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
    {
        deviceSlotControlPtr->rqst_fault = turnOnFaultLed;
    }
    else
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    // set the Mark/Identify control
    enclStatus = fbe_base_enclosure_edal_getBool(enclosure,
                                                 FBE_ENCL_COMP_MARK_COMPONENT,
                                                 FBE_ENCL_DRIVE_SLOT,
                                                 index,
                                                 &markComponent);
    if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
    {
        deviceSlotControlPtr->rqst_ident = markComponent;
    }
    else
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    // set the Device Off control
    enclStatus = fbe_base_enclosure_edal_getBool(enclosure,
                                                 FBE_ENCL_DRIVE_USER_REQ_POWER_CNTL,
                                                 FBE_ENCL_DRIVE_SLOT,
                                                 index,
                                                 &userRequest);
    if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
    {
        /* We will take the user requested value only if this is the new request
         * from user, otherwise, we will use the status value.
         */
        if (userRequest == TRUE)
        {
            enclStatus = fbe_base_enclosure_edal_getBool(enclosure,
                                                 FBE_ENCL_DRIVE_DEVICE_OFF,
                                                 FBE_ENCL_DRIVE_SLOT,
                                                 index,
                                                 &deviceOff);

            if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
            {
                deviceSlotControlPtr->dev_off = deviceOff;
            }
            else
            {
                return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
            }
        }
        else
        {
            enclStatus = fbe_base_enclosure_edal_getBool(enclosure,
                                                 FBE_ENCL_COMP_POWERED_OFF,
                                                 FBE_ENCL_DRIVE_SLOT,
                                                 index,
                                                 &devicePowerStatus);

            if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
            {
                deviceSlotControlPtr->dev_off = devicePowerStatus;
            }
            else
            {
                return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
            }
        }
    }
    else
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    return enclStatus; 

}   // end of fbe_eses_enclosure_array_dev_slot_setup_control


/*!*************************************************************************
* @fn fbe_eses_enclosure_exp_phy_setup_control()                  
***************************************************************************
*
* DESCRIPTION
*       @brief
*       This function will format the Expander PHY Control for the 
*       specified component.
*
* PARAMETERS
*       @param enclosure (INPUT)- The pointer to the enclosure.
*       @param generalDataPtr (INPUT) - handle to PHY component data
*       @param compType (INPUT) - EDAL component type 
*       @param ctrl_data (INPUT) - pointer to Control data
*
* RETURN VALUES
*      @return fbe_status_t FBE_STATUS_OK - the contol page was formatted without error.
*
* NOTES
*
* HISTORY
*  10-Nov-2008 Joe Perry - Created.
*
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_exp_phy_setup_control(void * enclosure,
                                         fbe_u32_t index,
                                         fbe_u32_t compType,
                                         fbe_u8_t *ctrl_data)
{   
    ses_ctrl_elem_exp_phy_struct    *phyControlPtr;
    fbe_bool_t                      disablePhy;
    fbe_enclosure_status_t          enclStatus;

    fbe_base_object_customizable_trace((fbe_base_object_t*)enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)enclosure),
                            "%s entry \n", __FUNCTION__);  

    // set pointer to control structure & clear it
    phyControlPtr = (ses_ctrl_elem_exp_phy_struct *)ctrl_data;
    fbe_zero_memory(phyControlPtr, sizeof(ses_ctrl_elem_exp_phy_struct));

    /*
     * Fill in PHY info
     */
    phyControlPtr->cmn_stat.select = 1;         // must set Select

    // set PHY Disable control
    enclStatus = fbe_base_enclosure_edal_getBool(enclosure,
                                                 FBE_ENCL_EXP_PHY_DISABLE,
                                                 FBE_ENCL_EXPANDER_PHY,
                                                 index,
                                                 &disablePhy);
    if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
    {
        phyControlPtr->cmn_stat.disable = disablePhy;
    }
    else
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    return enclStatus; 

}   // end of fbe_eses_enclosure_exp_phy_setup_control


fbe_enclosure_status_t 
fbe_eses_enclosure_temp_sensor_setup_control(void * enclosure,
                                             fbe_u32_t index,
                                             fbe_u32_t compType,
                                             fbe_u8_t *ctrl_data)
{   

    fbe_base_object_customizable_trace((fbe_base_object_t*)enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)enclosure),
                            "%s entry \n", __FUNCTION__);  
    
    /*TODO*/
    return FBE_ENCLOSURE_STATUS_OK; 
}


/*!*************************************************************************
* @fn fbe_eses_enclosure_exp_phy_setup_control()                  
***************************************************************************
*
* DESCRIPTION
*       @brief
*       This function will format the Mezzanine Connector LED
*       Control for the specified component.
*
* PARAMETERS
*       @param enclosure (INPUT)- The pointer to the enclosure.
*       @param generalDataPtr (INPUT) - handle to Mez Connnector
*       LED component data
*       @param compType (INPUT) - EDAL component type
*       @param ctrl_data (INPUT) - pointer to Control data
*
* RETURN VALUES
*      @return fbe_status_t FBE_STATUS_OK - the contol page was formatted without error.
*
* NOTES
*
* HISTORY
*  4-Jun-2010 D. McFarland - Created.
*
***************************************************************************/

fbe_enclosure_status_t 
fbe_eses_enclosure_connector_setup_control(void * enclosure,
                                           fbe_u32_t index,
                                           fbe_u32_t compType,
                                           fbe_u8_t *ctrl_data)
{   

    fbe_enclosure_status_t          enclStatus;
    ses_ctrl_elem_sas_conn_struct   *conn_cntrl_struct = NULL ;
    fbe_bool_t                     swapCtrl;
    fbe_bool_t                     faultLedCtrl;
    fbe_bool_t                     markLedCtrl;



    fbe_base_object_customizable_trace((fbe_base_object_t*)enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)enclosure),
                            "%s entry \n", __FUNCTION__);  
    
   // set pointer to control structure & clear it 
   conn_cntrl_struct = (ses_ctrl_elem_sas_conn_struct *)ctrl_data;
   fbe_zero_memory(conn_cntrl_struct, sizeof( ses_ctrl_elem_sas_conn_struct));

   conn_cntrl_struct->cmn_stat.select = 1;
 
   enclStatus = fbe_base_enclosure_edal_getBool(enclosure,
                                                 FBE_ENCL_COMP_CLEAR_SWAP,
                                                 FBE_ENCL_CONNECTOR,
                                                 index,
                                                 &swapCtrl);

    if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
    {
        conn_cntrl_struct->cmn_stat.rst_swap = swapCtrl;
    }
    else
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }


    // Copy over the fault LED control:
    enclStatus = fbe_base_enclosure_edal_getBool(enclosure,
                                                  FBE_ENCL_COMP_TURN_ON_FAULT_LED,
                                                  FBE_ENCL_CONNECTOR,
                                                  index,
                                                  &faultLedCtrl);

    if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
    {
     conn_cntrl_struct->rqst_fail = faultLedCtrl;
    }
    else
    {
     return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }
                                
    // Now copy over the mark LED control:
    enclStatus = fbe_base_enclosure_edal_getBool(enclosure,
                                               FBE_ENCL_COMP_MARK_COMPONENT,
                                               FBE_ENCL_CONNECTOR,
                                               index,
                                               &markLedCtrl);

    if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
    {
      conn_cntrl_struct->rqst_ident = markLedCtrl;
    }
    else
    {
      return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }
                                        
    return enclStatus; 
}

// JAP - PS Margining Test Control needs more work
fbe_enclosure_status_t 
fbe_eses_enclosure_ps_setup_control(void * enclosure,
                                    fbe_u32_t index,
                                    fbe_u32_t compType,
                                    fbe_u8_t *ctrl_data)
{   
    fbe_enclosure_status_t          enclStatus;
    ses_ps_info_elem_struct         *ps_cntrl_struct = NULL ;
    fbe_u8_t                        psMarginTestMode;

    fbe_base_object_customizable_trace((fbe_base_object_t*)enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)enclosure),
                            "%s entry \n", __FUNCTION__);  
    
   // set pointer to control structure & clear it 
   ps_cntrl_struct = (ses_ps_info_elem_struct *)ctrl_data;
   fbe_zero_memory(ps_cntrl_struct, sizeof( ses_ps_info_elem_struct));

//   ps_cntrl_struct->cmn_stat.select = 1;
 
   enclStatus = fbe_base_enclosure_edal_getU8(enclosure,
                                              FBE_ENCL_PS_MARGIN_TEST_MODE_CONTROL,
                                              FBE_ENCL_POWER_SUPPLY,
                                              index,
                                              &psMarginTestMode);

    if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
    {
        ps_cntrl_struct->margining_test_mode = psMarginTestMode;
    }
    else
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    return FBE_ENCLOSURE_STATUS_OK; 
}

fbe_enclosure_status_t 
fbe_eses_enclosure_cooling_setup_control(void * enclosure,
                                         fbe_u32_t index,
                                         fbe_u32_t compType,
                                         fbe_u8_t *ctrl_data)
{   

    fbe_base_object_customizable_trace((fbe_base_object_t*)enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)enclosure),
                            "%s entry \n",__FUNCTION__);  

    /*TODO*/
    return FBE_ENCLOSURE_STATUS_OK; 
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_display_setup_control()                  
***************************************************************************
*
* DESCRIPTION
*       @brief
*       This function will format the Display Control for the 
*       specified component.
*
* PARAMETERS
*       @param enclosure (INPUT)- The pointer to the enclosure.
*       @param generalDataPtr (INPUT) - handle to PHY component data
*       @param compType (INPUT) - EDAL component type 
*       @param ctrl_data (INPUT) - pointer to Control data
*
* RETURN VALUES
*      @return fbe_status_t FBE_STATUS_OK - the contol page was formatted without error.
*
* NOTES
*
* HISTORY
*  19-Dec-2008 Joe Perry - Created.
*
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_display_setup_control(void * enclosure,
                                         fbe_u32_t index,
                                         fbe_u32_t compType,
                                         fbe_u8_t *ctrl_data)
{   
    ses_ctrl_elem_display_struct   *displayControlPtr;
    fbe_u8_t                        displayMode, displayChar;
    fbe_bool_t                      displayMarked;
    fbe_enclosure_status_t          enclStatus;

    fbe_base_object_customizable_trace((fbe_base_object_t*)enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)enclosure),
                            "%s entry\n", __FUNCTION__);  


    // set pointer to control structure & clear it
    displayControlPtr = (ses_ctrl_elem_display_struct *)ctrl_data;
    fbe_zero_memory(displayControlPtr, sizeof(ses_ctrl_elem_display_struct));

    /*
     * Fill in Display info
     */
    displayControlPtr->cmn_ctrl.select = 1;         // must set Select

    // set Display Mode control
    enclStatus = fbe_base_enclosure_edal_getU8(enclosure,
                                               FBE_DISPLAY_MODE,
                                               FBE_ENCL_DISPLAY,
                                               index,
                                               &displayMode);

    if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
    {
        displayControlPtr->display_mode = displayMode;
    }
    else
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    // set Display Character control
    enclStatus = fbe_base_enclosure_edal_getU8(enclosure,
                                               FBE_DISPLAY_CHARACTER,
                                               FBE_ENCL_DISPLAY,
                                               index,
                                               &displayChar);
    // Temporary trace to track down DIMS 239447

    if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
    {
        if (displayChar == 0 || displayMode == 0)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)enclosure,
                                FBE_TRACE_LEVEL_ERROR,  
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)enclosure),
                                "%s DISPLAY_ERROR: setup_control Mode is 0x%x Display char is 0x%x\n", 
                                __FUNCTION__, displayMode, displayChar);
        }
        displayControlPtr->display_char = displayChar;
    }
    else
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    // set Display Marked
    enclStatus = fbe_base_enclosure_edal_getBool(enclosure,
                                                 FBE_ENCL_COMP_MARK_COMPONENT,
                                                 FBE_ENCL_DISPLAY,
                                                 index,
                                                 &displayMarked);
    if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
    {
        displayControlPtr->rqst_ident = displayMarked;
    }
    else
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    return enclStatus; 

}   // end of fbe_eses_enclosure_display_setup_control


/*!*************************************************************************
* @fn fbe_eses_enclosure_encl_setup_control()                  
***************************************************************************
*
* DESCRIPTION
*       @brief
*       This function will format the Enclosure Control for the 
*       specified component.
*
* PARAMETERS
*       @param enclosure (INPUT)- The pointer to the enclosure.
*       @param generalDataPtr (INPUT) - handle to PHY component data
*       @param compType (INPUT) - EDAL component type 
*       @param ctrl_data (INPUT) - pointer to Control data
*
* RETURN VALUES
*      @return fbe_status_t FBE_STATUS_OK - the contol page was formatted without error.
*
* NOTES
*
* HISTORY
*  10-Nov-2008 Joe Perry - Created.
*
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_encl_setup_control(void * enclosure,
                                      fbe_u32_t index,
                                      fbe_u32_t compType,
                                      fbe_u8_t *ctrl_data)
{   
    ses_ctrl_elem_encl_struct      *enclControlPtr;
    fbe_bool_t                      turnOnFaultLed, markComponent;
    fbe_u8_t                        powerCycleRequest;
    fbe_u8_t                        powerCycleDuration;
    fbe_u8_t                        powerCycleDelay;
    fbe_enclosure_status_t          enclStatus;



    fbe_base_object_customizable_trace((fbe_base_object_t*)enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)enclosure),
                           "%s entry, compType %s\n", 
                           __FUNCTION__, 
                           enclosure_access_printComponentType(compType));  

    // set pointer to control structure & clear it
    enclControlPtr = (ses_ctrl_elem_encl_struct *)ctrl_data;
    fbe_zero_memory(enclControlPtr, sizeof(ses_ctrl_elem_encl_struct));

    /*
     * Fill in Enclosure info
     */
    enclControlPtr->cmn_ctrl.select = 1;         // must set Select

    // set Failure Requested control
    enclStatus = fbe_base_enclosure_edal_getBool(enclosure,
                                                 FBE_ENCL_COMP_TURN_ON_FAULT_LED,
                                                 compType,
                                                 index,
                                                 &turnOnFaultLed);
    if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
    {
        enclControlPtr->rqst_failure = turnOnFaultLed;
    }
    else
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    // set Identify Requested control
    enclStatus = fbe_base_enclosure_edal_getBool(enclosure,
                                                 FBE_ENCL_COMP_MARK_COMPONENT,
                                                 compType,
                                                 index,
                                                 &markComponent);
    if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
    {
        enclControlPtr->rqst_ident = markComponent;
    }
    else
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    // set Power Cycle Request control
    enclStatus = fbe_base_enclosure_edal_getU8(enclosure,
                                               FBE_ENCL_LCC_POWER_CYCLE_REQUEST,
                                               compType,
                                               index,
                                               &powerCycleRequest);
    if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
    {
        enclControlPtr->power_cycle_rqst = powerCycleRequest;
        // clear Power Cycle setting now
        enclStatus = fbe_base_enclosure_edal_setU8(enclosure,
                                                   FBE_ENCL_LCC_POWER_CYCLE_REQUEST,
                                                   compType,
                                                   index,
                                                   FBE_ESES_ENCL_POWER_CYCLE_REQ_NONE);
    }
    else
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    // set Power Cycle Duration control
    enclStatus = fbe_base_enclosure_edal_getU8(enclosure,
                                               FBE_ENCL_LCC_POWER_CYCLE_DURATION,
                                               compType,
                                               index,
                                               &powerCycleDuration);
    if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
    {
        enclControlPtr->power_off_duration = powerCycleDuration;
    }
    else
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    // set Power Cycle Delay control
    enclStatus = fbe_base_enclosure_edal_getU8(enclosure,
                                               FBE_ENCL_LCC_POWER_CYCLE_DELAY,
                                               compType,
                                               index,
                                               &powerCycleDelay);
    if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
    {
        enclControlPtr->power_cycle_delay = powerCycleDelay;
    }
    else
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    return enclStatus; 

}   // end of fbe_eses_enclosure_encl_setup_control


/*!*************************************************************************
* @fn fbe_eses_enclosure_build_mode_param_list()                  
***************************************************************************
* @brief
*       This function builds the mode parameter list with the EMC ESES persistent mode page.
*
* @param     eses_enclosure (INPUT)- The pointer to the enclosure.
* @param     ctrl_elems (INPUT)- The control elems that needs to be built.
* @param     sg_list (INPUT)- The pointer to the sg list which contains the page.
* @param     page_size_ptr (OUTPUT) - The pointer to the page size(the size of the entire mode parameter list).
* @param     sg_count - The pointer to the sg count.
*
* @return  fbe_enclosure_status_t
*          Always returns FBE_ENCLOSURE_STATUS_OK.
*
* NOTES
*
* HISTORY
*   03-Dec-2008 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_build_mode_param_list(fbe_eses_enclosure_t * eses_enclosure,                                                      
                                                         fbe_sg_element_t * sg_list,
                                                         fbe_u16_t * page_size_ptr,
                                                         fbe_u32_t *sg_count)
{
    fbe_eses_mode_param_list_t * mode_param_list_p = NULL;
    fbe_eses_pg_eep_mode_pg_t * eep_mode_page_p = NULL;
    fbe_eses_pg_eenp_mode_pg_t * eenp_mode_page_p = NULL;

    /**********
     * BEGIN
     **********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry \n",__FUNCTION__);

    *sg_count = 2;
    /* Will set up sg_list[0].count at the end of the function. */
    sg_list[0].count = FBE_ESES_MODE_PARAM_LIST_HEADER_SIZE + FBE_ESES_EMC_ESES_PERSISTENT_MODE_PAGE_SIZE;
    sg_list[0].address = (fbe_u8_t *)&sg_list[0] + (*sg_count) * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    mode_param_list_p = (fbe_eses_mode_param_list_t *)sg_list[0].address;
    // Fill in the mode data list header.
    mode_param_list_p->medium_type = 0;
    mode_param_list_p->dev_specific_param = 0;
    mode_param_list_p->block_desc_len = 0;

    *page_size_ptr = FBE_ESES_MODE_PARAM_LIST_HEADER_SIZE;    

    if(eses_enclosure->mode_select_needed & FBE_ESES_MODE_SELECT_EEP_MODE_PAGE)
    {
        eep_mode_page_p = (fbe_eses_pg_eep_mode_pg_t *)((fbe_u8_t *)mode_param_list_p + *page_size_ptr);
        
        // Build EMC ESES persistent mode page.
        fbe_eses_enclosure_build_eep_mode_page(eses_enclosure, eep_mode_page_p);        

        *page_size_ptr = *page_size_ptr + FBE_ESES_EMC_ESES_PERSISTENT_MODE_PAGE_SIZE;
    }

    if(eses_enclosure->mode_select_needed & FBE_ESES_MODE_SELECT_EENP_MODE_PAGE)
    {
        eenp_mode_page_p = (fbe_eses_pg_eenp_mode_pg_t *)((fbe_u8_t *)mode_param_list_p + *page_size_ptr);

        // Build EMC ESES non persistent mode page.
        fbe_eses_enclosure_build_eenp_mode_page(eses_enclosure, eenp_mode_page_p);
     
        *page_size_ptr = *page_size_ptr + FBE_ESES_EMC_ESES_NON_PERSISTENT_MODE_PAGE_SIZE; 
    }

    sg_list[0].count = *page_size_ptr;

    return FBE_ENCLOSURE_STATUS_OK;
   
} // End of function fbe_eses_enclosure_build_mode_param_list

/*!*************************************************************************
* @fn fbe_eses_enclosure_build_eep_mode_page()                  
***************************************************************************
* @brief
*       This function builds the mode parameter list with the EMC ESES persistent (eep) mode page.
*
* @param     eses_enclosure (INPUT)- The pointer to the enclosure.
* @param     eep_mode_page_p (INPUT)- The pointer to the EMC ESES persistent (eep) mode page.
*
* @return   fbe_enclosure_status_t 
*     Always returns FBE_ENCLOSURE_STATUS_OK.
*
* NOTES
*
* HISTORY
*   03-Dec-2008 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t fbe_eses_enclosure_build_eep_mode_page(fbe_eses_enclosure_t * eses_enclosure, 
                                                       fbe_eses_pg_eep_mode_pg_t * eep_mode_page_p)                                                      
{
    /**********
     * BEGIN
     **********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__);

    // Fill in the EMC ESES persistent mode page.
    eep_mode_page_p->mode_pg_common_hdr.pg_code = SES_PG_CODE_EMC_ESES_PERSISTENT_MODE_PG;
    eep_mode_page_p->mode_pg_common_hdr.spf = 0; //Always 0 to indicate this is the Page_0 format. 
    eep_mode_page_p->mode_pg_common_hdr.pg_len = FBE_ESES_EMC_ESES_PERSISTENT_MODE_PAGE_LEN;
    //magnum, viper and derringer do not permit to change the bad_exp_recovery_enabled bit. Just set it to 0.
    eep_mode_page_p->bad_exp_recovery_enabled = 0; 
    eep_mode_page_p->disable_indicator_ctrl = FBE_ESES_ENCLOSURE_DISABLE_INDICATOR_CTRL_DEFAULT; // EMA does not set any indicator patterns except under client request. 
    eep_mode_page_p->ssu_disable = 0; // EMA controls spin-up of drives.
    eep_mode_page_p->ha_mode = FBE_ESES_ENCLOSURE_HA_MODE_DEFAULT; // high availability environment.
    
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry, HaMode %d, DisableIndCtrl %d\n", 
                            __FUNCTION__, 
                            eep_mode_page_p->ha_mode, eep_mode_page_p->disable_indicator_ctrl);

    return FBE_ENCLOSURE_STATUS_OK;
   
} // End of function fbe_eses_enclosure_build_eep_mode_page

/*!*************************************************************************
* @fn fbe_eses_enclosure_build_emc_eses_non_persistent_mode_page()                  
***************************************************************************
* @brief
*       This function builds the mode parameter list with the EMC ESES non persistent(eenp) mode page.
*
* @param     eses_enclosure (INPUT)- The pointer to the enclosure.
* @param     eenp_mode_page_p (INPUT)- The pointer to the EMC ESES non persistent (eenp) mode page.
*
* @return   fbe_enclosure_status_t 
*     Always returns FBE_ENCLOSURE_STATUS_OK.
*
* NOTES
*
* HISTORY
*   04-Dec-2008 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t fbe_eses_enclosure_build_eenp_mode_page(fbe_eses_enclosure_t * eses_enclosure, 
                               fbe_eses_pg_eenp_mode_pg_t * eenp_mode_page_p)
                                                      
{
    fbe_u8_t        spsCount = 0;

    /**********
     * BEGIN
     **********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__);

    // Fill in the EMC ESES persistent mode page.
    eenp_mode_page_p->mode_pg_common_hdr.pg_code = SES_PG_CODE_EMC_ESES_NON_PERSISTENT_MODE_PG;
    eenp_mode_page_p->mode_pg_common_hdr.spf = 0; //Always 0 to indicate this is the Page_0 format. 
    eenp_mode_page_p->mode_pg_common_hdr.pg_len = FBE_ESES_EMC_ESES_NON_PERSISTENT_MODE_PAGE_LEN;
    // The EMA does not alter the activity LED on the disk drive based on the
    // state of the Online pattern, leaving it set to a default behavior that is platform-dependent.
    eenp_mode_page_p->activity_led_ctrl = 0;
    // The LCC always powers up with the EMA in control of fan speeds. The EMA will not decrease
    // fan speed below a level it determines to be safe, regardless of host commands.
    eenp_mode_page_p->disable_auto_cooling_ctrl = 0;
    // The EMA may shut down part or all of the enclosure if it detects a problem.
    eenp_mode_page_p->disable_auto_shutdown = 0; 
    // Set SPS supported appropriately
    spsCount = fbe_eses_enclosure_get_number_of_sps(eses_enclosure);
    if (spsCount > 0)
    {
        eenp_mode_page_p->sps_dev_supported = 1;
    }
    else
    {
        eenp_mode_page_p->sps_dev_supported = 0;
    }

    // Set the test mode as requested.
    eenp_mode_page_p->test_mode = eses_enclosure->test_mode_rqstd; 

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                       FBE_TRACE_LEVEL_INFO,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                       "%s entry, TestMode %d, SpsDevSupp %d\n", 
                                       __FUNCTION__, eenp_mode_page_p->test_mode,
                                       eenp_mode_page_p->sps_dev_supported);

    return FBE_ENCLOSURE_STATUS_OK;
   
} // End of function fbe_eses_enclosure_build_eenp_mode_page


/*!*************************************************************************
*   @fn fbe_eses_enclosure_buildControlPage()                  
***************************************************************************
*
* DESCRIPTION
*       @brief
*       This function will generate control page.
*
* PARAMETERS
*       @param eses_enclosure - The pointer to the fbe_eses_enclosure_t.
*       @param ledAction - type of LED action to perform
*
* RETURN VALUES
*       @return fbe_status_t.
*
* NOTES
*
* HISTORY
*   25-Nov-2008     Joe Perry - Created.
*   18-Dec-2008     Naizhong Chiu - moved from usurper.c and renamed from
*                                   generateControlPage()
***************************************************************************/
fbe_status_t 
fbe_eses_enclosure_buildControlPage(fbe_eses_enclosure_t * eses_enclosure, 
                                    fbe_sg_element_t * sg_list,
                                    fbe_u16_t * page_size_ptr,
                                    fbe_u32_t *sg_count)
{
    fbe_enclosure_status_t                  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;   
    ses_pg_encl_ctrl_struct                 *emc_encl_ctrl_pg = NULL;
    fbe_u32_t                               index;
    fbe_bool_t                              writeDataFound;
    fbe_u8_t                                elemIndex;
    const fbe_eses_enclosure_comp_type_hdlr_t         *handler;
    fbe_u32_t                               componentOverallStatus;
    fbe_u8_t                                maxNumberOfComponents;
    fbe_u8_t                                *controlDataPtr;
    fbe_u16_t                               bufferOffset;
    fbe_enclosure_component_types_t         componentType;


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, outstanding write %d\n", 
                            __FUNCTION__,  eses_enclosure->outstanding_write);

    /*
     * Initialize some locals
     */
    *page_size_ptr = eses_enclosure->eses_status_control_page_length;

    *sg_count = 2;

    fbe_sg_element_init(&sg_list[0], 
                        eses_enclosure->eses_status_control_page_length,   // count
                       (fbe_u8_t *)&sg_list[0] + (*sg_count) * sizeof(fbe_sg_element_t)); // address

    fbe_sg_element_init(&sg_list[1], 
                       0,   // count
                       NULL); // address  

    emc_encl_ctrl_pg = (ses_pg_encl_ctrl_struct *)(sg_list->address);

    emc_encl_ctrl_pg->pg_code = SES_PG_CODE_ENCL_CTRL;
    //fbe_eses_set_pg_gen_code takes care of the byte swap.
    fbe_eses_set_pg_gen_code((ses_common_pg_hdr_struct *)emc_encl_ctrl_pg, 
                                eses_enclosure->eses_generation_code);
    //fbe_eses_set_pg_len takes care of the byte swap.
    fbe_eses_set_pg_len((ses_common_pg_hdr_struct *)emc_encl_ctrl_pg, 
                        eses_enclosure->eses_status_control_page_length);

    /* looping through all types, edal will find out which block it's in */
    for (componentType = 0; componentType < FBE_ESES_ENCLOSURE_MAX_COMPONENT_TYPE; componentType++)
    {
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
                // check if this component has Write/Control data
                encl_status = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_WRITE_DATA,
                                                            componentType,
                                                            index,
                                                            &writeDataFound);
                if ((encl_status == FBE_ENCLOSURE_STATUS_OK) && writeDataFound)
                {
                    // Get the element index for the specified component
                    encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_ENCL_COMP_ELEM_INDEX,
                                                                componentType,
                                                                index,
                                                                &elemIndex);
                    if (encl_status == FBE_ENCLOSURE_STATUS_OK)
                    {
                        handler = fbe_eses_enclosure_get_comp_type_hdlr(fbe_eses_enclosure_comp_type_hdlr_table, 
                                                                   componentType);
                        // It could be possible that we did not implement the setup control method
                        // for the particular element type. It shoud not be considered as a fault.
                        if((handler != NULL) &&(handler->setup_control != NULL))
                        {
                            // format ESES Control pages (call the correct handler)
                            bufferOffset = fbe_eses_elem_index_to_byte_offset(eses_enclosure, elemIndex);
                            controlDataPtr = (fbe_u8_t *)sg_list->address + bufferOffset;
                            /* We may need to set up overall control */
                            encl_status = handler->setup_control(eses_enclosure, 
                                                                index,
                                                                componentType,
                                                                controlDataPtr);
                            if (encl_status != FBE_ENCLOSURE_STATUS_OK)
                            {
                                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                        FBE_TRACE_LEVEL_ERROR,
                                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                        "%s, setupControlHandler failed, encl_status: 0x%x.\n",
                                                        __FUNCTION__, encl_status);
                                
                                continue;
                            }
                            // mark being sent
                            encl_status = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                            FBE_ENCL_COMP_WRITE_DATA_SENT,
                                                                            componentType,
                                                                            index,
                                                                            writeDataFound);

                            if (encl_status != FBE_ENCLOSURE_STATUS_OK)
                            {
                                break;
                            }
                        }
                        else
                        {
                            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                    FBE_TRACE_LEVEL_ERROR,
                                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                    "%s, handler pointer NULL\n",
                                                    __FUNCTION__ );
                            
                            encl_status = FBE_ENCLOSURE_STATUS_ENCL_FUNC_UNSUPPORTED;
                            continue;
                        }

                    }
                    else
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s,  can't get compElemIndex, enclStatus: 0x%x.\n",
                                    __FUNCTION__,  encl_status);
                    }

                }

            }   // for index

        }

    }

    if (encl_status == FBE_ENCLOSURE_STATUS_OK)
    {
        status = FBE_STATUS_OK;
    }
    else
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}   // end of fbe_eses_enclosure_buildControlPage


/*!*************************************************************************
*   @fn fbe_eses_enclosure_buildStringOutPage()                  
***************************************************************************
*
* DESCRIPTION
*       @brief
*       This function will generate a String Out page.
*
* PARAMETERS
*       @param eses_enclosure - The pointer to the fbe_eses_enclosure_t.
*       @param ledAction - type of LED action to perform
*
* RETURN VALUES
*       @return fbe_status_t.
*
* NOTES
*
* HISTORY
*   20-Feb-2009     Joe Perry - Created.
***************************************************************************/
fbe_status_t 
fbe_eses_enclosure_buildStringOutPage(fbe_eses_enclosure_t * eses_enclosure, 
                                      fbe_enclosure_mgmt_ctrl_op_t *eses_page_op,
                                      fbe_u16_t *parameter_length,
                                      fbe_sg_element_t * sg_list,
                                      fbe_u32_t *sg_count)
{
    fbe_eses_pg_str_out_t   *str_out_pg = NULL;
    fbe_u8_t                *str_out_data = NULL;
    fbe_u16_t               pageLength;

    *sg_count = 2;
    fbe_sg_element_init(&sg_list[0], 
                        eses_enclosure->eses_status_control_page_length,   // count
                        (fbe_u8_t *)&sg_list[0] + (*sg_count) * sizeof(fbe_sg_element_t)); // address
    fbe_sg_element_init(&sg_list[1], 
                        0,   // count
                        NULL); // address  

    str_out_pg = (fbe_eses_pg_str_out_t *)sg_list[0].address;
    str_out_pg->page_code = SES_PG_CODE_STR_OUT;
    str_out_data = &str_out_pg->str_out_data[0];
    fbe_copy_memory(str_out_data,
                    &(eses_page_op->cmd_buf.stringOutInfo),
                    FBE_BASE_EXP_STRING_OUT_CMD_MAX_LENGTH);

    str_out_pg->echo = 1;  //Set to 1, causes all commands in string out data to be echoed into active trace buffer(and hence returned 
                           // in the string in diagnostic page), as they are processed. 

    // Add 1 byte for the fourth byte in the string out page which includes the echo field.
    // It does not include the bytes before the page length bytes and the page length bytes.
    pageLength = (fbe_u16_t)strlen((char *)&(eses_page_op->cmd_buf.stringOutInfo)) + 1;
    *parameter_length = FBE_ESES_STR_OUT_PAGE_HEADER_SIZE + pageLength;
    fbe_eses_set_pg_len((ses_common_pg_hdr_struct *)str_out_pg, *parameter_length);
    sg_list[0].count = *parameter_length;

    return FBE_STATUS_OK;

}   // end of fbe_eses_enclosure_buildStringOutPage

/*!*************************************************************************
*   @fn fbe_eses_enclosure_buildThresholdOutPage()                 
***************************************************************************
*
* DESCRIPTION
*       @brief
*       This function will generate a Threshold Out page.
*
* PARAMETERS
*       @param eses_enclosure - The pointer to the fbe_eses_enclosure_t.
*       @param eses_page_op (INPUT)- The control elems that needs to be built.
*       @param sg_list (INPUT)- The pointer to the sg list which contains the page.
*       @param page_size_ptr (OUTPUT) - The pointer to the page size(the size of the entire mode parameter list).
*       @param sg_count - The pointer to the sg count.
*
* RETURN VALUES
*       @return fbe_status_t.
*
* NOTES
*
* HISTORY
*   09-Nov-2009     Dipak Patel - Created.
***************************************************************************/
fbe_status_t 
fbe_eses_enclosure_buildThresholdOutPage(fbe_eses_enclosure_t * eses_enclosure, 
                                         fbe_enclosure_mgmt_ctrl_op_t *eses_page_op,
                                         fbe_u16_t *page_size_ptr,
                                         fbe_sg_element_t * sg_list,
                                         fbe_u32_t *sg_count)
{

    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_enclosure_status_t                  encl_status = FBE_ENCLOSURE_STATUS_OK;    
    ses_common_pg_hdr_struct                *emc_encl_threshold_ctrl_pg = NULL;
    fbe_u8_t                                *controlDataPtr = NULL;
    fbe_enclosure_mgmt_exp_threshold_info_t *thresholdInfo = NULL;
    fbe_u8_t                                subEnclId = SES_SUBENCL_ID_NONE;
    fbe_u16_t                               byteOffset = 0;
    fbe_eses_elem_group_t *                 elem_group = NULL;
    fbe_u8_t                                group_id = 0;
    ses_elem_type_enum                      elem_type = SES_ELEM_TYPE_INVALID;  
    fbe_bool_t                              more_goups = TRUE;   
    fbe_u8_t                                side_id = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_enclosure_component_types_t         subencl_component_type = FBE_ENCL_INVALID_COMPONENT_TYPE;


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s \n", __FUNCTION__);

    /*
     * Initialize some locals
     */
    *page_size_ptr = eses_enclosure->eses_status_control_page_length;
    *sg_count = 2;

    fbe_sg_element_init(&sg_list[0], 
                        eses_enclosure->eses_status_control_page_length,   // count
                       (fbe_u8_t *)&sg_list[0] + (*sg_count) * sizeof(fbe_sg_element_t)); // address

    fbe_sg_element_init(&sg_list[1], 
                                0,     // count
                                NULL); // address  

    emc_encl_threshold_ctrl_pg = (ses_common_pg_hdr_struct *)(sg_list->address);

    emc_encl_threshold_ctrl_pg->pg_code = SES_PG_CODE_THRESHOLD_OUT;
    //fbe_eses_set_pg_gen_code takes care of the byte swap.
    fbe_eses_set_pg_gen_code((ses_common_pg_hdr_struct *)emc_encl_threshold_ctrl_pg, 
                                eses_enclosure->eses_generation_code);
    //fbe_eses_set_pg_len takes care of the byte swap.
    fbe_eses_set_pg_len((ses_common_pg_hdr_struct *)emc_encl_threshold_ctrl_pg, 
                        eses_enclosure->eses_status_control_page_length);

    controlDataPtr = (fbe_u8_t *)(sg_list[0].address);

    thresholdInfo = &(eses_page_op->cmd_buf.thresholdOutInfo);

    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);

    /* find element groups that contain the specified element type */
    while(more_goups)
    {
        // find element group for this element type
        // group_id specifies the starting group number for get_group_type
        group_id = fbe_eses_elem_group_get_group_type(eses_enclosure, thresholdInfo->elem_type, group_id);
        if (group_id == FBE_ENCLOSURE_VALUE_INVALID)
        {
            // no more groups of this type
            more_goups = FALSE;
            break;
        }

        /* Get the byte offset */
        byteOffset = fbe_eses_elem_group_get_group_byte_offset(elem_group, group_id);

        /* Get the element type of this element group. */
        elem_type = fbe_eses_elem_group_get_elem_type(elem_group, group_id);

        /* Get the subenclosure id of this element group. */
        subEnclId = fbe_eses_elem_group_get_subencl_id(elem_group, group_id);     
        
        /* Get the subenclosure component type for this element group. */
        encl_status = fbe_eses_enclosure_subencl_id_to_side_id_component_type(eses_enclosure,
                                                 subEnclId, 
                                                 &side_id,
                                                 &subencl_component_type);
        if(encl_status == FBE_ENCLOSURE_STATUS_OK)
        {
            if((thresholdInfo->elem_type == (fbe_enclosure_element_type_t)elem_type) && (subencl_component_type == FBE_ENCL_ENCLOSURE))
            {
                controlDataPtr  +=  byteOffset;
                fbe_copy_memory(controlDataPtr, &(thresholdInfo->exp_overall_info),
                                          sizeof(thresholdInfo->exp_overall_info));
                return FBE_STATUS_OK;
            }
        }
        // next group
        group_id++;
    } // while more_groups     
    return status;
}   // end of fbe_eses_enclosure_buildThresholdOutPage
// End of file fbe_eses_enclosure_build_pages.c

