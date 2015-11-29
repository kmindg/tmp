/***************************************************************************
 * Copyright (C) EMC Corporation 2009 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_drive_config_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contains cli functions for the drive configuration service.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *   03/28/2011:  chenl6 - created
 *  03/21/2012:  Adapted for drive configuration service interface.
 *  03/21/2012:  Adapted for drive configuration service interface.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
#include "fbe_cli_private.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_api_drive_configuration_service_interface.h"
#include "fbe_transport_trace.h"
#include "fbe_base_object_trace.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_file.h"
#include "fbe_cli_drive_config.h"
#include "fbe/fbe_api_common_transport.h"


#define DRIVE_CONFIG_USAGE  "\
dcs - shows drive configuration information\n\
usage:  dcs -help                   - For this help information.\n\
        dcs -b bus -e encl -s slot  - Target a single drive.\n\
            -all                    - Target all valid drives in array.\n\
            -download <file>        - Start download process with a file.\n\
                -fast_download      - Download to all drives at once.\n\
                -trial_run          - Issue Trial Run instead of download. \n\
                -check_tla          - Compare TLA of image with the drive. Default will check\n\
                -skip_tla           - Skip TLA Check\n\
                -check_rev          - Only upgrade if new rev is different.\n\
            \n\
            -download_status        - Display download process status.\n\
            -download_abort         - Abort download.\n\
            \n"

#define DRIVE_CONFIG_SRVC_USAGE  "\
dcssrvc - shows drive configuration information\n\
usage:  dcssrvc -help                               - For this help information.\n\
        dcssrvc -display_params                     - display tunable parameters \n\
        dcssrvc -set_stime <msec>                   - Set service timeout \n\
        dcssrvc -set_remap_stime <msec>             - Set remap service timeout \n\
        dcssrvc -set_hc_action <0|1>                - Health Check - enable[1]/disable[0]\n\
        dcssrvc -set_remap_retry_action <0|1>       - Remap Retries - enable[1]/disable[0]\n\
        dcssrvc -set_remap_hc_action <0|1>          - HC for failed remap - enable[1]/disable[0]\n\
        dcssrvc -set_fwdl_fail_action <0|1>         - Fail fw dl after max retry - enable[1]/disable[0]\n\
        dcssrvc -set_enforce_eq_drives <0|1>        - Enforce enhanced queuing drives - enable[1]/disable[0]\n\
        dcssrvc -set_drive_lockup_recovery <0|1>    - Drive lockup recovery error handling\n\
        dcssrvc -set_timeout_quiesce_handling <0|1> - Timeout Quiesce error handling\n\
        dcssrvc -set_pfa_handling <0|1>             - Handle drive PFA trips, such as 01/5D\n\
        dcssrvc -set_ms_capabilty <0|1>             - Set Mode Select capability\n\
        dcssrvc -set_breakup_fw_image <0|1>         - Force fw update to break up image\n\
        dcssrvc -set_4k <0|1>                       - 4K drive support \n\
\n\
        dcssrvc -get_mode_page_overrides [-def|-sam|-min|-addl] - Display default SAS override table.  Defaults to -addl\n\
                                                    -def = default overide table\n\
                                                    -sam = samsung overide table\n\
                                                    -min = minimal override table\n\
                                                    -addl = additional override table\n\
        dcssrvc -set_mode_page_byte [-def|-sam|-min|-addl] <page> <byte> <mask> <value> \n\
                                                    - Set a specific mode page byte offset in SAS\n\
                                                      override table.  Args in HEX.  Defaults to -addl\n\
        dcssrvc -clear_mode_page_overrides          - clears 'additional' mode page override table\n\
        dcssrvc -get_mp_addl_overrides              - Display 'additional' mode page override table\n\
        dcssrvc -add_mp_addl_overrides <page> <byte> <mask> <value> - Add Mod Page page/byte to 'additional' override table\n\
                                                      Args in HEX\n\
        dcssrvc -clear_mp_addl_overrides            - clear 'additional' mode page override table\n\
        dcssrvc -set_fw_image_chunk <KB>            - Set fw image chunk size in KB\n\
        \n"
/*TODO: Add a -reset_params.  -wayne */

/*! @enum fbe_cli_dcs_cmd_type  
 *  @brief This is the set of operations that we can perform via the
 *         drive configuration command.
 */
typedef enum 
{
    FBE_CLI_DCS_CMD_TYPE_INVALID = 0,
    FBE_CLI_DCS_CMD_TYPE_START_DOWNLOAD,
    FBE_CLI_DCS_CMD_TYPE_GET_DOWNLOAD_STATS,
    FBE_CLI_DCS_CMD_TYPE_DOWNLOAD_ABORT,
    FBE_CLI_DCS_CMD_DIEH_FORCE_CLEAR_UPDATE,

    /*srvc commands*/
    FBE_CLI_DCS_CMD_TYPE_DISPLAY_PARAMS,
    FBE_CLI_DCS_CMD_TYPE_SET_SERVICE_TIMEOUT,
    FBE_CLI_DCS_CMD_TYPE_SET_REMAP_SERVICE_TIMEOUT,
    FBE_CLI_DCS_CMD_TYPE_SET_HEALTHCHECK_ACTION,
    FBE_CLI_DCS_CMD_TYPE_SET_REMAP_RETRIES_ACTION,
    FBE_CLI_DCS_CMD_TYPE_SET_REMAP_HC_FOR_NON_RETRYABLE_ACTION,
    FBE_CLI_DCS_CMD_TYPE_SET_FWDL_FAIL_AFTER_MAX_RETRIES,
    FBE_CLI_DCS_CMD_TYPE_SET_ENFORCE_ENHANCED_QUEUING_DRIVES,
    FBE_CLI_DCS_CMD_TYPE_SET_DRIVE_LOCKUP_RECOVERY,
    FBE_CLI_DCS_CMD_TYPE_SET_TIMEOUT_QUIESCE_HANDLING,
    FBE_CLI_DCS_CMD_TYPE_SET_PFA_HANDLING,
    FBE_CLI_DCS_CMD_TYPE_SET_MODE_SELECT_CAPABILITY,
    FBE_CLI_DCS_CMD_TYPE_GET_MODE_PAGE_OVERRIDES,
    FBE_CLI_DCS_CMD_TYPE_SET_MODE_PAGE_BYTE,
    FBE_CLI_DCS_CMD_TYPE_CLEAR_MODE_PAGE_OVERRIDES,
    FBE_CLI_DCS_CMD_TYPE_SET_BREAK_UP_FW_IMAGE,
    FBE_CLI_DCS_CMD_TYPE_SET_FW_IMAGE_CHUNK_SIZE,
    FBE_CLI_DCS_CMD_TYPE_SET_4K_ENABLED,
    FBE_CLI_DCS_CMD_TYPE_SET_IGNORE_INVALID_IDENTITY,
    FBE_CLI_DCS_CMD_TYPE_MAX,

}
fbe_cli_dcs_cmd_type;


typedef struct dcs_tunables_table_s
{
    fbe_cli_dcs_cmd_type cmd;
    char *               cmd_str;
} dcs_tunables_table_t;

const dcs_tunables_table_t Dcs_Tunables_Table[] =
{
    {FBE_CLI_DCS_CMD_TYPE_SET_HEALTHCHECK_ACTION,                "-set_hc_action"}, 
    {FBE_CLI_DCS_CMD_TYPE_SET_REMAP_RETRIES_ACTION,              "-set_remap_retry_action"},
    {FBE_CLI_DCS_CMD_TYPE_SET_REMAP_HC_FOR_NON_RETRYABLE_ACTION, "-set_remap_hc_action"},
    {FBE_CLI_DCS_CMD_TYPE_SET_FWDL_FAIL_AFTER_MAX_RETRIES,       "-set_fwdl_fail_action"},
    {FBE_CLI_DCS_CMD_TYPE_SET_ENFORCE_ENHANCED_QUEUING_DRIVES,   "-set_enforce_eq_drives"},
    {FBE_CLI_DCS_CMD_TYPE_SET_DRIVE_LOCKUP_RECOVERY,             "-set_drive_lockup_recovery"},
    {FBE_CLI_DCS_CMD_TYPE_SET_TIMEOUT_QUIESCE_HANDLING,          "-set_timeout_quiesce_handling"},
    {FBE_CLI_DCS_CMD_TYPE_SET_PFA_HANDLING,                      "-set_pfa_handling"},
    {FBE_CLI_DCS_CMD_TYPE_SET_MODE_SELECT_CAPABILITY,            "-set_ms_capabilty"},
    {FBE_CLI_DCS_CMD_TYPE_SET_BREAK_UP_FW_IMAGE,                 "-set_breakup_fw_image"},
    {FBE_CLI_DCS_CMD_TYPE_SET_4K_ENABLED,                        "-set_4k"},
    {FBE_CLI_DCS_CMD_TYPE_SET_IGNORE_INVALID_IDENTITY,           "-set_ignore_invalid_identity"},
    /* DON'T FORGET TO UPDATE USAGE ABOVE */
    {FBE_CLI_DCS_CMD_TYPE_INVALID,                               NULL}  /*must be last*/
};

#define BYTE_SWAP_16(p) ((((p) >> 8) & 0x00ff) | (((p) << 8) & 0xff00))

#define BYTE_SWAP_32(p) \
    ((((p) & 0xff000000) >> 24) |   \
     (((p) & 0x00ff0000) >>  8) |   \
     (((p) & 0x0000ff00) <<  8) |   \
     (((p) & 0x000000ff) << 24))

/***********************************************
 *   LOCAL FUNCTION PROTOTYPES.
 ***********************************************/
/*static fbe_status_t fbe_cli_drive_config_start_download(fbe_u8_t *file, fbe_bool_t fast_download, fbe_bool_t check_tla);*/
static fbe_status_t fbe_cli_drive_config_start_download(fbe_u8_t *file, fbe_bool_t fast_download, fbe_bool_t trial_run, fbe_bool_t check_tla, fbe_bool_t is_force_download,
                                                         fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot);
static fbe_status_t fbe_cli_drive_config_start_download_all(fbe_u8_t *file, fbe_bool_t fast_download, fbe_bool_t trial_run, fbe_bool_t check_tla, fbe_bool_t is_force_download);
static fbe_status_t fbe_cli_drive_config_get_download_stats(void);
static fbe_status_t fbe_cli_drive_config_dieh_force_clear_update(void);
static fbe_u8_t * get_download_state_string(fbe_drive_configuration_download_process_state_t state);
static fbe_u8_t * get_download_drive_state_string(fbe_drive_configuration_download_drive_state_t state);
static fbe_u8_t * get_download_fail_reason_string(fbe_drive_configuration_download_process_fail_t fail_reason);
static fbe_u8_t * get_download_drive_fail_reason_string(fbe_drive_configuration_download_drive_fail_t fail_reason);
static fbe_status_t fbe_cli_drive_config_fwdl_read_image_file(fbe_drive_configuration_control_fw_info_t *fw_info,
                                                              fbe_u8_t *file);

static fbe_status_t fbe_cli_drive_config_display_params(void);
static fbe_status_t fbe_cli_drive_config_get_mode_page_overrides(fbe_drive_configuration_mode_page_override_table_id_t table_id);


/*!**************************************************************
 * fbe_cli_cmd_drive_config()
 ****************************************************************
 * @brief
 *   This function allows viewing and changing attributes for drive
 *   management object.
 *  
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  03/28/2011 - Created. chenl6
 *  03/21/2012:  Adapted for drive configuration service interface.
 *
 ****************************************************************/

void fbe_cli_cmd_drive_config(int argc , char ** argv)
{
    fbe_u32_t bus = 0xFF;
    fbe_u32_t enclosure = 0xFF;
    fbe_u32_t slot = 0xFF;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_cli_dcs_cmd_type command = FBE_CLI_DCS_CMD_TYPE_INVALID;
    fbe_u8_t *file = NULL;
    fbe_bool_t fast_download = FBE_FALSE;
    fbe_bool_t check_tla = FBE_TRUE;   
    fbe_bool_t single_drive_request = FBE_TRUE;
    fbe_bool_t trial_run = FBE_FALSE;
    fbe_bool_t is_force_download = FBE_FALSE;

    /*
     * Parse the command line.
     */
    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /* If they are asking for help, just display help and exit.
             */
            fbe_cli_printf("%s", DRIVE_CONFIG_USAGE);
            return;
        }
        else if (strcmp(*argv, "-b") == 0)
        {
            /* Get bus argument.
             */
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("-b, expected bus, too few arguments \n");
                return;
            }
            bus = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if (strcmp(*argv, "-e") == 0)
        {
            /* Get Enclosure argument.
             */
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("-e, expected enclosure, too few arguments \n");
                return;
            }
            enclosure = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if (strcmp(*argv, "-s") == 0)
        {
            /* Get Slot argument.
             */
            argc--;
            argv++;
            if (argc == 0)
            {
                fbe_cli_error("-s, expected slot, too few arguments \n");
                return;
            }
            slot = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if (strcmp(*argv, "-all") == 0)
        {
            single_drive_request = FBE_FALSE;
        }
        else if (strcmp(*argv, "-trial_run") == 0)
        {
            trial_run = FBE_TRUE;
        }        
        else if (strcmp(*argv, "-download") == 0)
        {
            command = FBE_CLI_DCS_CMD_TYPE_START_DOWNLOAD;
            /* Get file argument.
             */
            argc--;
            argv++;
            if (argc == 0)
            {
                fbe_cli_error("-download, expected file name, too few arguments \n");
                return;
            }
            file = *argv;
        }
        else if (strcmp(*argv, "-fast_download") == 0)
        {
            fast_download = FBE_TRUE;
        }
        else if (strcmp(*argv, "-check_tla") == 0)
        {
            check_tla = FBE_TRUE;
        }
        else if (strcmp(*argv, "-skip_tla") == 0)
        {
            check_tla = FBE_FALSE;
        }
        else if (strcmp(*argv, "-force") == 0)
        {
            /* This is force and upgrade, even if PVD is failed or RG is degraded.
               This should be an engineering cmd only.  Currently it's not shown in usage to hide from customer.
             */
            is_force_download = FBE_TRUE;
        }
        else if (strcmp(*argv, "-download_status") == 0)
        {
            command = FBE_CLI_DCS_CMD_TYPE_GET_DOWNLOAD_STATS;
        }
        else if (strcmp(*argv, "-download_abort") == 0)
        {
            command = FBE_CLI_DCS_CMD_TYPE_DOWNLOAD_ABORT;
        }
        else if (strcmp(*argv, "-dieh_force_clear_update") == 0)  /* back door to clear DIEH table ref counts, in case we find a bug.*/
        {
           command = FBE_CLI_DCS_CMD_DIEH_FORCE_CLEAR_UPDATE;
        }

        argc--;
        argv++;

    }   /* end of while */

    if (command == FBE_CLI_DCS_CMD_TYPE_START_DOWNLOAD)
    {
        /* check for invalid settings */
        if (trial_run && is_force_download)
        {
            fbe_cli_printf("\nError: Cannot specify both -trial_run and -force\n");
            return;
        }
        if (single_drive_request){
            if ((bus == 0xFF) || (enclosure == 0xFF) || (slot == 0xFF)){
                fbe_cli_printf("\nError: Missing parameter -b <bus> -e <enclosure> -s <slot>.\n");
                fbe_cli_printf("\nPlease specify -b -e -s or use -all option \n\n");
                fbe_cli_printf("%s", DRIVE_CONFIG_USAGE);
            }else{
                if ((bus >= FBE_API_PHYSICAL_BUS_COUNT) || (enclosure >= FBE_API_ENCLOSURES_PER_BUS)
                        || (slot >= FBE_API_ENCLOSURE_SLOTS)){
                    fbe_cli_printf("\nError: Invalid value specified for parameter -b <bus> -e <enclosure> -s <slot>.\n");
                    status = FBE_STATUS_GENERIC_FAILURE;
                }else{
                    status = fbe_cli_drive_config_start_download(file, fast_download, trial_run, check_tla, is_force_download, bus, enclosure, slot);
                }
            }
        }else{
            status = fbe_cli_drive_config_start_download_all(file, fast_download, trial_run, check_tla, is_force_download);
        }
    }
    else if (command == FBE_CLI_DCS_CMD_TYPE_GET_DOWNLOAD_STATS)
    {
        status = fbe_cli_drive_config_get_download_stats();
    }
    else if (command == FBE_CLI_DCS_CMD_TYPE_DOWNLOAD_ABORT)
    {
        status = fbe_api_drive_configuration_interface_abort_download();
    }
    else if (command == FBE_CLI_DCS_CMD_DIEH_FORCE_CLEAR_UPDATE)
    {
        status = fbe_cli_drive_config_dieh_force_clear_update();
    }

    else
    {
        fbe_cli_error("\nNo valid command specified\n");
        fbe_cli_printf("%s", DRIVE_CONFIG_USAGE);
        return;
    }

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nCommand failed status: 0x%x.\n", status);
        return;
    }
    return;
}      
/******************************************
 * end fbe_cli_cmd_drive_config()
 ******************************************/


/*!**************************************************************
 * fbe_cli_cmd_drive_config_srvc()
 ****************************************************************
 * @brief
 *   This is a service mode function which allows viewing and 
 *   changing attributes for drive configuration service.
 *  
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  11/14/2012  Wayne Garrett - Created.
 *
 ****************************************************************/

void fbe_cli_cmd_drive_config_srvc(int argc , char ** argv)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_cli_dcs_cmd_type command = FBE_CLI_DCS_CMD_TYPE_INVALID;
    fbe_time_t timeout = 30000;
    fbe_u32_t value = 0;
    fbe_bool_t do_enable = FBE_FALSE;   
    fbe_u8_t mode_select_page = 0;
    fbe_u8_t mode_select_byte = 0; 
    fbe_u8_t mode_select_mask = 0; 
    fbe_u8_t mode_select_value = 0;

    /*
     * Parse the command line.
     */
    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /* If they are asking for help, just display help and exit.
             */
            fbe_cli_printf("%s", DRIVE_CONFIG_SRVC_USAGE);
            return;
        }
        else if (strcmp(*argv, "-get_mode_page_overrides") == 0)
        {
            command = FBE_CLI_DCS_CMD_TYPE_GET_MODE_PAGE_OVERRIDES;
            value = FBE_DCS_MP_OVERRIDE_TABLE_ID_ADDITIONAL;
            argc--;
            argv++;
            if (argc > 0)
            {
                if (strcmp(*argv, "-def") == 0)
                {
                    value = FBE_DCS_MP_OVERRIDE_TABLE_ID_DEFAULT;
                }
                else if (strcmp(*argv, "-sam") == 0)
                {
                    value = FBE_DCS_MP_OVERRIDE_TABLE_ID_SAMSUNG;
                }
                else if (strcmp(*argv, "-min") == 0)
                {
                    value = FBE_DCS_MP_OVERRIDE_TABLE_ID_MINIMAL;
                }
                else if (strcmp(*argv, "-addl") == 0)
                {
                    value = FBE_DCS_MP_OVERRIDE_TABLE_ID_ADDITIONAL;
                }
                else
                {
                    fbe_cli_error("invalid arguments\n");
                    return;
                }
            }
            break;
        }
        else if (strcmp(*argv, "-set_mode_page_byte") == 0)
        {
            command = FBE_CLI_DCS_CMD_TYPE_SET_MODE_PAGE_BYTE;
            value = FBE_DCS_MP_OVERRIDE_TABLE_ID_ADDITIONAL;
            argc--;
            argv++;
            if(argc < 4)
            {
                fbe_cli_error("too few arguments\n");
                return;
            }
            if (argc > 4)  /* option was provided */
            {            
                if (strcmp(*argv, "-def") == 0)
                {
                    value = FBE_DCS_MP_OVERRIDE_TABLE_ID_DEFAULT;
                }
                else if (strcmp(*argv, "-sam") == 0)
                {
                    value = FBE_DCS_MP_OVERRIDE_TABLE_ID_SAMSUNG;
                }
                else if (strcmp(*argv, "-min") == 0)
                {
                    value = FBE_DCS_MP_OVERRIDE_TABLE_ID_MINIMAL;
                }
                else if (strcmp(*argv, "-addl") == 0)
                {
                    value = FBE_DCS_MP_OVERRIDE_TABLE_ID_ADDITIONAL;
                }
                else
                {
                    fbe_cli_error("invalid arguments\n");
                    return;
                }
            }
            mode_select_page = (fbe_u8_t)strtoul(argv[0], NULL, 16);
            mode_select_byte = (fbe_u8_t)strtoul(argv[1], NULL, 16);
            mode_select_mask = (fbe_u8_t)strtoul(argv[2], NULL, 16);
            mode_select_value = (fbe_u8_t)strtoul(argv[3], NULL, 16);
            break;
        }
        else if (strcmp(*argv, "-clear_mode_page_overrides") == 0)
        {
            command = FBE_CLI_DCS_CMD_TYPE_CLEAR_MODE_PAGE_OVERRIDES;
            break;
        }
        else if (strcmp(*argv, "-display_params") == 0)
        {
            command = FBE_CLI_DCS_CMD_TYPE_DISPLAY_PARAMS;
            break;
        }
        else if (strcmp(*argv, "-set_stime") == 0)
        {
            /* get msec arg */
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("too few arguments\n");
                return;
            }
            timeout = (fbe_u32_t)strtoul(*argv, 0, 0);
            command = FBE_CLI_DCS_CMD_TYPE_SET_SERVICE_TIMEOUT;
            break;
        }
        else if (strcmp(*argv, "-set_remap_stime") == 0)
        {
            /* get msec arg */
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("too few arguments\n");
                return;
            }
            timeout = (fbe_u32_t)strtoul(*argv, 0, 0);
            command = FBE_CLI_DCS_CMD_TYPE_SET_REMAP_SERVICE_TIMEOUT;
            break;
        }
        else if (strcmp(*argv, "-set_fw_image_chunk") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("too few arguments\n");
                return;
            }
            value = (fbe_u32_t)strtoul(*argv, 0, 0);  /* user provides value in KB */
            value *= 1024;  /* convert KB to Bytes*/
            command = FBE_CLI_DCS_CMD_TYPE_SET_FW_IMAGE_CHUNK_SIZE;
            break;
        }
        else
        {
            const dcs_tunables_table_t *p_table = &Dcs_Tunables_Table[0];

            while (p_table->cmd != FBE_CLI_DCS_CMD_TYPE_INVALID)
            {
                if (strcmp(*argv, p_table->cmd_str) == 0)
                {
                    /* get value arg*/
                    argc--;
                    argv++;
                    if(argc == 0)
                    {
                        fbe_cli_error("too few arguments\n");
                        return;
                    }
                    value = (fbe_u32_t)strtoul(*argv, 0, 0);
                    if (value !=0 && value != 1)
                    {
                        fbe_cli_error("invalid arguments\n");
                        return;
                    }
                    do_enable = (value==1)? FBE_TRUE : FBE_FALSE;
                    command = p_table->cmd;
                    break;
                }
                p_table++;
            }
        }
 
        argc--;
        argv++;
    }   /* end of while */

    if (command == FBE_CLI_DCS_CMD_TYPE_DISPLAY_PARAMS)
    {
        status = fbe_cli_drive_config_display_params();
    }
    else if (command == FBE_CLI_DCS_CMD_TYPE_SET_SERVICE_TIMEOUT)
    {
        status = fbe_api_drive_configuration_set_service_timeout(timeout);
    }
    else if (command == FBE_CLI_DCS_CMD_TYPE_SET_REMAP_SERVICE_TIMEOUT)
    {
        status = fbe_api_drive_configuration_set_remap_service_timeout(timeout);
    }
    else if (command == FBE_CLI_DCS_CMD_TYPE_SET_HEALTHCHECK_ACTION)
    {
        status = fbe_api_drive_configuration_set_control_flag(FBE_DCS_HEALTH_CHECK_ENABLED, do_enable);
    }
    else if (command == FBE_CLI_DCS_CMD_TYPE_SET_REMAP_RETRIES_ACTION)
    {
        status = fbe_api_drive_configuration_set_control_flag(FBE_DCS_REMAP_RETRIES_ENABLED, do_enable);
    }
    else if (command == FBE_CLI_DCS_CMD_TYPE_SET_REMAP_HC_FOR_NON_RETRYABLE_ACTION)
    {
        status = fbe_api_drive_configuration_set_control_flag(FBE_DCS_REMAP_HC_FOR_NON_RETRYABLE, do_enable);
    }
    else if (command == FBE_CLI_DCS_CMD_TYPE_SET_FWDL_FAIL_AFTER_MAX_RETRIES)
    {
        status = fbe_api_drive_configuration_set_control_flag(FBE_DCS_FWDL_FAIL_AFTER_MAX_RETRIES, do_enable);
    }   
    else if (command == FBE_CLI_DCS_CMD_TYPE_SET_ENFORCE_ENHANCED_QUEUING_DRIVES)
    {
        status = fbe_api_drive_configuration_set_control_flag(FBE_DCS_ENFORCE_ENHANCED_QUEUING_DRIVES, do_enable);
    }
    else if (command == FBE_CLI_DCS_CMD_TYPE_SET_DRIVE_LOCKUP_RECOVERY)
    {
        status = fbe_api_drive_configuration_set_control_flag(FBE_DCS_DRIVE_LOCKUP_RECOVERY, do_enable);
    }
    else if (command == FBE_CLI_DCS_CMD_TYPE_SET_TIMEOUT_QUIESCE_HANDLING)
    {
        status = fbe_api_drive_configuration_set_control_flag(FBE_DCS_DRIVE_TIMEOUT_QUIESCE_HANDLING, do_enable);
    }    
    else if (command == FBE_CLI_DCS_CMD_TYPE_SET_PFA_HANDLING)
    {
        status = fbe_api_drive_configuration_set_control_flag(FBE_DCS_PFA_HANDLING, do_enable);
    }    
    else if (command == FBE_CLI_DCS_CMD_TYPE_SET_MODE_SELECT_CAPABILITY)
    {
        status = fbe_api_drive_configuration_set_control_flag(FBE_DCS_MODE_SELECT_CAPABILITY, do_enable);
    }
    else if (command == FBE_CLI_DCS_CMD_TYPE_GET_MODE_PAGE_OVERRIDES)
    {
        status = fbe_cli_drive_config_get_mode_page_overrides(value);        
    }
    else if (command == FBE_CLI_DCS_CMD_TYPE_SET_MODE_PAGE_BYTE)
    {
        status = fbe_api_drive_configuration_interface_set_mode_page_byte(value, mode_select_page, mode_select_byte, mode_select_mask, mode_select_value);
    }
    else if (command == FBE_CLI_DCS_CMD_TYPE_CLEAR_MODE_PAGE_OVERRIDES)
    {
        status = fbe_api_drive_configuration_interface_mode_page_addl_override_clear();
    }
    else if (command == FBE_CLI_DCS_CMD_TYPE_SET_BREAK_UP_FW_IMAGE)
    {
        status = fbe_api_drive_configuration_set_control_flag(FBE_DCS_BREAK_UP_FW_IMAGE, do_enable);
    }
    else if (command == FBE_CLI_DCS_CMD_TYPE_SET_FW_IMAGE_CHUNK_SIZE)
    {
        status = fbe_api_drive_configuration_set_fw_image_chunk_size(value);
    }
    else if (command == FBE_CLI_DCS_CMD_TYPE_SET_4K_ENABLED)       
    {
        status = fbe_api_drive_configuration_set_control_flag(FBE_DCS_4K_ENABLED, do_enable);
    }
    else if (command == FBE_CLI_DCS_CMD_TYPE_SET_IGNORE_INVALID_IDENTITY)       
    {
#ifdef FBE_HANDLE_UNKNOWN_TLA_SUFFIX
        status = fbe_api_drive_configuration_set_control_flag(FBE_DCS_IGNORE_INVALID_IDENTITY, do_enable);
#endif
    }
    else
    {
        fbe_cli_error("\nNo valid command specified\n");
        fbe_cli_printf("%s", DRIVE_CONFIG_SRVC_USAGE);
        return;
    }

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nCommand failed status: 0x%x.\n", status);
        return;
    }
    return;
}      
/******************************************
 * end fbe_cli_cmd_drive_config_srvc()
 ******************************************/



/*!**************************************************************
 * fbe_cli_drive_config_start_download()
 ****************************************************************
 * @brief
 *  This function sends a download command to DMO.
 *
 * @param file - file name.
 * @param fast_download
 * @param check_tla
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  03/28/2011 - Created. chenl6
 *  03/21/2012:  Adapted for drive configuration service interface.
 *
 ****************************************************************/
fbe_status_t fbe_cli_drive_config_start_download(fbe_u8_t *file, fbe_bool_t fast_download, fbe_bool_t trial_run, fbe_bool_t check_tla, fbe_bool_t is_force_download,
                                                  fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_drive_configuration_control_fw_info_t fw_info;
    fbe_drive_selected_element_t *drive;
    
    /* Read f/w image from file. */    
    status = fbe_cli_drive_config_fwdl_read_image_file(&fw_info, file);
    if (status != FBE_STATUS_OK){
        fbe_cli_printf("\n%s Error returned from fbe_cli_drive_config_fwdl_read_image_file. Status %d \n",
                       __FUNCTION__, status);
        return status;
    }
    fw_info.header_image_p->cflags =  FBE_FDF_CFLAGS_ONLINE | FBE_FDF_CFLAGS_TRAILER_PRESENT;
    fw_info.header_image_p->cflags |= FBE_FDF_CFLAGS_SELECT_DRIVE;

    if (fast_download){
        fw_info.header_image_p->cflags2 |= FBE_FDF_CFLAGS2_FAST_DOWNLOAD;
    }
    if (trial_run){
        fw_info.header_image_p->cflags2 |= FBE_FDF_CFLAGS2_TRIAL_RUN;
    }
    if (check_tla){
        fw_info.header_image_p->cflags2 |= FBE_FDF_CFLAGS2_CHECK_TLA;
    }
    if (is_force_download)
    {
        fw_info.header_image_p->cflags2 |= FBE_FDF_CFLAGS2_FORCE;
    }

    /* Download to only drive at a time is supported for now.*/
    fw_info.header_image_p->num_drives_selected = 1;
    drive = &(fw_info.header_image_p->first_drive);
    drive->bus = bus;
    drive->enclosure = enclosure;
    drive->slot = slot;

    fbe_copy_memory(fw_info.header_image_p->ofd_filename, file, CSX_MIN((fbe_u32_t)strlen(file),FBE_FDF_FILENAME_LEN));
    

    /* Send download request to drive config service. */
    status = fbe_api_drive_configuration_interface_download_firmware(&fw_info);
    if (status == FBE_STATUS_OK){
        fbe_cli_printf("\n Successfully initiated firmware download. Check status using -download_status option. \n");

    }else{
        fbe_cli_printf("\n%s Error returned from fbe_api_drive_configuration_interface_download_firmware. Status %d \n",
                       __FUNCTION__, status);        
    }

    fbe_api_free_memory(fw_info.data_image_p);
    fbe_api_free_memory(fw_info.header_image_p);

    return status;
}
/******************************************
 * end fbe_cli_drive_config_start_download()
 ******************************************/

/*!**************************************************************
 * fbe_cli_drive_config_start_download_all()
 ****************************************************************
 * @brief
 *  This function sends a download command to DMO.
 *
 * @param file - file name.
 * @param fast_download
 * @param check_tla
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  03/28/2011 - Created. chenl6
 *  03/21/2012:  Adapted for drive configuration service interface.
 *
 ****************************************************************/
fbe_status_t fbe_cli_drive_config_start_download_all(fbe_u8_t *file, fbe_bool_t fast_download, fbe_bool_t trial_run, fbe_bool_t check_tla, fbe_bool_t is_force_download)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_drive_configuration_control_fw_info_t fw_info;
    
    
    /* Read f/w image from file. */    
    status = fbe_cli_drive_config_fwdl_read_image_file(&fw_info, file);
    if (status != FBE_STATUS_OK){
        fbe_cli_printf("\n%s Error returned from fbe_cli_drive_config_fwdl_read_image_file. Status %d \n",
                       __FUNCTION__, status);
        return status;
    }
    fw_info.header_image_p->cflags =  FBE_FDF_CFLAGS_ONLINE | FBE_FDF_CFLAGS_TRAILER_PRESENT;

    if (fast_download){
        fw_info.header_image_p->cflags2 |= FBE_FDF_CFLAGS2_FAST_DOWNLOAD;
    }
    if (trial_run){
        fw_info.header_image_p->cflags2 |= FBE_FDF_CFLAGS2_TRIAL_RUN;
    }
    if (check_tla){
        fw_info.header_image_p->cflags2 |= FBE_FDF_CFLAGS2_CHECK_TLA;
    }
    if (is_force_download)
    {
        fw_info.header_image_p->cflags2 |= FBE_FDF_CFLAGS2_FORCE;
    }

    
    fbe_copy_memory(fw_info.header_image_p->ofd_filename, file, CSX_MIN((fbe_u32_t)strlen(file),FBE_FDF_FILENAME_LEN));
    
    /* Send download request to drive config service. */
    status = fbe_api_drive_configuration_interface_download_firmware(&fw_info);
    if (status != FBE_STATUS_OK){
        fbe_cli_printf("\n%s Error returned from fbe_api_drive_configuration_interface_download_firmware. Status %d \n",
                       __FUNCTION__, status);    
    }

    fbe_api_free_memory(fw_info.data_image_p);
    fbe_api_free_memory(fw_info.header_image_p);

    return status;
}
/******************************************
 * end fbe_cli_drive_config_start_download_all()
 ******************************************/

/*!**************************************************************
 * fbe_cli_drive_config_get_download_stats()
 ****************************************************************
 * @brief
 *  This function sends a download command to DMO.
 *
 * @param file - file name.
 * @param fast_download
 * @param check_tla
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  03/28/2011 - Created. chenl6
 *  03/21/2012:  Adapted for drive configuration service interface.
 *
 ****************************************************************/
fbe_status_t fbe_cli_drive_config_get_download_stats(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t returned_count = 0;
    fbe_u32_t i;
    fbe_drive_configuration_download_get_drive_info_t get_all[FBE_MAX_DOWNLOAD_DRIVES] = {0};  /*TODO: currently code is hardcode to max 1000.  This should be fixed.*/
    fbe_drive_configuration_download_get_process_info_t dl_status = {0};

    status = fbe_api_drive_configuration_interface_get_download_process(&dl_status);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Failed to get download status.\n", __FUNCTION__);
        return status;
    }

    fbe_cli_printf("\n*************** DOWNLOAD PROCESS *****************\n");
    fbe_cli_printf("IN PROGRESS: %d\n", dl_status.in_progress);
    fbe_cli_printf("STATE: %s\n", get_download_state_string(dl_status.state));
    fbe_cli_printf("FAIL REASON: %s\n", get_download_fail_reason_string(dl_status.fail_code));
    fbe_cli_printf("START TIME: %02d:%02d:%02d \n", 
        dl_status.start_time.hour, dl_status.start_time.minute, dl_status.start_time.second);
    fbe_cli_printf("END TIME: %02d:%02d:%02d \n", 
        dl_status.stop_time.hour, dl_status.stop_time.minute, dl_status.stop_time.second);
    fbe_cli_printf("FDF FILE: %s\n", dl_status.fdf_filename);


    status = fbe_api_drive_configuration_interface_get_all_download_drives(get_all, FBE_MAX_DOWNLOAD_DRIVES, &returned_count);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Failed to get all download drives.\n", __FUNCTION__);
        return status;
    }

    for (i=0; i<returned_count; i++)
    {
        fbe_cli_printf("\t---------------------\n");
        fbe_cli_printf("\t[%3d]:          %d_%d_%d\n", i, get_all[i].bus, get_all[i].enclosure, get_all[i].slot);
        fbe_cli_printf("\tstate:          (%d) %s\n", get_all[i].state, get_download_drive_state_string(get_all[i].state));
        fbe_cli_printf("\tfail reason:    (%d) %s\n", get_all[i].fail_code, get_download_drive_fail_reason_string(get_all[i].fail_code));
        fbe_cli_printf("\tprior rev:      %s\n", get_all[i].prior_product_rev);
    }

    return status;
}
/******************************************
 * end fbe_cli_drive_config_get_download_stats()
 ******************************************/


/*!**************************************************************
 * fbe_cli_drive_config_dieh_force_clear_update()
 ****************************************************************
 * @brief
 *  Clears the DCS DIEH update state.  This can be
 *  used if an issue is found with our reference counting and
 *  a new table cannot be loaded.  
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  10/04/2012:  Wayne Garrett - Created
 *
 ****************************************************************/
static fbe_status_t fbe_cli_drive_config_dieh_force_clear_update(void)
{
    fbe_status_t status = FBE_STATUS_OK;

    status = fbe_api_drive_configuration_interface_dieh_force_clear_update();
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Failed to clear update state.\n", __FUNCTION__);
        return status;
    }

    return status;
}

/*!**************************************************************
 * get_download_state_string()
 ****************************************************************
 * @brief
 *  This function sends a drivegetlog command to DMO.
 *
 * @param bus - bus number.
 * @param enclosure - enclosure position.
 * @param slot - slot number.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  03/28/2011 - Created. chenl6
 *  03/21/2012:  Adapted for drive configuration service interface.
 *
 ****************************************************************/
fbe_u8_t * get_download_state_string(fbe_drive_configuration_download_process_state_t state)
{
    fbe_u8_t * string = NULL;

    switch (state)
    {
        case FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_IDLE:
            string = "IDLE";
            break;
        case FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_RUNNING:
            string = "RUNNING";
            break;
        case FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAILED:
            string = "FAILED";
            break;
        case FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_ABORTED:
            string = "ABORTED";
            break;
        case FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_SUCCESSFUL:
            string = "SUCCESSFUL";
            break;
        default:
            string = "UNKNOWN";
            break;
    }

    return string;
}
/******************************************
 * end get_download_state_string()
 ******************************************/

/*!**************************************************************
 * get_download_drive_state_string()
 ****************************************************************
 * @brief
 *  This function returns string representation for drive 
 *  download state.
 *
 * @param state - download drive state
 *
 * @return string
 *
 * @author
 *  12/18/2012 - Wayne Garrett Created. 
 *
 ****************************************************************/
static fbe_u8_t * get_download_drive_state_string(fbe_drive_configuration_download_drive_state_t state)
{
    fbe_u8_t * string = NULL;

    switch(state)
    {
        case FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_IDLE:
            string = "IDLE";
            break;

        case FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_INIT:
            string = "INIT";
            break;

        case FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_ACTIVE:
            string = "ACTIVE";
            break;

        case FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_COMPLETE:
            string = "COMPLETE";
            break;

        case FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_COMPLETE_TRIAL_RUN:
            string = "TRIAL_RUN";
            break;

        case FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL:
            string = "FAIL";
            break;

        default:
            string = "UNKNOWN";
            break;
    }
    return string;
}


/*!**************************************************************
 * get_download_fail_reason_string()
 ****************************************************************
 * @brief
 *  This function sends a drivegetlog command to DMO.
 *
 * @param bus - bus number.
 * @param enclosure - enclosure position.
 * @param slot - slot number.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  03/28/2011 - Created. chenl6
 *  03/21/2012:  Adapted for drive configuration service interface.
 *
 ****************************************************************/
fbe_u8_t * get_download_fail_reason_string(fbe_drive_configuration_download_process_fail_t fail_reason)
{
    fbe_u8_t * string = NULL;

    switch (fail_reason)
    {
        case FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE:
            string = "NO FAILURE";
            break;
        case FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_NONQUALIFIED:
            string = "NONQUALIFIED";
            break;
        case FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_NO_DRIVES_TO_UPGRADE:
            string = "NO DRIVE TO UPGRADE";
            break;
        case FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_FW_REV_MISMATCH:
            string = "REV MISMATCH";
            break;
        case FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_WRITE_BUFFER:
            string = "WRITE BUFFER";
            break;
        case FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_BRICK_DRIVE:
            string = "BRICK DRIVE";
            break;
       case FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_ABORTED:
            string = "ABORTED";
            break;
        default:
            string = "UNKNOWN";
            break;
    }

    return string;
}
/******************************************
 * end get_download_fail_reason_string()
 ******************************************/

/*!**************************************************************
 * get_download_drive_fail_reason_string()
 ****************************************************************
 * @brief
 *  This function returns string representation for drive 
 *  fail reason.
 *
 * @param fail_reason - drive fail reason
 *
 * @return string
 *
 * @author
 *  12/18/2012 - Wayne Garrett Created. 
 *
 ****************************************************************/
static fbe_u8_t * get_download_drive_fail_reason_string(fbe_drive_configuration_download_drive_fail_t fail_reason)
{
    fbe_u8_t * string = NULL;

    switch(fail_reason)
    {
        case FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_NO_FAILURE:
            string = "NONE";
            break;
    
        case FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_NONQUALIFIED:
            string = "NONQUAL";
            break;
    
        case FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_WRITE_BUFFER:
            string = "WRITE_BUFFER";
            break;
    
        case FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_POWER_UP:
            string = "POWER_UP";
            break;
    
        case FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_WRONG_REV:
            string = "WRONG_REV";
            break;
    
        case FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_ABORTED:
            string = "ABORTED";
            break;
    
        case FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_GENERIC_FAILURE:
            string = "GENERIC";
            break;

        default:
            string = "UNKNOWN";
            break;
    }

    return string;
}


/*!**************************************************************
 * @fn fbe_cli_drive_config_fwdl_read_image_file
 ****************************************************************
 * @brief
 *  This function reads a drive firmware file to download. 
 * 
 * @param drive_mgmt - pointer to drive mgmt object.
 * @param file - pointer to file name.
 * 
 * @return fbe_status_t 
 *
 * @version
 *   28-Feb-2011:  chenl6 - Created.
 *  03/21/2012:  Adapted for drive configuration service interface.
 *
 ****************************************************************/
fbe_status_t fbe_cli_drive_config_fwdl_read_image_file(fbe_drive_configuration_control_fw_info_t *fw_info,
                                                       fbe_u8_t *file)
{
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_file_handle_t                    fp = NULL;
    fbe_u32_t                            nbytes = 0;

    /* Open file. */
    fp = fbe_file_open(file, FBE_FILE_RDONLY, 0, NULL);
    if(fp == FBE_FILE_INVALID_HANDLE)
    {
        fbe_cli_error("\n%s Could not open file %s. \n", __FUNCTION__, file);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocate header memory. */
    fw_info->header_image_p = (fbe_fdf_header_block_t  *)fbe_api_allocate_memory(FBE_FDF_INSTALL_HEADER_SIZE);
    if (fw_info->header_image_p == NULL)
    {        
        fbe_file_close(fp);
        fbe_cli_error("\n%s Could not allocate header.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Read header from file. */
    nbytes = fbe_file_read(fp, fw_info->header_image_p, FBE_FDF_INSTALL_HEADER_SIZE, NULL);
    if(nbytes != FBE_FDF_INSTALL_HEADER_SIZE)
    {
        fbe_cli_error("\n%s Could not read header from file nbytes:%d.\n", __FUNCTION__,nbytes);
        fbe_api_free_memory(fw_info->header_image_p);
        fbe_file_close(fp);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fw_info->header_image_p->header_marker = BYTE_SWAP_32(fw_info->header_image_p->header_marker);
    fw_info->header_image_p->image_size = BYTE_SWAP_32(fw_info->header_image_p->image_size);
    fw_info->header_image_p->trailer_offset = BYTE_SWAP_16(fw_info->header_image_p->trailer_offset);

    /* Verify header */
    if (fw_info->header_image_p->header_marker != FBE_FDF_HEADER_MARKER)
    {
        fbe_api_free_memory(fw_info->header_image_p);		
        fbe_file_close(fp);
        fbe_cli_error("\n%s Header corrupted.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fw_info->header_size = FBE_FDF_INSTALL_HEADER_SIZE;

    /* Allocate image memory*/
    fw_info->data_image_p = (fbe_u8_t *)fbe_api_allocate_memory(fw_info->header_image_p->image_size);    
    if (fw_info->data_image_p == NULL)
    {
        fbe_api_free_memory(fw_info->header_image_p);		
        fbe_file_close(fp);
        fbe_cli_error("\n%s Could not allocate image.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Read firmware image from file. */
    nbytes = fbe_file_read(fp, fw_info->data_image_p, fw_info->header_image_p->image_size, NULL);
    if(nbytes != fw_info->header_image_p->image_size)
    {
        fbe_cli_error("\n%s Could not read image from file nbytes:%d, image_size:%d\n", __FUNCTION__, nbytes, fw_info->header_image_p->image_size);
        fbe_api_free_memory(fw_info->data_image_p);
        fbe_api_free_memory(fw_info->header_image_p);
        fbe_file_close(fp);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fw_info->data_size = fw_info->header_image_p->image_size;
    /* Close file. */
    fbe_file_close(fp);

    return status;
}
/******************************************************
 * end fbe_cli_drive_config_fwdl_read_image_file() 
 ******************************************************/


/*!**************************************************************
 * @fn fbe_cli_drive_config_display_params
 ****************************************************************
 * @brief
 *  This function displays the tunable parameters.
 * 
 * @return fbe_status_t 
 *
 * @version
 *  11/15/2012:  Wayne Garrett  - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_cli_drive_config_display_params(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dcs_tunable_params_t params = {0};

    status = fbe_api_drive_configuration_interface_get_params(&params);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Failed to get params.\n", __FUNCTION__);
        return status;
    }

    fbe_cli_printf("Health Check enabled            : %s\n", (params.control_flags & FBE_DCS_HEALTH_CHECK_ENABLED)?"yes":"no");
    fbe_cli_printf("Remap retries enabled           : %s\n", (params.control_flags & FBE_DCS_REMAP_RETRIES_ENABLED)?"yes":"no");
    fbe_cli_printf("HC for Remap non retryable      : %s\n", (params.control_flags & FBE_DCS_REMAP_HC_FOR_NON_RETRYABLE)?"yes":"no");
    fbe_cli_printf("FWDL fail on max retry          : %s\n", (params.control_flags & FBE_DCS_FWDL_FAIL_AFTER_MAX_RETRIES)?"yes":"no");
    fbe_cli_printf("Enforce enhanced queuing drives : %s\n", (params.control_flags & FBE_DCS_ENFORCE_ENHANCED_QUEUING_DRIVES)?"yes":"no");
    fbe_cli_printf("Drive lockup recovery           : %s\n", (params.control_flags & FBE_DCS_DRIVE_LOCKUP_RECOVERY)?"yes":"no");
    fbe_cli_printf("Drive timeout quiesce           : %s\n", (params.control_flags & FBE_DCS_DRIVE_TIMEOUT_QUIESCE_HANDLING)?"yes":"no");
    fbe_cli_printf("PFA handling                    : %s\n", (params.control_flags & FBE_DCS_PFA_HANDLING)?"yes":"no");
    fbe_cli_printf("Mode Select capability          : %s\n", (params.control_flags & FBE_DCS_MODE_SELECT_CAPABILITY)?"yes":"no");
    fbe_cli_printf("Always break up fw image        : %s\n", (params.control_flags & FBE_DCS_BREAK_UP_FW_IMAGE)?"yes":"no");
    fbe_cli_printf("4K enabled                      : %s\n", (params.control_flags & FBE_DCS_4K_ENABLED)?"yes":"no");               
    fbe_cli_printf("Invalid identity handling       : %s\n", (params.control_flags & FBE_DCS_IGNORE_INVALID_IDENTITY)?"yes":"no");
    fbe_cli_printf("Service Time Limit              : %d ms\n",(fbe_u32_t)params.service_time_limit_ms);
    fbe_cli_printf("Remap Service Time Limit        : %d ms\n", (fbe_u32_t)params.remap_service_time_limit_ms);
    fbe_cli_printf("Degraded Service Time Limit     : %d ms\n", (fbe_u32_t)params.emeh_service_time_limit_ms);
    fbe_cli_printf("FW image chunk size             : %d KB\n", (fbe_u32_t)params.fw_image_chunk_size/1024);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_cli_drive_config_get_mode_page_overrides
 ****************************************************************
 * @brief
 *  This function displays mode page override table.
 * 
 * @param table_id  - ID for specific override table
 * @return fbe_status_t 
 *
 * @version
 *  04/01/2013:  Wayne Garrett  - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_cli_drive_config_get_mode_page_overrides(fbe_drive_configuration_mode_page_override_table_id_t table_id)
{
    fbe_dcs_mode_select_override_info_t ms_info = {0};
    fbe_status_t status;
    fbe_u32_t i;

    ms_info.table_id = table_id;
    ms_info.num_entries = DCS_MS_INVALID_ENTRY;

    status = fbe_api_drive_configuration_interface_get_mode_page_overrides(&ms_info);

    if (FBE_STATUS_OK != status)
    {
        return status;
    }

    if (DCS_MS_INVALID_ENTRY == ms_info.num_entries)
    {
        fbe_cli_printf("Invalid num entries\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_cli_printf("Page | Byte | Mask | Value\n");
    fbe_cli_printf("--------------------------\n");
    for (i=0; i<ms_info.num_entries; i++)
    {
        fbe_cli_printf("0x%02x   0x%02x   0x%02x   0x%02x\n", 
                       ms_info.table[i].page, ms_info.table[i].byte_offset, ms_info.table[i].mask, ms_info.table[i].value);
    }

    return FBE_STATUS_OK;
}

/*************************
 * end file fbe_cli_lib_drive_config_cmds.c
 *************************/
