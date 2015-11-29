/***************************************************************************
* Copyright (C) EMC Corporation 2011 
* All rights reserved. 
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!*************************************************************************
*                 @file fbe_cli_lib_system_backup_restore_cmds.c
****************************************************************************
*
* @brief
*  This file contains cli functions for system backup restore related features in
*  FBE CLI.
*
* @ingroup fbe_cli
*
* @date
*  07/7/2012 - Created. Yang Zhang
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cli_private.h"
#include "fbe_cli_system_backup_restore_cmds.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe_system_limits.h"
#include "fbe/fbe_file.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_metadata_interface.h"
#include "fbe/fbe_api_metadata_interface.h"

/**************************
*   LOCAL FUNCTIONS
**************************/
static fbe_status_t fbe_cli_init_dump_db_header(fbe_dump_file_header_t* dump_file_header);
static fbe_file_handle_t fbe_cli_creat_dump_file(fbe_u8_t* dump_file_name,int prot_mode);
static fbe_status_t fbe_cli_init_dump_db_header(fbe_dump_file_header_t* dump_file_header);
static fbe_status_t fbe_cli_fill_up_dump_db_header(fbe_dump_file_header_t* dump_file_header_p,fbe_dump_section_type_t section_type,
	                                                         fbe_u32_t section_size,fbe_u32_t section_offset,fbe_u32_t objects_count);
static fbe_status_t fbe_cli_dump_header_to_backup_file(fbe_file_handle_t fp,fbe_dump_file_header_t* dump_file_header_p,
	                                                               fbe_u32_t header_offset,fbe_u32_t header_size);

static fbe_status_t fbe_cli_dump_system_db_header_to_backup_file(fbe_file_handle_t fp,void* buffer_p,fbe_u32_t buffer_size,
	                                                                    fbe_u32_t section_size,fbe_u32_t section_offset,
	                                                                    fbe_u32_t* actually_dump_size_p);
static fbe_status_t fbe_cli_restore_system_db_header(fbe_file_handle_t fp,void* buffer_p,fbe_u32_t buffer_size);
static fbe_status_t fbe_cli_corrupt_system_db_header(void);
static fbe_status_t fbe_cli_dump_system_db_to_backup_file(fbe_file_handle_t fp,void* buffer_p,fbe_u32_t buffer_size,
	                                                                    fbe_u32_t section_offset,
	                                                                    fbe_u32_t* actually_dump_size_p);
static fbe_status_t fbe_cli_restore_system_db(fbe_file_handle_t fp,void* buffer_p,fbe_u32_t buffer_size);
static fbe_status_t fbe_cli_dump_user_config_to_backup_file(fbe_file_handle_t fp,void* buffer_p,fbe_u32_t buffer_size,
	                                                                    fbe_u32_t section_offset,fbe_u32_t* actually_dump_size_p);
static fbe_status_t fbe_cli_destroy_system_db_lun(void);
static fbe_status_t fbe_cli_destroy_system_lun(fbe_private_space_layout_object_id_t system_lun_id);
static fbe_status_t fbe_cli_restore_user_config(fbe_file_handle_t fp,void* buffer_p,fbe_u32_t buffer_size);
static fbe_status_t fbe_cli_destroy_db_lun(void);
static fbe_status_t fbe_cli_dump_nonpaged_md_to_backup_file(fbe_file_handle_t fp,fbe_u32_t section_offset,fbe_u32_t total_objects_num,fbe_object_id_t start_obj_id,fbe_u32_t* actually_dump_size_p);
static fbe_status_t fbe_cli_restore_system_nonpaged_md(fbe_file_handle_t fp,void* buffer_p,fbe_u32_t buffer_size);
static fbe_status_t fbe_cli_restore_user_nonpaged_md(fbe_file_handle_t fp,void* buffer_p,fbe_u32_t buffer_size);
void fbe_cli_system_backup_restore(fbe_s32_t argc, fbe_s8_t ** argv)
{
    
    fbe_u8_t                             file_path[BACKUP_FILE_PATH_LENGTH] = {'\0'};
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_file_handle_t                    fp = NULL;
    fbe_u32_t                            actually_dump_size = 0;
	void*                                buffer_p = NULL;
    fbe_u32_t                            section_size = 0;
	fbe_u32_t                            section_offset = 0;
	fbe_dump_file_header_t               *dump_file_header_p = NULL;
	fbe_bool_t                           system_db_header_b = FBE_FALSE;
	fbe_bool_t                           system_DB_b = FBE_FALSE;
	fbe_bool_t                           user_config_b = FBE_FALSE;
	fbe_bool_t                           system_NP_b = FBE_FALSE;
	fbe_bool_t                           user_NP_b = FBE_FALSE;
	fbe_u32_t                            user_configurable_objects = 0;
		
    if (argc < 1)
    {
        /*If there are no arguments show usage*/
        fbe_cli_printf("%s", FBE_CLI_SYSTEM_DUMP_USAGE);
        return;
    }
    
    if (argc == 1 &&((strncmp(*argv, "-help", 6) == 0)))
    {
        /*If the arguments is "help"show usage*/
        fbe_cli_printf("%s", FBE_CLI_SYSTEM_DUMP_USAGE);
        return;
    }
		
    fbe_sprintf(file_path, sizeof(file_path),BACKUP_FILE_PATH_FORMAT);
    if ((strncmp(*argv, "-backup", 8) == 0))
    {
       argc--;
	   argv++;
	   while(argc != 0)
	   {
          //    step1. check file option
		  if((strncmp(*argv, "-file", 6) == 0)) 	
		  {
              argc--;
			  argv++;
		      if(argc != 0 && *argv != NULL)
			  {
				 fbe_zero_memory(file_path,sizeof(file_path));
				 fbe_copy_memory(file_path,*argv,(fbe_u32_t)strlen(*argv));
				 file_path[(fbe_u32_t)strlen(*argv)] = '\0';
				   /* there may be next option
				    So we move to next option*/
				 argc--;
				 argv++;
				 continue;
			  }else{/* there must be file name after "file" option*/
				fbe_cli_printf("\n");
				fbe_cli_error("Please specify the dump file name.\n\n");
				fbe_cli_printf("%s", FBE_CLI_SYSTEM_DUMP_USAGE);
				return;
			  }
		   }else if(strncmp(*argv, "-noquiesce", 11) == 0)
		   {
			 //TODO
				  /*
					1.)set flag 2.) goto step 2
					temporarily just return
			   */
			  argc--;
			  argv++;
			  continue;
		   }else
		   {
				fbe_cli_printf("\n");
				fbe_cli_error("Invalid input option\n\n");
				fbe_cli_printf("%s", FBE_CLI_SYSTEM_DUMP_USAGE);
				return;
		   }
	   }
	 
	   /*step2.  Open file. */
	   fp = fbe_file_open(file_path,FBE_FILE_RDWR|FBE_FILE_APPEND,0, NULL);
	   if(FBE_FILE_INVALID_HANDLE == fp)
	   {
		   fp = fbe_cli_creat_dump_file(file_path, FBE_FILE_RDWR|FBE_FILE_CREAT);
		   if(FBE_FILE_INVALID_HANDLE == fp)
		   {
			  fbe_cli_error ("%s:Failed to create system dump file\n", 
							 __FUNCTION__);
			  return;	
		   }
	   }
	   buffer_p = fbe_api_allocate_memory(SYSTEM_DUMP_BUFFER_SIZE);
	   if(buffer_p == NULL)
	   {
		   fbe_cli_error("\n%sFailed to allocate memory ...\n",__FUNCTION__);  
		   fbe_file_close(fp);
		   return;
	   }

	   dump_file_header_p = (fbe_dump_file_header_t *)fbe_api_allocate_memory(sizeof(fbe_dump_file_header_t));
	   if(dump_file_header_p == NULL )
	   {
		   fbe_cli_error("\n%sFailed to allocate memory ...\n",__FUNCTION__);  
		   fbe_file_close(fp);
		   fbe_api_free_memory(buffer_p);
		   return;
	   }
	   /*init the dump file header */
	   status = fbe_cli_init_dump_db_header(dump_file_header_p);
	   if(status != FBE_STATUS_OK)
	   {
		  fbe_cli_error ("%s:Failed to initialize dump file header \n", 
						__FUNCTION__);	 
		  fbe_file_close(fp);
		  fbe_api_free_memory(dump_file_header_p);
		  fbe_api_free_memory(buffer_p);
		  return;	
	   }	  
	   /*1. Dump the system db header */
       section_size = sizeof(database_system_db_header_t);
	   /*before the system db header, there is fbe dump file header
	        so the section offset should be  the size of dump file header*/
	   section_offset += dump_file_header_p->dump_file_header_size;
	   status = fbe_cli_dump_system_db_header_to_backup_file(fp,buffer_p,SYSTEM_DUMP_BUFFER_SIZE,section_size,section_offset,&actually_dump_size);
       if(status == FBE_STATUS_OK && actually_dump_size == section_size)
       {

		   fbe_cli_fill_up_dump_db_header(dump_file_header_p,FBE_DUMP_SECTION_SYSTEM_DB_HEADER,actually_dump_size,section_offset,0);

	   }else{
           /*make the section state in dump file header to invalid*/
		   dump_file_header_p->system_db_header_section.section_state = FBE_DUMP_SECTION_INVALID;
	   	}
	   /*2. Dump the system db  */
       section_size = SYSTEM_DUMP_SYSTEM_OBJECTS*(SYSTEM_DUMP_OBJECT_ENTRY_SIZE + SYSTEM_DUMP_USER_ENTRY_SIZE + DATABASE_MAX_EDGE_PER_OBJECT*SYSTEM_DUMP_EDGE_ENTRY_SIZE);
	   /*before the system db , there is fbe dump file header and system db header,
	        so the section offset should be  the  total size of dump file header and system db header*/
	   section_offset += sizeof(database_system_db_header_t);
	   /*zero the section buffer and actual dump size*/
	   fbe_zero_memory(buffer_p, SYSTEM_DUMP_BUFFER_SIZE);
	   actually_dump_size = 0;
	   status = fbe_cli_dump_system_db_to_backup_file(fp,buffer_p,SYSTEM_DUMP_BUFFER_SIZE,section_offset,&actually_dump_size);
       if(status == FBE_STATUS_OK && actually_dump_size == section_size)
       {
		   fbe_cli_fill_up_dump_db_header(dump_file_header_p,FBE_DUMP_SECTION_SYSTEM_DB,actually_dump_size,section_offset,SYSTEM_DUMP_SYSTEM_OBJECTS);

	   }else{
           /*make the section state in dump file header to invalid*/
		   dump_file_header_p->system_db_section.section_state = FBE_DUMP_SECTION_INVALID;
	   	}
	   /*3. Dump user objects  */
	   /*before the system db , there is fbe dump file header , system db header and system db,
	        so the section offset should be  the  total size of dump file header ,system db header and system db*/
	   section_offset += section_size; // at this time section_size is the sysytem db section size
	   /*clear the section size for dump user objects*/
	   section_size = 0;
	   fbe_api_database_get_max_configurable_objects(&user_configurable_objects);/*at this time the sectin size is equal to
	                                                                                                                    the max configurable object number*/

	   user_configurable_objects -= FBE_RESERVED_OBJECT_IDS;
       section_size = user_configurable_objects * (SYSTEM_DUMP_OBJECT_ENTRY_SIZE + SYSTEM_DUMP_USER_ENTRY_SIZE + DATABASE_MAX_EDGE_PER_OBJECT*SYSTEM_DUMP_EDGE_ENTRY_SIZE);
	   /*zero the section buffer and actual dump size*/
	   fbe_zero_memory(buffer_p, SYSTEM_DUMP_BUFFER_SIZE);
	   actually_dump_size = 0;
	   status = fbe_cli_dump_user_config_to_backup_file(fp,buffer_p,SYSTEM_DUMP_BUFFER_SIZE,section_offset,&actually_dump_size);
       if(status == FBE_STATUS_OK && actually_dump_size == section_size)
       {
		   fbe_cli_fill_up_dump_db_header(dump_file_header_p,FBE_DUMP_SECTION_USER_CONFIGURATION,actually_dump_size,section_offset,user_configurable_objects);

	   }else{
           /*make the section state in dump file header to invalid*/
		   dump_file_header_p->system_db_section.section_state = FBE_DUMP_SECTION_INVALID;
	   	}
	   
	   /*4. Dump system objects's NP  */
	   /*before the system nonpaged md section, there is fbe dump file header , system db header ,system db
	         and user config section.
	        so the section offset should be their total size */
	   section_offset += section_size; // at this time section_size is the user config section size
	   /*reinit the section size for dump system objects' NP*/
       section_size =  SYSTEM_DUMP_SYSTEM_OBJECTS * FBE_METADATA_NONPAGED_MAX_SIZE;
	   actually_dump_size = 0;
	   status = fbe_cli_dump_nonpaged_md_to_backup_file(fp,section_offset,SYSTEM_DUMP_SYSTEM_OBJECTS,FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST,&actually_dump_size);
       if(status == FBE_STATUS_OK && actually_dump_size == section_size)
       {
		   fbe_cli_fill_up_dump_db_header(dump_file_header_p,FBE_DUMP_SECTION_SYSTEM_NONPAGED_MD,actually_dump_size,section_offset,0);

	   }else{
           /*make the section state in dump file header to invalid*/
		   dump_file_header_p->system_db_section.section_state = FBE_DUMP_SECTION_INVALID;
	   	}
	   
	   /*5. Dump user objects's NP  */
	   /*before the system nonpaged md section, there is fbe dump file header , system db header ,system db
	         , user config section and system objects's NP.
	        so the section offset should be their total size */
	   section_offset += section_size; // at this time section_size is the system NP section size
	   /*reinit the section size for dump system objects' NP*/
       section_size =  user_configurable_objects * FBE_METADATA_NONPAGED_MAX_SIZE;
	   status = fbe_cli_dump_nonpaged_md_to_backup_file(fp,section_offset,user_configurable_objects,FBE_RESERVED_OBJECT_IDS,&actually_dump_size);
       if(status == FBE_STATUS_OK && actually_dump_size == section_size)
       {
		   fbe_cli_fill_up_dump_db_header(dump_file_header_p,FBE_DUMP_SECTION_USER_NONPAGED_MD,actually_dump_size,section_offset,0);

	   }else{
           /*make the section state in dump file header to invalid*/
		   dump_file_header_p->system_db_section.section_state = FBE_DUMP_SECTION_INVALID;
	   	}
	   //TODO: Add dumping other items here!!

	   /*After we dumped all the items, we dump file header*/
       status = fbe_cli_dump_header_to_backup_file(fp,dump_file_header_p,0,sizeof(fbe_dump_file_header_t));
	   if(status != FBE_STATUS_OK)
	   {
		  fbe_cli_error("\n%sFailed to dump file header!\n", __FUNCTION__);
		  
	   }else{
		   fbe_cli_printf("backup success! \n");
	   	}

	   /*we release the memory and close the file*/
       fbe_file_close(fp);
	   fbe_api_free_memory(dump_file_header_p);
	   fbe_api_free_memory(buffer_p);
	   return;

    }
	else if((strncmp(*argv, "-restore", 9) == 0))
	{
		argc--;
		argv++;
		while(argc != 0)
		{
			if((strncmp(*argv, "-file", 6) == 0)){
				//fp = fbe_file_open(*argv,FBE_FILE_RDWR|FBE_FILE_APPEND,0, NULL);
				argc--;
				argv++;
				if(argc != 0 && *argv != NULL)
				{
					fbe_zero_memory(file_path,sizeof(file_path));
					fbe_copy_memory(file_path,*argv,(fbe_u32_t)strlen(*argv));
					file_path[(fbe_u32_t)strlen(*argv)] = '\0';
					/* There may be other options
					So we move to next option*/
					argc--;
					argv++;
					continue;
				}else{/* there must be file name after "file" option*/
					fbe_cli_printf("\n");
					fbe_cli_error("Please specify the dump file name.\n\n");
					fbe_cli_printf("%s", FBE_CLI_SYSTEM_DUMP_USAGE);
					return;
				}
			}else if((strncmp(*argv, "-noquiesce", 11) == 0))
			{
				//TODO
					/*
					  1. set the flag
					 2. check other option
				*/
							
				/* There maybe other option after "noquiesce"
				So we move to next option*/
				argc--;
				argv++;
                continue;
			}else if((strncmp(*argv, "-SysDBHeader", 13) == 0))
			{
                /*this flag indicate we need to resotre the sysytem db header*/
				system_db_header_b = FBE_TRUE;
				argc--;
				argv++;
                continue;
			}else if((strncmp(*argv, "-SysDB", 7) == 0))
			{
                /*this flag indicate we need to resotre the sysytem db*/
				system_DB_b = FBE_TRUE;
				argc--;
				argv++;
                continue;
			}else if((strncmp(*argv, "-User", 6) == 0))
			{
                /*this flag indicate we need to resotre the user config*/
				user_config_b = FBE_TRUE;
				argc--;
				argv++;
                continue;
			}else if((strncmp(*argv, "-SysNP", 7) == 0))
			{
                /*this flag indicate we need to resotre the system NP*/
				system_NP_b = FBE_TRUE;
				argc--;
				argv++;
                continue;
			}else if((strncmp(*argv, "-UserNP", 8) == 0))
			{
                /*this flag indicate we need to resotre the user NP*/
				user_NP_b = FBE_TRUE;
				argc--;
				argv++;
                continue;
			}else if((strncmp(*argv, "-all", 5) == 0))
			{
                /*this flag indicate we need to resotre the sysytem db
				  and the system db header*/
				system_DB_b = FBE_TRUE;				
				system_db_header_b = FBE_TRUE;
				user_config_b = FBE_TRUE;
				user_NP_b = FBE_TRUE;
				system_NP_b = FBE_TRUE;
				argc--;
				argv++;
                continue;

			}else{
				fbe_cli_printf("\n");
				fbe_cli_error("Please specify the restore items.\n\n");
				fbe_cli_printf("%s", FBE_CLI_SYSTEM_DUMP_USAGE);
				return;
			}

		}
		/* Open file. */
		fp = fbe_file_open(file_path,FBE_FILE_RDWR|FBE_FILE_APPEND,0, NULL);
		if(fp == FBE_FILE_INVALID_HANDLE)
		{
			fbe_cli_error ("%s:Failed to open system dump file\n", 
								  __FUNCTION__);
			return;  
		}
		buffer_p = fbe_api_allocate_memory(SYSTEM_DUMP_BUFFER_SIZE);
		if(buffer_p == NULL)
		{
			fbe_cli_error("\n%sFailed to allocate memory ...\n",__FUNCTION__);	
			fbe_file_close(fp);

			return;
		}
        if(system_db_header_b){								  
			/*restore system db header*/
			status = fbe_cli_restore_system_db_header(fp,buffer_p,SYSTEM_DUMP_BUFFER_SIZE);
			if(status != FBE_STATUS_OK){
				fbe_cli_error("Failed to restore system db header\n");
						
			}
		}
		if(system_DB_b){
            /*restore system db*/
			status = fbe_cli_restore_system_db(fp,buffer_p,SYSTEM_DUMP_BUFFER_SIZE);
			if(status != FBE_STATUS_OK){
				fbe_cli_error("Failed to restore system db \n");
			}
		}
		
		if(user_config_b){
            /*restore system db*/
			status = fbe_cli_restore_user_config(fp,buffer_p,SYSTEM_DUMP_BUFFER_SIZE);
			if(status != FBE_STATUS_OK){
				fbe_cli_error("Failed to restore user config \n");
			}
		}
		
		if(system_NP_b){
            /*restore system NP*/
			status = fbe_cli_restore_system_nonpaged_md(fp,buffer_p,SYSTEM_DUMP_BUFFER_SIZE);
			if(status != FBE_STATUS_OK){
				fbe_cli_error("Failed to restore system NP \n");
			}
		}
		if(user_NP_b){
            /*restore user NP*/
			status = fbe_cli_restore_user_nonpaged_md(fp,buffer_p,SYSTEM_DUMP_BUFFER_SIZE);
			if(status != FBE_STATUS_OK){
				fbe_cli_error("Failed to restore user NP \n");
			}
		}
		fbe_api_free_memory(buffer_p); 				
		fbe_file_close(fp);
		return;

	}else if((strncmp(*argv, "-Systemtest", 12) == 0))
	{
        /*This is only for test!!! Corrupt the system db header , 
             systemd db lun .
             so the system can go into service mode*/
		argc--;
		argv++;
        if((argc != 0)&& (strncmp(*argv, "-dbHeader", 10) == 0)) 
        {
			status = fbe_cli_corrupt_system_db_header();
			if(status != FBE_STATUS_OK){
				fbe_cli_error("Failed to corrupt the system db header\n");
			}else{
				fbe_cli_printf("Corrupt the system db header\n");
			}
			return;

		}else if((argc != 0)&& (strncmp(*argv, "-Sysdb", 7) == 0))
		{
			status = fbe_cli_destroy_system_db_lun();
			
			if(status != FBE_STATUS_OK){
				fbe_cli_error("Failed to corrupt the system db lun\n");
			}else{
				fbe_cli_printf("Corrupt the system db lun\n");
			}
			return;

		}else{
			status = fbe_cli_corrupt_system_db_header();
			if(status != FBE_STATUS_OK){
				fbe_cli_error("Failed to corrupt the system db header\n");
			}else{
				fbe_cli_printf("Corrupt the system db header\n");
			}
			
			status = fbe_cli_destroy_system_db_lun();
			
			if(status != FBE_STATUS_OK){
				fbe_cli_error("Failed to corrupt the system db lun\n");
			}else{
				fbe_cli_printf("Corrupt the system db lun\n");
			}
			return;

		}
	}
	else if((strncmp(*argv, "-Usertest", 10) == 0))
	{
        /*This is only for test!!! Corrupt db lun.
             */
             
		status = fbe_cli_destroy_db_lun();
		
		if(status != FBE_STATUS_OK){
			fbe_cli_error("Failed to corrupt the db lun\n");
		}else{
			fbe_cli_printf("Corrupt the db lun\n");
		}
		return;
	}else
    {
        /*If there are no arguments show usage*/
        fbe_cli_error("EEROR: invalidate args\n");		
		return;
    }
	
}
static fbe_status_t fbe_cli_dump_system_db_header_to_backup_file(fbe_file_handle_t fp,void* buffer_p,fbe_u32_t buffer_size,
	                                                                             fbe_u32_t section_size,fbe_u32_t section_offset,fbe_u32_t* actually_dump_size_p)
{
    fbe_u32_t                            nbytes = 0;
	fbe_file_status_t                    err_status;
    fbe_status_t                         status;
	
    /*check the buffer size firstly*/
	if(buffer_size < section_size)
	{
		fbe_cli_printf("The section buffer is not big enough! \n");
		*actually_dump_size_p = -1;
		return FBE_STATUS_GENERIC_FAILURE;

	}

	status = fbe_api_database_get_system_db_header((database_system_db_header_t * )buffer_p);
	if(status != FBE_STATUS_OK)
	{ 
	    
		fbe_cli_printf("Get system db header FAILED! \n");
		*actually_dump_size_p = -1;
		return FBE_STATUS_GENERIC_FAILURE;
		
	}
	/*dump system db header to dump file*/
	if(fbe_file_lseek(fp,section_offset,SEEK_SET) == FBE_FILE_ERROR)
	{
		fbe_cli_error ("%s:Failed to seek the start position for the section\n", 
					   __FUNCTION__);
		*actually_dump_size_p = -1;
		return FBE_STATUS_GENERIC_FAILURE;

	}

	nbytes = fbe_file_write(fp,buffer_p,section_size, &err_status);
    /* We expect to read and write the number of bytes EXACTLY as it is in the fixed structure size */
    if((nbytes == 0) || (nbytes == FBE_FILE_ERROR) || (nbytes != (unsigned int)section_size))
    {
		fbe_cli_error ("%s:Failed to write section into backup file %d\n", 
					   __FUNCTION__,err_status);
		
		*actually_dump_size_p = nbytes;
        return FBE_STATUS_GENERIC_FAILURE;
    }
	*actually_dump_size_p = section_size;
    return FBE_STATUS_OK;
}
#if 0
static fbe_status_t fbe_cli_dump_system_db_to_backup_file(fbe_file_handle_t fp,void* buffer_p,fbe_u32_t buffer_size,
	                                                                    fbe_u32_t section_size,fbe_u32_t section_offset,fbe_u32_t* actually_dump_size_p)
{
    fbe_u32_t                            nbytes = 0;
	fbe_file_status_t                    err_status;
	fbe_dump_file_header_t               *dump_file_header_p = NULL;
    fbe_status_t                         status;
	fbe_u32_t                            actual_counts = 0;
	fbe_u32_t                            all_entries_size_per_obj = 0;
	
	/*the size of all entries(object entry,user entry and 16 edge entries) for noe single object*/
	all_entries_size_per_obj = SYSTEM_DUMP_OBJECT_ENTRY_SIZE + SYSTEM_DUMP_USER_ENTRY_SIZE + SYSTEM_DUMP_EDGE_ENTRY_SIZE * DATABASE_MAX_EDGE_PER_OBJECT;
	/*check the buffer size firstly. The buffer size must bigger the section size*/
	if(buffer_size < section_size)
	{
		fbe_cli_printf("The section buffer is not big enough! \n");
		*actually_dump_size_p = -1;
		return FBE_STATUS_GENERIC_FAILURE;

	}

	status = fbe_api_database_get_object_tables_in_range(buffer_p,buffer_size,
		                                          FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST,
		                                          FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST,
		                                          &actual_counts);
	if(status != FBE_STATUS_OK || actual_counts != SYSTEM_DUMP_SYSTEM_OBJECTS)
	{ 
	    
		fbe_cli_printf("Failed to get all the sysytem objects' entries! \n");
		*actually_dump_size_p = -1;
		return FBE_STATUS_GENERIC_FAILURE;
		
	}
	/*dump system db header to dump file*/
	if(fbe_file_lseek(fp,section_offset,SEEK_SET) == FBE_FILE_ERROR)
	{
		fbe_cli_error ("%s:Failed to seek the start position for the section\n", 
					   __FUNCTION__);
		*actually_dump_size_p = -1;
		return FBE_STATUS_GENERIC_FAILURE;

	}

	nbytes = fbe_file_write(fp,buffer_p,actual_counts * SYSTEM_DUMP_OBJECT_ENTRY_SIZE, &err_status);
    /* We expect to read and write the number of bytes EXACTLY as it is in the fixed structure size */
    if((nbytes == 0) || (nbytes == FBE_FILE_ERROR) || (nbytes != (unsigned int)actual_counts * SYSTEM_DUMP_OBJECT_ENTRY_SIZE))
    {
		fbe_cli_error ("%s:Failed to write section into backup file %s\n", 
					   __FUNCTION__,err_status);
		
		*actually_dump_size_p = nbytes;
        return FBE_STATUS_GENERIC_FAILURE;
    }
	section_offset += actual_counts * SYSTEM_DUMP_OBJECT_ENTRY_SIZE;
	actual_counts = 0;
	fbe_zero_memory(buffer_p,buffer_size);
	status = fbe_api_database_get_user_tables_in_range(buffer_p,buffer_size,
		                                          FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST,
		                                          FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST,
		                                          &actual_counts);
	if(status != FBE_STATUS_OK || actual_counts != SYSTEM_DUMP_SYSTEM_OBJECTS)
	{ 
	    
		fbe_cli_printf("Failed to get all the sysytem objects' entries! \n");
		return FBE_STATUS_GENERIC_FAILURE;
		
	}
	/*dump system db header to dump file*/
	if(fbe_file_lseek(fp,section_offset,SEEK_SET) == FBE_FILE_ERROR)
	{
		fbe_cli_error ("%s:Failed to seek the start position for the section\n", 
					   __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;

	}

	nbytes = fbe_file_write(fp,buffer_p,actual_counts * SYSTEM_DUMP_USER_ENTRY_SIZE, &err_status);
    /* We expect to read and write the number of bytes EXACTLY as it is in the fixed structure size */
    if((nbytes == 0) || (nbytes == FBE_FILE_ERROR) || (nbytes != (unsigned int)actual_counts * SYSTEM_DUMP_USER_ENTRY_SIZE))
    {
		fbe_cli_error ("%s:Failed to write section into backup file %s\n", 
					   __FUNCTION__,err_status);
		
		*actually_dump_size_p += nbytes;
        return FBE_STATUS_GENERIC_FAILURE;
    }
	section_offset += actual_counts * SYSTEM_DUMP_USER_ENTRY_SIZE;
	
	actual_counts = 0;
	fbe_zero_memory(buffer_p,buffer_size);
	status = fbe_api_database_get_edge_tables_in_range(buffer_p,buffer_size,
		                                          FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST,
		                                          FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST,
		                                          &actual_counts);
	if(status != FBE_STATUS_OK || actual_counts != SYSTEM_DUMP_SYSTEM_OBJECTS)
	{ 
	    
		fbe_cli_printf("Failed to get all the sysytem objects' entries! \n");
		return FBE_STATUS_GENERIC_FAILURE;
		
	}
	/*dump system db header to dump file*/
	if(fbe_file_lseek(fp,section_offset,SEEK_SET) == FBE_FILE_ERROR)
	{
		fbe_cli_error ("%s:Failed to seek the start position for the section\n", 
					   __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;

	}

	nbytes = fbe_file_write(fp,buffer_p,actual_counts * DATABASE_MAX_EDGE_PER_OBJECT*SYSTEM_DUMP_EDGE_ENTRY_SIZE, &err_status);
    /* We expect to read and write the number of bytes EXACTLY as it is in the fixed structure size */
    if((nbytes == 0) || (nbytes == FBE_FILE_ERROR) || (nbytes != (unsigned int)actual_counts * DATABASE_MAX_EDGE_PER_OBJECT*SYSTEM_DUMP_EDGE_ENTRY_SIZE))
    {
		fbe_cli_error ("%s:Failed to write section into backup file %s\n", 
					   __FUNCTION__,err_status);
		
		*actually_dump_size_p += nbytes;
        return FBE_STATUS_GENERIC_FAILURE;
    }
	
	*actually_dump_size_p += actual_counts * all_entries_size_per_obj;
    return FBE_STATUS_OK;
}
#endif
static fbe_status_t fbe_cli_dump_user_config_to_backup_file(fbe_file_handle_t fp,void* buffer_p,fbe_u32_t buffer_size,
	                                                                     fbe_u32_t section_offset,fbe_u32_t* actually_dump_size_p)
{
    fbe_u32_t                            nbytes = 0;
	fbe_file_status_t                    err_status;
    fbe_status_t                         status;
	fbe_u32_t                            actual_counts = 0;
	fbe_u32_t                            all_entries_size_per_obj = 0;
	fbe_u32_t                            max_sep_user_objects = 0;
	fbe_u32_t                            temp_objects_counts = 0;
	fbe_u32_t                            object_per_loop = 0;
    fbe_object_id_t                      start_nonsystem_object_id = 0;
	fbe_object_id_t                      end_nonsystem_object_id = 0;
    fbe_u32_t                            object_entries_offset = 0;
	fbe_u32_t                            user_entries_offset = 0;
	fbe_u32_t                            edge_entries_offset = 0;
	
	status = fbe_api_database_get_max_configurable_objects(&max_sep_user_objects);/*at this time the max 
	                 sep user objects including the PSL objects, so we need to minus the PSL RESERVED objetcs count*/
	if(status != FBE_STATUS_OK)
	{
		 fbe_cli_printf("Failed to get the max configurable objects in DB SRV! \n");
		 *actually_dump_size_p = -1;
		 return FBE_STATUS_GENERIC_FAILURE;
	}
	max_sep_user_objects = max_sep_user_objects - FBE_RESERVED_OBJECT_IDS;
	/*the size of all entries(object entry,user entry and 16 edge entries) for one single object*/
	all_entries_size_per_obj = SYSTEM_DUMP_OBJECT_ENTRY_SIZE + SYSTEM_DUMP_USER_ENTRY_SIZE + SYSTEM_DUMP_EDGE_ENTRY_SIZE * DATABASE_MAX_EDGE_PER_OBJECT;
    /*init three kinds of entries' start offset*/
	object_entries_offset = section_offset;
	user_entries_offset = object_entries_offset + max_sep_user_objects * SYSTEM_DUMP_OBJECT_ENTRY_SIZE;
	edge_entries_offset = user_entries_offset + max_sep_user_objects * SYSTEM_DUMP_USER_ENTRY_SIZE;
	*actually_dump_size_p = 0;

	/*1. Get all the user objects' object entries*/
    temp_objects_counts = max_sep_user_objects;

	start_nonsystem_object_id = FBE_RESERVED_OBJECT_IDS;
	while(temp_objects_counts)
	{
		object_per_loop = buffer_size / SYSTEM_DUMP_OBJECT_ENTRY_SIZE;
        if(object_per_loop > temp_objects_counts)
        {
			object_per_loop = temp_objects_counts;
		}
		end_nonsystem_object_id = start_nonsystem_object_id + object_per_loop - 1;
	   
		status = fbe_api_database_get_object_tables_in_range(buffer_p,buffer_size,
													  start_nonsystem_object_id,
													  end_nonsystem_object_id,
													  &actual_counts);
		if(status != FBE_STATUS_OK || actual_counts != object_per_loop)
		{ 
		 
			fbe_cli_printf("Failed to get objects' entries! \n");
			*actually_dump_size_p = -1;
			return FBE_STATUS_GENERIC_FAILURE;
		 
		}
		/*dump object entries  to dump file*/
		if(fbe_file_lseek(fp,object_entries_offset,SEEK_SET) == FBE_FILE_ERROR)
		{
			fbe_cli_error ("%s:Failed to seek the start position for the section\n", 
						   __FUNCTION__);
			*actually_dump_size_p = -1;
			return FBE_STATUS_GENERIC_FAILURE;
		
		}
		
		nbytes = fbe_file_write(fp,buffer_p,actual_counts * SYSTEM_DUMP_OBJECT_ENTRY_SIZE, &err_status);
		/* We expect to read and write the number of bytes EXACTLY as it is in the fixed structure size */
		if((nbytes == 0) || (nbytes == FBE_FILE_ERROR) || (nbytes != (unsigned int)actual_counts * SYSTEM_DUMP_OBJECT_ENTRY_SIZE))
		{
			fbe_cli_error ("%s:Failed to write section into backup file %d\n", 
						   __FUNCTION__,err_status);
			
			*actually_dump_size_p = nbytes;
			return FBE_STATUS_GENERIC_FAILURE;
		}
	   object_entries_offset += actual_counts * SYSTEM_DUMP_OBJECT_ENTRY_SIZE;
	   start_nonsystem_object_id += actual_counts;
	   temp_objects_counts -= actual_counts;
	}
	
	/*2. Get all the user objects' user entries*/
    fbe_zero_memory(buffer_p,buffer_size);
    temp_objects_counts = max_sep_user_objects;
	start_nonsystem_object_id = FBE_RESERVED_OBJECT_IDS;
	while(temp_objects_counts)
	{
		object_per_loop = buffer_size / SYSTEM_DUMP_USER_ENTRY_SIZE;
        if(object_per_loop > temp_objects_counts)
        {
			object_per_loop = temp_objects_counts;
		}
		end_nonsystem_object_id = start_nonsystem_object_id + object_per_loop - 1;
	   
		status = fbe_api_database_get_user_tables_in_range(buffer_p,buffer_size,
													  start_nonsystem_object_id,
													  end_nonsystem_object_id,
													  &actual_counts);
		if(status != FBE_STATUS_OK || actual_counts != object_per_loop)
		{ 
		 
			fbe_cli_printf("Failed to get objects' entries! \n");
			*actually_dump_size_p = -1;
			return FBE_STATUS_GENERIC_FAILURE;
		 
		}
		/*dump object entries  to dump file*/
		if(fbe_file_lseek(fp,user_entries_offset,SEEK_SET) == FBE_FILE_ERROR)
		{
			fbe_cli_error ("%s:Failed to seek the start position for the section\n", 
						   __FUNCTION__);
			*actually_dump_size_p = -1;
			return FBE_STATUS_GENERIC_FAILURE;
		
		}
		
		nbytes = fbe_file_write(fp,buffer_p,actual_counts * SYSTEM_DUMP_USER_ENTRY_SIZE, &err_status);
		/* We expect to read and write the number of bytes EXACTLY as it is in the fixed structure size */
		if((nbytes == 0) || (nbytes == FBE_FILE_ERROR) || (nbytes != (unsigned int)actual_counts * SYSTEM_DUMP_USER_ENTRY_SIZE))
		{
			fbe_cli_error ("%s:Failed to write section into backup file %d\n", 
						   __FUNCTION__,err_status);
			
			*actually_dump_size_p = nbytes;
			return FBE_STATUS_GENERIC_FAILURE;
		}
		user_entries_offset += actual_counts * SYSTEM_DUMP_USER_ENTRY_SIZE;
	   start_nonsystem_object_id += actual_counts;
	   temp_objects_counts -= actual_counts;
	}
	/*3. Get all the user objects' edge entries*/
    fbe_zero_memory(buffer_p,buffer_size);
    temp_objects_counts = max_sep_user_objects;
	start_nonsystem_object_id = FBE_RESERVED_OBJECT_IDS;
	while(temp_objects_counts)
	{
		object_per_loop = buffer_size / (SYSTEM_DUMP_EDGE_ENTRY_SIZE * DATABASE_MAX_EDGE_PER_OBJECT);
        if(object_per_loop > temp_objects_counts)
        {
			object_per_loop = temp_objects_counts;
		}
		end_nonsystem_object_id = start_nonsystem_object_id + object_per_loop - 1;
	   
		status = fbe_api_database_get_edge_tables_in_range(buffer_p,buffer_size,
													  start_nonsystem_object_id,
													  end_nonsystem_object_id,
													  &actual_counts);
		if(status != FBE_STATUS_OK || actual_counts != object_per_loop)
		{ 
		 
			fbe_cli_printf("Failed to get objects' entries! \n");
			*actually_dump_size_p = -1;
			return FBE_STATUS_GENERIC_FAILURE;
		 
		}
		/*dump object entries  to dump file*/
		if(fbe_file_lseek(fp,edge_entries_offset,SEEK_SET) == FBE_FILE_ERROR)
		{
			fbe_cli_error ("%s:Failed to seek the start position for the section\n", 
						   __FUNCTION__);
			*actually_dump_size_p = -1;
			return FBE_STATUS_GENERIC_FAILURE;
		
		}
		
		nbytes = fbe_file_write(fp,buffer_p,actual_counts * SYSTEM_DUMP_EDGE_ENTRY_SIZE * DATABASE_MAX_EDGE_PER_OBJECT, &err_status);
		/* We expect to read and write the number of bytes EXACTLY as it is in the fixed structure size */
		if((nbytes == 0) || (nbytes == FBE_FILE_ERROR) || (nbytes != (unsigned int)actual_counts * SYSTEM_DUMP_EDGE_ENTRY_SIZE * DATABASE_MAX_EDGE_PER_OBJECT))
		{
			fbe_cli_error ("%s:Failed to write section into backup file %d\n", 
						   __FUNCTION__,err_status);
			
			*actually_dump_size_p = nbytes;
			return FBE_STATUS_GENERIC_FAILURE;
		}
		edge_entries_offset += actual_counts * (SYSTEM_DUMP_EDGE_ENTRY_SIZE * DATABASE_MAX_EDGE_PER_OBJECT);
	   start_nonsystem_object_id += actual_counts;
	   temp_objects_counts -= actual_counts;
	}
	*actually_dump_size_p = all_entries_size_per_obj * max_sep_user_objects;
	   
  return FBE_STATUS_OK;
}
static fbe_status_t fbe_cli_dump_system_db_to_backup_file(fbe_file_handle_t fp,void* buffer_p,fbe_u32_t buffer_size,
	                                                                     fbe_u32_t section_offset,fbe_u32_t* actually_dump_size_p)
{
    fbe_u32_t                            nbytes = 0;
	fbe_file_status_t                    err_status;
    fbe_status_t                         status;
	fbe_u32_t                            actual_counts = 0;
	fbe_u32_t                            all_entries_size_per_obj = 0;
	fbe_u32_t                            temp_objects_counts = 0;
	fbe_u32_t                            object_per_loop = 0;
    fbe_object_id_t                      start_nonsystem_object_id = 0;
	fbe_object_id_t                      end_nonsystem_object_id = 0;
    fbe_u32_t                            object_entries_offset = 0;
	fbe_u32_t                            user_entries_offset = 0;
	fbe_u32_t                            edge_entries_offset = 0;
	fbe_u32_t                            total_objects = SYSTEM_DUMP_SYSTEM_OBJECTS;
		
	/*the size of all entries(object entry,user entry and 16 edge entries) for one single object*/
	all_entries_size_per_obj = SYSTEM_DUMP_OBJECT_ENTRY_SIZE + SYSTEM_DUMP_USER_ENTRY_SIZE + SYSTEM_DUMP_EDGE_ENTRY_SIZE * DATABASE_MAX_EDGE_PER_OBJECT;
    /*init three kinds of entries' start offset*/
	object_entries_offset = section_offset;
	user_entries_offset = object_entries_offset + total_objects * SYSTEM_DUMP_OBJECT_ENTRY_SIZE;
	edge_entries_offset = user_entries_offset + total_objects * SYSTEM_DUMP_USER_ENTRY_SIZE;
	*actually_dump_size_p = 0;

	/*1. Get all the user objects' object entries*/
    temp_objects_counts = total_objects;

	start_nonsystem_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST;
	while(temp_objects_counts)
	{
		object_per_loop = buffer_size / SYSTEM_DUMP_OBJECT_ENTRY_SIZE;
        if(object_per_loop > temp_objects_counts)
        {
			object_per_loop = temp_objects_counts;
		}
		end_nonsystem_object_id = start_nonsystem_object_id + object_per_loop - 1;
	   
		status = fbe_api_database_get_object_tables_in_range(buffer_p,buffer_size,
													  start_nonsystem_object_id,
													  end_nonsystem_object_id,
													  &actual_counts);
		if(status != FBE_STATUS_OK || actual_counts != object_per_loop)
		{ 
		 
			fbe_cli_printf("Failed to get objects' entries! \n");
			*actually_dump_size_p = -1;
			return FBE_STATUS_GENERIC_FAILURE;
		 
		}
		/*dump object entries  to dump file*/
		if(fbe_file_lseek(fp,object_entries_offset,SEEK_SET) == FBE_FILE_ERROR)
		{
			fbe_cli_error ("%s:Failed to seek the start position for the section\n", 
						   __FUNCTION__);
			*actually_dump_size_p = -1;
			return FBE_STATUS_GENERIC_FAILURE;
		
		}
		
		nbytes = fbe_file_write(fp,buffer_p,actual_counts * SYSTEM_DUMP_OBJECT_ENTRY_SIZE, &err_status);
		/* We expect to read and write the number of bytes EXACTLY as it is in the fixed structure size */
		if((nbytes == 0) || (nbytes == FBE_FILE_ERROR) || (nbytes != (unsigned int)actual_counts * SYSTEM_DUMP_OBJECT_ENTRY_SIZE))
		{
			fbe_cli_error ("%s:Failed to write section into backup file %d\n", 
						   __FUNCTION__,err_status);
			
			*actually_dump_size_p = nbytes;
			return FBE_STATUS_GENERIC_FAILURE;
		}
	   object_entries_offset += actual_counts * SYSTEM_DUMP_OBJECT_ENTRY_SIZE;
	   start_nonsystem_object_id += actual_counts;
	   temp_objects_counts -= actual_counts;
	}
	
	/*2. Get all the user objects' user entries*/
    fbe_zero_memory(buffer_p,buffer_size);
    temp_objects_counts = total_objects;
	start_nonsystem_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST;
	while(temp_objects_counts)
	{
		object_per_loop = buffer_size / SYSTEM_DUMP_USER_ENTRY_SIZE;
        if(object_per_loop > temp_objects_counts)
        {
			object_per_loop = temp_objects_counts;
		}
		end_nonsystem_object_id = start_nonsystem_object_id + object_per_loop - 1;
	   
		status = fbe_api_database_get_user_tables_in_range(buffer_p,buffer_size,
													  start_nonsystem_object_id,
													  end_nonsystem_object_id,
													  &actual_counts);
		if(status != FBE_STATUS_OK || actual_counts != object_per_loop)
		{ 
		 
			fbe_cli_printf("Failed to get objects' entries! \n");
			*actually_dump_size_p = -1;
			return FBE_STATUS_GENERIC_FAILURE;
		 
		}
		/*dump object entries  to dump file*/
		if(fbe_file_lseek(fp,user_entries_offset,SEEK_SET) == FBE_FILE_ERROR)
		{
			fbe_cli_error ("%s:Failed to seek the start position for the section\n", 
						   __FUNCTION__);
			*actually_dump_size_p = -1;
			return FBE_STATUS_GENERIC_FAILURE;
		
		}
		
		nbytes = fbe_file_write(fp,buffer_p,actual_counts * SYSTEM_DUMP_USER_ENTRY_SIZE, &err_status);
		/* We expect to read and write the number of bytes EXACTLY as it is in the fixed structure size */
		if((nbytes == 0) || (nbytes == FBE_FILE_ERROR) || (nbytes != (unsigned int)actual_counts * SYSTEM_DUMP_USER_ENTRY_SIZE))
		{
			fbe_cli_error ("%s:Failed to write section into backup file %d\n", 
						   __FUNCTION__,err_status);
			
			*actually_dump_size_p = nbytes;
			return FBE_STATUS_GENERIC_FAILURE;
		}
		user_entries_offset += actual_counts * SYSTEM_DUMP_USER_ENTRY_SIZE;
	   start_nonsystem_object_id += actual_counts;
	   temp_objects_counts -= actual_counts;
	}
	/*3. Get all the user objects' edge entries*/
    fbe_zero_memory(buffer_p,buffer_size);
    temp_objects_counts = total_objects;
	start_nonsystem_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST;
	while(temp_objects_counts)
	{
		object_per_loop = buffer_size / (SYSTEM_DUMP_EDGE_ENTRY_SIZE * DATABASE_MAX_EDGE_PER_OBJECT);
        if(object_per_loop > temp_objects_counts)
        {
			object_per_loop = temp_objects_counts;
		}
		end_nonsystem_object_id = start_nonsystem_object_id + object_per_loop - 1;
	   
		status = fbe_api_database_get_edge_tables_in_range(buffer_p,buffer_size,
													  start_nonsystem_object_id,
													  end_nonsystem_object_id,
													  &actual_counts);
		if(status != FBE_STATUS_OK || actual_counts != object_per_loop)
		{ 
		 
			fbe_cli_printf("Failed to get objects' entries! \n");
			*actually_dump_size_p = -1;
			return FBE_STATUS_GENERIC_FAILURE;
		 
		}
		/*dump object entries  to dump file*/
		if(fbe_file_lseek(fp,edge_entries_offset,SEEK_SET) == FBE_FILE_ERROR)
		{
			fbe_cli_error ("%s:Failed to seek the start position for the section\n", 
						   __FUNCTION__);
			*actually_dump_size_p = -1;
			return FBE_STATUS_GENERIC_FAILURE;
		
		}
		
		nbytes = fbe_file_write(fp,buffer_p,actual_counts * SYSTEM_DUMP_EDGE_ENTRY_SIZE * DATABASE_MAX_EDGE_PER_OBJECT, &err_status);
		/* We expect to read and write the number of bytes EXACTLY as it is in the fixed structure size */
		if((nbytes == 0) || (nbytes == FBE_FILE_ERROR) || (nbytes != (unsigned int)actual_counts * SYSTEM_DUMP_EDGE_ENTRY_SIZE * DATABASE_MAX_EDGE_PER_OBJECT))
		{
			fbe_cli_error ("%s:Failed to write section into backup file %d\n", 
						   __FUNCTION__,err_status);
			
			*actually_dump_size_p = nbytes;
			return FBE_STATUS_GENERIC_FAILURE;
		}
		edge_entries_offset += actual_counts * (SYSTEM_DUMP_EDGE_ENTRY_SIZE * DATABASE_MAX_EDGE_PER_OBJECT);
	   start_nonsystem_object_id += actual_counts;
	   temp_objects_counts -= actual_counts;
	}
	*actually_dump_size_p = all_entries_size_per_obj * total_objects;
	   
  return FBE_STATUS_OK;
}



static fbe_status_t fbe_cli_init_dump_db_header(fbe_dump_file_header_t* dump_file_header)
{

	if(dump_file_header == NULL)
	{
		fbe_cli_error ("%s:Failed to init the system db header: null pionter!\n", 
					   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;

	}
    /*initialize the dump file header. we only initialize the following  things.
	The other items in dump file header will be initialized when the corresponding 
	dump section is writing into the dump file.*/
	dump_file_header->dump_file_header_size = sizeof(fbe_dump_file_header_t);
    dump_file_header->magic_number = SYSTEM_DUMP_FILE_MAGIC_NUMBER;
	dump_file_header->dump_file_end_mask = SYSTEM_DUMP_FILE_END_MASK;
	dump_file_header->object_entry_size = SYSTEM_DUMP_OBJECT_ENTRY_SIZE;
	dump_file_header->user_entry_size = SYSTEM_DUMP_USER_ENTRY_SIZE;
	dump_file_header->edge_entry_size = SYSTEM_DUMP_EDGE_ENTRY_SIZE;
	return FBE_STATUS_OK;
}
static fbe_status_t fbe_cli_fill_up_dump_db_header(fbe_dump_file_header_t* dump_file_header_p,fbe_dump_section_type_t section_type,
	                                                         fbe_u32_t section_size,fbe_u32_t section_offset,fbe_u32_t objects_count)
{
    fbe_status_t  status = FBE_STATUS_OK;
	if(dump_file_header_p == NULL)
	{
		fbe_cli_error ("%s:Failed to init the system db header: null pionter!\n", 
					   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;

	}
	
	/*Fill up  section field in dump file header*/
	switch(section_type){
	    case FBE_DUMP_SECTION_SYSTEM_DB_HEADER:
			dump_file_header_p->system_db_header_section.section_offset = section_offset;
			dump_file_header_p->system_db_header_section.section_size = section_size;
			dump_file_header_p->system_db_header_section.section_state = FBE_DUMP_SECTION_VALID;
		   	break;
	    case FBE_DUMP_SECTION_SYSTEM_DB:
			dump_file_header_p->system_db_section.section_offset = section_offset;
			dump_file_header_p->system_db_section.section_size = section_size;
			dump_file_header_p->system_db_section.section_state = FBE_DUMP_SECTION_VALID;
            dump_file_header_p->system_object_numbers = objects_count;
			break;
		case FBE_DUMP_SECTION_USER_CONFIGURATION:
			dump_file_header_p->user_configration_section.section_offset = section_offset;
			dump_file_header_p->user_configration_section.section_size = section_size;
			dump_file_header_p->user_configration_section.section_state = FBE_DUMP_SECTION_VALID;
            dump_file_header_p->user_object_numbers = objects_count;
			break;
		case FBE_DUMP_SECTION_SYSTEM_NONPAGED_MD:
			dump_file_header_p->system_nonpaged_md_section.section_offset = section_offset;
			dump_file_header_p->system_nonpaged_md_section.section_size = section_size;
			dump_file_header_p->system_nonpaged_md_section.section_state = FBE_DUMP_SECTION_VALID;
		   	break;
		case FBE_DUMP_SECTION_USER_NONPAGED_MD:
			dump_file_header_p->user_nonpaged_md_section.section_offset = section_offset;
			dump_file_header_p->user_nonpaged_md_section.section_size = section_size;
			dump_file_header_p->user_nonpaged_md_section.section_state = FBE_DUMP_SECTION_VALID;
            break;
		default:
            status = FBE_STATUS_GENERIC_FAILURE;
			break;

	}
	return status;
}

/*!*************************************************************************
 * @file fbe_cli_creat_dump_file
 ***************************************************************************
 *
 * @brief
 *  This function will create a new dump file and init the dump file header.
 *
 * @param:  fbe_u8_t* filename --- the dump file full path
 *  
 ***************************************************************************/
static fbe_file_handle_t fbe_cli_creat_dump_file(fbe_u8_t* dump_file_name,int prot_mode)
{
    fbe_file_handle_t                    fp = NULL;

	fp = fbe_file_creat(dump_file_name, prot_mode);
	if(fp == FBE_FILE_INVALID_HANDLE)
	{
	   fbe_cli_error ("%s:Failed to create backup file %s\n", 
					  __FUNCTION__,dump_file_name);
	   return FBE_FILE_INVALID_HANDLE;	 
	}
    return fp;
}
static fbe_status_t fbe_cli_dump_header_to_backup_file(fbe_file_handle_t fp,fbe_dump_file_header_t* dump_file_header_p,
	                                                               fbe_u32_t header_offset,fbe_u32_t header_size)
{
    fbe_u32_t                            nbytes = 0;
	fbe_file_status_t                    err_status;
	
	fbe_file_lseek(fp,header_offset, SEEK_SET);
	nbytes = fbe_file_write(fp,dump_file_header_p,header_size, &err_status);
	/* We expect to read and write the number of bytes EXACTLY as it is in the fixed structure size */
	if((nbytes == 0) || (nbytes == FBE_FILE_ERROR) || (nbytes != sizeof(fbe_dump_file_header_t)))
	{
	   fbe_cli_error ("%s:Failed to write section into backup file %d\n", 
					 __FUNCTION__,err_status);
	  
	   return FBE_STATUS_GENERIC_FAILURE;
	}

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_cli_restore_system_db_header(fbe_file_handle_t fp,void* buffer_p,fbe_u32_t buffer_size)
{
	fbe_file_status_t                    err_status;
	fbe_dump_file_header_t               *dump_file_header_p = NULL;
    fbe_status_t                         status;
	fbe_u32_t                            section_offset = 0;
	fbe_u32_t                            section_size = 0 ;
	database_system_db_header_t          *system_db_header_p = NULL;


    /*check the buffer size firstly*/
	if(buffer_size < sizeof(database_system_db_header_t)
		||buffer_size < sizeof(fbe_dump_file_header_t))
	{
		fbe_cli_printf("The section buffer is not big enough! \n");
		return FBE_STATUS_GENERIC_FAILURE;
	}


	/*check dump file header*/
	fbe_zero_memory(buffer_p,buffer_size);
	dump_file_header_p = (fbe_dump_file_header_t*)buffer_p;
	fbe_file_lseek(fp, 0, SEEK_SET);
	fbe_file_read(fp,buffer_p,(unsigned int)sizeof(fbe_dump_file_header_t),&err_status);
    if(dump_file_header_p->magic_number != SYSTEM_DUMP_FILE_MAGIC_NUMBER
		||dump_file_header_p->system_db_header_section.section_state != FBE_DUMP_SECTION_VALID
		||dump_file_header_p->dump_file_end_mask != SYSTEM_DUMP_FILE_END_MASK)
    {
		fbe_cli_error("The dump file or the section is invalid! \n");
		return FBE_STATUS_GENERIC_FAILURE;
	}
	/*get the section offset and size*/
	section_offset = dump_file_header_p->system_db_header_section.section_offset;
	section_size = dump_file_header_p->system_db_header_section.section_size;
	/*If the dump file header is valid, we read thesystem db header section and restore it*/
	if(fbe_file_lseek(fp,section_offset,SEEK_SET) == FBE_FILE_ERROR)
	{
		fbe_cli_error ("%s:Failed to seek the start position for the section\n", 
					   __FUNCTION__);
		
		return FBE_STATUS_GENERIC_FAILURE;

	}
	/*read from the dump file to restore*/
    fbe_zero_memory(buffer_p,buffer_size);
	system_db_header_p = (database_system_db_header_t*)buffer_p;
    /*then read the system db section*/
    fbe_file_read(fp,system_db_header_p,section_size,&err_status);
    /*TEST!!*/
	if(system_db_header_p->magic_number != DATABASE_SYSTEM_DB_HEADER_MAGIC_NUMBER)
	{
		fbe_cli_printf("The section content is not correct! \n");
		
		return FBE_STATUS_GENERIC_FAILURE;
	}
	
	status = fbe_api_database_persist_system_db_header(system_db_header_p);
    if(status != FBE_STATUS_OK)
    {
		fbe_cli_error ("%s:Failed to persist system db header to disk \n", 
					  __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}
	
	fbe_cli_printf("System db header restore successfully! \n");
    return FBE_STATUS_OK;
}
static fbe_status_t fbe_cli_restore_system_db(fbe_file_handle_t fp,void* buffer_p,fbe_u32_t buffer_size)
{
	fbe_file_status_t                    err_status;
	fbe_dump_file_header_t               *dump_file_header_p = NULL;
    fbe_status_t                         status;
	fbe_u32_t                            section_offset = 0;
	fbe_u32_t                            section_size = 0 ;
    fbe_u32_t                            actual_counts = 0;

    /*check the buffer size firstly*/
	if(buffer_size < ( SYSTEM_DUMP_SYSTEM_OBJECTS*(SYSTEM_DUMP_OBJECT_ENTRY_SIZE + SYSTEM_DUMP_USER_ENTRY_SIZE + DATABASE_MAX_EDGE_PER_OBJECT*SYSTEM_DUMP_EDGE_ENTRY_SIZE))
		||buffer_size < sizeof(fbe_dump_file_header_t))
	{
		fbe_cli_printf("The section buffer is not big enough! \n");
		return FBE_STATUS_GENERIC_FAILURE;
	}


	/*check dump file header*/
	fbe_zero_memory(buffer_p,buffer_size);
	dump_file_header_p = (fbe_dump_file_header_t*)buffer_p;
	fbe_file_lseek(fp, 0, SEEK_SET);
	fbe_file_read(fp,buffer_p,(unsigned int)sizeof(fbe_dump_file_header_t),&err_status);
    if(dump_file_header_p->magic_number != SYSTEM_DUMP_FILE_MAGIC_NUMBER
		||dump_file_header_p->system_db_section.section_state != FBE_DUMP_SECTION_VALID
		||dump_file_header_p->dump_file_end_mask != SYSTEM_DUMP_FILE_END_MASK)
    {
		fbe_cli_error("The dump file or the section is invalid! \n");
		return FBE_STATUS_GENERIC_FAILURE;
	}
	/*get the section offset and size*/
	section_offset = dump_file_header_p->system_db_section.section_offset;
	section_size = dump_file_header_p->system_db_section.section_size;
	/*If the dump file header is valid, we read thesystem db header section and restore it*/
	if(fbe_file_lseek(fp,section_offset,SEEK_SET) == FBE_FILE_ERROR)
	{
		fbe_cli_error ("%s:Failed to seek the start position for the section\n", 
					   __FUNCTION__);
		
		return FBE_STATUS_GENERIC_FAILURE;

	}

	/*read from the dump file to restore*/
    fbe_zero_memory(buffer_p,buffer_size);
    /*then read the system db section*/
    fbe_file_read(fp,buffer_p,section_size,&err_status);
	
	status = fbe_api_database_persist_system_db(buffer_p,section_size,SYSTEM_DUMP_SYSTEM_OBJECTS,&actual_counts);
    if(status != FBE_STATUS_OK || SYSTEM_DUMP_SYSTEM_OBJECTS != actual_counts)
    {
		fbe_cli_error ("%s:Failed to persist system db header to disk \n", 
					  __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}
	
	fbe_cli_printf("System db restore successfully!\n");
    return FBE_STATUS_OK;
}
static fbe_status_t fbe_cli_restore_user_config(fbe_file_handle_t fp,void* buffer_p,fbe_u32_t buffer_size)
{
	fbe_file_status_t                    err_status;
	fbe_dump_file_header_t               *dump_file_header_p = NULL;
    fbe_status_t                         status;
	fbe_u32_t                            section_offset = 0;
	fbe_u32_t                            section_size = 0 ;
    fbe_u32_t                            temp_counts = 0;
	fbe_u32_t                            all_entries_size_per_obj = 0;
	fbe_u32_t                            max_sep_user_objects = 0;
	fbe_u32_t                            objects_per_loop = 0;
    fbe_u32_t                            object_entries_offset = 0;
	fbe_u32_t                            user_entries_offset = 0;
	fbe_u32_t                            edge_entries_offset = 0;
    fbe_database_config_entries_restore_t restore_op;

    /*check the buffer size firstly*/
	if(buffer_size < sizeof(fbe_dump_file_header_t))
	{
		fbe_cli_printf("The section buffer is not big enough! \n");
		return FBE_STATUS_GENERIC_FAILURE;
	}


	/*check dump file header*/
	fbe_zero_memory(buffer_p,buffer_size);
	dump_file_header_p = (fbe_dump_file_header_t*)buffer_p;
	fbe_file_lseek(fp, 0, SEEK_SET);
	fbe_file_read(fp,buffer_p,(unsigned int)sizeof(fbe_dump_file_header_t),&err_status);
    if(dump_file_header_p->magic_number != SYSTEM_DUMP_FILE_MAGIC_NUMBER
		||dump_file_header_p->user_configration_section.section_state != FBE_DUMP_SECTION_VALID
		||dump_file_header_p->dump_file_end_mask != SYSTEM_DUMP_FILE_END_MASK)
    {
		fbe_cli_error("The dump file or the section is invalid! \n");
		return FBE_STATUS_GENERIC_FAILURE;
	}
	
	/*get the section offset and size*/
	section_offset = dump_file_header_p->user_configration_section.section_offset;
	section_size = dump_file_header_p->user_configration_section.section_size;
	status = fbe_api_database_get_max_configurable_objects(&max_sep_user_objects);/*at this time the max 
	                 sep user objects including the PSL objects, so we need to minus the PSL RESERVED objetcs count*/
	if(status != FBE_STATUS_OK)
	{
		 fbe_cli_printf("Failed to get the max configurable objects in DB SRV! \n");
		 return FBE_STATUS_GENERIC_FAILURE;
	}
	max_sep_user_objects = max_sep_user_objects - FBE_RESERVED_OBJECT_IDS;
	/*the size of all entries(object entry,user entry and 16 edge entries) for one single object*/
	all_entries_size_per_obj = SYSTEM_DUMP_OBJECT_ENTRY_SIZE + SYSTEM_DUMP_USER_ENTRY_SIZE + SYSTEM_DUMP_EDGE_ENTRY_SIZE * DATABASE_MAX_EDGE_PER_OBJECT;
    /*init three kinds of entries' start offset*/
	object_entries_offset = section_offset;
	user_entries_offset = object_entries_offset + max_sep_user_objects * SYSTEM_DUMP_OBJECT_ENTRY_SIZE;
	edge_entries_offset = user_entries_offset + max_sep_user_objects * SYSTEM_DUMP_USER_ENTRY_SIZE;

	/*If the dump file header is valid, we read user config section and restore it*/
	/*1. clean up db lun and re-init persist service*/
	status = fbe_api_database_cleanup_reinit_persit_service();	
	if(status != FBE_STATUS_OK)
	{
		 fbe_cli_printf("Failed to clean up db lun and re-init persist service! \n");
		 return FBE_STATUS_GENERIC_FAILURE;
	}
	
	/*2. restore all the object entries*/
	
	temp_counts = max_sep_user_objects;
	while(temp_counts)
	{
	   objects_per_loop = FBE_MAX_DATABASE_RESTORE_SIZE / SYSTEM_DUMP_OBJECT_ENTRY_SIZE;
	   if(objects_per_loop > temp_counts)
	   {
		   objects_per_loop = temp_counts;
	   }
	   if(fbe_file_lseek(fp,object_entries_offset,SEEK_SET) == FBE_FILE_ERROR)
	   {
		   fbe_cli_error ("%s:Failed to seek the start position for the section\n", 
						  __FUNCTION__);
		   return FBE_STATUS_GENERIC_FAILURE;
	   }
	   /*read from the dump file to restore*/
	   fbe_zero_memory(&restore_op, sizeof (fbe_database_config_entries_restore_t));
	   /*then read the system db section*/
	   fbe_file_read(fp,restore_op.entries,objects_per_loop*SYSTEM_DUMP_OBJECT_ENTRY_SIZE,&err_status);
	   restore_op.entries_num = objects_per_loop;
	   restore_op.entry_size = SYSTEM_DUMP_OBJECT_ENTRY_SIZE;
	   restore_op.restore_type = FBE_DATABASE_CONFIG_RESTORE_TYPE_OBJECT;
	   status = fbe_api_database_restore_user_configuration(&restore_op);
	   if(status != FBE_STATUS_OK)
	   {
			fbe_cli_printf("Failed to restore objects entries! \n");
			return FBE_STATUS_GENERIC_FAILURE;
	   }
	   temp_counts -= objects_per_loop;
	   object_entries_offset += objects_per_loop * SYSTEM_DUMP_OBJECT_ENTRY_SIZE;
	}
	/*3. restore all the user entries*/
	
	temp_counts = max_sep_user_objects;
	while(temp_counts)
	{
	   objects_per_loop = FBE_MAX_DATABASE_RESTORE_SIZE / SYSTEM_DUMP_USER_ENTRY_SIZE;
	   if(objects_per_loop > temp_counts)
	   {
		   objects_per_loop = temp_counts;
	   }
	   if(fbe_file_lseek(fp,user_entries_offset,SEEK_SET) == FBE_FILE_ERROR)
	   {
		   fbe_cli_error ("%s:Failed to seek the start position for the section\n", 
						  __FUNCTION__);
		   return FBE_STATUS_GENERIC_FAILURE;
	   }
	   /*read from the dump file to restore*/
	   fbe_zero_memory(&restore_op, sizeof (fbe_database_config_entries_restore_t));
	   /*then read the system db section*/
	   fbe_file_read(fp,restore_op.entries,objects_per_loop*SYSTEM_DUMP_USER_ENTRY_SIZE,&err_status);
	   restore_op.entries_num = objects_per_loop;
	   restore_op.entry_size = SYSTEM_DUMP_USER_ENTRY_SIZE;
	   restore_op.restore_type = FBE_DATABASE_CONFIG_RESTORE_TYPE_USER;
	   status = fbe_api_database_restore_user_configuration(&restore_op);
	   if(status != FBE_STATUS_OK)
	   {
			fbe_cli_printf("Failed to restore user entries! \n");
			return FBE_STATUS_GENERIC_FAILURE;
	   }
	   temp_counts -= objects_per_loop;
	   user_entries_offset += objects_per_loop * SYSTEM_DUMP_USER_ENTRY_SIZE;
	}
	
	/*3. restore all the edge entries*/
	
	temp_counts = max_sep_user_objects;
	while(temp_counts)
	{
	   objects_per_loop = FBE_MAX_DATABASE_RESTORE_SIZE / (DATABASE_MAX_EDGE_PER_OBJECT * SYSTEM_DUMP_EDGE_ENTRY_SIZE);

	   if(objects_per_loop > temp_counts)
	   {
		   objects_per_loop = temp_counts;
	   }
	   if(fbe_file_lseek(fp,edge_entries_offset,SEEK_SET) == FBE_FILE_ERROR)
	   {
		   fbe_cli_error ("%s:Failed to seek the start position for the section\n", 
						  __FUNCTION__);
		   return FBE_STATUS_GENERIC_FAILURE;
	   }
	   /*read from the dump file to restore*/
	   fbe_zero_memory(&restore_op, sizeof (fbe_database_config_entries_restore_t));
	   /*then read the system db section*/
	   fbe_file_read(fp,restore_op.entries,objects_per_loop * (DATABASE_MAX_EDGE_PER_OBJECT * SYSTEM_DUMP_EDGE_ENTRY_SIZE),&err_status);
	   restore_op.entries_num = objects_per_loop * DATABASE_MAX_EDGE_PER_OBJECT;
	   restore_op.entry_size = SYSTEM_DUMP_EDGE_ENTRY_SIZE;
	   restore_op.restore_type = FBE_DATABASE_CONFIG_RESTORE_TYPE_EDGE;
	   status = fbe_api_database_restore_user_configuration(&restore_op);
	   if(status != FBE_STATUS_OK)
	   {
			fbe_cli_printf("Failed to restore edge entries! \n");
			return FBE_STATUS_GENERIC_FAILURE;
	   }
	   temp_counts -= objects_per_loop;
	   edge_entries_offset += objects_per_loop * DATABASE_MAX_EDGE_PER_OBJECT * SYSTEM_DUMP_EDGE_ENTRY_SIZE;
	}
	
	fbe_cli_printf("User config restore successfully!\n");
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_cli_corrupt_system_db_header(void)
{
    fbe_status_t                         status;
	database_system_db_header_t          *system_db_header_p = NULL;

	system_db_header_p = (database_system_db_header_t * )fbe_api_allocate_memory(SYSTEM_DUMP_BUFFER_SIZE);
	if(system_db_header_p == NULL)
	{
		fbe_cli_error("\n%sFailed to allocate memory ...\n",__FUNCTION__);	
		return FBE_STATUS_GENERIC_FAILURE;
	}

	status = fbe_api_database_get_system_db_header(system_db_header_p);
    if(status != FBE_STATUS_OK)
    {
		fbe_cli_error ("%s:Failed to get system db header to disk \n", 
					  __FUNCTION__);
		
		fbe_api_free_memory(system_db_header_p);
		return FBE_STATUS_GENERIC_FAILURE;
	}
    /*destroy the magic number of system db header*/
	fbe_zero_memory(system_db_header_p,sizeof(database_system_db_header_t));
	status = fbe_api_database_persist_system_db_header(system_db_header_p);
    if(status != FBE_STATUS_OK)
    {
		fbe_cli_error ("%s:Failed to persist system db header to disk \n", 
					  __FUNCTION__);
		
		fbe_api_free_memory(system_db_header_p);
		return FBE_STATUS_GENERIC_FAILURE;
	}
	
	fbe_cli_printf("System db header corrupt successfully! \n");
	
	fbe_api_free_memory(system_db_header_p);
    return FBE_STATUS_OK;
}
static fbe_status_t fbe_cli_destroy_system_db_lun(void)
{
    fbe_status_t  status = FBE_STATUS_GENERIC_FAILURE;
	
	status = fbe_cli_destroy_system_lun(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAID_METADATA);
	
    if(status != FBE_STATUS_OK)
    {
		fbe_cli_error ("%s:Failed to destroy system db lun \n", 
					  __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}
	status = fbe_cli_destroy_system_lun(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_DATABASE);
    if(status != FBE_STATUS_OK)
    {
		fbe_cli_error ("%s:Failed to destroy system db lun \n", 
					  __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}
    return FBE_STATUS_OK;
}
static fbe_status_t fbe_cli_destroy_system_lun(fbe_private_space_layout_object_id_t system_lun_id)
{
    
    fbe_database_control_system_db_op_t system_db_cmd;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
	
    //zero object entry
    system_db_cmd.cmd = FBE_DATABASE_CONTROL_SYSTEM_DB_WRITE_OBJECT;
    system_db_cmd.object_id = system_lun_id;
    system_db_cmd.persist_type = FBE_DATABASE_SYSTEM_DB_PERSIST_DELETE;

    status = fbe_api_database_system_db_send_cmd(&system_db_cmd);
    if(status != FBE_STATUS_OK)
    {
		return FBE_STATUS_GENERIC_FAILURE;
	}
	
    //zero user entry
    system_db_cmd.cmd = FBE_DATABASE_CONTROL_SYSTEM_DB_WRITE_USER;
    system_db_cmd.object_id = system_lun_id;
    system_db_cmd.persist_type = FBE_DATABASE_SYSTEM_DB_PERSIST_DELETE;

    status = fbe_api_database_system_db_send_cmd(&system_db_cmd);
    if(status != FBE_STATUS_OK)
    {
		return FBE_STATUS_GENERIC_FAILURE;
	}

    //zero edge entry
    system_db_cmd.cmd = FBE_DATABASE_CONTROL_SYSTEM_DB_WRITE_EDGE;
    system_db_cmd.object_id = system_lun_id;	
    system_db_cmd.edge_index = 0;
    system_db_cmd.persist_type = FBE_DATABASE_SYSTEM_DB_PERSIST_DELETE;

    status = fbe_api_database_system_db_send_cmd(&system_db_cmd);
    if(status != FBE_STATUS_OK)
    {
		return FBE_STATUS_GENERIC_FAILURE;
	}
    return status;
}
static fbe_status_t fbe_cli_destroy_db_lun(void)
{
    fbe_status_t         status;
	
	/*cleanup and re-initialize database LUN*/
	 status = fbe_api_database_cleanup_reinit_persit_service(); 
	 return status;

}
static fbe_status_t fbe_cli_dump_nonpaged_md_to_backup_file(fbe_file_handle_t fp,fbe_u32_t section_offset,fbe_u32_t total_objects_num,fbe_object_id_t start_obj_id,fbe_u32_t* actually_dump_size_p)
{
    fbe_u32_t                            nbytes = 0;
	fbe_file_status_t                    err_status;
    fbe_status_t                         status;
	fbe_u32_t                            actual_counts = 0;
    fbe_object_id_t                      start_nonsystem_object_id = 0;
    fbe_nonpaged_metadata_backup_restore_t nonpaged_backup_restore_op;		
    fbe_u32_t                            temp_objects_counts = 0;
    fbe_u32_t                            object_per_loop = 0;
	
	temp_objects_counts = total_objects_num;
	start_nonsystem_object_id = start_obj_id;
	while(temp_objects_counts)
	{
		object_per_loop = FBE_MAX_METADATA_BACKUP_RESTORE_SIZE / FBE_METADATA_NONPAGED_MAX_SIZE;
        if(object_per_loop > temp_objects_counts)
        {
			object_per_loop = temp_objects_counts;
		}
	   
	    fbe_zero_memory(&nonpaged_backup_restore_op, sizeof (fbe_nonpaged_metadata_backup_restore_t));
	    nonpaged_backup_restore_op.start_object_id = start_nonsystem_object_id;
	    nonpaged_backup_restore_op.object_number = object_per_loop;
	    status = fbe_api_metadata_nonpaged_backup_objects(&nonpaged_backup_restore_op);
 		if(status != FBE_STATUS_OK)
		{ 
		 
			fbe_cli_printf("Failed to backup system NP! \n");
			*actually_dump_size_p = -1;
			return FBE_STATUS_GENERIC_FAILURE;
		 
		}
		/*dump object entries  to dump file*/
		if(fbe_file_lseek(fp,section_offset,SEEK_SET) == FBE_FILE_ERROR)
		{
			fbe_cli_error ("%s:Failed to seek the start position for the section\n", 
						   __FUNCTION__);
			*actually_dump_size_p = -1;
			return FBE_STATUS_GENERIC_FAILURE;
		
		}
		
		nbytes = fbe_file_write(fp,nonpaged_backup_restore_op.buffer,object_per_loop * FBE_METADATA_NONPAGED_MAX_SIZE, &err_status);
		/* We expect to read and write the number of bytes EXACTLY as it is in the fixed structure size */
		if((nbytes == 0) || (nbytes == FBE_FILE_ERROR) || (nbytes != (unsigned int)object_per_loop * FBE_METADATA_NONPAGED_MAX_SIZE))
		{
			fbe_cli_error ("%s:Failed to write system NP into backup file %d\n", 
						   __FUNCTION__,err_status);
			
			*actually_dump_size_p = nbytes;
			return FBE_STATUS_GENERIC_FAILURE;
		}
	   section_offset += object_per_loop * FBE_METADATA_NONPAGED_MAX_SIZE;
	   start_nonsystem_object_id += object_per_loop;
	   temp_objects_counts -= object_per_loop;
	   actual_counts += object_per_loop;
	}
	
	*actually_dump_size_p = actual_counts * FBE_METADATA_NONPAGED_MAX_SIZE;
	   
    return FBE_STATUS_OK;
}
static fbe_status_t fbe_cli_restore_system_nonpaged_md(fbe_file_handle_t fp,void* buffer_p,fbe_u32_t buffer_size)
{
	fbe_file_status_t                    err_status;
	fbe_dump_file_header_t               *dump_file_header_p = NULL;
    fbe_status_t                         status;
	fbe_u32_t                            section_offset = 0;
	fbe_u32_t                            section_size = 0 ;
    fbe_u32_t                            temp_counts = 0;
	fbe_u32_t                            objects_per_loop = 0;
	fbe_object_id_t                      start_obj_id = 0;
    fbe_nonpaged_metadata_backup_restore_t nonpaged_backup_restore_op;		

    /*check the buffer size firstly*/
	if(buffer_size < sizeof(fbe_dump_file_header_t))
	{
		fbe_cli_printf("The section buffer is not big enough! \n");
		return FBE_STATUS_GENERIC_FAILURE;
	}


	/*check dump file header*/
	fbe_zero_memory(buffer_p,buffer_size);
	dump_file_header_p = (fbe_dump_file_header_t*)buffer_p;
	fbe_file_lseek(fp, 0, SEEK_SET);
	fbe_file_read(fp,buffer_p,(unsigned int)sizeof(fbe_dump_file_header_t),&err_status);
    if(dump_file_header_p->magic_number != SYSTEM_DUMP_FILE_MAGIC_NUMBER
		||dump_file_header_p->system_nonpaged_md_section.section_state != FBE_DUMP_SECTION_VALID
		||dump_file_header_p->dump_file_end_mask != SYSTEM_DUMP_FILE_END_MASK)
    {
		fbe_cli_error("The dump file or the section is invalid! \n");
		return FBE_STATUS_GENERIC_FAILURE;
	}
	
	/*get the section offset and size*/
	section_offset = dump_file_header_p->system_nonpaged_md_section.section_offset;
	section_size = dump_file_header_p->system_nonpaged_md_section.section_size;
	
	temp_counts = SYSTEM_DUMP_SYSTEM_OBJECTS;	
	if(section_size < temp_counts * FBE_METADATA_NONPAGED_MAX_SIZE )
	{
		fbe_cli_error("The system NP section size is incorrect! \n");
		return FBE_STATUS_GENERIC_FAILURE;

	}
	
	/*If the dump file header is valid, we read system Np section and restore it*/
	start_obj_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST;
	while(temp_counts)
	{
	   objects_per_loop = FBE_MAX_METADATA_BACKUP_RESTORE_SIZE / FBE_METADATA_NONPAGED_MAX_SIZE;
	   if(objects_per_loop > temp_counts)
	   {
		   objects_per_loop = temp_counts;
	   }
	   if(fbe_file_lseek(fp,section_offset,SEEK_SET) == FBE_FILE_ERROR)
	   {
		   fbe_cli_error ("%s:Failed to seek the start position for the section\n", 
						  __FUNCTION__);
		   return FBE_STATUS_GENERIC_FAILURE;
	   }
	   /*read from the dump file to restore*/
	   fbe_zero_memory(&nonpaged_backup_restore_op, sizeof (fbe_nonpaged_metadata_backup_restore_t));
	   /*then read the system db section*/
	   fbe_file_read(fp,nonpaged_backup_restore_op.buffer,objects_per_loop * FBE_METADATA_NONPAGED_MAX_SIZE,&err_status);
       nonpaged_backup_restore_op.start_object_id = start_obj_id;
	   nonpaged_backup_restore_op.object_number = objects_per_loop;
	   nonpaged_backup_restore_op.nonpaged_entry_size = FBE_METADATA_NONPAGED_MAX_SIZE;
	   status = fbe_api_metadata_nonpaged_restore_objects_to_disk(&nonpaged_backup_restore_op);
	   if(status != FBE_STATUS_OK)
	   {
			fbe_cli_printf("Failed to restore objects entries! \n");
			return FBE_STATUS_GENERIC_FAILURE;
	   }
	   temp_counts -= objects_per_loop;
	   section_offset += objects_per_loop * FBE_METADATA_NONPAGED_MAX_SIZE;
	   start_obj_id += objects_per_loop;
	}
	
	fbe_cli_printf("System NP restore successfully!\n");
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_cli_restore_user_nonpaged_md(fbe_file_handle_t fp,void* buffer_p,fbe_u32_t buffer_size)
{
	fbe_file_status_t                    err_status;
	fbe_dump_file_header_t               *dump_file_header_p = NULL;
    fbe_status_t                         status;
	fbe_u32_t                            section_offset = 0;
	fbe_u32_t                            section_size = 0 ;
    fbe_u32_t                            temp_counts = 0;
	fbe_u32_t                            objects_per_loop = 0;
	fbe_object_id_t                      start_obj_id = 0;
    fbe_nonpaged_metadata_backup_restore_t nonpaged_backup_restore_op;		
	
    /*check the buffer size firstly*/
	if(buffer_size < sizeof(fbe_dump_file_header_t))
	{
		fbe_cli_printf("The section buffer is not big enough! \n");
		return FBE_STATUS_GENERIC_FAILURE;
	}


	/*check dump file header*/
	fbe_zero_memory(buffer_p,buffer_size);
	dump_file_header_p = (fbe_dump_file_header_t*)buffer_p;
	fbe_file_lseek(fp, 0, SEEK_SET);
	fbe_file_read(fp,buffer_p,(unsigned int)sizeof(fbe_dump_file_header_t),&err_status);
    if(dump_file_header_p->magic_number != SYSTEM_DUMP_FILE_MAGIC_NUMBER
		||dump_file_header_p->user_nonpaged_md_section.section_state != FBE_DUMP_SECTION_VALID
		||dump_file_header_p->dump_file_end_mask != SYSTEM_DUMP_FILE_END_MASK)
    {
		fbe_cli_error("The dump file or the section is invalid! \n");
		return FBE_STATUS_GENERIC_FAILURE;
	}
	
	status = fbe_api_database_get_max_configurable_objects(&temp_counts);/*at this time the max 
	                 sep user objects including the PSL objects, so we need to minus the PSL RESERVED objetcs count*/
	if(status != FBE_STATUS_OK)
	{
		 fbe_cli_printf("Failed to get the max configurable objects in DB SRV! \n");
		 return FBE_STATUS_GENERIC_FAILURE;
	}
	temp_counts -= FBE_RESERVED_OBJECT_IDS;
	/*get the section offset and size*/
	section_offset = dump_file_header_p->user_nonpaged_md_section.section_offset;
	section_size = dump_file_header_p->user_nonpaged_md_section.section_size;
	
	if(section_size < temp_counts * FBE_METADATA_NONPAGED_MAX_SIZE )
	{
		fbe_cli_error("The user NP section size is incorrect! \n");
		return FBE_STATUS_GENERIC_FAILURE;

	}
	
	/*If the dump file header is valid, we read user NP section and restore it*/
	start_obj_id = FBE_RESERVED_OBJECT_IDS;
	while(temp_counts)
	{
	   objects_per_loop = FBE_MAX_METADATA_BACKUP_RESTORE_SIZE / FBE_METADATA_NONPAGED_MAX_SIZE;
	   if(objects_per_loop > temp_counts)
	   {
		   objects_per_loop = temp_counts;
	   }
	   if(fbe_file_lseek(fp,section_offset,SEEK_SET) == FBE_FILE_ERROR)
	   {
		   fbe_cli_error ("%s:Failed to seek the start position for the section\n", 
						  __FUNCTION__);
		   return FBE_STATUS_GENERIC_FAILURE;
	   }
	   /*read from the dump file to restore*/
	   fbe_zero_memory(&nonpaged_backup_restore_op, sizeof (fbe_nonpaged_metadata_backup_restore_t));
	   /*then read the system db section*/
	   fbe_file_read(fp,nonpaged_backup_restore_op.buffer,objects_per_loop * FBE_METADATA_NONPAGED_MAX_SIZE,&err_status);
       nonpaged_backup_restore_op.start_object_id = start_obj_id;
	   nonpaged_backup_restore_op.object_number = objects_per_loop;
	   nonpaged_backup_restore_op.nonpaged_entry_size = FBE_METADATA_NONPAGED_MAX_SIZE;
	   status = fbe_api_metadata_nonpaged_restore_objects_to_disk(&nonpaged_backup_restore_op);
	   if(status != FBE_STATUS_OK)
	   {
			fbe_cli_printf("Failed to restore objects entries! \n");
			return FBE_STATUS_GENERIC_FAILURE;
	   }
	   temp_counts -= objects_per_loop;
	   section_offset += objects_per_loop * FBE_METADATA_NONPAGED_MAX_SIZE;
	   start_obj_id += objects_per_loop;
	}
	
	fbe_cli_printf("User NP restore successfully!\n");
    return FBE_STATUS_OK;
}



