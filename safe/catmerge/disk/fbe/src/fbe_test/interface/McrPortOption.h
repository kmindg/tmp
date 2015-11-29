/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               McrPortOption.h
 ***************************************************************************
 *
 * DESCRIPTION: Set port base of SP , CMI and driver.
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    07/02/2012  Wason Wang Initial Version
 *
 **************************************************************************/

#ifndef __PORT_BASE_OPTION_H_
#define __PORT_BASE_OPTION_H_

#include "csx_ext.h"
#include "simulation/Program_Option.h"


class port_base_option: public Int_option {
public:
    port_base_option(UINT_32 value = 0, bool record = true);
    UINT_32 getSPPort();
    UINT_32 getCMIport();
    UINT_32 getDrivePort();
	static port_base_option *getCliSingleton();
};

#endif


