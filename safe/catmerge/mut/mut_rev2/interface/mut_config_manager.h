/*
 * mut_config_manager.h
 *
 *  Created on: Sep 21, 2011
 *      Author: londhn
 */

#ifndef MUT_CONFIG_MANAGER_H
#define MUT_CONFIG_MANAGER_H

#include "mut.h"
#include "mut_private.h"

class Mut_config_manager{

	int start;
	int end;
	int cmd_line_argc;
	char **cmd_line_argv;

	void mut_config_init(void);

	// parse helper functions
	int num_of_args_for_option(char *option);
	bool set_option_tests_range(char **argv);

	void mut_parse_options();

public:

	Mut_config_manager(int argc,char **argv);

	int get_start(){
		return start;
	}

	int get_end(){
		return end;
	}
};


#endif /* MUT_CONFIG_MANAGER_H_ */
