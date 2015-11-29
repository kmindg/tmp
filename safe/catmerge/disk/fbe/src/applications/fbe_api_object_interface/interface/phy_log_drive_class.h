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
 *      This file defines the Physical Logical Drive class.
 *
 *  History:
 *      12-May-2010 : inital version
 *
 *********************************************************************/

#ifndef T_PHYLOGDRIVECLASS_H
#define T_PHYLOGDRIVECLASS_H

#ifndef T_PHYCLASS_H
#include "phy_class.h"
#endif

/*********************************************************************
 *          Logical Drive class : bPhysical base class
 *********************************************************************/

class PhyLogDrive : public bPhysical
{
    protected:

        // Every object has an idnumber
        unsigned idnumber;
        unsigned pldrCount;

        // interface object name
        char * idname;

        // Logical Drive Interface function calls and usage
        char * phyLogDriveInterfaceFuncs;
        char * usage_format;

        // structure to capture logical drive info
        fbe_logical_drive_attributes_t attributes;
        fbe_logical_drive_attributes_t *attributes_p;
        fbe_lba_t imported_capacity;

        // drive location
        fbe_u32_t enclosure;
        fbe_u32_t port;
        fbe_u32_t slot;
        fbe_job_service_bes_t phys_location;

        // block size
        fbe_block_size_t block_size;
        fbe_block_size_t optimal_block_size;
        fbe_block_size_t imported_block_size;
        fbe_block_size_t imported_optimal_block_size;

        // fbe_block_transport_negotiate
        fbe_block_transport_negotiate_t capacity;

        // fbe_payload_block_operation_status
        fbe_payload_block_operation_status_t block_status;

        // fbe_payload_block_operation_qualifier
        fbe_payload_block_operation_qualifier_t block_qualifier;

        // private methods
        void edit_drive_info(
            fbe_logical_drive_attributes_t*);

        void edit_drive_block_size_info(
            fbe_block_transport_negotiate_t*,
            fbe_payload_block_operation_status_t,
            fbe_payload_block_operation_qualifier_t);
        void Phy_Intializing_variable();

    public:

        // Constructor/Destructor
        PhyLogDrive();
        ~PhyLogDrive(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        int      MyCountIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]);

        // Logical Drive methods
        fbe_status_t get_object_id          (int argc, char ** argv);
        fbe_status_t get_attributes         (int argc, char ** argv);
        fbe_status_t validate_block_size    (int argc, char ** argv);
        fbe_status_t get_drive_block_size   (int argc, char ** argv);
        fbe_status_t get_id_by_serial_number(int argc, char ** argv);
        fbe_status_t set_attributes         (int argc, char ** argv);
        fbe_status_t clear_eol(int argc , char ** argv);
};

#endif