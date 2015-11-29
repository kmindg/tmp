#ifndef __FBE_CLI_DEST_SERVICE_H__
#define __FBE_CLI_DEST_SERVICE_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_dest.h
 ***************************************************************************
 *
 * @brief
 *  This file contains DEST command cli definitions.
 *
 * @version
 *
 * History:
 *  05/01/2012  Created. kothal
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_cli.h"
#include "fbe/fbe_dest.h"
#include "fbe_dest_private.h"

/*********** Defines for DEST *******************/
#define FBE_FRU_STR_LEN             10         /* Disk format 00_00_000 */
#define FBE_DEST_MAX_FILENAME_LEN   128
#define FBE_DEST_MAX_PATHNAME_LEN   256
#define FBE_DEST_MAX_BUFFER_LEN     2048
#define FBE_DEST_MAX_STR_LEN        64
#define FBE_DEST_ANY_RANGE          0xFFFFFFFF
#define GET_START_LBA               0        /* This is used in switch case as case value to get start value of lba from user */
#define GET_END_LBA                 1        /* This is used in switch case as case value to get end value of lba from user */
#define FBE_DEST_INVALID_VALUE  0xffffffff
#define FBE_DEST_MAX_VALUE   FBE_DEST_INVALID_VALUE-1

/* Default values for LBA Start and LBA End are the beginning 
 * of the user space and the end of user space.  
 */

#define FBE_DEST_DEFAULT_LBA_START(x)   fbe_cli_dest_start_of_user_space(x)
#define FBE_DEST_DEFAULT_LBA_END(x)     fbe_cli_dest_end_of_user_space(x)
/**********************************************************************
 * FBE_DEST_SEARCH_PARAMS_TYPE
 **********************************************************************
 * The following is the type definition of search parameters table. 
 * These values are used to search the error table for any existing
 * error rules. 
 **********************************************************************/

typedef struct FBE_DEST_SEARCH_PARAMS_TYPE
{
    fbe_object_id_t objid;    /* FRU Number in B_E_D format. */
    fbe_lba_t lba_start;    /* LBA Start. */
    fbe_lba_t lba_end;      /* LBA End. */  
    fbe_dest_error_type_t error_type; 
}FBE_DEST_SEARCH_PARAMS;

/**********************************************************************
 * DEST_ERROR_SCENARIO
 **********************************************************************
 *  Enum containing the various builtin error scenarios, which are composed
 *  of a set of error records.
 **********************************************************************/
typedef enum FBE_DEST_ERROR_SCENARIO_ENUM
{
    FBE_DEST_SCENARIO_INVALID = 0,
    FBE_DEST_SCENARIO_SLOW_DRIVE,
    FBE_DEST_SCENARIO_RANDOM_MEDIA_ERROR,
    FBE_DEST_SCENARIO_PROACTIVE_SPARE,    
}FBE_DEST_ERROR_SCENARIO_TYPE;

typedef struct FBE_DEST_SCENARIO_ELEMENT_S
{
   fbe_char_t * name;
   FBE_DEST_ERROR_SCENARIO_TYPE value;
}FBE_DEST_SCENARIO_ELEMENT;

#define DEST_USAGE "\n\
dest <operation> <parameters>\n\n\
Commands:\n\
dest -add [add_args]   : Add a new record. If no args then interactive mode is used.\n\
dest -start            : Start error injection. \n\
dest -stop             : Stop error injection. \n\
dest -display          : Display Error Records. \n\
dest -state            : Displays the state of Dest Service. \n\
dest -search           : Search for a error record. \n\
dest -delete           : Delete an error record. \n\
dest -load [filename]  : Read the XML configuration file and add the record. \n\
dest -save [filename]  : Save the error record into an XML file. \n\
dest -clean            : Delete all error records in the queue. \n\
dest -fail             : Fails a drive. \n\
dest -glitch           : Glitches a drive for the specified time. \n\
dest -list_port_errors : List port errors that can be injected. \n\
dest -list_scsi_errors : List scsi errors that can be injected. \n\
dest -list_opcodes     : List opcodes that can be injected. \n\
dest -list_scenarios   : List built-in scenarios that can be injected. \n\
dest -add_scenario     : Add a built-in scenario. Use -list_scenarios for list of values.\n\
dest -add_spinup       : Adds errors during spinup/spindown. If no args then interactive mode is used. \n\n\
[add_args]\n\
-d <b_e_s>             : *Required*  Drive to inject.  bus_enclsoure_slot format. \n\
-oc <value>            : Op-code string or numerical representation of opcode. \n\
                         Use -list_opcodes for list of values. Defaults to ANY\n\
-et <type>             : *Required*  Error Type = none|scsi|port. \n\
'scsi' options:  {-serr <v> | {-sk <v> [-ilba] [-def]}} \n\
-serr <scsi_error_name>: For values use -list_scsi_errors.  \n\
                         If scsi_error_name selected, then don't provide -sk, -ilba or -def. \n\
-sk <sense_key_code>   : *Required for 'scsi'*  Sense Key code must be in hex format.  i.e. 0x031100.\n\
-ilba               : set Invalid LBA bit in sense data. Default is valid LBA.\n\
-def                : set deferred bit in sense data. Default is current.\n\
'port' options:\n\
-perr <value>       : *Required for 'port'*  Port Error, string or numerical representation.  \n\
                      Use -list_port_errors for string listing.\n\
-lr <start> <end>   : LBA Range [start..end] inclusive. Defaults to user space start/end LBA\n\
-delay <msec>       : Delay IO in milliseconds.  Defaults to 0. \n\
-num <value>        : Number of errors to insert. Defaults to 1.\n\
-freq <N>           : Frequency of insertion given as 1 insertion per N IOs. Defaults to 1 (every IO).\n\
-rfreq <N>          : Set insertions to be random, where probability of insertion is 1/N \n\
-time <seconds>     : Time for which the drive glitches, only specified for glitch option. \n\
\n\
Select zero or one of the following.  Default is no gap. \n\
-react_gap <type> <value> : type= time|io_count. Add gap between reactivations, which can be time based or \n\
                            io based. If 'time' value is msec.  If 'io_count' value is num IOs to skip. Defaults \n\
                            to no gap.\n\
-react_rgap <type> <value>: Same as previous option, except that time or io_count will be randomly \n\
selected from 1..N\n\
\n\
Select zero or one of the following.  Default is no reactivation \n\
-n_react <N>        : Number of Reactivations.  Defaults to 0. \n\
-n_rreact <N>       : Random Number of Reactivations will be randomly selected from 1..N\n\n\
Example(s):\n\
    dest -add -d 0_0_5 -oc any -et scsi -sk 0x031400    \n\
    dest -fail -d 0_0_7 \n\
    dest -glitch -d 0_0_9 -time 10 \n\
    dest -add_scenario random_media_error -d 0_0_6\n\
    dest -add_spinup -d 0_0_8 -et scsi -sk 0x031100\n\
"

/**************************************
 * DEST Error definitions
 *************************************/
typedef enum DEST_ERROR_INFO_TYPE
{
    DEST_SUCCESS = 0,
    
    /* Generic failure error. 
     */
    DEST_FAILURE,
    
    /* Quitting dest command. 
     */
    DEST_QUIT,

    /* This error is used to denote error in parsing XML file.
     */
    DEST_XML_PARSE_ERROR,

    /* This is DEST's error return for failure on reading a file.
     */
    DEST_FILE_READ_ERROR, 

    /* DEST returns the following when there is a error in writing to 
     * file.
     */
    DEST_FILE_WRITE_ERROR,  

    /* When there is a memory allocation error, the following error
     * is returned. 
     */
    DEST_MEMORY_ALLOCATION_ERROR,    

    /* The table is not empty but there is no record for the
     * specific FRU is the following error.
     */
    DEST_NO_FRU_RECORD,   

    /* There is an error record for the given FRU. But there are 
     * no rules. This is usually never the case since a FRU record
     * is created with at least one error rule. For deletion,
     * if the FRU record has only a single error rule, it cannot
     * be deleted.
     */
    DEST_NO_FRU_ERR_RULES, 

    /* To denote that the record already exists, following error
     * is returned.
     */
    DEST_RECORD_ALREADY_EXISTS,  

    /* If DEST command entered on the FCLI is not properly phrased, 
     * the following error is returned.
     */
    DEST_CMD_USAGE_ERROR,      

    /* If user entered an invalid parameter for the DEST record 
     * parameters, the following error is returned
     */
    DEST_NOT_VALID_ENTRY,

    /* Indicates DEST is already in progress
     */
    DEST_ALREADY_IN_PROGRESS,
    
}FBE_DEST_ERROR_INFO;

/* This is the root directory where the DEST configuration and log 
 * files are stored.
 */
#define FBE_EMC_ROOT EMCUTIL_BASE_EMC
#define FBE_DEST_DIRECTORY "dest_config"
#define FBE_DEST_CONFIG_FILE "config.xml"
#define FBE_DEST_XML_ERROR_RULE_TAG "ErrorRule"

/* This is the string defining the header tags.
 */
#define FBE_DEST_XML_HEADER_STRING "<?xml version=\"1.0\"?>\n<DESTConfig>"

/* This is the string defining the footer tags.
 */
#define FBE_DEST_XML_FOOTER_STRING "</DESTConfig>"

/**********************************************************************
 * XML_DATA_TYPE
 **********************************************************************
 * XML Data types are defined in the following enumeration types. The
 * following are the data tags used in the XML file and hence this
 * ENUM type will help decode the values during the XML parsing. 
 **********************************************************************/
typedef enum DEST_XML_TAG_TYPE
{
    fru=0,    
    lba_start,  /* LBA start location. */
    lba_end,    /* LBA end location. */
    opcode,     /* Opcode for which the error should be inserted. */
    error_type, /* Type of error*/
    error,      /* Error based on type. */
    valid_lba,  /* sense data valid field */
    deferred,   /* sense data error code field */
    num_of_times,           /* Number of times the error should be inserted. */
    frequency,              /* frequency of error injection per record */
    is_random_frequency,    /* Indicates if frequency is random */
    reactivation_gap_type,  /* Specifies if and how reactivation will be handled */
    reactivation_gap_msecs,         /* Number of milliseconds to skip before reactivating*/
    reactivation_gap_io_count,     /* Number of IO to skip before reactivating*/
    is_random_gap,          /* Use random gap*/
    max_rand_msecs_to_reactivate, /* Max msecs to wait before reactivating, when using random gap */
    max_rand_react_gap_io_count, /* Max IO to skip before recativating, when using random gap */
    is_random_reactivation,        /* Numer record recativations will be random */
    num_reactivations,         /* Number of times to reactivate the record.  */
    delay_io_msec,               /* How long to delay IO */    
}DEST_XML_TAG_TYPE;

/**********************************************************************
 * FBE_DEST_XML_PARSING_TYPE
 **********************************************************************
 * This is a global structure whose attributes are used by the DEST 
 * module for XML parsing. 
 **********************************************************************/
typedef struct FBE_DEST_XML_PARSING_TYPE
{
    /* Depth of any given XML element.
     */
    fbe_u32_t depth;
    
    /* Data type that is being parsed eg: Bus, Slot, Encl etc.
     */
    fbe_u32_t data_type;

    /* This flag is set by start_element_parser to denote that
     * this data should be parsed by data_element_parser. In XML file, 
     * there are a bunch of tags and for some of the tags we really 
     * don't have to care about the data. This flag is used to denote if
     * the data is of our interest or not.
     */
    fbe_bool_t data_flag;

    /* During the parsing of the XML file, we need to keep track of which
     * tag are we currently processing.
     */
    DEST_XML_TAG_TYPE data_tag;
    
    /* Save the XML data in a variable. This will be written to the 
     * table only when the tag is ended.
     */
    fbe_char_t data[FBE_DEST_MAX_STR_LEN]; 
    
    /* Save the error record in this global pointer.
     */
    fbe_dest_error_record_t * err_record_ptr;
	
	fbe_u8_t fru[FBE_FRU_STR_LEN];
}FBE_DEST_XML_PARSING;

/**********************************************************************
    Utility Functions
**********************************************************************/
fbe_u32_t fbe_dest_strtoint(fbe_u8_t* src_str);
fbe_u64_t fbe_dest_strtoint64(fbe_u8_t *src_str);
fbe_u8_t* fbe_dest_str_to_upper(fbe_u8_t *dest_ptr, fbe_u8_t *src_ptr, fbe_u8_t dest_len);
void fbe_dest_shift_args(fbe_char_t ***argv, fbe_u32_t *argc);
void fbe_dest_args(fbe_u32_t, fbe_char_t**, fbe_dest_error_record_t *, FBE_DEST_ERROR_INFO *);
fbe_bool_t fbe_cli_dest_exit_from_command(fbe_char_t *command_str);
FBE_DEST_ERROR_INFO fbe_cli_dest_drive_physical_layout(fbe_object_id_t objid, fbe_lba_t * start_user_space, fbe_lba_t * end_user_space, fbe_lba_t * end_drive_space);

/**********************************************************************
 *   FUNCTION DECLARATION
**********************************************************************/
void fbe_cli_dest(int argc, fbe_char_t** argv);
fbe_status_t fbe_cli_dest_usr_interface(fbe_u32_t args, fbe_char_t* argv[]);
void fbe_cli_dest_usr_intf_init(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage);
void fbe_cli_dest_usr_intf_start(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage);
void fbe_cli_dest_usr_intf_stop(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage) ;
void fbe_cli_dest_usr_intf_display(fbe_u32_t argc, fbe_char_t* argv[], fbe_char_t* usage);
void fbe_cli_dest_usr_intf_add(fbe_u32_t argc, fbe_char_t* argv[], fbe_char_t* usage);
void fbe_cli_dest_usr_intf_load(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage);
void fbe_cli_dest_usr_intf_save(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage);
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_intf_add_interactive(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage);
void fbe_cli_dest_usr_intf_search(fbe_u32_t argc, fbe_char_t* argv[], fbe_char_t* usage);
void fbe_cli_dest_usr_intf_delete(fbe_u32_t argc, fbe_char_t* argv[], fbe_char_t* usage);
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_intf_delete_interactive(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage);
void fbe_cli_dest_usr_intf_clean(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage);
void fbe_cli_dest_usr_intf_count_change(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage);
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_intf_add_scenario(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage);
void fbe_cli_dest_usr_intf_list_scenarios(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage);
void fbe_cli_dest_usr_intf_list_port_errors(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage);
void fbe_cli_dest_usr_intf_list_scsi_errors(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage);
void fbe_cli_dest_usr_intf_list_opcodes(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage);
void fbe_cli_dest_usr_intf_state(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage);
void fbe_cli_dest_usr_intf_fail(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage);
void fbe_cli_dest_usr_intf_glitch(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage);
void fbe_cli_dest_usr_intf_add_spinup(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage);
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_intf_add_spinup_interactive(fbe_u32_t args, fbe_char_t* argv[], fbe_char_t* usage);

void fbe_cli_dest_err_record_initialize(fbe_dest_error_record_t* err_rec_ptr);
FBE_DEST_ERROR_INFO fbe_cli_dest_validate_assign_lba_range(fbe_dest_error_record_t* err_rec_ptr, fbe_object_id_t objid);
FBE_DEST_ERROR_INFO fbe_cli_dest_validate_lba_range(fbe_dest_error_record_t* err_rec_ptr);
fbe_u32_t fbe_dest_validate_new_record(fbe_dest_error_record_t *);

fbe_u32_t fbe_dest_display_error_record(fbe_dest_error_record_t *);
fbe_status_t fbe_dest_display_error_records(fbe_dest_record_handle_t *handle_p);

FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_bes_list(fbe_object_id_t * const frulist, fbe_u32_t * const frucount);
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_lba_range(fbe_lba_t *lba_range_ptr);
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_error(fbe_dest_error_record_t* err_rec_ptr);
fbe_dest_error_type_t fbe_cli_dest_usr_get_error_type(void);
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_scsi_error(fbe_dest_error_record_t* err_rec_ptr);
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_port_error(fbe_dest_error_record_t* err_rec_ptr);
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_delay(fbe_dest_error_record_t* err_rec_ptr);
fbe_char_t* fbe_cli_dest_usr_get_opcode(fbe_char_t* opcode_str);
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_insertion_frequency(fbe_dest_error_record_t* err_rec_ptr);
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_reactivation_settings(fbe_dest_error_record_t* err_rec_ptr);
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_num_reactivations(fbe_dest_error_record_t* err_rec_ptr);
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_reactivation_io_count(fbe_u32_t * const gap_io_count);
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_rand_reactivation_io_count(fbe_u32_t * const gap_io_count);
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_msecs_reactivate(fbe_u32_t * const mseconds_p);
FBE_DEST_ERROR_INFO fbe_cli_dest_usr_get_rand_msecs_reactivate(fbe_u32_t * const mseconds_p);
FBE_DEST_ERROR_SCENARIO_TYPE fbe_cli_dest_usr_get_error_scenario(void);

fbe_u32_t fbe_dest_scsi_opcode_lookup(fbe_char_t*);
FBE_DEST_SCSI_ERR_LOOKUP* fbe_dest_scsi_error_lookup(fbe_char_t*);
void dest_scsi_opcode_lookup_fbe(fbe_char_t* scsi_opcode_ptr, fbe_u8_t *command, fbe_u32_t *count);
fbe_u32_t dest_port_error_lookup_fbe(fbe_char_t* scsi_name);
fbe_char_t* dest_port_status_lookup_fbe(fbe_u32_t port_status);

/**********************************************************************
*                    XML PARSING  PROTOTYPES                       *
*********************************************************************/
FBE_DEST_ERROR_INFO fbe_dest_usr_intf_load(fbe_char_t *filename_p);
FBE_DEST_ERROR_INFO fbe_dest_usr_intf_save(fbe_char_t *filename_p); 
void fbe_cli_dest_parser_var_initialize(FBE_DEST_XML_PARSING* parsing_ptr);
void fbe_cli_dest_parse_xml_data_element( void *data, const fbe_char_t* element, fbe_s32_t len);
void fbe_cli_dest_parse_xml_end_element(void *data, const fbe_char_t* element);
void fbe_cli_dest_parse_xml_start_element(void *data, const fbe_char_t *element, const fbe_char_t **attr); 
FBE_DEST_ERROR_INFO fbe_cli_dest_read_and_parse_xml_file(fbe_char_t* pwFilename);

#endif /* end __FBE_CLI_DEST_SERVICE_H__ */
