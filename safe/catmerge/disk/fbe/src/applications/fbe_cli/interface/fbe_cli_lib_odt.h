#ifndef FBE_CLI_LIB_ODT_H
#define FBE_CLI_LIB_ODT_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_odt.h
 ***************************************************************************
 *
 * @brief
 *  This file contains persist service command cli definitions.
 *
 * @version
 *  05/20/2011 - Created. Shubhada Savdekar
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_api_persist_interface.h"
#include "fbe_private_space_layout.h"


#define ODT_USAGE "\
Description: This command is used to write and to get the information from "\
"the persist service.\n"\
"\nUsage:\n"\
"odt <operations> <switches>\n"\
"       odt -h|-help \n"\
"       odt s\n"\
"       odt u\n"\
"       odt r -s <sector type>  [-e entry_id] [a]\n"\
"       odt w -s <sector tpe> -d <data>\n"\
"       odt wm -s <sector type>  -d <data> [-d] [data] ....[-d]  [data]\n"\
"       odt wa -s <sector type> -d <data>\n"\
"       odt l\n"\
"       odt d -s <sector type>  <-e entry_id>\n"\
"       odt dm -s <sector type <-e entry_id> [-e] [entry_id]....[-e][entry_id]\n"\
"       odt m -s <sector type>  <-e entry_id> -d <new data>\n"\
"       odt mm -s <sector type>  <-e entry_id> -d <new data>....[-e entry_id] [-d new data]\n"\
"Please note that, data and entry id are in hexadecimal.\n"\
"\nOperations:\n"\
"       -h|-help       - Help\n"\
"       s              - set the LUN\n"\
"       u              - unset the LUN\n"\
"       l              - Get layout information\n "\
"       r              - Read the whole sector or a single entry.\n"\
"       w|wm|wa        - Write the to a single entry or multiple entries.\n"\
"       se             - Show last written entries  (This feature is appicable for current session only.)\n"\
"       d | dm         - Delete a single entry or multiple entries\n"\
"       m |mm          - Modify a single entry or multiple entries\n"\
"\nSwitches:\n"\
"       -s  \n"\
"           > sep_obj         -SEP Objects\n"\
"           > sep_edge        -SEP Edge\n"\
"           > sep_admin       -SEP Admin conversion\n"\
"           > esp_obj         -ESP Objects\n"\
"           > sys             -System Global data\n"\
"           > dieh            -dieh data\n"\
"           > scratch_pad     -scratch_pad\n\n"\
"       -d                - Data\n"\
"       -e                - Entry\n"\
"        a                 - Display all the entries in a sector a single key press."\
"\nExamples:\n"\
"       odt w -s sep_obj -d 15\n"\
"       odt mm -s sep_edge -e 0 -d c1 d3 -e 1 -d b4 f5\n"\
"       odt r -s esp_obj -e 31\n"\
"       odt d -s sys -e 5\n"\
"       odt l\n\n"\

/*!**************************************************
 * @def FBE_CLI_ODT_MAX_ENTRIES
 ***************************************************
 * @brief This is the maximum entries can remain in buffer.
 ***************************************************/

#define FBE_CLI_ODT_MAX_ENTRIES      512

/*!**************************************************
 * @def FBE_CLI_ODT_ENTRY_SIZE
 ***************************************************
 * @brief This is the entry size in the persist service.
 ***************************************************/
#define FBE_CLI_ODT_ENTRY_SIZE         FBE_PERSIST_DATA_BYTES_PER_ENTRY

#define FBE_CLI_ODT_TRAN_ENTRY_MAX FBE_PERSIST_TRAN_ENTRY_MAX

/*!**************************************************
 * @def FBE_CLI_ODT_OBJECT_ID
 ***************************************************
 * @brief This is the object id of the persist LUN.
 ***************************************************/
#define FBE_CLI_ODT_OBJECT_ID  FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_DATABASE


/*!**************************************************
 * @def FBE_CLI_ODT_ENTRIES_DISPLAY_COUNT
 ***************************************************
 * @brief This is the number of entries to be displayed on a screen at a time.
 ***************************************************/
#define FBE_CLI_ODT_ENTRIES_DISPLAY_COUNT    5

#define FBE_CLI_ODT_ZERO                  0
#define FBE_CLI_ODT_DUMMYVAL         3
#define FBE_CLI_ODT_COLUMN_SIZE    8

#define FBE_CLI_ODT_DATA_BOGUS   -1
#define FBE_CLI_ODT_MINUS_ONE    -1

/*****************
 *   STRUCTURES
 *****************/

typedef struct persist_service_struct_s{
    fbe_persist_user_header_t          persist_header;  /*can be used with any data that is persisted*/
    fbe_u8_t* some_data;
}persist_service_struct_t;

typedef struct read_context_s{
    fbe_semaphore_t           sem;
    fbe_persist_entry_id_t    next_entry_id;
    fbe_u32_t                 entries_read;
}read_context_t;

typedef enum odt_w_count_enum
{
    FBE_CLI_ODT_W_COUNT_NONE = 0,
    FBE_CLI_ODT_W_COUNT_ONE, 
    FBE_CLI_ODT_W_COUNT_TWO, 
    FBE_CLI_ODT_W_COUNT_THREE,
    FBE_CLI_ODT_W_COUNT_FOUR,
    FBE_CLI_ODT_W_COUNT_FIVE,
    FBE_CLI_ODT_W_COUNT_SIX,
    FBE_CLI_ODT_W_COUNT_SEVEN
}fbe_cli_odt_w_count_enum_t;

typedef struct single_entry_context_s{
    fbe_semaphore_t sem;
    fbe_persist_entry_id_t  received_entry_id;
    fbe_bool_t              expect_error;
}single_entry_context_t;

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

void                fbe_cli_odt(int argc, char** argv);
fbe_status_t        fbe_cli_lib_odt_get_layout_info(void);
fbe_status_t        fbe_cli_lib_odt_set_lun(void);
fbe_status_t        fbe_cli_lib_odt_single_entry_read(fbe_persist_sector_type_t  target_sector,
     fbe_persist_entry_id_t  entry);
fbe_status_t        read_single_entry_completion_function(fbe_status_t read_status,
    fbe_persist_completion_context_t context, fbe_persist_entry_id_t next_entry);
fbe_status_t        fbe_cli_lib_odt_single_entry_write(fbe_u32_t argc, fbe_char_t**argv);
fbe_status_t        fbe_cli_lib_odt_single_entry_delete(fbe_u32_t argc, fbe_char_t **argv);
fbe_status_t        fbe_cli_lib_odt_delete_entries(fbe_u32_t argc, fbe_char_t **argv);
fbe_status_t        fbe_cli_lib_odt_sector_read(fbe_persist_sector_type_t    target_sector);
fbe_status_t        fbe_cli_lib_odt_modify_entries(fbe_u32_t argc, fbe_char_t**argv);
fbe_status_t        fbe_cli_lib_odt_modify_single_entry(fbe_u32_t argc, fbe_char_t**argv);
fbe_status_t        fbe_cli_lib_odt_write_entries(fbe_u32_t argc, fbe_char_t**argv);
fbe_status_t        fbe_cli_lib_odt_single_entry_write(fbe_u32_t argc, fbe_char_t**argv);
fbe_status_t        fbe_cli_lib_odt_write_entry_with_auto_entry_id_on_top(fbe_u32_t argc, fbe_char_t**argv);
fbe_status_t        fbe_cli_lib_odt_unset_lun(void);
void                fbe_cli_lib_odt_cleanup(persist_service_struct_t *odt_data);


//void                    create_lun(fbe_lun_number_t lun_number);
fbe_status_t        commit_completion_function (fbe_status_t commit_status,
    fbe_persist_completion_context_t context);
fbe_status_t        single_operation_completion(fbe_status_t op_status,
                                                                                   fbe_persist_entry_id_t entry_id,
                                                                                   fbe_persist_completion_context_t context);
fbe_status_t        sector_read_completion_function(fbe_status_t read_status, 
                                             fbe_persist_entry_id_t next_entry,
                                             fbe_u32_t entries_read,
                                             fbe_persist_completion_context_t context);

void                   fbe_cli_lib_odt_display_entry_ids_and_sectors(void);
void                   fbe_cli_lib_odt_read(fbe_u32_t argc, fbe_char_t**argv);
void                   fbe_cli_odt_print_buffer_contents(fbe_sg_element_t *sg_ptr,  fbe_lba_t disk_address,
                                       fbe_u32_t blocks, fbe_u32_t block_size);
void                   fbe_cli_odt_display_sector( fbe_u8_t * in_addr, fbe_u32_t w_count);
void                  fbe_cli_lib_odt_get_target_name(fbe_char_t**argv, fbe_persist_sector_type_t* target_sector);
fbe_status_t fbe_cli_lib_odt_get_single_entry(fbe_u32_t argc, fbe_char_t**argv, fbe_persist_entry_id_t*  entry);
fbe_status_t fbe_cli_lib_odt_get_data_for_single_entry(fbe_u32_t* argc, fbe_char_t**argv, persist_service_struct_t *   odt_data) ;
fbe_status_t fbe_cli_lib_odt_get_status(fbe_u32_t argc, fbe_char_t**argv, fbe_persist_sector_type_t*   target_sector_p,persist_service_struct_t *   odt_data);
fbe_status_t check_data_param(fbe_u32_t argc, fbe_char_t**argv);
fbe_status_t check_entry_param(fbe_u32_t argc, fbe_char_t**argv);

/*************************
 * end file fbe_cli_lib_odt.h
 *************************/

#endif/* end __FBE_CLI_LIB_ODT_H__ */

