
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                McrDefaultSPController.h
 ***************************************************************************
 *
 * DESCRIPTION: McrDefaultSPController class definition.  
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    01/2012 Created Wason Wang
 **************************************************************************/

#ifndef MCR_DEFAULT_SP_CONTROLLER_H
#define MCR_DEFAULT_SP_CONTROLLER_H


#include "simulation/DefaultSPControl.h"

//choosing which SP to debug
class McrDefaultSPController: public DefaultSPControl {

public:
    McrDefaultSPController();
    virtual ~McrDefaultSPController();

public:
	void initSPControllerWithOption(int pArgc, char **argv);	
    static const char* mszSimSPExeName;
};

#endif//end MCR_DEFAULT_SP_CONTROLLER_H




