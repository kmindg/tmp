/***************************************************************************
 *  terminator_encl_eses_plugin_config_page.c
 ***************************************************************************
 *
 *  Description
 *     This contains the funtions related to processing the EMC Enclosure 
 *      Status Diagnostic Page & EMC Enclosure control diagnostic page.(10h)
 *
 *  Note:
 *     *Change the name of this file to terminator_encl_eses_plugin_emc_page.c
 *      in the future as "EMC ctrl page" functions were also added to this.
 *      
 *
 *  History:
 *      Nov-13-08   Rajesh V. split emc page related functions to 
 *          seperate file.
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
/*        LOCAL DEFINES           */
/**********************************/

// For now we only support connector information
// elements and trace buffer information elements.
#define TERMINATOR_EMC_PAGE_NUM_INFO_ELEM_GROUPS        3
#define TERMINATOR_EMC_PAGE_NUM_TRACE_BUF_INFO_ELEMS    2

/**********************************/
/*        local functions         */
/**********************************/
static fbe_status_t emc_encl_stat_diag_page_build_sas_conn_inf_elems(fbe_u8_t *sas_conn_elem_start_ptr,
                                                                    fbe_u8_t **sas_conn_elem_end_ptr,
                                                                    fbe_terminator_device_ptr_t virtual_phy_handle,
                                                                    fbe_u8_t *num_sas_conn_info_elem);
static fbe_status_t emc_encl_stat_diag_page_build_trace_buffer_inf_elems(fbe_u8_t *trace_buffer_elem_start_ptr,
                                                                        fbe_u8_t **trace_buffer_elem_end_ptr,
                                                                        fbe_terminator_device_ptr_t virtual_phy_handle,
                                                                        fbe_u8_t *num_trace_buffer_info_elem);
static fbe_status_t emc_encl_stat_diag_page_build_general_info_expander_elems(fbe_u8_t *trace_buffer_elem_start_ptr,
                                                                        fbe_u8_t **trace_buffer_elem_end_ptr,
                                                                        fbe_terminator_device_ptr_t virtual_phy_handle,
                                                                        fbe_u8_t *num_general_info_elem);
fbe_status_t emc_encl_stat_diag_page_build_general_info_drive_slot_elems(fbe_u8_t *general_info_elem_start_ptr,
                                                             fbe_u8_t **general_info_elem_end_ptr,
                                                             fbe_terminator_device_ptr_t virtual_phy_handle,
                                                             fbe_u8_t *num_general_info_elem);
fbe_status_t emc_encl_stat_diag_page_build_ps_info_elems(fbe_u8_t *ps_info_elem_start_ptr,
                                                         fbe_u8_t **ps_info_elem_end_ptr,
                                                         fbe_terminator_device_ptr_t virtual_phy_handle,
                                                         fbe_u8_t *num_ps_info_elem);
static fbe_status_t emc_stat_page_sas_conn_info_elem_get_local_elem_index(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                                        fbe_u8_t position,
                                                                        fbe_u8_t *conn_elem_index);
static fbe_status_t emc_stat_page_sas_conn_info_elem_get_attach_sas_addr(fbe_u8_t position,
                                                                        fbe_u8_t conn_elem_index,
                                                                        fbe_u8_t *conn_id,
                                                                        fbe_u64_t *attached_sas_address,
                                                                        fbe_u8_t *sub_encl_id,
                                                                        fbe_terminator_device_ptr_t virtual_phy_handle);
static fbe_status_t emc_stat_page_sas_conn_info_elem_get_peer_elem_index(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                                        fbe_u8_t position,
                                                                        fbe_u8_t *conn_elem_index);
static fbe_status_t emc_encl_ctrl_page_process_sas_conn_inf_elems(fbe_u8_t *sas_conn_elems_start_ptr,
                                                        fbe_u8_t num_sas_conn_info_elems,
                                                        fbe_terminator_device_ptr_t virtual_phy_handle,
                                                        fbe_u8_t *sense_buffer);
static fbe_status_t emc_encl_ctrl_page_process_trace_buf_inf_elems(fbe_u8_t *trace_buf_inf_elems_start_ptr,
                                                        fbe_u8_t num_trace_buf_inf_elems,
                                                        fbe_terminator_device_ptr_t virtual_phy_handle,
                                                        fbe_u8_t *sense_buffer);
static fbe_status_t emc_encl_ctrl_page_process_general_inf_elems(fbe_u8_t *general_inf_elems_start_ptr,
                                                        fbe_u8_t num_general_inf_elems,
                                                        fbe_terminator_device_ptr_t virtual_phy_handle,
                                                        fbe_u8_t *sense_buffer);
/*********************************************************************
*           build_emc_enclosure_status_diagnostic_page ()
*********************************************************************
*
*  Description: This builds the emc enclosure status diagnostic page. 
*   Page 113 of spec.
*
*  Inputs: sg_list, enclosure information, virtual phy device id, 
*   port number.
*
*  Return Value: success or failure
*
*  History:
*    Aug08 created
*    
*********************************************************************/
fbe_status_t build_emc_enclosure_status_diagnostic_page(fbe_sg_element_t * sg_list, 
                                                        fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_pg_emc_encl_stat_struct *emc_encl_stat_diag_page_hdr;
    ses_pg_emc_encl_stat_struct terminatorEmcEnclStatus;
    fbe_u8_t *elements_start_ptr = NULL; 
    fbe_u8_t *elements_end_ptr = NULL;
    fbe_sas_enclosure_type_t  encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    fbe_eses_info_elem_group_hdr_t *info_elem_group_hdr = NULL;
    fbe_u8_t num_of_expander_elem=0;
    fbe_u8_t num_of_drive_elem=0;


    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);

    emc_encl_stat_diag_page_hdr = (ses_pg_emc_encl_stat_struct *)fbe_sg_element_address(sg_list);
    memset(emc_encl_stat_diag_page_hdr, 0, sizeof(ses_pg_emc_encl_stat_struct));
    // ignore other fields.
    emc_encl_stat_diag_page_hdr->hdr.pg_code = SES_PG_CODE_EMC_ENCL_STAT;
    // Gen code returned in bigEndian format.
    status = config_page_get_gen_code(virtual_phy_handle , 
                                      &emc_encl_stat_diag_page_hdr->hdr.gen_code);
    RETURN_ON_ERROR_STATUS;
    emc_encl_stat_diag_page_hdr->hdr.gen_code = BYTE_SWAP_32(emc_encl_stat_diag_page_hdr->hdr.gen_code);

    emc_encl_stat_diag_page_hdr->num_info_elem_groups = TERMINATOR_EMC_PAGE_NUM_INFO_ELEM_GROUPS;

    info_elem_group_hdr = (fbe_eses_info_elem_group_hdr_t *)(emc_encl_stat_diag_page_hdr + 1);
    info_elem_group_hdr->info_elem_type = FBE_ESES_INFO_ELEM_TYPE_SAS_CONN;
    info_elem_group_hdr->info_elem_size = FBE_ESES_EMC_CONTROL_SAS_CONN_INFO_ELEM_SIZE;
    elements_start_ptr = (fbe_u8_t *)(info_elem_group_hdr + 1);
    status = emc_encl_stat_diag_page_build_sas_conn_inf_elems(elements_start_ptr, 
                                                             &elements_end_ptr, 
                                                             virtual_phy_handle, 
                                                             &info_elem_group_hdr->num_info_elems);
    RETURN_ON_ERROR_STATUS;

    info_elem_group_hdr = (fbe_eses_info_elem_group_hdr_t *)(elements_end_ptr);
    info_elem_group_hdr->info_elem_type = FBE_ESES_INFO_ELEM_TYPE_TRACE_BUF;
    info_elem_group_hdr->info_elem_size = FBE_ESES_EMC_CONTROL_TRACE_BUF_INFO_ELEM_SIZE;
    elements_start_ptr = (fbe_u8_t *)(info_elem_group_hdr + 1);
    status = emc_encl_stat_diag_page_build_trace_buffer_inf_elems(elements_start_ptr, 
                                                                &elements_end_ptr, 
                                                                virtual_phy_handle, 
                                                                &info_elem_group_hdr->num_info_elems);   
    RETURN_ON_ERROR_STATUS;

    info_elem_group_hdr = (fbe_eses_info_elem_group_hdr_t *)(elements_end_ptr);
    info_elem_group_hdr->info_elem_type = FBE_ESES_INFO_ELEM_TYPE_GENERAL;
    info_elem_group_hdr->info_elem_size = FBE_ESES_EMC_CONTROL_GENERAL_INFO_ELEM_SIZE;
    elements_start_ptr = (fbe_u8_t *)(info_elem_group_hdr + 1);
    status = emc_encl_stat_diag_page_build_general_info_expander_elems(elements_start_ptr, 
                                                                &elements_end_ptr, 
                                                                virtual_phy_handle, 
                                                                &num_of_expander_elem);   
    RETURN_ON_ERROR_STATUS;
    info_elem_group_hdr->num_info_elems = num_of_expander_elem;

    status = emc_encl_stat_diag_page_build_general_info_drive_slot_elems(elements_end_ptr, 
                                                                &elements_end_ptr, 
                                                                virtual_phy_handle, 
                                                                &num_of_drive_elem);   
    RETURN_ON_ERROR_STATUS;
    info_elem_group_hdr->num_info_elems += num_of_drive_elem;

    info_elem_group_hdr = (fbe_eses_info_elem_group_hdr_t *)(elements_end_ptr);
    info_elem_group_hdr->info_elem_type = FBE_ESES_INFO_ELEM_TYPE_PS;
    info_elem_group_hdr->info_elem_size = FBE_ESES_EMC_CTRL_STAT_PS_INFO_ELEM_SIZE;
    elements_start_ptr = (fbe_u8_t *)(info_elem_group_hdr + 1);
    status = emc_encl_stat_diag_page_build_ps_info_elems(elements_start_ptr, 
                                                         &elements_end_ptr, 
                                                         virtual_phy_handle, 
                                                        &info_elem_group_hdr->num_info_elems);   
    RETURN_ON_ERROR_STATUS;

    emc_encl_stat_diag_page_hdr->hdr.pg_len = (fbe_u16_t)((elements_end_ptr - ((fbe_u8_t *)emc_encl_stat_diag_page_hdr)) - 4);
    // Always return page length in big endian format as actual expander firmware does that.
    emc_encl_stat_diag_page_hdr->hdr.pg_len = BYTE_SWAP_16(emc_encl_stat_diag_page_hdr->hdr.pg_len);

    status = terminator_get_emcEnclStatus(virtual_phy_handle, &terminatorEmcEnclStatus);
    RETURN_ON_ERROR_STATUS;
    emc_encl_stat_diag_page_hdr->shutdown_reason = terminatorEmcEnclStatus.shutdown_reason;

    return(status);     
}

/*********************************************************************
*            emc_encl_stat_diag_page_build_sas_conn_inf_elems ()
*********************************************************************
*
*  Description: This builds the SAS Connector INformation elements in
*    EMC enclosure status diagnostic page.
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of sas conn
*       information elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the sas inf. elements.
*   virtual phy object handle, number of sas connector inf. elements 
*       to be filled.
*
*  Return Value: success or failure
*
*  History:
*    Aug08 created
*    
*********************************************************************/

fbe_status_t emc_encl_stat_diag_page_build_sas_conn_inf_elems(fbe_u8_t *sas_conn_elem_start_ptr,
                                                             fbe_u8_t **sas_conn_elem_end_ptr,
                                                             fbe_terminator_device_ptr_t virtual_phy_handle,
                                                             fbe_u8_t *num_sas_conn_info_elem)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_sas_conn_info_elem_struct *sas_conn_inf_elem_ptr = NULL;    
    fbe_u8_t i;
    fbe_sas_enclosure_type_t    encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    fbe_u8_t num_sas_conn_info_elem_per_lcc = 0;
    terminator_sp_id_t spid;
    fbe_u8_t max_conns_per_port;
    terminator_conn_map_range_t return_range = CONN_IS_ILLEGAL; 

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);


    status = terminator_max_conns_per_lcc(encl_type, &num_sas_conn_info_elem_per_lcc);
    RETURN_ON_ERROR_STATUS;

    status = terminator_max_conns_per_port(encl_type, &max_conns_per_port);
    RETURN_ON_ERROR_STATUS;

    sas_conn_inf_elem_ptr = (ses_sas_conn_info_elem_struct *)(sas_conn_elem_start_ptr);
    for(i=0;i < num_sas_conn_info_elem_per_lcc; i++)
    {
        status = emc_stat_page_sas_conn_info_elem_get_local_elem_index(virtual_phy_handle, i, &sas_conn_inf_elem_ptr->conn_elem_index);
        RETURN_ON_ERROR_STATUS;
        status = emc_stat_page_sas_conn_info_elem_get_attach_sas_addr(  i, 
                                                                        sas_conn_inf_elem_ptr->conn_elem_index, 
                                                                        &sas_conn_inf_elem_ptr->conn_id,
                                                                        &sas_conn_inf_elem_ptr->attached_sas_address,
                                                                        &sas_conn_inf_elem_ptr->attached_sub_encl_id,
                                                                        virtual_phy_handle);
        RETURN_ON_ERROR_STATUS;
        // convert to big endian 
        sas_conn_inf_elem_ptr->attached_sas_address = 
            BYTE_SWAP_64(sas_conn_inf_elem_ptr->attached_sas_address);
        sas_conn_inf_elem_ptr++;
    }
    *num_sas_conn_info_elem += num_sas_conn_info_elem_per_lcc;
   status =  fbe_terminator_api_get_sp_id(&spid);

    if(config_page_element_exists(virtual_phy_handle,
                                SES_SUBENCL_TYPE_LCC,
                                !spid, 
                                SES_ELEM_TYPE_SAS_CONN, 
                                FALSE,
                                0,
                                FALSE,
                                NULL))
    {
                                  
                                  
    //Assume the peer LCC is not installed and fill its connectors
    // For now all peer lCC connector related info is hardcoded.

        for(i=0;i < num_sas_conn_info_elem_per_lcc; i++)
        {
            status = emc_stat_page_sas_conn_info_elem_get_peer_elem_index(virtual_phy_handle, i, &sas_conn_inf_elem_ptr->conn_elem_index);
            RETURN_ON_ERROR_STATUS;
            sas_conn_inf_elem_ptr->attached_sas_address = FBE_SAS_ADDRESS_INVALID;
            
            terminator_map_position_max_conns_to_range_conn_id(encl_type, 
                                                       i, 
                                                       max_conns_per_port, 
                                                       &return_range,   // Not used here.
                                                       &sas_conn_inf_elem_ptr->conn_id);

            sas_conn_inf_elem_ptr++;
        }
        *num_sas_conn_info_elem += num_sas_conn_info_elem_per_lcc;

    }

    *sas_conn_elem_end_ptr = (fbe_u8_t *)sas_conn_inf_elem_ptr; 
    

    
    status = FBE_STATUS_OK;
    return(status);          
}


/*********************************************************************
*            emc_encl_stat_diag_page_build_general_info_expander_elems ()
*********************************************************************
*
*  Description: This builds the General Information expander elements in
*    EMC enclosure status diagnostic page.
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of trace buffer
*       information elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the trace inf. elements.
*   virtual phy object handle, number of trace buffer inf. elements 
*       to be filled.
*
*  Return Value: success or failure
*  NOTES:
*   For now we fill local expander.
*
*  History:
*    Apr-13-2008 NCHIU Created.
*    
*********************************************************************/

fbe_status_t emc_encl_stat_diag_page_build_general_info_expander_elems(fbe_u8_t *general_info_elem_start_ptr,
                                                             fbe_u8_t **general_info_elem_end_ptr,
                                                             fbe_terminator_device_ptr_t virtual_phy_handle,
                                                             fbe_u8_t *num_general_info_elem)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_general_info_elem_expander_struct * general_info_elem_p = NULL;
    terminator_vp_eses_page_info_t *vp_eses_page_info;
    fbe_u8_t reset_reason  = 0;
     terminator_sp_id_t spid;

    status = terminator_get_eses_page_info(virtual_phy_handle, &vp_eses_page_info);
    RETURN_ON_ERROR_STATUS;

    *num_general_info_elem = 1;

    general_info_elem_p = (ses_general_info_elem_expander_struct *)(general_info_elem_start_ptr);
    memset(general_info_elem_p, 0, sizeof(ses_general_info_elem_expander_struct));

    general_info_elem_p->reset_reason =  reset_reason;
    status =  fbe_terminator_api_get_sp_id(&spid);

    status = config_page_get_start_elem_index_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_LCC,
                                                            spid, 
                                                            SES_ELEM_TYPE_SAS_EXP, 
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &general_info_elem_p->elem_index);
    RETURN_ON_ERROR_STATUS;


    *general_info_elem_end_ptr = (fbe_u8_t *)(general_info_elem_p+1); // next element

    return( FBE_STATUS_OK);          
}

/*********************************************************************
*            emc_encl_stat_diag_page_build_general_info_drive_slot_elems ()
*********************************************************************
*
*  Description: This builds the General Information drive slot  elements in
*    EMC enclosure status diagnostic page.
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of trace buffer
*       information elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the trace inf. elements.
*   virtual phy object handle, number of trace buffer inf. elements 
*       to be filled.
*
*  Return Value: success or failure
*
*  History:
*    Sep-02-2012 Rui Chang Created.
*    
*********************************************************************/

fbe_status_t emc_encl_stat_diag_page_build_general_info_drive_slot_elems(fbe_u8_t *general_info_elem_start_ptr,
                                                             fbe_u8_t **general_info_elem_end_ptr,
                                                             fbe_terminator_device_ptr_t virtual_phy_handle,
                                                             fbe_u8_t *num_general_info_elem)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_general_info_elem_array_dev_slot_struct * general_info_elem_p = NULL;
    ses_general_info_elem_array_dev_slot_struct  terminatorEmcGeneralInfoDriveSlotStatus = {0};
     terminator_sp_id_t spid;
    fbe_sas_enclosure_type_t    encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    fbe_u8_t  max_drive_slots=0;
    fbe_u32_t  driveIndex;
    fbe_u8_t  driveStartElemIndex=0;


    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    RETURN_ON_ERROR_STATUS;

    // DRIVE SLOTS
    status = terminator_max_drive_slots(encl_type, &max_drive_slots);
    RETURN_ON_ERROR_STATUS;

    *num_general_info_elem = max_drive_slots;

    status =  fbe_terminator_api_get_sp_id(&spid);
    RETURN_ON_ERROR_STATUS;

    status = config_page_get_start_elem_index_in_stat_page(virtual_phy_handle,
                                                           SES_SUBENCL_TYPE_LCC,
                                                           spid, 
                                                           SES_ELEM_TYPE_ARRAY_DEV_SLOT, 
                                                           FALSE,
                                                           0,
                                                           FALSE,
                                                           NULL,
                                                           &driveStartElemIndex);
    // some enclosures will not report  data, so return no fault
    if (status == FBE_STATUS_COMPONENT_NOT_FOUND)
    {
        return FBE_STATUS_OK;
    }
    RETURN_ON_ERROR_STATUS;

    for (driveIndex = 0; driveIndex < max_drive_slots; driveIndex++)
    {
        general_info_elem_p = (ses_general_info_elem_array_dev_slot_struct *)(general_info_elem_start_ptr) + driveIndex;
        memset(general_info_elem_p, 0, sizeof(ses_general_info_elem_array_dev_slot_struct));

        status = terminator_get_emcGeneralInfoDirveSlotStatus(virtual_phy_handle, &terminatorEmcGeneralInfoDriveSlotStatus, driveIndex);
        if(status != FBE_STATUS_OK)
        {
           return(status);
        }
        general_info_elem_p->battery_backed = terminatorEmcGeneralInfoDriveSlotStatus.battery_backed;
        general_info_elem_p->elem_index = driveStartElemIndex+driveIndex;    

        *general_info_elem_end_ptr = (fbe_u8_t *)(general_info_elem_p+1);   // next element
    }


    return FBE_STATUS_OK;          
}


/*********************************************************************
*            emc_encl_stat_diag_page_build_ps_info_elems ()
*********************************************************************
*
*  Description: This builds the PS Information expander elements in
*    EMC enclosure status diagnostic page.
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of trace buffer
*       information elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the trace inf. elements.
*   virtual phy object handle, number of trace buffer inf. elements 
*       to be filled.
*
*  Return Value: success or failure
*  NOTES:
*   For now we fill local expander.
*
*  History:
*    Jan-04-2012    Joe Perry   Created.
*    
*********************************************************************/

fbe_status_t emc_encl_stat_diag_page_build_ps_info_elems(fbe_u8_t                   *ps_info_elem_start_ptr,
                                                         fbe_u8_t                  **ps_info_elem_end_ptr,
                                                         fbe_terminator_device_ptr_t virtual_phy_handle,
                                                         fbe_u8_t                   *num_ps_info_elem)
{
    fbe_status_t                    status         = FBE_STATUS_GENERIC_FAILURE;
    ses_ps_info_elem_struct        *ps_info_elem_p = NULL;
    ses_ps_info_elem_struct         terminatorEmcPsInfoStatus;
    terminator_vp_eses_page_info_t *vp_eses_page_info;
    terminator_sp_id_t              spid;
    fbe_u32_t                       psIndex        = 0;

    status = terminator_get_eses_page_info(virtual_phy_handle, &vp_eses_page_info);
    RETURN_ON_ERROR_STATUS;

    *num_ps_info_elem = 2;

    *ps_info_elem_end_ptr = ps_info_elem_start_ptr;
    ps_info_elem_p        = (ses_ps_info_elem_struct *)ps_info_elem_start_ptr;

    for (psIndex = 0; psIndex < *num_ps_info_elem; ++psIndex, ++ps_info_elem_p)
    {
        memset(ps_info_elem_p, 0, sizeof(ses_ps_info_elem_struct));

        status = terminator_get_emcPsInfoStatus(virtual_phy_handle, &terminatorEmcPsInfoStatus);
        RETURN_ON_ERROR_STATUS;

        ps_info_elem_p->ps_elem_index          = psIndex;
        ps_info_elem_p->margining_test_mode    = terminatorEmcPsInfoStatus.margining_test_mode;
        ps_info_elem_p->margining_test_results = terminatorEmcPsInfoStatus.margining_test_results;

        status =  fbe_terminator_api_get_sp_id(&spid);
        RETURN_ON_ERROR_STATUS;

        status = config_page_get_start_elem_index_in_stat_page(virtual_phy_handle,
                                                               SES_SUBENCL_TYPE_PS,
                                                               spid,
                                                               SES_ELEM_TYPE_PS,
                                                               FALSE,
                                                               0,
                                                               FALSE,
                                                               NULL,
                                                               &ps_info_elem_p->ps_elem_index);
        // some enclosures (DPE) will not report PS data, so return no fault
        if (status == FBE_STATUS_COMPONENT_NOT_FOUND)
        {
            return FBE_STATUS_OK;
        }

        RETURN_ON_ERROR_STATUS;

        *ps_info_elem_end_ptr += sizeof(ses_ps_info_elem_struct); // next element
    }

    return FBE_STATUS_OK;
}

/*********************************************************************
*            emc_encl_stat_diag_page_build_trace_buffer_inf_elems ()
*********************************************************************
*
*  Description: This builds the Trace buffer INformation elements in
*    EMC enclosure status diagnostic page.
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of trace buffer
*       information elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the trace inf. elements.
*   virtual phy object handle, number of trace buffer inf. elements 
*       to be filled.
*
*  Return Value: success or failure
*  NOTES:
*   For now we fill only 2 trace buffer elements -> Active Trace buffer &
*    saved trace buffer(client initiated).
*
*  History:
*    Nov-13-2008 Rajesh V Created.
*    
*********************************************************************/

fbe_status_t emc_encl_stat_diag_page_build_trace_buffer_inf_elems(fbe_u8_t *trace_buffer_elem_start_ptr,
                                                             fbe_u8_t **trace_buffer_elem_end_ptr,
                                                             fbe_terminator_device_ptr_t virtual_phy_handle,
                                                             fbe_u8_t *num_trace_buffer_info_elem)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_trace_buf_info_elem_struct *trace_buffer_inf_elem_ptr = NULL; 
    terminator_vp_eses_page_info_t *vp_eses_page_info;
    terminator_sp_id_t spid;

    status = terminator_get_eses_page_info(virtual_phy_handle, &vp_eses_page_info);
    RETURN_ON_ERROR_STATUS;

    *num_trace_buffer_info_elem = TERMINATOR_EMC_PAGE_NUM_TRACE_BUF_INFO_ELEMS;

    trace_buffer_inf_elem_ptr = (ses_trace_buf_info_elem_struct *)(trace_buffer_elem_start_ptr);
    memset(trace_buffer_inf_elem_ptr, 0, sizeof(ses_trace_buf_info_elem_struct));

    status =  fbe_terminator_api_get_sp_id(&spid);

    status = eses_get_buf_id_by_subencl_info(vp_eses_page_info, 
                                             SES_SUBENCL_TYPE_LCC, 
                                             spid, 
                                             SES_BUF_TYPE_ACTIVE_TRACE, 
                                             FBE_FALSE,
                                             0,
                                             FBE_FALSE,
                                             0,
                                             FBE_FALSE,
                                             0, 
                                             &trace_buffer_inf_elem_ptr->buf_id);
    RETURN_ON_ERROR_STATUS;

    
    trace_buffer_inf_elem_ptr->buf_action =  FBE_ESES_TRACE_BUF_ACTION_STATUS_ACTIVE;

    status = config_page_get_start_elem_index_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_LCC,
                                                            spid, 
                                                            SES_ELEM_TYPE_SAS_EXP, 
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &trace_buffer_inf_elem_ptr->elem_index);
    RETURN_ON_ERROR_STATUS;

   
    trace_buffer_inf_elem_ptr++;
    memset(trace_buffer_inf_elem_ptr, 0, sizeof(ses_trace_buf_info_elem_struct));
    
    status = eses_get_buf_id_by_subencl_info(vp_eses_page_info, 
                                             SES_SUBENCL_TYPE_LCC, 
                                             spid, 
                                             SES_BUF_TYPE_SAVED_TRACE, 
                                             FBE_FALSE,
                                             0,
                                             FBE_FALSE,
                                             0,
                                             FBE_FALSE,
                                             0, 
                                             &trace_buffer_inf_elem_ptr->buf_id);
    RETURN_ON_ERROR_STATUS;
    
    
    trace_buffer_inf_elem_ptr->buf_action =  FBE_ESES_TRACE_BUF_ACTION_STATUS_CLIENT_INIT_SAVE_BUF;


    status = config_page_get_start_elem_index_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_LCC,
                                                            spid, 
                                                            SES_ELEM_TYPE_SAS_EXP, 
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &trace_buffer_inf_elem_ptr->elem_index);
    RETURN_ON_ERROR_STATUS;
    


    *trace_buffer_elem_end_ptr = (fbe_u8_t *)(trace_buffer_inf_elem_ptr+1);

    return( FBE_STATUS_OK);          
}


fbe_status_t emc_stat_page_sas_conn_info_elem_get_local_elem_index(
    fbe_terminator_device_ptr_t virtual_phy_handle, 
    fbe_u8_t position,
    fbe_u8_t *conn_elem_index)
{

    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t max_conns;
    fbe_u8_t start_element_index = 0;
    fbe_sas_enclosure_type_t encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    terminator_sp_id_t spid;

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    RETURN_ON_ERROR_STATUS;

    status = terminator_max_conns_per_lcc(encl_type, &max_conns);
    if((status != FBE_STATUS_OK) ||
       (position >= max_conns))
    {
        return(status);
    }

   status =  fbe_terminator_api_get_sp_id(&spid);

    //Get from configuration page parsing routines.
    status = config_page_get_start_elem_index_in_stat_page(virtual_phy_handle,
                                                           SES_SUBENCL_TYPE_LCC,
                                                           spid, 
                                                           SES_ELEM_TYPE_SAS_CONN, 
                                                           FALSE,
                                                           0,
                                                           FALSE,
                                                           NULL,
                                                           &start_element_index);
    RETURN_ON_ERROR_STATUS;
   
    *conn_elem_index = (start_element_index + position);

    status = FBE_STATUS_OK;
    return(status);
}

fbe_status_t emc_stat_page_sas_conn_info_elem_get_attach_sas_addr(
    fbe_u8_t position,
    fbe_u8_t conn_elem_index,
    fbe_u8_t *conn_id,
    fbe_u64_t *attached_sas_address,
    fbe_u8_t *sub_encl_id,
    fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u8_t max_conns_per_port;
    fbe_u8_t max_conns_per_lcc;
    fbe_sas_enclosure_type_t            encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    terminator_conn_map_range_t         range = CONN_IS_ILLEGAL;

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    RETURN_ON_ERROR_STATUS;
    status = terminator_max_conns_per_port(encl_type, &max_conns_per_port);
    RETURN_ON_ERROR_STATUS;

    status = terminator_max_conns_per_lcc(encl_type, &max_conns_per_lcc);
    RETURN_ON_ERROR_STATUS;

    if((status != FBE_STATUS_OK) || (position >= max_conns_per_lcc))
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    switch(encl_type)
    {
        case FBE_SAS_ENCLOSURE_TYPE_VIPER:
        case FBE_SAS_ENCLOSURE_TYPE_BUNKER:
        case FBE_SAS_ENCLOSURE_TYPE_CITADEL:	
        case FBE_SAS_ENCLOSURE_TYPE_DERRINGER:    

        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM:
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP:
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP:
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP:
        case FBE_SAS_ENCLOSURE_TYPE_FALLBACK:
        case FBE_SAS_ENCLOSURE_TYPE_BOXWOOD:
        case FBE_SAS_ENCLOSURE_TYPE_KNOT:
        case FBE_SAS_ENCLOSURE_TYPE_PINECONE:
        case FBE_SAS_ENCLOSURE_TYPE_STEELJAW:
        case FBE_SAS_ENCLOSURE_TYPE_RAMHORN:
        case FBE_SAS_ENCLOSURE_TYPE_ANCHO:
	case FBE_SAS_ENCLOSURE_TYPE_RHEA:
	case FBE_SAS_ENCLOSURE_TYPE_MIRANDA:
	case FBE_SAS_ENCLOSURE_TYPE_CALYPSO:
        case FBE_SAS_ENCLOSURE_TYPE_TABASCO:
            terminator_map_position_max_conns_to_range_conn_id(encl_type, position, max_conns_per_port, &range, conn_id);
            switch(range)
            {
                case CONN_IS_DOWNSTREAM:
                    // Indicates the connector belongs to the extension port
                    status = terminator_get_downstream_wideport_sas_address(virtual_phy_handle, attached_sas_address);
                    *sub_encl_id = FBE_ESES_SUBENCL_SIDE_ID_INVALID;
                    break;
                case CONN_IS_UPSTREAM:
                    // Indicates the connector belongs to the primary port
                    status = terminator_get_upstream_wideport_sas_address(virtual_phy_handle, attached_sas_address);
                    *sub_encl_id = FBE_ESES_SUBENCL_SIDE_ID_INVALID;
                    break;
                case CONN_IS_RANGE0:
                    if((encl_type == FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM) ||
                       (encl_type == FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP) ||
                       (encl_type == FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP)||
                       (encl_type == FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP))
                    {
                        /* Connectors in CONN_IS_RANGE0 are not used by Voyager ICM.*/
                        *attached_sas_address = FBE_SAS_ADDRESS_INVALID;
                        *sub_encl_id = FBE_ESES_SUBENCL_SIDE_ID_INVALID;
                        status = FBE_STATUS_OK;
                    }
                    break;

                case CONN_IS_INTERNAL_RANGE1:
                    // search child list to match connector id. for EE0 or EE1 (Voyager, Viking or Cayenne)
                    status = terminator_get_child_expander_wideport_sas_address_by_connector_id(virtual_phy_handle, *conn_id, attached_sas_address);
                    if(encl_type == FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM) 
                    {
                       *sub_encl_id = 2; // The id here is for the Voyager EE LCC
                    }
                    else
                    {
                       *sub_encl_id = FBE_ESES_SUBENCL_SIDE_ID_INVALID;  // For Viking ( the same LCC board)
                    }
                    break;
 
                case CONN_IS_ILLEGAL:
                default:
                    status = FBE_STATUS_GENERIC_FAILURE;
                    break;
            }
            break;
        
        case FBE_SAS_ENCLOSURE_TYPE_MAGNUM:
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE:
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP:
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP:
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP:
            // Magnum, Voyager EE has a 4XSAS connector to the SAS controller. It will not have any
            // expansion port connectors. This will return SAS address of port 0.    
            status = terminator_get_upstream_wideport_sas_address(virtual_phy_handle, attached_sas_address);
            break;

        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return(status);
}

fbe_status_t emc_stat_page_sas_conn_info_elem_get_peer_elem_index(
    fbe_terminator_device_ptr_t virtual_phy_handle, 
    fbe_u8_t position,
    fbe_u8_t *conn_elem_index)
{

    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t max_conns;
    fbe_u8_t start_element_index = 0;
    fbe_sas_enclosure_type_t encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    terminator_sp_id_t spid;

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    RETURN_ON_ERROR_STATUS;

    status = terminator_max_conns_per_lcc(encl_type, &max_conns);
    if((status != FBE_STATUS_OK) ||
       (position >= max_conns))
    {
        return(status);
    }
     status =  fbe_terminator_api_get_sp_id(&spid);

    //Get from configuration page parsing routines.
    status = config_page_get_start_elem_index_in_stat_page(virtual_phy_handle,
                                                           SES_SUBENCL_TYPE_LCC,
                                                           !spid, 
                                                           SES_ELEM_TYPE_SAS_CONN, 
                                                           FALSE,
                                                           0,
                                                           FALSE,
                                                           NULL,
                                                           &start_element_index);



    *conn_elem_index = (start_element_index + position);

    status = FBE_STATUS_OK;
    return(status);
}


/**************************************************************************
 *  process_emc_encl_ctrl_page()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function processes EMC Enclosure Control Diagnostic Page.
 *
 *  PARAMETERS:
 *     Scatter gather list pointer containing the page, virtual phy object
 *     object handle, sense buffer to be set if necessary.
 *
 *  RETURN VALUES/ERRORS:
 *     success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *     Jan-29-2008: Rajesh V created.
 **************************************************************************/
fbe_status_t process_emc_encl_ctrl_page(fbe_sg_element_t *sg_list,
                                        fbe_terminator_device_ptr_t virtual_phy_handle,
                                        fbe_u8_t *sense_buffer)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_pg_emc_encl_ctrl_struct *emc_encl_ctrl_page_hdr;
    fbe_u32_t gen_code;
    fbe_u8_t *elements_start_ptr = NULL; 
    fbe_sas_enclosure_type_t encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    fbe_eses_info_elem_group_hdr_t *info_elem_group_hdr = NULL;
    fbe_u8_t info_elem_group = 0;

    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    if(status != FBE_STATUS_OK)
    {
        ema_not_ready(sense_buffer);
        return(status);
    }

    emc_encl_ctrl_page_hdr = (ses_pg_emc_encl_ctrl_struct *)fbe_sg_element_address(sg_list);

    status = config_page_get_gen_code(virtual_phy_handle, &gen_code);
    if(status != FBE_STATUS_OK)
    {
        ema_not_ready(sense_buffer);
        return(status);
    }

    if((BYTE_SWAP_32(emc_encl_ctrl_page_hdr->hdr.gen_code)) != gen_code)
    {
        // Generation code mismatch.
        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_ENCLOSURE_SERVICES_TRANSFER_REFUSED,
                                FBE_SCSI_ASCQ_ENCLOSURE_SERVICES_TRANSFER_REFUSED);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    info_elem_group_hdr = (fbe_eses_info_elem_group_hdr_t *)(emc_encl_ctrl_page_hdr + 1);
    for(info_elem_group = 0; info_elem_group < emc_encl_ctrl_page_hdr->num_info_elem_groups; info_elem_group ++)
    {
        switch(info_elem_group_hdr->info_elem_type)
        {
            case FBE_ESES_INFO_ELEM_TYPE_SAS_CONN:
                elements_start_ptr = (fbe_u8_t *)(info_elem_group_hdr + 1);
                status = emc_encl_ctrl_page_process_sas_conn_inf_elems(elements_start_ptr,                                                         
                                                                info_elem_group_hdr->num_info_elems,
                                                                virtual_phy_handle, 
                                                                sense_buffer);
                RETURN_ON_ERROR_STATUS;
                break;

            case FBE_ESES_INFO_ELEM_TYPE_TRACE_BUF:
                elements_start_ptr = (fbe_u8_t *)(info_elem_group_hdr + 1);
                status = emc_encl_ctrl_page_process_trace_buf_inf_elems(elements_start_ptr, 
                                                                        info_elem_group_hdr->num_info_elems, 
                                                                        virtual_phy_handle, 
                                                                        sense_buffer); 
                RETURN_ON_ERROR_STATUS;
                break;

            case FBE_ESES_INFO_ELEM_TYPE_GENERAL:
                elements_start_ptr = (fbe_u8_t *)(info_elem_group_hdr + 1);
                status = emc_encl_ctrl_page_process_general_inf_elems(elements_start_ptr, 
                                                                      info_elem_group_hdr->num_info_elems, 
                                                                      virtual_phy_handle,
                                                                      sense_buffer); 
                RETURN_ON_ERROR_STATUS;
                break; 

            case FBE_ESES_INFO_ELEM_TYPE_ENCL_TIME:
                elements_start_ptr = (fbe_u8_t *)(info_elem_group_hdr + 1);   
                break;

            case FBE_ESES_INFO_ELEM_TYPE_DRIVE_POWER:
                elements_start_ptr = (fbe_u8_t *)(info_elem_group_hdr + 1);  
                break;

            default:
                build_sense_info_buffer(sense_buffer, 
                                        FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                        FBE_SCSI_ASC_UNSUPPORTED_ENCLOSURE_FUNCTION,
                                        FBE_SCSI_ASCQ_UNSUPPORTED_ENCLOSURE_FUNCTION);
                return(FBE_STATUS_GENERIC_FAILURE);
                break;
        }

        info_elem_group_hdr = (fbe_eses_info_elem_group_hdr_t *)(((fbe_u8_t *)info_elem_group_hdr) +
                                                                  (sizeof(info_elem_group_hdr)) +
                                                                  ((info_elem_group_hdr->info_elem_size) *
                                                                   (info_elem_group_hdr->num_info_elems)));
    }

    return(status);
}


/*********************************************************************
*            emc_encl_ctrl_page_process_trace_buf_inf_elems ()
*********************************************************************
*
*  Description: 
*    This processes the Trace Buffer INformation elements in
*    EMC enclosure control diagnostic page.
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of trace buffer
*       information elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the trace buffer information elements.
*   virtual phy object handle, number of trace buffer inf. elements 
*
*  Return Value: 
*       success or failure
*
*  Note: 
*
*  History:
*      Jan-29-2008: Rajesh V created.
*    
*********************************************************************/

fbe_status_t emc_encl_ctrl_page_process_trace_buf_inf_elems(fbe_u8_t *trace_buf_inf_elems_start_ptr,
                                                        fbe_u8_t num_trace_buf_inf_elems,
                                                        fbe_terminator_device_ptr_t virtual_phy_handle,
                                                        fbe_u8_t *sense_buffer)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_trace_buf_info_elem_struct *trace_buffer_inf_elem_ptr = NULL; 
    terminator_eses_buf_info_t *buf_info; // pointer to the buffer information 
                                          // for a particular buffer
    fbe_u8_t i;

    trace_buffer_inf_elem_ptr = (ses_trace_buf_info_elem_struct *)trace_buf_inf_elems_start_ptr;
    for(i = 0; i < num_trace_buf_inf_elems; i++)
    {
        // Currently the only buffer action supported in EMC ctrl page is "Clear Buffer"
        if(trace_buffer_inf_elem_ptr->buf_action == FBE_ESES_TRACE_BUF_ACTION_CTRL_CLEAR_BUF)
        {
            status =  eses_get_buf_info_by_buf_id(virtual_phy_handle, 
                                                trace_buffer_inf_elem_ptr->buf_id,
                                                &buf_info);
            if(status != FBE_STATUS_OK)
            {
                if (trace_buffer_inf_elem_ptr->buf_id != 0xff)
                {
                    // Most probably the buffer with the given buffer ID does
                    // not exist. 
                    build_sense_info_buffer(sense_buffer, 
                                            FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                            FBE_SCSI_ASC_UNSUPPORTED_ENCLOSURE_FUNCTION,
                                            FBE_SCSI_ASCQ_UNSUPPORTED_ENCLOSURE_FUNCTION);
                    return(status);
                }
                else
                {
                    // clear all buffers.
                }
            }
            else if(buf_info->buf_len != 0)
            {
                memset(buf_info->buf, 0, buf_info->buf_len);
            }
        }
        trace_buffer_inf_elem_ptr++;
    }
    return(FBE_STATUS_OK);
}



/*********************************************************************
*            emc_encl_ctrl_page_process_sas_conn_inf_elems ()
*********************************************************************
*
*  Description: 
*    This processes the SAS Connector INformation elements in
*    EMC enclosure control diagnostic page.
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of sas conn
*       information elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the sas inf. elements.
*   virtual phy object handle, number of sas connector inf. elements 
*
*  Return Value: 
*       success or failure
*
*  Note: 
*      For now we DONT do any processing for SAS Connector Information
*      elements.
*
*  History:
*      Jan-29-2008: Rajesh V created.
*    
*********************************************************************/

fbe_status_t emc_encl_ctrl_page_process_sas_conn_inf_elems(fbe_u8_t *sas_conn_elems_start_ptr,
                                                        fbe_u8_t num_sas_conn_info_elems,
                                                        fbe_terminator_device_ptr_t virtual_phy_handle,
                                                        fbe_u8_t *sense_buffer)
{
    return(FBE_STATUS_OK);
}



/*********************************************************************
*            emc_encl_ctrl_page_process_general_inf_elems ()
*********************************************************************
*
*  Description: 
*    This processes the General Information elements in
*    EMC enclosure control diagnostic page.
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of general information elements
*   virtual phy object handle  - virtual phy object handle
*   num_general_inf_elems   - number of general inf. elements
*   sense_buffer                   - Buffer to be filled with error.
*
*  Return Value: 
*       success or failure
*
*  History:
*      July-03-2009: Dipak Patel created.
*    
*********************************************************************/
fbe_status_t emc_encl_ctrl_page_process_general_inf_elems(fbe_u8_t *general_inf_elems_start_ptr,
                                                        fbe_u8_t num_general_inf_elems,
                                                        fbe_terminator_device_ptr_t virtual_phy_handle,
                                                        fbe_u8_t *sense_buffer)

{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_general_info_elem_array_dev_slot_struct *general_inf_elems_ptr = NULL;
    fbe_u8_t start_element_index;
    fbe_u8_t max_drive_slots;
    fbe_u8_t drive_slot_number;
    fbe_sas_enclosure_type_t encl_type;
    fbe_u8_t last_elem_index;
    fbe_u32_t i;
    fbe_bool_t drive_slot_available = FBE_TRUE;
    ses_stat_elem_array_dev_slot_struct array_dev_slot_stat_elem;
    terminator_sp_id_t spid;
    fbe_u32_t powerCycleDurationInMs = 0;

    // so far we only need to handle drive control.  
    // But in the future, we need to add support for expander reset reason.
    general_inf_elems_ptr = (ses_general_info_elem_array_dev_slot_struct *)general_inf_elems_start_ptr;
     status =  fbe_terminator_api_get_sp_id(&spid);

    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    RETURN_ON_ERROR_STATUS;

    for(i = 0; i < num_general_inf_elems; i++)
    {
        if(general_inf_elems_ptr->power_cycle == TRUE)
        {       
            fbe_u8_t drive_power_down_count;
                   
            status = config_page_get_start_elem_index_in_stat_page(virtual_phy_handle, 
                                                                  SES_SUBENCL_TYPE_LCC, 
                                                                  spid, 
                                                                  SES_ELEM_TYPE_ARRAY_DEV_SLOT, 
                                                                  FALSE, 
                                                                  0, 
                                                                  FALSE, 
                                                                  NULL, 
                                                                  &start_element_index);

            RETURN_ON_ERROR_STATUS;
            
            status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
            RETURN_ON_ERROR_STATUS;
            
            status = terminator_max_drive_slots(encl_type, &max_drive_slots);
            RETURN_ON_ERROR_STATUS;

            last_elem_index = start_element_index + max_drive_slots -1;

            // we will skip general info for expander element here.
            if(general_inf_elems_ptr->elem_index < start_element_index || general_inf_elems_ptr->elem_index > last_elem_index)
               continue;

            drive_slot_number = ((general_inf_elems_ptr->elem_index)- start_element_index);

            status = terminator_virtual_phy_is_drive_slot_available(virtual_phy_handle,
                                                                    drive_slot_number,
                                                                    &drive_slot_available);
            RETURN_ON_ERROR_STATUS;

            // Get the current status of the drive slot.
            status = terminator_get_drive_eses_status(virtual_phy_handle,
                                                      drive_slot_number, 
                                                      &array_dev_slot_stat_elem);
            RETURN_ON_ERROR_STATUS;

            // We only try to do a power cycle if the slot has a drive inserted in it and
            // the slot is powered on
            if((!drive_slot_available) &&
               (!array_dev_slot_stat_elem.dev_off))            {

            if(general_inf_elems_ptr->duration == 0)
            {
                powerCycleDurationInMs =  TERMINATOR_ESES_DRIVE_POWER_CYCLE_DELAY;  // 5 seconds.
            }
            else
            {
                powerCycleDurationInMs = general_inf_elems_ptr->duration * 500;    // convert the duration from 0.5 second unit to millisecond unit.
            }

            status = terminator_virtual_phy_power_cycle_drive(virtual_phy_handle,
                                                              powerCycleDurationInMs,
                                                              drive_slot_number);

            RETURN_ON_ERROR_STATUS;

                //Increment the power down count for the drive slot.

                status = terminator_get_drive_power_down_count(virtual_phy_handle,
                                                                     drive_slot_number,
                                                                     &drive_power_down_count);
                RETURN_ON_ERROR_STATUS;

                drive_power_down_count++;

                status = terminator_set_drive_power_down_count(virtual_phy_handle,
                                                               drive_slot_number,
                                                               drive_power_down_count);
                RETURN_ON_ERROR_STATUS;
            }
        }

        // Currently, we only do battery backed set for jetfire.
        if(encl_type == FBE_SAS_ENCLOSURE_TYPE_FALLBACK)
        {
            status = config_page_get_start_elem_index_in_stat_page(virtual_phy_handle, 
                                                                  SES_SUBENCL_TYPE_LCC, 
                                                                  spid, 
                                                                  SES_ELEM_TYPE_ARRAY_DEV_SLOT, 
                                                                  FALSE, 
                                                                  0, 
                                                                  FALSE, 
                                                                  NULL, 
                                                                  &start_element_index);

            RETURN_ON_ERROR_STATUS;
            drive_slot_number = ((general_inf_elems_ptr->elem_index)- start_element_index);

            status = terminator_set_emcGeneralInfoDirveSlotStatus(virtual_phy_handle, (ses_general_info_elem_array_dev_slot_struct*)general_inf_elems_ptr, drive_slot_number);
            if(status != FBE_STATUS_OK)
            {
               return(status);
            }
        }

        general_inf_elems_ptr++;
    }
    return FBE_STATUS_OK;
}
