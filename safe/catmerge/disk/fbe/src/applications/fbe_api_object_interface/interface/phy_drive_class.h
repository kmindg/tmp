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
 *      This file defines the Physical Drive class.
 *
 *  History:
 *      12-May-2010 : inital version
 *
 *********************************************************************/

#ifndef T_PHYDRIVECLASS_H
#define T_PHYDRIVECLASS_H

#ifndef T_PHYCLASS_H
#include "phy_class.h"
#endif

/*********************************************************************
 *          Physical Drive class : bPhysical base class
 *********************************************************************/

class PhyDrive : public bPhysical
{
    protected:
        
        // Every object has an idnumber
        unsigned idnumber;
        unsigned pdrCount;

        // interface object name
        char  * idname;
  
        // Physical Drive Interface function calls and usage 
        char * phyDriveInterfaceFuncs; 
        char * usage_format;

        // structure to capture physical drive info from api
        fbe_physical_drive_information_t drive_information;
    
        // drive location 
        fbe_u32_t enclosure;
        fbe_u32_t port;
        fbe_u32_t slot;
        fbe_job_service_bes_t phys_location;

        // private methods 
        void edit_asynch_drive_info(
            fbe_physical_drive_information_asynch_t*);
        void edit_error_counts(
            fbe_physical_drive_mgmt_get_error_counts_t*);
        void edit_diag_info(
            fbe_physical_drive_receive_diag_pg82_t*, fbe_u8_t);
        void edit_drive_info(
            fbe_physical_drive_information_t*);
        void edit_buffer(char *, fbe_u8_t *, fbe_u32_t, fbe_lba_t);
        void Phy_Drive_Intializing_variable();

        fbe_status_t get_object_id_from_bes(char* b_e_s);

    public:

        // Constructor/Destructor
        PhyDrive();
        ~PhyDrive(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        int      MyCountIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]); 
       
        // FBE API get drive info methods 
        fbe_status_t get_object_id         (int argc, char ** argv);
        fbe_status_t get_cached_drive_info (int argc, char ** argv);
        fbe_status_t get_drive_info        (int argc, char ** argv);
        fbe_status_t get_drive_info_asynch (int argc, char ** argv);
        fbe_status_t receive_diag_info     (int argc, char ** argv);
        fbe_status_t get_disk_error_log    (int argc, char ** argv); 
        fbe_status_t is_wr_cache_enabled   (int argc, char ** argv);
        
        // FBE API change state methods 
        fbe_status_t reset_drive           (int argc, char ** argv);
        fbe_status_t drive_reset_slot      (int argc, char ** argv);
        fbe_status_t power_cycle_drive     (int argc, char ** argv);
        fbe_status_t fail_drive            (int argc, char ** argv);
        
        // Set drive commands
        fbe_status_t set_drive_death_reason(int argc , char ** argv);
        fbe_status_t set_write_uncorrectable(int argc, char ** argv);
        
        // FBE API error stat methods
        fbe_status_t get_error_counts      (int argc, char ** argv);
        fbe_status_t clear_error_counts    (int argc, char ** argv);
        
        // FBE API io timeout methods
        fbe_status_t get_default_io_timeout(int argc, char ** argv);
        fbe_status_t set_default_io_timeout(int argc, char ** argv);
        
        // FBE API queue depth methods
        fbe_status_t get_object_queue_depth(int argc, char ** argv);
        fbe_status_t set_object_queue_depth(int argc, char ** argv);
        fbe_status_t get_drive_queue_depth (int argc, char ** argv);

        // FBE API fw download methods
        fbe_status_t fw_download_start_asynch(int argc, char ** argv);
        fbe_status_t fw_download_asynch    (int argc, char ** argv);
        fbe_status_t fw_download_stop      (int argc, char ** argv);

        // Clear Eol
        fbe_status_t clear_eol(int argc, char**argv);

        // Special Engineering method for sending pass thru cmds.  Use
        // with CARE!   Please contact Wayne Garrett for support.
        fbe_status_t send_pass_thru(int argc, int index, char ** argv);
        
        // ------------------------------------------------------------
};

#endif