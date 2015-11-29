#ifndef CLUTCH_KERNEL_IF_H
#define CLUTCH_KERNEL_IF_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2003
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  DESCRIPTION
 *      This file defines the kernel level interface to the ASE
 *      clutch service.
 *   
 *
 *  HISTORY
 *      08-Jan-2003     Created     Joe Shimkus
 *
 ***************************************************************************/
 
#include "clutch_common_if.h"
#include "compare.h"
#include "EmcPrefast.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

/*
 * Define the DLL declaration.
 */
 
#ifdef  CLUTCH_EXPORT
#define CLUTCH_DLL          CSX_MOD_EXPORT
#else
#define CLUTCH_DLL          CSX_MOD_IMPORT
#endif // CLUTCH_EXPORT
 
/*
 * Constants.
 */
 
/* 
 * CLUTCH_NO_STATUS_ARRAY defines the constant to be used for
 * clutchPerformOperation() when the caller does not wish to specify an output
 * status array.
 */
 
#define CLUTCH_NO_STATUS_ARRAY          NULL

/*
 * Data Types.
 */
 
/*
 * CLUTCH_STATUS defines the type for the clutch status.
 */
 
typedef EMCPAL_STATUS       CLUTCH_STATUS;

/* 
 * CLUTCH_CALLBACK defines the function prototype for callback functions
 * registered with the clutch mechanism for clutch types.
 */
 
typedef CLUTCH_STATUS   (*CLUTCH_CALLBACK)( CLUTCH_MEMBER_ID    *memberId,
                                            UINT_32             operationId,
                                            VOID                *additonalArgument,
                                            UINT_32             additionalArgumentLength    );

/*
 * Function interfaces.
 */
 
CLUTCH_DLL  CLUTCH_STATUS   clutchAddMembers(   CLUTCH_ID           *clutchId,
                                                CLUTCH_MEMBER_LIST  memberList,
                                                UINT_32             entriesInList               );

CLUTCH_DLL  CLUTCH_STATUS   clutchContainsMember(   CLUTCH_ID           *clutchId,
                                                    CLUTCH_MEMBER_ID    *primaryMemberId,
                                                    BOOLEAN             *containsMember         );

CLUTCH_DLL  CLUTCH_STATUS   clutchCreate(       CLUTCH_NAME         clutchName,
                                                CLUTCH_TYPE         clutchType,
                                                CLUTCH_ATTRIBUTES   clutchAttributes,
                                                CLUTCH_ID           *clutchId                   );

CLUTCH_DLL  CLUTCH_STATUS   clutchDestroy(      CLUTCH_ID           *clutchId                   );

CLUTCH_DLL  CLUTCH_STATUS   clutchEnumerate(    CLUTCH_MEMBER_ID    *memberId,
                                                CLUTCH_ENUM_KEY     *enumKey,
                                                CLUTCH_LIST         clutchList,
                                                UINT_32             *entriesInList,
                                                UINT_32             *remainingClutches          );
                                            
CLUTCH_DLL  CLUTCH_STATUS   clutchEnumerateByType(  CLUTCH_TYPE         type,
                                                    CLUTCH_MEMBER_ID    *memberId,
                                                    CLUTCH_ENUM_KEY     *enumKey,
                                                    CLUTCH_LIST         clutchList,
                                                    UINT_32             *entriesInList,
                                                    UINT_32             *remainingClutches      );
                                                
CLUTCH_DLL  CLUTCH_STATUS   clutchEnumerateMembers( CLUTCH_ID               *clutchId,
                                                    CLUTCH_ENUM_MEMBER_KEY  *enumKey,
                                                    CLUTCH_MEMBER_LIST      memberList,
                                                    UINT_32                 *entriesInList,
                                                    UINT_32                 *remainingMembers   );
                                                
CLUTCH_DLL  CLUTCH_STATUS   clutchGetAttributes(    CLUTCH_ID           *clutchId,
                                                    UINT_32             *attributesLength,
                                                    CLUTCH_ATTRIBUTES   clutchAttributes,
                                                    UINT_32             *totalAttributesLength  );
                                                
CLUTCH_DLL  CLUTCH_STATUS   clutchPerformOperation( CLUTCH_ID           *clutchId,
                                                    UINT_32             operationId,
                                                    VOID                *additionalArgument,
                                                    UINT_32             additionalArgumentLength,
                                                    CLUTCH_MEMBER_ID    *firstFailingMember,
                                                    CLUTCH_STATUS       *statusArray            );
                                                
CLUTCH_DLL  CLUTCH_STATUS   clutchRegisterCallbackForType(  CLUTCH_TYPE     type,
                                                            CLUTCH_CALLBACK callbackFunction    );


CLUTCH_DLL  CLUTCH_STATUS   clutchRemoveMembers(    CLUTCH_ID           *clutchId,
                                                    CLUTCH_MEMBER_LIST  memberList,
                                                    UINT_32             entriesInList           );
                                                
CLUTCH_DLL  CLUTCH_STATUS   clutchSetAttributes(    CLUTCH_ID           *clutchId,
                                                    CLUTCH_ATTRIBUTES   clutchAttributes        );

CLUTCH_DLL  CLUTCH_STATUS   clutchSetName(          CLUTCH_ID       *clutchId,
                                                    CLUTCH_NAME     clutchName                  );

CLUTCH_DLL  CLUTCH_STATUS   clutchXlateIdToName(    CLUTCH_ID       *clutchId,
                                                    UINT_32         nameBufferLength,
                                                    CLUTCH_NAME     clutchName,
                                                    UINT_32         *nameLength                 );
                                                
CLUTCH_DLL  CLUTCH_STATUS   clutchXlateNameToId(    CLUTCH_NAME     clutchName,
                                                    CLUTCH_TYPE     clutchType,
                                                    CLUTCH_ID       *clutchId                   );

#endif  // CLUTCH_KERNEL_IF_H
