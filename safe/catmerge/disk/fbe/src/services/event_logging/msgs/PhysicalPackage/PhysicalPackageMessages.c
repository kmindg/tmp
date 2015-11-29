//**************************************************************************
// Copyright (C) Data General Corporation 2010
// All rights reserved.
// Licensed material -- property of Data General Corporation
//*************************************************************************

//  This is a dummy stub used to produce a message dll for event logging.
//  This may change with VC 6.0, but for now the stub is necessary to cause a
//  .LIB to be created.

#include "windows.h"
#include "csx_ext.h"

void CSX_MOD_EXPORT K10DummyMsgLibFunction(void);

void K10DummyMsgLibFunction(void)
{
    return;
}

BOOL APIENTRY DllMain(HANDLE h, DWORD dw, LPVOID pv)
{
    UNREFERENCED_PARAMETER( h );
    UNREFERENCED_PARAMETER( dw );
    UNREFERENCED_PARAMETER( pv );
    return TRUE;
}