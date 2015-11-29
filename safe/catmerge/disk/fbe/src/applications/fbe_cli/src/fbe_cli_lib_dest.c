/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_dest.c
 ***************************************************************************
 *
 * @brief This file is part of DEST (Drive Error Simulation Tool) service and
 * provides the interface
 * to DEST on FBE Cli.
 *
 * @ingroup fbe_cli
 *
 * History:
 *  05/01/2012  Created. kothal
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "fbe_cli_private.h"
#include "fbe_cli_dest.h"
#include "fbe_dest_private.h"
#include "fbe/fbe_dest.h"
#include "fbe/fbe_api_lurg_interface.h"
#include "fbe_job_service.h"
#include "fbe_api_dest_injection_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"

FBE_DEST_CMDS_TBL fbe_dest_cmds_tbl_a[] =
{   
    {"-init", fbe_cli_dest_usr_intf_init, "dest -init is no longer supported. Use -load to load xml config file"},
    {"-start", fbe_cli_dest_usr_intf_start, "dest -start - Start error insertion"},
    {"-stop", fbe_cli_dest_usr_intf_stop, "dest -stop - Stop error insertion"},  
    {"-add", fbe_cli_dest_usr_intf_add, "dest -add [add_args]: Add record.  No args will use interactive mode"},
    {"-add_spinup", fbe_cli_dest_usr_intf_add_spinup, "dest -add_spinup [add_args]: Add errors during spinup/spindown.  No args will use interactive mode"},
    {"-search", fbe_cli_dest_usr_intf_search, "dest -search - Search in interactive mode"}, 
    {"-delete", fbe_cli_dest_usr_intf_delete, "dest -delete - Delete a record in interactive mode"}, 
    {"-display", fbe_cli_dest_usr_intf_display, "dest - display - Display all error records in interactive mode\n"}, 
    {"-fail", fbe_cli_dest_usr_intf_fail, "dest -fail - Fails a drive.  No args will use interactive mode"},
    {"-glitch", fbe_cli_dest_usr_intf_glitch, "dest -glitch - Glitches a drive for a specified time.  No args will use interactive mode"},
    {"-load", fbe_cli_dest_usr_intf_load, "dest -load - Load the configuration file. \n"}, 
    {"-save", fbe_cli_dest_usr_intf_save, "dest -save - Save the configuration file. \n"}, 
    {"-clean", fbe_cli_dest_usr_intf_clean, "dest -clean - Deletes all error records. \n"}, 
    {"-add_scenario", (VOID (*) (fbe_u32_t , char* argv[], TEXT* )) fbe_cli_dest_usr_intf_add_scenario, "dest -add_scenario [scen_name] -d b_e_s - Add a built-in scenario.  No args will use interactive mode"}, /* SAFEBUG - temporary cast to trick compiler -- needs to be fixed as this function should not return status */
    {"-list_scenarios", fbe_cli_dest_usr_intf_list_scenarios, "dest -list_scenarios - List build-in scenarios\n"},
    {"-list_port_errors", fbe_cli_dest_usr_intf_list_port_errors, "dest -list_port_errors - List port errors that can be injected\n"},
    {"-list_scsi_errors", fbe_cli_dest_usr_intf_list_scsi_errors, "dest -list_scsi_errors - List scsi errors that can be injected\n"},
    {"-list_opcodes", fbe_cli_dest_usr_intf_list_opcodes, "dest -list_opcodes - List opcodes that can be injected\n"},
    {"-state",  fbe_cli_dest_usr_intf_state , "dest -state - Displays state of DEST Service"},
    {NULL, NULL, "DEST commands - add, start, stop, search, delete, display, load, save, clean \n"}
};

/**********************************************************************
 * The following is scsi error lookup table. User identifies the error 
 * string  as SCSI error. This needs to be translated to the 
 * defined scsi error value. This table provides the same purpose.
 **********************************************************************/
FBE_DEST_SCSI_ERR_LOOKUP dest_scsi_err_lookup_a[] =
{
    /* string name,                         value,                                  error_code,  valid LBA*/

    {"FBE_SCSI_CC_NOERR",                   FBE_SCSI_CC_NOERR,                      "000001",   1},
    {"FBE_SCSI_CC_AUTO_REMAPPED",           FBE_SCSI_CC_AUTO_REMAPPED,              "011802",   1},
    {"FBE_SCSI_CC_RECOVERED_BAD_BLOCK",     FBE_SCSI_CC_RECOVERED_BAD_BLOCK,        "011800",   1},
    {"FBE_SCSI_CC_RECOVERED_ERR_CANT_REMAP",FBE_SCSI_CC_RECOVERED_ERR_CANT_REMAP,   "011800",   0},
    {"FBE_SCSI_CC_DIE_RETIREMENT_START",    FBE_SCSI_CC_DIE_RETIREMENT_START,       "0118FD",   1},
    {"FBE_SCSI_CC_DIE_RETIREMENT_END",      FBE_SCSI_CC_DIE_RETIREMENT_END,         "0118FE",   1},
    {"FBE_SCSI_CC_PFA_THRESHOLD_REACHED",   FBE_SCSI_CC_PFA_THRESHOLD_REACHED,      "015D00",   1},
    {"FBE_SCSI_CC_RECOVERED_ERR_NOSYNCH",   FBE_SCSI_CC_RECOVERED_ERR_NOSYNCH,      "015C02",   1},
    {"FBE_SCSI_CC_RECOVERED_ERR",           FBE_SCSI_CC_RECOVERED_ERR,              "015C00",   1},
    {"FBE_SCSI_CC_BECOMING_READY",          FBE_SCSI_CC_BECOMING_READY,             "020401",   1},
    {"FBE_SCSI_CC_NOT_SPINNING",            FBE_SCSI_CC_NOT_SPINNING,               "020402",   1},
    {"FBE_SCSI_CC_FORMAT_IN_PROGRESS",      FBE_SCSI_CC_FORMAT_IN_PROGRESS,         "020404",   1},
    {"FBE_SCSI_CC_NOT_READY",               FBE_SCSI_CC_NOT_READY,                  "020000",   1},
    {"FBE_SCSI_CC_FORMAT_CORRUPTED",        FBE_SCSI_CC_FORMAT_CORRUPTED,           "023100",   1},
    {"FBE_SCSI_CC_SANITIZE_INTERRUPTED",    FBE_SCSI_CC_SANITIZE_INTERRUPTED,       "033180",   1},
    //{"FBE_SCSI_CC_MEDIA_ERROR_BAD_DEFECT_LIST", FBE_SCSI_CC_MEDIA_ERROR_BAD_DEFECT_LIST,,,},
    {"FBE_SCSI_CC_HARD_BAD_BLOCK",          FBE_SCSI_CC_HARD_BAD_BLOCK,             "031100",   1},
    {"FBE_SCSI_CC_MEDIA_ERR_CANT_REMAP",    FBE_SCSI_CC_MEDIA_ERR_CANT_REMAP,       "031100",   0},
    {"FBE_SCSI_CC_HARDWARE_ERROR_PARITY",   FBE_SCSI_CC_HARDWARE_ERROR_PARITY,      "044700",   1},
    {"FBE_SCSI_CC_HARDWARE_ERROR_SELF_TEST",FBE_SCSI_CC_HARDWARE_ERROR_SELF_TEST,   "043E03",   1},
    {"FBE_SCSI_CC_HARDWARE_ERROR",          FBE_SCSI_CC_HARDWARE_ERROR,             "040000",   1},
    {"FBE_SCSI_CC_ILLEGAL_REQUEST",         FBE_SCSI_CC_ILLEGAL_REQUEST,            "050000",   1},
    {"FBE_SCSI_CC_DEV_RESET",               FBE_SCSI_CC_DEV_RESET,                  "062900",   1},
    {"FBE_SCSI_CC_MODE_SELECT_OCCURRED",    FBE_SCSI_CC_MODE_SELECT_OCCURRED,       "062A00",   1},
    {"FBE_SCSI_CC_SYNCH_SUCCESS",           FBE_SCSI_CC_SYNCH_SUCCESS,              "065C01",   1},
    {"FBE_SCSI_CC_SYNCH_FAIL",              FBE_SCSI_CC_SYNCH_FAIL,                 "065C02",   1},
    {"FBE_SCSI_CC_UNIT_ATTENTION",          FBE_SCSI_CC_UNIT_ATTENTION,             "060000",   1},
    {"FBE_SCSI_CC_ABORTED_CMD_PARITY_ERROR", FBE_SCSI_CC_ABORTED_CMD_PARITY_ERROR,  "0B4700",   1},
    {"FBE_SCSI_CC_ABORTED_CMD",             FBE_SCSI_CC_ABORTED_CMD,                "0B0000",   1},
    {"FBE_SCSI_CC_UNEXPECTED_SENSE_KEY",    FBE_SCSI_CC_UNEXPECTED_SENSE_KEY,       "070000",   1},
    //{"FBE_SCSI_CC_MEDIA_ERR_DO_REMAP",      FBE_SCSI_CC_MEDIA_ERR_DO_REMAP,,,},
    //{"FBE_SCSI_CC_MEDIA_ERR_DONT_REMAP",    FBE_SCSI_CC_MEDIA_ERR_DONT_REMAP,,,},
    {"FBE_SCSI_CC_HARDWARE_ERROR_FIRMWARE", FBE_SCSI_CC_HARDWARE_ERROR_FIRMWARE,    "048000",   1},
    {"FBE_SCSI_CC_HARDWARE_ERROR_NO_SPARE", FBE_SCSI_CC_HARDWARE_ERROR_NO_SPARE,    "043200",   1},
    /* Won't allow injecting on the following name sense it depends on vendor == SEAGATE.  User will need to
       ensure Seagate drive and inject scsi error code. */
    //{"FBE_SCSI_CC_GENERAL_FIRMWARE_ERROR",  FBE_SCSI_CC_GENERAL_FIRMWARE_ERROR,     0x098000,   1},
    {"FBE_SCSI_CC_MEDIA_ERR_WRITE_ERROR",   FBE_SCSI_CC_MEDIA_ERR_WRITE_ERROR,      "030C00",   1},
    {"FBE_SCSI_CC_DEFECT_LIST_ERROR",       FBE_SCSI_CC_DEFECT_LIST_ERROR,          "031900",   1},
    //{"FBE_SCSI_CC_VENDOR_SPECIFIC_09_80_00", FBE_SCSI_CC_VENDOR_SPECIFIC_09_80_00,,,},
    {"FBE_SCSI_CC_SEEK_POSITIONING_ERROR",  FBE_SCSI_CC_SEEK_POSITIONING_ERROR,     "041500",   1},
    {"FBE_SCSI_CC_SEL_ID_ERROR",            FBE_SCSI_CC_SEL_ID_ERROR,               "06FF00",   1},
    {"FBE_SCSI_CC_RECOVERED_WRITE_FAULT",   FBE_SCSI_CC_RECOVERED_WRITE_FAULT,      "010300",   1},
    {"FBE_SCSI_CC_MEDIA_ERROR_WRITE_FAULT", FBE_SCSI_CC_MEDIA_ERROR_WRITE_FAULT,    "030300",   1},
    {"FBE_SCSI_CC_HARDWARE_ERROR_WRITE_FAULT", FBE_SCSI_CC_HARDWARE_ERROR_WRITE_FAULT, "040300", 1},
    {"FBE_SCSI_CC_INTERNAL_TARGET_FAILURE", FBE_SCSI_CC_INTERNAL_TARGET_FAILURE,    "044400",   1},
    {"FBE_SCSI_CC_RECOVERED_DEFECT_LIST_ERROR", FBE_SCSI_CC_RECOVERED_DEFECT_LIST_ERROR, "011900", 1},
    //{"FBE_SCSI_CC_ABORTED_CMD_ATA_TO",      FBE_SCSI_CC_ABORTED_CMD_ATA_TO,,,},
    {"FBE_SCSI_CC_INVALID_LUN",             FBE_SCSI_CC_INVALID_LUN,                "052500",   1},
    {"FBE_SCSI_LOGICAL_UNIT_FAILED_SELF_CONFIGURATION", FBE_SCSI_LOGICAL_UNIT_FAILED_SELF_CONFIGURATION, "024C00", 1},
    //{"FBE_SCSI_CC_DATA_OFFSET_ERROR",       FBE_SCSI_CC_DATA_OFFSET_ERROR,,,},
    {"FBE_SCSI_CC_SUPER_CAP_FAILURE",       FBE_SCSI_CC_SUPER_CAP_FAILURE,          "018030",   1},
    {"FBE_SCSI_CC_DRIVE_TABLE_REBUILD",     FBE_SCSI_CC_DRIVE_TABLE_REBUILD,        "020405",   1},
    //{"FBE_SCSI_CC_DEFERRED_ERROR",          FBE_SCSI_CC_DEFERRED_ERROR,,,},
    {"FBE_SCSI_CC_WRITE_PROTECT",           FBE_SCSI_CC_WRITE_PROTECT,              "072700",   1},
    {"FBE_SCSI_CC_SENSE_DATA_MISSING",      FBE_SCSI_CC_SENSE_DATA_MISSING,         "000000",   1},
    //{"FBE_SCSI_CC_TEMP_THRESHOLD_REACHED",  FBE_SCSI_CC_TEMP_THRESHOLD_REACHED,,,},

    {NULL, -1, NULL, -1}   // last record marker
};

/**********************************************************************
 * The following is scsi port error lookup table. User identifies the error 
 * string  as SCSI error. This needs to be translated to the 
 * defined scsi port error value.
 **********************************************************************/
FBE_DEST_PORT_ERR_LOOKUP fbe_dest_scsi_port_err_lookup_a[] =
{
    {"SCSI_INVALIDREQUEST",                 FBE_PORT_REQUEST_STATUS_INVALID_REQUEST},
    {"SCSI_DEVICE_BUSY",                    FBE_PORT_REQUEST_STATUS_BUSY},
    {"SCSI_DEVICE_NOT_PRESENT",             FBE_PORT_REQUEST_STATUS_DEVICE_NOT_LOGGED_IN},
    {"SCSI_BADBUSPHASE",                    FBE_PORT_REQUEST_STATUS_PROTOCOL_ERROR},
    {"SCSI_IO_TIMEOUT_ABORT",               FBE_PORT_REQUEST_STATUS_ABORT_TIMEOUT},
    {"SCSI_SELECTIONTIMEOUT",               FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT},
    {"SCSI_TOOMUCHDATA",                    FBE_PORT_REQUEST_STATUS_DATA_OVERRUN},
    {"SCSI_XFERCOUNTNOTZERO",               FBE_PORT_REQUEST_STATUS_DATA_UNDERRUN},
    {"SCSI_DRVABORT",                       FBE_PORT_REQUEST_STATUS_ABORTED_BY_DEVICE},
    {"SCSI_INCIDENTAL_ABORT",               FBE_PORT_REQUEST_STATUS_INCIDENTAL_ABORT},
    {NULL, -1},
};

/**********************************************************************
 * The following lists the built-in scenarios
 **********************************************************************/
FBE_DEST_SCENARIO_ELEMENT fbe_dest_scenario_table_a[] =
{
    {"SLOW_IO",             FBE_DEST_SCENARIO_SLOW_DRIVE},
    {"RANDOM_MEDIA_ERROR",  FBE_DEST_SCENARIO_RANDOM_MEDIA_ERROR},
    {"PROACTIVE_SPARE",     FBE_DEST_SCENARIO_PROACTIVE_SPARE},       
    {NULL, 0}
};

/**********************************************************************
 * The following is the lookup table for SCSI Opcode values. 
 * First values are the specific strings, second values are the hex 
 * values and final values are the short cuts. User could enter either 
 * the shortcuts or the specific string name as opcode entry in the 
 * error rule. This table will be looked up for both of these values 
 * to find the corresponding hex value.
 *********************************************************************/
FBE_DEST_SCSI_OPCODE_LOOKUP fbe_dest_scsi_opcode_lookup_a[] =
{
    {"SCSI_COPY",               SCSI_COPY,              "COPY"},
    {"SCSI_FORCE_RESERVE",      SCSI_FORCE_RESERVE,     "FRESERVE"},
    {"SCSI_DEFINE_GROUP",       SCSI_DEFINE_GROUP,      "DEFINE_GROUP"},
    {"SCSI_FORMAT_UNIT",        SCSI_FORMAT_UNIT,       "FORMAT_UNIT"},
    {"SCSI_INQUIRY",            SCSI_INQUIRY,           "INQUIRY"},
    {"SCSI_LOG_SENSE",          SCSI_LOG_SENSE,         "LOG_SENSE"},
    {"SCSI_MAINT_IN",           SCSI_MAINT_IN,          "MAINT"},
    {"SCSI_MAINT_OUT",          SCSI_MAINT_OUT,         "MAINT"},
    {"SCSI_MODE_SELECT_6",      SCSI_MODE_SELECT_6,     "MODE_SELECT"},
    {"SCSI_MODE_SELECT_10",     SCSI_MODE_SELECT_10,    "MODE_SELECT"},
    {"SCSI_MODE_SENSE_6",       SCSI_MODE_SENSE_6,      "MODE_SENSE"},
    {"SCSI_MODE_SENSE_10",      SCSI_MODE_SENSE_10,     "MODE_SENSE"},
    {"SCSI_PER_RESERVE_IN",     SCSI_PER_RESERVE_IN,    "RESERVE"},
    {"SCSI_PER_RESERVE_OUT",    SCSI_PER_RESERVE_OUT,   "RESERVE"},
    {"SCSI_PREFETCH",           SCSI_PREFETCH,          "PREFETCH"},
    {"SCSI_READ",               SCSI_READ,              "READ"},
    {"SCSI_READ_6",             SCSI_READ_6,            "READ"},
    {"SCSI_READ_10",            SCSI_READ_10,           "READ"},
    {"SCSI_READ_12",            SCSI_READ_12,           "READ"},
    {"SCSI_READ_16",            SCSI_READ_16,           "READ"},
    {"SCSI_READ_BUFFER",        SCSI_READ_BUFFER,       "READ"},
    {"SCSI_READ_CAPACITY",      SCSI_READ_CAPACITY,     "READ"},
    {"SCSI_READ_CAPACITY_16",   SCSI_READ_CAPACITY_16,  "READ"}, 
    {"SCSI_REASSIGN_BLOCKS",    SCSI_REASSIGN_BLOCKS,   "REASSIGN"},
    {"SCSI_RCV_DIAGNOSTIC",     SCSI_RCV_DIAGNOSTIC,    "DIAGNOSTIC"},
    {"SCSI_RELEASE",            SCSI_RELEASE,           "RELEASE"},
    {"SCSI_RELEASE_10",         SCSI_RELEASE_10,        "RELEASE"},
    {"SCSI_RELEASE_GROUP",      SCSI_RELEASE_GROUP,     "RELEASE"},
    {"SCSI_REQUEST_SENSE",      SCSI_REQUEST_SENSE,     "SENSE"},
    {"SCSI_REPORT_LUNS",        SCSI_REPORT_LUNS,       "LUNS"},
    {"SCSI_RESERVE",            SCSI_RESERVE,           "RESERVE"},
    {"SCSI_RESERVE_10",         SCSI_RESERVE_10,        "RESERVE"},
    {"SCSI_RESERVE_GROUP",      SCSI_RESERVE_GROUP,     "RESERVE"},
    {"SCSI_REZERO",             SCSI_REZERO,            "ZEROS"},
    {"SCSI_SEND_DIAGNOSTIC",    SCSI_SEND_DIAGNOSTIC,   "DIAGNOSTIC"},
    {"SCSI_RECEIVE_DIAGNOSTIC",  SCSI_RECEIVE_DIAGNOSTIC, "DIAGNOSTIC"},
    {"SCSI_SKIPMASK_READ",      SCSI_SKIPMASK_READ,     "SKIPMASK"},
    {"SCSI_SKIPMASK_WRITE",     SCSI_SKIPMASK_WRITE,    "SKIPMASK"},
    {"SCSI_START_STOP_UNIT",    SCSI_START_STOP_UNIT,   "STARTSTOP"},
    {"SCSI_SYNCRONIZE",         SCSI_SYNCRONIZE,        "SYNC"},
    {"SCSI_TEST_UNIT_READY",    SCSI_TEST_UNIT_READY,   "UNITREADY"},
    {"SCSI_VERIFY",             SCSI_VERIFY,            "VERIFY"},
    {"SCSI_VERIFY_16",          SCSI_VERIFY_16,         "VERIFY"},
    {"SCSI_VOLUME_SET_IN",      SCSI_VOLUME_SET_IN,     "VOLUME"},
    {"SCSI_VOLUME_SET_OUT",     SCSI_VOLUME_SET_OUT,    "VOLUME"},
    {"SCSI_WRITE",              SCSI_WRITE,             "WRITE"},
    {"SCSI_WRITE_6",            SCSI_WRITE_6,           "WRITE"},
    {"SCSI_WRITE_10",           SCSI_WRITE_10,          "WRITE"},
    {"SCSI_WRITE_12",           SCSI_WRITE_12,          "WRITE"},
    {"SCSI_WRITE_16",           SCSI_WRITE_16,          "WRITE"},
    {"SCSI_WRITE_BUFFER",       SCSI_WRITE_BUFFER,      "WRITE"},
    {"SCSI_WRITE_SAME",         SCSI_WRITE_SAME,        "WRITE"},
    {"SCSI_WRITE_SAME_16",      SCSI_WRITE_SAME_16,     "WRITE"},
    {"SCSI_WRITE_VERIFY",       SCSI_WRITE_VERIFY,      "WRITE"},
    {"SCSI_WRITE_VERIFY_16",    SCSI_WRITE_VERIFY_16,   "WRITE"},
    {NULL,                      0,                   NULL},

};

/**********************************************************************
 * fbe_cli_dest
 **********************************************************************
 *
 * @brief 
 *  This is the function that gets the user commands from the FCLI and
 *  passes the commands along with other parameters to the DEST module.
 *
 *  
 * @param 
 *  argc, [IO] - Number of command line parameters.
 *  argv, [IO] - array of command line parameters in strings.
 *  usage_p, [I] - description of DEST usage. 
 *
 * @return
 *  Nothing.
 *   
 *  
 **********************************************************************/
void fbe_cli_dest(int argc,
                  char** argv)
{

    /* Call dest_usr_interface function with the given arugments.
     * dest_usr_interface would return either SUCCESS or 
     * DEST_INVALID_ENTRY. If the return value is not success,
     * print the usage mode to the user. 
     */
    if(fbe_cli_dest_usr_interface(argc, argv) != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s\n", DEST_USAGE);
    }  
   
    return;
}

/************************************
 * end of fbe_cli_dest()
 * *********************************/

 
/**********************************************************************
 * fbe_cli_dest_usr_interface() 
 **********************************************************************
 * @brief
 *  This function provides the user interface to the FCLI. Based
 *  on the command entered by the user, this function invokes the right
 *  function call and passes the command line argument, if any.
 *  
 * @param 
 *  args, [I] - number of commandline arguments
 *  argv, [I] - command line arguments
 *   
 *  
 * @return
 *  Always returns DEST_SUCCESS.
 * 
 *
 **********************************************************************/
fbe_status_t fbe_cli_dest_usr_interface(fbe_u32_t args, char* argv[])
{
    FBE_DEST_CMDS_TBL *fbe_dest_cmd_tbl_ptr; /* Pointer to command table. */ 
    fbe_dest_state_type dest_state;
    
    /* Get the command table. 
     */
    fbe_dest_cmd_tbl_ptr = &fbe_dest_cmds_tbl_a[0];
    
    /* Check if there is any command along with DEST or if the user 
     * is just looking for help. If there is no command entered,
     * we return DEST_NOT_VALID_ENTRY. Calling function would display
     * the help/usage if DEST_SUCCESS is not returned.
     */
    if(args < 1)
    {
        return FBE_STATUS_INVALID;
    }
    
    /* Look in the commands table for matching command.  
     */    
    while( (fbe_dest_cmd_tbl_ptr->cmd_name != NULL) && 
            (strcmp(*argv, fbe_dest_cmd_tbl_ptr->cmd_name) ))
    {
        fbe_dest_cmd_tbl_ptr++;
    }
    
    /* Check the command extraced from the table. If there is a command
    * then execute it. It not, check non interactive cmd options
    */ 
    if(fbe_dest_cmd_tbl_ptr->cmd_name != NULL)
    {
        /* We found a match for the command, execute it. Before that
         * check if the third parameter is requesting for help.
         */
        if(args > 2)
        {
            if( (!strcmp(argv[2], "-h")) || (!strcmp(argv[2], "-help")) ||
                    (!strcmp(argv[2], "h")) || (!strcmp(argv[2], "help")) )
            {
                
                fbe_cli_printf("%s\n", fbe_dest_cmd_tbl_ptr->usage);
                return FBE_STATUS_OK;
            }
        }          
        
        if (fbe_api_dest_injection_get_state(&dest_state) != FBE_STATUS_OK)
        {       
            fbe_cli_printf("Please issue 'net start newneitpackage' to start DEST Service on SP.\n");  
            return FBE_STATUS_OK;  
        } 
        
        /* Execute the routine.*/  
        (fbe_dest_cmd_tbl_ptr->routine)(args, 
                                        argv, 
                                        fbe_dest_cmd_tbl_ptr->usage);
    }
    else
    {
        /* flush whitespaces so that FBE_CLI prompt is not displayed twice
        */
        fflush(stdout);
        return FBE_STATUS_INVALID;        
    }
    
    /* flush whitespaces so that FBE_CLI prompt is not displayed twice
    */
    fflush(stdout);    
    return FBE_STATUS_OK;
}

/*****************************************
 * end fbe_cli_dest_usr_interface() 
 *****************************************/
/**********************************************************************
 * fbe_cli_dest_usr_intf_init() 
 **********************************************************************
 * @brief
 *  This function is callback for user interface. This call is 
 *  triggered when the user enters init command.

 * @param
 *  args,   [I] - number of commandline arguments
 *  argv,   [I] - command line arguments
 *  usage,  [I] - description of usage of this command.
 *
 * @return
 *  Nothing. 
 * 
 *
 ***********************************************************************/
void fbe_cli_dest_usr_intf_init(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage)
{   
    fbe_cli_printf("%s\n", usage);
    /* flush whitespaces so that FBE_CLI prompt is not displayed twice
    */
    fflush(stdout);
    return;
}
/*****************************************
 * end fbe_cli_dest_usr_intf_init() 
 *****************************************/
/**********************************************************************
 * fbe_cli_dest_usr_intf_start() 
 **********************************************************************
 * @brief
 *  This function is callback for user interface. This call is 
 *  triggered when the user enters start command.This command will 
 *  inturn invoke dest_start which will start the error insertion 
 *  process
 * 
 *  
 * @param
 *  args,   [I] - number of commandline arguments
 *  argv,   [I] - command line arguments
 *  usage,  [I] - description of usage of this command.
 *
 * @return
 *  Nothing. 
 * 
 *
 ***********************************************************************/
void fbe_cli_dest_usr_intf_start(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage)
{
    FBE_DEST_ERROR_INFO status;
    /* Invoke dest start function.
     */
    status = fbe_api_dest_injection_start();
    if(status == FBE_STATUS_OK)
    {
        fbe_cli_printf("DEST: Error Insertion Started.\n");
    }
   /* flush whitespaces so that FBE_CLI prompt is not displayed twice
    */
    fflush(stdout);
    return;
}
/*****************************************
 * end fbe_cli_dest_usr_intf_start() 
 *****************************************/

/**********************************************************************
 * fbe_cli_dest_usr_intf_stop() 
 **********************************************************************
 * @brief
 *  This function is callback for user interface. This call is 
 *  triggered when the user enters stop command.This command will 
 *  inturn invoke dest_start which will stop the error insertion 
 *  process
 * 
 * @param
 *  args,   [I] - number of commandline arguments
 *  argv,   [I] - command line arguments
 *  usage,  [I] - description of usage of this command.
 *
 * @return
 *  Nothing. 
 *
 *
 ***********************************************************************/
void fbe_cli_dest_usr_intf_stop(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage) 
{ 
    FBE_DEST_ERROR_INFO status;
    /* Invoke the dest stop function.
     */
    status = fbe_api_dest_injection_stop();
    if(status == FBE_STATUS_OK)
    {
        fbe_cli_printf("DEST: Error Insertion Stopped.\n");
    }  
    /* flush whitespaces so that FBE_CLI prompt is not displayed twice
    */
    fflush(stdout);
    return;
}
/*****************************************
 * end fbe_cli_dest_usr_intf_stop() 
 *****************************************/

/**********************************************************************
 * fbe_cli_dest_usr_intf_load() 
 **********************************************************************
 * @brief
 * This function is callback for user interface. This call is 
 * triggered when the user enters load. If there is a third argument
 * present, that would be the file name that has the error rule 
 * to be loaded. Otherwise the default config file is loaded. 
 *  
 * @param
 *  args,   [I] - number of commandline arguments
 *  argv,   [I] - command line arguments
 *  usage,  [I] - description of usage of this command.
 *
 * @return
 *  Nothing. 
 * 
 *
 ***********************************************************************/
void fbe_cli_dest_usr_intf_load(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage)
{
    FBE_DEST_ERROR_INFO status;    
    
    /* Invoke the actual load function with the filename. 
     * Filename could be NULL (loads default value) or actual value. 
     */
    status = fbe_dest_usr_intf_load(argv[1]); 
    if(status != DEST_SUCCESS)
    {
        fbe_cli_printf("\nDEST: Error %X loading the Config File\n", status);
    }
    
    /* flush whitespaces so that FBE_CLI prompt is not displayed twice
    */
    fflush(stdout);
    return;
}
/*****************************************
 * end fbe_cli_dest_usr_intf_load() 
 *****************************************/

/**********************************************************************
 * fbe_cli_dest_usr_intf_save() 
  **********************************************************************
 * @brief
 *  This function is callback for user interface. This call is 
 *  triggered when the user enters save. If there is a third argument
 *  present, that would be the file name to which the configuration 
 *  should be saved. Otherwise the config is saved to the same file. 
 *
 *  Execution: dest -save <filename>
 * 
 *  
 * @param
 *  args,   [I] - number of commandline arguments
 *  argv,   [I] - command line arguments
 *  usage,  [I] - description of usage of this command.
 *
 * @return
 *  Nothing. 
 *
 *
 **********************************************************************/
void fbe_cli_dest_usr_intf_save(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage)
{
    fbe_char_t buff[FBE_DEST_MAX_STR_LEN];
    fbe_char_t filename[FBE_DEST_MAX_FILENAME_LEN];
    fbe_bool_t write = TRUE;    
    
    fbe_zero_memory(buff,sizeof(buff));
    fbe_zero_memory(filename,sizeof(filename));
    
    /* Check if the user entered any file name. If there is no file 
     * name, DEST would overwrite the existing file. Ask the user if
     * it is okay to overwrite the file. If the answer is yes, over
     * write the file. write flag is used to identify the user's option
     * in this piece of code. 
     */

    if(argv[1] == NULL)
    {
        write = FALSE;
        fbe_cli_printf("DEST: This would overwrite the existing File.\n");
        fbe_cli_printf("Do you wish to continue?\n");
        scanf("%s",buff); 
        if(strlen(buff) && ( !strcmp(buff, "Y") || !strcmp(buff, "y")))
        {
            write = TRUE;
        }
    }
    else
    {
        /*User has given file name, copy that file name*/       
        strncpy(filename,argv[1], FBE_DEST_MAX_FILENAME_LEN-1);
    }
    
    /* If write flag is set, go ahead and call the function responsible
     * for saving the configuration.
     */
    if(write)
    {           
        if(fbe_dest_usr_intf_save(filename) != DEST_SUCCESS)
        {
           fbe_cli_printf("DEST: Could not write to the file.\n");
        }                 
    }    
    /* flush whitespaces so that FBE_CLI prompt is not displayed twice
    */
    fflush(stdout);
    return;
}
/*****************************************
 * end fbe_cli_dest_usr_intf_save() 
 *****************************************/

/**********************************************************************
 * fbe_cli_dest_usr_intf_clean() 
  **********************************************************************
 * @brief
 *  This function is callback for user interface. This call is 
 *  triggered when the user enters clean. It removes all the error records. 
 * 
 *  
 * @param
 *  args,   [I] - number of commandline arguments
 *  argv,   [I] - command line arguments
 *  usage,  [I] - description of usage of this command.
 *
 * @return
 *  Nothing. 
 *
 *
 **********************************************************************/
void fbe_cli_dest_usr_intf_clean(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage)
{
    FBE_DEST_ERROR_INFO status;
    /* Invoke the dest clean function.
     */
    status = fbe_api_dest_injection_remove_all_records();
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("DEST: Failed to clean the error record queue.\n");
    }  
    else
        fbe_cli_printf("DEST: Deleted all records in the record queue.\n");
        
    /* flush whitespaces so that FBE_CLI prompt is not displayed twice
    */
    fflush(stdout);
    return;
}
/*****************************************
 * end fbe_cli_dest_usr_intf_clean() 
 *****************************************/

/**********************************************************************
 * fbe_cli_dest_usr_intf_add_interactive() 
 **********************************************************************
 * @brief
 *  This function is callback for user interface. This call is 
 *  triggered when the user enters add command. This command prompts
 *  the user to enter all the record parameter values and then
 *  the record is submitted to fbe_api_dest_injection_add_record function.
 *  
 * @param
 *  args,   [I] - number of commandline arguments
 *  argv,   [I] - command line arguments
 *  usage,  [I] - description of usage of this command.
 *
 * @return
 *  Nothing. 
 * 
 *
 ***********************************************************************/
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_intf_add_interactive(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage)
{   
    
    fbe_char_t buff[DEST_MAX_STR_LEN]; 
    fbe_dest_error_record_t err_record;
    fbe_u8_t i;  
    fbe_dest_record_handle_t err_record_handle = NULL;
    fbe_bool_t status = DEST_SUCCESS;
    fbe_lba_t lba_range[2];           
    fbe_object_id_t frulist[DEST_MAX_FRUS_PER_RECORD];
    fbe_u32_t frucount = 0;
    
    fbe_zero_memory(buff,sizeof(buff)); 
    
    /* Fill the error record with default values.
     */
    fbe_cli_dest_err_record_initialize(&err_record);     
    
    status = fbe_cli_dest_usr_get_bes_list(frulist, &frucount);    
    if (status != DEST_SUCCESS)
    {
        fbe_cli_printf("%s:\tDEST: Error in getting bes.\n",
                       __FUNCTION__);
        return status;
    }
    err_record.object_id = frulist[0];    
    
    /* Get LBA Range from the user.*/         
    status = fbe_cli_dest_usr_get_lba_range(lba_range);
    if (status != DEST_SUCCESS)
    {
        return status;
    }
    err_record.lba_start = lba_range[0];
    err_record.lba_end = lba_range[1];     
    
    /* Get the opcode from the user.*/     
    fbe_cli_dest_usr_get_opcode(err_record.opcode);
    
    /* Get error from the user.*/     
    status = fbe_cli_dest_usr_get_error(&err_record);
    if (DEST_SUCCESS != status)
    {
        return status;
    }
    
    /*Get the insertion count and frequency */
    status = fbe_cli_dest_usr_get_insertion_frequency(&err_record);
    if (DEST_SUCCESS != status)
    {
        return status;
    }
    
    /* Get reactivation settings */    
    status = fbe_cli_dest_usr_get_reactivation_settings(&err_record);
    if (DEST_SUCCESS != status)
    {
        return status;
    }
    
    /* validate record */
    status = fbe_dest_validate_new_record(&err_record);
    if (status != DEST_SUCCESS)
    {
        fbe_cli_printf("DEST: ADD failed.  There is a problem validating the record.\n");
        return DEST_FAILURE;
    }
    
    /* For each fru add error rule
    */ 
    
    for(i=0; i<frucount; i++)
    {
        err_record.object_id = frulist[i];
        status = fbe_cli_dest_validate_assign_lba_range(&err_record, err_record.object_id);
        if (status != DEST_SUCCESS)
        {
            fbe_cli_printf("DEST: ADD failed. Wrong LBA.\n");
            return DEST_FAILURE;
        }
        else
        {
            /* Start error injection in Physical Package.   */
            if (fbe_api_dest_injection_add_record(&err_record, &err_record_handle) != FBE_STATUS_OK)        
            {       
                fbe_cli_printf("DEST: %s ADD failed.There is a problem creating the record.\n",__FUNCTION__);       
                break;
            }   
            else
            {    
                fbe_cli_printf("DEST: DEST ADD issued.\n");                                
                fbe_dest_display_error_record(&err_record);            
            }  
        }
    }         
    return DEST_SUCCESS;  
}
/*****************************************
 * end fbe_dest_usr_intf_add_interactive() 
 *****************************************/
/**********************************************************************
 * fbe_cli_dest_usr_intf_add() 
 **********************************************************************
 * @brief
 *  This function is callback for user interface. This call is 
 *  triggered when the user enters add command. All add arguments
 *  will be provided on command line.
 * 
 *  
 * @param
 *  args,   [I] - number of commandline arguments
 *  argv,   [I] - command line arguments
 *  usage,  [I] - description of usage of this command.
 *
 * @return
 *  Nothing. 
 * 
 *
 ***********************************************************************/
void fbe_cli_dest_usr_intf_add(fbe_u32_t argc, fbe_char_t* argv[], fbe_char_t* usage)
{    
    FBE_DEST_ERROR_INFO status = DEST_SUCCESS;
    fbe_dest_error_record_t err_record; 
    fbe_dest_record_handle_t err_record_handle = NULL;
    
    /* If not args passed to 'add' then use interactive mode
     */
    if (argc == 1)
    {
        status = fbe_cli_dest_usr_intf_add_interactive(argc, argv, usage);
        if (status != DEST_SUCCESS)
        {
            /*  DEST_FAILURE is returned when we fail to get drive info
            */
            if ( status == DEST_FAILURE )
            {
                fbe_cli_printf("%s:\tDEST: Failed to create a record. %X\n", __FUNCTION__, status);
            }
            /* DEST_QUIT is returned when user explicitly
            * quits dest. 
            */
            if ( status == DEST_QUIT )
            {
                fbe_cli_printf("%s:\tDEST: Quit Dest. %X\n", __FUNCTION__, status);
            }          
            
            return;
        }
        /* flush whitespaces so that FBE_CLI prompt is not displayed twice
        */
        fflush(stdout);
        return;
    }else
    {                   
        /* Fill the error record with default values.
        */
        fbe_cli_dest_err_record_initialize(&err_record);        
        
        /* shift args passed "dest -add" */
        fbe_dest_shift_args(&argv,&argc);       
        fbe_dest_args(argc, argv, &err_record, &status);
        
        switch (status)
        {
            case DEST_SUCCESS:
                /* good.  nothing do do.*/
                break;      
                
            case DEST_CMD_USAGE_ERROR:
                dest_printf("DEST: Missing option for argument %s\n", argv[0]);                
                fbe_cli_printf("%s\n", DEST_USAGE);
                return;                 
                
            case DEST_FAILURE:
                dest_printf("DEST: Invalid argument: %s\n", argv[0]);       
                return;                 
                
            default:
                dest_printf("DEST: Error parsing command line\n");              
                break;        
        }      
        
        /* validate record */
        status = fbe_dest_validate_new_record(&err_record);
        if (status != DEST_SUCCESS)
        {
            fbe_cli_printf("DEST: ADD failed.  There is a problem validating the record.\n");
            return;
        }
        
        /* Start error injection in Physical Package.   */
        
        if (fbe_api_dest_injection_add_record(&err_record, &err_record_handle) != FBE_STATUS_OK)        
        {       
            fbe_cli_printf("DEST: %s ADD failed.There is a problem creating the record.\n",__FUNCTION__);            
        }   
        else
        {    
            fbe_cli_printf("DEST: DEST ADD issued.\n");                                 
            fbe_dest_display_error_record(&err_record); 
        }  
        
        /* flush whitespaces so that FBE_CLI prompt is not displayed twice
        */
        fflush(stdout);
        return;
    }
    
}
/*****************************************
 * end fbe_cli_dest_usr_intf_add() 
 *****************************************/
/**********************************************************************
 * fbe_cli_dest_usr_intf_add_spinup_interactive() 
 **********************************************************************
 * @brief
 *  This function is callback for user interface. This call is 
 *  triggered when the user enters add_spinup command. This command prompts
 *  the user to enter when to insert the error, type/value of error.
 *  
 * @param
 *  args,   [I] - number of commandline arguments
 *  argv,   [I] - command line arguments
 *  usage,  [I] - description of usage of this command.
 *
 * @return
 *  Nothing. 
 * 
 *
 ***********************************************************************/
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_intf_add_spinup_interactive(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage)
{   
    fbe_dest_error_record_t err_record;
    fbe_u8_t i;      
    fbe_dest_record_handle_t err_record_handle = NULL;
    fbe_bool_t status = DEST_SUCCESS;            
    fbe_object_id_t frulist[DEST_MAX_FRUS_PER_RECORD];
    fbe_u32_t frucount = 0;
    
    /* Fill the error record with default values.
     */
    fbe_cli_dest_err_record_initialize(&err_record);     
    
    status = fbe_cli_dest_usr_get_bes_list(frulist, &frucount);    
    if (status != DEST_SUCCESS)
    {
        fbe_cli_printf("%s:\tDEST: Error in getting bes.\n",
                       __FUNCTION__);
        return status;
    }
    err_record.object_id = frulist[0];  
    
    strncpy(err_record.opcode, "STARTSTOP", DEST_MAX_STR_LEN);
    
     err_record.injection_flag = FBE_DEST_INJECTION_ON_SEND_PATH;
    
    /* Get error from the user.*/     
    status = fbe_cli_dest_usr_get_error(&err_record);
    if (DEST_SUCCESS != status)
    {
        return status;
    }     
  
    /* For each fru add error rule
    */ 
    
    for(i=0; i<frucount; i++)
    {
        err_record.object_id = frulist[i];       
       
        /* Start error injection in Physical Package.   */
        if (fbe_api_dest_injection_add_record(&err_record, &err_record_handle) != FBE_STATUS_OK)        
        {       
            fbe_cli_printf("DEST: %s ADD failed.There is a problem creating the record.\n",__FUNCTION__);       
            break;
        }   
        else
        {    
            fbe_cli_printf("DEST: DEST ADD issued.\n");     
            fbe_cli_printf("DEST: Need to check the pdo's state to figure out if the insertion occured at spinup or spindown.\n"); 
            fbe_dest_display_error_record(&err_record);            
        }         
    }         
    return DEST_SUCCESS;  
}
/*****************************************
 * end fbe_cli_dest_usr_intf_add_spinup_interactive() 
 *****************************************/
/**********************************************************************
 * fbe_cli_dest_usr_intf_add_spinup() 
 **********************************************************************
 * @brief
 *  This function is callback for user interface. This call is 
 *  triggered when the user enters add command. All add arguments
 *  will be provided on command line.
 * 
 *  
 * @param
 *  args,   [I] - number of commandline arguments
 *  argv,   [I] - command line arguments
 *  usage,  [I] - description of usage of this command.
 *
 * @return
 *  Nothing. 
 * 
 *
 ***********************************************************************/
void fbe_cli_dest_usr_intf_add_spinup(fbe_u32_t argc, fbe_char_t* argv[], fbe_char_t* usage)
{    
    FBE_DEST_ERROR_INFO status = DEST_SUCCESS;
    fbe_dest_error_record_t err_record; 
    fbe_dest_record_handle_t err_record_handle = NULL;
    
    /* If not args passed to 'add' then use interactive mode
     */
    if (argc == 1)
    {
        status = fbe_cli_dest_usr_intf_add_spinup_interactive(argc, argv, usage);
        if (status != DEST_SUCCESS)
        {
            /*  DEST_FAILURE is returned when we fail to get drive info
            */
            if ( status == DEST_FAILURE )
            {
                fbe_cli_printf("%s:\tDEST: Invalid Entry.\n", __FUNCTION__);
            }
            /* DEST_QUIT is returned when user explicitly
            * quits dest. 
            */
            if ( status == DEST_QUIT )
            {
                fbe_cli_printf("%s:\tDEST: Quit Dest.\n", __FUNCTION__);
            }            
        }
       /* flush whitespaces so that FBE_CLI prompt is not displayed twice
        */
        fflush(stdout);
        return;
    }
    else
    {                   
        /* Fill the error record with default values.
        */
        fbe_cli_dest_err_record_initialize(&err_record);
        
        /* When a drive is spundown, cdb opcode is FBE_SCSI_START_STOP_UNIT*/ 
        strncpy(err_record.opcode, "STARTSTOP", DEST_MAX_STR_LEN);
        
        /* We want to inject errors on send path, not on completion path*/
        err_record.injection_flag = FBE_DEST_INJECTION_ON_SEND_PATH;
        
        /* shift args passed "dest -add" */
        fbe_dest_shift_args(&argv,&argc);       
        fbe_dest_args(argc, argv, &err_record, &status);
        
        switch (status)
        {
            case DEST_SUCCESS:
                /* good.  nothing do do.*/
                break;      
                
            case DEST_CMD_USAGE_ERROR:
                dest_printf("DEST: Missing option for argument %s\n", argv[0]);                
                fbe_cli_printf("%s\n", DEST_USAGE);
                return;                 
                
            case DEST_FAILURE:
                dest_printf("DEST: Invalid argument: %s\n", argv[0]);       
                return;                 
                
            default:
                dest_printf("DEST: Error parsing command line\n");              
                break;        
        }      
        
        /* validate record */
        status = fbe_dest_validate_new_record(&err_record);
        if (status != DEST_SUCCESS)
        {
            fbe_cli_printf("DEST: ADD failed.  There is a problem validating the record.\n");
            return;
        }
        
        /* Start error injection in Physical Package.   */
        
        if (fbe_api_dest_injection_add_record(&err_record, &err_record_handle) != FBE_STATUS_OK)        
        {       
            fbe_cli_printf("DEST: %s ADD failed.There is a problem creating the record.\n",__FUNCTION__);            
        }   
        else
        {    
            fbe_cli_printf("DEST: DEST ADD issued.\n");             
            fbe_dest_display_error_record(&err_record); 
        }  
        
        /* flush whitespaces so that FBE_CLI prompt is not displayed twice
        */
        fflush(stdout);
        return;
    }
    
}
/*****************************************
 * end fbe_cli_dest_usr_intf_add_spinup() 
 *****************************************/
/**********************************************************************
 * fbe_dest_usr_get_search_params() 
 **********************************************************************
 * @brief:
 *  This function is used to prompt the user for search parameters viz.
 *  fru, lba range and opcode. This function will be used by both
 *  search and delete error rules from the user prompt. 
 * 
 *  
 * @param:
 *  search_params_ptr,  [O]- Fill up the search parameter values.
 *
 * @return:   fbe_status_t   
 *                 DEST_SUCCESS - Success  
 *                 DEST_NOT_VALID_ENTRY - invaid params entered"
 **********************************************************************/
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_search_params(FBE_DEST_SEARCH_PARAMS* search_params_ptr)
{
    fbe_lba_t lba_range[2];
    FBE_DEST_ERROR_INFO status = DEST_NOT_VALID_ENTRY;  
    fbe_object_id_t frulist[DEST_MAX_FRUS_PER_RECORD];
    fbe_u32_t frucount = 0;

    /* The passed in pointer is not valid. So just return at this 
    * point or we end up panicking.
    */
    if(search_params_ptr == NULL)
    {
        fbe_cli_printf ("%s:Invalid search parameter input. ! Sts:%X\n", __FUNCTION__, status);
        return DEST_NOT_VALID_ENTRY;
    }
    /* Initialize the LBA Values.
    */
    search_params_ptr->lba_start=0;
    search_params_ptr->lba_end=0;

    /* Get the FRU number.
    */
    status = fbe_cli_dest_usr_get_bes_list(frulist, &frucount);    
    if (status != DEST_SUCCESS)
    {
        fbe_cli_printf("%s:\tDEST: Error in getting bes.\n",
                       __FUNCTION__);
        return status;
    }
    search_params_ptr->objid = frulist[0];    

    /* Get the LBA Range.
    */         
    status = fbe_cli_dest_usr_get_lba_range(lba_range);
    if (status != DEST_SUCCESS)
    {
        return status;
    }       
    search_params_ptr->lba_start = lba_range[0];
    search_params_ptr->lba_end = lba_range[1];      
    
    return DEST_SUCCESS;
}
/*****************************************
 * end fbe_cli_dest_usr_get_search_params
 *****************************************/
/**********************************************************************
 * fbe_cli_dest_usr_intf_search
 **********************************************************************
 *
 * @brief This function collects the parameters for searching the error
 * record.
 *
 * @param Error record handle
 * @param Pointer to error record
 *
 * @return     fbe_status_t
 *
 *********************************************************************/
void fbe_cli_dest_usr_intf_search(fbe_u32_t argc, fbe_char_t* argv[], fbe_char_t* usage)
{
    FBE_DEST_ERROR_INFO status = DEST_FAILURE; 
    FBE_DEST_SEARCH_PARAMS search_params_ptr;
    fbe_dest_record_handle_t handle_p = NULL;
    fbe_dest_error_record_t record_p;   
 
    status = fbe_cli_dest_usr_get_search_params(&search_params_ptr);
    if (status != DEST_SUCCESS)
    {
        if (status == DEST_NOT_VALID_ENTRY)
        {
            fbe_cli_printf("DEST: Enter valid parameters \n");           
        }      
        return;
    }       
    
    /*Fill-up error record*/
    record_p.object_id = search_params_ptr.objid;   
    record_p.lba_start = search_params_ptr.lba_start;
    record_p.lba_end = search_params_ptr.lba_end;  
    
    /*Validate the LBA range.*/
    status = fbe_cli_dest_validate_assign_lba_range(&record_p, record_p.object_id);
    
    if(status != DEST_SUCCESS)
    {
        fbe_cli_printf("%s:\tDEST: Start and Stop LBA range (%d - %d) invalid. %X\n",
                       __FUNCTION__, (int)record_p.lba_start, (int)record_p.lba_end, status);
        
        /* flush whitespaces so that FBE_CLI prompt is not displayed twice
        */
        fflush(stdout);        
        return;
    }   
    
    do{
    
        status = fbe_api_dest_injection_search_record(&record_p, &handle_p);  
        
        if(status != FBE_STATUS_OK || handle_p == NULL)
        {       
            fbe_cli_printf("%s:\tDEST: No records to display. %X\n", __FUNCTION__, status);
            /* flush whitespaces so that FBE_CLI prompt is not displayed twice
            */       
            fflush(stdout);       
            break;            
        }       
        /*  print the record */
        status=  fbe_dest_display_error_record(&record_p);
        if (status != DEST_SUCCESS)
        {
            fbe_cli_printf("%s:\tDEST: Error occured while printing. %X\n", __FUNCTION__, status);
        }   
        /* flush whitespaces so that FBE_CLI prompt is not displayed twice
        */
        fflush(stdout);   
    
    }while(1);
    /* flush whitespaces so that FBE_CLI prompt is not displayed twice
    */
    fflush(stdout);
    return;
}
/******************************************
 * end of fbe_cli_dest_usr_intf_search
 *****************************************/
 /**********************************************************************
 * @fn fbe_cli_dest_usr_intf_state
 **********************************************************************
 *
 * @brief This function displays the state of DEST Service
 *
 * @param: argc,   [I] - number of commandline arguments
 *         argv,   [I] - command line arguments
 *         usage,  [I] - description of usage of this command.
 *
* @return:  Nothing. 
 *
 *********************************************************************/
void fbe_cli_dest_usr_intf_state(fbe_u32_t argc, fbe_char_t* argv[], fbe_char_t* usage)
{    
    FBE_DEST_ERROR_INFO status = DEST_FAILURE; 
    fbe_dest_state_type dest_state = FBE_DEST_NOT_INIT;
    fbe_char_t dest_state_string[DEST_MAX_STR_LEN];;
    /* Display dest state.
     */
    status = fbe_api_dest_injection_get_state(&dest_state);
    if(status == FBE_STATUS_OK)
    {
        switch (dest_state) {
            case FBE_DEST_NOT_INIT:
                 strncpy(dest_state_string, "Not initialized", DEST_MAX_STR_LEN);                
                break;
            case FBE_DEST_INIT:
                strncpy(dest_state_string, "Initialized", DEST_MAX_STR_LEN);                 
                break;
            case FBE_DEST_START:
                 strncpy(dest_state_string, "Started", DEST_MAX_STR_LEN);                   
                break;
            case FBE_DEST_STOP:
                strncpy(dest_state_string, "Stopped", DEST_MAX_STR_LEN);                  
                break;
            default:
                break;
        }
        fbe_cli_printf("DEST State: %s.\n", dest_state_string);
    }
    if(dest_state == FBE_DEST_NOT_INIT)
    {         
         fbe_cli_printf("Please issue 'net start newneitpackage' to start DEST Service on SP.\n");         
    }
   
   /* flush whitespaces so that FBE_CLI prompt is not displayed twice
    */
    fflush(stdout);
    return; 
}
/**********************************************************************
 * @fn fbe_cli_dest_usr_intf_display
 **********************************************************************/
 
/**********************************************************************
 * @fn fbe_cli_dest_usr_intf_display
 **********************************************************************
 *
 * @brief This function collects the parameters for searching the error
 * record.
 *
 *  @param: args,   [I] - number of commandline arguments
 *          argv,   [I] - command line arguments
 *          usage,  [I] - description of usage of this command.
 *
 * @return :  Nothing. 
 *
 *********************************************************************/
void fbe_cli_dest_usr_intf_display(fbe_u32_t argc, fbe_char_t* argv[], fbe_char_t* usage)
{
    fbe_dest_record_handle_t record_handle = NULL;
    fbe_status_t status;    
    
    /* Display dest state.
     */  
    fbe_cli_dest_usr_intf_state(0,NULL,NULL);
   
    status = fbe_dest_display_error_records(&record_handle);    
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s:\tDEST: Failed to list the record. %X\n", __FUNCTION__, status);     
    }
    
    /* flush whitespaces so that FBE_CLI prompt is not displayed twice
    */
    fflush(stdout);
    return;
}
/**********************************************************************
 * @fn fbe_cli_dest_usr_intf_display
 **********************************************************************/
  
 /**********************************************************************
 * fbe_cli_dest_usr_intf_fail() 
 **********************************************************************
 * @brief
 *  This function is callback for user interface. This call is 
 *  triggered to fail a drive.
 *  
 * @param: args,   [I] - number of commandline arguments
 *         argv, [I] - command line arguments
 *         usage,  [I] - description of usage of this command.
 *
 * @return:  Nothing. 
 * 
 ***********************************************************************/
void fbe_cli_dest_usr_intf_fail(fbe_u32_t argc, fbe_char_t* argv[], fbe_char_t* usage)
{    
    FBE_DEST_ERROR_INFO status = DEST_SUCCESS;
    fbe_dest_error_record_t err_record; 
    fbe_dest_record_handle_t err_record_handle = NULL;    
    
    /* Fill the error record with default values.
    */
    fbe_cli_dest_err_record_initialize(&err_record);    
    
    err_record.dest_error_type = FBE_DEST_ERROR_TYPE_FAIL;
        
     /* shift args passed "dest -fail" */
    fbe_dest_shift_args(&argv,&argc);       
    fbe_dest_args(argc, argv, &err_record, &status);    
    switch (status)
    {
        case DEST_SUCCESS:
        /* good.  nothing do do.*/
        break;      
                
        case DEST_CMD_USAGE_ERROR:
            dest_printf("DEST: Missing option for argument %s\n", argv[0]);                
            fbe_cli_printf("%s\n", DEST_USAGE);
            return;                 
                
        case DEST_FAILURE:
            dest_printf("DEST: Invalid argument: %s\n", argv[0]);       
            return;                 
                
        default:
            dest_printf("DEST: Error parsing command line\n");              
            break;        
    }      
        
    /* validate record */
    status = fbe_dest_validate_new_record(&err_record);
    if (status != DEST_SUCCESS)
    {
        fbe_cli_printf("DEST: ADD failed.  There is a problem validating the record.\n");
        return;
    }
        
     /* Start error injection in Physical Package.  */
        
    if (fbe_api_dest_injection_add_record(&err_record, &err_record_handle) != FBE_STATUS_OK)        
    {       
        fbe_cli_printf("DEST: %s ADD failed.There is a problem creating the record.\n",__FUNCTION__);            
    }   
    else
    {    
        fbe_cli_printf("DEST: DEST ADD issued.\n");                 
        fbe_dest_display_error_record(&err_record); 
    }       
    
    /* flush whitespaces so that FBE_CLI prompt is not displayed twice
    */
    fflush(stdout);
    return;
}
/*****************************************
 * end fbe_cli_dest_usr_intf_fail() 
 *****************************************/

/**********************************************************************
* fbe_cli_dest_usr_intf_glitch() 
**********************************************************************
* @brief
*  This function is callback for user interface. This call is 
*  triggered to glitch a drive.
* 
* @param:
*  args,   [I] - number of commandline arguments
*  argv,   [I] - command line arguments
*  usage,  [I] - description of usage of this command.
*
* @return: 
*  Nothing. 
* 
***********************************************************************/
void fbe_cli_dest_usr_intf_glitch(fbe_u32_t argc, fbe_char_t* argv[], fbe_char_t* usage)
{    
    FBE_DEST_ERROR_INFO status = DEST_SUCCESS;
    fbe_dest_error_record_t err_record; 
    fbe_dest_record_handle_t err_record_handle = NULL;    
    
    /* Fill the error record with default values.
    */
    fbe_cli_dest_err_record_initialize(&err_record);    
    
    err_record.dest_error_type = FBE_DEST_ERROR_TYPE_GLITCH;
    
    /* shift args passed "dest -glitch" */
    fbe_dest_shift_args(&argv,&argc);       
    fbe_dest_args(argc, argv, &err_record, &status);    
    switch (status)
    {
        case DEST_SUCCESS:
            /* good.  nothing do do.*/
            break;      
            
        case DEST_CMD_USAGE_ERROR:
            dest_printf("DEST: Missing option for argument %s\n", argv[0]);                
            fbe_cli_printf("%s\n", DEST_USAGE);
            return;                 
            
        case DEST_FAILURE:
            dest_printf("DEST: Invalid argument: %s\n", argv[0]);       
            return;                 
            
        default:
            dest_printf("DEST: Error parsing command line\n");              
            break;        
    }      
    
    /* validate record */
    status = fbe_dest_validate_new_record(&err_record);
    if (status != DEST_SUCCESS)
    {
        fbe_cli_printf("DEST: ADD failed.  There is a problem validating the record.\n");
        return;
    }
    
    /* Start error injection in Physical Package.   */
    
    if (fbe_api_dest_injection_add_record(&err_record, &err_record_handle) != FBE_STATUS_OK)        
    {       
        fbe_cli_printf("DEST: %s ADD failed.There is a problem creating the record.\n",__FUNCTION__);            
    }   
    else
    {    
        fbe_cli_printf("DEST: DEST ADD issued.\n");                 
        fbe_dest_display_error_record(&err_record); 
    }       
        
   /* flush whitespaces so that FBE_CLI prompt is not displayed twice
    */
    fflush(stdout);
    return;
}
/*****************************************
 * end fbe_cli_dest_usr_intf_glitch() 
 *****************************************/
/**********************************************************************
 * fbe_cli_dest_usr_intf_delete() 
 **********************************************************************
 * @brief
 *  This function is callback for user interface. This call is 
 *  triggered when the user enters add command. All add arguments
 *  will be provided on command line.
 *  
 * @param:
 *  args,   [I] - number of commandline arguments
 *  argv,   [I] - command line arguments
 *  usage,  [I] - description of usage of this command.
 *
 * @return: Nothing. 
 * 
 *
 ***********************************************************************/
void fbe_cli_dest_usr_intf_delete(fbe_u32_t argc, fbe_char_t* argv[], fbe_char_t* usage)
{    
    FBE_DEST_ERROR_INFO status; 
   
    if (argc >= 2)
    {      
        fbe_dest_error_record_t record_p;
        fbe_dest_record_handle_t handle_p = NULL;
        
       /* shift args passed command */
        fbe_dest_shift_args(&argv,&argc); 
       
        record_p.record_id = fbe_dest_strtoint(argv[0]);        
        status = fbe_api_dest_injection_get_record_handle(&record_p, &handle_p);
        
        if(status != FBE_STATUS_OK || handle_p == NULL)
        {
            fbe_cli_printf("DEST: Unable to get handle of an error record. %X\n", status);  
            fflush(stdout);
            return;
        }      
        
        status = fbe_api_dest_injection_remove_record(handle_p);
        
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("DEST: Unable to delete the record. %X\n", status);   
            fflush(stdout);
            return;
        }   
        
        fbe_cli_printf("DEST: Successfully deleted one error record. %X\n", status);        
         
    }     
    /* flush whitespaces so that FBE_CLI prompt is not displayed twice
    */
    fflush(stdout);
    return;
}
/*****************************************
 * end fbe_cli_dest_usr_intf_delete() 
 *****************************************/

/**********************************************************************
 * fbe_cli_dest_usr_get_error_scenario()
 **********************************************************************
 * @brief
 *  This function is used to get the error scenario from the user. The user
 *  is prompted to enter error scenario out of the listed possible options.     
 *
 * @return     FBE_DEST_ERROR_INFO 
 * 
 *********************************************************************/
FBE_DEST_ERROR_SCENARIO_TYPE fbe_cli_dest_usr_get_error_scenario(void)
{
    fbe_char_t buff[DEST_MAX_STR_LEN];
    fbe_u32_t input;
    
    EmcpalZeroVariable(buff);
    
    dest_printf("Select error scenario\n");
    fbe_cli_dest_usr_intf_list_scenarios(0,NULL,NULL);    
    scanf("%s", buff);
    
    // Exit from the command if "quit" is entered;
    if (fbe_cli_dest_exit_from_command(buff))
    {
        fbe_cli_printf("\nDEST: Quitting DEST command...\n\n");
        return FBE_DEST_SCENARIO_INVALID;
    }

    input = fbe_dest_strtoint(buff);
    
    switch (input)
    {
        case 1:    
            return FBE_DEST_SCENARIO_SLOW_DRIVE;
        case 2:
            return FBE_DEST_SCENARIO_RANDOM_MEDIA_ERROR;            
        case 3:
            return FBE_DEST_SCENARIO_PROACTIVE_SPARE;        
        default:
            return FBE_DEST_SCENARIO_INVALID;
            
    }
}
/**********************************************************************
 * fbe_cli_dest_usr_get_error_scenario()
 **********************************************************************/

/**********************************************************************
 * dest_usr_add_scenario_slow_drive()
 **********************************************************************
 * @brief
 *  This function is used to insert pre-configured error record to 
 * slows a drive.    
 *
 * @param: objid,  list of frus
 *         frucount,   number of frus
 *
 * @return     FBE_DEST_ERROR_INFO 
 * 
 *********************************************************************/
void dest_usr_add_scenario_slow_drive(fbe_object_id_t *objid, fbe_u32_t frucount)
{
   fbe_u8_t i; 
   fbe_dest_error_record_t rec;
   fbe_dest_record_handle_t rec_handle = NULL;  
   fbe_lba_t end_of_drive = 0;

    /* Add slow drive scenario*/
    fbe_cli_dest_err_record_initialize(&rec);  
   
    strncpy(rec.opcode, DEST_ANY_STR, DEST_MAX_STR_LEN);
    rec.dest_error_type = FBE_DEST_ERROR_TYPE_NONE;  // no errors, just delay
    rec.num_of_times_to_insert = FBE_DEST_MAX_VALUE; 
    rec.is_random_frequency = TRUE;
    rec.frequency = 5;    // will be randomly inserted on average of 1 out every <frequency> ios.
    rec.delay_io_msec = 1000;  // msec
    rec.react_gap_type = FBE_DEST_REACT_GAP_NONE;     
    
    for(i=0; i<frucount; i++)
    {
        rec.object_id = objid[i];
        fbe_cli_dest_drive_physical_layout(rec.object_id, &rec.lba_start, &rec.lba_end, &end_of_drive);
        
        /* Start error injection in Physical Package.   */
        if (fbe_api_dest_injection_add_record(&rec, &rec_handle) != FBE_STATUS_OK)      
        {       
            fbe_cli_printf("DEST: %s ADD failed.There is a problem creating the record.\n",__FUNCTION__);       
            break;
        }   
        else
        {    
            fbe_cli_printf("DEST: DEST ADD issued.\n");         
            fbe_dest_display_error_record(&rec);            
        }  
    }
} 
/**********************************************************************
 * dest_usr_add_scenario_slow_drive()
 **********************************************************************/

/**********************************************************************
 * dest_user_add_scenario_random_media_error()
 **********************************************************************
 * @brief
 *  This function is used to insert pre-configured random media error record.
 *
 * @param: objid,  list of frus
 *         frucount,   number of frus
 *
 * @return :  Nothing.
 * 
 *********************************************************************/
void dest_user_add_scenario_random_media_error(fbe_object_id_t *objid, fbe_u32_t frucount)
{
    fbe_u8_t i; 
    fbe_dest_error_record_t rec;
    fbe_dest_record_handle_t rec_handle = NULL;  
    fbe_lba_t end_of_drive = 0;
    
    /* Add random media error scenario*/
    fbe_cli_dest_err_record_initialize(&rec);    
   
    rec.dest_error_type = FBE_DEST_ERROR_TYPE_SCSI; 
    rec.dest_error.dest_scsi_error.scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION;
    rec.num_of_times_to_insert = FBE_DEST_MAX_VALUE; 
    rec.is_random_frequency = TRUE;
    rec.frequency = 5; // will be randomly inserted on average of 1 out every <frequency> ios.
    rec.delay_io_msec = 0;  // msec
    rec.react_gap_type = FBE_DEST_REACT_GAP_NONE;  
    
    for (i=0; i<frucount; i++)
    {
        rec.object_id = objid[i];
        fbe_cli_dest_drive_physical_layout(rec.object_id, &rec.lba_start, &rec.lba_end, &end_of_drive);         
        strncpy(rec.opcode, "READ", DEST_MAX_STR_LEN);
        /* error value - 0x031100*/
        rec.dest_error.dest_scsi_error.scsi_sense_key = 03;
        rec.dest_error.dest_scsi_error.scsi_additional_sense_code = 11;
        rec.dest_error.dest_scsi_error.scsi_additional_sense_code_qualifier = 00;
        
        if (fbe_api_dest_injection_add_record(&rec, &rec_handle) != FBE_STATUS_OK)
        {                    
            strncpy(rec.opcode, "WRITE", DEST_MAX_STR_LEN);
            /* error value - 0x031503*/
            rec.dest_error.dest_scsi_error.scsi_sense_key = 03;
            rec.dest_error.dest_scsi_error.scsi_additional_sense_code = 15;
            rec.dest_error.dest_scsi_error.scsi_additional_sense_code_qualifier = 03;
            
            if (fbe_api_dest_injection_add_record(&rec, &rec_handle) != FBE_STATUS_OK)
            {       
                fbe_cli_printf("DEST: %s ADD failed.There is a problem creating the record.\n",__FUNCTION__);
                break;
            }
            else
            {    
                fbe_cli_printf("DEST: DEST ADD issued.\n");             
                fbe_dest_display_error_record(&rec);
            }
        }
        else
        {    
            fbe_cli_printf("DEST: DEST ADD issued.\n");             
            fbe_dest_display_error_record(&rec);
        }
    }  
}
/**********************************************************************
 * dest_user_add_scenario_random_media_error()
 **********************************************************************/

/**********************************************************************
 * dest_usr_add_scenario_proactive_spare()
 **********************************************************************
 * @brief
 *  This function is used to insert pre-configured error record to 
 * initiate proactive sparing
 *
 * @param: objid,  list of frus
 * @param:  frucount,   number of frus
 *
 * @return:  Nothing.
 * 
 *********************************************************************/

void dest_usr_add_scenario_proactive_spare(fbe_object_id_t *objid, fbe_u32_t frucount)
{
    fbe_u8_t i; 
    fbe_dest_error_record_t rec;
    fbe_dest_record_handle_t rec_handle = NULL;  
    fbe_lba_t end_of_drive = 0;
    
    /* Add proactive spare scenario*/
    fbe_cli_dest_err_record_initialize(&rec);  
   
    rec.dest_error_type = FBE_DEST_ERROR_TYPE_SCSI;    
    rec.dest_error.dest_scsi_error.scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION;
    /* error value - 0x015D00*/
    rec.dest_error.dest_scsi_error.scsi_sense_key = 01;
    rec.dest_error.dest_scsi_error.scsi_additional_sense_code = 0x5D;
    rec.dest_error.dest_scsi_error.scsi_additional_sense_code_qualifier = 00;
    
    for(i=0; i<frucount; i++)
    {
        rec.object_id = objid[i];
        fbe_cli_dest_drive_physical_layout(rec.object_id, &rec.lba_start, &rec.lba_end, &end_of_drive);
         
        /* Start error injection in Physical Package.   */
        if (fbe_api_dest_injection_add_record(&rec, &rec_handle) != FBE_STATUS_OK)              
        {       
            fbe_cli_printf("DEST: %s ADD failed.There is a problem creating the record.\n",__FUNCTION__);       
            break;
        }   
        else
        {    
            fbe_cli_printf("DEST: DEST ADD issued.\n");         
            fbe_dest_display_error_record(&rec);            
        }  
    }
}
/**********************************************************************
 * dest_usr_add_scenario_proactive_spare()
 **********************************************************************/
  
/**********************************************************************
 * fbe_cli_dest_usr_intf_add_scenario() 
 **********************************************************************
 * @brief
 *  This function is callback for user interface. This call is 
 *  triggered when the user enters add command. This command prompts
 *  the user to enter an error scenario, which is a preconfigured
 *  set of error records.  
 * 
 * @param
 *  args,   [I] - number of commandline arguments
 *  argv,   [I] - command line arguments
 *  usage,  [I] - description of usage of this command.
 *
 *  @return
 *  Nothing. 
 * 
 *
 ***********************************************************************/
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_intf_add_scenario(fbe_u32_t argc, fbe_char_t* argv[], fbe_char_t* usage)
{
    FBE_DEST_ERROR_SCENARIO_TYPE scenario = FBE_DEST_SCENARIO_INVALID;
    FBE_DEST_SCENARIO_ELEMENT * scen_p = fbe_dest_scenario_table_a;
    fbe_char_t buff[DEST_MAX_STR_LEN];
    fbe_job_service_bes_t bes;
    fbe_dest_error_record_t err_record_ptr;
    FBE_DEST_ERROR_INFO status;   
    fbe_object_id_t frulist[DEST_MAX_FRUS_PER_RECORD];
    fbe_u32_t frucount = 0;
    int i = 0;
    
    for(i = 0; i < DEST_MAX_FRUS_PER_RECORD; i++)
    {
        frulist[i] = FBE_OBJECT_ID_INVALID;
    }
    /* shift args passed command */
    fbe_dest_shift_args(&argv,&argc);  

    if (argc == 0)
    {
        /* No args passed in.  Get interactively*/
        scenario = fbe_cli_dest_usr_get_error_scenario();  
        if(scenario == FBE_DEST_SCENARIO_INVALID)
        {
            return DEST_QUIT;
        }
        /*Get the fru number from User in b_e_d format.*/      
        status = fbe_cli_dest_usr_get_bes_list(frulist, &frucount);    
        if (status != DEST_SUCCESS)
        {
            fbe_cli_printf("%s:\tDEST: Error in getting bes.\n",
                           __FUNCTION__);
            return status;
        }       
    }
    else if (argc >= 3)
    {
        /* get scenario from commandline*/
        fbe_dest_str_to_upper(buff, argv[0], sizeof(buff));
        while (scen_p->name != NULL)
        {                    
            if (!strcmp(buff, scen_p->name))
            {
                scenario = scen_p->value;
                break;
            }
            scen_p++;
        }

        /* get drive */
        if (!strcmp(argv[1],"-d"))
        {                      
            if(fbe_cli_convert_diskname_to_bed(argv[2], &bes) != FBE_STATUS_OK)
            {
                fbe_cli_printf("That is an invalid entry. Please try again\n");                                  
            }               
            
            /*Find the object id of a fru*/
            status = fbe_api_get_physical_drive_object_id_by_location(bes.bus, bes.enclosure, bes.slot, &err_record_ptr.object_id);
            
            if(err_record_ptr.object_id ==  FBE_OBJECT_ID_INVALID)
            { 
                fbe_cli_printf("%s:\tDEST: Error occured while finding the Object ID for drive %d_%d_%d. %X\n",
                               __FUNCTION__, bes.bus, bes.enclosure, bes.slot, status);
            } 
            frucount = 1;
            frulist[0] = err_record_ptr.object_id;
        }
       
    }
    else
    {
        dest_printf("DEST: Error: Invalid args to -add_scenario\n");
        return DEST_FAILURE;
    }

    switch(scenario)
    {
        case FBE_DEST_SCENARIO_SLOW_DRIVE:
            dest_usr_add_scenario_slow_drive(frulist, frucount);
            break;

        case FBE_DEST_SCENARIO_RANDOM_MEDIA_ERROR:
            dest_user_add_scenario_random_media_error(frulist, frucount);
            break;
            
        case FBE_DEST_SCENARIO_PROACTIVE_SPARE:
            dest_usr_add_scenario_proactive_spare(frulist, frucount);
            break;            
            
        default:
            dest_printf("DEST: Invalid Error Scenario %d\n", scenario);
            return DEST_SUCCESS;
    }
    /* flush whitespaces so that FBE_CLI prompt is not displayed twice
    */
    fflush(stdout);    
    return DEST_SUCCESS;
}
/*****************************************
 * end fbe_cli_dest_usr_intf_add_scenario() 
 *****************************************/

/**********************************************************************
 * fbe_cli_dest_usr_intf_list_port_errors() 
 **********************************************************************
 * @brief
 * List all possible port errors that can be added to error record
 *    
 *  
 *  @return
 *  none.
 *
 **********************************************************************/
void fbe_cli_dest_usr_intf_list_port_errors(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage)
{
    FBE_DEST_PORT_ERR_LOOKUP *port_table_p = fbe_dest_scsi_port_err_lookup_a;

    dest_printf("\nThe following PORT error strings can be injected:\n");
    while(port_table_p->scsi_name != NULL)
    {
        dest_printf("%s\n",port_table_p->scsi_name);
        port_table_p++;
    }
}
/**********************************************************************
 * fbe_cli_dest_usr_intf_list_port_errors() 
 **********************************************************************/

/**********************************************************************
 * fbe_cli_dest_usr_intf_list_opcodes() 
 **********************************************************************
 * @brief
 * List all possible opocde strings that can be added to error record
 *   
 * 
 *  None.
 *  
 *  @return
 *  none.
 *
 **********************************************************************/
void fbe_cli_dest_usr_intf_list_opcodes(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage)
{
    FBE_DEST_SCSI_OPCODE_LOOKUP *opcode_table_p = fbe_dest_scsi_opcode_lookup_a;

    dest_printf("\nThe following opcode name, value or shortcut can be injected:\n");
    dest_printf("%-30s %-6s %s\n",
                "Name", "Value", "Shortcut");         
    while(opcode_table_p->scsi_name != NULL)
    {
        dest_printf("%-30s 0x%-2x   %s\n",opcode_table_p->scsi_name, opcode_table_p->scsi_value, opcode_table_p->scsi_shortcut);
        opcode_table_p++;
    }
    dest_printf("%-30s %-4s   %s\n", "ANY", "ANY", "ANY");
}
/**********************************************************************
 * fbe_cli_dest_usr_intf_list_opcodes() 
 **********************************************************************/

/**********************************************************************
 * fbe_cli_dest_usr_intf_list_scenarios() 
 **********************************************************************
 * @brief
 * List possible built-in scenarios that can be added to error record
 *   
 *   
 *  @return
 *  none.
 *
 **********************************************************************/
void fbe_cli_dest_usr_intf_list_scenarios(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage)
{
    fbe_u8_t i = 1;
    FBE_DEST_SCENARIO_ELEMENT * scen_p = fbe_dest_scenario_table_a;

    while (scen_p->name != NULL)
    {
        dest_printf("%d. %s\n",i,  scen_p->name);
        scen_p++;
        i++;
    }
}
/**********************************************************************
 * fbe_cli_dest_usr_intf_list_scenarios() 
 **********************************************************************/

/**********************************************************************
 * fbe_cli_dest_err_record_initialize
 **********************************************************************
 * 
 * @brief
 *  Initialize the error record to the default values. This makes
 *  implemenation safe from any unwarranted values. If the value 
 *  entered is not a valid one, the default value is chosen instead. 
 *
 * @param
 *  err_rec_ptr, [IO] - This is the error record pointer for which the 
 *                      default values are initialized.
 *                      
 *  @return
 *  None 
 *  
 *********************************************************************/    
void fbe_cli_dest_err_record_initialize(fbe_dest_error_record_t* err_rec_ptr)
{

    fbe_zero_memory(err_rec_ptr, sizeof(fbe_dest_error_record_t));

    /* Key element for a record is FRU number. If the user did not
     * enter a valid FRU number, the record should be dropped. Hence
     * the field is initialized with invalid FRU value so that if the
     * user did not enter the right value, it will be dropped. 
     */
    err_rec_ptr->object_id                = FBE_OBJECT_ID_INVALID;
    
    /* LBA values will be initialized to 0. When adding the error 
     * rule to the table, if these values are still 0s (which might be
     * the case when the user did not enter the right value) then
     * the entire user space will be considered for that FRU.
     */
    err_rec_ptr->lba_start              = 0;
    err_rec_ptr->lba_end                = DEST_INVALID_LBA_RANGE;
    
    /* Opcode string is defaulted to ANY. Which means if no opcode
    * is selected, this error will be injected with out considering 
    * the opcode value. 
    */    
    strncpy(err_rec_ptr->opcode, DEST_ANY_STR, DEST_MAX_STR_LEN);

    err_rec_ptr->dest_error_type = FBE_DEST_ERROR_TYPE_SCSI;
    fbe_zero_memory(&(err_rec_ptr->dest_error), sizeof(err_rec_ptr->dest_error));
    err_rec_ptr->dest_error.dest_scsi_error.scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION;

    err_rec_ptr->valid_lba = 1;
    err_rec_ptr->deferred = 0;
    err_rec_ptr->delay_io_msec = 0;

    /* Default the number of times to insert the error to 1.
     */
    err_rec_ptr->num_of_times_to_insert = 1;

    /* Number of IOs per error insertion.  Defaults to insertion on every IO.
     */
    err_rec_ptr->frequency = 1;
    err_rec_ptr->is_random_frequency = FALSE;

    /* default to injection on completion path */
    err_rec_ptr->injection_flag = 0;

    /* Initialize reactivation settings */
    err_rec_ptr->react_gap_type = FBE_DEST_REACT_GAP_NONE;
    err_rec_ptr->react_gap_msecs = 0;
    err_rec_ptr->react_gap_io_count = 0;

    err_rec_ptr->is_random_gap = FALSE;
    err_rec_ptr->max_rand_msecs_to_reactivate = 0;
    err_rec_ptr->max_rand_react_gap_io_count = 0;


    /* If secs_to_reactivate is valid, the default is that the record will
     * always be reactivated after the time interval is up.
     */
    err_rec_ptr->is_random_react_interations = FALSE;
    err_rec_ptr->num_of_times_to_reactivate = FBE_DEST_ALWAYS_REACTIVATE;

    /* Turn on when a glitch needs to be inserted to the drive
    */
    err_rec_ptr->glitch_time = 0;
    
    /* This is the logs field that keeps track of number of times, 
     * the error is inserted.
     */
    err_rec_ptr->logs.times_inserted    = 0;

    /* Field that keeps track of how many times we reset the
     * times_inserted field due to a reactivation.
     */
    err_rec_ptr->logs.times_reset = 0;
    
    err_rec_ptr->times_reset = 0;
    
    err_rec_ptr->times_inserting = 0;
    err_rec_ptr->times_inserted = 0;

    err_rec_ptr->is_active_record = FALSE;

    return;
}
/******************************************
 * end of  fbe_cli_dest_err_record_initialize
 *****************************************/

/**********************************************************************
 * fbe_cli_dest_usr_get_bes_list()
 **********************************************************************
 * @brief
 *  This function is used to get the FRU number from the user. The user
 *  is prompted to enter Bus number, enclosure number and slot number.
 *  The entered value is checked to see if it is valid.  
 *
 * @param 
 *
 * @return     FBE_DEST_ERROR_INFO 
 * 
 *********************************************************************/
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_bes_list(fbe_object_id_t * const frulist, fbe_u32_t * const frucount)
{
    fbe_char_t buff[DEST_MAX_STR_LEN]; 
    fbe_char_t frulist_str[DEST_MAX_FRUS_PER_RECORD][DEST_MAX_STR_LEN];
    fbe_char_t *fru_str;
    fbe_u8_t i;    
    fbe_job_service_bes_t bes;
    fbe_object_id_t objid;
    fbe_bool_t    bGoodFru = FALSE;    
    fbe_status_t            status;
    
    /* Prompt the user to enter the values in one of the suggested
     * formats.
     */           
    fbe_zero_memory(frulist_str,sizeof(frulist_str));  
    
    do
    {
        dest_printf("Enter Drive Number.  Multiple drives can be added:");                
        scanf("%s", buff);          
        
        // Exit from the command if "quit" is entered;
        if (fbe_cli_dest_exit_from_command(buff))
        {
            fbe_cli_printf("\nDEST: Quitting DEST command...\n\n");
            return DEST_QUIT;
        }
        
        fru_str = strtok(buff, ","); 
        i = 0;        
    
        while(fru_str != NULL && i < DEST_MAX_FRUS_PER_RECORD)
        {
            strncpy(frulist_str[i], fru_str, strlen(fru_str));
            fru_str = strtok(NULL, ",");
            i++;
        }
    
        *frucount = i;
    
        bGoodFru = FALSE;
        for(i=0; i<*frucount; i++)
        {                                               
            status = fbe_cli_convert_diskname_to_bed(frulist_str[i], &bes);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("That is an invalid entry. Please try again\n");
                bGoodFru = FALSE;
                break;
            }          
            
            /*Find the object id of a fru*/
            status = fbe_api_get_physical_drive_object_id_by_location(bes.bus, bes.enclosure,
                     bes.slot, &objid);
            
            if(objid ==  FBE_OBJECT_ID_INVALID)
            { 
                fbe_cli_printf("%s:\tDEST: Error occured while finding the Object ID for drive %d_%d_%d. %X\n",
                               __FUNCTION__, bes.bus, bes.enclosure, bes.slot, status);
                bGoodFru = FALSE;
                break;
               // return DEST_FAILURE;
            }    
    
            bGoodFru = TRUE;
            frulist[i] = objid;       
    
            //fbe_cli_printf("DEST: added fru 0x%x, frucount %d.\n", frulist[i], i);
        }        
    
    } while( !bGoodFru );
  

    return DEST_SUCCESS;
}
/*****************************************
 * end fbe_cli_dest_usr_get_bes_list
 *****************************************/

/**********************************************************************
 *fbe_cli_dest_usr_get_lba_range() 
 **********************************************************************
 * @brief
 *  This function will prompt user to enter the lba range. If the lba
 *  range is left empty, default values are stored. Usually default
 *  values for LBA range at the user interface level are all 0s. 
 * 
 * PARAMETERES:
 *  lba_range_ptr,  [O] - Array of LBA Range
 *
 *  
 * Returns:
 *  Nothing 
 *
 *  
 **********************************************************************/
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_lba_range(fbe_lba_t* lba_range_ptr)
{
    fbe_u8_t        buff[FBE_DEST_MAX_STR_LEN];
    fbe_u32_t       MAX_NO_OF_LBA_VALUES = 2, ith_lba_value=0;
    fbe_char_t      *tmp_ptr;
    
    lba_range_ptr[0] = 0;
    lba_range_ptr[1] = 0;
    
    while(ith_lba_value < MAX_NO_OF_LBA_VALUES)
    {
        switch(ith_lba_value)
        {
            case GET_START_LBA:
            {
                /* Get lba start from the user.*/
                fbe_cli_printf("Start LBA (Use 0x prefix for Hex) [Enter 0x0 or 0X0 for Default]: ");
                break;
            }
            
            case GET_END_LBA:
            {
                /* Get lba end from the user.*/
                fbe_cli_printf("LBA End. (Use 0x prefix for Hex) [Enter 0x0 or 0X0 for Default]: ");
                break;
            }
    
            default:
            {
                fbe_cli_printf("Check default value : %d", ith_lba_value);
                return DEST_SUCCESS;
            }
        }
    
        fbe_zero_memory(buff,sizeof(buff));         
        scanf("%s",buff);   
        // Exit from the command if "quit" is entered;
        if (fbe_cli_dest_exit_from_command(buff))
        {
            fbe_cli_printf("\nDEST: Quitting DEST command...\n\n");
            return DEST_QUIT;
        }
        
        tmp_ptr= buff;
    
        /* Check string for "0x" or "0X" prefix
         */
        if(('0' == *tmp_ptr) && 
                (('x' == *(tmp_ptr+1)) || ('X' == *(tmp_ptr+1))))
        {
            tmp_ptr += 2;
    
            if(!strcmp(tmp_ptr,""))
            {
                fbe_cli_printf("Invalid LBA value detected. Please provide correct LBA value in hex format. \n");
            }
            else
            {
                lba_range_ptr[ith_lba_value]= fbe_atoh(tmp_ptr);
    
                if(lba_range_ptr[ith_lba_value] < 0 || lba_range_ptr[ith_lba_value]> MAX_DISK_LBA)
                {
                    fbe_cli_error("\nInvalid LBA value detected. Please try again\n");
                }
                else
                {
                    ith_lba_value++;
                }
            }
        }
        else /* Provided format is not in hex format */
        {
            fbe_cli_printf("Invalid LBA value detected. Please provide correct LBA value in hex format. \n");
        }
    }
    
    return DEST_SUCCESS;
}
/*****************************************
 * end fbe_cli_dest_usr_get_lba_range() 
 *****************************************/
/**********************************************************************
 *fbe_cli_dest_usr_get_error() 
 **********************************************************************
 * @brief
 * This function will prompt the user to enter error to insert. 
 * 
 * PARAMETERES:
 *  err_rec_ptr - Pointer to Error record
 *
 *  
 * Returns:
 *  Nothing 
 * 
 **********************************************************************/
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_error(fbe_dest_error_record_t* err_rec_ptr)
{
    fbe_u8_t                buff[FBE_NEIT_MAX_STR_LEN];
    fbe_u8_t                error_type = 0;
    fbe_bool_t              loop = FBE_FALSE;
    FBE_DEST_ERROR_INFO status = DEST_FAILURE;     
    
    do
    {
        /* prompt the user to enter the error type.
        */
        fbe_cli_printf("Enter error type.\n");   
        fbe_cli_printf("   1: delay only\n");
        fbe_cli_printf("   2: scsi\n");
        fbe_cli_printf("   3: port\n");       
        scanf("%s",buff);
    
        // Exit from the connand if "quit" is entered;
        if (fbe_cli_dest_exit_from_command(buff))
        {
            fbe_cli_printf("\nDEST: Quitting DEST command...\n\n");
            return DEST_QUIT;
        }     
        
        if(strlen(buff))
        {
            error_type = fbe_dest_strtoint(buff);
            if(error_type == FBE_STATUS_INVALID)
            {
                fbe_cli_error("\nInvalid entry. Please try again\n");
            }
            else
            {
                loop = FBE_TRUE;
            }
        }
        else
        {
            fbe_cli_error("\nInvalid entry. Please try again\n");
        }
    }while(!loop);
    
    switch(error_type)
    {
        case FBE_DEST_ERROR_TYPE_NONE:
            err_rec_ptr->dest_error_type = FBE_DEST_ERROR_TYPE_NONE;
            /* no error will be inserted.  This option is used when inserting a delay only*/
            dest_scsi_opcode_lookup_fbe(err_rec_ptr->opcode,     
                                        err_rec_ptr->dest_error.dest_scsi_error.scsi_command,
                                        &err_rec_ptr->dest_error.dest_scsi_error.scsi_command_count); 
            break;      
            
        case FBE_DEST_ERROR_TYPE_SCSI: 
            err_rec_ptr->dest_error_type = FBE_DEST_ERROR_TYPE_SCSI; 
            err_rec_ptr->dest_error.dest_scsi_error.scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION;
            status = fbe_cli_dest_usr_get_scsi_error(err_rec_ptr);              
            break;      
            
        case FBE_DEST_ERROR_TYPE_PORT: 
             err_rec_ptr->dest_error_type = FBE_DEST_ERROR_TYPE_PORT;
             status = fbe_cli_dest_usr_get_port_error(err_rec_ptr);          
             break;
            
        default:
            fbe_cli_printf("DEST: Invalid error type\n"); 
            break;
    } 
    
    /* Get Delay*/
    status = fbe_cli_dest_usr_get_delay(err_rec_ptr);
     
    return status;
}

/*****************************************
 * end fbe_cli_dest_usr_get_error() 
 *****************************************/
/**********************************************************************
 * fbe_cli_dest_usr_get_error_type() 
 **********************************************************************
 * @brief
 *  This function will prompt the user to enter a type of error to
 *  insert. 
 *
 * @param
 *  None.
 *  
 *  @return
 *  Retuns the error type
 * 
 **********************************************************************/
fbe_dest_error_type_t fbe_cli_dest_usr_get_error_type(void)
{   
    fbe_char_t error_type_str[DEST_MAX_STR_LEN];
    fbe_u32_t input;
    fbe_bool_t  loop = FBE_FALSE;
    
    do  
    {        
        fbe_cli_printf("Enter error type.\n");   
        fbe_cli_printf("   1: delay only\n");
        fbe_cli_printf("   2: scsi\n");
        fbe_cli_printf("   3: port\n");       
        scanf("%s",error_type_str);
        
        // Exit from the command if "quit" is entered;
        if (fbe_cli_dest_exit_from_command(error_type_str))
        {
            fbe_cli_printf("\nDEST: Quitting DEST command...\n\n");
            return FBE_DEST_ERROR_TYPE_INVALID;
        }
        
        if (strlen(error_type_str))
        {
            input =  fbe_dest_strtoint(error_type_str);   
            if(input == FBE_STATUS_INVALID)
            {
                fbe_cli_error("\nInvalid entry for error type. Please try again\n");
            }
            else
            {
                 loop = FBE_TRUE;
            }
              
        }
    } while (!loop); 
        
    switch(input)
    {
        case 1:   
            return FBE_DEST_ERROR_TYPE_NONE; 
                
        case 2:
            return FBE_DEST_ERROR_TYPE_SCSI;                               
                
        case 3:
            return FBE_DEST_ERROR_TYPE_PORT; 
                
        default:
            return FBE_DEST_ERROR_TYPE_INVALID;
               
    }    
    
}
/*****************************************
 * end fbe_cli_dest_usr_get_error_type() 
 *****************************************/
/**********************************************************************
 * fbe_cli_dest_usr_get_scsi_error() 
 **********************************************************************
 * @brief
 *  This function will prompt the user to enter error. If the error
 *  entered is in hex format, it will be considered as sense key value,
 *  else the error is SCSI string. 
 *
 * @param
 * err_rec_ptr - Pointer to Error record
 * 
 *  
 *  @return nothing
 * 
 **********************************************************************/
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_scsi_error(fbe_dest_error_record_t* err_rec_ptr)
{       
    fbe_char_t* temp_str_ptr;   
    fbe_char_t buff[DEST_MAX_STR_LEN]; 
    FBE_DEST_SCSI_ERR_LOOKUP* lookup_rec_p;
    fbe_u32_t error_val;
    
    while(1)  /* We'll hang here until we get it right */ 
    {    
        fbe_cli_printf("Enter Sense Key to inject or Scsi Error Name.\n");
        fbe_cli_dest_usr_intf_list_scsi_errors(0,NULL,NULL);    
        fbe_cli_printf("Example Sense Key Value:   0x031100\n");
        scanf("%s",buff);
        
        // Exit from the command if "quit" is entered;
        if (fbe_cli_dest_exit_from_command(buff))
        {
            fbe_cli_printf("\nDEST: Quitting DEST command...\n\n");
            return DEST_QUIT;
        }        
                
        /* This function has been rewritten due to dims #224998.
         * At this point, we can have one of three things as input:  
         * 1. A hex number with a leading 0x prefix.
         * 2. An error code label such as SCSI_CC_HARD_BAD_BLOCK.
         * 3. Garbage.  If it's garbage, we inform the user and give them another try.
         *    Decimal numbers are not allowed.  It makes no sense to enter a sense key as a decimal number.
         */
        
        /* Check for a hex number: */
        temp_str_ptr = strstr(buff, "0x");       /* Returns a pointer to the beginning of the 0x, or NULL if not found */
        
        if (temp_str_ptr == NULL)
        {
            lookup_rec_p = fbe_dest_scsi_error_lookup(buff);
            if (lookup_rec_p == NULL) 
            {
                fbe_cli_printf("DEST: Invalid hex value or scsi error name. Please try again.\n"); 
            }
            else  // user entered scsi error status name
            {
                /* convert scsi name to error settings*/
                strncpy(buff, lookup_rec_p->error_code, DEST_MAX_STR_LEN);
                err_rec_ptr->valid_lba = lookup_rec_p->valid_lba;
                break;
            }
        }
        else
        {
            /* This may be a valid hex value.
             */
            temp_str_ptr += 2;                        /* We have a leading 0x that we need to skip over. */
            
            if (fbe_atoh(temp_str_ptr) == 0xFFFFFFFFU )  /* atoh is really ascii hex to integer!!! */
            {
                fbe_cli_printf("DEST: Invalid hex value. Please try again.\n");                
            }
            else 
            {
                /* atoh told us that all chars are valid hex digits. */                 
                strncpy( buff, temp_str_ptr, DEST_MAX_STR_LEN);
                break;  /* We have a valid hex value here, return a pointer to the string */
            }    
        }
    
    }   
   
    
    dest_scsi_opcode_lookup_fbe(err_rec_ptr->opcode, 
                                err_rec_ptr->dest_error.dest_scsi_error.scsi_command,
                                &err_rec_ptr->dest_error.dest_scsi_error.scsi_command_count);   
    
    error_val = fbe_atoh(buff);         
    err_rec_ptr->dest_error.dest_scsi_error.scsi_sense_key = (error_val >> 16) & 0xFF;
    err_rec_ptr->dest_error.dest_scsi_error.scsi_additional_sense_code = (error_val >> 8) & 0xFF;
    err_rec_ptr->dest_error.dest_scsi_error.scsi_additional_sense_code_qualifier = error_val & 0xFF;
    err_rec_ptr->dest_error.dest_scsi_error.port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
        
    return DEST_SUCCESS;
}
/*****************************************
 * end fbe_cli_dest_usr_get_scsi_error() 
 *****************************************/
/**********************************************************************
 * fbe_cli_dest_usr_get_port_error() 
 **********************************************************************
 * @brief
 *  This function will prompt the user to enter port error. 
 *
 * @param
 *  err_rec_ptr - Pointer to Error record
 * 
 *  @return nothing 
 * 
 **********************************************************************/

FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_port_error(fbe_dest_error_record_t* err_rec_ptr)
{
    fbe_char_t error_uppercase[DEST_MAX_STR_LEN]; 
    fbe_char_t error_str[DEST_MAX_STR_LEN];
    fbe_u32_t port_error = 0;

    while(1)  /* We'll hang here until we get it right */ 
    {    
        fbe_cli_printf("Please enter a port error (string or numerical value). Possible values are:\n");
        fbe_cli_dest_usr_intf_list_port_errors(0,NULL,NULL);            
        scanf("%s", error_str);  
        
        // Exit from the command if "quit" is entered;
        if (fbe_cli_dest_exit_from_command(error_str))
        {
            fbe_cli_printf("\nDEST: Quitting DEST command...\n\n");
            return DEST_QUIT;
        }
        

        if (!strlen(error_str))
        {
            strncpy(error_str, "SCSI_IO_TIMEOUT_ABORT", DEST_MAX_STR_LEN);
        }

        /* if port error is numerical value then skip checking table */
        if (fbe_dest_strtoint(error_str) != DEST_INVALID_VALUE)
        {
            return DEST_SUCCESS;
        }

        fbe_dest_str_to_upper(error_uppercase, error_str, sizeof(error_uppercase));          
        
        /* search table for a match */
         port_error = dest_port_error_lookup_fbe(error_uppercase); 
         if (port_error == DEST_INVALID_VALUE)
         {                    
             /* still not valid */
             dest_printf("DEST: Error is not recognized. Record not created in DEST package.\n");            
         }
         else //match
             break;
    } 
    
    dest_scsi_opcode_lookup_fbe(err_rec_ptr->opcode, 
                                err_rec_ptr->dest_error.dest_scsi_error.scsi_command,
                                &err_rec_ptr->dest_error.dest_scsi_error.scsi_command_count); 
          
    err_rec_ptr->dest_error.dest_scsi_error.port_status = port_error;   
    
    return DEST_SUCCESS;
}
/**********************************************************************
 * fbe_cli_dest_usr_get_port_error() 
 **********************************************************************/
/**********************************************************************
 * fbe_cli_dest_usr_get_delay() 
 **********************************************************************
 * @brief
 *  This function will prompt the user to enter an IO delay in msec.
 *
 * @param
 *  err_rec_ptr - Pointer to Error record
 * 
 *  
 *  @return
 *  Retuns the delay
 * 
 **********************************************************************/
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_delay(fbe_dest_error_record_t* err_rec_ptr)
{
    fbe_char_t delay_str[DEST_MAX_STR_LEN];
    fbe_u32_t delay;
    
    do
    { 
        fbe_cli_printf("Enter delay in msec [0]");
        scanf("%s", delay_str);
        
        // Exit from the command if "quit" is entered;
        if (fbe_cli_dest_exit_from_command(delay_str))
        {
            fbe_cli_printf("\nDEST: Quitting DEST command...\n\n");
            return DEST_QUIT;
        }
    
        if(!strlen(delay_str))
        {
            strncpy(delay_str, "0", DEST_MAX_STR_LEN);
        }
    
        delay = fbe_dest_strtoint(delay_str);   
    
    } while (delay == DEST_INVALID_VALUE);  
     
    err_rec_ptr->delay_io_msec = delay;
    
    return DEST_SUCCESS;
}
/*****************************************
 * end fbe_cli_dest_usr_get_delay() 
 *****************************************/
/**********************************************************************
 * fbe_cli_dest_usr_get_opcode() 
 **********************************************************************
 * @brief
 *  This function will prompt the user to enter opcode. If the opcode
 *  entered is a valid string, then the value is returned. If not N/A
 *  is returned.
 * 
 *  
 * @param
 *  opcode_str, [O] - Opcode String entered by the user. 
 *  
 *
 *  @return
 *   hex value of the corrsponding opcode string.
 *
 *
 **********************************************************************/
fbe_char_t* fbe_cli_dest_usr_get_opcode(fbe_char_t* opcode_str)
{   
    fbe_char_t opcode[DEST_MAX_STR_LEN];
    
    fbe_cli_printf("OPCODE: Enter SCSI OPCODE string or shortcut such as read, write\n");
    scanf("%s", opcode);    
       
    /* Make sure that the error value is right. 
     */
    if(!strlen(opcode))
    {        
        strncpy(opcode, DEST_ANY_STR, DEST_MAX_STR_LEN);
    }
    fbe_dest_str_to_upper(opcode_str, opcode, DEST_MAX_STR_LEN);    
    
    while( (fbe_dest_scsi_opcode_lookup(opcode_str) == DEST_INVALID_VALUE) && 
            (strcmp(opcode_str, DEST_ANY_STR)))
    {
        fbe_cli_printf("Invalid Entry\nEnter Opcode [ANY]");
        scanf("%s", opcode);
        if(!strlen(opcode))
        {           
            strncpy(opcode, DEST_ANY_STR, DEST_MAX_STR_LEN);
        }
        fbe_dest_str_to_upper(opcode_str, opcode, DEST_MAX_STR_LEN);    
    }
    return opcode_str;
}
 /*****************************************
  * end fbe_cli_dest_usr_get_opcode() 
  *****************************************/
/**********************************************************************
 * fbe_cli_dest_usr_get_insertion_frequency() 
 **********************************************************************
 * @brief
 *  Get the insertion count and frequency (fixed or random).   
 *   
 * 
 * @param
 *  err_rec_ptr - Pointer to Error record
 *
 *  @return
 *  None.
 *
 *
 **********************************************************************/
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_insertion_frequency(fbe_dest_error_record_t* err_rec_ptr)
{
    fbe_char_t buff[DEST_MAX_STR_LEN];
    fbe_u32_t strategy;
    fbe_bool_t validInput = FALSE;

    /* prompt the user for number of insertions
     */               
    err_rec_ptr->num_of_times_to_insert = 1;   
    fbe_cli_printf("Enter times to insert [1]");
    scanf("%s", buff);    
    // Exit from the command if "quit" is entered;
    if (fbe_cli_dest_exit_from_command(buff))
    {
        fbe_cli_printf("\nDEST: Quitting DEST command...\n\n");
        return DEST_QUIT;
    }
    
    if(strlen(buff))
    {
        err_rec_ptr->num_of_times_to_insert = fbe_dest_strtoint(buff);               
    }

    do
    { 
        validInput = TRUE;

        strategy = 1;
        fbe_cli_printf("Enter insertion strategy:[1]\n");   
        fbe_cli_printf("   1: Insertion on every IO\n");
        fbe_cli_printf("   2: Fixed Frequency.  Fixed rate of insertion over given number of IOs.\n");
        fbe_cli_printf("   3: Random Insertion.  Randomly inserted IO over an average frequency.\n");
        scanf("%s", buff);  
        
        // Exit from the command if "quit" is entered;
        if (fbe_cli_dest_exit_from_command(buff))
        {
            fbe_cli_printf("\nDEST: Quitting DEST command...\n\n");
            return DEST_QUIT;
        }

        if (strlen(buff))
        {
            strategy = fbe_dest_strtoint(buff);   
        }

        switch(strategy)
        {
            case 1:   
                err_rec_ptr->frequency = 1;   
                break;   
                   
            case 2:
                err_rec_ptr->frequency = 1;
                fbe_cli_printf("Enter frequency.  Number of IOs per insertion.\n");   
                fbe_cli_printf("Example: Value of 2 will cause error insert on every 2nd IO.\n"); 
                scanf("%s", buff);               
                if(strlen(buff) )
                {
                    err_rec_ptr->frequency = fbe_dest_strtoint(buff);               
                }   
               
                if(err_rec_ptr->frequency == DEST_INVALID_VALUE)
                {
                    validInput = FALSE;
                }
                break; 

        case 3:
                err_rec_ptr->is_random_frequency = TRUE;  
                err_rec_ptr->frequency = 5;
                fbe_cli_printf("Enter average frequency per insertion.\n");
                fbe_cli_printf("Example.  Value of 5 will randomly inject errors that average 1 out of 5 IOs over length of test\n");
                scanf("%s", buff);               
                if(strlen(buff) )
                {
                    err_rec_ptr->frequency = fbe_dest_strtoint(buff);             
                }  

                if(err_rec_ptr->frequency == DEST_INVALID_VALUE)
                {
                    validInput = FALSE;
                }    
                break;

            default:
                validInput = FALSE;
        }

    } while (!validInput);   
    
    return DEST_SUCCESS;
}
/*****************************************
 * end fbe_cli_dest_usr_get_insertion_frequency() 
 *****************************************/
/**********************************************************************
 * fbe_cli_dest_usr_get_reactivation_settings() 
 **********************************************************************
 * @brief
 *  This function will prompt the user to enter reactivation settings,
 *  which will control the reactivation gap. A record which was disabled
 *  due to hitting the requisite number of times can be re-enabled after
 *  a specific time or io count delay.  
 *
 * @param
 *  err_rec_ptr - Pointer to Error record
 *
 *  
 *  @return
 *  none.
 *
 **********************************************************************/
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_reactivation_settings(fbe_dest_error_record_t* err_rec_ptr)
{
    fbe_char_t buff[DEST_MAX_STR_LEN]; 
    fbe_u32_t react_type;
    

    while(1)
    {
        fbe_cli_printf("Enter Reactivation type:[1] \n");
        fbe_cli_printf("   1: No record reactivation\n");
        fbe_cli_printf("   2: Based on time\n");
        fbe_cli_printf("   3: Based on IO count\n");
        fbe_cli_printf("   4: Based on random time\n");
        fbe_cli_printf("   5: Based on random IO count\n");
        scanf("%s", buff);  
        
        // Exit from the command if "quit" is entered;
        if (fbe_cli_dest_exit_from_command(buff))
        {
            fbe_cli_printf("\nDEST: Quitting DEST command...\n\n");
            return DEST_QUIT;
        }
    
        if(!strlen(buff))
        {
            strncpy(buff, "1", DEST_MAX_STR_LEN);
        }
    
        react_type = fbe_dest_strtoint(buff);
    
        switch (react_type)
        {
            case 1:
                err_rec_ptr->react_gap_type = FBE_DEST_REACT_GAP_NONE;
                break;
            case 2:
                err_rec_ptr->is_random_gap = FALSE;
                err_rec_ptr->react_gap_type = FBE_DEST_REACT_GAP_TIME;
                fbe_cli_dest_usr_get_msecs_reactivate(&err_rec_ptr->react_gap_msecs);                  
                break;
            case 3:
                err_rec_ptr->is_random_gap = FALSE;
                err_rec_ptr->react_gap_type = FBE_DEST_REACT_GAP_IO_COUNT;
                fbe_cli_dest_usr_get_reactivation_io_count(&err_rec_ptr->react_gap_io_count); 
                break;
            case 4:
                err_rec_ptr->is_random_gap = TRUE;
                err_rec_ptr->react_gap_type = FBE_DEST_REACT_GAP_TIME;
                fbe_cli_dest_usr_get_rand_msecs_reactivate(&err_rec_ptr->max_rand_msecs_to_reactivate); 
                break;
            case 5:
                err_rec_ptr->is_random_gap = TRUE;
                err_rec_ptr->react_gap_type = FBE_DEST_REACT_GAP_IO_COUNT;
                fbe_cli_dest_usr_get_rand_reactivation_io_count(&err_rec_ptr->max_rand_react_gap_io_count);
                break;

            default:  // invalid entry
                continue;
                break;
        }

        if (err_rec_ptr->react_gap_type != FBE_DEST_REACT_GAP_NONE)
        {        
            fbe_cli_dest_usr_get_num_reactivations(err_rec_ptr);
        }

        break; // valid entry
    }      
    
    return DEST_SUCCESS;
}
/*****************************************
 * end fbe_cli_dest_usr_get_reactivation_settings() 
 *****************************************/
/**********************************************************************
 * fbe_cli_dest_usr_get_num_reactivations() 
 **********************************************************************
 * @brief
 *  This function will prompt the user to enter if the record has be to reactivated randomly.
 *
 * @param
 * err_rec_ptr - Pointer to Error record
 *  
 *  @return
 *  none.
 *
 **********************************************************************/
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_num_reactivations(fbe_dest_error_record_t* err_rec_ptr)
{
    fbe_char_t buff[DEST_MAX_STR_LEN]; 

    fbe_cli_printf("Random reactivation iterations? (y/n)");
    scanf("%s", buff);
    
    if (!strlen(buff))
    {
        strncpy(buff, "n", DEST_MAX_STR_LEN);
    }

    if (buff[0] == 'y' || buff[0] == 'Y')   // random interations
    {        
        err_rec_ptr->is_random_react_interations = TRUE;
        fbe_cli_printf("Max number of times to reactivate.  Random interation will be somewhere from 0..max\n");
        scanf("%s", buff);
        
        // Exit from the command if "quit" is entered;
        if (fbe_cli_dest_exit_from_command(buff))
        {
            fbe_cli_printf("\nDEST: Quitting DEST command...\n\n");
            return DEST_QUIT;
        }        
    
        if (!strlen(buff))
        {
            strncpy(buff, "0", DEST_MAX_STR_LEN);
        }
        err_rec_ptr->num_of_times_to_reactivate = fbe_dest_strtoint(buff);  
    }
    else  // fixed num reactivations
    {
       err_rec_ptr->is_random_react_interations = FALSE;
       fbe_cli_printf("Number of times to reactivate [0]");
       scanf("%s", buff);
       
       // Exit from the command if "quit" is entered;
        if (fbe_cli_dest_exit_from_command(buff))
        {
            fbe_cli_printf("\nDEST: Quitting DEST command...\n\n");
            return DEST_QUIT;
        }
    
        if (!strlen(buff))
        {
            strncpy(buff, "0", DEST_MAX_STR_LEN);
        }
        err_rec_ptr->num_of_times_to_reactivate = fbe_dest_strtoint(buff);
    }
    
    return DEST_SUCCESS;
}
/**********************************************************************
 * fbe_cli_dest_usr_get_num_reactivations() 
 **********************************************************************/
/**********************************************************************
 * fbe_cli_dest_usr_get_reactivation_io_count() 
 **********************************************************************
 * @brief
 *  This function will prompt the user to enter the number of ios to skip before 
 * re-enabling the disabled error record. 
 *
 * @param
 *  gap_io_count  -  number of ios to skip before re-enabling the disabled error record. 
 *
 *  
 *  @return
 *  none.
 *
 **********************************************************************/
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_reactivation_io_count(fbe_u32_t * const gap_io_count)
{
    fbe_char_t buff[DEST_MAX_STR_LEN]; 

    do
    {
        fbe_cli_printf("Enter num IO to skip [0]");
        scanf("%s", buff); 
        
        // Exit from the command if "quit" is entered;
        if (fbe_cli_dest_exit_from_command(buff))
        {
            fbe_cli_printf("\nDEST: Quitting DEST command...\n\n");
            return DEST_QUIT;
        }

        if(!strlen(buff))
        {
            strncpy(buff, "0", DEST_MAX_STR_LEN);
        }

        *gap_io_count = fbe_dest_strtoint(buff);
    } while (*gap_io_count == DEST_INVALID_VALUE);
    
    return DEST_SUCCESS;

}
/**********************************************************************
 * fbe_cli_dest_usr_get_reactivation_io_count() 
 **********************************************************************/
/**********************************************************************
 * fbe_cli_dest_usr_get_rand_reactivation_io_count() 
 **********************************************************************
 * @brief
 *  This function will prompt the user to enter the maximum number of ios to 
 * skip in case of a random reactivation iteration before re-enabling the disabled  
 * error record.
 *
 * @param
 *  gap_io_count  -  number of ios to skip before re-enabling the disabled error record. 
 * 
 *  
 *  @return
 *  none.
 *
 **********************************************************************/
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_rand_reactivation_io_count(fbe_u32_t * const gap_io_count)
{
    fbe_char_t buff[DEST_MAX_STR_LEN]; 

    do
    {
        fbe_cli_printf("Enter max num IO to skip [0]");
        scanf("%s", buff);
        
        // Exit from the command if "quit" is entered;
        if (fbe_cli_dest_exit_from_command(buff))
        {
            fbe_cli_printf("\nDEST: Quitting DEST command...\n\n");
            return DEST_QUIT;
        }
        
        if(!strlen(buff))
        {
            strncpy(buff, "0", DEST_MAX_STR_LEN);
        }

        *gap_io_count = fbe_dest_strtoint(buff);
    } while (*gap_io_count == DEST_INVALID_VALUE);
     
    return DEST_SUCCESS;
}
/**********************************************************************
 * fbe_cli_dest_usr_get_rand_reactivation_io_count() 
 **********************************************************************

/**********************************************************************
 * fbe_cli_dest_usr_get_msecs_reactivate() 
 **********************************************************************
 * @brief
 *  This function will prompt the user to enter a milliseconds value to 
 *  reactivate a record.  If a record is disabled due to being hit
 *  the requisite number of times they may re-enable it after
 *  a specific time delay that can range from 0..0xFFFFFFFE seconds.
 *
 * @param
 *  mseconds_p,  [IO] - Miliseconds entered by the user.  
 *  
 *  @return
 *  none.
 * 
 **********************************************************************/
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_msecs_reactivate(fbe_u32_t * const mseconds_p)
{
    fbe_char_t buff[DEST_MAX_STR_LEN]; 

    do
    {
        fbe_cli_printf("Milliseconds to reactivate in.");
        scanf("%s", buff);
        
        // Exit from the command if "quit" is entered;
        if (fbe_cli_dest_exit_from_command(buff))
        {
            fbe_cli_printf("\nDEST: Quitting DEST command...\n\n");
            return DEST_QUIT;
        }
        
        *mseconds_p = fbe_dest_strtoint(buff);       

    } while (*mseconds_p == DEST_INVALID_VALUE);
    
    return DEST_SUCCESS;
}
/*****************************************
 * end fbe_cli_dest_usr_get_msecs_reactivate() 
 *****************************************/
/**********************************************************************
 * fbe_cli_dest_usr_get_rand_msecs_reactivate() 
 **********************************************************************
 * @brief
 *  This function will prompt the user to enter a maximumu milliseconds value to 
 *  wait before reactivating a record.  
 *
 * @param
 *  mseconds_p,  [IO] - Miliseconds entered by the user. 
 * 
 *  
 *  @return
 *  none.
 * 
 **********************************************************************/
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_rand_msecs_reactivate(fbe_u32_t * const mseconds_p)
{
    fbe_char_t buff[DEST_MAX_STR_LEN]; 

    do
    {
        fbe_cli_printf("Max milliseconds to wait before reactivating.");
        scanf("%s", buff);
        // Exit from the command if "quit" is entered;
        if (fbe_cli_dest_exit_from_command(buff))
        {
            fbe_cli_printf("\nDEST: Quitting DEST command...\n\n");
            return DEST_QUIT;
        }
        *mseconds_p = fbe_dest_strtoint(buff);       

    } while (*mseconds_p == DEST_INVALID_VALUE);
    
     return DEST_SUCCESS;
}
/*****************************************
 * end fbe_cli_dest_usr_get_rand_msecs_reactivate() 
 *****************************************/
/*!**************************************************************
 * @fn fbe_dest_display_error_records
 ****************************************************************
 * @brief
 * This function is used to print the records.
 * 
 * @param handle_p - Pointer to error record handle
 *
 * @return fbe_status_t 
 *
 ****************************************************************/
fbe_status_t fbe_dest_display_error_records(fbe_dest_record_handle_t *handle_p)
{
    fbe_status_t status;
    fbe_dest_error_record_t record_p;

    /*Initialize the Error record*/
    fbe_cli_dest_err_record_initialize(&record_p); 

    do{
        /* For getting first record, we send handle_p as NULL. */
        status = fbe_api_dest_injection_get_record_next(&record_p, handle_p);

        if(*handle_p == NULL || status != FBE_STATUS_OK)
        {
            return status;
        }

        status = fbe_dest_display_error_record(&record_p);

    }while(*handle_p != NULL);

    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_dest_display_error_records
 *****************************************/
/**********************************************************************
* fbe_dest_display_error_record
**********************************************************************  
* 
*  @brief:
*  This function is used to display the error record. 
*  
* @param
*  rec_p,   [I] - Error Record to be displayed.
*  
*      
*  @return
*  DEST_SUCCESS - upon successful completion.
*  DEST_NO_FRU_RECORD - if the passed argument is NULL. 
*
*
**********************************************************************/
FBE_DEST_ERROR_INFO fbe_dest_display_error_record(fbe_dest_error_record_t * rec_p)
{ 
    fbe_char_t gap_type_str[DEST_MAX_STR_LEN];
    fbe_char_t error_type_str[DEST_MAX_STR_LEN]; 
    fbe_char_t port_error[DEST_MAX_STR_LEN];     
    fbe_job_service_bes_t bes;
    FBE_DEST_ERROR_INFO status;  
    fbe_package_id_t package_id = FBE_PACKAGE_ID_PHYSICAL;
    fbe_class_id_t class_id = FBE_CLASS_ID_SAS_PHYSICAL_DRIVE ;

    /* Verify if the record pointer passed is not NULL.
    */
    if(rec_p == NULL)
    {
        fbe_cli_printf("DEST: This record is not valid to display\n");
        return DEST_NO_FRU_RECORD;
    }  
      
    status = fbe_cli_get_bus_enclosure_slot_info(rec_p->object_id,
            class_id, &bes.bus, &bes.enclosure, &bes.slot, package_id);
   
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s:\tDEST: Failed to get bes info. %X\n", __FUNCTION__, status);
        return DEST_FAILURE;
    }  

    switch (rec_p->react_gap_type)
    {
        case FBE_DEST_REACT_GAP_INVALID:
           strncpy(gap_type_str, "invalid", DEST_MAX_STR_LEN);
           break;
        case FBE_DEST_REACT_GAP_IO_COUNT:
            strncpy(gap_type_str, "io_count", DEST_MAX_STR_LEN);
            break;
        case FBE_DEST_REACT_GAP_NONE:
            strncpy(gap_type_str, "none", DEST_MAX_STR_LEN);
            break;
        case FBE_DEST_REACT_GAP_TIME:
            strncpy(gap_type_str, "time", DEST_MAX_STR_LEN);
            break;
        default:
            strncpy(gap_type_str, "unkown", DEST_MAX_STR_LEN);
            break;    
    }    
   
    fbe_cli_printf("--------------------------------------------------\n");
    fbe_cli_printf("Record id:                 %u\n", rec_p->record_id);
    fbe_cli_printf("Object id:                 0x%x\n", rec_p->object_id);
    fbe_cli_printf("Drive:                     %u_%u_%u\n", bes.bus, bes.enclosure, bes.slot);
    fbe_cli_printf("LBA:                       [0x%llX .. 0x%llX]\n", (unsigned long long)rec_p->lba_start, (unsigned long long)rec_p->lba_end);
    fbe_cli_printf("Opcode:                    %s\n", rec_p->opcode);
   
    
    switch (rec_p->dest_error_type)
    {
        case FBE_DEST_ERROR_TYPE_INVALID:
            strncpy(error_type_str, "invalid", DEST_MAX_STR_LEN);
            break;
        case FBE_DEST_ERROR_TYPE_NONE:
            strncpy(error_type_str, "none", DEST_MAX_STR_LEN);
            break;
        case FBE_DEST_ERROR_TYPE_PORT:          
            strncpy(port_error, dest_port_status_lookup_fbe(rec_p->dest_error.dest_scsi_error.port_status), DEST_MAX_STR_LEN);  
            strncpy(error_type_str, "port", DEST_MAX_STR_LEN);            
            fbe_cli_printf("Port Error:                %s\n", port_error);      
            break;
        case FBE_DEST_ERROR_TYPE_SCSI:
            strncpy(error_type_str, "scsi", DEST_MAX_STR_LEN);            
            fbe_cli_printf("Error(SK/ASC/ASCQ):        %x/%x/%x\n", rec_p->dest_error.dest_scsi_error.scsi_sense_key, rec_p->dest_error.dest_scsi_error.scsi_additional_sense_code, rec_p->dest_error.dest_scsi_error.scsi_additional_sense_code_qualifier);      
            break;
        case FBE_DEST_ERROR_TYPE_GLITCH:
            strncpy(error_type_str, "glitch", DEST_MAX_STR_LEN);
            break;
        case FBE_DEST_ERROR_TYPE_FAIL:
            strncpy(error_type_str, "fail", DEST_MAX_STR_LEN);
            break;
        default:
            strncpy(error_type_str, "unkown", DEST_MAX_STR_LEN);
            break;
    }
    
    fbe_cli_printf("Error Type:                %s\n", error_type_str);   
    fbe_cli_printf("Sense Data - valid lba:    %u\n", rec_p->valid_lba);
    fbe_cli_printf("Sense Data - deferred:     %u\n", rec_p->deferred);
    fbe_cli_printf("Num insertions:            %u\n", rec_p->num_of_times_to_insert);
    fbe_cli_printf("Frequency:                 %u\n", rec_p->frequency);
    fbe_cli_printf("Is random freq:            %u\n", rec_p->is_random_frequency);
    fbe_cli_printf("Reactivation Gap Type:     %s\n", gap_type_str);
    fbe_cli_printf("Gap milliseconds:          %u\n", rec_p->react_gap_msecs);
    fbe_cli_printf("Gap IO count:              %u\n", rec_p->react_gap_io_count);
    fbe_cli_printf("Is random gap:             %u\n", rec_p->is_random_gap);
    fbe_cli_printf("Random gap sec max:        %u\n", rec_p->max_rand_msecs_to_reactivate);
    fbe_cli_printf("Random gap count max:      %u\n", rec_p->max_rand_react_gap_io_count);
    fbe_cli_printf("Is random reactivations:   %u\n", rec_p->is_random_react_interations);
    fbe_cli_printf("Num reactivations:         %u\n", rec_p->num_of_times_to_reactivate);
    fbe_cli_printf("Delay IO msec:             %u\n", rec_p->delay_io_msec);
    fbe_cli_printf("stats - Times reactivated: %u\n", rec_p->times_reset);
    fbe_cli_printf("stats - Times inserted:    %u\n", rec_p->times_inserted); 
    
    /* flush whitespaces so that FBE_CLI prompt is not displayed twice
    */
    fflush(stdout);   
    return DEST_SUCCESS;
}
/*****************************************
* end fbe_dest_display_error_record()
*****************************************/

/****************************************************************
 * @fn fbe_cli_dest_drive_physical_layout
 ****************************************************************
 *  @brief                                                
 *        This function is called to determine start/end of user space
 * and end of the drive
 *
 *  @param:                                                      
 *       objid - PDO object id,
 *       start_user_space - Start of user space on drive
 *       end_user_space - End of user space on drive
 *       end_drive_space - End lba of drive
 *
 *  @return: FBE_DEST_ERROR_INFO
 *
 *      
 *****************************************************************/
FBE_DEST_ERROR_INFO fbe_cli_dest_drive_physical_layout(fbe_object_id_t objid, fbe_lba_t * start_user_space, fbe_lba_t * end_user_space, fbe_lba_t * end_drive_space)
{
    fbe_status_t status = FBE_STATUS_INVALID; 
    fbe_object_id_t pvd_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pdo_id = FBE_OBJECT_ID_INVALID;
    fbe_api_provision_drive_info_t pvd_info;
    fbe_u32_t port_number, drive_number, enclosure_number;
    fbe_physical_drive_information_t drive_information;    
    
    status = fbe_api_get_object_port_number (objid, &port_number);
    if (status != FBE_STATUS_OK) {
        return DEST_FAILURE;
    }
    status = fbe_api_get_object_enclosure_number(objid, &enclosure_number);    
    if (status != FBE_STATUS_OK) {
        return DEST_FAILURE;
    }
    status = fbe_api_get_object_drive_number (objid, &drive_number);
    if (status != FBE_STATUS_OK) {
        return DEST_FAILURE;
    }
    
    /* Get the provision drive object using b_e_s info */
    status = fbe_api_provision_drive_get_obj_id_by_location(port_number, enclosure_number, drive_number, &pvd_id);
    
    if(status != FBE_STATUS_OK)
    {
        /* if unable to get the object id of the drive, then return error from here */
        fbe_cli_printf ("%s:Failed to get object id of %d_%d_%d ! Sts:%X\n", __FUNCTION__, port_number, enclosure_number, drive_number, status);
        return DEST_FAILURE;
    }
    /* Get the provision object information */
    fbe_zero_memory(&pvd_info, sizeof pvd_info);
    status = fbe_api_provision_drive_get_info(pvd_id, &pvd_info);  
    
    if(status != FBE_STATUS_OK)
    {
        /* if unable to get the pvd object info, then return error from here */
        fbe_cli_printf ("%s:Failed to get pvd object info ! Sts:%X\n", __FUNCTION__, status);
        return DEST_FAILURE;
    }
    *start_user_space = pvd_info.default_offset;
    *end_user_space = pvd_info.capacity;
    
    /*Find the object id of a fru*/             
    if (FBE_STATUS_OK !=  fbe_api_get_physical_drive_object_id_by_location(port_number, enclosure_number, drive_number, &pdo_id))                
    { 
        fbe_cli_printf("%s:\tDEST: Error occured while finding the Object ID for drive %d_%d_%d.\n",
                       __FUNCTION__, port_number, enclosure_number, drive_number);    
        return DEST_FAILURE;
    }   
    
    fbe_zero_memory(&drive_information, sizeof(fbe_physical_drive_information_t));  
    
    status = fbe_api_physical_drive_get_drive_information(pdo_id, &drive_information, FBE_PACKET_FLAG_NO_ATTRIB);   
    if(status != FBE_STATUS_OK)
    {
        /* if unable to get the pvd object info, then return error from here */
        fbe_cli_printf ("%s:Failed to get pvd object info ! Sts:%X\n", __FUNCTION__, status);
        return DEST_FAILURE;
    }
    *end_drive_space = drive_information.gross_capacity;   
    
    return DEST_SUCCESS;
    
}
/*****************************************
 * end fbe_cli_dest_drive_physical_layout
 *****************************************/
 
/*!**************************************************************
 * @fn fbe_cli_dest_validate_assign_lba_range
 ****************************************************************
 * @brief
 *  This function validates the LBA range of a given error record
 *  pointer. If the LBA start and LBA end of the error rule is 0,
 *  then the user space is considered. Otherwise, the values entered
 *  are treated as the values. The user space will also be considered
 *  if the LBA end entered is less than LBA start. 
 * 
 * @param err_rec_ptr - Pointer to error Record to be inserted to the table.
 *        objid - object id of drive
 *
 * @return FBE_DEST_ERROR_INFO
 * 
 ****************************************************************/
FBE_DEST_ERROR_INFO fbe_cli_dest_validate_assign_lba_range(fbe_dest_error_record_t* 
        err_rec_ptr, fbe_object_id_t objid)
{
    fbe_lba_t start_user_space = 0;
    fbe_lba_t end_user_space = 0;
    fbe_lba_t end_drive_space = 0;
    
    fbe_cli_dest_drive_physical_layout(objid, &start_user_space, &end_user_space, &end_drive_space);      

    if(DEST_ANY_RANGE == end_user_space)
    {
        return DEST_FAILURE;
    }
    
    /* Default lba_start to the start of user space if the user input is invalid.
    */
    if((err_rec_ptr->lba_start <= 0) || (err_rec_ptr->lba_start > end_drive_space))
    {
        /* Set lba_start at beginning of the user space.*/
        err_rec_ptr->lba_start = start_user_space;
    }
    
    /* Default lba_end to the end of user space if the user input is invalid.
    */
    if((err_rec_ptr->lba_end <= 0) || (err_rec_ptr->lba_end > end_drive_space))
    {
        /* Set lba_end at end of the user space.*/
        err_rec_ptr->lba_end = end_user_space;
    }

    /* If LBA start and LBA end are both equal to 0 or LBA start is 
     * greater than LBA end, then the entered values are considered
     * not valid. Hence replace the values with entire user space range.
     */
    if( ((err_rec_ptr->lba_start == 0) && (err_rec_ptr->lba_end == 0)) ||
            (err_rec_ptr->lba_start > err_rec_ptr->lba_end))
    {
        /* Get the beginning of the user space.*/
        err_rec_ptr->lba_start = start_user_space;

        /* Get the end of the user space.*/
        err_rec_ptr->lba_end = end_user_space;
    } 

    if( ((err_rec_ptr->lba_start == 0) && (err_rec_ptr->lba_end == 0)) ||
            ((err_rec_ptr->lba_start > err_rec_ptr->lba_end)) ||
            (err_rec_ptr->lba_start == FBE_DEST_ANY_RANGE))
    {
        return FBE_DEST_ANY_RANGE;
    }
    return DEST_SUCCESS;
}
/*****************************************
 * end fbe_cli_dest_validate_lba_range
 *****************************************/

/**********************************************************************
 * fbe_dest_validate_new_record() 
 **********************************************************************
 *
 *  @brief:
 *  This function is used to validate the eligibility of a record in
 *  the lookup table. Validate the New error record to see if all the 
 *  values submitted are legible. If the values are not legible, then 
 *  the entry is not stored in the list. 
 * 
 *  
 * @param
 *  new_rec_ptr, [IO] - Record to be inserted into the error table. 
 *
 *  @return
 *  DEST_SUCCESS - if record can be inserted.
 *  DEST_FAILURE - if there is any error with the record.  
 *
 * 
 **********************************************************************/
FBE_DEST_ERROR_INFO fbe_dest_validate_new_record(fbe_dest_error_record_t * err_record_ptr)
{
    FBE_DEST_ERROR_INFO status = DEST_SUCCESS;    
    fbe_lba_t end_of_drive = 0;
    
    if (err_record_ptr->object_id == FBE_OBJECT_ID_INVALID)
    {
        dest_printf("DEST: Error: drive invalid or not provided\n");
        status = DEST_FAILURE;
    }
    if (err_record_ptr->lba_start == DEST_INVALID_VALUE_64 ||
            err_record_ptr->lba_end == DEST_INVALID_VALUE_64 ||
            err_record_ptr->lba_start > err_record_ptr->lba_end)
    {
        /* if lba is not entered, assign userspace start and end as our lba range */
         fbe_cli_dest_drive_physical_layout(err_record_ptr->object_id, &err_record_ptr->lba_start, &err_record_ptr->lba_end, &end_of_drive);             
    }
    if (err_record_ptr->dest_error_type == FBE_DEST_ERROR_TYPE_INVALID)
    {
        dest_printf("DEST: Error: error_type invalid or not provided.\n");
        status = DEST_FAILURE;
    }
    if (err_record_ptr->dest_error_type == FBE_DEST_ERROR_TYPE_SCSI &&
             !strcmp(err_record_ptr->dest_error.dest_scsi_error.scsi_command, DEST_ANY_STR))
    {
        dest_printf("DEST: Error: SK error is invalid or not provided.\n");
        status = DEST_FAILURE;
    }
    if (err_record_ptr->dest_error_type == FBE_DEST_ERROR_TYPE_PORT && 
            !strcmp(err_record_ptr->dest_error.dest_scsi_error.scsi_command, DEST_ANY_STR) &&
            !strcmp(dest_port_status_lookup_fbe(err_record_ptr->dest_error.dest_scsi_error.port_status), DEST_ANY_STR))
    {        
        fbe_cli_printf("DEST: Error: Port error is invalid or not provided. error=%u\n", err_record_ptr->dest_error.dest_port_error.port_status);
        status = DEST_FAILURE;
    }
    if(err_record_ptr->dest_error_type == FBE_DEST_ERROR_TYPE_GLITCH && err_record_ptr->glitch_time == 0 )
    {
        err_record_ptr->glitch_time = 10;
    }
    if (err_record_ptr->delay_io_msec == DEST_INVALID_VALUE)
    {
        err_record_ptr->delay_io_msec = 0;       
    }    
    if  (strcmp(err_record_ptr->opcode,"STARTSTOP") && (fbe_dest_scsi_opcode_lookup(err_record_ptr->opcode) == DEST_INVALID_VALUE))
    {
            strncpy(err_record_ptr->opcode, DEST_ANY_STR, DEST_MAX_STR_LEN);
            dest_scsi_opcode_lookup_fbe(err_record_ptr->opcode, 
                                        err_record_ptr->dest_error.dest_scsi_error.scsi_command,
                                        &err_record_ptr->dest_error.dest_scsi_error.scsi_command_count);             
    }      
    if (err_record_ptr->num_of_times_to_insert == DEST_INVALID_VALUE)
    {
        err_record_ptr->num_of_times_to_insert = 1;        
    }
    if (err_record_ptr->frequency == DEST_INVALID_VALUE)
    {
        err_record_ptr->frequency = 1 ;       
    }
    if (err_record_ptr->react_gap_type == FBE_DEST_REACT_GAP_INVALID)
    {
        dest_printf("DEST: Error: Invalid reactivation gap type\n");
        status = DEST_FAILURE;
    }
    if (err_record_ptr->is_random_gap)
    {
        if (err_record_ptr->react_gap_type == FBE_DEST_REACT_GAP_TIME &&
                err_record_ptr->max_rand_msecs_to_reactivate == DEST_INVALID_VALUE)
        {
            err_record_ptr->max_rand_msecs_to_reactivate = 0;                   
        }
        else if (err_record_ptr->react_gap_type == FBE_DEST_REACT_GAP_IO_COUNT &&
                 err_record_ptr->max_rand_react_gap_io_count == DEST_INVALID_VALUE)
        {
            err_record_ptr->max_rand_react_gap_io_count = 0;           
        }
    }
    else
    {
        if (err_record_ptr->react_gap_type == FBE_DEST_REACT_GAP_TIME &&
                err_record_ptr->react_gap_msecs == DEST_INVALID_VALUE)
        {
            err_record_ptr->react_gap_msecs = 0;           
        }
        else if (err_record_ptr->react_gap_type == FBE_DEST_REACT_GAP_IO_COUNT &&
                 err_record_ptr->react_gap_io_count == DEST_INVALID_VALUE)
        {
            err_record_ptr->react_gap_io_count = 0;           
        }
    }
    if (err_record_ptr->num_of_times_to_reactivate == DEST_INVALID_VALUE)
    {
        err_record_ptr->num_of_times_to_reactivate = 0;        
    }    
    
    return status;
}
/*****************************************
 * end fbe_dest_validate_new_record()
 *****************************************/

/**********************************************************************
 * fbe_dest_scsi_error_lookup() 
 **********************************************************************
 *  @brief:
 * List all possible error strings that can be added to error record
 *   
 *  
 *  @return
 *  none.
 *
 **********************************************************************/

void fbe_cli_dest_usr_intf_list_scsi_errors(fbe_u32_t args, fbe_char_t* argv[],  fbe_char_t* usage)
{
    FBE_DEST_SCSI_ERR_LOOKUP *scsi_table_p = dest_scsi_err_lookup_a;

    fbe_cli_printf("\nThe following SCSI error strings can be injected:\n");
    while(scsi_table_p->scsi_name != NULL)
    {
        fbe_cli_printf("%s\n",scsi_table_p->scsi_name);
        scsi_table_p++;
    }
}
/*****************************************
 * end fbe_cli_dest_usr_intf_list_scsi_errors
 *****************************************/

/**********************************************************************
 * fbe_dest_scsi_error_lookup() 
 **********************************************************************
 *  @brief:
 *  This function is used to provide lookup into the scsi error codes. 
 *  For given SCSI Error Code in string format, provided by the user,
 *  this function returns a ptr to the matching record in the lookup
 *  table.
 *   
 * @param
 *  scsi_error_str_ptr, [I] - SCSI Error code in string format. 
 * 
 *
 *  @return
 *  Returns ptr to matching record in lookup table.  If there is no
 *  match, NULL is returned.
 * 
 *
 **********************************************************************/
FBE_DEST_SCSI_ERR_LOOKUP* fbe_dest_scsi_error_lookup(fbe_char_t* scsi_error_str_ptr)
{
    FBE_DEST_SCSI_ERR_LOOKUP* scsi_lookup_tbl_ptr;

    if(scsi_error_str_ptr == NULL)
    {
        return NULL;
    }

    scsi_lookup_tbl_ptr = &dest_scsi_err_lookup_a[0];
    

    /* Check if the string provided in the argument is a match 
     * to any entry in the lookup table.
     */
    while(scsi_lookup_tbl_ptr->scsi_name != NULL)
    {
        if(!strcmp(scsi_lookup_tbl_ptr->scsi_name, scsi_error_str_ptr))
        {
            break;
        }
        scsi_lookup_tbl_ptr++;
    }            
   
   /* Return the value. The values could be either a hex value or NULL.
    */ 
    if (scsi_lookup_tbl_ptr->scsi_name == NULL)
    {
        return NULL;
    }

    return scsi_lookup_tbl_ptr;    
}
/*****************************************
 * end dest_scsi_error_lookup
 *****************************************/

/**********************************************************************
 * dest_scsi_opcode_lookup() 
 **********************************************************************
 *  @brief:
 *  This function is used to provide lookup into the scsi opcodes.
 *  For given SCSI opcode in string format, provided by the user,
 *  this function returns the hex value as defined in the scsi.h file 
 * 
 * @param
 *  scsi_opcode_ptr,    [I] -  scsi_opcode in string format. 
 *
 * 
 *  @return
 *  Hex value of the corrsponding string definition. If no match is
 *  found, DEST_ANY_RANGE is returned (0xffffffff).
 *
 *
 **********************************************************************/
fbe_u32_t fbe_dest_scsi_opcode_lookup(fbe_char_t* scsi_opcode_ptr)
{
    FBE_DEST_SCSI_OPCODE_LOOKUP* scsi_lookup_tbl_ptr; 

    /* Get a pointer to the SCSI Opcode lookup table.
     */
    scsi_lookup_tbl_ptr = &fbe_dest_scsi_opcode_lookup_a[0];
    
    /* Check if the passed string value is not NULL. If it is NULL, 
     * strcmp functions in this routine will panic. So we will return 
     * here.
     */
    if(scsi_opcode_ptr == NULL)
    {
        return DEST_INVALID_VALUE;
    }
     
    /* convert user opcode to uppercase, since opcode in lookup table is uppercase*/
    fbe_dest_str_to_upper(scsi_opcode_ptr, scsi_opcode_ptr, DEST_MAX_STR_LEN);
    
    /* Check the SCSI opcode lookup table for a match with the 
     * opcode string provided by the user. Also, check the shortcut 
     * since that could be an entry too.
     */
    while(scsi_lookup_tbl_ptr->scsi_name != NULL)
    {
        if( !strcmp(scsi_lookup_tbl_ptr->scsi_name, scsi_opcode_ptr) ||
            !strcmp(scsi_lookup_tbl_ptr->scsi_shortcut, scsi_opcode_ptr))
        {
            return scsi_lookup_tbl_ptr->scsi_value;
        }
        scsi_lookup_tbl_ptr++;
    }

    /* If there is no match, must be a numerical value.
     */
    return fbe_dest_strtoint(scsi_opcode_ptr);    
} 
/*****************************************
 * end dest_scsi_opcode_lookup
 *****************************************/

/**********************************************************************
 * dest_scsi_opcode_lookup_fbe() 
 **********************************************************************
 *  @brief:
 *  This function is used to provide lookup into the scsi opcodes.
 *  For given SCSI opcode in string format, provided by the user,
 *  this function returns the hex value as defined in the scsi.h file 
 * 
 * @param
 *  scsi_opcode_ptr,    [I] -  scsi_opcode in string format. 
 *  command,            [I] -  scsi opcode in array. 
 * 
 * 
 *  @return
 *  NONE.
 *
 *
 **********************************************************************/
void dest_scsi_opcode_lookup_fbe(fbe_char_t* scsi_opcode_ptr, fbe_u8_t *command, fbe_u32_t *count)
{
    FBE_DEST_SCSI_OPCODE_LOOKUP* scsi_lookup_tbl_ptr;
    fbe_u32_t i = 0;
    fbe_u32_t oc;

    *count = 0;

    /* Get a pointer to the SCSI Opcode lookup table.
     */
    scsi_lookup_tbl_ptr = &fbe_dest_scsi_opcode_lookup_a[0];
        
    /* Check if the passed string value is not NULL. If it is NULL, 
     * strcmp functions in this routine will panic. So we will return 
     * here.
     */
    if(scsi_opcode_ptr == NULL)
    {
        return;
    }
    

    /* before checking table, the user may have selcted a numerical opcode value. If so
       add this to command array and skip table lookup. */
    i = 0;
    oc = fbe_dest_strtoint(scsi_opcode_ptr);

    if (oc == DEST_INVALID_VALUE)   // not a numerical value.  look it up in table.
    {                
        if (!strcmp(scsi_opcode_ptr, DEST_ANY_STR))
        {
            command[0] = 0xFF;   // First entry of 0xFF indicates any opcode;
            (*count)++;
        }
        else
        {
            /* Check the SCSI opcode lookup table for a match with the 
             * opcode string provided by the user. Also, check the shortcut 
             * since that could be an entry too.
             */
            while((scsi_lookup_tbl_ptr->scsi_name != NULL) && (*count < 11))
            {
                if( !strcmp(scsi_lookup_tbl_ptr->scsi_name, scsi_opcode_ptr) ||
                    !strcmp(scsi_lookup_tbl_ptr->scsi_shortcut, scsi_opcode_ptr))
                {
                    command[*count] = scsi_lookup_tbl_ptr->scsi_value;
                    (*count)++;
                }
                scsi_lookup_tbl_ptr++;
            }
        }

    }
    else  // numerical value.  
    {
        if (oc > 0xFF)
        {
            fbe_cli_printf("DEST: %s invalid opcode 0x%x\n",__FUNCTION__,oc);
            return;
        }      

        command[*count] = (fbe_u8_t) oc;
        (*count)++;
    }

    return;    
} 
/*****************************************
 * end dest_scsi_opcode_lookup_fbe
 *****************************************/
/**********************************************************************
 * dest_port_error_lookup_fbe
 **********************************************************************
 *  @brief:
 *  This function looks up port status with a scsi error. 
 *  
 *  
 * @param
 *  scsi_name, [I] - Pointer to scsi name. 
 *
 *  @return
 *  Port status.
 * 
 *
 ***********************************************************************/
fbe_u32_t dest_port_error_lookup_fbe(fbe_char_t* scsi_name)
{
    FBE_DEST_PORT_ERR_LOOKUP * port_lookup_tbl_ptr;

    if(scsi_name == NULL)
    {
        return DEST_INVALID_VALUE;
    }
  
    port_lookup_tbl_ptr = &fbe_dest_scsi_port_err_lookup_a[0];    

    /* Check if the string provided in the argument is a match 
     * to any entry in the lookup table.
     */
    while(port_lookup_tbl_ptr->scsi_name != NULL)
    {
        if(!strcmp(port_lookup_tbl_ptr->scsi_name, scsi_name))
        {
            return port_lookup_tbl_ptr->scsi_value;
        }
        port_lookup_tbl_ptr++;
    }            
   
    /* port error might be provided as a numerical value, so convert it. If it's not a valid value then dest_strtoint
       will return an error.
       */       
    return fbe_dest_strtoint(scsi_name);
}
/**************************************
 * end of dest_port_error_lookup_fbe
 *************************************/

/**********************************************************************
 * dest_port_error_lookup_fbe
 **********************************************************************
 *  @brief:
 *  This function looks up scsi error with a port status
 * 
 *  
 * @param
 *  port_status, [I] - Pointer to scsi name. 
 *
 *  @return
 *  Scsi error.
 * 
 *
 ***********************************************************************/
fbe_char_t* dest_port_status_lookup_fbe(fbe_u32_t port_status)
{  
   FBE_DEST_PORT_ERR_LOOKUP * port_lookup_tbl_ptr;
   
   port_lookup_tbl_ptr = &fbe_dest_scsi_port_err_lookup_a[0];  
   
   /* Check if the string provided in the argument is a match 
     * to any entry in the lookup table.
     */
    while(port_lookup_tbl_ptr->scsi_name != NULL)
    {
        if(port_lookup_tbl_ptr->scsi_value == port_status)
        {
            return port_lookup_tbl_ptr->scsi_name;
        }
        port_lookup_tbl_ptr++;
    }  
    return DEST_ANY_STR;
}
/**************************************
 * end of dest_port_error_lookup_fbe
 *************************************/

/**********************************************************************
 * fbe_dest_strtoint() 
 **********************************************************************
 *  @brief:
 *  This is a quick implementation of converting a given string to
 *  the int. This function identifies between a hex value or a int. 
 * 
 *  
 * @param
 *  dest_ptr,   [O] - Array of characters with all upper case.
 *  src_ptr,    [I] - Array of characters that need to be converted 
 *                  to uppercase.
 *
 *  @return
 *  The string with all uppercase characters.
 * 
 ***********************************************************************/
fbe_u32_t fbe_dest_strtoint(fbe_u8_t * src_str)
{
    fbe_char_t temp_str[DEST_MAX_STR_LEN];
    fbe_u8_t * temp_str_ptr = 0;
    fbe_u8_t * hex_str_ptr = 0;
    fbe_u32_t intval, src_len;    
    
    if(src_str == NULL)
    {
        return DEST_INVALID_VALUE;
    }

    fbe_dest_str_to_upper(temp_str, src_str, sizeof(temp_str));   // covert 0x to upper so I don't have to check both.

    /* remove any leading spaces */
    temp_str_ptr = temp_str;
    while(*temp_str_ptr == ' ')
    {
        temp_str_ptr++;
    }


    hex_str_ptr = strstr(temp_str_ptr, "0X");   

    if(hex_str_ptr == NULL)
    {

        /* Make sure there are no alpha characters. 
         */
        src_len = (fbe_u32_t)strlen(temp_str_ptr);
        while(src_len)
        {
            src_len--;
            /* 48 is ASCII notation for 0 and 57 is for ASCII notation for
             * 9.
             */
            if( (temp_str_ptr[src_len] < 48 ) || (temp_str_ptr[src_len] > 57))
            {
                return DEST_INVALID_VALUE;
            }
        }
        /* This is a decimal value.
         */
        intval = atoi(temp_str_ptr);
    }
    else
    {
        /* This is a hex value.
         */
        intval = fbe_atoh(hex_str_ptr+2);
    }
    return intval;
}
        
/**************************
 * end of fbe_dest_strtoint
 *************************/
/**********************************************************************
 * fbe_dest_str_to_upper()
 **********************************************************************
 * @brief:
 *  This is a quick implementation of converting a given string to
 *  the uppercase. 
 * 
 *  
 * @param:
 *  dest_ptr,   [O] - Array of characters with all upper case.
 *  src_ptr,    [I] - Array of characters that need to be converted 
 *                  to uppercase.
 *
 * @return:
 *  The string with all uppercase characters.
 * 
 *
 ***********************************************************************/
fbe_u8_t* fbe_dest_str_to_upper(fbe_u8_t *dest_ptr, fbe_u8_t *src_ptr, fbe_u8_t dest_len)
{
    fbe_u8_t i = 0;
    while(*src_ptr != '\0' && i < dest_len-1)
    {
        *dest_ptr = toupper(*src_ptr);        
        dest_ptr++;
        src_ptr++;
         i++;
    } 
    *dest_ptr =  '\0';

    return dest_ptr;
}
/**************************
 * end of fbe_dest_str_to_upper
 *************************/
/**********************************************************************
 * dest_strtoint64() 
 **********************************************************************
 *  @brief:
 *  This is a quick implementation of converting a given string to
 *  the int. This function identifies between a hex value or a int. 
 *  
 *  
 * @param
 *  dest_ptr,   [O] - Array of characters with all upper case.
 *  src_ptr,    [I] - Array of characters that need to be converted 
 *                  to uppercase.
 *
 *  @return
 *  The string with all uppercase characters.
 * 
 *
 ***********************************************************************/
fbe_u64_t fbe_dest_strtoint64(fbe_u8_t * src_str)
{
    fbe_u8_t * temp_str_ptr = 0;
    fbe_u8_t * hex_str_ptr = 0;
    fbe_u64_t intval, src_len;    
    
    if(src_str == NULL)
    {
        return DEST_INVALID_VALUE_64;
    }

    /* remove any leading spaces */
    temp_str_ptr = src_str;
    while(*temp_str_ptr == ' ')
    {
        temp_str_ptr++;
    }

    hex_str_ptr = strstr(temp_str_ptr, "0x");

    if(hex_str_ptr == NULL)
    {
        /* Make sure there are no alpha characters. 
         */
        src_len = (fbe_u32_t)strlen(temp_str_ptr);
        while(src_len)
        {
            src_len--;
            /* 48 is ASCII notation for 0 and 57 is for ASCII notation for
             * 9.
             */
            if( (temp_str_ptr[src_len] < 48 ) || (temp_str_ptr[src_len] > 57))
            {
                return DEST_INVALID_VALUE_64;
            }
        }
        /* This is a decimal value.
         */
        intval = atoi(temp_str_ptr);
    }
    else
    {
        /* This is a hex value.
         */
        intval = fbe_atoh(hex_str_ptr+2);
    }
    return intval;
}
        
/**************************
 * end of dest_strtoint64
 *************************/

/****************************************************************
 * @fn fbe_dest_shift_args
 ****************************************************************
 *  @brief This function shifts args passed to it
 *
 *  @param: argv, argc
 *
 *  @return: void
 * 
 *****************************************************************/
void fbe_dest_shift_args(fbe_char_t ***argv, fbe_u32_t *argc)
{
    (*argv)++;
    (*argc)--;
}
/*****************************************
 * end fbe_dest_shift_args
 *****************************************/

/****************************************************************
 * @fn fbe_dest_args
 ****************************************************************
 *  @brief This function provides interface for non-interactive mode
 *  to enter fields of an error record. 
 *
 *  @param: argv, argc, err_record_ptr, status
 *
 *  @return: void
 * 
 *****************************************************************/
void fbe_dest_args(fbe_u32_t argc, fbe_char_t* argv[], fbe_dest_error_record_t * err_record_ptr, FBE_DEST_ERROR_INFO *status)
{
    fbe_char_t buff[DEST_MAX_STR_LEN];
    fbe_job_service_bes_t bes;
    fbe_u32_t error_val;
    fbe_bool_t required_drive_arg_set = FALSE;   
    *status = DEST_SUCCESS;    
    
    while (argc > 0)
    {           
        if (!strcmp(argv[0],"-d"))
        {
            
            if (argc > 1)
            {                   
                fbe_dest_shift_args(&argv,&argc);                                         
                if( fbe_cli_convert_diskname_to_bed(argv[0], &bes) != FBE_STATUS_OK)
                {
                    fbe_cli_printf("That is an invalid entry. Please try again\n"); 
                    break;
                }               
                
                /*Find the object id of a fru*/             
                if (FBE_STATUS_OK !=  fbe_api_get_physical_drive_object_id_by_location(bes.bus, bes.enclosure, bes.slot, &err_record_ptr->object_id))                
                { 
                    fbe_cli_printf("%s:\tDEST: Error occured while finding the Object ID for drive %d_%d_%d.\n",
                                   __FUNCTION__, bes.bus, bes.enclosure, bes.slot);
                    break;                    
                }                   
                required_drive_arg_set = TRUE;            
            }
            else
            {
                *status = DEST_CMD_USAGE_ERROR;
                break;
            }
        }
        else if(!strcmp(argv[0],"-lr"))
        {
            if (argc > 2)
            {
                fbe_dest_shift_args(&argv,&argc);            
                err_record_ptr->lba_start = fbe_dest_strtoint64(argv[0]);            
                fbe_dest_shift_args(&argv,&argc);
                err_record_ptr->lba_end = fbe_dest_strtoint64(argv[0]);
                *status = fbe_cli_dest_validate_assign_lba_range(err_record_ptr, err_record_ptr->object_id);
                if (*status != DEST_SUCCESS)
                {
                    fbe_cli_printf("DEST: ADD failed. Wrong LBA.\n");
                    break;
                }
            }
            else
            {
                *status = DEST_CMD_USAGE_ERROR;
                break;
            }
        }
        else if (!strcmp(argv[0],"-oc"))  // op-code          
        {
            if  (strcmp(err_record_ptr->opcode,"STARTSTOP"))   
            {
                if (argc > 1)
                {
                    fbe_dest_shift_args(&argv,&argc);
                    fbe_dest_str_to_upper(err_record_ptr->opcode, argv[0], sizeof(err_record_ptr->opcode)); 
                    dest_scsi_opcode_lookup_fbe(err_record_ptr->opcode, 
                                                err_record_ptr->dest_error.dest_scsi_error.scsi_command,
                                                &err_record_ptr->dest_error.dest_scsi_error.scsi_command_count); 
                }
                else
                {
                    *status = DEST_CMD_USAGE_ERROR;
                    break;
                }
            }
        }
        else if (!strcmp(argv[0],"-num"))  // num insertions
        {
            if (argc > 1)
            {
                fbe_dest_shift_args(&argv,&argc);
                err_record_ptr->num_of_times_to_insert = fbe_dest_strtoint(argv[0]);
            }
            else
            {
                *status = DEST_CMD_USAGE_ERROR;
                break;
            }
        }
        else if (err_record_ptr->dest_error_type == FBE_DEST_ERROR_TYPE_GLITCH)
        {            
            if (!strcmp(argv[0],"-time")) //glitch time
            {
                if (argc > 1)
                {
                    fbe_dest_shift_args(&argv,&argc);
                    err_record_ptr->glitch_time = fbe_dest_strtoint(argv[0]);;
                }
                else
                {
                    *status = DEST_CMD_USAGE_ERROR;
                    break;
                }
            }
        }
        else if(!strcmp(argv[0],"-et"))
        {
            if(err_record_ptr->dest_error_type != FBE_DEST_ERROR_TYPE_GLITCH || err_record_ptr->dest_error_type != FBE_DEST_ERROR_TYPE_FAIL)
            {
                if (argc > 1)
                {
                    fbe_dest_shift_args(&argv,&argc);
                    if (!strcmp(argv[0],"none"))
                    {
                        err_record_ptr->dest_error_type = FBE_DEST_ERROR_TYPE_NONE;
                    }
                    else if (!strcmp(argv[0],"scsi"))
                    {
                        err_record_ptr->dest_error_type = FBE_DEST_ERROR_TYPE_SCSI;                   
                        err_record_ptr->dest_error.dest_scsi_error.scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION;
                    }
                    else if (!strcmp(argv[0],"port"))
                    {
                        err_record_ptr->dest_error_type = FBE_DEST_ERROR_TYPE_PORT;
                    }
                    else //invalid arg
                    {
                        *status = DEST_FAILURE;
                        break;
                    }
                }
                else
                {
                    *status = DEST_CMD_USAGE_ERROR;
                    break;
                }
            }
        }
        else if (!strcmp(argv[0],"-serr"))  // if this option selected user should not select -sk, -ilba or -def
        {   
            FBE_DEST_SCSI_ERR_LOOKUP* lookup_rec_p;
            if (argc > 1)
            {
                fbe_dest_shift_args(&argv,&argc);
                fbe_dest_str_to_upper(buff,argv[0], sizeof(buff));
               
                lookup_rec_p = fbe_dest_scsi_error_lookup(buff);
                if (lookup_rec_p == NULL) 
                {
                    fbe_cli_printf("DEST: Invalid hex value or scsi error name. Please try again.\n"); 
                    *status = DEST_FAILURE;
                    break;
                }
                else  // user entered scsi error status name
                {
                    /* convert scsi name to error settings*/
                    strncpy(buff, lookup_rec_p->error_code, DEST_MAX_STR_LEN);
                    err_record_ptr->valid_lba = lookup_rec_p->valid_lba;                       
                }                   
                    
                dest_scsi_opcode_lookup_fbe(err_record_ptr->opcode, 
                                            err_record_ptr->dest_error.dest_scsi_error.scsi_command,
                                            &err_record_ptr->dest_error.dest_scsi_error.scsi_command_count); 
                
                error_val = fbe_atoh(buff);         
                err_record_ptr->dest_error.dest_scsi_error.scsi_sense_key = (error_val >> 16) & 0xFF;
                err_record_ptr->dest_error.dest_scsi_error.scsi_additional_sense_code = (error_val >> 8) & 0xFF;
                err_record_ptr->dest_error.dest_scsi_error.scsi_additional_sense_code_qualifier = error_val & 0xFF;                 
                err_record_ptr->dest_error.dest_scsi_error.port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
                   
            }
            else
            {
                *status = DEST_CMD_USAGE_ERROR;
                break;
            }            
        }
        else if (!strcmp(argv[0],"-sk"))  
        {   
            fbe_char_t *temp_str_ptr;
            if (argc > 1)
            {
                fbe_dest_shift_args(&argv,&argc);
                fbe_dest_str_to_upper(buff,argv[0], sizeof(buff));  
                temp_str_ptr = buff;  
                if (!strncmp(buff,"0X",2)) // strip 0x from value
                {
                    temp_str_ptr = buff + 2;
                }
                dest_scsi_opcode_lookup_fbe(err_record_ptr->opcode, 
                                            err_record_ptr->dest_error.dest_scsi_error.scsi_command,
                                            &err_record_ptr->dest_error.dest_scsi_error.scsi_command_count); 
                    
                error_val = fbe_atoh(temp_str_ptr);         
                err_record_ptr->dest_error.dest_scsi_error.scsi_sense_key = (error_val >> 16) & 0xFF;
                err_record_ptr->dest_error.dest_scsi_error.scsi_additional_sense_code = (error_val >> 8) & 0xFF;
                err_record_ptr->dest_error.dest_scsi_error.scsi_additional_sense_code_qualifier = error_val & 0xFF;                 
                err_record_ptr->dest_error.dest_scsi_error.port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
            }
            else
            {
                *status = DEST_CMD_USAGE_ERROR;
                break;
            }            
        }           
        else if (!strcmp(argv[0],"-ilba")) // invalid lba bit
        {
            err_record_ptr->valid_lba = 0;
        }
        else if (!strcmp(argv[0],"-def"))  // deferred bit
        {
            err_record_ptr->deferred = 1;
        }   
        else if (!strcmp(argv[0],"-perr"))
        {                
            fbe_u32_t port_error = 0;
            fbe_char_t error_uppercase[DEST_MAX_STR_LEN];
            if (argc > 1)
            {
                fbe_dest_shift_args(&argv,&argc);
                fbe_dest_str_to_upper(error_uppercase, argv[0], sizeof(error_uppercase));          
                    
                /* search table for a match */
                port_error = dest_port_error_lookup_fbe(error_uppercase); 
                if (port_error == DEST_INVALID_VALUE)
                {                    
                    /* still not valid */
                    dest_printf("DEST: Error is not recognized. Record not created in DEST package.\n");            
                }
                dest_scsi_opcode_lookup_fbe(err_record_ptr->opcode, 
                                            err_record_ptr->dest_error.dest_scsi_error.scsi_command,
                                            &err_record_ptr->dest_error.dest_scsi_error.scsi_command_count); 
                    
                err_record_ptr->dest_error.dest_scsi_error.port_status = port_error;                     
            }
            else
            {
                *status = DEST_CMD_USAGE_ERROR;
                break;
            }
        }
        else if (!strcmp(argv[0],"-delay"))
        {
            if (argc > 1)
            {
                fbe_dest_shift_args(&argv,&argc);
                err_record_ptr->delay_io_msec = fbe_dest_strtoint(argv[0]);;
            }
            else
            {
                *status = DEST_CMD_USAGE_ERROR;
                break;
            }
        }
        else if (!strcmp(argv[0],"-freq"))  // frequency
        {
            if (argc > 1)
            {
                fbe_dest_shift_args(&argv,&argc);
                err_record_ptr->frequency = fbe_dest_strtoint(argv[0]);
            }
            else
            {
                *status = DEST_CMD_USAGE_ERROR;
                break;
            }        
        }
        else if (!strcmp(argv[0],"-rfreq"))  // random frequency
        {
            err_record_ptr->is_random_frequency = TRUE;
            if (argc > 1)
            {
                fbe_dest_shift_args(&argv,&argc);
                err_record_ptr->frequency = fbe_dest_strtoint(argv[0]);
            }
            else
            {
                *status = DEST_CMD_USAGE_ERROR;
                break;
            }  
        }
        else if (!strcmp(argv[0],"-react_gap"))  // reactivation gap
        {
            if (argc > 2)
            {
                fbe_dest_shift_args(&argv,&argc);
                if (!strcmp(argv[0],"time"))
                {
                    err_record_ptr->react_gap_type = FBE_DEST_REACT_GAP_TIME;
                    fbe_dest_shift_args(&argv,&argc);
                    err_record_ptr->react_gap_msecs = fbe_dest_strtoint(argv[0]);
                }
                else if (!strcmp(argv[0],"io_count"))
                {
                    err_record_ptr->react_gap_type = FBE_DEST_REACT_GAP_IO_COUNT;
                    fbe_dest_shift_args(&argv,&argc);
                    err_record_ptr->react_gap_io_count = fbe_dest_strtoint(argv[0]);
                }
                else
                {
                    err_record_ptr->react_gap_type = FBE_DEST_REACT_GAP_INVALID;
                }
            }
            else
            {
                *status = DEST_CMD_USAGE_ERROR;
                break;
            }
        }
        else if (!strcmp(argv[0],"-react_rgap"))  //random reactivation gap
        {
            if (argc > 2)
            {
                err_record_ptr->is_random_gap = TRUE;
                fbe_dest_shift_args(&argv,&argc);
                if (!strcmp(argv[0],"time"))
                {
                    err_record_ptr->react_gap_type = FBE_DEST_REACT_GAP_TIME;
                    fbe_dest_shift_args(&argv,&argc);
                    err_record_ptr->max_rand_msecs_to_reactivate = fbe_dest_strtoint(argv[0]);
                }
                else if (!strcmp(argv[0],"io_count"))
                {
                    err_record_ptr->react_gap_type = FBE_DEST_REACT_GAP_IO_COUNT;
                    fbe_dest_shift_args(&argv,&argc);
                    err_record_ptr->max_rand_react_gap_io_count = fbe_dest_strtoint(argv[0]);
                }
                else
                {
                    err_record_ptr->react_gap_type = FBE_DEST_REACT_GAP_INVALID;
                }     
            }
            else
            {
                *status = DEST_CMD_USAGE_ERROR;
                break;
            }
        }
        else if (!strcmp(argv[0],"-n_react")) //number reactivations
        {
            if (argc > 1)
            {
                fbe_dest_shift_args(&argv,&argc);
                err_record_ptr->num_of_times_to_reactivate = fbe_dest_strtoint(argv[0]);
            }
            else
            {
                *status = DEST_CMD_USAGE_ERROR;
                break;
            }            
        }
        else if (!strcmp(argv[0],"-n_rreact")) // random number reactivations
        {
            if (argc > 1)
            {
                err_record_ptr->is_random_react_interations = TRUE;
                fbe_dest_shift_args(&argv,&argc);
                err_record_ptr->num_of_times_to_reactivate = fbe_dest_strtoint(argv[0]);
             }
             else
             {
                *status = DEST_CMD_USAGE_ERROR;
                break;
             }            
        }      
        
        else  //invalid arg  
        {
            *status = DEST_FAILURE;
            break;
        }
        
        fbe_dest_shift_args(&argv,&argc);
    }
    
    if (!required_drive_arg_set)
    {
        fbe_cli_printf ("DEST: Error: Required drive argument not set\n");    
        return;
    }
  
}

/*!*******************************************************************
 * fbe_cli_dest_exit_from_command()
 *********************************************************************
 * @brief       Function to decide whether to exit from command.
 *
 * @param      command_str - command string
 *
 * @return      fbe_bool_t  true for exit, else false
 * 
 * 
 *********************************************************************/
fbe_bool_t fbe_cli_dest_exit_from_command(fbe_char_t *command_str)
{
    if ((NULL != command_str) && (!strcmp(command_str, "quit") || !strcmp(command_str, "q")))
    {
        return FBE_TRUE;
    }   
    else
    {
        return FBE_FALSE;
    }
}
/****************************
* fbe_cli_dest_exit_from_command()
****************************/

/*****************************************
 * end fbe_dest_args
 *****************************************/
