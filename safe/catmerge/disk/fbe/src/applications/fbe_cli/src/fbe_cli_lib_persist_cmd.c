/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_persist.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the function that dispaly and control the persistence service
 *
 * @ingroup fbe_cli
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
//#include <ctype.h>
#include "fbe_trace.h"
#include "fbe/fbe_class.h"
#include "fbe_cli_lib_persist_cmd.h"
#include "fbe/fbe_api_persist_interface.h"


static void fbe_cli_lib_persist_show_lba_info(void);
static void fbe_cli_lib_persist_add_hook(char * argv);
static void fbe_cli_lib_persist_remove_hook(char * argv);


/**************************************************************************
 * @fn fbe_cli_lib_persist_cmd
 *           (int argc , char ** argv)
 **************************************************************************
 *
 *  @brief
 *      This function calls different fbe_cli commands.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 *  @return   none
 *
 **************************************************************************/
void fbe_cli_lib_persist_cmd(int argc , char ** argv)
{

    if (argc == 0){
        fbe_cli_printf("%s", PERSIST_INFO_USAGE);
        return;
    }

    // Parse the command line
    while(argc > 0)
    {
        // Check the command type 
        if(strcmp(*argv, "-h") == 0){
            argc--;
            argv++;
            if(argc == 0){
                fbe_cli_printf("%s", PERSIST_INFO_USAGE);
                return;
            }
        }

        if(strcmp(*argv, "-info") == 0){
            argc--;
            argv++;
            if(argc == 0){
                fbe_cli_lib_persist_show_lba_info();
                return;
            }
        }
    }
   
    return;
} 

/*show lba layout and general information*/
static void fbe_cli_lib_persist_show_lba_info(void)
{
	fbe_status_t							status;
	fbe_persist_control_get_layout_info_t	info;

	status = fbe_api_persist_get_layout_info(&info);
	if (status != FBE_STATUS_OK) {
		fbe_cli_error("Failed to get layout info from persistence service\n");
		return;
	}

	if (info.lun_object_id != FBE_OBJECT_ID_INVALID) {
		fbe_cli_printf("\nPersistence LUN object ID:%d\n", info.lun_object_id);
	}else{
		fbe_cli_printf("\nPersistence LUN object ID:LUN NOT SET !\n");
	}

	fbe_cli_printf("Start of Journal lba:0x%llX\n",
		       (unsigned long long)info.journal_start_lba);
	fbe_cli_printf("Start of SEP Objects lba:0x%llX\n",
		       (unsigned long long)info.sep_object_start_lba);
	fbe_cli_printf("Start of SEP Edges lba:0x%llX\n",
		       (unsigned long long)info.sep_edges_start_lba);
        fbe_cli_printf("Start of SEP Admin Conversion Table lba:0x%llX\n",
		       (unsigned long long)info.sep_admin_conversion_start_lba);
	fbe_cli_printf("Start of ESP objects lba:0x%llX\n",
		       (unsigned long long)info.esp_objects_start_lba);
	fbe_cli_printf("Start of Global system data lba:0x%llX\n\n",
		       (unsigned long long)info.system_data_start_lba);

}

/**************************************************************************
 * @fn fbe_cli_persist_hook
 *           (int argc , char ** argv)
 **************************************************************************
 *
 *  @brief
 *      This function set/unset hook in persist service
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 *  @return   none
 *
 **************************************************************************/
void fbe_cli_persist_hook(int argc , char ** argv)
{

    if (argc == 0){
        fbe_cli_printf("%s", PERSIST_HOOK_USAGE);
        return;
    }

    // Parse the command line
    while(argc > 0)
    {
        // Check the command type 
        if(strcmp(*argv, "-h") == 0){
            argc--;
            argv++;
            if(argc == 0){
                fbe_cli_printf("%s", PERSIST_HOOK_USAGE);
                return;
            }
        }
        else if(strcmp(*argv, "-add_hook") == 0){
            argc--;
            argv++;

            fbe_cli_lib_persist_add_hook(*argv);
            argc--;
            argv++;

            return;
        }
        else if(strcmp(*argv, "-remove_hook") == 0){
            argc--;
            argv++;
        
            fbe_cli_lib_persist_remove_hook(*argv);
            argc--;
            argv++;
        
            return;
        }
        else{
            fbe_cli_error("%s", PERSIST_HOOK_USAGE);
            return;
        }
    }
   
    return;
}


static void fbe_cli_lib_persist_add_hook(char * argv)
{
    fbe_status_t    status = FBE_STATUS_INVALID;
    fbe_package_id_t    package = FBE_PACKAGE_ID_INVALID;

    if(strcmp(argv, "panic_writing_journal") == 0)
    {
        argv++;
        if(strcmp(argv, "sep") == 0)
            package = FBE_PACKAGE_ID_SEP_0;
        else if(strcmp(argv, "esp") == 0)
            package = FBE_PACKAGE_ID_ESP;
        else
            {
                fbe_cli_error("Wrong target package parameter!\n");
                return;
            }
            
        status = fbe_api_persist_add_hook(FBE_PERSIST_HOOK_TYPE_PANIC_WHEN_WRITING_JOURNAL_REGION, package);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_error("Failed to add hook for persist service!\n");
            return;
        }

        fbe_cli_printf("Successfully add hook in persist service!\n");
        return;
    }
    else if(strcmp(argv, "panic_writing_live_region") == 0)
    {
        argv++;
        if(strcmp(argv, "sep") == 0)
            package = FBE_PACKAGE_ID_SEP_0;
        else if(strcmp(argv, "esp") == 0)
            package = FBE_PACKAGE_ID_ESP;
        else
            {
                fbe_cli_error("Wrong target package parameter!\n");
                return;
            }
        status = fbe_api_persist_add_hook(FBE_PERSIST_HOOK_TYPE_PANIC_WHEN_WRITING_LIVE_DATA_REGION, package);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_error("Failed to add hook for persist service!\n");
            return;
        }

        fbe_cli_printf("Successfully add hook in persist service!\n");
        return;
    }
    else
    {
        fbe_cli_error("Unrecognized parameter \"%s\".\n", argv);
        return;
    }
    
}

static void fbe_cli_lib_persist_remove_hook(char * argv)
{
    fbe_status_t    status = FBE_STATUS_INVALID;
    fbe_package_id_t    package = FBE_PACKAGE_ID_INVALID;

    if(strcmp(argv, "panic_writing_journal") == 0)
    {
        argv++;
        if(strcmp(argv, "sep") == 0)
            package = FBE_PACKAGE_ID_SEP_0;
        else if(strcmp(argv, "esp") == 0)
            package = FBE_PACKAGE_ID_ESP;
        else
            {
                fbe_cli_error("Wrong target package parameter!\n");
                return;
            }
        status = fbe_api_persist_remove_hook(FBE_PERSIST_HOOK_TYPE_PANIC_WHEN_WRITING_JOURNAL_REGION, package);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_error("Failed to remove hook for persist service!\n");
            return;
        }

        fbe_cli_printf("Successfully remove hook in persist service!\n");
        return;
    }
    else if(strcmp(argv, "panic_writing_live_region") == 0)
    {
        argv++;
        if(strcmp(argv, "sep") == 0)
            package = FBE_PACKAGE_ID_SEP_0;
        else if(strcmp(argv, "esp") == 0)
            package = FBE_PACKAGE_ID_ESP;
        else
            {
                fbe_cli_error("Wrong target package parameter!\n");
                return;
            }
        status = fbe_api_persist_remove_hook(FBE_PERSIST_HOOK_TYPE_PANIC_WHEN_WRITING_LIVE_DATA_REGION, package);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_error("Failed to remove hook for persist service!\n");
            return;
        }

        fbe_cli_printf("Successfully remove hook in persist service!\n");
        return;
    }
    else
    {
        fbe_cli_error("Unrecognized parameter \"%s\".\n", argv);
        return;
    }
    
}
