//  File: bgs_exports.h     Component: Background Services Shared Header File

#ifndef BGS_EXPORTS_H
#define BGS_EXPORTS_H

//-----------------------------------------------------------------------------
//  Copyright (C) EMC Corporation 2008
//  All rights reserved.
//  Licensed material -- property of EMC Corporation
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
//  MODULE ABSTRACT:  Background Services Shared/Exported Header File
//
//    This is header file for definitions specific to Background Services which
//     are used both within BGS and by other Modules (such as DBA and Flare/CM)
//
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:
//


//-----------------------------------------------------------------------------
//  NAMED CONSTANTS (#defines):
//


//-----------------------------------------------------------------------------
//  ENUMERATIONS:
//


//  Background Services Monitor Types
//  There is 1 monitor for each background service within the BGS environment.
//  Background Services are referred to by the enumeration of their monitor.
typedef enum bgs_monitor_types_enum
{
    BGS_MON_TYPE_NONE   = 0,
    BGS_MON_TYPE_SWAP,                  
    BGS_MON_TYPE_MIN    = BGS_MON_TYPE_SWAP,
    BGS_MON_TYPE_DB_RB,                  
    BGS_MON_TYPE_REB,                   
    BGS_MON_TYPE_EQZ,                
    BGS_MON_TYPE_PACO,         
    BGS_MON_TYPE_BV,                 
    BGS_MON_TYPE_LZ,               
    BGS_MON_TYPE_DZ,   
    BGS_MON_TYPE_EXP_DEF,       
    BGS_MON_TYPE_SV,           
    // End list of monitors

    BGS_MON_TYPE_MAX   = BGS_MON_TYPE_SV,
    
    // BGS_MON_TYPE_ALL is a special reserved Monitor type that identifies *all*
    //  background services. It is used by some background service halt/unhalt
    //  applications.
    //  It *must* be out of the range of BGS_MON_TYPE_MIN->BGS_MON_TYPE_MAX as
    //  it does not identify a unique background service.
    BGS_MON_TYPE_ALL = (BGS_MON_TYPE_MAX+1)

} bgs_monitor_types_enum_t;


//-----------------------------------------------------------------------------
//  TYPEDEFS: 
//


//-----------------------------------------------------------------------------
//  MACROS:


//-----------------------------------------------------------------------------
//  FUNCTION PROTOTYPES: 
//


#endif // BGS_EXPORTS_H
