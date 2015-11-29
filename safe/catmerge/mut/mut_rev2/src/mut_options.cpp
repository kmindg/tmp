/*
 * mut_options.cpp
 *
 *  Created on: Sep 29, 2011
 *      Author: londhn
 */


#include "mut_options.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// help option
void mut_help_option::set(){


    printf("Running this program executes all tests\n");

    Options::show_all_help();
    printf("  [N] execute test number N\n");
    printf("  [N-M] execute tests numbered N through M\n");

    exit(0);
}

void mut_timeout_option::set(char *argv)
{
    Option::set(argv);  // needed for help & command line construction
    value = atoi(argv);

    if (value <= 0)
    {
        MUT_INTERNAL_ERROR(("Timeout should be positive number"))
    }
    return;
}

void mut_timeout_option::set(unsigned long timeout)
{
	value = timeout;
    return;
}

unsigned long mut_timeout_option::get(){
    return value;
}

void mut_iteration_option::set(char *argv){
    int arg;
    arg = atoi(argv);
    if (arg == 0)
    {
        _set(0); // endless iterations
    }
    else if(arg < 0)
    {
        MUT_INTERNAL_ERROR(("Iteration count must be 0 or larger ( %d)", arg))
    }
    else{
        _set(arg);
    }
}

void mut_abort_policy_option::set(char *argv){
	if(argv != NULL){
		value = argv;
	}else{
		value = "debug";
	}
}



