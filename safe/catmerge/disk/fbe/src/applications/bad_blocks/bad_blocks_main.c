#include <windows.h>
#include "fbe/fbe_disk_block_correct.h"
#include "fbe/fbe_emcutil_shell_include.h"

static void CSX_GX_RT_DEFCC fbe_bad_blocks_csx_cleanup_handler(csx_rt_proc_cleanup_context_t context);

HANDLE outFileHandle = NULL;
unsigned int outputMode;
char filename[FBE_BAD_BLOCKS_CONSOLE_MAX_CHAR];
char scriptFileName[FBE_BAD_BLOCKS_CONSOLE_MAX_CHAR];
char direct_command[FBE_BAD_BLOCKS_CONSOLE_MAX_CHAR];

void fbe_bad_blocks_print_usage(void);
void fbe_bad_blocks_get_command_args(int argc, char * argv[], fbe_bad_blocks_cmd_t * bb_command);
fbe_status_t fbe_bad_blocks_validate_command(fbe_bad_blocks_cmd_t * bb_command);

fbe_bad_blocks_cmd_t bb_command = {
    0,
    FBE_FALSE,
    NULL,
    0,
    0,
    "test",
};

/*!**************************************************************
 * main()
 ****************************************************************
 * @brief
 *  parse and execute the bad blocks command to clean uncorrectable
 *  sectors as specified in the event log
 *
 * @param 
 *      argc - number of input parameters
 *      argv - pointer to array of parameters
 *
 * @return None.   
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
int __cdecl main (int argc , char *argv[])
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

#include "fbe/fbe_emcutil_shell_maincode.h"

    // retrieve the bad blocks command
    fbe_bad_blocks_get_command_args(argc, argv, &bb_command);

    fbe_trace_init();
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_CRITICAL_ERROR);
    csx_rt_proc_cleanup_handler_register(fbe_bad_blocks_csx_cleanup_handler, fbe_bad_blocks_csx_cleanup_handler, NULL);

    // initialize the fbe api connections
    printf("\n\nFBE BadBlocks - Initializing...");
    status = fbe_disk_block_correct_initialize(&bb_command);
    if (status != FBE_STATUS_OK) {
        printf("Failed to init FBE API\n");
        return -1;
    }

    // execute the bad blocks command
    status = fbe_disk_block_correct_execute(&bb_command);

    csx_rt_proc_cleanup_handler_deregister(fbe_bad_blocks_csx_cleanup_handler);
    fbe_disk_block_correct_destroy_fbe_api(&bb_command);
    return 0;
}

/*!**************************************************************
 * fbe_bad_blocks_get_command_args()
 ****************************************************************
 * @brief
 *  get the bad blocks command and validate them
 *
 * @param 
 *      argc - number of input parameters
 *      argv - pointer to array of parameters
 *      bb_command - the bad blocks command structure to populate
 *
 * @return None.   
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
void fbe_bad_blocks_get_command_args(int argc, char * argv[], fbe_bad_blocks_cmd_t * bb_command)
{    
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    int currentArg = 1;
    char cmdLineChar = argv[currentArg][0];
    char * command = NULL;

    if (cmdLineChar == 's')
    {
       bb_command->is_sim = FBE_TRUE;
       currentArg++;
    } else if (cmdLineChar == 'h') {
        fbe_bad_blocks_print_usage();
        exit(1);
    }

    while (currentArg < argc)
    {
        printf("The argument is currentArg: %s+++\n", argv[currentArg]);
        command = argv[currentArg];
        if (strcmp (command, "-report") == 0) {
            bb_command->command |= FBE_BAD_BLOCKS_COMMAND_REPORT;
            printf("the value of command is %i", bb_command->command);
        } else if (strcmp (command, "-clean") == 0) {
            bb_command->command |= FBE_BAD_BLOCKS_COMMAND_CLEAN;
            printf("the value of command is %i", bb_command->command);
        } else if (strcmp (command, "-file") == 0) {
            currentArg++;
            bb_command->file = argv[currentArg];
            printf("the value of file is %s", bb_command->file);
        } else if (strcmp (command, "-startdate") == 0) {
            currentArg++;
            bb_command->startdate = fbe_disk_block_correct_string_to_date(argv[currentArg]);
        } else if (strcmp (command, "-enddate") == 0) {
            currentArg++;
            bb_command->enddate = fbe_disk_block_correct_string_to_date(argv[currentArg]);
        } else {
            fbe_bad_blocks_print_usage();
            exit(1);
        }
        currentArg++;
    }
    status = fbe_bad_blocks_validate_command(bb_command);
    if (status == FBE_STATUS_GENERIC_FAILURE) {
        exit(1);
    }
}

/*!**************************************************************
 * fbe_bad_blocks_validate_command()
 ****************************************************************
 * @brief
 *  Validate the bad blocks command parameters
 *
 * @param 
 *      bb_command - the bad blocks command structure to validate
 *
 * @return None.   
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t fbe_bad_blocks_validate_command(fbe_bad_blocks_cmd_t * bb_command) 
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    if ((bb_command->command & FBE_BAD_BLOCKS_COMMAND_CLEAN) && !(bb_command->command & FBE_BAD_BLOCKS_COMMAND_REPORT)) {
        if (bb_command->file == NULL) {
            printf("Please enter a valid filename");
            fbe_bad_blocks_print_usage();
            return status;
        }
    } else if (bb_command->command & FBE_BAD_BLOCKS_COMMAND_REPORT) {
        if (bb_command->file == NULL) {
            printf("Please enter a valid filename");
            fbe_bad_blocks_print_usage();
            return status;
        } else if (bb_command->startdate &&
                   bb_command->enddate) {
            if (bb_command->enddate < bb_command->startdate) {
                printf("Please enter valid start and end dates\n");
                return status;
            }
        }
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_bad_blocks_csx_cleanup_handler()
 ****************************************************************
 * @brief
 *  required function for cleanup
 *
 * @param 
 *
 * @return None.   
 *
 * @author 
 *
 ****************************************************************/
static void CSX_GX_RT_DEFCC fbe_bad_blocks_csx_cleanup_handler(csx_rt_proc_cleanup_context_t context)
{
    FBE_UNREFERENCED_PARAMETER(context);
    fbe_disk_block_correct_destroy_fbe_api(&bb_command);
}

/*!**************************************************************
 * fbe_bad_blocks_print_usage()
 ****************************************************************
 * @brief
 *  Print the usage of the bad blocks command
 *
 * @param None.
 *
 * @return None.   
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
void fbe_bad_blocks_print_usage(void)
{
    printf("Usage: badblocks [mode] \n\n");
    printf("          -report -file <path to local file> -startdate <mm/dd/yyyy>\n");
    printf("              -enddate <mm/dd/yyyy>\n\n");
	printf("          -clean -file <path to the local file>\n\n");
    printf("          -report -clean -file <path to local file> -startdate <mm/dd/yyyy>\n"); 
    printf("              -enddate <mm/dd/yyyy>\n\n");
    printf("       Where mode = s[simulator] h[help] Default is kernel\n");
    printf("             filename = Path to output file surrounded by quotes.  No space\n");
    printf("                        should be used between the output_type and filename.\n");
    printf("Examples:\n");
    printf("  'badblocks s -report -clean -file \"C:\\output.txt\"\n\n");
    printf("  'badblocks -report -file \"C:\\output.txt\"\n\n");

    return;
}


