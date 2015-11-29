/****************************************************************************
 *                       file_util_class.h
 ****************************************************************************
 *
 *  Description:
 *    This header file contains class definitions for use with the 
 *    file_util_class.cpp module.
 *    
 *  Table of Contents:       
 *     fileUtilClass  
 *
 *  History:
 *      07-March-2011 : initial version
 *
 ****************************************************************************/

#ifndef T_FILEUTILCLASS_H
#define T_FILEUTILCLASS_H

// Load fbe includes if PRIVATE_H is not defined.
#ifndef PRIVATE_H
#include "private.h" 
#include "fbe_apix_util.h"
using namespace fbeApixUtil;

// Load ioctl includes if IOCLT_PRIVATE_H is not defined.
#else 
#include "ioctl_private.h" 
#endif

// Global variable declarations and assignments
#ifndef  FBEAPIX_H
#include "fbe_apix.h"
#endif

/****************************************************************************
 *                          fileUtilClass                                   *
 ****************************************************************************
 *
 *  Description: 
 *      This class contains methods that perform IO and handle pre and post
 *      processing for fbe apix function calls. 
 *
 *  Notes:
 *      Base class. This is a general class for file IO and other types of 
 *      common processing methods.
 *
 *  History:
 *      07-March-2011 : initial version
 *
 ****************************************************************************/

class fileUtilClass 
{
    protected:
        // Every object has an idnumber
        unsigned idnumber;
        unsigned utlCount;

        // common output hold areas
        char api_call[100];  // FBE API function name
        char func_call[100]; // name of funtion that calls FBE API
        char key[50];        // generally short name 
        char dkey[10];       // <DEBUG!>
        char ukey[10];       // <USAGE!>
        char temp[12000];     // FBE API return data or error message
     
        // debug and help argument flags
        int Debug;   
        unsigned help;
        unsigned xIoctl;

        // output messages
        char BAD_INPUT[MAX_STRING_LENGTH];
        char help_msg[MAX_STRING_LENGTH];
        char params[MAX_STRING_LENGTH];
        char cObjId[10];
        
        // File pointers 
        FILE * lfp;
        FILE * tfp;
        HANDLE dlh;
        char * filename;

    public:

        // Constructor 
        fileUtilClass(int &argc, char *argv[]); 
        fileUtilClass(); 
        void common_constructor_processing(void);

        // Destructor
        virtual ~fileUtilClass();
          
        // Routine that handles data returned from functions
        virtual void vWriteFile(char *key, char *data);
        
        // Open, close, and write FILE methods
        virtual unsigned Open_Trans_File (void);
        virtual unsigned Open_Log_File (void);
        virtual unsigned WriteFile (FILE *f, char *k, char *d);
        void Close_Files (void);

        // Accessor methods : flags; string of arguments
        void setDebug(int);
        int getDebug(void); 
        unsigned getHelp(void);
        unsigned getIoctl(void);
        char * getArgs(void);

        // Accessor methods : object data and syntax help
        virtual void dumpme(void);
        virtual void Usage(char *);
            
        // call preprocessing
        fbe_status_t call_preprocessing(
            char * sname, char * fname, char * api, char *uformat,
            int argc, int ix);

        // call post processing
        void call_post_processing(
            fbe_status_t passfail, char *temp, char * key,
            char * params);

        // String to hex
        fbe_status_t  string_to_hex64(fbe_u8_t * s_p,
                       fbe_u64_t * result);
};

#endif
