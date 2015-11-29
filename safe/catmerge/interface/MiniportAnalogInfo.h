//***************************************************************************
// Copyright (C) EMC Corporation 2009
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//++
// File Name:
//      MiniportAnalogInfo.h
//
// Contents:
//      Defines the analog settings for the Miniport
//
// Revision History:
//      05/09/10    Phil Leef       Created.
//
//--
#ifndef _MINIPORT_ANALOG_INFORMATION_H_
#define _MINIPORT_ANALOG_INFORMATION_H_

#include "generic_types.h"

#define MAX_MINIPORT_PORT_ANALOG_PARAMETERS  10
#define MAX_MINIPORT_CTRL_ANALOG_PARAMETERS  2

#pragma pack(1)

typedef struct
{
    ULONG       SPAST_REG0;
    ULONG       SPAST_REG1;
    ULONG       SPAST_REG2;
    ULONG       SPAST_REG3;
    ULONG       SPAST_REG4;
    ULONG       SPAST_REG5;
} SPCV_PM80x9_PORT_ANALOG_PARAMETERS;

typedef struct
{
    ULONG       SPAST_REG0;
    ULONG       SPAST_REG1;
    ULONG       SPAST_REG2;
    ULONG       SPAST_REG3;
    ULONG       SPAST_REG4;
    ULONG       SPAST_REG5;
    ULONG       SPAST_REG6;
    ULONG       SPAST_REG7;
    ULONG       SPAST_REG8;
} SPCV_PM807x_PORT_ANALOG_PARAMETERS;

typedef union
{
    ULONG                               AnalogSettings[MAX_MINIPORT_PORT_ANALOG_PARAMETERS];
    SPCV_PM80x9_PORT_ANALOG_PARAMETERS  PM80x9AnalogSettings;
    SPCV_PM807x_PORT_ANALOG_PARAMETERS  PM807xAnalogSettings;
} PORT_MINIPORT_ANALOG_INFO;

typedef struct
{
    ULONG       reserved;
} SPCV_PM80x9_CTRL_ANALOG_PARAMETERS;

typedef struct
{
    ULONG       reserved;
} SPCV_PM807x_CTRL_ANALOG_PARAMETERS;

typedef union
{
    ULONG                               AnalogSettings[MAX_MINIPORT_CTRL_ANALOG_PARAMETERS];
    SPCV_PM80x9_CTRL_ANALOG_PARAMETERS  PM80x9AnalogSettings;
    SPCV_PM807x_CTRL_ANALOG_PARAMETERS  PM807xAnalogSettings;
} CTRL_MINIPORT_ANALOG_INFO;


#pragma pack()

#endif  // _MINIPORT_ANALOG_INFORMATION_H_
