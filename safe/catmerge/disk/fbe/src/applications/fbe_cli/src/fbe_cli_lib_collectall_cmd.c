/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_collectall_cmd.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the function that collects all information from other
 *  fbe cli command.  Porting this from fbe_cli_src_enclosure_cmds.c file.
 *
 * @ingroup fbe_cli
 *
 * HISTORY
 *   11/22/2010:  KMai - created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
//#include <ctype.h>
#include "fbe_trace.h"
#include "fbe/fbe_class.h"
#include "fbe_cli_lib_collectall_cmd.h"
#include "fbe_cli_lurg.h"
#include "fbe_cli_lib_cooling_mgmt_cmds.h"
#include "fbe_cli_lib_enclosure_mgmt_cmds.h"
#include "fbe_cli_lib_board_mgmt_cmds.h"
#include "fbe_cli_lib_energy_stats_cmds.h"
#include "fbe_cli_lib_module_mgmt_cmds.h"
#include "fbe_cli_lib_ps_mgmt_cmds.h"
#include "fbe_cli_lib_board_mgmt_cmds.h"
#include "fbe_cli_lib_resume_prom_cmds.h"
#include "fbe_cli_lib_sfp_info_cmds.h"
#include "fbe_cli_lib_sepls_cmd.h"
#include "fbe_cli_lib_disk_info_cmd.h"
#include "fbe_cli_sp_cmd.h"
#include "fbe_cli_services.h"
#include "fbe_cli_database.h"

/**************************************************************************
 * @fn fbe_cli_lib_collectall_cmd
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
void fbe_cli_lib_collectall_cmd(int argc , char ** argv)
{

    // There should be a minimum of arguments. 
    if ((argc > 1)||(argc < 0))
    {
        fbe_cli_printf(" invalid number of arguments %d\n", argc);
        fbe_cli_printf("%s", COLLECT_ALL_USAGE);
        return;
    }

    // Parse the command line
    while(argc > 0)
    {
        // Check the command type 
        if(strcmp(*argv, "-h") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("%s", COLLECT_ALL_USAGE);
                return;
            }
        }
    }

    // Displays different fbecli commands
    argc =  0;   
    *argv = NULL;
    fbe_cli_lib_display_cmds(argc, argv);

    return;
} // End of fbe_cli_lib_collectall_cmd


/**************************************************************************
 * @fn fbe_cli_lib_display_cmds
 *           (int argc , char ** argv)
 **************************************************************************
 *
 *  @brief
 *      This function calls and displays different fbe_cli commands.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 *  @return   none
 *
 **************************************************************************/
void fbe_cli_lib_display_cmds(int default_argc, char** default_argv)
{
    int temp_argc = default_argc;
    char** temp_argv = default_argv;
	
    // Display SP Stat.  
    fbe_cli_printf("\n----------------------------------------------------------------");	
    fbe_cli_printf("\nFBECLI> spstat:\n\n");
    fbe_cli_cmd_sp(default_argc, default_argv);

    // Display Enclosure Stat.  This is equivalent to spstat in Flare
    fbe_cli_printf("\n----------------------------------------------------------------");	
    fbe_cli_printf("\nFBECLI> Enclstat:\n\n");
    fbe_cli_cmd_enclstat(default_argc, default_argv);

    // Display CMI info
    fbe_cli_printf("\n----------------------------------------------------------------");	
    fbe_cli_printf("\nFBECLI> CMI Info:\n\n");
    temp_argc = 1;
    *temp_argv = "-info";
    fbe_cli_cmi_commands(temp_argc, temp_argv);

    // Display DATABASE info
    fbe_cli_printf("\n----------------------------------------------------------------");	
    fbe_cli_printf("\nFBECLI> database Info:\n\n");
    temp_argc = 1;
    *temp_argv = "-getState";
    fbe_cli_database(temp_argc, temp_argv);

    // Display sepls info
    fbe_cli_printf("\n----------------------------------------------------------------");	
    fbe_cli_printf("\nFBECLI> sepls:\n\n");
    temp_argc = 2;
    *temp_argv = "-allsep";
    *(temp_argv+1) = "-objtree";
    fbe_cli_cmd_sepls(temp_argc, temp_argv);
    
    // Display RGINFO info
    fbe_cli_printf("\n----------------------------------------------------------------");	
    fbe_cli_printf("\nFBECLI> rginfo Info:\n\n");
    temp_argc = 3;
    *temp_argv = "-d";
    *(temp_argv+1) = "-rg";
    *(temp_argv+2) ="all";
    fbe_cli_rginfo(temp_argc, temp_argv);
    
    // Display LUNINFO info
    fbe_cli_printf("\n----------------------------------------------------------------");	
    fbe_cli_printf("\nFBECLI> luninfo Info:\n\n");
    temp_argc = 1;
    *temp_argv = "-all";
    fbe_cli_cmd_luninfo(temp_argc, temp_argv);    
        
    // Display base_config info
    fbe_cli_printf("\n----------------------------------------------------------------");	
    fbe_cli_printf("\nFBECLI> base_config:\n\n");
    fbe_cli_cmd_base_config(default_argc, default_argv);

    // Display info for PDO
    fbe_cli_printf("\n----------------------------------------------------------------");	
    fbe_cli_printf("\nFBECLI> pdo:\n\n");
    fbe_cli_cmd_physical_drive(default_argc, default_argv);

    // Display the PVD info so sniff results are shown
    fbe_cli_printf("\n----------------------------------------------------------------");	
    fbe_cli_printf("\nFBECLI> PVDInfo:\n\n");	
    fbe_cli_provision_drive_info(default_argc, default_argv);

    // Display the Disk info so using diskinfo command with '-l'
    fbe_cli_printf("\n----------------------------------------------------------------");	
    fbe_cli_printf("\nFBECLI> Disk Info with -l option:\n\n");	
    temp_argc = 1;
    *temp_argv = "-l";
    fbe_cli_cmd_disk_info(temp_argc, temp_argv);    

    // Display the Disk info so using diskinfo command 
    fbe_cli_printf("\n----------------------------------------------------------------");	
    fbe_cli_printf("\nFBECLI> Disk Info with default option:\n\n");	
    fbe_cli_cmd_disk_info(default_argc, default_argv);

    // Display getlogs info for sep
    fbe_cli_printf("\n----------------------------------------------------------------");	
    fbe_cli_printf("\nFBECLI> getlogs Info:\n\n");
    temp_argc = 1;
    *temp_argv = "sep";
    fbe_cli_cmd_getlogs(temp_argc, temp_argv);    

    // Display getlogs info for esp
    fbe_cli_printf("\n----------------------------------------------------------------");	
    fbe_cli_printf("\nFBECLI> getlogs esp:\n\n");
    temp_argc = 1;
    *temp_argv = "esp";
    fbe_cli_cmd_getlogs(temp_argc, temp_argv);    

    // Display bvd info
    fbe_cli_printf("\n----------------------------------------------------------------");	
    fbe_cli_printf("\nFBECLI> bvd Info:\n\n");
    temp_argc = 1;
    *temp_argv = "-get_stat";
    fbe_cli_bvd_commands(temp_argc, temp_argv);    

	//is HS disabled by any chance ?
	fbe_cli_printf("\n----------------------------------------------------------------");
	fbe_cli_printf("\nFBECLI> ahs -state:\n\n");
	temp_argc = 1;
	*temp_argv = "-state";
	fbe_cli_auto_hot_spare(temp_argc, temp_argv);

    // Display all Enclosure data faults
    fbe_cli_printf("\n----------------------------------------------------------------");	
    fbe_cli_printf("\nFBECLI> Encledal -fault:\n\n");
    temp_argc = 1;
    *temp_argv = "-fault";
    fbe_cli_cmd_display_encl_data(temp_argc, temp_argv);

    // Display all Enclosure data including the processor enclosure data
    fbe_cli_printf("\n----------------------------------------------------------------");	
    fbe_cli_printf("\nFBECLI> encledal:\n\n");
    temp_argc = 0;
    *temp_argv = NULL;
    fbe_cli_cmd_display_encl_data(temp_argc, temp_argv);

    // Display the enclprvdata
    fbe_cli_printf("\n----------------------------------------------------------------");	
    fbe_cli_printf("\nFBECLI> Enclprvdata:\n\n");	
    fbe_cli_cmd_enclosure_private_data(default_argc, default_argv);

    // Display the enclstatistics
    fbe_cli_printf("\n----------------------------------------------------------------");	
    fbe_cli_printf("\nFBECLI> Enclstatistics:\n\n");	
    fbe_cli_cmd_display_statistics(default_argc, default_argv);

    //Display the Cooling Mgmt info
    fbe_cli_printf("\n----------------------------------------------------------------");
    fbe_cli_printf("\nFBECLI> coolingmgmt:\n\n");
    temp_argc = 0;
    *temp_argv = NULL;
    fbe_cli_cmd_coolingmgmt(default_argc, default_argv);

    //Display the Enclosure Mgmt info
    fbe_cli_printf("\n----------------------------------------------------------------");
    fbe_cli_printf("\nFBECLI> enclmgmt:\n\n");
    fbe_cli_cmd_enclmgmt(default_argc, default_argv);

    //Display the Board Mgmt info
    fbe_cli_printf("\n----------------------------------------------------------------");
    fbe_cli_printf("\nFBECLI> boardmgmt:\n\n");
    fbe_cli_cmd_boardmgmt(default_argc, default_argv);

    //Display the Energy Stats
    fbe_cli_printf("\n----------------------------------------------------------------");
    fbe_cli_printf("\nFBECLI> eirstat:\n\n");
    fbe_cli_cmd_eirstat(default_argc, default_argv);

    //Display the SLIC info
    fbe_cli_printf("\n----------------------------------------------------------------");
    fbe_cli_printf("\nFBECLI> ioports:\n\n");
    fbe_cli_cmd_ioports(default_argc, default_argv);

    //Display the PS Mgmt info
    fbe_cli_printf("\n----------------------------------------------------------------");
    fbe_cli_printf("\nFBECLI> psmgmt:\n\n");
    fbe_cli_cmd_psmgmt(default_argc, default_argv);

    
    //Display the Resume Prom info
    fbe_cli_printf("\n----------------------------------------------------------------");
    fbe_cli_printf("\nFBECLI> resumeprom:\n\n");
    temp_argc = 2;
    *temp_argv = "-r";
    *(temp_argv+1) = "-all";
    fbe_cli_get_prom_info(temp_argc, temp_argv);

    //Display the sfp info
    fbe_cli_printf("\n----------------------------------------------------------------");
    fbe_cli_printf("\nFBECLI> sfpinfo:\n\n");
    fbe_cli_cmd_sfp_info(default_argc, default_argv);

    //Display the Cooling SPS info
    fbe_cli_printf("\n----------------------------------------------------------------");
    fbe_cli_printf("\nFBECLI> spsinfo:\n\n");
    fbe_cli_cmd_spsinfo(default_argc, default_argv);

    //Display the BBU info
    fbe_cli_printf("\n----------------------------------------------------------------");
    fbe_cli_printf("\nFBECLI> bbuinfo:\n\n");
    fbe_cli_cmd_bbuinfo(default_argc, default_argv);

    //Display the ESP CacheStatus info
    fbe_cli_printf("\n----------------------------------------------------------------");
    fbe_cli_printf("\nFBECLI> espcs -all:\n\n");
    temp_argc = 1;
    *temp_argv = "-all";
    fbe_cli_cmd_espcs(temp_argc, temp_argv);

    //Display the bus info
    fbe_cli_printf("\n----------------------------------------------------------------");
    fbe_cli_printf("\nFBECLI> businfo:\n\n");
    fbe_cli_cmd_bus_info(default_argc, default_argv);

    //Display the board mgmt info
    fbe_cli_printf("\n----------------------------------------------------------------");
    fbe_cli_printf("\nFBECLI> boardmgmt:\n\n");
    fbe_cli_cmd_boardmgmt(default_argc, default_argv);
        
    //Display the boardprvdata info
    fbe_cli_printf("\n----------------------------------------------------------------");
    fbe_cli_printf("\nFBECLI> boardprvdata:\n\n");
    fbe_cli_display_board_info(default_argc, default_argv);

    // Display all drive mgmt info
    fbe_cli_printf("\n----------------------------------------------------------------");	
    fbe_cli_printf("\nFBECLI> drivemgmt -drive_info:\n\n");
    temp_argc = 1;
    *temp_argv = "-drive_info";
    fbe_cli_cmd_drive_mgmt(temp_argc, temp_argv);
	
    fbe_cli_printf("\n----------------------------------------------------------------");
    fbe_cli_printf("\nFBECLI> eventlog:\n\n");
    fbe_cli_cmd_enclosure_e_log_command(default_argc, default_argv);

    fbe_cli_printf("\n----------------------------------------------------------------");
    fbe_cli_printf("\nFBECLI> allenclbuf:\n\n");
    fbe_cli_cmd_all_enclbuf(default_argc, default_argv);

    // Display DATABASE Stats
    fbe_cli_printf("\n----------------------------------------------------------------");	
    fbe_cli_printf("\nFBECLI> database Stats:\n\n");
    temp_argc = 1;
    *temp_argv = "-getStats";
    fbe_cli_database(temp_argc, temp_argv);


    // Display bgo status flags info
    fbe_cli_printf("\n----------------------------------------------------------------");	
    fbe_cli_printf("\nFBECLI> bgo status flag info:\n\n");
    temp_argc = 1;
    *temp_argv = "-get_bgo_flags";
    fbe_cli_bgo_status(temp_argc, temp_argv);

    // Validate FBE database
    fbe_cli_printf("\n----------------------------------------------------------------");	
    fbe_cli_printf("\nFBECLI> db_dbg validate caller action:\n\n");
    temp_argc = 1;
    *temp_argv = "-collect_validate";
    fbe_cli_database_debug(temp_argc, temp_argv);

    /* Disable the fbecli command expcmds in the spcollect at the request of the expander group. */
    //fbe_cli_printf("\n----------------------------------------------------------------");
    //fbe_cli_printf("\nFBECLI> expcmds:\n\n");
    //fbe_cli_cmd_expander_cmd(default_argc, default_argv);

    return;
} // End of fbe_cli_lib_display_cmds
