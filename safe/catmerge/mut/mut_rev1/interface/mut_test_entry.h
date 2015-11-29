#ifndef MUT_TEST_ENTRY_H
#define MUT_TEST_ENTRY_H

#include "cabstracttest.h"

// wraps a single test in a suite
enum Mut_test_status_e {
    MUT_TEST_NOT_EXECUTED       = 1,
    MUT_TEST_RESULT_UKNOWN      = 2,
    MUT_TEST_PASSED             = 3,
    MUT_TEST_FAILED             = 4,
    MUT_TEST_ERROR              = 5,
    MUT_TEST_ERROR_IN_TEST      = 6,
    MUT_TEST_ERROR_IN_TEARDOWN  = 7,
    MUT_TEST_VECTORED_EXCEPTION = 8,
    MUT_TEST_TIMEOUT_IN_TEST    = 9,
    MUT_TEST_INIT_THREAD_ERROR  = 10,
    MUT_TEST_TEARDOWN_THREAD_ERROR = 11,
    MUT_TEST_TIMEOUT_IN_TEARDOWN   = 12,
    MUT_TEST_CSX_PANIC = 13,
};



class MutString;

class Mut_test_entry {
private:
    MutString *test_id;
    CAbstractTest * const test_instance;
	unsigned long time_out;
    int test_index;
    enum Mut_test_status_e volatile mStatus;
    static int global_test_counter;

public:
    Mut_test_entry();
    ~Mut_test_entry();
    static void reset_global_test_counter();

    static void inc_global_test_counter();

    static int get_global_test_counter();

    const char*  get_test_id();

	unsigned long get_time_out();

    CAbstractTest*  get_test_instance();

    void set_test_index(int index);
    int get_test_index();

    void set_status(enum Mut_test_status_e status);
    enum Mut_test_status_e get_status();
    const char *get_status_str();

    bool statusFailed();
 
    //various constuctors and an assignment operator 
    
    Mut_test_entry(CAbstractTest * const instance, const char* id, unsigned long testTimeout);
    
    Mut_test_entry(const Mut_test_entry& rhs);
    Mut_test_entry& operator=(const Mut_test_entry& rhs);
};
#endif


