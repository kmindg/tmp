
/***************************************************************************
 *  terminator_encl_eses_plugin_addl_stat_page.c
 ***************************************************************************
 *
 *  Description
 *     This contains the funtions related to Additional Status diagnostic
 *      page.
 *      
 *  NOTES:
 *      No longer filling in ESC electronics elements in Addl Status Page.
 *
 *  History:
 *      Nov-24-08	Rajesh V. Split the Addl status page functions into 
 *                      seperate file.
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
/*        local functions         */
/**********************************/
static fbe_status_t addl_elem_stat_page_build_sas_exp_elements(
    fbe_u8_t *status_elements_start_ptr, 
    fbe_u8_t **status_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle);
static fbe_status_t addl_elem_stat_page_build_stat_descriptors(
    fbe_u8_t *status_elements_start_ptr, 
    fbe_u8_t **status_elements_end_ptr, 
    fbe_terminator_device_ptr_t virtual_phy_handle);
static fbe_status_t addl_elem_stat_page_build_device_slot_elements(
    fbe_u8_t *status_elements_start_ptr, 
    fbe_u8_t **status_elements_end_ptr, 
    fbe_terminator_device_ptr_t virtual_phy_handle);
static fbe_status_t addl_elem_stat_page_dev_slots_get_elem_index(
    fbe_terminator_device_ptr_t virtual_phy_handle, fbe_u8_t drive_slot_num,  fbe_u8_t *elem_index);
static fbe_status_t addl_elem_stat_page_dev_slot_phy_desc_get_sas_addr(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u8_t dev_slot_num, 
    fbe_u64_t *sas_address);
static fbe_status_t addl_elem_stat_page_phy_desc_get_phy_id(
    fbe_u8_t dev_slot_num, fbe_u8_t *phy_id);
static fbe_u32_t addl_elem_stat_page_size(fbe_sas_enclosure_type_t encl_type);
static fbe_status_t addl_elem_stat_page_build_esc_electronics_elements(
    fbe_u8_t *status_elements_start_ptr, 
    fbe_u8_t **status_elements_end_ptr, 
    fbe_sas_enclosure_type_t encl_type);
static fbe_status_t addl_elem_stat_page_esc_elec_phy_desc_get_other_elem_index(
    fbe_u8_t phy_id, fbe_u8_t *elem_index, fbe_sas_enclosure_type_t encl_type);
static fbe_status_t addl_elem_stat_page_esc_electronics_get_elem_index(fbe_u8_t *elem_index);
static fbe_status_t  addl_elem_stat_page_sas_exp_phy_desc_get_other_elem_index_for_drive_phy(
    fbe_terminator_device_ptr_t virtual_phy_handle, fbe_u8_t drive_slot, fbe_u8_t *elem_index);
static fbe_status_t addl_elem_stat_page_sas_exp_phy_desc_get_conn_elem_index_for_conn_phy(
    fbe_terminator_device_ptr_t virtual_phy_handle, fbe_u8_t connector, fbe_u8_t connector_id, fbe_u8_t *elem_index);
static fbe_status_t addl_elem_stat_page_sas_exp_phy_desc_get_other_elem_index_for_conn_phy(
     fbe_terminator_device_ptr_t virtual_phy_handle, fbe_u8_t connector, fbe_u8_t connector_id, fbe_u8_t *elem_index);
static fbe_status_t addl_elem_stat_page_sas_exp_get_elem_index(fbe_terminator_device_ptr_t virtual_phy_handle, fbe_u8_t *elem_index);
static fbe_status_t addl_elem_stat_page_sas_exp_get_sas_address(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u64_t *sas_address);


/*********************************************************************
*           build_additional_element_status_diagnostic_page ()
*********************************************************************
*
*  Description: This builds the Addl element status diagnostic page. 
*
*  Inputs: sg_list, enclosure information, virtual phy device id, 
*   port number.
*
*  Return Value: success or failure
*
*  History:
*    Aug08 	Rajesh V. created 
*    
*********************************************************************/
fbe_status_t build_additional_element_status_diagnostic_page(fbe_sg_element_t * sg_list, fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_common_pg_hdr_struct *addl_elem_stat_page_hdr;
    fbe_u8_t *status_elements_start_ptr = NULL; // point to first overall status element
    fbe_u8_t *status_elements_end_ptr = NULL;
    fbe_sas_enclosure_type_t    encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    if(fbe_sg_element_count(sg_list) < addl_elem_stat_page_size(encl_type))
    {
        return(status);
    }
    addl_elem_stat_page_hdr = (ses_common_pg_hdr_struct *)fbe_sg_element_address(sg_list);
    memset(addl_elem_stat_page_hdr, 0, sizeof(ses_common_pg_hdr_struct));
    addl_elem_stat_page_hdr->pg_code = SES_PG_CODE_ADDL_ELEM_STAT;
    // Gen code returned in bigendian format.
    status = config_page_get_gen_code(virtual_phy_handle ,&addl_elem_stat_page_hdr->gen_code); 
    RETURN_ON_ERROR_STATUS;
    addl_elem_stat_page_hdr->gen_code = BYTE_SWAP_32(addl_elem_stat_page_hdr->gen_code);

    status_elements_start_ptr = (fbe_u8_t *) (((fbe_u8_t *)addl_elem_stat_page_hdr) + FBE_ESES_PAGE_HEADER_SIZE);
    status = addl_elem_stat_page_build_stat_descriptors(status_elements_start_ptr, &status_elements_end_ptr, virtual_phy_handle);
    RETURN_ON_ERROR_STATUS;
    addl_elem_stat_page_hdr->pg_len = (fbe_u16_t)(status_elements_end_ptr - (fbe_u8_t *)(addl_elem_stat_page_hdr) - 4);
    // Always return page length in big endian format as actual expander firmware does that.
    addl_elem_stat_page_hdr->pg_len = BYTE_SWAP_16(addl_elem_stat_page_hdr->pg_len);

    status = FBE_STATUS_OK;
    return(status); 
}
/*********************************************************************
*          addl_elem_stat_page_build_stat_descriptors ()
*********************************************************************
*
*  Description: This builds the Addl element status diagnostic page 
*   status descriptors(device slot elements, sas expander elements
*   ----etc).
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of expander elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the expander elements.
*   enclosure information, virtual phy device id and port number.
*
*  Return Value: success or failure
*
*  History:
*    Aug08 	Rajesh V. created
*    
*********************************************************************/

fbe_status_t addl_elem_stat_page_build_stat_descriptors(
    fbe_u8_t *status_elements_start_ptr, 
    fbe_u8_t **status_elements_end_ptr, 
    fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t *next_status_element_set_ptr = status_elements_start_ptr;

    status = addl_elem_stat_page_build_device_slot_elements(
        next_status_element_set_ptr, status_elements_end_ptr, virtual_phy_handle);
    RETURN_ON_ERROR_STATUS;

    next_status_element_set_ptr = (*status_elements_end_ptr);
    status = addl_elem_stat_page_build_sas_exp_elements(
        next_status_element_set_ptr, status_elements_end_ptr, virtual_phy_handle);
    RETURN_ON_ERROR_STATUS;

/************************************************************************
 * Ignore ESC electronics elements as now all their functionality
 * was decided to transfer to SAS Exp elements.
 ***********************************************************************
    next_status_element_set_ptr = (*status_elements_end_ptr);
    status = addl_elem_stat_page_build_esc_electronics_elements(
        next_status_element_set_ptr, status_elements_end_ptr, virtual_phy_handle);
    RETURN_ON_ERROR_STATUS;
 ***********************************************************************/

    status = FBE_STATUS_OK;
    return(status);    
}

/*********************************************************************
*           addl_elem_stat_page_build_sas_exp_elements ()
*********************************************************************
*
*  Description: This builds the SAS expander elements for the Addl elem
*   status page. Pg 99.
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of expander elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the expander elements.
*   enclosure information, virtual phy device id and port number.
*
*  Return Value: success or failure
*
*  History:
*    Aug08 	created
*    
*********************************************************************/
fbe_status_t addl_elem_stat_page_build_sas_exp_elements(
    fbe_u8_t *status_elements_start_ptr, 
    fbe_u8_t **status_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_addl_elem_stat_desc_hdr_struct *addl_elem_stat_desc_ptr = NULL;
    ses_sas_exp_prot_spec_info_struct *sas_exp_prot_spec_info_ptr = NULL;
    ses_sas_exp_phy_desc_struct *sas_exp_phy_desc_ptr = NULL;
    fbe_u8_t drive_slot, connector, i;
    fbe_u8_t connector_id;
    fbe_sas_enclosure_type_t           encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    RETURN_ON_ERROR_STATUS;

    addl_elem_stat_desc_ptr = (ses_addl_elem_stat_desc_hdr_struct *)status_elements_start_ptr; 
    memset(addl_elem_stat_desc_ptr, 0, sizeof(ses_addl_elem_stat_desc_hdr_struct));
    // only SAS for now (6 is for SAS)
    addl_elem_stat_desc_ptr->protocol_id = 0x6; 
    addl_elem_stat_desc_ptr->eip = 0x1;

    //addl_elem_stat_desc_ptr->desc_len = sizeof(ses_esc_elec_prot_spec_info_struct) + 23*sizeof(ses_esc_elec_exp_phy_desc_struct) + 2;


    status = addl_elem_stat_page_sas_exp_get_elem_index(virtual_phy_handle, &addl_elem_stat_desc_ptr->elem_index);
    RETURN_ON_ERROR_STATUS;

    sas_exp_prot_spec_info_ptr = (ses_sas_exp_prot_spec_info_struct *)
        ((fbe_u8_t *)addl_elem_stat_desc_ptr + FBE_ESES_ADDL_ELEM_STATUS_DESC_HDR_SIZE );
    status = terminator_max_phys(encl_type, &sas_exp_prot_spec_info_ptr->num_exp_phy_descs);
    RETURN_ON_ERROR_STATUS;
    sas_exp_prot_spec_info_ptr->desc_type = 0x1;

    status = addl_elem_stat_page_sas_exp_get_sas_address(virtual_phy_handle, &sas_exp_prot_spec_info_ptr->sas_address);
    RETURN_ON_ERROR_STATUS;
    // convert to big endian
    sas_exp_prot_spec_info_ptr->sas_address = 
        BYTE_SWAP_64(sas_exp_prot_spec_info_ptr->sas_address);

    sas_exp_phy_desc_ptr = (ses_sas_exp_phy_desc_struct *)
        ((fbe_u8_t *)sas_exp_prot_spec_info_ptr + FBE_ESES_SAS_EXP_PROT_SPEC_INFO_HEADER_SIZE);
    // fill in the phy descs. As per recent changes to ESES, T10 spec it was
    // decided that the phy's are arranged in order as per their SAS phy 
    // identifiers(logical), which is too much an assumption :(
    for(i=0; i < sas_exp_prot_spec_info_ptr->num_exp_phy_descs; i++)
    {
        memset(sas_exp_phy_desc_ptr, 0, sizeof(ses_sas_exp_phy_desc_struct));
        if(terminator_phy_corresponds_to_drive_slot(i, &drive_slot, encl_type))
        {         
            sas_exp_phy_desc_ptr->conn_elem_index = 0xFF;
            status = addl_elem_stat_page_sas_exp_phy_desc_get_other_elem_index_for_drive_phy(
                virtual_phy_handle, drive_slot, &sas_exp_phy_desc_ptr->other_elem_index);
		terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: encl_type:%d drive_slot:%d other_elem_index:%d status:0x%x\n", 
                             __FUNCTION__, encl_type, drive_slot,
                             sas_exp_phy_desc_ptr->other_elem_index,
                             status);
            RETURN_ON_ERROR_STATUS;
        }
        else if(terminator_phy_corresponds_to_connector(i, &connector, &connector_id, encl_type))
        {
            // The connector is a number between 0 and max_single_lane_port_conns, so the resulting
            // element index should be properly calculated.
            status = addl_elem_stat_page_sas_exp_phy_desc_get_other_elem_index_for_conn_phy(
                 virtual_phy_handle, connector, connector_id, &sas_exp_phy_desc_ptr->other_elem_index);
            RETURN_ON_ERROR_STATUS;

            status = addl_elem_stat_page_sas_exp_phy_desc_get_conn_elem_index_for_conn_phy(
                 virtual_phy_handle, connector, connector_id, &sas_exp_phy_desc_ptr->conn_elem_index);
            terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: encl_type:%d connector:%d connector_id:%d elem_index:%d other_elem_index:%d\n", 
                             __FUNCTION__, encl_type, connector, connector_id, 
                             sas_exp_phy_desc_ptr->conn_elem_index,
                             sas_exp_phy_desc_ptr->other_elem_index);
            RETURN_ON_ERROR_STATUS;      
        } 
        else
        {

            terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: encl_type:%d phy:%d is not a drive or connector, max:%d\n", 
                             __FUNCTION__, encl_type, i, sas_exp_prot_spec_info_ptr->num_exp_phy_descs);
            //Ex: Phy 8 in Viper doesnot correspond to drive slot or connector.
            sas_exp_phy_desc_ptr->other_elem_index = 0xFF; 
            sas_exp_phy_desc_ptr->conn_elem_index = 0xFF;

        }

        sas_exp_phy_desc_ptr++;      
    }
    *status_elements_end_ptr = (fbe_u8_t *)sas_exp_phy_desc_ptr;
    addl_elem_stat_desc_ptr->desc_len =(fbe_u8_t) (*status_elements_end_ptr - status_elements_start_ptr - 2);
    status = FBE_STATUS_OK;
    return(status);        
}

/*********************************************************************
*          addl_elem_stat_page_build_esc_electronics_elements ()
*********************************************************************
*
*  Description: This builds the ESC elecronics elements for the Addl elem
*   status page. Note that after changes to the SES spec ESC electronics
*   is now in the SAS expander elements.
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of expander elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the expander elements.
*   enclosure information, virtual phy device id and port number.
*
*  Return Value: success or failure

*  Note:
*   As per the new  modifications this is not useful anymore 
*   from our perspective. OUTDATED & no longer USED as
*   sper the modified ESES Spec.    
*
*  History:
*    Aug08 	created
*    
*********************************************************************/
fbe_status_t addl_elem_stat_page_build_esc_electronics_elements(
    fbe_u8_t *status_elements_start_ptr, 
    fbe_u8_t **status_elements_end_ptr, 
    fbe_sas_enclosure_type_t encl_type)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_addl_elem_stat_desc_hdr_struct *addl_elem_stat_desc_ptr = NULL;
    ses_esc_elec_prot_spec_info_struct *esc_elec_prot_spec_info_ptr = NULL;
    ses_esc_elec_exp_phy_desc_struct *esc_elec_exp_phy_desc_ptr = NULL;
    fbe_u8_t i, drive_slot;

    addl_elem_stat_desc_ptr = (ses_addl_elem_stat_desc_hdr_struct *)status_elements_start_ptr; 
    memset(addl_elem_stat_desc_ptr, 0, sizeof(ses_addl_elem_stat_desc_hdr_struct));
    // only SAS for now (6 is for SAS)
    addl_elem_stat_desc_ptr->protocol_id = 0x6; 
    addl_elem_stat_desc_ptr->eip = 0x1;

    addl_elem_stat_desc_ptr->desc_len = (fbe_u8_t) (sizeof(ses_esc_elec_prot_spec_info_struct) + 23*sizeof(ses_esc_elec_exp_phy_desc_struct) + 2);

    //Ther element index should be changed once we get the configuration diagnostic page offsets
    // For esc electronics elements this is invalid as we dont return esc elements in the status 
    // page, yet. Even if we return there is only one ESC electronics element.
    status = addl_elem_stat_page_esc_electronics_get_elem_index(&addl_elem_stat_desc_ptr->elem_index);
    RETURN_ON_ERROR_STATUS;

    esc_elec_prot_spec_info_ptr = (ses_esc_elec_prot_spec_info_struct *)
        ((fbe_u8_t *)addl_elem_stat_desc_ptr + FBE_ESES_ADDL_ELEM_STATUS_DESC_HDR_SIZE );
    status = terminator_max_phys(encl_type, &esc_elec_prot_spec_info_ptr->num_exp_phy_descs);
    RETURN_ON_ERROR_STATUS;
    esc_elec_prot_spec_info_ptr->desc_type = 0x1;

    esc_elec_exp_phy_desc_ptr = (ses_esc_elec_exp_phy_desc_struct *)
        ((fbe_u8_t *)esc_elec_prot_spec_info_ptr + FBE_ESES_ESC_ELEC_PROT_SPEC_INFO_HEADER_SIZE);
    // fill in the phy descs
    for(i=0; i < esc_elec_prot_spec_info_ptr->num_exp_phy_descs; i++)
    {
        memset(esc_elec_exp_phy_desc_ptr, 0, sizeof(ses_esc_elec_exp_phy_desc_struct));
        esc_elec_exp_phy_desc_ptr->phy_id = i;
        if(!terminator_phy_corresponds_to_drive_slot(i, &drive_slot, encl_type))
        {
            esc_elec_exp_phy_desc_ptr->conn_elem_index = 0xFF; // fill this once connector elements filled in stat page
            esc_elec_exp_phy_desc_ptr->other_elem_index = 0xFF;
        }
        else
        {

        // for the phys not mapped to drive slots this will always be invalid.
            esc_elec_exp_phy_desc_ptr->conn_elem_index = 0xFF;
            status = addl_elem_stat_page_esc_elec_phy_desc_get_other_elem_index(
                esc_elec_exp_phy_desc_ptr->phy_id, &esc_elec_exp_phy_desc_ptr->other_elem_index, encl_type);
            RETURN_ON_ERROR_STATUS;
        }       
        //ignored the sas address field here.
        //....
        esc_elec_exp_phy_desc_ptr++;      
    }
    *status_elements_end_ptr = (fbe_u8_t *)esc_elec_exp_phy_desc_ptr;
    status = FBE_STATUS_OK;
    return(status);    
}

/*********************************************************************
*           addl_elem_stat_page_build_device_slot_elements ()
*********************************************************************
*
*  Description: This builds the device slot elements for the Addl elem
*   status page. 
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of expander elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the expander elements.
*   enclosure information, virtual phy device id and port number.
*
*  Return Value: success or failure
*
*  History:
*    Aug08 	created
*    
*********************************************************************/

fbe_status_t addl_elem_stat_page_build_device_slot_elements(
    fbe_u8_t *status_elements_start_ptr, 
    fbe_u8_t **status_elements_end_ptr, 
    fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_addl_elem_stat_desc_hdr_struct *addl_elem_stat_desc_ptr = NULL;
    ses_array_dev_slot_prot_spec_info_struct *dev_slot_prot_spec_info_ptr = NULL;
    ses_array_dev_phy_desc_struct *dev_phy_desc_ptr = NULL;
    fbe_u8_t max_drive_slots = 0;
    fbe_u8_t i;
    fbe_sas_enclosure_type_t    encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    fbe_u32_t slot_number = 0;

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);

    addl_elem_stat_desc_ptr = (ses_addl_elem_stat_desc_hdr_struct *)status_elements_start_ptr; 
    memset(addl_elem_stat_desc_ptr, 0, sizeof(ses_addl_elem_stat_desc_hdr_struct));

    status = terminator_max_drive_slots(encl_type, &max_drive_slots);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }
    for(i=0; i<max_drive_slots; i++)
    {
        memset(addl_elem_stat_desc_ptr, 0, sizeof(ses_addl_elem_stat_desc_hdr_struct));
        // only SAS for now (6 is for SAS)
        addl_elem_stat_desc_ptr->protocol_id = 0x6; 
        addl_elem_stat_desc_ptr->eip = 0x1;
        addl_elem_stat_desc_ptr->desc_len = 0x22;

        //Using configuration diagnostic page offsets
        if((addl_elem_stat_page_dev_slots_get_elem_index(virtual_phy_handle, i, &addl_elem_stat_desc_ptr->elem_index)) !=
            FBE_STATUS_OK)
        {
            return(FBE_STATUS_GENERIC_FAILURE);
        }
        dev_slot_prot_spec_info_ptr = (ses_array_dev_slot_prot_spec_info_struct *)(((fbe_u8_t *)addl_elem_stat_desc_ptr) + FBE_ESES_ADDL_ELEM_STATUS_DESC_HDR_SIZE);
        memset(dev_slot_prot_spec_info_ptr, 0, sizeof(ses_array_dev_slot_prot_spec_info_struct));
        dev_slot_prot_spec_info_ptr->num_phy_descs = 0x1;
        dev_slot_prot_spec_info_ptr->desc_type = 0;
        dev_slot_prot_spec_info_ptr->not_all_phys = 0x1;

        slot_number = i;
        // update the slot number
        status = terminator_update_enclosure_drive_slot_number(virtual_phy_handle, &slot_number);
        if(status != FBE_STATUS_OK)
        {
            return status;
        }
        dev_slot_prot_spec_info_ptr->dev_slot_num = (fbe_u8_t)slot_number;

        dev_phy_desc_ptr = (ses_array_dev_phy_desc_struct *)(((fbe_u8_t *)dev_slot_prot_spec_info_ptr) + FBE_ESES_ARRAY_DEV_SLOT_PROT_SPEC_INFO_HEADER_SIZE );
        memset(dev_phy_desc_ptr, 0, sizeof(ses_array_dev_phy_desc_struct));

        // This is not the actual phy to device mapping. So can be ignored in simulation.
        if(addl_elem_stat_page_phy_desc_get_phy_id(i,&dev_phy_desc_ptr->phy_id) !=
            FBE_STATUS_OK)
        {
            return(status);
        }

        // this is sas address of the drive physical slot i;
        if(addl_elem_stat_page_dev_slot_phy_desc_get_sas_addr(virtual_phy_handle, i, &dev_phy_desc_ptr->sas_address) !=
            FBE_STATUS_OK)
        {
            return(status);
        }
        // convert to bigendian
        dev_phy_desc_ptr->sas_address = BYTE_SWAP_64(dev_phy_desc_ptr->sas_address);

        terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: encl_type:%d max_drives:%d slot_number:%d phy_id:%d\n", 
                             __FUNCTION__, encl_type, max_drive_slots, slot_number, 
                             dev_phy_desc_ptr->phy_id);



        // remember to ask ping if this structure needs the field "attached sas address"
        // this represents the enclosure SMP physical sas address...

        addl_elem_stat_desc_ptr = (ses_addl_elem_stat_desc_hdr_struct *)
            (((fbe_u8_t *)addl_elem_stat_desc_ptr) + 
            FBE_ESES_ADDL_ELEM_STATUS_DESC_HDR_SIZE + 
            FBE_ESES_ARRAY_DEV_SLOT_PROT_SPEC_INFO_HEADER_SIZE + 
            sizeof(ses_array_dev_phy_desc_struct)) ;
    }
    *status_elements_end_ptr = (fbe_u8_t *)addl_elem_stat_desc_ptr;
    status = FBE_STATUS_OK;
    return(status);
}

fbe_status_t addl_elem_stat_page_dev_slots_get_elem_index(fbe_terminator_device_ptr_t virtual_phy_handle, 
                                                          fbe_u8_t drive_slot_num,  
                                                          fbe_u8_t *elem_index)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u8_t start_element_index = 0;
    terminator_sp_id_t spid;

    status =  fbe_terminator_api_get_sp_id(&spid);

        //Get from configuration page parsing routines.
        status = config_page_get_start_elem_index_in_stat_page(virtual_phy_handle,
                                                           SES_SUBENCL_TYPE_LCC,
                                                           spid, 
                                                           SES_ELEM_TYPE_ARRAY_DEV_SLOT, 
                                                           FALSE,
                                                           0,
                                                           FALSE,
                                                           NULL,
                                                           &start_element_index);


    *elem_index = start_element_index + drive_slot_num;
    return(status);
}

fbe_status_t addl_elem_stat_page_esc_elec_phy_desc_get_other_elem_index(
    fbe_u8_t phy_id, fbe_u8_t *elem_index, fbe_sas_enclosure_type_t encl_type)
{
    fbe_u8_t drive_slot;

    //Change in future. There should be a call again to addl_elem_stat_page_dev_slots_get_elem_index()
    // when we obtain the corresponding phy slot.
    if(terminator_phy_corresponds_to_drive_slot(phy_id, &drive_slot, encl_type))
    {
        *elem_index = drive_slot;
        return(FBE_STATUS_OK);
    }
    return(FBE_STATUS_GENERIC_FAILURE);
}

fbe_status_t  addl_elem_stat_page_sas_exp_phy_desc_get_other_elem_index_for_drive_phy(
    fbe_terminator_device_ptr_t virtual_phy_handle, fbe_u8_t drive_slot, fbe_u8_t *elem_index)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u8_t start_element_index = 0;   
    fbe_u8_t max_drive_slots;
    fbe_sas_enclosure_type_t encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    terminator_sp_id_t spid;

    status =  fbe_terminator_api_get_sp_id(&spid);

        //Get from configuration page parsing routines.
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


    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    RETURN_ON_ERROR_STATUS;

    status = terminator_max_drive_slots(encl_type, &max_drive_slots);
    RETURN_ON_ERROR_STATUS;

    if(drive_slot < max_drive_slots)
    {
        *elem_index = start_element_index + drive_slot;
        return(FBE_STATUS_OK);
    }

    return(FBE_STATUS_GENERIC_FAILURE);
}

fbe_status_t addl_elem_stat_page_sas_exp_phy_desc_get_other_elem_index_for_conn_phy(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u8_t connector, 
    fbe_u8_t connector_id, 
    fbe_u8_t *elem_index)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t start_element_index = 0;
    fbe_u8_t max_single_lane_port_conns;
    fbe_sas_enclosure_type_t encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    terminator_sp_id_t spid;

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
   

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    RETURN_ON_ERROR_STATUS;

    status = terminator_max_single_lane_conns_per_port(encl_type, &max_single_lane_port_conns);
    RETURN_ON_ERROR_STATUS;

    switch(encl_type)
    {
        case FBE_SAS_ENCLOSURE_TYPE_VIPER:
        case FBE_SAS_ENCLOSURE_TYPE_MAGNUM:
        case FBE_SAS_ENCLOSURE_TYPE_BUNKER:
        case FBE_SAS_ENCLOSURE_TYPE_CITADEL:
        case FBE_SAS_ENCLOSURE_TYPE_DERRINGER:    
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM:
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
            if(connector < max_single_lane_port_conns)
            {
                // The 
                //other element index for connector phys should always
                //point to the whole connector element(whose physical link
                //is FFh)
                *elem_index = start_element_index  + (max_single_lane_port_conns +1) * connector_id;

                status = FBE_STATUS_OK;
            }
            else
            {
                status = FBE_STATUS_GENERIC_FAILURE;
            }
            break;

        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP:
            if(connector_id >= 0 && connector_id <= 4) 
            {
                // The 
                //other element index for connector phys should always
                //point to the whole connector element(whose physical link
                //is FFh)
                *elem_index = start_element_index  + 5 * connector_id;
                status = FBE_STATUS_OK;
            }
            else if(connector_id >= 5 && connector_id < max_single_lane_port_conns)
            {
                // The 
                //other element index for connector phys should always
                //point to the whole connector element(whose physical link
                //is FFh)
                *elem_index = start_element_index  + (5 * (connector_id - 1)) + 9;
                status = FBE_STATUS_OK;
            }
            else
            {
                status = FBE_STATUS_GENERIC_FAILURE;
            }
                   
            break;

        case FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP:
            if(connector_id == 0 || connector_id == 1) 
            {
                // The 
                //other element index for connector phys should always
                //point to the whole connector element(whose physical link
                //is FFh)
                *elem_index = start_element_index  + 5 * connector_id;
                status = FBE_STATUS_OK;
            }
            else if(connector_id > 1 && connector_id < 6)
            {
                // The 
                //other element index for connector phys should always
                //point to the whole connector element(whose physical link
                //is FFh)
                *elem_index = start_element_index  + 5 * 2 + 6 * (connector_id - 2);
                status = FBE_STATUS_OK;
            }
            else if(connector_id == 6 || connector_id == 7)
            {
                // The 
                //other element index for connector phys should always
                //point to the whole connector element(whose physical link
                //is FFh)
                *elem_index = start_element_index  + 5 * 2 + 6 * 4 + 5 * (connector_id - 6);
                status = FBE_STATUS_OK;
            }
            else
            {
                status = FBE_STATUS_GENERIC_FAILURE;
            }

            break;

        case FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP:
            if(connector_id >= 0 && connector_id <= 3) 
            {
                // The 
                //other element index for connector phys should always
                //point to the whole connector element(whose physical link
                //is FFh)
                *elem_index = start_element_index  + 5 * connector_id;
                status = FBE_STATUS_OK;
            }
            else if(connector_id == 4 || connector_id == 5)
            {
                // The 
                //other element index for connector phys should always
                //point to the whole connector element(whose physical link
                //is FFh)
                *elem_index = start_element_index  + 5 * 4 + 9 * (connector_id - 4);
                status = FBE_STATUS_OK;
            }
            else
            {
                status = FBE_STATUS_GENERIC_FAILURE;
            }
                   
            break;

        /* As Voyager EE has only primary port, element index should be calculated as below */
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE:
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP:
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP:
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP:
            if(connector < max_single_lane_port_conns)
            {                
                *elem_index = start_element_index;

                status = FBE_STATUS_OK;
            }
            else
            {
                status = FBE_STATUS_GENERIC_FAILURE;
            }
            break;

        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return(status);
}

fbe_status_t addl_elem_stat_page_sas_exp_phy_desc_get_conn_elem_index_for_conn_phy(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u8_t connector, 
    fbe_u8_t connector_id, 
    fbe_u8_t *elem_index)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t start_element_index = 0;   
    fbe_u8_t max_single_lane_port_conns;
    fbe_sas_enclosure_type_t encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    terminator_sp_id_t spid;

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

   
    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    RETURN_ON_ERROR_STATUS;


    status = terminator_max_single_lane_conns_per_port(encl_type, &max_single_lane_port_conns);
    RETURN_ON_ERROR_STATUS;
    switch(encl_type)
    {
        case FBE_SAS_ENCLOSURE_TYPE_VIPER:
        case FBE_SAS_ENCLOSURE_TYPE_MAGNUM:
        case FBE_SAS_ENCLOSURE_TYPE_BUNKER:
        case FBE_SAS_ENCLOSURE_TYPE_CITADEL:
        case FBE_SAS_ENCLOSURE_TYPE_DERRINGER:
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM:
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
            if(connector < max_single_lane_port_conns)
            {
                
                *elem_index = start_element_index + connector + 1  + (max_single_lane_port_conns +1) * connector_id;

                //1 for the connector element that
                //represents the connector as a whole.(for primary port).

                    //expansion port connectors. +1 is again for the element 
                    //that represents connector element as a whole , this time for
                    // expansion port.

                status = FBE_STATUS_OK;
            }
            else
            {
                status = FBE_STATUS_GENERIC_FAILURE;
            }
            break;

        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP:
            if(connector_id >= 0 && connector_id <= 4) 
            {
                // The 
                //other element index for connector phys should always
                //point to the whole connector element(whose physical link
                //is FFh)
                *elem_index = start_element_index  + connector + 1  + 5 * connector_id;
                status = FBE_STATUS_OK;
            }
            else if(connector_id >= 5 && connector_id < max_single_lane_port_conns)
            {
                // The 
                //other element index for connector phys should always
                //point to the whole connector element(whose physical link
                //is FFh)
                *elem_index = start_element_index  + connector + 1  + (5 * (connector_id - 1)) + 9;
                status = FBE_STATUS_OK;
            }
            else
            {
                status = FBE_STATUS_GENERIC_FAILURE;
            }

            break;

        case FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP:
            if(connector_id == 0 || connector_id == 1) 
            {
                //1 for the connector element that
                //represents the connector as a whole.
                *elem_index = start_element_index  + connector + 1 + 5 * connector_id;
                status = FBE_STATUS_OK;
            }
            else if(connector_id > 1 && connector_id < 6)
            {
                //1 for the connector element that
                //represents the connector as a whole.
                *elem_index = start_element_index  + connector + 1 + 5 * 2 + 6 * (connector_id - 2);
                status = FBE_STATUS_OK;
            }
            else if(connector_id == 6 || connector_id == 7)
            {
                //1 for the connector element that
                //represents the connector as a whole.
                *elem_index = start_element_index  + connector + 1 + 5 * 2 + 6 * 4 + 5 * (connector_id - 6);
                status = FBE_STATUS_OK;
            }
            else
            {
                status = FBE_STATUS_GENERIC_FAILURE;
            }
            break;

        case FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP:
            if(connector_id >= 0 && connector_id <= 3) 
            {
                //1 for the connector element that
                //represents the connector as a whole.
                *elem_index = start_element_index  + connector + 1 + 5 * connector_id;
                status = FBE_STATUS_OK;
            }
            else if(connector_id == 4 || connector_id == 5)
            {
                //1 for the connector element that
                //represents the connector as a whole.
                *elem_index = start_element_index  + connector + 1 + 5 * 4 + 9 * (connector_id - 4);
                status = FBE_STATUS_OK;
            }
            else
            {
                status = FBE_STATUS_GENERIC_FAILURE;
            }

            break;
         /* As Voyager EE or Viking DRVSXP has only primary port, element index should be calculated as below */   
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE:
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP:
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP:
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP:
            if(connector < max_single_lane_port_conns)
            {
                
                *elem_index = start_element_index + connector + 1;
                status = FBE_STATUS_OK;
            }
            else
            {
                status = FBE_STATUS_GENERIC_FAILURE;
            }
            break;


        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return(status);
}

fbe_status_t addl_elem_stat_page_esc_electronics_get_elem_index(fbe_u8_t *elem_index)
{
    //we dont yet have esc elec elements in status page.
    *elem_index = 0;
    return(FBE_STATUS_OK);
}

fbe_status_t addl_elem_stat_page_sas_exp_get_elem_index(fbe_terminator_device_ptr_t virtual_phy_handle, fbe_u8_t *elem_index)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;    
    terminator_sp_id_t spid;

    *elem_index = 0;

    status =  fbe_terminator_api_get_sp_id(&spid);

        //Get from configuration page parsing routines.
        status = config_page_get_start_elem_index_in_stat_page(virtual_phy_handle,
                                                           SES_SUBENCL_TYPE_LCC,
                                                           spid, 
                                                           SES_ELEM_TYPE_SAS_EXP, 
                                                           FALSE,
                                                           0,
                                                           FALSE,
                                                           NULL,
                                                           elem_index);
        RETURN_ON_ERROR_STATUS;

    
    status = FBE_STATUS_OK;
    return(status);
}

/*
 *Just a wrapper for IO module Addl elem stat page functions to
 *call termiantor and get sas address of the enclosure.
 */
fbe_status_t addl_elem_stat_page_sas_exp_get_sas_address(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u64_t *sas_address)
{
    return(terminator_get_encl_sas_address_by_virtual_phy_id(virtual_phy_handle, sas_address));
}

fbe_status_t addl_elem_stat_page_dev_slot_phy_desc_get_sas_addr(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u8_t dev_slot_num, 
    fbe_u64_t *sas_address)
{
    fbe_status_t status = FBE_STATUS_OK;

    *sas_address = 0;
    status = terminator_get_sas_address_by_slot_number_and_virtual_phy_handle(
        virtual_phy_handle, dev_slot_num, sas_address);
    return(status);
}

fbe_status_t addl_elem_stat_page_phy_desc_get_phy_id(
    fbe_u8_t dev_slot_num, fbe_u8_t *phy_id)
{
    // use array or spec to get mapping as per the spec in future.
    *phy_id = dev_slot_num;

    return(FBE_STATUS_OK);

}

/* The below PAGE SIZE functions are outdated. 
 * Should be modified in the near future.
 */
fbe_u32_t addl_elem_stat_page_size(fbe_sas_enclosure_type_t encl_type)
{
    switch(encl_type)
    {
        case FBE_SAS_ENCLOSURE_TYPE_VIPER:
        case FBE_SAS_ENCLOSURE_TYPE_BUNKER:
        case FBE_SAS_ENCLOSURE_TYPE_CITADEL:
        case FBE_SAS_ENCLOSURE_TYPE_DERRINGER:    
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM:
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE:
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP:
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP:
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP:
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP:
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP:
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP:
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
            return(FBE_ESES_ADDL_ELEM_STATUS_DESC_HDR_SIZE + 
                FBE_ESES_ARRAY_DEV_SLOT_PROT_SPEC_INFO_HEADER_SIZE + 
                sizeof(ses_array_dev_phy_desc_struct));
            break;
            // others here
        default:
            break;
    }
    return(0);
}

