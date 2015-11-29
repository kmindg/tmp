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
 *      This file defines the SEP Provision Drive (PVD) class.
 *
 *  History:
 *      29-Mar-2011 : inital version
 *
 *********************************************************************/

#ifndef T_SEP_PROVISION_DRIVE_CLASS_H
#define T_SEP_PROVISION_DRIVE_CLASS_H

#ifndef T_SEP_CLASS_H
#include "sep_class.h"
#endif

#define FBE_JOB_SERVICE_WAIT_TIMEOUT       600000  /*ms*/
/*********************************************************************
 *          Provision Drive class : bSEP base class
 *********************************************************************/

class sepProvisionDrive : public bSEP
{
    protected:
        
        // Every object has an idnumber
        unsigned idnumber;
        unsigned pvdrCount;

        // interface object name
        char  * idname;
  
        // Provision Drive Interface function calls and usage 
        char * sepProvisionDriveInterfaceFuncs; 
        char * usage_format;

        // Variables
        //disk zero checkpoint
        fbe_lba_t disk_zero_checkpoint;

        // disk zero percentage
        fbe_u16_t disk_zero_percent;

        //PVD config type
        fbe_provision_drive_config_type_t config_type;

        // PVD sniff state
        fbe_bool_t sniff_verify_state;
        fbe_u32_t state;

        // drive location 
        fbe_u32_t port;
        fbe_u32_t enclosure;
        fbe_u32_t slot;
        fbe_job_service_bes_t phys_location;
        fbe_u32_t pool_id;
        fbe_object_id_t src_obj_id, dest_obj_id;

        
        fbe_lba_t in_checkpoint;
        char cpy_status[50];

        //paged metadata
        fbe_chunk_index_t chunk_offset;
        fbe_u32_t index;
        fbe_chunk_index_t max_chunk_count;
        fbe_provision_drive_paged_metadata_t *current_paged_p ;
        fbe_provision_drive_paged_metadata_t *paged_p ;

       //setslf
        fbe_bool_t is_enable;

        // Data Structures
        // provision drive structure to hold sniff verify report
        fbe_provision_drive_verify_report_t pvd_verify_report;

        // provision drive structure to hold sniff verify status information
        fbe_provision_drive_get_verify_status_t disk_verify_status;

        // provision drive fbe api structure to hold pvd object info
        fbe_api_provision_drive_info_t provision_drive_info;

        // provision drive api structure to hold background priorities
        fbe_provision_drive_set_priorities_t get_priorities;

        // provision drive fbe api structure to update pvd config type
        fbe_api_provision_drive_update_config_type_t update_pvd_config_type;

        // provision drive fbe api structure to hold spare drive pool list
        fbe_api_provisional_drive_get_spare_drive_pool_t spare_drive_pool;

        // sep job service interface fbe api data structure 
        fbe_api_job_service_update_pvd_sniff_verify_status_t 
            update_pvd_sniff_verify_state;

        // provision drive fbe api structure to update pvd pool id
        fbe_api_provision_drive_update_pool_id_t update_pvd_pool_id;

        // copy status data structure
        fbe_provision_drive_copy_to_status_t copy_status;
        fbe_provision_drive_map_info_t  map_info;
        fbe_api_virtual_drive_get_info_t virtual_drive_info;

        // Private methods
        void edit_disk_zero_chkpt(fbe_lba_t);
        void edit_disk_verify_report(fbe_provision_drive_verify_report_t *);
        void edit_disk_verify_status(fbe_provision_drive_get_verify_status_t *);
        void edit_pvd_obj_info(fbe_api_provision_drive_info_t *);
        void edit_background_priorities(fbe_provision_drive_set_priorities_t *);
        void edit_pvd_pool_id(fbe_u32_t pool_id);
        char* edit_spare_drive_pool();
        bool is_valid_config_type(char ** argv);
        void Sep_Provision_Intializing_variable();
        char* edit_copy_status( fbe_provision_drive_copy_to_status_t);
        char* edit_config_type(fbe_provision_drive_config_type_t );
        void edit_get_copyto_checkpoint(fbe_api_virtual_drive_get_info_t *vd_get_info_p);

    public:

        // Constructor/Destructor
        sepProvisionDrive();
        ~sepProvisionDrive(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]); 
       
        // get drive info methods 
        fbe_status_t get_object_id             (int argc, char ** argv);
        fbe_status_t get_pvd_object_info       (int argc, char ** argv);
        fbe_status_t get_pvd_obj_dr_location   (int argc, char ** argv);
        fbe_status_t get_background_priorities (int argc, char ** argv);

        // Disk Zero methods
        fbe_status_t initiate_disk_zeroing     (int argc, char ** argv);
        fbe_status_t get_disk_zero_checkpoint  (int argc, char ** argv);
        fbe_status_t get_disk_zero_percentage  (int argc, char ** argv);
        fbe_status_t set_disk_zero_checkpoint (int argc , char ** argv);

        // Disk Verify
        fbe_status_t set_sniff_verify            (int argc, char ** argv);
        fbe_status_t get_verify_report         (int argc, char ** argv);
        fbe_status_t clear_verify_report       (int argc, char ** argv);
        fbe_status_t get_disk_verify_status    (int argc, char ** argv);

        // Spare config, Spare pool methods
        fbe_status_t update_config_type        (int argc, char ** argv);
        fbe_status_t get_spare_drive_pool      (int argc, char ** argv);
        
        // get pool id
        fbe_status_t get_pvd_pool_id(int argc, char ** argv);
        
        // set pool id
        fbe_status_t set_pvd_pool_id(int argc, char ** argv);

        // set end of life state for pvd
        fbe_status_t set_pvd_eol_state(int argc, char ** argv);

        // set verify checkpoint
        fbe_status_t set_verify_checkpoint(int argc ,char ** argv);

        // Copy to replacement disk
        fbe_status_t copy_to_replacement_disk(int argc ,char ** argv);

        //Copy to proactive spare
        fbe_status_t copy_to_proactive_spare( int argc, char * * argv);

        // Clear the EOL state 
        fbe_status_t clear_eol_state(int argc ,char ** argv);

        // Clear the Eol on PVD ldo and pdo
        fbe_status_t packaged_clear_eol_state(int argc , char ** argv);

        // Set PVD config type
        fbe_status_t set_config_type (int argc , char ** argv);

        //get paged metadata
        fbe_status_t get_paged_metadata(int argc, char ** argv);

        // map lba to chunk
        fbe_status_t map_lba_to_chunk(int argc , char ** argv);

        // get copy to check point
        fbe_status_t get_copyto_check_point(int argc , char ** argv);

        //set single loop failure
        fbe_status_t is_slf(int argc , char ** argv);

        //clear drive fault bit
        fbe_status_t  clear_drive_fault_bit( int argc , char ** argv);

        // ------------------------------------------------------------
};

#endif
