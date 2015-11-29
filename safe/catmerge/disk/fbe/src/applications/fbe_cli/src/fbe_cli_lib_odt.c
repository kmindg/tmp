/***************************************************************************
* Copyright (C) EMC Corporation 2009
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_cli_lib_odt.c
***************************************************************************
*
* @brief
*  This file contains the object database tester for persist service .
*
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include"fbe_cli_lib_odt.h"
#include "fbe_cli_private.h"
#include "fbe/fbe_cli.h"
#include "string.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_data_pattern.h"
#include "stdlib.h"

/*************************
*   LOCAL DEFINITIONS
*************************/

static fbe_bool_t                    init_buffer_flag = FBE_FALSE;
static fbe_bool_t                    show_all_entries = FBE_FALSE;
static  fbe_persist_user_header_t    odt_header[FBE_CLI_ODT_MAX_ENTRIES];
static fbe_u32_t                     cli_lib_odt_index;
static void*                         read_buffer = NULL;

static fbe_status_t set_lun_completion_function (fbe_status_t commit_status, fbe_persist_completion_context_t context);
static char* convert_target_name_to_string( fbe_persist_sector_type_t target );
static void  fbe_cli_lib_odt_init_buffer(void);

/****************************************************************
* fbe_cli_odt(int argc, char** argv)
****************************************************************
* @brief
*  This function is used to perform operations on the persist service.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
* @return - none.  
*
* @revision
* * 05/16/2011: created:  Shubhada Savdekar
* * 07/28/2011: modified: Shubhada Savdekar
*
****************************************************************/
void fbe_cli_odt (int argc, char**argv)
{
    fbe_status_t                    status= FBE_STATUS_INVALID ;

    if (argc == 0) {
        fbe_cli_printf("odt: ERROR : Too few arguments.\n");
        fbe_cli_printf("%s", ODT_USAGE);
        return;
    }

    if ((strcmp(*argv, "-help") == 0) ||(strcmp(*argv, "-h") == 0)) {
        /* If they are asking for help, just display help and exit.*/
        fbe_cli_printf("%s", ODT_USAGE);
        return;
    }

    /*get the layout information*/
    if(strcmp(*argv, "l") == 0) {
        status =fbe_cli_lib_odt_get_layout_info();
        if (status != FBE_STATUS_OK) {
            return;
        }
        return;
    }

    /*Set the LUN */
    if(strcmp(*argv, "s") == 0) {
        status =  fbe_cli_lib_odt_set_lun();
        if ((status != FBE_STATUS_OK)  && (status != FBE_STATUS_PENDING)) {
            fbe_cli_printf("odt: ERROR: Error in set LUN\n");
            fbe_cli_printf("odt: ERROR:The error code returned is %d", status);
            return;
        }
        return;
    }

    /*Unset the LUN*/
    if(strcmp(*argv, "u") == 0) {
        status = fbe_cli_lib_odt_unset_lun();
        if(status == FBE_STATUS_GENERIC_FAILURE){
            fbe_cli_printf("odt: ERROR: Failed to unset the LUN.\n");
            fbe_cli_printf("odt: ERROR:The error code returned is %d", status);
            return;
        }
        return;
    }

    /*read the entry*/
    if(strcmp(*argv, "r")==0) {
        argc--;
        argv++;
        fbe_cli_lib_odt_read((fbe_u32_t)argc, (fbe_char_t**)argv);
        return;
    }
    
    /*write the entry*/
    if(strcmp(*argv, "w") == 0) {
        /* Initialize the buffer for written entries */
        if(init_buffer_flag == FBE_FALSE) {
             fbe_cli_lib_odt_init_buffer();
             init_buffer_flag = FBE_TRUE;
        }

        status= fbe_cli_lib_odt_single_entry_write((fbe_u32_t)argc, (fbe_char_t**)argv);
        if (status != FBE_STATUS_OK) {
            fbe_cli_printf("odt: ERROR:The error code returned is %d", status);
            return;
        }
        return;
    }

    /*write the entry with auto entryid up*/
    if(strcmp(*argv, "wa") == 0) {
        /* Initialize the buffer for written entries */
        if(init_buffer_flag == FBE_FALSE) {
            fbe_cli_lib_odt_init_buffer();
            init_buffer_flag = FBE_TRUE;
        }

        status= fbe_cli_lib_odt_write_entry_with_auto_entry_id_on_top((fbe_u32_t)argc, (fbe_char_t**)argv);
        if (status != FBE_STATUS_OK) {
            return;
        }
        return;
    }

    /*write the multiple entries*/
    if(strcmp(*argv, "wm") == 0) {

        /* Initialize the buffer for written entries */
        if(init_buffer_flag == FBE_FALSE) {
            fbe_cli_lib_odt_init_buffer();
            init_buffer_flag = FBE_TRUE;
        }

        status= fbe_cli_lib_odt_write_entries((fbe_u32_t)argc, (fbe_char_t**)argv);
        if (status != FBE_STATUS_OK) {
            return;
        }
        return;
    }
    
    /*modify the entry*/
    if(strcmp(*argv, "m") == 0) {
        status= fbe_cli_lib_odt_modify_single_entry((fbe_u32_t)argc, (fbe_char_t**)argv);
        if (status != FBE_STATUS_OK) {
            return;
        }
        return;
    }

    /*modify multiple  entries*/
    if(strcmp(*argv, "mm") == 0) {
        status= fbe_cli_lib_odt_modify_entries((fbe_u32_t)argc, (fbe_char_t**)argv);
        if (status != FBE_STATUS_OK) {
            return;
        }
        return;
    }

    /*Delete the entry*/
     if(strcmp(*argv, "d") == 0) {
        status=  fbe_cli_lib_odt_single_entry_delete((fbe_u32_t)argc, (fbe_char_t**)argv);
        if (status != FBE_STATUS_OK) {
            return;
        }
        return;
    }

    /*Delete multiple entries*/
    if(strcmp(*argv, "dm") == 0) {
        status= fbe_cli_lib_odt_delete_entries((fbe_u32_t)argc, (fbe_char_t**)argv);
        if (status != FBE_STATUS_OK) {
            return;
        }
        return;
    }

    /*Show the recently written entries in the FBECLI session*/
    if(strcmp(*argv, "se") == 0){
        fbe_cli_lib_odt_display_entry_ids_and_sectors();
        return;
    }

    /*No matches found*/
    fbe_cli_printf("odt: ERROR: Invalid Input !!\n");
    fbe_cli_printf("%s", ODT_USAGE);
    return ;
}


/****************************************************************
* fbe_cli_lib_odt_read(fbe_u32_t argc, fbe_char_t**argv)
****************************************************************
* @brief
*  This function is used to select any of the following read 
*  operations on the persist service:
*                      Read a single entry
*                      Read a sector
*
*  @param    argc - argument count
*  @param    argv - argument string
*
* @return - none.  
*
* @revision
* * 05/25/2011: created:  Shubhada Savdekar
*
****************************************************************/

void fbe_cli_lib_odt_read(fbe_u32_t argc, fbe_char_t**argv)
{
    fbe_status_t                       status=FBE_STATUS_INVALID ;
    fbe_persist_sector_type_t          target_sector;
    fbe_persist_entry_id_t             entry = -1;

    /*Initialize*/
    show_all_entries = FBE_FALSE;

    /*Extract the command*/
    while(argc > 0) {
        /*Get entry id from the user*/
        if (strcmp(*argv, "-e") == 0) {
            argc--;
            argv++;

            status = fbe_cli_lib_odt_get_single_entry(argc, argv, &entry);
            if(status != FBE_STATUS_OK){
                fbe_cli_printf("odt: ERROR: Error in read operation\n");
                fbe_cli_printf("odt: ERROR:The error code returned is %d", status);
                return;
            }
            argc--;
            argv++;
        }
        /*Get the show_all_entries  flag*/
        else if(strcmp(*argv, "a") == 0){
            show_all_entries = FBE_TRUE;
            argc--;
            argv++;
        }
        /* Get sector type*/
        else if(strcmp(*argv,"-s") == 0) {
            argc--;
            argv++;

            status = fbe_cli_lib_odt_get_status(argc, argv, &target_sector,NULL);
            if(status != FBE_STATUS_OK){
                return ;
            }
            argc--;
            argv++;
        }
        else {
            fbe_cli_printf("Invalid Input !!\n");
            fbe_cli_printf("%s", ODT_USAGE);
            return ;
        }
    }

    if(( target_sector == FBE_PERSIST_SECTOR_TYPE_INVALID)  &&  (entry == FBE_CLI_ODT_MINUS_ONE)) {
        fbe_cli_printf("Invalid Input !!\n");
        fbe_cli_printf("%s", ODT_USAGE);
        return ;
    }

    if(entry == FBE_CLI_ODT_MINUS_ONE) {
        status = fbe_cli_lib_odt_sector_read(target_sector);
        if (status != FBE_STATUS_OK) {
            fbe_cli_printf("odt: ERROR:The error code returned is %d", status);
            return;
        }
        return;
    }

    status = fbe_cli_lib_odt_single_entry_read(target_sector, entry);
    if (status != FBE_STATUS_OK) {
        return;
    }
}

/****************************************************************
* fbe_cli_lib_odt_sector_read(fbe_u32_t argc, fbe_char_t**argv)
****************************************************************
* @brief
*  This function is used to perform READ operationon on the sector.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
*@return fbe_status_t status of the operation.
*
* @revision
* * 05/25/2011: created:  Shubhada Savdekar
*
****************************************************************/
fbe_status_t fbe_cli_lib_odt_sector_read(fbe_persist_sector_type_t target_sector)
{

    fbe_status_t                    status=FBE_STATUS_INVALID ;
    fbe_status_t                    wait_status;
    fbe_u32_t                       total_entries_read = 0;
    persist_service_struct_t*       temp;
    fbe_u32_t                       count;
    fbe_sg_element_t *              sg_element;
    fbe_u32_t                       block_count;
    fbe_u8_t*                       temp_read_buffer;
    read_context_t *                read_context = (read_context_t *)malloc (sizeof(read_context_t));

    if(read_context == NULL) {
        fbe_cli_printf("Failed to allocate memory for read_context!\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if(read_buffer != NULL){
        fbe_api_free_memory(read_buffer); /* SAFEBUG - wrong API was being used */
        read_buffer = NULL;
    }

    /*Allcat ethe buffer for reading the data from persist service*/
    read_buffer = (persist_service_struct_t *)fbe_api_allocate_memory(FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE);
    if (read_buffer == NULL) {
        fbe_cli_printf("Can't allocate memory to read_buffer!\n ");
        free(read_context);
        read_context = NULL;
        return FBE_STATUS_GENERIC_FAILURE;
    }

   read_context->next_entry_id = FBE_PERSIST_SECTOR_START_ENTRY_ID;

   sg_element = (fbe_sg_element_t *)fbe_api_allocate_memory(sizeof(fbe_sg_element_t) * 2);
    if (sg_element == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for sg list\n", __FUNCTION__); 
        free(read_context);
        read_context = NULL;
        fbe_api_free_memory(read_buffer); /* SAFEBUG - wrong API was being used */
        read_buffer = NULL;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*Semaphore initialization*/
    fbe_semaphore_init(&read_context->sem, 0, 1);

    do {
        memset(read_buffer, 0, FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE);
        /*Read the data from the sector. It will read the 400 entries at a single time.*/
        status = fbe_api_persist_read_sector(target_sector, read_buffer,
                                             FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE,
                                             read_context->next_entry_id,
                                             sector_read_completion_function,
                                             (fbe_persist_completion_context_t)read_context);
        if(status == FBE_STATUS_GENERIC_FAILURE){
            fbe_cli_printf("Error in reading  the sector\n");
            fbe_semaphore_destroy(&read_context->sem);
            free(read_context);
            read_context = NULL;
            fbe_api_free_memory(read_buffer); /* SAFEBUG - wrong API was being used */
            read_buffer = NULL;
            fbe_api_free_memory(sg_element);
            sg_element = NULL;
            return status;
        }

        wait_status = fbe_semaphore_wait_ms(&read_context->sem, 10000);
        if(wait_status == FBE_STATUS_TIMEOUT){
            fbe_cli_printf("odt: TIMEOUT ERROR: Timeout in reading  the sector\n");
            fbe_semaphore_destroy(&read_context->sem);
            free(read_context);
            read_context = NULL;
            fbe_api_free_memory(read_buffer); /* SAFEBUG - wrong API was being used */
            read_buffer = NULL;
            fbe_api_free_memory(sg_element);
            sg_element = NULL;
            return status;
        }

        /*Display the read entries. */
        temp = read_buffer;
        temp_read_buffer = read_buffer;
        temp_read_buffer += sizeof( fbe_persist_user_header_t );

         for(count = 0 ; count <read_context->entries_read; count++){
            if(show_all_entries == FBE_FALSE){
                if(( count != 0) && (count % FBE_CLI_ODT_ENTRIES_DISPLAY_COUNT == 0)) {
                    fbe_cli_printf("Press any key to continue...\n");
                    getch();
                 }
            }
            fbe_cli_printf("Entry_id:..........%llx\nData:\n",
			   (unsigned long long)temp->persist_header.entry_id);

            for(block_count = 0; block_count < FBE_PERSIST_BLOCKS_PER_ENTRY ; block_count ++){
                fbe_cli_printf("\nThe data for block %d is:\n", block_count);
                status = fbe_data_pattern_sg_fill_with_memory(sg_element, 
                                                              temp_read_buffer,
                                                              FBE_PERSIST_BLOCKS_PER_ENTRY,
                                                              FBE_PERSIST_DATA_BYTES_PER_BLOCK,
                                                              FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS,
                                                              FBE_DATA_PATTERN_MAX_BLOCKS_PER_SG);
                 if (status != FBE_STATUS_OK ) {
                    fbe_cli_error("Setting up Sg failed.\n");
                    fbe_semaphore_destroy(&read_context->sem);
                    fbe_api_free_memory(read_buffer);
                    read_buffer = NULL;
                    fbe_api_free_memory(sg_element);
                    sg_element = NULL;
                    free(read_context);
                    read_context = NULL;
                    return status;
                }
                fbe_cli_odt_print_buffer_contents(sg_element, 0, 1, FBE_PERSIST_DATA_BYTES_PER_BLOCK);
                temp_read_buffer+= FBE_BYTES_PER_BLOCK;
                
            } /* end of for (block_count = 0 ; ..)*/

            temp =(    persist_service_struct_t*) temp_read_buffer;
            temp_read_buffer+=sizeof(temp->persist_header.entry_id);

            fbe_cli_printf("\n-----------------------------------------------------------\n");
        }

        total_entries_read+= read_context->entries_read;
        
    } while (read_context->next_entry_id != FBE_PERSIST_NO_MORE_ENTRIES_TO_READ);

    fbe_cli_printf("Total %d entries read from sector  %s.\n", total_entries_read,
                   convert_target_name_to_string(target_sector));
    fbe_cli_printf("odt: SUCCESS: Data Buffer displayed successfully !!\n\n");

    fbe_semaphore_destroy(&read_context->sem);
    if( read_context != NULL) {
        free(read_context);
        read_context = NULL;
    }

    if(read_buffer != NULL) {
        fbe_api_free_memory(read_buffer); /* SAFEBUG - wrong API was being used */
        read_buffer = NULL;
    }

    if(sg_element != NULL) {
        fbe_api_free_memory(sg_element);
        sg_element = NULL;
    }
    return status;
}

/**************************************************************
* sector_read_completion_function(fbe_status_t read_status, 
*                                 fbe_persist_entry_id_t next_entry,
*                                 fbe_u32_t entries_read,
                                  fbe_persist_completion_context_t context)
****************************************************************
* @brief
*  This is the completion function for sector read operation.
*
* @param    read_status - status of the read operation
* @param     next_entry
* @param....entries_read
* @param    context       - completion context
*
*
*@return fbe_status_t status of the operation.
*
* @revision
* * 05/25/2011: created:  Shubhada Savdekar
*
****************************************************************/
fbe_status_t sector_read_completion_function(fbe_status_t read_status,
                                             fbe_persist_entry_id_t next_entry,
                                             fbe_u32_t entries_read,
                                             fbe_persist_completion_context_t context)
{
    read_context_t * read_context = ( read_context_t *)context;

    read_context->next_entry_id= next_entry;
    read_context->entries_read = entries_read;
    fbe_semaphore_release(&read_context->sem, 0, 1 ,FALSE);
    return FBE_STATUS_OK;
}


/****************************************************************
* fbe_cli_lib_odt_single_entry_read(fbe_u32_t argc, fbe_char_t**argv)
****************************************************************
* @brief
*  This function is used to perform READ operationon for the single 
*  entry  in the persist service.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
*@return fbe_status_t status of the operation.
*
* @revision
* * 05/16/2011: created:  Shubhada Savdekar
*
****************************************************************/
fbe_status_t fbe_cli_lib_odt_single_entry_read(fbe_persist_sector_type_t  target_sector,
                                               fbe_persist_entry_id_t  entry)
{
    fbe_status_t                            status;
    fbe_status_t                            wait_status;
    fbe_semaphore_t                         sem;
    fbe_sg_element_t *                      sg_element;
    persist_service_struct_t *              odt_temp = NULL;
    fbe_u32_t                               block_count;
    fbe_u8_t*                               temp_read_buffer;

    odt_temp = fbe_api_allocate_memory(sizeof(persist_service_struct_t));

    if(odt_temp == NULL) {
        fbe_cli_printf("Failed to allocate memory for odt_temp !\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(odt_temp, sizeof (persist_service_struct_t));

    /*Add target sector and requested entry to odt_temp->persist_header.entry_id  */
    odt_temp->persist_header.entry_id = target_sector;
    odt_temp->persist_header.entry_id <<=32;
    odt_temp->persist_header.entry_id |= entry;

    if(read_buffer != NULL){
        fbe_api_free_memory(read_buffer);
        read_buffer = NULL;
    }
    
    read_buffer = (fbe_u8_t*)fbe_api_allocate_memory( FBE_PERSIST_DATA_BYTES_PER_ENTRY);
    if (read_buffer == NULL) {
        fbe_cli_printf("Can't allocate memory to read_buffer!\n ");
        fbe_cli_lib_odt_cleanup(odt_temp);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_element = (fbe_sg_element_t *)fbe_api_allocate_memory(sizeof(fbe_sg_element_t) * 2);
    if (sg_element == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Unable to allocate memory for sg list\n",
                                __FUNCTION__);
        fbe_api_free_memory(read_buffer);
        fbe_cli_lib_odt_cleanup(odt_temp);
        read_buffer = NULL;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_semaphore_init(&sem, 0, 1);

    /*Read the data*/
    status = fbe_api_persist_read_single_entry(odt_temp->persist_header.entry_id ,
                                               read_buffer,
                                               FBE_PERSIST_DATA_BYTES_PER_ENTRY,
                                               read_single_entry_completion_function,
                                               &sem);
    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING) {
        fbe_cli_printf("Error while reading single entry !\n");
        fbe_api_free_memory(read_buffer);
        fbe_cli_lib_odt_cleanup(odt_temp);
        fbe_api_free_memory(sg_element);
        fbe_semaphore_destroy(&sem);
        return status;
    }

    wait_status = fbe_semaphore_wait_ms(&sem, 10000);
    if (wait_status == FBE_STATUS_TIMEOUT){
        fbe_cli_printf("odt: TIMEOUT ERROR: Timeout at read entry \n");
        fbe_api_free_memory(read_buffer);
        fbe_cli_lib_odt_cleanup(odt_temp);
        fbe_api_free_memory(sg_element);
        fbe_semaphore_destroy(&sem);
        return status;
    }


    temp_read_buffer = read_buffer;
    fbe_cli_printf("\nThe read sector type is:..%s\nEntry_id is:..........%llx\n\n", 
                   convert_target_name_to_string(target_sector),
		   (unsigned long long)entry);

    for(block_count = 0; block_count < FBE_PERSIST_BLOCKS_PER_ENTRY ; block_count ++){
        fbe_cli_printf("\nThe data for block %d is:\n", block_count);

        status = fbe_data_pattern_sg_fill_with_memory(sg_element,  temp_read_buffer, FBE_PERSIST_BLOCKS_PER_ENTRY,
                                                      FBE_PERSIST_DATA_BYTES_PER_BLOCK,
                                                      FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS,
                                                      FBE_DATA_PATTERN_MAX_BLOCKS_PER_SG);
        if (status != FBE_STATUS_OK ) {
            fbe_cli_error("Setting up Sg failed.\n");
            fbe_api_free_memory(read_buffer);
            fbe_cli_lib_odt_cleanup(odt_temp);
            fbe_api_free_memory(sg_element);
            fbe_semaphore_destroy(&sem);
            return status;
        }

        fbe_cli_odt_print_buffer_contents(sg_element, 0, 1, FBE_PERSIST_DATA_BYTES_PER_BLOCK);
        temp_read_buffer+= FBE_BYTES_PER_BLOCK;
    }/*end of for block_count =0*/

    fbe_cli_printf("-------------------------------------------\n");
    fbe_cli_printf("odt: SUCCESS: The data read successfully!!\n\n");

    fbe_semaphore_destroy(&sem);

    if(read_buffer){
        fbe_api_free_memory(read_buffer);
        read_buffer = NULL;
    }

    fbe_cli_lib_odt_cleanup(odt_temp);

    if(sg_element) {
        fbe_api_free_memory(sg_element);
        sg_element = NULL;
    }

    return status;
}


/**************************************************************
* read_single_entry_completion_function(fbe_status_t read_status, 
*                                       fbe_persist_completion_context_t context)
****************************************************************
* @brief
*  This is the completion function for single entry read operation.
*
* @param    read_status - status of the read operation
* @param    context       - completion context
*
*
*@return fbe_status_t status of the operation.
*
* @revision
* * 05/16/2011: created:  Shubhada Savdekar
*
****************************************************************/
fbe_status_t read_single_entry_completion_function(fbe_status_t read_status,
                                                   fbe_persist_completion_context_t context,
                                                   fbe_persist_entry_id_t next_entry)
{
    fbe_semaphore_t *   sem = (fbe_semaphore_t *)context;

    if ( read_status != FBE_STATUS_OK){
        fbe_cli_printf("odt: ERROR: Error in reading the single entry\n");
        return read_status;
    }
    fbe_semaphore_release(sem, 0, 1 ,FALSE);

    return FBE_STATUS_OK;
}

/****************************************************************
*fbe_cli_lib_odt_modify_single_entry(fbe_u32_t argc, fbe_char_t**argv)
****************************************************************
* @brief
*  This function is used to modify the entry on the persist service.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
*@return fbe_status_t status of the operation.
*
* @revision
* * 06/14/2011: created:  Shubhada Savdekar
*
****************************************************************/
fbe_status_t  fbe_cli_lib_odt_modify_single_entry(fbe_u32_t argc, fbe_char_t**argv)
{
    fbe_status_t                            status=FBE_STATUS_OK;
    fbe_status_t                            wait_status;
    persist_service_struct_t *              odt_data = NULL;
    fbe_persist_sector_type_t               target_sector =  FBE_PERSIST_SECTOR_TYPE_INVALID;
    single_entry_context_t      single_entry_context;
    fbe_persist_entry_id_t  entry = FBE_CLI_ODT_MINUS_ONE;
    
    argc--;
    argv++;

    if( argc == 0){
        fbe_cli_printf("Not enough arguments\n");
        fbe_cli_printf("%s", ODT_USAGE);
        return status;
    }

    /*Extract the command*/
    while (argc > 0) {
        if (strcmp(*argv, "-d") == 0) {
            argc--;
            argv++;

            /*Allocate the memory for writing the buffer*/
            odt_data = fbe_api_allocate_memory(sizeof(persist_service_struct_t));

            if(odt_data == NULL) {
                fbe_cli_printf("Failed to allocate memory for odt_data!\n");
                return FBE_STATUS_GENERIC_FAILURE;
            }

            fbe_zero_memory(odt_data, sizeof (persist_service_struct_t));
            
            odt_data->some_data = (fbe_u8_t*) fbe_api_allocate_memory(FBE_CLI_ODT_ENTRY_SIZE * sizeof(fbe_u8_t));

            if(odt_data->some_data == NULL) {
                fbe_cli_printf("Failed to allocate memory for odt_data->some_data!\n");
                fbe_cli_lib_odt_cleanup(odt_data);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            fbe_zero_memory( odt_data->some_data, (FBE_CLI_ODT_ENTRY_SIZE * sizeof(fbe_u8_t)));
            
            status = fbe_cli_lib_odt_get_data_for_single_entry(&argc, argv, odt_data);
            if(status != FBE_STATUS_OK){
                return status;
            }

        }
        else if (strcmp(*argv, "-e") == 0) {
            argc--;
            argv++;

            status = fbe_cli_lib_odt_get_single_entry(argc, argv, &entry);
            if(status != FBE_STATUS_OK){
                fbe_cli_lib_odt_cleanup(odt_data);
                return status;
            }
            argc--;
            argv++;
        }

        /* Get sector type*/
        else if(strcmp(*argv,"-s") == 0) {
            argc--;
            argv++;

            status = fbe_cli_lib_odt_get_status(argc,
                argv, &target_sector, odt_data);
            if(status != FBE_STATUS_OK){
                return status;
            }
            
            argc--;
            argv++;
        }

        else {
            fbe_cli_printf("odt:ERROR: Invalid Input !!\n");
            fbe_cli_printf("%s", ODT_USAGE);
            fbe_cli_lib_odt_cleanup(odt_data);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    if(odt_data == NULL){
        fbe_cli_printf("odt: ERROR: Invalid Input!!\n");
        fbe_cli_printf("%s", ODT_USAGE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if((target_sector == FBE_PERSIST_SECTOR_TYPE_INVALID) || 
            (entry == FBE_CLI_ODT_MINUS_ONE)){
        fbe_cli_printf("odt: ERROR: Invalid Input!!\n");
        fbe_cli_printf("%s", ODT_USAGE);
        fbe_cli_lib_odt_cleanup(odt_data);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*  Add target sector and requested entry to single_entry_context.received_entry_id */
    single_entry_context.received_entry_id = target_sector;
    single_entry_context.received_entry_id <<=32;
    single_entry_context.received_entry_id |=entry;
   
   /* Semaphores are used because, we are commiting the transaction. The commit 
    *  transaction has asynchronous interface
    */
    fbe_semaphore_init(&single_entry_context.sem, 0, 1);

    single_entry_context.expect_error = FBE_FALSE;   

    /*Modify the data*/
    status  = fbe_api_persist_modify_single_entry(single_entry_context.received_entry_id,
                                             (fbe_u8_t *) (odt_data->some_data),
                                              FBE_CLI_ODT_ENTRY_SIZE,
                                              single_operation_completion,
                                              &single_entry_context);

    if(status == FBE_STATUS_GENERIC_FAILURE)
    {
        fbe_cli_error("odt ERROR:Error in modifying the single entry.\n");

        fbe_cli_lib_odt_cleanup(odt_data);
        fbe_semaphore_destroy(&single_entry_context.sem);
        return status;
    }

    wait_status = fbe_semaphore_wait_ms(&single_entry_context.sem, 10000);
    if(wait_status == FBE_STATUS_TIMEOUT)
    {
        fbe_cli_error("odt ERROR:Timeout error in deleting the single entry.\n");
        fbe_cli_lib_odt_cleanup(odt_data);
        fbe_semaphore_destroy(&single_entry_context.sem);
        return status;
    }

    fbe_semaphore_destroy(&single_entry_context.sem);
    fbe_cli_lib_odt_cleanup(odt_data);

    fbe_cli_printf("odt: SUCCESS:The data entry modified successfully!!\n\n");
    return status;

}

/****************************************************************
*fbe_cli_lib_odt_modify_entries(fbe_u32_t argc, fbe_char_t**argv)
****************************************************************
* @brief
*  This function is used to modify the multiple entries in the persist service.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
*@return fbe_status_t status of the operation.
*
* @revision
* * 06/14/2011: created:  Shubhada Savdekar
*
****************************************************************/

fbe_status_t fbe_cli_lib_odt_modify_entries(fbe_u32_t argc, fbe_char_t**argv)
{
    fbe_status_t                            status=FBE_STATUS_OK;
    fbe_persist_transaction_handle_t        handle;
    fbe_semaphore_t                         sem;
    fbe_status_t                            wait_status;
    persist_service_struct_t *              odt_data = NULL;
    fbe_persist_sector_type_t               target_sector =  FBE_PERSIST_SECTOR_TYPE_INVALID;
    fbe_u32_t                               count =0;
    fbe_u32_t                               entry_count =0;
    fbe_persist_entry_id_t                  entry;
    fbe_bool_t                              data_entry = FBE_FALSE;
    
    argc--;
    argv++;

    if( argc == 0){
        fbe_cli_printf("odt: ERROR: Not enough arguments\n");
        fbe_cli_printf("%s", ODT_USAGE);
        return status;
    }

    /*Extract the command*/
    while (argc > 0) {
        if (strcmp(*argv, "-e") == 0) {
            argc--;
            argv++;

            status = check_entry_param(argc, argv);
            if(status != FBE_STATUS_OK){
                return status;
            }
            break;
        }

        /* Get sector type*/
        else if(strcmp(*argv,"-s") == 0) {
            argc--;
            argv++;

            status = fbe_cli_lib_odt_get_status(argc, argv, &target_sector, NULL);
            if(status != FBE_STATUS_OK){
                return status;
            }
            
            argc--;
            argv++;
        }

        else {
            fbe_cli_printf("odt:ERROR: Invalid Input !!\n");
            fbe_cli_printf("%s", ODT_USAGE);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    if(target_sector == FBE_PERSIST_SECTOR_TYPE_INVALID) {
        fbe_cli_printf("odt: ERROR: Invalid sector type!!\n");
        fbe_cli_printf("%s", ODT_USAGE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*Allocate the memory for writing the buffer*/
    odt_data = fbe_api_allocate_memory(sizeof(persist_service_struct_t));

    if(odt_data == NULL) {
        fbe_cli_printf("Failed to allocate memory for odt_data !\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(odt_data, sizeof (persist_service_struct_t));
    
    odt_data->some_data = (fbe_u8_t*) fbe_api_allocate_memory(FBE_CLI_ODT_ENTRY_SIZE * sizeof(fbe_u8_t));

    if(odt_data->some_data == NULL) {
        fbe_cli_printf("Failed to allocate memory for odt_data->some_data!\n");
        fbe_cli_lib_odt_cleanup(odt_data);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(odt_data->some_data, (FBE_CLI_ODT_ENTRY_SIZE*sizeof(fbe_u8_t)));
    
    /*Initialize semaphore*/
    fbe_semaphore_init(&sem, 0, 1);

    /* Start the transaction*/
    if((status = fbe_api_persist_start_transaction(&handle)) !=FBE_STATUS_OK){
        fbe_cli_printf("odt: ERROR: Can not start transaction\n");
        fbe_cli_lib_odt_cleanup(odt_data);
        fbe_semaphore_destroy(&sem);
        return status;
    }

    while(argc>=0){
       if(entry_count ==FBE_CLI_ODT_TRAN_ENTRY_MAX) {
            entry_count = 0;
            status = fbe_api_persist_commit_transaction(handle,  commit_completion_function,
                                                       (fbe_persist_completion_context_t)&sem);

           wait_status = fbe_semaphore_wait_ms(&sem, 10000);
           if(wait_status != FBE_STATUS_OK){
               fbe_cli_printf("odt: ERROR: Semaphore error\n");
               fbe_cli_lib_odt_cleanup(odt_data);
               fbe_semaphore_destroy(&sem);
               return status;
            }
            if((status = fbe_api_persist_start_transaction(&handle)) !=FBE_STATUS_OK){
                fbe_cli_printf("odt: ERROR: Can not start transaction\n");
                fbe_cli_lib_odt_cleanup(odt_data);
                return status;
            }
        }

        if((argc == 0) || (strcmp(*argv, "-e")==0)){
            entry_count ++;

            odt_data->persist_header.entry_id = target_sector;
            odt_data->persist_header.entry_id <<=32;
            odt_data->persist_header.entry_id |= entry;

            /*Modify the data*/
            status = fbe_api_persist_modify_entry(handle, 
                                                 (fbe_u8_t *) (odt_data->some_data),
                                                 FBE_CLI_ODT_ENTRY_SIZE,
                                                 odt_data->persist_header.entry_id);
            if(status != FBE_STATUS_OK){
                fbe_cli_printf("odt: ERROR: Can not proceed transaction\n");
                status = fbe_api_persist_abort_transaction(handle);
                fbe_cli_lib_odt_cleanup(odt_data);
                fbe_semaphore_destroy(&sem);
                return FBE_STATUS_GENERIC_FAILURE;
            }

           /*Display the output*/
           fbe_cli_printf("\nThe entry %llu in target sector %s modified successfully!!\n",
                          (unsigned long long)entry,
			  convert_target_name_to_string(target_sector));
           fbe_cli_printf("-----------------------------------------------------------\n");

           if(argc == 0){
              break;
           }

           data_entry = FBE_FALSE;
           count = 0;
           fbe_zero_memory( odt_data->some_data, (FBE_CLI_ODT_ENTRY_SIZE * sizeof(fbe_u8_t)));
           argc--;
           argv++;
        }

        if(strcmp(*argv, "-d") == 0){
            data_entry =FBE_TRUE;
            argc--;
            argv++;
        }

        if(data_entry){
            odt_data->some_data[count] = fbe_atoh(*argv);
            if(  odt_data->some_data[count]==FBE_CLI_ODT_DATA_BOGUS){
                fbe_cli_error("Invalid data byte!!\n");
                fbe_cli_lib_odt_cleanup(odt_data);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            count++;
        }
        else {
                status = fbe_cli_lib_odt_get_single_entry(argc, argv, &entry);
                if(status != FBE_STATUS_OK){
                    fbe_cli_lib_odt_cleanup(odt_data);
                    return status;
                }
        }
        argc--;
        argv++;
    }//while loop

    /*Commit the transaction */
    status = fbe_api_persist_commit_transaction(handle,  commit_completion_function,
        (fbe_persist_completion_context_t)&sem);
    if(status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        fbe_cli_printf("odt: ERROR: Error in commit\n");
        status = fbe_api_persist_abort_transaction(handle);
        fbe_cli_lib_odt_cleanup(odt_data);
        fbe_semaphore_destroy(&sem);
        return status;
    }

    wait_status = fbe_semaphore_wait_ms(&sem, 10000);
    if(wait_status != FBE_STATUS_OK){
        fbe_cli_printf("odt: TIMEOUT ERROR: Timeout at write entry\n");
        fbe_semaphore_destroy(&sem);
        fbe_cli_lib_odt_cleanup(odt_data);
        return status;
    }
    
    fbe_cli_printf("odt: SUCCESS: The data modified successfully!!\n\n");
    fbe_semaphore_destroy(&sem);
    fbe_cli_lib_odt_cleanup(odt_data);
    return status;
}

/****************************************************************
* fbe_cli_lib_odt_single_entry_delete(fbe_persist_transaction_handle_t transaction_handle, 
                                      fbe_persist_entry_id_t entry_id)
****************************************************************
* @brief
* Delete the given single entry at a time.
*
*@return fbe_status_t status of the operation.
*
* @revision
* * 07/25/2011: created:  Shubhada Savdekar
*
****************************************************************/
fbe_status_t  fbe_cli_lib_odt_single_entry_delete(fbe_u32_t argc, fbe_char_t **argv)
{
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_status_t                           wait_status;
    persist_service_struct_t *             odt_temp = NULL;
    fbe_persist_sector_type_t              target_sector =  FBE_PERSIST_SECTOR_TYPE_INVALID;
    single_entry_context_t      single_entry_context;
    fbe_persist_entry_id_t                 entry = FBE_CLI_ODT_MINUS_ONE;

    argc--;
    argv++;

    if( argc == 0){
        fbe_cli_printf("Not enough arguments\n");
        fbe_cli_printf("%s", ODT_USAGE);
        return status;
    }

    /*Extract the command*/
    while(argc > 0) {
        /*Get entry id from the user*/
        if (strcmp(*argv, "-e") == 0) {
            argc--;
            argv++;

            status = fbe_cli_lib_odt_get_single_entry(argc, argv, &entry);
            if(status != FBE_STATUS_OK){
                return status;
            }

            argc--;
            argv++;
        }

        /* Get sector type*/
        else if(strcmp(*argv,"-s") == 0) {
            argc--;
            argv++;

            status = fbe_cli_lib_odt_get_status(argc, argv, &target_sector, NULL);
            if(status != FBE_STATUS_OK){
                return status;
            }
            
            argc--;
            argv++;
        }

        else {
            fbe_cli_printf("odt: ERROR: Invalid Input !!\n");
            fbe_cli_printf("%s", ODT_USAGE);
            return status;
        }
    }//end of while

    if(entry == FBE_CLI_ODT_MINUS_ONE){
        fbe_cli_printf("odt: ERROR: Invalid Input!!\n");
        fbe_cli_printf("%s", ODT_USAGE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if(target_sector == 0) {
        fbe_cli_printf("odt: ERROR: Invalid Sector type!!\n");
        fbe_cli_printf("%s", ODT_USAGE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    odt_temp = fbe_api_allocate_memory(sizeof(persist_service_struct_t));

    if(odt_temp == NULL) {
        fbe_cli_printf("Failed to allocate memory for odt_temp!\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_zero_memory(odt_temp, sizeof (persist_service_struct_t));

    /*Add target sector and requested entry to odt_temp->persist_header.entry_id  */
    single_entry_context.received_entry_id = target_sector;
    single_entry_context.received_entry_id <<=32;
    single_entry_context.received_entry_id |=entry;

    fbe_semaphore_init(&single_entry_context.sem, 0, 1);
    single_entry_context.expect_error = FBE_FALSE;

    status  = fbe_api_persist_delete_single_entry(single_entry_context.received_entry_id,
                                                  single_operation_completion,
                                                  &single_entry_context);

    if(status == FBE_STATUS_GENERIC_FAILURE)
    {
        fbe_cli_error("odt ERROR:Error in deleting the single entry.\n");
        fbe_cli_lib_odt_cleanup(odt_temp);
        fbe_semaphore_destroy(&single_entry_context.sem);
        return status;
    }

    wait_status = fbe_semaphore_wait_ms(&single_entry_context.sem, 10000);
    if(wait_status == FBE_STATUS_TIMEOUT)
    {
        fbe_cli_error("odt ERROR:Timeout error in deleting the single entry.\n");
        fbe_cli_lib_odt_cleanup(odt_temp);
        fbe_semaphore_destroy(&single_entry_context.sem);
        return status;
    }

    fbe_semaphore_destroy(&single_entry_context.sem);
    fbe_cli_lib_odt_cleanup(odt_temp);

    fbe_cli_printf("odt: SUCCESS:The data entry deleted successfully!!\n\n");
    return status;
}

/****************************************************************
* fbe_cli_lib_odt_delete_entries(fbe_u32_t argc, fbe_char_t**argv)
****************************************************************
* @brief
* Delete the multiple entries.
*
*@return fbe_status_t status of the operation.
*
* @revision
* * 05/25/2011: created:  Shubhada Savdekar
*
****************************************************************/
fbe_status_t fbe_cli_lib_odt_delete_entries(fbe_u32_t argc, fbe_char_t**argv)
{
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_status_t                           wait_status;
    fbe_persist_transaction_handle_t       handle;
    persist_service_struct_t *             odt_temp = NULL;
    fbe_persist_sector_type_t              target_sector;
    fbe_persist_entry_id_t                 entry = -1;
    fbe_semaphore_t                        sem;
    fbe_u32_t    entry_count =0;    

    argc--;
    argv++;

    if( argc == 0){
        fbe_cli_printf("Not enough arguments\n");
        fbe_cli_printf("%s", ODT_USAGE);
        return status;
    }

    /*Extract the command*/
    while(argc > 0) {
        /*Get entry id from the user*/
        if (strcmp(*argv, "-e") == 0) {
            argc--;
            argv++;

            status = check_entry_param(argc, argv);
            if(status != FBE_STATUS_OK){
                return status;
            }

            break;
        }
        /* Get sector type*/
        else if(strcmp(*argv,"-s") == 0) {
            argc--;
            argv++;

            status = fbe_cli_lib_odt_get_status(argc, argv, &target_sector, NULL);
            if(status != FBE_STATUS_OK){
                return status;
            }
            
            argc--;
            argv++;
        }
        else {
            fbe_cli_printf("odt: ERROR: Invalid Input !!\n");
            fbe_cli_printf("%s", ODT_USAGE);
            return status;
        }
    }

    if(argc == 0){
            fbe_cli_printf("odt: ERROR: Invalid Input !!\n");
            fbe_cli_printf("%s", ODT_USAGE);
            return status;
    }
    
    if(target_sector == FBE_PERSIST_SECTOR_TYPE_INVALID) {
        fbe_cli_printf("odt: ERROR: Invalid sector type!!\n");
        fbe_cli_printf("%s", ODT_USAGE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    odt_temp = fbe_api_allocate_memory(sizeof(persist_service_struct_t));

    if(odt_temp == NULL) {
        fbe_cli_printf("Failed to allocate memory for odt_temp !\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_zero_memory(odt_temp, sizeof (persist_service_struct_t));

    /*Initialize the semaphore*/
    fbe_semaphore_init(&sem, 0, 1);

    /* Start the transaction*/
    if((status = fbe_api_persist_start_transaction(&handle)) !=FBE_STATUS_OK){
        fbe_cli_printf("odt: ERROR: Can not start transaction\n");
        fbe_cli_lib_odt_cleanup(odt_temp);
        fbe_semaphore_destroy(&sem);
        return status;
    }

    /*Get entries from the user*/
    while(argc >= 0){
        if(entry_count ==FBE_CLI_ODT_TRAN_ENTRY_MAX) {
            entry_count = 0;
            status = fbe_api_persist_commit_transaction(handle,  commit_completion_function,
                                                       (fbe_persist_completion_context_t)&sem);
            if(status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
                fbe_cli_printf("odt: ERROR: Error in commit\n");
                status = fbe_api_persist_abort_transaction(handle);
                fbe_semaphore_destroy(&sem);
                fbe_cli_lib_odt_cleanup(odt_temp);
                return status;
            }

           wait_status = fbe_semaphore_wait_ms(&sem, 10000);
           if(wait_status != FBE_STATUS_OK){
               fbe_cli_printf("odt: ERROR: Semaphore error\n");
               fbe_cli_lib_odt_cleanup(odt_temp);
               fbe_semaphore_destroy(&sem);
               return status;
            }
            if((status = fbe_api_persist_start_transaction(&handle)) !=FBE_STATUS_OK){
                fbe_cli_printf("odt: ERROR: Can not start transaction\n");
                fbe_cli_lib_odt_cleanup(odt_temp);
                return status;
            }
        }
        
        if((argc == 0) || (strcmp(*argv, "-e")==0)){
            entry_count ++;

            odt_temp->persist_header.entry_id = target_sector;
            odt_temp->persist_header.entry_id <<=32;
            odt_temp->persist_header.entry_id |= entry;

            /*Delete the data*/
            status = fbe_api_persist_delete_entry(handle, odt_temp->persist_header.entry_id);
    
            if(status != FBE_STATUS_OK){
                fbe_cli_printf("odt: ERROR: Can not delete the entry !!\n");
                fbe_cli_printf("%s", ODT_USAGE);
                status = fbe_api_persist_abort_transaction(handle);
                fbe_cli_lib_odt_cleanup(odt_temp);
                fbe_semaphore_destroy(&sem);
                return status;
            }

            /*Display the output*/
            fbe_cli_printf("\nThe entry %llu deleted successfully from target sector %s.\n",
                           (unsigned long long)entry,
			   convert_target_name_to_string(target_sector));
            fbe_cli_printf("-----------------------------------------------------------\n");

            if(argc == 0){
                break;
            }
               
            argc--;
            argv++;
        }

        if((argc ==0) || (strcmp(*argv, "-e")==0)){
            break;
        }

        status = fbe_cli_lib_odt_get_single_entry(argc, argv, &entry);
        if(status != FBE_STATUS_OK){
            fbe_cli_lib_odt_cleanup(odt_temp);
            return status;
        }
        argc--;
        argv++;
    }/*end of while*/

    /*Commit the transaction */
    status = fbe_api_persist_commit_transaction(handle,  commit_completion_function,
                                               (fbe_persist_completion_context_t)&sem);
    if(status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        fbe_cli_printf("odt: ERROR: Error in commit.\n");
        status = fbe_api_persist_abort_transaction(handle);
        fbe_semaphore_destroy(&sem);
        return status;
    }

    wait_status = fbe_semaphore_wait_ms(&sem, 10000);
    if(wait_status != FBE_STATUS_OK){
        fbe_cli_printf("odt: ERROR: Semaphore error\n");
        fbe_semaphore_destroy(&sem);
        fbe_cli_lib_odt_cleanup(odt_temp);
        return status;
    }
    fbe_semaphore_destroy(&sem);

    fbe_cli_lib_odt_cleanup(odt_temp);
    fbe_cli_printf("odt: SUCCESS:The data entry deleted successfully!!\n\n");

    return status;
}

/****************************************************************
*fbe_cli_lib_odt_display_entry_ids_and_sectors()
****************************************************************
* @brief
*  This function is used to display the last 512 write entries along 
*  with their target sectors.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
* @return - none.  
*
* @revision
* * 05/16/2011: created:  Shubhada Savdekar
*
****************************************************************/
void fbe_cli_lib_odt_display_entry_ids_and_sectors()
{
    fbe_persist_entry_id_t            entry_id;
    fbe_u32_t                         count;

    fbe_cli_printf("----------------------------------------------------\n");
    fbe_cli_printf("|  Entry_id  |   Sector Type                    |\n");
    fbe_cli_printf("----------------------------------------------------\n");

    for (count = 0; count < FBE_CLI_ODT_MAX_ENTRIES; count ++) {
        entry_id = odt_header[count].entry_id;
        if(strcmp("INVALID", convert_target_name_to_string(entry_id)) == 0){
            break;
        }

        fbe_cli_printf("|    %llu     |", (unsigned long long)entry_id);

        entry_id &= 0xFFFFFFFF00000000;
        entry_id >>= 32;

        fbe_cli_printf("   %s    |\n", convert_target_name_to_string(entry_id));
    }
    fbe_cli_printf("----------------------------------------------------\n\n");
}

/****************************************************************
* fbe_cli_lib_odt_write_entry_with_auto_entry_id_on_top(
* fbe_u32_t argc, fbe_char_t**argv)
****************************************************************
* @brief
*  This function is used to perform WRITE operation on the persist service 
* with entry added to the top.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
*@return fbe_status_t status of the operation.
*
* @revision
* * 07/24/2011: created:  Shubhada Savdekar
*
****************************************************************/
fbe_status_t fbe_cli_lib_odt_write_entry_with_auto_entry_id_on_top(
    fbe_u32_t argc, fbe_char_t**argv)
{
    fbe_status_t                            status=FBE_STATUS_OK;
    fbe_persist_transaction_handle_t        handle;
    fbe_semaphore_t                         sem;
    fbe_status_t                            wait_status;
    persist_service_struct_t *              odt_data = NULL;
    fbe_persist_sector_type_t               target_sector;
    fbe_u32_t                               count =0;
    fbe_u32_t                               entry_count =0;

    argc--;
    argv++;

    if( argc == 0){
        fbe_cli_printf("Not enough arguments\n");
        fbe_cli_printf("%s", ODT_USAGE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*Extract the command*/
    while (argc > 0) {
        if (strcmp(*argv, "-d") == 0) {
            argc--;
            argv++;
            status = check_data_param(argc, argv);
            if(status != FBE_STATUS_OK){
                return status;
            }
            break;
        }
        
        /* Get sector type*/
        else if(strcmp(*argv,"-s") == 0) {
            argc--;
            argv++;

            status = fbe_cli_lib_odt_get_status(argc, argv, &target_sector, NULL);
            if(status != FBE_STATUS_OK){
                return status;
            }
            argc--;
            argv++;
        }

        else {
            fbe_cli_printf("odt:ERROR: Invalid Input !!\n");
            fbe_cli_printf("%s", ODT_USAGE);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    if (argc <= 0) {
        fbe_cli_printf("odt: ERROR: Invalid Input !!\n");
        fbe_cli_printf("%s", ODT_USAGE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if(target_sector == 0) {
        fbe_cli_printf("%s", ODT_USAGE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*Allocate the memory for writing the buffer*/
    odt_data = fbe_api_allocate_memory(sizeof(persist_service_struct_t));

    if(odt_data == NULL) {
        fbe_cli_printf("Failed to allocate memory for odt_data!\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(odt_data, sizeof(persist_service_struct_t));
    
    odt_data->some_data = (fbe_u8_t*) fbe_api_allocate_memory(FBE_CLI_ODT_ENTRY_SIZE * sizeof(fbe_u8_t));

    if(odt_data->some_data == NULL) {
        fbe_cli_printf("Failed to allocate memory for odt_data->some_data!\n");
        fbe_cli_lib_odt_cleanup(odt_data);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(odt_data->some_data, (FBE_CLI_ODT_ENTRY_SIZE * sizeof(fbe_u8_t)));
    
    /*Initialize semaphore*/
    fbe_semaphore_init(&sem, 0, 1);

    /* Start the transaction*/
    if((status = fbe_api_persist_start_transaction(&handle)) !=FBE_STATUS_OK){
        fbe_cli_printf("odt: ERROR: Can not start transaction\n");
        fbe_cli_lib_odt_cleanup(odt_data);
        fbe_semaphore_destroy(&sem);
        return status;
    }

    /* get the data from the user*/
     while(argc >= 0) { 
        
         if(entry_count ==FBE_CLI_ODT_TRAN_ENTRY_MAX){
             entry_count = 0;
             status = fbe_api_persist_commit_transaction(handle,  commit_completion_function,
                                                           (fbe_persist_completion_context_t)&sem);
             if(status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
                fbe_cli_printf("odt: ERROR: Error in commit\n");
                status = fbe_api_persist_abort_transaction(handle);
                fbe_cli_lib_odt_cleanup(odt_data);
                fbe_semaphore_destroy(&sem);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            if((status = fbe_api_persist_start_transaction(&handle)) !=FBE_STATUS_OK){
                fbe_cli_printf("odt: ERROR: Can not start transaction\n");
                fbe_semaphore_destroy(&sem);
                fbe_cli_lib_odt_cleanup(odt_data);
                return status;
           }
        }

        if((argc == 0) || (strcmp(*argv, "-d")==0)){
            entry_count ++;
            /*Write the data*/
            status = fbe_api_persist_write_entry_with_auto_entry_id_on_top(handle, target_sector,
                                                                          (fbe_u8_t *)(odt_data->some_data),
                                                                          FBE_CLI_ODT_ENTRY_SIZE,
                                                                          &odt_data->persist_header.entry_id);
            if(status != FBE_STATUS_OK){
                fbe_cli_printf("odt: ERROR: Can not proceed transaction\n");
                status = fbe_api_persist_abort_transaction(handle);
                fbe_cli_lib_odt_cleanup(odt_data);
                fbe_semaphore_destroy(&sem);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /*Display the output*/
            fbe_cli_printf("\nThe write sector type is:    %s.\n", 
                                 convert_target_name_to_string( target_sector ));
            fbe_cli_printf("The received Entry_id is:    %llx\n",
			   (unsigned long long)odt_data->persist_header.entry_id);
            fbe_cli_printf("-----------------------------------------------------------\n");

            if(argc == 0){
               break;
            }
            argc--;
            argv++;
            count = 0;
            fbe_zero_memory( odt_data->some_data, (FBE_CLI_ODT_ENTRY_SIZE * sizeof(fbe_u8_t)));
        }
        if((argc ==0) || (strcmp(*argv, "-d")==0)){
            break;
        }
        odt_data->some_data[count] = fbe_atoh(*argv);
        if(odt_data->some_data[count] == FBE_CLI_ODT_DATA_BOGUS){
            fbe_cli_error("Invalid data byte!!\n");
            fbe_cli_lib_odt_cleanup(odt_data);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        count++;
        argc--;
        argv++;
    }/*End of While*/

    /*Commit the transaction */
    status = fbe_api_persist_commit_transaction(handle,  commit_completion_function,
        (fbe_persist_completion_context_t)&sem);
    if(status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        fbe_cli_printf("odt: ERROR: Error in commit\n");
        status = fbe_api_persist_abort_transaction(handle);
        fbe_cli_lib_odt_cleanup(odt_data);
        fbe_semaphore_destroy(&sem);
        return status;
    }
    
    wait_status = fbe_semaphore_wait_ms(&sem, 10000);
    if(wait_status != FBE_STATUS_OK){
        fbe_cli_printf("odt: TIMEOUT ERROR: Timeout at write entry\n");
        fbe_semaphore_destroy(&sem);
        fbe_cli_lib_odt_cleanup(odt_data);
        return status;
    }

    fbe_cli_printf("odt: SUCCESS: The data written successfully!!\n\n");
    odt_header[cli_lib_odt_index++].entry_id= odt_data->persist_header.entry_id;

    if(cli_lib_odt_index == FBE_CLI_ODT_MAX_ENTRIES){
        cli_lib_odt_index= 0;
    }

    fbe_semaphore_destroy(&sem);
    fbe_cli_lib_odt_cleanup(odt_data);
    return status;
}

/****************************************************************
*fbe_cli_lib_odt_write(fbe_u32_t argc, fbe_char_t**argv)
****************************************************************
* @brief
*  This function is used to perform WRITE operationon the persist service.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
*@return fbe_status_t status of the operation.
*
* @revision
* * 05/16/2011: created:  Shubhada Savdekar
*
****************************************************************/
fbe_status_t fbe_cli_lib_odt_single_entry_write(fbe_u32_t argc, fbe_char_t**argv)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_status_t                    wait_status;
    persist_service_struct_t *      odt_data = NULL;
    single_entry_context_t          single_entry_context;
    fbe_persist_sector_type_t       target_sector =  FBE_PERSIST_SECTOR_TYPE_INVALID;

    argc--;
    argv++;

    if( argc == 0){
        fbe_cli_printf("odt: ERROR: Not enough arguments\n");
        fbe_cli_printf("%s", ODT_USAGE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*Extract the command*/
    while (argc > 0) {
        if (strcmp(*argv, "-d") == 0) {
            argc--;
            argv++;

            /*Allocate the memory for writing the buffer*/
            odt_data = fbe_api_allocate_memory(sizeof(persist_service_struct_t));

            if(odt_data == NULL) {
                fbe_cli_printf("Failed to allocate memory for odt_data!\n");
                return FBE_STATUS_GENERIC_FAILURE;
            }

            fbe_zero_memory(odt_data, sizeof(persist_service_struct_t));
            
            odt_data->some_data = (fbe_u8_t*) fbe_api_allocate_memory(FBE_CLI_ODT_ENTRY_SIZE * sizeof(fbe_u8_t));

            if(odt_data->some_data == NULL) {
                fbe_cli_printf("Failed to allocate memory for odt_data->some_data!\n");
                fbe_cli_lib_odt_cleanup(odt_data);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            fbe_zero_memory(odt_data->some_data, (FBE_CLI_ODT_ENTRY_SIZE * sizeof(fbe_u8_t)));
            
            status = fbe_cli_lib_odt_get_data_for_single_entry(&argc, argv, odt_data);
            if(status != FBE_STATUS_OK){
                return status;
            }
        }
        
        /* Get sector type*/
        else if(strcmp(*argv,"-s") == 0) {
            argc--;
            argv++;

            status = fbe_cli_lib_odt_get_status(argc, argv, &target_sector, odt_data);
            if(status != FBE_STATUS_OK){
                return status;
            }
            
            argc--;
            argv++;
        }

        else {
            fbe_cli_printf("odt:ERROR: Invalid Input !!\n");
            fbe_cli_printf("%s", ODT_USAGE);
            fbe_cli_lib_odt_cleanup(odt_data);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    if(odt_data == NULL){
        fbe_cli_printf("odt: ERROR: Invalid Input!!\n");
        fbe_cli_printf("%s", ODT_USAGE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(target_sector == 0) {
        fbe_cli_printf("odt: ERROR: Invalid sector type!!\n");
        fbe_cli_printf("%s", ODT_USAGE);
        fbe_cli_lib_odt_cleanup(odt_data);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*Initialize the semaphore*/
    fbe_semaphore_init(&single_entry_context.sem, 0, 1);
    single_entry_context.expect_error = FBE_FALSE;

    /*write some stuff to the persist service as a single shot*/
    status  = fbe_api_persist_write_single_entry(target_sector, (fbe_u8_t *) (odt_data->some_data),
                                                 FBE_CLI_ODT_ENTRY_SIZE,
                                                 single_operation_completion,
                                                 &single_entry_context);

    if(status == FBE_STATUS_GENERIC_FAILURE)
    {
        fbe_cli_error("odt ERROR:Error in writing the single entry.\n");
        fbe_semaphore_destroy(&single_entry_context.sem);
        fbe_cli_lib_odt_cleanup(odt_data);
        return status;
    }


    wait_status = fbe_semaphore_wait_ms(&single_entry_context.sem, 10000);
    if(wait_status == FBE_STATUS_TIMEOUT)
    {
        fbe_cli_printf("odt: TIMEOUT ERROR: Timeout at write entry\n");
        fbe_semaphore_destroy(&single_entry_context.sem);
        fbe_cli_lib_odt_cleanup(odt_data);
        return status;
    }

    /*Display the output*/
    fbe_cli_printf("\nThe write sector type is:    %s.\n", 
    convert_target_name_to_string( target_sector ));
    fbe_cli_printf("The received Entry_id is:    %llx\n",
		   (unsigned long long)single_entry_context.received_entry_id);
    fbe_cli_printf("-----------------------------------------------------------\n");

    fbe_cli_printf("odt: SUCCESS: The data written successfully!!\n\n");
    odt_header[cli_lib_odt_index++].entry_id= single_entry_context.received_entry_id;

    if(cli_lib_odt_index == FBE_CLI_ODT_MAX_ENTRIES){
        cli_lib_odt_index= 0;
    }

    fbe_semaphore_destroy(&single_entry_context.sem);
    fbe_cli_lib_odt_cleanup(odt_data);
    return status;
}

/****************************************************************
*fbe_cli_lib_odt_write_entries(fbe_u32_t argc, fbe_char_t**argv)
****************************************************************
* @brief
*  This function is used to perform WRITE operationon the persist service.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
*@return fbe_status_t status of the operation.
*
* @revision
* * 06/16/2011: created:  Shubhada Savdekar
*
****************************************************************/
fbe_status_t fbe_cli_lib_odt_write_entries(fbe_u32_t argc, fbe_char_t**argv)
{

    fbe_status_t                            status=FBE_STATUS_OK;
    fbe_persist_transaction_handle_t        handle;
    fbe_semaphore_t                         sem;
    fbe_status_t                            wait_status;
    persist_service_struct_t*               odt_data = NULL;
    fbe_persist_sector_type_t               target_sector =  FBE_PERSIST_SECTOR_TYPE_INVALID;
    fbe_u32_t    count =0;
    fbe_u32_t    entry_count =0;

    argc--;
    argv++;

    if( argc == 0){
        fbe_cli_printf("Not enough arguments\n");
        fbe_cli_printf("%s", ODT_USAGE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*Extract the command*/
    while (argc > 0) {
        if (strcmp(*argv, "-d") == 0) {
            argc--;
            argv++;

            status = check_data_param(argc, argv);
            if(status != FBE_STATUS_OK){
                return status;
            }
            break;
        }
        
        /* Get sector type*/
        else if(strcmp(*argv,"-s") == 0) {
            argc--;
            argv++;

            status = fbe_cli_lib_odt_get_status(argc, argv, &target_sector, NULL);
            if(status != FBE_STATUS_OK){
                return status;
            }
            
            argc--;
            argv++;
        }

        else {
            fbe_cli_printf("odt:ERROR: Invalid Input !!\n");
            fbe_cli_printf("%s", ODT_USAGE);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    if (argc <= 0) {
        fbe_cli_printf("odt: ERROR: Invalid Input !!\n");
        fbe_cli_printf("%s", ODT_USAGE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if(target_sector == 0) {
        fbe_cli_printf("odt: ERROR: Invalid sector type!!\n");
        fbe_cli_printf("%s", ODT_USAGE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*Allocate the memory for writing the buffer*/
    odt_data = fbe_api_allocate_memory(sizeof(persist_service_struct_t));

    if(odt_data == NULL) {
        fbe_cli_printf("Failed to allocate memory for odt_data!\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_zero_memory(odt_data, sizeof(persist_service_struct_t));
    
    odt_data->some_data = (fbe_u8_t*) fbe_api_allocate_memory(FBE_CLI_ODT_ENTRY_SIZE * sizeof(fbe_u8_t));

    if(odt_data->some_data == NULL) {
        fbe_cli_printf("Failed to allocate memory for odt_data->some_data!\n");
        fbe_cli_lib_odt_cleanup(odt_data);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_zero_memory( odt_data->some_data, (FBE_CLI_ODT_ENTRY_SIZE * sizeof(fbe_u8_t)));

    /*Initialize semaphore*/
    fbe_semaphore_init(&sem, 0, 1);

    /* Start the transaction*/
    if((status = fbe_api_persist_start_transaction(&handle)) !=FBE_STATUS_OK){
        fbe_cli_printf("odt: ERROR: Can not start transaction\n");
        fbe_semaphore_destroy(&sem);
        fbe_cli_lib_odt_cleanup(odt_data);
        return status;
    }

    /* get the data from the user*/
    while(argc >= 0){ 
        if(entry_count ==FBE_CLI_ODT_TRAN_ENTRY_MAX) {
            entry_count = 0;
            status = fbe_api_persist_commit_transaction(handle,  commit_completion_function,
                                                       (fbe_persist_completion_context_t)&sem);
            if(status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
                fbe_cli_printf("odt: ERROR: Error in commit\n");
                status = fbe_api_persist_abort_transaction(handle);
                fbe_cli_lib_odt_cleanup(odt_data);
                fbe_semaphore_destroy(&sem);
                return status;
            }
            wait_status = fbe_semaphore_wait_ms(&sem, 10000);
            if(wait_status != FBE_STATUS_OK){
                fbe_cli_printf("odt: TIMEOUT ERROR: Timeout at commit transaction\n");
                fbe_cli_lib_odt_cleanup(odt_data);
                fbe_semaphore_destroy(&sem);
                return wait_status;
            }
            
            if((status = fbe_api_persist_start_transaction(&handle)) !=FBE_STATUS_OK){
                fbe_cli_printf("odt: ERROR: Can not start transaction\n");
                fbe_cli_lib_odt_cleanup(odt_data);
                fbe_semaphore_destroy(&sem);
                return status;
            }
        }

       if((argc == 0) || (strcmp(*argv, "-d")==0)){
           entry_count ++;
           /*Write the data*/
           status = fbe_api_persist_write_entry_with_auto_entry_id_on_top(handle, target_sector,
                                                (fbe_u8_t *) (odt_data->some_data),
                                                 FBE_CLI_ODT_ENTRY_SIZE,
                                                 &odt_data->persist_header.entry_id);
           if(status != FBE_STATUS_OK){
               fbe_cli_printf("odt: ERROR: Can not proceed transaction\n");
               status = fbe_api_persist_abort_transaction(handle);
               fbe_cli_lib_odt_cleanup(odt_data);
               fbe_semaphore_destroy(&sem);
               return FBE_STATUS_GENERIC_FAILURE;
           }

           /*Display the output*/
           fbe_cli_printf("\nThe write sector type is:    %s.\n", 
               convert_target_name_to_string( target_sector ));
           fbe_cli_printf("The received Entry_id is:    %llx\n",
                          (unsigned long long)odt_data->persist_header.entry_id);
           fbe_cli_printf("-----------------------------------------------------------\n");

           odt_header[cli_lib_odt_index++].entry_id= odt_data->persist_header.entry_id;
           if(cli_lib_odt_index == FBE_CLI_ODT_MAX_ENTRIES){
                cli_lib_odt_index= 0;
           }

           if(argc == 0){
              break;
           }
               
           argc--;
           argv++;
           count = 0;

           fbe_zero_memory( odt_data->some_data, (FBE_CLI_ODT_ENTRY_SIZE * sizeof(fbe_u8_t)));
       }

       if((argc ==0) || (strcmp(*argv, "-d")==0)){
           break;
       }
       odt_data->some_data[count] = fbe_atoh(*argv);
       if( odt_data->some_data[count] == FBE_CLI_ODT_DATA_BOGUS){
           fbe_cli_error("Invalid data byte!!\n");
           fbe_cli_lib_odt_cleanup(odt_data);
           return FBE_STATUS_GENERIC_FAILURE;
       }
       count++;
       argc--;
       argv++;
    }

    /*Commit the transaction */
    status = fbe_api_persist_commit_transaction(handle,  commit_completion_function,
        (fbe_persist_completion_context_t)&sem);
    if(status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        fbe_cli_printf("odt: ERROR: Error in commit\n");
        status = fbe_api_persist_abort_transaction(handle);
        fbe_cli_lib_odt_cleanup(odt_data);
        fbe_semaphore_destroy(&sem);
        return status;
    }
    wait_status = fbe_semaphore_wait_ms(&sem, 10000);
    if(wait_status != FBE_STATUS_OK){
        fbe_cli_printf("odt: TIMEOUT ERROR: Timeout at write entry\n");
        fbe_cli_lib_odt_cleanup(odt_data);
        fbe_semaphore_destroy(&sem);
        return status;
    }

    fbe_cli_printf("odt: SUCCESS: The data written successfully!!\n\n");

    fbe_semaphore_destroy(&sem);
    fbe_cli_lib_odt_cleanup(odt_data);
    return status;
}

/************************************************************************
 *   check_data_param(fbe_u32_t argc, fbe_char_t**argv)
 ************************************************************************
 *
 * @brief
 *   This function is used to validate the data entered on command line.
 *
 *  @param    argc     - argument count
 *  @param    argv     - argument string
 *
 * @return fbe_status_t status of the operation.
 *
 * @revision
 * 07/25/2011: Created: Shubhada Savdekar
 ************************************************************************/
fbe_status_t check_data_param(fbe_u32_t argc, fbe_char_t**argv){
    fbe_status_t     status=FBE_STATUS_OK;

    if (argc <= 0) {
        fbe_cli_printf("odt: ERROR: Invalid Input !!\n");
        fbe_cli_printf("%s", ODT_USAGE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if(strcmp(*argv, "-d")==0){
        fbe_cli_printf("odt: ERROR: Please give the data correct!!\n");
        fbe_cli_printf("%s", ODT_USAGE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************************************
* read_single_entry_completion_function(fbe_status_t read_status, 
*                                       fbe_persist_completion_context_t context)
****************************************************************
* @brief
*  This is the completion function for single entry read operation.
*
* @param    read_status - status of the read operation
* @param    context       - completion context
*
*
*@return fbe_status_t status of the operation.
*
* @revision
* * 05/16/2011: created:  Shubhada Savdekar
*
****************************************************************/
fbe_status_t single_operation_completion(fbe_status_t op_status,
                                        fbe_persist_entry_id_t entry_id,
                                        fbe_persist_completion_context_t context)
{
    single_entry_context_t *    single_entry_context = (single_entry_context_t *)context;

    if (single_entry_context->expect_error) {
        if( op_status != FBE_STATUS_GENERIC_FAILURE)
        {
            fbe_cli_error("odt Error: Unexpected op_status 0x%x.\n", op_status);
            return op_status;
        }
    }
    else{
        if( op_status != FBE_STATUS_OK) {
            fbe_cli_error("odt Error: Unexpected op_status 0x%x.\n", op_status);
            return op_status;
        }
    }
    single_entry_context->received_entry_id = entry_id;
    fbe_semaphore_release(&single_entry_context->sem, 0, 1 ,FALSE);

    return FBE_STATUS_OK;

}

/**************************************************************
* commit_completion_function (fbe_status_t commit_status, 
*                             fbe_persist_completion_context_t context)
****************************************************************
* @brief
*  This is the completion function for commit operation.
*
* @param    commit_status - status of the commit operation
* @param    context           - completion context
*
*@return fbe_status_t status of the operation.
*
* @revision
* * 05/16/2011: created:  Shubhada Savdekar
*
****************************************************************/

fbe_status_t commit_completion_function (fbe_status_t commit_status,
                                         fbe_persist_completion_context_t context)
{
    fbe_semaphore_t *    sem = (fbe_semaphore_t *)context;
    fbe_semaphore_release(sem, 0, 1 ,FALSE);
    return FBE_STATUS_OK;
}


/**************************************************************
*fbe_cli_lib_set_lun() 
****************************************************************
* @brief
*  This function is used to set a LUN for  the persist service.
*
*@return fbe_status_t status of the operation.
*
* @revision
* * 05/16/2011: created:  Shubhada Savdekar
*
****************************************************************/
fbe_status_t fbe_cli_lib_odt_set_lun()
{
    fbe_status_t             status = FBE_STATUS_OK;
    fbe_semaphore_t          sem;
    fbe_status_t             wait_status;

    fbe_semaphore_init(&sem, 0, 1);

    /*First Unset the LUN*/
    status = fbe_api_persist_unset_lun();
    if(status != FBE_STATUS_OK) {
        fbe_cli_printf("odt: ERROR: Error in setting the persist LUN");
        return status;
    }

    /*Set the LUN*/
    status = fbe_api_persist_set_lun(FBE_CLI_ODT_OBJECT_ID,  set_lun_completion_function, &sem);
    if((status ==  FBE_STATUS_GENERIC_FAILURE)) {
        fbe_cli_printf("odt: ERROR: Can not set LUN. Please wait !!\n");
        fbe_semaphore_destroy(&sem);
        return status;
    }

    wait_status = fbe_semaphore_wait_ms(&sem, 10000);
    if((wait_status ==  FBE_STATUS_TIMEOUT)) {
        fbe_cli_printf("odt: TIMEOUT ERROR: Timeout at  set LUN\n");
        fbe_semaphore_destroy(&sem);
        return status;
    }
    fbe_semaphore_destroy(&sem);
    fbe_cli_printf("odt: SUCCESS: The persist LUN set successfully!!\n");
    return status;
}

/**************************************************************
* set_lun_completion_function (fbe_status_t commit_status, 
*                              fbe_persist_completion_context_t context)
****************************************************************
* @brief
*  This is the completion function for commit operation.
*
* @param    commit_status - status of the commit operation
* @param    context           - completion context
*
*
*@return fbe_status_t status of the operation.
*
* @revision
* * 05/16/2011: created:  Shubhada Savdekar
*
****************************************************************/
static fbe_status_t set_lun_completion_function (fbe_status_t commit_status, 
                                                 fbe_persist_completion_context_t context)
{
    fbe_semaphore_t *  sem = (fbe_semaphore_t *)context;
    if(commit_status !=FBE_STATUS_OK)
    { 
        fbe_cli_printf("odt: ERROR: Commit status 0x%x is not OK in set lun completion function\n", commit_status);
        return commit_status;
    }
    fbe_semaphore_release(sem, 0, 1 ,FALSE);
    return FBE_STATUS_OK;
}

/**************************************************************
*fbe_cli_lib_unset_lun() 
****************************************************************
* @brief
*  This function is used to set a LUN for  the persist service.
*
*@return fbe_status_t status of the operation.
*
* @revision
* * 07/18/2011: created:  Shubhada Savdekar
*
****************************************************************/
fbe_status_t fbe_cli_lib_odt_unset_lun()
{
    fbe_status_t                    status= FBE_STATUS_INVALID ;

    status = fbe_api_persist_unset_lun();
    if(status == FBE_STATUS_OK)
    {
        fbe_cli_printf("odt: SUCCESS: The LUN unset successfully.\n");
    }
    else
    {
        fbe_cli_printf("odt: ERROR: Error in unset LUN.\n");
    }
    
    return status;
}

/****************************************************************
* convert_target_name_to_string( fbe_persist_sector_type_t target )
****************************************************************
* @brief
*  This function is used to conver the target type to corresponding string.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
*
*@return fbe_status_t status of the operation.
*
* @revision
* * 05/16/2011: created:  Shubhada Savdekar
*
****************************************************************/
static char* convert_target_name_to_string( fbe_persist_sector_type_t target )
{
    char *      target_name;

    switch(target){
        case 1:
           target_name = "FBE_PERSIST_SECTOR_TYPE_SEP_OBJECTS";
            break;
        case 2:
            target_name = "FBE_PERSIST_SECTOR_TYPE_SEP_EDGES";
            break;
        case 3:
            target_name = "FBE_PERSIST_SECTOR_TYPE_SEP_ADMIN_CONVERSION";
            break;
        case 4:
            target_name = "FBE_PERSIST_SECTOR_TYPE_ESP_OBJECTS";
            break;
        case 5:
            target_name = "FBE_PERSIST_SECTOR_TYPE_SYSTEM_GLOBAL_DATA";
            break;
        case 6:
            target_name = "FBE_PERSIST_SECTOR_TYPE_SCRATCH_PAD";
            break;
         case 7:
             target_name =  "FBE_PERSIST_SECTOR_TYPE_DIEH_RECORD";
             break;
         case 8:
             target_name =  "FBE_PERSIST_SECTOR_TYPE_LAST" ;
             break;
        default:
            target_name = "INVALID";
            break;
    }

   return target_name;
}

/****************************************************************
*  fbe_cli_lib_odt_init_buffer(void) 
****************************************************************
* @brief
*  This function is used to initialize the buffer that maintains the entries of 
*  last 512 write operations.
*
*
*  @param    void
*
*
*@return fbe_status_t status of the operation.
*
* @revision
* * 05/16/2011: created:  Shubhada Savdekar
*
****************************************************************/
static void  fbe_cli_lib_odt_init_buffer(void)
{
    fbe_u32_t       count = 0;

    /*create 512 entries*/    
    for (count = 0; count < FBE_CLI_ODT_MAX_ENTRIES; count ++) {
        odt_header[count].entry_id = 0xFFFFFFFFFFFFFFFF;
    }
    cli_lib_odt_index =0 ;
}

/****************************************************************
*fbe_cli_lib_odt_get_layout_info (void) 
****************************************************************
* @brief
*  This function is used to perform WRITE operationon the persist service.
*
*  @param    void
*
*
*@return fbe_status_t status of the operation.
*
* @revision
* * 05/16/2011: created:  Shubhada Savdekar
*
****************************************************************/
fbe_status_t  fbe_cli_lib_odt_get_layout_info (void)
{
    fbe_persist_control_get_layout_info_t    get_info;
    fbe_status_t                             status = FBE_STATUS_INVALID;
    fbe_lba_t                                   total_blocks;

    /*Get total blocks needed for the persist LUN.*/
    status  = fbe_api_persist_get_total_blocks_needed(&total_blocks);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("odt: ERROR: Failed to get total blocks needed for persist LUN.\n");
        return status;
    }

    status = fbe_api_persist_get_layout_info(&get_info);

    if (status != FBE_STATUS_OK) {
        fbe_cli_error("odt: ERROR: Failed to get layout info from persistence service\n");
        return status;
    }

    if (get_info.lun_object_id == FBE_OBJECT_ID_INVALID) {
         fbe_cli_printf("\nPersist LUN object ID:LUN NOT SET !\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(get_info.journal_start_lba != FBE_PERSIST_START_LBA +1) {
        fbe_cli_printf("\nWrong journal start LBA!\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_cli_printf("\nPersist LUN object ID:%d\n", get_info.lun_object_id);
    fbe_cli_printf("Persist LUN will need 0x%llX blocks\n",
		   (unsigned long long)total_blocks);
    fbe_cli_printf("----------------------------------------------------\n\n");
    fbe_cli_printf("Start of Journal lba:..................... .0x%llX\n",
		   (unsigned long long)get_info.journal_start_lba);
    fbe_cli_printf("Start of SEP Objects lba:...................0x%llX\n",
		   (unsigned long long)get_info.sep_object_start_lba);
    fbe_cli_printf("Start of SEP Edges lba:.....................0x%llX\n",
		   (unsigned long long)get_info.sep_edges_start_lba);
    fbe_cli_printf("Start of SEP Admin Conversion Table lba:....0x%llX\n",
		   (unsigned long long)get_info.sep_admin_conversion_start_lba);
    fbe_cli_printf("Start of SEP Objects lba:...................0x%llX\n",
		   (unsigned long long)get_info.esp_objects_start_lba);
    fbe_cli_printf("Start of Global system data lba:........... 0x%llX\n\n",
		   (unsigned long long)get_info.system_data_start_lba);
    fbe_cli_printf("----------------------------------------------------\n");

    return status;
}

/****************************************************************
 *  fbe_cli_odt_print_buffer_contents()
 ****************************************************************
 *
 * @brief
 *  Print the contents of the buffer
 *
 * @param sg_ptr       - Buffer which contains the data to be 
 *                       printed.
 * @param disk_address - Starting disk address in sectors.
 * @param blocks       - Blocks to display.
 * @param block_size   - The logical block size for this request
 *
 * @return NONE
 *
 * @revision
 *  
 *
 ****************************************************************/

void fbe_cli_odt_print_buffer_contents(fbe_sg_element_t *sg_ptr,  fbe_lba_t disk_address,
                                 fbe_u32_t blocks,  fbe_u32_t block_size)
{
     fbe_u32_t count = 0, orig_count = 0;
     fbe_char_t *addr;
     fbe_u32_t words;
     fbe_bool_t warning=FALSE;
     fbe_u32_t current_block = 0;

     /* Sanity check the block size.
      */
     if ((block_size > FBE_PERSIST_BYTES_PER_BLOCK) 
         || (block_size < FBE_PERSIST_DATA_BYTES_PER_BLOCK))
     {
         fbe_cli_error(" %s: block_size: %d not supported.\n",
                __FUNCTION__, block_size);
         return;
     }

     /* Display the block using ddt display sector.
      */
     while (sg_ptr->count)
     {
         addr = sg_ptr->address;
         count = sg_ptr->count;

         while (count > FBE_CLI_ODT_ZERO)       /* One loop pass per sector */
         {
             if (count < block_size)
             {
                 warning = TRUE;
                 orig_count = count;
                 words = (count + FBE_CLI_ODT_DUMMYVAL) / FBE_CLI_ODT_COLUMN_SIZE; /* round up to a whole word */
             }
             else
             {
                 words = block_size / FBE_CLI_ODT_COLUMN_SIZE;
             }
      
             fbe_cli_odt_display_sector(addr,words);
             addr += block_size;
             disk_address++;
             count -= block_size;

             current_block ++;
          
             if ( current_block >= blocks )
             {
                 /* We displayed more than required, exit.
                  */
                 return;
             }
          
             if (warning == TRUE)
             {
                 fbe_cli_printf("Warning:  SG element is not not a multiple"
                     "of block_size %d bytes.\n", block_size);
                 fbe_cli_printf("          count mod block_size"
                    " = %d\n\n", orig_count);
             }
         
         } /* end while there are more blocks in the sg entry*/
         sg_ptr++;
     }
     return;
} 

/************************************************************************
 *  fbe_cli_odt_display_sector()
 ************************************************************************
 *
 * @brief
 *   This function displays ONE SECTOR of data.  
 *
 * @param in_lba  - fru LBA
 * @param in_addr - buffer address
 * @param w_count - Number of 32-bit words to display
 *
 * @return (none)
 *
 * @revision
 * 
 ************************************************************************/

void fbe_cli_odt_display_sector(fbe_u8_t * in_addr, 
                                fbe_u32_t w_count)
{
    fbe_u32_t    *w_ptr;
    fbe_u32_t    offset = FBE_CLI_ODT_ZERO;

    w_ptr = (fbe_u32_t *)in_addr;

    while (w_count >= FBE_CLI_ODT_COLUMN_SIZE)
    {
        fbe_cli_printf("%08X %08X %08X %08X %08X %08X %08X %08X \n",
        *w_ptr, *(w_ptr + FBE_CLI_ODT_W_COUNT_ONE), *(w_ptr + FBE_CLI_ODT_W_COUNT_TWO), *(w_ptr + FBE_CLI_ODT_W_COUNT_THREE), *(w_ptr + FBE_CLI_ODT_W_COUNT_FOUR),
        *(w_ptr + FBE_CLI_ODT_W_COUNT_FIVE), *(w_ptr + FBE_CLI_ODT_W_COUNT_SIX), *(w_ptr + FBE_CLI_ODT_W_COUNT_SEVEN));
        
        offset += FBE_CLI_ODT_COLUMN_SIZE*FBE_CLI_ODT_COLUMN_SIZE;
        w_ptr += FBE_CLI_ODT_COLUMN_SIZE;
        w_count -= FBE_CLI_ODT_COLUMN_SIZE;
    }
    return;
}

/************************************************************************
 *  fbe_cli_lib_odt_get_target_name(fbe_char_t**argv, fbe_persist_sector_type_t* target_sector)
 ************************************************************************
 *
 * @brief
 *   This function displays ONE SECTOR of data.  
 *
 * @param in_lba  - fru LBA
 * @param in_addr - buffer address
 * @param w_count - Number of 32-bit words to display
 *
 * @return (none)
 *
 * @revision
 * 
 ************************************************************************/
void fbe_cli_lib_odt_get_target_name(fbe_char_t**argv, fbe_persist_sector_type_t* target_sector)
{
    if (strcmp(*argv , "sep_obj") == 0) {
        *target_sector =  FBE_PERSIST_SECTOR_TYPE_SEP_OBJECTS;
    }
    else if (strcmp(*argv , "sep_edge") == 0) {
        *target_sector =  FBE_PERSIST_SECTOR_TYPE_SEP_EDGES;
    }
    else if (strcmp(*argv , "sep_admin") == 0) {
        *target_sector =  FBE_PERSIST_SECTOR_TYPE_SEP_ADMIN_CONVERSION;
    }
    else if (strcmp(*argv , "esp_obj") == 0) {
        *target_sector =  FBE_PERSIST_SECTOR_TYPE_ESP_OBJECTS;
    }
    else if (strcmp(*argv , "sys") == 0) {
        *target_sector =  FBE_PERSIST_SECTOR_TYPE_SYSTEM_GLOBAL_DATA;
    }
    else if (strcmp(*argv , "scratch_pad") == 0) {
        *target_sector =  FBE_PERSIST_SECTOR_TYPE_SCRATCH_PAD;
    }
    else if (strcmp(*argv , "dieh") == 0) {
        *target_sector =  FBE_PERSIST_SECTOR_TYPE_DIEH_RECORD;
    }
    else if (strcmp(*argv , "last") == 0) {
        *target_sector =  FBE_PERSIST_SECTOR_TYPE_LAST;
    }
    else {
        *target_sector =     FBE_PERSIST_SECTOR_TYPE_INVALID;
    }
}

/************************************************************************
 *   fbe_cli_lib_odt_get_status(fbe_u32_t argc, fbe_char_t**argv,
 *      fbe_persist_sector_type_t*   target_sector_p, 
 *      persist_service_struct_t *   odt_data)
 ************************************************************************
 *
 * @brief
 *   This function is used to get the status from a command line.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *  @param    target_sector_p - pointer to target sector
 *  @param    odt_data - pointer to the odt data structure
 *
 * @return fbe_status_t status of the operation.
 *
 * @revision
 * 
 ************************************************************************/
fbe_status_t fbe_cli_lib_odt_get_status(fbe_u32_t argc,
                                       fbe_char_t**argv,
                                       fbe_persist_sector_type_t* target_sector_p,
                                       persist_service_struct_t *   odt_data)
{
    fbe_status_t status = FBE_STATUS_OK;

    if (argc <= 0) {
        fbe_cli_printf("odt: ERROR: Invalid Input !!\n");
        fbe_cli_printf("%s", ODT_USAGE);
        fbe_cli_lib_odt_cleanup(odt_data);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_cli_lib_odt_get_target_name(argv,target_sector_p);

     if(*target_sector_p ==   FBE_PERSIST_SECTOR_TYPE_INVALID)
    {
        fbe_cli_printf("odt: ERROR: Invalid Input !!\n");
        fbe_cli_printf("%s", ODT_USAGE);
        fbe_cli_lib_odt_cleanup(odt_data);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}

/************************************************************************
 *   fbe_cli_lib_odt_get_data_for_single_entry(fbe_u32_t* argc,
 *       fbe_char_t**argv, 
 *       persist_service_struct_t *   odt_data) 
 ************************************************************************
 *
 * @brief
 *   This function is used to get the data for the entry from a command line.
 *
 *  @param    *argc - pointer to argument count
 *  @param    argv - argument string
 *  @param    odt_data - pointer to the odt data structure
 *
 * @return fbe_status_t status of the operation.
 *
 * @revision
 * 
 ************************************************************************/

fbe_status_t fbe_cli_lib_odt_get_data_for_single_entry(fbe_u32_t* argc,
                                                      fbe_char_t**argv,
                                                      persist_service_struct_t * odt_data)
{
    fbe_u32_t          count =0;
    fbe_status_t       status = FBE_STATUS_OK;

    if ((*argc) <= 0) {
        fbe_cli_printf("odt: ERROR: Invalid Input !!\n");
        fbe_cli_printf("%s", ODT_USAGE);
        fbe_cli_lib_odt_cleanup(odt_data);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the data from the user*/
    while((*argc) > 0){
        odt_data->some_data[count] = fbe_atoh(*argv);
        if((strcmp(*argv, "-d") == 0) || ( odt_data->some_data[count] == FBE_CLI_ODT_DATA_BOGUS)){
            fbe_cli_error("Invalid data!!\n");
            fbe_cli_lib_odt_cleanup(odt_data);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        count++;
        (*argc)--;
        argv++;
    }
    return status;
}

/************************************************************************
 *   fbe_cli_lib_odt_get_single_entry(fbe_u32_t argc, fbe_char_t**argv, 
 *       fbe_persist_entry_id_t*  entry_p)
 ************************************************************************
 *
 * @brief
 *   This function is used to get the entry number from a command line.
 *
 *  @param    argc     - argument count
 *  @param    argv     - argument string
 *  @param    entry_p - pointer to the entry variable
 *
 * @return fbe_status_t status of the operation.
 *
 * @revision
 * 07/25/2011: Created: Shubhada Savdekar
 ************************************************************************/
fbe_status_t fbe_cli_lib_odt_get_single_entry(fbe_u32_t argc, fbe_char_t**argv, fbe_persist_entry_id_t*  entry_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    if (argc <= 0) {
        fbe_cli_printf("odt: ERROR: Invalid Input !!\n");
        fbe_cli_printf("%s", ODT_USAGE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_atoh64((*argv), entry_p);
    if(status != FBE_STATUS_OK){
        fbe_cli_printf("odt: ERROR: Invalid Input !!\n");
        fbe_cli_printf("%s", ODT_USAGE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}

/************************************************************************
 *   check_entry_param(fbe_u32_t argc, fbe_char_t**argv)
 ************************************************************************
 *
 * @brief
 *   This function is used to check the entry number entered on command line.
 *
 *  @param    argc     - argument count
 *  @param    argv     - argument string
 *
 * @return fbe_status_t status of the operation.
 *
 * @revision
 * 07/25/2011: Created: Shubhada Savdekar
 ************************************************************************/
fbe_status_t check_entry_param(fbe_u32_t argc, fbe_char_t**argv)
{
    fbe_status_t     status=FBE_STATUS_OK;

    if (argc <= 0) {
        fbe_cli_printf("odt: ERROR: Invalid Input !!\n");
        fbe_cli_printf("%s", ODT_USAGE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if(strcmp(*argv, "-e")==0){
        fbe_cli_printf("odt: ERROR: Please give the entry id correct!!\n");
        fbe_cli_printf("%s", ODT_USAGE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;

}

/****************************************************************
*fbe_cli_lib_odt_cleanup(persist_service_struct_t *odt_data)
****************************************************************
* @brief
*  This function perform any cleanup.
*
*  @param    odt_data
*
* @return None
*
* @revision
* * 01/28/2014: Created:  Sandeep Chaudhari
*
****************************************************************/
void fbe_cli_lib_odt_cleanup(persist_service_struct_t *odt_data)
{
    if(odt_data != NULL)
    {
        if(odt_data->some_data != NULL)
        {
            fbe_api_free_memory(odt_data->some_data);
            odt_data->some_data = NULL;
        }
        fbe_api_free_memory(odt_data);
        odt_data = NULL;
    }
}
