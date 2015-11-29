/***************************************************************************
* Copyright (C) EMC Corporation 2010 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
***************************************************************************/

/*!*************************************************************************
* @file fbe_cli_lib_perf_stats_cmds.c
***************************************************************************
*
* @brief
*  This file contains cli functions for the performance statistics related 
 *  features in FBE CLI.
*
* @ingroup fbe_cli
*
* @version
*  07/01/2010 - Created. Swati Fursule
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include <stdlib.h> /*for atoh*/
#include <stdio.h>
#include "fbe_cli_private.h"
#include "fbe_trace.h"
#include "fbe/fbe_class.h"
#include "fbe_cli_perf_stats.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_perfstats_interface.h"
#include "fbe/fbe_api_perfstats_sim.h"
#include "fbe/fbe_cli.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_discovery_interface.h"

static void fbe_cli_cmd_perfstats_enable(int argc, char** argv);
static void fbe_cli_cmd_perfstats_disable(int argc, char** argv);
static void fbe_cli_cmd_perfstats_clear(void);
static void fbe_cli_cmd_perfstats_log(int argc, char** argv);
static void fbe_cli_cmd_perfstats_dump(int argc, char** argv);
static void fbe_cli_lib_perfstats_dump_summed_sep_stats(fbe_lun_performance_counters_t *lun_stats,
                                                        fbe_cli_perfstats_tags_t stat_tags,
                                                        fbe_bool_t brief_mode);
static void fbe_cli_lib_perfstats_dump_per_core_sep_stats(fbe_lun_performance_counters_t *lun_stats,
                                                        fbe_cli_perfstats_tags_t stat_tags,
                                                        fbe_bool_t brief_mode);
static void fbe_cli_lib_perfstats_print_summed_sep_stats(FILE *fp,
                                                         fbe_cli_perfstats_tags_t stat_tags,
                                                         fbe_lun_performance_counters_t *old_stats,
                                                         fbe_lun_performance_counters_t *new_stats);
static void fbe_cli_lib_perfstats_print_per_core_sep_stats(FILE *fp,
                                                         fbe_cli_perfstats_tags_t stat_tags,
                                                         fbe_lun_performance_counters_t *old_stats,
                                                         fbe_lun_performance_counters_t *new_stats);
static void fbe_cli_lib_perfstats_print_sep_header(FILE *fp,
                                                   fbe_cli_perfstats_tags_t stat_tags,
                                                   fbe_bool_t   summed);

static void fbe_cli_lib_perfstats_dump_summed_pp_stats(fbe_pdo_performance_counters_t *pdo_stats,
                                                       fbe_cli_perfstats_tags_t stat_tags,
                                                       fbe_bool_t brief_mode);
static void fbe_cli_lib_perfstats_dump_per_core_pp_stats(fbe_pdo_performance_counters_t *pdo_stats,
                                                         fbe_cli_perfstats_tags_t stat_tags,
                                                         fbe_bool_t brief_mode);
static void fbe_cli_lib_perfstats_print_summed_pp_stats(FILE *fp,
                                                        fbe_cli_perfstats_tags_t stat_tags,
                                                        fbe_pdo_performance_counters_t *old_stats,
                                                        fbe_pdo_performance_counters_t *new_stats);
static void fbe_cli_lib_perfstats_print_per_core_pp_stats(FILE *fp,
                                                          fbe_cli_perfstats_tags_t stat_tags,
                                                          fbe_pdo_performance_counters_t *old_stats,
                                                          fbe_pdo_performance_counters_t *new_stats);
static void fbe_cli_lib_perfstats_print_pp_header(FILE *fp,
                                                  fbe_cli_perfstats_tags_t stat_tags,
                                                  fbe_bool_t   summed);
static void fbe_cli_cmd_perfstats_status(int argc, char** argv);

char filename_list[64][8192]; 

/*!*******************************************************************
*  fbe_cli_cmd_perf_stats
*********************************************************************
* @brief Function to implement perfstats commands execution.
*    fbe_cli_cmd_perf_stats executes a SEP/PP Tester operation.  This command
*               is for simulation/lab debug only.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
* @return - none.  
 *
* @revision:
*  05/01/2012 Created. Ryan Gadsby
*
*********************************************************************/
void fbe_cli_cmd_perfstats(int argc, char** argv)
{
    if (argc == 0)
    {
        fbe_cli_printf("perfstats: ERROR: Too few args.\n");
        fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
        return;
    }

    //depending on first argument, reroute cmd
    if ((strcmp(*argv, "-help") == 0) ||
        (strcmp(*argv, "-h") == 0) )
    {
        fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
        return;
    }
    else if (strcmp(*argv, "-enable") == 0)
    {
        fbe_cli_cmd_perfstats_enable(argc - 1, ++argv);
    }
    else if (strcmp(*argv, "-disable") == 0)
    {
        fbe_cli_cmd_perfstats_disable(argc - 1, ++argv);
    }
    else if (strcmp(*argv, "-clear") == 0)
    {
        fbe_cli_cmd_perfstats_clear();
    }
    else if (strcmp(*argv, "-log") == 0)
    {
        fbe_cli_cmd_perfstats_log(argc - 1, ++argv);
    }
   else if (strcmp(*argv, "-dump") == 0)
    {
        fbe_cli_cmd_perfstats_dump(argc - 1, ++argv);
    }
   else if (strcmp(*argv, "-status") == 0)
   {
        fbe_cli_cmd_perfstats_status(argc - 1, ++argv);
   }
   else 
    {
        fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
    }

    return;
}
/******************************************
* end fbe_cli_cmd_perf_stats()
******************************************/

static void fbe_cli_cmd_perfstats_status(int argc, char** argv)
{
    fbe_bool_t is_sep_enabled = FBE_FALSE;
    fbe_bool_t is_pp_enabled = FBE_FALSE;
    fbe_status_t status;


    status = fbe_api_perfstats_is_statistics_collection_enabled_for_package(FBE_PACKAGE_ID_SEP_0,
                                                                            &is_sep_enabled);

    if (status != FBE_STATUS_OK) 
    {
            fbe_cli_printf("ERROR: Couldn't get status for sep package!\n");
    }

    status = fbe_api_perfstats_is_statistics_collection_enabled_for_package(FBE_PACKAGE_ID_PHYSICAL,
                                                                            &is_pp_enabled);

    if (status != FBE_STATUS_OK) 
    {
            fbe_cli_printf("ERROR: Couldn't get status for physical package!\n");
    }

    fbe_cli_printf("PerfStats for SEP: %s\n", (is_sep_enabled)? "Enabled":"Disabled");
    fbe_cli_printf("PerfStats for PP:  %s\n", (is_pp_enabled)? "Enabled":"Disabled");
        
}

/*!**************************************************************
*          fbe_cli_cmd_perfstats_enable()
****************************************************************
*
* @brief   Parse perfstats commands to enable stat logging
*
*  @param    argc - argument count
*  @param    argv - argument string
*
* @return  None.
*
* @revision:
*  05/01/2012 Created. Ryan Gadsby
*  06/08/2012 Added support for PDO stats. Darren Insko
*
****************************************************************/
static void fbe_cli_cmd_perfstats_enable(int argc, char** argv)
{
    fbe_package_id_t package_id;
    fbe_object_id_t  object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t        bus = FBE_API_PHYSICAL_BUS_COUNT;
    fbe_u32_t        enclosure = FBE_API_ENCLOSURES_PER_BUS;
    fbe_u32_t        slot = FBE_API_ENCLOSURE_SLOTS;
    fbe_lun_number_t    lun_number;
    fbe_status_t        status;
    char                *object_token;
    fbe_bool_t       b_target_all = FBE_TRUE;
    fbe_const_class_info_t      *class_info_p;

    if (argc < 2)
    {
        fbe_cli_printf("perfstats: ERROR: Too few args.\n");
        fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
        return;
    }
    //parse arguments
    if (!strcmp(*argv, "-package"))
    {           
        argc--;                     
        argv++;
        if (!strcmp(*argv, "sep")) //enable everything for SEP
        {
            package_id = FBE_PACKAGE_ID_SEP_0;
        }
        else if (!strcmp(*argv, "pp")) //enable everything for physical package 
        {
            package_id = FBE_PACKAGE_ID_PHYSICAL;
        }
        else
        {
            fbe_cli_printf("ERROR: Invalid package name. Valid packages are sep and pp\n");
            return;
        }
        argc--;                     
        argv++;
    }
    else
    {
        fbe_cli_printf("ERROR: You have to specify a package name.\n");
        return;
    }

    /***** NEED TO DO *****/
    /* Create a common routine that parses these args */
    /* and is shared by the log and dump functions.   */
    while (argc && **argv == '-')
    {
        if (!strcmp(*argv, "-object_id"))
        {
            argc--;
            argv++;
            if ((bus < FBE_API_PHYSICAL_BUS_COUNT) ||
                (enclosure < FBE_API_ENCLOSURES_PER_BUS) ||
                (slot < FBE_API_ENCLOSURE_SLOTS))
            {
                fbe_cli_error("-object_id, bus, enclosure, and/or slot numbers have already been specified.\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
        //convert from hex, the next token should be the object ID
        object_token = *argv;
        if ((*object_token == '0') && 
            (*(object_token + 1) == 'x' || *(object_token + 1) == 'X'))
        {
            object_token += 2;
        }
        object_id = fbe_atoh(object_token);
            if ((object_id < 0) ||
                (object_id == FBE_OBJECT_ID_INVALID) ||
                (object_id > FBE_MAX_OBJECTS))
        {
            fbe_cli_printf("ERROR: Invalid object ID: 0x%x\n", object_id);
            fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
            return;
            }
            b_target_all = FBE_FALSE;
        }
        if (!strcmp(*argv, "-b"))
        {
            argc--;
            argv++;
            if (package_id != FBE_PACKAGE_ID_PHYSICAL)
            {
                fbe_cli_error("-b, a bus can only be specified for physical package statistics.\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            if (object_id != FBE_OBJECT_ID_INVALID)
            {
                fbe_cli_error("-b, object ID has already been specified\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            bus = fbe_atoi(*argv);
            if (bus < 0)
            {
                fbe_cli_error("-b, invalid bus number\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            b_target_all = FBE_FALSE;
        }
        if (!strcmp(*argv, "-e"))
        {
            argc--;
            argv++;
            if (package_id != FBE_PACKAGE_ID_PHYSICAL)
            {
                fbe_cli_error("-e, an enclosure can only be specified when dumping physical package statistics.\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            if (object_id != FBE_OBJECT_ID_INVALID)
            {
                fbe_cli_error("-e, object ID has already been specified\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            enclosure = fbe_atoi(*argv);
            if (enclosure < 0)
            {
                fbe_cli_error("-e, invalid enclosure number\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            b_target_all = FBE_FALSE;
        }
        if (!strcmp(*argv, "-s"))
        {
            argc--;
            argv++;
            if (package_id != FBE_PACKAGE_ID_PHYSICAL)
            {
                fbe_cli_error("-s, a slot can only be specified when dumping physical package statistics.\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            if (object_id != FBE_OBJECT_ID_INVALID)
            {
                fbe_cli_error("-s, object ID has already been specified\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            slot = fbe_atoi(*argv);
            if (slot < 0)
            {
                fbe_cli_error("-s, invalid slot number\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            b_target_all = FBE_FALSE;
        }
        argc--;
        argv++;
    } /* End of while */

    if (b_target_all == FBE_TRUE)
    {//enable everything for the package
        status = fbe_api_perfstats_enable_statistics_for_package(package_id);

        if (status != FBE_STATUS_OK) 
        {
            fbe_cli_printf("ERROR: Couldn't enable statistics for package!\n");
        }
        else
        {
            fbe_cli_printf("Statistics collection for package enabled\n");
        }
    }
    else if (package_id == FBE_PACKAGE_ID_SEP_0)
    {
        status = fbe_api_database_lookup_lun_by_object_id(object_id,
                                                          &lun_number); 
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("ERROR: LUN with object ID 0x%x not found.\n",
                           object_id);
            return;
        }

            status = fbe_api_lun_enable_peformance_stats(object_id);

            if (status != FBE_STATUS_OK) 
            {
                fbe_cli_printf("ERROR: Couldn't enable statistics for LUN %d!\n", lun_number);
            }
            else 
            {
                fbe_cli_printf("Enabled statistics for LUN %d!\n", lun_number);
            }
        }
    else if (package_id == FBE_PACKAGE_ID_PHYSICAL)
    {
        if (object_id == FBE_OBJECT_ID_INVALID)
        { 
            if ((bus >= FBE_API_PHYSICAL_BUS_COUNT) ||
                (enclosure >= FBE_API_ENCLOSURES_PER_BUS) ||
                (slot >= FBE_API_ENCLOSURE_SLOTS))
            {
                fbe_cli_error("To specify a single physical drive input -b <bus> -e <enclosure> -s <slot>\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }

            /* Get the object ID for this target */
            status = fbe_api_get_physical_drive_object_id_by_location(bus, enclosure, slot, &object_id);

            if ((status != FBE_STATUS_OK) || (object_id == FBE_OBJECT_ID_INVALID))
            {
                fbe_cli_error("Failed to get a physical drive object ID for bus: %d  enclosure: %d  slot: %d\n",
                              bus, enclosure, slot);
                return;
            }
        }

        /* Verify that this object ID represents a valid physical drive */
        status = fbe_cli_get_class_info(object_id, package_id, &class_info_p);

        if (status != FBE_STATUS_OK)
        {
            fbe_cli_error("Failed to get the class info for this object ID\n");
            return;
        }

        if ((class_info_p->class_id <= FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST) ||
            (class_info_p->class_id >= FBE_CLASS_ID_PHYSICAL_DRIVE_LAST))
       {
            fbe_cli_error("This object id 0x%x is not a physical drive but a %s (0x%x).\n",
                          object_id, class_info_p->class_name, class_info_p->class_id);
            return;
        }

        if ((bus >= FBE_API_PHYSICAL_BUS_COUNT) ||
            (enclosure >= FBE_API_ENCLOSURES_PER_BUS) ||
            (slot >= FBE_API_ENCLOSURE_SLOTS))
        {
            status = fbe_cli_get_bus_enclosure_slot_info(object_id,
                                                         class_info_p->class_id,
                                                         &bus,
                                                         &enclosure,
                                                         &slot,
                                                         FBE_PACKAGE_ID_PHYSICAL);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("Can't get bus, enclosure, slot information for PDO object ID: 0x%x, class ID: 0x%x, status: 0x%x\n",
                              object_id, class_info_p->class_id, status);
                return;
            }
        }

        status = fbe_api_physical_drive_enable_disable_perf_stats(object_id, FBE_TRUE);

        if (status != FBE_STATUS_OK) 
        {
            fbe_cli_error("Couldn't enable statistics for PDO %d_%d_%d!\n", bus, enclosure, slot);
        }
        else 
        {
            fbe_cli_printf("Enabled statistics for PDO %d_%d_%d!\n", bus, enclosure, slot);
        }
    }
    else
    {
        fbe_cli_error("Unrecognized package id: %d is not supported\n", package_id);
    }

    return;
}
/******************************************
 * end fbe_cli_cmd_perfstats_enable()
 ******************************************/

/*!**************************************************************
*          fbe_cli_cmd_perfstats_disable()
****************************************************************
*
* @brief   Parse perfstats commands to disable stat logging
*
*  @param    argc - argument count
*  @param    argv - argument string
*
* @return  None.
*
* @revision:
*  05/01/2012 Created. Ryan Gadsby
*  06/08/2012 Added support for PDO stats. Darren Insko
*
****************************************************************/
static void fbe_cli_cmd_perfstats_disable(int argc, char** argv)
{
    fbe_package_id_t package_id;
    fbe_object_id_t  object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t        bus = FBE_API_PHYSICAL_BUS_COUNT;
    fbe_u32_t        enclosure = FBE_API_ENCLOSURES_PER_BUS;
    fbe_u32_t        slot = FBE_API_ENCLOSURE_SLOTS;
    fbe_lun_number_t lun_number;
    fbe_status_t     status;
    char             *object_token;
    fbe_bool_t       b_target_all = FBE_TRUE;
    fbe_const_class_info_t      *class_info_p;

    if (argc < 2)
    {
        fbe_cli_printf("perfstats: ERROR: Too few args.\n");
        fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
        return;
    }
    //parse arguments
    if (!strcmp(*argv, "-package"))
    {           
        argc--;                     
        argv++;
        if (!strcmp(*argv, "sep")) //enable everything for SEP
        {
            package_id = FBE_PACKAGE_ID_SEP_0;
        }
        else if (!strcmp(*argv, "pp")) //enable everything for physical package 
        {
            package_id = FBE_PACKAGE_ID_PHYSICAL;
        }
        else
        {
            fbe_cli_printf("ERROR: Invalid package name. Valid packages are sep and pp\n");
            return;
        }
        argc--;                     
        argv++;
    }
    else
    {
        fbe_cli_printf("ERROR: You have to specify a package name.\n");
        return;
    }

    /***** NEED TO DO *****/
    /* Create a common routine that parses these args */
    /* and is shared by the log and dump functions.   */
    while (argc && **argv == '-')
    {
        if (!strcmp(*argv, "-object_id"))
        {
            argc--;
            argv++;
            if ((bus < FBE_API_PHYSICAL_BUS_COUNT) ||
                (enclosure < FBE_API_ENCLOSURES_PER_BUS) ||
                (slot < FBE_API_ENCLOSURE_SLOTS))
            {
                fbe_cli_error("-object_id, bus, enclosure, and/or slot numbers have already been specified.\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            /* Convert from hex, the next token should be the object ID */
            object_token = *argv;
            if ((*object_token == '0') && 
                (*(object_token + 1) == 'x' || *(object_token + 1) == 'X'))
            {
                object_token += 2;
            }
            object_id = fbe_atoh(object_token);
            if ((object_id < 0) ||
                (object_id == FBE_OBJECT_ID_INVALID) ||
                (object_id > FBE_MAX_OBJECTS))
            {
                fbe_cli_printf("ERROR: Invalid object ID: 0x%x\n", object_id);
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            b_target_all = FBE_FALSE;
        }
        if (!strcmp(*argv, "-b"))
        {
            argc--;
            argv++;
            if (package_id != FBE_PACKAGE_ID_PHYSICAL)
            {
                fbe_cli_error("-b, a bus can only be specified for physical package statistics.\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            if (object_id != FBE_OBJECT_ID_INVALID)
            {
                fbe_cli_error("-b, object ID has already been specified\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            bus = fbe_atoi(*argv);
            if (bus < 0)
            {
                fbe_cli_error("-b, invalid bus number\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            b_target_all = FBE_FALSE;
        }
        if (!strcmp(*argv, "-e"))
        {
            argc--;
            argv++;
            if (package_id != FBE_PACKAGE_ID_PHYSICAL)
            {
                fbe_cli_error("-e, an enclosure can only be specified when dumping physical package statistics.\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            if (object_id != FBE_OBJECT_ID_INVALID)
            {
                fbe_cli_error("-e, object ID has already been specified\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            enclosure = fbe_atoi(*argv);
            if (enclosure < 0)
            {
                fbe_cli_error("-e, invalid enclosure number\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            b_target_all = FBE_FALSE;
        }
        if (!strcmp(*argv, "-s"))
        {
            argc--;
            argv++;
            if (package_id != FBE_PACKAGE_ID_PHYSICAL)
            {
                fbe_cli_error("-s, a slot can only be specified when dumping physical package statistics.\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            if (object_id != FBE_OBJECT_ID_INVALID)
            {
                fbe_cli_error("-s, object ID has already been specified\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            slot = fbe_atoi(*argv);
            if (slot < 0)
            {
                fbe_cli_error("-s, invalid slot number\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            b_target_all = FBE_FALSE;
        }
        argc--;
        argv++;
    } /* End of while */

    if (b_target_all == FBE_TRUE)
    {//enable everything for the package
        status = fbe_api_perfstats_disable_statistics_for_package(package_id);
        if (status != FBE_STATUS_OK) 
        {
            fbe_cli_printf("ERROR: Couldn't disable statistics for package!\n");
        }
        else
        {
            fbe_cli_printf("Statistics collection for package disabled\n");
        }
    }
    else if (package_id == FBE_PACKAGE_ID_SEP_0)
    {
        status = fbe_api_database_lookup_lun_by_object_id(object_id,
                                                          &lun_number); 
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("ERROR: LUN with object ID 0x%x not found.\n",
                           object_id);
            return;
        }

        status = fbe_api_lun_disable_peformance_stats(object_id);
   
        if (status != FBE_STATUS_OK) 
        {
            fbe_cli_printf("ERROR: Couldn't disable statistics for LUN %d!\n", lun_number);
        }
        else 
        {
            fbe_cli_printf("Disabled statistics for LUN %d!\n", lun_number);
        }
    }
    else if (package_id == FBE_PACKAGE_ID_PHYSICAL)
    {
        if (object_id == FBE_OBJECT_ID_INVALID)
        { 
            if ((bus >= FBE_API_PHYSICAL_BUS_COUNT) ||
                (enclosure >= FBE_API_ENCLOSURES_PER_BUS) ||
                (slot >= FBE_API_ENCLOSURE_SLOTS))
            {
                fbe_cli_error("To specify a single physical drive input -b <bus> -e <enclosure> -s <slot>\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }

            /* Get the object ID for this target */
            status = fbe_api_get_physical_drive_object_id_by_location(bus, enclosure, slot, &object_id);

            if ((status != FBE_STATUS_OK) || (object_id == FBE_OBJECT_ID_INVALID))
            {
                fbe_cli_error("Failed to get a physical drive object ID for bus: %d  enclosure: %d  slot: %d\n",
                              bus, enclosure, slot);
                return;
            }
        }

        /* Verify that this object ID represents a valid physical drive */
        status = fbe_cli_get_class_info(object_id, package_id, &class_info_p);

        if (status != FBE_STATUS_OK)
        {
            fbe_cli_error("Failed to get the class info for this PDO object ID\n");
            return;
        }

        if ((class_info_p->class_id <= FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST) ||
            (class_info_p->class_id >= FBE_CLASS_ID_PHYSICAL_DRIVE_LAST))
        {
            fbe_cli_error("This object id 0x%x is not a physical drive but a %s (0x%x).\n",
                          object_id, class_info_p->class_name, class_info_p->class_id);
            return;
        }

        if ((bus >= FBE_API_PHYSICAL_BUS_COUNT) ||
            (enclosure >= FBE_API_ENCLOSURES_PER_BUS) ||
            (slot >= FBE_API_ENCLOSURE_SLOTS))
        {
            status = fbe_cli_get_bus_enclosure_slot_info(object_id,
                                                         class_info_p->class_id,
                                                         &bus,
                                                         &enclosure,
                                                         &slot,
                                                         FBE_PACKAGE_ID_PHYSICAL);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("Can't get bus, enclosure, slot information for PDO object ID: 0x%x, class ID: 0x%x, status: 0x%x\n",
                              object_id, class_info_p->class_id, status);
                return;
            }
        }

        status = fbe_api_physical_drive_enable_disable_perf_stats(object_id, FBE_FALSE);

        if (status != FBE_STATUS_OK) 
        {
            fbe_cli_error("Couldn't disable statistics for PDO %d_%d_%d!\n", bus, enclosure, slot);
        }
        else 
        {
            fbe_cli_printf("Disabled statistics for PDO %d_%d_%d!\n", bus, enclosure, slot);
        }
    }
    else
    {
        fbe_cli_error("Unrecognized package id: %d is not supported\n", package_id);
    }

    return;
}
/******************************************
 * end fbe_cli_cmd_perfstats_disable()
 ******************************************/

/*!**************************************************************
*          fbe_cli_cmd_perfstats_clear()
****************************************************************
*
* @brief   Clear all performance statistics.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
* @return  None.
*
* @revision:
*  05/01/2012 Created. Ryan Gadsby
*
****************************************************************/
static void fbe_cli_cmd_perfstats_clear(void)
{
    fbe_api_perfstats_zero_statistics_for_package(FBE_PACKAGE_ID_SEP_0);
    fbe_api_perfstats_zero_statistics_for_package(FBE_PACKAGE_ID_PHYSICAL);
    return;
}
/******************************************
* end fbe_cli_cmd_perfstats_clear()
******************************************/

/*!**************************************************************
*          fbe_cli_cmd_perfstats_log()
****************************************************************
*
* @brief   Parse perfstats commands to log stats to local file
*  @param    argc - argument count
*  @param    argv - argument string
*
* @return  None.
*
* @revision:
*  05/01/2012 Created. Ryan Gadsby
*  06/08/2012 Added support for PDO stats. Darren Insko
*
****************************************************************/
static void fbe_cli_cmd_perfstats_log(int argc, char** argv)
{
    char log_dir[64];
    fbe_bool_t sum_all_stats = FBE_FALSE;
    fbe_u32_t  sample_interval = FBE_CLI_PERFSTATS_DEFAULT_DURATION_MSEC;
    fbe_u32_t  sample_count = FBE_CLI_PERFSTATS_DEFAULT_SAMPLE_COUNT;
    fbe_cli_perfstats_tags_t stat_tags = FBE_CLI_PERFSTATS_TAG_ALL;
    char *object_token;
    char path_buffer[256];
    char cmd_buffer[256];
    char filename_buffer[64];

    fbe_object_id_t *obj_list;
    fbe_u32_t       obj_count;
    fbe_u32_t       obj_i;
    fbe_u32_t       lun_number;
    fbe_u32_t       lun_offset;
    fbe_u32_t       pdo_offset;
    
    fbe_u64_t container_ptr = 0;

    fbe_status_t status;

    union
    {
        fbe_perfstats_physical_package_container_t *pp_container;
        fbe_perfstats_sep_container_t              *sep_container;
    }new_stats;

    union
    {
        fbe_perfstats_physical_package_container_t *pp_container;
        fbe_perfstats_sep_container_t              *sep_container;
    }old_stats;
   
    FILE *file_list[8192];

    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;
    fbe_package_id_t package_id = FBE_PACKAGE_ID_INVALID;
    fbe_u32_t        bus = FBE_API_PHYSICAL_BUS_COUNT;
    fbe_u32_t        enclosure = FBE_API_ENCLOSURES_PER_BUS;
    fbe_u32_t        slot = FBE_API_ENCLOSURE_SLOTS;
    fbe_bool_t       b_target_all = FBE_TRUE;
    fbe_const_class_info_t      *class_info_p;

    //default log_dir
    sprintf(log_dir, "%s", FBE_CLI_PERFSTATS_DEFAULT_DIRECTORY);
    if (argc < 2)
    {
        fbe_cli_printf("perfstats: ERROR: Too few args.\n");
        fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
        return;
    }

    //check package
    if (!strcmp(*argv, "-package"))
    {           
        argc--;                     
        argv++;
        if(argc) 
        {
            if (!strcmp(*argv, "sep"))
            {
                package_id = FBE_PACKAGE_ID_SEP_0;
            }
            else if (!strcmp(*argv, "pp"))
            {     
                package_id = FBE_PACKAGE_ID_PHYSICAL;
            }
        }
        else
        {
            fbe_cli_printf("ERROR: Invalid package name. Valid packages are sep and pp\n");
            return;
        }
        argc--;                     
        argv++;
    }
    else
    {
        fbe_cli_printf("ERROR: You have to specify a package name.\n");
        fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
        return;
    }

    /***** NEED TO DO *****/
    /* Create a common routine that parses these args */
    /* and is shared by the log and dump functions.   */
    while (argc && **argv == '-')
    {
        if (!strcmp(*argv, "-object_id"))
        {
            argc--;
            argv++;
            if ((bus < FBE_API_PHYSICAL_BUS_COUNT) ||
                (enclosure < FBE_API_ENCLOSURES_PER_BUS) ||
                (slot < FBE_API_ENCLOSURE_SLOTS))
            {
                fbe_cli_error("-object_id, bus, enclosure, and/or slot numbers have already been specified.\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            object_token = *argv;
            if ((*object_token == '0') && 
                (*(object_token + 1) == 'x' || *(object_token + 1) == 'X'))
            {
                object_token += 2;
           }
            object_id = fbe_atoh(object_token);
            if ((object_id < 0) || 
                (object_id == FBE_OBJECT_ID_INVALID) ||
                (object_id > FBE_MAX_OBJECTS))
            {
                fbe_cli_printf("ERROR: Invalid object ID: 0x%x\n", object_id);
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            b_target_all = FBE_FALSE;
        }
        if (!strcmp(*argv, "-b"))
        {
            argc--;
            argv++;
            if (package_id != FBE_PACKAGE_ID_PHYSICAL)
            {
                fbe_cli_error("-b, a bus can only be specified for physical package statistics.\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            if (object_id != FBE_OBJECT_ID_INVALID)
            {
                fbe_cli_error("-b, object ID has already been specified\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            bus = fbe_atoi(*argv);
            if (bus < 0)
            {
                fbe_cli_error("-b, invalid bus number\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            b_target_all = FBE_FALSE;
        }
        if (!strcmp(*argv, "-e"))
        {
            argc--;
            argv++;
            if (package_id != FBE_PACKAGE_ID_PHYSICAL)
            {
                fbe_cli_error("-e, an enclosure can only be specified when dumping physical package statistics.\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            if (object_id != FBE_OBJECT_ID_INVALID)
            {
                fbe_cli_error("-e, object ID has already been specified\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            enclosure = fbe_atoi(*argv);
            if (enclosure < 0)
            {
                fbe_cli_error("-e, invalid enclosure number\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            b_target_all = FBE_FALSE;
        }
        if (!strcmp(*argv, "-s"))
        {
            argc--;
            argv++;
            if (package_id != FBE_PACKAGE_ID_PHYSICAL)
            {
                fbe_cli_error("-s, a slot can only be specified when dumping physical package statistics.\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            if (object_id != FBE_OBJECT_ID_INVALID)
            {
                fbe_cli_error("-s, object ID has already been specified\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
           }
            slot = fbe_atoi(*argv);
            if (slot < 0)
            {
                fbe_cli_error("-s, invalid slot number\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            b_target_all = FBE_FALSE;
        }
        if (!strcmp(*argv, "-interval"))
        {
            argc--;
            argv++;

            sample_interval = fbe_atoi(*argv) * 1000; //msecs
            if (!sample_interval)
            {   //need positive interval
                fbe_cli_printf("ERROR, need positive sample interval");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
        }
        if (!strcmp(*argv, "-summed"))
        {
            sum_all_stats = FBE_TRUE;
        }
        if (!strcmp(*argv, "-dir"))
        {
            argc--;
            argv++;
            sprintf(log_dir, "%s", *argv);
        }
       
        //handle tags
        if (!strcmp(*argv, "-tag"))
        {
            argc--;
            argv++;
            stat_tags = 0;
            while (argc && **argv != '-')
            {
                 if (!strcmp(*argv, "lun_rblk"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_LUN_RBLK;
                 }
                 else if (!strcmp(*argv, "lun_wblk"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_LUN_WBLK;
                 }
                 else if (!strcmp(*argv, "lun_riops"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_LUN_RIOPS;
                 }
                 else if (!strcmp(*argv, "lun_wiops"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_LUN_WIOPS;
                 }
                 else if (!strcmp(*argv, "lun_stpc"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_LUN_STPC;
                 }
                 else if (!strcmp(*argv, "lun_mr3w"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_LUN_MR3W;
                 }
                 else if (!strcmp(*argv, "lun_rhis"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_LUN_RHIS;
                 }
                 else if (!strcmp(*argv, "lun_whis"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_LUN_WHIS;
                 }
                 else if (!strcmp(*argv, "rg_rblk"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_RG_RBLK;
                 }
                 else if (!strcmp(*argv, "rg_wblk"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_RG_WBLK;
                 }
                 else if (!strcmp(*argv, "rg_riops"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_RG_RIOPS;
                 }
                 else if (!strcmp(*argv, "rg_wiops"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_RG_WIOPS;
                 }
                 else if (!strcmp(*argv, "pdo_rblk"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_PDO_RBLK;
                 }
                 else if (!strcmp(*argv, "pdo_wblk"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_PDO_WBLK;
                 }
                 else if (!strcmp(*argv, "pdo_sblk"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_PDO_SBLK;
                 }
                 else if (!strcmp(*argv, "pdo_riops"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_PDO_RIOPS;
                 }
                 else if (!strcmp(*argv, "pdo_wiops"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_PDO_WIOPS;
                 }
                else if (!strcmp(*argv, "pdo_util"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_PDO_UTIL;
                 }
                 else if (!strcmp(*argv, "lun_nzqa"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_LUN_NZQA;
                 }
                 else if (!strcmp(*argv, "lun_saql"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_LUN_SAQL;
                 }
                 else if (!strcmp(*argv, "lun_crrt"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_LUN_CRRT;
                 }
                 else if (!strcmp(*argv, "lun_cwrt"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_LUN_CWRT;
                 }
                 else if (!strcmp(*argv, "pdo_this"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_PDO_THIS;
                 }
                 else
                 {//invalid tag
                      fbe_cli_printf("ERROR, \"%s\" is not a valid tag\n", *argv);
                      return;
                 }
                 argc--;
                 argv++;
            }
            if (argc == 0) {
                break;
            }
        }
        argc--;
        argv++;
    }
    //get the appropriate stat container and populate the object list
    if (fbe_cli_is_simulation()) 
    {
        status = fbe_api_perfstats_sim_get_statistics_container_for_package(&container_ptr, package_id);
    }
    else
    {
        status = fbe_api_perfstats_get_statistics_container_for_package(&container_ptr,package_id);
    }
    if (status != FBE_STATUS_OK) 
    {
        fbe_cli_printf("ERROR: Failed to get statistics package for package id: %d", package_id);
        return;
    }

    if (package_id == FBE_PACKAGE_ID_SEP_0)
    {
        old_stats.sep_container = fbe_api_allocate_memory(sizeof(fbe_perfstats_sep_container_t));
        if (!old_stats.sep_container) 
        {
            fbe_cli_printf("old_stats_allocate_failure\n");
        }
        new_stats.sep_container = (fbe_perfstats_sep_container_t *)(fbe_ptrhld_t) container_ptr;
        if (b_target_all == FBE_TRUE)
        { //get all the objects
            status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN,FBE_PACKAGE_ID_SEP_0,&obj_list,&obj_count);
        }
        else
        {
            obj_count = 1;
            obj_list = &object_id;
        }
        //open a file for every object we're going to collect for
        EmcutilFilePathMake(path_buffer, sizeof(path_buffer), EMCUTIL_BASE_CDRIVE, log_dir, NULL);
  
        fbe_cli_printf("Making directory: %s\n", path_buffer);
        sprintf(cmd_buffer, "mkdir %s", path_buffer);
        system(cmd_buffer);
        fbe_cli_printf("Opening files\n");
        for (obj_i = 0; obj_i < obj_count; obj_i++)
        { 
            //look up lun number and open the file
            status = fbe_api_database_lookup_lun_by_object_id(*(obj_list+obj_i),
                                                              &lun_number);
            //create file
            sprintf(filename_buffer, "LUN%d.csv", lun_number);
            EmcutilFilePathMake(path_buffer, sizeof(path_buffer), EMCUTIL_BASE_CDRIVE, log_dir, filename_buffer, NULL);
            sprintf(filename_list[obj_i], "%s", path_buffer);

            fbe_cli_printf("Opening file for path: %s\n", path_buffer);
            file_list[obj_i] = fopen(path_buffer, "w");
            if (!file_list[obj_i])
            {
                fbe_cli_printf("ERROR, could not open file: %s", path_buffer);
                return;
            }

            fbe_cli_lib_perfstats_print_sep_header(file_list[obj_i],
                                                   stat_tags,
                                                   sum_all_stats);            
        }
        //log loop
        while (sample_count) {
            //control+c to exit
               
                fbe_cli_printf("Copying stats\n");
                //copy old stats over
                memcpy(old_stats.sep_container, new_stats.sep_container, sizeof(fbe_perfstats_sep_container_t));
                //sleep
                fbe_cli_printf("Sleeping for %d milliseconds\n", sample_interval);
                fbe_api_sleep(sample_interval);
                //for each object, compare and write differences to file corresponding to object codes
                for (obj_i = 0; obj_i < obj_count; obj_i++)
                {
                    //get offset for object_id
                    fbe_cli_printf("Getting offset for object %d id: 0x%x\n", obj_i, *(obj_list+obj_i));
                    status = fbe_api_perfstats_get_offset_for_object(*(obj_list+obj_i),
                                                                     package_id,
                                                                     &lun_offset);
                    if (lun_offset >= PERFSTATS_INVALID_OBJECT_OFFSET) 
                    {
                        fbe_cli_printf("Invalid offset of object ID 0x%x\n", *(obj_list+obj_i));
                        return;
                    }
                    if (sum_all_stats)
                    {
                        fbe_cli_lib_perfstats_print_summed_sep_stats(file_list[obj_i],
                                                                     stat_tags,
                                                                     &old_stats.sep_container->lun_stats[lun_offset],
                                                                     &new_stats.sep_container->lun_stats[lun_offset]);
                    }
                    else
                    {
                        fbe_cli_lib_perfstats_print_per_core_sep_stats(file_list[obj_i],
                                                                       stat_tags,
                                                                       &old_stats.sep_container->lun_stats[lun_offset],
                                                                       &new_stats.sep_container->lun_stats[lun_offset]);
                    }
                }
                sample_count--;
        }
    } 
    else if (package_id == FBE_PACKAGE_ID_PHYSICAL)
    {
        old_stats.pp_container = fbe_api_allocate_memory(sizeof(fbe_perfstats_physical_package_container_t));
        if (!old_stats.pp_container) 
        {
            fbe_cli_printf("old_stats_allocate_failure\n");
            return;
        }
        new_stats.pp_container = (fbe_perfstats_physical_package_container_t *)(fbe_ptrhld_t) container_ptr;
        if (b_target_all == FBE_TRUE)
        {
            fbe_u32_t       object_count = 0;
            fbe_object_id_t *object_list_p = NULL;
            fbe_u32_t       total_objects = 0;
            fbe_u32_t       obj_i = 0;

            status = fbe_api_get_total_objects(&object_count, package_id);

            if (status != FBE_STATUS_OK)
            {
                return;
            }

            /* Allocate memory for the objects */
            object_list_p = fbe_api_allocate_memory(sizeof(fbe_object_id_t) * object_count);

            if (object_list_p == NULL)
            {
                 fbe_cli_error("%s Unable to allocate memory for object list\n", __FUNCTION__);
                return;
            }

            /* Find the count of total objects */
            status = fbe_api_enumerate_objects(object_list_p, object_count, &total_objects, package_id);

            if (status != FBE_STATUS_OK)
            {
                fbe_api_free_memory(object_list_p);
                return;
            }

            /* open a file for every object we're going to collect for */
            EmcutilFilePathMake(path_buffer, sizeof(path_buffer), EMCUTIL_BASE_CDRIVE, log_dir, NULL);
            fbe_cli_printf("Making directory: %s\n", path_buffer);
            sprintf(cmd_buffer, "mkdir %s", path_buffer);
            system(cmd_buffer);

            fbe_cli_printf("Opening files\n");
            /* Obtain the physical drive's Class ID, Class Name, Port, Enclosure */
            /* and Slot numbers, which are used to uniquely identify it.         */

            /* Loop over all objects found in the table */
            for (obj_i = 0; obj_i < total_objects; obj_i++)
           {
                object_id = object_list_p[obj_i];

                status = fbe_cli_get_class_info(object_id, FBE_PACKAGE_ID_PHYSICAL, &class_info_p);

                /* Match the class id to a valid filter class id. */
                if ((status == FBE_STATUS_OK) &&
                    (class_info_p->class_id > FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST) &&
                    (class_info_p->class_id < FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST))
                {
                    //get offset for object_id
                    status = fbe_api_perfstats_get_offset_for_object(object_id,
                                                                     package_id,
                                                                     &pdo_offset);
                    if (status != FBE_STATUS_OK || pdo_offset >= PERFSTATS_INVALID_OBJECT_OFFSET)
                    {
                        fbe_cli_error("%s Failed to get offset for PDO object id: 0x%x\n", __FUNCTION__, object_id);
                        continue;
                    }

                    status = fbe_cli_get_bus_enclosure_slot_info(object_id,
                                                                 class_info_p->class_id,
                                                                 &bus,
                                                                 &enclosure,
                                                                 &slot,
                                                                 FBE_PACKAGE_ID_PHYSICAL);
                    if (status != FBE_STATUS_OK)
                   {
                        fbe_cli_error("Can't get bus, enclosure, slot information for PDO object ID: 0x%x, class ID: 0x%x, status: 0x%x\n",
                                      object_id, class_info_p->class_id, status);
                        continue;
                    }

                    sprintf(filename_buffer,"Disk%d_%d_%d.csv", bus, enclosure, slot);
                    EmcutilFilePathMake(path_buffer, sizeof(path_buffer), EMCUTIL_BASE_CDRIVE, log_dir, filename_buffer, NULL);
                    fbe_cli_printf("Opening file for path: %s\n", path_buffer);
                    sprintf(filename_list[obj_i], "%s", path_buffer);
                    file_list[obj_i] = fopen(path_buffer, "w");
                    if (!file_list[obj_i])
                    {
                        fbe_cli_printf("ERROR, could not open file: %s", path_buffer);
                        continue;
                    }

                    fbe_cli_lib_perfstats_print_pp_header(file_list[obj_i],
                                                          stat_tags,
                                                          sum_all_stats);
                }
            } /* End of loop over all objects found */

            /* Display our totals */
            fbe_cli_printf("Discovered %.4d objects in total.\n", total_objects);

            //log loop
            while (sample_count) {
                //control+c to exit
                fbe_cli_printf("Copying stats\n");
                //copy old stats over
                memcpy(old_stats.pp_container, new_stats.pp_container, sizeof(fbe_perfstats_physical_package_container_t));
                //sleep
                fbe_cli_printf("Sleeping for %d milliseconds\n", sample_interval);
                fbe_api_sleep(sample_interval);
                //for each object, compare and write differences to file corresponding to object codes
                for (obj_i = 0; obj_i < total_objects; obj_i++)
                {
                    object_id = object_list_p[obj_i];

                    status = fbe_cli_get_class_info(object_id, FBE_PACKAGE_ID_PHYSICAL, &class_info_p);

                    /* Match the class id to a valid filter class id. */
                    if ((status == FBE_STATUS_OK) &&
                        (class_info_p->class_id > FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST) &&
                        (class_info_p->class_id < FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST))
                    {
                        //get offset for object_id
                        status = fbe_api_perfstats_get_offset_for_object(object_id,
                                                                         package_id,
                                                                         &pdo_offset);
                        if (status != FBE_STATUS_OK || pdo_offset >= PERFSTATS_INVALID_OBJECT_OFFSET)
                        {
                            fbe_cli_error("%s Failed to get offset for PDO object id: 0x%x\n", __FUNCTION__, object_id);
                            continue;
                        }

                        if (sum_all_stats)
                        {
                            fbe_cli_lib_perfstats_print_summed_pp_stats(file_list[obj_i],
                                                                        stat_tags,
                                                                        &old_stats.pp_container->pdo_stats[pdo_offset],
                                                                        &new_stats.pp_container->pdo_stats[pdo_offset]);
                        }
                        else
                        {
                            fbe_cli_lib_perfstats_print_per_core_pp_stats(file_list[obj_i],
                                                                          stat_tags,
                                                                          &old_stats.pp_container->pdo_stats[pdo_offset],
                                                                          &new_stats.pp_container->pdo_stats[pdo_offset]);
                        }
                    }
                } /* End of loop over all objects found */
                sample_count--;
            }

            /* Free up the memory we allocated for the object list */
            fbe_api_free_memory(object_list_p);
        }
        else
       {
            if (object_id == FBE_OBJECT_ID_INVALID)
            { 
                if ((bus >= FBE_API_PHYSICAL_BUS_COUNT) ||
                    (enclosure >= FBE_API_ENCLOSURES_PER_BUS) ||
                    (slot >= FBE_API_ENCLOSURE_SLOTS))
               {
                    fbe_cli_error("To specify a single physical drive input -b <bus> -e <enclosure> -s <slot>\n");
                    fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                    return;
                }

                /* Get the object ID for this target */
                status = fbe_api_get_physical_drive_object_id_by_location(bus, enclosure, slot, &object_id);

                if ((status != FBE_STATUS_OK) || (object_id == FBE_OBJECT_ID_INVALID))
                {
                    fbe_cli_error("Failed to get a physical drive object ID for bus: %d  enclosure: %d  slot: %d\n",
                                  bus, enclosure, slot);
                    return;
                }
            }

            /* Verify that this object ID represents a valid physical drive */
            status = fbe_cli_get_class_info(object_id, package_id, &class_info_p);

            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("Failed to get the class info for this PDO object ID\n");
                return;
            }

            if ((class_info_p->class_id <= FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST) ||
                (class_info_p->class_id >= FBE_CLASS_ID_PHYSICAL_DRIVE_LAST))
            {
                fbe_cli_error("This object id 0x%x is not a physical drive but a %s (0x%x).\n",
                              object_id, class_info_p->class_name, class_info_p->class_id);
                return;
            }

            //get offset for object_id
            status = fbe_api_perfstats_get_offset_for_object(object_id,
                                                             package_id,
                                                             &pdo_offset);
            if (status != FBE_STATUS_OK || pdo_offset >= PERFSTATS_INVALID_OBJECT_OFFSET)
            {
                fbe_cli_error("%s Failed to get offset for PDO object id: 0x%x\n", __FUNCTION__, object_id);
                return;
            }

            //open a file for every object we're going to collect for
            EmcutilFilePathMake(path_buffer, sizeof(path_buffer), EMCUTIL_BASE_CDRIVE, log_dir, NULL);
            fbe_cli_printf("Making directory: %s\n", path_buffer);
            sprintf(cmd_buffer, "mkdir %s", path_buffer);
            system(cmd_buffer);

            fbe_cli_printf("Opening files\n");
            /* Obtain the physical drive's Class ID, Class Name, Port, Enclosure */
            /* and Slot numbers, which are used to uniquely identify it.         */
            status = fbe_cli_get_bus_enclosure_slot_info(object_id,
                                                         class_info_p->class_id,
                                                         &bus,
                                                         &enclosure,
                                                         &slot,
                                                         FBE_PACKAGE_ID_PHYSICAL);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("Can't get bus, enclosure, slot information for PDO object ID: 0x%x, class ID: 0x%x, status: 0x%x\n",
                              object_id, class_info_p->class_id, status);
                return;
            }

            sprintf(filename_buffer,"Disk%d_%d_%d.csv", bus, enclosure, slot);
            EmcutilFilePathMake(path_buffer, sizeof(path_buffer), EMCUTIL_BASE_CDRIVE, log_dir, filename_buffer, NULL);
            fbe_cli_printf("Opening file for path: %s\n", path_buffer);
            sprintf(filename_list[0], "%s", path_buffer);
            file_list[0] = fopen(path_buffer, "w");
            if (!file_list[0])
            {
                fbe_cli_printf("ERROR, could not open file: %s", path_buffer);
                return;
           }

            fbe_cli_lib_perfstats_print_pp_header(file_list[0],
                                                  stat_tags,
                                                  sum_all_stats);
            //log loop
            while (sample_count) 
            { //control+c to exit
                fbe_cli_printf("Copying stats\n");
                //copy old stats over
                memcpy(old_stats.pp_container, new_stats.pp_container, sizeof(fbe_perfstats_physical_package_container_t));
                //sleep
                fbe_cli_printf("Sleeping for %d milliseconds\n", sample_interval);
                fbe_api_sleep(sample_interval);
                if (sum_all_stats)
                {
                    fbe_cli_lib_perfstats_print_summed_pp_stats(file_list[0],
                                                                stat_tags,
                                                                &old_stats.pp_container->pdo_stats[pdo_offset],
                                                                &new_stats.pp_container->pdo_stats[pdo_offset]);
                }
                else
                {
                    fbe_cli_lib_perfstats_print_per_core_pp_stats(file_list[0],
                                                                  stat_tags,
                                                                  &old_stats.pp_container->pdo_stats[pdo_offset],
                                                                  &new_stats.pp_container->pdo_stats[pdo_offset]);
                }
                sample_count--;
            }
            //open new file and start over
        }
    }
    else
    {
        fbe_cli_error("Unrecognized package id: %d is not supported\n", package_id);
    }

    return;
}
/*********************************
* end fbe_cli_cmd_perfstats_log()
*********************************/

/*!**************************************************************
*          fbe_cli_cmd_perfstats_dump()
****************************************************************
*
* @brief   Parse perfstats commands to dump raw counters to stdout.
*  @param    argc - argument count
*  @param    argv - argument string
*
* @return  None.
*
* @revision:
*  05/01/2012 Created. Ryan Gadsby
*  06/08/2012 Added support for PDO stats. Darren Insko
*
****************************************************************/
static void fbe_cli_cmd_perfstats_dump(int argc, char** argv)
{
    fbe_bool_t sum_all_stats = FBE_FALSE;
    fbe_bool_t brief_mode = FBE_FALSE;
    fbe_cli_perfstats_tags_t stat_tags = FBE_CLI_PERFSTATS_TAG_ALL;
    char *object_token;

    fbe_object_id_t *obj_list;
    fbe_u32_t       obj_count;
    fbe_u32_t       obj_i;
    
    fbe_u64_t container_ptr = 0;
    fbe_u32_t lun_offset;
    fbe_u32_t pdo_offset;

    fbe_status_t status;

    union{
        fbe_perfstats_physical_package_container_t *pp_container;
        fbe_perfstats_sep_container_t              *sep_container;
    }stats;

    fbe_object_id_t  object_id = FBE_OBJECT_ID_INVALID;
    fbe_package_id_t package_id = FBE_PACKAGE_ID_INVALID;
    fbe_u32_t        bus = FBE_API_PHYSICAL_BUS_COUNT;
    fbe_u32_t        enclosure = FBE_API_ENCLOSURES_PER_BUS;
    fbe_u32_t        slot = FBE_API_ENCLOSURE_SLOTS;
    fbe_bool_t       b_target_all = FBE_TRUE;

    fbe_class_id_t  filter_class_id;
    fbe_u32_t       object_count = 0;
    fbe_object_id_t *object_list_p = NULL;
    fbe_u32_t       total_objects = 0;
    fbe_class_id_t  class_id;
    fbe_u32_t       found_objects_count = 0;
    fbe_const_class_info_t      *class_info_p;

    if (argc < 2)
    {
        fbe_cli_printf("perfstats: ERROR: Too few args.\n");
        fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
        return;
    }

    //check package
    if (!strcmp(*argv, "-package"))
    {           
        argc--;                     
        argv++;
        if(argc) 
        {
            if (!strcmp(*argv, "sep"))
            {
                package_id = FBE_PACKAGE_ID_SEP_0;
            }
            else if (!strcmp(*argv, "pp"))
            {     
                package_id = FBE_PACKAGE_ID_PHYSICAL;
            }
        }
        else
        {
            fbe_cli_printf("ERROR: Invalid package name. Valid packages are sep and pp.\n");
            return;
        }
        argc--;                     
        argv++;
    }
    else
    {
        fbe_cli_printf("ERROR: You have to specify a package name.\n");
        fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
        return;
    }

    /***** NEED TO DO *****/
    /* Create a common routine that parses these args */
    /* and is shared by the log and dump functions.   */
    while (argc && **argv == '-')
    {
        if (!strcmp(*argv, "-object_id"))
        {
            argc--;
            argv++;
            if ((bus < FBE_API_PHYSICAL_BUS_COUNT) ||
                (enclosure < FBE_API_ENCLOSURES_PER_BUS) ||
                (slot < FBE_API_ENCLOSURE_SLOTS))
            {
                fbe_cli_error("-object_id, bus, enclosure, and/or slot numbers have already been specified.\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            object_token = *argv;
            if ((*object_token == '0') && 
                (*(object_token + 1) == 'x' || *(object_token + 1) == 'X'))
            {
                object_token += 2;
            }
            object_id = fbe_atoh(object_token);
            if ((object_id < 0) ||
                (object_id == FBE_OBJECT_ID_INVALID) ||
                (object_id > FBE_MAX_OBJECTS))
            {
                fbe_cli_printf("ERROR: Invalid object ID: 0x%x\n", object_id);
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            b_target_all = FBE_FALSE;
        }
        if (!strcmp(*argv, "-b"))
        {
            argc--;
            argv++;
            if (package_id != FBE_PACKAGE_ID_PHYSICAL)
            {
                fbe_cli_error("-b, a bus can only be specified for physical package statistics.\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            if (object_id != FBE_OBJECT_ID_INVALID)
            {
                fbe_cli_error("-b, object ID has already been specified\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            bus = fbe_atoi(*argv);
            if (bus < 0)
            {
                fbe_cli_error("-b, invalid bus number\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            b_target_all = FBE_FALSE;
        }
        if (!strcmp(*argv, "-e"))
        {
            argc--;
            argv++;
            if (package_id != FBE_PACKAGE_ID_PHYSICAL)
            {
                fbe_cli_error("-e, an enclosure can only be specified when dumping physical package statistics.\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            if (object_id != FBE_OBJECT_ID_INVALID)
            {
                fbe_cli_error("-e, object ID has already been specified\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            enclosure = fbe_atoi(*argv);
            if (enclosure < 0)
            {
                fbe_cli_error("-e, invalid enclosure number\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            b_target_all = FBE_FALSE;
        }
        if (!strcmp(*argv, "-s"))
        {
            argc--;
            argv++;
            if (package_id != FBE_PACKAGE_ID_PHYSICAL)
            {
                fbe_cli_error("-s, a slot can only be specified when dumping physical package statistics.\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            if (object_id != FBE_OBJECT_ID_INVALID)
            {
                fbe_cli_error("-s, object ID has already been specified\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            slot = fbe_atoi(*argv);
            if (slot < 0)
            {
                fbe_cli_error("-s, invalid slot number\n");
                fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                return;
            }
            b_target_all = FBE_FALSE;
        }
        if (!strcmp(*argv, "-summed"))
        {
            sum_all_stats = FBE_TRUE;
        }
        if (!strcmp(*argv, "-brief"))
        {
            brief_mode = FBE_TRUE;
        }
        //handle tags
        if (!strcmp(*argv, "-tag"))
        {
            argc--;
            argv++;
            stat_tags = 0;
            while (argc && **argv != '-')
            {
                 if (!strcmp(*argv, "lun_rblk"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_LUN_RBLK;
                 }
                 else if (!strcmp(*argv, "lun_wblk"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_LUN_WBLK;
                 }
                 else if (!strcmp(*argv, "lun_riops"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_LUN_RIOPS;
                 }
                 else if (!strcmp(*argv, "lun_wiops"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_LUN_WIOPS;
                 }
                 else if (!strcmp(*argv, "lun_stpc"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_LUN_STPC;
                 }
                 else if (!strcmp(*argv, "lun_mr3w"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_LUN_MR3W;
                 }
                 else if (!strcmp(*argv, "lun_rhis"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_LUN_RHIS;
                 }
                 else if (!strcmp(*argv, "lun_whis"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_LUN_WHIS;
                 }
                 else if (!strcmp(*argv, "rg_rblk"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_RG_RBLK;
                 }
                 else if (!strcmp(*argv, "rg_wblk"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_RG_WBLK;
                 }
                 else if (!strcmp(*argv, "rg_riops"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_RG_RIOPS;
                 }
                 else if (!strcmp(*argv, "rg_wiops"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_RG_WIOPS;
                 }
                 else if (!strcmp(*argv, "pdo_rblk"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_PDO_RBLK;
                 }
                 else if (!strcmp(*argv, "pdo_wblk"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_PDO_WBLK;
                 }
                 else if (!strcmp(*argv, "pdo_sblk"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_PDO_SBLK;
                 }
                 else if (!strcmp(*argv, "pdo_riops"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_PDO_RIOPS;
                 }
                 else if (!strcmp(*argv, "pdo_wiops"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_PDO_WIOPS;
                 }
                 else if (!strcmp(*argv, "pdo_util"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_PDO_UTIL;
                 }
                 else if (!strcmp(*argv, "lun_nzqa"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_LUN_NZQA;
                 }
                 else if (!strcmp(*argv, "lun_saql"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_LUN_SAQL;
                 }
                 else if (!strcmp(*argv, "lun_crrt"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_LUN_CRRT;
                 }
                 else if (!strcmp(*argv, "lun_cwrt"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_LUN_CWRT;
                 }
                 else if (!strcmp(*argv, "pdo_this"))
                 {
                     stat_tags |= FBE_CLI_PERFSTATS_TAG_PDO_THIS;
                 }
                 else
                 {//invalid tag
                      fbe_cli_printf("ERROR, \"%s\" is not a valid tag\n", *argv);
                      return;
                 }
                 argc--;
                 argv++;
            }
            if (argc == 0) {
                break;
            }
            
        }
        argc--;
        argv++;
    }
    //get the appropriate stat container and populate the object list
    if (fbe_cli_is_simulation()) 
    {
        status = fbe_api_perfstats_sim_get_statistics_container_for_package(&container_ptr, package_id);
    }
    else
    {
        status = fbe_api_perfstats_get_statistics_container_for_package(&container_ptr,package_id);
    }
    if (status != FBE_STATUS_OK) 
    {
        fbe_cli_printf("ERROR: Failed to get statistics package for package id: %d", package_id);
        return;
    }
   
    if (package_id == FBE_PACKAGE_ID_SEP_0)
    {
        
        stats.sep_container = (fbe_perfstats_sep_container_t *)(fbe_ptrhld_t) container_ptr;
        if (b_target_all == FBE_TRUE)
        { //get all the objects
            status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN,FBE_PACKAGE_ID_SEP_0,&obj_list,&obj_count);
        }
        else
        {
            obj_count = 1;
            obj_list = &object_id;
        }

        fbe_cli_printf("Object Count: %d\n", obj_count);

        for (obj_i = 0; obj_i < obj_count; obj_i++)
        { 
            //look up lun number and open the file
            fbe_api_perfstats_get_offset_for_object(*(obj_list+obj_i),
                                                    package_id,
                                                    &lun_offset);
            if (lun_offset >= PERFSTATS_INVALID_OBJECT_OFFSET) 
            {
                fbe_cli_printf("Invalid offset of object ID 0x%x\n", *(obj_list+obj_i));
                return;
            }
            if (sum_all_stats)
            {
                fbe_cli_printf("%d\n", lun_offset);
                fbe_cli_lib_perfstats_dump_summed_sep_stats(&stats.sep_container->lun_stats[lun_offset],
                                                            stat_tags,
                                                            brief_mode);
            }
            else
            {
                fbe_cli_lib_perfstats_dump_per_core_sep_stats(&stats.sep_container->lun_stats[lun_offset],
                                                              stat_tags,
                                                              brief_mode);
            }                            
        }
    } 
    else if (package_id == FBE_PACKAGE_ID_PHYSICAL)
    {
        stats.pp_container = (fbe_perfstats_physical_package_container_t *)(fbe_ptrhld_t) container_ptr;
        if (b_target_all == FBE_TRUE)
        {
            for (filter_class_id = FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST +1; filter_class_id < FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST; filter_class_id++)
            {
                status = fbe_api_get_total_objects(&object_count, package_id);
                
                if (status != FBE_STATUS_OK)
                {
                    return;
                }

                /* Allocate memory for the objects */
                object_list_p = fbe_api_allocate_memory(sizeof(fbe_object_id_t) * object_count);

                if (object_list_p == NULL)
               {
                    fbe_cli_error("Unable to allocate memory for object list %s\n", __FUNCTION__);
                    return;
                }

                /* Find the count of total objects */
                status = fbe_api_enumerate_objects(object_list_p, object_count, &total_objects, package_id);
                
                if (status != FBE_STATUS_OK)
                {
                    fbe_api_free_memory(object_list_p);
                    return;
                }

                /* Loop over all objects found in the table */
                for (obj_i = 0; obj_i < total_objects; obj_i++)
                {
                    fbe_object_id_t object_id = object_list_p[obj_i];
                    status = fbe_api_get_object_class_id(object_id, &class_id, package_id);

                    /* Match the class id to the filter class id. */
                    if ((status == FBE_STATUS_OK) && (class_id == filter_class_id))
                    {
                        //get offset for object_id
                        status = fbe_api_perfstats_get_offset_for_object(object_id,
                                                                         package_id,
                                                                         &pdo_offset);
                        if (status != FBE_STATUS_OK || pdo_offset >= PERFSTATS_INVALID_OBJECT_OFFSET)
                        {
                            fbe_cli_error("%s Failed to get offset for PDO object id: 0x%x\n", __FUNCTION__, object_id);
                            return;
                        }

                        stats.pp_container->pdo_stats[pdo_offset].object_id = object_id;
                        if (sum_all_stats)
                        {
                            fbe_cli_lib_perfstats_dump_summed_pp_stats(&stats.pp_container->pdo_stats[pdo_offset],
                                                                       stat_tags,
                                                                       brief_mode);
                        }
                        else
                        {
                            fbe_cli_lib_perfstats_dump_per_core_pp_stats(&stats.pp_container->pdo_stats[pdo_offset],
                                                                         stat_tags,
                                                                         brief_mode);
                        }

                        found_objects_count++;
                    } /* End of class id fetched OK and it is a match */
                    else if (status != FBE_STATUS_OK)
                    {
                        fbe_cli_error("can't get object class_id for id [%x], status: %x\n",
                                      object_id, status);
                    }
                } /* End of loop over all objects found */

                /* Display our totals */
                fbe_cli_printf("Discovered %.4d objects in total.\n", total_objects);
                fbe_cli_printf("Discovered %.4d objects matching filter of class %d.\n",
                               found_objects_count, filter_class_id);

                /* Free up the memory we allocated for the object list */
                fbe_api_free_memory(object_list_p);
            }
        }
        else
        {
            if (object_id == FBE_OBJECT_ID_INVALID)
            { 
                if ((bus >= FBE_API_PHYSICAL_BUS_COUNT) ||
                    (enclosure >= FBE_API_ENCLOSURES_PER_BUS) ||
                    (slot >= FBE_API_ENCLOSURE_SLOTS))
                {
                    fbe_cli_error("To specify a single physical drive input -b <bus> -e <enclosure> -s <slot>\n");
                    fbe_cli_printf("%s", PERFSTATS_CMD_USAGE);
                    return;
                }

                /* Get the object ID for this target */
                status = fbe_api_get_physical_drive_object_id_by_location(bus, enclosure, slot, &object_id);

                if ((status != FBE_STATUS_OK) || (object_id == FBE_OBJECT_ID_INVALID))
                {
                    fbe_cli_error("Failed to get a physical drive object ID for bus: %d  enclosure: %d  slot: %d\n",
                                  bus, enclosure, slot);
                    return;
                }
            }

            /* Verify that this object ID represents a valid physical drive */
            status = fbe_cli_get_class_info(object_id, package_id, &class_info_p);

            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("Failed to get the class info for this PDO object ID\n");
                return;
            }

            if ((class_info_p->class_id <= FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST) ||
                (class_info_p->class_id >= FBE_CLASS_ID_PHYSICAL_DRIVE_LAST))
            {
                fbe_cli_error("This object id 0x%x is not a physical drive but a %s (0x%x).\n",
                              object_id, class_info_p->class_name, class_info_p->class_id);
                return;
            }

            //get offset for object_id
            status = fbe_api_perfstats_get_offset_for_object(object_id,
                                                             package_id,
                                                             &pdo_offset);
            if (status != FBE_STATUS_OK || pdo_offset >= PERFSTATS_INVALID_OBJECT_OFFSET)
            {
                fbe_cli_error("%s Failed to get offset for PDO object id: 0x%x\n", __FUNCTION__, object_id);
                return;
            }

            stats.pp_container->pdo_stats[pdo_offset].object_id = object_id;
            if (sum_all_stats)
            {
                fbe_cli_lib_perfstats_dump_summed_pp_stats(&stats.pp_container->pdo_stats[pdo_offset],
                                                           stat_tags,
                                                           brief_mode);
            }
            else
            {
                fbe_cli_lib_perfstats_dump_per_core_pp_stats(&stats.pp_container->pdo_stats[pdo_offset],
                                                             stat_tags,
                                                             brief_mode);
            }
        }
    }
    else
    {
        fbe_cli_error("Unrecognized package id: %d is not supported\n", package_id);
    }

    return;
}
/**********************************
* end fbe_cli_cmd_perfstats_dump()
**********************************/

static void fbe_cli_lib_perfstats_dump_summed_sep_stats(fbe_lun_performance_counters_t  *lun_stats,
                                                        fbe_cli_perfstats_tags_t        stat_tags,
                                                        fbe_bool_t                      brief_mode)
{
    fbe_perfstats_lun_sum_t summed_stats;
    fbe_lun_number_t        lun_number;
    fbe_u32_t               histogram_bucket_i;
    fbe_u32_t               disk_i;

    fbe_api_perfstats_get_summed_lun_stats(&summed_stats, lun_stats);
    
    //lun number
    fbe_api_database_lookup_lun_by_object_id(lun_stats->object_id,
                                             &lun_number);

    fbe_cli_printf("=== LUN %d (ID:0x%x) ===\n", lun_number, lun_stats->object_id);

    //for each tag, print out stats
    //header
    fbe_cli_printf("                            |        -SUM-         \n");
    fbe_cli_printf("---------------------------------------------------\n");
    //LUN Read blocks
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_RBLK) 
    {
        if (FBE_IS_FALSE(brief_mode) || summed_stats.lun_blocks_read) 
        {
            fbe_cli_printf("LUN Blocks Read:            |%20lu\n", (unsigned long)summed_stats.lun_blocks_read);
        }
    }

    //LUN Write blocks
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_WBLK) 
    {
        if (FBE_IS_FALSE(brief_mode) || summed_stats.lun_blocks_written) 
        {
            fbe_cli_printf("LUN Blocks Written:         |%20lu\n", (unsigned long)summed_stats.lun_blocks_written);
        }
    }

    //LUN Read IOs
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_RIOPS)
    {
       if (FBE_IS_FALSE(brief_mode) || summed_stats.lun_read_requests) 
        {
            fbe_cli_printf("LUN Read IO Count:          |%20lu\n", (unsigned long)summed_stats.lun_read_requests);
        }
    }

    //LUN Read IOs
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_WIOPS)
    {
        if (FBE_IS_FALSE(brief_mode) || summed_stats.lun_write_requests) 
        {
            fbe_cli_printf("LUN Write IO Count:         |%20lu\n", (unsigned long)summed_stats.lun_write_requests);
        }
    }

    //LUN Stripe Crossings
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_STPC)
    {
        if (FBE_IS_FALSE(brief_mode) || summed_stats.stripe_crossings)
        {
            fbe_cli_printf("LUN Stripe Crossings:       |%20lu\n", (unsigned long)summed_stats.stripe_crossings);
        }
    }

    //LUN Stripe Writes
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_MR3W)
    {
        if (FBE_IS_FALSE(brief_mode) || summed_stats.stripe_writes)
        {
            fbe_cli_printf("LUN Stripe Writes:          |%20lu\n", (unsigned long)summed_stats.stripe_writes);
        }
    }

    //LUN Read Size Histogram
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_RHIS)
    {
        fbe_u32_t bucket_size = 1;
        for (histogram_bucket_i = 0; histogram_bucket_i < FBE_PERFSTATS_HISTOGRAM_BUCKET_LAST; histogram_bucket_i++) 
        {
            if (FBE_IS_FALSE(brief_mode) || summed_stats.lun_io_size_read_histogram[histogram_bucket_i])
            {
                fbe_cli_printf("LUN %4d Block Reads:       |%20lu\n", bucket_size, (unsigned long)summed_stats.lun_io_size_read_histogram[histogram_bucket_i]);
            }
            bucket_size *=2;
        } 
    }

    //LUN Write Size Histogram
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_WHIS)
    {
        fbe_u32_t bucket_size = 1;
        for (histogram_bucket_i = 0; histogram_bucket_i < FBE_PERFSTATS_HISTOGRAM_BUCKET_LAST; histogram_bucket_i++) 
        {
            if (FBE_IS_FALSE(brief_mode) || summed_stats.lun_io_size_write_histogram[histogram_bucket_i])
            {
                fbe_cli_printf("LUN %4d Block Writes:      |%20lu\n", bucket_size, (unsigned long)summed_stats.lun_io_size_write_histogram[histogram_bucket_i]);
            }
            bucket_size *=2;
        } 
    }

    //RG Read Blocks (per_disk)
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_RG_RBLK)
    {
        for(disk_i = 0; disk_i < FBE_XOR_MAX_FRUS; disk_i++) 
        {
            if (FBE_IS_FALSE(brief_mode) || summed_stats.disk_blocks_read[disk_i])
            {
                fbe_cli_printf("RG Blocks Read (pos %2d):    |%20lu\n", disk_i, (unsigned long)summed_stats.disk_blocks_read[disk_i]);
            }
        }
    }

    //RG Write Blocks (per_disk)
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_RG_WBLK)
    {
        for(disk_i = 0; disk_i < FBE_XOR_MAX_FRUS; disk_i++) 
        {
            if (FBE_IS_FALSE(brief_mode) || summed_stats.disk_blocks_written[disk_i])
            {
                fbe_cli_printf("RG Blocks Written (pos %2d): |%20lu\n", disk_i, (unsigned long)summed_stats.disk_blocks_written[disk_i]);
            }
        }
    }

    //RG Read Requests
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_RG_RIOPS)
    {
        for(disk_i = 0; disk_i < FBE_XOR_MAX_FRUS; disk_i++) 
        {
            if (FBE_IS_FALSE(brief_mode) || summed_stats.disk_reads[disk_i])
            {
                fbe_cli_printf("RG Reads (pos %2d):          |%20lu\n", disk_i, (unsigned long)summed_stats.disk_reads[disk_i]);
            }
        }
    }

    //RG Write Requests
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_RG_WIOPS)
    {
        for(disk_i = 0; disk_i < FBE_XOR_MAX_FRUS; disk_i++) 
        {
            if (FBE_IS_FALSE(brief_mode) || summed_stats.disk_writes[disk_i])
            {
                fbe_cli_printf("RG Writes (pos %2d):         |%20lu\n", disk_i, (unsigned long)summed_stats.disk_writes[disk_i]);
            }
        }
    }

    //LUN Nonzero Queue Arrivals
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_NZQA)
    {
        if (FBE_IS_FALSE(brief_mode) || summed_stats.non_zero_queue_arrivals)
        {
            fbe_cli_printf("LUN Nonzero Q Arrivals:     |%20lu\n", (unsigned long)summed_stats.non_zero_queue_arrivals);
        }
    }

    //LUN Sum Queue Arrival Length
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_SAQL)
    {
        if (FBE_IS_FALSE(brief_mode) || summed_stats.sum_arrival_queue_length)
        {
            fbe_cli_printf("LUN Sum Q Arrival Len:      |%20lu\n", (unsigned long)summed_stats.sum_arrival_queue_length);
        }
    }

    //LUN Cumulative read response time
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_CRRT)
    {
        if (FBE_IS_FALSE(brief_mode) || summed_stats.cumulative_read_response_time)
        {
            fbe_cli_printf("LUN Cum. Read Response Time |%20lu\n", (unsigned long)summed_stats.cumulative_read_response_time);
        }
    }

    //LUN Cumulative write response time
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_CWRT)
    {
        if (FBE_IS_FALSE(brief_mode) || summed_stats.cumulative_write_response_time)
        {
            fbe_cli_printf("LUN Cum. Write Response Time|%20lu\n", (unsigned long)summed_stats.cumulative_write_response_time);
        }
    }
        
    fbe_cli_printf("\n");
    return;
}


static void fbe_cli_lib_perfstats_dump_per_core_sep_stats(fbe_lun_performance_counters_t *lun_stats,
                                                          fbe_cli_perfstats_tags_t stat_tags,
                                                          fbe_bool_t                brief_mode)
{
    fbe_lun_number_t        lun_number;
    fbe_u32_t               histogram_bucket_i;
    fbe_u32_t               disk_i;
    fbe_cpu_id_t            core_i;
    fbe_u32_t               total_cores;
    fbe_u64_t               intermediate_sum = 0;
    char line_buffer[512];
    char token_buffer[32];


    fbe_object_id_t object_id;
    fbe_board_mgmt_platform_info_t platform_info;

    fbe_api_get_board_object_id(&object_id);
    fbe_api_board_get_platform_info(object_id, &platform_info);
    if (fbe_cli_is_simulation())
    {
        total_cores = 6; //use six cores for simulation, doesn't really matter
    }
    else
    {
        total_cores = platform_info.hw_platform_info.processorCount;
    }

    //lun number
    fbe_api_database_lookup_lun_by_object_id(lun_stats->object_id,
                                             &lun_number);

    fbe_cli_printf("=== LUN %d (ID:0x%x) ===\n", lun_number, lun_stats->object_id);

    //for each tag, print out stats
    //header       
    fbe_cli_printf("                             |");
    for (core_i = 0; core_i < total_cores; core_i++)
    {
        fbe_cli_printf(" Core%02d   |", core_i);
    }
    fbe_cli_printf("\n------------------------------");
    for (core_i = 0; core_i < total_cores; core_i++)
    {
        fbe_cli_printf("-----------");
    }

    //LUN Read blocks
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_RBLK) 
    {
        intermediate_sum = 0;
        line_buffer[0] = '\0';
        for (core_i = 0; core_i < total_cores; core_i++)
        {
            sprintf(token_buffer, "%10lu|", (unsigned long)lun_stats->lun_blocks_read[core_i]);
            intermediate_sum += lun_stats->lun_blocks_read[core_i];
            strcat(line_buffer, token_buffer);
        }
        if (FBE_IS_FALSE(brief_mode) || intermediate_sum) 
        {
            fbe_cli_printf("\nLUN Blocks Read:             |%s", line_buffer);
        }
    }

    //LUN Write blocks
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_WBLK) 
    {
        intermediate_sum = 0;
        line_buffer[0] = '\0';
        for (core_i = 0; core_i < total_cores; core_i++)
        {
            sprintf(token_buffer, "%10lu|", (unsigned long)lun_stats->lun_blocks_written[core_i]);
            intermediate_sum += lun_stats->lun_blocks_written[core_i];
            strcat(line_buffer, token_buffer);
        }
        if (FBE_IS_FALSE(brief_mode) || intermediate_sum) 
        {
            fbe_cli_printf("\nLUN Blocks Written:          |%s", line_buffer);
        }
    }

    //LUN Read IOs
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_RIOPS)
    {   
        intermediate_sum = 0;
        line_buffer[0] = '\0';
        for (core_i = 0; core_i < total_cores; core_i++)
        {
            sprintf(token_buffer, "%10lu|", (unsigned long)lun_stats->lun_read_requests[core_i]);
            intermediate_sum += lun_stats->lun_read_requests[core_i];
            strcat(line_buffer, token_buffer);
        }
        if (FBE_IS_FALSE(brief_mode) || intermediate_sum) 
        {
            fbe_cli_printf("\nLUN Reads:                   |%s", line_buffer);
        }
    }

    //LUN Read IOs
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_WIOPS)
    {
        intermediate_sum = 0;
        line_buffer[0] = '\0';
        for (core_i = 0; core_i < total_cores; core_i++)
        {
            sprintf(token_buffer, "%10lu|", (unsigned long)lun_stats->lun_write_requests[core_i]);
            intermediate_sum += lun_stats->lun_write_requests[core_i];
            strcat(line_buffer, token_buffer);
        }
        if (FBE_IS_FALSE(brief_mode) || intermediate_sum) 
        {
            fbe_cli_printf("\nLUN Writes:                  |%s", line_buffer);
        }
    }

    //LUN Stripe Crossings
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_STPC)
    {
        intermediate_sum = 0;
        line_buffer[0] = '\0';
        for (core_i = 0; core_i < total_cores; core_i++)
        {
            sprintf(token_buffer, "%10lu|", (unsigned long)lun_stats->stripe_crossings[core_i]);
            intermediate_sum += lun_stats->stripe_crossings[core_i];
            strcat(line_buffer, token_buffer);
        }
        if (FBE_IS_FALSE(brief_mode) || intermediate_sum) 
        {
            fbe_cli_printf("\nLUN Stripe Crossings:        |%s", line_buffer);
        }
    }

    //LUN Stripe Writes
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_MR3W)
    {
        intermediate_sum = 0;
        line_buffer[0] = '\0';
        for (core_i = 0; core_i < total_cores; core_i++)
        {
            sprintf(token_buffer, "%10lu|", (unsigned long)lun_stats->stripe_writes[core_i]);
            intermediate_sum += lun_stats->stripe_writes[core_i];
            strcat(line_buffer, token_buffer);
        }
        if (FBE_IS_FALSE(brief_mode) || intermediate_sum) 
        {
            fbe_cli_printf("\nLUN Stripe Writes:           |%s", line_buffer);
        }
    }

    //LUN Read Size Histogram
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_RHIS)
    {
        fbe_u32_t bucket_size = 1;
        for (histogram_bucket_i = 0; histogram_bucket_i < FBE_PERFSTATS_HISTOGRAM_BUCKET_LAST; histogram_bucket_i++) 
        {
            intermediate_sum = 0;
            line_buffer[0] = '\0';
            
            for (core_i = 0; core_i < total_cores; core_i++)
            {
                sprintf(token_buffer, "%10lu|", (unsigned long)lun_stats->lun_io_size_read_histogram[histogram_bucket_i][core_i]);
                intermediate_sum += lun_stats->lun_io_size_read_histogram[histogram_bucket_i][core_i];
                strcat(line_buffer, token_buffer);
            }
            if (FBE_IS_FALSE(brief_mode) || intermediate_sum)
            {
                fbe_cli_printf("\nLUN %4d Block Reads:        |%s", bucket_size, line_buffer);
            }
            bucket_size *=2;
        } 
    }

    //LUN Write Size Histogram
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_WHIS)
    {
       fbe_u32_t bucket_size = 1;
        for (histogram_bucket_i = 0; histogram_bucket_i < FBE_PERFSTATS_HISTOGRAM_BUCKET_LAST; histogram_bucket_i++) 
        {
            intermediate_sum = 0;
            line_buffer[0] = '\0';
            
            for (core_i = 0; core_i < total_cores; core_i++)
            {
                sprintf(token_buffer, "%10lu|", (unsigned long)lun_stats->lun_io_size_write_histogram[histogram_bucket_i][core_i]);
                intermediate_sum += lun_stats->lun_io_size_write_histogram[histogram_bucket_i][core_i];
                strcat(line_buffer, token_buffer);
            }
            if (FBE_IS_FALSE(brief_mode) || intermediate_sum)
            {
                fbe_cli_printf("\nLUN %4d Block Writes:       |%s", bucket_size, line_buffer);
            }
            bucket_size *=2;
        } 
    }

    //RG Read Blocks (per_disk)
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_RG_RBLK)
    {
        for(disk_i = 0; disk_i < FBE_XOR_MAX_FRUS; disk_i++) 
        {
            intermediate_sum = 0;
            line_buffer[0] = '\0';
            
            for (core_i = 0; core_i < total_cores; core_i++)
            {
                sprintf(token_buffer, "%10lu|", (unsigned long)lun_stats->disk_blocks_read[disk_i][core_i]);
                intermediate_sum += lun_stats->disk_blocks_read[disk_i][core_i];
                strcat(line_buffer, token_buffer);
            }
            if (FBE_IS_FALSE(brief_mode) || intermediate_sum)
            {
                fbe_cli_printf("\nRG Blocks Read (pos %2d):     |%s", disk_i, line_buffer);
            }
        }
    }

    //RG Write Blocks (per_disk)
   if (stat_tags & FBE_CLI_PERFSTATS_TAG_RG_WBLK)
    {
        for(disk_i = 0; disk_i < FBE_XOR_MAX_FRUS; disk_i++) 
        {
            intermediate_sum = 0;
            line_buffer[0] = '\0';
            
            for (core_i = 0; core_i < total_cores; core_i++)
            {
                sprintf(token_buffer, "%10lu|", (unsigned long)lun_stats->disk_blocks_written[disk_i][core_i]);
                intermediate_sum += lun_stats->disk_blocks_written[disk_i][core_i];
                strcat(line_buffer, token_buffer);
            }
            if (FBE_IS_FALSE(brief_mode) || intermediate_sum)
            {
                fbe_cli_printf("\nRG Blocks Written(pos %2d):   |%s", disk_i, line_buffer);
            }
        }
    }

    //RG Read Requests
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_RG_RIOPS)
    {
        for(disk_i = 0; disk_i < FBE_XOR_MAX_FRUS; disk_i++) 
        {
            intermediate_sum = 0;
            line_buffer[0] = '\0';
            
            for (core_i = 0; core_i < total_cores; core_i++)
            {
                sprintf(token_buffer, "%10lu|", (unsigned long)lun_stats->disk_reads[disk_i][core_i]);
                intermediate_sum += lun_stats->disk_reads[disk_i][core_i];
                strcat(line_buffer, token_buffer);
            }
            if (FBE_IS_FALSE(brief_mode) || intermediate_sum)
            {
                fbe_cli_printf("\nRG Reads(pos %2d):            |%s", disk_i, line_buffer);
            }
        }
    }

    //RG Write Requests
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_RG_WIOPS)
    {
        for(disk_i = 0; disk_i < FBE_XOR_MAX_FRUS; disk_i++) 
        {
            intermediate_sum = 0;
            line_buffer[0] = '\0';
            
            for (core_i = 0; core_i < total_cores; core_i++)
            {
                sprintf(token_buffer, "%10lu|", (unsigned long)lun_stats->disk_writes[disk_i][core_i]);
                intermediate_sum += lun_stats->disk_writes[disk_i][core_i];
                strcat(line_buffer, token_buffer);
           }
            if (FBE_IS_FALSE(brief_mode) || intermediate_sum)
            {
                fbe_cli_printf("\nRG Writes(pos %2d):           |%s", disk_i, line_buffer);
            }
        }
    }

    //LUN Nonzero Queue Arrival
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_NZQA)
    {
        intermediate_sum = 0;
        line_buffer[0] = '\0';
        for (core_i = 0; core_i < total_cores; core_i++)
        {
            sprintf(token_buffer, "%10lu|", (unsigned long)lun_stats->non_zero_queue_arrivals[core_i]);
            intermediate_sum += lun_stats->non_zero_queue_arrivals[core_i];
            strcat(line_buffer, token_buffer);
        }
        if (FBE_IS_FALSE(brief_mode) || intermediate_sum) 
        {
            fbe_cli_printf("\nLUN Nonzero Q Arrivals:      |%s", line_buffer);
        }
    }

    //LUN Sum Queue Arrival Length
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_SAQL)
    {
        intermediate_sum = 0;
        line_buffer[0] = '\0';
        for (core_i = 0; core_i < total_cores; core_i++)
        {
            sprintf(token_buffer, "%10lu|", (unsigned long)lun_stats->sum_arrival_queue_length[core_i]);
            intermediate_sum += lun_stats->sum_arrival_queue_length[core_i];
            strcat(line_buffer, token_buffer);
        }
        if (FBE_IS_FALSE(brief_mode) || intermediate_sum) 
        {
            fbe_cli_printf("\nLUN Sum Arrival Q Len:       |%s", line_buffer);
        }
    }

    //LUN Cumulative Read Response Time
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_CRRT)
    {
        intermediate_sum = 0;
        line_buffer[0] = '\0';
        for (core_i = 0; core_i < total_cores; core_i++)
        {
            sprintf(token_buffer, "%10lu|", (unsigned long)lun_stats->cumulative_read_response_time[core_i]);
            intermediate_sum += lun_stats->cumulative_read_response_time[core_i];
            strcat(line_buffer, token_buffer);
        }
        if (FBE_IS_FALSE(brief_mode) || intermediate_sum) 
        {
            fbe_cli_printf("\nLUN Cum. Read Response Time: |%s", line_buffer);
        }
    }

    //LUN Cumulative Read Response Time
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_CWRT)
    {
        intermediate_sum = 0;
        line_buffer[0] = '\0';
        for (core_i = 0; core_i < total_cores; core_i++)
        {
            sprintf(token_buffer, "%10lu|", (unsigned long)lun_stats->cumulative_write_response_time[core_i]);
            intermediate_sum += lun_stats->cumulative_write_response_time[core_i];
            strcat(line_buffer, token_buffer);
        }
        if (FBE_IS_FALSE(brief_mode) || intermediate_sum) 
        {
            fbe_cli_printf("\nLUN Cum. Write Response Time:|%s", line_buffer);
        }
    }
       
    fbe_cli_printf("\n\n\n");
    return;
}

/*!**************************************************************
*          fbe_cli_lib_perfstats_print_summed_sep_stats()
****************************************************************
*
* @brief   Print the summed counters from an fbe_lun_performance_counters_t to a file
* 
 *  @param    fp - pointer to a file handle
*  @param    stat_tags - bitmask of which counters we want to actually print to file
*              @see fbe_cli_perfstats_tags_t
*  @param    old_stats - pointer to the lun performance counters from the last sample we took
*  @param    new_stats - pointer to the lun performance counters from the latest sample we took
*
* @return  None.
*
* @revision:
*  05/01/2012 Created. Ryan Gadsby
*
****************************************************************/
static void fbe_cli_lib_perfstats_print_summed_sep_stats(FILE *fp,
                                                         fbe_cli_perfstats_tags_t stat_tags,
                                                         fbe_lun_performance_counters_t *old_stats,
                                                         fbe_lun_performance_counters_t *new_stats)
{
    fbe_perfstats_lun_sum_t old_sum;
    fbe_perfstats_lun_sum_t new_sum;
    fbe_u64_t               time_difference = 0;
    fbe_u32_t               disk_i = 0;
    fbe_u32_t               histogram_bucket_i = 0;

    fbe_api_perfstats_get_summed_lun_stats(&old_sum, old_stats);
    fbe_api_perfstats_get_summed_lun_stats(&new_sum, new_stats);


    time_difference = (new_sum.timestamp - old_sum.timestamp)/1000;
    
    if (time_difference) { //otherwise counters haven't been updated
        fprintf(fp,"%lu,", (unsigned long)new_sum.timestamp); //always print timestamp
        //LUN Read MB/s
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_RBLK) 
        {
            fprintf(fp,"%llu,", (new_sum.lun_blocks_read - old_sum.lun_blocks_read)/time_difference);
        }

        //LUN Write MB/s
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_WBLK)
        {
            fprintf(fp,"%llu,", (new_sum.lun_blocks_written - old_sum.lun_blocks_written)/time_difference); 
        }

        //LUN Read IOPS
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_RIOPS)
        {
            fprintf(fp,"%llu,", (new_sum.lun_read_requests - old_sum.lun_read_requests) / time_difference);
        }

        //LUN Write IOPS
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_WIOPS)
        {
            fprintf(fp,"%llu,", (new_sum.lun_write_requests - old_sum.lun_write_requests) / time_difference);
        }

        //LUN Stripe Crossings
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_STPC)
        {
            fprintf(fp,"%llu,", (new_sum.stripe_crossings - old_sum.stripe_crossings)/time_difference);
        }

        //LUN Stripe Writes
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_MR3W)
        {
            fprintf(fp,"%llu,", (new_sum.stripe_writes - old_sum.stripe_writes)/time_difference);
        }

        //LUN Read Size Histogram
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_RHIS)
        {
            for (histogram_bucket_i = 0; histogram_bucket_i < FBE_PERFSTATS_HISTOGRAM_BUCKET_LAST; histogram_bucket_i++) {
                fprintf(fp,"%lu,", (long unsigned int)((new_sum.lun_io_size_read_histogram[histogram_bucket_i] - old_sum.lun_io_size_read_histogram[histogram_bucket_i])/time_difference));
            } 
        }

        //LUN Write Size Histogram
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_WHIS)
        {
            for (histogram_bucket_i = 0; histogram_bucket_i < FBE_PERFSTATS_HISTOGRAM_BUCKET_LAST; histogram_bucket_i++) 
            {
                fprintf(fp,"%lu,", (long unsigned int)((new_sum.lun_io_size_write_histogram[histogram_bucket_i] - old_sum.lun_io_size_write_histogram[histogram_bucket_i])/time_difference));
            } 
        }

        //RG Read MB/s (per_disk)
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_RG_RBLK)
        {
            for(disk_i = 0; disk_i < FBE_XOR_MAX_FRUS; disk_i++) 
            {
                fprintf(fp,"%llu,", (new_sum.disk_blocks_read[disk_i] - old_sum.disk_blocks_read[disk_i])/time_difference);
            }
        }
        //RG Write MB/s (per_disk)
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_RG_WBLK)
        {
            for(disk_i = 0; disk_i < FBE_XOR_MAX_FRUS; disk_i++) 
            {
                fprintf(fp,"%llu,", (new_sum.disk_blocks_written[disk_i] - old_sum.disk_blocks_written[disk_i])/time_difference);
            }
        }
        //RG Read IOPS (per_disk)
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_RG_RIOPS)
        {
            for(disk_i = 0; disk_i < FBE_XOR_MAX_FRUS; disk_i++)
            {
                fprintf(fp,"%llu,", (new_sum.disk_reads[disk_i] - old_sum.disk_reads[disk_i])/time_difference);
            }
        } 
        //RG Write IOPS (per_disk)
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_RG_WIOPS)
        {
            for(disk_i = 0; disk_i < FBE_XOR_MAX_FRUS; disk_i++)
            {
                fprintf(fp,"%lu,", (long unsigned int)((new_sum.disk_writes[disk_i] - old_sum.disk_writes[disk_i])/time_difference));
            }
        }
        //LUN Nonzero Queue Arrivals
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_NZQA)
        {
            fprintf(fp,"%lu,", (long unsigned int)((new_sum.non_zero_queue_arrivals - old_sum.non_zero_queue_arrivals)/time_difference));
        }
        //LUN Sum Arrival Queue Length
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_SAQL)
        {
            fprintf(fp,"%lu,", (long unsigned int)((new_sum.sum_arrival_queue_length - old_sum.sum_arrival_queue_length)/time_difference));
        }
        //LUN Cumulative Read Response Time
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_CRRT)
        {
            fprintf(fp,"%lu,", (long unsigned int)((new_sum.cumulative_read_response_time - old_sum.cumulative_read_response_time)/time_difference));
        }
        //LUN Cumulative Write Response Time
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_CWRT)
        {
            fprintf(fp,"%lu,", (long unsigned int)((new_sum.cumulative_write_response_time - old_sum.cumulative_write_response_time)/time_difference));
        }
        fprintf(fp, "\n");
    }
    fflush(fp);
    return;
}

/******************************************
* end fbe_cli_lib_perfstats_print_summed_sep_stats()
******************************************/

/*!**************************************************************
*          fbe_cli_lib_perfstats_print_per_core_sep_stats()
****************************************************************
*
* @brief   Print the per-core counters from an fbe_lun_performance_counters_t to a file
* 
 *  @param    fp - pointer to a file handle
*  @param    stat_tags - bitmask of which counters we want to actually print to file
*              @see fbe_cli_perfstats_tags_t
*  @param    old_stats - pointer to the lun performance counters from the last sample we took
*  @param    new_stats - pointer to the lun performance counters from the latest sample we took
*
* @return  None.
*
* @revision:
*  05/01/2012 Created. Ryan Gadsby
*
****************************************************************/
static void fbe_cli_lib_perfstats_print_per_core_sep_stats(FILE *fp,
                                                           fbe_cli_perfstats_tags_t stat_tags,
                                                           fbe_lun_performance_counters_t *old_stats,
                                                           fbe_lun_performance_counters_t *new_stats){
    fbe_cpu_id_t            core_i = 0;
    fbe_u32_t               disk_i = 0;
    fbe_u32_t               histogram_bucket_i = 0;
    fbe_u64_t               time_difference = 0;
    fbe_status_t            status;
    fbe_u32_t               total_cores;
    fbe_object_id_t object_id;
    fbe_board_mgmt_platform_info_t platform_info;

    status = fbe_api_get_board_object_id(&object_id);
    status = fbe_api_board_get_platform_info(object_id, &platform_info);
    total_cores = platform_info.hw_platform_info.processorCount;

    time_difference = (new_stats->timestamp - old_stats->timestamp) / 1000;

    if (time_difference) 
    {
        fprintf(fp,"%lu,", (unsigned long)new_stats->timestamp); //always print timestamp
        //LUN Read MB/s
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_RBLK)
        {
            for (core_i = 0; core_i < total_cores; core_i++) 
            {
                fprintf(fp,"%llu,", (new_stats->lun_blocks_read[core_i] - old_stats->lun_blocks_read[core_i])/time_difference);
            }
        }

        //LUN Write MB/s
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_WBLK)
        {
            for (core_i = 0; core_i < total_cores; core_i++) 
            {
                fprintf(fp,"%llu,", (new_stats->lun_blocks_written[core_i] - old_stats->lun_blocks_written[core_i])/time_difference);
            }
        }

        //LUN Read IOPS
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_RIOPS)
        {
            for (core_i = 0; core_i < total_cores; core_i++) 
            {
                fprintf(fp,"%llu,", (new_stats->lun_read_requests[core_i] - old_stats->lun_read_requests[core_i])/time_difference);
            }
        }

        //LUN Write IOPS
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_WIOPS)
        {
            for (core_i = 0; core_i < total_cores; core_i++) 
            {
                fprintf(fp,"%llu,", (new_stats->lun_write_requests[core_i] - old_stats->lun_write_requests[core_i])/time_difference);
            }
        }

        //LUN Stripe Crossings
       if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_STPC) 
        {
            for (core_i = 0; core_i < total_cores; core_i++)
            {
                fprintf(fp,"%llu,", (new_stats->stripe_crossings[core_i] - old_stats->stripe_crossings[core_i]) / time_difference);
            }
        }

        //LUN Stripe Writes
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_MR3W) 
        {
            for (core_i = 0; core_i < total_cores; core_i++)
            {
                fprintf(fp,"%llu,", (new_stats->stripe_writes[core_i] - old_stats->stripe_writes[core_i]) / time_difference);
            }
        }

        //LUN Read Size Histogram
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_RHIS)
        {
            for (core_i = 0; core_i < total_cores; core_i++)
            {
                for (histogram_bucket_i = 0; histogram_bucket_i < FBE_PERFSTATS_HISTOGRAM_BUCKET_LAST; histogram_bucket_i++) 
                {
                    fprintf(fp,"%llu,", (new_stats->lun_io_size_read_histogram[histogram_bucket_i][core_i] - old_stats->lun_io_size_read_histogram[histogram_bucket_i][core_i])/time_difference);
                } 
            }
        }

        //LUN Write Size Histogram
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_WHIS)
        {
            for (core_i = 0; core_i < total_cores; core_i++)
            {
                for (histogram_bucket_i = 0; histogram_bucket_i < FBE_PERFSTATS_HISTOGRAM_BUCKET_LAST; histogram_bucket_i++) 
                {
                    fprintf(fp,"%llu,", (new_stats->lun_io_size_write_histogram[histogram_bucket_i][core_i] - old_stats->lun_io_size_write_histogram[histogram_bucket_i][core_i])/time_difference);
                } 
            }
        }

        //RG Read MB/s (per_disk)
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_RG_RBLK)
        {
            for (core_i = 0; core_i < total_cores; core_i++)
            {
                for(disk_i = 0; disk_i < FBE_XOR_MAX_FRUS; disk_i++) 
                {
                    fprintf(fp,"%llu,", (new_stats->disk_blocks_read[disk_i][core_i] - old_stats->disk_blocks_read[disk_i][core_i]) / time_difference);
                }
            }
        }
        //RG Write MB/s (per_disk)
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_RG_WBLK)
        {
            for (core_i = 0; core_i < total_cores; core_i++)
            {
                for(disk_i = 0; disk_i < FBE_XOR_MAX_FRUS; disk_i++)
                {
                    fprintf(fp,"%llu,", (new_stats->disk_blocks_written[disk_i][core_i] - old_stats->disk_blocks_written[disk_i][core_i]) / time_difference);
                }
            }
        }
        //RG Read IOPS (per_disk)
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_RG_RIOPS)
        {
             for (core_i = 0; core_i < total_cores; core_i++)
             {
                for(disk_i = 0; disk_i < FBE_XOR_MAX_FRUS; disk_i++) 
                {
                    fprintf(fp,"%llu,", ((new_stats->disk_reads[disk_i][core_i] - old_stats->disk_reads[disk_i][core_i]) / 2097152)/ time_difference);
                }
            }
        } 
        //RG Write IOPS (per_disk)
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_RG_WIOPS)
        {
            for (core_i = 0; core_i < total_cores; core_i++)
            {
                for(disk_i = 0; disk_i < FBE_XOR_MAX_FRUS; disk_i++) 
                {
                    fprintf(fp,"%llu,", ((new_stats->disk_writes[disk_i][core_i] - old_stats->disk_writes[disk_i][core_i]) / 2097152)/ time_difference);
                }
            }
        }

        //LUN Nonzero Queue Arrivals
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_NZQA)
        {
            for (core_i = 0; core_i < total_cores; core_i++)
            {
                fprintf(fp,"%lu,", (long unsigned int)((new_stats->non_zero_queue_arrivals[core_i] - old_stats->non_zero_queue_arrivals[core_i])/time_difference));
            }
        }
        //LUN Sum Arrival Queue Length
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_SAQL)
        {
            for (core_i = 0; core_i < total_cores; core_i++)
            {
                fprintf(fp,"%lu,", (long unsigned int)((new_stats->sum_arrival_queue_length[core_i] - old_stats->sum_arrival_queue_length[core_i])/time_difference));
            }
        }
        //LUN Cumulative Read Response Time
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_CRRT)
        {
            for (core_i = 0; core_i < total_cores; core_i++)
            {
                fprintf(fp,"%lu,", (long unsigned int)((new_stats->cumulative_read_response_time[core_i] - old_stats->cumulative_read_response_time[core_i])/time_difference));
            }
        }
        //LUN Cumulative Write Response Time
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_CWRT)
        {
            for (core_i = 0; core_i < total_cores; core_i++)
            {
                fprintf(fp,"%lu,", (long unsigned int)((new_stats->cumulative_write_response_time[core_i] - old_stats->cumulative_write_response_time[core_i])/time_difference));
            }
        }
        fprintf(fp, "\n");
    }
    fflush(fp);
    return;
}

/******************************************
* end fbe_cli_lib_perfstats_print_summed_sep_stats()
******************************************/

/*!**************************************************************
*          fbe_cli_lib_perfstats_print_sep_header()
****************************************************************
*
* @brief   Print the per-core counters from an fbe_lun_performance_counters_t to a file
* 
 *  @param    fp - pointer to a file handle
*  @param    stat_tags - bitmask of which counters we want to actually print to file
*              @see fbe_cli_perfstats_tags_t
*  @param    summed - FBE_TRUE if using summed stats, FBE_FALSE if using per-core stats
*
* @return  None.
*
* @revision:
*  05/01/2012 Created. Ryan Gadsby
*
****************************************************************/

static void fbe_cli_lib_perfstats_print_sep_header(FILE *fp,
                                                   fbe_cli_perfstats_tags_t stat_tags,
                                                   fbe_bool_t   summed)
{
    fbe_u32_t total_cores;
    fbe_u32_t core_i;
    fbe_u32_t blk_count_i;
    fbe_u32_t disk_i;
    fbe_status_t status;
    fbe_object_id_t object_id;
    fbe_board_mgmt_platform_info_t platform_info;

    if (summed)
    {
        total_cores = 1; 
    }
    else
    {   
        status = fbe_api_get_board_object_id(&object_id);
        status = fbe_api_board_get_platform_info(object_id, &platform_info);
        total_cores = platform_info.hw_platform_info.processorCount;
    }

    fprintf(fp,"Timestamp"); //always print timestamp

    //LUN Read MB/s
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_RBLK)
    {
        for (core_i = 0; core_i < total_cores; core_i++)
        {
            fprintf(fp,",LUN Read Blocks/s(Core%d)", core_i);
        }
    }

    //LUN Write MB/s
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_WBLK)
    {
        for (core_i = 0; core_i < total_cores; core_i++) 
        {
            fprintf(fp,",LUN Write Blocks/s(Core%d)", core_i);
        }
    }

    //LUN Read IOPS
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_RIOPS)
    {
        for (core_i = 0; core_i < total_cores; core_i++) 
        {
            fprintf(fp,",LUN Read IOPS(Core%d)", core_i);
        }
    }

    //LUN Write IOPS
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_WIOPS)
    {
        for (core_i = 0; core_i < total_cores; core_i++) 
        {
            fprintf(fp,",LUN Write IOPS(Core%d)", core_i);
        }
    }

    //LUN Stripe Crossings
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_STPC)
    {
        for (core_i = 0; core_i < total_cores; core_i++) 
        {
            fprintf(fp,",LUN Stripe Crossings per second(Core%d)", core_i);
        }
    }

    //LUN Stripe Writes
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_MR3W) 
    {
        for (core_i = 0; core_i < total_cores; core_i++) 
        {
            fprintf(fp,",LUN Stripe writes per second(Core%d)", core_i);
        }
    }

    //LUN Read Size Histogram
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_RHIS)
    {
        for (core_i = 0; core_i < total_cores; core_i++) 
        {
            for (blk_count_i = 1; blk_count_i <= 1024; blk_count_i *=2)
            {
                fprintf(fp,",LUN %d Block Reads(Core%d)", blk_count_i, core_i);
            }
        }
    }

    //LUN Write Size Histogram
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_WHIS)
    {
        for (core_i = 0; core_i < total_cores; core_i++) 
        {
            for (blk_count_i = 1; blk_count_i <= 1024; blk_count_i *=2)
            { 
                fprintf(fp,",LUN %d Block Writes(Core%d)", blk_count_i, core_i);
            }
        }
    }

    //RG Read MB/s (per_disk)
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_RG_RBLK)
    {
        for (disk_i = 0; disk_i < FBE_XOR_MAX_FRUS; disk_i++)
        {
            for (core_i = 0; core_i < total_cores; core_i++) 
            {
                fprintf(fp,",RG Read Blocks/s[pos%d](Core%d)", disk_i, core_i);
            }
       }
    }
    //RG Write MB/s (per_disk)
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_RG_WBLK)
    {
        for (disk_i = 0; disk_i < FBE_XOR_MAX_FRUS; disk_i++)
        {
            for (core_i = 0; core_i < total_cores; core_i++) 
            {
                fprintf(fp,",RG Write Blocks/s[pos%d](Core%d)", disk_i, core_i);
            }
        }
    }
    //RG Read IOPS (per_disk)
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_RG_RIOPS)
    {
        for (disk_i = 0; disk_i < FBE_XOR_MAX_FRUS; disk_i++)
        {
            for (core_i = 0; core_i < total_cores; core_i++) 
            {
                fprintf(fp,",RG Read IOPs[pos%d](Core%d)", disk_i, core_i);
            }
        }
    }
    //RG Write IOPS (per_disk)
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_RG_WIOPS)
    {
        for (disk_i = 0; disk_i < FBE_XOR_MAX_FRUS; disk_i++)
        {
            for (core_i = 0; core_i < total_cores; core_i++) 
            {
                fprintf(fp,",RG Write IOPs[pos%d](Core%d)", disk_i, core_i);
            }
        }
    }
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_NZQA) 
    {
        for (core_i = 0; core_i < total_cores; core_i++) 
        {
            fprintf(fp,",LUN Nonzero Queue Arrivals(Core%d)", core_i);
        }
    }
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_SAQL) 
    {
        for (core_i = 0; core_i < total_cores; core_i++) 
        {
            fprintf(fp,",LUN Sum Arrival Queue Length(Core%d)", core_i);
        }
    }
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_CRRT) 
    {
        for (core_i = 0; core_i < total_cores; core_i++) 
        {
            fprintf(fp,",LUN Cumulative Read Response Time(Core%d)", core_i);
        }
    }
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_LUN_CWRT) 
    {
        for (core_i = 0; core_i < total_cores; core_i++) 
        {
            fprintf(fp,",LUN Cumulative Write Response Time(Core%d)", core_i);
        }
    }
    fprintf(fp,"\n");
    fflush(fp);
    return;
}

/******************************************
* end fbe_cli_lib_perfstats_print_sep_header()
******************************************/

/*!**************************************************************
*          fbe_cli_lib_perfstats_dump_summed_pp_stats()
****************************************************************
*
* @brief   Dump the summed counters from an fbe_pdo_performance_counters_t to the screen
* 
 *  @param    fp - pointer to a file handle
*  @param    stat_tags - bitmask of which counters we want to actually print to file
*              @see fbe_cli_perfstats_tags_t
*  @param    old_stats - pointer to the pdo performance counters from the last sample we took
*  @param    new_stats - pointer to the pdo performance counters from the latest sample we took
*
* @return  None.
*
* @revision:
*  06/08/2012 Created. Darren Insko
*
****************************************************************/
static void fbe_cli_lib_perfstats_dump_summed_pp_stats(fbe_pdo_performance_counters_t  *pdo_stats,
                                                       fbe_cli_perfstats_tags_t        stat_tags,
                                                       fbe_bool_t                      brief_mode)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_perfstats_pdo_sum_t     summed_stats;
    fbe_const_class_info_t      *class_info_p;
    fbe_job_service_bes_t       phy_disk = {FBE_PORT_NUMBER_INVALID, FBE_ENCLOSURE_NUMBER_INVALID, FBE_ENCLOSURE_SLOT_NUMBER_INVALID};

    fbe_api_perfstats_get_summed_pdo_stats(&summed_stats, pdo_stats);
   
    /* Obtain the physical drive's Class ID, Class Name, Port, Enclosure */
    /* and Slot numbers, which are used to uniquely identify it.         */
    status = fbe_cli_get_class_info(pdo_stats->object_id, FBE_PACKAGE_ID_PHYSICAL, &class_info_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("Failed to get the class info for this PDO object ID\n");
        return;
    }
   
    status = fbe_cli_get_bus_enclosure_slot_info(pdo_stats->object_id,
                                                 class_info_p->class_id,
                                                 &phy_disk.bus,
                                                 &phy_disk.enclosure,
                                                 &phy_disk.slot,
                                                 FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("Can't get bus, enclosure, slot information for PDO object ID: 0x%x, class ID: 0x%x, status: 0x%x\n",
                      pdo_stats->object_id, class_info_p->class_id, status);
        return;
    }

    fbe_cli_printf("=== Drive %d_%d_%d (ID:0x%x)  Class ID: 0x%x  Class Name: %s ===\n",
                   phy_disk.bus, phy_disk.enclosure, phy_disk.slot, pdo_stats->object_id, class_info_p->class_id, class_info_p->class_name);

    fbe_cli_printf("Disk Serial Number: %s\n", pdo_stats->serial_number);
    //for each tag, print out stats
    //always print timestamp
    fbe_cli_printf("Monitor Timestamp: %llu\n", pdo_stats->last_monitor_timestamp);
    fbe_cli_printf("Update Timestamp : %llu\n", pdo_stats->timestamp);
    //header
    fbe_cli_printf("                            |        -SUM-         \n");
    fbe_cli_printf("---------------------------------------------------\n");
    //Disk Blocks Read
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_RBLK) 
    {
        if (FBE_IS_FALSE(brief_mode) || summed_stats.disk_blocks_read)
        {
            fbe_cli_printf("PDO Blocks Read:            |%13llu\n", (unsigned long long)summed_stats.disk_blocks_read);
        }
    }

    //Disk blocks Written
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_WBLK) 
    {
        if (FBE_IS_FALSE(brief_mode) || summed_stats.disk_blocks_written)
        {
            fbe_cli_printf("PDO Blocks Written:         |%13llu\n", (unsigned long long)summed_stats.disk_blocks_written);
        }
    }

    //Disk blocks Seeked
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_SBLK) 
    {
        if (FBE_IS_FALSE(brief_mode) || summed_stats.sum_blocks_seeked)
        {
            fbe_cli_printf("PDO Blocks Seeked:         |%13llu\n", (unsigned long long)summed_stats.sum_blocks_seeked);
        }
    }

    //Disk Read IOs
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_RIOPS)
    {
        if (FBE_IS_FALSE(brief_mode) || summed_stats.disk_reads)
        {
            fbe_cli_printf("PDO Read IO Count:        |%13llu\n", (unsigned long long)summed_stats.disk_reads);
        }
    }

    //Disk Read IOs
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_WIOPS)
    {
        if (FBE_IS_FALSE(brief_mode) || summed_stats.disk_writes)
        {
            fbe_cli_printf("PDO Write IO Count:       |%llu\n", (unsigned long long)summed_stats.disk_writes);
        }
    }

    //Busy Ticks, Idle tiks, utilization
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_UTIL)
    {
        if (FBE_IS_FALSE(brief_mode) || summed_stats.busy_ticks)
        {
            fbe_cli_printf("PDO Busy Ticks:       |%llu\n", (unsigned long long)summed_stats.busy_ticks);
        }

        if (FBE_IS_FALSE(brief_mode) || summed_stats.idle_ticks)
        {
            fbe_cli_printf("PDO Idle Ticks:       |%llu\n", (unsigned long long)summed_stats.idle_ticks);
        }
        if (FBE_IS_FALSE(brief_mode) || summed_stats.busy_ticks)
        {
            fbe_cli_printf("PDO Utilization:      |%llu", (unsigned long long)((summed_stats.busy_ticks*100)/(summed_stats.busy_ticks+summed_stats.idle_ticks)));
        }
        
    }

    //Response Time Histogram
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_THIS)
    {
        fbe_u32_t histogram_bucket_i;
        if (FBE_IS_FALSE(brief_mode) || summed_stats.disk_srv_time_histogram[0])
        {
            fbe_cli_printf("\nGTE %8dus PDO Service Time:       |%llu", 0, summed_stats.disk_srv_time_histogram[0]);  
        }
        for (histogram_bucket_i = 1; histogram_bucket_i < FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_LAST; histogram_bucket_i++) 
        {
            if (FBE_IS_FALSE(brief_mode) || summed_stats.disk_srv_time_histogram[histogram_bucket_i])
            {
                fbe_cli_printf("\nGTE %8dus PDO Service Time:       |%llu", (1 << (histogram_bucket_i - 1)) *100, summed_stats.disk_srv_time_histogram[histogram_bucket_i]);  
            }
        } 
    }

    fbe_cli_printf("\n");
    return;
}
/**************************************************
* end fbe_cli_lib_perfstats_dump_summed_pp_stats()
**************************************************/

/*!**************************************************************
*          fbe_cli_lib_perfstats_dump_per_core_pp_stats()
****************************************************************
*
* @brief   Dump the summed counters from an fbe_pdo_performance_counters_t to the screen
* 
 *  @param    fp - pointer to a file handle
*  @param    stat_tags - bitmask of which counters we want to actually print to file
*              @see fbe_cli_perfstats_tags_t
*  @param    old_stats - pointer to the pdo performance counters from the last sample we took
*  @param    new_stats - pointer to the pdo performance counters from the latest sample we took
*
* @return  None.
*
* @revision:
*  06/08/2012 Created. Darren Insko
*
****************************************************************/
static void fbe_cli_lib_perfstats_dump_per_core_pp_stats(fbe_pdo_performance_counters_t *pdo_stats,
                                                         fbe_cli_perfstats_tags_t stat_tags,
                                                         fbe_bool_t                brief_mode)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_const_class_info_t      *class_info_p;
    fbe_port_number_t           port = FBE_PORT_NUMBER_INVALID;
    fbe_enclosure_number_t      enclosure = FBE_ENCLOSURE_NUMBER_INVALID;
    fbe_enclosure_slot_number_t slot = FBE_ENCLOSURE_SLOT_NUMBER_INVALID;
    fbe_cpu_id_t                core_i;
    fbe_u32_t                   total_cores;
    fbe_u64_t                   intermediate_sum = 0;
    fbe_u64_t                   busy_ticks = 0;
    fbe_u64_t                   idle_ticks = 0;
    fbe_u32_t                   histogram_bucket_i = 0;
    char                        line_buffer[512];
    char                        token_buffer[32];

    fbe_object_id_t object_id;
    fbe_board_mgmt_platform_info_t platform_info;

    fbe_api_get_board_object_id(&object_id);
    fbe_api_board_get_platform_info(object_id, &platform_info);
    if (fbe_cli_is_simulation())
    {
        total_cores = 6; //use six cores for simulation, doesn't really matter
    }
    else
    {
        total_cores = platform_info.hw_platform_info.processorCount;
    }

    /* Obtain the physical drive's Class ID, Class Name, Port, Enclosure */
    /* and Slot numbers, which are used to uniquely identify it.         */
    status = fbe_cli_get_class_info(pdo_stats->object_id, FBE_PACKAGE_ID_PHYSICAL, &class_info_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("Failed to get the class info for this PDO object ID\n");
        return;
    }
   
    status = fbe_cli_get_bus_enclosure_slot_info(pdo_stats->object_id,
                                                 class_info_p->class_id,
                                                 &port,
                                                 &enclosure,
                                                 &slot,
                                                 FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("Can't get port, enclosure, slot information for PDO object ID: 0x%x, class ID: 0x%x, status: 0x%x\n",
                      pdo_stats->object_id, class_info_p->class_id, status);
        return;
    }

    fbe_cli_printf("=== Drive %d_%d_%d (ID:0x%x)  Class ID: 0x%x  Class Name: %s ===\n",
                   port, enclosure, slot, pdo_stats->object_id, class_info_p->class_id, class_info_p->class_name);

    //for each tag, print out stats
    fbe_cli_printf("Monitor Timestamp: %llu\n", pdo_stats->last_monitor_timestamp);
    fbe_cli_printf("Update Timestamp : %llu\n", pdo_stats->timestamp);
    //header       
    fbe_cli_printf("                                       |");
    for (core_i = 0; core_i < total_cores; core_i++)
    {
        fbe_cli_printf("  Core%02d  |", core_i);
    }
    fbe_cli_printf("\n----------------------------------------");
    for (core_i = 0; core_i < total_cores; core_i++)
    {
        fbe_cli_printf("-----------");
    }

    //Disk Blocks Read
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_RBLK) 
    {
        intermediate_sum = 0;
        line_buffer[0] = '\0';
        for (core_i = 0; core_i < total_cores; core_i++)
        {
            sprintf(token_buffer, "%10lu|", (unsigned long)pdo_stats->disk_blocks_read[core_i]);
            intermediate_sum += pdo_stats->disk_blocks_read[core_i];
            strcat(line_buffer, token_buffer);
        }
        if (FBE_IS_FALSE(brief_mode) || intermediate_sum) 
        {
            fbe_cli_printf("\nPDO Blocks Read:                       |%s", line_buffer);
        }
    }

    //Disk Blocks Written
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_WBLK) 
    {
        intermediate_sum = 0;
        line_buffer[0] = '\0';
        for (core_i = 0; core_i < total_cores; core_i++)
        {
            sprintf(token_buffer, "%10lu|", (unsigned long)pdo_stats->disk_blocks_written[core_i]);
            intermediate_sum += pdo_stats->disk_blocks_written[core_i];
            strcat(line_buffer, token_buffer);
        }
        if (FBE_IS_FALSE(brief_mode) || intermediate_sum) 
        {
            fbe_cli_printf("\nPDO Blocks Written:                    |%s", line_buffer);
        }
    }

    //Disk Blocks Seeked
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_SBLK) 
    {
        intermediate_sum = 0;
        line_buffer[0] = '\0';
        for (core_i = 0; core_i < total_cores; core_i++)
        {
            sprintf(token_buffer, "%10lu|", (unsigned long)pdo_stats->sum_blocks_seeked[core_i]);
            intermediate_sum += pdo_stats->sum_blocks_seeked[core_i];
            strcat(line_buffer, token_buffer);
        }
        if (FBE_IS_FALSE(brief_mode) || intermediate_sum) 
        {
            fbe_cli_printf("\nPDO Blocks Seeked:                     |%s", line_buffer);
        }
    }

    //Disk Read IOs
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_RIOPS)
    {
        intermediate_sum = 0;
        line_buffer[0] = '\0';
        for (core_i = 0; core_i < total_cores; core_i++)
        {
            sprintf(token_buffer, "%10lu|", (unsigned long)pdo_stats->disk_reads[core_i]);
            intermediate_sum += pdo_stats->disk_reads[core_i];
            strcat(line_buffer, token_buffer);
        }
        if (FBE_IS_FALSE(brief_mode) || intermediate_sum) 
        {
            fbe_cli_printf("\nPDO Reads:                             |%s", line_buffer);
        }
    }

    //Disk Write IOs
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_WIOPS)
    {
        intermediate_sum = 0;
        line_buffer[0] = '\0';
        for (core_i = 0; core_i < total_cores; core_i++)
        {
            sprintf(token_buffer, "%10lu|", (unsigned long)pdo_stats->disk_writes[core_i]);
            intermediate_sum += pdo_stats->disk_writes[core_i];
            strcat(line_buffer, token_buffer);
        }
        if (FBE_IS_FALSE(brief_mode) || intermediate_sum) 
        {
            fbe_cli_printf("\nPDO Writes:                            |%s", line_buffer);
        }
    }

    //Disk Utilization = busy_ticks/(busy_ticks + idle_ticks)
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_UTIL)
    {
        //actual utilization, then raw ticks
        intermediate_sum = 0;
        line_buffer[0] = '\0';
        for (core_i = 0; core_i < total_cores; core_i++)
        {
            busy_ticks = pdo_stats->busy_ticks[core_i];
            idle_ticks = pdo_stats->idle_ticks[core_i];
            if (busy_ticks != 0)
            {
                sprintf(token_buffer, "%10llu|", busy_ticks/(busy_ticks + idle_ticks));
                intermediate_sum += busy_ticks/(busy_ticks + idle_ticks);
            }
            else
            {
                sprintf(token_buffer, "%10llu|", busy_ticks);
            }
            strcat(line_buffer, token_buffer);
        }
        if (FBE_IS_FALSE(brief_mode) || intermediate_sum) 
        {
            fbe_cli_printf("\nPDO Utilization:                       |%s", line_buffer);
        }

        intermediate_sum = 0;
        line_buffer[0] = '\0';
        for (core_i = 0; core_i < total_cores; core_i++)
        {
            sprintf(token_buffer, "%20llu|", (unsigned long long)pdo_stats->busy_ticks[core_i]);
            intermediate_sum += pdo_stats->busy_ticks[core_i];
            strcat(line_buffer, token_buffer);
        }
        if (FBE_IS_FALSE(brief_mode) || intermediate_sum) 
        {
            fbe_cli_printf("\nbusy ticks:                            |%s", line_buffer);
        }

        intermediate_sum = 0;
        line_buffer[0] = '\0';
        for (core_i = 0; core_i < total_cores; core_i++)
        {
            sprintf(token_buffer, "%20llu|", (unsigned long long)pdo_stats->idle_ticks[core_i]);
            intermediate_sum += pdo_stats->idle_ticks[core_i];
            strcat(line_buffer, token_buffer);
        }
        if (FBE_IS_FALSE(brief_mode) || intermediate_sum) 
        {
            fbe_cli_printf("\nidle ticks:                            |%s", line_buffer);
        }
    }

    //Disk Service Time Histogram
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_THIS)
    {
        for (histogram_bucket_i = 0; histogram_bucket_i < FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_LAST; histogram_bucket_i++) 
        {
            intermediate_sum = 0;
            line_buffer[0] = '\0';
            for (core_i = 0; core_i < total_cores; core_i++)
            {
                sprintf(token_buffer, "%10lu|", (unsigned long)pdo_stats->disk_srv_time_histogram[histogram_bucket_i][core_i]);
                intermediate_sum += pdo_stats->disk_srv_time_histogram[histogram_bucket_i][core_i];
                strcat(line_buffer, token_buffer);
            }
            if (FBE_IS_FALSE(brief_mode) || intermediate_sum)
            {
                if (histogram_bucket_i == 0)
                {
                    fbe_cli_printf("\nGTE %8dus PDO Service Time:       |%s", histogram_bucket_i, line_buffer);
                }
                else
                {
                   fbe_cli_printf("\nGTE %8dus PDO Service Time:       |%s", (1 << (histogram_bucket_i - 1))*100, line_buffer);
                }
            }
        } 
    }

    fbe_cli_printf("\n\n\n");
    return;
}
/****************************************************
* end fbe_cli_lib_perfstats_dump_per_core_pp_stats()
****************************************************/

/*!**************************************************************
*          fbe_cli_lib_perfstats_print_summed_pp_stats()
****************************************************************
*
* @brief   Print the summed counters from an fbe_pdo_performance_counters_t to a file
* 
 *  @param    fp - pointer to a file handle
*  @param    stat_tags - bitmask of which counters we want to actually print to file
*              @see fbe_cli_perfstats_tags_t
*  @param    old_stats - pointer to the pdo performance counters from the last sample we took
*  @param    new_stats - pointer to the pdo performance counters from the latest sample we took
*
* @return  None.
*
* @revision:
*  06/08/2012 Created. Darren Insko
*
****************************************************************/
static void fbe_cli_lib_perfstats_print_summed_pp_stats(FILE *fp,
                                                        fbe_cli_perfstats_tags_t stat_tags,
                                                        fbe_pdo_performance_counters_t *old_stats,
                                                        fbe_pdo_performance_counters_t *new_stats)
{
    fbe_perfstats_pdo_sum_t old_sum;
    fbe_perfstats_pdo_sum_t new_sum;
    fbe_u64_t               time_difference = 0;

    fbe_api_perfstats_get_summed_pdo_stats(&old_sum, old_stats);
    fbe_api_perfstats_get_summed_pdo_stats(&new_sum, new_stats);


    time_difference = (new_sum.timestamp - old_sum.timestamp)/1000000;
    
    if (time_difference) { //otherwise counters haven't been updated
        fprintf(fp,"%lu,", (unsigned long)new_sum.timestamp); //always print timestamp
        //PDO Blocks Read/sec
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_RBLK) 
        {
            fprintf(fp,"%llu,", (new_sum.disk_blocks_read - old_sum.disk_blocks_read)/time_difference);
        }

        //PDO Blocks Written/sec
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_WBLK)
        {
            fprintf(fp,"%llu,", (new_sum.disk_blocks_written - old_sum.disk_blocks_written)/time_difference); 
        }

        //PDO Blocks Seeked/sec
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_SBLK)
        {
            fprintf(fp,"%llu,", (new_sum.sum_blocks_seeked - old_sum.sum_blocks_seeked)/time_difference); 
        }

        //PDO Read IOPS
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_RIOPS)
        {
            fprintf(fp,"%llu,", (new_sum.disk_reads - old_sum.disk_reads)/time_difference);
        }

        //PDO Write IOPS
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_WIOPS)
        {
            fprintf(fp,"%llu,", (new_sum.disk_writes - old_sum.disk_writes)/time_difference);
        }

        //PDO Utilization
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_UTIL)
        {
            fprintf(fp,"%llu,", new_sum.busy_ticks/(new_sum.busy_ticks + new_sum.idle_ticks));
        }

        fprintf(fp, "\n");
        fflush(fp);
    }

   return;
}
/****************************************************
* end fbe_cli_lib_perfstats_print_summed_pp_stats()
***************************************************/

/*!**************************************************************
*          fbe_cli_lib_perfstats_print_per_core_pp_stats()
****************************************************************
*
* @brief   Print the per-core counters from an fbe_pdo_performance_counters_t to a file
* 
 *  @param    fp - pointer to a file handle
*  @param    stat_tags - bitmask of which counters we want to actually print to file
*              @see fbe_cli_perfstats_tags_t
*  @param    old_stats - pointer to the pdo performance counters from the last sample we took
*  @param    new_stats - pointer to the pdo performance counters from the latest sample we took
*
* @return  None.
*
* @revision:
*  06/08/2012 Created. Darren Insko
*
****************************************************************/
static void fbe_cli_lib_perfstats_print_per_core_pp_stats(FILE *fp,
                                                           fbe_cli_perfstats_tags_t stat_tags,
                                                           fbe_pdo_performance_counters_t *old_stats,
                                                           fbe_pdo_performance_counters_t *new_stats)
{
    fbe_status_t                   status;
    fbe_object_id_t                object_id;
    fbe_board_mgmt_platform_info_t platform_info;
    fbe_u32_t                      total_cores;
    fbe_u64_t                      time_difference = 0;
    fbe_cpu_id_t                   core_i = 0;
    fbe_u64_t                      busy_ticks = 0;
    fbe_u64_t                      idle_ticks = 0;
    fbe_u32_t                      histogram_bucket_i = 0;

    status = fbe_api_get_board_object_id(&object_id);
    status = fbe_api_board_get_platform_info(object_id, &platform_info);
    total_cores = platform_info.hw_platform_info.processorCount;

    time_difference = (new_stats->timestamp - old_stats->timestamp) / 1000000;

    if (time_difference) 
    {
        fprintf(fp,"%lu,", (unsigned long)new_stats->timestamp); //always print timestamp
        //PDO Blocks Read per sec
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_RBLK)
        {
            for (core_i = 0; core_i < total_cores; core_i++) 
            {
                fprintf(fp,"%llu,", (new_stats->disk_blocks_read[core_i] - old_stats->disk_blocks_read[core_i])/time_difference);
            }
        }

        //PDO Blocks Written/sec
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_WBLK)
        {
            for (core_i = 0; core_i < total_cores; core_i++) 
            {
                fprintf(fp,"%llu,", (new_stats->disk_blocks_written[core_i] - old_stats->disk_blocks_written[core_i])/time_difference);
            }
        }

        //PDO Blocks Seeked/sec
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_SBLK)
        {
            for (core_i = 0; core_i < total_cores; core_i++) 
            {
                fprintf(fp,"%llu,", (new_stats->sum_blocks_seeked[core_i] - old_stats->sum_blocks_seeked[core_i])/time_difference);
            }
        }

        //PDO Read IOPS
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_RIOPS)
        {
            for (core_i = 0; core_i < total_cores; core_i++) 
            {
                fprintf(fp,"%llu,", (new_stats->disk_reads[core_i] - old_stats->disk_reads[core_i])/time_difference);
            }
        }

        //PDO Write IOPS
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_WIOPS)
        {
            for (core_i = 0; core_i < total_cores; core_i++) 
            {
                fprintf(fp,"%llu,", (new_stats->disk_writes[core_i] - old_stats->disk_writes[core_i])/time_difference);
            }
        }

        //PDO Utilization
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_UTIL)
        {
            for (core_i = 0; core_i < total_cores; core_i++) 
            {
                busy_ticks = new_stats->busy_ticks[core_i];
               idle_ticks = new_stats->idle_ticks[core_i];
                if (busy_ticks != 0)
                {
                    fprintf(fp,"%llu,", busy_ticks/(busy_ticks + idle_ticks));
                }
                else
                {
                    fprintf(fp,"%llu,", busy_ticks);
                }
            }
        }

        //Disk Service Time Histogram
        if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_THIS)
        {
            for (histogram_bucket_i = 0; histogram_bucket_i < FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_LAST; histogram_bucket_i++)
            {
                for (core_i = 0; core_i < total_cores; core_i++)
                {
                    fprintf(fp,"%llu,", (new_stats->disk_srv_time_histogram[histogram_bucket_i][core_i] - old_stats->disk_srv_time_histogram[histogram_bucket_i][core_i])/time_difference);
                }
            }
        }

        fprintf(fp, "\n");
        fflush(fp);
    }

    return;
}
/***************************************************
* end fbe_cli_lib_perfstats_print_per_core_pp_stats()
***************************************************/

/*!**************************************************************
*          fbe_cli_lib_perfstats_print_pp_header()
****************************************************************
*
* @brief   Print the per-core counters from an fbe_pdo_performance_counters_t to a file
* 
 *  @param    fp - pointer to a file handle
*  @param    stat_tags - bitmask of which counters we want to actually print to file
*              @see fbe_cli_perfstats_tags_t
*  @param    summed - FBE_TRUE if using summed stats, FBE_FALSE if using per-core stats
*
* @return  None.
*
* @revision:
*  06/08/2012 Created. Darren Insko
*
****************************************************************/

static void fbe_cli_lib_perfstats_print_pp_header(FILE *fp,
                                                   fbe_cli_perfstats_tags_t stat_tags,
                                                   fbe_bool_t   summed)
{
    fbe_status_t                   status;
    fbe_object_id_t                object_id;
    fbe_board_mgmt_platform_info_t platform_info;
    fbe_u32_t                      total_cores;
    fbe_u32_t                      core_i;
    fbe_u32_t                      histogram_bucket_i = 0;

    if (summed)
    {
        total_cores = 1; 
    }
    else
    {   
        status = fbe_api_get_board_object_id(&object_id);
        status = fbe_api_board_get_platform_info(object_id, &platform_info);
        total_cores = platform_info.hw_platform_info.processorCount;
    }

    fprintf(fp,"Timestamp"); //always print timestamp

    //PDO Blocks Read/sec
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_RBLK)
    {
        for (core_i = 0; core_i < total_cores; core_i++)
        {
            fprintf(fp,",PDO Blocks Read/sec (Core%d)", core_i);
        }
    }

    //PDO Blocks Written/sec
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_WBLK)
    {
       for (core_i = 0; core_i < total_cores; core_i++) 
        {
            fprintf(fp,",PDO Blocks Written/sec (Core%d)", core_i);
        }
    }

    //PDO Blocks Seeked/sec
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_SBLK)
    {
        for (core_i = 0; core_i < total_cores; core_i++) 
        {
            fprintf(fp,",PDO Blocks Seeked/sec (Core%d)", core_i);
        }
    }

    //PDO Read IOPS
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_RIOPS)
    {
        for (core_i = 0; core_i < total_cores; core_i++) 
        {
            fprintf(fp,",PDO Read IOPS (Core%d)", core_i);
        }
    }

    //PDO Write IOPS
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_WIOPS)
    {
        for (core_i = 0; core_i < total_cores; core_i++) 
        {
            fprintf(fp,",PDO Write IOPS (Core%d)", core_i);
        }
    }

    //PDO Utilization
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_UTIL)
    {
        for (core_i = 0; core_i < total_cores; core_i++) 
        {
            fprintf(fp,",PDO Utilization (Core%d)", core_i);
        }
    }

    //Disk Service Time Histogram
    if (stat_tags & FBE_CLI_PERFSTATS_TAG_PDO_THIS)
    {
        for (histogram_bucket_i = 0; histogram_bucket_i < FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_LAST; histogram_bucket_i++)
        {
            for (core_i = 0; core_i < total_cores; core_i++)
            {
                if (histogram_bucket_i == 0)
                {
                    fprintf(fp,",GTE %dus PDO Service Time (Core%d)", histogram_bucket_i, core_i);
                }
                else
                {
                    fprintf(fp,",GTE %dus PDO Service Time (Core%d)", (1 << (histogram_bucket_i - 1))*100, core_i);
                }
            }
        } 
    }

    fprintf(fp,"\n");
    fflush(fp);
    return;
}
/*********************************************
* end fbe_cli_lib_perfstats_print_pp_header()
*********************************************/

/*************************
* end file fbe_cli_lib_perf_stats_cmds.c
*************************/
