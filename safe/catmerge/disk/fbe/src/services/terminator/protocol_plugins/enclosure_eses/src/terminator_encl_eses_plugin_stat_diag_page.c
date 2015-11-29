/***************************************************************************
 *  terminator_encl_eses_plugin_stat_diag_page.c
 ***************************************************************************
 *
 *  Description
 *      Routines related to enclosure status diagnostic page.
 *      
 *
 *  History:
 *      Oct-31-08 -- Rajesh V Split the Enclosure status diagnostic page
 *                          functions into seperate file.
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
static fbe_status_t enclosure_status_diagnostic_page_build_status_elements(
    fbe_u8_t *encl_stat_diag_page_start_ptr, 
    fbe_u8_t **stat_elem_end_ptr, 
    fbe_terminator_device_ptr_t virtual_phy_handle);
static fbe_status_t enclosure_status_diagnostic_page_build_device_slot_status_elements(
    fbe_u8_t *device_slot_status_elements_start_ptr, 
    fbe_u8_t **device_slot_status_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle);
static fbe_status_t enclosure_status_diagnostic_page_build_exp_phy_status_elements(
    fbe_u8_t *exp_phy_status_elements_start_ptr, 
    fbe_u8_t **exp_phy_status_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle);
static fbe_status_t enclosure_status_diagnostic_page_build_local_sas_conn_status_elements(
    fbe_u8_t *sas_conn_status_elements_start_ptr, 
    fbe_u8_t **sas_conn_status_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle);
static fbe_status_t enclosure_status_diagnostic_page_build_peer_sas_conn_status_elements(
    fbe_u8_t *sas_conn_status_elements_start_ptr, 
    fbe_u8_t **sas_conn_status_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle);
static fbe_status_t encl_stat_diag_page_sas_conn_elem_get_conn_phyical_link(
    fbe_u8_t index, 
    fbe_sas_enclosure_type_t encl_type,
    fbe_u8_t *conn_physical_link);
static fbe_status_t encl_stat_diag_page_sas_conn_elem_get_sas_conn_status(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u8_t index,
    ses_stat_elem_sas_conn_struct *sas_conn_stat_ptr);
static fbe_status_t encl_stat_diag_page_exp_phy_elem_get_exp_index(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u8_t position,
    fbe_u8_t phy_id,
    fbe_u8_t *exp_index);
static fbe_status_t enclosure_status_diagnostic_page_build_sas_exp_status_elements(
    fbe_u8_t *sas_exp_status_elements_start_ptr, 
    fbe_u8_t **sas_exp_status_elements_end_ptr);
static fbe_u32_t enclosure_status_diagnostic_page_size(fbe_sas_enclosure_type_t encl_type);

/* power supply status element related functions */
static fbe_status_t enclosure_status_diagnostic_page_build_psa0_status_elements(
    fbe_u8_t *sas_ps_elements_start_ptr, 
    fbe_u8_t **sas_ps_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle);
static fbe_status_t enclosure_status_diagnostic_page_build_psb0_status_elements(
    fbe_u8_t *sas_ps_elements_start_ptr, 
    fbe_u8_t **sas_ps_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle);
static fbe_status_t enclosure_status_diagnostic_page_build_psa1_status_elements(
    fbe_u8_t *sas_ps_elements_start_ptr, 
    fbe_u8_t **sas_ps_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle);
static fbe_status_t enclosure_status_diagnostic_page_build_psb1_status_elements(
    fbe_u8_t *sas_ps_elements_start_ptr, 
    fbe_u8_t **sas_ps_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle);
/* end of power supply status element related functions */


/* power supply status element related functions */
static fbe_status_t enclosure_status_diagnostic_page_build_psa_status_elements(
    fbe_u8_t *sas_ps_elements_start_ptr, 
    fbe_u8_t **sas_ps_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle);
static fbe_status_t enclosure_status_diagnostic_page_build_psb_status_elements(
    fbe_u8_t *sas_ps_elements_start_ptr, 
    fbe_u8_t **sas_ps_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle);
/* end of power supply status element related functions */

/* cooling element related functions */
static fbe_status_t enclosure_status_diagnostic_page_build_psa_cooling_status_elements(
    fbe_u8_t *sas_cooling_elements_start_ptr, 
    fbe_u8_t **sas_cooling_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle);
static fbe_status_t enclosure_status_diagnostic_page_build_psb_cooling_status_elements(
    fbe_u8_t *sas_cooling_elements_start_ptr, 
    fbe_u8_t **sas_cooling_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle);

/* End of cooling element related functions */

/* temperature sensor related functions */
static fbe_status_t enclosure_status_diagnostic_page_build_local_temp_sensor_status_elements(
    fbe_u8_t *sas_temp_sensor_elements_start_ptr, 
    fbe_u8_t **sas_temp_sensor_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle);
static fbe_status_t enclosure_status_diagnostic_page_build_chassis_temp_sensor_status_elements(
    fbe_u8_t *sas_temp_sensor_elements_start_ptr, 
    fbe_u8_t **sas_temp_sensor_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle);
static fbe_status_t enclosure_status_diagnostic_page_build_peer_temp_sensor_status_elements(
    fbe_u8_t *sas_temp_sensor_elements_start_ptr, 
    fbe_u8_t **sas_temp_sensor_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle);

static fbe_status_t 
enclosure_status_diagnostic_page_build_local_ps_temp_sensor_status_elements(
    fbe_u8_t *sas_temp_sensor_elements_start_ptr, 
    fbe_u8_t **sas_temp_sensor_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle);
static fbe_status_t 
enclosure_status_diagnostic_page_build_peer_ps_temp_sensor_status_elements(
    fbe_u8_t *sas_temp_sensor_elements_start_ptr, 
    fbe_u8_t **sas_temp_sensor_elements_end_ptr);

/* End of temperature sensor related fucntions */

fbe_status_t enclosure_status_diagnostic_page_build_chassis_encl_status_elements(
    fbe_u8_t *encl_stat_elem_start_ptr, 
    fbe_u8_t **encl_stat_elem_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle);
fbe_status_t enclosure_status_diagnostic_page_build_local_encl_status_elements(
    fbe_u8_t *encl_stat_elem_start_ptr, 
    fbe_u8_t **encl_stat_elem_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle);
fbe_status_t enclosure_status_diagnostic_page_build_peer_encl_status_elements(
    fbe_u8_t *encl_stat_elem_start_ptr, 
    fbe_u8_t **encl_stat_elem_end_ptr);
fbe_status_t enclosure_status_diagnostic_page_build_ee_encl_status_elements(
    fbe_u8_t *encl_stat_elem_start_ptr, 
    fbe_u8_t **encl_stat_elem_end_ptr);
fbe_status_t enclosure_status_diagnostic_page_build_ext_cooling_status_elements(
                                                fbe_u8_t *encl_stat_elem_start_ptr, 
                                                fbe_u8_t **encl_stat_elem_end_ptr,
                                                fbe_terminator_device_ptr_t virtual_phy_handle,
                                                fbe_u8_t  fanSlot);
fbe_status_t enclosure_status_diagnostic_page_build_bem_cooling_status_elements(
    fbe_u8_t *encl_stat_elem_start_ptr, 
    fbe_u8_t **encl_stat_elem_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle);

fbe_status_t enclosure_status_diagnostic_page_build_chassis_cooling_status_elements(
                                                fbe_u8_t *encl_stat_elem_start_ptr, 
                                                fbe_u8_t **encl_stat_elem_end_ptr,
                                                fbe_terminator_device_ptr_t virtual_phy_handle);

static fbe_status_t enclosure_status_diagnostic_page_build_display0_status_elements(fbe_u8_t *display_stat_elem_start_ptr, 
                                                                            fbe_u8_t **display_stat_elem_end_ptr,
                                                                            fbe_terminator_device_ptr_t virtual_phy_handle);
static fbe_status_t enclosure_status_diagnostic_page_build_display1_status_elements(fbe_u8_t *display_stat_elem_start_ptr, 
                                                                            fbe_u8_t **display_stat_elem_end_ptr,
                                                                            fbe_terminator_device_ptr_t virtual_phy_handle);





/*********************************************************************
* build_enclosure_status_diagnostic_page()
*********************************************************************
*
*  Description: This builds the Enclosure Status diagnostic page.
*
*  Inputs: 
*   Sg List and Virtual Phy object handle.
*
*  Return Value: success or failure
*
*  Notes: 
*
*  History:
*    Aug08  Rajesh V. created
*    
*********************************************************************/
fbe_status_t build_enclosure_status_diagnostic_page(
    fbe_sg_element_t * sg_list, 
    fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_pg_encl_stat_struct  *encl_status_diag_page_hdr = NULL;
    fbe_u8_t *encl_stat_diag_page_start_ptr = NULL; // point to first overall status element
    fbe_u8_t *stat_elem_end_ptr = NULL;
    fbe_u16_t encl_stat_page_size = 0;
    fbe_sas_enclosure_type_t encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);

    if(fbe_sg_element_count(sg_list) < enclosure_status_diagnostic_page_size(encl_type))
    {
        return(status);
    }
    encl_status_diag_page_hdr = (ses_pg_encl_stat_struct *)fbe_sg_element_address(sg_list);
    encl_status_diag_page_hdr->pg_code = SES_PG_CODE_ENCL_STAT;
    // ignore these for now.
    encl_status_diag_page_hdr->unrecov = 0;
    encl_status_diag_page_hdr->crit = 0;
    encl_status_diag_page_hdr->non_crit = 0;
    // for now ignore info, as anyhow the client compares each poll response
    // and takes action in casae of a change. if this is ignored generation
    // code is also ignored.
    encl_status_diag_page_hdr->info = 0;

    status = config_page_get_gen_code(virtual_phy_handle , &encl_status_diag_page_hdr->gen_code);
    RETURN_ON_ERROR_STATUS;
    encl_status_diag_page_hdr->gen_code = BYTE_SWAP_32(encl_status_diag_page_hdr->gen_code);

    // ignore for now,, he he ;-) I like these words
    encl_status_diag_page_hdr->invop = 0;

    // Get the page size from the config page being used.
    status = config_page_get_encl_stat_diag_page_size(virtual_phy_handle, &encl_stat_page_size);
    RETURN_ON_ERROR_STATUS;

    encl_status_diag_page_hdr->pg_len = encl_stat_page_size - 4;
    encl_status_diag_page_hdr->pg_len = BYTE_SWAP_16(encl_status_diag_page_hdr->pg_len);

    encl_stat_diag_page_start_ptr = ((fbe_u8_t *)encl_status_diag_page_hdr);

    status = enclosure_status_diagnostic_page_build_status_elements(encl_stat_diag_page_start_ptr, 
                                                                    &stat_elem_end_ptr, 
                                                                    virtual_phy_handle);
    return(status);
    // page length can be obtained by subtracting status_elements_end_ptr from
    // status_elements_start_ptr. keep for future.
}

/*********************************************************************
*  enclosure_status_diagnostic_page_build_status_elements ()
*********************************************************************
*
*  Description: This builds the Status elements for the encl 
*   status diagnostic page. 
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of status elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of all status elements.
*   enclosure information, virtual phy device id and port number.
*
*  Return Value: success or failure
*
*  Notes: 
*   We also return peer LCC connector elements
*
*  History:
*    Aug08 created
*    Sep08 Start using offsets--etc from Configuration Diag Page module
*    
*********************************************************************/
fbe_status_t enclosure_status_diagnostic_page_build_status_elements(
    fbe_u8_t *encl_stat_diag_page_start_ptr, 
    fbe_u8_t **stat_elem_end_ptr, 
    fbe_terminator_device_ptr_t virtual_phy_handle)
{

    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t *stat_elem_start_ptr = encl_stat_diag_page_start_ptr;
    //This contains the start offset of the particular element set as 
    //dictated by configuration page.
    fbe_u16_t stat_elem_byte_offset = 0;
    terminator_sp_id_t spid;
    fbe_u32_t slot_id;
    fbe_u8_t max_lcc_count = 0;
    fbe_u8_t max_ee_lcc_count = 0;
    fbe_u8_t max_ext_cooling_elems_count = 0;
    fbe_sas_enclosure_type_t  encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    status =  fbe_terminator_api_get_sp_id(&spid);

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }
    // Build Array device slot status elements.
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
                                                            &stat_elem_byte_offset);
    
    
    if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
       (status != FBE_STATUS_OK))
    {
        // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
        // does not have the particular element. This is legal as for ex: we dont have
        // PS elems in the configuration page for magnum as they are monitored by specl
        // on magnum DPE and not monitored by the "ESES talking EMA".
        return(status);
    }
    else if(status == FBE_STATUS_OK)
    {

        stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
        status = enclosure_status_diagnostic_page_build_device_slot_status_elements(stat_elem_start_ptr, 
                                                                                    stat_elem_end_ptr, 
                                                                                    virtual_phy_handle);
        RETURN_ON_ERROR_STATUS;
    }

    // Build Expander Phy status elements.
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
                                                            &stat_elem_byte_offset);


    if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
       (status != FBE_STATUS_OK))
    {
        // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
        // does not have the particular element.
        return(status);
    }
    else if(status == FBE_STATUS_OK)
    {

        stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
        status = enclosure_status_diagnostic_page_build_exp_phy_status_elements(stat_elem_start_ptr, 
                                                                                stat_elem_end_ptr, 
                                                                                virtual_phy_handle);
        RETURN_ON_ERROR_STATUS;
    }



    // Build Local SAS Connector status elements.
    status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_LCC,
                                                            spid, 
                                                            SES_ELEM_TYPE_SAS_CONN, 
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &stat_elem_byte_offset);

    
    if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
       (status != FBE_STATUS_OK))
    {
        // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
        // does not have the particular element.
        return(status);
    }
    else if(status == FBE_STATUS_OK)
    {
        stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
        status = enclosure_status_diagnostic_page_build_local_sas_conn_status_elements(stat_elem_start_ptr, 
                                                                                    stat_elem_end_ptr, 
                                                                                    virtual_phy_handle);
        RETURN_ON_ERROR_STATUS;
    }



    // Build Peer SAS Connector status elements.

    status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_LCC,
                                                            !spid, 
                                                            SES_ELEM_TYPE_SAS_CONN, 
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &stat_elem_byte_offset);


    if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
       (status != FBE_STATUS_OK))
    {
        // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
        // does not have the particular element.
        return(status);
    }
    else if(status == FBE_STATUS_OK)
    {
        stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
        status = enclosure_status_diagnostic_page_build_peer_sas_conn_status_elements(stat_elem_start_ptr, 
                                                                                  stat_elem_end_ptr, 
                                                                                  virtual_phy_handle);
        RETURN_ON_ERROR_STATUS;
    }

    // Build local SAS Exp status elements.
    status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_LCC,
                                                            spid, 
                                                            SES_ELEM_TYPE_SAS_EXP, 
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &stat_elem_byte_offset);

   
    if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
       (status != FBE_STATUS_OK))
    {
        // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
        // does not have the particular element.
        return(status);
    }
    else if(status == FBE_STATUS_OK)
    {
        stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
        status = enclosure_status_diagnostic_page_build_sas_exp_status_elements(stat_elem_start_ptr, 
                                                                                stat_elem_end_ptr);
        RETURN_ON_ERROR_STATUS;
    }

    // Build peer SAS Exp status elements.
    status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_LCC,
                                                            !spid, 
                                                            SES_ELEM_TYPE_SAS_EXP, 
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &stat_elem_byte_offset);

   
    if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
       (status != FBE_STATUS_OK))
    {
        // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
        // does not have the particular element.
        return(status);
    }
    else if(status == FBE_STATUS_OK)
    {
        stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
        status = enclosure_status_diagnostic_page_build_sas_exp_status_elements(stat_elem_start_ptr, 
                                                                                stat_elem_end_ptr);
        RETURN_ON_ERROR_STATUS;
    }

    if ( (encl_type == FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP) ||
         (encl_type == FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP) )
    {
        // Build PS A1 status element
        status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                                SES_SUBENCL_TYPE_PS,
                                                                spid, 
                                                                SES_ELEM_TYPE_PS, 
                                                                FALSE,
                                                                0,
                                                                TRUE,
                                                                3,
                                                                FALSE,
                                                                NULL,
                                                                &stat_elem_byte_offset);
        
        if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
           (status != FBE_STATUS_OK))
        {
            // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
            // does not have the particular element.
            return(status);
        }
        else if(status == FBE_STATUS_OK)
        {
            stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
            status = enclosure_status_diagnostic_page_build_psa1_status_elements(stat_elem_start_ptr, 
                                                                                stat_elem_end_ptr, 
                                                                                virtual_phy_handle);
            RETURN_ON_ERROR_STATUS;  
        }
        
        // Build PS B1 status element
        status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                                SES_SUBENCL_TYPE_PS,
                                                                !spid, 
                                                                SES_ELEM_TYPE_PS, 
                                                                FALSE,
                                                                0,
                                                                TRUE,
                                                                4,
                                                                FALSE,
                                                                NULL,
                                                                &stat_elem_byte_offset);
        
        if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
           (status != FBE_STATUS_OK))
        {
            // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
            // does not have the particular element.
            return(status);
        }
        else if(status == FBE_STATUS_OK)
        {
            stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
            status = enclosure_status_diagnostic_page_build_psb1_status_elements(stat_elem_start_ptr, 
                                                                                stat_elem_end_ptr, 
                                                                                virtual_phy_handle);
            RETURN_ON_ERROR_STATUS;  
        }
        
        // Build PS A0 status element
        status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                                SES_SUBENCL_TYPE_PS,
                                                                spid, 
                                                                SES_ELEM_TYPE_PS, 
                                                                FALSE,
                                                                0,
                                                                TRUE,
                                                                5,
                                                                FALSE,
                                                                NULL,
                                                                &stat_elem_byte_offset);
        
        if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
           (status != FBE_STATUS_OK))
        {
            // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
            // does not have the particular element.
            return(status);
        }
        else if(status == FBE_STATUS_OK)
        {
            stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
            status = enclosure_status_diagnostic_page_build_psa0_status_elements(stat_elem_start_ptr, 
                                                                                stat_elem_end_ptr, 
                                                                                virtual_phy_handle);
            RETURN_ON_ERROR_STATUS;  
        }
        
        // Build PS B0 status element.
        status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                                SES_SUBENCL_TYPE_PS,
                                                                !spid, 
                                                                SES_ELEM_TYPE_PS, 
                                                                FALSE,
                                                                0,
                                                                TRUE,
                                                                6,
                                                                FALSE,
                                                                NULL,
                                                                &stat_elem_byte_offset);
        
        if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
           (status != FBE_STATUS_OK))
        {
            // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
            // does not have the particular element.
            return(status);
        }
        else if(status == FBE_STATUS_OK)
        {
            stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
            status = enclosure_status_diagnostic_page_build_psb0_status_elements(stat_elem_start_ptr, 
                                                                                stat_elem_end_ptr, 
                                                                                virtual_phy_handle);
            RETURN_ON_ERROR_STATUS; 
        }
    }
    else
    {
        // Build PS A status element
        status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                                SES_SUBENCL_TYPE_PS,
                                                                spid, 
                                                                SES_ELEM_TYPE_PS, 
                                                                FALSE,
                                                                0,
                                                                FALSE,
                                                                0,
                                                                FALSE,
                                                                NULL,
                                                                &stat_elem_byte_offset);
        
        if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
           (status != FBE_STATUS_OK))
        {
            // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
            // does not have the particular element.
            return(status);
        }
        else if(status == FBE_STATUS_OK)
        {
            stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
            status = enclosure_status_diagnostic_page_build_psa_status_elements(stat_elem_start_ptr, 
                                                                                stat_elem_end_ptr, 
                                                                                virtual_phy_handle);
            RETURN_ON_ERROR_STATUS;  
        }
        
        
        // Build PS B status element.
        status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                                SES_SUBENCL_TYPE_PS,
                                                                !spid, 
                                                                SES_ELEM_TYPE_PS, 
                                                                FALSE,
                                                                0,
                                                                FALSE,
                                                                0,
                                                                FALSE,
                                                                NULL,
                                                                &stat_elem_byte_offset);
        
        if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
           (status != FBE_STATUS_OK))
        {
            // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
            // does not have the particular element.
            return(status);
        }
        else if(status == FBE_STATUS_OK)
        {
            stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
            status = enclosure_status_diagnostic_page_build_psb_status_elements(stat_elem_start_ptr, 
                                                                                stat_elem_end_ptr, 
                                                                                virtual_phy_handle);
            RETURN_ON_ERROR_STATUS; 
        }
    }


    //Build PS A cooling elements
    status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_PS,
                                                            spid, 
                                                            SES_ELEM_TYPE_COOLING, 
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &stat_elem_byte_offset);
   
    if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
       (status != FBE_STATUS_OK))
    {
        // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
        // does not have the particular element.
        return(status);
    }
    else if(status == FBE_STATUS_OK)
    {
        stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
        status = enclosure_status_diagnostic_page_build_psa_cooling_status_elements(stat_elem_start_ptr, 
                                                                                    stat_elem_end_ptr, 
                                                                                    virtual_phy_handle);
        RETURN_ON_ERROR_STATUS;  
    }



    //Build PS B cooling elements
   
    status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_PS,
                                                            !spid, 
                                                            SES_ELEM_TYPE_COOLING, 
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &stat_elem_byte_offset);


    if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
       (status != FBE_STATUS_OK))
    {
        // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
        // does not have the particular element.
        return(status);
    }
    else if(status == FBE_STATUS_OK)
    {
        stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
        status = enclosure_status_diagnostic_page_build_psb_cooling_status_elements(stat_elem_start_ptr, 
                                                                                    stat_elem_end_ptr, 
                                                                                    virtual_phy_handle);
        RETURN_ON_ERROR_STATUS;  
    }

    //Build Local Temperature Sensor elements
    status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_LCC,
                                                            (encl_type == FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM)? 2:0, 
                                                            SES_ELEM_TYPE_TEMP_SENSOR, 
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &stat_elem_byte_offset);

    if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
       (status != FBE_STATUS_OK))
    {
        // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
        // does not have the particular element.
        return(status);
    }
    else if(status == FBE_STATUS_OK)
    {
        stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
        status = enclosure_status_diagnostic_page_build_local_temp_sensor_status_elements(stat_elem_start_ptr, 
                                                                                        stat_elem_end_ptr, 
                                                                                        virtual_phy_handle);
        RETURN_ON_ERROR_STATUS;  
    }

    //Build Peer Temperature Sensor elements
    status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_LCC,
                                                            (encl_type == FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM)? 3:1, 
                                                            SES_ELEM_TYPE_TEMP_SENSOR, 
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &stat_elem_byte_offset);

     if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
       (status != FBE_STATUS_OK))
    {
        // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
        // does not have the particular element.
        return(status);
    }
    else if(status == FBE_STATUS_OK)
    {
        stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
        status = enclosure_status_diagnostic_page_build_peer_temp_sensor_status_elements(stat_elem_start_ptr, 
                                                                                         stat_elem_end_ptr,
                                                                                         virtual_phy_handle);
        RETURN_ON_ERROR_STATUS; 
    }


    //Build Chassis Temperature Sensor elements
    status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_CHASSIS,
                                                            SIDE_UNDEFINED, 
                                                            SES_ELEM_TYPE_TEMP_SENSOR, 
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &stat_elem_byte_offset);
     if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
       (status != FBE_STATUS_OK))
    {
        // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
        // does not have the particular element.
        return(status);
    }
    else if(status == FBE_STATUS_OK)
    {
        stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
        status = enclosure_status_diagnostic_page_build_chassis_temp_sensor_status_elements(stat_elem_start_ptr, 
                                                                                            stat_elem_end_ptr,
                                                                                            virtual_phy_handle);
        RETURN_ON_ERROR_STATUS; 
    }

    //Build chassis Enclosure element (encl element in CHASSIS Subenclosure)
    status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_CHASSIS,
                                                            SIDE_UNDEFINED, 
                                                            SES_ELEM_TYPE_ENCL, 
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &stat_elem_byte_offset);
    if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
       (status != FBE_STATUS_OK))
    {
        // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
        // does not have the particular element.
        return(status);
    }
    else if(status == FBE_STATUS_OK)
    {
        stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
        status = enclosure_status_diagnostic_page_build_chassis_encl_status_elements(stat_elem_start_ptr, 
                                                                                    stat_elem_end_ptr, 
                                                                                    virtual_phy_handle);
        RETURN_ON_ERROR_STATUS;
    }



    //Build local lcc Enclosure element  (encl element in LOCAL LCC Subenclosure)
    status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_LCC,
                                                            spid, 
                                                            SES_ELEM_TYPE_ENCL, 
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &stat_elem_byte_offset);

    if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
       (status != FBE_STATUS_OK))
    {
        // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
        // does not have the particular element.
        return(status);
    }
    else if(status == FBE_STATUS_OK)
    {
        stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
        status = enclosure_status_diagnostic_page_build_local_encl_status_elements(stat_elem_start_ptr, 
                                                                                stat_elem_end_ptr, 
                                                                                virtual_phy_handle);
        RETURN_ON_ERROR_STATUS;
    }


    //Build Peer lcc Enclosure element (encl element in PEER LCC Subenclosure)
    status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_LCC,
                                                            !spid, 
                                                            SES_ELEM_TYPE_ENCL, 
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &stat_elem_byte_offset);

    if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
       (status != FBE_STATUS_OK))
    {
        // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
        // does not have the particular element.
        return(status);
    }
    else if(status == FBE_STATUS_OK)
    {
        stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
        status = enclosure_status_diagnostic_page_build_peer_encl_status_elements(stat_elem_start_ptr, 
                                                                                stat_elem_end_ptr);
        RETURN_ON_ERROR_STATUS;
    }

    status = terminator_max_lccs(encl_type, &max_lcc_count);
    status = terminator_max_ee_lccs(encl_type, &max_ee_lcc_count);

    //Build ee lcc Enclosure element 
    for(slot_id = (max_lcc_count - max_ee_lcc_count); slot_id < max_lcc_count; slot_id++) 
    {             
        status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                         SES_SUBENCL_TYPE_LCC,
                                                         slot_id, 
                                                         SES_ELEM_TYPE_ENCL, 
                                                         FALSE,
                                                         0,
                                                         FALSE,
                                                         0,
                                                         FALSE,
                                                         NULL,
                                                         &stat_elem_byte_offset);

        if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
           (status != FBE_STATUS_OK))
        {
            // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
            // does not have the particular element.
            return(status);
        }
        else if(status == FBE_STATUS_OK)
        {
            stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
            status = enclosure_status_diagnostic_page_build_ee_encl_status_elements(stat_elem_start_ptr, 
                                                                                stat_elem_end_ptr);
            RETURN_ON_ERROR_STATUS;
        }
    }

    //Build ext fan Enclosure element 
    status = terminator_max_ext_cooling_elems(encl_type, &max_ext_cooling_elems_count);
 
    for(slot_id = 0; slot_id < max_ext_cooling_elems_count; slot_id++) 
    {
        status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                         SES_SUBENCL_TYPE_COOLING,
                                                         slot_id, 
                                                         SES_ELEM_TYPE_COOLING, 
                                                         FALSE,
                                                         0,
                                                         FALSE,
                                                         0,
                                                         FALSE,
                                                         NULL,
                                                         &stat_elem_byte_offset);

        if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
           (status != FBE_STATUS_OK))
        {
            // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
            // does not have the particular element.
            return(status);
        }
        else if(status == FBE_STATUS_OK)
        {
            stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
            status = enclosure_status_diagnostic_page_build_ext_cooling_status_elements(stat_elem_start_ptr, 
                                                                                stat_elem_end_ptr, 
                                                                                virtual_phy_handle,
                                                                                slot_id);
            RETURN_ON_ERROR_STATUS;
        } 
    }
    
    //Build BEM fan Enclosure element 
        status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                         SES_SUBENCL_TYPE_LCC,
                                                         spid, 
                                                         SES_ELEM_TYPE_COOLING, 
                                                         FALSE,
                                                         0,
                                                         FALSE,
                                                         0,
                                                         FALSE,
                                                         NULL,
                                                         &stat_elem_byte_offset);

        if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
           (status != FBE_STATUS_OK))
        {
            // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
            // does not have the particular element.
            return(status);
        }
        else if(status == FBE_STATUS_OK)
        {
            stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
            status = enclosure_status_diagnostic_page_build_bem_cooling_status_elements(stat_elem_start_ptr, 
                                                                                stat_elem_end_ptr,
                                                                                virtual_phy_handle);
            RETURN_ON_ERROR_STATUS;
        }
 
    //Build Chassis Cooling status
        status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                         SES_SUBENCL_TYPE_CHASSIS,
                                                         SIDE_UNDEFINED, 
                                                         SES_ELEM_TYPE_COOLING, 
                                                         FALSE,
                                                         0,
                                                         FALSE,
                                                         0,
                                                         FALSE,
                                                         NULL,
                                                         &stat_elem_byte_offset);

        if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
           (status != FBE_STATUS_OK))
        {
            // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
            // does not have the particular element.
            return(status);
        }
        else if(status == FBE_STATUS_OK)
        {
            stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
            status = enclosure_status_diagnostic_page_build_chassis_cooling_status_elements(stat_elem_start_ptr, 
                                                                                stat_elem_end_ptr,
                                                                                virtual_phy_handle);
            RETURN_ON_ERROR_STATUS;
        }
    // NOTE: In future, all the below display functions should be implemented
    // in a single function. This may be useful as we get enclosures with
    // more than 2 displays and different characters in each display. The single
    // function should determine these display parameters and builds display elements
    // based on the enclosures display properties.
    
    // Build the display elements for the first physical DISPLAY on enclosure.

    status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_LCC,
                                                            spid, 
                                                            SES_ELEM_TYPE_DISPLAY, 
                                                            TRUE,
                                                            2,
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &stat_elem_byte_offset);

    
    if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
       (status != FBE_STATUS_OK))
    {
        // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
        // does not have the particular element.
        return(status);
    }
    else if(status == FBE_STATUS_OK)
    {
        stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
        status = enclosure_status_diagnostic_page_build_display0_status_elements(stat_elem_start_ptr, 
                                                                                stat_elem_end_ptr,
                                                                                virtual_phy_handle);
        RETURN_ON_ERROR_STATUS;
    }


    // Build the display elements for the second physical DISPLAY on enclosure.
    status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_LCC,
                                                            spid, 
                                                            SES_ELEM_TYPE_DISPLAY, 
                                                            TRUE,
                                                            1,
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &stat_elem_byte_offset);

   
    if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
       (status != FBE_STATUS_OK))
    {
        // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
        // does not have the particular element.
        return(status);
    }
    else if(status == FBE_STATUS_OK)
    {
        stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
        status = enclosure_status_diagnostic_page_build_display1_status_elements(stat_elem_start_ptr, 
                                                                                stat_elem_end_ptr,
                                                                                virtual_phy_handle);
        RETURN_ON_ERROR_STATUS;
    }

/*******************************************************
    //Build Local PS Temperature Sensor elements
    status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_PS,
                                                            spid, 
                                                            SES_ELEM_TYPE_TEMP_SENSOR, 
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &stat_elem_byte_offset);

    if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
       (status != FBE_STATUS_OK))
    {
        // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
        // does not have the particular element.
        return(status);
    }
    else if(status == FBE_STATUS_OK)
    {
        stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
        status = enclosure_status_diagnostic_page_build_local_ps_temp_sensor_status_elements(stat_elem_start_ptr, 
                                                                                        stat_elem_end_ptr, 
                                                                                        virtual_phy_handle);
        RETURN_ON_ERROR_STATUS;  
    }

    //Build Peer PS Temperature Sensor elements
    status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_PS,
                                                            !spid, 
                                                            SES_ELEM_TYPE_TEMP_SENSOR, 
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &stat_elem_byte_offset);

     if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
       (status != FBE_STATUS_OK))
    {
        // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
        // does not have the particular element.
        return(status);
    }
    else if(status == FBE_STATUS_OK)
    {
        stat_elem_start_ptr = encl_stat_diag_page_start_ptr + stat_elem_byte_offset;
        status = enclosure_status_diagnostic_page_build_peer_ps_temp_sensor_status_elements(stat_elem_start_ptr, 
                                                                                        stat_elem_end_ptr);
        RETURN_ON_ERROR_STATUS; 
    }
 
*********************************************************/

    return(FBE_STATUS_OK);

}

/*********************************************************************
*  enclosure_status_diagnostic_page_build_sas_exp_status_elements ()
*********************************************************************
*
*  Description: This builds the SAS expander elements for the encl 
*   status diagnostic page. 
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of expander elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the expander elements.
*   enclosure information, virtual phy device id and port number.
*
*  Return Value: success or failure
*
*  Notes: 
*   There is only one sas exp element.
*
*  History:
*    Aug08  created
*    
*********************************************************************/
fbe_status_t enclosure_status_diagnostic_page_build_sas_exp_status_elements(
    fbe_u8_t *sas_exp_status_elements_start_ptr, 
    fbe_u8_t **sas_exp_status_elements_end_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_sas_exp_struct *individual_sas_exp_stat_ptr = NULL;
    ses_stat_elem_sas_exp_struct *overall_sas_exp_stat_ptr = NULL;
    
    overall_sas_exp_stat_ptr = (ses_stat_elem_sas_exp_struct *)sas_exp_status_elements_start_ptr; 
    // for now all of fields are ignored in overall status element
    memset (overall_sas_exp_stat_ptr, 0, sizeof(ses_stat_elem_sas_exp_struct));
    individual_sas_exp_stat_ptr = (overall_sas_exp_stat_ptr + 1); 
    memset (individual_sas_exp_stat_ptr, 0, sizeof(ses_stat_elem_sas_exp_struct));
    individual_sas_exp_stat_ptr->cmn_stat.elem_stat_code = SES_STAT_CODE_OK;

    *sas_exp_status_elements_end_ptr = (fbe_u8_t *)(individual_sas_exp_stat_ptr + 1);

    return(status);
}

/*********************************************************************
*  enclosure_status_diagnostic_page_build_local_sas_conn_status_elements ()
*********************************************************************
*
*  Description: This builds the local SAS connector elements for the encl 
*   status diagnostic page. (LOCAL LCC)
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of expander elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the expander elements.
*   enclosure information, virtual phy device id and port number.
*
*  Return Value: success or failure
*
*  Notes: 
*
*  History:
*    Aug08  created
*    
*********************************************************************/
fbe_status_t enclosure_status_diagnostic_page_build_local_sas_conn_status_elements(
    fbe_u8_t *sas_conn_status_elements_start_ptr, 
    fbe_u8_t **sas_conn_status_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_sas_conn_struct *individual_sas_conn_stat_ptr = NULL;
    ses_stat_elem_sas_conn_struct *overall_sas_conn_stat_ptr = NULL;
    fbe_u8_t max_conns = 0;
    fbe_u8_t i;
    fbe_sas_enclosure_type_t            encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);

    overall_sas_conn_stat_ptr = (ses_stat_elem_sas_conn_struct *)sas_conn_status_elements_start_ptr; 

    // for now all of fields are ignored in overall status element
    memset (overall_sas_conn_stat_ptr, 0, sizeof(ses_stat_elem_sas_conn_struct));

    individual_sas_conn_stat_ptr = overall_sas_conn_stat_ptr;  
    status = terminator_max_conns_per_lcc(encl_type, &max_conns);
    RETURN_ON_ERROR_STATUS;

    for(i=0; i<max_conns; i++)
    {
        individual_sas_conn_stat_ptr++; 
        memset(individual_sas_conn_stat_ptr, 0, sizeof(ses_stat_elem_sas_conn_struct));
        status = encl_stat_diag_page_sas_conn_elem_get_conn_phyical_link(i, 
            encl_type, &individual_sas_conn_stat_ptr->conn_physical_link);
        RETURN_ON_ERROR_STATUS;
        /* Get the conn status from terminator */
        status = encl_stat_diag_page_sas_conn_elem_get_sas_conn_status(virtual_phy_handle, i, individual_sas_conn_stat_ptr);
        if(status != FBE_STATUS_OK)
        {
            return(status);
        }

    }
    *sas_conn_status_elements_end_ptr = (fbe_u8_t *)( individual_sas_conn_stat_ptr + 1);

    return(status);    
}



/*********************************************************************
*  enclosure_status_diagnostic_page_build_peer_sas_conn_status_elements ()
*********************************************************************
*
*  Description: This builds the peer SAS connector elements for the encl 
*   status diagnostic page. (PEER LCC)
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of expander elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the expander elements.
*   enclosure information, virtual phy device id and port number.
*
*  Return Value: success or failure
*
*  Notes: 
*   We assume peeer lcc is not installed and fill its conn elem.
*
*  History:
*    Aug08  created
*    
*********************************************************************/
fbe_status_t enclosure_status_diagnostic_page_build_peer_sas_conn_status_elements(
    fbe_u8_t *sas_conn_status_elements_start_ptr, 
    fbe_u8_t **sas_conn_status_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_sas_conn_struct *individual_sas_conn_stat_ptr = NULL;
    ses_stat_elem_sas_conn_struct *overall_sas_conn_stat_ptr = NULL;
    fbe_u8_t max_total_conns = 0;
    fbe_u8_t max_conns_per_port = 0;
    fbe_u8_t i;
    fbe_sas_enclosure_type_t            encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);

    overall_sas_conn_stat_ptr = (ses_stat_elem_sas_conn_struct *)sas_conn_status_elements_start_ptr; 

    // for now all of fields are ignored in overall status element
    memset (overall_sas_conn_stat_ptr, 0, sizeof(ses_stat_elem_sas_conn_struct));

    individual_sas_conn_stat_ptr = overall_sas_conn_stat_ptr;  

    status = terminator_max_conns_per_port(encl_type, &max_conns_per_port);
    RETURN_ON_ERROR_STATUS;

    if (max_conns_per_port == 0)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: max connections per port is zero.\n",
                         __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = terminator_max_conns_per_lcc(encl_type, &max_total_conns);
    RETURN_ON_ERROR_STATUS;

    // Fill in the peer LCC SAS connector element statuses. 
    // There is also a overall SAS connector element as the peer lcc is a
    // seperate subenclosure. For now assume that peer lcc is not inserted.
    for(i=0; i<max_total_conns; i++)
    {

        individual_sas_conn_stat_ptr++; 
        memset(individual_sas_conn_stat_ptr, 0, sizeof(ses_stat_elem_sas_conn_struct));
        individual_sas_conn_stat_ptr->conn_type = 0x2;

        encl_stat_diag_page_sas_conn_elem_get_conn_phyical_link(i, 
                                                                encl_type, 
                                                                &individual_sas_conn_stat_ptr->conn_physical_link); 
    
        individual_sas_conn_stat_ptr->cmn_stat.elem_stat_code = SES_STAT_CODE_NOT_INSTALLED;
    }
    *sas_conn_status_elements_end_ptr = (fbe_u8_t *)( individual_sas_conn_stat_ptr + 1);    

    return(status);    
}

/*********************************************************************
* enclosure_status_diagnostic_page_build_exp_phy_status_elements()
*********************************************************************
*
*  Description: This builds the peer Phy status elements for the encl 
*   status diagnostic page. 
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of expander elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the expander elements.
*   enclosure information, virtual phy device id and port number.
*
*  Return Value: success or failure
*
*  Notes: 
*  
*
*  History:
*    Aug08  created
*    
*********************************************************************/

fbe_status_t enclosure_status_diagnostic_page_build_exp_phy_status_elements(
    fbe_u8_t *exp_phy_status_elements_start_ptr, 
    fbe_u8_t **exp_phy_status_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_exp_phy_struct *individual_exp_phy_stat_ptr = NULL;
    ses_stat_elem_exp_phy_struct *overall_exp_phy_stat_ptr = NULL;
    fbe_u8_t max_phy_slots = 0;
    fbe_u8_t i;
    fbe_sas_enclosure_type_t            encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    overall_exp_phy_stat_ptr = (ses_stat_elem_exp_phy_struct *)exp_phy_status_elements_start_ptr; 

    // set the fields in expander phy status element
    // for now all of fields are ignored in overall status element
    memset (overall_exp_phy_stat_ptr, 0, sizeof(ses_stat_elem_exp_phy_struct));

    individual_exp_phy_stat_ptr = overall_exp_phy_stat_ptr;  
    status = terminator_max_phys(encl_type, &max_phy_slots);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    for(i=0; i<max_phy_slots; i++)
    {
        individual_exp_phy_stat_ptr++; 
        memset(individual_exp_phy_stat_ptr, 0, sizeof(ses_stat_elem_exp_phy_struct));
        /* Get the phy status page for terminator */
        status = terminator_get_phy_eses_status(virtual_phy_handle, i, individual_exp_phy_stat_ptr);
        RETURN_ON_ERROR_STATUS;
        status = encl_stat_diag_page_exp_phy_elem_get_exp_index(virtual_phy_handle,
                                                                i,
                                                                individual_exp_phy_stat_ptr->phy_id, 
                                                                &individual_exp_phy_stat_ptr->exp_index);
        RETURN_ON_ERROR_STATUS;

    }
    *exp_phy_status_elements_end_ptr = (fbe_u8_t *)(individual_exp_phy_stat_ptr + 1);   

    return(status);
}

/*********************************************************************
* enclosure_status_diagnostic_page_build_device_slot_status_elements()
*********************************************************************
*
*  Description: This builds the Array device slot status elements for 
*   the encl status diagnostic page. 
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of expander elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the expander elements.
*   enclosure information, virtual phy device id and port number.
*
*  Return Value: success or failure
*
*  Notes: 
*  
*
*  History:
*    Aug08  created
*    
*********************************************************************/
fbe_status_t enclosure_status_diagnostic_page_build_device_slot_status_elements(
    fbe_u8_t *device_slot_status_elements_start_ptr, 
    fbe_u8_t **device_slot_status_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_array_dev_slot_struct *individual_array_dev_slot_stat_ptr = NULL;
    ses_stat_elem_array_dev_slot_struct *overall_array_dev_slot_stat_ptr = NULL;
    fbe_u8_t max_drive_slots = 0;
    fbe_u32_t i;
    fbe_sas_enclosure_type_t  encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    overall_array_dev_slot_stat_ptr = (ses_stat_elem_array_dev_slot_struct *)device_slot_status_elements_start_ptr; 

    // set the fields in overall device slot status element
    // for now all of fields are ignored in overall status element
    memset (overall_array_dev_slot_stat_ptr, 0, sizeof(ses_stat_elem_array_dev_slot_struct));

    individual_array_dev_slot_stat_ptr = overall_array_dev_slot_stat_ptr;  
    status = terminator_max_drive_slots(encl_type, &max_drive_slots);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    for(i=0; i<max_drive_slots; i++)
    {
        individual_array_dev_slot_stat_ptr++; 
        memset(individual_array_dev_slot_stat_ptr, 0, sizeof(ses_stat_elem_array_dev_slot_struct));
        /* Get the drive status page from terminator */
        terminator_get_drive_eses_status(virtual_phy_handle, i, individual_array_dev_slot_stat_ptr);
    }
    *device_slot_status_elements_end_ptr = (fbe_u8_t *)(individual_array_dev_slot_stat_ptr + 1);   

    return(status);
}

fbe_status_t 
enclosure_status_diagnostic_page_build_psa0_status_elements(
    fbe_u8_t *sas_ps_elements_start_ptr, 
    fbe_u8_t **sas_ps_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_ps_struct *individual_ps_stat_ptr = NULL;
    ses_stat_elem_ps_struct *overall_ps_stat_ptr = NULL;
    
    overall_ps_stat_ptr = (ses_stat_elem_ps_struct *)sas_ps_elements_start_ptr; 
    // for now all of fields are ignored in overall status element
    memset (overall_ps_stat_ptr, 0, sizeof(ses_stat_elem_ps_struct));
    status = terminator_get_ps_eses_status(virtual_phy_handle, PS_1, overall_ps_stat_ptr);
    RETURN_ON_ERROR_STATUS;

    individual_ps_stat_ptr = (overall_ps_stat_ptr + 1); 
    memset (individual_ps_stat_ptr, 0, sizeof(ses_stat_elem_ps_struct));
    status = terminator_get_ps_eses_status(virtual_phy_handle, PS_1, individual_ps_stat_ptr);
    RETURN_ON_ERROR_STATUS;

    individual_ps_stat_ptr = (overall_ps_stat_ptr + 2); 
    memset (individual_ps_stat_ptr, 0, sizeof(ses_stat_elem_ps_struct));
    status = terminator_get_ps_eses_status(virtual_phy_handle, PS_1, individual_ps_stat_ptr);
    RETURN_ON_ERROR_STATUS;

    *sas_ps_elements_end_ptr = (fbe_u8_t *)(individual_ps_stat_ptr + 1);

    return(status);    
}

fbe_status_t 
enclosure_status_diagnostic_page_build_psa1_status_elements(
    fbe_u8_t *sas_ps_elements_start_ptr, 
    fbe_u8_t **sas_ps_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_ps_struct *individual_ps_stat_ptr = NULL;
    ses_stat_elem_ps_struct *overall_ps_stat_ptr = NULL;
    
    overall_ps_stat_ptr = (ses_stat_elem_ps_struct *)sas_ps_elements_start_ptr; 
    // for now all of fields are ignored in overall status element
    memset (overall_ps_stat_ptr, 0, sizeof(ses_stat_elem_ps_struct));
    status = terminator_get_ps_eses_status(virtual_phy_handle, PS_0, overall_ps_stat_ptr);
    RETURN_ON_ERROR_STATUS;

    individual_ps_stat_ptr = (overall_ps_stat_ptr + 1); 
    memset (individual_ps_stat_ptr, 0, sizeof(ses_stat_elem_ps_struct));
    status = terminator_get_ps_eses_status(virtual_phy_handle, PS_0, individual_ps_stat_ptr);
    RETURN_ON_ERROR_STATUS;

    individual_ps_stat_ptr = (overall_ps_stat_ptr + 2); 
    memset (individual_ps_stat_ptr, 0, sizeof(ses_stat_elem_ps_struct));
    status = terminator_get_ps_eses_status(virtual_phy_handle, PS_0, individual_ps_stat_ptr);
    RETURN_ON_ERROR_STATUS;

    *sas_ps_elements_end_ptr = (fbe_u8_t *)(individual_ps_stat_ptr + 1);

    return(status);    
}

fbe_status_t 
enclosure_status_diagnostic_page_build_psb0_status_elements(
    fbe_u8_t *sas_ps_elements_start_ptr, 
    fbe_u8_t **sas_ps_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_ps_struct *individual_ps_stat_ptr = NULL;
    ses_stat_elem_ps_struct *overall_ps_stat_ptr = NULL;
    
    overall_ps_stat_ptr = (ses_stat_elem_ps_struct *)sas_ps_elements_start_ptr; 
    // for now all of fields are ignored in overall status element
    memset (overall_ps_stat_ptr, 0, sizeof(ses_stat_elem_ps_struct));
    status = terminator_get_ps_eses_status(virtual_phy_handle, PS_3, overall_ps_stat_ptr);
    RETURN_ON_ERROR_STATUS;

    individual_ps_stat_ptr = (overall_ps_stat_ptr + 1); 
    memset (individual_ps_stat_ptr, 0, sizeof(ses_stat_elem_ps_struct));
    status = terminator_get_ps_eses_status(virtual_phy_handle, PS_3, individual_ps_stat_ptr);
    RETURN_ON_ERROR_STATUS;

    individual_ps_stat_ptr = (overall_ps_stat_ptr + 2); 
    memset (individual_ps_stat_ptr, 0, sizeof(ses_stat_elem_ps_struct));
    status = terminator_get_ps_eses_status(virtual_phy_handle, PS_3, individual_ps_stat_ptr);
    RETURN_ON_ERROR_STATUS;

    *sas_ps_elements_end_ptr = (fbe_u8_t *)(individual_ps_stat_ptr + 1);

    return(status);    
}

fbe_status_t 
enclosure_status_diagnostic_page_build_psb1_status_elements(
    fbe_u8_t *sas_ps_elements_start_ptr, 
    fbe_u8_t **sas_ps_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_ps_struct *individual_ps_stat_ptr = NULL;
    ses_stat_elem_ps_struct *overall_ps_stat_ptr = NULL;
    
    overall_ps_stat_ptr = (ses_stat_elem_ps_struct *)sas_ps_elements_start_ptr; 
    // for now all of fields are ignored in overall status element
    memset (overall_ps_stat_ptr, 0, sizeof(ses_stat_elem_ps_struct));
    status = terminator_get_ps_eses_status(virtual_phy_handle, PS_2, overall_ps_stat_ptr);
    RETURN_ON_ERROR_STATUS;

    individual_ps_stat_ptr = (overall_ps_stat_ptr + 1); 
    memset (individual_ps_stat_ptr, 0, sizeof(ses_stat_elem_ps_struct));
    status = terminator_get_ps_eses_status(virtual_phy_handle, PS_2, individual_ps_stat_ptr);
    RETURN_ON_ERROR_STATUS;

    individual_ps_stat_ptr = (overall_ps_stat_ptr + 2); 
    memset (individual_ps_stat_ptr, 0, sizeof(ses_stat_elem_ps_struct));
    status = terminator_get_ps_eses_status(virtual_phy_handle, PS_2, individual_ps_stat_ptr);
    RETURN_ON_ERROR_STATUS;

    *sas_ps_elements_end_ptr = (fbe_u8_t *)(individual_ps_stat_ptr + 1);

    return(status);    
}

/*********************************************************************
* enclosure_status_diagnostic_page_build_psa_status_elements ()
*********************************************************************
*
*  Description: This builds the Power Supply(A) status 
*   elements in the encl status diagnostic page. 
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of Power Supply elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the Power Supply elements.
*   Virtual phy handle.
*
*  Return Value: success or failure
*
*  Notes: 
*
*  History:
*   Oct-??-2008 -- Rajesh V Created.
*    
*********************************************************************/
fbe_status_t 
enclosure_status_diagnostic_page_build_psa_status_elements(
    fbe_u8_t *sas_ps_elements_start_ptr, 
    fbe_u8_t **sas_ps_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_ps_struct *individual_ps_stat_ptr = NULL;
    ses_stat_elem_ps_struct *overall_ps_stat_ptr = NULL;

    overall_ps_stat_ptr = (ses_stat_elem_ps_struct *)sas_ps_elements_start_ptr; 
    // for now all of fields are ignored in overall status element
    memset (overall_ps_stat_ptr, 0, sizeof(ses_stat_elem_ps_struct));
    status = terminator_get_ps_eses_status(virtual_phy_handle, PS_0, overall_ps_stat_ptr);
    RETURN_ON_ERROR_STATUS;

    individual_ps_stat_ptr = (overall_ps_stat_ptr + 1); 
    memset (individual_ps_stat_ptr, 0, sizeof(ses_stat_elem_ps_struct));
    status = terminator_get_ps_eses_status(virtual_phy_handle, PS_0, individual_ps_stat_ptr);
    RETURN_ON_ERROR_STATUS;
    *sas_ps_elements_end_ptr = (fbe_u8_t *)(individual_ps_stat_ptr + 1);

    return(status);    
}

/*********************************************************************
* enclosure_status_diagnostic_page_build_psb_status_elements ()
*********************************************************************
*
*  Description: This builds the Power Supply(B) status 
*   elements in the encl status diagnostic page. 
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of Power Supply elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the Power Supply elements.
*   Virtual phy handle.
*
*  Return Value: success or failure
*
*  Notes: 
*
*  History:
*   Oct-??-2008 -- Rajesh V Created.
*    
*********************************************************************/
fbe_status_t enclosure_status_diagnostic_page_build_psb_status_elements(
    fbe_u8_t *sas_ps_elements_start_ptr, 
    fbe_u8_t **sas_ps_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_ps_struct *individual_ps_stat_ptr = NULL;
    ses_stat_elem_ps_struct *overall_ps_stat_ptr = NULL;
    
    overall_ps_stat_ptr = (ses_stat_elem_ps_struct *)sas_ps_elements_start_ptr; 
    // for now all of fields are ignored in overall status element
    memset (overall_ps_stat_ptr, 0, sizeof(ses_stat_elem_ps_struct));
    status = terminator_get_ps_eses_status(virtual_phy_handle, PS_1, overall_ps_stat_ptr);
    RETURN_ON_ERROR_STATUS;


    individual_ps_stat_ptr = (overall_ps_stat_ptr + 1); 
    memset (individual_ps_stat_ptr, 0, sizeof(ses_stat_elem_ps_struct));
    status = terminator_get_ps_eses_status(virtual_phy_handle, PS_1, individual_ps_stat_ptr);
    RETURN_ON_ERROR_STATUS;
    *sas_ps_elements_end_ptr = (fbe_u8_t *)(individual_ps_stat_ptr + 1);

    return(status);    
}

/*********************************************************************
* enclosure_status_diagnostic_page_build_psa_cooling_status_elements ()
*********************************************************************
*
*  Description: This builds the Power Supply(A) Cooling status 
*   elements in the encl status diagnostic page. 
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of Cooling elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the cooling elements.
*   Virtual phy handle.
*
*  Return Value: success or failure
*
*  Notes: 
*
*  History:
*   Oct-??-2008 -- Rajesh V Created.
*    
*********************************************************************/
fbe_status_t enclosure_status_diagnostic_page_build_psa_cooling_status_elements(
    fbe_u8_t *sas_cooling_elements_start_ptr, 
    fbe_u8_t **sas_cooling_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_cooling_struct *individual_cooling_stat_ptr = NULL;
    ses_stat_elem_cooling_struct *overall_cooling_stat_ptr = NULL;
    
    overall_cooling_stat_ptr = (ses_stat_elem_cooling_struct *)sas_cooling_elements_start_ptr; 
    // for now all of fields are ignored in overall status element
    memset (overall_cooling_stat_ptr, 0, sizeof(ses_stat_elem_cooling_struct));

    individual_cooling_stat_ptr = (overall_cooling_stat_ptr + 1); 
    memset (individual_cooling_stat_ptr, 0, sizeof(ses_stat_elem_cooling_struct));
    status = terminator_get_cooling_eses_status(virtual_phy_handle, COOLING_0, individual_cooling_stat_ptr);
    RETURN_ON_ERROR_STATUS;

    individual_cooling_stat_ptr = (overall_cooling_stat_ptr + 2); 
    memset (individual_cooling_stat_ptr, 0, sizeof(ses_stat_elem_cooling_struct));
    status = terminator_get_cooling_eses_status(virtual_phy_handle, COOLING_1, individual_cooling_stat_ptr);
    RETURN_ON_ERROR_STATUS;

    *sas_cooling_elements_end_ptr = (fbe_u8_t *)(individual_cooling_stat_ptr + 1);
    return(status);    
}

/*********************************************************************
* enclosure_status_diagnostic_page_build_psb_cooling_status_elements ()
*********************************************************************
*
*  Description: This builds the Power Supply(B) Cooling status 
*   elements in the encl status diagnostic page. 
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of cooling elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the cooling elements.
*   Virtual phy handle.
*
*  Return Value: success or failure
*
*  Notes: 
*
*  History:
*   Oct-??-2008 -- Rajesh V Created.
*    
*********************************************************************/
fbe_status_t 
enclosure_status_diagnostic_page_build_psb_cooling_status_elements(
    fbe_u8_t *sas_cooling_elements_start_ptr, 
    fbe_u8_t **sas_cooling_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_cooling_struct *individual_cooling_stat_ptr = NULL;
    ses_stat_elem_cooling_struct *overall_cooling_stat_ptr = NULL;
    
    overall_cooling_stat_ptr = (ses_stat_elem_cooling_struct *)sas_cooling_elements_start_ptr; 
    // for now all of fields are ignored in overall status element
    memset (overall_cooling_stat_ptr, 0, sizeof(ses_stat_elem_cooling_struct));

    individual_cooling_stat_ptr = (overall_cooling_stat_ptr + 1); 
    memset (individual_cooling_stat_ptr, 0, sizeof(ses_stat_elem_cooling_struct));
    status = terminator_get_cooling_eses_status(virtual_phy_handle, COOLING_2, individual_cooling_stat_ptr);
    RETURN_ON_ERROR_STATUS;

    individual_cooling_stat_ptr = (overall_cooling_stat_ptr + 2); 
    memset (individual_cooling_stat_ptr, 0, sizeof(ses_stat_elem_cooling_struct));
    status = terminator_get_cooling_eses_status(virtual_phy_handle, COOLING_3, individual_cooling_stat_ptr);
    RETURN_ON_ERROR_STATUS;

    *sas_cooling_elements_end_ptr = (fbe_u8_t *)(individual_cooling_stat_ptr + 1);
    return(status);    
}


/*********************************************************************
* enclosure_status_diagnostic_page_build_local_temp_sensor_status_elements ()
*********************************************************************
*
*  Description: This builds the Local Temperature sensor status 
*   elements in the encl status diagnostic page. 
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of temp sensor elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the temp sensor elements.
*   Virtual phy handle.
*
*  Return Value: success or failure
*
*  Notes: 
*
*  History:
*   Oct-??-2008 -- Rajesh V Created.
*    
*********************************************************************/
fbe_status_t 
enclosure_status_diagnostic_page_build_local_temp_sensor_status_elements(
    fbe_u8_t *sas_temp_sensor_elements_start_ptr, 
    fbe_u8_t **sas_temp_sensor_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_temp_sensor_struct *individual_temp_sensor_stat_ptr = NULL;
    ses_stat_elem_temp_sensor_struct *overall_temp_sensor_stat_ptr = NULL;
    fbe_u8_t max_temp_sensor_elems_per_lcc = 0;
    fbe_sas_enclosure_type_t            encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    fbe_u8_t i;

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    
    overall_temp_sensor_stat_ptr = (ses_stat_elem_temp_sensor_struct *)sas_temp_sensor_elements_start_ptr; 
    memset (overall_temp_sensor_stat_ptr, 0, sizeof(ses_stat_elem_temp_sensor_struct));
    status = terminator_get_overall_temp_sensor_eses_status(virtual_phy_handle, TEMP_SENSOR_0, overall_temp_sensor_stat_ptr);
    RETURN_ON_ERROR_STATUS;

    individual_temp_sensor_stat_ptr = overall_temp_sensor_stat_ptr;  
    status = terminator_max_temp_sensors_per_lcc(encl_type, &max_temp_sensor_elems_per_lcc);
    RETURN_ON_ERROR_STATUS;

    for(i=0; i<max_temp_sensor_elems_per_lcc; i++)
    {
        individual_temp_sensor_stat_ptr++; 
        memset(individual_temp_sensor_stat_ptr, 0, sizeof(ses_stat_elem_sas_conn_struct));
        status = terminator_get_temp_sensor_eses_status(virtual_phy_handle, TEMP_SENSOR_0, individual_temp_sensor_stat_ptr);
        RETURN_ON_ERROR_STATUS;
    }

    *sas_temp_sensor_elements_end_ptr = (fbe_u8_t *)(individual_temp_sensor_stat_ptr + 1);
    return(status);    
}

/*********************************************************************
* enclosure_status_diagnostic_page_build_peer_temp_sensor_status_elements ()
*********************************************************************
*
*  Description: This builds the Peer Temperature sensor status 
*   elements in the encl status diagnostic page. 
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of temp sensor elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the temp sensor elements.
*   Virtual phy handle.
*
*  Return Value: success or failure
*
*  Notes: 
*
*  History:
*   Oct-??-2008 -- Rajesh V Created.
*    
*********************************************************************/
fbe_status_t 
enclosure_status_diagnostic_page_build_peer_temp_sensor_status_elements(
    fbe_u8_t *sas_temp_sensor_elements_start_ptr, 
    fbe_u8_t **sas_temp_sensor_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_temp_sensor_struct *individual_temp_sensor_stat_ptr = NULL;
    ses_stat_elem_temp_sensor_struct *overall_temp_sensor_stat_ptr = NULL;
    fbe_u8_t max_temp_sensor_elems_per_lcc = 0;
    fbe_sas_enclosure_type_t            encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    fbe_u8_t i;

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    
    overall_temp_sensor_stat_ptr = (ses_stat_elem_temp_sensor_struct *)sas_temp_sensor_elements_start_ptr; 
    memset (overall_temp_sensor_stat_ptr, 0, sizeof(ses_stat_elem_temp_sensor_struct));
    // As peer LCC(containing subenclosure for temp sensor) is assumed NOT INSTALLED
    overall_temp_sensor_stat_ptr->cmn_stat.elem_stat_code = SES_STAT_CODE_NOT_INSTALLED;

    individual_temp_sensor_stat_ptr = overall_temp_sensor_stat_ptr;  
    status = terminator_max_temp_sensors_per_lcc(encl_type, &max_temp_sensor_elems_per_lcc);
    RETURN_ON_ERROR_STATUS;

    for(i=0; i<max_temp_sensor_elems_per_lcc; i++)
    {
        individual_temp_sensor_stat_ptr++; 
        memset (individual_temp_sensor_stat_ptr, 0, sizeof(ses_stat_elem_temp_sensor_struct));
        // As peer LCC(containing subenclosure for temp sensor) is assumed NOT INSTALLED 
        individual_temp_sensor_stat_ptr->cmn_stat.elem_stat_code = SES_STAT_CODE_NOT_INSTALLED;
        individual_temp_sensor_stat_ptr->ot_failure = 0;
        individual_temp_sensor_stat_ptr->ot_warning = 0;
    }

    *sas_temp_sensor_elements_end_ptr = (fbe_u8_t *)(individual_temp_sensor_stat_ptr + 1);
    return(status);    
}


/*********************************************************************
* enclosure_status_diagnostic_page_build_local_ps_temp_sensor_status_elements ()
*********************************************************************
*
*  Description: This builds the Local Power Supply Temperature sensor status 
*   elements in the encl status diagnostic page. (Power Supply Subenclosure)
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of temp sensor elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the temp sensor elements.
*   Virtual phy handle.
*
*  Return Value: success or failure
*
*  Notes: 
*
*  History:
*   Dec-6-2010 -- Rajesh V Created.
*    
*********************************************************************/
fbe_status_t 
enclosure_status_diagnostic_page_build_local_ps_temp_sensor_status_elements(
    fbe_u8_t *sas_temp_sensor_elements_start_ptr, 
    fbe_u8_t **sas_temp_sensor_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_temp_sensor_struct *individual_temp_sensor_stat_ptr = NULL;
    ses_stat_elem_temp_sensor_struct *overall_temp_sensor_stat_ptr = NULL;
    
    overall_temp_sensor_stat_ptr = (ses_stat_elem_temp_sensor_struct *)sas_temp_sensor_elements_start_ptr; 
    memset (overall_temp_sensor_stat_ptr, 0, sizeof(ses_stat_elem_temp_sensor_struct));
    overall_temp_sensor_stat_ptr->cmn_stat.elem_stat_code = SES_STAT_CODE_OK;

    individual_temp_sensor_stat_ptr = (overall_temp_sensor_stat_ptr + 1); 
    memset (individual_temp_sensor_stat_ptr, 0, sizeof(ses_stat_elem_temp_sensor_struct));
    individual_temp_sensor_stat_ptr->cmn_stat.elem_stat_code = SES_STAT_CODE_OK;

    individual_temp_sensor_stat_ptr ++;
    memset (individual_temp_sensor_stat_ptr, 0, sizeof(ses_stat_elem_temp_sensor_struct));
    individual_temp_sensor_stat_ptr->cmn_stat.elem_stat_code = SES_STAT_CODE_OK;

    *sas_temp_sensor_elements_end_ptr = (fbe_u8_t *)(individual_temp_sensor_stat_ptr + 1);
    return(status);    
}

/*********************************************************************
* enclosure_status_diagnostic_page_build_peer_ps_temp_sensor_status_elements ()
*********************************************************************
*
*  Description: This builds the Peer Power Supply Temperature sensor status 
*   elements in the encl status diagnostic page. (Power Supply Subenclosure)
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of temp sensor elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the temp sensor elements.
*   Virtual phy handle.
*
*  Return Value: success or failure
*
*  Notes: 
*
*  History:
*   Dec-6-2010 -- Rajesh V Created.
*    
*********************************************************************/
fbe_status_t 
enclosure_status_diagnostic_page_build_peer_ps_temp_sensor_status_elements(
    fbe_u8_t *sas_temp_sensor_elements_start_ptr, 
    fbe_u8_t **sas_temp_sensor_elements_end_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_temp_sensor_struct *individual_temp_sensor_stat_ptr = NULL;
    ses_stat_elem_temp_sensor_struct *overall_temp_sensor_stat_ptr = NULL;
    
    overall_temp_sensor_stat_ptr = (ses_stat_elem_temp_sensor_struct *)sas_temp_sensor_elements_start_ptr; 
    memset (overall_temp_sensor_stat_ptr, 0, sizeof(ses_stat_elem_temp_sensor_struct));
    overall_temp_sensor_stat_ptr->cmn_stat.elem_stat_code = SES_STAT_CODE_OK;

    individual_temp_sensor_stat_ptr = (overall_temp_sensor_stat_ptr + 1); 
    memset (individual_temp_sensor_stat_ptr, 0, sizeof(ses_stat_elem_temp_sensor_struct));
    individual_temp_sensor_stat_ptr->cmn_stat.elem_stat_code = SES_STAT_CODE_OK;

    individual_temp_sensor_stat_ptr ++;
    memset (individual_temp_sensor_stat_ptr, 0, sizeof(ses_stat_elem_temp_sensor_struct));
    individual_temp_sensor_stat_ptr->cmn_stat.elem_stat_code = SES_STAT_CODE_OK;

    *sas_temp_sensor_elements_end_ptr = (fbe_u8_t *)(individual_temp_sensor_stat_ptr + 1);
    return(status);    
}


/*********************************************************************
*  enclosure_status_diagnostic_page_build_chassis_encl_status_elements ()
*********************************************************************
*
*  Description: This builds the Chassis encl status elements in the encl 
*   status diagnostic page. 
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of encl elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the encl elements.
*   Virtual phy handle.
*
*  Return Value: success or failure
*
*  Notes: 
*   There is only one encl status element.
*
*  History:
*   Oct-31-2008 -- Rajesh V Created.
*    
*********************************************************************/

fbe_status_t enclosure_status_diagnostic_page_build_chassis_encl_status_elements(
    fbe_u8_t *encl_stat_elem_start_ptr, 
    fbe_u8_t **encl_stat_elem_end_ptr,
     fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_encl_struct *individual_encl_stat_ptr = NULL;
    ses_stat_elem_encl_struct *overall_encl_stat_ptr = NULL;
    
    overall_encl_stat_ptr = (ses_stat_elem_encl_struct *)encl_stat_elem_start_ptr; 
    // for now all of fields are ignored in overall status element
    memset (overall_encl_stat_ptr, 0, sizeof(ses_stat_elem_encl_struct));

    individual_encl_stat_ptr = (overall_encl_stat_ptr + 1); 
    status = terminator_get_chassis_encl_eses_status(virtual_phy_handle, individual_encl_stat_ptr);
    RETURN_ON_ERROR_STATUS;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s:Raj:Stat page Chassis STAT elem, failure_rqsted set to %d \n",
                        __FUNCTION__, individual_encl_stat_ptr->failure_rqsted);

    *encl_stat_elem_end_ptr = (fbe_u8_t *)(individual_encl_stat_ptr + 1);
    return(status);    
}

/*********************************************************************
*  enclosure_status_diagnostic_page_build_local_encl_status_elements ()
*********************************************************************
*
*  Description: This builds the local LCC's encl status elements in 
*   Encl status diagnostic page. 
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of encl elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the encl elements.
*   Virtual phy handle.
*
*  Return Value: success or failure
*
*  Notes: 
*   There is only one encl status element.
*
*  History:
*   Oct-31-2008 -- Rajesh V Created.
*    
*********************************************************************/
fbe_status_t enclosure_status_diagnostic_page_build_local_encl_status_elements(
    fbe_u8_t *encl_stat_elem_start_ptr, 
    fbe_u8_t **encl_stat_elem_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_encl_struct *individual_encl_stat_ptr = NULL;
    ses_stat_elem_encl_struct *overall_encl_stat_ptr = NULL;
    
    overall_encl_stat_ptr = (ses_stat_elem_encl_struct *)encl_stat_elem_start_ptr; 
    // for now all of fields are ignored in overall status element
    memset (overall_encl_stat_ptr, 0, sizeof(ses_stat_elem_encl_struct));

    individual_encl_stat_ptr = (overall_encl_stat_ptr + 1); 
    status = terminator_get_encl_eses_status(virtual_phy_handle, individual_encl_stat_ptr);
    RETURN_ON_ERROR_STATUS;

    *encl_stat_elem_end_ptr = (fbe_u8_t *)(individual_encl_stat_ptr + 1);
    return(status);    
}

/*********************************************************************
*  enclosure_status_diagnostic_page_build_peer_encl_status_elements ()
*********************************************************************
*
*  Description: This builds the Peer LCC's encl status elements in 
*   Encl status diagnostic page. 
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of encl elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the encl elements.
*   Virtual phy handle.
*
*  Return Value: success or failure
*
*  Notes: 
*   There is only one encl status element.
*
*  History:
*   Oct-31-2008 -- Rajesh V Created.
*    
*********************************************************************/
fbe_status_t enclosure_status_diagnostic_page_build_peer_encl_status_elements(
    fbe_u8_t *encl_stat_elem_start_ptr, 
    fbe_u8_t **encl_stat_elem_end_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_encl_struct *individual_encl_stat_ptr = NULL;
    ses_stat_elem_encl_struct *overall_encl_stat_ptr = NULL;
    
    overall_encl_stat_ptr = (ses_stat_elem_encl_struct *)encl_stat_elem_start_ptr; 
    // for now all of fields are ignored in overall status element
    memset (overall_encl_stat_ptr, 0, sizeof(ses_stat_elem_encl_struct));

    individual_encl_stat_ptr = (overall_encl_stat_ptr + 1); 
    memset (individual_encl_stat_ptr, 0, sizeof(ses_stat_elem_encl_struct));
    individual_encl_stat_ptr->cmn_stat.elem_stat_code = SES_STAT_CODE_OK;

    *encl_stat_elem_end_ptr = (fbe_u8_t *)(individual_encl_stat_ptr + 1);
    return(status);    
}

/*********************************************************************
*  enclosure_status_diagnostic_page_build_ee_encl_status_elements ()
*********************************************************************
*
*   Description: This builds the EE LCC's encl status elements in 
*                Encl status diagnostic page. 
*
*   Inputs: 
*   status_elements_start_ptr - pointer to the start of encl elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*                             end address of the encl elements.
*   Virtual phy handle.
*
*   Return Value: success or failure
*
*   Notes: 
*   
*
*   History:
*   Jun-03-2011 -- Rashmi Sawale Created.
*    
*********************************************************************/
fbe_status_t enclosure_status_diagnostic_page_build_ee_encl_status_elements(fbe_u8_t *encl_stat_elem_start_ptr, 
                                                                            fbe_u8_t **encl_stat_elem_end_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_encl_struct *individual_encl_stat_ptr = NULL;
    ses_stat_elem_encl_struct *overall_encl_stat_ptr = NULL;
    
    overall_encl_stat_ptr = (ses_stat_elem_encl_struct *)encl_stat_elem_start_ptr; 
    memset (overall_encl_stat_ptr, 0, sizeof(ses_stat_elem_encl_struct));
    overall_encl_stat_ptr->cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
        
    individual_encl_stat_ptr = (overall_encl_stat_ptr + 1); 
    memset (individual_encl_stat_ptr, 0, sizeof(ses_stat_elem_encl_struct));
    individual_encl_stat_ptr->cmn_stat.elem_stat_code = SES_STAT_CODE_OK;

    *encl_stat_elem_end_ptr = (fbe_u8_t *)(individual_encl_stat_ptr + 1);
    return(status);    
}

/*********************************************************************
*  enclosure_status_diagnostic_page_build_ext_cooling_status_elements ()
*********************************************************************
*
*   Description: This builds the ext Cooling status elements in 
*                Encl status diagnostic page. 
*
*   Inputs: 
*   status_elements_start_ptr - pointer to the start of encl elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*                             end address of the encl elements.
*   Virtual phy handle.
*
*   Return Value: success or failure
*
*   Notes: 
*   
*
*   History:
*   Jun-03-2011 -- Rashmi Sawale Created.
*    
*********************************************************************/
fbe_status_t enclosure_status_diagnostic_page_build_ext_cooling_status_elements(
                                                fbe_u8_t *encl_stat_elem_start_ptr, 
                                                fbe_u8_t **encl_stat_elem_end_ptr,
                                                fbe_terminator_device_ptr_t virtual_phy_handle,
                                                fbe_u8_t  fanSlot)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_cooling_struct *individual_ext_cooling_stat_ptr = NULL;
    ses_stat_elem_cooling_struct *overall_ext_cooling_stat_ptr = NULL;
    
    overall_ext_cooling_stat_ptr = (ses_stat_elem_cooling_struct *)encl_stat_elem_start_ptr; 
    memset (overall_ext_cooling_stat_ptr, 0, sizeof(ses_stat_elem_cooling_struct));
    status = terminator_get_cooling_eses_status(virtual_phy_handle, fanSlot+COOLING_4, overall_ext_cooling_stat_ptr);
    RETURN_ON_ERROR_STATUS;
   
    individual_ext_cooling_stat_ptr = (overall_ext_cooling_stat_ptr + 1); 
    memset (individual_ext_cooling_stat_ptr, 0, sizeof(ses_stat_elem_encl_struct));
    status = terminator_get_cooling_eses_status(virtual_phy_handle, fanSlot+COOLING_4, individual_ext_cooling_stat_ptr);
    RETURN_ON_ERROR_STATUS;

    *encl_stat_elem_end_ptr = (fbe_u8_t *)(individual_ext_cooling_stat_ptr + 1);
    return(status);
}

/*********************************************************************
*  enclosure_status_diagnostic_page_build_bem_cooling_status_elements ()
*********************************************************************
*
*   Description: This builds the bem Cooling status elements in 
*                Encl status diagnostic page. 
*
*   Inputs: 
*   status_elements_start_ptr - pointer to the start of encl elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*                             end address of the encl elements.
*   Virtual phy handle.
*
*   Return Value: success or failure
*
*   Notes: 
*   
*
*   History:
*   18-07-2012 -- Rui Chang Created.
*    
*********************************************************************/
fbe_status_t enclosure_status_diagnostic_page_build_bem_cooling_status_elements(
                                                fbe_u8_t *encl_stat_elem_start_ptr, 
                                                fbe_u8_t **encl_stat_elem_end_ptr,
                                                fbe_terminator_device_ptr_t virtual_phy_handle)
{

 

    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_cooling_struct *individual_bem_cooling_stat_ptr = NULL;
    ses_stat_elem_cooling_struct *overall_bem_cooling_stat_ptr = NULL;
    fbe_u8_t slot_id=0;
    fbe_u8_t max_bem_cooling_elems_count=0;
    fbe_sas_enclosure_type_t            encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);

    status = terminator_max_bem_cooling_elems(encl_type, &max_bem_cooling_elems_count);
    RETURN_ON_ERROR_STATUS;

    overall_bem_cooling_stat_ptr = (ses_stat_elem_cooling_struct *)encl_stat_elem_start_ptr; 
    memset (overall_bem_cooling_stat_ptr, 0, sizeof(ses_stat_elem_cooling_struct));
    overall_bem_cooling_stat_ptr->cmn_stat.elem_stat_code = SES_STAT_CODE_OK; 

    individual_bem_cooling_stat_ptr = overall_bem_cooling_stat_ptr;  

    for(slot_id = 0; slot_id < max_bem_cooling_elems_count; slot_id++) 
    {
        individual_bem_cooling_stat_ptr ++; 
        memset (individual_bem_cooling_stat_ptr, 0, sizeof(ses_stat_elem_encl_struct));
        status = terminator_get_cooling_eses_status(virtual_phy_handle, slot_id, individual_bem_cooling_stat_ptr);
        RETURN_ON_ERROR_STATUS;
    }

    *encl_stat_elem_end_ptr = (fbe_u8_t *)(individual_bem_cooling_stat_ptr + 1);
    return(status);
}

/*********************************************************************
*  enclosure_status_diagnostic_page_build_bem_cooling_status_elements ()
*********************************************************************
*
*   Description: This builds the chassis cooling status elements in 
*                Encl status diagnostic page. 
*
*   Inputs: 
*   status_elements_start_ptr - pointer to the start of encl elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*                             end address of the encl elements.
*   Virtual phy handle.
*
*   Return Value: success or failure
*
*   Notes: 
*   
*
*   History:
*   25-08-2012 -- Rui Chang Created.
*...12/13/2013 -- zhoue1 Add Viking support
*    
*********************************************************************/
fbe_status_t enclosure_status_diagnostic_page_build_chassis_cooling_status_elements(
                                                fbe_u8_t *encl_stat_elem_start_ptr, 
                                                fbe_u8_t **encl_stat_elem_end_ptr,
                                                fbe_terminator_device_ptr_t virtual_phy_handle)
{

 

    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_cooling_struct individual_bem_cooling_stat = {0};
    ses_stat_elem_cooling_struct *individual_chassis_cooling_stat_ptr = NULL;
    ses_stat_elem_cooling_struct *overall_chassis_cooling_stat_ptr = NULL;
    fbe_u8_t slot_id=0;
    fbe_u8_t max_bem_cooling_elems_count=0;
    fbe_u8_t max_chassis_cooling_elems_count=0;
    fbe_sas_enclosure_type_t            encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    fbe_u8_t  faultBemFanCount=0;

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);

    overall_chassis_cooling_stat_ptr = (ses_stat_elem_cooling_struct *)encl_stat_elem_start_ptr; 
    memset (overall_chassis_cooling_stat_ptr, 0, sizeof(ses_stat_elem_cooling_struct));
            
    /* Jetfire chassis cooling status depend on BEM fan status*/
// ENCL_CLEANUP - needed for Moons?
    if ((encl_type == FBE_SAS_ENCLOSURE_TYPE_FALLBACK) ||
        (encl_type == FBE_SAS_ENCLOSURE_TYPE_CALYPSO))
    {
        status = terminator_max_bem_cooling_elems(encl_type, &max_bem_cooling_elems_count);
        RETURN_ON_ERROR_STATUS;
    
        for(slot_id = 0; slot_id < max_bem_cooling_elems_count; slot_id++) 
        {
            status = terminator_get_cooling_eses_status(virtual_phy_handle, slot_id, &individual_bem_cooling_stat);
            RETURN_ON_ERROR_STATUS;
            if (individual_bem_cooling_stat.cmn_stat.elem_stat_code == SES_STAT_CODE_CRITICAL)
            {
                faultBemFanCount++;
            }
        }
    
        if (faultBemFanCount < 1)
        {
            overall_chassis_cooling_stat_ptr->cmn_stat.elem_stat_code = SES_STAT_CODE_OK; 
        }
        else if (faultBemFanCount == 1)
        {
            overall_chassis_cooling_stat_ptr->cmn_stat.elem_stat_code = SES_STAT_CODE_NONCRITICAL; 
        }
        else
        {
            overall_chassis_cooling_stat_ptr->cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL; 
        }

    }
    else if ((encl_type == FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP) ||
             (encl_type == FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP) ||
              (encl_type == FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP))
    {
        status = terminator_max_ext_cooling_elems(encl_type, &max_chassis_cooling_elems_count);
        RETURN_ON_ERROR_STATUS;
        
        individual_chassis_cooling_stat_ptr = overall_chassis_cooling_stat_ptr;
        status = terminator_get_cooling_eses_status(virtual_phy_handle, slot_id, individual_chassis_cooling_stat_ptr);
        RETURN_ON_ERROR_STATUS;

        for(slot_id = 0; slot_id < max_chassis_cooling_elems_count; slot_id++) 
        {
            individual_chassis_cooling_stat_ptr++;
            memset(individual_chassis_cooling_stat_ptr, 0, sizeof(ses_stat_elem_sas_conn_struct));
            status = terminator_get_cooling_eses_status(virtual_phy_handle, slot_id, individual_chassis_cooling_stat_ptr);
            RETURN_ON_ERROR_STATUS;
        }
    }
    else
    {
            overall_chassis_cooling_stat_ptr->cmn_stat.elem_stat_code = SES_STAT_CODE_OK; 
    }

    *encl_stat_elem_end_ptr = (fbe_u8_t *)(overall_chassis_cooling_stat_ptr + 1);
    return(status);

}

/*********************************************************************
* enclosure_status_diagnostic_page_build_chassis_temp_sensor_status_elements ()
*********************************************************************
*
*  Description: This builds the Chassis Temperature sensor status 
*   elements in the encl status diagnostic page. 
*
*  Inputs: 
*   status_elements_start_ptr - pointer to the start of temp sensor elements
*   status_elements_end_ptr - pointer to be returned, that indicates the
*       end address of the temp sensor elements.
*   Virtual phy handle.
*
*  Return Value: success or failure
*
*  Notes: 
*
*  History:
*   Dec-20-2009 -- Rajesh V Created.
*    
*********************************************************************/
fbe_status_t 
enclosure_status_diagnostic_page_build_chassis_temp_sensor_status_elements(
    fbe_u8_t *sas_temp_sensor_elements_start_ptr, 
    fbe_u8_t **sas_temp_sensor_elements_end_ptr,
    fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_stat_elem_temp_sensor_struct *individual_temp_sensor_stat_ptr = NULL;
    ses_stat_elem_temp_sensor_struct *overall_temp_sensor_stat_ptr = NULL;
    
    overall_temp_sensor_stat_ptr = 
        (ses_stat_elem_temp_sensor_struct *)sas_temp_sensor_elements_start_ptr; 
    memset (overall_temp_sensor_stat_ptr, 
            0, 
            sizeof(ses_stat_elem_temp_sensor_struct));
    overall_temp_sensor_stat_ptr->cmn_stat.elem_stat_code = 
        SES_STAT_CODE_OK;


    individual_temp_sensor_stat_ptr = (overall_temp_sensor_stat_ptr + 1); 
    memset (individual_temp_sensor_stat_ptr, 0, sizeof(ses_stat_elem_temp_sensor_struct));
    individual_temp_sensor_stat_ptr->cmn_stat.elem_stat_code = 
        SES_STAT_CODE_OK;

    /* 
    // Temperature sensor for air inlet temperature.                                                                                                                   ;
    individual_temp_sensor_stat_ptr = (overall_temp_sensor_stat_ptr + 1); 
    memset (individual_temp_sensor_stat_ptr, 0, sizeof(ses_stat_elem_temp_sensor_struct));
    status = terminator_get_temp_sensor_eses_status(virtual_phy_handle,                                                                                                          ;
                                                    TEMP_SENSOR_2,                                                                                                               ;
                                                    individual_temp_sensor_stat_ptr);
    RETURN_ON_ERROR_STATUS; 
    */ 

    *sas_temp_sensor_elements_end_ptr = 
        (fbe_u8_t *)(overall_temp_sensor_stat_ptr + 1);
    return(status);    
}



fbe_status_t enclosure_status_diagnostic_page_build_display0_status_elements(fbe_u8_t *display_stat_elem_start_ptr, 
                                                                            fbe_u8_t **display_stat_elem_end_ptr,
                                                                            fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_stat_elem_display_struct *individual_display_stat_ptr = NULL;
    ses_stat_elem_display_struct *overall_display_stat_ptr = NULL;
    
    overall_display_stat_ptr = (ses_stat_elem_display_struct *)display_stat_elem_start_ptr; 
    // for now all of fields are ignored in overall status element
    memset (overall_display_stat_ptr, 0, sizeof(ses_stat_elem_display_struct));

    // There is a display status element for each display character (One-One mapping)
    individual_display_stat_ptr = (overall_display_stat_ptr + 1); 
    status = terminator_get_display_eses_status(virtual_phy_handle, DISPLAY_CHARACTER_0, individual_display_stat_ptr);
    RETURN_ON_ERROR_STATUS;

    individual_display_stat_ptr ++;
    status = terminator_get_display_eses_status(virtual_phy_handle, DISPLAY_CHARACTER_1, individual_display_stat_ptr);
    RETURN_ON_ERROR_STATUS;


    *display_stat_elem_end_ptr = (fbe_u8_t *)(individual_display_stat_ptr + 1);
    return(FBE_STATUS_OK);    
}

fbe_status_t enclosure_status_diagnostic_page_build_display1_status_elements(fbe_u8_t *display_stat_elem_start_ptr, 
                                                                            fbe_u8_t **display_stat_elem_end_ptr,
                                                                            fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_stat_elem_display_struct *individual_display_stat_ptr = NULL;
    ses_stat_elem_display_struct *overall_display_stat_ptr = NULL;
    
    overall_display_stat_ptr = (ses_stat_elem_display_struct *)display_stat_elem_start_ptr; 
    // for now all of fields are ignored in overall status element
    memset (overall_display_stat_ptr, 0, sizeof(ses_stat_elem_display_struct));

    // There is a display status element for each display character (One-One mapping)
    individual_display_stat_ptr = (overall_display_stat_ptr + 1); 
    status = terminator_get_display_eses_status(virtual_phy_handle, DISPLAY_CHARACTER_2, individual_display_stat_ptr);
    RETURN_ON_ERROR_STATUS;

    *display_stat_elem_end_ptr = (fbe_u8_t *)(individual_display_stat_ptr + 1);
    return(FBE_STATUS_OK);    
}

fbe_status_t encl_stat_diag_page_sas_conn_elem_get_conn_phyical_link(
    fbe_u8_t position, 
    fbe_sas_enclosure_type_t encl_type,
    fbe_u8_t *conn_physical_link)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u8_t max_conns_per_port;
    fbe_u8_t max_conns_per_lcc; 

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
        case FBE_SAS_ENCLOSURE_TYPE_MAGNUM:
        case FBE_SAS_ENCLOSURE_TYPE_BUNKER:
        case FBE_SAS_ENCLOSURE_TYPE_CITADEL:
        case FBE_SAS_ENCLOSURE_TYPE_DERRINGER:
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM:
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE:
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP:
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP:
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
            if((position%max_conns_per_port) == 0)
            {
                *conn_physical_link = 0xFF;
            }
            else 
            {
                *conn_physical_link = ((position%max_conns_per_port)-1);
            }
            break;

        case FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP:
            if((position == 0) || 
               (position == 5) ||
               (position == 10) ||
               (position == 16) ||
               (position == 22) ||
               (position == 28) ||
               (position == 34) ||
               (position == 39)) 
            {
                *conn_physical_link = 0xFF;
            }
            else if(position > 0 && position < 10) 
            {
                *conn_physical_link = (position%5) - 1;
            }
            else if(position > 10 && position < 34) 
            {
                *conn_physical_link = ((position - 10)%6) - 1;
            }
            else if(position > 34 && position < 43) 
            {
                *conn_physical_link = ((position - 34)%5) - 1;
            }
            else
            {
                *conn_physical_link = 0xFF;
            }
            break;

        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP:
            if((position == 0) || 
               (position == 5) ||
               (position == 10) ||
               (position == 15) ||
               (position == 20)) 
            {
                *conn_physical_link = 0xFF;
            }
            else if(position > 0 && position < 20) 
            {
                *conn_physical_link = (position%5) - 1;
            }
            else if(position > 20 && position < 28) 
            {
                *conn_physical_link = ((position - 20)%9) - 1;
            }
            else
            {
                *conn_physical_link = 0xFF;
            }
            break;

        case FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP:
            if((position == 0) || 
               (position == 5) ||
               (position == 10) ||
               (position == 15) ||
               (position == 20) ||
               (position == 29))
            {
                *conn_physical_link = 0xFF;
            }
            else if(position > 0 && position < 20) 
            {
                *conn_physical_link = (position%5) - 1;
            }
            else if(position > 20 && position < 38) 
            {
                *conn_physical_link = ((position - 20)%9) - 1;
            }
            else
            {
                *conn_physical_link = 0xFF;
            }
            break;

        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }
    return(status);

}
    
fbe_status_t encl_stat_diag_page_sas_conn_elem_get_sas_conn_status(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u8_t position,
    ses_stat_elem_sas_conn_struct *sas_conn_stat_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u8_t max_conns_per_port;
    fbe_u8_t max_conns_per_lcc; 
    fbe_bool_t inserted = FBE_FALSE;
    fbe_sas_enclosure_type_t            encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    fbe_u8_t conn_id;
    terminator_conn_map_range_t         range = CONN_IS_UPSTREAM;

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

    inserted = FBE_FALSE;

    switch(encl_type)
    {
        case FBE_SAS_ENCLOSURE_TYPE_DERRINGER:
        case FBE_SAS_ENCLOSURE_TYPE_VIPER:
        case FBE_SAS_ENCLOSURE_TYPE_BUNKER:
        case FBE_SAS_ENCLOSURE_TYPE_CITADEL:
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
            terminator_map_position_max_conns_to_range_conn_id(encl_type, position, max_conns_per_port, &range, &conn_id);
            switch(range)
            {
                case CONN_IS_DOWNSTREAM:
                    // Indicates the connector belongs to the extension port
                    status = terminator_get_downstream_wideport_device_status(virtual_phy_handle, &inserted);    
                    break;
                case CONN_IS_UPSTREAM:
                    // Indicates the connector belongs to the primary port
                    status = terminator_get_upstream_wideport_device_status(virtual_phy_handle, &inserted);
                    break;
                case CONN_IS_RANGE0:
                    if((encl_type == FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM) ||
                        (encl_type == FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP) ||
                        (encl_type == FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP) ||
                        (encl_type == FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP) )
                    {
                        // Indicates the connector external unused.
                        inserted = FALSE;
                        status = FBE_STATUS_OK;
                    }
                    break;
                case CONN_IS_INTERNAL_RANGE1:
                case CONN_IS_INTERNAL_RANGE2:
                    // search child list to match connector id. for EE0 or EE1 (Voyager or Viking) and EE2 or EE3(viking)
                    // The position range for EE0 and EE1 would be from (max_conns* 4) to ((max_conn*6) -1)
                    // The position range for EE2 and EE3 would be from (max_conns* 4) to ((max_conn*6) -1) 
                    status = terminator_get_child_expander_wideport_device_status_by_connector_id(virtual_phy_handle, conn_id, &inserted);
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
            // expansion port connectors.
            status = terminator_get_upstream_wideport_device_status(virtual_phy_handle, &inserted);
            break;

        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }
            

    if(status == FBE_STATUS_OK)
    {
        if(inserted)
        {
            status = terminator_get_sas_conn_eses_status(virtual_phy_handle, position, sas_conn_stat_ptr);
        }
        else
        {
            sas_conn_stat_ptr->cmn_stat.elem_stat_code = SES_STAT_CODE_NOT_INSTALLED;
        }
    }

    if((range == CONN_IS_INTERNAL_RANGE1) || (range == CONN_IS_INTERNAL_RANGE2))
    {
        sas_conn_stat_ptr->conn_type = FBE_ESES_CONN_TYPE_INTERNAL_CONN;
    }
    else
    {   
        sas_conn_stat_ptr->conn_type = FBE_ESES_CONN_TYPE_MINI_SAS_4X;
    }
    return(status);
}

fbe_status_t encl_stat_diag_page_exp_phy_elem_get_exp_index(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u8_t position,
    fbe_u8_t phy_id,
    fbe_u8_t *exp_index)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t start_element_index = 0;
    fbe_u8_t max_phys;
    fbe_sas_enclosure_type_t encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    terminator_sp_id_t spid;

    status =  fbe_terminator_api_get_sp_id(&spid);


    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    RETURN_ON_ERROR_STATUS;

    status = terminator_max_phys(encl_type, &max_phys);
    RETURN_ON_ERROR_STATUS;

    //Some sanity checks on passed parameters
    if((position >= max_phys) ||
       (phy_id >= max_phys))
    {
        return(status);
    }

    //Get from configuration page parsing routines.
    status = config_page_get_start_elem_index_in_stat_page(virtual_phy_handle,
                                                           SES_SUBENCL_TYPE_LCC,
                                                           spid, 
                                                           SES_ELEM_TYPE_SAS_EXP, 
                                                           FALSE,
                                                           0,
                                                           FALSE,
                                                           NULL,
                                                           &start_element_index);
    RETURN_ON_ERROR_STATUS;

    //Expander index is the same for all Phys.
    // This should be chagned to SAS exp once 
    // we get the config page contaning esc-elec elements.
    *exp_index = start_element_index;

    status = FBE_STATUS_OK;
    return(status);
}

/* The below PAGE SIZE functions are outdated. 
 * Should be modified in the near future.
 */
fbe_u32_t enclosure_status_diagnostic_page_size(fbe_sas_enclosure_type_t encl_type)
{
    switch(encl_type)
    {
        case FBE_SAS_ENCLOSURE_TYPE_BOXWOOD:
        case FBE_SAS_ENCLOSURE_TYPE_PINECONE:
        case FBE_SAS_ENCLOSURE_TYPE_STEELJAW:
        case FBE_SAS_ENCLOSURE_TYPE_RHEA:
            return((FBE_ESES_PAGE_HEADER_SIZE + (13*sizeof(ses_stat_elem_array_dev_slot_struct))));
            break;

        case FBE_SAS_ENCLOSURE_TYPE_DERRINGER:
        case FBE_SAS_ENCLOSURE_TYPE_TABASCO:
                return((FBE_ESES_PAGE_HEADER_SIZE + (26*sizeof(ses_stat_elem_array_dev_slot_struct))));
            break;

        case FBE_SAS_ENCLOSURE_TYPE_CITADEL:
        case FBE_SAS_ENCLOSURE_TYPE_FALLBACK:
        case FBE_SAS_ENCLOSURE_TYPE_KNOT:
        case FBE_SAS_ENCLOSURE_TYPE_RAMHORN:
        case FBE_SAS_ENCLOSURE_TYPE_MIRANDA:
        case FBE_SAS_ENCLOSURE_TYPE_CALYPSO:
            return((FBE_ESES_PAGE_HEADER_SIZE + (26*sizeof(ses_stat_elem_array_dev_slot_struct))));
            break;

        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE:
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP:
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP:
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP:
            return((FBE_ESES_PAGE_HEADER_SIZE + (30*sizeof(ses_stat_elem_array_dev_slot_struct))));
            break;

        case FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP:
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP:
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP:
            return( FBE_ESES_PAGE_HEADER_SIZE );
            break;

        case FBE_SAS_ENCLOSURE_TYPE_BUNKER:
        case FBE_SAS_ENCLOSURE_TYPE_VIPER:
        case FBE_SAS_ENCLOSURE_TYPE_ANCHO:
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM:        
            return((FBE_ESES_PAGE_HEADER_SIZE + (16*sizeof(ses_stat_elem_array_dev_slot_struct))));
            break;
            // others here
        default:
            break;
    }
    return(0);
}
