#ifndef __FBE_CLI_LURG_SERVICE_H__
#define __FBE_CLI_LURG_SERVICE_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lurg.h
 ***************************************************************************
 *
 * @brief
 *  This file contains LUN/RG command cli definitions.
 *
 * @version
 *   4/20/2010:  Created. Sanjay Bhave
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "global_catmerge_rg_limits.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#include "fbe/fbe_cli.h"
#include "fbe/fbe_lun.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_protocol_error_injection_service.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"


void fbe_cli_bind(fbe_s32_t argc , fbe_s8_t** argv);
void fbe_cli_lun_hard_zero(fbe_s32_t argc , fbe_s8_t** argv);
fbe_bool_t fbecli_validate_atoi_args(const TEXT * arg_value);
void fbe_cli_lustat(fbe_s32_t argc , fbe_s8_t ** argv);
void fbe_cli_hs_configuration(fbe_s32_t argc, fbe_s8_t ** argv);
void fbe_cli_hs_display(fbe_u32_t bus_number, fbe_bool_t list_hs_in_bus);
void fbe_cli_unbind(fbe_s32_t argc, fbe_s8_t** argv);
void fbe_cli_createrg(fbe_s32_t argc, fbe_s8_t** argv);
void fbe_cli_removerg(fbe_s32_t argc, fbe_s8_t** argv);
void fbe_cli_zero_disk(fbe_s32_t argc, fbe_s8_t** argv);
void fbe_cli_zero_lun(fbe_s32_t argc, fbe_s8_t** argv);
void fbe_cli_recreate_system_object(fbe_s32_t argc, fbe_s8_t** argv);
void fbe_cli_recover_system_object_config(fbe_s32_t argc, fbe_s8_t** argv);
void fbe_cli_modify_system_object_config(fbe_s32_t argc, fbe_s8_t** argv);
void fbe_cli_modify_object_entry_of_system_object(fbe_object_id_t system_object_id, fbe_s32_t argc, fbe_s8_t** argv); 
void fbe_cli_modify_user_entry_of_system_object(fbe_object_id_t system_object_id, fbe_s32_t argc, fbe_s8_t** argv); 
void fbe_cli_modify_edge_entry_of_system_object(fbe_object_id_t system_object_id, fbe_s32_t argc, fbe_s8_t** argv); 
void fbe_cli_destroy_broken_system_parity_rg_and_lun(fbe_s32_t argc, fbe_s8_t** argv);
void fbe_cli_generate_config_for_system_parity_rg_and_lun(fbe_s32_t argc, fbe_s8_t** argv);
void fbe_cli_lun_set_write_bypass_mode(fbe_s32_t argc, fbe_s8_t** argv);
void fbe_cli_create_system_rg(fbe_s32_t argc, fbe_s8_t** argv);
void fbe_cli_bind_system_lun(fbe_s32_t argc, fbe_s8_t** argv);
void fbe_cli_private_luns_access(fbe_s32_t argc, fbe_s8_t** argv);


#define LURG_BIND_USAGE "\
bind [switches]...\n\
General Switches:\n\
  -rg          RG Number\n\
  -type        Raid type\n\
  -disk        Disk number(s)\n\
  -cap         RG capacity \n\
  -sq          Size_Unit \n\
  -ndb         1/0, -addroffset and -cap are required if set to 1 \n\
  -addroffset  offset \n\
  -lun         lun number\n\
  -placement   placement\n\
  -wwn         32 hex digits splitted by colon\n\
  -cnt         Count number\n\
  -noinitialverify 1/0 \n\
  -userprivate  bind user private LUN \n\
\n\
E.g\n\
bind -rg 3 -type r5 -disk 0_0_5 0_0_6 0_0_7 -cap 5000 \n\
bind -type r5 -disk 0_0_5 0_0_6 0_0_7 \n\
bind -rg 3 -wwn 60:06:01:61:23:45:67:89:31:00:00:00:00:00:00:00 -cap 5000 \n\
\n"

#define LURG_LUSTAT_USAGE "\
lustat [switches] \n\
General Switches:\n\
-rg      group          - Interpret as a group\n\
-dsz    mb/gb/tb   - Sets the display size of LUNs\n"

#define LURG_PRIVATE_LUNS_ACCESS_USAGE "\
private_lun_access [switches]... \n\
General Switches:\n\
-lun_obj_id <#> - lun object id in decimal number \n\
-paint            - perform write/read and verify the content\n\
-offset lba       - start of the request\n\
-size   size      - total size to read/write in blocks\n\
-iosize io size   - size of each individual IO in blocks\n\
-q      q_depth   - max number of outstanding io\n\
\n\
example:\n\
prvlunacc -lun_obj_id 50 -paint\n\
prvlunacc -lun_obj_id 50 -size 40960 -iosize 2048 -paint\n\
\n"

#define LURG_UNBIND_USAGE "\
unbind [switches] \n\
-lun    <lun number> \n\
-lun_id <lun id> \n\
-unbind_broken_lun    - For a broken lun, allow to remove it even system triple mirror double degraded \n\
-h \n\
\n"
 
#define LURG_HS_USAGE "\
hsconfig [switches] \n\
\n\
General Switches:\n\
  -h           - Help\n\
  -c  <b_e_d>  - Configure hot spare on specific bus, enclosure and slot \n\
  -u  <b_e_d>  - Unconfigure hot spare on specific bus, enclosure and slot \n\
  -p  <b_e_d>  - Get the performance tier for the drive on specific bus, enclosure and slot \n\
  -m           - Get the drive type to performance tier mapping table \n\
  -d  <bus>    - Display hot spare on specific bus \n\
\n\
  -delay_swap <value in seconds>\n\
               - Change the delay in swapping the hot spare.\n\
                 Note: Value should not be equal to 0 or 300 seconds (5 minutes).\n\
\n"

#define LURG_CREATERG_USAGE "\
createrg [switches]...\n\
General Switches:\n\
  -rg       RG Number\n\
  -type     Raid type\n\
  -disk     Disk number(s)\n\
  -cap      RG capacity \n\
  -ps_idle_time Power Saving time in seconds \n\
  -large_elsz Use elsz of 1024 (default is 128)\n\
  -userprivate  As user private\n\
\n\
E.g\n\
createrg -rg 3 -type r5 -disk 0_0_5 0_0_6 0_0_7 -cap 5000 -ps_idle_time 1800\n\
\n"

#define LURG_REMOVERG_USAGE "\
removerg [switches]...\n\
General Switches:\n\
  -rg       RG Number\n\
  -objid    Object ID\n\
  -remove_broken_rg  - For a broken Raid group, allow to remove it even system triple mirror double degraded \n\
\n\
E.g\n\
removerg -rg 3\n\
or\n\
rrg -rg 4\n\
\n"

#define ZERO_DISK_USAGE "\
zero_disk [switches]...\n\
General Switches:\n\
          -b         Bus number.\n\
          -e         Enclosure number.\n\
          -s         Slot number.\n\
          -status    Get the status of disk zeroing. \n \
          -consumed  zero consumed area only. \n \
\n\
E.g\n\
zero_disk -b 0 -e 1 -s 1\n\
zero_disk -b 0 -e 1 -s 1 -status\n\
or\n\
zd -b 0 -e 1 -s 1\n\
\n"

#define ZERO_LUN_USAGE "\
zero_lun [switches]...\n\
-lun    <lun number>    Zero the LUN with LUN number\n\
-lun_objid <lun id>     Zero the LUN with LUN object ID\n\
-r                      Trigger reboot zero, only available at active side\n\
-h/-help                Help, show this usage\n\
\n"

#define HARD_ZERO_LUN_USAGE "\
lun_hard_zero [switches]...\n\
-lun    <lun number>    Zero the LUN with LUN number\n\
-lun_objid <lun id>     Zero the LUN with LUN object ID\n\
-lba <lba>              Zero the LUN starts from the lba\n\
-blocks <blocks>        Zero the LUN with the blocks\n\
-threads <threads>      Zero the LUN with the threads\n\
-h/-help                Help, show this usage\n\
\n"


#define LUN_WRITE_BYPASS_USAGE "\
lun_write_bypass [switches] [0 | 1]\n\
-lun    <lun number>    LUN with LUN number\n\
-lun_id <lun id>     LUN with LUN object ID\n\
-m      < 0 |1>      0 -no bypass ; 1 -do bypass\n\
-h/-help                Help, show this usage\n\
Example: lun_write_bypass -lun 0 1 \n\
Will setLun 0 to write bypass mode \n\
\n"

#define RECREATE_SYSTEM_OBJECT_USAGE "\
recreate_system_object [switches]...\n\
-lun    <lun number>        recreate system lun with LUN number\n\
-lun_id <lun object id>     recreate system lun with lun object id\n\
-ndb    <0 | 1>             for recreating LUN: 0 - without ndb flags(default option); 1 - with ndb flags\n\
-rg     <rg number>         recreate system RG with RG number\n\
-rg_id  <rg object id>      recreate system RG with RG object id\n\
-pvd_id <pvd object id>     recreate system PVD with PVD object id\n\
-force_nr                   when recreating PVD, force it to notify all raid NR. It won't notify RG NR in default\n\
-need_zero                  when recreating PVD, mark the PVD need zero. It won't notify RG NR in default\n\
-h/-help                    Help, show this usage\n\
Example: \n\
recreate_system_object  -lun_id 36 -ndb 1\n\
recreate_system_object  -rg_id 16 \n\
recreate_system_object  -pvd_id 1 \n\
recreate_system_object  -pvd_id 1 -need_zero  (NOTE: PVD data will be zeroed)\n\
recreate_system_object  -pvd_id 1 -need_zero -force_nr (zero the PVD data, notify NR) \n\
\n"

#define MODIFY_SYSTEM_OBJECT_CONFIG_USAGE "\
modify_system_object_config(msoc) [-object_id <object_id>] [-object_entry/-user_entry/-edge_entry][switches]...\n\
-h/-help         Help, show this usage\n\
-object_id <object id>  modify system object config\n\
\n\
[-object_entry] switches: \n\
    -state 0/1       -set the entry invalid/valid\n\
    -version_header_size -set the version header size\n\
    -db_class_id pvd/mirror/striper/lun/parity -set the class_id\n\
    -set_config_union  0  -zero the union \n\
\n\
[-user_entry] switches: \n\
    -state 0/1       -set the entry invalid/valid\n\
    -version_header_size -set the version header size\n\
    -db_class_id pvd/mirror/striper/lun/parity -set the class_id\n\
    -user_data_union 0 -zero the union \n\
\n\
[-edge_entry] swithes: \n\
    -edge_index    -set the client_index of edge, default is 0, this switch must be frist \n\
    -state 0/1       -set the entry invalid/valid\n\
    -version_header_size -set the version header size\n\
    -server_id     -set server_id  \n\
    -capacity      -set edge capacity \n\
    -offset        -set offset \n\
    -ignore_offset ignore/invalid   -set ignore_offset \n\
Example: \n\
modify_system_object_config  -object_id 31 -object_entry -state 0 -set_config_union 0 \n\
\n"


#define RECOVER_SYSTEM_OBJECT_CONFIG_USAGE "\
This tool generates config for system object in local SP memory and then persist config to disk. \n\
After issue this, you need to reboot both SPs at the same time. \n\
The generated config will work only after rebooting both SPs at the same time\n\
recover_system_object_config(rsoc) [-object_id <object_id>] [-object_entry/-user_entry/-edge_entry][switches]...\n\
-h/-help         Help, show this usage\n\
-object_id <object_id>  Reinitialize the system rg/lun from psl \n\
-all     Reinitialize all the system rgs and luns from psl \n\
Example: \n\
recover_system_object_config  -object_id 31  \n\
rsoc -all \n\
\n"


#define DESTROY_SYSTEM_PARITY_RG_AND_LUN "\
destroy_system_parity_rg_and_lun  \n\
-h/-help                    Help, show this usage \n\
Destroy all system parity Raid group. All above LUNs on this RG will be destroy too. \n\
\n"

#define GENERATE_CONFIG_FOR_SYSTEM_PARITY_RG_AND_LUN "\
generate_config_for_system_parity_rg_and_lun   \n\
-h/-help                    Help, show this usage\n\
-ndblun                     Generate system parity rgs and generate system luns in ndb way \n\
Generate the configuration for system parity raid group and related LUNs from PSL\n\
\n"

#define LURG_CREATE_SYSTEM_RG_USAGE "\
create_system_rg [switches]...\n\
General Switches:\n\
  -rg_id    RG object id\n\
  -rg       RG group number\n\
\n\
E.g\n\
create_system_rg -rg 1003 -rg_id 16\n\
NOTE: we need provide rg object id because it is hardcoded for system RG\n\
\n"

#define LURG_BIND_SYSTEM_LUN_USAGE "\
bind_system_lun [switches]...\n\
General Switches:\n\
  -lun_id    RG object id\n\
  -lun       RG group number\n\
  -ndb \n\
  -wwn       World Wide Name \n\
  -udn       User Defined Name \n\
\n\
E.g\n\
bind_system_lun -lun 8224 -lun_id 49 -wwn 60:06:01:61:23:45:67:89:31:00:00:00:00:00:00:00  \n\
NOTE:      \n\
    1 lun_id and lun number are required.     \n\
    2 The format of WWN. \n\
       For example:  -wwn 60:01:34:ff:FF:0a:11:00:   \n\
       a Every byte in the WWN is splited semicolon \n\
       b Every byte value is hexadecimal number.    \n\
       c Every byte splited by semicolon are in range [0, FF].     \n\
       d The number of bytes can't execeed 16.      \n\
    3 The format of UDN:    \n\
       a Composed by characters and numbers    \n\
       b Use \\ stands for the Blank space, for example:    \n\
          -udn asdf\\\\1223   the udn will be:asdf  1223    \n\
\n"

#define DISPLAY_NO_USER_PREF  0x0
#define DISPLAY_IN_MB         0x1
#define DISPLAY_IN_GB         0x2
#define DISPLAY_IN_TB         0x3

#define DIRECTION_DOWNSTREAM 0x1
#define DIRECTION_UPSTREAM   0x2
#define DIRECTION_BOTH       (DIRECTION_DOWNSTREAM | DIRECTION_UPSTREAM)

/* Assuming maximum of 16 drives in R6 RG. */
#define MAX_NUM_DRIVES_IN_RG 16

/* This is defined in fbe_lun_private.h and should be changed 
 * when the value for this in that files changes 
 */
#define FBE_LUN_CHUNK_SIZE 0x800 /* in blocks ( 2048 blocks ) */

#define fbe_cli_lustat_line_size 512

#define FBE_RG_USER_COUNT 256
#define RAID_GROUP_COUNT 256
#define MAX_RAID_GROUP_ID GLOBAL_CATMERGE_RAID_GROUP_ID_MAX
#define INVALID_RAID_GROUP_ID MAX_RAID_GROUP_ID+1

/* Number of disk for an individual disk is one.
 */
#define RAID_INDIVIDUAL_DISK_WIDTH 1 
#define RAID_STRIPED_MIN_DISKS 2
#define RAID_STRIPED_MAX_DISK  16

#define RAID_MIRROR_MIN_DISKS  2

#define RAID3_STRIPED_MIN_DISK 5
#define RAID3_STRIPED_MAX_DISK 9

#define RAID6_STRIPED_MIN_DISK 3

#define RAID10_MIR_STRIPED_MIN_DISK 1

#define HOT_SPARE_NUM_DISK 1

#define MAX_LUN_ID 10240 /* assuming 10K LUN limit */
#define INVALID_LUN_ID MAX_LUN_ID+1

#define FBE_INVALID_PERCENTAGE 101   /*Percentage upto 101 will be valid*/

/*
 * General purpose sizes imported from FLARE generics.h
 */
#define FBE_KILOBYTE        1024
#define FBE_MEGABYTE        1048576
#define FBE_GIGABYTE        1073741824
#define FBE_TERABYTE        1099511627776

#define FBE_BITS_PER_BYTE   8

#define FBE_CLI_BIND_DEFAULT_COUNT 1


typedef struct fbe_cli_lurg_lun_details_s {
    /*! This field is used to store lun number 
     */
    fbe_lun_number_t      lun_number;
    /*! This field is used to store life cycle state of 
     *  lun number 
     */
    fbe_lifecycle_state_t lifecycle_state;
    /*! This field is used to store other details about LUN 
     *  like capacity, address offset
     */
    fbe_database_lun_info_t lun_info;

    /*! This field is used to store lifecycle state name 
     *  of LUN
     */
    const char            *p_lifecycle_state_str;
    /*! This field is used to store RG type string
     */
    const char            *p_rg_str;

    /* get LUN Zero status */
    fbe_api_lun_get_zero_status_t fbe_api_lun_get_zero_status;
}fbe_cli_lurg_lun_details_t;

typedef struct fbe_cli_lurg_pes_s {
    /*! This will hold port number of the drive 
     */
    fbe_port_number_t            port_num;

    /*! This will hold enclosure number of the drive 
     */
    fbe_enclosure_number_t       encl_num;

    /*! This will hold slot number of the drive 
     */
    fbe_enclosure_slot_number_t  slot_num;
    
}fbe_cli_lurg_pes_t;

typedef struct fbe_cli_lurg_downstream_object_list_s {
    /*! This field is used to return the number of downstream object
     *  information.
     */
    fbe_u32_t number_of_downstream_objects;

    /*! This filed is used to hold the list of downstream object-ids. 
     */
    fbe_object_id_t downstream_object_list[MAX_NUM_DRIVES_IN_RG + 1];
}fbe_cli_lurg_downstream_object_list_t;

typedef struct fbe_cli_lurg_rg_details_s {
    /*! This field is used to store RG number 
     */
    fbe_u32_t                                     rg_number;
    
    /*! This flag helps in deciding whether to retrieve downstream
     *  object(s), upstream object(s) or both.
     */
    fbe_u32_t                                     direction_to_fetch_object;
    
    /*! These represent the VD and PVD objects related to given
     *  RG
     */
    fbe_api_base_config_downstream_object_list_t  downstream_vd_object_list;
    fbe_cli_lurg_downstream_object_list_t         downstream_pvd_object_list;
    fbe_api_base_config_upstream_object_list_t    upstream_object_list;

    /*! List of drives in the RG */
    fbe_cli_lurg_pes_t                            drive_list[MAX_NUM_DRIVES_IN_RG + 1];

    /*! List of VDs mode in the RG */
    fbe_virtual_drive_configuration_mode_t        downstream_vd_mode_list[MAX_NUM_DRIVES_IN_RG];
}fbe_cli_lurg_rg_details_t;

typedef struct fbe_cli_lustat_cmd_line_opt_s {
    fbe_u32_t  display_size;
}fbe_cli_lustat_cmd_line_opt_t;

typedef struct fbe_cli_exptd_lifecycle_wait_params_s {
    fbe_u32_t               object_handle_p;
    fbe_lifecycle_state_t   expected_lifecycle_state;
    fbe_u32_t               timeout_ms;
    fbe_package_id_t        package_id;
}fbe_cli_exptd_lifecycle_wait_params_t;

/*List of object ids*/
typedef struct fbe_cli_object_list_s {
    /*! This field is used to return the number of objects
     *  in object_list.
     */
    fbe_u32_t number_of_objects;

    /*! This filed is used to hold the list of object-ids. 
     */
    fbe_object_id_t object_list[FBE_MAX_SEP_OBJECTS];
}fbe_cli_object_list_t;


fbe_status_t fbe_cli_get_lun_details(fbe_object_id_t lun_id, fbe_cli_lurg_lun_details_t* lun_details);
fbe_status_t fbe_cli_get_rg_details(fbe_object_id_t lun_id, fbe_cli_lurg_rg_details_t* lun_details);
fbe_status_t fbe_cli_list_lun_in_system(void);
fbe_bool_t fbe_cli_is_system_lun(fbe_object_id_t object_id_to_check, 
                                      fbe_object_id_t *system_object_list,
                                      fbe_u32_t total_system_objects);
fbe_status_t fbe_cli_generate_random_wwn_number(fbe_assigned_wwid_t *wwn);
fbe_bool_t fbe_cli_checks_for_simillar_wwn_number(fbe_assigned_wwid_t wwn);
fbe_bool_t fbe_cli_checks_for_simillar_wwn_by_class(fbe_assigned_wwid_t wwn,    
                                                    fbe_class_id_t class_to_check);
fbe_status_t fbe_cli_display_lun_stat(fbe_cli_lurg_lun_details_t *lun_details, fbe_cli_lurg_rg_details_t *rg_details);
static fbe_u8_t* fbe_cli_lustat_add_string(fbe_u8_t* line, fbe_u8_t* seeker, const fbe_u8_t* text);
static fbe_u8_t* fbe_cli_lustat_reset_line(fbe_u8_t* line);
static void fbe_cli_lustat_print_line(fbe_u8_t* line);
fbe_u16_t fbe_cli_get_drive_rebuild_progress(fbe_cli_lurg_pes_t *drive_location, fbe_lba_t reb_checkpoint);
static fbe_status_t fbe_cli_get_capacity_displaysize(fbe_u32_t capacity);
static fbe_status_t fbe_cli_list_lun_in_rg (fbe_object_id_t rg_num);

fbe_status_t  fbe_cli_convert_diskname_to_bed(fbe_u8_t disk_name_a[], fbe_job_service_bes_t * phys_location);
fbe_status_t  fbe_cli_convert_encl_to_be(fbe_u8_t disk_name_a[], fbe_job_service_be_t * phys_location);
fbe_status_t fbe_cli_check_disks_for_raid_type(fbe_object_id_t, fbe_raid_group_type_t);
fbe_raid_group_type_t fbe_cli_check_valid_raid_type(fbe_u8_t *rg_type);
static fbe_bool_t fbe_cli_is_valid_raid_group_number(fbe_raid_group_number_t raid_group_id);
static fbe_status_t fbe_cli_find_smallest_free_raid_group_number(fbe_raid_group_number_t * raid_group_id);
fbe_lba_t fbe_cli_convert_str_to_lba(fbe_u8_t *str);
static fbe_status_t fbe_cli_find_smallest_free_lun_number(fbe_lun_number_t* lun_number_p);
static fbe_status_t fbe_cli_validate_lun_request(fbe_api_lun_create_t *lun_create_req);
fbe_status_t fbe_cli_get_state_name(fbe_lifecycle_state_t state, const fbe_char_t  ** pp_state_name);
fbe_status_t fbe_cli_get_RG_name(fbe_lifecycle_state_t state, const fbe_char_t ** pp_raid_type_name);
fbe_status_t fbe_cli_configure_spare(fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot);
fbe_status_t fbe_cli_unconfigure_spare(fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot);
fbe_status_t fbe_cli_update_pvd_configuration(fbe_object_id_t  pvd_object_id, fbe_provision_drive_config_type_t config_type);
void fbe_cli_display_hot_spare_row(fbe_object_id_t object_id, fbe_api_provision_drive_info_t *hs_details);
void fbe_cli_hs_set_swap_delay(fbe_u64_t swap_delay,fbe_api_virtual_drive_permanent_spare_trigger_time_t *spare_config);

fbe_lba_t fbe_cli_convert_size_unit_to_lba(fbe_lba_t unit_size, fbe_u8_t *str);
fbe_status_t fbe_cli_get_drive_rebuild_status(fbe_cli_lurg_lun_details_t *lun_details,fbe_cli_lurg_rg_details_t *rg_details,fbe_u32_t index);
static void fbe_cli_list_lun_by_rg_num(fbe_object_id_t rg_id);
fbe_status_t fbe_cli_get_rg_in_system(fbe_cli_object_list_t *object_list);
fbe_status_t fbe_cli_object_id_list_sort(fbe_cli_object_list_t *object_list);

/*************************
 * end file fbe_cli_lurg.h
 *************************/

#endif /* end __FBE_CLI_LURG_H__ */
