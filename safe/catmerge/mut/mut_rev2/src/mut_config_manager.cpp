/*
 * mut_config_manager.cpp
 *
 *  Created on: Sep 21, 2011
 *      Author: londhn
 */
#include "mut.h"
#include "mut_private.h"
#include "mut_config_manager.h"
#include "mut_test_control.h"
#include "mut_options.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <ctype.h>

#include "EmcPAL_Memory.h"

using namespace std;

extern Mut_test_control *mut_control;

/* name of executable we are currently running */
static char *mut_program = NULL;
static char *mut_cmdLine = NULL;

/** \fn char mut_get_cmdLine()
 * return the name/path to the current executable
 */
char *   mut_get_cmdLine()
{
    return mut_cmdLine;
}

/** \fn char mut_get_program_name()
 * return the name/path to the current executable
 */
char *  mut_get_program_name()
{
    return mut_program;
}

static int containsSpace(char *arg) {
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

//static void MutZeroMemory(void *dest,unsigned int len){
//	bzero(dest,len);
//}

static void mut_save_cmdLine(int argc, char **argv) {
    unsigned int length;
    int index;
    char **ptr;


    for(ptr = argv, length = 0, index = 0; index < argc;index++, ptr++) {
        length += argLength(*ptr);
    }

    length++;  // include room for string terminator

    mut_cmdLine = (char*)malloc(length);
    RtlZeroMemory(mut_cmdLine, length);

    for(ptr = argv, index = 0; index < argc; index++, ptr++) {
        delimit(mut_cmdLine, *ptr);
    }

}

void Mut_config_manager::mut_parse_options(){
	if (*cmd_line_argv != NULL)
    {
        mut_program = *cmd_line_argv;
    }

    /* increment onto the next arguments */
    --cmd_line_argc;
    ++cmd_line_argv;

    mut_save_cmdLine(cmd_line_argc, cmd_line_argv);
}

Mut_config_manager::Mut_config_manager(int argc, char **argv)
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
			mut_control->mut_log->log_message("Start test number is greater than end test number");
			return 1;
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
