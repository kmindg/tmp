 /***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
* @file fbe_cli_lib_rderr_cmds.c
 ***************************************************************************
 *
 * @brief
*  This file contains cli functions for the RDERR related features in FBE CLI.
 *
 * @ingroup fbe_cli
 *
 * @date
 *  07/01/2010 - Created. Swati Fursule
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
#include <stdlib.h>
#include "fbe_cli_private.h"
#include "fbe_trace.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_api_logical_error_injection_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_trace.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe_cli_rderr.h"
#include "fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_database_interface.h"


void fbe_cli_rderr_parse_cmds(int argc, char** argv);

fbe_status_t fbe_cli_rderr_usr_intf_add(
                        fbe_api_logical_error_injection_record_t* record_p);

static void fbe_cli_rderr_usr_intf_add_ex(void);

void fbe_cli_rderr_get_params_modify_delete_records(int argc, char** argv,
                                            fbe_u32_t* start_p,
                                            fbe_u32_t* end_p);
void fbe_cli_rderr_records_operation(fbe_u32_t start, 
                                     fbe_u32_t end,
                                     fbe_bool_t b_create,
                                     fbe_bool_t b_modify,
                                     fbe_bool_t b_delete);
void fbe_cli_rderr_modify_mode(fbe_u32_t start, 
                               fbe_u32_t end,
                               fbe_api_logical_error_injection_mode_t mode);
void fbe_cli_rderr_modify_errtype(fbe_u32_t start, 
                               fbe_u32_t end,
                               fbe_api_logical_error_injection_type_t mode);
void fbe_cli_rderr_modify_gap(fbe_u32_t start, 
                              fbe_u32_t end,
                              fbe_lba_t gap_in_blocks);
void fbe_cli_rderr_modify_pos_bitmask(fbe_u32_t start, 
                               fbe_u32_t end,
                               fbe_u16_t pos_bitmask);


fbe_status_t fbe_cli_rderr_get_params_enable_disable_rderr(int argc, char** argv,
                                                   fbe_bool_t* b_enable_rderr_p,
                                                   fbe_object_id_t* object_id_p,
                                                   fbe_package_id_t* package_id_p,
                                                   fbe_class_id_t* class_id_p);
void fbe_cli_rderr_enable_disable_rderr(fbe_bool_t b_enable,
                                        fbe_object_id_t object_id,
                                        fbe_package_id_t package_id,
                                        fbe_class_id_t class_id);
void fbe_cli_rderr_switch_table(int argc, char** argv);
void fbe_cli_rderr_get_tables_info(void);
void fbe_cli_rderr_get_records(void);
void fbe_cli_rderr_get_statistics(int argc, char** argv);
/**************************************************
 * GLOBALS
 **************************************************/
const fbe_char_t *fbe_cli_rderr_mode_strings[FBE_LOGICAL_ERROR_INJECTION_MODE_LAST] =
{
    "INV",                      /* FBE_LOGICAL_ERROR_INJECTION_MODE_INVALID */
    "CNT",                      /* FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT */
    "ALW",                      /* FBE_LOGICAL_ERROR_INJECTION_MODE_ALWAYS */
    "RND",                      /* FBE_LOGICAL_ERROR_INJECTION_MODE_RANDOM */
    "SKP",                      /* FBE_LOGICAL_ERROR_INJECTION_MODE_SKIP  */
    "SKI",                      /* FBE_LOGICAL_ERROR_INJECTION_MODE_SKIP_INSERT  */
    "TNS",                      /* FBE_LOGICAL_ERROR_INJECTION_MODE_TRANS */
    "TNR",                      /* FBE_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED */
    "URM",                      /* FBE_LOGICAL_ERROR_INJECTION_MODE_TRANS */
    "SLB",                      /* FBE_LOGICAL_ERROR_INJECTION_MODE_INJECT_SAME_LBA */
    "UNK",
};

/******************************
 * fbe_cli_rderr_err_type_strings
 ******************************
 * There is one string per type of error in the
 * FBE_XOR_ERR_TYPE_MAX structure.  
 *
 ******************************/
const fbe_char_t *fbe_cli_rderr_err_type_strings[FBE_XOR_ERR_TYPE_MAX] =
{
    "NONE",     /* FBE_XOR_ERR_TYPE_NONE = 0,       0x00 */
    "SFT_MD",   /* FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR, 0x01 */
    "HRD_MD",   /* FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR, 0x02 */
    "RND_MD",   /* FBE_XOR_ERR_TYPE_RND_MEDIA_ERR,  0x03 */
    "CRC",      /* FBE_XOR_ERR_TYPE_CRC,            0x04 */
    "KL_CRC",   /* FBE_XOR_ERR_TYPE_KLOND_CRC,      0x05 */
    "DH_CRC",   /* FBE_XOR_ERR_TYPE_DH_CRC,         0x06 */
    "RD_CRC",   /* FBE_XOR_ERR_TYPE_RAID_CRC,       0x07 */
    "CR_CRC",   /* FBE_XOR_ERR_TYPE_CORRUPT_CRC,    0x08 */
    "WS",       /* FBE_XOR_ERR_TYPE_WS,             0x09 */
    "TS",       /* FBE_XOR_ERR_TYPE_TS,             0x0A */
    "SS",       /* FBE_XOR_ERR_TYPE_SS,             0x0B */
    "BWS",      /* FBE_XOR_ERR_TYPE_BOGUS_WS,       0x0C */
    "BTS",      /* FBE_XOR_ERR_TYPE_BOGUS_TS,       0x0D */
    "BSS",      /* FBE_XOR_ERR_TYPE_BOGUS_SS,       0x0E */
    "1NS",      /* FBE_XOR_ERR_TYPE_1NS,            0x0F */
    "1S",       /* FBE_XOR_ERR_TYPE_1S,             0x10 */
    "1R",       /* FBE_XOR_ERR_TYPE_1R,             0x11 */
    "1D",       /* FBE_XOR_ERR_TYPE_1D,             0x12 */
    "1COD",     /* FBE_XOR_ERR_TYPE_1COD,           0x13 */
    "1COP",     /* FBE_XOR_ERR_TYPE_1COP,           0x14 */
    "1POC",     /* FBE_XOR_ERR_TYPE_1POC,           0x15 */
    "COH",      /* FBE_XOR_ERR_TYPE_COH,            0x16 */
    "COR_DA",   /* FBE_XOR_ERR_TYPE_CORRUPT_DATA,   0x17 */
    "NPOC",     /* FBE_XOR_ERR_TYPE_N_POC_COH,      0x18 */
    "POC_CO",   /* FBE_XOR_ERR_TYPE_POC_COH,        0x19 */
    "UKN_CO",   /* FBE_XOR_ERR_TYPE_COH_UNKNOWN,    0x1A */
    "RB_ERR",   /* FBE_XOR_ERR_TYPE_RB_FAILED,      0x1B */
    "LBA",      /* FBE_XOR_ERR_TYPE_LBA_STAMP,      0x1C */
    "SIG_BIT",    /* FBE_XOR_ERR_TYPE_SINGLE_BIT_CRC  0x1D */ /* Single bit CRC error injection not supported in rderr */
    "MULT_BIT",  /* FBE_XOR_ERR_TYPE_MULTI_BIT_CRC   0x1E */ /* Multi bit CRC error injection not supported in rderr */
    "TIMEOUT",      /* FBE_XOR_ERR_TYPE_TIMEOUT_ERR,      0x1F */
    "COR_CRC",    /* FBE_XOR_ERR_TYPE_CORRUPT_CRC_INJECTED  0x20 */ 
    "COR_DAT",  /* FBE_XOR_ERR_TYPE_CORRUPT_DATA_INJECTED   0x21 */ 

    "BAD_MAGIC",  /* 0x22 */
    "BAD_SEQ",    /* 0x23 */
    "SIL_DROP",  /* FBE_XOR_ERR_TYPE_SILENT_DROP   0x24 */ 
    "INVALIDATED",               /* 0x25 */
    "BAD_CRC",                   /* 0x26 */
    "DELAY_DOWN",                /* 0x27 */
    "DELAY_UP",                  /* 0x28 */
    "COPY_CRC",                  /* 0x29 */
    "PVD_MD",              /* 0x2A */
    "IO_UNEXP",             /* 0x2B*/
    "INCMPLTE_WR",          /* 0x2C*/
    "UNKN",     /* FBE_XOR_ERR_TYPE_UNKNOWN,        0x23 */                                        
    /* FBE_XOR_ERR_TYPE_MAX,                        0x24 */

};

#define FBE_CLI_LEI_FIELDS_PER_LINE 6
static fbe_bool_t b_use_simulation_table = FBE_FALSE;

/*!*******************************************************************
 * @var fbe_cli_rderr
 *********************************************************************
 * @brief Function to implement binding of LUNs.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  6/11/2010 - Created. Swati Fursule
 *********************************************************************/
void fbe_cli_rderr(int argc, char** argv)
{
    fbe_cli_printf("%s", "\n");
    fbe_cli_rderr_parse_cmds(argc, argv);

    return;

}
/******************************************
 * end fbe_cli_rderr()
 ******************************************/


/*!**************************************************************
 *          fbe_cli_rderr_parse_cmds()
 ****************************************************************
 *
 * @brief   Parse rderr commands
 * rderr [-h,-r,-d,-I,-t,-m,-e,-mode,-errtype,-gap,-pos, -ena, -enable, 
 * -disable, -enable_for_rg, -disable_for_rg –enable_for_object, 
 * -disable_for_object, -stats, -stats_for_rg, -stats_for_object] 
 * <recnum| rg number | all>
 * @param   argument count
*  @param arguments list
 *
 * @return  None.
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rderr_parse_cmds(int argc, char** argv)
{
    /*********************************
     **    VARIABLE DECLARATIONS    **
     *********************************/
    fbe_status_t        status;
    if (argc == 0)
    {
        fbe_api_logical_error_injection_get_stats_t   stats;
        status = fbe_api_logical_error_injection_get_stats(&stats);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rderr: ERROR: Not able to get statistics %d\n", 
                           status);
            return;
        }
        fbe_cli_printf("rderr Status               %s\n", 
               (stats.b_enabled ? "ENABLED" : "DISABLED"));
        return;
    }
    /* 
     * Parse the rderr commands and call particular functions
     */
    if ((strcmp(*argv, "-help") == 0) ||
        (strcmp(*argv, "-h") == 0))
    {
        /* If they are asking for help, just display help and exit.
        */
        fbe_cli_printf("%s", RDERR_CMD_USAGE);
        fbe_cli_printf("%s", "\n");
        return;
    }

    /*
     * !TODO Send rderr command to ktrace output
     *
    fcli_cmd_to_ktrace(argc, argv);*/

    /* Parse args
     */
    while (argc && **argv == '-')
    {
        argc--;

        if ( (!strcmp(*argv, "-d")) ||
             (!strcmp(*argv, "-display"))
            )
        {
            fbe_cli_rderr_get_records();
            return;
        }
        else if ( (!strcmp(*argv, "-i")) ||
                  (!strcmp(*argv, "-info"))
                )
        {
            fbe_cli_rderr_get_tables_info();
            return;
        }
        else if ( (!strcmp(*argv, "-t")) ||
                  (!strcmp(*argv, "-table"))
                )
        {
            fbe_cli_rderr_switch_table(argc, argv);
            return;
        }
        else if ( (!strcmp(*argv, "-c")) ||
                  (!strcmp(*argv, "-create"))
                )
        {
            fbe_u32_t index = 0; /*not used for create operation*/
            fbe_cli_rderr_records_operation(index, index,
                                            FBE_TRUE, /* create records*/ 
                                            FBE_FALSE, /* modify records*/ 
                                            FBE_FALSE/* delete records*/);
            return;
        }
        else if ( (!strcmp(*argv, "-a")) ||
                  (!strcmp(*argv, "-add")) )
        {
            fbe_cli_rderr_usr_intf_add_ex();
            return;
        }
        else if ( (!strcmp(*argv, "-hardware_tables")) ||
                  (!strcmp(*argv, "-hw"))
                )
        {
            b_use_simulation_table = FBE_FALSE;
            return;
        }
        else if ( (!strcmp(*argv, "-simulation_tables")) ||
                  (!strcmp(*argv, "-sim"))
                )
        {
            b_use_simulation_table = FBE_TRUE;
            return;
        }
        else if ( (!strcmp(*argv, "-m") ) ||
                  (!strcmp(*argv, "-modify"))
                )
        {
            fbe_u32_t start = 0, end = 0;
            fbe_cli_rderr_get_params_modify_delete_records(argc, argv, &start, &end);
            fbe_cli_rderr_records_operation(start, end, 
                                            FBE_FALSE, /* create records*/ 
                                            FBE_TRUE, /* modify records*/ 
                                            FBE_FALSE/* delete records*/);
            return;
        }
        else if ( (!strcmp(*argv, "-mode") )
                )
        {
            fbe_u32_t start = 0, end = 0;
            fbe_api_logical_error_injection_mode_t mode;
            argc--;
            argv++;
            if (argc <= 0)
            {
                fbe_cli_printf("rderr: ERROR: Argc %x\n", argc);
                fbe_cli_printf("rderr: ERROR: Too less args.\n");
                fbe_cli_printf(RDERR_CMD_USAGE);
                fbe_cli_printf("%s", "\n");
                return;
            }

            /* get the mode value*/
            mode = fbe_api_lei_string_to_enum(*argv, 
                                              fbe_cli_rderr_mode_strings, 
                                              FBE_LOGICAL_ERROR_INJECTION_MODE_LAST);
            /* get the start and end records value*/
            fbe_cli_rderr_get_params_modify_delete_records(argc, argv, &start, &end);
            fbe_cli_rderr_modify_mode(start, end, mode);
            return;
        }
        else if ( (!strcmp(*argv, "-errtype") )
                )
        {
            fbe_u32_t start = 0, end = 0;
            fbe_api_logical_error_injection_type_t errtype;
            argc--;
            argv++;
            if (argc <= 0)
            {
                fbe_cli_printf("rderr: ERROR: Argc %x\n", argc);
                fbe_cli_printf("rderr: ERROR: Too less args.\n");
                fbe_cli_printf(RDERR_CMD_USAGE);
                fbe_cli_printf("%s", "\n");
                return;
            }

            /* get the mode value*/
            errtype = fbe_api_lei_string_to_enum(*argv, 
                                              fbe_cli_rderr_err_type_strings, 
                                              FBE_XOR_ERR_TYPE_MAX);
            /* get the start and end records value*/
            fbe_cli_rderr_get_params_modify_delete_records(argc, argv, &start, &end);
            fbe_cli_rderr_modify_errtype(start, end, errtype);
            return;
        }
        else if ( (!strcmp(*argv, "-pos") )
                )
        {
            fbe_u32_t start = 0, end = 0;
            fbe_u16_t pos_bitmask;
            fbe_char_t *tmp_ptr;

            argc--;
            argv++;
            if (argc <= 0)
            {
                fbe_cli_printf("rderr: ERROR: Argc %x\n", argc);
                fbe_cli_printf("rderr: ERROR: Too less args.\n");
                fbe_cli_printf(RDERR_CMD_USAGE);
                fbe_cli_printf("%s", "\n");
                return;
            }

            /* get the pos bitmask value*/
            tmp_ptr = *argv;
            if ((*tmp_ptr == '0') && 
                (*(tmp_ptr + 1) == 'x' || *(tmp_ptr + 1) == 'X'))
            {
                tmp_ptr += 2;
            }
            pos_bitmask = fbe_atoh(tmp_ptr);
            /* get the start and end records value*/
            fbe_cli_rderr_get_params_modify_delete_records(argc, argv, &start, &end);
            fbe_cli_rderr_modify_pos_bitmask(start, end, pos_bitmask);
            return;
        }
        else if ( (!strcmp(*argv, "-gap") )
                )
        {
            fbe_u32_t start = 0, end = 0;
            fbe_lba_t gap;
            fbe_char_t *tmp_ptr;
            argc--;
            argv++;
            if (argc <= 0)
            {
                fbe_cli_printf("rderr: ERROR: Argc %x\n", argc);
                fbe_cli_printf("rderr: ERROR: Too less args.\n");
                fbe_cli_printf(RDERR_CMD_USAGE);
                fbe_cli_printf("%s", "\n");
                return;
            }

            /* get the gap value*/
            tmp_ptr = *argv;
            if ((*tmp_ptr == '0') && 
                (*(tmp_ptr + 1) == 'x' || *(tmp_ptr + 1) == 'X'))
            {
                tmp_ptr += 2;
            }
            gap = fbe_atoh(tmp_ptr);
            /* get the start and end records value*/
            fbe_cli_rderr_get_params_modify_delete_records(argc, argv, &start, &end);
            fbe_cli_rderr_modify_gap(start, end, gap);
            return;
        }
        else if ( (!strcmp(*argv, "-r")) ||
                  (!strcmp(*argv, "-remove"))
                )
        {
            fbe_u32_t start = 0, end = 0;
            fbe_cli_rderr_get_params_modify_delete_records(argc, argv, &start, &end);
            fbe_cli_rderr_records_operation(start, end, 
                                            FBE_FALSE, /* create records*/ 
                                            FBE_FALSE, /* modify records*/ 
                                            FBE_TRUE/* delete records*/);
            return;
        }
        else if (!strcmp(*argv, "-enable") || !strcmp(*argv, "-ena"))
        {
            status = fbe_api_logical_error_injection_enable();
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("rderr: ERROR: Not able to enable error injection %d\n", 
                           status);
                return;
            }
            fbe_cli_printf("RDERR status %s\n", "ENABLED");
        }
        else if (!strcmp(*argv, "-disable") || !strcmp(*argv, "-dis"))
        {
            status = fbe_api_logical_error_injection_disable();
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("rderr: ERROR: Not able to disable error injection %d\n", 
                           status);
                return;
            }
            fbe_cli_printf("RDERR status %s\n", "DISABLED");
        }
        else if (!strcmp(*argv, "-enable_for_rg") || 
                (!strcmp(*argv, "-ena_rg") ) ||
                (!strcmp(*argv, "-enable_for_object") ) ||
                (!strcmp(*argv, "-ena_obj") ) || 
                (!strcmp(*argv, "-enable_for_class") ) ||
                (!strcmp(*argv, "-ena_cla") ) ||
                (!strcmp(*argv, "-disable_for_rg") ) ||
                (!strcmp(*argv, "-dis_rg") ) ||
                (!strcmp(*argv, "-disable_for_object") ) ||
                (!strcmp(*argv, "-dis_obj") ) || 
                (!strcmp(*argv, "-disable_for_class") ) ||
                (!strcmp(*argv, "-dis_cla") ) 
                )
        {
            fbe_status_t status;
            fbe_bool_t b_enable_rderr = FBE_FALSE;
            fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;
            fbe_package_id_t package_id = FBE_OBJECT_ID_INVALID;
            fbe_class_id_t class_id = FBE_OBJECT_ID_INVALID;
            status = fbe_cli_rderr_get_params_enable_disable_rderr(argc, argv, &b_enable_rderr,
                                                                   &object_id,
                                                                   &package_id,
                                                                   &class_id);
            /*Proceed only getting parameter succeeded*/
            if(status == FBE_STATUS_OK)
            {
                fbe_cli_rderr_enable_disable_rderr(b_enable_rderr, /*enable/disable rderr*/
                                               object_id,
                                               package_id,
                                               class_id);
            }
            return;
        }
        else if ((!strcmp(*argv, "-stats") ) ||
                 (!strcmp(*argv, "-stats_for_rg") ) ||
                 (!strcmp(*argv, "-stats_for_object") )
                 )
        {
            fbe_cli_rderr_get_statistics(argc, argv);
            return;
        }
        else if (!strcmp(*argv, "-enable_software_errors") || !strcmp(*argv, "-ena_sw"))
        {
            fbe_api_raid_group_enable_raid_lib_random_errors(100, FBE_TRUE /* Only user rgs */);
        }
        else if (!strcmp(*argv, "-disable_software_errors") || !strcmp(*argv, "-dis_sw"))
        {
            fbe_api_raid_group_reset_raid_lib_error_testing_stats();
        }
        else
        {
            fbe_cli_printf("rderr: ERROR: Invalid Argument.\n");
            fbe_cli_printf(RDERR_CMD_USAGE);
            return;
        }
        argv++;
    }
    fbe_cli_printf("%s", "\n");
    if (argc > 0)
    {
        fbe_cli_printf("rderr: ERROR: Argc %x\n", argc);
        fbe_cli_printf("rderr: ERROR: Too many args.\n");
        fbe_cli_printf(RDERR_CMD_USAGE);
        fbe_cli_printf("%s", "\n");
    }

    return;

}
/******************************************
 * end fbe_cli_rginfo_parse_cmds()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_rderr_get_params_enable_disable_rderr()
 ****************************************************************
 *
 * @brief   Parse params for modify or delete records commands
 * @param   argument count
*  @param   arguments list
 *
 * @return  fbe_status_t.
 *
 * @author
 *  09/21/2010 - Created. Swati Fursule
 *
 ****************************************************************/
fbe_status_t fbe_cli_rderr_get_params_enable_disable_rderr(int argc, char** argv,
                                                   fbe_bool_t* b_enable_rderr_p,
                                                   fbe_object_id_t* object_id_p,
                                                   fbe_package_id_t* package_id_p,
                                                   fbe_class_id_t* class_id_p)

{
    fbe_bool_t b_enable_rderr = FBE_FALSE;
    fbe_bool_t b_raid_group = FBE_FALSE;
    fbe_bool_t b_object = FBE_FALSE;
    fbe_bool_t b_class = FBE_FALSE;
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;
    fbe_package_id_t package_id = FBE_OBJECT_ID_INVALID;
    fbe_class_id_t class_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_char_t         *tmp_ptr;
    fbe_u32_t           raid_group_num = FBE_RAID_ID_INVALID;

    /*
     * For raid group, look up Raid group with raid group number
     * else with object id
     */
    if(!strncmp(*argv, "-ena", 4))
    {
        b_enable_rderr = FBE_TRUE;
    }
    /* check enable or disable for raid group*/
    if( (!strcmp(*argv, "-disable_for_rg")) || 
        (!strcmp(*argv, "-dis_rg") ) ||
        (!strcmp(*argv, "-enable_for_rg") )|| 
        (!strcmp(*argv, "-ena_rg") ) 
        )
    {
        b_raid_group = FBE_TRUE;
    }
    /* check enable or disable for object*/
    else if( (!strcmp(*argv, "-disable_for_object")) || 
        (!strcmp(*argv, "-dis_obj") ) ||
        (!strcmp(*argv, "-enable_for_object") )|| 
        (!strcmp(*argv, "-ena_obj") ) 
        )
    {
       b_object = FBE_TRUE;
    }
    /* check enable or disable for class*/
    else
    {
        b_class = FBE_TRUE;
    }
    argc--;
    argv++;
    if (argc < 0)
    {
        fbe_cli_printf("rderr: ERROR: Invalid Argument\n");
        fbe_cli_printf(RDERR_CMD_USAGE);
        fbe_cli_printf("%s", "\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if(b_raid_group)
    {
        tmp_ptr = *argv;
        if ((*tmp_ptr == '0') && 
            (*(tmp_ptr + 1) == 'x' || *(tmp_ptr + 1) == 'X'))
        {
            tmp_ptr += 2;
        }
        raid_group_num = fbe_atoh(tmp_ptr);

        status = fbe_api_database_lookup_raid_group_by_number(
            raid_group_num, &object_id);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rderr: ERROR: RaidGroup 0x%x does not exists\n",
                           raid_group_num);
            return status;
        }
        package_id = FBE_PACKAGE_ID_SEP_0;
    }
    else
    {
        /* get object id/class id and package id*/
        tmp_ptr = *argv;
        if ((*tmp_ptr == '0') && 
            (*(tmp_ptr + 1) == 'x' || *(tmp_ptr + 1) == 'X'))
        {
            tmp_ptr += 2;
        }
        if(b_object)
        {
            object_id = fbe_atoh(tmp_ptr);
        }
        else
        {
            class_id = fbe_atoh(tmp_ptr);
            if ((class_id <= FBE_CLASS_ID_INVALID) ||
                (class_id  > FBE_CLASS_ID_LAST) )
            {
                fbe_cli_printf("rderr: ERROR: Invalid Class Id.\n");
                fbe_cli_printf("%s", RDERR_CMD_USAGE);
                return FBE_STATUS_GENERIC_FAILURE;
            }

        }
        argc--;
        argv++;
        if (argc < 0)
        {
            fbe_cli_printf("rderr: ERROR: Invalid Argument\n");
            fbe_cli_printf(RDERR_CMD_USAGE);
            fbe_cli_printf("%s", "\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }
        tmp_ptr = *argv;
        if ((*tmp_ptr == '0') && 
            (*(tmp_ptr + 1) == 'x' || *(tmp_ptr + 1) == 'X'))
        {
            tmp_ptr += 2;
        }
        package_id = fbe_atoh(tmp_ptr);
    }

    *object_id_p = object_id;
    *package_id_p = package_id;
    *class_id_p = class_id;
    *b_enable_rderr_p = b_enable_rderr;
    return status;
}
/******************************************
 * end fbe_cli_rderr_get_params_enable_disable_rderr()
 ******************************************/

/**********************************************************************
 * fbe_cli_rderr_switch_table
 **********************************************************************
 * 
 * @brief This function switch to the table provided
 *
 * @param argc
 * @param argv
 *
 * @return     void
 *
 * @author  
 *  09/21/2010 - Created. Swati Fursule
 *  
 *********************************************************************/
void fbe_cli_rderr_switch_table(int argc, char** argv)
{
    fbe_api_logical_error_injection_table_description_t table_desc;
    fbe_u32_t table;
    fbe_status_t        status;

    argc--;
    argv++;
    if (argc < 0)
    {
        fbe_cli_printf("rderr: ERROR: Invalid Argument\n");
        fbe_cli_printf(RDERR_CMD_USAGE);
        fbe_cli_printf("%s", "\n");
        return;
    }

    /* get the table number*/
    table = atoi(*argv);
    if(table<0 && table > FBE_API_LOGICAL_ERROR_INJECTION_MAX_TABLES)
    {
        fbe_cli_printf("rderr: ERROR: Invalid Argument.(table)\n");
        return;
    }
    status = fbe_api_logical_error_injection_load_table(table, b_use_simulation_table); 
    /*true for simulation*/
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("rderr: ERROR: Rderr should be disabled , could not load table%d\n",
                   table);
        return;
    }
    status = fbe_api_logical_error_injection_get_table_info(table, b_use_simulation_table, &table_desc);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("rderr: ERROR: could not get tables info \n"
                   );
        return;
    }

    fbe_cli_printf("Rderr table %d is loaded for LEI service\n",
        table);
    return;
}

/**********************************************************************
 * fbe_cli_rderr_get_tables_info
 **********************************************************************
 * 
 * @brief This function get all tables information
 *
 * @param None
 *
 * @return     void
 *
 * @author  
 *  09/21/2010 - Created. Swati Fursule
 *  
 *********************************************************************/
void fbe_cli_rderr_get_tables_info(void)
{
    fbe_u32_t table_index;
    fbe_status_t        status;

    fbe_api_logical_error_injection_table_description_t table_desc;
    /*true for simulation*/
    for(table_index=0; table_index<FBE_API_LOGICAL_ERROR_INJECTION_MAX_TABLES;
        table_index++)
    {
        status = fbe_api_logical_error_injection_get_table_info(table_index, b_use_simulation_table, &table_desc);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rderr: ERROR: could not get tables info \n"
                       );
            return;
        }
    }
    return;
}
/******************************************
 * end fbe_cli_rderr_get_tables_info()
 ******************************************/


/**********************************************************************
 * fbe_cli_rderr_get_records
 **********************************************************************
 * 
 * @brief This function get the records and display those.
 *
 * @param None
 *
 * @return     void
 *
 * @author  
 *  09/21/2010 - Created. Swati Fursule
 *  
 *********************************************************************/
void fbe_cli_rderr_get_records(void)
{
    fbe_api_logical_error_injection_get_records_t get_records;
    fbe_status_t        status;
    fbe_api_logical_error_injection_table_description_t table_desc;
    /* Display the record contents. 
     */
    status = fbe_api_logical_error_injection_get_records(&get_records);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("rderr: ERROR: Not able to get error records %d\n", 
                   status);
        return;
    }
    /* display active table information*/
    status = fbe_api_logical_error_injection_get_active_table_info(FBE_TRUE, &table_desc);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("rderr: ERROR: could not get tables info \n"
                   );
        return;
    }
    /* 
     * Display the error records.
     */
    fbe_api_logical_error_injection_error_display(get_records, 0, FBE_API_LEI_MAX_RECORDS-1);
    fbe_cli_printf("Error records information displayed successfully\n");
    return;
}
/******************************************
 * end fbe_cli_rderr_get_records()
 ******************************************/

/**********************************************************************
 * fbe_cli_rderr_get_statistics
 **********************************************************************
 * 
 * @brief This function enable or disable rderr depending on the i/p
 *
 * @param b_enable
 * @param object_id
 * @param package_id
 * @param class_id
 *
 * @return     void
 *
 * @author  
 *  09/21/2010 - Created. Swati Fursule
 *  
 *********************************************************************/
void fbe_cli_rderr_get_statistics(int argc, char** argv)
{
    fbe_status_t        status;
    fbe_char_t         *tmp_ptr;
    fbe_u32_t           raid_group_num = FBE_RAID_ID_INVALID;


    if (!strcmp(*argv, "-stats") )
    {
        fbe_api_logical_error_injection_get_stats_t   stats;
        status = fbe_api_logical_error_injection_get_stats(&stats);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rderr: ERROR: Not able to get statistics %d\n", 
                           status);
            return;
        }
        fbe_api_logical_error_injection_display_stats(stats);
    }
    else
    {
        fbe_api_logical_error_injection_get_object_stats_t get_obj_stats;
        fbe_bool_t is_raid_group = FBE_FALSE;
        fbe_object_id_t     object_id = FBE_OBJECT_ID_INVALID;

        /*
         * For raid group, look up Raid group with raid group number
         * else with object id
         */
        if(!strcmp(*argv, "-stats_for_rg"))
        {
            is_raid_group = FBE_TRUE;
        }
        argc--;
        argv++;
        if (argc < 0)
        {
            fbe_cli_printf("rderr: ERROR: Invalid Argument\n");
            fbe_cli_printf(RDERR_CMD_USAGE);
            fbe_cli_printf("%s", "\n");
            return;
        }
        if(is_raid_group)
        {
            tmp_ptr = *argv;
            if ((*tmp_ptr == '0') && 
                (*(tmp_ptr + 1) == 'x' || *(tmp_ptr + 1) == 'X'))
            {
                tmp_ptr += 2;
            }
            raid_group_num = fbe_atoh(tmp_ptr);

            status = fbe_api_database_lookup_raid_group_by_number(
                raid_group_num, &object_id);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("rderr: ERROR: RaidGroup 0x%x does not exists\n",
                               raid_group_num);
                return;
            }
        }
        else
        {
            tmp_ptr = *argv;
            if ((*tmp_ptr == '0') && 
                (*(tmp_ptr + 1) == 'x' || *(tmp_ptr + 1) == 'X'))
            {
                tmp_ptr += 2;
            }
            object_id = fbe_atoh(tmp_ptr);
        }
        status = fbe_api_logical_error_injection_get_object_stats(
            &get_obj_stats, object_id);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rderr: ERROR: Not able to get object status"
                           " for Object Id 0x%x\n", 
                           object_id);
            return;
        }
        fbe_api_logical_error_injection_display_obj_stats(get_obj_stats);
    }
    return;
}
/******************************************
 * end fbe_cli_rderr_get_statistics()
 ******************************************/


/**********************************************************************
 * fbe_cli_rderr_enable_disable_rderr
 **********************************************************************
 * 
 * @brief This function enable or disable rderr depending on the i/p
 *
 * @param b_enable
 * @param object_id
 * @param package_id
 * @param class_id
 *
 * @return     void
 *
 * @author  
 *  09/21/2010 - Created. Swati Fursule
 *  
 *********************************************************************/
void fbe_cli_rderr_enable_disable_rderr(fbe_bool_t b_enable,
                                        fbe_object_id_t object_id,
                                        fbe_package_id_t package_id,
                                        fbe_class_id_t class_id)
{
    fbe_status_t status;
    /* check if class id is invalid, that mean do it on object*/
    if(class_id == FBE_OBJECT_ID_INVALID)
    {
        if (b_enable)
        {
            status = fbe_api_logical_error_injection_enable_object(
                object_id, package_id);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("rderr: ERROR: Not able to enable error injection"
                               " for Object id 0x%x and Package id 0x%x\n", 
                               object_id, package_id);
                return;
            }
            fbe_cli_printf("RDERR status ENABLED for Object id 0x%x and Package id 0x%x\n"
                           ,object_id, package_id);
        }
        else
        {
            status = fbe_api_logical_error_injection_disable_object(
                object_id, package_id);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("rderr: ERROR: Not able to disable error injection"
                               " for Object id 0x%x and Package id 0x%x\n", 
                               object_id, package_id);
                return;
            }
            fbe_cli_printf("RDERR status DISABLED for Object id 0x%x and Package id 0x%x\n"
                           ,object_id, package_id);
        }
    }
    else
    {
        if (b_enable)
        {
            status = fbe_api_logical_error_injection_enable_class(
                class_id, package_id);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("rderr: ERROR: Not able to enable error injection"
                               " for Class id 0x%x and Package id 0x%x\n", 
                               class_id, package_id);
                return;
            }
            fbe_cli_printf("RDERR status ENABLED for Class id 0x%x and Package id 0x%x\n"
                           ,class_id, package_id);
        }
        else
        {
            status = fbe_api_logical_error_injection_disable_class(
                class_id, package_id);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("rderr: ERROR: Not able to enable error injection"
                               " for Class id 0x%x and Package id 0x%x\n", 
                               class_id, package_id);
                return;
            }
            fbe_cli_printf("RDERR status DISABLED for Class id 0x%x and Package id 0x%x\n"
                           ,class_id, package_id);
        }
    }
}
/******************************************
 * end fbe_cli_rderr_enable_disable_rderr()
 ******************************************/
fbe_status_t fbe_cli_rderr_display_enum_string(const char *string_array_p[],
                                               fbe_u32_t string_count,
                                               fbe_u32_t num_per_line)
{
    fbe_u32_t index;
    fbe_u32_t line_index = 0;
    for ( index = 0; index < string_count; index++)
    {
        if (string_array_p[index] == NULL)
        {
            fbe_cli_printf("%s index %d not valid for array\n", __FUNCTION__, (int)index);
            return FBE_STATUS_GENERIC_FAILURE;
        }
           
        if (index == string_count - 1)
        {
            fbe_cli_printf("%s\n", string_array_p[index]);
        }
        else if (((line_index != 0) && (line_index % num_per_line) == 0))
        {
            fbe_cli_printf("%s\n            ", string_array_p[index]);
            line_index = 0;
        }
        else
        {
            fbe_cli_printf("%s, ", string_array_p[index]);
            line_index++;
        }
    }
    return FBE_STATUS_OK;
}
/**********************************************************************
 * fbe_cli_rderr_get_errmode
 **********************************************************************
 * 
 * @brief This function get the lba from user
 *
 * @param err_mode_p
 * @param disp_string
 *
 * @return     fbe_status_t
 *
 * @author  
 *  09/21/2010 - Created. Swati Fursule
 *  
 *********************************************************************/
fbe_status_t fbe_cli_rderr_get_errmode(fbe_api_logical_error_injection_mode_t* err_mode_p,
                                       fbe_u8_t *disp_string)
{
    fbe_u8_t                         buff[FBE_RDERR_MAX_STR_LEN];
    fbe_bool_t                       loop = FBE_FALSE;
    int                              rc;
    do
    {
        fbe_cli_printf("Error modes: ");
        fbe_cli_rderr_display_enum_string(&fbe_cli_rderr_mode_strings[0], 
                                          FBE_LOGICAL_ERROR_INJECTION_MODE_LAST, FBE_CLI_LEI_FIELDS_PER_LINE);
        fbe_cli_printf("%s ", disp_string);
        rc = scanf("%s",buff);

        if (rc == 1) /* One-time 'loop' */
        {
            /* get the mode value*/
            *err_mode_p = fbe_api_lei_string_to_enum(buff, 
                                          fbe_cli_rderr_mode_strings, 
                                          FBE_LOGICAL_ERROR_INJECTION_MODE_LAST);
            loop = FBE_TRUE;
        }
    }
    while(!loop);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cli_rderr_get_errmode()
 ******************************************/

/**********************************************************************
 * fbe_cli_rderr_get_errtype
 **********************************************************************
 * 
 * @brief This function get the lba from user
 *
 * @param err_type_p
 * @param disp_string
 *
 * @return     fbe_status_t
 *
 * @author  
 *  09/21/2010 - Created. Swati Fursule
 *  
 *********************************************************************/
fbe_status_t fbe_cli_rderr_get_errtype(fbe_api_logical_error_injection_type_t* err_type_p,
                                       fbe_u8_t *disp_string)
{
    fbe_u8_t                         buff[FBE_RDERR_MAX_STR_LEN];
    fbe_bool_t                       loop = FBE_FALSE;
    int                              rc;
    do
    {
        fbe_cli_printf("Error types: ");
        fbe_cli_rderr_display_enum_string(fbe_cli_rderr_err_type_strings, 
                                          FBE_XOR_ERR_TYPE_MAX, FBE_CLI_LEI_FIELDS_PER_LINE);
        fbe_cli_printf("%s ", disp_string);
        rc = scanf("%s",buff);

        if (rc == 1)/* One-time 'loop' */
        {
            /* get the mode value*/
            *err_type_p = fbe_api_lei_string_to_enum(buff, 
                                                     fbe_cli_rderr_err_type_strings, 
                                                     FBE_XOR_ERR_TYPE_MAX);
            loop = FBE_TRUE;
        }
    }
    while(!loop);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cli_rderr_get_errtype()
 ******************************************/


/**********************************************************************
 * fbe_cli_rderr_get_lba
 **********************************************************************
 * 
 * @brief This function get the lba from user
 *
 * @param lba_p
 * @param disp_string
 *
 * @return     fbe_status_t
 *
 * @author  
 *  09/21/2010 - Created. Swati Fursule
 *  
 *********************************************************************/
fbe_status_t fbe_cli_rderr_get_lba(fbe_lba_t* lba_p,
                                   fbe_u8_t *disp_string)
{
    fbe_u8_t                         buff[FBE_RDERR_MAX_STR_LEN];
    fbe_status_t status = FBE_STATUS_OK;
    do
    {
        fbe_cli_printf("%s (begin with 0x) \n", disp_string);
        scanf("%s",buff);
        /* This may be a valid hex value.
        */
        /* atoh told us that all chars are valid hex digits. */
        status = fbe_atoh64(buff, lba_p);
    }
    while(status != FBE_STATUS_OK);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cli_rderr_get_lba()
 ******************************************/

/**********************************************************************
 * fbe_cli_rderr_get_value
 **********************************************************************
 * 
 * @brief This function get the required the parameters from user
 *
 * @param Pointer to error record **************TODO
 *
 * @return     fbe_status_t
 *
 * @author  
 *  09/21/2010 - Created. Swati Fursule
 *  
 *********************************************************************/
fbe_status_t fbe_cli_rderr_get_value(fbe_u16_t* record_field_p,
                                     fbe_u8_t *disp_string,
                                     fbe_u16_t start_range,
                                     fbe_u16_t end_range)
{
    fbe_u8_t                         buff[FBE_RDERR_MAX_STR_LEN];
    fbe_bool_t                       loop = FBE_FALSE;
    fbe_u8_t*                        temp_str_ptr;
    do
    {
        fbe_cli_printf("%s (begin with 0x) \n", disp_string);
        scanf("%s",buff);
        /* Check for a hex number: */
        /* Returns a pointer to the beginning of the 0x, or NULL if not found */
        temp_str_ptr = strstr(buff, "0x");
        /*tmp_ptr = *argv;
        if ((*tmp_ptr == '0') && 
            (*(tmp_ptr + 1) == 'x' || *(tmp_ptr + 1) == 'X'))
        {
            tmp_ptr += 2;
        }*/

        while (temp_str_ptr != NULL)/* One-time 'loop' */
        {
            /* This may be a valid hex value.
            */
            /* We have a leading 0x that we need to skip over. */
            temp_str_ptr += 2;
            if (fbe_atoh(temp_str_ptr) == 0xFFFFFFFFU )  /* atoh is really ascii hex to integer!!! */
            {
                fbe_cli_printf("Invalid hex value. Please try again.\n");                
                break;   /* ugh.  Not a valid hex number, even though it starts with 0x */
            }
            else 
            {
                /* atoh told us that all chars are valid hex digits. */
                *record_field_p = fbe_atoh(temp_str_ptr);
                loop = FBE_TRUE;
                break;  /* We have a valid hex value here, return a pointer to the string */
            }    
        }
    }
    while(!loop);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cli_rderr_get_value()
 ******************************************/

/**********************************************************************
 * fbe_cli_rderr_usr_intf_add
 **********************************************************************
 * 
 * @brief This function collects the parameters for error insertion, 
 * updates the Error record.
 *
 * @param Pointer to error record
 *
 * @return     fbe_status_t
 *
 * @author  
 *  09/21/2010 - Created. Swati Fursule
 *  
 *********************************************************************/
fbe_status_t fbe_cli_rderr_usr_intf_add(
                        fbe_api_logical_error_injection_record_t* record_p)
{
    fbe_cli_rderr_get_errtype(&(record_p->err_type),"Error type? ");
    fbe_cli_rderr_get_errmode(&(record_p->err_mode),"Error mode? ");

    /*Get lba value*/
    fbe_cli_rderr_get_lba(&(record_p->lba), "LBA? ");
    fbe_cli_rderr_get_lba(&(record_p->blocks), "Blocks? ");
    /* The valid range of pos_bitmap = [0x1, 0x8000].
     */
    /*Get position bitmap value*/
    fbe_cli_rderr_get_value(&(record_p->pos_bitmap),
                            "Position? ",
                            0x1,
                            0x8000);

    /*Get array width*/
    fbe_cli_rderr_get_value(&(record_p->width),
                            "Array Width? ",
                            0,
                            FBE_RAID_MAX_DISK_ARRAY_WIDTH);

    /*Get Count of errs injected */
    fbe_cli_rderr_get_value(&(record_p->err_count),
                            "Error Count? ",
                            0x1,
                            0x8000);

    /*Get limit of errors to inject*/
    fbe_cli_rderr_get_value(&(record_p->err_limit),
                            "Error Limit? ",
                            0x1,
                            0x8000);

    /*Get limit of errors to skip*/
    fbe_cli_rderr_get_value(&(record_p->skip_limit),
                            "Skip Limit? ",
                            0x1,
                            0x8000);

    /*Get Count of errs to skip */
    fbe_cli_rderr_get_value(&(record_p->skip_count),
                            "Skip Count? ",
                            0x1,
                            0x8000);

    return FBE_STATUS_OK;

}
/******************************************
 * end fbe_cli_rderr_usr_intf_add()
 ******************************************/

/**********************************************************************
 * fbe_cli_rderr_usr_intf_add_ex
 **********************************************************************
 * 
 * @brief This is a more extensive add of a record.
 *
 * @param Pointer to error record
 *
 * @return     fbe_status_t
 *
 * @author  
 *  5/13/2014 - Created. Rob Foley
 *  
 *********************************************************************/
void fbe_cli_rderr_usr_intf_add_ex(void)
{
    fbe_status_t status;
    fbe_api_logical_error_injection_record_t  error_record = {0}; //  error injection record

    fbe_zero_memory(&error_record, sizeof(fbe_api_logical_error_injection_record_t));
    fbe_cli_rderr_get_errtype(&(error_record.err_type),"Error type? ");
    fbe_cli_rderr_get_errmode(&(error_record.err_mode),"Error mode? ");

    /*Get lba value*/
    fbe_cli_rderr_get_lba(&(error_record.lba), "LBA? ");
    fbe_cli_rderr_get_lba(&(error_record.blocks), "Blocks? ");
    /* The valid range of pos_bitmap = [0x1, 0x8000].
     */
    /*Get position bitmap value*/
    fbe_cli_rderr_get_value(&(error_record.pos_bitmap),
                            "Position? ",
                            0x1,
                            0x8000);

    /*Get array width*/
    fbe_cli_rderr_get_value(&(error_record.width),
                            "Array Width? ",
                            0,
                            FBE_RAID_MAX_DISK_ARRAY_WIDTH);

    if (error_record.err_mode != FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS){
        /*Get Count of errs injected */
        fbe_cli_rderr_get_value(&(error_record.err_count),
                                "Error Count? ",
                                0x1,
                                0x8000);
    
        /*Get limit of errors to inject*/
        fbe_cli_rderr_get_value(&(error_record.err_limit),
                                "Error Limit? ",
                                0x1,
                                0x8000);
    
        /*Get limit of errors to skip*/
        fbe_cli_rderr_get_value(&(error_record.skip_limit),
                                "Skip Limit? ",
                                0x1,
                                0x8000);
    
        /*Get Count of errs to skip */
        fbe_cli_rderr_get_value(&(error_record.skip_count),
                                "Skip Count? ",
                                0x1,
                                0x8000);
    }

    fbe_cli_rderr_get_value(&(error_record.err_adj),
                            "Error Adjacentcy Bitmap? ",
                            0x1,
                            0xFFFF);

    /*Create a new record.*/
    status = fbe_api_logical_error_injection_create_record(&error_record);

    if (status != FBE_STATUS_OK) {
        fbe_cli_printf("rderr: ERROR: Failed to create a record. %d\n", 
                       status);
        return;
    }

    fbe_cli_printf("rderr: SUCCESS: Successfully created an error record. \n");
    return;

}
/******************************************
 * end fbe_cli_rderr_usr_intf_add_ex()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_rderr_get_params_modify_delete_records()
 ****************************************************************
 *
 * @brief   Parse params for modify or delete records commands
 * @param   argument count
*  @param arguments list
 *
 * @return  None.
 *
 * @author
 *  09/21/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rderr_get_params_modify_delete_records(int argc, char** argv,
                                            fbe_u32_t* start_p,
                                            fbe_u32_t* end_p)
{
    fbe_u32_t start = 0;
    fbe_u32_t end = (FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS - 1);
    /* Edit the record contents. 
     */
    if ((argc > 0) && strcmp(*argv, "a"))
    {
        argc--;
        argv++;

        start = end = atoi(*argv);
        if ((start < 0) || (start >= FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS))
        {
            fbe_cli_printf("rderr: ERROR: Invalid Start record [0x%x < x < 0x%x] \n", 
                      0, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS);
            return;
        }
        if ((argc > 0) && (**argv != '-') && (**argv != '\n'))
        {
            argc--;
            argv++;

            end = atoi(*argv);
            if ((end < start) || (end >= FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS))
            {
                fbe_cli_printf("rderr: ERROR: Invalid End record [0x%x < x < 0x%x] \n", 
                    0, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS);
                return;
            }
        }
    }
    *start_p = start;
    *end_p = end;
}
/******************************************
 * end fbe_cli_rderr_get_params_modify_delete_records()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_rderr_records_operation()
 ****************************************************************
 *
 * @brief   Parse params for create, modify or delete records commands
 * @param   argument start, end
 * @param   argument b_create
 * @param   argument b_modify
 * @param   argument b_delete
 *
 * @return  None.
 *
 * @author
 *  09/21/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rderr_records_operation(fbe_u32_t start, 
                                     fbe_u32_t end,
                                     fbe_bool_t b_create,
                                     fbe_bool_t b_modify,
                                     fbe_bool_t b_delete)
{
    fbe_status_t status;
    fbe_u32_t cur = 0;
    fbe_bool_t is_finished = FBE_FALSE;
    if(b_create)
    {
        fbe_api_logical_error_injection_record_t    error_record = {0}; //  error injection record
        /*Get the logical error injection parameters.*/
        status = fbe_cli_rderr_usr_intf_add(&error_record);

        /*Create a new record.*/
        status = fbe_api_logical_error_injection_create_record(&error_record);

        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rderr: ERROR: Failed to create a record. %d\n", 
                 status);
            return;
        }

        fbe_cli_printf("rderr: SUCCESS: Successfully created an error record. \n");
        return;
    }
    cur = start;
    do
    {
        if(b_modify)
        {
            /* modify records*/
            fbe_api_logical_error_injection_modify_record_t   mod_record = {0};
            status = fbe_cli_rderr_usr_intf_add(&mod_record.modify_record);
            mod_record.record_handle = cur; /* index is record handle*/
            
            status = fbe_api_logical_error_injection_modify_record(&mod_record);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("rderr: ERROR: Not able to modify error records %d\n", 
                           status);
                return;
            }

            if (cur++ < end)
            {
                fbe_u8_t buff[FBE_RDERR_MAX_STR_LEN];
                fbe_cli_printf("Continue for record 0x%x ? (Y/N) [Y]: ", cur);
                scanf("%s",buff);
                if (!strcmp(buff, "Y") || !strcmp(buff, "y") || *buff == 0)
                {
                    is_finished = FBE_FALSE;
                }
                else
                {
                    fbe_cli_printf("rderr: User aborted\r\n");
                    is_finished = FBE_TRUE;
                }
            }
            else
            {
                fbe_cli_printf("Modifications/Deletion Completed\r\n");
                is_finished = FBE_TRUE;
            }
        }
        else if (b_delete)
        {
            /* delete records*/
            fbe_api_logical_error_injection_delete_record_t del_record = {0};
            del_record.record_handle = start; /* index is record handle*/
            status = fbe_api_logical_error_injection_delete_record(&del_record);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("rderr: ERROR: Not able to delete error records %d\n", 
                           status);
                return;
            }
            cur++;
            if (cur > end)
            {
                is_finished = FBE_TRUE;
            }
        }

    }
    while (!is_finished);
    return;
}
/******************************************
 * end fbe_cli_rderr_records_operation()
 ******************************************/
/*!**************************************************************
 *          fbe_cli_rderr_copy_record_details()
 ****************************************************************
 *
 * @brief   Parse params for modify mode for selected records
 * @param   argument start, end
 * @param   argument mode
 *
 * @return  None.
 *
 * @author
 *  09/21/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rderr_copy_record_details(
           fbe_api_logical_error_injection_record_t* dest_rec_p, 
           fbe_api_logical_error_injection_record_t src_rec)
{
    dest_rec_p->pos_bitmap = src_rec.pos_bitmap;
    dest_rec_p->width = src_rec.width;
    dest_rec_p->lba = src_rec.lba;
    dest_rec_p->blocks = src_rec.blocks;
    dest_rec_p->err_type = src_rec.err_type;
    dest_rec_p->err_mode = src_rec.err_mode;
    dest_rec_p->err_count = src_rec.err_count;
    dest_rec_p->err_limit = src_rec.err_limit;
    dest_rec_p->skip_count = src_rec.skip_count;
    dest_rec_p->skip_limit = src_rec.skip_limit;
    dest_rec_p->start_bit = src_rec.start_bit;
    dest_rec_p->crc_det = src_rec.crc_det;
    dest_rec_p->num_bits = src_rec.num_bits;
    dest_rec_p->bit_adj = src_rec.bit_adj;
    dest_rec_p->crc_det = src_rec.crc_det;
    return;

}
/*!**************************************************************
 *          fbe_cli_rderr_modify_mode()
 ****************************************************************
 *
 * @brief   Parse params for modify mode for selected records
 * @param   argument start, end
 * @param   argument mode
 *
 * @return  None.
 *
 * @author
 *  09/21/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rderr_modify_mode(fbe_u32_t start, 
                               fbe_u32_t end,
                               fbe_api_logical_error_injection_mode_t mode)
{
    fbe_status_t status;
    fbe_api_logical_error_injection_get_records_t get_records;
    fbe_u32_t cur = 0;

    /* modify mode field one by one*/
    cur = start;
    do
    {
        /* modify records*/
        fbe_api_logical_error_injection_modify_record_t   mod_record = {0};
        /* Get all the record contents. (modified too)
         */
        status = fbe_api_logical_error_injection_get_records(&get_records);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rderr: ERROR: Not able to get error records %d\n", 
                       status);
            return;
        }
        /* 
         * No need to Display the error records, since can be viewed later with -d
         *
        fbe_api_logical_error_injection_error_display(get_records, cur, cur);*/
        fbe_cli_rderr_copy_record_details(&mod_record.modify_record, get_records.records[cur]);
        mod_record.modify_record.err_mode = mode;
        mod_record.record_handle = cur; /* index is record handle*/
        
        status = fbe_api_logical_error_injection_modify_record(&mod_record);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rderr: ERROR: Not able to modify error records %d\n", 
                       status);
            return;
        }
    }
    while(cur++ < end);
    fbe_cli_printf("Modifications Completed\r\n");

    return;
}
/******************************************
 * end fbe_cli_rderr_modify_mode()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_rderr_modify_errtype()
 ****************************************************************
 *
 * @brief   Parse params for modify mode for selected records
 * @param   argument start, end
 * @param   argument errtype
 *
 * @return  None.
 *
 * @author
 *  09/21/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rderr_modify_errtype(fbe_u32_t start, 
                               fbe_u32_t end,
                               fbe_api_logical_error_injection_type_t errtype)
{
    fbe_status_t status;
    fbe_api_logical_error_injection_get_records_t get_records;
    fbe_u32_t cur = 0;

    /* modify mode field one by one*/
    cur = start;
    do
    {
        /* modify records*/
        fbe_api_logical_error_injection_modify_record_t   mod_record = {0};
        /* Get all the record contents. (modified too)
         */
        status = fbe_api_logical_error_injection_get_records(&get_records);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rderr: ERROR: Not able to get error records %d\n", 
                       status);
            return;
        }
        /* 
         * No need to Display the error records, since can be viewed later with -d
         *
        fbe_api_logical_error_injection_error_display(get_records, cur, cur);*/
        fbe_cli_rderr_copy_record_details(&mod_record.modify_record, get_records.records[cur]);
        mod_record.modify_record.err_type = errtype;
        mod_record.record_handle = cur; /* index is record handle*/
        
        status = fbe_api_logical_error_injection_modify_record(&mod_record);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rderr: ERROR: Not able to modify error records %d\n", 
                       status);
            return;
        }
    }
    while(cur++ < end);
    fbe_cli_printf("Modifications Completed\r\n");

    return;
}
/******************************************
 * end fbe_cli_rderr_modify_errtype()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_rderr_modify_pos_bitmask()
 ****************************************************************
 *
 * @brief   Parse params for modify position bitmask for selected records
 * @param   argument start, end
 * @param   argument pos_bitmask
 *
 * @return  None.
 *
 * @author
 *  09/21/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rderr_modify_pos_bitmask(fbe_u32_t start, 
                               fbe_u32_t end,
                               fbe_u16_t pos_bitmask)
{
    fbe_status_t status;
    fbe_api_logical_error_injection_get_records_t get_records;
    fbe_u32_t cur = 0;

    /* modify mode field one by one*/
    cur = start;
    do
    {
        /* modify records*/
        fbe_api_logical_error_injection_modify_record_t   mod_record = {0};
        /* Get all the record contents. (modified too)
         */
        status = fbe_api_logical_error_injection_get_records(&get_records);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rderr: ERROR: Not able to get error records %d\n", 
                       status);
            return;
        }
        /* 
         * No need to Display the error records, since can be viewed later with -d
         *
        fbe_api_logical_error_injection_error_display(get_records, cur, cur);*/
        fbe_cli_rderr_copy_record_details(&mod_record.modify_record, get_records.records[cur]);
        mod_record.modify_record.pos_bitmap = pos_bitmask;
        mod_record.record_handle = cur; /* index is record handle*/
        
        status = fbe_api_logical_error_injection_modify_record(&mod_record);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rderr: ERROR: Not able to modify error records %d\n", 
                       status);
            return;
        }
    }
    while(cur++ < end);
    fbe_cli_printf("Modifications Completed\r\n");

    return;
}
/******************************************
 * end fbe_cli_rderr_modify_pos_bitmask()
 ******************************************/
/*!**************************************************************
 *          fbe_cli_rderr_modify_gap()
 ****************************************************************
 *
 * @brief   Parse params for modify mode for selected records
 * @param   argument start, end
 * @param   argument gap_in_blocks
 *
 * @return  None.
 *
 * @author
 *  09/21/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rderr_modify_gap(fbe_u32_t start, 
                              fbe_u32_t end,
                              fbe_lba_t gap_in_blocks)
{
    fbe_status_t status;
    fbe_api_logical_error_injection_get_records_t get_records;
    fbe_u32_t cur = 0;
    fbe_lba_t current_lba =0;

    /* modify mode field one by one*/
    cur = start;
    do
    {
        /* modify records*/
        fbe_api_logical_error_injection_modify_record_t   mod_record = {0};
        /* Get all the record contents. (modified too)
         */
        status = fbe_api_logical_error_injection_get_records(&get_records);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rderr: ERROR: Not able to get error records %d\n", 
                       status);
            return;
        }
        /* 
         * No need to Display the error records, since can be viewed later with -d
         *
        fbe_api_logical_error_injection_error_display(get_records, cur, cur);*/
        fbe_cli_rderr_copy_record_details(&mod_record.modify_record, get_records.records[cur]);
        /* Start with the first lba in the range.
         */
        /* For the rest of the range, space out everything evenly.
         */
        /*current_lba = get_records.records[cur].lba;*/
        if(cur != start)
        {
            current_lba += gap_in_blocks;
        }
        

        mod_record.modify_record.lba = current_lba;
        mod_record.record_handle = cur; /* index is record handle*/
        
        status = fbe_api_logical_error_injection_modify_record(&mod_record);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rderr: ERROR: Not able to modify error records %d\n", 
                       status);
            return;
        }
    }
    while(cur++ < end);
    fbe_cli_printf("Modifications Completed\r\n");

    return;
}
/******************************************
 * end fbe_cli_rderr_modify_gap()
 ******************************************/

/*************************
 * end file fbe_cli_lib_rderr_cmds.c
 *************************/
