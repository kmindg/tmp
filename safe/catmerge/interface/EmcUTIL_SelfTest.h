#ifndef EMCUTIL_SELFTEST_H_
#define EMCUTIL_SELFTEST_H_

//***************************************************************************
// Copyright (C) EMC Corporation 2011
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

/* file  EmcUTIL_SelfTest.h */
//
// Contents:
//      The master header file for the EMCUTIL utility SelfTest APIs.

CSX_CDECLS_BEGIN

////////////////////////////////////

CSX_GLOBAL csx_bool_t EmcutilRegSelfTestA(
    csx_bool_t beVerbose);                 // run self-test normal variant

CSX_GLOBAL csx_bool_t EmcutilRegSelfTestW(
    csx_bool_t beVerbose);                 // run self-test Unicode variant

////////////////////////////////////

CSX_GLOBAL csx_bool_t EmcutilRegSelfTestPLA(
    csx_bool_t beVerbose);

////////////////////////////////////

CSX_GLOBAL void Emcutil_RunCheckTimeTest(void);

////////////////////////////////////
CSX_GLOBAL void Emcutil_RunSetTimeTest(void);

CSX_CDECLS_END

//++
//.End file EmcUTIL_SelfTest.h
//--

#endif                                     /* EMCUTIL_SELFTEST_H_ */
