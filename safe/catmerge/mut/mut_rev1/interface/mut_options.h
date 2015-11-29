/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_options.h
 ***************************************************************************
 *
 * DESCRIPTION: defines all (most?) option classes used by mut proper
 *       all (most?) new framework option classes should be defined here.
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    10/2010 Created JD
 **************************************************************************/
#ifndef MUT_OPTIONS_H
#define MUT_OPTIONS_H
#include "simulation/Program_Option.h"
#include "simulation/arguments.h"
#include "mut_private.h"

#include <vector>
using namespace std;

class mut_help_option : private Program_Option{
public:
    void set(); // mut_config_manager.cpp
    void set(char *argv) {}; 
    
    mut_help_option();    
};


class mut_list_option : public Bool_option{
public:
   
    mut_list_option():Bool_option("-list", "", 1, true, "list all tests"){};
    
};

class mut_info_option : public Bool_option{
public:

    mut_info_option():Bool_option("-info", "", 1, true, "display test information"){};
    
};

class mut_isolate_option: public Bool_option {
public:
    mut_isolate_option(): Bool_option("-isolate", "", 1, true, "execute each test in a separate process"){}
};

class mut_isolated_option: public Bool_option {
public:
    mut_isolated_option();
};

class mut_logdir_option : public String_option{
public:
    mut_logdir_option(); // mut_log_options.cpp
 
};

class mut_filename_option : public String_option{
public:
    mut_filename_option(); // mut_log_options.cpp
    
};


class mut_text_option : public Bool_option{
public:
    
    mut_text_option():Bool_option("-text", "", 1, true, "generate textual output"){};
    
};


class mut_xml_option : public Bool_option{
public:
    
    mut_xml_option():Bool_option("-xml", "", 1, true, "generate structured XML output"){};
    
};

class mut_disableConsole_option : public Bool_option{
public:
    mut_disableConsole_option():Bool_option("-disableconsole","",1, true, "logs are not set to the console"){};
    
};

class mut_formatfile_option : public String_option{
public:
    mut_formatfile_option():String_option("-formatfile"," filename",2, true, "load text output format specification from file"){};
    
};

class mut_cli_init_formatfile_option : public Program_Option{
public:
    mut_cli_init_formatfile_option():Program_Option("-initformatfile", " filename", 2, true, "creates text output format specification file with default content"){};
   
    void set(char *argv); // mut_log_options.cpp
     
};

class mut_run_tests_option : public String_option{
public:
    mut_run_tests_option():String_option("-run_tests"," test1[,test2]...",2, true, "comma separated list of tests (names and/or numbers) to run"){set_dnf(true);}
    
};

class mut_run_testsuites_option : public String_option{
public:
    mut_run_testsuites_option():String_option("-run_testsuites"," suite1[,suite2]...",2, true, "comma separated list of testsuites to run"){set_dnf(true);}
    
};

class mut_timeout_option : public Program_Option {
    unsigned long value;
public:
    mut_timeout_option():Program_Option("-timeout"," timeout",2, true, "timeout in seconds for single test")
    {
        set_dnf(true);
		char **optionValue = (char**)malloc(2 * sizeof(char*));
	
		Arguments::getInstance()->getCli("-timeout",optionValue);
	
		if(*optionValue != NULL){
			//traverse the "-" argument
			optionValue++;	
			value = atoi(*optionValue);
			Program_Option::set(*optionValue);
			Program_Option::set();
		}else{
			value = 600;
		}
    };

    void set(char *argv);
	void set(unsigned long timeout);
    unsigned long get();
};


class mut_nofailontimeout_option: public Bool_option {
public: 
	mut_nofailontimeout_option(): Bool_option("-nofailontimeout", "", 1,true,"do not fail tests on timeout"){}
};

class mut_dump_on_timeout_option: public Bool_option {
public:
    mut_dump_on_timeout_option(): Bool_option("-dumpOnTimeout", "", 1, true, "create a dmp file on test timeout", false) {}
};

class mut_iteration_option : public Int_option{
public:
    mut_iteration_option():Int_option("-i"," <n>",2, true, "specify the number of iterations to run (0 for endless)", true, 1) {}
    
    void set(char *argv);
};

class mut_monitorexit_option : public Bool_option  {
public:
    mut_monitorexit_option(): Bool_option("-monitorexit", "", 1, true, "Trap to debugger on unexpected exit"){};
   
};

class mut_core_option : public Bool_option {
public:
    mut_core_option(): Bool_option("-core", "", 1, true, "Create a core dump when the test fails. Clean up the logs and exit"){};

};

class mut_nodebugger_option : public Bool_option {
public:
    mut_nodebugger_option(): Bool_option("-nodebugger", "", 1, true, "Do not go to the debugger when a test fails"){};

};


class mut_test_pause_option : public Bool_option {
public:
    mut_test_pause_option(): Bool_option("-pause_test", "", 1, true, "Enable test pausing point in case code"){};
};

class mut_color_output_option : public Bool_option {
public:
    mut_color_output_option(): Bool_option("-color_output", "", 1, true, "Enable customized log colors set in code"){};
};

class mut_log_level_option : public Program_Option {
    //private class with details about each log level
    class log_level_detail {
        const char* name;
        mut_logging_t log_level;
        const char* legend;
    public:
        log_level_detail(const char* _name, mut_logging_t _log_level, const char* _legend){ 
            name = _name;
            log_level = _log_level;
            legend = _legend;
        }
        const char*  get_name(){
            return name;
        }
        mut_logging_t get_log_level(){
            return log_level;
        }
        const char* get_legend(){
            return legend;
        }
    };

    // private data for the log_level option class.
    vector<log_level_detail *> log_level_list;
    mut_logging_t log_level;
public:
    char *value;
    mut_log_level_option(); // see mut_log_options.cpp
    virtual void set(char *argv);
    mut_logging_t get();
    virtual void print_help();
};


class mut_user_option: public String_option{
public:
    mut_user_option(const char * option_name, int min_num_args, bool show, const char* desc):String_option(option_name, "", min_num_args, show, desc){
        _set("");
    }
    
};
#endif



