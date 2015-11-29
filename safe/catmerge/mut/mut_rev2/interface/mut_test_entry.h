/*
 * mut_test_entry.h
 *
 *  Created on: Sep 21, 2011
 *      Author: londhn
 *

#ifndef MUT_TEST_ENTRY_H_
#define MUT_TEST_ENTRY_H_

#include "defaulttest.h"

enum Mut_test_status_e{
	MUT_TEST_NOT_EXECUTED       = 1,
	MUT_TEST_RESULT_UKNOWN      = 2,
	MUT_TEST_PASSED 			= 3,
	MUT_TEST_FAILED 			= 4,
	MUT_TEST_ERROR 				= 5,
	MUT_TEST_ERROR_IN_TEST      = 6,
	MUT_TEST_ERROR_IN_TEARDOWN  = 7,
	MUT_TEST_VECTORED_EXCEPTION = 8,
	MUT_TEST_TIMEOUT_IN_TEST    = 9,
	MUT_TEST_INIT_THREAD_ERROR  = 10,
	MUT_TEST_TEARDOWN_THREAD_ERROR = 11,
	MUT_TEST_TIMEOUT_IN_TEARDOWN   = 12,

};

class MutString;

class Mut_test_entry{
private:
	MutString *test_id;
	DefaultTest * const test_instance;
	int test_index;
	unsigned long time_out;
	enum Mut_test_status_e volatile mStatus;
	static int global_test_counter;

public:
	Mut_test_entry();
	~Mut_test_entry();

	static void reset_global_test_counter();

	static void inc_global_test_counter();

	static int get_global_test_counter();

	const char*  get_test_id();

	DefaultTest * get_test_instance();

	unsigned long get_time_out();

    void set_test_index(int index);
    int get_test_index();

    void set_status(enum Mut_test_status_e status);
    enum Mut_test_status_e get_status();
    const char *get_status_str();

    bool statusFailed();

    //various constructors
    Mut_test_entry(DefaultTest * const instance, const char* id, unsigned long testTimeout);
};

#endif /* MUT_TEST_ENTRY_H_ */
