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
 *      This file defines the SEP LUN INTERFACE class.
 *
 *  History:
 *      07-March-2011 : inital version
 *
 *********************************************************************/

#ifndef T_SEP_LUN_CLASS_H
#define T_SEP_LUN_CLASS_H

#ifndef T_SEP_CLASS_H
#include "sep_class.h"
#endif

#define SYS_ENABLE        1
#define SYS_DISABLE       0

/*********************************************************************
 *          SEP LUN class : bSEP base class
 *********************************************************************/

class sepLun : public bSEP
{
    protected:

        // Every object has an idnumber
        unsigned idnumber;
        unsigned sepLunCount;

        // interface object name
        char  * idname;

        // SEP LUN Interface function calls and usage
        char * sepLunInterfaceFuncs;
        char * usage_format;

        // SEP LUN interface fbe api data structures
        fbe_api_lun_get_zero_status_t  lu_zero_status;
        fbe_lun_get_verify_status_t    lu_verify_status;
        fbe_lun_verify_type_t          lu_verify_type; 
        fbe_lun_verify_report_t        lu_verify_report; 
        fbe_api_lun_destroy_t          lu_destroy;
        fbe_lun_set_power_saving_policy_t    in_policy;
        fbe_assigned_wwid_t  lu_update_struct;
        fbe_api_lun_calculate_capacity_info_t  capacity_info;
        fbe_database_lun_info_t        lu_info;
        fbe_api_lun_get_status_t       lu_rebuild_status,
                                    lu_bgverify_status;
        fbe_api_lun_get_lun_info_t     lun_info;

        // private methods
       void edit_lu_zero_info(
            fbe_api_lun_get_zero_status_t*);

        void edit_lu_verify_info(
            fbe_lun_get_verify_status_t*);

        void edit_lu_verify_report( fbe_lun_verify_report_t*);

        char * edit_lu_perf_stats(
            fbe_lun_performance_statistics_t *,
            fbe_u32_t);
        void edit_get_lun_info(
            fbe_api_lun_get_lun_info_t *get_lun_info);
        void Sep_Lun_Intializing_variable();

        void show_rg_map_info( fbe_raid_group_map_info_t * );
        fbe_status_t get_lun_verify_type(char **argv);

    public:

        // Constructor/Destructor
        sepLun();
        ~sepLun(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        int      MyCountIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]);

        // get lun zero status
        fbe_status_t get_lun_zero_status (int argc, char ** argv);

        // lun verify methods
        fbe_status_t initiate_lun_verify (int argc, char ** argv);
        fbe_status_t get_lun_verify_status (int argc, char ** argv);
        fbe_status_t get_lun_bgverify_status (int argc, char ** argv);
        fbe_status_t get_lun_verify_report (int argc, char ** argv);
        fbe_status_t clear_lun_verify_report (int argc, char ** argv);

        // lun performance stats methods
        fbe_status_t enable_perf_stats (int argc, char ** argv);
        fbe_status_t disable_perf_stats (int argc, char ** argv);

        // get performance stats
        fbe_status_t get_perf_stats (int argc, char ** argv);

        // unbind LUN
        fbe_status_t unbind_lun (int argc, char ** argv);

        // lun power saving policy methods
        fbe_status_t get_lun_power_saving_policy(int argc, char ** argv);
        fbe_status_t set_lun_power_saving_policy(int argc, char ** argv);

        // update LUN method
        fbe_status_t update_lun(int argc, char ** argv);

        // calculate imported capacity method
        fbe_status_t calculate_lun_imported_capacity(int argc, char ** argv);

        // calculate exported capacity method
        fbe_status_t calculate_lun_exported_capacity(int argc, char ** argv);
        
        // calculate max lun size method
        fbe_status_t calculate_max_lun_size(int argc, char ** argv);
        
        // get LUN's rebuild status
        fbe_status_t get_lun_rebuild_status (int argc, char ** argv);
        
        // get LUN info
        fbe_status_t get_lun_info (int args, char ** argv);

        // Maps an LBA of a LUN to a physical address
        fbe_status_t lun_map_lba(int argc , char ** argv);

        // Initiate verify on all the LUNs
        fbe_status_t initiate_verify_on_all_existing_luns (int argc , char ** argv);

        // ------------------------------------------------------------
};

#endif
