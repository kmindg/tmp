/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_options.cpp
 ***************************************************************************
 *
 * DESCRIPTION: Implementation for (most) options used by mut framework
 *      exceptions would be related to specific other functionality like mut_log
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    10/2010 Created JD
 **************************************************************************/


#include "mut_options.h"

#include "simulation/arguments.h"

class Arguments;

mut_help_option::mut_help_option():Program_Option("-help","", 1, true,"this message"){

	char **optionValue = (char**)malloc(2 * sizeof(char*));

	Arguments::getInstance()->getCli(Program_Option::get_name(),optionValue);

	if(*optionValue != NULL){
		mut_help_option::set();
	}
}

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
    Program_Option::set(argv);  // needed for help & command line construction
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

mut_isolated_option::mut_isolated_option()
: Bool_option("-isolated", "", 1, false, "hidden cli controlling execution of test in a separate process"){

	char **optionValue = (char**)malloc(sizeof(char*)+sizeof(char*));

	Arguments::getInstance()->getCli(Program_Option::get_name(),optionValue);

	if(*optionValue != NULL){
		set(*optionValue);
	}	
}
