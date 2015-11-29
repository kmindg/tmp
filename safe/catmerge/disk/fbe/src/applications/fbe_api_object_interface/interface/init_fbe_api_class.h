/****************************************************************************
 *                       init_fbe_api_class.h
 ****************************************************************************
 *
 *  Description:
 *    This header file contains class definitions for use with the 
 *    init_fbe_api_class.cpp module.
 *
 *  Table of Contents:      
 *      initFbeApiCLASS   
 *
 *  History:
 *      2010-05-12 : inital version
 *
 ****************************************************************************/

#ifndef T_INITFBEAPICLASS_H
#define T_INITFBEAPICLASS_H

#ifndef T_FILEUTILCLASS_H
#include "file_util_class.h"
#endif

/****************************************************************************
 *                          initFbeApiCLASS                                 *
 ****************************************************************************
 *
 *  Description: 
 *      This header file contains class definitions for use with the 
 *      init_fbe_api_class.cpp module.
 *
 *  Notes:
 *      Base class.
 *
 *  History:
 *      2010-05-12 : inital version
 *
 ****************************************************************************/

class initFbeApiCLASS : public fileUtilClass
{
    protected:

       // Every object has an idnumber
       unsigned idnumber;
       unsigned apiCount;
       
       // status and flags
       fbe_bool_t   simulation_mode;
       fbe_status_t status;
        
    public:

        // Constructor
        initFbeApiCLASS () {          
            idnumber = FBEAPIINIT;
            apiCount = ++gApiCount;
    
            if (Debug) {
                sprintf(temp, 
                    "initFbeApiCLASS constructor (%d)\n", idnumber);
                vWriteFile(dkey, temp);
            }
        }
        
        // Destructor
        ~initFbeApiCLASS(){} 
        void fbe_cli_destroy_fbe_api(
            fbe_sim_transport_connection_target_t);

        // Accessor methods
        void dumpme(void);

        // Kernel initialization routine
        fbe_status_t Init_Kernel ( 
            fbe_sim_transport_connection_target_t);
        
        // Simulator initialization routine
        fbe_status_t Init_Simulator (
            fbe_sim_transport_connection_target_t);
 
        // List of FBE objects
        fbe_status_t getObjects(void);
};

#endif
