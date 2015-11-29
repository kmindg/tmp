/***************************************************************************
 * Copyright (C) EMC Corporation 2007
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/


/***************************************************************************
 * RaidCalcCli.cpp
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the "main" function for the RaidCalcCli Application
 *  along with the supporting functions for the interface to the Raid 
 *  Calculator DLL.
 *
 * FUNCTIONS
 *  out_hex(RAID_CALC_STRUCT& CalcStruct,CALC_FLAG& calc_flag)
 *  out_dec(RAID_CALC_STRUCT& CalcStruct,CALC_FLAG& calc_flag)
 *  display_help()
 *  calc_logical_to_physical(RAID_CALC_STRUCT& CalcStruct)
 *  calc_physical_to_logical(RAID_CALC_STRUCT& CalcStruct)
 *  calculate_elements_per_parity_stripe(RAID_CALC_STRUCT& CalcStruct)
 *  determine_raid_type(PCHAR,RAID_CALC_STRUCT& CalcStruct)
 *  read_and_validate_input(PCHAR argv)
 *  validate_input(CONST PCHAR string)
 *  hex_to_long(CONST PCHAR String)
 *
 * NOTES
 *  None.
 *
 * HISTORY
 *  03/27/2007  Created. PP.
 *
 ***************************************************************************/

/************************
 * INCLUDE FILES
 ************************/

#include "stdafx.h"
#include "RaidCalcCli.h"

#pragma warning( disable : 4101)  
#pragma warning( disable : 4129)
#pragma warning( disable : 4244)

/****************************************
 * LOCAL FUNCTIONS
 ****************************************/

/*************************************************************************
 * display_help ()
 *************************************************************************
 *
 * DESCRIPTION:
 *   Displays help of all available switches and their 
 *   valid combinations.
 *
 * PARAMETERS:
 *   No Parameters.
 *
 * RETURNS 
 *   None
 *
 * HISTORY:
 *   03/27/2007  PP - created
 *************************************************************************/
void display_help()
{
    printf("\n\nRaidcalccli command can be used in "
            "script and interactive mode.");
    printf("\n\n[A] Interactive mode : ");
    printf("\n\n    Command: RaidCalcCli -i");
    printf("\n\n[B] Script mode : ");
    printf("\n\n    1. Conversion from lba to pba command:");
    printf("\n\n       RaidCalcCli -lba -r -elsz -addr_offset \n"
            " \b                    -w -default_elements_per_parity"
            " \b -align_lba -output ");
    printf("\n\n    2. Conversion from pba to lba command:");
    printf("\n \n       RaidCalcCli -pba -dpos -r -elsz -addr_offset \n"
            " \b                   -w -default_elements_per_parity "
            " \b -align_lba -output ");


    printf("\n\nDescription of switches:\n");
    printf("\n   -lba     <logical block address>");
    printf("\n   -pba     <physical block address>");
    printf("\n   -dpos    <data position in the array width>");
    printf("\n   -r       <R0 |R1 |R1_0 |R3 |R5 |R6 |ID>");
    printf("\n   -elsz    <element size>");
    printf("\n   -addr_offset  "
            "<address offset for the calculation>");
    printf("\n   -w       <array width>");
    printf("\n   -default_elements_per_parity  "   
            "<default elements per parity stripe> Optional");
    printf("\n   -align_lba  <alignment offset> Optional");
    printf("\n   -output  <hex|dec> Optional \n");

    printf("\n\nValues of the switches has to be provided in "
           "hexadecimal format.");
    printf("\n\n");
    return;
}
/************************
 * end display_help()
 ************************/

/***********************************************************************
 * determine_raid_type ()
 ************************************************************************
 *
 * DESCRIPTION:
 *  Determines the raid type from the input string.
 *
 * PARAMETERS:
 *  PCHAR raid_t -- Input string  
 *  
 * RETURNS 
 *  If input string is valid - Corresponding RAID_TYPE.
 *  If input string is invalid - INVALID_RAID_TYPE.
 *  
 * HISTORY:
 *  04/20/2007  PP - created
 *******************************************************************************/

void determine_raid_type(PCHAR raid_t,RAID_CALC_STRUCT& CalcStruct)
{
    RAID_TYPE index;
    /* Determines the raid type by comparing the input string 
     */
    for(index = RAID0; index < RAID_MAX_UNIT_TYPE ; index = (RAID_TYPE) (index + 1))
    {
        /* Compare the input string with raid type string.
         */
        if (strcmp(raid_t,raid_input_string[index]) == 0)
        {
            /* Check if Raid 6.
             */
            if(index == RAID6)
            {
                /* Set the parameters.
                 */
                CalcStruct.elementsPerParityStripe = 
                    RAID_DEFAULT_ELEMENTS_PER_PARITY_STRIPE_HAMMER;
                CalcStruct.platform = RAID_CALC_PIRANHA;
                CalcStruct.ArrayWidth = DEFAULT_ARRAY_WIDTH_RAID6;
            }
            /* set raid type.
             */
            CalcStruct.raidType = index;
            return;
        }
    }
    /* Match not found,set INVALID_RAID_TYPE.
     */
    CalcStruct.raidType = INVALID_RAID_TYPE;
    return;
}
/*****************************************
 * end determine_raid_type()
 *****************************************/
/****************************************************************
 * calc_logical_to_physical()
 ****************************************************************
 * 
 * DESCRIPTION:
 *   Converts the logical block address to physical block address
 *   using the information contained in the CalcStructure by calling 
 *   RaidCalcDll function.
 *
 * PARAMETERS:
 *   CalcStruct - ptr to CalcStructure.
 *
 * RETURNS 
 *    None.
 *
 * HISTORY:
 *   03/27/2007  PP - created
 *****************************************************************/

void calc_logical_to_physical(RAID_CALC_STRUCT& CalcStruct)
{
    /* Calls the raid calc dll function.
     */
    RaidCalcLogicalToPhysical(CalcStruct);
    return;
}
/*****************************************
 * end calc_logical_to_physical()
 *****************************************/

/****************************************************************
 * calc_physical_to_logical()
 ****************************************************************
 *
 * DESCRIPTION:
 *   Converts the physical block address to logical block address
 *   using the information contained in the CalcStructure by calling 
 *   RaidCalcDll function.
 *
 * PARAMETERS:
 *  CalcStruct - ptr to CalcStructure.
 *
 * RETURNS 
 *  None.
 *
 * HISTORY:
 *   03/27/2007  PP - created
 *****************************************************************/
void calc_physical_to_logical(RAID_CALC_STRUCT& CalcStruct)
{
    /* Calls the raid calc dll function.
     */
    RaidCalcPhysicalToLogical(CalcStruct);
    return;
}
/*****************************************
 * end calc_physical_to_logical()
 *****************************************/

/****************************************************************
 * out_hex()
 ****************************************************************
 * DESCRIPTION:
 *   Displays the output in the hexadecimal format.
 *
 * PARAMETERS:
 *  CalcStruct - ptr to CalcStructure.
 *  calc_flag - ptr to CALC_FLAG.
 *
 * RETURNS 
 *  None.
 *
 * HISTORY:
 *   03/27/2007  PP - created
 *****************************************************************/
void out_hex(RAID_CALC_STRUCT& CalcStruct,CALC_FLAG& calc_flag )
{
   RAID_TYPE raid_index = RAID5;
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

   switch(calc_flag)
   {
       /* Prints the LBA to PBA conversion. 
        */
       case LOG_TO_PHYS:
           {
               printf("\n Logical block address : 0x%llX", CalcStruct.Lba);
               printf("\n\nLogical to physical Calculations:\n");
               printf("\n Physical block address: 0x%llX", CalcStruct.Pba);
           }
           break;
        /* Prints the PBA to LBA conversion. 
         */
       case PHYS_TO_LOG:
           {
               printf("\n Physical block address: 0x%llX", CalcStruct.Pba);
               printf("\n\nPhysical to Logical calculations:\n");
               printf("\n Logical block address: 0x%llX", CalcStruct.Lba);
           }
           break;
       default:
           {
               printf("\nCalculation flag is not set properly.\n");
           }
           break;
   }
    /* Prints the output information according to Raid type.
     * For example, for Raid 1 and Raid 10 - primary and secondary positions,
     * for Raid 3/5/6 - Data and parity position, etc.
     */
    if((CalcStruct.raidType == RAID1) || (CalcStruct.raidType == RAID10))
    {
        printf("\n Primary position: 0x%X", CalcStruct.dataPos);
        printf("\n Secondary position: 0x%X", CalcStruct.parityPos);
    }
    else
    {
        if ((CalcStruct.raidType == RAID3) || (CalcStruct.raidType == RAID5) ||
            (CalcStruct.raidType == RAID6) || (CalcStruct.raidType == IND_DISK) ||
            (CalcStruct.raidType == RAID0))
        {
            printf("\n Data position: 0x%X", CalcStruct.dataPos);
            
            if ((CalcStruct.raidType == RAID3) || (CalcStruct.raidType == RAID5) ||
                (CalcStruct.raidType == RAID6))
            {
                printf("\n Parity position: 0x%X",CalcStruct.parityPos);
            }
            if(CalcStruct.raidType == RAID6)
            {
                printf("\n Diag position: 0x%X",CalcStruct.parityPos2);
            }
        }
    }
    printf("\n");
    return;
}
/*****************************************
 * end out_hex()
 *****************************************/
/****************************************************************
 * out_dec()
 ****************************************************************
 * DESCRIPTION:
 *   Displays the output in the decimal format.
 *
 * PARAMETERS:
 *  CalcStruct - ptr to CalcStructure.
 *  calc_flag - ptr to CALC_FLAG.
 *
 * RETURNS 
 *  None.
 *
 * HISTORY:
 *   03/27/2007  PP - created
 *****************************************************************/

void out_dec(RAID_CALC_STRUCT& CalcStruct,CALC_FLAG& calc_flag )
{
   RAID_TYPE raid_index = RAID5;
   CHAR local_raid_type[RAID_TYPE_STR_SIZE];
   raid_index = CalcStruct.raidType;

   if((raid_index < 0 ) || (raid_index > RAID_MAX_UNIT_TYPE ))
   {
       printf("\nInvalid Raid Type.\n");
       return;
   }
   else
   {
       /* Determine the raid type from the CalcStruct.
        */
       strcpy(local_raid_type, raid_output_string[raid_index]);
   }
   
   /* Prints the Raid calculator Geometry.
    */
    printf("\n \nRaid Calculator Geometry\n");
    printf("\n Raid type:%s ", local_raid_type);
    printf("\n Element Size: %d", CalcStruct.sectorsPerElement);
    printf("\n Address Offset: %llu", CalcStruct.AddressOffset);
    printf("\n Array Width: %d", CalcStruct.ArrayWidth);
    printf("\n Alignment Lba: %d", (int)CalcStruct.AlignmentLba);
    printf("\n Elements per parity stripe: %u",
               CalcStruct.elementsPerParityStripe);

    switch(calc_flag)
    {
        /* Prints the LBA to PBA conversion. 
         */
        case LOG_TO_PHYS:
            {
                printf("\n Logical block address : %llu", CalcStruct.Lba);
                printf("\n\nLogical to physical Calculation\n");
                printf("\n Physical Block Address: %llu", CalcStruct.Pba);
            }
            break;
        /* Prints the PBA to LBA conversion. 
         */
        case PHYS_TO_LOG:
            {
                printf("\n Physical Block Address: %llu", CalcStruct.Pba);
                printf("\n\nPhysical to Logical Calculation\n");
                printf("\n Logical Block Address: %llu", CalcStruct.Lba);
            }
            break;
        default:
            {
                printf("\nCalculation flag is not set properly.\n");
            }
            break;
    }
    /* Prints the output information according to Raid type.
     * For example, for Raid 1 and Raid 10 - primary and secondary positions,
     * for Raid 3/5/6 - Data and parity position,etc.
     */
    if((CalcStruct.raidType == RAID1) || (CalcStruct.raidType == RAID10))
    {
        printf("\n Primary Position: %d", CalcStruct.dataPos);
        printf("\n Secondary Position: %d", CalcStruct.parityPos);
    }
    else
    {
        if ((CalcStruct.raidType == RAID3) || (CalcStruct.raidType == RAID5) ||
            (CalcStruct.raidType == RAID6) || (CalcStruct.raidType == IND_DISK) ||
            (CalcStruct.raidType == RAID0))
        {
            printf("\n Data Position: %d", CalcStruct.dataPos);
            
            if ((CalcStruct.raidType == RAID3) || (CalcStruct.raidType == RAID5) ||
                (CalcStruct.raidType == RAID6))
            {
                printf("\n Parity Position: %d", CalcStruct.parityPos);
            }
            if(CalcStruct.raidType == RAID6)
            {
                printf("\n Diag Position: %d", CalcStruct.parityPos2);
            }
        }
    }
    printf("\n");
    return;
}
/*****************************************
 * end out_dec()
 *****************************************/
/****************************************************************
 * calculate_elements_per_parity_stripe()
 ****************************************************************
 * 
 * DESCRIPTION:
 *  Calculates the elements per parity stripe.
 *
 * PARAMETERS:
 *  CalcStruct - ptr to CalcStructure.
 *  
 * RETURNS:
 *   None.
 *
 * HISTORY:
 *   03/27/2007  PP - created
 ****************************************************************/

void calculate_elements_per_parity_stripe(RAID_CALC_STRUCT& CalcStruct)
{
   
	/* Initialize to to a reasonable value */
	  CalcStruct.elementsPerParityStripe = 1;
	
    /* Only for the RAID6 the platform is RAID_CALC_PIRANHA
     * and for RAID5 it may be RAID_CALC_CX or RAID_CALC_LONGBOW.
     * For each platform value, go ahead and determine 
     * what the correct sectors per element and elements per parity stripe are.
     */
                
    if ( CalcStruct.platform == RAID_CALC_LONGBOW ||
         CalcStruct.platform == RAID_CALC_CX )
    {
        default_sectors = RAID_DEFAULT_SECTORS_PER_ELEMENT;
        CalcStruct.elementsPerParityStripe = 
            RAID_DEFAULT_ELEMENTS_PER_PARITY_STRIPE_CX;
    }
    else if ( CalcStruct.platform == RAID_CALC_PIRANHA )
    {
        default_sectors = RAID_DEFAULT_SECTORS_PER_ELEMENT_PIRANHA;
        CalcStruct.elementsPerParityStripe = 
            RAID_DEFAULT_ELEMENTS_PER_PARITY_STRIPE_PIRANHA;
    }

    if (CalcStruct.sectorsPerElement < default_sectors)
    {
        /* If our sectors per element is less than 
         * the default (Morley parity size),
         * then we have more than one elements per parity stripe.
         */
         CalcStruct.elementsPerParityStripe =
             (default_sectors / CalcStruct.sectorsPerElement) +
             ((default_sectors % CalcStruct.sectorsPerElement) ? 1 : 0);
    }
    else if (CalcStruct.sectorsPerElement >= default_sectors)
    {
        /* If our sectors per element is greater than 
         * the default (Morley parity size),
         * then we will fix the elements per parity stripe at 1.
         */
         CalcStruct.elementsPerParityStripe = 1;
    }/*end of if (CalcStruct.sectorsPerElement <  default...)-else */
    return;
}
/************************************************
 * end calculate_elements_per_parity_stripe()
 ************************************************/

/****************************************************************
 * validate_input()
 ****************************************************************
 * 
 * DESCRIPTION:
 *  Validates the input whether it is valid hex string or not. 
 *
 * PARAMETERS:
 *  string - const ptr to string.
 *  
 * RETURNS:
 *   TRUE: Input string is a valid integer.
 *   FALSE: Input string is not a valid integer.
 *
 * HISTORY:
 *   04/09/2007  PP - created
 ****************************************************************/
BOOL validate_input(CONST PCHAR string)
{
    UINT_32 index;
    BOOL valid = TRUE;
    /* Check every character in the input string.
     */
    for (index = 0; index < strlen(string); index++)
    {
        /* Check whether character is a valid hex or not.
         */
        if (!CHAR_HEX_VALID(string[index]))
        {
            valid = FALSE;
            break;
        }
    }
    return valid;
}
/************************************************
 * end validate_input()
 ************************************************/
/****************************************************************
 * read_and_validate_input()
 ****************************************************************
 * 
 * DESCRIPTION:
 *  Read and Validates the input whether it is valid hex string or not,
 *  by calling the validate_input().
 *
 * PARAMETERS:
 *  argv - input string.
 *  
 * RETURNS:
 *   Validated input value.
 *   If input value is not valid then return the 0xffffffff invalid value 
 *
 * HISTORY:
 *   04/20/2007  PP - created
 ****************************************************************/
ULONGLONG read_and_validate_input(PCHAR fun_argv)
{
    CHAR input_array[INPUT_ARRAY_SIZE];
    ULONGLONG input = 0;

    /* The integer encoded by the input string should fit into a
     * ULONGLONG (8 bytes).  Since an 8 byte integer can be represented
     * as 16 hex digits, the maximum string length should be 16.
     */
    if(strlen(fun_argv) > INPUT_MAX_STRLEN)
    {
        return INVALID_INPUT;
    }

    /* Read the input string into the conversion buffer.
     * This is larger than INPUT_MAX_STRLEN, as we need to 
     * allow for a string terminator.
     */
    strcpy(input_array, fun_argv);

    /* If an input string is valid hex string then only return the value,
     * otherwise return INVALID_INPUT.
     */
    if(validate_input(input_array))
    {
        /* Get the long value equivalent to hex string.
         */
        input = hex_to_long(input_array);
        return input;
    }
    else
    {
        return INVALID_INPUT;
    }
}
/************************************************
 * end read_and_validate_input()
 ************************************************/

/****************************************************************
 * hex_to_long()
 ****************************************************************
 * 
 * DESCRIPTION:
 *  string_to_hex - converts the hex string into long. 
 *
 * PARAMETERS:
 *  String - input hex string.
 *  
 * RETURNS:
 *   Equivalent long value.
 *    
 * HISTORY:
 *   05/23/2007  PP - created
 ****************************************************************/

ULONGLONG hex_to_long (CONST PCHAR String)
{
    CHAR Buffer[BUFFER_SIZE];
    PCHAR pBuffer;
    ULONGLONG num = 0, neg = 0;
    ULONGLONG len = 0;
    
    /* Get the length of the string.
     */
    len = strlen(String);
    
    if (len >= BUFFER_SIZE)
    {
        strncpy( Buffer, String, 19 );
        Buffer[19] = '\0';
    }
    else
    {
        strcpy(Buffer, String);
        Buffer[len] = '\0';
    }

    pBuffer = Buffer;

    if (*pBuffer == '-')
    {
        pBuffer++, neg++;
    }
    
    /* Calculate the number which is equivalent hex string.
     */
    while(pBuffer && *pBuffer != '\0' && *pBuffer != ' ')
    {
        if (*pBuffer >= '0' && *pBuffer <= '9')
        {
            num = (num * 16) + (*pBuffer++ - '0');
        }
        else
        {
            if (*pBuffer >= 'a' && *pBuffer <= 'f')
            {
                num = (num * 16) + ((*pBuffer++ - 'a') + 10);
            }
            else
            {
                if (*pBuffer >= 'A' && *pBuffer <= 'F')
                {
                    num = (num * 16) + ((*pBuffer++ - 'A') + 10);
                }
                else
                {
                    return(0);
                }
            }
        }
    }

    if (neg)
    {
        num = 0 - num;
    }
    
    return(num);
} 
/************************************************
 * end hex_to_long()
 ************************************************/

/****************************************************************
 * main()
 ****************************************************************
 * DESCRIPTION:
 *  This is the main entry point for the Raid Calc Cli. This function calls 
 *  the appropriate functions to get the desired output.
 *
 * PARAMETERS:
 *  argc     (INPUT) Number of Arguments.
 *  argv     (INPUT) Argument List.
 *  
 * RETURNS:
 *   None.
 *
 * HISTORY:
 *   03/27/2007  PP - created
 ****************************************************************/

int main(int argc, char *argv[])
{
    CHAR main_raid_type[RAID_TYPE_STR_SIZE];
    CHAR output[OUTPUT_STR_SIZE]; 
    ULONGLONG format_input;
    RAID_TYPE raid_index;
    OUTPUT_FORMAT output_index = HEX;
    CALC_FLAG calc_index;
	BOOL elements_per_parity_stripe_input = FALSE;
          
    /* Initializes the Raid Calc Struct.
     */
    RaidCalcInit(CalcStruct);
    /* Calculate the elements per parity stripe.
     */
    calculate_elements_per_parity_stripe(CalcStruct);
   
    /* If this is the only argument then display help.
     */
    if ((argc == 1 ) || ((strcmp(*(argv + 1),"-help") == 0)))
    {
        display_help();
        return 1;
    }
   
    argv++;
    argc--;

    /* This is to enter into interactive mode.
     */
    if (strcmp(*argv,"-i") == 0)
    {
        /* This will have the functionality for the
         * interactive mode.
         */
        RaidCalcCli_I();
        return 1;
    }
    
    if (argc < 10)
    {
        printf("\nToo few arguments.\n");
        printf("Expected 10 arguments.\n");
        display_help();
        return 1;
    }
    UINT_32 count = 0;
    UINT_32 argcount = argc;
    for(count = 0; count < argcount; count++)
    {
       /* Check for the valid combination of the arguments.
        */
        if(((strcmp(argv[count],"-lba") == 0) && (strcmp(argv[count+2],"-pba") == 0)) ||
            ((strcmp(argv[count],"-lba") == 0) && (strcmp(argv[count+2],"-dpos") == 0)) ||
            ((strcmp(argv[count],"-pba") == 0) && (strcmp(argv[count+2],"-lba") == 0)))
        {
            /* If any mismatch occurs, display  
             * invalid arugument combination.
             */
            printf("\nInvalid argument combination.\n");
            display_help();
            return 1;
        }
    }
    /* Parse all arguments and set the corresponding 
     * field in the CalcStruct for further processing.
     */
    
    /* Check the argument for lba and set the CalcStructure.
     */
    if (strcmp(*argv,"-lba") == 0)
    {
        /* Sets the flag to true for the Lba to pba conversion.
         */
        calc_flag = LOG_TO_PHYS;
        argv++;
        argc--;

        format_input = read_and_validate_input(*argv);
        if(format_input != INVALID_INPUT)
        {
            CalcStruct.Lba = format_input;
            if(CalcStruct.Lba > MAX_LBA)
            {
                printf("\nLBA should be less than 0x%X\n", (unsigned int)MAX_LBA);
                return 1;
            }
        }
        else
        {
            printf("\nInvalid -lba argument value.\n");
            return 1;
        }
    }
    /* if the argument is not lba then check for the pba.
     */
    else
    {
        if (strcmp(*argv,"-pba") == 0)
        {
            calc_flag = PHYS_TO_LOG;
            /* This is to check whether all the arguments 
             * are present or not.
             */
            if(argc < 13)
            {
                printf("\n Too few arguments.\n");
                display_help();
                return 1;
            }

            argv++;
            argc--;
            
            format_input = read_and_validate_input(*argv);
            if(format_input != INVALID_INPUT)
            {
                CalcStruct.Pba = format_input;
                if(CalcStruct.Pba > MAX_PBA)
                {
                    printf("\nPBA should be less than 0x%X\n", (unsigned int)MAX_PBA);
                    return 1;
                }
            }
            else
            {
                printf("\nInvalid -pba argument value.\n");
                return 1;
            }
            
        }
        else
        {
            printf("\nInvalid argument. -lba/pba argument expected.\n");
            return 1;
        }

        argv++;
        argc--;

        /* Check the argument for data position and set the CalcStructure.
         */
        if (strcmp(*argv,"-dpos") == 0)
        {
            argv++;
            argc--;
            
            format_input = read_and_validate_input(*argv);
            if(format_input != INVALID_INPUT)
            {
                CalcStruct.dataPos = format_input;
            }
            else
            {
                printf("\nInvalid -dpos argument value.\n");
                return 1;
            }
        }
        else
        {
            printf("\nInvalid argument. -dpos argument expected.\n");
            return 1;
        }
    }

    argv++;
    argc--;

    /* Check the argument for raid type and set the CalcStructure.
     */
    if (strcmp(*argv,"-r") == 0)
    {
        argv++;
        argc--;
        strcpy(main_raid_type, *argv);
        /* Determine the raid type from the input string.
         */
        determine_raid_type(main_raid_type,CalcStruct);
        /* Check for the valid raid type. 
         */
        if(CalcStruct.raidType == INVALID_RAID_TYPE)
        {
            printf("\nInvalid Raid Type.\n");
            return 1;
        } 
    }
    else
    {
        printf("\nInvalid argument. -r argument expected.\n");
        return 1;
    }

    argv++;
    argc--;

    /* Check the argument for element size and set the CalcStructure.
     */
    if (strcmp(*argv,"-elsz") == 0)
    {
        argv++;
        argc--;
        format_input = read_and_validate_input(*argv);
        if(format_input != INVALID_INPUT)
        {
            if((format_input > MAX_ELSZ)||(format_input < MIN_VALUE))
            {
               printf("\nElement size should be greater than or equal to  0x%x " 
                        "and less than or equal to 0x%x\n",MIN_VALUE,MAX_ELSZ); 
               return 1;
            }
            CalcStruct.sectorsPerElement = format_input;
        }
        else
        {
            printf("\nInvalid -elsz argument value.\n");
            return 1;
        }
        /* Display the error message for the invalid element size.
         */
        if (((CalcStruct.raidType == RAID1) || (CalcStruct.raidType == IND_DISK)) &&
            (CalcStruct.sectorsPerElement != DEFAULT_SECTORS_PER_ELEMENT))
        {
            printf("\nElement size should be 0x%X.\n",DEFAULT_SECTORS_PER_ELEMENT);
            return 1;
        }
       
    }
    else
    {
        printf("\nInvalid argument. -elsz argument expected.\n");
        return 1;
    }

    argv++;
    argc--;

    /* Check the argument for address offset and set the CalcStructure.
     */
    if (strcmp(*argv,"-addr_offset") == 0)
    {
        argv++;
        argc--;
        format_input = read_and_validate_input(*argv);
        if(format_input > MAX_ADDR_OFFSET)
        {
            printf("\nAddress offset should be less than or equal to %x\n",
                    (unsigned int)MAX_ADDR_OFFSET);
            return 1;
        }
        if(format_input != INVALID_INPUT)
        {
            CalcStruct.AddressOffset = format_input;
            
        }
        else
        {
            printf("\nInvalid -addr_offset argument value.\n");
            return 1;
        }
        /* Check whether pba is greater than address offset or not.
         */
        if((calc_flag == PHYS_TO_LOG) &&
            (CalcStruct.Pba < CalcStruct.AddressOffset))
        {
            printf("\npba should not be less than address offset.\n");
            return 1;
        }
    }
    else
    {
        printf("\nInvalid argument. -addr_offset argument expected.\n");
        return 1;
    }

    argv++;
    argc--;

    /* Check the argument for array width and set the CalcStructure.
     */
    if (strcmp(*argv,"-w") == 0)
    {
        argv++;
        argc--;
        format_input = read_and_validate_input(*argv);
        if(format_input != INVALID_INPUT)
        {
            CalcStruct.ArrayWidth = format_input;
        }
        else
        {
            printf("\n Invalid -w argument value.\n");
            return 1;
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
                    printf("\nArray width should be greater than "
                            "or euqal to %d and less than %d "
                            "for Raid 0 ,Raid 5 and Raid 6.\n",
                            MIN_ARRAY_WIDTH, MAX_ARRAY_WIDTH);
                    array_wd_error = TRUE;
                }
            }
            break;
            case RAID1:
            {
                if (CalcStruct.ArrayWidth != DEFAULT_ARRAY_WIDTH_RAID1)
                {
                    printf("\nArray width should be %d for Raid 1.\n",
                        DEFAULT_ARRAY_WIDTH_RAID1);
                    array_wd_error = TRUE;
                }
            }
            break;
            case RAID10:
            {
                if((CalcStruct.ArrayWidth < DEFAULT_ARRAY_WIDTH_RAID10 ) ||
                ((CalcStruct.ArrayWidth % DEFAULT_ARRAY_WIDTH_RAID10 ) != 0)||
                (CalcStruct.ArrayWidth > MAX_ARRAY_WIDTH))
                {
                    printf("\nArray width should be greater than "
                           "or equal to %d ,less than %d and even for Raid 10.\n",
                           DEFAULT_ARRAY_WIDTH_RAID10, MAX_ARRAY_WIDTH );
                    array_wd_error = TRUE;
                }
            }
            break;
            case RAID3:
            {
                if(!((CalcStruct.ArrayWidth == DEFAULT_ARRAY_WIDTH_RAID3_1) || 
                    (CalcStruct.ArrayWidth == DEFAULT_ARRAY_WIDTH_RAID3_2)))
                {
                    printf("\nArray width should be equal "
                            "to %d or %d for Raid 3.\n",
                            DEFAULT_ARRAY_WIDTH_RAID3_1,
                            DEFAULT_ARRAY_WIDTH_RAID3_2);
                    array_wd_error = TRUE;
                }
            }
            break;
            case IND_DISK:
            {
                if (CalcStruct.ArrayWidth != DEFAULT_ARRAY_WIDTH_ID)
                {
                    printf("\nArray width should be %d for individual disk.\n",
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
            return 1;
        }
        if(CalcStruct.dataPos >= CalcStruct.ArrayWidth)
        {
            printf("\nData position should be less than array width.\n");
            return 1;
        }
    }
    else
    {
        printf("\nInvalid argument. -w argument expected.\n");
        return 1;
    }
                                                        
    argv++;
    argc--;

     /* This is the check for the optional arguments
     * as -default_elements_per_parity, -align_lba and 
	 * -ouput are optional arguments.
     */

	 /* Check the arguments for default elements per parity and 
     * set the CalcStructure.  This argument is optional.
     */
    if (argc != 0)
    {
    if ((strcmp(*argv,"-default_elements_per_parity") == 0) ||
        (strcmp(*argv,"-def") == 0))
    {
        argv++;
        argc--;
        format_input = read_and_validate_input(*argv);
        if(format_input != INVALID_INPUT)
        {
            CalcStruct.elementsPerParityStripe = format_input;
            if(CalcStruct.elementsPerParityStripe < MIN_VALUE)
            {
                printf("\nElements per parity stripe should be" 
                        "greater than or equal to 0x%x.\n",MIN_VALUE);
                return 1;
            }
        }
        else
        {
            printf("\n Invalid -default_elements_per_parity argument value.\n");
            return 1;
        }
		   elements_per_parity_stripe_input = TRUE;
		   argv++;
           argc--;
		}
    }

    if (argc != 0)
    {
        if ((strcmp(*argv,"-align_lba") == 0) || (strcmp(*argv,"-output") == 0))
        {
            if(strcmp(*argv,"-align_lba") == 0)
            {
                argv++;
                argc--;
                format_input = read_and_validate_input(*argv);
                if(format_input != INVALID_INPUT)
                {
                    CalcStruct.AlignmentLba = format_input;
                }
                else
                {
                    printf("\n Invalid -align_lba argument value.\n");
                    return 1;
                }
                argv++;
                argc--;
            }
            if(argc != 0)
            {
                if (strcmp(*argv,"-output") == 0)
                {
                    argv++;
                    argc--;
                    strcpy(output, *argv);
                    /* Check for the output format and 
                    * set the flag as per the requirement.
                    */
                    if (strcmp(output,"hex") == 0)
                    {
                        output_form = HEX ;
                    }
                    else
                    {
                        if (strcmp(output,"dec") == 0)
                        {
                            output_form = DEC;
                        }
                        else
                        {
                            printf("\nInvalid -output argument.\n");
                            return 1;
                        }
                    }
                }
                else
                {
                    printf("\nInvalid argument. -align_lba/-output argument expected.\n");
                    return 1;
                }
            }
                //printf("\nInvalid argument. -align_lba argument expected.\n");
                //return 1;
            //}

            
        }
    }
        /* Check for any more arguments entered by user 
         * which are not required. Print the error message.
         */
        if(argc != 0)
        {
            argv++;
            argc--;
        
            if(argc != 0)
            {
                printf("\n%s invalid argument.\n", *argv);
                return 1;
            }
        }

    /* If necessary, calculate the elements per parity stripe.
     */
    if( !elements_per_parity_stripe_input )
	{
         calculate_elements_per_parity_stripe(CalcStruct);
	}

    /* Calls the appropriate function for lba to pba conversion or vice versa.
     */
    calc_index = calc_flag;
    switch(calc_index)
    {
        case LOG_TO_PHYS:
            {
                /* Call for the lba to pba conversion.
                 */
                calc_logical_to_physical(CalcStruct);
            }
            break;
        case PHYS_TO_LOG:
            {
                /* Call for the pba to lba conversion.
                 */
                calc_physical_to_logical(CalcStruct);
            }
            break;
        default:
            {
                printf("\nInvalid Flag for calculation.\n");
            }
            break;
    }
    

    /* Display the output as per flag setting.
     */
    output_index = output_form;
    switch(output_index)
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

    return 0;
}
/*****************************************
 * end main()
 *****************************************/
/****************************************
 * end RaidCalcCli.cpp
 ****************************************/


