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
 *      This file defines the SEP RAID GROUP INTERFACE class.
 *
 *  History:
 *      31-Mar-2011 : inital version
 *
 *********************************************************************/

#ifndef T_SEP_RAID_GROUP_CLASS_H
#define T_SEP_RAID_GROUP_CLASS_H

#include "global_catmerge_rg_limits.h"

#ifndef T_SEPCLASS_H
#include "sep_class.h"
#endif

#define RAID_INDIVIDUAL_DISK_WIDTH 1 
#define RAID_STRIPED_MIN_DISKS 2
#define RAID_STRIPED_MAX_DISK  16
#define RAID_MIRROR_MIN_DISKS  2
#define RAID3_STRIPED_MIN_DISK 5
#define RAID3_STRIPED_MAX_DISK 9
#define RAID6_STRIPED_MIN_DISK 3
#define RAID10_MIR_STRIPED_MIN_DISK 1
#define HOT_SPARE_NUM_DISK 1
#define MAX_RAID_GROUP_ID GLOBAL_CATMERGE_RAID_GROUP_ID_MAX

/*********************************************************************
 *          SEP RAID GROUP class : bSEP base class
 *********************************************************************/
class sepRaidGroup : public bSEP
{
    protected:

        // Every object has an idnumber
        unsigned idnumber;
        unsigned sepRaidGroupCount;
        char params_temp[250];

        // interface object name
        char  * idname;

        // SEP Raid Group Interface function calls and usage
        char * sepRaidGroupInterfaceFuncs;
        char * usage_format;

        // SEP Raid Group interface fbe api data structures
        fbe_api_raid_group_get_info_t             raid_group_info[FBE_MAX_DOWNSTREAM_OBJECTS];
        fbe_api_destroy_rg_t                      destroy_rg;
        fbe_raid_group_get_power_saving_info_t    rg_ps_policy;
        fbe_lun_verify_type_t                     lu_verify_type; 
        fbe_api_rg_get_status_t                   rebuild_status;
        fbe_api_rg_get_status_t                   bg_verify_status;
        
        // SEP Raid Group interface variables
        fbe_raid_group_number_t         raid_group_id;
        fbe_raid_group_type_t           raid_type;
        fbe_object_id_t                 drive_count;
        fbe_status_t                    status;
        fbe_object_id_t                 rg_obj_id;
        fbe_api_raid_group_create_t     rg_request;
        fbe_u64_t                       ps_idle_time;
        fbe_lba_t                       capacity;
        fbe_lun_verify_type_t           rg_verify_type;
        fbe_api_base_config_upstream_object_list_t upstream_object_list;
        fbe_api_base_config_upstream_object_list_t upstream_lun_object_list;
        fbe_u32_t lun_count;
        fbe_u32_t *raid_group_luns;
        fbe_raid_group_map_info_t  rg_map_info;
        fbe_u32_t index;
        fbe_chunk_index_t max_chunk_count;
        fbe_chunk_index_t chunk_offset;
        fbe_raid_group_paged_metadata_t *current_paged_p;
        fbe_raid_group_paged_metadata_t *paged_p ;
        fbe_lba_t logical_lba, physical_lba;
         fbe_api_job_service_raid_group_destroy_t in_fbe_raid_group_destroy_req;
         fbe_u32_t prefered_position;
         fbe_parity_get_write_log_info_t write_log_info;
         fbe_raid_group_control_get_bg_op_speed_t get_bg_op;
         fbe_raid_group_background_op_t bg_op_code;
         fbe_u32_t speed;

        // fbe_apix data structure to hold the list of drives in a RG
        fbe_apix_rg_drive_list_t rg_drives;

        // private methods
        char* edit_raid_group_info(void);

        fbe_status_t extract_optional_arguments(char **argv);

        fbe_status_t find_smallest_free_raid_group_number(
            fbe_raid_group_number_t *raid_group_id);

        fbe_raid_group_type_t check_valid_raid_type(
            fbe_u8_t *rg_type);
        
        fbe_status_t check_disks_for_raid_type(
            fbe_object_id_t drive_count,
            fbe_raid_group_type_t raid_type);

        fbe_status_t assign_validate_raid_group_id (
            fbe_raid_group_number_t *raid_group_id,
            fbe_object_id_t *rg_obj_id);

        void edit_raid_group_power_saving_policy(
            fbe_raid_group_get_power_saving_info_t *rg_ps_policy);

        void Sep_Raid_Intializing_variable();

        const fbe_char_t* raid_group_state(fbe_object_id_t 
        object_id);
        
        void get_bg_op_code(char**argv);

    public:

        // Constructor/Destructor
        sepRaidGroup();
        ~sepRaidGroup(){
            if (raid_group_luns){
                delete raid_group_luns;
            }
        }

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        int      MyCountIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]);

        // get Raid Group information method
        fbe_status_t get_raid_group_info (int argc, char ** argv);

        // create Raid Group method
        fbe_status_t create_raid_group (int argc, char **argv);

        // destroy raid group
        fbe_status_t destroy_raid_group (int argc, char ** argv);
        
        // get raid group power saving policy
        fbe_status_t get_raid_group_power_saving_policy(
            int argc, 
            char ** argv);

        // initiate verify
        fbe_status_t initiate_rg_verify (int argc, char ** argv);

        // get rebuild status
        fbe_status_t get_rebuild_status (int argc, char ** argv);

        // get bgverify status
        fbe_status_t get_bgverify_status (int argc, char ** argv);

        // get physical lba from logical lba
        fbe_status_t get_physical_lba (int argc, char ** argv);

        //get lba mapping
        fbe_status_t get_lba_mapping(int argc, char ** argv);

        //get pba mapping
        fbe_status_t get_pba_mapping(int argc, char ** argv);

        // get paged metadata
        fbe_status_t get_paged_metadata(int argc, char ** argv);

        // get raid group state
        fbe_status_t get_raid_group_state(int argc, char ** argv);

       // set mirror prefered posiiton
        fbe_status_t set_mirror_prefered_position(int argc, char ** argv);

       // get write log info
       fbe_status_t get_write_log_info(int argc, char ** argv);

        // get background operation speed
       fbe_status_t get_background_operation_speed(int argc, char ** argv);

        // set background operation speed
       fbe_status_t set_background_operation_speed(int argc, char ** argv);

        // ------------------------------------------------------------
};

#endif
