#ifndef FBE_CLI_LIB_SFP_INFO_CMDS_H
#define FBE_CLI_LIB_SFP_INFO_CMDS_H


#include "fbe/fbe_cli.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_module_info.h"

/* Function prototypes*/
void fbe_cli_cmd_sfp_info(fbe_s32_t argc,char ** argv);
static void fbe_cli_specific_port_sfp_info(fbe_u32_t device_type_index,
								           fbe_u32_t slot_num_index,
								           fbe_u32_t port_num_index);
static void fbe_cli_specific_slot_sfp_info(fbe_u32_t device_type_index,
								           fbe_u32_t slot_num_index);
static void fbe_cli_specific_device_sfp_info(fbe_u32_t device_type_index);
static void fbe_cli_all_sfp_info(void);

static void fbe_cli_print_sfp_info(fbe_esp_module_sfp_info_t *sfp_info);


fbe_u8_t* fbe_cli_get_port_role(fbe_u32_t port);
fbe_u8_t* fbe_cli_convert_port_role_id_to_name(fbe_port_role_t *port_role);
fbe_u8_t* fbe_cli_convert_ioport_role_id_to_name( fbe_ioport_role_t *port_role);


#define SFP_INFO_USAGE "\
sfpinfo/sfp - Show all SFP related information \n\
Usage: \n\
       sfpinfo -h                            -Display help information.\n\
       sfpinfo                               -Display information for all device types.\n\
       sfpinfo -d (mezz or iom)              -Display information for a specified device type.\n\
       sfpinfo -d (mezz or iom) -s #         -Display information for a specified device type and a slot number.\n\
       sfpinfo -d (mezz or iom) -s # -p #    -Display information for a specified device type, a slot number and a port nubmer.\n\
Device Types: \n\
       mezz : FBE_DEVICE_TYPE_MEZZANINE \n\
       iom  : FBE_DEVICE_TYPE_IOMODULE \n"

#endif /*FBE_CLI_LIB_SFP_INFO_CMDs_H*/

