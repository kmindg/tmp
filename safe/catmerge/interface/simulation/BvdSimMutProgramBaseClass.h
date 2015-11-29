/***************************************************************************
 * Copyright (C) EMC Corporation 2012-2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               BvdSimMutProgramBaseClass.h
 ***************************************************************************
 *
 * DESCRIPTION:  Declaration/implementation of the BvdSimMutProgramBaseClass API 
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    06/02/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __BVDSIMMUTPROGRAMBASECLASS__
#define __BVDSIMMUTPROGRAMBASECLASS__
# include "EmcUTIL_CsxShell_Interface.h"

#include "csx_ext.h"
#include "csx_ext_p_stdio.h"
 
# include "simulation/ArrayDirector.h"
# include "simulation/Program_Option.h"
# include "simulation/SpId_option.h"
# include "simulation/Segment_option.h"
# include "simulation/RBA_option.h"
# include "simulation/PlatformConfig_option.h"
# include "simulation/CacheSize_option.h"
# include "ktrace.h"
# include <stdio.h>

#include "simulation/arguments.h"
#include "processorCount.h"

/*
 * Test programs use these CLIare initialized in BvdSimUnitTestSetup
 * These global pointers are located in BvdSimMutBaseClass.cpp
 */
extern Bool_option *useExistingSystemDevices;
extern Bool_option *leaveSystemDevices;
extern Bool_option *useNVCM;

/* CG - start grman disabled we'll enable it in mut_init */
#define MY_CONTAINER_ARGS MUT_STANDALONE_TEST_DEFAULT_CONTAINER_ARGS

/*  
 *  This opt_virtual_affinity_mask setting will keep the cache tests off of core 0 to improve developer workstation interactivity.
 *  We can set the CPU affinity mask to only expose up to 20 CPUs to the container as that is all the cache tests appear to
 *  support.  We are going to limit the cache tests to the first X-1 CPUs.   X is currently set to 20 which is the MaxCPUs that
 *  MCC is limited to.
 *
 *  Note that for linux we add opt_default_stack_size.   This is to limit the amount of stack used by each thread.
 */
#if defined(ALAMOSA_WINDOWS_ENV)
#define MY_RT_ARGS_FORMAT "opt_use_realtime_threads=1 opt_virtual_affinity_mask=0x%llx opt_dpc_run_asap=1 opt_trace_all_output=1"
#else //defined(ALAMOSA_WINDOWS_ENV)
#define MY_RT_ARGS_FORMAT "opt_use_realtime_threads=1 opt_virtual_affinity_mask=0x%llx opt_dpc_run_asap=1 opt_trace_all_output=1 opt_default_stack_size=393216"
#endif //defined(ALAMOSA_WINDOWS_ENV)

typedef void (*BvdSimSuiteInit)(mut_testsuite_t *suite);

// Maximum number of cpus our system currently supports.   If there are less csx will ignore the bits
// for any CPU that is not there.   If there are more cpus, csx will be limited by the mask.   We will
// not use CPU 0.  UPDATE to 32 for Hyperion
const static UINT_32 MAXCPUS_SUPPORTED_BY_MCC = MAX_CORES;

// Dynmically generate the opt_virtual_affinity_mask to be used for this run of tests.   We limit
// the CPUs to the MAXCPUS that the platform supports.   Currently MCC limits itself to 20 CPUs.
// Note:  While this code could just return a mask, it should ideally determine the number of CPUs
// on the system and compare that number to max supported by MCC.   Then we could additionally have 
// the tests run on different CPUs than the SPs are running on.
static char* GenerateMY_RT_ARGS(int argc, char **argv) 
{
    static char rtArgs[512];
    csx_u64_t platformCPUs = 0;
    
    // use the input argc and argv as arguments for command line processing.
    Arguments::init(argc,argv);
    
    // Look for the opt_virtual_affinity_mask command line option
    HexInt64_option* processor_mask = new HexInt64_option("-opt_virtual_affinity_mask", "0x1-0xFFFFFFFFFFFFFFFE default 0xFFFFF", "Virtual CPU mask for testing ",
        ((csx_u64_t) 0xFFFFFFFFFFFFFFFE), true, false);
    
    memset(rtArgs, 0, sizeof(rtArgs));

    // See if we got the processor mask as an input parameter.
    if(processor_mask->get() != 0) {
        // The user has specified an affinity mask, validate it.
        UINT_32 bitCount = 0;
        for(UINT_32 index = 0; index < 64; index++) {
            if(processor_mask->get() & ((csx_u64_t) 1 << index)) {
                bitCount++;
            }
        }
        if(bitCount < MAXCPUS_SUPPORTED_BY_MCC) {
            platformCPUs = processor_mask->get();
        } else {
            platformCPUs = (((csx_u64_t) 1 << platformCPUs) - (csx_u64_t) 1) & (csx_u64_t) ~0x1;
        }
            
    } else {
        platformCPUs = (((csx_u64_t) 1 << platformCPUs) - (csx_u64_t) 1) & (csx_u64_t) ~0x1;
    }
    // If the mask is empty, then just use 1 cpu.
    if(!platformCPUs) {
        platformCPUs = 1;
    }
    csx_p_sprintf(rtArgs, MY_RT_ARGS_FORMAT, platformCPUs);
    return rtArgs;
}

class BvdSimUnitTestSetup {
public:
    BvdSimUnitTestSetup(int argc , char ** argv, char const *suiteName, BvdSimSuiteInit suiteInit = NULL)
    : mCSXProgramContext(MY_CONTAINER_ARGS, GenerateMY_RT_ARGS(argc,argv)),
      mArgc(argc), mArgv(argv), mSuiteInit(suiteInit), mName(suiteName) {

        Arguments::resetArgs(argc,argv);
        
        useExistingSystemDevices = new Bool_option("-useExistingSystemDevices", "Do not delete system devices, during setup");
        leaveSystemDevices = new Bool_option("-leaveSystemDevices", "Do not delete system devices, during tearDown");
        useNVCM = new Bool_option("-NonVolatileCacheMirror","Use NVCM for mirroring cache");
    }

/* CGCG - had to move this to avoid breaking the EmcUTIL Shell mechanism */
    void *operator new (size_t size) throw (){
        if (size == 0) size = 1;
         return malloc(size);
     }
    void operator delete (void *p){
        free(p);
    }

    virtual ~BvdSimUnitTestSetup() {
        delete useExistingSystemDevices;
        delete leaveSystemDevices;
        delete useNVCM;
    }

    virtual int execute(mut_function_t SetUp = NULL, mut_function_t CleanUp = NULL) {
        init_mut();

        mSuite = mut_create_testsuite_advanced(mName, SetUp, CleanUp);

        if(mSuiteInit != NULL) {
            (*mSuiteInit)(mSuite);
        }
        else {
            populateSuite(mSuite);
        }
        
        MUT_RUN_TESTSUITE(mSuite);
        return 0;
    }

    virtual void populateSuite(mut_testsuite_t *suite) {};

    virtual const char *getSuiteName() {
        return mName;
    }

    virtual void init_mut() {
        mut_init(getArgC(), getArgV());
    }
    

    int              getArgC() { return mArgc; }
    char           **getArgV() { return mArgv; }


private:
    Emcutil_Shell_MainApp mCSXProgramContext;
    int             mArgc;
    char            **mArgv;
    char            const *mName;
    mut_testsuite_t *mSuite;
    void            (*mSuiteInit)(mut_testsuite_t *suite);
};

class BvdSimPeerSPTestSetup: public BvdSimUnitTestSetup {
public:
    BvdSimPeerSPTestSetup(int argc , char ** argv, char const *name, BvdSimSuiteInit suiteInit = NULL)
    : BvdSimUnitTestSetup(argc, argv, name, suiteInit), mSuiteInit(suiteInit) {
        /*
         * Access the SPId_option singleton, which causes it to be registered
         * in the MUT options list. Otherwise, The program command line
         * can't be processed properly
         */
                
        Arguments::resetArgs(argc,argv); 
         
        SpId_option::getCliSingleton();
        Master_IcId_option::getCliSingleton();
        Cache_Size_option::getCliSingleton();
        Platform_Config_option::getCliSingleton();
    }

    virtual int execute(mut_function_t SetUp = NULL, mut_function_t CleanUp = NULL) {
        /*
         * Note, during BvdSimPeerSPTestSetup instance creation,
         * field mSpId was initialized.
         * This field is an option. The root option class constructor
         * adds all options to the global list of options, that MUT
         * uses during initialization.
         *
         *  Once mSpId is on the global options list
         *  it will be processed by the mut_init cli processor
         */

        /*
         * Initialize MUT.
         * While parsing the CLI, the mSpId, mSegment
         */
        init_mut();


        /*
         * Only configure SP environment when CLI arguments are specified
         */

        if(SpId_option::getCliSingleton()->isSet()) {
            /*
             * This process is a Peer SP simulation process.
             * Report back to the calling process and start
             * the debugger, as needed.
             *
             */
            ArrayDirector::ConfigureDebugger(SpId_option::getCliSingleton()->get());
            ArrayDirector::handle_simulation_query(SpId_option::getCliSingleton()->get());
        }
        

        /*
         * NOTE, MUT args -list and -help provide useful information about this program
         * When they are specified on the command line, the program will not execute tests
         * 
         * A fully populated Suite is required for -list to function, which is why
         * The suite initialization is not contained within the conditional logic above
         *
         * Fundimentally, this program needs to be invoked in one of the following ways:
         *      program -list       generates the list of tests contained in suite
         *      program -help       generates the command line arguments that configure program
         *      program -SP -segment required to configure SP process before tests execute
         */

        mSuite = mut_create_testsuite_advanced(getSuiteName(), SetUp, CleanUp);

        if(mSuiteInit != NULL) {
            (*mSuiteInit)(mSuite);
        }
        else {
           populateSuite(mSuite); 
        }

        MUT_RUN_TESTSUITE(mSuite);

        return 0;
    }

    SpId_option *getSpIdOption() {
        return SpId_option::getCliSingleton();
    }

private:
    mut_testsuite_t *mSuite;
    void            (*mSuiteInit)(mut_testsuite_t *suite);
};

//
// NOTE: This class should not be used as a basis for attempting to create 3 legged stool tests
//   All new test development, whether or not it is a multi-sp test should  be done with
//  the classes found in BvdSim3LMutBaseClass.h   Check out the PassThru DualSP3L example
//
class BvdSimRemoteSPTestSetup: public BvdSimPeerSPTestSetup {
public:
    BvdSimRemoteSPTestSetup(int argc, char **argv, char const *name)
    : BvdSimPeerSPTestSetup(argc, argv, name) {
        csx_p_strcpy(mComposedName, "");
        if(!getSpIdOption()->isSet()) {
            KtraceSetPrefix("Control");
        } else {
            if(getSpIdOption()->get() == 0) {
              KtraceSetPrefix("SPA");
            } else {
              KtraceSetPrefix("SPB");
            }
        }
    }
    const char *getSuiteName() {
        if(csx_p_strlen(mComposedName) == 0) {
            csx_p_strcpy(mComposedName, BvdSimUnitTestSetup::getSuiteName());
            csx_p_strcat(mComposedName, "_");
            if(!getSpIdOption()->isSet()) {
                csx_p_strcat(mComposedName, "control");
            }
            else {
                if(getSpIdOption()->get() == 0) {
                    csx_p_strcat(mComposedName, "SPA_childP");
                }
                else {
                    csx_p_strcat(mComposedName, "SPB_childP");
                }
            }
        }

        return (const char *)mComposedName;
    }

private:
    char mComposedName[1024];
};

class SP_Script_option: public String_option {
public:
    SP_Script_option(const char *programName,const char *key, const char *help, int argCount = 1)
    : String_option(key, "{script}", argCount, TRUE, help) {
        strcat_s(logName, sizeof(logName), programName);
        strcat_s(logName, sizeof(logName), "_");
        const char *ptr = key+1;  // skip over the dash
        strcpy_s(logName, sizeof(logName), ptr);
    }

    const char *getLogName() {
        return (const char *)logName;
    };

private:
    char logName[32];
};

class SPA_Script_option: public SP_Script_option {
public:
    SPA_Script_option(const char *programName, 
                        const char *help = "Dual SP simulation, script that executes on SPA")
    : SP_Script_option(programName, "-SPA", help, 1) {}
};

class SPB_Script_option: public SP_Script_option {
public:
    SPB_Script_option(const char *programName, 
                        const char *help = "Dual SP simulation, script that executes on SPB")
    : SP_Script_option(programName, "-SPB", help, 1) {}
};

class SPAOnly_Script_option: public SP_Script_option {
public:
    SPAOnly_Script_option(const char *programName,
                            const char *help = "Single SP simulation, script that executes on SPA")
    : SP_Script_option(programName, "-SPAOnly", help, 1) {}
};

class BvdSimSPScriptSetup: public BvdSimPeerSPTestSetup {
private:
    SP_Script_option *spaOnlyCli;
    SP_Script_option *spaCli;
    SP_Script_option *spbCli;
    SP_Script_option *mSelectedOption;

public:
    BvdSimSPScriptSetup(const char *programName, int argc , char ** argv)
    : BvdSimPeerSPTestSetup(argc, argv, programName) {
        /*
         * Instantiate required options
         * They will automatically be registered on the MUT cli list
         */
        spaOnlyCli    = new SPAOnly_Script_option(programName);
        spaCli        = new SPA_Script_option(programName);
        spbCli        = new SPB_Script_option(programName);
        mSelectedOption = NULL;
    }

    SP_Script_option *getSelectedOption() {
        if(mSelectedOption == NULL) {
            if(spaCli->isSet()) {
                mSelectedOption = spaCli;
            }
            else {
                if(spaOnlyCli->isSet()) {
                    mSelectedOption = spaOnlyCli;
                }
                else {
                    if(spbCli->isSet()) {
                        mSelectedOption = spbCli;
                    }
                    else {
                        // use spaOnlyCli when none are specified
                        mSelectedOption = spaOnlyCli;
                    }
                }
            }
        }

        /*
         * Should really abort if one of the SP optons was not specified
         */

        return mSelectedOption;
    }

    SP_Script_option *getSpaOnlyOption() {
        return spaOnlyCli;
    }

    SP_Script_option *getSpaOption() {
        return spaCli;
    }

    SP_Script_option *getSpbOption() {
        return spbCli;
    }

    const char *getSuiteName() {
        return getSelectedOption()->getLogName();
    }
    virtual void populateSuite(mut_testsuite_t *suite) = 0;
};

#endif // __BVDSIMMUTPROGRAMBASECLASS__
