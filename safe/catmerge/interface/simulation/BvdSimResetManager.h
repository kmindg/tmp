/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               BvdSimResetManager.h
 ***************************************************************************
 *
 * DESCRIPTION:  Declaration of the BvdResetManager class
 *               This facade hides the details of the BvdSimDeviceManager
 *               from application code.
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    01/31/2010  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __BVDSIMRESETMANAGER__
#define __BVDSIMRESETMANAGER__
# include "ResetManager.h"

#if defined(WDMSERVICES_EXPORT)
#define WDMSERVICEPUBLIC CSX_MOD_EXPORT
#else
#define WDMSERVICEPUBLIC CSX_MOD_IMPORT
#endif

class BvdSimResetManager: public ResetManager
{
public:
    static WDMSERVICEPUBLIC BvdSimResetManager *getInstance();
    void WDMSERVICEPUBLIC reset();
private:
    BvdSimResetManager();
};
#endif
