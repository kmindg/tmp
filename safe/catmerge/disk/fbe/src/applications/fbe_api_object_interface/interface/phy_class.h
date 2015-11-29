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
 *      This file defines the Physical base class.
 *
 *  History:
 *      07-March-2011 : initial version
 *
 *********************************************************************/

#ifndef T_PHYCLASS_H
#define T_PHYCLASS_H

#ifndef T_OBJECT_H
#include "object.h"
#endif

/*********************************************************************
 * bPhysical base class : Object Base class : fileUtilClass Base class
 *********************************************************************/

class bPhysical : public Object
{
    protected:

        // Every object has an idnumber
        unsigned idnumber;
        unsigned phyCount;

        // object fbe api data
        fbe_u32_t object_id;
        fbe_object_id_t drive_object_id;

        // status indicator and flags
        fbe_status_t status, passFail;
        fbe_bool_t enabled;
        fbe_bool_t force_download; 
            
        // Physical drive fbe api variables.
        fbe_lba_t  lba;
        fbe_time_t drive_timeout;
        fbe_u32_t  depth, qdepth, duration, diag_context;
        
        // Redefinition to shorten data type names ....
        typedef struct asynch_drive_info_s {
            fbe_physical_drive_information_t  info;                                                                                         
            void (* completion_function)(void *context);
        } asynch_drive_info_t;
        
        // Physical drive fbe api data structures 
		fbe_physical_drive_mgmt_get_error_counts_t  error_stats;  
        //fbe_drive_mgmt_t                           *drive_mgmt;
	    fbe_physical_drive_fwdl_start_asynch_t     *start_context;
        fbe_physical_drive_fwdl_stop_asynch_t      *stop_context;
		fbe_physical_drive_fw_info_asynch_t        *dl_context;
        fbe_physical_drive_mgmt_receive_diag_page_t diag_info;
        fbe_physical_drive_information_t            drive_info;
        fbe_physical_drive_information_asynch_t     drive_info_asynch;
        fbe_base_physical_drive_death_reason_t      reason;
        FBE_DRIVE_DEAD_REASON                       death_reason;
        
        //fbe_physical_drive_information_t 
        fbe_drive_vendor_id_t drive_vendor_id;
        char * product_rev;
        char * product_serial_num;
        

    public:
        
        // Constructor
        bPhysical();
        bPhysical(FLAG);
       
        //Destructor
        ~bPhysical(){}

        // Accessor methods (object  data)
        virtual unsigned MyIdNumIs(void);
        virtual char * MyIdNameIs(void) {return 0;}
        virtual void dumpme(void);

        // Accessor methods (FBE API data)
        fbe_u32_t getObjId(void);
        fbe_drive_vendor_id_t getVendorId(void);
        char * getProdRev(void);
        char * getSerialNum(void);
        
        // Select and call interface method 
        virtual fbe_status_t select(int i, int c, char *a[]){
             return FBE_STATUS_INVALID;
        }  

        // help 
        virtual void HelpCmds(char *);

};

#endif
