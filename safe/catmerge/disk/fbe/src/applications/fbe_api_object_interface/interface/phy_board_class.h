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
 *      This file defines the PHYSICAL BOARD class.
 *
 *  History:
 *      05-Aug-2011 : initial version
 *
 *********************************************************************/

#ifndef T_PHY_BOARD_CLASS_H
#define T_PHY_BOARD_CLASS_H

#ifndef T_PHYCLASS_H
#include "phy_class.h"
#endif

/*********************************************************************
 *          PHYSICAL BOARD class : bPhysical base class
 *********************************************************************/

class PhyBoard : public bPhysical
{
    protected:

        // Every object has an idnumber
        unsigned idnumber;
        unsigned phyBoardCount;
        
        // interface object name
        char  * idname;

        // PHYSICAL BOARD function calls and usage
        char * phyBoardInterfaceFuncs;
        char * usage_format;

        // PHYSICAL BOARD interface fbe api data structures
        fbe_object_id_t object_id;

        // private methods

        // display ps count
        void edit_board_object_id(fbe_object_id_t object_id);

        //Initializing vaiables
        void  phy_board_initializing_variable();
    public:

        // Constructor/Destructor
        PhyBoard();
        ~PhyBoard(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]);

        // FBE API get board object id
        fbe_status_t get_board_object_id(int argc, char ** argv);
        
        // ------------------------------------------------------------
};

#endif
