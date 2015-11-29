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
 *      This file defines the ESP EIR class.
 *
 *  History:
 *      18-July-2011 : inital version
 *
 *********************************************************************/

#ifndef T_ESP_EIR_CLASS_H
#define T_ESP_EIR_CLASS_H

#ifndef T_ESP_CLASS_H
#include "esp_class.h"
#endif

/*********************************************************************
 *          ESP EIR class : bESP base class
 *********************************************************************/
class espEir : public bESP
{
    protected:

        // Every object has an idnumber
        unsigned idnumber;
        unsigned espEirCount;

        // interface object name
        char  * idname;

        // ESP EIR function calls and usage
        char * espEirInterfaceFuncs;
        char * usage_format;

        // ESP EIR fbe api data structures
        fbe_eir_data_t fbe_eir_data;

        #if 1
        char encl_data[1000];
        char data[100];
        char sps_data[500];
        #endif

        // private methods
        void edit_encl_data(fbe_eir_data_t*);
        void edit_eir_sps_data(fbe_eir_data_t*);
        void esp_eir_initializing_variable ();
        
    public:

        // Constructor/Destructor
        espEir();
        ~espEir(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]);

        // FBE API get EIR info
        fbe_status_t get_esp_eir_data(int argc, char ** argv);

};

#endif

