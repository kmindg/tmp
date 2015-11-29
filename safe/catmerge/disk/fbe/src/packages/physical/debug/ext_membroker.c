/********************************************************
 * Copyright (C) EMC Corporation 1989 - 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ********************************************************/


#include "pp_dbgext.h"
#include "stdio.h"
#include "winbase.h"

#include "SharedMemory.h"
#include "MemBrokerInternal.h"

VOID MBDbgPrintSystemTime( PLARGE_INTEGER    PSystemTime  );

VOID MBDbgPrintClientInfo ( ULONG64 HeadAddr, ULONG64 ClientInfoAddr );

VOID MBDbgDisplayClientData( ULONG64 DisplayAddr );

//
//  Description:
//  This function prints global data of memory broker as well as
//  basic stats of all memory broker's clients.
//
//  Arguments:
//	NONE
//
//  Return Value:
//	NONE
//

CSX_DBG_EXT_DEFINE_CMD(membroker, "membroker")
{
    ULONG64 SmdGlobalsAddr;
    ULONG64 MBGlobalsAddr;
    ULONG32 Offset;
    BOOLEAN BoolVal;
    ULONG32 Value32;
    ULONG64 Value64;
    LARGE_INTEGER LargeIntVal;
    ULONG64 ClientArrayAddr;

    //
    // Mem Broker's global data is a part of SMD's global data.
    // So first get the address of SMD's global data and then
    // from that address get the address of Mem Broker's global
    // data.
    // 
    
    FBE_GET_EXPRESSION("Smd", SmdGlobals, &SmdGlobalsAddr);

    if( SmdGlobalsAddr == 0) 
    {
        csx_dbg_ext_print(" Error: Failed to get the address of Smd!SmdGlobals \n");
    }

    FBE_GET_FIELD_OFFSET("Smd", SMD_GLOBALS, "MemBroker", &Offset);

    MBGlobalsAddr = SmdGlobalsAddr + Offset;


    csx_dbg_ext_print("\n\n=========================================\n");
    csx_dbg_ext_print(" Memory Broker Global Data \n");
    csx_dbg_ext_print("=========================================\n");


    csx_dbg_ext_print(" MemBroker Global Data Address = 0x%I64x \n", MBGlobalsAddr);


    FBE_GET_FIELD_OFFSET("Smd", SMD_MEMBROKER_GLOBALS, "ClientHandleArray[0]", &Offset);

    ClientArrayAddr = MBGlobalsAddr + Offset ;

    FBE_GET_FIELD_DATA("Smd", MBGlobalsAddr, SMD_MEMBROKER_GLOBALS, "IsBrokerInitialized", sizeof(BoolVal), &BoolVal );
    csx_dbg_ext_print(" MemBroker Initialized = %s \n", (BoolVal == 0 ? "FALSE" : "TRUE"));

    FBE_GET_FIELD_DATA("Smd", MBGlobalsAddr, SMD_MEMBROKER_GLOBALS, "TotalNumOfClients", sizeof(Value32), &Value32 );
    csx_dbg_ext_print(" Num. of Clients = %d \n", Value32);

    FBE_GET_FIELD_DATA("Smd", MBGlobalsAddr, SMD_MEMBROKER_GLOBALS, "TotalDiscretionaryAlloc", sizeof(Value64), &Value64 );
    csx_dbg_ext_print(" Total Discretionary Allocs = %f MB \n", ((float)Value64)/((float)(1024*1024)));

    FBE_GET_FIELD_DATA("Smd", MBGlobalsAddr, SMD_MEMBROKER_GLOBALS, "TotalBalanceHint", sizeof(Value64), &Value64 );
    csx_dbg_ext_print(" Total Balance Hits = %I64d \n", Value64);

    FBE_GET_FIELD_DATA("Smd", MBGlobalsAddr, SMD_MEMBROKER_GLOBALS, "StationaryReserve", sizeof(Value64), &Value64 );
    csx_dbg_ext_print(" Stationary Reserve = %I64d MB \n", Value64 );

    FBE_GET_FIELD_DATA("Smd", MBGlobalsAddr, SMD_MEMBROKER_GLOBALS, "StationaryLowWatermark", sizeof(Value64), &Value64 );
    csx_dbg_ext_print(" Stationary Low Mark = %I64d MB \n", Value64);

    FBE_GET_FIELD_DATA("Smd", MBGlobalsAddr, SMD_MEMBROKER_GLOBALS, "StationaryHighWatermark", sizeof(Value64), &Value64 );
    csx_dbg_ext_print(" Stationary High Mark = %I64d MB \n", Value64);

    FBE_GET_FIELD_DATA("Smd", MBGlobalsAddr, SMD_MEMBROKER_GLOBALS, "LastRebalancingTime", sizeof(LARGE_INTEGER), &LargeIntVal );
    csx_dbg_ext_print(" Tick Count At Last Rebalancing = %I64d \n", LargeIntVal.QuadPart);

    FBE_GET_FIELD_DATA("Smd", MBGlobalsAddr, SMD_MEMBROKER_GLOBALS, "OutStandingRequests", sizeof(Value32), &Value32 );

    FBE_GET_FIELD_DATA("Smd", MBGlobalsAddr, SMD_MEMBROKER_GLOBALS, "StatisticsUpdateLock", sizeof(Value32), &Value32 );

    FBE_GET_FIELD_DATA("Smd", MBGlobalsAddr, SMD_MEMBROKER_GLOBALS, "FragmentationHighWatermark", sizeof(Value32), &Value32 );

    FBE_GET_FIELD_DATA("Smd", MBGlobalsAddr, SMD_MEMBROKER_GLOBALS, "FragmentationEliminationProcessInProgress", sizeof(Value32), &Value32 );




    // Now let's print all the client's data, but only if the strucutre is initialized.

    MBDbgPrintClientInfo(ClientArrayAddr, 0 );
}

#pragma data_seg ("EXT_HELP$4membroker")
static char membrokerUsageMsg[] = 
"!membroker\n"
"  Displays summary information of membroker global data and the clients registered to it\n";
#pragma data_seg (".data")


//
//  Description:
//  This function prints a specific client's memory broker data
//
//  Arguments:
//	NONE
//
//  Return Value:
//	NONE
//
//  History:
//      9-Dec-08 GSP  Added structure initalztion check, fixed argumrnt parsing
//

CSX_DBG_EXT_DEFINE_CMD(membroker_client_info, "membroker_client_info")
{
    ULONG64 ClientInfoAddr;
    ULONG64 SmdGlobalsAddr;
    ULONG64 MBGlobalsAddr;
    ULONG32 Offset;
    ULONG64 ClientArrayAddr;

    ClientInfoAddr = (ULONG64)GetArgument64(args, 1);

    if( ClientInfoAddr == 0) 
    {
        csx_dbg_ext_print(" Invalid Address \n");
        return;
    }

    FBE_GET_EXPRESSION("Smd", SmdGlobals, &SmdGlobalsAddr);

    if( SmdGlobalsAddr == 0) 
    {
        csx_dbg_ext_print(" Error: Failed to get the address of Smd!SmdGlobals \n");
    }

    FBE_GET_FIELD_OFFSET ("Smd", SMD_GLOBALS, "MemBroker", &Offset);

    
    MBGlobalsAddr = SmdGlobalsAddr + Offset;

    csx_dbg_ext_print(" MemBroker Global Data Address = 0x%I64x \n", MBGlobalsAddr);

    FBE_GET_FIELD_OFFSET ("Smd", SMD_MEMBROKER_GLOBALS, "ClientHandleArray[0]", &Offset);

    ClientArrayAddr = MBGlobalsAddr + Offset;

    MBDbgPrintClientInfo ( ClientArrayAddr, ClientInfoAddr );
}

#pragma data_seg ("EXT_HELP$4membroker_client_info")
static char membrokerClientUsageMsg[] = 
"!membroker_client_info <client_info_address>\n"
"  Displays summary information of a specific client registered to memory broker \n";
#pragma data_seg (".data")

//++
// Function:
//      MBDbgPrintClientInfo
//
// Description:
//      Displays client's data. All clients data will be displayed
//      if a client name is not passed as an argument.
//
// Arguments:
//      ClientArrayAddr - The address of the first element of the array.
//      ClientAddress  - Address of the client whose info is sought.
//  
// Returns:
//      None
//--

VOID MBDbgPrintClientInfo ( ULONG64 ClientArrayAddr, ULONG64 ClientInfoAddr )
{
    csx_s32_t  index;
    ULONG64 DisplayAddr=0;
    
    for (index = 0; index < NUMBER_OF_CLIENTS_SUPPORTED ; index++)
    {
        csx_dbg_ext_util_read_in_pointer(ClientArrayAddr+(sizeof(ULONG64) * index), &DisplayAddr );

        if (DisplayAddr == 0)
            continue;

        if( ClientInfoAddr == 0 ) 
        {
            // Display all the clients
            MBDbgDisplayClientData( DisplayAddr );
        }
        else
        {
            // Display only specified client

            if(ClientInfoAddr == DisplayAddr) 
            {
                MBDbgDisplayClientData( DisplayAddr );
            }
        }
        
    }

}


//++
// Function:
//      MBDbgPrintClientInfo
//
// Description:
//      Displays a client's data. 
// 
// Arguments:
//      DisplayAddr - Address of the data structure where client's data is present.            
//  
// Returns:
//      None
//--

VOID MBDbgDisplayClientData( ULONG64 DisplayAddr )
{
    CHAR    ClientName[BROKER_CLIENT_MAX_NAME_LENGTH];
    ULONG32 Offset;
    ULONG32 Value32;
    ULONG64 Value64;

    FBE_GET_FIELD_DATA_WITHOUT_MODULE_NAME(DisplayAddr, "_MEM_BROKER_CLIENT_INFO", "ClientName", BROKER_CLIENT_MAX_NAME_LENGTH, ClientName );
    csx_dbg_ext_print("\n\n=========================================\n");
    csx_dbg_ext_print(" Client Name = %s \n", ClientName);
    csx_dbg_ext_print("=========================================\n");

    csx_dbg_ext_print("Client Data Address 0x%I64x\n", DisplayAddr);

    FBE_GET_FIELD_DATA_WITHOUT_MODULE_NAME( DisplayAddr, "_MEM_BROKER_CLIENT_INFO", "Inertia", sizeof(Value32), &Value32 );
    csx_dbg_ext_print(" Inertia = %d \n", Value32);

    FBE_GET_FIELD_DATA_WITHOUT_MODULE_NAME( DisplayAddr, "_MEM_BROKER_CLIENT_INFO", "BalanceHint", sizeof(Value64), &Value64 );
    csx_dbg_ext_print(" Balance Hint = %I64d \n", Value64);

    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME("_MEM_BROKER_CLIENT_INFO", "pReclamationCallback", &Offset);


    csx_dbg_ext_util_read_in_pointer( (DisplayAddr + Offset), &Value64 );

    csx_dbg_ext_print(" Reclamation Callback Ptr = 0x%I64x \n", Value64);

    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME("_MEM_BROKER_CLIENT_INFO", "pReclamationCookie", &Offset);

    csx_dbg_ext_util_read_in_pointer( (DisplayAddr + Offset), &Value64 );

    csx_dbg_ext_print(" Reclamation Cookie Ptr = 0x%I64x \n", Value64);

    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME("_MEM_BROKER_CLIENT_INFO", "pNotificationCallback", &Offset);

    csx_dbg_ext_util_read_in_pointer( (DisplayAddr + Offset), &Value64 );

    csx_dbg_ext_print(" Notification Callback Ptr = 0x%I64x \n", Value64);

    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME("_MEM_BROKER_CLIENT_INFO", "pNotificationCookie", &Offset);

    csx_dbg_ext_util_read_in_pointer( (DisplayAddr + Offset), &Value64 );

    csx_dbg_ext_print(" Notification Cookie Ptr = 0x%I64x \n", Value64);

    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME("_MEM_BROKER_CLIENT_INFO", "pGetBalanceHintCallback", &Offset);

    csx_dbg_ext_print(" Error: Failed to obtain the offset of pGetBalanceHintCallback in MEM_BROKER_CLIENT_INFO \n");


    csx_dbg_ext_util_read_in_pointer( (DisplayAddr + Offset), &Value64 );

    csx_dbg_ext_print(" Get BalanceHint Callback = 0x%I64x \n", Value64);

    FBE_GET_FIELD_DATA_WITHOUT_MODULE_NAME(DisplayAddr, "_MEM_BROKER_CLIENT_INFO", "RequestNotification", sizeof(Value32), &Value32 );
    csx_dbg_ext_print(" Request Notification = %d \n", Value32);

    FBE_GET_FIELD_DATA_WITHOUT_MODULE_NAME(DisplayAddr, "_MEM_BROKER_CLIENT_INFO", "DiscretionaryTrigger", sizeof(Value64), &Value64 );
    csx_dbg_ext_print(" DiscretionaryTrigger = %f MB \n", ((float)Value64)/((float)(1024*1024)));

    FBE_GET_FIELD_DATA_WITHOUT_MODULE_NAME(DisplayAddr, "_MEM_BROKER_CLIENT_INFO", "NumFreesSinceLastRebalance", sizeof(Value64), &Value64 );
    csx_dbg_ext_print(" NumFreesSinceLastRebalance =  0x%I64x \n", Value64);

    FBE_GET_FIELD_DATA_WITHOUT_MODULE_NAME(DisplayAddr, "_MEM_BROKER_CLIENT_INFO", "ReferenceBalancePercentage", sizeof(Value64), &Value64 );
    csx_dbg_ext_print(" Reference Balance Percentage =  %I64d %% \n", Value64);

    FBE_GET_FIELD_DATA_WITHOUT_MODULE_NAME(DisplayAddr, "_MEM_BROKER_CLIENT_INFO", "BaselineBalanceGoal", sizeof(Value64), &Value64 );
    csx_dbg_ext_print(" Baseline Balance Goal = %f MB \n", ((float)Value64)/((float)(1024*1024)));

    FBE_GET_FIELD_DATA_WITHOUT_MODULE_NAME(DisplayAddr, "_MEM_BROKER_CLIENT_INFO", "InertiaAdjustmentPercentage", sizeof(Value64), &Value64 );
    csx_dbg_ext_print(" Inertia Adjustment Percentage =  %I64d %% \n", Value64);

    FBE_GET_FIELD_DATA_WITHOUT_MODULE_NAME(DisplayAddr, "_MEM_BROKER_CLIENT_INFO", "AdjustedBalanceGoal", sizeof(Value64), &Value64 );
    csx_dbg_ext_print(" Adjusted Balance Goal = %f MB \n", ((float)Value64)/((float)(1024*1024)));

    FBE_GET_FIELD_DATA_WITHOUT_MODULE_NAME(DisplayAddr, "_MEM_BROKER_CLIENT_INFO", "AmountOverAdjustedBalanceGoal", sizeof(Value64), &Value64 );
    csx_dbg_ext_print(" Amount Over Adjusted Balance Goal = %f MB \n", ((float)Value64)/((float)(1024*1024)));

    FBE_GET_FIELD_DATA_WITHOUT_MODULE_NAME(DisplayAddr, "_MEM_BROKER_CLIENT_INFO", "RecallPercentage", sizeof(Value64), &Value64 );
    csx_dbg_ext_print(" Recall Percentage =  %I64d %% \n", Value64);

    FBE_GET_FIELD_DATA_WITHOUT_MODULE_NAME(DisplayAddr, "_MEM_BROKER_CLIENT_INFO", "RecallAmount", sizeof(Value64), &Value64 );
    csx_dbg_ext_print(" Recall Amount = %f MB \n", ((float)Value64)/((float)(1024*1024)));

    FBE_GET_FIELD_DATA_WITHOUT_MODULE_NAME(DisplayAddr, "_MEM_BROKER_CLIENT_INFO", "SuggestedDiscretionaryAllocAmount", sizeof(Value64), &Value64 );
    csx_dbg_ext_print(" Suggested Discretionary Alloc Amount = %f MB \n", ((float)Value64)/((float)(1024*1024)));

    FBE_GET_FIELD_DATA_WITHOUT_MODULE_NAME( DisplayAddr, "_MEM_BROKER_CLIENT_INFO", "CurrentDiscretionaryAlloc", sizeof(Value64), &Value64 );
    csx_dbg_ext_print(" Current Discretionary Alloc = %f MB \n", ((float)Value64)/((float)(1024*1024)));

}   




// 
//++
// Function:
//      MBDbgPrintSystemTime
//
// Description:
//      Displays a human readable version of the NT system time.
//
// Arguments:
//      PSystemTime    - Pointer to a large integer that contains the system
//                       time expressed as ticks since Jan 1, 1970.
//
// Returns:
//      None
//--

VOID
MBDbgPrintSystemTime( PLARGE_INTEGER    PSystemTime  )
{
    BOOLEAN       bSuccess         = TRUE;
    SYSTEMTIME    systemTime       = {0};
    FILETIME      fileTime         = {0};
    FILETIME      localFileTime    = {0};

    if (bSuccess)
    {
        fileTime.dwLowDateTime  = (DWORD) PSystemTime->LowPart;
        fileTime.dwHighDateTime = (DWORD) PSystemTime->HighPart;

        bSuccess = FileTimeToLocalFileTime(
                       &fileTime,
                       &localFileTime
                       );

        if (!bSuccess)
        {
            csx_dbg_ext_print ("NOT SET");
        }
    }

    if (bSuccess)
    {
        bSuccess = FileTimeToSystemTime(
                       &localFileTime,
                       &systemTime
                       );

        if (!bSuccess)
        {
            csx_dbg_ext_print ("NOT SET");
        }
    }
    if (bSuccess)
    {
        csx_dbg_ext_print(
            "%02d:%02d:%02d.%03d %02d/%02d/%4d",
            systemTime.wHour,
            systemTime.wMinute,
            systemTime.wSecond,
            systemTime.wMilliseconds,
            systemTime.wMonth,
            systemTime.wDay,
            systemTime.wYear
            );
    }

}

