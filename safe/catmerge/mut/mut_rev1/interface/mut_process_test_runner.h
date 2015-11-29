/***************************************************************************
 *                                mut_process_test_runner.h
 ***************************************************************************
 *
 * DESCRIPTION: MUT process test runner definition
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *
 **************************************************************************/
#include "mut_abstract_test_runner.h"
# include "simulation/Program_Option.h"

class Mut_test_control;
class Mut_testsuite;
class Mut_test_entry;

class Mut_process_test_runner: public Mut_abstract_test_runner {
public:
    Mut_process_test_runner(Mut_test_control *mut_control,
                            Mut_testsuite *current_suite,
                            Mut_test_entry *current_test);
    const char *getName();

    void setTestStatus(enum Mut_test_status_e status);
    Mut_test_entry *getTest(); // returns the current test

    void run();

private:

    char *constructCmdLine();
    bool optionOnExcludeList(Program_Option *option);

    Mut_test_control    *mControl;
    Mut_testsuite       *mSuite;
    Mut_test_entry      *mTest;
    bool                mRecursing;
};

