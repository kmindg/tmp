#ifndef RAID_CALC_CLI_I_H
#define RAID_CALC_CLI_I_H
/**********************************************************************
 * Copyright (C) EMC Corporation 2006-2007
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 **********************************************************************/

/********************************************************************
 * RaidCalcCli_I.h
 ********************************************************************
 *
 * DESCRIPTION:
 *   This file contains the macro declaration, function prototypes 
 *   and global variables used by Interactive mode of 
 *   the CLI version of the Raid Calculator.
 *
 * Table of Contents:
 *
 * HISTORY:
 *   05-APR-07:  Created by Pradnya Patankar
 *
 *********************************************************************/

/*************************
 * INCLUDE FILES
 *************************/

#include "generic_types.h"
#include "fbe/fbe_raid_calc.h"
#include "RaidCalcCli.h"

/*******************************
 * MACRO DECLARATION
 *******************************/

/* Default values for the Raid Calculator.
 */

#define DEFAULT_ADDRESS_OFFSET 577536
#define DEFAULT_ARRAY_WIDTH 5
#define DEFAULT_ELEMENTS_PER_PARITY 1
#define DEFAULT_ALIGN_LBA 0

/* Size of the command buffer.
 */
#define CMD_BUF          260
/* Maximum value of argument counter.
 */
#define MAX_ARGC          100
/* It is used in the parse_cmd() to define 
 * the size of the token_p which gets the next token 
 * in the command string.
 */
#define TOKEN_SIZE        10
/* Size of the local string.
 */
#define LOCAL_STR          100
/* To check the space,formfeed and new lines in the command 
 * so that it is avoided while counting the arguments.
 */
CONST CHAR *raidCalcCli_whitespace_p = " \t\f\n";
/* It is the buffer used for the getting the command in the RaidCalcCli_I().
 */
CHAR command_buffer[CMD_BUF];

/*********************************
 ** set COMMAND DEFINITIONS     **
 *********************************/

#define SET_STR                "set"
#define SET_A_STR              "s"
#define SET_SUMMARY_STR        "Sets the Raid Calculator to the given values.\n" 
#define SET_USAGE              "set can be used in following two ways.\n\n"    \
"1.set -r -elsz -addr_offset -w "                                              \
"-default_elements_per_parity [-align_lba]\n\n"                                \
"2.set -default\n\n"                                                           \
"Description of the switches.\n\n"                                             \
"-r             : Raid type (R0 |R1| R1_0| R3| R5| R6| ID) \n"                 \
"-elsz          : Element size.\n"                                             \
"-addr_offset   : Address offset.\n"                                           \
"-w             : Array Width.\n"                                              \
"-default_elements_per_parity : Default elements per parity stripe.\n"         \
"-align_lba     : Alignment offset.\n"                                         \
"-default       : Set Raid calculator to the default values. \n"

/*********************************
 ** convert COMMAND DEFINITIONS **
 *********************************/

#define CONVERT_STR            "convert"
#define CONVERT_A_STR          "c"
#define CONVERT_SUMMARY_STR    "Converts the output of the Raid Calculator "\
                               "to hex/dec.\n"
#define CONVERT_USAGE          "\nRequired argument.\n"                    \
"-hex : Output format is hexadecimal.\n"                                   \
"Or\n"                                                                     \
"-dec : Output format is decimal\n"                                       
   

/*********************************
 ** display COMMAND DEFINITIONS **
 *********************************/

#define DISPLAY_STR            "display"
#define DISPLAY_A_STR          "d"
#define DISPLAY_SUMMARY_STR    "Displays the current setting "\
                               "of the raid calculator.\n" 
#define DISPLAY_USAGE          ""


/*********************************
 ** getlba COMMAND DEFINITIONS  **
 *********************************/

#define GETLBA_STR            "getlba"
#define GETLBA_A_STR          "l"
#define GETLBA_SUMMARY_STR    "Calculates the logical block address "\
                              "and parity position.\n" 
#define GETLBA_USAGE          "\nRequired arguments.\n"              \
"\n-pba : Physical disk address.\n"                                  \
"-dpos: Data position.\n"                                            

/*********************************
 ** getpba COMMAND DEFINITIONS  **
 *********************************/

#define GETPBA_STR            "getpba"
#define GETPBA_A_STR          "p"
#define GETPBA_SUMMARY_STR    "Calculates the physical block address data "  \
                              "and parity position.\n"  
#define GETPBA_USAGE          "\nRequired argument.\n"                       \
"\n-lba : Logical block address.\n"

/*********************************
 ** HELP COMMAND DEFINITIONS    **
 *********************************/

#define HELP_STR            "help"
#define HELP_A_STR          "h"
#define HELP_SUMMARY_STR    "Displays the available commands.\n"
#define HELP_USAGE                                                             \
"\nd/display - Display the current setting of the raid calculator.\n"          \
"c/convert - Convert the all the values\n"                                     \
"            (calculated in the previous operation) in the \n"                 \
"            Hexadecimal format.\n"                                            \
"            If the option is -dec then it will convert in \n"                 \
"            the decimal format.\n"                                            \
"l/getlba -  Calculate the logical block address, parity position.\n"          \
"p/getpba -  Calculate the physical disk address, parity position.\n"          \
"s/set -     Set the raid type, element size, address offset,\n"               \
"            array width and alignment offset for the logical to physical\n"   \
"            physical to logical calculation.\n"                               \
"h/help -    Display the available commands.\n"                                \
"q/exit -    Exit from the Raid Calculator CLI.\n"

/*********************************
 ** exit COMMAND DEFINITIONS    **
 *********************************/

#define EXIT_STR            "exit"
#define EXIT_A_STR          "q"
#define EXIT_SUMMARY_STR    "Exit from the Raid Calculator CLI.\n"                               
#define EXIT_USAGE          ""                                     \

/* Command table definition.
 * It is a structure used to keep the information about the commands
 * used in the interactive mode. It has the name of command, 
 * command abbreviation, function pointer to execute the corresponding 
 * command, help string and usage string for the command.  
 */

typedef struct raid_cli_cmd_tbl_s
{
    TEXT *name_p;               /* name of command */
    TEXT *name_a_p;             /* name abbreviation */
    void (*execute_routine) (UINT_32 argc,	/* fcn corresponding */
                             TEXT * argv[],	/* to command */
                             TEXT * usage_p);
    TEXT *help_p;               /* help string */
    TEXT *usage_p;              /* usage string */
}
RAID_CLI_CMD_TBL_S;


void raid_cli_print_usage(TEXT * cmd_p,
                          TEXT * usage_p);
void raid_cli_print_help(TEXT * cmd_p,
                         TEXT * cmd_a_p,
                         TEXT * help_p);
void raid_cli_x_set(UINT_32 argc,
                    TEXT * local_argv[],
                    TEXT * usage_p);
void  raid_cli_x_convert(UINT_32 argc,
                         TEXT * local_argv[],
                         TEXT * usage_p);
void raid_cli_x_display(UINT_32 argc,
                        TEXT * local_argv[],
                        TEXT * usage_p);
void raid_cli_x_getlba(UINT_32 argc,
                       TEXT * local_argv[],
                       TEXT * usage_p);
void raid_cli_x_getpba(UINT_32 argc,
                       TEXT * local_argv[],
                       TEXT * usage_p);
void raid_cli_x_exit(UINT_32 argc,
                     TEXT * local_argv[],
                     TEXT * usage_p);
void raid_cli_x_help(UINT_32 argc,
                     TEXT * local_argv[],
                     TEXT * usage_p);
UINT_32 parse_cmd(PCHAR command_buffer,PCHAR local_argv[]);
void RaidCalcCli_I(void);

/******************
* Variables
******************/
/* Variable used in the local functions to get the formatted value 
 * returned by the read_and_validate_input().
 */
ULONGLONG format_input;
CHAR output[OUTPUT_STR_SIZE];

extern CALC_FLAG calc_flag;
extern OUTPUT_FORMAT output_form;
extern BOOL array_wd_error;
extern PCHAR raid_input_string[RAID_T_STR_ARR_SIZE]; 
extern PCHAR raid_output_string[RAID_T_STR_ARR_SIZE];

/***************************************************
 * END RaidCalcCli_I.h
 ***************************************************/
#endif // RAID_CALC_CLI_I_H
