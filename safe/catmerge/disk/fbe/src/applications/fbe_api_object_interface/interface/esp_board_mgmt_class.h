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
 *      This file defines the ESP BOARD INTERFACE class.
 *
 *  History:
 *      05-Aug-2011 : inital version
 *
 *********************************************************************/

#ifndef T_ESP_BOARD_MGMT_CLASS_H
#define T_ESP_BOARD_MGMT_CLASS_H

#ifndef T_ESP_CLASS_H
#include "esp_class.h"
#endif

/*********************************************************************
 *          ESP BOARD MGMT class : bESP base class
 *********************************************************************/

class espBoardMgmt : public bESP
{
    protected:

        // Every object has an idnumber
        unsigned idnumber;
        unsigned espBoardMgmtCount;
        char params_temp[250];

        // interface object name
        char  * idname;

        // ESP BOARD Mgmt Interface function calls and usage
        char * espBoardMgmtInterfaceFuncs;
        char * usage_format;

        // ESP BOARD MGMT interface fbe api data structures
        fbe_board_mgmt_set_PostAndOrReset_t post_reset;
        SP_ID sp;

    public:

        // Constructor/Destructor
        espBoardMgmt();
        ~espBoardMgmt(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]);

        // FBE API reboot sp
        fbe_status_t reboot_sp(int argc, char ** argv);
};

#endif
