/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_process_test_runner.cpp
 ***************************************************************************
 *
 * DESCRIPTION: MUT process test runner implementation
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *
 **************************************************************************/

# include "mut_process_test_runner.h"
# include "mut_test_listener.h"

# include "mut_test_control.h"
# include "mut_testsuite.h"
# include "mut_test_entry.h"
# include "simulation/SpId_option.h"
# include "simulation/Program_Option.h"

Mut_process_test_runner::Mut_process_test_runner(Mut_test_control *control,
                                                Mut_testsuite *suite,
                                                Mut_test_entry *test)
: mControl(control), mSuite(suite), mTest(test), mRecursing(false) {}


const char *Mut_process_test_runner::getName() {
    return mTest->get_test_id();
}

Mut_test_entry   *Mut_process_test_runner::getTest() {
    return mTest;
}

class MutProcess_option: public Int_option {
public:
    MutProcess_option(UINT_32 index)
    : Int_option("-run_tests", "Test index", "Index of test to be executed") {
		
		char **optionValue = (char**)malloc(2 * sizeof(char*));

		Arguments::getInstance()->getCli("-run_tests",optionValue);

		if(*optionValue != NULL){
			//traverse the "-" argument
			optionValue++;

			Int_option::set(*optionValue);
		}
		else {
			Int_option::_set(index);
		}
    }

};

bool Mut_process_test_runner::optionOnExcludeList(Program_Option *option) {

	// Options on the exclusion list
	if(option != &mControl->run_tests
        && option != &mControl->isolate_mode
        && option != &mControl->mut_log->text
        && option != &mControl->mut_log->xml
		&& option != Master_IcId_option::getCliSingleton()) {
		return false;
	}
	return true;
}

char *Mut_process_test_runner::constructCmdLine() {
    Options *options =  Options::factory();
    options->registerOption(&mControl->isolated_mode);
    options->registerOption(new MutProcess_option(mTest->get_test_index()));
    options->registerOption(Master_IcId_option::getCliSingleton());
	
    
    /* 
     * Class MutProcess_option will be used by Options::constructCommandLine 
     * to add -run_test # to the command line
     *
     * Note, a user invoked mut to launch the current process.  The user
     * might have specified "-run_test x,y,z" in that command line to 
     * specify that only some tests were to be execute. 
     *
     * In addition, the user that invoked this process did specify the 
     * -isolate cli argument. The new commandLine contains -isolated CLI,
     * so exclude -isolate cli to simplify the new command line.
     *
     * The code below obtains the list of options that were specified when
     * the user launch much and then exclude command line -run_test
     * (in Class MutProcess_option) to select test for execution
     */


    Program_Option **selectedOptions;
    UINT_32 noSelectedOptions;
    Options::getSelectedOptions(&selectedOptions, &noSelectedOptions);

    UINT_32 index;

    for(index = 0;index < noSelectedOptions; index++) {
		// Register options on the local list, if they are not on the exclusion list.
	    if(!optionOnExcludeList(selectedOptions[index]))
			options->registerOption(selectedOptions[index]);    
    }

    char *cmd = options->constructCommandLine(mut_get_program_name(), false);

    delete options;

    return cmd;
}

void Mut_process_test_runner::setTestStatus(enum Mut_test_status_e status) {

    if (mRecursing) return;
    mRecursing = true;

    mTest->set_status(status);
    if(mTest->statusFailed()) {
        mut_process_test_failure();
    }
    
    mRecursing = false;
}

void Mut_process_test_runner::run() {

    Mut_test_listener *listener = new Mut_test_listener(mControl);
    listener->start();

	// Default to 'unknown' test status
    setTestStatus(MUT_TEST_RESULT_UKNOWN);

    mControl->mut_log->mut_report_test_started(mSuite, mTest);

    SpId_option* spidOpt = (SpId_option*) Options::get_option("-SP");
    if(spidOpt) {
        Split_Affinity_option* pSa = Split_Affinity_option::getCliSingleton();
        if(pSa->isSet()) {
            // The caller has requested Split Affinity, let's find out how many
            // CPUs we have to play with.
            csx_p_sys_info_t  sys_info;
            csx_p_get_sys_info(&sys_info);
            csx_nuint_t numCpus = sys_info.os_cpu_count;
            // See if we have at least the minimum number of cpus for using split affinity
            if(numCpus >= SPLIT_AFFINITY_MINIMUM_CPU_NUMBER_SUPPORTED) {
                Virtual_Affinity_option *pVAO = Virtual_Affinity_option::getCliSingleton();
                UINT_64 processor_mask = 0;
                processor_mask = (((UINT_64) 0x1 << numCpus/2) - (UINT_64) 1);
                if(spidOpt->get() == 0) {
                    KvPrintIo("SPA opt_virtual_affinity_mask = 0x%llx\n", processor_mask);
                } else {
                    processor_mask = processor_mask << (numCpus/2);
                    KvPrintIo("SPB opt_virtual_affinity_mask = 0x%llx\n", processor_mask);
                }
                pVAO->_set(processor_mask);
            }
        }
    }
    /*
     * fork off a process to run the test
     * Construct command line with testname or testnumber including all user specified CLI
     */
    char *cmd = constructCmdLine();
	
    mut_printf(MUT_LOG_TEST_STATUS, "Isolating test %s", mTest->get_test_id());
    mut_printf(MUT_LOG_TEST_STATUS, "Invoking %s", cmd);

#ifdef ALAMOSA_WINDOWS_ENV
    system(cmd);
#else /* ALAMOSA_WINDOWS_ENV - STDPORT - debugging hook */
    if (getenv("DISPLAY") && getenv("SAFE_DO_XTERM")) {
        char * do_prefix = getenv("SAFE_DO_PREFIX");
        char * do_geometry = getenv("SAFE_DO_GEOMETRY");
        if (do_prefix == NULL) { do_prefix = ""; }
        if (do_geometry == NULL) { do_geometry = "-geometry 120x20+20+20 -sb -sl 5000"; }
        csx_string_t realCommandLine;
        realCommandLine = csx_p_strbuild_always("xterm %s -e %s %s", do_geometry, do_prefix, cmd);
        system(realCommandLine);
        csx_p_strfree(realCommandLine);
    } else {
        system(cmd);
    }
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT - debugging hook */

    delete[] cmd;

    /*
     * now that the isolated test process has completed
     * Post returned status to the test
     */

    setTestStatus(listener->getTestStatus());
    delete listener;

	// Before calling test above we set the test result to 'unknown'.
	// Now we check the test result post test:
	// If 'unknown' then it hasn't been changed during the test so the test passed.
	// If 'failed' then the test has changed it and it has a problem.
	if(mTest->get_status() == MUT_TEST_RESULT_UKNOWN)
		setTestStatus(MUT_TEST_PASSED);

    /*
     * report status for this test
     */
    mControl->mut_log->mut_report_test_finished(mSuite, mTest, mTest->get_status_str());

}
