/*********************************************************************
 * Copyright (C) EMC Corporation, 1989 - 2011
 * All Rights Reserved.
 * Licensed Material-Property of EMC Corporation.
 * This software is made available solely pursuant to the terms
 * of a EMC license agreement which governs its use.
 *********************************************************************/

/*********************************************************************
 *
 *  Description:
 *      This file defines the SEP BIND INTERFACE class.
 *
 *  History:
 *      25-May-2011 : initial version
 *
 *********************************************************************/

#ifndef T_SEP_BIND_CLASS_H
#define T_SEP_BIND_CLASS_H

#ifndef T_SEP_CLASS_H
#include "sep_class.h"
#endif

// General purpose sizes imported from FLARE generics.h
#define FBE_KILOBYTE        1024
#define FBE_MEGABYTE        1048576
#define FBE_GIGABYTE        1073741824
#define FBE_TERABYTE        1099511627776


// This is defined in fbe_lun_private.h and should be changed 
//when the value for this in that files changes
// in blocks ( 2048 blocks )
#define FBE_LUN_CHUNK_SIZE 0x800
#define FBE_CLI_BIND_DEFAULT_COUNT 1

// assuming 10K LUN limit */
#define MAX_LUN_ID 10240 
#define INVALID_LUN_ID MAX_LUN_ID+1


/*********************************************************************
 *          SEP LUN DETAILS STRUCT
 *********************************************************************/

typedef struct lun_details_s {
    fbe_lun_number_t      lun_number;
    fbe_lifecycle_state_t lifecycle_state;
    fbe_database_lun_info_t lun_info;
    const char            *p_lifecycle_state_str;
    const char            *p_rg_str;
    fbe_api_lun_get_zero_status_t fbe_api_lun_get_zero_status;
}lun_details_t;

/*********************************************************************
 *          SEP BIND class : bSEP base class
 *********************************************************************/

class sepBind : public bSEP
{
    protected:

        // Every object has an idnumber
        unsigned idnumber;
        unsigned sepBindCount;
        char params_temp[250];

        // interface object name
        char  * idname;

        // SEP BIND Interface function calls and usage
        char * sepBindInterfaceFuncs;
        char * usage_format;

        // SEP BIND interface fbe api data structures
        fbe_api_lun_create_t            lun_create_req;

        // By default create LUN with capacity of 0x1000
        fbe_lba_t                       lun_capacity;

        fbe_bool_t                      is_ndb_req;
        fbe_lba_t                       addr_offset;
        fbe_lun_number_t                lun_no;
        fbe_assigned_wwid_t             wwn;   
        fbe_block_transport_placement_t block_transport_placement;
        fbe_bool_t                      is_create_rg_b;
        fbe_api_raid_group_get_info_t   raid_group_info_p;
        fbe_u32_t                       lun_cap_offset;

        fbe_object_id_t                 lu_id;
        fbe_u32_t                       index;
        fbe_u32_t                       cnt;

        // private methods
        fbe_status_t _bind(char **argv);
        fbe_status_t extract_optional_arguments(char **argv);
        
        fbe_lba_t convert_size_unit_to_lba(
            fbe_lba_t unit_size,
            fbe_u8_t *str);

        fbe_bool_t validate_atoi_args(const TEXT * arg_value);

        fbe_bool_t checks_for_similar_wwn_number(
            fbe_assigned_wwid_t wwn);

        fbe_status_t get_lun_details(
            fbe_object_id_t lun_id, 
            lun_details_t* lun_details);

        fbe_status_t get_RG_name(
            fbe_u32_t  rg_type, 
            const fbe_char_t** pp_raid_type_name);

        fbe_status_t get_state_name(
            fbe_lifecycle_state_t state, 
            const fbe_char_t  ** pp_state_name);

        fbe_status_t find_smallest_free_lun_number(
            fbe_lun_number_t* lun_number_p);

        fbe_status_t validate_lun_request(
            fbe_api_lun_create_t *lun_create_req);

        void sep_bind_initializing_variables();
    public:

        // Constructor/Destructor
        sepBind();
        ~sepBind(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);

        // helper method 
        fbe_lba_t convert_str_to_lba(fbe_u8_t *str);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]);

        // bind with raid group method
        fbe_status_t bind_rg (int argc, char ** argv);

        // ------------------------------------------------------------
};

#endif
