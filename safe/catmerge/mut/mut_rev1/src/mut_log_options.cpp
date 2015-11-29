/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_log_options.cpp
 ***************************************************************************
 *
 * DESCRIPTION: Implements options related to the mut log
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    10/2010 Created JD
 **************************************************************************/


// implementation of mut log option classes

#include"mut_options.h"
#include"mut_log.h"
#ifdef ALAMOSA_WINDOWS_ENV
#include <direct.h>
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */
#ifndef ALAMOSA_WINDOWS_ENV
#include <unistd.h>
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */

enum mut_log_type_e Mut_log::get_log_type(){
    if (text.get())
    {
        if(xml.get())
        {
            return MUT_LOG_BOTH;
        }
        else 
        {
            return MUT_LOG_TEXT;
        }
    }
    else if (xml.get())
    {
        return MUT_LOG_XML;
    }
    else return MUT_LOG_NONE;
}


// mut_cli_init_format_file_option
void mut_cli_init_formatfile_option::set(char *argv){
    FILE *f;
    int i;

    Program_Option::set(argv);  // needed for help & command line construction
    if (!(f = fopen(argv, "w")))
    {
        MUT_INTERNAL_ERROR(("Unable to create text format file \"%s\"", argv))
    }
    for (i = 0; i < mut_formatitem_Size; i++)
    {
        fprintf(f, "%c%s\n%s%c%c\n", mut_log_format_spec_char, mut_format_class[i], mut_format_spec_txt[i], mut_log_format_spec_char, mut_log_format_spec_char);
    }

    fclose(f);

    exit(0);
    return;

}

mut_logdir_option::mut_logdir_option():String_option("-logdir"," dir",2, true, "specify directory log results are to be written to"){
        _set(_getcwd(NULL, 0));
};



mut_log_level_option::mut_log_level_option():Program_Option("-verbosity","",2, true,""),value(NULL){

    log_level_list.push_back(new log_level_detail("suite",  MUT_LOG_SUITE_STATUS, "display/log suite progress"));
    log_level_list.push_back(new log_level_detail("test",  MUT_LOG_TEST_STATUS, "display/log suite and test progress"));
    log_level_list.push_back(new log_level_detail("low",  MUT_LOG_LOW, "display/log control and minimal test progress"));
    log_level_list.push_back(new log_level_detail("medium",  MUT_LOG_MEDIUM, "display/log control and medium test progress"));
    log_level_list.push_back(new log_level_detail("high",  MUT_LOG_HIGH, "display/log control and all test progress"));
    log_level_list.push_back(new log_level_detail("ktrace",  MUT_LOG_KTRACE, "display/log all messages(Default)"));
    log_level = MUT_LOG_KTRACE;
	
	char **optionValue = (char**)malloc(2 * sizeof(char*));

	Arguments::getInstance()->getCli("-verbosity",optionValue);

	if(*optionValue != NULL){
		//traverse the "-" argument
		optionValue++;

		mut_log_level_option::set(*optionValue);
	}
}

void mut_log_level_option::set(char *argv)
{
    Program_Option::set(argv);  // needed for help & command line construction

    vector<log_level_detail *>::iterator i = log_level_list.begin();
    while (i != log_level_list.end())
    { 
        if(strcmp(argv, (*i)->get_name()) == 0)
        {
            value = argv;   // save keywork so that formatOption returns complete info
            log_level = (*i)->get_log_level();
            return;
        }
        i++;
    }

    MUT_INTERNAL_ERROR(("Incorrect argument \"%s\" for -verbosity option", argv))
    
    return;
}

mut_logging_t mut_log_level_option::get(){
    return log_level;
}
void mut_log_level_option::print_help()
{
    vector<log_level_detail *>::iterator i = log_level_list.begin();
    while(i != log_level_list.end())
    {
        printf("  [-verbosity %s] %s\n", (*i)->get_name(), (*i)->get_legend());
        i++;
    }
    
    return;
}


extern char mut_filename_format_default[];

mut_filename_option::mut_filename_option():String_option("-filename"," format", 2, true, "specify the format of log filename")
{
    _set(mut_filename_format_default);
}
