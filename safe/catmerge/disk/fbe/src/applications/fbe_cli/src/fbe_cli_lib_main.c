#include "fbe/fbe_cli.h"
#include "fbe_cli_neit.h"
#include "fbe_cli_dest.h"
#include "fbe_cli_rdt.h"
#include "fbe_cli_wldt.h"
#include "fbe_cli_rderr.h"
#include "fbe_cli_rginfo.h"
#include "fbe_cli_sector_trace_info.h"
#include "fbe_cli_perf_stats.h"
#include "fbe_cli_dps_stats.h"
#include "fbe/fbe_package.h"
#include "fbe_cli_bvd.h"
#include "fbe_cli_scheduler_service.h"
#include "fbe_cli_firmware_cmd.h"
#include "fbe_cli_event_log_cmds.h"
#include "fbe_cli_cmi_service.h"
#include "fbe_cli_lib_collectall_cmd.h"
#include "fbe_cli_services.h"
#include "fbe_cli_lib_persist_cmd.h"
#include "fbe_cli_trace_limits.h"
#include "fbe_cli_lib_provision_drive_cmds.h"
#include "fbe_cli_panic_sp.h"
#include "fbe_cli_lib_module_mgmt_cmds.h"
#include "fbe_cli_lib_energy_stats_cmds.h"
#include "fbe_cli_lib_disk_info_cmd.h"
#include "fbe_cli_lib_ps_mgmt_cmds.h"
#include "fbe_cli_lib_enclosure_mgmt_cmds.h"
#include "fbe_cli_drive_mgmt_obj.h"
#include "fbe_cli_lib_cooling_mgmt_cmds.h"
#include "fbe_cli_lib_green_cmds.h"
#include "fbe_cli_ddt.h"
#include "fbe_cli_wisd.h"
#include "fbe_cli_lib_odt.h"
#include "fbe_cli_luninfo.h"
#include "fbe_cli_database.h"
#include "fbe_cli_lib_sfp_info_cmds.h"
#include "fbe_cli_lib_get_wwn_cmds.h"
#include "fbe_cli_lib_resume_prom_cmds.h"
#include "fbe_cli_cms.h"
#include "fbe_cli_sp_cmd.h"
#include "fbe_cli_lib_sepls_cmd.h"
#include "fbe_cli_bus_cmd.h"
#include "fbe_cli_drive_config.h"
#include "fbe_cli_metadata.h"
#include "fbe_cli_lib_copy_cmds.h"
#include "fbe_cli_perf_stats.h"
#include "fbe_cli_lib_board_mgmt_cmds.h"
#include "fbe_cli_system_backup_restore_cmds.h"
#include "fbe_cli_lib_signature_cmd.h"
#include "fbe_cli_lib_npinfo.h"
#include "fbe_cli_lib_get_faults_cmds.h"
#include "fbe_cli_lib_mminfo.h"
#include "fbe_cli_lib_encryption_cmds.h"
#include "fbe_cli_emeh_cmds.h"
#include "fbe_cli_extpool.h"
#include "fbe_cli_replace_chassis.h"

#ifdef C4_INTEGRATED
#include "fbe_cli_becheck.h"
#include "fbe_cli_c4_mirror.h"
#endif

static fbe_cli_cmd_t cli_cmds[] =
{
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "list",
        "l",
        "-help",
        "list stuff.",
        fbe_cli_cmd_list,
        FBE_CLI_USER_ACCESS
        
    },
    {   FBE_CLI_CMD_TYPE_STANDALONE,
        "help",
        "h",
        "",
        "display usage information.",
        fbe_cli_cmd_print_usage,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_QUIT,
        "quit",
        "q",
        "",
        "exit from prompt mode.",
        (fbe_cli_cmd_func)0,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "access",
        "acc",
        "",
        "Sets access mode to user or eng mode",
        fbe_cli_x_access,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "clearlogs",
        "clearlogs",
        "operation <parameters>",
        "Clear Event Log Messages for specified package, e.g. pp,sep,esp",
        fbe_cli_cmd_clearlogs,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "getlogs",
        "getlogs",
        "operation <parameters>",
        "Show Event Log Messages for specified package, e.g. pp,sep,esp",
        fbe_cli_cmd_getlogs,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "getfaults",
        "getfaults",
        "",
        "Show All Array Faults for specified package, e.g. esp",
        fbe_cli_cmd_get_faults,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "trace",
        "t",
        "-help",
        "display/modify trace levels.",
        fbe_cli_cmd_trace,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "trace_limits",
        "tl",
        "operation <parameters>",
        "Set or show trace error limits information.",
        fbe_cli_trace_limits,
        FBE_CLI_USER_ACCESS
        
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "allenclbuf",
        "aebuf",
        "",
        "All enclosure Buffer comand.",
        fbe_cli_cmd_all_enclbuf,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "base_config",
        "bc",
        "",
        "show base config related information",
        fbe_cli_cmd_base_config,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "base_object",
        "bo",
        "",
        "display and change base object attributes.",
        fbe_cli_cmd_base_object,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "enclbuf",
        "eb",
        "",
        "Enclosure [ESES]Buffer commands",
        fbe_cli_cmd_enclbuf,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "encldbg",
        "ed",
        "",
        "Enclosure debug command",
        fbe_cli_cmd_enclosure_envchange,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "encledal",
        "edal",
        "",
        "Display component content of Enclosure Data Library (EDAL)",
        fbe_cli_cmd_display_encl_data,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "enclelemgrp",
        "eeg",
        "",
        "Display enclosure element group info",
        fbe_cli_cmd_enclosure_info,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "enclfwdlsts",
        "efds",
        "",
        "Display firmware download status",
        fbe_cli_cmd_display_dl_info,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "enclinq",
        "ei",
        "",
        "Enclosure Inquiry command",
        fbe_cli_cmd_raw_inquiry,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "enclprvdata",
        "epd",
        "",
        "Display enclosure private object data",
        fbe_cli_cmd_enclosure_private_data,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "enclrecdiag",
        "erd",
        "",
        "Enclosure Receive Diagnostice command (all pages)",
        fbe_cli_cmd_eses_pass_through,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "enclsetled",
        "esl",
        "",
        "LED debug commands (set/display)",
        fbe_cli_cmd_enclosure_setled,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "enclslottophy",
        "estp",
        "",
        "Display drive slot to phy mapping info",
        fbe_cli_cmd_slottophy,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "enclstat",
        "es",
        "",
        "Display enclosure status",
        fbe_cli_cmd_enclstat,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "enclstatistics",
        "estc",
        "",
        "Display Enclosure Statistics Data",
        fbe_cli_cmd_display_statistics,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "enclstrout",
        "eso",
        "",
        "Enclosure (expander)String Out command",
        fbe_cli_cmd_enclosure_expStringOut,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "enclstrin",
        "esi",
        "",
        "Enclosure (expander)String In command.",
        fbe_cli_cmd_enclosure_expStringIn,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "enclthreshin",
        "eti",
        "",
        "Threshold In command.",
        fbe_cli_cmd_enclosure_expThresholdIn,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "enclthreshout",
        "eto",
        "",
        "Enclosure (expander)Threshold out command.",
        fbe_cli_cmd_enclosure_expThresholdOut,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "eventlog",
        "elog",
        "",
        "Prints e-log trace buffer.",
        fbe_cli_cmd_enclosure_e_log_command,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "expcmds",
        "expc",
        "",
        "Sends exapander cmds and print output.",
        fbe_cli_cmd_expander_cmd,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_STANDALONE,
        "object_classes",
        "oc",
        "<class_num>",
        "display the class hierarchy of an object.",
        fbe_cli_cmd_object_classes,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "pdo",
        "pd",
        "",
        "display physical drive information\n",
        fbe_cli_cmd_physical_drive,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "boardprvdata",
        "bdprv",
        "",
        "Display the base board private data",
        fbe_cli_display_board_info,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "boardmgmt",
        "bm",
        "",
        "Displays Board Information",
        fbe_cli_cmd_boardmgmt,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "businfo",
        "bi",
        "",
        "Display backend bus information.",
        fbe_cli_cmd_bus_info,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "configmgmtport",
        "cmp",
        "",
        "Configure the management port speed setting.",
        fbe_cli_module_mgmt_config_mgmt_port,
        FBE_CLI_USER_ACCESS,
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "coolingmgmt",
        "cmgmt",
        "",
        "Display fan information",
        fbe_cli_cmd_coolingmgmt,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "drivemgmt",
        "dmo",
        "",
        "display drive management object information",
        fbe_cli_cmd_drive_mgmt,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "eirstat",
        "eir",
        "",
        "Display summary of Energy Info Reporting stats",
        fbe_cli_cmd_eirstat,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "enclmgmt",
        "em",
        "",
        "Display enclosure information",
        fbe_cli_cmd_enclmgmt,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "env_limits",
        "el",  
        "",
        "Display environment limits.",
        fbe_cli_cmd_display_env_limits,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "firmwareupgrade",
        "fup",
        "operation <parameters>",
        "Show all Firmware Upgrade related information",
        fbe_cli_cmd_firmware_upgrade,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "forcerpread",
        "frr",
        "",
        "Forces resume prom reading",
        fbe_cli_cmd_force_rp_read,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "ioports",
        "ioports",
        "",
        "Display IOM and Port Information",
        fbe_cli_cmd_ioports,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "psmgmt",
        "psmgmt",
        "",
        "Displays Power Supply Status",
        fbe_cli_cmd_psmgmt,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "resumeprom",
        "rp",
        "",
        "resume prom related commands",
        fbe_cli_get_prom_info,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "setport",
        "setp",
        "",
        "Set port.",
        fbe_cli_cmd_set_port_cmd,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "sfpinfo",
        "sfp",
        "",
        "Show all SFP related information",
        fbe_cli_cmd_sfp_info,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "spsinfo",
        "si",
        "",
        "Show all SPS related information",
        fbe_cli_cmd_spsinfo,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "bbuinfo",
        "bbuinfo",
        "",
        "Show all BBU related information",
        fbe_cli_cmd_bbuinfo,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "spstat",
        "sp",
        "",
        "Get the SP HW status.\n",
        fbe_cli_cmd_sp,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "auto_hot_spare",
        "ahs",
        "",
        "Control hot spare automatic swap",
        fbe_cli_auto_hot_spare,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "bgo_status",
        "bgos",
        "",
        "Get the status of one or more Background Operation for Disk\\RAID",
        fbe_cli_bgo_status,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "bind",
        "b",
        "<RG number> <disk number> <size>",
        "bind a Logical Unit",
        fbe_cli_bind,
        FBE_CLI_USER_ACCESS  

    },

    
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "bvd",
        "bv",
        "operation <parameters>",
        "Show BVD object related information",
        fbe_cli_bvd_commands,
        FBE_CLI_USER_ACCESS
        
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "cmi",
        "cmi",
        "",
        "Get CMI related information",
        fbe_cli_cmi_commands,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "collectall",
        "clall",
        "",
        "Display output of different fbe_cli commands",
        fbe_cli_lib_collectall_cmd,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "config_emv",
        "cgemv",
        "operation <parameters>",
        "Get/Set emv value",
        fbe_cli_config_emv,
        FBE_CLI_SRVC_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "copy",
        "copy",
        "",
        "Initial copy commands.",
        fbe_cli_copy,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "createrg",
        "crg",
        "<RG type> <RG number> <RG type> <disk names> <Size> <Expansion rate> <Idle time>",
        "Create new RG/ modify existing RG",
        fbe_cli_createrg,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "database",
        "db",
        "",
        "Get database information",
        fbe_cli_database,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "system_db_header",
        "system_db_header",
        "",
        "Print/recover/set db_header",
        fbe_cli_system_db_header,
        FBE_CLI_SRVC_ACCESS
    },   
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "driveconfig",
        "dcs",
        "",
        "display drive configuration service information",
        fbe_cli_cmd_drive_config,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "dcs_srvc",
        "dcssrvc",
        "",
        "display drive configuration service information",
        fbe_cli_cmd_drive_config_srvc,
        FBE_CLI_SRVC_ACCESS
    },

    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "getwwn",
        "gw",
        "",
        "get current World Wide Name Seed",
        fbe_cli_cmd_get_wwn,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "green",
        "gr",
        "",
        "Enable/Disable Power save",
        fbe_cli_green,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "hsconfig",
        "hs",
        "<disk number>",
        "Configure/Unconfigure/Display Configured Hot spare",
        fbe_cli_hs_configuration,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "luninfo",
        "li",
        "",
        "Display Lun configuration",
        fbe_cli_cmd_luninfo,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "cache_zero_bitmap_count",
        "czbc",
        "",
        "Get cache zero bitmap count for given Lun Capacity",
        fbe_cli_lun_get_cache_zero_bitmap_block_count,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "lustat",
        "ls",
        "<RG number> / <LUN number>",
        "display state of selected Logical Unit(s)",
        fbe_cli_lustat,
        FBE_CLI_USER_ACCESS
        
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "private_lun_access",
        "prvlunacc",
        "",
        "read/write private LUNs.",
        fbe_cli_private_luns_access,
        FBE_CLI_SRVC_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "metadata",
        "md",
        "",
        "Get metadata information",
        fbe_cli_metadata,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "persist",
        "per",
        "",
        "Persistence service related commands",
        fbe_cli_lib_persist_cmd,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "pvdinfo",
        "pvdinfo",
        "operation <parameters>",
        "Pvd information access",
        fbe_cli_provision_drive_info,
        FBE_CLI_USER_ACCESS
        
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "removerg",
        "rrg",
        "<RG number> <Object ID>",
        "Remove an existing RG",
        fbe_cli_removerg,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "extpool",
        "extpool",
        "operation <parameters>",
        "extpool",
        fbe_cli_extpool,
        FBE_CLI_SRVC_ACCESS
        
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "rginfo",
        "rginfo",
        "operation <parameters>",
        "Raid group information access",
        fbe_cli_rginfo,
        FBE_CLI_USER_ACCESS
        
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "scheduler",
        "sched",
        "",
        "Set/Get Scheduler related information",
        fbe_cli_cmd_scheduler,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "sector_trace_info",
        "stinfo",
        "operation <parameters>",
        "Sector tracing information access",
        fbe_cli_sector_trace_info,
        FBE_CLI_USER_ACCESS
        
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "error_trace_counters",
        "etc",
        "operation <parameters>",
        "Error Trace Counters access",
        fbe_cli_error_trace_counters,
        FBE_CLI_USER_ACCESS
        
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "diskinfo",
        "di",
        "",
        "Display Disk Info",
        fbe_cli_cmd_disk_info,
        FBE_CLI_USER_ACCESS
    },
    {	FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "setdefaultnonpagedmd",
        "setnp",
        "",
        "set object's default nonpaged metadata",
        fbe_cli_cmd_set_default_nonpaged_metadata,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "sepls",
        "sepls",
        "",
        "List SEP objects",
        fbe_cli_cmd_sepls,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "set_pvd_debug_flag",
        "spddf",
        "",
        "To set provision drive debug flag",
        fbe_cli_provision_drive_set_debug_flag,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "unbind",
        "ub",
        "<LUN number>",
        "unbind the selected LUN",
        fbe_cli_unbind,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "zero_disk",
        "zd",
        "",
        "Zero the specified disk",
        fbe_cli_zero_disk,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "zero_lun",
        "zl",
        "",
        "Zero the specified LUN.",
        fbe_cli_zero_lun,
        FBE_CLI_USER_ACCESS
    },

    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "lun_hard_zero",
        "lhz",
        "<Lun object id> <lba> <blocks> <threads>",
        "Hard zero a LUN",
        fbe_cli_lun_hard_zero,
        FBE_CLI_USER_ACCESS  
    },

    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "wwnseed",
        "wwnseed",
        "",
        "Set, get array midplane wwn seed info and user modified wwn seed flag.",
        fbe_cli_cmd_wwn_seed,
        FBE_CLI_USER_ACCESS
    },

    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "systemid",
        "sysid",
        "",
        "Get or set sysetm id info",
        fbe_cli_cmd_system_id,
        FBE_CLI_USER_ACCESS
     },

    {
        FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "modify_system_object_config",
        "msoc",
        "",
        "modify config of system object.",
        fbe_cli_modify_system_object_config,
        FBE_CLI_SRVC_ACCESS
    },
 
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "recreate_system_object",
        "rso",
        "",
        "recreate system PVD, RG or LUN.",
        fbe_cli_recreate_system_object,
        FBE_CLI_SRVC_ACCESS
    },

    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "destroy_broken_system_parity_rg_and_lun",
        "dbsprl",
        "",
        "destroy system parity RG and LUN if it is broken.",
        fbe_cli_destroy_broken_system_parity_rg_and_lun,
        FBE_CLI_SRVC_ACCESS
    },

    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "generate_config_for_system_parity_rg_and_lun",
        "gcsprl",
        "",
        "generate configuration for system parity RG and LUN.",
        fbe_cli_generate_config_for_system_parity_rg_and_lun,
        FBE_CLI_SRVC_ACCESS
    },

    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "recover_system_object_config",
        "rsoc",
        "",
        "reinitialize system object config.",
        fbe_cli_recover_system_object_config,
        FBE_CLI_SRVC_ACCESS
    },

    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "run_script",
        "rs",
        "",
        "Allows a script to execute FBE_CLI commands",
        fbe_cli_x_run_script,
        FBE_CLI_SRVC_ACCESS
    },    
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "rdgen",
        "rd",
        "",
        "generate I/O",
        fbe_cli_cmd_rdgen_service,
        FBE_CLI_SRVC_ACCESS
        
    }, 
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "proactivespare",
        "ps",
        "",
        "Proactive Spare",
        fbe_cli_cmd_proactive_spare,
        FBE_CLI_SRVC_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "neit",
        "ne",
        "operation <parameters>",
        "Inject an Error",
        fbe_cli_neit,
        FBE_CLI_SRVC_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "dest",
        "dt",
        "operation <parameters>",
        "Inject an Error via DEST",
        fbe_cli_dest,
        FBE_CLI_SRVC_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "rdt",
        "rdt",
        "operation <parameters>",
        "Raid driver tester operations",
        fbe_cli_rdt,
        FBE_CLI_SRVC_ACCESS
        
    },    
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "rderr",
        "rderr",
        "operation <parameters>",
        "Raid driver error injection operations",
        fbe_cli_rderr,
        FBE_CLI_SRVC_ACCESS
        
    },   
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "setverify",
        "v",
        "",
        "Enable/Disable/Configure Sniff or Background Verify",
        fbe_cli_setverify,
        FBE_CLI_SRVC_ACCESS
    },    
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "dlslock",
        "dls",
        "",
        "Cluster lock  ",
        fbe_cli_cluster_lock,
        FBE_CLI_SRVC_ACCESS
    },    
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "rebootsp",
        "reboot",
        "",
        "Reboot this SP or the Peer SP",
        fbe_cli_reboot_sp,
        FBE_CLI_SRVC_ACCESS
    },    
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "config_invalidate_db",
        "cidb",
        "",
        "Invalidate DB",
        fbe_cli_config_invalidate_db,
        FBE_CLI_SRVC_ACCESS
    },     
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "panicsp",
        "panic",
        "",
        "Panic this SP",
        fbe_cli_cmd_panic_sp,
        FBE_CLI_SRVC_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "wldt",
        "wldt",
        "operation <parameters>",
        "write log driver tester operations",
        fbe_cli_wldt,
        FBE_CLI_SRVC_ACCESS
        
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "drivemgmt_srvc",
        "dmo_srvc",
        "",
        "display drive management object information",
        fbe_cli_cmd_drive_mgmt_srvc,
        FBE_CLI_SRVC_ACCESS
    },  
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "ddt",
        "ddt",
        "",
        "Device Driver Tester",
        fbe_cli_ddt,
        FBE_CLI_SRVC_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "wisd",
        "wisd",
        "",
        "Where Is System Drives",
        fbe_cli_wisd,
        FBE_CLI_SRVC_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "odt",
        "odt",
        "",
        "Object Database Tester",
        fbe_cli_odt,
        FBE_CLI_SRVC_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "dps",
        "dps",
        "",
        "Display dps memory statistics",
        fbe_cli_cmd_dps_stats,
        FBE_CLI_SRVC_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "dps_add_mem",
        "dps_add_mem",  // no short hand
        "",
        "Add more dps memory to NEIT",
        fbe_cli_cmd_dps_add_mem,
        FBE_CLI_SRVC_ACCESS
    },  
    {
        FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "rmzerochkpt",
        "rzc",
        "",
        "Invalidate zero checkpoint for PVD object",
        fbe_cli_remove_zero_checkpoint,
        FBE_CLI_SRVC_ACCESS,
    },
    {
        FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "pvddeltime",
        "pvddeltime",
        "",
        "Set/Get system time threshold for deleting PVD",
        fbe_cli_pvd_time_threshold,
        FBE_CLI_SRVC_ACCESS,
    },
    {
        FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "sleep_twenty",
        "sleep20",
        "",
        "Sleep for 20 seconds",
        fbe_cli_sleep_at_prompt_for_twenty_seconds,
        FBE_CLI_SRVC_ACCESS,
    }, 
    {
        FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "sleep",
        "sleep",
        "",
        "Sleep for specified seconds",
        fbe_cli_sleep_at_prompt,
        FBE_CLI_SRVC_ACCESS,
    },    
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "set_ica_stamp",
        "sis",
        "",
        "Set ICA stamp for all disks",
        fbe_cli_cmd_set_ica_stamp,
        FBE_CLI_SRVC_ACCESS
    },
    #if 0/*CMS disabled*/
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "cluster_memory",
        "cms",
        "",
        "Cluster Memory related CLI commands",
        fbe_cli_cms,
        FBE_CLI_USER_ACCESS
    },
    #endif
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "bgo_enable",
        "bgoe",
        "",
        "Enable  one or more Background Operations for Disk\\RAID",
        fbe_cli_bgo_enable,
        FBE_CLI_SRVC_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "bgo_disable",
        "bgod",
        "",
        "Disable one or more Background Operations for Disk\\RAID",
        fbe_cli_bgo_disable,
        FBE_CLI_SRVC_ACCESS
    },   
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "lun_write_bypass",
        "lwb",
        "",
        "Set bypass mode for lun ",
        fbe_cli_lun_set_write_bypass_mode,
        FBE_CLI_SRVC_ACCESS
    },  
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "perfstats",
        "pstats",
        "",
        "Enables, Disables, and logs performance statistics",
        fbe_cli_cmd_perfstats,
        FBE_CLI_SRVC_ACCESS
    },
    {  FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "online_planned_drive",
        "onpd",
        "",
        "Make a planned drive online, which was blocked by homewrecker in earlier time",
        fbe_cli_cmd_online_planned_drive,
        FBE_CLI_USER_ACCESS
    },    
    {
        FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "pvd_srvc",
        "pvdsrvc",
        "",
        "Clear EOL and Drive Fault State for PVD object",
        fbe_cli_cmd_clear_pvd_drive_states,
        FBE_CLI_SRVC_ACCESS,
    },   
    {	
        FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "system_backup_restore",
        "sysbr",
        "",
        "backup and restore system configuration",
        fbe_cli_system_backup_restore,
        FBE_CLI_SRVC_ACCESS
    },
    {  
        FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "set_bg_op_speed",
        "sbgos",
        "",
        "Set/Read Background operation speed",
        fbe_cli_set_bg_op_speed,
        FBE_CLI_SRVC_ACCESS
    }, 	
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "operate_fru_signature",
        "opfrusig",
        "",
        "get/set disk fru signature",
        fbe_cli_cmd_signature,
        FBE_CLI_SRVC_ACCESS
    },
   {
        FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "pdo_srvc",
        "pdosrvc",
        "",
        "Set EOL and Drive Fault State for PDO object",
        fbe_cli_cmd_set_pdo_drive_states,
        FBE_CLI_SRVC_ACCESS,
    }, 
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "create_system_rg",
        "csr",
        "<RG number> <RG object id>",
        "create system rg",
        fbe_cli_create_system_rg,
        FBE_CLI_SRVC_ACCESS    
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "bind_system_lun",
        "csr",
        "<LUN number> <LUN object id>",
        "bind system lun",
        fbe_cli_bind_system_lun,
        FBE_CLI_SRVC_ACCESS    
    },
    {
        FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "database_debug_hook",
        "db_hook",
        "<operation> <hook_type>",
        "Set/Unset database hook",
        fbe_cli_database_hook,
        FBE_CLI_SRVC_ACCESS,
    },
    {
        FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "persist_debug_hook",
        "ps_hook",
        "<operation> <hook_type>",
        "Set/Unset persist service hook",
        fbe_cli_persist_hook,
        FBE_CLI_SRVC_ACCESS,
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "espcachestatus",
        "espcs",
        "",
        "Display Detailed ESP CacheStatus.\n",
        fbe_cli_cmd_espcs,
        FBE_CLI_USER_ACCESS
    },
    {
        FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "npinfo",
        "npinfo",
        "operation <parameters>",
        "Nonpaged metadata information access",
        fbe_cli_npinfo,
        FBE_CLI_SRVC_ACCESS,
    },
    {
        FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "mminfo",
        "mminfo",
        "operation <parameters>",
        "metadata memory information access",
        fbe_cli_mminfo,
        FBE_CLI_SRVC_ACCESS,
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "encrypt",
        "encrypt",
        "",
        "Encryption Related processing",
        fbe_cli_encryption,
        FBE_CLI_SRVC_ACCESS    
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "database_debug",
        "db_dbg",
        "",
        "Database debug command, information",
        fbe_cli_database_debug,
        FBE_CLI_USER_ACCESS
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "emeh",
        "emeh",
        "",
        "Extended Media Error Handling commands",
        fbe_cli_emeh,
        FBE_CLI_SRVC_ACCESS
        
    },
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "pvdweartimer",
        "pvdweartimer",
        "",
        "PVD Wear leveling commands",
        fbe_cli_provision_drive_wear_level_timer,
        FBE_CLI_SRVC_ACCESS
        
    },
#ifdef C4_INTEGRATED
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "becheck",
        "becheck",
        "",
        "Backend check",
        fbe_cli_becheck_cmd,
        FBE_CLI_USER_ACCESS
    },
    {
        FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "mirror",
        "mirror",
        "",
        "Get mirror status",
        fbe_cli_c4_mirror,
        FBE_CLI_USER_ACCESS
    },
    {   
        FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "replacechassis",
        "rc",
        "",
        "replace chassis related commands",
        fbe_cli_replace_chassis,
        FBE_CLI_USER_ACCESS
     },

#endif
};

static int fbe_cli_cmd_count = sizeof(cli_cmds)/sizeof(fbe_cli_cmd_t);

void fbe_cli_cmd_lookup(char *argv0, fbe_cli_cmd_t **cmd)
{
    int ii;
    fbe_bool_t  cmdFound = FALSE;

    *cmd = NULL;
    for (ii = 0; ii < fbe_cli_cmd_count; ii++) {
        *cmd = &cli_cmds[ii];
        if ((strcmp(argv0, cli_cmds[ii].name) == 0) ||(strcmp(argv0, cli_cmds[ii].name_a_p) == 0)) {
            cmdFound = TRUE;
            break;
        }
    }

    if (!cmdFound)
    {
        *cmd = NULL;
    }

}

void fbe_cli_print_usage(void)
{
    fbe_cli_cmd_t * cmd;
    int ii;
    char buf[64] = {0};

    printf("\n");
    printf("usage: fbe_cli.exe <command>\n\n");
    printf("commands:\n");
    printf("Notes: command full Name/Abbreviation - Summary\n\n");

    for (ii = 0; ii < fbe_cli_cmd_count; ii++) 
    {
        cmd = &cli_cmds[ii];

        // If user specifies ENG mode, then print out all commands.  If not, only print out 
        // User Access commands. 
        if (((operating_mode == USER_MODE) && (cmd->access == FBE_CLI_USER_ACCESS)) ||
            (operating_mode == ENG_MODE))
        {
            fbe_sprintf(buf, sizeof(buf), "%s/%s", cmd->name, cmd->name_a_p);
            printf(" %-18s -%s\n", buf, cmd->description);
        }
    }
    printf("\n");
    fflush(stdout);
}

void fbe_cli_cmd_print_usage(int argc , char ** argv)
{
    fbe_cli_print_usage();
}

fbe_status_t 
fbe_get_package_id(fbe_package_id_t * package_id)
{
    *package_id = FBE_PACKAGE_ID_PHYSICAL;
    return FBE_STATUS_OK;
}

