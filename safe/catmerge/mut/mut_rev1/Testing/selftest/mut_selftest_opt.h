#ifndef MUT_SELFTEST_OPT_H
#define MUT_SELFTEST_OPT_H

#include "simulation/Program_Options.h"

// this is a "user" defined option that mut_selftest uses.
class mut_selftest_option : public String_option {
public:
    mut_selftest_option(): String_option("-selftest", "exename", 2, false,""){};
  
};
#endif
