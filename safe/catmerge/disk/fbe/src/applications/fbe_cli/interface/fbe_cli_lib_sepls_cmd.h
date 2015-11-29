#ifndef __FBE_CLI_SEPLS_H__
#define __FBE_CLI_SEPLS_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_sepls.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definition for sepls command of FBE Cli.
 *
 * @date
 * 01/12/2012 - Created. Trupti Ghate
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_cli.h"
#include "fbe/fbe_api_common.h"
#include "fbe_cli_lurg.h"


typedef enum fbe_sepls_display_object_e
{
    FBE_SEPLS_DISPLAY_OBJECTS_NONE = 0,
    FBE_SEPLS_DISPLAY_ONLY_LUN_OBJECTS,
    FBE_SEPLS_DISPLAY_ONLY_RG_OBJECTS,
    FBE_SEPLS_DISPLAY_ONLY_VD_OBJECTS,
    FBE_SEPLS_DISPLAY_ONLY_PVD_OBJECTS,
    FBE_SEPLS_DISPLAY_LEVEL_1, 
    FBE_SEPLS_DISPLAY_LEVEL_2, 
    FBE_SEPLS_DISPLAY_LEVEL_3,
    FBE_SEPLS_DISPLAY_LEVEL_4,
    FBE_SEPLS_DISPLAY_OBJECT_TREE,
} fbe_sepls_display_object_t;

typedef enum fbe_sepls_object_type_e
{
    FBE_SEPLS_OBJECT_TYPE_RG =1,
    FBE_SEPLS_OBJECT_TYPE_LUN,
    FBE_SEPLS_OBJECT_TYPE_VD,
    FBE_SEPLS_OBJECT_TYPE_PVD,
    FBE_SEPLS_OBJECT_TYPE_LDO,
    FBE_SEPLS_OBJECT_TYPE_PDO,
}fbe_sepls_object_type_t;

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
fbe_u32_t fbe_cli_lib_sepls_get_display_format(void);
void fbe_cli_lib_sepls_set_display_format(fbe_sepls_display_object_t selected_display_format);
fbe_object_id_t fbe_cli_lib_sepls_get_current_object_id_to_display(void);
void fbe_cli_lib_sepls_set_object_id_to_display(fbe_object_id_t object_id_to_display);
void fbe_cli_cmd_sepls(int argc, char** argv);
void fbe_cli_sepls_print_lun_details(fbe_object_id_t lun_id, fbe_cli_lurg_rg_details_t * rg_details);
fbe_u32_t fbe_cli_sepls_rg_info(fbe_object_id_t object_id_arg, fbe_package_id_t package_id);
void fbe_cli_sepls_display_raid_type(fbe_raid_group_type_t raid_type);
void  fbe_cli_sepls_print_rg_details(fbe_object_id_t rg_object_id, fbe_cli_lurg_rg_details_t* rg_details, fbe_block_size_t*    exported_block_size);
void fbe_cli_sepls_print_vd_details(fbe_object_id_t vd_id, fbe_block_size_t    exported_block_size);
void fbe_cli_sepls_print_pvd_details(fbe_object_id_t pvd_id, fbe_api_provision_drive_info_t *provision_drive_info);
void fbe_cli_sepls_print_ldo_details(fbe_object_id_t ldo_id, fbe_u32_t capacity);
void fbe_cli_sepls_print_pdo_details(fbe_object_id_t pdo_obj_id, fbe_lba_t capacity);
void fbe_cli_sepls_is_display_allowed(fbe_sepls_object_type_t object_type, fbe_bool_t* b_display);
fbe_lba_t fbe_cli_sepls_calculate_rebuild_capacity(fbe_lba_t total_lun_capacity,  fbe_u32_t rg_width, fbe_raid_group_type_t raid_type);
fbe_status_t fbe_cli_sepls_get_rg_object_id(fbe_object_id_t obj_id_arg, fbe_object_id_t* rg_object_id, fbe_package_id_t package_id);

#define SEPLS_CMD_USAGE "sepls [-help]|[-allsep]|[-lun]|[-rg]|[-vd]|[-pvd]|[-level{1|2|3|4}]|[-objtree [-package {physical | sep}]]\n" \
"  Displays SEP objects info.\n"\
"  By default we show the level 1 SEP objects, and the configured hot spares.\n"\
"  -allsep - This option displays all the sep objects\n"\
"  -lun - This option displays all the lun objects\n"\
"  -rg - This option displays all the Raid Group objects\n"\
"  -vd - This option displays all the VD objects\n"\
"  -pvd - This option displays all the PVD objects\n"\
"  -level <1,2,3,4> - This option display objects upto the depth level specified\n"\
"       1 - Will display only Raid Group objects\n"\
"       2 - Will display LUN and Raid Group objects\n"\
"       3 - Will display LUN, Raid Group and VD objects\n"\
"       4 - Will display LUN, Raid Group, VD and PVD objects\n"\
"  -objtree [-package {physical | sep}]- This will display the object hierarchy associated with Raid Group of specified object.\n" \
"    For example, \n\t sepls -o 0x5 -p physical\n" 
/*************************
 * end file fbe_cli_sepls.h
 *************************/

#endif /* end __FBE_CLI_SEPLS_H__ */

