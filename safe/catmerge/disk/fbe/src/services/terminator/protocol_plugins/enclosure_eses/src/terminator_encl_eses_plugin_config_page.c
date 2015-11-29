/***************************************************************************
 *  terminator_encl_eses_plugin_config_page.c
 ***************************************************************************
 *
 *  Description
 *     This contains the funtions related to parsing the configuration
 *      diagnostic page.
 *
 *
 *  History:
 *      Oct-16-08   Rajesh V Created by splitting config page routines into
 *          seperate file.
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe_terminator_common.h"
#include "terminator_sas_io_api.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_eses.h"
#include "fbe_scsi.h"
#include "fbe_terminator.h"
#include "terminator_encl_eses_plugin.h"
#include "generic_utils_lib.h"
#include <stdlib.h>
/**********************************/
/*        local variables         */
/**********************************/
// Viper configuration info
/* configuration page contents side A and Side B*/
static fbe_u8_t viper_config_page_with_ps[][1200]= VIPER_CONFIGURATION_PAGE_CONTENTS;
//static fbe_u8_t viper_config_page_with_ps[][1400]= VIPER_CONFIGURATION_PAGE_CONTENTS_WITH_SPS_ATTACHED;
static terminator_eses_config_page_info_t viper_config_page_info_with_ps;
// Magnum configuration info
static fbe_u8_t magnum_config_page_with_ps[] = MAGNUM_CONFIGURATION_PAGE_CONTENTS;
static terminator_eses_config_page_info_t magnum_config_page_info_with_ps;
// Bunker configuration info
static fbe_u8_t bunker_config_page_with_ps[] = BUNKER_CONFIGURATION_PAGE_CONTENTS;
static terminator_eses_config_page_info_t bunker_config_page_info_with_ps;
// Citadel configuration info
static fbe_u8_t citadel_config_page_with_ps[][1200]= CITADEL_CONFIGURATION_PAGE_CONTENTS;
static terminator_eses_config_page_info_t citadel_config_page_info_with_ps;
// Derringer configuration info
static fbe_u8_t derringer_config_page_with_ps[][1200] = DERRINGER_CONFIGURATION_PAGE_CONTENTS;
static terminator_eses_config_page_info_t derringer_config_page_info_with_ps;
// ICM configuration info
static fbe_u8_t voyager_icm_config_page_with_ps[][1800] = VOYAGER_ICM_CONFIGURATION_PAGE_CONTENTS;
static terminator_eses_config_page_info_t voyager_icm_config_page_info_with_ps;
// VOYAGER_EE configuration info
static fbe_u8_t voyager_ee_config_page_with_ps[][1200] = VOYAGER_EE_CONFIGURATION_PAGE_CONTENTS;
static terminator_eses_config_page_info_t voyager_ee_config_page_info_with_ps;
// VIKING IOSXP configuration info
static fbe_u8_t viking_iosxp_config_page_with_ps[][1800] = VIKING_IOSXP_CONFIGURATION_PAGE_CONTENTS;
static terminator_eses_config_page_info_t viking_iosxp_config_page_info_with_ps;
// VIKING DRVSXP configuration info
static fbe_u8_t viking_drvsxp_config_page_with_ps[][1200] = VIKING_DRVSXP_CONFIGURATION_PAGE_CONTENTS;
static terminator_eses_config_page_info_t viking_drvsxp_config_page_info_with_ps;
//CAYENNE IOSXP configuration info
static fbe_u8_t cayenne_iosxp_config_page_with_ps[][1800] = CAYENNE_IOSXP_CONFIGURATION_PAGE_CONTENTS;
static terminator_eses_config_page_info_t cayenne_iosxp_config_page_info_with_ps;
// CAYENNE DRVSXP configuration info
static fbe_u8_t cayenne_drvsxp_config_page_with_ps[][1200] = CAYENNE_DRVSXP_CONFIGURATION_PAGE_CONTENTS;
static terminator_eses_config_page_info_t cayenne_drvsxp_config_page_info_with_ps;
// NAGA IOSXP configuration info
static fbe_u8_t naga_iosxp_config_page_with_ps[][1800] = NAGA_IOSXP_CONFIGURATION_PAGE_CONTENTS;
static terminator_eses_config_page_info_t naga_iosxp_config_page_info_with_ps;
// NAGA DRVSXP configuration info
static fbe_u8_t naga_drvsxp_config_page_with_ps[][1200] = NAGA_DRVSXP_CONFIGURATION_PAGE_CONTENTS;
static terminator_eses_config_page_info_t naga_drvsxp_config_page_info_with_ps;

// Fallback configuration info
static fbe_u8_t fallback_config_page_with_ps[][1200] = FALLBACK_CONFIGURATION_PAGE_CONTENTS;
static terminator_eses_config_page_info_t fallback_config_page_info_with_ps;
// Boxwood configuration info
static fbe_u8_t boxwood_config_page_with_ps[][1200] = BOXWOOD_CONFIGURATION_PAGE_CONTENTS;
static terminator_eses_config_page_info_t boxwood_config_page_info_with_ps;
// Knot configuration info
static fbe_u8_t knot_config_page_with_ps[][1200] = KNOT_CONFIGURATION_PAGE_CONTENTS;
static terminator_eses_config_page_info_t knot_config_page_info_with_ps;
// Pinecone configuration info
static fbe_u8_t pinecone_config_page_with_ps[][1200] = PINECONE_CONFIGURATION_PAGE_CONTENTS;
static terminator_eses_config_page_info_t pinecone_config_page_info_with_ps;
// Steeljaw configuration info
static fbe_u8_t steeljaw_config_page_with_ps[][1200] = BOXWOOD_CONFIGURATION_PAGE_CONTENTS;  //TODO, changr2, change to real steeljaw
static terminator_eses_config_page_info_t steeljaw_config_page_info_with_ps;
// Ramhorn configuration info
static fbe_u8_t ramhorn_config_page_with_ps[][1200] = RAMHORN_CONFIGURATION_PAGE_CONTENTS;  //TODO, changr2, change to real ramhorn
static terminator_eses_config_page_info_t ramhorn_config_page_info_with_ps;
// Ancho configuration info
static fbe_u8_t ancho_config_page_with_ps[][1200]= ANCHO_CONFIGURATION_PAGE_CONTENTS;
static terminator_eses_config_page_info_t ancho_config_page_info_with_ps;
// Calypso configuration info
static fbe_u8_t calypso_config_page_with_ps[][1200] = CALYPSO_CONFIGURATION_PAGE_CONTENTS;
static terminator_eses_config_page_info_t calypso_config_page_info_with_ps;
// Rhea configuration info
static fbe_u8_t rhea_config_page_with_ps[][1200] = RHEA_CONFIGURATION_PAGE_CONTENTS;
static terminator_eses_config_page_info_t rhea_config_page_info_with_ps;
// Miranda configuration info
static fbe_u8_t miranda_config_page_with_ps[][1200] = MIRANDA_CONFIGURATION_PAGE_CONTENTS;
static terminator_eses_config_page_info_t miranda_config_page_info_with_ps;
// Tabasco configuration info
static fbe_u8_t tabasco_config_page_with_ps[][1200] = TABASCO_CONFIGURATION_PAGE_CONTENTS;
static terminator_eses_config_page_info_t tabasco_config_page_info_with_ps;

/**********************************/
/*        local functions         */
/**********************************/
fbe_status_t get_start_elem_offset_by_config_page_info(terminator_eses_config_page_info_t *config_page_info,
                                                        ses_subencl_type_enum subencl_type,
                                                        terminator_eses_subencl_side side,
                                                        ses_elem_type_enum elem_type,
                                                        fbe_bool_t consider_num_possible_elems,
                                                        fbe_u8_t num_possible_elems,
                                                        fbe_bool_t consider_sub_encl_id,
                                                        fbe_u8_t sub_encl_id,
                                                        fbe_bool_t consider_type_desc_text,
                                                        fbe_u8_t *type_desc_text,
                                                        fbe_u16_t *offset);
fbe_status_t get_start_elem_index_by_config_page_info(terminator_eses_config_page_info_t *config_page_info,
                                                    ses_subencl_type_enum subencl_type,
                                                    terminator_eses_subencl_side side,
                                                    ses_elem_type_enum elem_type,
                                                    fbe_bool_t consider_num_possible_elems,
                                                    fbe_u8_t num_possible_elems,
                                                    fbe_bool_t consider_type_desc_text,
                                                    fbe_u8_t *type_desc_text,
                                                    fbe_u8_t *index);
static fbe_status_t config_page_get_num_of_sec_subencls_by_config_page(fbe_u8_t *config_page,
                                                                fbe_u8_t *num_sec_subencls);

static fbe_status_t config_page_get_gen_code_by_config_page (fbe_u8_t *config_page,
                                                             fbe_u32_t *gen_code);
static fbe_status_t config_page_set_all_ver_descs_in_config_page(fbe_u8_t *config_page,
                                                        terminator_eses_config_page_info_t *config_page_info,
                                                        ses_ver_desc_struct *ver_desc,
                                                        fbe_u16_t eses_version);
static fbe_status_t config_page_set_subencl_ver_descs_in_config_page(fbe_u8_t *config_page,
                                                                    ses_subencl_type_enum subencl_type,
                                                                    terminator_eses_subencl_side side,
                                                                    ses_ver_desc_struct *ver_desc_start,
                                                                    fbe_u8_t num_of_ver_desc,
                                                                    fbe_u16_t eses_version);
static fbe_status_t config_page_set_gen_code_by_config_page(fbe_u8_t *config_page,
                                                            fbe_u32_t gen_code);

static fbe_status_t config_page_convert_fw_rev_to_subencl_prod_rev_level(
                                           ses_subencl_prod_rev_level_struct * subencl_prod_rev_level_p, 
                                           fbe_char_t * comp_rev_level_p);

static fbe_status_t config_page_get_subencl_slot(ses_subencl_desc_struct * subencl_desc_ptr, 
                                          fbe_u8_t * subencl_slot_ptr);

/*********************************************************************
*  config_page_parse_given_config_page ()
**********************************************************************
*
*  Description: This builds the structures containing SUBENCLOSURE
*       DESC information,  TYPE DESC HEADER information, VERSION
*       DESC information , BUFFER DESC information & others by parsing
*       the  given configuration page.
*
*  Inputs:
*   ses_subencl_desc_struct subencl_desc[] - subenclosure descriptor
*       array to fill
*   ses_type_desc_hdr_struct type_desc_hdr[] - type desc header array
*       to fill
*   offset_of_first_elem[] - Array where each element gives the offset
*       of first element of the given element type (each index in this
*       array corresponds to the index in type_desc_hdr[] array)
*   ses_ver_desc_struct ver_desc_array[]- The version descriptors of
*       of each of the subenclosures are stored consecutively in this
*       array. To know which version descriptors belong to which
*       subenclosure, we need the above "subencl_desc[]" array.
*   elem_index_of_first_elem[] - Array where each element gives index
*       of first element of the given element type (each index in this
*       array corresponds to the index in type_desc_hdr[] array)
*   terminator_eses_subencl_buf_desc_info_t subencl_buf_desc_info[] - Array
*       that contains the buffer descriptor info of each of the
*       subenclosures.
*   number of subenclosures, type descriptor headers, version descriptors
*   and buffer descriptors that are in the given configuration page.
*   encl_stat_diag_page_size - The size of the Enclosure status diagnostic
*       page( this is to be caluclated from the config data)
*
*  Return Value: success or failure
*
*  Notes:
*
*  History:
*    Sep08  Rajesh V created
*
*********************************************************************/
fbe_status_t config_page_parse_given_config_page(fbe_u8_t *config_page,
                                                ses_subencl_desc_struct subencl_desc[],
                                                fbe_u8_t * subencl_slot,
                                                ses_type_desc_hdr_struct type_desc_hdr[],
                                                fbe_u8_t  elem_index_of_first_elem[],
                                                fbe_u16_t offset_of_first_elem[],
                                                ses_ver_desc_struct ver_desc_array[],
                                                terminator_eses_subencl_buf_desc_info_t subencl_buf_desc_info[],
                                                fbe_u8_t *num_subencls,
                                                fbe_u8_t *num_type_desc_headers,
                                                fbe_u8_t *num_ver_descs,
                                                fbe_u8_t *num_buf_descs,
                                                fbe_u16_t *encl_stat_diag_page_size)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t group_id = 0;
    fbe_u8_t elem_index = 0;
    fbe_u8_t element = 0;   // Loop through all the elements within the group.
    fbe_u8_t subencl = 0; // Loop through all the subenclosures.
    fbe_u8_t ver_desc = 0; // Loop through the version descriptors in a given SubEnclosure.
    fbe_u8_t buf_desc = 0; // Loop through the buffer descriptors in a given subenclosure.
    fbe_u16_t offset_in_ctrl_stat_pg = 0;
    fbe_u8_t total_num_subencls = 0;
    fbe_u8_t total_num_groups = 0;
    fbe_u8_t *ptr;
    ses_buf_desc_struct *buf_desc_ptr;


    ses_pg_config_struct * config_pg_ptr = NULL;
    ses_subencl_desc_struct * subencl_desc_ptr = NULL;
    ses_type_desc_hdr_struct * elem_type_desc_hdr_ptr = NULL;

        terminator_sp_id_t spid;
    
    status =  fbe_terminator_api_get_sp_id(&spid);

    *num_subencls = 0;
    *num_type_desc_headers = 0;
    *num_ver_descs = 0;
    *encl_stat_diag_page_size = 0;
    *num_buf_descs = 0;


    config_pg_ptr = (ses_pg_config_struct *)config_page;
    offset_in_ctrl_stat_pg = FBE_ESES_PAGE_HEADER_SIZE; // byte offset in the control/status page.

    // Get the pointer to the first subencl descriptor.
    subencl_desc_ptr = (ses_subencl_desc_struct *)((fbe_u8_t *)config_page + FBE_ESES_PAGE_HEADER_SIZE);
    // Get the total number of subenclosures.
    // (Adding 1 for the primary subenclosure).
    total_num_subencls = config_pg_ptr->num_secondary_subencls + 1;
    *num_subencls = total_num_subencls;

    // Process the subenclosure descriptor list.
    for(subencl = 0; subencl < total_num_subencls; subencl ++ )
    {
        // The total number of groups is equal to the total number of type descriptor headers.
        total_num_groups += subencl_desc_ptr->num_elem_type_desc_hdrs;

        
        if(config_page == voyager_ee_config_page_with_ps[spid])
        {
            if(subencl_desc_ptr->subencl_type == SES_SUBENCL_TYPE_LCC)
            {
                subencl_desc_ptr->side = subencl_desc_ptr->side%2;
            }
        }
        

        //For now I think it is safe to assume they are arranged as per the sub encl ids.
        memcpy(&subencl_desc[subencl], subencl_desc_ptr, sizeof(ses_subencl_desc_struct));

        //FILL in the version descriptor information
        for(ver_desc = 0; ver_desc < subencl_desc[subencl].num_ver_descs; ver_desc++, (*num_ver_descs)++)
        {
            memcpy(&ver_desc_array[*num_ver_descs],
                   &subencl_desc_ptr->ver_desc[ver_desc],
                   sizeof(ses_ver_desc_struct));
        }



        // FILL the buffer descriptor information

        ptr = (fbe_u8_t *)&subencl_desc_ptr->ver_desc[ver_desc];
        subencl_buf_desc_info[subencl].num_buf_descs = *ptr;
        // Get pointer to the first buffer desc, if it exists
        buf_desc_ptr = (ses_buf_desc_struct *)(ptr + 1);
        for(buf_desc = 0; buf_desc < subencl_buf_desc_info[subencl].num_buf_descs; buf_desc++, (*num_buf_descs)++)
        {
            memcpy(&subencl_buf_desc_info[subencl].buf_desc[buf_desc],
                   &buf_desc_ptr[buf_desc],
                   sizeof(ses_buf_desc_struct));
        }
        
        config_page_get_subencl_slot(subencl_desc_ptr, &subencl_slot[subencl]);

        subencl_desc_ptr = fbe_eses_get_next_ses_subencl_desc_p(subencl_desc_ptr);
    } // End of Processing the subenclosure descriptor list.

    *num_type_desc_headers = total_num_groups;

    // Process the type descriptor header list.
    elem_type_desc_hdr_ptr = (ses_type_desc_hdr_struct *)subencl_desc_ptr;

    for(group_id = 0; group_id < total_num_groups; group_id ++)
    {
        type_desc_hdr[group_id].elem_type = elem_type_desc_hdr_ptr->elem_type;
        type_desc_hdr[group_id].num_possible_elems = elem_type_desc_hdr_ptr->num_possible_elems;
        type_desc_hdr[group_id].subencl_id = elem_type_desc_hdr_ptr->subencl_id;
        offset_of_first_elem[group_id] = offset_in_ctrl_stat_pg;
        elem_index_of_first_elem[group_id] = elem_index;

        // Get the byte offset in the control/status page for the first element in the group.
        // This is to consider the overall status element for each element group.
        offset_in_ctrl_stat_pg += FBE_ESES_CTRL_STAT_ELEM_SIZE;

        for (element = 0; element < elem_type_desc_hdr_ptr->num_possible_elems; element++)
        {
            offset_in_ctrl_stat_pg += FBE_ESES_CTRL_STAT_ELEM_SIZE;
            elem_index ++;
        }

        // Get the pointer to the next type descriptor header.
        elem_type_desc_hdr_ptr = (ses_type_desc_hdr_struct *)((fbe_u8_t *)elem_type_desc_hdr_ptr + FBE_ESES_ELEM_TYPE_DESC_HEADER_SIZE);
    }
    *encl_stat_diag_page_size = offset_in_ctrl_stat_pg;

    status = FBE_STATUS_OK;
    return(status);
}

/*********************************************************************
*  config_page_get_subencl_slot ()
**********************************************************************
*
*  Description: This function gets the subenclosure slot number.
*
*  Params: subencl_desc_ptr
*          subencl_slot_ptr
* 
*  Return Value: success or failure
*
*  Notes:
*
*  History:
*    Jan-07-2014  PHE - created
*
*********************************************************************/
static fbe_status_t config_page_get_subencl_slot(ses_subencl_desc_struct * subencl_desc_ptr, 
                                          fbe_u8_t * subencl_slot_ptr)
{
    fbe_char_t              *subencl_text_p = NULL; 
    fbe_u8_t                *subencl_text_len_p = NULL;
    fbe_u8_t                 subencl_text_len = 0;

    subencl_text_p = fbe_eses_get_ses_subencl_text_p(subencl_desc_ptr);

    subencl_text_len_p = fbe_eses_get_ses_subencl_text_len_p(subencl_desc_ptr);
    subencl_text_len = *subencl_text_len_p;

    if(subencl_desc_ptr->subencl_type == SES_SUBENCL_TYPE_PS) 
    {
        if(strncmp((char *)subencl_text_p, 
                       FBE_ESES_SUBENCL_TEXT_POWER_SUPPLY_A, subencl_text_len) == 0)
        {
            *subencl_slot_ptr = 0;
        }
        else if(strncmp((char *)subencl_text_p, 
                               FBE_ESES_SUBENCL_TEXT_POWER_SUPPLY_B, subencl_text_len) == 0)
        {
            *subencl_slot_ptr = 1;
        }
        else if(strncmp((char *)subencl_text_p, 
                               FBE_ESES_SUBENCL_TEXT_POWER_SUPPLY_A1, subencl_text_len) == 0)
        {
            *subencl_slot_ptr = 0;
        }
        else if(strncmp((char *)subencl_text_p, 
                               FBE_ESES_SUBENCL_TEXT_POWER_SUPPLY_A0, subencl_text_len) == 0)
        {
            *subencl_slot_ptr = 1;
        }
        else if(strncmp((char *)subencl_text_p, 
                               FBE_ESES_SUBENCL_TEXT_POWER_SUPPLY_B1, subencl_text_len) == 0)
        {
            *subencl_slot_ptr = 2;
        }
        else if(strncmp((char *)subencl_text_p, 
                               FBE_ESES_SUBENCL_TEXT_POWER_SUPPLY_B0, subencl_text_len) == 0)
        {
            *subencl_slot_ptr = 3;
        }
        else
        {   
            *subencl_slot_ptr = 0;
        }
    }
    else
    {
        *subencl_slot_ptr = subencl_desc_ptr->side;
    }

    return FBE_STATUS_OK;
}

/*********************************************************************
*  config_page_init_type_desc_hdr_structs ()
**********************************************************************
*
*  Description: This builds the structures required by the interface
*  routines that return offset and index for a particular element
*  type in the enclosure status diagnostic page.
*
*  Inputs:
*   None
*
*  Return Value: success or failure
*
*  Notes:
*
*  History:
*    Sep08  Rajesh V created
*
*********************************************************************/
fbe_status_t config_page_init_type_desc_hdr_structs(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sp_id_t spid;
    
   status =  fbe_terminator_api_get_sp_id(&spid);

    // Parse and build viper configuration page related structures.
   status = config_page_parse_given_config_page(viper_config_page_with_ps[spid],
                                                viper_config_page_info_with_ps.subencl_desc_array,
                                                &viper_config_page_info_with_ps.subencl_slot[0],
                                                viper_config_page_info_with_ps.type_desc_array,
                                                viper_config_page_info_with_ps.elem_index_of_first_elem_array,
                                                viper_config_page_info_with_ps.offset_of_first_elem_array,
                                                viper_config_page_info_with_ps.ver_desc_array,
                                                viper_config_page_info_with_ps.subencl_buf_desc_info,
                                                &viper_config_page_info_with_ps.num_subencls,
                                                &viper_config_page_info_with_ps.num_type_desc_headers,
                                                &viper_config_page_info_with_ps.num_ver_descs,
                                                &viper_config_page_info_with_ps.num_buf_descs,
                                                &viper_config_page_info_with_ps.encl_stat_diag_page_size);
    RETURN_ON_ERROR_STATUS;


    // Parse and build magnum configuration page related structures.
    status = config_page_parse_given_config_page(magnum_config_page_with_ps,
                                                magnum_config_page_info_with_ps.subencl_desc_array,
                                                &magnum_config_page_info_with_ps.subencl_slot[0],
                                                magnum_config_page_info_with_ps.type_desc_array,
                                                magnum_config_page_info_with_ps.elem_index_of_first_elem_array,
                                                magnum_config_page_info_with_ps.offset_of_first_elem_array,
                                                magnum_config_page_info_with_ps.ver_desc_array,
                                                magnum_config_page_info_with_ps.subencl_buf_desc_info,
                                                &magnum_config_page_info_with_ps.num_subencls,
                                                &magnum_config_page_info_with_ps.num_type_desc_headers,
                                                &magnum_config_page_info_with_ps.num_ver_descs,
                                                &magnum_config_page_info_with_ps.num_buf_descs,
                                                &magnum_config_page_info_with_ps.encl_stat_diag_page_size);
    RETURN_ON_ERROR_STATUS;

    // Parse and build bunker configuration page related structures.
    status = config_page_parse_given_config_page(bunker_config_page_with_ps,
                                                bunker_config_page_info_with_ps.subencl_desc_array,
                                                &bunker_config_page_info_with_ps.subencl_slot[0],
                                                bunker_config_page_info_with_ps.type_desc_array,
                                                bunker_config_page_info_with_ps.elem_index_of_first_elem_array,
                                                bunker_config_page_info_with_ps.offset_of_first_elem_array,
                                                bunker_config_page_info_with_ps.ver_desc_array,
                                                bunker_config_page_info_with_ps.subencl_buf_desc_info,
                                                &bunker_config_page_info_with_ps.num_subencls,
                                                &bunker_config_page_info_with_ps.num_type_desc_headers,
                                                &bunker_config_page_info_with_ps.num_ver_descs,
                                                &bunker_config_page_info_with_ps.num_buf_descs,
                                                &bunker_config_page_info_with_ps.encl_stat_diag_page_size);
    RETURN_ON_ERROR_STATUS;


   // Parse and build citadel configuration page related structures.
    status = config_page_parse_given_config_page(citadel_config_page_with_ps[spid],
                                                citadel_config_page_info_with_ps.subencl_desc_array,
                                                &citadel_config_page_info_with_ps.subencl_slot[0],
                                                citadel_config_page_info_with_ps.type_desc_array,
                                                citadel_config_page_info_with_ps.elem_index_of_first_elem_array,
                                                citadel_config_page_info_with_ps.offset_of_first_elem_array,
                                                citadel_config_page_info_with_ps.ver_desc_array,
                                                citadel_config_page_info_with_ps.subencl_buf_desc_info,
                                                &citadel_config_page_info_with_ps.num_subencls,
                                                &citadel_config_page_info_with_ps.num_type_desc_headers,
                                                &citadel_config_page_info_with_ps.num_ver_descs,
                                                &citadel_config_page_info_with_ps.num_buf_descs,
                                                &citadel_config_page_info_with_ps.encl_stat_diag_page_size);
    RETURN_ON_ERROR_STATUS;
    // Parse and build derringer configuration page related structures.
    status = config_page_parse_given_config_page(derringer_config_page_with_ps[spid],
                                                derringer_config_page_info_with_ps.subencl_desc_array,
                                                &derringer_config_page_info_with_ps.subencl_slot[0],
                                                derringer_config_page_info_with_ps.type_desc_array,
                                                derringer_config_page_info_with_ps.elem_index_of_first_elem_array,
                                                derringer_config_page_info_with_ps.offset_of_first_elem_array,
                                                derringer_config_page_info_with_ps.ver_desc_array,
                                                derringer_config_page_info_with_ps.subencl_buf_desc_info,
                                                &derringer_config_page_info_with_ps.num_subencls,
                                                &derringer_config_page_info_with_ps.num_type_desc_headers,
                                                &derringer_config_page_info_with_ps.num_ver_descs,
                                                &derringer_config_page_info_with_ps.num_buf_descs,
                                                &derringer_config_page_info_with_ps.encl_stat_diag_page_size);
    RETURN_ON_ERROR_STATUS;

    // Parse and build icm configuration page related structures.
    status = config_page_parse_given_config_page(voyager_icm_config_page_with_ps[spid],
                                                voyager_icm_config_page_info_with_ps.subencl_desc_array,
                                                &voyager_icm_config_page_info_with_ps.subencl_slot[0],
                                                voyager_icm_config_page_info_with_ps.type_desc_array,
                                                voyager_icm_config_page_info_with_ps.elem_index_of_first_elem_array,
                                                voyager_icm_config_page_info_with_ps.offset_of_first_elem_array,
                                                voyager_icm_config_page_info_with_ps.ver_desc_array,
                                                voyager_icm_config_page_info_with_ps.subencl_buf_desc_info,
                                                &voyager_icm_config_page_info_with_ps.num_subencls,
                                                &voyager_icm_config_page_info_with_ps.num_type_desc_headers,
                                                &voyager_icm_config_page_info_with_ps.num_ver_descs,
                                                &voyager_icm_config_page_info_with_ps.num_buf_descs,
                                                &voyager_icm_config_page_info_with_ps.encl_stat_diag_page_size);
    RETURN_ON_ERROR_STATUS;

    status = config_page_parse_given_config_page(voyager_ee_config_page_with_ps[spid],
                                                voyager_ee_config_page_info_with_ps.subencl_desc_array,
                                                &voyager_ee_config_page_info_with_ps.subencl_slot[0],
                                                voyager_ee_config_page_info_with_ps.type_desc_array,
                                                voyager_ee_config_page_info_with_ps.elem_index_of_first_elem_array,
                                                voyager_ee_config_page_info_with_ps.offset_of_first_elem_array,
                                                voyager_ee_config_page_info_with_ps.ver_desc_array,
                                                voyager_ee_config_page_info_with_ps.subencl_buf_desc_info,
                                                &voyager_ee_config_page_info_with_ps.num_subencls,
                                                &voyager_ee_config_page_info_with_ps.num_type_desc_headers,
                                                &voyager_ee_config_page_info_with_ps.num_ver_descs,
                                                &voyager_ee_config_page_info_with_ps.num_buf_descs,
                                                &voyager_ee_config_page_info_with_ps.encl_stat_diag_page_size);
    RETURN_ON_ERROR_STATUS;     

    // Parse and build viking iosxp configuration page related structures.
    status = config_page_parse_given_config_page(viking_iosxp_config_page_with_ps[spid],
                                                viking_iosxp_config_page_info_with_ps.subencl_desc_array,
                                                &viking_iosxp_config_page_info_with_ps.subencl_slot[0],
                                                viking_iosxp_config_page_info_with_ps.type_desc_array,
                                                viking_iosxp_config_page_info_with_ps.elem_index_of_first_elem_array,
                                                viking_iosxp_config_page_info_with_ps.offset_of_first_elem_array,
                                                viking_iosxp_config_page_info_with_ps.ver_desc_array,
                                                viking_iosxp_config_page_info_with_ps.subencl_buf_desc_info,
                                                &viking_iosxp_config_page_info_with_ps.num_subencls,
                                                &viking_iosxp_config_page_info_with_ps.num_type_desc_headers,
                                                &viking_iosxp_config_page_info_with_ps.num_ver_descs,
                                                &viking_iosxp_config_page_info_with_ps.num_buf_descs,
                                                &viking_iosxp_config_page_info_with_ps.encl_stat_diag_page_size);
    RETURN_ON_ERROR_STATUS;

    // Parse and build viking drvsxp configuration page related structures.
    status = config_page_parse_given_config_page(viking_drvsxp_config_page_with_ps[spid],
                                                viking_drvsxp_config_page_info_with_ps.subencl_desc_array,
                                                &viking_drvsxp_config_page_info_with_ps.subencl_slot[0],
                                                viking_drvsxp_config_page_info_with_ps.type_desc_array,
                                                viking_drvsxp_config_page_info_with_ps.elem_index_of_first_elem_array,
                                                viking_drvsxp_config_page_info_with_ps.offset_of_first_elem_array,
                                                viking_drvsxp_config_page_info_with_ps.ver_desc_array,
                                                viking_drvsxp_config_page_info_with_ps.subencl_buf_desc_info,
                                                &viking_drvsxp_config_page_info_with_ps.num_subencls,
                                                &viking_drvsxp_config_page_info_with_ps.num_type_desc_headers,
                                                &viking_drvsxp_config_page_info_with_ps.num_ver_descs,
                                                &viking_drvsxp_config_page_info_with_ps.num_buf_descs,
                                                &viking_drvsxp_config_page_info_with_ps.encl_stat_diag_page_size);
    RETURN_ON_ERROR_STATUS;
    
    // Parse and build naga iosxp configuration page related structures.
    status = config_page_parse_given_config_page(naga_iosxp_config_page_with_ps[spid],
                                                naga_iosxp_config_page_info_with_ps.subencl_desc_array,
                                                &naga_iosxp_config_page_info_with_ps.subencl_slot[0],
                                                naga_iosxp_config_page_info_with_ps.type_desc_array,
                                                naga_iosxp_config_page_info_with_ps.elem_index_of_first_elem_array,
                                                naga_iosxp_config_page_info_with_ps.offset_of_first_elem_array,
                                                naga_iosxp_config_page_info_with_ps.ver_desc_array,
                                                naga_iosxp_config_page_info_with_ps.subencl_buf_desc_info,
                                                &naga_iosxp_config_page_info_with_ps.num_subencls,
                                                &naga_iosxp_config_page_info_with_ps.num_type_desc_headers,
                                                &naga_iosxp_config_page_info_with_ps.num_ver_descs,
                                                &naga_iosxp_config_page_info_with_ps.num_buf_descs,
                                                &naga_iosxp_config_page_info_with_ps.encl_stat_diag_page_size);
    RETURN_ON_ERROR_STATUS;

    // Parse and build naga drvsxp configuration page related structures.
    status = config_page_parse_given_config_page(naga_drvsxp_config_page_with_ps[spid],
                                                naga_drvsxp_config_page_info_with_ps.subencl_desc_array,
                                                &naga_drvsxp_config_page_info_with_ps.subencl_slot[0],
                                                naga_drvsxp_config_page_info_with_ps.type_desc_array,
                                                naga_drvsxp_config_page_info_with_ps.elem_index_of_first_elem_array,
                                                naga_drvsxp_config_page_info_with_ps.offset_of_first_elem_array,
                                                naga_drvsxp_config_page_info_with_ps.ver_desc_array,
                                                naga_drvsxp_config_page_info_with_ps.subencl_buf_desc_info,
                                                &naga_drvsxp_config_page_info_with_ps.num_subencls,
                                                &naga_drvsxp_config_page_info_with_ps.num_type_desc_headers,
                                                &naga_drvsxp_config_page_info_with_ps.num_ver_descs,
                                                &naga_drvsxp_config_page_info_with_ps.num_buf_descs,
                                                &naga_drvsxp_config_page_info_with_ps.encl_stat_diag_page_size);
    RETURN_ON_ERROR_STATUS;     

    
    // Parse and build iosxp configuration page related structures.
    status = config_page_parse_given_config_page(cayenne_iosxp_config_page_with_ps[spid],
                                                cayenne_iosxp_config_page_info_with_ps.subencl_desc_array,
                                                &cayenne_iosxp_config_page_info_with_ps.subencl_slot[0],
                                                cayenne_iosxp_config_page_info_with_ps.type_desc_array,
                                                cayenne_iosxp_config_page_info_with_ps.elem_index_of_first_elem_array,
                                                cayenne_iosxp_config_page_info_with_ps.offset_of_first_elem_array,
                                                cayenne_iosxp_config_page_info_with_ps.ver_desc_array,
                                                cayenne_iosxp_config_page_info_with_ps.subencl_buf_desc_info,
                                                &cayenne_iosxp_config_page_info_with_ps.num_subencls,
                                                &cayenne_iosxp_config_page_info_with_ps.num_type_desc_headers,
                                                &cayenne_iosxp_config_page_info_with_ps.num_ver_descs,
                                                &cayenne_iosxp_config_page_info_with_ps.num_buf_descs,
                                                &cayenne_iosxp_config_page_info_with_ps.encl_stat_diag_page_size);
    RETURN_ON_ERROR_STATUS;

    status = config_page_parse_given_config_page(cayenne_drvsxp_config_page_with_ps[spid],
                                                cayenne_drvsxp_config_page_info_with_ps.subencl_desc_array,
                                                &cayenne_drvsxp_config_page_info_with_ps.subencl_slot[0],
                                                cayenne_drvsxp_config_page_info_with_ps.type_desc_array,
                                                cayenne_drvsxp_config_page_info_with_ps.elem_index_of_first_elem_array,
                                                cayenne_drvsxp_config_page_info_with_ps.offset_of_first_elem_array,
                                                cayenne_drvsxp_config_page_info_with_ps.ver_desc_array,
                                                cayenne_drvsxp_config_page_info_with_ps.subencl_buf_desc_info,
                                                &cayenne_drvsxp_config_page_info_with_ps.num_subencls,
                                                &cayenne_drvsxp_config_page_info_with_ps.num_type_desc_headers,
                                                &cayenne_drvsxp_config_page_info_with_ps.num_ver_descs,
                                                &cayenne_drvsxp_config_page_info_with_ps.num_buf_descs,
                                                &cayenne_drvsxp_config_page_info_with_ps.encl_stat_diag_page_size);
    RETURN_ON_ERROR_STATUS;     

    // Parse and build fallback configuration page related structures.
    status = config_page_parse_given_config_page(fallback_config_page_with_ps[spid],
                                                fallback_config_page_info_with_ps.subencl_desc_array,
                                                &fallback_config_page_info_with_ps.subencl_slot[0],
                                                fallback_config_page_info_with_ps.type_desc_array,
                                                fallback_config_page_info_with_ps.elem_index_of_first_elem_array,
                                                fallback_config_page_info_with_ps.offset_of_first_elem_array,
                                                fallback_config_page_info_with_ps.ver_desc_array,
                                                fallback_config_page_info_with_ps.subencl_buf_desc_info,
                                                &fallback_config_page_info_with_ps.num_subencls,
                                                &fallback_config_page_info_with_ps.num_type_desc_headers,
                                                &fallback_config_page_info_with_ps.num_ver_descs,
                                                &fallback_config_page_info_with_ps.num_buf_descs,
                                                &fallback_config_page_info_with_ps.encl_stat_diag_page_size);
    RETURN_ON_ERROR_STATUS;
    // Parse and build boxwood configuration page related structures.
    status = config_page_parse_given_config_page(boxwood_config_page_with_ps[spid],
                                                boxwood_config_page_info_with_ps.subencl_desc_array,
                                                &boxwood_config_page_info_with_ps.subencl_slot[0],
                                                boxwood_config_page_info_with_ps.type_desc_array,
                                                boxwood_config_page_info_with_ps.elem_index_of_first_elem_array,
                                                boxwood_config_page_info_with_ps.offset_of_first_elem_array,
                                                boxwood_config_page_info_with_ps.ver_desc_array,
                                                boxwood_config_page_info_with_ps.subencl_buf_desc_info,
                                                &boxwood_config_page_info_with_ps.num_subencls,
                                                &boxwood_config_page_info_with_ps.num_type_desc_headers,
                                                &boxwood_config_page_info_with_ps.num_ver_descs,
                                                &boxwood_config_page_info_with_ps.num_buf_descs,
                                                &boxwood_config_page_info_with_ps.encl_stat_diag_page_size);
    RETURN_ON_ERROR_STATUS;

    // Parse and build knot configuration page related structures.
    status = config_page_parse_given_config_page(knot_config_page_with_ps[spid],
                                                knot_config_page_info_with_ps.subencl_desc_array,
                                                &knot_config_page_info_with_ps.subencl_slot[0],
                                                knot_config_page_info_with_ps.type_desc_array,
                                                knot_config_page_info_with_ps.elem_index_of_first_elem_array,
                                                knot_config_page_info_with_ps.offset_of_first_elem_array,
                                                knot_config_page_info_with_ps.ver_desc_array,
                                                knot_config_page_info_with_ps.subencl_buf_desc_info,
                                                &knot_config_page_info_with_ps.num_subencls,
                                                &knot_config_page_info_with_ps.num_type_desc_headers,
                                                &knot_config_page_info_with_ps.num_ver_descs,
                                                &knot_config_page_info_with_ps.num_buf_descs,
                                                &knot_config_page_info_with_ps.encl_stat_diag_page_size);
    RETURN_ON_ERROR_STATUS;

    // Parse and build pinecone configuration page related structures.
    status = config_page_parse_given_config_page(pinecone_config_page_with_ps[spid],
                                                pinecone_config_page_info_with_ps.subencl_desc_array,
                                                &pinecone_config_page_info_with_ps.subencl_slot[0],
                                                pinecone_config_page_info_with_ps.type_desc_array,
                                                pinecone_config_page_info_with_ps.elem_index_of_first_elem_array,
                                                pinecone_config_page_info_with_ps.offset_of_first_elem_array,
                                                pinecone_config_page_info_with_ps.ver_desc_array,
                                                pinecone_config_page_info_with_ps.subencl_buf_desc_info,
                                                &pinecone_config_page_info_with_ps.num_subencls,
                                                &pinecone_config_page_info_with_ps.num_type_desc_headers,
                                                &pinecone_config_page_info_with_ps.num_ver_descs,
                                                &pinecone_config_page_info_with_ps.num_buf_descs,
                                                &pinecone_config_page_info_with_ps.encl_stat_diag_page_size);
    RETURN_ON_ERROR_STATUS;

    // Parse and build steeljaw configuration page related structures.
    status = config_page_parse_given_config_page(steeljaw_config_page_with_ps[spid],
                                                steeljaw_config_page_info_with_ps.subencl_desc_array,
                                                &steeljaw_config_page_info_with_ps.subencl_slot[0],
                                                steeljaw_config_page_info_with_ps.type_desc_array,
                                                steeljaw_config_page_info_with_ps.elem_index_of_first_elem_array,
                                                steeljaw_config_page_info_with_ps.offset_of_first_elem_array,
                                                steeljaw_config_page_info_with_ps.ver_desc_array,
                                                steeljaw_config_page_info_with_ps.subencl_buf_desc_info,
                                                &steeljaw_config_page_info_with_ps.num_subencls,
                                                &steeljaw_config_page_info_with_ps.num_type_desc_headers,
                                                &steeljaw_config_page_info_with_ps.num_ver_descs,
                                                &steeljaw_config_page_info_with_ps.num_buf_descs,
                                                &steeljaw_config_page_info_with_ps.encl_stat_diag_page_size);
    RETURN_ON_ERROR_STATUS;

    // Parse and build ramhorn configuration page related structures.
    status = config_page_parse_given_config_page(ramhorn_config_page_with_ps[spid],
                                                ramhorn_config_page_info_with_ps.subencl_desc_array,
                                                &ramhorn_config_page_info_with_ps.subencl_slot[0],
                                                ramhorn_config_page_info_with_ps.type_desc_array,
                                                ramhorn_config_page_info_with_ps.elem_index_of_first_elem_array,
                                                ramhorn_config_page_info_with_ps.offset_of_first_elem_array,
                                                ramhorn_config_page_info_with_ps.ver_desc_array,
                                                ramhorn_config_page_info_with_ps.subencl_buf_desc_info,
                                                &ramhorn_config_page_info_with_ps.num_subencls,
                                                &ramhorn_config_page_info_with_ps.num_type_desc_headers,
                                                &ramhorn_config_page_info_with_ps.num_ver_descs,
                                                &ramhorn_config_page_info_with_ps.num_buf_descs,
                                                &ramhorn_config_page_info_with_ps.encl_stat_diag_page_size);
    RETURN_ON_ERROR_STATUS;

        
    // Parse and build ancho configuration page related structures.
    status = config_page_parse_given_config_page(ancho_config_page_with_ps[spid],
                                                 ancho_config_page_info_with_ps.subencl_desc_array,
                                                 &ancho_config_page_info_with_ps.subencl_slot[0],
                                                 ancho_config_page_info_with_ps.type_desc_array,
                                                 ancho_config_page_info_with_ps.elem_index_of_first_elem_array,
                                                 ancho_config_page_info_with_ps.offset_of_first_elem_array,
                                                 ancho_config_page_info_with_ps.ver_desc_array,
                                                 ancho_config_page_info_with_ps.subencl_buf_desc_info,
                                                 &ancho_config_page_info_with_ps.num_subencls,
                                                 &ancho_config_page_info_with_ps.num_type_desc_headers,
                                                 &ancho_config_page_info_with_ps.num_ver_descs,
                                                 &ancho_config_page_info_with_ps.num_buf_descs,
                                                 &ancho_config_page_info_with_ps.encl_stat_diag_page_size);
    RETURN_ON_ERROR_STATUS;
        

    // Parse and build calypso configuration page related structures.
    status = config_page_parse_given_config_page(calypso_config_page_with_ps[spid],
                                                calypso_config_page_info_with_ps.subencl_desc_array,
                                                &calypso_config_page_info_with_ps.subencl_slot[0],
                                                calypso_config_page_info_with_ps.type_desc_array,
                                                calypso_config_page_info_with_ps.elem_index_of_first_elem_array,
                                                calypso_config_page_info_with_ps.offset_of_first_elem_array,
                                                calypso_config_page_info_with_ps.ver_desc_array,
                                                calypso_config_page_info_with_ps.subencl_buf_desc_info,
                                                &calypso_config_page_info_with_ps.num_subencls,
                                                &calypso_config_page_info_with_ps.num_type_desc_headers,
                                                &calypso_config_page_info_with_ps.num_ver_descs,
                                                &calypso_config_page_info_with_ps.num_buf_descs,
                                                &calypso_config_page_info_with_ps.encl_stat_diag_page_size);
    RETURN_ON_ERROR_STATUS;
    // Parse and build rhea configuration page related structures.
    status = config_page_parse_given_config_page(rhea_config_page_with_ps[spid],
                                                rhea_config_page_info_with_ps.subencl_desc_array,
                                                &rhea_config_page_info_with_ps.subencl_slot[0],
                                                rhea_config_page_info_with_ps.type_desc_array,
                                                rhea_config_page_info_with_ps.elem_index_of_first_elem_array,
                                                rhea_config_page_info_with_ps.offset_of_first_elem_array,
                                                rhea_config_page_info_with_ps.ver_desc_array,
                                                rhea_config_page_info_with_ps.subencl_buf_desc_info,
                                                &rhea_config_page_info_with_ps.num_subencls,
                                                &rhea_config_page_info_with_ps.num_type_desc_headers,
                                                &rhea_config_page_info_with_ps.num_ver_descs,
                                                &rhea_config_page_info_with_ps.num_buf_descs,
                                                &rhea_config_page_info_with_ps.encl_stat_diag_page_size);
    RETURN_ON_ERROR_STATUS;

    // Parse and build miranda configuration page related structures.
    status = config_page_parse_given_config_page(miranda_config_page_with_ps[spid],
                                                miranda_config_page_info_with_ps.subencl_desc_array,
                                                &miranda_config_page_info_with_ps.subencl_slot[0],
                                                miranda_config_page_info_with_ps.type_desc_array,
                                                miranda_config_page_info_with_ps.elem_index_of_first_elem_array,
                                                miranda_config_page_info_with_ps.offset_of_first_elem_array,
                                                miranda_config_page_info_with_ps.ver_desc_array,
                                                miranda_config_page_info_with_ps.subencl_buf_desc_info,
                                                &miranda_config_page_info_with_ps.num_subencls,
                                                &miranda_config_page_info_with_ps.num_type_desc_headers,
                                                &miranda_config_page_info_with_ps.num_ver_descs,
                                                &miranda_config_page_info_with_ps.num_buf_descs,
                                                &miranda_config_page_info_with_ps.encl_stat_diag_page_size);
    RETURN_ON_ERROR_STATUS;

    // Parse and build tabasco configuration page related structures.
    status = config_page_parse_given_config_page(tabasco_config_page_with_ps[spid],
                                                tabasco_config_page_info_with_ps.subencl_desc_array,
                                                &tabasco_config_page_info_with_ps.subencl_slot[0],
                                                tabasco_config_page_info_with_ps.type_desc_array,
                                                tabasco_config_page_info_with_ps.elem_index_of_first_elem_array,
                                                tabasco_config_page_info_with_ps.offset_of_first_elem_array,
                                                tabasco_config_page_info_with_ps.ver_desc_array,
                                                tabasco_config_page_info_with_ps.subencl_buf_desc_info,
                                                &tabasco_config_page_info_with_ps.num_subencls,
                                                &tabasco_config_page_info_with_ps.num_type_desc_headers,
                                                &tabasco_config_page_info_with_ps.num_ver_descs,
                                                &tabasco_config_page_info_with_ps.num_buf_descs,
                                                &tabasco_config_page_info_with_ps.encl_stat_diag_page_size);
    RETURN_ON_ERROR_STATUS;

    //Initialize for config page without psa,psb--etc here.

    status = FBE_STATUS_OK;
    return(status);
}



/*********************************************************************
*  config_page_get_start_elem_offset_in_stat_page ()
**********************************************************************
*
*  Description:
*   This function returns the current offset of the given status
*   element type in Encl stat page.
*
*  Inputs:
*   virtual phy object handle.
*   subencl_type - subenclosure type
*   side - side (A or B indicated by 0 & 1 now)
*   elem_type - Status element type.
*   offset - pointer to offset to be returned
*
*  Return Value: success or failure
*
*  Notes:
*
*  History:
*    Sep08  Rajesh V created
*
*********************************************************************/
fbe_status_t config_page_get_start_elem_offset_in_stat_page(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                            ses_subencl_type_enum subencl_type,
                                                            terminator_eses_subencl_side side,
                                                            ses_elem_type_enum elem_type,
                                                            fbe_bool_t consider_num_possible_elems,
                                                            fbe_u8_t num_possible_elems,
                                                            fbe_bool_t consider_sub_encl_id,
                                                            fbe_u8_t sub_encl_id,
                                                            fbe_bool_t consider_type_desc_text,
                                                            fbe_u8_t *type_desc_text,
                                                            fbe_u16_t *offset)
{

    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_eses_config_page_info_t *current_config_page_info;
    vp_config_diag_page_info_t *vp_config_diag_page_info;

    // Get the virtual phy configuration information for the given Virtual Phy handle
    status = eses_get_config_diag_page_info(virtual_phy_handle, &vp_config_diag_page_info);
    RETURN_ON_ERROR_STATUS;

    // Get the base configuration page information from the VP config info.
    current_config_page_info = vp_config_diag_page_info->config_page_info;

    status = get_start_elem_offset_by_config_page_info(current_config_page_info,
                                                       subencl_type,
                                                       side,
                                                       elem_type,
                                                       consider_num_possible_elems,
                                                       num_possible_elems,
                                                       consider_sub_encl_id,
                                                       sub_encl_id,
                                                       consider_type_desc_text,
                                                       type_desc_text,
                                                       offset);
    RETURN_ON_ERROR_STATUS;

    return(FBE_STATUS_OK);
}

/*********************************************************************
*  get_start_elem_offset_by_config_page_info ()
**********************************************************************
*
*  Description:
*   This function returns the offset of the given status element type
*    in Encl stat page, WRT to the given configuration page.
*
*  Inputs:
*   config_page_info - pointer to configuration page to use.
*   subencl_type - subenclosure type
*   side - side (A or B indicated by 0 & 1 now)
*   elem_type - Status element type.
*   offset - pointer to offset to be returned
*
*  Return Value: success or failure
*
*  Notes:
*
*  History:
*    Sep08  Rajesh V created
*
*********************************************************************/
fbe_status_t get_start_elem_offset_by_config_page_info(terminator_eses_config_page_info_t *config_page_info,
                                                        ses_subencl_type_enum subencl_type,
                                                        terminator_eses_subencl_side side,
                                                        ses_elem_type_enum elem_type,
                                                        fbe_bool_t consider_num_possible_elems,
                                                        fbe_u8_t num_possible_elems,
                                                        fbe_bool_t consider_sub_encl_id,
                                                        fbe_u8_t sub_encl_id,
                                                        fbe_bool_t consider_type_desc_text,
                                                        fbe_u8_t *type_desc_text,
                                                        fbe_u16_t *offset)
{
    fbe_u8_t i;

    // Ignore type descriptor text related parameters for now.
    UNREFERENCED_PARAMETER(consider_type_desc_text);
    UNREFERENCED_PARAMETER(type_desc_text);

    for(i=0; i < config_page_info->num_type_desc_headers ;i++)
    {
        if((config_page_info->type_desc_array[i].elem_type == elem_type) &&
           ((!consider_num_possible_elems) || (config_page_info->type_desc_array[i].num_possible_elems == num_possible_elems)) &&
           ((!consider_sub_encl_id) || (config_page_info->type_desc_array[i].subencl_id == sub_encl_id)) &&
           ((config_page_info->subencl_desc_array[config_page_info->type_desc_array[i].subencl_id].subencl_type)
                == subencl_type ) &&
           ((config_page_info->subencl_desc_array[config_page_info->type_desc_array[i].subencl_id].side)
                == side) )
        {
            *offset = config_page_info->offset_of_first_elem_array[i];
            return(FBE_STATUS_OK);
        }
    }

    // Assume FBE_STATUS_COMPONENT_NOT_FOUND implies the particular element
    // is not present in the configuration page. Ex: we dont have PS elems
    // in the configuration page for magnum as they are monitored by specl
    // on magnum DPE and not monitored by the ESES talking EMA.
    return(FBE_STATUS_COMPONENT_NOT_FOUND);
}

/*********************************************************************
*  config_page_get_start_elem_index_in_stat_page()
**********************************************************************
*
*  Description:
*   This function returns the current index of first element(individual)
*   of the given status element type in Encl stat page.
*
*  Inputs:
*   subencl_type - subenclosure type
*   side - side (A or B indicated by 0 & 1 now)
*   elem_type - Status element type.
*   offset - pointer to offset to be returned
*
*  Return Value: success or failure
*
*  Notes:
*
*  History:
*    Sep08  Rajesh V created
*
*********************************************************************/

fbe_status_t config_page_get_start_elem_index_in_stat_page(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                            ses_subencl_type_enum subencl_type,
                                                            terminator_eses_subencl_side side,
                                                            ses_elem_type_enum elem_type,
                                                            fbe_bool_t consider_num_possible_elems,
                                                            fbe_u8_t num_possible_elems,
                                                            fbe_bool_t consider_type_desc_text,
                                                            fbe_u8_t *type_desc_text,
                                                            fbe_u8_t *index)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_eses_config_page_info_t *current_config_page_info;
    vp_config_diag_page_info_t *vp_config_diag_page_info;

    // Get the virtual phy configuration information for the given Virtual Phy handle
    status = eses_get_config_diag_page_info(virtual_phy_handle, &vp_config_diag_page_info);
    RETURN_ON_ERROR_STATUS;

    // Get the base configuration page information from the VP config info.
    current_config_page_info = vp_config_diag_page_info->config_page_info;

    status = get_start_elem_index_by_config_page_info(current_config_page_info,
                                                      subencl_type,
                                                      side,
                                                      elem_type,
                                                      consider_num_possible_elems,
                                                      num_possible_elems,
                                                      consider_type_desc_text,
                                                      type_desc_text,
                                                      index);
    RETURN_ON_ERROR_STATUS;

    return(FBE_STATUS_OK);
}

/*********************************************************************
* get_start_elem_index_by_config_page_info ()
**********************************************************************
*
*  Description:
*   This function returns the index of the first element(individual) of
*   the given status element type in Encl stat page, WRT to
*   the given configuration page.
*
*  Inputs:
*   config_page_info - pointer to configuration page to use.
*   subencl_type - subenclosure type
*   side - side (A or B indicated by 0 & 1 now)
*   elem_type - Status element type.
*   offset - pointer to offset to be returned
*
*  Return Value: success or failure
*
*  Notes:
*
*  History:
*    Sep08  Rajesh V created
*
*********************************************************************/
fbe_status_t get_start_elem_index_by_config_page_info(terminator_eses_config_page_info_t *config_page_info,
                                                    ses_subencl_type_enum subencl_type,
                                                    terminator_eses_subencl_side side,
                                                    ses_elem_type_enum elem_type,
                                                    fbe_bool_t consider_num_possible_elems,
                                                    fbe_u8_t num_possible_elems,
                                                    fbe_bool_t consider_type_desc_text,
                                                    fbe_u8_t *type_desc_text,
                                                    fbe_u8_t *index)
{
    fbe_u8_t i;

    // Ignore type descriptor text related parameters for now.
    UNREFERENCED_PARAMETER(consider_type_desc_text);
    UNREFERENCED_PARAMETER(type_desc_text);

    for(i=0; i < config_page_info->num_type_desc_headers ;i++)
    {
        if((config_page_info->type_desc_array[i].elem_type == elem_type) &&
           ((!consider_num_possible_elems) || (config_page_info->type_desc_array[i].num_possible_elems == num_possible_elems)) &&
           ((config_page_info->subencl_desc_array[config_page_info->type_desc_array[i].subencl_id].subencl_type)
                == subencl_type ) &&
            ((config_page_info->subencl_desc_array[config_page_info->type_desc_array[i].subencl_id].side)
                == side) )
        {
            *index = config_page_info->elem_index_of_first_elem_array[i];
            return(FBE_STATUS_OK);
        }
    }

    // Assume FBE_STATUS_COMPONENT_NOT_FOUND implies the particular element
    // is not present in the configuration page. Ex: we dont have PS elems
    // in the configuration page for magnum as they are monitored by specl
    // on magnum DPE and not monitored by the ESES talking EMA.
    return(FBE_STATUS_COMPONENT_NOT_FOUND);
}

/*********************************************************************
* config_page_get_encl_stat_diag_page_size ()
**********************************************************************
*
*  Description:
*   Gives the size of the Enclosure status diagnostic page from CONGIG
*   Page being used.
*
*  Inputs:
*   Virtual phy object handle.
*
*  Return Value: success or failure
*
*  Notes:
*
*  History:
*    Nov-19-2008    Rajesh V created
*
*********************************************************************/
fbe_status_t config_page_get_encl_stat_diag_page_size(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                      fbe_u16_t *page_size)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    vp_config_diag_page_info_t *config_diag_page_info;

    status = eses_get_config_diag_page_info(virtual_phy_handle, &config_diag_page_info);
    RETURN_ON_ERROR_STATUS;

    *page_size = config_diag_page_info->config_page_info->encl_stat_diag_page_size;

    return(FBE_STATUS_OK);
}

/*********************************************************************
* config_page_set_gen_code_by_config_page ()
**********************************************************************
*
*  Description:
*   Sets the gneration code in the given configuration page.
*
*  Inputs:
*   Configuration page, generation code to set.
*
*  Return Value: success or failure
*
*  Notes:
*
*  History:
*    Nov-19-2008    Rajesh V created
*
*********************************************************************/
fbe_status_t config_page_set_gen_code_by_config_page(fbe_u8_t *config_page,
                                                     fbe_u32_t gen_code)
{
    ses_pg_config_struct *config_pg_hdr;

    if(config_page == NULL)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    config_pg_hdr = (ses_pg_config_struct *)config_page;
    gen_code = BYTE_SWAP_32(gen_code);
    memcpy(&config_pg_hdr->gen_code, &gen_code, sizeof(fbe_u32_t));

    return(FBE_STATUS_OK);
}

/*********************************************************************
* config_page_get_gen_code_by_config_page ()
**********************************************************************
*
*  Description:
*   Gets the gneration code in the given configuration page.
*
*  Inputs:
*   Configuration page, generation code to fill.
*
*  Return Value: success or failure
*
*  Notes:
*
*  History:
*    Nov-19-2008    Rajesh V created
*
*********************************************************************/
fbe_status_t config_page_get_gen_code_by_config_page (fbe_u8_t *config_page,
                                                      fbe_u32_t *gen_code)
{
    ses_pg_config_struct *config_pg_hdr;

    if(config_page == NULL)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    config_pg_hdr = (ses_pg_config_struct *)config_page;
    *gen_code = config_pg_hdr->gen_code;
    *gen_code = BYTE_SWAP_32(*gen_code);

    return(FBE_STATUS_OK);
}

/*********************************************************************
* config_page_set_subencl_ver_descs_in_config_page ()
**********************************************************************
*
*  Description:
*   Sets the version descriptor block in the given config page.
*
*  Inputs:
*   Configuration page, Subenclosure type & side for the given version
*   descriptor, version descriptor to set, number of version
*   descriptors in the given subenclosure.
*
*  Return Value: success or failure
*
*  Notes:
*
*  History:
*    Nov-19-2008    Rajesh V created
*
*********************************************************************/
fbe_status_t config_page_set_subencl_ver_descs_in_config_page(fbe_u8_t *config_page,
                                                            ses_subencl_type_enum subencl_type,
                                                            terminator_eses_subencl_side side,
                                                            ses_ver_desc_struct *ver_desc_start,
                                                            fbe_u8_t num_ver_descs,
                                                            fbe_u16_t eses_version)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_subencl_desc_struct * subencl_desc_ptr = NULL;
    ses_pg_config_struct * config_pg_hdr = NULL;
    fbe_u8_t subencl = 0;
    fbe_u8_t total_num_subencls = 0;
    fbe_u8_t *config_pg_ptr = (fbe_u8_t *)config_page;
    ses_subencl_prod_rev_level_struct subencl_prod_rev_level = {0};
    fbe_u8_t subencl_slot = 0;


    config_pg_hdr = (ses_pg_config_struct *)config_page;
    subencl_desc_ptr = (ses_subencl_desc_struct *)((fbe_u8_t *)config_pg_ptr
                                                    + FBE_ESES_PAGE_HEADER_SIZE);
    // Get the total number of subenclosures.
    // (Adding 1 for the primary subenclosure).
    total_num_subencls =  config_pg_hdr->num_secondary_subencls + 1;

    for(subencl = 0; subencl < total_num_subencls; subencl ++ )
    {
        config_page_get_subencl_slot(subencl_desc_ptr, &subencl_slot);

        if((subencl_type == subencl_desc_ptr->subencl_type) &&
           (side == subencl_slot) )
        {
            // We are now at the required Subenclosure Descriptor
            // Loop through the version descriptors
            if(num_ver_descs != subencl_desc_ptr->num_ver_descs)
            {
                return(FBE_STATUS_GENERIC_FAILURE);
            }
            memcpy(subencl_desc_ptr->ver_desc, ver_desc_start, (num_ver_descs)*sizeof(ses_ver_desc_struct));

            if(eses_version == ESES_REVISION_2_0)
            {
                //Convert the first version descriptor comp_rev_level to subencl_prod_rev_level.
                subencl_desc_ptr->subencl_prod_rev_level.digits[0] = ver_desc_start->cdes_rev.cdes_1_0_rev.comp_rev_level[0];
                subencl_desc_ptr->subencl_prod_rev_level.digits[1] = ver_desc_start->cdes_rev.cdes_1_0_rev.comp_rev_level[1];
                if(ver_desc_start->cdes_rev.cdes_1_0_rev.comp_rev_level[2] == '.')
                {
                    subencl_desc_ptr->subencl_prod_rev_level.digits[2] = ver_desc_start->cdes_rev.cdes_1_0_rev.comp_rev_level[3];
                    subencl_desc_ptr->subencl_prod_rev_level.digits[3] = ver_desc_start->cdes_rev.cdes_1_0_rev.comp_rev_level[4];
                }
                else
                {
                    subencl_desc_ptr->subencl_prod_rev_level.digits[2] = ver_desc_start->cdes_rev.cdes_1_0_rev.comp_rev_level[2];
                    subencl_desc_ptr->subencl_prod_rev_level.digits[3] = ver_desc_start->cdes_rev.cdes_1_0_rev.comp_rev_level[3];
                }
            }
            else
            {
                config_page_convert_fw_rev_to_subencl_prod_rev_level(&subencl_prod_rev_level, 
                                                                  &ver_desc_start->cdes_rev.cdes_1_0_rev.comp_rev_level[0]);
                memcpy(&subencl_desc_ptr->subencl_prod_rev_level, 
                       &subencl_prod_rev_level, 
                       4);
            }
            status = FBE_STATUS_OK;
            break;
        }
        subencl_desc_ptr = fbe_eses_get_next_ses_subencl_desc_p(subencl_desc_ptr);
    }

    return(status);
}

/*********************************************************************
* config_page_convert_fw_rev_to_subencl_prod_rev_level ()
**********************************************************************
*
*  Description:
*   Convert the version discriptor component rev level to subEnclosure product rev level.
*
*  Inputs:
*   the version discriptor component rev level.
*
*  Return Value: success or failure
*
*  Notes:
*
*  History:
*    Jan-06-2014    PHE - created
*
*********************************************************************/
static fbe_status_t config_page_convert_fw_rev_to_subencl_prod_rev_level(
                                           ses_subencl_prod_rev_level_struct * subencl_prod_rev_level_p, 
                                           fbe_char_t * comp_rev_level_p)
{
    fbe_char_t majorNumber[3] = {0};
    fbe_char_t minorNumber[3] = {0};
    fbe_char_t debugVersion[2] = {0};
    fbe_u32_t temp1 = 0, temp2 = 0, temp3 = 0, temp = 0;
    fbe_u8_t digits[5] = {0};

    majorNumber[0] = *comp_rev_level_p;
    majorNumber[1] = *(comp_rev_level_p + 1);
    if(*(comp_rev_level_p + 2) == '.') 
    {
        minorNumber[0] = *(comp_rev_level_p + 3);
        minorNumber[1] = *(comp_rev_level_p + 4);
        debugVersion[0] = ' ';
    }
    else
    {
        minorNumber[0] = *(comp_rev_level_p + 2);
        minorNumber[1] = *(comp_rev_level_p + 3);
        debugVersion[0] = *(comp_rev_level_p + 4);
    }

    temp1 = atoi(&majorNumber[0]) << 11;
    temp2 = atoi(&minorNumber[0]) << 4;

    if(debugVersion[0] == ' ') 
    {
        temp3 = 0;
    }
    else
    {
        temp3 = debugVersion[0] - 'a' + 1;
    }

    temp = temp1|temp2|temp3;
    fbe_sprintf(&digits[0], 5, "%04X", temp);
    memcpy(&subencl_prod_rev_level_p->digits[0], &digits[0], 4);

    return FBE_STATUS_OK;
}


/*********************************************************************
* config_page_initialize_config_page_info ()
**********************************************************************
*
*  Description:
*   Fills the configuration page related info in the virtual
*   phy object during its creation.
*
*  Inputs:
*   Pointer to the virtual PHy configuration page information.
*
*  Return Value: success or failure
*
*  Notes:
*
*  History:
*    Nov-19-2008    Rajesh V created
*
*********************************************************************/
fbe_status_t config_page_initialize_config_page_info(fbe_sas_enclosure_type_t encl_type, vp_config_diag_page_info_t *vp_config_diag_page_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t subencl, buf_desc, buf;
    terminator_sp_id_t spid;
    RESUME_PROM_STRUCTURE * pResumeData = NULL;
    dev_eeprom_rev0_info_struct * pEepromData = NULL;
    fbe_u32_t checksum = 0;
    fbe_u32_t viking_ps_buf_len = 0;
    
    status =  fbe_terminator_api_get_sp_id(&spid);


    switch(encl_type)
    {
        case FBE_SAS_ENCLOSURE_TYPE_BULLET:
        case FBE_SAS_ENCLOSURE_TYPE_VIPER:
            vp_config_diag_page_info->config_page = viper_config_page_with_ps[spid];
            vp_config_diag_page_info->config_page_info = &viper_config_page_info_with_ps;
            vp_config_diag_page_info->config_page_size = sizeof(viper_config_page_with_ps[spid]);
            break;

        case FBE_SAS_ENCLOSURE_TYPE_MAGNUM:
            vp_config_diag_page_info->config_page = magnum_config_page_with_ps;
            vp_config_diag_page_info->config_page_info = &magnum_config_page_info_with_ps;
            vp_config_diag_page_info->config_page_size = sizeof(magnum_config_page_with_ps);
            break;

     case FBE_SAS_ENCLOSURE_TYPE_BUNKER:
            vp_config_diag_page_info->config_page = bunker_config_page_with_ps;
            vp_config_diag_page_info->config_page_info = &bunker_config_page_info_with_ps;
            vp_config_diag_page_info->config_page_size = sizeof(bunker_config_page_with_ps);
            break;

    case FBE_SAS_ENCLOSURE_TYPE_CITADEL:
            vp_config_diag_page_info->config_page = citadel_config_page_with_ps[spid];
            vp_config_diag_page_info->config_page_info = &citadel_config_page_info_with_ps;
            vp_config_diag_page_info->config_page_size = sizeof(citadel_config_page_with_ps[spid]);
            break;

        case FBE_SAS_ENCLOSURE_TYPE_DERRINGER:
            vp_config_diag_page_info->config_page = derringer_config_page_with_ps[spid];
            vp_config_diag_page_info->config_page_info = &derringer_config_page_info_with_ps;
            vp_config_diag_page_info->config_page_size = sizeof(derringer_config_page_with_ps[spid]);
            break;

        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM:
            vp_config_diag_page_info->config_page = voyager_icm_config_page_with_ps[spid];
            vp_config_diag_page_info->config_page_info = &voyager_icm_config_page_info_with_ps;
            vp_config_diag_page_info->config_page_size = sizeof(voyager_icm_config_page_with_ps[spid]);
            break;

        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE:
            vp_config_diag_page_info->config_page = voyager_ee_config_page_with_ps[spid];
            vp_config_diag_page_info->config_page_info = &voyager_ee_config_page_info_with_ps;
            vp_config_diag_page_info->config_page_size = sizeof(voyager_ee_config_page_with_ps[spid]);
            break;
            
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP:
            vp_config_diag_page_info->config_page = viking_iosxp_config_page_with_ps[spid];
            vp_config_diag_page_info->config_page_info = &viking_iosxp_config_page_info_with_ps;
            vp_config_diag_page_info->config_page_size = sizeof(viking_iosxp_config_page_with_ps[spid]);
            break;

        case FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP:
            vp_config_diag_page_info->config_page = viking_drvsxp_config_page_with_ps[spid];
            vp_config_diag_page_info->config_page_info = &viking_drvsxp_config_page_info_with_ps;
            vp_config_diag_page_info->config_page_size = sizeof(viking_drvsxp_config_page_with_ps[spid]);
            break;

        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP:
            vp_config_diag_page_info->config_page = cayenne_iosxp_config_page_with_ps[spid];
            vp_config_diag_page_info->config_page_info = &cayenne_iosxp_config_page_info_with_ps;
            vp_config_diag_page_info->config_page_size = sizeof(cayenne_iosxp_config_page_with_ps[spid]);
            break;

        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP:
            vp_config_diag_page_info->config_page = cayenne_drvsxp_config_page_with_ps[spid];
            vp_config_diag_page_info->config_page_info = &cayenne_drvsxp_config_page_info_with_ps;
            vp_config_diag_page_info->config_page_size = sizeof(cayenne_drvsxp_config_page_with_ps[spid]);
            break;      

        case FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP:
            vp_config_diag_page_info->config_page = naga_iosxp_config_page_with_ps[spid];
            vp_config_diag_page_info->config_page_info = &naga_iosxp_config_page_info_with_ps;
            vp_config_diag_page_info->config_page_size = sizeof(naga_iosxp_config_page_with_ps[spid]);
            break;

        case FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP:
            vp_config_diag_page_info->config_page = naga_drvsxp_config_page_with_ps[spid];
            vp_config_diag_page_info->config_page_info = &naga_drvsxp_config_page_info_with_ps;
            vp_config_diag_page_info->config_page_size = sizeof(naga_drvsxp_config_page_with_ps[spid]);
            break;

        case FBE_SAS_ENCLOSURE_TYPE_FALLBACK:
            vp_config_diag_page_info->config_page = fallback_config_page_with_ps[spid];
            vp_config_diag_page_info->config_page_info = &fallback_config_page_info_with_ps;
            vp_config_diag_page_info->config_page_size = sizeof(fallback_config_page_with_ps[spid]);
            break;

        case FBE_SAS_ENCLOSURE_TYPE_BOXWOOD:
            vp_config_diag_page_info->config_page = boxwood_config_page_with_ps[spid];
            vp_config_diag_page_info->config_page_info = &boxwood_config_page_info_with_ps;
            vp_config_diag_page_info->config_page_size = sizeof(boxwood_config_page_with_ps[spid]);
            break;

        case FBE_SAS_ENCLOSURE_TYPE_KNOT:
            vp_config_diag_page_info->config_page = knot_config_page_with_ps[spid];
            vp_config_diag_page_info->config_page_info = &knot_config_page_info_with_ps;
            vp_config_diag_page_info->config_page_size = sizeof(knot_config_page_with_ps[spid]);
            break;

        case FBE_SAS_ENCLOSURE_TYPE_PINECONE:
            vp_config_diag_page_info->config_page = pinecone_config_page_with_ps[spid];
            vp_config_diag_page_info->config_page_info = &pinecone_config_page_info_with_ps;
            vp_config_diag_page_info->config_page_size = sizeof(pinecone_config_page_with_ps[spid]);
            break;

        case FBE_SAS_ENCLOSURE_TYPE_STEELJAW:
            vp_config_diag_page_info->config_page = steeljaw_config_page_with_ps[spid];
            vp_config_diag_page_info->config_page_info = &steeljaw_config_page_info_with_ps;
            vp_config_diag_page_info->config_page_size = sizeof(steeljaw_config_page_with_ps[spid]);
            break;

        case FBE_SAS_ENCLOSURE_TYPE_RAMHORN:
            vp_config_diag_page_info->config_page = ramhorn_config_page_with_ps[spid];
            vp_config_diag_page_info->config_page_info = &ramhorn_config_page_info_with_ps;
            vp_config_diag_page_info->config_page_size = sizeof(ramhorn_config_page_with_ps[spid]);
            break;

        case FBE_SAS_ENCLOSURE_TYPE_ANCHO:
            vp_config_diag_page_info->config_page = ancho_config_page_with_ps[spid];
            vp_config_diag_page_info->config_page_info = &ancho_config_page_info_with_ps;
            vp_config_diag_page_info->config_page_size = sizeof(ancho_config_page_with_ps[spid]);
            break;

        case FBE_SAS_ENCLOSURE_TYPE_CALYPSO:
            vp_config_diag_page_info->config_page = calypso_config_page_with_ps[spid];
            vp_config_diag_page_info->config_page_info = &calypso_config_page_info_with_ps;
            vp_config_diag_page_info->config_page_size = sizeof(calypso_config_page_with_ps[spid]);
            break;

        case FBE_SAS_ENCLOSURE_TYPE_RHEA:
            vp_config_diag_page_info->config_page = rhea_config_page_with_ps[spid];
            vp_config_diag_page_info->config_page_info = &rhea_config_page_info_with_ps;
            vp_config_diag_page_info->config_page_size = sizeof(rhea_config_page_with_ps[spid]);
            break;

        case FBE_SAS_ENCLOSURE_TYPE_MIRANDA:
            vp_config_diag_page_info->config_page = miranda_config_page_with_ps[spid];
            vp_config_diag_page_info->config_page_info = &miranda_config_page_info_with_ps;
            vp_config_diag_page_info->config_page_size = sizeof(miranda_config_page_with_ps[spid]);
            break;

        case FBE_SAS_ENCLOSURE_TYPE_TABASCO:
            vp_config_diag_page_info->config_page = tabasco_config_page_with_ps[spid];
            vp_config_diag_page_info->config_page_info = &tabasco_config_page_info_with_ps;
            vp_config_diag_page_info->config_page_size = sizeof(tabasco_config_page_with_ps[spid]);
            break;
        default:
            return(FBE_STATUS_GENERIC_FAILURE);

    }

    status = config_page_get_gen_code_by_config_page(vp_config_diag_page_info->config_page, &vp_config_diag_page_info->gen_code);
    RETURN_ON_ERROR_STATUS;

    // Initialize Version descriptor information.
    vp_config_diag_page_info->ver_desc = NULL;
    if(vp_config_diag_page_info->config_page_info->num_ver_descs != 0)
    {
        vp_config_diag_page_info->ver_desc = (ses_ver_desc_struct *)fbe_terminator_allocate_memory((vp_config_diag_page_info->config_page_info->num_ver_descs)*
                                                                        (sizeof(ses_ver_desc_struct)));
        if(vp_config_diag_page_info->ver_desc == NULL)
        {
            return(FBE_STATUS_GENERIC_FAILURE);
        }

        // Copy the version descriptor array built from the
        // configuration page into the version descriptor
        // array of the virtual phy/ enclosure object.
        memcpy(vp_config_diag_page_info->ver_desc,
            vp_config_diag_page_info->config_page_info->ver_desc_array,
            (vp_config_diag_page_info->config_page_info->num_ver_descs)*sizeof(ses_ver_desc_struct));
    }


    // Initialize the buffer information

    vp_config_diag_page_info->num_bufs = 0;
    vp_config_diag_page_info->buf_info = NULL;
    if(vp_config_diag_page_info->config_page_info->num_buf_descs != 0)
    {
        vp_config_diag_page_info->buf_info = (terminator_eses_buf_info_t *)fbe_terminator_allocate_memory((vp_config_diag_page_info->config_page_info->num_buf_descs) *
                                                                                (sizeof(terminator_eses_buf_info_t)));

        if(vp_config_diag_page_info->buf_info == NULL)
        {
            return(FBE_STATUS_INSUFFICIENT_RESOURCES);
        }

        vp_config_diag_page_info->num_bufs = vp_config_diag_page_info->config_page_info->num_buf_descs;

        buf = 0;
        // Get the buffer descirptors in each of the subenclosure and fill the buffer information
        // in the virtual phy accordingly.
        for(subencl=0; subencl < vp_config_diag_page_info->config_page_info->num_subencls; subencl++)
        {
            for(buf_desc=0; buf_desc < vp_config_diag_page_info->config_page_info->subencl_buf_desc_info[subencl].num_buf_descs; buf_desc++, buf++)
            {
                vp_config_diag_page_info->buf_info[buf].buf_id = vp_config_diag_page_info->config_page_info->subencl_buf_desc_info[subencl].buf_desc[buf_desc].buf_id;
                vp_config_diag_page_info->buf_info[buf].writable = vp_config_diag_page_info->config_page_info->subencl_buf_desc_info[subencl].buf_desc[buf_desc].writable;
                vp_config_diag_page_info->buf_info[buf].buf_type = vp_config_diag_page_info->config_page_info->subencl_buf_desc_info[subencl].buf_desc[buf_desc].buf_type;
                vp_config_diag_page_info->buf_info[buf].buf_offset_boundary = 0x2;
                
                if( ((encl_type == FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP) || 
                     (encl_type == FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP) ||
                     (encl_type == FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP) ||
                     (encl_type == FBE_SAS_ENCLOSURE_TYPE_TABASCO))  &&
                   (vp_config_diag_page_info->config_page_info->subencl_desc_array[subencl].subencl_type == SES_SUBENCL_TYPE_PS) &&
                   (vp_config_diag_page_info->buf_info[buf].buf_type == SES_BUF_TYPE_EEPROM)) 
                {
                    /* Viking PS resume buffer size is 336. */
                    viking_ps_buf_len = 336;
                    vp_config_diag_page_info->buf_info[buf].buf = (fbe_u8_t *)fbe_terminator_allocate_memory(viking_ps_buf_len);
                    vp_config_diag_page_info->buf_info[buf].buf_len = viking_ps_buf_len;
                    fbe_zero_memory(vp_config_diag_page_info->buf_info[buf].buf, viking_ps_buf_len);
                    pEepromData = (dev_eeprom_rev0_info_struct *)vp_config_diag_page_info->buf_info[buf].buf;
                    fbe_copy_memory(&pEepromData->emc_serialnum, "SimulateSN", 10);
                }
                else if(vp_config_diag_page_info->buf_info[buf].buf_type == SES_BUF_TYPE_EEPROM)
                {
                    vp_config_diag_page_info->buf_info[buf].buf = (fbe_u8_t *)fbe_terminator_allocate_memory(sizeof(RESUME_PROM_STRUCTURE));
                    vp_config_diag_page_info->buf_info[buf].buf_len = sizeof(RESUME_PROM_STRUCTURE);
                    fbe_zero_memory(vp_config_diag_page_info->buf_info[buf].buf, sizeof(RESUME_PROM_STRUCTURE));
                    pResumeData = (RESUME_PROM_STRUCTURE *)vp_config_diag_page_info->buf_info[buf].buf;
                    checksum = calculateResumeChecksum(pResumeData);
                    pResumeData->resume_prom_checksum = checksum;
                }
                else
                {
                    vp_config_diag_page_info->buf_info[buf].buf = NULL;
                    vp_config_diag_page_info->buf_info[buf].buf_len = 0;
                }
            }
        }
    }


    return(FBE_STATUS_OK);
}

/*********************************************************************
* config_page_set_all_ver_descs_in_config_page ()
**********************************************************************
*
*  Description:
*   Sets the given version descriptors in the given configuration page.
*
*  Inputs:
*   Configuration page, configuration page info having parsed
*   information about the given config page, version descriptor
*   array to be filled into the configuration page.
*
*  Return Value: success or failure
*
*  Notes:
*
*  History:
*    Nov-19-2008    Rajesh V created
*
*********************************************************************/
fbe_status_t config_page_set_all_ver_descs_in_config_page(fbe_u8_t *config_page,
                                                        terminator_eses_config_page_info_t *config_page_info,
                                                        ses_ver_desc_struct *ver_desc,
                                                        fbe_u16_t eses_version)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t subencl;
    fbe_u8_t ver_desc_start_index, num_of_ver_descs;

    // The configuration page is loaded here.
    for(subencl = 0; subencl < config_page_info->num_subencls; subencl++)
    {
        status = config_page_info_get_subencl_ver_desc_position(config_page_info,
                                                                config_page_info->subencl_desc_array[subencl].subencl_type,
                                                                config_page_info->subencl_slot[subencl],
                                                                &ver_desc_start_index,
                                                                &num_of_ver_descs);
        RETURN_ON_ERROR_STATUS;

        status = config_page_set_subencl_ver_descs_in_config_page(config_page,
                                                                   config_page_info->subencl_desc_array[subencl].subencl_type,
                                                                   config_page_info->subencl_slot[subencl],
                                                                   &ver_desc[ver_desc_start_index],
                                                                   num_of_ver_descs,
                                                                   eses_version);
        RETURN_ON_ERROR_STATUS;
    }

    return(FBE_STATUS_OK);
}

/*********************************************************************
* build_configuration_diagnostic_page()
*********************************************************************
*
*  Description: This builds the Configuration diagnostic page.
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
fbe_status_t build_configuration_diagnostic_page(fbe_sg_element_t * sg_list,
                                                 fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status;
    vp_config_diag_page_info_t *config_diag_page_info;
    fbe_u8_t *buffer = NULL;
    fbe_u16_t eses_version = 0;

    buffer = fbe_sg_element_address(sg_list);
    if(buffer == NULL)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Get the eses_version_desc ESES_REVISION_1_0 or ESES_REVISION_2_0 */
    fbe_terminator_sas_enclosure_get_eses_version_desc(virtual_phy_handle, &eses_version);

    status = eses_get_config_diag_page_info(virtual_phy_handle, &config_diag_page_info);
    RETURN_ON_ERROR_STATUS;

    memcpy(buffer, config_diag_page_info->config_page, (config_diag_page_info->config_page_size) * sizeof(fbe_u8_t));
    status = config_page_set_all_ver_descs_in_config_page(buffer,
                                                          config_diag_page_info->config_page_info,
                                                          config_diag_page_info->ver_desc,
                                                          eses_version);

    RETURN_ON_ERROR_STATUS;


    status = config_page_set_gen_code_by_config_page(buffer,
                                                     config_diag_page_info->gen_code);
    RETURN_ON_ERROR_STATUS;

    return(FBE_STATUS_OK);
}

/*********************************************************************
* config_page_info_get_subencl_ver_desc_position ()
**********************************************************************
*
*  Description:
*   Gets the position of the "version descriptors" for the given
*   Subenclosure.
*
*  Inputs:
*   Configuration page info having parsed information about the config
*   page, Subenclosure type & side, starting index of the version descs,
*   number of version descriptors in the subenclosure.
*
*  Return Value: success or failure
*
*  Notes:
*
*  History:
*    Nov-19-2008    Rajesh V created
*
*********************************************************************/
fbe_status_t config_page_info_get_subencl_ver_desc_position(terminator_eses_config_page_info_t *config_page_info,
                                                            ses_subencl_type_enum subencl_type,
                                                            terminator_eses_subencl_side side,
                                                            fbe_u8_t *start_index,
                                                            fbe_u8_t *num_ver_descs)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t subencl;

    *start_index = 0;
    *num_ver_descs = 0;
    for(subencl = 0; subencl < config_page_info->num_subencls; subencl++)
    {
        if((config_page_info->subencl_desc_array[subencl].subencl_type == subencl_type) &&
           (config_page_info->subencl_slot[subencl] == side))
        {
            *num_ver_descs = config_page_info->subencl_desc_array[subencl].num_ver_descs;
            status = FBE_STATUS_OK;
            break;

        }

        (*start_index) += config_page_info->subencl_desc_array[subencl].num_ver_descs;
    }

    return(status);
}

/*********************************************************************
* config_page_get_subencl_id ()
**********************************************************************
*
*  Description:
*   Gets Subenclosure ID
*
*  Inputs:
*   Virtual Phy configuration information, Subenclosure Type & side,
*   Subenclosure ID to be returned.
*
*  Return Value: success or failure
*
*  Notes:
*
*  History:
*    Nov-20-2008    Rajesh V created
*
*********************************************************************/
fbe_status_t config_page_get_subencl_id(vp_config_diag_page_info_t vp_config_diag_page_info,
                                        ses_subencl_type_enum subencl_type,
                                        terminator_eses_subencl_side side,
                                        fbe_u8_t *subencl_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t subencl;

    for(subencl = 0; subencl < vp_config_diag_page_info.config_page_info->num_subencls; subencl++)
    {
        if((vp_config_diag_page_info.config_page_info->subencl_desc_array[subencl].subencl_type == subencl_type) &&
           (vp_config_diag_page_info.config_page_info->subencl_slot[subencl] == side))
        {
            status = FBE_STATUS_OK;
            *subencl_id = subencl;
            break;

        }
    }

    return(status);
}

/*********************************************************************
* config_page_get_num_of_sec_subencls_by_config_page ()
**********************************************************************
*
*  Description:
*   Gets the number of subenclosures in given config page.
*
*  Inputs:
*   config page & number of subenclosures to be returned.
*
*  Return Value: success or failure
*
*  Notes:
*
*  History:
*    Nov-20-2008    Rajesh V created
*
*********************************************************************/
fbe_status_t config_page_get_num_of_sec_subencls_by_config_page(fbe_u8_t *config_page,
                                                                fbe_u8_t *num_sec_subencls)
{
    ses_pg_config_struct *config_pg_hdr;

    if(config_page == NULL)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    config_pg_hdr = (ses_pg_config_struct *)config_page;
    *num_sec_subencls = config_pg_hdr->num_secondary_subencls;

    return(FBE_STATUS_OK);
}

/*********************************************************************
* config_page_get_num_of_sec_subencls ()
**********************************************************************
*
*  Description:
*   Gets the number of subenclosures in given virtual phy object.
*
*  Inputs:
*   virtual Phy handle & number of subenclosures to be returned.
*
*  Return Value: success or failure
*
*  Notes:
*
*  History:
*    Nov-20-2008    Rajesh V created
*
*********************************************************************/
fbe_status_t config_page_get_num_of_sec_subencls(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                 fbe_u8_t *num_sec_subencls)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    vp_config_diag_page_info_t *config_diag_page_info;

    status = eses_get_config_diag_page_info(virtual_phy_handle, &config_diag_page_info);
    RETURN_ON_ERROR_STATUS;

    status = config_page_get_num_of_sec_subencls_by_config_page(config_diag_page_info->config_page,
                                                                num_sec_subencls);
    return(status);
}

/*********************************************************************
* config_page_get_gen_code ()
**********************************************************************
*
*  Description:
*   Gets the Generation code being used by given virtual phy object.
*
*  Inputs:
*   virtual Phy handle & generation code to be returned.
*
*  Return Value: success or failure
*
*  Notes:
*
*  History:
*    Nov-21-2008    Rajesh V created
*
*********************************************************************/
fbe_status_t config_page_get_gen_code(fbe_terminator_device_ptr_t virtual_phy_handle,
                                      fbe_u32_t *gen_code)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    vp_config_diag_page_info_t *config_diag_page_info;

    status = eses_get_config_diag_page_info(virtual_phy_handle, &config_diag_page_info);
    RETURN_ON_ERROR_STATUS;

    *gen_code = config_diag_page_info->gen_code;

    return(FBE_STATUS_OK);
}

/*********************************************************************
* config_page_info_get_buf_id_by_subencl_info ()
**********************************************************************
*
*  Description:
*   Gets the buffer identifier of the required buffer, uniquely identified
*   by the subenclosure and buffer type information.
*
*  Inputs:
*   virtual phy configuration data, subenclosure type, subenclosure side,
*   buffer type, other buffer attributes to consider, pointer to the
*   "buffer id" to be filled in.
*
*  Return Value: success or failure
*
*  Notes:
*
*  History:
*    JAN-08-2008    Rajesh V created
*
*********************************************************************/
fbe_status_t config_page_info_get_buf_id_by_subencl_info(vp_config_diag_page_info_t vp_config_diag_page_info,
                                                        ses_subencl_type_enum subencl_type,
                                                        terminator_eses_subencl_side side,
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
    fbe_u8_t i, subencl_id;
    terminator_eses_config_page_info_t *config_page_info;
    terminator_eses_subencl_buf_desc_info_t *subencl_buf_desc_info;


    status = config_page_get_subencl_id(vp_config_diag_page_info, subencl_type, side, &subencl_id);
    RETURN_ON_ERROR_STATUS;

    // reset status before searching for buffer.
    status = FBE_STATUS_GENERIC_FAILURE;

    config_page_info = vp_config_diag_page_info.config_page_info;
    // ***The below is based on assumption that subenclosure descs
    // in current config page are arranged consecutively by
    // Subenclosure ID. Only a very minor change is required if
    // this assumption no longer holds in future ***.
    subencl_buf_desc_info = &config_page_info->subencl_buf_desc_info[subencl_id];
    // Find the required buffer descriptor in all the buffer descriptors that
    // belong to a particular Subenclosure.
    for(i=0; i < subencl_buf_desc_info->num_buf_descs; i++)
    {
        if( (subencl_buf_desc_info->buf_desc[i].buf_type == buf_type) &&
            ((!consider_writable) || (subencl_buf_desc_info->buf_desc[i].writable == writable)) &&
            ((!consider_buf_index) || (subencl_buf_desc_info->buf_desc[i].buf_index == buf_index)) &&
            ((!consider_buf_spec_info) || (subencl_buf_desc_info->buf_desc[i].buf_spec_info == buf_spec_info)) )
        {
            *buf_id = subencl_buf_desc_info->buf_desc[i].buf_id;
            status = FBE_STATUS_OK;
            break;
        }
    }
    return(status);
}


/*********************************************************************
*  config_page_element_exists()
**********************************************************************
*
*  Description:
*   This function determines if the given element exists or not in the
*   virtual phy ESES configuration info.
*
*  Inputs:
*   subencl_type - subenclosure type
*   side - side (A or B indicated by 0 & 1 now)
*   elem_type - Status element type.
*   Other ESES attributes to distinguish similar kind of elements
*
*  Return Value: TRUE or FALSE
*
*  Notes:
*
*  History:
*    April-20-2009  Rajesh V created
*
*********************************************************************/

fbe_bool_t config_page_element_exists(fbe_terminator_device_ptr_t virtual_phy_handle,
                                        ses_subencl_type_enum subencl_type,
                                        terminator_eses_subencl_side side,
                                        ses_elem_type_enum elem_type,
                                        fbe_bool_t consider_num_possible_elems,
                                        fbe_u8_t num_possible_elems,
                                        fbe_bool_t consider_type_desc_text,
                                        fbe_u8_t *type_desc_text)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_eses_config_page_info_t *current_config_page_info;
    vp_config_diag_page_info_t *vp_config_diag_page_info;

    // Get the virtual phy configuration information for the given Virtual Phy handle
    status = eses_get_config_diag_page_info(virtual_phy_handle, &vp_config_diag_page_info);
    RETURN_ON_ERROR_STATUS;

    // Get the base configuration page information from the VP config info.
    current_config_page_info = vp_config_diag_page_info->config_page_info;

    return(config_page_info_element_exists(current_config_page_info,
                                        subencl_type,
                                        side,
                                        elem_type,
                                        consider_num_possible_elems,
                                        num_possible_elems,
                                        consider_type_desc_text,
                                        type_desc_text));
}

/*********************************************************************
* config_page_info_element_exists ()
**********************************************************************
*
*  Description:
*   This function determines if the given element exists or not in the
*   given base configuration information
*
*  Inputs:
*   subencl_type - subenclosure type
*   side - side (A or B indicated by 0 & 1 now)
*   elem_type - Status element type.
*   offset - pointer to offset to be returned
*   Other ESES attributes to distinguish similar kind of elements
*
*  Return Value: TRUE or FALSE
*
*  Notes:
*
*  History:
*    April-20-2009  Rajesh V created
*
*********************************************************************/
fbe_bool_t config_page_info_element_exists(terminator_eses_config_page_info_t *config_page_info,
                                            ses_subencl_type_enum subencl_type,
                                            terminator_eses_subencl_side side,
                                            ses_elem_type_enum elem_type,
                                            fbe_bool_t consider_num_possible_elems,
                                            fbe_u8_t num_possible_elems,
                                            fbe_bool_t consider_type_desc_text,
                                            fbe_u8_t *type_desc_text)
{
    fbe_u8_t i;

    // Ignore type descriptor text related parameters for now.
    UNREFERENCED_PARAMETER(consider_type_desc_text);
    UNREFERENCED_PARAMETER(type_desc_text);

    for(i=0; i < config_page_info->num_type_desc_headers ;i++)
    {
        if((config_page_info->type_desc_array[i].elem_type == elem_type) &&
           ((!consider_num_possible_elems) || (config_page_info->type_desc_array[i].num_possible_elems == num_possible_elems)) &&
           ((config_page_info->subencl_desc_array[config_page_info->type_desc_array[i].subencl_id].subencl_type)
                == subencl_type ) &&
           ((config_page_info->subencl_desc_array[config_page_info->type_desc_array[i].subencl_id].side)
                == side) )
        {
            return(FBE_TRUE);
        }
    }

    return(FBE_FALSE);
}

/*********************************************************************
* config_page_get_subencl_desc_by_subencl_id ()
**********************************************************************
*
*  Description:
*   Gets the subenclosure descriptor info given the subenclosure ID.
*
*  Inputs:
*   Virtual Phy config page info, subenclosure ID.
*
*  Return Value:
*   success or failure
*
*  Notes:
*   The subenclosure descriptor structure has all the attributes of the
*   given subenclosure
*
*  History:
*    Oct-08-2010    Rajesh V created
*
*********************************************************************/
fbe_status_t 
config_page_get_subencl_desc_by_subencl_id(vp_config_diag_page_info_t *vp_config_diag_page_info,
                                           fbe_u8_t subencl_id,
                                           ses_subencl_desc_struct **subencl_desc)
{
   
    *subencl_desc = 
        &vp_config_diag_page_info->config_page_info->subencl_desc_array[subencl_id];

    return FBE_STATUS_OK;
}
