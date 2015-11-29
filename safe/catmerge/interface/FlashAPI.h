//
// Copyright (C) 2007   EMC Corporation
//
//
//
//  Revision History
//  ----------------
//  20-Feb-07   PhaniMVS        Initial version.
//  21-Feb-07   Sabry           Adding functions For Flash image Deletion and FlashICA Automation tool.
//

#ifndef _FlashAPI_h
#define _FlashAPI_h

#include <windows.h>
#include "CaptivePointer.h"
#include "K10TraceMsg.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

typedef enum 
{
  FlashIca_Success = 0,
  FlashIca_Failure = 1,
  FlashIca_NotFound = 2
}FlashIca_Success_Failure;

class CSX_CLASS_EXPORT FlashAPI
{
public:
    
    // Constructor and destructor

    FlashAPI() {  
            INIT_K10TRACE(mnpK, mpProc, GLOBALMANAGEMENT_STRING);
        }

    ~FlashAPI() {}

    //      Opens a named pipe to Flare. Return ERROR_SUCCESS.
    DWORD   ConnectToFlare( PHANDLE phFLARE );

    //      Reads 512 bytes data from flare asynchronousily
    DWORD   ReadFromFlare( HANDLE hFlare, LPSTR szBuf, LPOVERLAPPED pOverlapped);

    //      Write data to FLARE asynchronousily.
    DWORD   WriteToFlare( HANDLE hFlare, char *pBuf, int len, LPOVERLAPPED lpOverlapped);

    //      Returns true if disks are invalidated
    bool    InvalidateDisks();

    bool    GetFlashDrive( LPSTR pszDrv,CHAR *ptrIniFilePath );

    bool    IsFlashDrv(LPCSTR drv,CHAR *pptrIniFilePath);

    FlashIca_Success_Failure    DeleteFlashImage(); 

    bool    IsSimulationMode(LPCSTR lpFName);

    bool    ExecuteInvalidateDisks(bool spStatus);
private:

#pragma warning(disable:4251)
    NPtr<K10TraceMsg>       mnpK;
#pragma warning(default:4251)

    char*                   mpProc;

};

#endif // _FlashAPI_h

