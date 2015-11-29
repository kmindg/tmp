#ifndef __FBE_CLI_SYSTEM_BACKUP_RESTORE_H__
#define __FBE_CLI_SYSTEM_BACKUP_RESTORE_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_system_backup_restore_cmds.h
 ***************************************************************************
 *
 * @brief
 *  This file contains system backup adn restore command cli definitions.
 *
 * @version
 *  07/04/2012 - Created. Yang Zhang
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
 
#include "fbe_private_space_layout.h"
#include "fbe_database.h"

/**********************************
* MACRO DEFINATIONS FOR BACK UP FILE
**********************************/
#define BACKUP_FILE_PATH_LENGTH 64
#define BACKUP_FILE_PATH_FORMAT "./sep_system_dump.txt"
#define SYSTEM_DUMP_FILE_MAGIC_NUMBER     20121212
#define SYSTEM_DUMP_FILE_END_MASK         20131313
#define SYSTEM_DUMP_OBJECT_ENTRY_SIZE     sizeof(database_object_entry_t)
#define SYSTEM_DUMP_USER_ENTRY_SIZE       sizeof(database_user_entry_t)
#define SYSTEM_DUMP_EDGE_ENTRY_SIZE       sizeof(database_edge_entry_t)
#define SYSTEM_DUMP_SYSTEM_OBJECTS        (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST + 1)
#define SYSTEM_DUMP_BUFFER_SIZE           SYSTEM_DUMP_SYSTEM_OBJECTS*(SYSTEM_DUMP_OBJECT_ENTRY_SIZE + SYSTEM_DUMP_USER_ENTRY_SIZE + DATABASE_MAX_EDGE_PER_OBJECT*SYSTEM_DUMP_EDGE_ENTRY_SIZE) 
 /*!********************************************************************* 
  * @enum fbe_dump_section_type_t
  *  
  * @brief 
  * This enumeration lists all the possible dump section state.
  *
  **********************************************************************/
 enum fbe_dump_section_type_e
 {
	 FBE_DUMP_SECTION_SYSTEM_DB_HEADER ,
	 FBE_DUMP_SECTION_SYSTEM_DB ,
	 FBE_DUMP_SECTION_USER_CONFIGURATION,
	 FBE_DUMP_SECTION_SYSTEM_NONPAGED_MD,
	 FBE_DUMP_SECTION_USER_NONPAGED_MD
 };
 typedef fbe_u32_t fbe_dump_section_type_t;

 /*!********************************************************************* 
  * @enum fbe_dump_section_state_t
  *  
  * @brief 
  * This enumeration lists all the possible dump section state.
  *
  **********************************************************************/
 enum fbe_dump_section_state_e
 {
	 FBE_DUMP_SECTION_UNINITIALIZED ,
	 FBE_DUMP_SECTION_VALID ,
	 FBE_DUMP_SECTION_INVALID
 };
 typedef fbe_u32_t fbe_dump_section_state_t;
  /*!****************************************************************************
  * @struct fbe_dump_file_section_t
  ******************************************************************************
  * @brief
  *  This structure defines the contents of each section unit the dump file will hold
  *
  ******************************************************************************/
#pragma pack(1)
 typedef struct fbe_dump_file_section_s
 {
	 fbe_dump_section_state_t	section_state;
	 /*the offset of the section unit. 
	 It is the position where the section unit start
	 in the dump file*/
	 fbe_u32_t	 section_offset;
	 /*the total size  of the section unit
		  in the dump file*/
	 fbe_u32_t	 section_size;	 
 }
 fbe_dump_file_section_t;
#pragma pack()

 /*!****************************************************************************
 * @struct fbe_dump_file_header_t
 ******************************************************************************
 * @brief
 *  This structure define the foramt for dump file header
 *
 ******************************************************************************/
#pragma pack(1)
typedef struct fbe_dump_file_header_s
{
    
    fbe_u64_t       magic_number; /*indicate the dump file is valid or not*/  
    fbe_u64_t       version;    /* The statically compiled version of Sep package */
    fbe_u32_t       dump_file_header_size;/*the size of dump file header*/

	fbe_u32_t       system_object_numbers;/*the total system objects in sep */
	fbe_u32_t       user_object_numbers;/*the total user object in sep*/
	fbe_u32_t       object_entry_size;/*the size of object entry in database*/
	fbe_u32_t       user_entry_size;/* the size of user entry in database*/	
	fbe_u32_t       edge_entry_size;/* the size of edge entry in database*/
    
	/*The region for system database header*/
	fbe_dump_file_section_t    system_db_header_section;
	/*The region for all entried in system database raw mirror */
	fbe_dump_file_section_t    system_db_section;	
	/*The region for user object edge and user entries*/
	fbe_dump_file_section_t    user_configration_section;	
	/*The region for system nonpaged metadata*/
	fbe_dump_file_section_t	system_nonpaged_md_section;
	/*The region for user nonpaged metadata*/
	fbe_dump_file_section_t	user_nonpaged_md_section;

	/*It is only for masking the dump file header end*/
    fbe_u64_t       dump_file_end_mask;
}
fbe_dump_file_header_t;
#pragma pack()

/*********************************
 * FUNCTIONS DECLARE
 *********************************/
void fbe_cli_system_backup_restore(fbe_s32_t argc, fbe_s8_t ** argv);
#define FBE_CLI_SYSTEM_DUMP_USAGE  "\
system_backup_restore or sysbr -  operations about system dump file\n\
usage:  \n\
  sysbr -backup   dump to the system dump file.\n\
         [-file]   the dump file name,use the defualt file if not specified \n\
  sysbr -restore  persist the system configuration to disk.\n\
         [-file]   the dump file name,use the defualt file if not specified  \n\
         -SysDBHeader	 restore the db header to disk.\n\
         -SysDB    restore system db to disk.\n\
         -User	   restore user configuration to disk.\n\
         -SysNP	   restore system NP to disk.\n\
         -UserNP   restore user NP to disk.\n\
        \n\
"


#endif 
/*********************************
 * end file  fbe_cli_system_backup_restore_cmds.h
 *********************************/

