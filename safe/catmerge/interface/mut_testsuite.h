/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_testsuite.h
 ***************************************************************************
 *
 * DESCRIPTION: Mut_testuite class definition.  
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    10/2010 Created JD
 **************************************************************************/

#ifndef MUT_TESTSUITE_H
#define MUT_TESTSUITE_H

#if ! defined(MUT_REV2)

#include "mut.h"
#include "mut_sdk.h"

#if defined(MUT_DLL_EXPORT)
#define MUT_CLASSPUBLIC CSX_CLASS_EXPORT
#else
#define MUT_CLASSPUBLIC CSX_CLASS_IMPORT
#endif

class Mut_test_entry;

/*
 * Opaque declarations which encapsulates/hides a vector
 * in this Public API
 *
 */
class ListOfTests;
class ListOfSuites;
class MutString;
class MUT_CLASSPUBLIC Mut_testsuite {
private:
    //PMthread main_test_thread;     
    mut_thread_t thread;
    //const char *name;           /* Symbolic testsuite name */
    
    mut_function_t startup;       /* suite setup function */
    mut_function_t teardown;   /* suite tear_down function */
    MutString *name;
public:

    ListOfTests *test_list;

    // subclasses must provide a constructor that provides a string
    // ie.  mysuitesubclassConst(const char* suitename) : Mut_testsuite(suitename){};
    Mut_testsuite(const char *n, mut_function_t s=NULL, mut_function_t t=NULL);

    virtual ~Mut_testsuite();

    void info(); // outputs info on each test in suite
    void list_tests(); // lists each test in suite
    bool is_last(ListOfSuites *suite_list);  // returns true if 'this' is last in the suite_list
    

    const char* get_name();

    void set_main_test_thread_handle(void* h);
    mut_thread_t * get_main_test_thread();
    

    virtual void suiteStartUp();

    virtual void suiteTearDown();    

};

MUT_API void mut_add_user_suite(Mut_testsuite *suite);
MUT_API void mut_add_test_with_description(Mut_testsuite *suite, const char *test_id,
        mut_function_t test,
        mut_function_t startup, mut_function_t teardown, const char* short_desc, const char * long_desc);
MUT_API int mut_run_testsuite(Mut_testsuite *suite);


#define MUT_ADD_SUITE_CLASS(testsuite) \
    mut_add_user_suite(testsuite);

#else

#include "mut.h"

#include "cabstracttest.h"

class Mut_test_entry;

/*
 * Opaque declarations which encapsulates/hides a vector
 * in this Public API
 *
 */
class ListOfTests;
class ListOfSuites;
class MutString;

class Mut_testsuite {

private:

	mut_function_t startup;
    mut_function_t teardown;
    MutString *name;
public:

	ListOfTests *test_list;

    // subclasses must provide a constructor that provides a string
    // ie.  mysuitesubclassConst(const char* suitename) : Mut_testsuite(suitename){};
    Mut_testsuite(){};
    Mut_testsuite(const char *n, mut_function_t s=0, mut_function_t t=0);

    virtual ~Mut_testsuite();

    void info(); //outputs info on each test in the suite
    void list_tests(); //lists each test in suite
    bool is_last(ListOfSuites *suite_list); //returns true if this is last in the suite_list

    const char* get_name();

    void set_main_test_thread_handle(void* h);

    void addTest(const char *test_id,CAbstractTest *test_instance,unsigned long timeout = 0xffffffff);

    virtual void suiteStartUp();

    virtual void suiteTearDown();

    virtual void populateSuite(){};

    void execute();
};

MUT_API void mut_add_user_suite(Mut_testsuite *suite);
MUT_API void mut_add_test_with_description(Mut_testsuite *suite, const char *test_id,
        mut_function_t test,
        mut_function_t startup, mut_function_t teardown, const char* short_desc, const char* long_desc);
MUT_API void mut_run_testsuite(Mut_testsuite *suite);

#define MUT_ADD_SUITE_CLASS(testsuite) \
    mut_add_user_suite(testsuite);

#endif

#endif
