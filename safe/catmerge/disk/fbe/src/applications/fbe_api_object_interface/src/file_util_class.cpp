/*********************************************************************
 * Copyright (C) EMC Corporation, 1989 - 2013
 * All Rights Reserved.
 * Licensed Material-Property of EMC Corporation.
 * This software is made available solely pursuant to the terms
 * of a EMC license agreement which governs its use.
 *********************************************************************/

/*********************************************************************
 *
 *  Description:
 *      This file defines methods of the file utilities class. 
 *
 *  Notes:
 *      These methods handle the processing to open, read, and 
 *      write the transaction and log files. They also include 
 *      common pre and post processing function call methods
 *      used by the fbe_ioctl_object_interface library methods.
 *
 *  History:
 *      07-March-2011 : initial version
 *      09-October-2013 : ARs 534121,539086
 *
 *********************************************************************/

#ifndef T_FILEUTILCLASS_H
#include "file_util_class.h"
#endif
#include "csx_ext.h"
#include "EmcUTIL.h"

/*********************************************************************
 * fileUtilClass Class :: Base class Constructors
 *********************************************************************
 *
 *  Description:
 *      Initialize class variables and increment the class object 
 *      counter. Checks for debug (-d) or help (-h) options in the 1st 
 *      argument and sets related class and global variables (flags) 
 *      as appropriate. 
 *
 *      There are 2 constructors. The constructor with arguments is 
 *      designed to be called once to perform initial class processing. 
 *      [fileUtilClass(int &argc, char *argv[])]
 *
 *      The other contructor is called by a class constructor to get 
 *      access to IO and general utility functions as well as for 
 *      class initialization. [fileUtilClass()] 
 *
 *      Both methods call the common_constructor_processing function
 *      to handle common processing.
 *
 *  Input: Parameters
 *       argc, *argv[] - command line arguments are passed when call 
 *                       directly
 *       none -  when call from a class contructor 
 *
 *  Output: Console
 *      Debug messages (trace purposes)
 *
 * Returns:
 *      Nothing
 *
 *  History
 *      07-March-2011 : initial version
 *
 *********************************************************************/

/*********************************************************************
 * fileUtilClass Class :: Base class Constructor (int, char *)
 *********************************************************************/
 
 fileUtilClass::fileUtilClass(int &argc, char *argv[]) 
{          
    // default assignments
    Debug  = gDebug = 0;
    help   = gHelp  = 0;
    xIoctl = gIoctl = 0;

    tfp = lfp = gLfp = gTfp = NULL;
    dlh = NULL;
    filename = NULL;
    
    // initialization common to constructors
    common_constructor_processing();

    // If parameters, perform the following checks:
    if (argc > 1 ){
        
       // copy arguments 
       strcpy(iCmd,"fbe_ioctl_object_interface_cli.exe\0");
       for (int i=1; i<argc; i++) {
            strcat(iCmd, " ");
            strcat(iCmd, argv[i]);
        }   
        
        // Check for -ioc (ioctl command)
        for (int i=0; i<argc; i++) {
            for (int j=0; argv[i][j] != '\0'; j++) {
                argv[i][j] = toupper(argv[i][j]);
            }
            if (strncmp (argv[i], "-IOC", 4) == 0) {
                gIoctl = xIoctl = 1;
            }
        }
        
        // Check for debug or help input parameters
        if (strncmp (argv[1],"-H", 2) == 0) {
            gHelp = help = 1;

        } else if (strncmp (argv[1],"-D", 2) == 0) {
            gDebug = Debug = 1;
        }
    }

    // trace message
    if (Debug) {
        sprintf(temp, "%s\n %s (%d)\n %s (%d)\n %s (%d)\n", 
            "fileUtilClass::fileUtilClass",
            "Initial class constructor1", idnumber,
            "helP", help, 
            "Debug", Debug);
        vWriteFile(dkey, temp);
    }
}

/*********************************************************************
 * fileUtilClass Class :: Base class Constructor ()
 *********************************************************************/

fileUtilClass::fileUtilClass() 
{
    Debug  = gDebug = 0;
    help   = gHelp = 0;
    xIoctl = gIoctl;

    tfp = lfp = NULL;
    dlh = NULL;
    filename = NULL;

    // common processing
    common_constructor_processing();

    // trace message
    if (Debug) {
        sprintf(temp, "%s\n %s (%d)\n",
            "fileUtilClass::fileUtilClass",
            "class constructor2", idnumber);
        vWriteFile(dkey, temp);
    }
}

/*********************************************************************
 * fileUtilClass Class :: common_constructor_processing 
 *********************************************************************/

void fileUtilClass::common_constructor_processing(void)
{   
    idnumber = FILEUTILCLASS;
    utlCount = ++gUtlCount;    
    strcpy(dkey, "<DEBUG!>"); 
    strcpy(ukey, "<USAGE!>"); 
    strcpy(BAD_INPUT, "Missing or Bad Input");
}

/*********************************************************************
 * fileUtilClass Class :: Destructor
 *********************************************************************
 *
 *  Description:
 *      Close files.
 *
 *  Input: Parameters
 *      none
 *
 *  Output: stdout (Console)/file
 *      Debug messages (trace purposes)
 *
 * Returns:
 *      Nothing
 *
 *  History
 *      07-March-2011 : initial version
 *
 *********************************************************************/

fileUtilClass::~fileUtilClass()
{
    // Echo if Debug 
    if (Debug) {
        sprintf(temp, "%s\n %s\n %s\n",
            "fileUtilClass::~fileUtilClass destructor",
            "Closing c:\\FbeApiXlog.txt",
            "Closing c:\\FbeApiX.txt");
        vWriteFile(dkey, temp);
    }
    Close_Files();
}

void fileUtilClass::Close_Files(void)
{
    fclose(gLfp);
    fclose(gTfp);
}

/*********************************************************************
 * fileUtilClass Class :: Open_Trans_File ()
 *********************************************************************
 *
 *  Description:
 *      Open transaction file.
 *
 *  Input: Parameters
 *      none
 *
 *  Output: Console.
 *      Debug and error messages.
 *
 *  Returns:
 *      status 0 (success) or -1 (error)
 *
 *  History:
 *       07-March-2011 : initial version.
 *
 *********************************************************************/

unsigned fileUtilClass::Open_Trans_File ()
{
    fbe_char_t filename[32];

    EmcutilFilePathMake(filename, sizeof(filename), EMCUTIL_BASE_CDRIVE,
							"FbeApiExt.txt", NULL);
    // Open file.  This will overwrite any existing file of the same name.
    gTfp = fopen(filename, "w");
        
    if (!gTfp) {
        printf("can't open output transaction file\n");
        exit(-1);
    }
   
    // write first line to file 
    fprintf(gTfp, "***********************************************\n");
    fflush(gTfp);
    
    // Echo file name if Debug 
    if (Debug) {
        sprintf(temp, "%s\n %s %s\n",
           "fileUtilClass::Open_Trans_File",
           "Opening:", filename);
        vWriteFile(dkey, temp);
    }

    return (0);
}

/*********************************************************************
 * fileUtilClass Class :: Open_Log_File ()
 *********************************************************************
 *
 *  Description:
 *      Open log file. 
 *
 *  Input: Parameters
 *      none
 *
 *  Output: Console.
 *      Debug and file error messages.
 *
 *  Returns:
 *      status - 0 (success) or -1 (error)
 *
 *  History:
 *       07-March-2011 : initial version.
 *
 *********************************************************************/

unsigned fileUtilClass::Open_Log_File ()
{
    fbe_char_t filename[32];

    EmcutilFilePathMake(filename, sizeof(filename), EMCUTIL_BASE_CDRIVE,
							"FbeApiExtLog.txt", NULL);
    // Open file.  This will overwrite any existing file of the same name.
    gLfp = fopen(filename, "a");
	
    
    if (!gLfp) {
        printf("can't open output log file\n");
        exit(-1);
    }
   
    // write first line to file 
    fprintf(gLfp, "***********************************************\n");
    fflush(gLfp);

    // Echo file name if Debug 
    if (Debug) {
        sprintf(temp, "%s\n %s %s\n",
            "fileUtilClass::Open_Log_File",
            "Opening:", filename);
        vWriteFile(dkey, temp);
    }

    return (0);
}

/*********************************************************************
 * fileUtilClass Class :: vWriteFile ()
 *********************************************************************
 *
 *  Description:
 *      Writes the name of data and data strings passed in to a log 
 *      file, transaction file, and console.
 *
 *  Input: Parameters
 *      *key - name of data 
 *      *buffer - data 
 *
 *  Output: Console.
 *      Key and data lines.
 *
 *  Returns:
 *      nothing
 *
 *  History:
 *       07-March-2011 : initial version.
 *       09-October-2013 : ARs 534121,539086
 *
 *********************************************************************/

void  fileUtilClass::vWriteFile(char *key, char *buffer)
{

    // Write to transaction file if not debug.
    if (strncmp (key,"<DEBUG", 6) != 0){
        if (gTfp != NULL) {
            WriteFile(gTfp, key, buffer);
        }
    }

    // write to log file
    if (gLfp != NULL) {
        WriteFile(gLfp, key, buffer);
    }

    // Output to stdout
    printf("<Key>  %s\n", key);
    // newlines are not consistently used
    if( buffer[strlen(buffer) - 1] == '\n' ) {
        printf("<Data> %s", buffer);
    } else {
        printf("<Data> %s\n", buffer);
    }
    
    return;
}

/*********************************************************************
 * fileUtilClass Class :: WriteFile ()
 *********************************************************************
 *
 *  Description:
 *      Write strings to the specified output file in the following
 *      format.
 *          key: <string> 
 *          data: <string>
 *
 *  Input: Parameters
 *      *key  - points to the name of the string to output
 *      *data - points to the string to output
 *      *fp   - points to a file which will log the output.
 *      close - 1 to close file.
 *  
 *  Output: Console.
 *      none
 *
 *  Returns:
 *      0 (success) or 1 (error)
 *
 *  History:
 *       07-March-2011 : initial version.
 *
 *********************************************************************/

unsigned fileUtilClass::WriteFile (FILE *fp, char *key, char *data)
{
     
    // Get key and data length
    unsigned klen = (unsigned)strlen(key);

    //Output key
    if (klen) {
        fprintf(fp, "<Key>  %s", key);
        fputs("\n", fp);
        fflush(fp);
    }

    // Check if key was passed and output line with tag.
    if (klen) {
        fprintf(fp, "<Data> %s", data);

    // Output line without tag.
    } else {
        fprintf(fp, "%s", data);
    }

    fputs("\n\n", fp);
    fflush(fp);

    return (0);
}

/*********************************************************************
 * fileUtilClass Class :: Accessor methods
 *********************************************************************
 *
 *  Description:
 *      This section contains routines that operate on class data.
 *
 *  Input: Parameters
 *      setDebug() - 0 (off) or 1 (on)
 *
 *  Returns:
 *      getDebug() - 0 (off) or 1 (on)
 *      getHelp()  - 0 (no)  or 1 (yes)
 *      getArgs()  - ioctl progranm name and arguments string
 *
 *  Output: Console
 *      dumpme()   - general info about the object 
 *      Usage()    - Command syntax and help.
 *      getHelp()  - help message lines
 *
 *  History
 *      07-March-2011 : initial version
 *
 *********************************************************************/

char * fileUtilClass::getArgs(void)
{
    return iCmd;
}

void fileUtilClass::setDebug(int onOff)
{
    gDebug = Debug = onOff;
}

int fileUtilClass::getDebug(void)
{
    return (Debug);
}

unsigned fileUtilClass::getHelp(void)
{
    return help;
}

void fileUtilClass::dumpme(void) 
{   
    strcpy (key, "fileUtilClass::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", idnumber, utlCount);
    vWriteFile(key, temp);
}

void fileUtilClass::Usage(char * lines)
{
    char * help =
        "Usage: %s [-d|-h|?] [-k|-s] [-a|-b] " \
        "[-Package] [-Interface] " \
        "[function call (short name)]\n" \
        "--------------------------\n" \
        "[Optional parameters] \n" \
        "--------------------------\n" 
        " -d    Debug\n" \
        " -h|?  Help (%s ?|more)\n" \
        "--------------------------\n" \
        "[Required parameters for Fbe] \n" \
        "--------------------------\n" \
        " -k|-s driver type k[kernel] or s[simulator]\n" \
        " -a|-b SP[A] or SP[B]\n" \
        "--------------------------\n" \
        "[-Package] \n" \
        "--------------------------\n" \
        " -PHY  Fbe Physical Package\n" \
        " -ESP  Fbe Environmental System package\n" \
        " -SYS  Fbe System package\n" \
        " -SEP  Fbe Storage Extent Package\n" \
        " -IOC  Ioctl Package\n" \
        "--------------------------\n" \
        "[-Interface]\n" \
        "--------------------------\n";
    
    char * TARGETNAME = "fbe_api_object_interface_cli";
    unsigned len = unsigned( strlen(lines) + strlen(help) );
    char * temp = new char[len + (2*strlen(TARGETNAME)) + 1];

    sprintf(temp, (char *) help, TARGETNAME, TARGETNAME);
    strcat (temp, lines);
    
    vWriteFile(ukey, temp);
    delete (temp);

    return;
}

/*********************************************************************
*  fileUtilClass Class ::  call_preprocessing()   
*********************************************************************
*
*  Description:
*      This method handles all the common processing required prior 
*      to making the actual FBE API function call.
*       
*  Input: Parameters
*      *sname   - short name (string) 
*      *fname   - interface function name (string)
*      *api     - FBE API function name (sting)
*      *uformat - command usage format (string)
*      argc     - number of arguments
*      ix       - index into agrument list (no debug option)
*
*  Output: Console
*      debug messages
*
*  Returns:
*      Nothing
*
*  History
*      27-Jun-2011 : initial version
*
*********************************************************************/

fbe_status_t fileUtilClass::call_preprocessing(char * sname, char * fname,
    char * api, char * uformat, int argc, int ix)
{
    // Set class status to OK
    fbe_status_t  status = FBE_STATUS_OK;

    // Identify function
    strcpy(key, sname); 
     if(strlen(api) > 100)
    {
        sprintf(temp, "<ERROR!> API name length is larger that buffer value");
        vWriteFile(ukey, temp);
        return status;
    }
    strcpy(api_call, api);
    strcpy(func_call, fname);

    // Trace function
    if (Debug) 
    {
        sprintf(temp, "%s\n %s %s\n %s %s\n %s %s\n",
            "call_preprocessing",
            "short name:", key,
            "function:", func_call, 
            "call    :", api_call);
        vWriteFile(dkey, temp);
    }

    // Construct usage message for command
    char * hold_usage = new char[HOLD_USAGE_LENGTH(uformat, key)];
    sprintf(hold_usage, uformat, key, key);  

    // List help
    if (help) 
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        vWriteFile(ukey, hold_usage);  
    } 

    // Check correct number of parameters entered
    else if ( (!Debug && argc < ix) || (Debug && argc < ix+1) )
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        int missing = (!Debug) ? (ix - argc) : (ix+1 - argc);

        sprintf(temp, "<ERROR!> too few arguments: "
                "input (%d) missing (%d)\n%s", 
                argc, missing, hold_usage);

        vWriteFile(ukey, temp);  
    } 

    // Release memory
    delete hold_usage;

    return status;
}

/*********************************************************************
*  fileUtilClass Class ::  call_post_processing()   
*********************************************************************
*
*  Description:
*      This method handles all the common processing after making the
*      actual FBE API function call.
*       
*  Input: Parameters
*      passfail - call passed or failed indicator 
*      *temp    - error message or data returned by call (string)
*      *key     - short name (string)
*      *params  - call arguments (string)
*
*  Output: Console
*      debug messages
*
*  Returns:
*      Nothing
*
*  History
*      27-Jun-2011 : initial version
*
*********************************************************************/

void fileUtilClass::call_post_processing( 
     fbe_status_t passFail, char *temp, char * key, char * params)
{   
    // Memory Allocation: key and input parameters.
    char * hold_key = new char[strlen(key) + strlen(params) + 10];
    sprintf(hold_key, "%s %s", key, params);

    // Memory Allocation: error and Debug messages
    char * hold_temp = new char[HOLD_TEMP_LENGTH(temp,api_call)];

    // Trace function
    if (Debug) 
    {
        sprintf(hold_temp, "%s\n %s 0x%x (%d)\n %s 0x%x (%d)\n %s\n", 
            "call_post_processing",
            "passFail", passFail, passFail,
            "STATUS_OK", FBE_STATUS_OK, FBE_STATUS_OK,
            temp);
        vWriteFile(dkey, hold_temp);
    }

    // Call succeeded - Output results of FBE API call
    if (passFail == FBE_STATUS_OK) {      
        vWriteFile(hold_key, temp);

    // Call failed - Construct error message
    } else { 
        sprintf(hold_temp, "<ERROR!> "
            "<%s> %s\n <%s> 0x%x (%d)\n <%s> %s\n ",
            "call failed!", api_call, 
            "Status", passFail, passFail, 
            "Error Message", temp);
        vWriteFile(hold_key, hold_temp);
    }

    // Release memory.
    delete hold_key;
    delete hold_temp;

    return;
} 


/*!**************************************************************
 * string_to_hex64()
 ****************************************************************
 * @brief
 *  This function gives u64 value for hexadecimal string.
 *
 * @param s_p - hexadecimal string.
 * @param result - u64 value
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t  fileUtilClass::string_to_hex64(fbe_u8_t * s_p,
                       fbe_u64_t * result)
{
    /*skip leading spaces*/
    while(isspace(*s_p))
    {
        s_p++;
    }
    /*skip 0x */
    if(*s_p == '0' && (*(s_p+1) == 'x' || *(s_p+1) == 'X'))
    {
        s_p+=2;
    }
    if(isxdigit(*s_p))
    {
        fbe_u64_t r = 0;
        do
        {
            if(isdigit(*s_p))
            {
                r = 16*r + (fbe_u64_t)(*s_p - '0');
            }
            else
            {
                r = 16*r + (fbe_u64_t)((toupper(*s_p) - 55));
            }
        }
        while(isxdigit(*++s_p));
        /*skip trailing spaces*/
        while(isspace(*s_p))
        {
            s_p++;
        }
        /*check that we get into the end of line*/
        if(!(*s_p))
        {
            *result = r;
            return FBE_STATUS_OK;
        }
    }
    return FBE_STATUS_GENERIC_FAILURE;
}

/*********************************************************************
 * fileUtilClass end of file
 *********************************************************************/
