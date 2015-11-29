#ifndef CLUTCH_USER_IF_H
#define CLUTCH_USER_IF_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2003
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  DESCRIPTION
 *      This file defines the user level interface to the ASE
 *      clutch service provided via the Clutch Thin Driver (CTD).
 *   
 *
 *  HISTORY
 *      08-Jan-2003     Created     Joe Shimkus
 *
 ***************************************************************************/
 
#include "clutch_common_if.h"
//#include "devioctl.h"

/*
 * Constants.
 */
 
/*
 * Device IOCTLs.
 */

// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.  (Quoted from "/ddk/inc/devioctl.h".)

#define FILE_DEVICE_CLUTCHES			0x9000	// unique in devioctl.h

#define CLUTCHES_CONTROL_CODE(operation)                                \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_CLUTCHES, (operation), EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

/*
 * IOCTL_CLUTCHES_GET_OPERATION defines the IOCTL to use when accessing information
 * from clutches.
 */

#define IOCTL_CLUTCHES_GET_OPERATION    CLUTCHES_CONTROL_CODE(0x800)

/*
 * IOCTL_CLUTCHES_SET_OPERATION defines the IOCTL to use when setting information
 * in clutches.
 */

#define IOCTL_CLUTCHES_SET_OPERATION    CLUTCHES_CONTROL_CODE(0x900)

/*
 * Clutch device name.
 *
 * The device name is used by clients in order to communicate with the CTD.
 */

#define CLUTCH_DEVICE_NAME              "\\\\.\\clutchThinDriver"
 
/*
 * Argument type IDs.
 */
 
#define CLUTCH_TYPE_MINIMUM             0

#define CLUTCH_TYPE_UINT_32             (CLUTCH_TYPE_MINIMUM)

#define CLUTCH_TYPE_CLUTCH_ATTRIBUTES   (CLUTCH_TYPE_UINT_32+1)
#define CLUTCH_TYPE_CLUTCH_ENUM_KEY     (CLUTCH_TYPE_CLUTCH_ATTRIBUTES+1)
#define CLUTCH_TYPE_CLUTCH_ID           (CLUTCH_TYPE_CLUTCH_ENUM_KEY+1)
#define CLUTCH_TYPE_CLUTCH_LIST         (CLUTCH_TYPE_CLUTCH_ID+1)
#define CLUTCH_TYPE_CLUTCH_NAME         (CLUTCH_TYPE_CLUTCH_LIST+1)
#define CLUTCH_TYPE_CLUTCH_TYPE         (CLUTCH_TYPE_CLUTCH_NAME+1)
#define CLUTCH_TYPE_MEMBER_ENUM_KEY     (CLUTCH_TYPE_CLUTCH_TYPE+1)
#define CLUTCH_TYPE_MEMBER_ID           (CLUTCH_TYPE_MEMBER_ENUM_KEY+1)
#define CLUTCH_TYPE_MEMBER_LIST         (CLUTCH_TYPE_MEMBER_ID+1)

#define CLUTCH_TYPE_MAXIMUM             (CLUTCH_TYPE_MEMBER_LIST)
#define CLUTCH_TYPE_NUMBER_OF_TYPES     (CLUTCH_TYPE_MAXIMUM+1)

/*
 * "Get" operation IDs.
 */
 
#define CLUTCH_GET_MINIMUM              0

#define CLUTCH_CONTAINS_MEMBER          (CLUTCH_GET_MINIMUM)
#define CLUTCH_ENUMERATE                (CLUTCH_CONTAINS_MEMBER+1)
#define CLUTCH_ENUMERATE_BY_TYPE        (CLUTCH_ENUMERATE+1)
#define CLUTCH_ENUMERATE_MEMBERS        (CLUTCH_ENUMERATE_BY_TYPE+1)
#define CLUTCH_GET_ATTRIBUTES           (CLUTCH_ENUMERATE_MEMBERS+1)
#define CLUTCH_XLATE_ID_TO_NAME         (CLUTCH_GET_ATTRIBUTES+1)
#define CLUTCH_XLATE_NAME_TO_ID         (CLUTCH_XLATE_ID_TO_NAME+1)

#define CLUTCH_GET_MAXIMUM              (CLUTCH_XLATE_NAME_TO_ID)
#define CLUTCH_GET_NUMBER_OF_OPS        (CLUTCH_GET_MAXIMUM+1)

/*
 * "Set" operation identifiers.
 */
 
#define CLUTCH_SET_MINIMUM              0

#define CLUTCH_ADD_MEMBERS              (CLUTCH_SET_MINIMUM)
#define CLUTCH_CREATE                   (CLUTCH_ADD_MEMBERS+1)
#define CLUTCH_DESTROY                  (CLUTCH_CREATE+1)
#define CLUTCH_REMOVE_MEMBERS           (CLUTCH_DESTROY+1)
#define CLUTCH_SET_ATTRIBUTES           (CLUTCH_REMOVE_MEMBERS+1)
#define CLUTCH_SET_NAME                 (CLUTCH_SET_ATTRIBUTES+1)

#define CLUTCH_SET_MAXIMUM              (CLUTCH_SET_NAME)
#define CLUTCH_SET_NUMBER_OF_OPS        (CLUTCH_SET_MAXIMUM+1)

/*
 * Data Types.
 */

/* 
 * CLUTCH_ARGUMENT_COUNT defines the type used to specify the number of
 * arguments passed through the CTD.
 */
 
typedef UINT_8                          CLUTCH_ARGUMENT_COUNT;

/*
 * CLUTCH_ARGUMENT_LENGTH defines the type used to specify the length of individual
 * arguments (in bytes) passed through the CTD.
 */
 
typedef UINT_32                         CLUTCH_ARGUMENT_LENGTH;

/*
 * CLUTCH_ARGUMENT_TYPE defines the type used to specify the type of individual
 * arguments passed through the CTD.
 */
 
typedef UINT_8                          CLUTCH_ARGUMENT_TYPE;

/*
 * CLUTCH_OPERATION defines the type used to specify the clutch operation being
 * performed via the CTD.
 */
 
typedef UINT_8                          CLUTCH_OPERATION;
 
/*
 * Macros.
 */
 
/* 
 * The following macros are provided to facilitate decoding and encoding of
 * arguments communicated through the CTD.  All macros take as their first input a
 * pointer to the pointer addressing the buffer being deconstructed/constructed. 
 * This is so that the macro may update the pointer for use in subsequent macro
 * invocations.
 */

#define CLUTCH_DECODE_ARGUMENT_COUNT(bufferPtrPtr, countPtr)    \
do                                                              \
{                                                               \
    char    *acBufferPtr;                                       \
                                                                \
    acBufferPtr = *(bufferPtrPtr);                              \
                                                                \
    *(CLUTCH_ARGUMENT_COUNT *)(countPtr) =                      \
                    *(CLUTCH_ARGUMENT_COUNT *)acBufferPtr;      \
    acBufferPtr += sizeof(CLUTCH_ARGUMENT_COUNT);               \
                                                                \
    *(bufferPtrPtr) = acBufferPtr;                              \
}                                                               \
while (FALSE)

#define CLUTCH_DECODE_ARGUMENT_LENGTH(bufferPtrPtr, lengthPtr)  \
do                                                              \
{                                                               \
    char    *alBufferPtr;                                       \
                                                                \
    alBufferPtr = *(bufferPtrPtr);                              \
                                                                \
    *(CLUTCH_ARGUMENT_LENGTH *)(lengthPtr) =                    \
                        *(CLUTCH_ARGUMENT_LENGTH *)alBufferPtr; \
    alBufferPtr += sizeof(CLUTCH_ARGUMENT_LENGTH);              \
                                                                \
    *(bufferPtrPtr) = alBufferPtr;                              \
}                                                               \
while (FALSE)

#define CLUTCH_DECODE_ARGUMENT_TYPE(bufferPtrPtr, typePtr)      \
do                                                              \
{                                                               \
    char    *atBufferPtr;                                       \
                                                                \
    atBufferPtr = *(bufferPtrPtr);                              \
                                                                \
    *(CLUTCH_ARGUMENT_TYPE *)(typePtr) =                        \
                    *(CLUTCH_ARGUMENT_TYPE *)atBufferPtr;       \
    atBufferPtr += sizeof(CLUTCH_ARGUMENT_TYPE);                \
                                                                \
    *(bufferPtrPtr) = atBufferPtr;                              \
}                                                               \
while (FALSE)

#define CLUTCH_DECODE_OPERATION(bufferPtrPtr, operationPtr)     \
do                                                              \
{                                                               \
    char    *opBufferPtr;                                       \
                                                                \
    opBufferPtr = *(bufferPtrPtr);                              \
                                                                \
    *(CLUTCH_OPERATION *)(operationPtr) =                       \
                    *(CLUTCH_OPERATION *)opBufferPtr;           \
    opBufferPtr += sizeof(CLUTCH_OPERATION);                    \
                                                                \
    *(bufferPtrPtr) = opBufferPtr;                              \
}                                                               \
while (FALSE)


/*
 * Encoding macros.
 */

#define CLUTCH_ENCODE_ARGUMENT_COUNT(bufferPtrPtr, count)       \
do                                                              \
{                                                               \
    char    *acBufferPtr;                                       \
                                                                \
    acBufferPtr = *(bufferPtrPtr);                              \
                                                                \
    *(CLUTCH_ARGUMENT_COUNT *)acBufferPtr = (count);            \
    acBufferPtr += sizeof(CLUTCH_ARGUMENT_COUNT);               \
                                                                \
    *(bufferPtrPtr) = acBufferPtr;                              \
}                                                               \
while (FALSE)

#define CLUTCH_ENCODE_ARGUMENT_LENGTH(bufferPtrPtr, length)     \
do                                                              \
{                                                               \
    char    *alBufferPtr;                                       \
                                                                \
    alBufferPtr = *(bufferPtrPtr);                              \
                                                                \
    *(CLUTCH_ARGUMENT_LENGTH *)alBufferPtr = (length);          \
    alBufferPtr += sizeof(CLUTCH_ARGUMENT_LENGTH);              \
                                                                \
    *(bufferPtrPtr) = alBufferPtr;                              \
}                                                               \
while (FALSE)

#define CLUTCH_ENCODE_ARGUMENT_TYPE(bufferPtrPtr, type)         \
do                                                              \
{                                                               \
    char    *atBufferPtr;                                       \
                                                                \
    atBufferPtr = *(bufferPtrPtr);                              \
                                                                \
    *(CLUTCH_ARGUMENT_TYPE *)atBufferPtr = (type);              \
    atBufferPtr += sizeof(CLUTCH_ARGUMENT_TYPE);                \
                                                                \
    *(bufferPtrPtr) = atBufferPtr;                              \
}                                                               \
while (FALSE)

#define CLUTCH_ENCODE_CLUTCH_ATTRIBUTES(bufferPtrPtr,           \
                                        attributes,             \
                                        length)                 \
do                                                              \
{                                                               \
    char    *caBufferPtr;                                       \
                                                                \
    caBufferPtr   = *(bufferPtrPtr);                            \
                                                                \
    CLUTCH_ENCODE_ARGUMENT_TYPE(&caBufferPtr,                   \
                                CLUTCH_TYPE_CLUTCH_ATTRIBUTES); \
                                                                \
    CLUTCH_ENCODE_ARGUMENT_LENGTH(&caBufferPtr, (length));      \
                                                                \
    memcpy(caBufferPtr, (char *) (attributes), (length));       \
    caBufferPtr += (length);                                    \
                                                                \
    *(bufferPtrPtr) = caBufferPtr;                              \
}                                                               \
while (FALSE)
                                        
#define CLUTCH_ENCODE_CLUTCH_ENUM_KEY(bufferPtrPtr,             \
                                        enumKey)                \
do                                                              \
{                                                               \
    char    *cekBufferPtr;                                      \
                                                                \
    cekBufferPtr   = *(bufferPtrPtr);                           \
                                                                \
    CLUTCH_ENCODE_ARGUMENT_TYPE(&cekBufferPtr,                  \
                                CLUTCH_TYPE_CLUTCH_ENUM_KEY);   \
                                                                \
    CLUTCH_ENCODE_ARGUMENT_LENGTH(&cekBufferPtr,                \
                                    sizeof(CLUTCH_ENUM_KEY));   \
                                                                \
    memcpy(cekBufferPtr, (char *) &(enumKey),                   \
                                    sizeof(CLUTCH_ENUM_KEY));   \
    cekBufferPtr += sizeof(CLUTCH_ENUM_KEY);                    \
                                                                \
    *(bufferPtrPtr) = cekBufferPtr;                             \
}                                                               \
while (FALSE)

#define CLUTCH_ENCODE_CLUTCH_ID(bufferPtrPtr,                   \
                                clutchId)                       \
do                                                              \
{                                                               \
    char    *ciBufferPtr;                                       \
                                                                \
    ciBufferPtr   = *(bufferPtrPtr);                            \
                                                                \
    CLUTCH_ENCODE_ARGUMENT_TYPE(&ciBufferPtr,                   \
                                CLUTCH_TYPE_CLUTCH_ID);         \
                                                                \
    CLUTCH_ENCODE_ARGUMENT_LENGTH(&ciBufferPtr,                 \
                                    sizeof(CLUTCH_ID));         \
                                                                \
    memcpy(ciBufferPtr, (char *) &(clutchId),                   \
                                        sizeof(CLUTCH_ID));     \
    ciBufferPtr += sizeof(CLUTCH_ID);                           \
                                                                \
    *(bufferPtrPtr) = ciBufferPtr;                              \
}                                                               \
while (FALSE)

#define CLUTCH_ENCODE_CLUTCH_LIST(bufferPtrPtr,                 \
                                    clutchList,                 \
                                    numberInList)               \
do                                                              \
{                                                               \
    char    *clBufferPtr;                                       \
                                                                \
    clBufferPtr   = *(bufferPtrPtr);                            \
                                                                \
    CLUTCH_ENCODE_ARGUMENT_TYPE(&clBufferPtr,                   \
                                CLUTCH_TYPE_CLUTCH_LIST);       \
                                                                \
    CLUTCH_ENCODE_ARGUMENT_LENGTH(&clBufferPtr,                 \
                            sizeof(CLUTCH_ID)*(numberInList));  \
                                                                \
    memcpy(clBufferPtr,                                         \
            (char *) (clutchList),                              \
            sizeof(CLUTCH_ID)*(numberInList));                  \
    clBufferPtr += sizeof(CLUTCH_ID)*(numberInList);            \
                                                                \
    CLUTCH_ENCODE_UINT_32(&clBufferPtr, (numberInList));        \
                                                                \
    *(bufferPtrPtr) = clBufferPtr;                              \
}                                                               \
while (FALSE)

#define CLUTCH_ENCODE_CLUTCH_NAME(bufferPtrPtr,                 \
                                    name,                       \
                                    length)                     \
do                                                              \
{                                                               \
    char    *cnBufferPtr;                                       \
                                                                \
    cnBufferPtr   = *(bufferPtrPtr);                            \
                                                                \
    CLUTCH_ENCODE_ARGUMENT_TYPE(&cnBufferPtr,                   \
                                CLUTCH_TYPE_CLUTCH_NAME);       \
                                                                \
    CLUTCH_ENCODE_ARGUMENT_LENGTH(&cnBufferPtr, (length));      \
                                                                \
    memcpy(cnBufferPtr, (char *) (name), (length));             \
    cnBufferPtr += (length);                                    \
                                                                \
    *(bufferPtrPtr) = cnBufferPtr;                              \
}                                                               \
while (FALSE)

#define CLUTCH_ENCODE_CLUTCH_TYPE(bufferPtrPtr,                 \
                                    clutchType)                 \
do                                                              \
{                                                               \
    char    *ctBufferPtr;                                       \
                                                                \
    ctBufferPtr   = *(bufferPtrPtr);                            \
                                                                \
    CLUTCH_ENCODE_ARGUMENT_TYPE(&ctBufferPtr,                   \
                                CLUTCH_TYPE_CLUTCH_TYPE);       \
                                                                \
    CLUTCH_ENCODE_ARGUMENT_LENGTH(&ctBufferPtr,                 \
                                    sizeof(CLUTCH_TYPE));       \
                                                                \
    memcpy(ctBufferPtr,                                         \
            (char *) &(clutchType),                             \
                    sizeof(CLUTCH_TYPE));                       \
    ctBufferPtr += sizeof(CLUTCH_TYPE);                         \
                                                                \
    *(bufferPtrPtr) = ctBufferPtr;                              \
}                                                               \
while (FALSE)

#define CLUTCH_ENCODE_MEMBER_ENUM_KEY(bufferPtrPtr,             \
                                        enumKey)                \
do                                                              \
{                                                               \
    char    *mekBufferPtr;                                      \
                                                                \
    mekBufferPtr   = *(bufferPtrPtr);                           \
                                                                \
    CLUTCH_ENCODE_ARGUMENT_TYPE(&mekBufferPtr,                  \
                                CLUTCH_TYPE_MEMBER_ENUM_KEY);   \
                                                                \
    CLUTCH_ENCODE_ARGUMENT_LENGTH(&mekBufferPtr,                \
                            sizeof(CLUTCH_ENUM_MEMBER_KEY));    \
                                                                \
    memcpy(mekBufferPtr, (char *) &(enumKey),                   \
            sizeof(CLUTCH_ENUM_MEMBER_KEY));                    \
    mekBufferPtr += sizeof(CLUTCH_ENUM_MEMBER_KEY);             \
                                                                \
    *(bufferPtrPtr) = mekBufferPtr;                             \
}                                                               \
while (FALSE)

#define CLUTCH_ENCODE_MEMBER_ID(bufferPtrPtr,                   \
                                memberId)                       \
do                                                              \
{                                                               \
    char    *miBufferPtr;                                       \
                                                                \
    miBufferPtr   = *(bufferPtrPtr);                            \
                                                                \
    CLUTCH_ENCODE_ARGUMENT_TYPE(&miBufferPtr,                   \
                                CLUTCH_TYPE_MEMBER_ID);         \
                                                                \
    CLUTCH_ENCODE_ARGUMENT_LENGTH(&miBufferPtr,                 \
                                    sizeof(CLUTCH_MEMBER_ID));  \
                                                                \
    memcpy(miBufferPtr, (char *) &(memberId),                   \
                                sizeof(CLUTCH_MEMBER_ID));      \
    miBufferPtr += sizeof(CLUTCH_MEMBER_ID);                    \
                                                                \
    *(bufferPtrPtr) = miBufferPtr;                              \
}                                                               \
while (FALSE)

#define CLUTCH_ENCODE_MEMBER_LIST(bufferPtrPtr,                 \
                                    memberList,                 \
                                    numberInList)               \
do                                                              \
{                                                               \
    char    *mlBufferPtr;                                       \
                                                                \
    mlBufferPtr   = *(bufferPtrPtr);                            \
                                                                \
    CLUTCH_ENCODE_ARGUMENT_TYPE(&mlBufferPtr,                   \
                                CLUTCH_TYPE_MEMBER_LIST);       \
                                                                \
    CLUTCH_ENCODE_ARGUMENT_LENGTH(&mlBufferPtr,                 \
                    sizeof(CLUTCH_MEMBER_ID)*(numberInList));   \
                                                                \
    memcpy(mlBufferPtr,                                         \
            (char *) (memberList),                              \
            sizeof(CLUTCH_MEMBER_ID)*(numberInList));           \
    mlBufferPtr += sizeof(CLUTCH_MEMBER_ID)*(numberInList);     \
                                                                \
    CLUTCH_ENCODE_UINT_32(&mlBufferPtr, (numberInList));        \
                                                                \
    *(bufferPtrPtr) = mlBufferPtr;                              \
}                                                               \
while (FALSE)

#define CLUTCH_ENCODE_OPERATION(bufferPtrPtr, operation)        \
do                                                              \
{                                                               \
    char    *opBufferPtr;                                       \
                                                                \
    opBufferPtr = *(bufferPtrPtr);                              \
                                                                \
    *(CLUTCH_OPERATION *)opBufferPtr = (operation);             \
    opBufferPtr += sizeof(CLUTCH_OPERATION);                    \
                                                                \
    *(bufferPtrPtr) = opBufferPtr;                              \
}                                                               \
while (FALSE)

#define CLUTCH_ENCODE_UINT_32(bufferPtrPtr, uint32)             \
do                                                              \
{                                                               \
    char    *u32BufferPtr;                                      \
                                                                \
    u32BufferPtr = *(bufferPtrPtr);                             \
                                                                \
    CLUTCH_ENCODE_ARGUMENT_TYPE(&u32BufferPtr,                  \
                                CLUTCH_TYPE_UINT_32);           \
                                                                \
    CLUTCH_ENCODE_ARGUMENT_LENGTH(&u32BufferPtr,                \
                                  sizeof(UINT_32));             \
                                                                \
    memcpy(u32BufferPtr, (char *) &(uint32), sizeof(UINT_32));  \
    u32BufferPtr += sizeof(UINT_32);                            \
                                                                \
    *(bufferPtrPtr) = u32BufferPtr;                             \
}                                                               \
while (FALSE)

/*
 * The "get" macros assume that they are pointing at the
 * particular type with which they deal.  In other words, the
 * type has already been decoded and skipped over by a call to
 * CLUTCH_DECODE_ARGUMENT_TYPE().
 */
 
#define CLUTCH_GET_CLUTCH_ATTRIBUTES_PTR(bufferPtrPtr,          \
                                            attributesPtr)      \
do                                                              \
{                                                               \
    char                    *capBufferPtr;                      \
    CLUTCH_ARGUMENT_LENGTH  argumentLength;                     \
                                                                \
    capBufferPtr = *(bufferPtrPtr);                             \
                                                                \
    CLUTCH_DECODE_ARGUMENT_LENGTH(&capBufferPtr, &argumentLength); \
                                                                \
    *(CLUTCH_ATTRIBUTES *)(attributesPtr) =                     \
                                (CLUTCH_ATTRIBUTES)capBufferPtr;\
                                                                \
    capBufferPtr += argumentLength;                             \
                                                                \
    *(bufferPtrPtr) = capBufferPtr;                             \
}                                                               \
while (FALSE)
                                        
#define CLUTCH_GET_CLUTCH_ENUM_KEY_PTR(bufferPtrPtr,            \
                                        enumKeyPtrPtr)          \
do                                                              \
{                                                               \
    char                    *cekpBufferPtr;                     \
    CLUTCH_ARGUMENT_LENGTH  argumentLength;                     \
                                                                \
    cekpBufferPtr = *(bufferPtrPtr);                            \
                                                                \
    CLUTCH_DECODE_ARGUMENT_LENGTH(&cekpBufferPtr, &argumentLength); \
                                                                \
    memcpy(enumKeyPtrPtr, &cekpBufferPtr, sizeof(void *));       \
                                                                \
    cekpBufferPtr += argumentLength;                            \
                                                                \
    *(bufferPtrPtr) = cekpBufferPtr;                            \
}                                                               \
while (FALSE)                                        

#define CLUTCH_GET_CLUTCH_ID_PTR(bufferPtrPtr,                  \
                                    clutchIdPtrPtr)             \
do                                                              \
{                                                               \
    char                    *cipBufferPtr;                      \
    CLUTCH_ARGUMENT_LENGTH  argumentLength;                     \
                                                                \
    cipBufferPtr = *(bufferPtrPtr);                             \
                                                                \
    CLUTCH_DECODE_ARGUMENT_LENGTH(&cipBufferPtr, &argumentLength); \
                                                                \
    *(CLUTCH_ID **)(clutchIdPtrPtr) = (CLUTCH_ID *)cipBufferPtr;   \
                                                                \
    cipBufferPtr += argumentLength;                             \
                                                                \
    *(bufferPtrPtr) = cipBufferPtr;                             \
}                                                               \
while (FALSE)                                        

#define CLUTCH_GET_CLUTCH_LIST_PTR(bufferPtrPtr,                \
                                    clutchListPtr,              \
                                    numberInListPtrPtr)         \
do                                                              \
{                                                               \
    char                    *clpBufferPtr;                         \
    CLUTCH_ARGUMENT_LENGTH  argumentLength;                     \
    CLUTCH_ARGUMENT_TYPE    argumentType;                       \
                                                                \
    clpBufferPtr = *(bufferPtrPtr);                             \
                                                                \
    CLUTCH_DECODE_ARGUMENT_LENGTH(&clpBufferPtr, &argumentLength); \
                                                                \
    *(CLUTCH_LIST *)(clutchListPtr) = (CLUTCH_LIST)clpBufferPtr;\
    clpBufferPtr += argumentLength;                             \
                                                                \
    CLUTCH_DECODE_ARGUMENT_TYPE(&clpBufferPtr, &argumentType);  \
                                                                \
    CLUTCH_GET_UINT_32_PTR(&clpBufferPtr, (numberInListPtrPtr));\
                                                                \
    *(bufferPtrPtr) = clpBufferPtr;                             \
}                                                               \
while (FALSE)

#define CLUTCH_GET_CLUTCH_NAME_PTR(bufferPtrPtr,                \
                                    namePtr)                    \
do                                                              \
{                                                               \
    char                    *cnpBufferPtr;                      \
    CLUTCH_ARGUMENT_LENGTH  argumentLength;                     \
                                                                \
    cnpBufferPtr = *(bufferPtrPtr);                             \
                                                                \
    CLUTCH_DECODE_ARGUMENT_LENGTH(&cnpBufferPtr, &argumentLength); \
                                                                \
    *(CLUTCH_NAME *)(namePtr) = (CLUTCH_NAME)cnpBufferPtr;      \
                                                                \
    cnpBufferPtr += argumentLength;                             \
                                                                \
    *(bufferPtrPtr) = cnpBufferPtr;                             \
}                                                               \
while (FALSE)

#define CLUTCH_GET_CLUTCH_TYPE_PTR(bufferPtrPtr,                \
                                    clutchTypePtrPtr)           \
do                                                              \
{                                                               \
    char                    *ctpBufferPtr;                      \
    CLUTCH_ARGUMENT_LENGTH  argumentLength;                     \
                                                                \
    ctpBufferPtr = *(bufferPtrPtr);                             \
                                                                \
    CLUTCH_DECODE_ARGUMENT_LENGTH(&ctpBufferPtr, &argumentLength); \
                                                                \
    *(CLUTCH_TYPE **)(clutchTypePtrPtr) =                       \
                                    (CLUTCH_TYPE *)ctpBufferPtr;\
                                                                \
    ctpBufferPtr += argumentLength;                             \
                                                                \
    *(bufferPtrPtr) = ctpBufferPtr;                             \
}                                                               \
while (FALSE)                                        

#define CLUTCH_GET_MEMBER_ENUM_KEY_PTR(bufferPtrPtr,            \
                                        enumKeyPtrPtr)          \
do                                                              \
{                                                               \
    char                    *mekpBufferPtr;                     \
    CLUTCH_ARGUMENT_LENGTH  argumentLength;                     \
                                                                \
    mekpBufferPtr = *(bufferPtrPtr);                            \
                                                                \
    CLUTCH_DECODE_ARGUMENT_LENGTH(&mekpBufferPtr, &argumentLength); \
                                                                \
    memcpy(enumKeyPtrPtr, &mekpBufferPtr, sizeof(void *));   \
                                                                \
    mekpBufferPtr += argumentLength;                            \
                                                                \
    *(bufferPtrPtr) = mekpBufferPtr;                            \
}                                                               \
while (FALSE)                                        

#define CLUTCH_GET_MEMBER_ID_PTR(bufferPtrPtr,                  \
                                    memberIdPtrPtr)             \
do                                                              \
{                                                               \
    char                    *mipBufferPtr;                      \
    CLUTCH_ARGUMENT_LENGTH  argumentLength;                     \
                                                                \
    mipBufferPtr = *(bufferPtrPtr);                             \
                                                                \
    CLUTCH_DECODE_ARGUMENT_LENGTH(&mipBufferPtr, &argumentLength); \
                                                                \
    *(CLUTCH_MEMBER_ID **)(memberIdPtrPtr) =                    \
                                (CLUTCH_MEMBER_ID *)mipBufferPtr;  \
                                                                \
    mipBufferPtr += argumentLength;                             \
                                                                \
    *(bufferPtrPtr) = mipBufferPtr;                             \
}                                                               \
while (FALSE)                                        

#define CLUTCH_GET_MEMBER_LIST_PTR(bufferPtrPtr,                \
                                    memberListPtr,              \
                                    numberInListPtrPtr)         \
do                                                              \
{                                                               \
    char                    *mlpBufferPtr;                         \
    CLUTCH_ARGUMENT_LENGTH  argumentLength;                     \
    CLUTCH_ARGUMENT_TYPE    argumentType;                       \
                                                                \
    mlpBufferPtr = *(bufferPtrPtr);                             \
                                                                \
    CLUTCH_DECODE_ARGUMENT_LENGTH(&mlpBufferPtr, &argumentLength); \
                                                                \
    *(CLUTCH_MEMBER_LIST *)(memberListPtr) =                    \
                                (CLUTCH_MEMBER_LIST)mlpBufferPtr;  \
                                                                \
    mlpBufferPtr += argumentLength;                             \
                                                                \
    CLUTCH_DECODE_ARGUMENT_TYPE(&mlpBufferPtr, &argumentType);  \
                                                                \
    CLUTCH_GET_UINT_32_PTR(&mlpBufferPtr, (numberInListPtrPtr));\
                                                                \
    *(bufferPtrPtr) = mlpBufferPtr;                             \
}                                                               \
while (FALSE)

#define CLUTCH_GET_UINT_32_PTR(bufferPtrPtr, uint32PtrPtr)      \
do                                                              \
{                                                               \
    char                    *u32pBufferPtr;                     \
    CLUTCH_ARGUMENT_LENGTH  argumentLength;                     \
                                                                \
    u32pBufferPtr = *(bufferPtrPtr);                            \
                                                                \
    CLUTCH_DECODE_ARGUMENT_LENGTH(&u32pBufferPtr, &argumentLength); \
                                                                \
    *(UINT_32 **)(uint32PtrPtr) = (UINT_32 *)u32pBufferPtr;     \
                                                                \
    u32pBufferPtr += argumentLength;                            \
                                                                \
    *(bufferPtrPtr) = u32pBufferPtr;                            \
}                                                               \
while (FALSE)

#endif  // CLUTCH_USER_IF_H
