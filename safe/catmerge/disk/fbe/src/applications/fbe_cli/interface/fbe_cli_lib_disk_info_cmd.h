#ifndef FBE_CLI_LIB_DISK_INFO_CMD_H
#define FBE_CLI_LIB_DISK_INFO_CMD_H

#include "fbe/fbe_cli.h"

void fbe_cli_cmd_disk_info(int argc , char ** argv);
static fbe_status_t fbe_cli_disk_display_info(fbe_u32_t object_id, fbe_u32_t context,fbe_package_id_t package_id);
static fbe_status_t fbe_cli_disk_display_info_logical_attributes(fbe_u32_t port, fbe_u32_t enclosure, fbe_u32_t slot);
fbe_status_t fbe_cli_display_disk_lun_stat(fbe_cli_lurg_lun_details_t *lun_details, fbe_cli_lurg_rg_details_t *rg_details);
static fbe_status_t fbe_cli_disk_display_info_metadata(fbe_u32_t port,fbe_u32_t enclosure,fbe_u32_t slot);
void fbe_cli_disk_display_list(void);
static fbe_status_t fbe_cli_disk_display_list_attr(fbe_u32_t object_id, fbe_u32_t context,fbe_package_id_t package_id);
void fbe_cli_disk_format( fbe_job_service_bes_t *phys_location);
void fbe_cli_disk_address_range( fbe_job_service_bes_t *phys_location);
static fbe_status_t fbe_cli_disk_get_user_rg_list(fbe_object_id_t object_id,
                                                  fbe_api_base_config_upstream_object_list_t * upstream_object_list_p);
static fbe_status_t fbe_cli_disk_get_system_rg_list(fbe_object_id_t object_id,
                                                    fbe_api_base_config_upstream_object_list_t * upstream_object_list_p);


#define DISK_INFO_USAGE  "\n\
di/diskinfo - Display disk info and format disk\n\
usage:\n\
 di                                 - Display Information for all available Disks.\n\
 di -l                              - Display basic info for all available Disks.\n\
 di -p                              - Display all disks info based on PDO format.\n\
 di -bms                            - Display BMS raw data (Logpage 0x15) for all disks \n\
 di -bms -file                      - Store the BMS raw data and nice format for all disks \n\
 di -logpage <page# in hex>         - Display specified logpage number for all disks \n\
 di <b_e_s>                         - Display Individual Disk Information including Logial Info based on specified disk\n\
\n\
 di <b_e_s> -p                      - Display individual disk info only in Physical Disk Object format\n\
 di <b_e_s> -s                      - Display Address Ranges for a Specified Disk\n\
 di <b_e_s> -bms                    - Display specified disk BMS raw data\n\
 di <b_e_s> -bms -file              - Display and store specified disk BMS raw data and nice format\n\
 di <b_e_s> -logage <page# in hex>  - Display specified disk with specified logpage (in hex)\n\
\n\
example:\n\
    di                              - for displaying information for all available Disks.\n\
    di 0_1_12                       - for displaying Individual disk Information.\n\
    di -l                           - for displaying basic info for all available disks.\n\
    di -p                           - for displaying Physical Disk Object Info for all available disks\n\
    di -bms                         - for displaying BMS raw data for all available disks.\n\
    di -bms -file                   - for displaying and storing BMS raw data/nice format for all available disks.\n\
    di -logpage <page# in hex>      - for displaying Logpage 0x15 for all available disks\n\
    di 0_0_4 -p                     - for displaying only Physical Disk Object Info for specified disk\n\
    di 2_0_5 -s                     - for displaying Address Ranges for a Specified Disk.\n\
    di 0_0_4 -bms                   - for displaying BMS raw data for the specified disk\n\
    di 0_0_4 -bms -file             - for displaying/storing BMS raw data/nice format for the specified disk\n\
    di 2_0_5 -logpage 0x15          - for displaying Logpage 0x15 (BMS) raw data for a specified Disk.\n\
\n\
"

   
#endif /* FBE_CLI_LIB_DISK_INFO_CMD_H */

