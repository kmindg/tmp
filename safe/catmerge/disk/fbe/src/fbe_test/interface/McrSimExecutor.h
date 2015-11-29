/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                McrSimExecutor.h
 ***************************************************************************
 *
 * DESCRIPTION: CMcrSimExecutor class definition.  
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    11/2011 Created WW
 **************************************************************************/

#ifndef MCR_SIM_EXECUTOR_H
#define MCR_SIM_EXECUTOR_H

//#include "McrTestSuitePhysicalPackageTest.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "fbe/fbe_types.h"
#ifdef __cplusplus
}
#endif

#include "McrDefaultSPController.h"
#include "McrDriveSimControl.h"
#include "simulation/BvdSimMutProgramBaseClass.h"


//control the SP sim process and the drive sim process
class McrEnvironment: 
    public McrDefaultSPController, public McrDriveSimControl
{
	public:
		McrEnvironment();
		virtual ~McrEnvironment();
		void killOrphan();
		void init(int pArgc, char **argv);
};
	

class McrSimExecutor: public BvdSimUnitTestSetup
{
public:

    McrSimExecutor(int Argc , char **argv, bool isExt = false, char const *name = NULL);

    virtual ~McrSimExecutor();

	fbe_status_t init(int Argc , char **argv);
    
    int run(); 

private:
	static McrEnvironment* mpEnvController; 

    fbe_bool_t mIsSelfTest;
	bool		mIsExtended;	

private:
	//static member for call back
     static void _startupSystemSuite(void);
     static void _startupBaseSuite(void);
     static void _startupDualspSuite(void);
     static void _teardownSystemSuite(void);   
    //static void _startupBaseSuitePostPowercycle(void);
     static void _teardownDualspSuite(void);
     static void _teardownBaseSuite(void);
    //static void _startupBaseSuiteSingleSP(fbe_sim_transport_connection_target_t sp);

    
    void _checkSpec(int argc, char **argv);
    void _generateSelfTestArg(int *argc_p, char ***argv_p);
    void _setConsoleTitle(int argc , char **argv);
    //void _processFbeIoDelay(void);

};

#endif//end MCR_SIM_EXECUTOR_H

