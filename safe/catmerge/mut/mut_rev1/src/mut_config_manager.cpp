/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_config_manager.cpp
 ***************************************************************************
 *
 * DESCRIPTION: MUT Framework configuration manager 
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    9/2/2010 redone for CPP version
 **************************************************************************/
#include "mut.h"
#include "mut_private.h"
#include "mut_config_manager.h"
#include "mut_log_private.h"
#include "mut_test_control.h"
#include "mut_options.h"

#include <stdio.h>
#include <string.h>
//#include <ctype.h>
#include <windows.h>
#include <vector>
using namespace std;


/* name of executable we are currently running */
static char *mut_program = NULL;
static char *mut_cmdLine = NULL;
static char *mut_exePath = NULL;

/** \fn char mut_get_cmdLine()
 * return the name/path to the current executable
 */
MUT_API char * mut_get_cmdLine()
{
    return mut_cmdLine;
}

/** \fn char mut_get_program_name()
 * return the name/path to the current executable
 */
MUT_API char *mut_get_program_name()
{
    return mut_program;
}

/** \fn char mut_get_exePath()
 * return the path to the current executable
 */
MUT_API char * mut_get_exePath()
{
    return mut_exePath;
}

static BOOL containsSpace(char *arg) {
    char *ptr = strchr(arg, ' ');
    return (ptr != NULL);
}

static unsigned int argLength(char *arg) {
    unsigned int len = (unsigned int)strlen(arg) + 1;  // add 1 for a space delimiter

    if(containsSpace(arg)) {
        len += 2;
    }

    return len;
}

static void delimit(char *buffer, char *arg) {
    if(containsSpace(arg)) {
        strcat(buffer, "\"");
        strcat(buffer, arg);
        strcat(buffer, "\"");
    }
    else {
        strcat(buffer, arg);
    }

    // append space between arguments
    strcat(mut_cmdLine, " ");
}

static void mut_save_cmdLine(int argc, char **argv) {
    unsigned int length;
    int index;
    char **ptr;


    for(ptr = argv, length = 0, index = 0; index < argc;index++, ptr++) {
        length += argLength(*ptr);
    }

    length++;  // include room for string terminator

    mut_cmdLine = (char*)malloc(length);
    csx_p_memzero(mut_cmdLine, length);

    for(ptr = argv, index = 0; index < argc; index++, ptr++) {
        delimit(mut_cmdLine, *ptr);
    }

}


void Mut_config_manager::mut_parse_options(){
	if (*cmd_line_argv != NULL)
    {
	    int index;
        mut_program = *cmd_line_argv;

        // see if an executable path was specified.
        for(index = (int) strlen(mut_program); index >= 0; index--) {
#ifdef C4_INTEGRATED
            if(mut_program[index] != '/') {
#else //C4_INTEGRATED
            if(mut_program[index] != '\\') {
#endif //C4_INTEGRATED
                continue;
            }
            break;
        }

        if(index >= 0) {
            mut_exePath = (char*) malloc(index+1);
            csx_p_memzero(mut_exePath, index+1);
            for(int count = 0; count < index; count++) {
                mut_exePath[count] = mut_program[count];
            }
        } else {
            csx_size_t          path_len;
            csx_status_e        csx_status;

            // Allocate a big enough string to build the path
            mut_exePath = (char*) malloc(1024);
            csx_p_memzero(mut_exePath, 1024);

            // Get the current working directory for the executable.
            csx_status = csx_p_native_working_directory_get(mut_exePath, 1024, &path_len);
            if (!CSX_SUCCESS(csx_status)) {
                MUT_INTERNAL_ERROR(("Unable to get working directory"))
            }
        }
    }

	/* increment onto the next arguments */
    --cmd_line_argc;
    ++cmd_line_argv;

	mut_save_cmdLine(cmd_line_argc, cmd_line_argv);

    /* now we restrict that the tests range must be the first argument if specified */
    if (cmd_line_argc > 0 && set_option_tests_range(cmd_line_argv))
    {
        /* increment onto the next arguments */
        --cmd_line_argc;
        ++cmd_line_argv;
    }
}


Mut_config_manager::Mut_config_manager(int argc, char** argv)
{
    // initialize members start and end
    start = end = -1;
    //house keeping
    cmd_line_argc = argc;
    cmd_line_argv = argv;

    mut_parse_options();
}


// parse 'N' or 'N-M' style test range
bool Mut_config_manager::set_option_tests_range(char **argv)
{
    char *ptr =  strchr(*argv, '-');
    if (!isdigit(**argv))
    {
        return 0;
    }

    if(ptr != NULL)
    {
        /*
         * user specified a range
         */ 
        int len = (int)(ptr - *argv) + 1;
        char *_start = (char *)malloc(len); 
        *_start = '\0';
        strncpy(_start, *argv, len - 1);
        *(_start+len-1) = '\0';
        start = atoi(_start);
        free(_start);
        ptr++;
        end = atoi(ptr);
        
        if (start > end)
        {
            MUT_INTERNAL_ERROR(("Start test number is greater than end test number"))
        }
        
    }
    else
    {
        /*
         * user specified a single number
         */
        start = atoi(*argv);
        end = atoi(*argv);
    }

	return 1;
}
