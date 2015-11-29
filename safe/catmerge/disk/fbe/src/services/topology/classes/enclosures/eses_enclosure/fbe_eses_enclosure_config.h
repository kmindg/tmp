#ifndef ESES_ENCLOSURE_CONFIG_H
#define ESES_ENCLOSURE_CONFIG_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_eses.h"
#include "fbe_eses_enclosure_private.h"


/*************************
 * EDAL Component Type Handler
 *************************/
typedef struct fbe_eses_enclosure_comp_type_hdlr_s
{
    // This is the EDAL component type. See fbe_enclosure_component_types_t.
    fbe_enclosure_component_types_t  comp_type;

    // This function knows how to initialize the component specific configuration
    // and mapping info.
    fbe_enclosure_status_t (* init_comp_specific_config_info) (
                                      fbe_eses_enclosure_t * eses_enclosure,
                                      fbe_enclosure_component_types_t component_type,
                                      fbe_u8_t component_index);

    // This function knows how to extract a group of element status
    // and put them in the correct place in the correct format.
    // group_data points to the offset for the group
    fbe_enclosure_status_t (* extract_status) (void * enclosure,
                                               fbe_u8_t group_id,
                                               fbe_eses_ctrl_stat_elem_t * status_elem_ptr);
    // This function knows how to set up inidivual control for the element.
    // The position off the cntl_page is calculated off group_index, 
    // which can be retrieved from element_list table.
    // This function also updates overall control field if needed.
    fbe_enclosure_status_t (* setup_control) (void * enclosure,
                                              fbe_u32_t index,
                                              fbe_u32_t compType,
                                              fbe_u8_t *ctrl_data);

    // This function will take data from an EDAL blob & set the corresponding fields
    // in the enclosure's EDAL data
    fbe_enclosure_status_t (* copy_control) (void *enclosure,
                                             fbe_u32_t index,
                                             fbe_u32_t compType,
                                             void *controlDataPtr);

} fbe_eses_enclosure_comp_type_hdlr_t;


/*************************
 * Element Group
 *************************/


/***********************************************
 * Accessor methods for the element group 
 ***********************************************/

const fbe_eses_enclosure_comp_type_hdlr_t * fbe_eses_enclosure_get_comp_type_hdlr(
                          const fbe_eses_enclosure_comp_type_hdlr_t  * hdlr_table[],
                          fbe_enclosure_component_types_t component_type);

/* element group : elem_type*/
static __forceinline ses_elem_type_enum fbe_eses_elem_group_get_elem_type(fbe_eses_elem_group_t * elem_group,
                                                                            fbe_u8_t group_id)
{
    return elem_group[group_id].elem_type;
}

static __forceinline fbe_status_t fbe_eses_elem_group_set_elem_type(fbe_eses_elem_group_t * elem_group,
                                                                    fbe_u8_t group_id, 
                                                                    ses_elem_type_enum elem_type)
{
    elem_group[group_id].elem_type = elem_type;
    return FBE_STATUS_OK;
}

/* element group : num_possible_elems*/
static __forceinline fbe_u8_t fbe_eses_elem_group_get_num_possible_elems(fbe_eses_elem_group_t * elem_group,
                                                                fbe_u8_t group_id)
{
    return (elem_group[group_id].num_possible_elems);
}

static __forceinline fbe_status_t fbe_eses_elem_group_set_num_possible_elems(fbe_eses_elem_group_t * elem_group,
                                                              fbe_u8_t group_id, 
                                                              fbe_u8_t num_possible_elems)
{
    elem_group[group_id].num_possible_elems = num_possible_elems;
    return FBE_STATUS_OK;
}

/* element group : subencl_id*/
static __forceinline fbe_u8_t fbe_eses_elem_group_get_subencl_id(fbe_eses_elem_group_t * elem_group,
                                                                 fbe_u8_t group_id)
{
    return elem_group[group_id].subencl_id;
}

static __forceinline fbe_status_t fbe_eses_elem_group_set_subencl_id(fbe_eses_elem_group_t * elem_group,
                                                                     fbe_u8_t group_id, 
                                                                     fbe_u8_t subencl_id)
{
    elem_group[group_id].subencl_id = subencl_id;
    return FBE_STATUS_OK;
}
/* element group : byte_offset*/
static __forceinline fbe_u16_t fbe_eses_elem_group_get_group_byte_offset(fbe_eses_elem_group_t * elem_group,
                                                                              fbe_u8_t group_id)
{
    return elem_group[group_id].byte_offset;
}

static __forceinline fbe_status_t fbe_eses_elem_group_set_byte_offset(fbe_eses_elem_group_t * elem_group,
                                                                      fbe_u8_t group_id, 
                                                                      fbe_u16_t byte_offset)
{
    elem_group[group_id].byte_offset = byte_offset;
    return FBE_STATUS_OK;
}

/* element group : first_elem_index*/
static __forceinline fbe_u8_t fbe_eses_elem_group_get_first_elem_index(fbe_eses_elem_group_t * elem_group,
                                                                        fbe_u8_t group_id)
{
    return elem_group[group_id].first_elem_index;   
}

static __forceinline fbe_status_t fbe_eses_elem_group_set_first_elem_index(fbe_eses_elem_group_t * elem_group,
                                                                       fbe_u8_t group_id, 
                                                                       fbe_u8_t first_elem_index)
{
    elem_group[group_id].first_elem_index = first_elem_index;
    return FBE_STATUS_OK;
}

static __forceinline fbe_u8_t fbe_eses_elem_group_get_first_indiv_elem_offset(fbe_eses_elem_group_t * elem_group,
                                                                        fbe_u8_t group_id)
{
    if (SES_ELEM_INDEX_NONE == elem_group[group_id].first_elem_index)
    {
        return SES_ELEM_INDEX_NONE;
    }
    return (elem_group[group_id].first_elem_index + group_id + 1);   
}
static __forceinline fbe_u8_t fbe_eses_elem_group_elem_offset_to_elem_index(fbe_eses_elem_group_t * elem_group,
                                                                            fbe_u8_t target_element_offset,
                                                                            fbe_u8_t group_id)
{
    fbe_u8_t offset;

    if (SES_ELEM_INDEX_NONE == elem_group[group_id].first_elem_index)
    {
        return SES_ELEM_INDEX_NONE;
    }
    offset = fbe_eses_elem_group_get_first_indiv_elem_offset(elem_group, group_id);
    // sanity check
    if (offset > target_element_offset)
    {
        return SES_ELEM_INDEX_NONE;
    }
    offset = target_element_offset - offset;
    // sanity check
    if (offset > elem_group[group_id].num_possible_elems)
    {
        return SES_ELEM_INDEX_NONE;
    }
    return (elem_group[group_id].first_elem_index + offset);
}


   
static __forceinline fbe_u16_t fbe_eses_elem_group_get_elem_byte_offset(fbe_eses_elem_group_t * elem_group, 
                                        fbe_u8_t group_id,
                                        fbe_u8_t group_offset)
{
    return  (fbe_eses_elem_group_get_group_byte_offset(elem_group,group_id) + group_offset * FBE_ESES_CTRL_STAT_ELEM_SIZE);
} 


static __forceinline fbe_u8_t fbe_eses_elem_group_get_group_type(fbe_eses_enclosure_t * eses_enclosure, fbe_u8_t elem_type, fbe_u8_t start)
{
    fbe_u8_t group_id;
    fbe_u8_t group_limit;
    fbe_eses_elem_group_t * elem_group = NULL;

    group_limit = fbe_eses_enclosure_get_number_of_actual_elem_groups(eses_enclosure);
    if (start >= group_limit)
    {
        return FBE_ENCLOSURE_VALUE_INVALID;
    }

    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);
    for(group_id = start; group_id < group_limit; group_id ++)
    {
        if(elem_type == fbe_eses_elem_group_get_elem_type(elem_group, group_id))
        {
            return group_id;
        }
    }
    return FBE_ENCLOSURE_VALUE_INVALID;
}

static __forceinline fbe_u8_t fbe_eses_enclosure_elem_index_to_group_id(fbe_eses_enclosure_t * eses_enclosure, fbe_u8_t elem_index)
{
    fbe_u8_t group_id = 0;
    fbe_u8_t first_elem_index = 0;
    fbe_u8_t num_possible_elems = 0;
    fbe_eses_elem_group_t * elem_group = NULL;

    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);

    for(group_id = 0; group_id < fbe_eses_enclosure_get_number_of_actual_elem_groups(eses_enclosure); group_id ++)
    {
        num_possible_elems = fbe_eses_elem_group_get_num_possible_elems(elem_group, group_id);
        if (num_possible_elems != 0)
        {
            first_elem_index = fbe_eses_elem_group_get_first_elem_index(elem_group, group_id);
            if((elem_index >= first_elem_index) &&
               (elem_index < first_elem_index + num_possible_elems))
            {
                return group_id;
            }
        }
    }

    return FBE_ENCLOSURE_VALUE_INVALID;
}

static __forceinline fbe_u8_t fbe_eses_elem_group_get_elem_offset_group_id(fbe_eses_enclosure_t * eses_enclosure, fbe_u8_t target_offset, fbe_bool_t *is_overall)
{
    fbe_u8_t group_count = 0;
    fbe_u8_t overall_elem_offset = 0;   // the first elem offset in the group
    fbe_u8_t group_limit = 0;
    fbe_eses_elem_group_t * groupp = NULL;

    // Init to FALSE.
    *is_overall = FALSE;
    groupp = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);
    group_limit = fbe_eses_enclosure_get_number_of_actual_elem_groups(eses_enclosure);

    for(group_count = 0; group_count < group_limit; group_count++, groupp++)
    {
        if (overall_elem_offset == target_offset)
        {
            // this case is true for the overall element
            *is_overall = TRUE;
            return group_count;
            break;
        }
        else if (overall_elem_offset <= target_offset)
        {
            // check this group
            if ((overall_elem_offset + groupp->num_possible_elems) >= target_offset)
            {
                return group_count;
                break;
            }
        }

        // Adjust the current offset count by the number of 
        // elements in the current group. Add 1 to include the 
        // overall element (not represented by element index 
        // but is represented by statistics element offset
        overall_elem_offset += (groupp->num_possible_elems + 1);
    } // end for each group

    return FBE_ENCLOSURE_VALUE_INVALID;
}

/***********************************************
 * End of accessor methods for the element group 
 ***********************************************/

#endif // End of #ifndef ESES_ENCLOSURE_CONFIG_H
