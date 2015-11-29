/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                McrDriveSimControl.h
 ***************************************************************************
 *
 * DESCRIPTION: McrDriveSimControl class definition.  
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    01/2012 Created Wason Wang
 **************************************************************************/


#ifndef MCR_DRIVER_SIM_CONTROL_H
#define MCR_DRIVER_SIM_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/Fbe_platform.h"
#ifdef __cplusplus
}
#endif

class McrDriveSimControl {

public:
    McrDriveSimControl();
    virtual ~McrDriveSimControl();

public:	
    fbe_status_t bootDriveSimulator();
    fbe_status_t shutdownDriveSimulator(void);
    
    void setDriveTypeRemoteMemory(void);
    void restoreDriveTypeDefault(void);


private:    	
    //: start driver     
	static void driveSimControlThread(void* context);
	
	//: terminate driver
    void shutdownDriveProcess(void);

private:
    enum{ FILE_NAME_LENGTH = 128 };    
   
private:
    void* mpDriveThread;
    bool  mbIsRunning;
    static const char* mszSimDriveSimExeName;
    
    const unsigned char* mszDriveServerIP;
    fbe_api_sim_server_pid mDriveServerPID;
    
	//"fbe_test_drive_server_pid_%d.txt" : record PID
    char mszDriveServerPIDFileName[FILE_NAME_LENGTH];    

};

#endif//end MCR_DRIVER_SIM_CONTROL_H



