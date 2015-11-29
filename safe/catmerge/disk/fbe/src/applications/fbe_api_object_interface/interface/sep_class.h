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
 *      This file defines the Storage Extent base class.
 *
 *  History:
 *      07-March-2011 : initial version
 *
 *********************************************************************/

#ifndef T_SEP_CLASS_H
#define T_SEP_CLASS_H

#ifndef T_OBJECT_H
#include "object.h"
#endif

#define CASENAME(E) case E: return #E
#define DFLTNAME(E) default: return #E

// Assuming maximum of 16 drives in R6 RG.
#define MAX_NUM_DRIVES_IN_RG 16

#define DIRECTION_DOWNSTREAM 0x1
#define DIRECTION_UPSTREAM   0x2
#define DIRECTION_BOTH       (DIRECTION_DOWNSTREAM | DIRECTION_UPSTREAM)

/*********************************************************************
 * bSEP base class : Object Base class : fileUtilClass Base class
 *********************************************************************/

class bSEP : public Object
{
    protected:

        // Every object has an idnumber
        unsigned idnumber;
        unsigned sepCount;

        // object fbe api data
        fbe_object_id_t    lu_object_id;
        fbe_object_id_t    pvd_object_id;
        fbe_object_id_t    rg_object_id;
        fbe_object_id_t    vd_object_id;

        // LUN ID 
        fbe_u32_t lun_number;

        // RG ID
        fbe_u32_t raid_group_num;

        // status indicator and flags
        fbe_status_t status, passFail;
        fbe_bool_t enabled;

        // struct to hold port, enclosure, slot of a drive.
        typedef struct fbe_apix_pes_s {
            // This will hold port number of the drive 
            fbe_port_number_t            port_num;

            // This will hold enclosure number of the drive 
            fbe_enclosure_number_t       encl_num;

            // This will hold slot number of the drive 
            fbe_enclosure_slot_number_t  slot_num;
        }fbe_apix_pes_t;


        typedef struct fbe_apix_rg_drive_list_s {
            // List of drives in the RG 
            fbe_apix_pes_t    drive_list[MAX_NUM_DRIVES_IN_RG];
        }fbe_apix_rg_drive_list_t;

        // Index into argument list. Gives visibility to all
        // class methods. The select method assigns the value of
        // the passed in argument [index] to [c].
        int c;

        // private methods
        
        // This method will get the drives for the given RG object ID
        fbe_status_t get_drives_by_rg(fbe_object_id_t , 
                                      fbe_apix_rg_drive_list_t *);
        void Sep_Intializing_variable();

    public:

        // Constructor
        bSEP();
        bSEP(FLAG);

        //Destructor
        ~bSEP(){}

        // Accessor methods (object  data)
        virtual unsigned MyIdNumIs(void);
        virtual char * MyIdNameIs(void) {return 0;}
        virtual void dumpme(void);

        // Select and call interface method
        virtual fbe_status_t select(int i, int c, char *a[]){
             return FBE_STATUS_INVALID;
        }

        // help
        virtual void HelpCmds(char *);

};

#endif
