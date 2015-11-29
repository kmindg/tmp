/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                option.cpp
 ***************************************************************************
 *
 * DESCRIPTION: Option implementation file.  Implements a generic option
 *      processing class/library
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    10/2010 Created JD
 **************************************************************************/

#include "simulation/Program_Option.h"
#include "simulation/arguments.h"

#include <vector>
#include <ctype.h>
#include <algorithm>
#include <string.h>
#include <stdio.h>

#include "EmcPAL_Misc.h"

using namespace std;

#define MAX_CONFIG_LENGTH 64

// vector that holds all the options.
static vector<Program_Option *> option_list;


class ListOfArguments {
public:
    ListOfArguments() {};
    ~ListOfArguments() {};

    vector<char *>::iterator get_list_begin() { return list.begin(); }
    vector<char *>::iterator get_list_end()   { return list.end(); }
    void push_back(char *v) { list.push_back(v); }
    void clear() { list.clear(); }
    size_t size() { return list.size(); }

private:
    vector<char *> list;
};


class ListOfOptions {
public:
    ListOfOptions() {};
    ~ListOfOptions() {};

    vector<Program_Option *>::iterator get_list_begin() { return list.begin(); }
    vector<Program_Option *>::iterator get_list_end()   { return list.end(); }
    void push_back(Program_Option *v) { list.push_back(v); }
    void clear() { list.clear(); }
    size_t size() { return list.size(); }

private:
    vector<Program_Option *> list;
};


static Options *Options_Singleton = NULL;

Options::Options():  mOptionsList(new ListOfOptions()) { }

Options::~Options() {
    delete mOptionsList;
}

Options *Options::factory() {
    return new Options();
}

Options *Options::getInstance() {
    if(Options_Singleton == NULL) {
       Options_Singleton = new Options();
    }

    return Options_Singleton;
}

void Options::register_option(Program_Option *option) {
    getInstance()->registerOption(option);
}


static bool comp(Program_Option * l, Program_Option* r){
    return (strcmp(l->get_name(),r->get_name()) <= 0);
}

// help option
void Options::show_all_help(){

    sort (option_list.begin(),option_list.end(), comp);
    
    vector<Program_Option *>::iterator i = option_list.begin();
    while (i != option_list.end())
    {
        // Show the item?
        if((*i)->get_show() == true)
        {
            (*i)->print_help();
        }
        i++;
    }
}

void Options::opt_to_lower(char* opt_in, char* opt_out)
{
    int index = 0;
    while (opt_in[index])
    {
        opt_out[index] = tolower(opt_in[index]);
        index++;
    }
    opt_out[index]= '\0';			
    return;
}


/** \fn int num_of_args_for_option(char *option)
 *  \param option - name of the option
 *  \return number of parameters for this option
 */
int Options::num_of_args_for_option(char *option)
{
    vector<Program_Option *>::iterator i = option_list.begin();
    char optionlower[30];

    while (i != option_list.end())
    {
        opt_to_lower(option, optionlower);
        if (strcmp((*i)->get_name(), optionlower) == 0)
        {
            return (*i)->get_num_of_args();
        }
        i++;
    }
    PARSE_ERROR(("Option not found %s", optionlower));
}

/*
 * This code is very similar to Options::parse_options
 * except parse_options parses argc/argv using the static option_list and fails on parse error
 * while process_options parse argc/argv using the instance field mOptionsList and ignores most parse errors
 */
void Options::process_options(int argc, char** argv) {
     vector<Program_Option *>::iterator i;
     int num_of_args;
     char argvlower[128];
     char namelower[128];
     

    while (argc > 0)
    {
    	if (*argv[0] == '-')
		{
            opt_to_lower(argv[0], argvlower);
            
	        i = mOptionsList->get_list_begin();


			while (i != mOptionsList->get_list_end())
            {
                opt_to_lower((char *)(*i)->get_name(), namelower); 
                if (strcmp(argvlower, namelower) == 0)
                {
                    num_of_args = (*i)->get_num_of_args();
                 
                    if (argc < num_of_args)
                    {
                        PARSE_ERROR(("Not enough arguments for option \"%s\"", argvlower))
                    }
                    // mark that option was encountered in command line
                    (*i)->set();

                    // step past the option
                    argc --;
                    argv ++;

                    // process all option values
                    while(*argv != NULL && *argv[0] != '-') {
                        (*i)->set(*argv);
                        argc--;
                        argv++;
                    }

                    break;
                }
                else{
                    i++;
                }
            }
        }
        else {
            argc--;
            argv++;  // Skip over non-keyword argument (may be the Program name)
        }
    }
}

Program_Option *Options::get_option(const char* option_name){
    vector<Program_Option *>::iterator i = option_list.begin();
    vector<char *>::iterator v;
    vector<char *>::iterator ve;

	while (i != option_list.end())
    {
        if (!strcmp(option_name, (*i)->get_name()))
        {
            return *i;
        }
        i++;
    }
    return NULL;
}

void Options::registerOption(Program_Option *option) {
	mOptionsList->push_back(option);
}

void Options::getSelectedOptions(Program_Option ***list, UINT_32 *noOptions) {
    UINT_32 count = 0;
    vector<Program_Option *>::iterator i = option_list.begin();

    /*
     * first count number of selected options
     */
    while(i != option_list.end()) {
        Program_Option *o = (*i);
        if(o->isSet()) {
            count++;
        }
        i++;
    }
    /*
     * Allocate & init array of options
     */
    Program_Option **localList = (Program_Option **)malloc(sizeof(void *) * count);
    UINT_32 index = 0;

    i = option_list.begin();
    while(i != option_list.end()) {
        Program_Option *o = (*i);
        if(o->isSet()) {
            localList[index] =  o;
            index++;
        }
        i++;
    }

    *list = localList;
    *noOptions = count;

    return ;
}

char *Options::constructCommandLine(const char *programPath, bool appendKnownArguments) {

    vector<Program_Option *>::iterator optionIter;
    UINT_32 cmdLineLen = (UINT_32)strlen(programPath) + 1; // he terminator

    /*
     * Calculate command line length
     */

	// Calculate length of all options to be forwarded on the local list
    for(optionIter = mOptionsList->get_list_begin(); optionIter != mOptionsList->get_list_end(); ++optionIter) {
        Program_Option *option = *optionIter;
        if (option->get_dnf()) {
            continue;
        }
        cmdLineLen += option->formattedLength() + 1;  // each arg requires 1 space for a delimiter
    }

	// Calculate length of options set on the global list
    if (appendKnownArguments) {
		for(optionIter = option_list.begin(); optionIter != option_list.end(); ++optionIter) {
			Program_Option *option = *optionIter;
			if (option->get_dnf() || !option->isSet()) {
				continue;
			}
			
			cmdLineLen += option->formattedLength() + 1; // Only add if not already on the local list
		}
    }
	/*
     * allocate buffer and format the commandline into the buffer
     */
    char *buffer = new char[cmdLineLen + 2];// add space for string terminator and program delimeter
    strcpy(buffer, programPath);

	// Allocate buffer to contain all options to be forwarded on the local list
    for(optionIter = mOptionsList->get_list_begin(); optionIter != mOptionsList->get_list_end(); ++optionIter) {
        Program_Option *option = *optionIter;
        if (option->get_dnf()) {
            continue;
        }
        char *formattedArg = option->format();

        strcat(buffer, " "); // first add delim
        strcat(buffer, formattedArg); // then add argument

        delete formattedArg;
    }

	// Allocate buffer to contain options that have been set on the global list
    if(appendKnownArguments){
		for(optionIter = option_list.begin(); optionIter != option_list.end(); ++optionIter) {
			Program_Option *option = *optionIter;
			if (option->get_dnf() || !option->isSet()) {
				continue;
			}

			char *formattedArg = option->format();

			strcat(buffer, " "); // first add delim
			strcat(buffer, formattedArg); // then add argument

			delete formattedArg;
		}
    }

    return buffer;
}


// implementation for Option class
Program_Option::Program_Option (const char* _name, const char* _arg, const char* _help_txt, int _num_of_args, bool _show, bool record):dnf(false),mSet(false), 
    name(_name),mArg(_arg),help_txt(_help_txt),num_of_args(_num_of_args),show(_show), value_list(new ListOfArguments()) 
{
    if(record) {
        option_list.push_back(this);
    }
}
// second constructor, same values, different order to support different interfaces
Program_Option::Program_Option (const char* _name, const char* _arg, int _num_of_args,  bool _show, const char* _help_txt, bool record):dnf(false),mSet(false),
    name(_name),mArg(_arg),help_txt(_help_txt),num_of_args(_num_of_args),show(_show), value_list(new ListOfArguments())                    
{                                                                                                                                              
    if(record) {                                                                                                                               
        option_list.push_back(this);                                                                                                           
    }                                                                                                                                          
}                                                                                                                                              


Program_Option::~Program_Option() { delete value_list;}

void Program_Option::post_error(){
    PARSE_ERROR(("Error setting option \"%s\" ", name))
}
const char *Program_Option::get_name() {
    return name;
}

int Program_Option::get_num_of_args(){
    return num_of_args;
}

void Program_Option::set() {
    mSet = true;
}
bool Program_Option::isSet() {
    return mSet;
}

void Program_Option::set(char *arg) {
    mArg = arg;
    mSet = true; // if we're setting mArg, mSet must be true too
}

char *Program_Option::getString() {
    return (char *)mArg;
}

char *Program_Option::format() {
    UINT_32 len = formattedLength() + 1; // add terminator
    char *buffer = new char[len];

    *buffer = '\0';

    strcpy(buffer, get_name());
    strcat(buffer, " ");
    strcat(buffer, getString());

    return buffer;
}

void Program_Option::set_num_of_args(int n) {
    num_of_args = n;
}

bool Program_Option::get_show(){
    return show;
}

void Program_Option::print_help(){
    if(*mArg == '\0' || *mArg == ' ') {
        printf("  [%s%s] %s\n", name, mArg, help_txt);
    }
    else {
        printf("  [%s %s] %s\n", name, mArg, help_txt);
    }
}

UINT_32 Program_Option::formattedLength() {
    /*
     * The formated argument is _name + " " + _arg
     */
    return (UINT_32)strlen(name) + (UINT_32)strlen(getString()) + 1;
}


// Int_option implementation

Int_option::Int_option(const char* name, const char* arg, const char *help_txt, int default_value, bool _show, bool record)
:Program_Option(name, arg, help_txt, 2, _show, record), value(default_value){

	itoa(default_value, valueStr, 10); // Set valueStr to the default value.

	if(Arguments::getInstance() != NULL){
		char **optionValue = (char**)malloc(3 * sizeof(char*));

		Arguments::getInstance()->getCli((char*)name,optionValue);

		if(*optionValue != NULL){
			//traverse the argument
			optionValue++;
			//Set the value for each option
			while( *optionValue != NULL && *optionValue[0] != '-'){
				set(*optionValue);
				optionValue++;
			}
			Program_Option::set();	
		}
	}
}

Int_option::Int_option(const char* name,const char* arg, int num_of_args, bool show, const char *help_txt, bool record, int default_value)
: Program_Option(name,arg, num_of_args, show, help_txt, record), value(default_value) {
    
	itoa(default_value, valueStr, 10); // Set valueStr to the default value.
 
	if(Arguments::getInstance() != NULL){
		char **optionValue = (char**)malloc((num_of_args + 1) * sizeof(char*));

		Arguments::getInstance()->getCli((char*)name,optionValue);

		if(*optionValue != NULL){
			//traverse the argument
			optionValue++;

			if(*optionValue == NULL){
				PARSE_ERROR(("Integer type options must have num_of_args = 2, option %s --> %d args provided", name, num_of_args))
			}

			//Set the value for each option
			while( *optionValue != NULL && *optionValue[0] != '-'){
				set(*optionValue);
				optionValue++;
			}
			Program_Option::set();	
		}
	}
}


void Int_option::_set(int v) {
    value = v;
}

void Int_option::set(char * argv)
{
    _set(atoi(argv));
	Program_Option::set();
    return;
}

char *Int_option::getString() {
    //coverity [secure_coding]
    sprintf(valueStr, "%d", value);
    return valueStr;
}

int Int_option::get(){
    return value;
}
void Int_option::post_error(){
    PARSE_ERROR(("Error setting option \"%s\" with value %d", get_name(), value))
}

//String_option implementation
String_option::String_option(const char* name,const char* arg, int num_of_args, bool show,  const char *help_txt, bool record)
: Program_Option(name,arg, num_of_args, show, help_txt, record){ 
    	
	valueStr[0] = '\0';

	if(Arguments::getInstance() != NULL){
		char **optionValue = (char**)malloc((num_of_args + 1) * sizeof(char*));

		Arguments::getInstance()->getCli((char*)name,optionValue);

		if(*optionValue != NULL){
			//traverse the "-" argument
			optionValue++;
			
			//set the values
			while( *optionValue != NULL && *optionValue[0] != '-'){
				set(*optionValue);
				optionValue++;
			}

			Program_Option *i = Options::get_option(name);

			if((*i).get_num_of_args() == 1){
				set("");
			}

			Program_Option::set();	
		}
	}
}

void String_option::_set(char * v) {
    value_list->push_back(v);
}

char *String_option::getString() {
    return valueStr;
};

void String_option::set() {
    value_list->clear();
}
void String_option::set(char *argv)
{
    value_list->push_back(argv);
    if (strlen(valueStr)+1 + strlen(argv) > sizeof(valueStr)-1)
    {
        PARSE_ERROR(("string length overflow in String_option::set(char*)"));
    }
    if (strlen(valueStr) > 0) {
        strcat(valueStr, " ");
    }
    strcat(valueStr, argv);
	Program_Option::set();
}

void String_option::post_error()
{
    PARSE_ERROR(("Error setting option \"%s\" with value %s", get_name(), *value_list->get_list_begin()))
}

char *String_option::get(){
    if (value_list->size()) {
        return *(value_list->get_list_begin());
    }
    else {
        return NULL;
    }
}

char **String_option::getValues() {
    char **argArray =  new char *[value_list->size() + 1]; // add space for terminator
    char **ptr = argArray;
    vector<char *>::iterator valuesIter;
    UINT_32 argc = 0;

    for(valuesIter = value_list->get_list_begin(); valuesIter != value_list->get_list_end(); valuesIter++) {
        *ptr = *valuesIter;
        ptr++;
        argc++;
    }
    *ptr = NULL;

    return argArray;  // caller is responsible for deleting returned memory
}

// Bool_option implementation
// assume option to be false, set to true when called.
Bool_option::Bool_option(const char* name, const char* arg, const char *_help_txt, bool _show, bool record)
:Program_Option(name, arg, _help_txt, 1, _show, record),value(false){
	
	if(Arguments::getInstance() != NULL){
		char **optionValue = (char**)malloc(sizeof(char*));

		Arguments::getInstance()->getCli((char*)name,optionValue);

		if(*optionValue != NULL){
			set(*optionValue);
			Program_Option::set();	
		}
	}
}

Bool_option::Bool_option(const char* name,const char* arg, int num_of_args, bool show, const char *help_txt, bool record)
:Program_Option(name,arg, num_of_args, show, help_txt, record ),value(false){

    if (num_of_args != 1)
    {
        PARSE_ERROR(("Bool type options must have num_of_args = 1, %d provided", num_of_args))
    }

	if(Arguments::getInstance() != NULL){
		char **optionValue = (char**)malloc((num_of_args + 1) * sizeof(char*));

		Arguments::getInstance()->getCli((char*)name,optionValue);

		if(*optionValue != NULL){
			set(*optionValue);
			Program_Option::set();	
		}

	}
}

void Bool_option::_set(bool b){
    value = b;
}

void Bool_option::set(char *argv)
{
    value = true;
    return;
}

char *Bool_option::getString() {
    return "";
}

bool Bool_option::get(){
    return value;
}
void Bool_option::post_error(){
    PARSE_ERROR(("Error setting option \"%s\" with value %d", get_name(), (value ? 1: 0)))
}

UINT_32 Bool_option::formattedLength() {
    /*
     * The formated argument is _name
     */
    return (UINT_32)strlen(get_name());
}

char *Bool_option::format() {
    UINT_32 len = formattedLength() + 1;
    char *buffer = new char[len];

    *buffer = '\0';
    strcpy(buffer, get_name());

    return buffer;  // caller is responsible for freeing the buffer
}

