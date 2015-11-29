/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_signature_cmd.c
 ***************************************************************************
 *
 * @brief
 *  This file contains FBE cli command for get/set fru signature.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *  2012,Aug,14 - He Wei created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_cli_lib_signature_cmd.h"
#include "fbe_port.h"
#include "fbe_enclosure.h"
#include "fbe_database.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "stdio.h"

/*************************************************************************
 *                            @fn fbe_cli_cmd_signature ()                                                           *
 *************************************************************************
 *
 *  @brief
 *   fbe_cli_cmd_signature performs the execution of the get/set fru signature command
 *   
 *
 * @param :
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   argv     (INPUT) ARGUMENT LIST
 *
 * @return:
 *   None
 *
 * @author
 *   2012,Aug,14 - He Wei created
 *
 *************************************************************************/
 void fbe_cli_cmd_signature(fbe_s32_t argc, char **argv)
 {

     fbe_u32_t bus = FBE_PORT_NUMBER_INVALID;
     fbe_u32_t enclosure = FBE_ENCLOSURE_NUMBER_INVALID;
     fbe_u32_t slot = FBE_SLOT_NUMBER_INVALID;
     fbe_status_t status;
     fbe_bool_t is_read = FBE_FALSE; /*TRUE stands for read, false stands for no operation*/
     fbe_bool_t is_write = FBE_FALSE;
     fbe_bool_t is_clear = FBE_FALSE;
     fbe_fru_signature_t signature = {0};
     fbe_cmi_service_get_info_t cmi_info;
     
     status = fbe_api_cmi_service_get_info(&cmi_info);
     if(FBE_STATUS_OK != status)
     {
         fbe_cli_error("\nFailed to get cmi info ... Error: %d\nPlease use this cli on active SP\n", status);
         return;
     }
     
     if(FBE_CMI_STATE_ACTIVE != cmi_info.sp_state
        && FBE_CMI_STATE_SERVICE_MODE != cmi_info.sp_state)
     {
         fbe_cli_error("\n\tThis cli can only be used on active side\n");
         return;
     }
     
     while (argc > 0)
     {
         if ((strcmp(*argv, "-help") == 0) ||
             (strcmp(*argv, "-h") == 0))
         {
             /* If they are asking for help, just display help and exit.
             */
             fbe_cli_printf("%s", FBE_CLI_SIGNATURE_USAGE);
             return;
         }
         else if ((strcmp(*argv, "-get") == 0))
         {
            is_read = FBE_TRUE;
            is_write = FBE_FALSE;
            is_clear = FBE_FALSE;
            
            argc--;
            argv++;
         }
         else if ((strcmp(*argv, "-set") == 0))
         {
            is_read = FBE_FALSE;
            is_write = FBE_TRUE;
            is_clear = FBE_FALSE;
            
            argc--;
            argv++;
         }
          else if ((strcmp(*argv, "-clear") == 0))
         {
            is_read = FBE_FALSE;
            is_write = FBE_FALSE;
            is_clear = FBE_TRUE;
            
            argc--;
            argv++;
         }
         else if ((strcmp(*argv, "-b") == 0))
         {
             /* Get the Bus number from command line */
             argc--;
             argv++;
             /* argc should be greater than 0 here */
             if (argc <= 0)
             {
                 fbe_cli_error("bus is expected\n");
                 return;
             }
     
             bus = atoi(*argv);
     
             argc--;
             argv++;
         }
         /* Get the Enclosure number from command line */
         else if ((strcmp(*argv, "-e") == 0))
         {
             argc--;
             argv++;
     
             /* argc should be greater than 0 here */
             if (argc <= 0)
             {
                 fbe_cli_error("Enclosure is expected\n");
                 return;
             }
     
             enclosure = atoi(*argv);
     
             argc--;
             argv++;
         }
         /* Get the Slot number from command line */
         else if ((strcmp(*argv, "-s") == 0))
         {
             argc--;
             argv++;
     
             /* argc should be greater than 0 here */
             if (argc <= 0)
             {
                 fbe_cli_error("Slot is expected\n");
                 return;
             }
     
             slot = atoi(*argv);
     
             argc--;
             argv++;
         }
         else
         {
             /* The command line parameters should be properly entered */
             fbe_cli_error("Invalid argument.\n");
             fbe_cli_printf("%s", FBE_CLI_SIGNATURE_USAGE);
             return;
         }
     }
     
     if( bus == FBE_PORT_NUMBER_INVALID ||
         enclosure == FBE_ENCLOSURE_NUMBER_INVALID ||
         slot == FBE_SLOT_NUMBER_INVALID)
     {
         fbe_cli_error("Invalid argument.\n");
         fbe_cli_printf("%s", FBE_CLI_SIGNATURE_USAGE);
         return;
     }

     
     signature.bus = bus;
     signature.enclosure = enclosure;
     signature.slot = slot;

     if (is_read == FBE_TRUE && is_write == FBE_FALSE && is_clear == FBE_FALSE)
     {
        status = fbe_api_database_get_disk_signature_info(&signature);

        
        fbe_cli_printf("\nFRU SIGNATURE :\n\n");
        fbe_cli_printf("\tBus: %d, Enclosure: %d, Slot: %d\n", 
                        signature.bus, signature.enclosure, signature.slot);
        
        fbe_cli_printf("\tmagic string: %s\n", signature.magic_string);
        fbe_cli_printf("\twwn seed: 0x%x\n", (unsigned int)signature.system_wwn_seed);
        fbe_cli_printf("\tsturcture version: 0x%x\n", signature.version);
     }
     else if (is_read == FBE_FALSE && is_write == FBE_TRUE && is_clear == FBE_FALSE)
     {
         if(FBE_CMI_STATE_ACTIVE != cmi_info.sp_state)
         {
             fbe_cli_error("\n\tThis cli can only be used on active side\n");
             return;
         }
         
        status = fbe_api_database_set_disk_signature_info(&signature);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_error("Write fail.\n");
        }else
        {
            fbe_cli_printf("Write successfully.\n");
        }
     }
     else if (is_read == FBE_FALSE && is_write == FBE_FALSE && is_clear == FBE_TRUE)
     {
         if(FBE_CMI_STATE_ACTIVE != cmi_info.sp_state)
         {
             fbe_cli_error("\n\tThis cli can only be used on active side\n");
             return;
         }
     
         status = fbe_api_database_clear_disk_signature_info(&signature);
         if (status != FBE_STATUS_OK)
         {
             fbe_cli_error("Clear fail.\n");
         }else
         {
             fbe_cli_printf("Clear successfully.\n");
         }
     }
     else
     {
         fbe_cli_error("Invalid argument.\n");
         fbe_cli_printf("%s", FBE_CLI_SIGNATURE_USAGE);
         return;
     }

    
    return;
 }
 
/******************************************************
 * end fbe_cli_lib_signature_cmd() 
 ******************************************************/
