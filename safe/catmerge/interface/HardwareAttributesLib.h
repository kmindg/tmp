#ifndef __HARDWARE_ATTR_LIB__
#define __HARDWARE_ATTR_LIB__

/**********************************************************************
 *      Company Confidential
 *          Copyright (c) EMC Corp, 2003
 *          All Rights Reserved
 *          Licensed material - Property of EMC Corp
 **********************************************************************/

/**********************************************************************
 *
 *  File Name: 
 *          HardwareAttributesLib.h
 *
 *  Description:
 *          Function prototypes for the Hardware attributes functions 
 *          exported as a statically linked library.
 *
 *  Revision History:
 *          09-Dec-08       Phil Leef   Created
 *          
 **********************************************************************/

#include "spid_types.h"
#include "sfp_support.h"
#include "familyfruid.h"
#include "MiniportAnalogInfo.h"
#include "fbe_device_types.h"

#define MAX_FAMILY_ID           9
#define MAX_FRU_ID              MAX_IOMODULE_COUNT
#define MAX_BDF_FBE_MAPPINGS    100
#define MAX_BUS_NUMBER          256

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _HW_PLATFORM_ATTR
{
    ULONG           fltRegCount;
    ULONG           slavePortCount;
    ULONG           tempSensorCount;
    ULONG           powerSupplyCount;
    ULONG           mgmtModuleCount;
    ULONG           bmcCount;
    ULONG           fanPackCount;
    ULONG           fanCountPerPack;
    ULONG           mezzanineCount;
    ULONG           batteryCount;
    ULONG           suitcaseCount;
    ULONG           bemCount;
    ULONG           ioModuleCount;
    ULONG           dimmCount;
    ULONG           driveCount;
    ULONG           cacheCardCount;
} HW_PLATFORM_ATTR, *PHW_PLATFORM_ATTR;

/*************************
 * MODULE_PROTOCOL
 *************************/
typedef enum
{
    PROTOCOL_UNKNOWN    = 0x0,
    PROTOCOL_FIBRE      = 0x1,
    PROTOCOL_ISCSI      = 0x2,
    PROTOCOL_SAS        = 0x3,
    PROTOCOL_FCOE       = 0x4,
    PROTOCOL_AGNOSTIC   = 0x5,
} MODULE_PROTOCOL;

typedef struct _HW_MODULE_PORT_ATTR
{
    ULONG                       PHYmapping;
    BOOL                        SFPcapable;
    BOOL                        SFPaware;
    SFP_MEDIA_TYPE              SFPmedia;
    PORT_MINIPORT_ANALOG_INFO   miniportAnalogInfo;
} HW_MODULE_PORT_ATTR, *PHW_MODULE_PORT_ATTR;


typedef struct _HW_MODULE_CTRL_ATTR
{
    MODULE_PROTOCOL             protocol;
    ULONG                       availableLinkSpeeds;
    CTRL_MINIPORT_ANALOG_INFO   miniportAnalogInfo;
    ULONG                       ioPortCount;
    HW_MODULE_PORT_ATTR         portInfo[4];
} HW_MODULE_CTRL_ATTR, *PHW_MODULE_CTRL_ATTR;

typedef struct _HW_MODULE_PORT_INDEX_ATTR
{
    ULONG                   controllerIndex;
    ULONG                   relativePortIndex;
} HW_MODULE_PORT_INDEX_ATTR, *PHW_MODULE_PORT_INDEX_ATTR;

typedef struct _HW_DEVICE_ATTR
{    
    BOOL                        diplexFPGA;
    ULONG                       totalIoPortCount;
    HW_MODULE_PORT_INDEX_ATTR   absolutePortIndex[10];
    ULONG                       ctrlCount;
    HW_MODULE_CTRL_ATTR         ctrlInfo[3];
} HW_DEVICE_ATTR, *PHW_DEVICE_ATTR;

typedef struct _HW_MODULE_FAMILY_ID
{
    HW_DEVICE_ATTR              fruID[MAX_FRU_ID];
} HW_MODULE_FAMILY_ID, *PHW_MODULE_FAMILY_ID;

typedef struct _HW_MODULE_ATTR
{    
    HW_MODULE_FAMILY_ID         familyID[MAX_FAMILY_ID];
} HW_MODULE_ATTR, *PHW_MODULE_ATTR;



/*************************
 * BDF - FBE #
 *************************/
typedef enum _BDF_FBE_MODULE_NUM_e
{
   FBE_0,
   FBE_1,
   FBE_2,
   FBE_3,
   FBE_4,
   FBE_5,
   FBE_6,
   FBE_7,
   FBE_8,
   FBE_9,
   FBE_10,
   FBE_11,
   FBE_12,
   FBE_13,
   FBE_14,
   FBE_15,
   FBE_16,
   FBE_17,
   FBE_18,
   FBE_19,
   FBE_20,
   FBE_21,
   FBE_22,
   FBE_23,
   FBE_NUM_NA,
   FBE_NUM_INVALID = 0xFF,
   FBE_DIMM_INVALID = -1
} BDF_FBE_MODULE_NUM;

#define MAX_FBE_STRING_DESC  30
typedef struct _BRD_FBE_ID_INFO_
{
   BDF_FBE_MODULE_NUM fbe_num;
   UINT_64            fbeDevice;
   char               fbeDesc[MAX_FBE_STRING_DESC];
}BDF_FBE_INFO, *PBDF_FBE_INFO;


typedef struct _BDF_FBE_ATTR
{
    BDF_FBE_MODULE_NUM fbe_num;
    UINT_64            fbeDevice;
    ULONG              busNumber;
    ULONG              deviceNumber;
    ULONG              functionNumber;
}BDF_FBE_ATTR, *PBDF_FBE_ATTR;

/*************************
 * DIMM Configurations
 *************************/
#define MAX_DIMM_COUNT      24
#define MAX_DIMM_CONFIGS    2

typedef struct _HW_VALID_DIMM_CONFIG
{
    ULONG                   dimmCount;
    ULONG                   dimmDensity[MAX_DIMM_COUNT];
} HW_VALID_DIMM_CONFIG, *PHW_VALID_DIMM_CONFIG;

typedef struct _HW_VALID_DIMM_CONFIGS
{
    ULONG                   configCount;
    HW_VALID_DIMM_CONFIG    dimmConfig[MAX_DIMM_CONFIGS];
} HW_VALID_DIMM_CONFIGS, *PHW_VALID_DIMM_CONFIGS;


ULONG HwAttrLib_getIndexFromFamilyID(HW_MODULE_TYPE ffid);
void HwAttrLib_HardwarePlatformAttributesLibInitHwAttr(SPID_PLATFORM_INFO platformInfo);

USHORT HwAttrLib_getEnclosureSpsCount (SPID_PLATFORM_INFO *platformInfo,
                                       BOOL processorEnclosure,
                                       USHORT busNum,
                                       USHORT enclNum);
BOOL HwAttrLib_isBbuSelfTestSupported(HW_MODULE_TYPE ffid);

void HwAttrLib_initBdfFbeTranslationAttr(PSPID_PLATFORM_INFO platformInfo);

const char* HwAttrLib_decodeFbeType(UINT_64 fbeDevice);

void HwAttrLib_GetBdfFbeInfo(ULONG busNum,
                             ULONG deviceNum,
                             PBDF_FBE_INFO pfbe_info);

BOOL HwAttrLib_GetValidDimmConfigs(PSPID_PLATFORM_INFO      platformInfo,
                                   PHW_VALID_DIMM_CONFIGS   validDimmConfigs);


extern HW_PLATFORM_ATTR hwPlatformAttr[MAX_CPU_MODULE];
extern HW_MODULE_ATTR   hwModuleAttr;
extern ULONG            hildaFibreSpeeds;
extern ULONG            hildaToeSpeeds;

#ifdef __cplusplus
}
#endif

#endif // __HARDWARE_ATTR_LIB__
