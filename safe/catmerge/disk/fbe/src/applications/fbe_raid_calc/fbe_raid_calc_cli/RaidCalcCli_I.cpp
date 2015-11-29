/***************************************************************************
 * Copyright (C) EMC Corporation 2007
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/


/***************************************************************************
 * RaidCalcCli_I.cpp
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the "main" function for Interactive mode of 
 *  the RaidCalcCli Application along with the supporting functions 
 *  for the different commands of Interactive mode.
 *
 * FUNCTIONS
 *  RaidCalcCli_I - RaidCalcCli_I() is the main entry point 
 *      for the Raid Calculator CLI interactive mode and 
 *      the main loop for RaidCalcCli> that executes each command.
 *  raid_cli_x_set - executes the set command.
 *  raid_cli_x_convert - executes the convert commmand.
 *  raid_cli_x_display - executes the display command.
 *  raid_cli_x_getlba - executes the getlba command.
 *  raid_cli_x_getpba - executes the getpba command.
 *  raid_cli_x_help - executes the help command.
 *  raid_cli_x_exit - executes the exit command.
 *  raid_cli_print_usage - displays the usage strings 
 *      associated with the command.
 *  raid_cli_print_help - displays the help strings 
 *      associated with the command.
 *  parse_cmd - parse the command to get the number of  
 *      arguments in the command and return the count.
 *  display - displays the current setting of the Raid Calculator.
 *  
 * NOTES
 *  None.
 *
 * HISTORY
 *  04/09/2007  Created. PP.
 ****************************************************************************/

/************************
 * INCLUDE FILES
 ************************/

#include "stdafx.h"
#include "RaidCalcCli_I.h"

#pragma warning( disable : 4101)  
#pragma warning( disable : 4129)
#pragma warning( disable : 4244)

/* This array of structure has the information about the commands used in the 
 * interactive mode. It has the command string,command abbreviation, function
 * corresponding to the command, help string and usage string for the command.
 */

RAID_CLI_CMD_TBL_S raid_cli_cmd_tbl_a[] =
{
    {SET_STR, SET_A_STR, raid_cli_x_set, SET_SUMMARY_STR,SET_USAGE},
    {CONVERT_STR, CONVERT_A_STR, raid_cli_x_convert, 
        CONVERT_SUMMARY_STR, CONVERT_USAGE},
    {DISPLAY_STR, DISPLAY_A_STR, raid_cli_x_display, 
        DISPLAY_SUMMARY_STR, DISPLAY_USAGE},
    {GETLBA_STR, GETLBA_A_STR, raid_cli_x_getlba, 
        GETLBA_SUMMARY_STR, GETLBA_USAGE},
    {GETPBA_STR, GETPBA_A_STR, raid_cli_x_getpba, 
        GETPBA_SUMMARY_STR, GETPBA_USAGE},
    {HELP_STR, HELP_A_STR, raid_cli_x_help, HELP_SUMMARY_STR, HELP_USAGE},
    {EXIT_STR, EXIT_A_STR, raid_cli_x_exit, EXIT_SUMMARY_STR, EXIT_USAGE}
};

/****************************************
 * LOCAL FUNCTIONS
 ****************************************/

/***************************************************************************
 * raid_cli_print_help()
 ***************************************************************************
 *
 *  DESCRIPTION:
 *    Displays the help strings associated with the command.
 *
 *  PARAMETERS:
 *    cmd_p (INPUT) COMMAND NAME
 *    cmd_a_p(INPUT) COMMAND ABBREVIATION  
 *    help_p (INPUT) HELP STRING
 *
 *  RETURN VALUES:
 *    None.
 *
 *  HISTORY:
 *    04/12/2007  PP - created
 ***************************************************************************/
void raid_cli_print_help(TEXT * cmd_p,
                         TEXT * cmd_a_p,
                         TEXT * help_p)
{
    printf("\n%10s/%-3s- %s\n", cmd_p, cmd_a_p, help_p);
    return;
}
/*****************************************
 * end raid_cli_print_help()
 *****************************************/

/*************************************************************************
 * raid_cli_print_usage ()
 *************************************************************************
 *
 *  DESCRIPTION:
 *    Displays the usage strings associated with the command.
 *
 *  PARAMETERS:
 *    cmd_p   (INPUT) COMMAND NAME
 *    usage_p (INPUT) USAGE STRING
 *
 *  RETURN VALUES:
 *    None.
 *
 *  HISTORY:
 *    04/12/2007  PP - created
 *************************************************************************/

void raid_cli_print_usage(TEXT * cmd_p,
                          TEXT * usage_p)
{
    TEXT *head_p;
    TEXT *string_p;
    TEXT local_str[LOCAL_STR];

    printf("\nCommand name: %s \n", cmd_p);
    printf("%s\n", usage_p);
    return;
}
/*****************************************
 * end raid_cli_print_usage()
 *****************************************/

/*************************************************************************
 * display()
 *************************************************************************
 *
 *  DESCRIPTION:
 *    Displays the current setting of the Raid Calculator.
 *    
 *  PARAMETERS:
 *    CalcStruct - ptr to CalcStructure.
 *
 *  RETURN VALUES:
 *    None
 *
 *  HISTORY:
 *    04/09/2007  PP - created
 **************************************************************************/

void display(RAID_CALC_STRUCT& CalcStruct)
{
   RAID_TYPE raid_index ;
   CHAR local_raid_type[RAID_TYPE_STR_SIZE];
   raid_index = CalcStruct.raidType;

   if((raid_index < 0 ) || (raid_index > RAID_MAX_UNIT_TYPE ))
   {
       printf("\nInvalid Raid Type.\n");
       return;
   }
   else
   {
       /* Retrieve the raid type from the CalcStruct.
        */
       strcpy(local_raid_type, raid_output_string[raid_index]);
   }
   /* Prints the Raid calculator Geometry.
    */
   printf("\n \nRaid Calculator Geometry:\n");
   printf("\n Raid type: %s ", local_raid_type);
   printf("\n Element size: 0x%X", CalcStruct.sectorsPerElement);
   printf("\n Address offset: 0x%llX", CalcStruct.AddressOffset);
   printf("\n Array width: 0x%X", CalcStruct.ArrayWidth);
   printf("\n Alignment lba: 0x%llX", CalcStruct.AlignmentLba);
   printf("\n Elements per parity stripe: 0x%X",
               CalcStruct.elementsPerParityStripe);
   return;
}
/*****************************************
 * end display()
 *****************************************/

/*************************************************************************
 * raid_cli_x_help()
 *************************************************************************
 *
 *  DESCRIPTION:
 *    raid_cli_x_help - execute the Help command.
 *    Displays all the available commands.
 *
 *  PARAMETERS:
 *    argc     (INPUT) NUMBER OF ARGUMENTS
 *    local_argv  (INPUT) ARGUMENT LIST
 *    usage_p  (INPUT) USAGE COMMAND
 *
 *  RETURN VALUES:
 *    None
 *
 *  HISTORY:
 *    04/11/2007  PP - created
 **************************************************************************/

void raid_cli_x_help(UINT_32 argc,
                     TEXT * local_argv[],
                     TEXT * usage_p)
{
    TEXT *cmd;
    cmd = local_argv[0];
    if( argc > 1)
    {
        local_argv++;
        printf("\n%s invalid argument.\n", *local_argv);
        return;
    }
    raid_cli_print_usage(cmd, usage_p);
    return;
}
/*****************************************
 * end raid_cli_x_help()
 *****************************************/

/****************************************************************
 * raid_cli_x_getpba()
 ****************************************************************
 * 
 * DESCRIPTION:
 *   Executes the "getpba" command and converts the logical block 
 *   address to physical block address using the information 
 *   contained in the CalcStructure by calling RaidCalcDll function.
 *
 * PARAMETERS:
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   local_argv     (INPUT) ARGUMENT LIST
 *   usage_p  (INPUT) USAGE COMMAND
 *
 * RETURNS 
 *    None.
 *
 * HISTORY:
 *   04/11/2007  PP - created
 *****************************************************************/
void raid_cli_x_getpba(UINT_32 argc,
                       TEXT * local_argv[],
                       TEXT * usage_p)
{
    TEXT *cmd;
    cmd = local_argv[0];
    if(argc < 2)
    {
        printf("\nToo few arguments.\n");
        raid_cli_print_usage(cmd, usage_p);
        return;
    }

    local_argv++;
    argc--;

    /* Check the argument for lba and set the CalcStruct.
     */
    if (strcmp(*local_argv,"-lba") == 0)
    {
        /* Set the flag to true for the Lba to pba conversion.
         */
        calc_flag = LOG_TO_PHYS;
        local_argv++;
        argc--;

        if(argc == 0)
        {
            printf("\nEnter the -lba arument value.\n");
            return;
        }
        

        format_input = read_and_validate_input(*local_argv);
        if(format_input != INVALID_INPUT)
        {
            CalcStruct.Lba = format_input;
            if(CalcStruct.Lba > MAX_LBA)
            {
                printf("\nLBA should be less than 0x%X.\n", (unsigned int)MAX_LBA);
                return;
            }
        }
        else
        {
            printf("\n Invalid -lba argument value.\n");
            return;
        }
    }
    else
    {
        printf("\nInvalid argument. -lba argument expected.\n");
        return;
    }
    /* Check for any more arguments entered by user 
     * which are not required. Print the error message.
     */
    local_argv++;
    argc--;
    if(argc != 0)
    {
        printf("\n%s invalid argument.\n", *local_argv);
        raid_cli_print_usage(cmd, usage_p);
        return;
    }
    
    /* Call for the lba to pba conversion.
     */
    RaidCalcLogicalToPhysical(CalcStruct);
    /* Display the output.
     */
    out_hex(CalcStruct,calc_flag);

    return;

}
/*****************************************
 * end raid_cli_x_getpba()
 *****************************************/

/****************************************************************
 * raid_cli_x_getlba()
 ****************************************************************
 * 
 * DESCRIPTION:
 *   Executes the "getlba" command and converts the physical block 
 *   address to logical block address using the information 
 *   contained in the CalcStructure by calling RaidCalcDll function.
 *
 * PARAMETERS:
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   local_argv     (INPUT) ARGUMENT LIST
 *   usage_p  (INPUT) USAGE COMMAND
 *
 * RETURNS 
 *    None.
 *
 * HISTORY:
 *   04/11/2007  PP - created
 *****************************************************************/

void raid_cli_x_getlba(UINT_32 argc,
                       TEXT * local_argv[],
                       TEXT * usage_p)
{
    TEXT *cmd;
    cmd = local_argv[0];
    if(argc < 4)
    {
        printf("\nToo few arguments.\n");
        raid_cli_print_usage(cmd, usage_p);
        return;
    }
    local_argv++;
    argc--;
    
    if (strcmp(*local_argv,"-pba") == 0)
    {
        calc_flag = PHYS_TO_LOG;
        
        local_argv++;
        argc--;
        
        format_input = read_and_validate_input(*local_argv);
        if(format_input != INVALID_INPUT)
        {
            CalcStruct.Pba = format_input;
            if(CalcStruct.Pba > MAX_PBA)
            {
                printf("\nPBA should be less than 0x%X.\n", (unsigned int)MAX_PBA);
                return;
            }
        }
        else
        {
            printf("\nInvalid -pba argument value.\n");
            return;
        }
        /* Check whether pba is greater than address offset or not.
         */
        if((calc_flag == PHYS_TO_LOG) &&
            (CalcStruct.Pba < CalcStruct.AddressOffset))
        {
            printf("\npba should not be less than address offset.\n");
            return;
        }
        
    }
    else
    {
        printf("\nInvalid argument. -pba argument expected.\n");
        return;
    }

    local_argv++;
    argc--;

    /* Check the argument for data position and set the CalcStruct.
     */
    if (strcmp(*local_argv,"-dpos") == 0)
    {
        local_argv++;
        argc--;
        
        format_input = read_and_validate_input(*local_argv);
        if(format_input != INVALID_INPUT)
        {
            CalcStruct.dataPos = format_input;
            if(CalcStruct.dataPos > CalcStruct.ArrayWidth)
            {
                printf("\nData position should be less than "
                            "array width.\n");
                return;
            }
        }
        else
        {
            printf("\nInvalid -dpos argument value.\n");
            return;
        }
    }
    else
    {
        printf("\nInvalid argument. -dpos argument expected.\n");
        return;
    }
    /* Check for any more arguments entered by user 
     * which are not required. Print the error message.
     */
    local_argv++;
    argc--;
    if(argc != 0)
    {
        printf("\n%s invalid argument.\n", *local_argv);
        raid_cli_print_usage(cmd, usage_p);
        return;
    }
    
    /* Call for the lba to pba conversion.
     */
    RaidCalcPhysicalToLogical(CalcStruct);
    /* Display the output.
     */
    out_hex(CalcStruct,calc_flag);
    return;
}
/*****************************************
 * end raid_cli_x_getlba()
 *****************************************/

/****************************************************************
 * raid_cli_x_display()
 ****************************************************************
 * 
 * DESCRIPTION:
 *   Executes the "display" command and displays the current setting
 *   of the Raid Calculator.
 *
 * PARAMETERS:
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   local_argv     (INPUT) ARGUMENT LIST
 *   usage_p  (INPUT) USAGE COMMAND
 *
 * RETURNS 
 *    None.
 *
 * HISTORY:
 *   04/11/2007  PP - created
 *****************************************************************/

void raid_cli_x_display(UINT_32 argc,
                        TEXT * local_argv[],
                        TEXT * usage_p)
{
   TEXT *cmd;
   cmd = local_argv[0];
   if(argc > 1)
   {
       local_argv++;
       printf("\n%s invalid argument.\n", *local_argv);
       raid_cli_print_usage(cmd, usage_p);
       return;
   }
   display(CalcStruct);
   return;
}
/*****************************************
 * end raid_cli_x_display()
 *****************************************/

/****************************************************************
 * raid_cli_x_exit()
 ****************************************************************
 * 
 * DESCRIPTION:
 *   Exit from the RaidCalcCli. 
 *
 * PARAMETERS:
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   local_argv     (INPUT) ARGUMENT LIST
 *   usage_p  (INPUT) USAGE COMMAND
 *
 * RETURNS 
 *    None.
 *
 * HISTORY:
 *   04/11/2007  PP - created
 *****************************************************************/

void raid_cli_x_exit(UINT_32 argc,
                     TEXT * local_argv[],
                     TEXT * usage_p)
{
   return;
}
/*****************************************
 * end raid_cli_x_exit()
 *****************************************/

/****************************************************************
 * raid_cli_x_convert()
 ****************************************************************
 * 
 * DESCRIPTION:
 *   Converts the output of the Raid Calculator to specified format
 *   (either Hexadecimal or decimal).
 *
 * PARAMETERS:
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   local_argv     (INPUT) ARGUMENT LIST
 *   usage_p  (INPUT) USAGE COMMAND
 *
 * RETURNS 
 *    None.
 *
 * HISTORY:
 *   04/11/2007  PP - created
 *****************************************************************/

void  raid_cli_x_convert(UINT_32 argc,
                         TEXT * local_argv[],
                         TEXT * usage_p)
{
    TEXT *cmd;
    cmd = local_argv[0];
    if(argc < 2)
    {
        printf("\nToo few arguments.\n");
        raid_cli_print_usage(cmd, usage_p);
        return;
    }
    
    local_argv++;
    argc--;
    strcpy(output, *local_argv);

    /* Check for the output format and 
     * set the flag as per the requirement.
     */
    if (strcmp(output,"-hex") == 0)
    {
        output_form = HEX ;
    }
    else
    {
        if (strcmp(output,"-dec") == 0)
        {
            output_form = DEC;
        }
        else
        {
            printf("\nInvalid convert argument value.\n");
            return;
        }
    }
    /* Check for any more arguments entered by user 
     * which are not required. Print the error message.
     */
    local_argv++;
    argc--;
    if(argc != 0)
    {
        printf("\n%s invalid argument.\n", *local_argv);
        raid_cli_print_usage(cmd, usage_p);
        return;
    }
        
    switch(output_form)
    {
    case HEX:
        {
            out_hex(CalcStruct,calc_flag);
        }
        break;
    case DEC:
        {
            out_dec(CalcStruct,calc_flag);
        }
        break;
    default:
        {
            printf("\nInvalid Format to display\n");
        }
        break;
    }
    return;

}
/*****************************************
 * end raid_cli_x_convert()
 *****************************************/

/****************************************************************
 * raid_cli_x_set()
 ****************************************************************
 * 
 * DESCRIPTION:
 *   Set the Raid Calculator to default setting or to the specified 
 *   values.
 *
 * PARAMETERS:
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   local_argv     (INPUT) ARGUMENT LIST
 *   usage_p  (INPUT) USAGE COMMAND
 *
 * RETURNS 
 *    None.
 *
 * HISTORY:
 *   04/11/2007  PP - created
 *****************************************************************/

void raid_cli_x_set(UINT_32 argc,
                    TEXT * local_argv[],
                    TEXT * usage_p)
{
    TEXT *cmd;
    CHAR set_raid_type[RAID_TYPE_STR_SIZE];
    UINT_32 raid_index;
	BOOL elements_per_parity_stripe_input = FALSE;

    cmd = local_argv[0];
    
    if(argc < 2)
    {
        printf("\nToo few arguments.\n");
        raid_cli_print_usage(cmd, usage_p);
        return;
    }
    /* Set the raid Calculator to the default setting.
     */
    
    if(strcmp(*(local_argv +1),"-default") == 0 )
    {
        CalcStruct.raidType = RAID5;
        CalcStruct.sectorsPerElement = DEFAULT_SECTORS_PER_ELEMENT;
        CalcStruct.AddressOffset = DEFAULT_ADDRESS_OFFSET;
        CalcStruct.ArrayWidth = DEFAULT_ARRAY_WIDTH;
        CalcStruct.elementsPerParityStripe = DEFAULT_ELEMENTS_PER_PARITY;
        CalcStruct.AlignmentLba = DEFAULT_ALIGN_LBA;
        
        /* Increment the local_argv and decrement argc,
         * to check the argument(if present) after the -default.
         */
        local_argv=local_argv + 2;
        argc = argc - 2;
        /* Display the error for the invalid argument.
         */
        if (argc != 0)
        {
            printf("\n%s invalid argument.\n", *local_argv);
            raid_cli_print_usage(cmd, usage_p);
            return;
        }
    }
    else
    {
    
        if(argc < 11)
        {
            printf("Too few arguments");
            raid_cli_print_usage(cmd, usage_p);
            return;
        }
        local_argv++;
        argc--;
        
        /* Check the argument for raid type and set the CalcStruct.
         */
        if (strcmp(*local_argv,"-r") == 0)
        {
            local_argv++;
            argc--;
            strcpy(set_raid_type, *local_argv);
            /* Determine the raid type from the input string.
             */
            determine_raid_type(set_raid_type,CalcStruct);
            /* Check for the valid raid type. 
             */
            if(CalcStruct.raidType == INVALID_RAID_TYPE)
            {
                printf("\nInvalid Raid Type.\n");
                return;
            } 
        }
        else
        {
            printf("\nInvalid argument. -r argument expected.\n");
            return;
        }

        local_argv++;
        argc--;

        /* Check the argument for element size and set the CalcStruct.
        */
        if (strcmp(*local_argv,"-elsz") == 0)
        {
            local_argv++;
            argc--;
            format_input = read_and_validate_input(*local_argv);
            if(format_input != INVALID_INPUT)
            {
                if((format_input > MAX_ELSZ)||(format_input < MIN_VALUE))
                {
                   printf("\nElement size should be greater than or "
                            "equal to  0x%x and less than or equal to "
                            "0x%x\n",MIN_VALUE,MAX_ELSZ); 
                   return;
                }
                CalcStruct.sectorsPerElement = format_input;
            }
            else
            {
                printf("\nInvalid -elsz argument value.\n");
                return;
            }
            /* Display the error message for the invalid element size.
             */
            if (((CalcStruct.raidType == RAID1) || (CalcStruct.raidType == IND_DISK)) &&
                (CalcStruct.sectorsPerElement != DEFAULT_SECTORS_PER_ELEMENT))
            {
                printf("\nElement size should be 0x%X.\n",DEFAULT_SECTORS_PER_ELEMENT);
                return;
            }
        }
        else
        {
            printf("\nInvalid argument. -elsz argument expected.\n");
            return;
        }

        local_argv++;
        argc--;

        /* Check the argument for address offset and set the CalcStruct.
         */
        if (strcmp(*local_argv,"-addr_offset") == 0)
        {
            local_argv++;
            argc--;
            format_input = read_and_validate_input(*local_argv);
            if(format_input > MAX_ADDR_OFFSET)
            {
                printf("\nAddress offset should be less than or equal to 0x%X.\n",
                        (unsigned int)MAX_ADDR_OFFSET);
                return;
            }
            if(format_input != INVALID_INPUT)
            {
                CalcStruct.AddressOffset = format_input;
            }
            else
            {
                printf("\nInvalid -addr_offset argument value.\n");
                return;
            }
            
        }
        else
        {
            printf("\nInvalid argument. -addr_offset argument expected.\n");
            return;
        }

        local_argv++;
        argc--;

        /* Check the argument for array width and set the CalcStruct.
         */
        if (strcmp(*local_argv,"-w") == 0)
        {
            local_argv++;
            argc--;
            format_input = read_and_validate_input(*local_argv);
            if(format_input != INVALID_INPUT)
            {
                CalcStruct.ArrayWidth = format_input;
            }
            else
            {
                printf("\nInvalid -w argument value.\n");
                return;
            }
            raid_index = CalcStruct.raidType;
            /* Set an error if the array width is not apropriate to raid type.
            */
            switch(raid_index)
            {
                case RAID0:
                case RAID5:
                case RAID6:
                {
                    if((CalcStruct.ArrayWidth < MIN_ARRAY_WIDTH) || 
                        (CalcStruct.ArrayWidth > MAX_ARRAY_WIDTH))
                    {
                        printf("\n Array width should be greater than "
                                "or euqal to %d and less than %d "
                                "for Raid 0 ,Raid 5 and Raid 6.\n",
                                MIN_ARRAY_WIDTH, MAX_ARRAY_WIDTH);
                        array_wd_error = TRUE;
                    }
                }
                break;
                case RAID1:
                {
                    if (CalcStruct.ArrayWidth > DEFAULT_ARRAY_WIDTH_RAID1)
                    {
                        printf("\n Array width should be %d for Raid 1.\n",
                            DEFAULT_ARRAY_WIDTH_RAID1);
                        array_wd_error = TRUE;
                    }
                }
                break;
                case RAID10:
                {
                    if((CalcStruct.ArrayWidth < DEFAULT_ARRAY_WIDTH_RAID10 ) ||
                    ((CalcStruct.ArrayWidth % DEFAULT_ARRAY_WIDTH_RAID10 ) != 0))
                    {
                        printf("\n Array width should be greater than "
                                "or equal to %d and even for Raid 10.\n",
                                DEFAULT_ARRAY_WIDTH_RAID10 );
                        array_wd_error = TRUE;
                    }
                }
                break;
                case RAID3:
                {
                    if(!((CalcStruct.ArrayWidth == DEFAULT_ARRAY_WIDTH_RAID3_1) || 
                        (CalcStruct.ArrayWidth == DEFAULT_ARRAY_WIDTH_RAID3_2)))
                    {
                        printf("\n Array width should be equal "
                                "to %d or %d for Raid 3.\n",
                                DEFAULT_ARRAY_WIDTH_RAID3_1,
                                DEFAULT_ARRAY_WIDTH_RAID3_2);
                        array_wd_error = TRUE;
                    }
                }
                break;
                case IND_DISK:
                {
                    if (CalcStruct.ArrayWidth > DEFAULT_ARRAY_WIDTH_ID)
                    {
                        printf("\n Array width should be %d for individual disk.\n",
                                DEFAULT_ARRAY_WIDTH_ID);
                        array_wd_error = TRUE;
                    }
                }
                break;
                default :
                {
                    printf("\nInvalid Raid type.\n");
                }
                break;
            }
            /* Return from the main for an error.
            */
            if( array_wd_error == TRUE)
            {
                return;
            }
        }
        else
        {
            printf("\nInvalid argument. -w argument expected.\n");
            return;
        }
                                                            
        local_argv++;
        argc--;


        /* Check the argument for default elements per parity and 
        * set the CalcStruct.
        */
        if ((strcmp(*local_argv,"-default_elements_per_parity") == 0) ||
            (strcmp(*local_argv,"-def") == 0))
        {
            local_argv++;
            argc--;
            format_input = read_and_validate_input(*local_argv);
            if(format_input != INVALID_INPUT)
            {
                CalcStruct.elementsPerParityStripe = format_input;
                if(CalcStruct.elementsPerParityStripe < MIN_VALUE)
                {
                    printf("\nElements per parity stripe should be " 
                            "greater than or equal to 0x%x.\n",MIN_VALUE);
                    return;
                }
            }
            else
            {
                printf("\nInvalid -default_elements_per_parity argument value.\n");
                return;
            }
			elements_per_parity_stripe_input = TRUE;
        }
        else
        {
            printf("\nInvalid argument. " 
                "-default_elements_per_parity argument expected.\n");
            return;
        }

        local_argv++;
        argc--;

        /* This is the check for the optional arguments
        * as the align_lba and -ouput are optional argument.
        */
        if (argc != 0)
        {
            if (strcmp(*local_argv,"-align_lba") == 0)
            {
                local_argv++;
                argc--;
                format_input = read_and_validate_input(*local_argv);
                if(format_input != INVALID_INPUT)
                {
                    CalcStruct.AlignmentLba = format_input;
                }
                else
                {
                    printf("\nInvalid -align_lba argument value.\n");
                    return;
                }
            }
            else
            {
                printf("\nInvalid argument. -align_lba argument expected.\n");
                return;
            }
            /* Check for any more arguments entered by user 
             * which are not required. Print the error message.
             */
            local_argv++;
            argc--;
            if(argc != 0)
            {
                printf("\n%s invalid argument.\n", *local_argv);
                raid_cli_print_usage(cmd, usage_p);
               return;
            }
        }
   
    }
	if( !elements_per_parity_stripe_input )
	{
        calculate_elements_per_parity_stripe(CalcStruct);
	}
    display(CalcStruct);
    return;
}
/*****************************************
 * end raid_cli_x_set()
 *****************************************/

/****************************************************************
 * parse_cmd()
 ****************************************************************
 * 
 * DESCRIPTION:
 *   Parse the command to get the number of  arguments in the command and 
 *   return the count.
 *
 * PARAMETERS:
 *   command_buffer     (INPUT) NUMBER OF ARGUMENTS
 *   local_argv         (OUTPUT) ARRAY OF POINTERS TO ARGUMENTS
 *   
 * RETURNS 
 *    Number of arguments in command.
 *
 * HISTORY:
 *   04/11/2007  PP - created
 *****************************************************************/

UINT_32 parse_cmd(PCHAR command_buffer,PCHAR local_argv[])
{
    UINT_32 argc;
    PCHAR token_p;
    UINT_32 i;

    token_p = (PCHAR)malloc(TOKEN_SIZE);
    memset(token_p,0,TOKEN_SIZE);

    /* Parse the command line to get the argument count.
     */
    for (token_p = strtok(command_buffer, raidCalcCli_whitespace_p), argc = 0;
         token_p && (argc < MAX_ARGC - 1);
         token_p = strtok(NULL, raidCalcCli_whitespace_p), argc++)
    {
        local_argv[argc] = token_p;
    }
    local_argv[argc] = NULL;
    
    /* Print the error for arguments beyond limit(MAX_ARGC). 
     */ 
    if (token_p)
    {
        printf("%s: too many tokens on command\n", local_argv[0]);
        argc = 1;
    }

    free(token_p);
    /* Return the number of arguments in the command.
     */
    return (argc);
}
/*****************************************
 * end parse_cmd()
 *****************************************/

/**********************************************************************
 * RaidCalcCli_I()
 **********************************************************************
 *
 * DESCRIPTION:
 *   RaidCalcCli_I() is the main entry point for the Raid Calculator CLI
 *   interactive mode and the main loop for RaidCalcCli> that executes 
 *   each command.Will return when an exit is requested. 
 *
 * PARAMETERS
 *   None.
 *
 * RETURN VALUES
 *   None
 *
 * NOTES:
 *   
 * HISTORY:
 *   04/09/2007  PP - created
 **********************************************************************/

void RaidCalcCli_I(void)
{
    RAID_CLI_CMD_TBL_S *cmd_p;
    /* command line arguments.
     */
    PCHAR RaidCli_argv[MAX_ARGC];
    /* single command or looping. 
     */
    BOOL loop;
    /* name portion of path/name.typ of exe.
     */
    CHAR nameP[20]="RaidCalcCli";
    TEXT *help_option_p = "-h";
    BOOL cmd_found = FALSE;
    BOOL cmd_not_executed = TRUE;
    UINT_32 idx;
  
    /* If all we have is the file name then
     * prompt for commands and loop until ^C. */
    loop = TRUE;
    
    /* Initializes the Raid Calc Struct.
     */
    RaidCalcInit(CalcStruct);
    
    /* Prepare loop for collecting and processing commands.
     */
    do 
    {       
        /* if we are looping ask for a command.
         */
         printf ("\n%s> ", nameP);
        /* Clear the command buffer so that there is not 
         * change of previous command getting executed.
         */
        for(idx = 0;idx < CMD_BUF; idx++)
        {
            command_buffer[idx] = '\0';
         }
        /* Initially command found should be false.
         */
        cmd_found = FALSE;
        
        /* Get the command entered by the user.
         */
        CHAR cmd;
        UINT_32 idx=0;
        while(((cmd=getchar()) != '\n') && (idx < CMD_BUF-1))
        {
             command_buffer[idx]=cmd;
             idx++;
        }
        
        command_buffer[idx] ='\0';
        /* Flush the stdin as if command is too long,
         * it will remain in the buffer and start 
         * executiing considering it as a next command.
         */
        fflush(stdin);
                    
        UINT_32 count = parse_cmd(command_buffer,RaidCli_argv);
        
        if(count != 0)
        {
            cmd_not_executed = TRUE;
            /* Check the entered string with the commands in the command table.
             */
            for (cmd_p = &raid_cli_cmd_tbl_a[0];
                        cmd_p->execute_routine && cmd_not_executed ;
                        cmd_p++)
            {
                /* Check the entered string with the command 
                 * and command abbreviation.
                 */
                if(cmd_p->name_p == 0x0000)
					break;
                if (!strcmp(RaidCli_argv[0], cmd_p->name_p) ||
                    !strcmp(RaidCli_argv[0], cmd_p->name_a_p))
                {
                    cmd_found = TRUE;
                    /* Check for -h (help) option */
                    if ((count >= 2) && 
                        (strcmp(*(RaidCli_argv+1), help_option_p) == 0))
                    {
                        /* Print the help for the commad.
                         */
                        raid_cli_print_help(cmd_p->name_p,
                                            cmd_p->name_a_p, cmd_p->help_p);
                        /* Print the usage of the command.
                         */
                        raid_cli_print_usage(cmd_p->name_p,
                                                cmd_p->usage_p);
                    }
                    else
                    {
                        /* more than 2 arguments */
                        (cmd_p->execute_routine) (count, RaidCli_argv,
                                                    cmd_p->usage_p);
                    }
                    cmd_not_executed = FALSE;
                    break;
                }
            }
            /* Command not found. Print the error message.
            */
            if (cmd_found == FALSE)
            {
                printf("\nCommand \"%s\" NOT found\n", command_buffer);
            }
            printf("\n");
            
        }
        else
        {
            printf("\n");
            break;
        }
              
        /* Exit from the RaidCalcCli.
         */
        if (strcmp(command_buffer,"q") == 0) 
        {
            break ;
        }
   }
   while (loop);
   return;
}
/*****************************************
 * end RaidCalcCli_I()
 *****************************************/

/****************************************
 * end RaidCalcCli_I.cpp
 ****************************************/



