/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                Program_Option.h
 ***************************************************************************
 *
 * DESCRIPTION: Generic option processing class
 *  Provides:
 *          Program_Option -- base class, maintains list of all options created
 *          Int_option -- sub class, takes single integer option, default 0
 *          Bool_option -- sub class, takes no parameters, presence on the
 *              command line indicates true, default false
 *          String_option -- sub class, takes 1 or more string parameters, default {"",}
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    10/2010 Created JD
 **************************************************************************/
#ifndef OPTION_H
#define OPTION_H


# include "csx_ext.h"
# include "generic_types.h"
# include <cstdlib>


#define PARSE_ERROR(msg)\
    printf("[ Option error, function %s\n", __FUNCTION__); \
    printf msg; \
    printf("]\n"); \
    CSX_EXIT(1);

/*
 * Opaque declaration of ListOfArguments & ListOfOptions, which encapsulates/hides a vector
 * in this Public API
 *
 */
class ListOfArguments;
class ListOfOptions;

class Arguments;
/*
 * Opaque declaration of Option, needed by method Options::register_option
 */
class Program_Option;

class Options {
public:
    virtual ~Options();

    static Options *getInstance();

    /*
     * The following methods use the default Options singleton
     */

    static void register_option(Program_Option *option);

    // print the help info for all options.
    static void show_all_help();

    // get the number of args an option takes
    static int num_of_args_for_option(char *Program_Option);

    static void opt_to_lower(char* opt_in, char *opt_out); // lowercase an option

    static Program_Option *get_option(const char *name);

    /*
     * Options instance methods
     */
    void registerOption(Program_Option *option);

    void process_options(int argc, char** argv); // also MUT specific, needs refactoring
    char *constructCommandLine(const char *programPath, bool appendKnownArguments=true);

    static Program_Option *findOption(const char *name);

    static void getSelectedOptions(Program_Option ***list, UINT_32 *noOptions);

    static Options *factory();

private:
    Options();
    ListOfOptions   *mOptionsList;
};
    

class Program_Option {
    
    const char *name; // this options namespace
    const char *mArg; // when displaying help,   -name mArg is printed
    int num_of_args;
    bool show; // display help for this option
    const char *help_txt; // help string to display
    bool dnf; // do not forward when building a command line for a child process to interpret.

protected:
    friend class Options;
    ListOfArguments *value_list; // list of arguments provided

    void set_num_of_args(int n);
    virtual void set();
    bool mSet;                      // true when the argument was specified on the command line
    void set_dnf(bool d){dnf=d;}
public:
    
    // constructor takes
    // a string for the name, 
    // the name of it's argument or NULL,
    // the number of args including itself
    // whether it should be listed in help
    // the help string for the option
    // whether it should be recorded to the master list of options
    Program_Option (const char* _name, const char* _arg, const char* _help_txt, int _num_of_args = 1, bool _show = true, bool record = true);
    Program_Option (const char* _name, const char* _arg, int _num_of_args, bool _show, const char* _help_txt, bool record = true);
    virtual ~Program_Option();
    virtual void set(char *arg);
    virtual void post_error();
    virtual char *getString();
    const char *get_name();

    int get_num_of_args();
    bool get_store();
    bool get_show();
    virtual void print_help();

    virtual UINT_32 formattedLength();
    virtual char *format();
    virtual bool isSet();  // returns true when the argument was set on the command line
    bool get_dnf(){return dnf;}
private:

};

// use this for an integer type option with a default value of 0
class Int_option : public Program_Option {
private:
    char valueStr[20];
    int value;

   
public:
    virtual void _set(int v);// direct setter for value for derived classes
    virtual void set(char *arg);

    Int_option(const char* name, const char* arg, const char *help_txt = "", int default_value = 0, bool _show = true, bool record = true);
    Int_option(const char* name, const char* arg, int num_of_args, bool show, const char *help_txt, bool record = true, int default_value = 0);

    char *getString();
    int get();

    virtual void post_error();

};

// string value option
class String_option : public Program_Option{
private:
    char valueStr[1024];
    // char *value;
protected:
    virtual void _set(char * v);

public:
    String_option(const char* name,const char* arg, int num_of_args, bool show,  const char *help_txt, bool record = true);
    virtual void set(char *arg);
    virtual void post_error();

    char *getString();
    char *get();
    virtual void set();

    virtual char **getValues();
};

//boolean value option, defaults to false
class Bool_option : public Program_Option {
private:
    bool value;

public:
    virtual void set()   { _set(true); Program_Option::set();}    // used by Options when parsing the command line and encountering option
    virtual void _set(bool b);
    virtual void set(char *arg);
    // assume option to be false, set to true when called.
    Bool_option(const char* name,const char* arg, const char *_help_txt = "", bool _show = true, bool record = true);
    Bool_option(const char* name,const char* arg, int num_of_args, bool show, const char *help_txt, bool record = true);
    char *getString();
    bool get();
    virtual void post_error();
    
    virtual UINT_32 formattedLength();
    virtual char *format();
};

#endif
