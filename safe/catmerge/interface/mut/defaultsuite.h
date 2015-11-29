/*
 * defaultsuite.h
 *
 *  Created on: Jan 12, 2012
 *      Author: londhn
 */
 
#ifndef DEFAULTSUITE_H
#define DEFAULTSUITE_H

#include "mut.h"
#include "mut/defaulttest.h"

#include "mut/abstractsuite.h"

class DefaultTest;

/*
 * Opaque declarations which encapsulates/hides a vector
 * in this Public API
 *
 */
class ListOfTests;
class ListOfSuites;

class DefaultSuite:public AbstractSuite {
public:

	ListOfTests *test_list;

	DefaultSuite(){};
	DefaultSuite(const char* name);

	~DefaultSuite();

	// timeout below will be added in seconds, later on the the runner class the timeout will be 
	// converted into milliseconds
	virtual void addTest(DefaultTest *test, unsigned long timeout = 600);

	virtual void populate()=0;

	virtual void startUp(){};

	virtual void tearDown(){};

	const char* getName();

	int execute(){

		//add if else condition for checking for printing help or listing test
		//checking if any of the options have been specified or not
		run_all_tests();

		return status;
	}

private:
	//status of the suite execution
	int status;

	const char *suiteName;

	void info();		//outputs info on each test;
	void list_tests(); 	//lists each test in the suite
	bool is_last(ListOfSuites *suite_list); //returns true if this is the last in the suite list

	static int mut_str_in_list(const char *list, const char *str, int index);

	//printhelp() //depending on the implementation of the options class
	//printlist() //depending on the implementation of the options class - if we are using argument class to parse.
	int run_all_tests();
	
	static int test_counter;
};

#endif /* DEFAULT_SUITE_H */