#ifndef __FBE_CLI_NEIT_SERVICE_H__
#define __FBE_CLI_NEIT_SERVICE_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_neit.h
 ***************************************************************************
 *
 * @brief
 *  This file contains NEIT command cli definitions.
 *
 * @version
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_cli.h"
#include "fbe/fbe_lun.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe_protocol_error_injection_service.h"


/*********** Defines for NEIT *******************/
#define SUCCESS 1
#define FBE_FRU_STR_LEN             10         /* Disk format 00_00_000 */
#define FBE_NEIT_MAX_STR_LEN        64
#define FBE_NEIT_ANY_RANGE          0xFFFFFFFF
#define FBE_NEIT_INVALID_SECONDS    CSX_CONST_U32(0xFFFFFFFF);
#define GET_START_LBA               0        /* This is used in switch case as case value to get start value of lba from user */
#define GET_END_LBA                 1        /* This is used in switch case as case value to get end value of lba from user */
/* This is the size of the buffer we use for reading the XML file.
 */
#define FBE_NEIT_MAX_FILENAME_LEN   128
#define FBE_NEIT_MAX_PATHNAME_LEN   256

/* Default values for LBA Start and LBA End are the beginning 
 * of the user space and the end of user space.  
 */
extern fbe_u32_t phy_to_log_fru[];

#define FBE_NEIT_DEFAULT_LBA_START(x)   fbe_cli_neit_what_is_fru_private_space(x)
#define FBE_NEIT_DEFAULT_LBA_END(x)     fbe_fru_capacity(x)

/* This is the value which is returned when user quits 
* neit command explicitely
*/
#define FBE_NEIT_QUIT -1
/**********************************************************************
 * FBE_NEIT_SEARCH_PARAMS_TYPE
 **********************************************************************
 * The following is the type definition of search parameters table. 
 * These values are used to search the error table for any existing
 * error rules. 
 **********************************************************************/

typedef struct FBE_NEIT_SEARCH_PARAMS_TYPE
{
    fbe_job_service_bes_t bes;    /* FRU Number in B_E_D format. */
    LBA_T lba_start;    /* LBA Start. */
    LBA_T lba_end;      /* LBA End. */
    fbe_protocol_error_injection_error_type_t error_type;
    BOOLEAN print_flag; /* To print the search result or not. */    
}FBE_NEIT_SEARCH_PARAMS;


#define LURG_NEIT_USAGE "\
neit operation <parameters>\n\
Commands:\n\
-start           -   Start the Error insertion.\n\
-stop            -   Stop the error insertion.\n\
-add             -   Add a new record in the interactive mode.\n\
-rem             -   Remove an existing record interactively.\n\
-rmobj          -   Remove ab object.\n\
-list            -   List the error records inserted.\n\
-quit or q       -   Quit neit command.\n\
\n\
-count_change frunum, error, count - Change the existing error record with given FRU\n\
                                    and error to have the count frequency.\n\
\n"

#define LURG_NEIT_COUNT_CHANGE_USAGE "\
neit -count_change <fru> <error> <count>\n\
\n\
Commands:\n\
-count_change frunum, error, count - Change the existing error record with given FRU\n\
                                    and error to have the count frequency.\n\
\n"


/**************************************
 * NEIT Error definitions
 *************************************/
typedef enum FBE_NEIT_ERROR_INFO_TYPE
{
    FBE_NEIT_SUCCESS = 1,       //FBE_SUCCESS
    
    /* Generic failure error. 
     */
    FBE_NEIT_FAILURE,
    
    /* This error is used to denote error in parsing XML file.
     */
    FBE_NEIT_XML_PARSE_ERROR,

    /* This is NEIT's error return for failure on reading a file.
     */
    FBE_NEIT_FILE_READ_ERROR, 

    /* NEIT returns the following when there is a error in writing to 
     * file.
     */
    FBE_NEIT_FILE_WRITE_ERROR,  

    /* When there is a memory allocation error, the following error
     * is returned. 
     */
    FBE_NEIT_MEMORY_ALLOCATION_ERROR, 

    /* When the Error Table list is empty, the following error is 
     * returned.
     */
    FBE_NEIT_TABLE_IS_EMPTY, 

    /* The table is not empty but there is no record for the
     * specific FRU is the following error.
     */
    FBE_NEIT_NO_FRU_RECORD,   

    /* There is an error record for the given FRU. But there are 
     * no rules. This is usually never the case since a FRU record
     * is created with at least one error rule. For deletion,
     * if the FRU record has only a single error rule, it cannot
     * be deleted.
     */
    FBE_NEIT_NO_FRU_ERR_RULES, 

    /* To denote that the record already exists, following error
     * is returned.
     */
    FBE_NEIT_RECORD_ALREADY_EXISTS,  

    /* If NEIT command entered on the FCLI is not properly phrased, 
     * the following error is returned.
     */
    FBE_NEIT_CMD_USAGE_ERROR,      

    /* If user entered an invalid parameter for the NEIT record 
     * parameters, the following error is returned
     */
    FBE_NEIT_NOT_VALID_ENTRY,
    
}FBE_NEIT_ERROR_INFO;

/*************************
 *   FUNCTION DECLARATION
 *************************/
void fbe_cli_neit(int argc, char** argv);
static fbe_status_t fbe_cli_neit_usr_intf_add(fbe_protocol_error_injection_error_record_t* record_p);
static fbe_status_t fbe_cli_neit_usr_intf_remove(fbe_protocol_error_injection_record_handle_t *handle_p, fbe_protocol_error_injection_error_record_t* record_p);
static fbe_status_t fbe_cli_neit_usr_intf_search(fbe_protocol_error_injection_record_handle_t handle_p, fbe_protocol_error_injection_error_record_t* record_p);
fbe_status_t fbe_cli_neit_usr_intf_count_change(fbe_u32_t args, char* argv[], fbe_protocol_error_injection_error_record_t* record_p);
fbe_bool_t fbe_cli_neit_error_count_change(fbe_job_service_bes_t bes, fbe_u8_t *err_type, fbe_u32_t num_of_times_to_insert);
VOID fbe_cli_neit_err_record_initialize(fbe_protocol_error_injection_error_record_t* record_p);
static void fbe_cli_neit_usr_get_lba(fbe_protocol_error_injection_error_record_t* record_p);
static fbe_status_t fbe_cli_neit_usr_get_lba_range(fbe_lba_t *lba_range_ptr);
static fbe_status_t neit_usr_get_secs_reactivate(fbe_u32_t * seconds_p,fbe_u32_t * numofresets_p);
static fbe_status_t fbe_cli_neit_usr_get_times_to_insert_err(fbe_protocol_error_injection_error_record_t* record_p);
fbe_u32_t fbe_cli_neit_validate_lba_range(fbe_protocol_error_injection_error_record_t* err_rec_ptr, fbe_job_service_bes_t *bes);
static fbe_status_t fbe_cli_neit_usr_get_fru(fbe_job_service_bes_t* bes);
static fbe_status_t fbe_cli_neit_usr_get_error(fbe_protocol_error_injection_error_record_t* record_ptr);
static fbe_status_t fbe_cli_neit_usr_get_reactivation_data(fbe_protocol_error_injection_error_record_t* record_ptr);
static fbe_u8_t* fbe_cli_neit_usr_get_scsi_command(void);
static fbe_scsi_sense_key_t fbe_cli_neit_usr_get_sense_key(void);
static fbe_scsi_additional_sense_code_t fbe_cli_neit_usr_get_sense_code(void);
static fbe_scsi_additional_sense_code_qualifier_t fbe_cli_neit_usr_get_sense_code_qualifier(void);
static fbe_sata_command_t fbe_cli_neit_usr_get_sata_command(void);
static fbe_status_t fbe_cli_neit_usr_get_frequency(fbe_protocol_error_injection_error_record_t* record_ptr);
fbe_u8_t* fbe_neit_str_to_upper(fbe_u8_t *dest_ptr, fbe_u8_t *src_ptr);
static fbe_lba_t fbe_cli_neit_what_is_fru_private_space(fbe_job_service_bes_t* bes);
fbe_lba_t fbe_fru_capacity(fbe_job_service_bes_t* bes);
static fbe_status_t fbe_cli_neit_usr_get_search_params(FBE_NEIT_SEARCH_PARAMS* search_params_ptr);
static fbe_protocol_error_injection_error_type_t fbe_cli_neit_user_get_err_type(void);
static fbe_status_t fbe_cli_neit_print_record(fbe_protocol_error_injection_error_record_t* record_p);
fbe_status_t fbe_cli_neit_list_record(fbe_protocol_error_injection_record_handle_t *handle_p);
static void fbe_cli_neit_display_table_header(void);

fbe_bool_t fbe_cli_neit_exit_from_command(fbe_char_t *command_str);
fbe_status_t fbe_cli_neit_usr_get_scsi_error(fbe_protocol_error_injection_error_record_t* record_ptr);
/*-------------------
    Utility Functions
--------------------*/
fbe_u32_t fbe_neit_strtoint(fbe_u8_t* src_str);

#endif /* end __FBE_CLI_NEIT_SERVICE_H__ */
