#include "mut_test_entry.h"
#include "mut_private.h"
#include "mut_test_control.h"

// implementation of Mut_test_entry class.

extern Mut_test_control *mut_control;
struct Mut_test_status_translation_s {
    enum Mut_test_status_e status;
    const char *value;
};

static struct Mut_test_status_translation_s Mut_test_status_translations[] = 
{
    {MUT_TEST_NOT_EXECUTED,         "Not Executed"},
    {MUT_TEST_RESULT_UKNOWN,	    "Result Unknown"},		
    {MUT_TEST_PASSED,               "Passed"},
    {MUT_TEST_FAILED,               "Failed"},
    {MUT_TEST_ERROR,                "Error"},
    {MUT_TEST_ERROR_IN_TEST,        "ERROR (EXCEPTION_EXECUTE_HANDLER in setup and test)"},
    {MUT_TEST_ERROR_IN_TEARDOWN,    "ERROR (EXCEPTION_EXECUTE_HANDLER in tear down)"},
    {MUT_TEST_VECTORED_EXCEPTION,   "ERROR (EXCEPTION caught in unhandled exception handler)"},
    {MUT_TEST_TIMEOUT_IN_TEST,      "ERROR (Timeout in setup and test)"},
    {MUT_TEST_INIT_THREAD_ERROR,    "ERROR (Failed to init setup and test thread)"},
    {MUT_TEST_TEARDOWN_THREAD_ERROR,"ERROR (Failed to init teardown thread)"},
    {MUT_TEST_TIMEOUT_IN_TEARDOWN,  "ERROR (Timeout in teardown)"},
    {MUT_TEST_CSX_PANIC,            "ERROR (CSX PANIC)"},
};

Mut_test_entry::Mut_test_entry(CAbstractTest * const instance, const char *id, unsigned long testTimeout)
:test_instance(instance), test_id(new MutString(id)), mStatus(MUT_TEST_NOT_EXECUTED) {
    test_index = Mut_test_entry::global_test_counter;

	if(testTimeout == 0xffffffff){
		time_out = mut_control->timeout.get();
	}else{
		time_out = testTimeout;
	}
}

Mut_test_entry::~Mut_test_entry(){
    delete test_id;
}

Mut_test_entry::Mut_test_entry(const Mut_test_entry& rhs) :test_instance(rhs.test_instance){
        MUT_INTERNAL_ERROR(("copy constructor not implemented on Mut_test_entry class"));
};

Mut_test_entry& Mut_test_entry::operator=(const Mut_test_entry& rhs){
    MUT_INTERNAL_ERROR(("assignment operator= not implemented on Mut_test_entry class"));
    return *this;
}

void Mut_test_entry::reset_global_test_counter(){
    global_test_counter=0;
}

void Mut_test_entry::inc_global_test_counter(){
    global_test_counter++;
}

int Mut_test_entry::get_global_test_counter(){
    return global_test_counter;
}

const char*  Mut_test_entry::get_test_id(){
    return test_id->c_str();
}

unsigned long Mut_test_entry::get_time_out(){
	return time_out;
}

CAbstractTest*  Mut_test_entry::get_test_instance(){
    return test_instance;
}

void Mut_test_entry::set_test_index(int index){
    test_index = index;
}
int Mut_test_entry::get_test_index(){
    return test_index;
}

void Mut_test_entry::set_status(enum Mut_test_status_e status){
    mStatus = status;
}

enum Mut_test_status_e Mut_test_entry::get_status(){
    return mStatus;
}

bool Mut_test_entry::statusFailed() {
    return !((get_status() == MUT_TEST_PASSED) || (get_status() == MUT_TEST_NOT_EXECUTED) || (get_status() == MUT_TEST_RESULT_UKNOWN));
}

const char * Mut_test_entry::get_status_str(){
    UINT_32 index;
    for(index = 0; index < sizeof(Mut_test_status_translations)/ sizeof(struct Mut_test_status_translation_s); index++) {
        if(mStatus == Mut_test_status_translations[index].status) {
            return Mut_test_status_translations[index].value;
        }
    }
    return "unknown/invalid status";
}

