/************************************************************************
 *
 *  Copyright (c) 2011 EMC Corporation.
 *  All rights reserved.
 *
 *  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.  ACCESS TO THIS
 *  SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT. THIS CODE IS TO
 *  BE KEPT STRICTLY CONFIDENTIAL.
 *
 *  UNAUTHORIZED MODIFICATIONS OF THIS FILE WILL VOID YOUR SUPPORT
 *  CONTRACT WITH EMC CORPORATION.  IF SUCH MODIFICATIONS ARE FOR
 *  THE PURPOSE OF CIRCUMVENTING LICENSING LIMITATIONS, LEGAL ACTION
 *  MAY RESULT.
 *
 *
 ************************************************************************/

#ifndef __TestControlRegistry__h__
#define __TestControlRegistry__h__


//++
// File Name:
//      TestControlRegistry.h
//
// Contents:
//      This file holds the registry of drivers and features that use the
//      Test Control mechanism.
//
//      See services/TestControl/TestControlOverview.txt for an overview of
//      the Test Control mechanism.
//--


#include "TestControlUserDefs.h"


// Driver Registry
//
// Leave zero unassigned.
// Maximum value is (2^16)-1.
//
#define TctlDriver_SnapBack                             1
//
// Add more above here.


// Feature Registry
//
// Leave zero unassigned.
// Maximum value is (2^16)-1.
//
#define TctlFeature_Tctl                                1
#define TctlFeature_SnapBack                            2
//
// Add more above here.


#endif // __TestControlRegistry__h__
