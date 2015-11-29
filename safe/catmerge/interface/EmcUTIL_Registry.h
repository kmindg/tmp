#ifndef EMCUTIL_REGISTRY_H_
#define EMCUTIL_REGISTRY_H_

//***************************************************************************
// Copyright (C) EMC Corporation 2011
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

/*! @file  EmcUTIL_Registry.h
 *
 */

// Header file for the utility Registry-like APIs.

////////////////////////////////////

#include "EmcUTIL.h"

CSX_CDECLS_BEGIN

/*! @addtogroup emcutil_registry_abs
 *  @{
 * 
 * @brief
 * Here is a universal "Registry" abstraction that should work equally well,
 * with the same key scheme used in CLARiiON on Windows, in all use cases.
 *
 *  - Windows kernel space in Windows CLARiiON use case - talks to real Registry
 *  - Windows user space in Windows CLARiiON use case - talks to real Registry (UMODE_ENV)
 *  - Windows user space in Windows CLARiiON use case - talks to private Registry provided by CSX (SIMMODE_ENV)
 *  - CSX backed use cases (Linux) - talks to Registry provided by CSX
 *
 * This Registry abstraction is in theory universal. All direct access to the Windows Registry
 * should be replaceable by this abstraction.
 *
 * This Registry abstraction is based loosely on the Windows user space Registry API set 'model'
 * with a large number of convenience wrappers for cases where the richness of the open-key/act/close-key
 * model is more than is required.
 *
 * This Registry abstraction is bascially intended to give one-liner style access to the Registry
 * for the most common use cases (get or set a value), while also providing almost a direct
 * mapping for more complex Registry usage coded today to the Windows user space Registry API set.
 *
 * This Registry abstraction is dual Unicode/normal capable - using the same FooBar[A|W] model
 * that Microsoft uses for the Windows user space Registry API set. Why is this done? This is
 * done to accomodate direct replacement - even in Unicode-using cases, of all current usage of the
 * Windows user space Registry API set. If - by this time - all pointless Unicode usage was washed
 * out of the CLARiiON code base, this would not be neccessary. However Unicode is still the common
 * case - and in the case of much user space code will always be the case (in the Windows use
 * deployment) - so this Registry abstraction is fully Unicode-capable.
 *
 * This Registry abstraction is type-aware (like the Windows user space Registry API set), supports
 * strings, multistrings, 32 bit numbers, 64 bit numbers, and binary blobs.
 *
 * This Registry abstraction is fully symmetric and consistent - for example the same convenience
 * wrappers are provided for all types and Unicode/normal variants - however silly - with insofar
 * as possible the same operational semantics.
 *
 * This Registry abstraction returns a small set of abstracted testable status codes. Users who
 * convert to this Registry abstraction should test for these codes explicitly. However these
 * codes are actually - insofar as possible - numerically a valid match for the use case being used,
 * so failure to test for the abstracted status code may still work.
 *
 * This Registry abstraction - in the use cases where it maps through to the real Windows Registry,
 * does not have any dependency on CSX. This is so that no additional requirement is placed upon
 * code that does not today have the CSX environment available and initialized.
 *
 * This Registry abstraction can - and should - replace the one that exists in the EMCPAL. The EMCPAL
 * abstraction is limited in what it supppirts - for example it has no path through to the real Windows
 * Registry in the Windows user space use case. To this end this Registry abstraction has a full set
 * of "PLA" (PAL look-alike) APIs. I suggest converting to the PLA API set and retiring the EMCPAL
 * abstraction.
 *
 * This Registry abstraction has a set of abstract types for all arguments to the APIs. You do not
 * need to use these types in your code - they exist to help with the Unicode/normal support scheme,
 * etc... (except * for cases where the type provided is truly unique - like EMCUTIL_REG_VALUE_TYPE).
 * Use whatever types are natural for the code you are converting - if they conflict with the types
 * associated with the API - the compiler will tell you and then you can find out what you are doing wrong.
 *
 * This Registry abstraction has selftest APIs that can be called to at exercise and
 * validate basic correctness of all APIs. For the PLA APIs, the test code used in the EMCPAL
 * has been stolen and reworked to test the PLA APIs. The intent is for these selftest APIs
 * to be hooked into frameworks like the PAL Test Framework. The SelfTest code has been exercised
 * with all backends (Windows kernel space, Windows user space, CSX)
 *
 * This Registry abstraction is really CSX code with non-CSX-like names. It has been developed
 * in the CSX environment. It is only deployed as a static library in the CLARiiON tree in order
 * to make it available to code that does not today have the CSX environment available and initialized.
 *
 * This Registry abstraction supports an 'access flags' concept like the Windows user space Registry API
 * set. This is of questionable value in a use case like ours, and is ignored when CSX is providing the
 * Registry.
 *
 * This Registry scheme is compatible with the Windows Registry keys used in CLARiiON today.
 * All of the APIs that take a base Registry will accept either the Windows kernel space style
 * keys (\\Registry\\...) or Windows user space style keys. The API supports an equivalent to
 * HKEY_LOCAL_MACHINE as well to accomodate common Windows user space Registry API usage.
 *
 * Here is the basic model:
 *
 *  - EmcutilRegOpenKey[A|W]() - analog to RegOpenKeyEx[A|W]() - returns key handle
 *  - EmcutilRegCreateKey[A|W]() - analog to RegCreateKeyEx[A|W]() - returns key handle
 *  - EmcutilRegFlushKey() - analog to RegFlushKey()
 *  - EmcutilRegCloseKey() - analog to RegCloseKey()
 *  - EmcutilRegQueryPath[A|W]() - way to get access to to the full text path of a key from a key handle
 *  - EmcutilRegDeleteKey[A|W]() - analog to RegDeleteKey()
 *  - EmcutilRegDeleteValue[A|W]() - analog to RegDeleteValue()
 *  - EmcutilRegSetValue[A|W]() - analog to RegSetValueEx()
 *  - EmcutilRegQueryValue[A|W]() - analog to RegQueryValueEx()
 *  - EmcutilRegQuerySubkeys() - analog to RegQueryInfoKey() [partial - info about keys below]
 *  - EmcutilRegQueryValues[A|W]() - analog to RegQueryInfoKey[A|W]() [partial - info about values below]
 *  - EmcutilRegEnumSubkeys[A|W]() - analog to RegEnumKeyEx[A|W]()
 *  - EmcutilRegEnumValues[A|W]() - analog to RegEnumValue[A|W]()
 *
 * Here are a set of simplified convenience wrappers for EmcutilRegSetValue[A|W]() where you
 * know the type of what you are setting:
 *
 *  - EmcutilRegSetValueString[A|W]()
 *  - EmcutilRegSetValueMultiString[A|W]()
 *  - EmcutilRegSetValue32[A|W]()
 *  - EmcutilRegSetValue64[A|W]()
 *  - EmcutilRegSetValueBinary[A|W]()
 *
 * Here are a set of simplified convenience wrappers for EmcutilRegQueryValue[A|W]() where you
 * know the type of what you are querying, and want the API to allocate memory for what is
 * being returned to you:
 *
 *  - EmcutilRegQueryValueString[A|W]()
 *  - EmcutilRegQueryValueMultiString[A|W]()
 *  - EmcutilRegQueryValue32[A|W]()
 *  - EmcutilRegQueryValue64[A|W]()
 *  - EmcutilRegQueryValueBinary[A|W]()
 *  - EmcutilRegQueryReturnAllocatedValue() - your means to return a returned String/MultiString/Binary
 *                                            allocated by the API
 *
 * Here are a set of simplified convenience wrappers for EmcutilRegSetValue[A|W]() where you
 * know the type of what you are setting, and want to directly use an absolute Registry key
 * (rather than open a key yourself and close it):
 *
 *  - EmcutilRegSetValueStringDirect[A|W]()
 *  - EmcutilRegSetValueMultiStringDirect[A|W]()
 *  - EmcutilRegSetValue32Direct[A|W]()
 *  - EmcutilRegSetValue64Direct[A|W]()
 *  - EmcutilRegSetValueBinaryDirect[A|W]()
 *
 * Here are a set of simplified convenience wrappers for EmcutilRegQueryValue[A|W]() where you
 * know the type of what you are querying, and want the API to allocate memory for what is
 * being returned to you, and want to directly use an absolute Registry key
 * (rather than open a key yourself and close it):
 *
 *  - EmcutilRegQueryValueStringDirect[A|W]()
 *  - EmcutilRegQueryValueMultiStringDirect[A|W]()
 *  - EmcutilRegQueryValue32Direct[A|W]()
 *  - EmcutilRegQueryValue64Direct[A|W]()
 *  - EmcutilRegQueryValueBinaryDirect[A|W]()
 *  - EmcutilRegQueryReturnAllocatedValue() - your means to return a returned String/MultiString/Binary
 *                                            allocated by the API
 *
 * This looks like a complex API. It is not very complex, just double the normal size to
 * accomodate the normal and Unicode use cases, then expanded by the four sets of convenience
 * wrappers with simplified usage models, each of which also has normal/Unicode variants.
 */

////////////////////////////////////


typedef csx_cstring_t EMCUTIL_REG_SUBKEY_NAME_A;	/*!< Subkey name - constant Ascii string */
typedef csx_string_t EMCUTIL_REG_SUBKEY_VNAME_A;	/*!< Subkey name - variable Ascii string */
typedef csx_cwstring_t EMCUTIL_REG_SUBKEY_NAME_W;	/*!< Subkey name - constant Unicode string */
typedef csx_wstring_t EMCUTIL_REG_SUBKEY_VNAME_W;	/*!< Subkey name - variable Unicode string */

typedef csx_cstring_t EMCUTIL_REG_VALUE_NAME_A;		/*!< Value name - constant Ascii string */
typedef csx_string_t EMCUTIL_REG_VALUE_VNAME_A;		/*!< Value name - variable Ascii string */
typedef csx_cwstring_t EMCUTIL_REG_VALUE_NAME_W;	/*!< Value name - constant Unicode string */
typedef csx_wstring_t EMCUTIL_REG_VALUE_VNAME_W;	/*!< Value name - variable Unicode string */

typedef csx_cstring_t EMCUTIL_REG_STRING_A;			/*!< Constant Ascii string */
typedef csx_string_t EMCUTIL_REG_VSTRING_A;			/*!< Variable Ascii string */
typedef csx_cwstring_t EMCUTIL_REG_STRING_W;		/*!< Constant Unicode string */
typedef csx_wstring_t EMCUTIL_REG_VSTRING_W;		/*!< Variable Unicode string */

typedef csx_char_t EMCUTIL_REG_CHAR_A;				/*!< Ascii character */
typedef csx_wchar_t EMCUTIL_REG_CHAR_W;				/*!< Unicode character */

#ifdef EMCUTIL_UNICODE_IN_USE
typedef EMCUTIL_REG_SUBKEY_NAME_W EMCUTIL_REG_SUBKEY_NAME;		/*!< Constant subkey name (in Unicode) */
typedef EMCUTIL_REG_SUBKEY_VNAME_W EMCUTIL_REG_SUBKEY_VNAME;	/*!< Variable subkey name (in Unicode) */
typedef EMCUTIL_REG_VALUE_NAME_W EMCUTIL_REG_VALUE_NAME;		/*!< Constant value name (in Unicode) */
typedef EMCUTIL_REG_VALUE_VNAME_W EMCUTIL_REG_VALUE_VNAME;		/*!< Variable value name (in Unicode) */
typedef EMCUTIL_REG_STRING_W EMCUTIL_REG_STRING;				/*!< Constant string (in Unicode) */
typedef EMCUTIL_REG_VSTRING_W EMCUTIL_REG_VSTRING;				/*!< Variable string (in Unicode) */
typedef EMCUTIL_REG_CHAR_W EMCUTIL_REG_CHAR;					/*!< Character (in Unicode) */
#else
typedef EMCUTIL_REG_SUBKEY_NAME_A EMCUTIL_REG_SUBKEY_NAME;		/*!< Constant subkey name (in Ascii) */
typedef EMCUTIL_REG_SUBKEY_VNAME_A EMCUTIL_REG_SUBKEY_VNAME;	/*!< Variable subkey name (in Ascii) */
typedef EMCUTIL_REG_VALUE_NAME_A EMCUTIL_REG_VALUE_NAME;		/*!< Constant value name (in Ascii) */
typedef EMCUTIL_REG_VALUE_VNAME_A EMCUTIL_REG_VALUE_VNAME;		/*!< Variable value name (in Ascii) */
typedef EMCUTIL_REG_STRING_A EMCUTIL_REG_STRING;				/*!< Constant string (in Ascii) */
typedef EMCUTIL_REG_VSTRING_A EMCUTIL_REG_VSTRING;				/*!< Variable string (in Ascii) */
typedef EMCUTIL_REG_CHAR_A EMCUTIL_REG_CHAR;					/*!< Character (in Ascii) */
#endif

/*! @brief Always in characters, not bytes, always includes space for terminating NUL */
typedef EMCUTIL_SIZE_T EMCUTIL_REG_VALUE_NAME_SIZE;
/*! @brief Always in characters, not bytes, always includes space for terminating NUL */
typedef EMCUTIL_SIZE_T EMCUTIL_REG_SUBKEY_NAME_SIZE;

typedef csx_u32_t EMCUTIL_REG_ACCESS_FLAGS;							/*!< Registry access flags */
#if defined(EMCUTIL_CONFIG_BACKEND_WU) || defined(EMCUTIL_CONFIG_BACKEND_WK)
#define EMCUTIL_REG_KEY_QUERY_VALUE         KEY_QUERY_VALUE			/*!< Query value flag (for Windows) */
#define EMCUTIL_REG_KEY_SET_VALUE           KEY_SET_VALUE			/*!< Set value flag (for Windows) */
#define EMCUTIL_REG_KEY_CREATE_SUB_KEY      KEY_CREATE_SUB_KEY		/*!< Create subkey flag (for Windows) */
#define EMCUTIL_REG_KEY_ENUMERATE_SUB_KEYS  KEY_ENUMERATE_SUB_KEYS	/*!< Enumerate subkeys flag (for Windows) */
#define EMCUTIL_REG_KEY_READ                KEY_READ				/*!< Read access flag (for Windows) */
#define EMCUTIL_REG_KEY_WRITE               KEY_WRITE				/*!< Write access flag (for Windows) */
#define EMCUTIL_REG_KEY_EXECUTE             KEY_EXECUTE				/*!< Execute access flag (for Windows) */
#define EMCUTIL_REG_KEY_ALL_ACCESS          KEY_ALL_ACCESS			/*!< All access flags (for Windows) */
#elif defined(EMCUTIL_CONFIG_BACKEND_CSX)
#define EMCUTIL_REG_KEY_QUERY_VALUE         (0x0001)				/*!< Query value flag (for CSX) */
#define EMCUTIL_REG_KEY_SET_VALUE           (0x0002)				/*!< Set value flag (for CSX) */
#define EMCUTIL_REG_KEY_CREATE_SUB_KEY      (0x0004)				/*!< Create subkey flag (for CSX) */
#define EMCUTIL_REG_KEY_ENUMERATE_SUB_KEYS  (0x0008)				/*!< Enumerate subkeys flag (for CSX) */
#define EMCUTIL_REG_KEY_READ                (0x0010)				/*!< Read access flag (for CSX) */
#define EMCUTIL_REG_KEY_WRITE               (0x0020)				/*!< Write access flag (for CSX) */
#define EMCUTIL_REG_KEY_EXECUTE             (0x0040)				/*!< Execute access flag (for CSX) */
#define EMCUTIL_REG_KEY_ALL_ACCESS          (0x00FF)				/*!< All access flag s(for CSX) */
#else
#error "Error: bad EMCUTIL_CONFIG configuration..."
#endif

typedef csx_u32_t EMCUTIL_REG_CREATE_FLAGS;				/*!< Registry create flags */
#define EMCUTIL_REG_CREATE_FLAG_NONE            0		/*!< Registry create no flag */
#define EMCUTIL_REG_CREATE_FLAG_VOLATILE        1		/*!< Registry create volatile flag */
#define EMCUTIL_REG_CREATE_FLAG_FAIL_IF_MISSING 2		/*!< Registry create flag fail-if-missing */

typedef csx_u32_t EMCUTIL_REG_CREATE_DISPOSITION;		/*!< Registry create disposition */
#define EMCUTIL_REG_CREATE_DISPOSITION_CREATED  1		/*!< Registry create disposition created */
#define EMCUTIL_REG_CREATE_DISPOSITION_EXISTED  2		/*!< Registry create disposition existed */

/*! @brief Registry value type
 */
typedef enum {
#ifdef EMCUTIL_CONFIG_BACKEND_CSX
    EMCUTIL_REG_TYPE_DWORD = CSX_P_HENV_VALUE_TYPE_4BYTE,
    EMCUTIL_REG_TYPE_QWORD = CSX_P_HENV_VALUE_TYPE_8BYTE,
    EMCUTIL_REG_TYPE_STRING = CSX_P_HENV_VALUE_TYPE_STRING,
    EMCUTIL_REG_TYPE_MSTRING = CSX_P_HENV_VALUE_TYPE_MSTRING,
    EMCUTIL_REG_TYPE_BINARY = CSX_P_HENV_VALUE_TYPE_BINARY
#else
#if defined(REG_SZ)
    EMCUTIL_REG_TYPE_DWORD = REG_DWORD,
    EMCUTIL_REG_TYPE_QWORD = REG_QWORD,
    EMCUTIL_REG_TYPE_STRING = REG_SZ,
    EMCUTIL_REG_TYPE_MSTRING = REG_MULTI_SZ,
    EMCUTIL_REG_TYPE_BINARY = REG_BINARY
#elif defined(EMCPAL_REGISTRY_VALUE_TYPE_U32)
    EMCUTIL_REG_TYPE_DWORD = EMCPAL_REGISTRY_VALUE_TYPE_U32,
    EMCUTIL_REG_TYPE_QWORD = EMCPAL_REGISTRY_VALUE_TYPE_U64,
    EMCUTIL_REG_TYPE_STRING = EMCPAL_REGISTRY_VALUE_TYPE_STRING,
    EMCUTIL_REG_TYPE_MSTRING = EMCPAL_REGISTRY_VALUE_TYPE_MULTISTRING,
    EMCUTIL_REG_TYPE_BINARY = EMCPAL_REGISTRY_VALUE_TYPE_BINARY
#else
    EMCUTIL_REG_TYPE_STRING = 1,
    EMCUTIL_REG_TYPE_BINARY = 3,
    EMCUTIL_REG_TYPE_DWORD = 4,
    EMCUTIL_REG_TYPE_MSTRING = 7,
    EMCUTIL_REG_TYPE_QWORD = 11,
#endif
#endif
} EMCUTIL_REG_VALUE_TYPE;

#define EMCUTIL_REG_TYPE_U32 EMCUTIL_REG_TYPE_DWORD		/*!< Registry unigned 32-bit type */
#define EMCUTIL_REG_TYPE_U64 EMCUTIL_REG_TYPE_QWORD		/*!< Registry unigned 64-bit type */

typedef csx_cpvoid_t EMCUTIL_REG_VALUE_PTR;				/*!< Registry value (constant) pointer */
typedef csx_pvoid_t EMCUTIL_REG_VALUE_VPTR;				/*!< Registry value (variable) pointer */
/*! @brief Always in bytes */
typedef EMCUTIL_SIZE_T EMCUTIL_REG_VALUE_SIZE;

typedef csx_nuint_t EMCUTIL_REG_VALUE_COUNT;			/*!< Values count */
typedef csx_nuint_t EMCUTIL_REG_VALUE_INDEX;			/*!< Values index */
typedef csx_nuint_t EMCUTIL_REG_SUBKEY_INDEX;			/*!< Subkey count */
typedef csx_nuint_t EMCUTIL_REG_SUBKEY_COUNT;			/*!< Subkey count */

typedef csx_pvoid_t EMCUTIL_REG_HANDLE;					/*!< Registry handle */

////////////////////////////////////

#ifdef HKEY_CLASSES_ROOT
#define EMCUTIL_REG_CLASSES_ROOT ((EMCUTIL_REG_HANDLE)(csx_ptrhld_t)HKEY_CLASSES_ROOT)
#else
#define EMCUTIL_REG_CLASSES_ROOT ((EMCUTIL_REG_HANDLE)(csx_ptrhld_t)0x80000000)		/*!< Registry classes root */
#endif
#ifdef HKEY_LOCAL_MACHINE
#define EMCUTIL_REG_LOCAL_MACHINE ((EMCUTIL_REG_HANDLE)(csx_ptrhld_t)HKEY_LOCAL_MACHINE)	/*!< Registry local machine */
#else
#define EMCUTIL_REG_LOCAL_MACHINE ((EMCUTIL_REG_HANDLE)(csx_ptrhld_t)0x80000002)	/*!< Registry local machine */
#endif

////////////////////////////////////

/*!
 * @brief Open registry key (Ascii stings)
 * @param existingKeyHandle Can be NULL or EMCUTIL_REG_LOCAL_MACHINE/HKEY_CLASSES_ROOT or existing KeyHandle
 * @param subKeyName Can be subkey, or absolute/full key in either kernel or user style
 * @param accessFlags Access flags, ignored off of Windows
 * @param newKeyHandleRv [OUT] Pointer to open key handle
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegOpenKeyA(
    EMCUTIL_REG_HANDLE existingKeyHandle,
    EMCUTIL_REG_SUBKEY_NAME_A subKeyName,
    EMCUTIL_REG_ACCESS_FLAGS accessFlags,
    EMCUTIL_REG_HANDLE * newKeyHandleRv); 

/*!
 * @brief Open registry key (Unicode)
 * @param existingKeyHandle Can be NULL or EMCUTIL_REG_LOCAL_MACHINE/HKEY_CLASSES_ROOT or existing KeyHandle
 * @param subKeyName Can be subkey, or absolute/full key in either kernel or user style (in Unicode)
 * @param accessFlags Access flags, ignored off of Windows
 * @param newKeyHandleRv [OUT] Pointer to open key handle
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegOpenKeyW(
    EMCUTIL_REG_HANDLE existingKeyHandle,
    EMCUTIL_REG_SUBKEY_NAME_W subKeyName,
    EMCUTIL_REG_ACCESS_FLAGS accessFlags,
    EMCUTIL_REG_HANDLE * newKeyHandleRv);

/*!
 * @brief Open the registry key. Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegOpenKey EmcutilRegOpenKeyA
#else
#define EmcutilRegOpenKey EmcutilRegOpenKeyW
#endif

//////////

/*!
 * @brief Create registry key (Ascii strings)
 * @param existingKeyHandle Can be NULL or EMCUTIL_REG_LOCAL_MACHINE/HKEY_CLASSES_ROOT or existing KeyHandle
 * @param subKeyName Can be subkey, or absolute/full key in either kernel or user style
 * @param createFlags Tells API whether VOLATILE or NORMAL - VOLATILE not supported off of Windows
 * @param accessFlags Ignored off of Windows
 * @param newKeyHandleRv [OUT] New key handle
 * @param dispositionRv [OUT] Tells you whether CREATED or EXISTED
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegCreateKeyA(
    EMCUTIL_REG_HANDLE existingKeyHandle,
    EMCUTIL_REG_SUBKEY_NAME_A subKeyName,
    EMCUTIL_REG_CREATE_FLAGS createFlags,
    EMCUTIL_REG_ACCESS_FLAGS accessFlags,
    EMCUTIL_REG_HANDLE * newKeyHandleRv,
    EMCUTIL_REG_CREATE_DISPOSITION * dispositionRv);

/*!
 * @brief Create registry key (Unicode)
 * @param existingKeyHandle Can be NULL or EMCUTIL_REG_LOCAL_MACHINE/HKEY_CLASSES_ROOT or existing KeyHandle
 * @param subKeyName Can be subkey, or absolute/full key in either kernel or user style (in Unicode)
 * @param createFlags Tells API whether VOLATILE or NORMAL - VOLATILE not supported off of Windows
 * @param accessFlags Ignored off of Windows
 * @param newKeyHandleRv [OUT] New key handle
 * @param dispositionRv [OUT] Tells you whether CREATED or EXISTED
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegCreateKeyW(
    EMCUTIL_REG_HANDLE existingKeyHandle,
    EMCUTIL_REG_SUBKEY_NAME_W subKeyName,
    EMCUTIL_REG_CREATE_FLAGS createFlags,
    EMCUTIL_REG_ACCESS_FLAGS accessFlags,
    EMCUTIL_REG_HANDLE * newKeyHandleRv,
    EMCUTIL_REG_CREATE_DISPOSITION * dispositionRv);

/*!
 * @brief Create registry key. Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegCreateKey EmcutilRegCreateKeyA
#else
#define EmcutilRegCreateKey EmcutilRegCreateKeyW
#endif

//////////

/*!
 * @brief Flushes open key data to disk - not supported off of Windows
 * @param openKeyHandle Open key handle
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegFlushKey(
    EMCUTIL_REG_HANDLE openKeyHandle);

/*!
 * @brief Closes open key
 * @param openKeyHandle Open key handle
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegCloseKey(
    EMCUTIL_REG_HANDLE openKeyHandle);

//////////

/*!
 * @brief Returns pointer to absolute/full key 'path'. Valid as long as key handle is open
 * @param openKeyHandle Open key handle
 * @param keyPathRv [OUT] Pointer to absolute/full path of the key
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryPathA(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_SUBKEY_NAME_A * keyPathRv);

/*!
 * @brief Returns pointer to absolute/full key 'path' in Unicode. Valid as long as key handle is open
 * @param openKeyHandle Open key handle
 * @param keyPathRv [OUT] Pointer to absolute/full path of the key in Unicode
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryPathW(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_SUBKEY_NAME_W * keyPathRv);

/*!
 * @brief Returns pointer to absolute/full key path. Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegQueryPath EmcutilRegQueryPath
#else
#define EmcutilRegQueryPath EmcutilRegQueryPath
#endif

//////////

/*!
 * @brief Tells you if two full absolute/full key 'paths' are the same - keys can have multiple styles
 * @param Path1 Absolute/full key 'path' 1
 * @param Path2 Absolute/full key 'path' 2
 * @param SameRv [OUT] Pointer to the result (bool). True if match, false if mismatch
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegComparePaths(
    EMCUTIL_REG_SUBKEY_NAME_A Path1,
    EMCUTIL_REG_SUBKEY_NAME_A Path2,
    csx_bool_t * SameRv);

//////////

/*!
 * @brief Delete registry key (Ascii strings)
 * @param openKeyHandle Open key handle
 * @param subKeyName Subkey name
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegDeleteKeyA(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_SUBKEY_NAME_A subKeyName);

/*!
 * @brief Delete registry key (Unicode)
 * @param openKeyHandle Open key handle
 * @param subKeyName Subkey name in Unicode
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegDeleteKeyW(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_SUBKEY_NAME_W subKeyName);

/*!
 * @brief Delete registry key. Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegDeleteKey EmcutilRegDeleteKeyA
#else
#define EmcutilRegDeleteKey EmcutilRegDeleteKeyW
#endif

//////////

/*!
 * @brief Delete registry key value (Ascii strings)
 * @param openKeyHandle Open key handle
 * @param valueName Name of value
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegDeleteValueA(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_A valueName);

/*!
 * @brief Delete registry key value (Unicode)
 * @param openKeyHandle Open key handle
 * @param valueName Name of value in Unicode
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegDeleteValueW(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_W valueName);

/*!
 * @brief Delete registry key value. Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegDeleteValue EmcutilRegDeleteValueA
#else
#define EmcutilRegDeleteValue EmcutilRegDeleteValueW
#endif

//////////

/*!
 * @brief Set registry key value (Ascii strings)
 * @param openKeyHandle Open key handle
 * @param valueName Name of value
 * @param valueType Value type (int, dword, sting, etc.)
 * @param valuePtr Pointer to the value
 * @param valueSizeBytes Value size in bytes, for string types must include terminating NUL
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegSetValueA(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_A valueName,
    EMCUTIL_REG_VALUE_TYPE valueType,
    EMCUTIL_REG_VALUE_PTR valuePtr,
    EMCUTIL_REG_VALUE_SIZE valueSizeBytes);

/*!
 * @brief Set registry key value (Unicode)
 * @param openKeyHandle Open key handle
 * @param valueName Name of value in Unicode
 * @param valueType Value type (int, dword, sting, etc.)
 * @param valuePtr Pointer to the value
 * @param valueSizeBytes Value size in bytes, for string types must include terminating NUL
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegSetValueW(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_W valueName,
    EMCUTIL_REG_VALUE_TYPE valueType,
    EMCUTIL_REG_VALUE_PTR valuePtr,
    EMCUTIL_REG_VALUE_SIZE valueSizeBytes);

/*!
 * @brief Set registry key value. Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegSetValue EmcutilRegSetValueA
#else
#define EmcutilRegSetValue EmcutilRegSetValueW
#endif

//////////

/*!
 * @brief Query for registry key value
 * @param openKeyHandle Open key handle
 * @param valueName Name of value
 * @param valueTypeRv [OUT] Pointer to place where the value type is returned
 * @param valueBuffer Pointer to buffer for value data
 * @param valueSizeBytesAvailable Value data buffer size in bytes, 
 *        for string types must include space for terminating NUL
 * @param valueSizeBytesRequiredRv [OUT] Required value data buffer size in bytes
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)\n
 *        EMCUTIL_STATUS_MORE_DATA if value data buffer is too small
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryValueA(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_A valueName,
    EMCUTIL_REG_VALUE_TYPE * valueTypeRv,
    EMCUTIL_REG_VALUE_VPTR valueBuffer,
    EMCUTIL_REG_VALUE_SIZE valueSizeBytesAvailable,
    EMCUTIL_REG_VALUE_SIZE * valueSizeBytesRequiredRv);

/*!
 * @brief Query for registry key value (Unicode)
 * @param openKeyHandle Open key handle
 * @param valueName Name of value in Unicode
 * @param valueTypeRv [OUT] Pointer to place where the value type is returned
 * @param valueBuffer Pointer to buffer for value data
 * @param valueSizeBytesAvailable Value data buffer size in bytes, 
 *        for string types must include space for terminating NUL
 * @param valueSizeBytesRequiredRv [OUT] Required value data buffer size in bytes
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)\n
 *        EMCUTIL_STATUS_MORE_DATA if value data buffer is too small
*/
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryValueW(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_W valueName,
    EMCUTIL_REG_VALUE_TYPE * valueTypeRv,
    EMCUTIL_REG_VALUE_VPTR valueBuffer,
    EMCUTIL_REG_VALUE_SIZE valueSizeBytesAvailable,
    EMCUTIL_REG_VALUE_SIZE * valueSizeBytesRequiredRv);

/*!
 * @brief Query for registry key value. Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegQueryValue EmcutilRegQueryValueA
#else
#define EmcutilRegQueryValue EmcutilRegQueryValueW
#endif

//////////

/*!
 * @brief Returns number of subkeys under open key
 * @param openKeyHandle Open key handle
 * @param numSubkeysRv [OUT] Number of subkeys 
 * @param longestSubkeyNameSizeRv [OUT] Size of longest subkey name, always includes space for terminating NUL
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQuerySubkeys(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_SUBKEY_COUNT * numSubkeysRv,
    EMCUTIL_REG_SUBKEY_NAME_SIZE * longestSubkeyNameSizeRv);

/*!
 * @brief Returns number of values under open key (Ascii strings)
 * @param openKeyHandle Open key handle
 * @param numValuesRv [OUT] Number of values below open key
 * @param longestValueNameSizeRv [OUT] Size of longest value name. In chars, does include space for terminating NUL
 * @param largestValueSizeBytesRv [OUT] Size of longest value. In bytes, for string types always includes space 
 *        for terminating NUL
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryValuesA(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_COUNT * numValuesRv,
    EMCUTIL_REG_VALUE_NAME_SIZE * longestValueNameSizeRv,   
    EMCUTIL_REG_VALUE_SIZE * largestValueSizeBytesRv);

/*!
 * @brief Returns number of values under open key (Unicode)
 * @param openKeyHandle Open key handle
 * @param numValuesRv [OUT] Number of values below open key
 * @param longestValueNameSizeRv [OUT] Size of longest value name. In chars, does include space for terminating NUL
 * @param largestValueSizeBytesRv [OUT] Size of longest value. In bytes, for string types always includes space for 
 *        terminating NUL
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryValuesW(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_COUNT * numValuesRv,
    EMCUTIL_REG_VALUE_NAME_SIZE * longestValueNameSizeRv,
    EMCUTIL_REG_VALUE_SIZE * largestValueSizeBytesRv);

/*!
 * @brief Returns number of values under open key.  Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegQueryValues EmcutilRegQueryValuesA
#else
#define EmcutilRegQueryValues EmcutilRegQueryValuesW
#endif

//////////

/*!
 * @brief Retrieve indexed subkey name under open key (Ascii strings)
 * @param openKeyHandle Open key handle
 * @param subkeyIndex Subkey index
 * @param subKeyNameBuffer Subkey name buffer
 * @param subKeyNameSizeAvailable Size of subkey name buffer in chars, must include space for terminating NUL
 * @param subKeyNameSizeRequiredRv [OUT] Required size of subkey name buffer in chars
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)\n
 *        EMCUTIL_STATUS_MORE_DATA if subkey name buffer is too small\n
 *        EMCUTIL_STATUS_NO_MORE_ENTRIES if indexed value does not exist
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegEnumSubkeysA(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_SUBKEY_INDEX subkeyIndex,
    EMCUTIL_REG_SUBKEY_VNAME_A subKeyNameBuffer,
    EMCUTIL_REG_SUBKEY_NAME_SIZE subKeyNameSizeAvailable,
    EMCUTIL_REG_SUBKEY_NAME_SIZE * subKeyNameSizeRequiredRv);

/*!
 * @brief Retrieve indexed subkey name under open key (Unicode)
 * @param openKeyHandle Open key handle
 * @param subkeyIndex Subkey index
 * @param subKeyNameBuffer Subkey name buffer
 * @param subKeyNameSizeAvailable Size of subkey name buffer in chars, must include space for terminating NUL
 * @param subKeyNameSizeRequiredRv [OUT] Required size of subkey name buffer in chars
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise).\n
 *        EMCUTIL_STATUS_MORE_DATA if subkey name buffer is too small\n
 *        EMCUTIL_STATUS_NO_MORE_ENTRIES if indexed value does not exist
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegEnumSubkeysW(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_SUBKEY_INDEX subkeyIndex,
    EMCUTIL_REG_SUBKEY_VNAME_W subKeyNameBuffer,
    EMCUTIL_REG_SUBKEY_NAME_SIZE subKeyNameSizeAvailable,
    EMCUTIL_REG_SUBKEY_NAME_SIZE * subKeyNameSizeRequiredRv);

/*!
 * @brief Retrieve indexed subkey name under open key. Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegEnumSubkeys EmcutilRegEnumSubkeysA
#else
#define EmcutilRegEnumSubkeys EmcutilRegEnumSubkeysW
#endif

//////////

/*!
 * @brief Retrieve indexed value under open key (Ascii strings)
 * @param openKeyHandle Open key handle
 * @param valueIndex Value index
 * @param valueNameBuffer Buffer for value name
 * @param valueNameSizeAvailable Size of buffer for value name in chars, must include space for terminating NUL
 * @param valueNameSizeRequiredRv [OUT] Required size of buffer for value name.
 * @param valueTypeRv [OUT] Returned value type
 * @param valueBuffer Value buffer
 * @param valueSizeBytesAvailable Size of value buffer in bytes, for string types must include space for terminating NUL
 * @param valueSizeBytesRequiredRv [OUT] Required size of value buffer in bytes
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)\n
 *        EMCUTIL_STATUS_MORE_DATA if any of available buffers is too small\n
 *        EMCUTIL_STATUS_NO_MORE_ENTRIES if indexed value does not exist
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegEnumValuesA(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_INDEX valueIndex,
    EMCUTIL_REG_VALUE_VNAME_A valueNameBuffer,
    EMCUTIL_REG_VALUE_NAME_SIZE valueNameSizeAvailable,
    EMCUTIL_REG_VALUE_NAME_SIZE * valueNameSizeRequiredRv,
    EMCUTIL_REG_VALUE_TYPE * valueTypeRv,
    EMCUTIL_REG_VALUE_VPTR valueBuffer,
    EMCUTIL_REG_VALUE_SIZE valueSizeBytesAvailable,
    EMCUTIL_REG_VALUE_SIZE * valueSizeBytesRequiredRv);

/*!
 * @brief Retrieve indexed value under open key (Unicode)
 * @param openKeyHandle Open key handle
 * @param valueIndex Value index
 * @param valueNameBuffer Buffer for value name (name returned in Unicode)
 * @param valueNameSizeAvailable Size of buffer for value name in chars, must include space for terminating NUL
 * @param valueNameSizeRequiredRv [OUT] Required size of buffer for value name.
 * @param valueTypeRv [OUT] Returned value type
 * @param valueBuffer Value buffer
 * @param valueSizeBytesAvailable Size of value buffer in bytes, for string types must include space for terminating NUL
 * @param valueSizeBytesRequiredRv [OUT] Required size of value buffer in bytes
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)\n
 *        EMCUTIL_STATUS_MORE_DATA if any of available buffers is too small\n
 *        EMCUTIL_STATUS_NO_MORE_ENTRIES if indexed value does not exist
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegEnumValuesW(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_INDEX valueIndex,
    EMCUTIL_REG_VALUE_VNAME_W valueNameBuffer,
    EMCUTIL_REG_VALUE_NAME_SIZE valueNameSizeAvailable,
    EMCUTIL_REG_VALUE_NAME_SIZE * valueNameSizeRequiredRv,
    EMCUTIL_REG_VALUE_TYPE * valueTypeRv,
    EMCUTIL_REG_VALUE_VPTR valueBuffer,
    EMCUTIL_REG_VALUE_SIZE valueSizeBytesAvailable,
    EMCUTIL_REG_VALUE_SIZE * valueSizeBytesRequiredRv);

/*!
 * @brief Retrieve indexed value under open key. Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegEnumValues EmcutilRegEnumValuesA
#else
#define EmcutilRegEnumValues EmcutilRegEnumValuesW
#endif

//////////

/*!
 * @brief Convenience wrapper for EmcutilRegSetValue for Ascii strings
 * @param openKeyHandle Open key handle
 * @param valueName Value name
 * @param value String value (Ascii)
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegSetValueStringA(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_A valueName,
    csx_cstring_t value);

/*!
 * @brief Convenience wrapper for EmcutilRegSetValue for Unicode strings
 * @param openKeyHandle Open key handle
 * @param valueName Value name (Unicode)
 * @param value String value (Unicode)
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegSetValueStringW(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_W valueName,
    csx_cwstring_t value);

/*!
 * @brief Convenience wrapper for EmcutilRegSetValue for Ascii multistrings
 * @param openKeyHandle Open key handle
 * @param valueName Value name
 * @param value Multistring value (Ascii)
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegSetValueMultiStringA(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_A valueName,
    csx_cstring_t value);

/*!
 * @brief Convenience wrapper for EmcutilRegSetValue for Unicode multistrings
 * @param openKeyHandle Open key handle
 * @param valueName Value name (Unicode)
 * @param value Multistring value (Unicode)
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegSetValueMultiStringW(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_W valueName,
    csx_cwstring_t value); 

/*!
 * @brief Convenience wrapper for EmcutilRegSetValue for 32-bit numbers
 * @param openKeyHandle Open key handle
 * @param valueName Value name
 * @param value 32-bit value to be set
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegSetValue32A(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_A valueName,
    csx_u32_t value);

/*!
 * @brief Convenience wrapper for EmcutilRegSetValue for 32-bit numbers (with Unicode value name)
 * @param openKeyHandle Open key handle
 * @param valueName Value name (Unicode)
 * @param value 32-bit value to be set
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegSetValue32W(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_W valueName,
    csx_u32_t value);

/*!
 * @brief Convenience wrapper for EmcutilRegSetValue for 64-bit numbers
 * @param openKeyHandle Open key handle
 * @param valueName Value name
 * @param value 64-bit value to be set
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegSetValue64A(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_A valueName,
    csx_u64_t value);

/*!
 * @brief Convenience wrapper for EmcutilRegSetValue for 64-bit numbers (with Unicode value name)
 * @param openKeyHandle Open key handle
 * @param valueName Value name (Unicode)
 * @param value 64-bit value to be set
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegSetValue64W(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_W valueName,
    csx_u64_t value);

/*!
 * @brief Convenience wrapper for EmcutilRegSetValue for binary blobs
 * @param openKeyHandle Open key handle
 * @param valueName Value name
 * @param valuePtr Pointer to buffer with binary blob
 * @param valueSizeBytes Size of binary blob in bytes
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegSetValueBinaryA(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_A valueName,
    EMCUTIL_REG_VALUE_PTR valuePtr,
    EMCUTIL_REG_VALUE_SIZE valueSizeBytes);

/*!
 * @brief Convenience wrapper for EmcutilRegSetValue for binary blobs (with Unicode value name)
 * @param openKeyHandle Open key handle
 * @param valueName Value name (Unicode)
 * @param valuePtr Pointer to buffer with binary blob
 * @param valueSizeBytes Size of binary blob in bytes
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegSetValueBinaryW(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_W valueName,
    EMCUTIL_REG_VALUE_PTR valuePtr,
    EMCUTIL_REG_VALUE_SIZE valueSizeBytes);

/*!
 * @brief Set a sting value. Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegSetValueString EmcutilRegSetValueStringA
#else
#define EmcutilRegSetValueString EmcutilRegSetValueStringW
#endif

/*!
 * @brief Set a multistring value. Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegSetValueMultiString EmcutilRegSetValueMultiStringA
#else
#define EmcutilRegSetValueMultiString EmcutilRegSetValueMultiStringW
#endif

/*!
 * @brief Set 32-bit value. Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegSetValue32 EmcutilRegSetValue32A
#else
#define EmcutilRegSetValue32 EmcutilRegSetValue32W
#endif

/*!
 * @brief Set 64-bit value. Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegSetValue64 EmcutilRegSetValue64A
#else
#define EmcutilRegSetValue64 EmcutilRegSetValue64W
#endif

/*!
 * @brief Set binary blob value. Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegSetValueBinary EmcutilRegSetValueBinaryA
#else
#define EmcutilRegSetValueBinary EmcutilRegSetValueBinaryW
#endif

//////////

/*!
 * @brief Convenience wrapper for EmcutilRegSetValue for strings, takes absolute/full Registry key
 * @param subKeyName Full key pathname
 * @param valueName Value name
 * @param value String value (Ascii)
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegSetValueStringDirectA(
    EMCUTIL_REG_SUBKEY_NAME_A subKeyName,
    EMCUTIL_REG_VALUE_NAME_A valueName,
    csx_cstring_t value);

/*!
 * @brief Convenience wrapper for EmcutilRegSetValue for strings, takes absolute/full Registry key. 
 *        Using Unicode strings.
 * @param subKeyName Full key pathname (Unicode)
 * @param valueName Value name (Unicode
 * @param value String value (Unicode)
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegSetValueStringDirectW(
    EMCUTIL_REG_SUBKEY_NAME_W subKeyName,
    EMCUTIL_REG_VALUE_NAME_W valueName,
    csx_cwstring_t value);

/*!
 * @brief Convenience wrapper for EmcutilRegSetValue for multistrings, takes absolute/full Registry key
 * @param subKeyName Full key pathname
 * @param valueName Value name
 * @param value Multistring value (Ascii)
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegSetValueMultiStringDirectA(
    EMCUTIL_REG_SUBKEY_NAME_A subKeyName,
    EMCUTIL_REG_VALUE_NAME_A valueName,
    csx_cstring_t value);

/*!
 * @brief Convenience wrapper for EmcutilRegSetValue for multistrings, takes absolute/full Registry key. 
 *        Using Unicode strings.
 * @param subKeyName Full key pathname (Unicode)
 * @param valueName Value name (Unicode
 * @param value Multistring value (Unicode)
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegSetValueMultiStringDirectW(
    EMCUTIL_REG_SUBKEY_NAME_W subKeyName,
    EMCUTIL_REG_VALUE_NAME_W valueName,
    csx_cwstring_t value);

/*!
 * @brief Convenience wrapper for EmcutilRegSetValue for 32-bit numbers, takes absolute/full Registry key
 * @param subKeyName Full key pathname
 * @param valueName Value name
 * @param value 32-bit value to be set
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegSetValue32DirectA(
    EMCUTIL_REG_SUBKEY_NAME_A subKeyName,
    EMCUTIL_REG_VALUE_NAME_A valueName,
    csx_u32_t value);

/*!
 * @brief Convenience wrapper for EmcutilRegSetValue for 32-bit numbers, takes absolute/full Registry key. 
 *        Using Unicode strings.
 * @param subKeyName Full key pathname (Unicode)
 * @param valueName Value name (Unicode)
 * @param value 32-bit value to be set
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegSetValue32DirectW(
    EMCUTIL_REG_SUBKEY_NAME_W subKeyName,
    EMCUTIL_REG_VALUE_NAME_W valueName,
    csx_u32_t value);

/*!
 * @brief Convenience wrapper for EmcutilRegSetValue for 64-bit numbers, takes absolute/full Registry key
 * @param subKeyName Full key pathname
 * @param valueName Value name
 * @param value 64-bit value to be set
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegSetValue64DirectA(
    EMCUTIL_REG_SUBKEY_NAME_A subKeyName,
    EMCUTIL_REG_VALUE_NAME_A valueName,
    csx_u64_t value);

/*!
 * @brief Convenience wrapper for EmcutilRegSetValue for 64-bit numbers, takes absolute/full Registry key. 
 *        Using Unicode strings.
 * @param subKeyName Full key pathname (Unicode)
 * @param valueName Value name (Unicode)
 * @param value 64-bit value to be set
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegSetValue64DirectW(
    EMCUTIL_REG_SUBKEY_NAME_W subKeyName,
    EMCUTIL_REG_VALUE_NAME_W valueName,
    csx_u64_t value); 

/*!
 * @brief Convenience wrapper for EmcutilRegSetValue for binary blobs, takes absolute/full Registry key
 * @param subKeyName Full key pathname
 * @param valueName Value name
 * @param valuePtr Pointer to buffer with binary blob
 * @param valueSizeBytes Size of binary blob in bytes in bytes
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegSetValueBinaryDirectA(
    EMCUTIL_REG_SUBKEY_NAME_A subKeyName,
    EMCUTIL_REG_VALUE_NAME_A valueName,
    EMCUTIL_REG_VALUE_PTR valuePtr,
    EMCUTIL_REG_VALUE_SIZE valueSizeBytes);

/*!
 * @brief Convenience wrapper for EmcutilRegSetValue for binary blobs, takes absolute/full Registry key. 
 *        Using Unicode strings.
 * @param subKeyName Full key pathname (Unicode)
 * @param valueName Value name (Unicode)
 * @param valuePtr Pointer to buffer with binary blob
 * @param valueSizeBytes Size of binary blob in bytes
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegSetValueBinaryDirectW(
    EMCUTIL_REG_SUBKEY_NAME_W subKeyName,
    EMCUTIL_REG_VALUE_NAME_W valueName,
    EMCUTIL_REG_VALUE_PTR valuePtr,
    EMCUTIL_REG_VALUE_SIZE valueSizeBytes);

/*!
 * @brief Set string value, takes absolute/full Registry key. Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegSetValueStringDirect EmcutilRegSetValueStringDirectA
#else
#define EmcutilRegSetValueStringDirect EmcutilRegSetValueStringDirectW
#endif

/*!
 * @brief Set multistring value, takes absolute/full Registry key. Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegSetValueMultiStringDirect EmcutilRegSetValueMultiStringDirectA
#else
#define EmcutilRegSetValueMultiStringDirect EmcutilRegSetValueMultiStringDirectW
#endif

/*!
 * @brief Set 32-bit value, takes absolute/full Registry key. Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegSetValue32Direct EmcutilRegSetValue32DirectA
#else
#define EmcutilRegSetValue32Direct EmcutilRegSetValue32DirectW
#endif

/*!
 * @brief Set 64-bit value, takes absolute/full Registry key. Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegSetValue64Direct EmcutilRegSetValue64DirectA
#else
#define EmcutilRegSetValue64Direct EmcutilRegSetValue64DirectW
#endif

/*!
 * @brief Set binary blob value, takes absolute/full Registry key. Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegSetValueBinaryDirect EmcutilRegSetValueBinaryDirectA
#else
#define EmcutilRegSetValueBinaryDirect EmcutilRegSetValueBinaryDirectW
#endif

//////////

/*!
 * @brief Free the value returned by EmcutilRegQueryValue<String|MultiString|Binary>[Direct]
 * @param valuePtr Pointer to the value to be freed
 */
EMCUTIL_API csx_void_t EMCUTIL_CC EmcutilRegQueryReturnAllocatedValue(
    csx_pvoid_t valuePtr);

//////////

/*!
 * @brief Convenience wrapper for EmcutilRegQueryValue for strings.\n
 *        Call EmcutilRegQueryReturnAllocatedValue() to free returned value.
 * @param openKeyHandle Open key handle
 * @param valueName Value name
 * @param valueDefault Optional default value
 * @param valuePtrRv [OUT] Pointer to returned string
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryValueStringA(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_A valueName,
    csx_cstring_t valueDefault,
    csx_string_t * valuePtrRv);

/*!
 * @brief Convenience wrapper for EmcutilRegQueryValue for strings. Using Unicode strings.\n
 *        Call EmcutilRegQueryReturnAllocatedValue() to free returned value.
 * @param openKeyHandle Open key handle
 * @param valueName Value name (Unicode)
 * @param valueDefault Optional default value (Unicode)
 * @param valuePtrRv [OUT] Pointer to returned string (in Unicode)
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryValueStringW(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_W valueName,
    csx_cwstring_t valueDefault,
    csx_wstring_t * valuePtrRv); 

/*!
 * @brief Convenience wrapper for EmcutilRegQueryValue for multistrings.\n
 *        Call EmcutilRegQueryReturnAllocatedValue() to free returned value.
 * @param openKeyHandle Open key handle
 * @param valueName Value name
 * @param valueDefault Optional default value
 * @param valuePtrRv [OUT] Pointer to returned multistring
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryValueMultiStringA(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_A valueName,
    csx_cstring_t valueDefault,
    csx_string_t * valuePtrRv);

/*!
 * @brief Convenience wrapper for EmcutilRegQueryValue for multistrings. Using Unicode strings.\n
 *        Call EmcutilRegQueryReturnAllocatedValue() to free returned value.
 * @param openKeyHandle Open key handle
 * @param valueName Value name (Unicode)
 * @param valueDefault Optional default value (Unicode)
 * @param valuePtrRv [OUT] Pointer to returned multistring (in Unicode)
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryValueMultiStringW(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_W valueName,
    csx_cwstring_t valueDefault,
    csx_wstring_t * valuePtrRv);

/*!
 * @brief Convenience wrapper for EmcutilRegQueryValue for 32-bit numbers.\n
 *        Call EmcutilRegQueryReturnAllocatedValue()() to free returned value.
 * @param openKeyHandle Open key handle
 * @param valueName Value name
 * @param valueDefault Pointer to optional default value
 * @param valuePtrRv [OUT] Pointer to returned 32-bit value
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryValue32A(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_A valueName,
    csx_u32_t * valueDefault,
    csx_u32_t * valuePtrRv);

/*!
 * @brief Convenience wrapper for EmcutilRegQueryValue for 32-bit numbers. Using Unicode strings.\n
 *        Call EmcutilRegQueryReturnAllocatedValue() to free returned value.
 * @param openKeyHandle Open key handle
 * @param valueName Value name (Unicode)
 * @param valueDefault Pointer to optional default value
 * @param valuePtrRv [OUT] Pointer to returned 32-bit value
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryValue32W(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_W valueName,
    csx_u32_t * valueDefault,
    csx_u32_t * valuePtrRv);

/*!
 * @brief Convenience wrapper for EmcutilRegQueryValue for 64-bit numbers.\n
 *        Call EmcutilRegQueryReturnAllocatedValue() to free returned value.
 * @param openKeyHandle Open key handle
 * @param valueName Value name
 * @param valueDefault Pointer to optional default value
 * @param valuePtrRv [OUT] Pointer to returned 64-bit value
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryValue64A(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_A valueName,
    csx_u64_t * valueDefault,
    csx_u64_t * valuePtrRv);

/*!
 * @brief Convenience wrapper for EmcutilRegQueryValue for 64-bit numbers. Using Unicode strings.\n
 *        Call EmcutilRegQueryReturnAllocatedValue() to free returned value.
 * @param openKeyHandle Open key handle
 * @param valueName Value name (Unicode)
 * @param valueDefault Pointer to optional default value
 * @param valuePtrRv [OUT] Pointer to returned 64-bit value
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryValue64W(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_W valueName,
    csx_u64_t * valueDefault,
    csx_u64_t * valuePtrRv);

/*!
 * @brief Convenience wrapper for EmcutilRegQueryValue for binary blobs.\n
 *        Call EmcutilRegQueryReturnAllocatedValue() to free returned value.
 * @param openKeyHandle Open key handle
 * @param valueName Value name
 * @param valueDefaultPtr Pointer to optional default value
 * @param valueDefaultSize Size of optional default value
 * @param valuePtrRv [OUT] Pointer to returned binary blob
 * @param valueSizeRv [OUT] Pointer to size of returned binary blob
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryValueBinaryA(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_A valueName,
    csx_cpvoid_t valueDefaultPtr,
    EMCUTIL_SIZE_T valueDefaultSize,
    csx_pvoid_t * valuePtrRv,
    EMCUTIL_SIZE_T * valueSizeRv);

/*!
 * @brief Convenience wrapper for EmcutilRegQueryValue for binary blobs. Using Unicode strings.\n
 *        Call EmcutilRegQueryReturnAllocatedValue() to free returned value.
 * @param openKeyHandle Open key handle
 * @param valueName Value name (Unicode).
 * @param valueDefaultPtr Pointer to optional default value
 * @param valueDefaultSize Size of optional default value
 * @param valuePtrRv [OUT] Pointer to returned binary blob
 * @param valueSizeRv [OUT] Pointer to size of returned binary blob
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryValueBinaryW(
    EMCUTIL_REG_HANDLE openKeyHandle,
    EMCUTIL_REG_VALUE_NAME_W valueName,
    csx_cpvoid_t valueDefaultPtr,
    EMCUTIL_SIZE_T valueDefaultSize, 
    csx_pvoid_t * valuePtrRv,
    EMCUTIL_SIZE_T * valueSizeRv);

/*!
 * @brief Query for string value. Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegQueryValueString EmcutilRegQueryValueStringA
#else
#define EmcutilRegQueryValueString EmcutilRegQueryValueStringW
#endif

/*!
 * @brief Query for multistring value. Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegQueryValueMultiString EmcutilRegQueryValueMultiStringA
#else
#define EmcutilRegQueryValueMultiString EmcutilRegQueryValueMultiStringW
#endif

/*!
 * @brief Query for 32-bit value. Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegQueryValue32 EmcutilRegQueryValue32A
#else
#define EmcutilRegQueryValue32 EmcutilRegQueryValue32W
#endif

/*!
 * @brief Query for 64-bit value. Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegQueryValue64 EmcutilRegQueryValue64A
#else
#define EmcutilRegQueryValue64 EmcutilRegQueryValue64W
#endif

/*!
 * @brief Query for binary blob value. Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegQueryValueBinary EmcutilRegQueryValueBinaryA
#else
#define EmcutilRegQueryValueBinary EmcutilRegQueryValueBinaryW
#endif

//////////

/*!
 * @brief Convenience wrapper for EmcutilRegQueryValue for strings. 
 *        Same as EmcutilRegQueryValueString but takes absolute/full Registry key.\n
 *        Call EmcutilRegQueryReturnAllocatedValue() to free returned value.
 * @param subKeyName Full key pathname
 * @param valueName Value name
 * @param valueDefault Optional default value
 * @param valuePtrRv [OUT] Pointer to returned string
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryValueStringDirectA(
    EMCUTIL_REG_SUBKEY_NAME_A subKeyName,
    EMCUTIL_REG_VALUE_NAME_A valueName,
    csx_cstring_t valueDefault,
    csx_string_t * valuePtrRv);

/*!
 * @brief Convenience wrapper for EmcutilRegQueryValue for strings. Using Unicode strings. 
 *        Same as EmcutilRegQueryValueString but takes absolute/full Registry key.\n
 *        Call EmcutilRegQueryReturnAllocatedValue() to free returned value.
 * @param subKeyName Full key pathname (Unicode)
 * @param valueName Value name (Unicode)
 * @param valueDefault Optional default value (Unicode)
 * @param valuePtrRv [OUT] Pointer to returned string (in Unicode)
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryValueStringDirectW(
    EMCUTIL_REG_SUBKEY_NAME_W subKeyName,
    EMCUTIL_REG_VALUE_NAME_W valueName,
    csx_cwstring_t valueDefault, 
    csx_wstring_t * valuePtrRv);

/*!
 * @brief Convenience wrapper for EmcutilRegQueryValue for multistrings. 
 *        Same as EmcutilRegQueryValueMultiString but takes absolute/full Registry key.\n
 *        Call EmcutilRegQueryReturnAllocatedValue() to free returned value.
 * @param subKeyName Full key pathname
 * @param valueName Value name
 * @param valueDefault Optional default value
 * @param valuePtrRv [OUT] Pointer to returned multistring value (Ascii)
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryValueMultiStringDirectA(
    EMCUTIL_REG_SUBKEY_NAME_A subKeyName,
    EMCUTIL_REG_VALUE_NAME_A valueName,
    csx_cstring_t valueDefault,
    csx_string_t * valuePtrRv);

/*!
 * @brief Convenience wrapper for EmcutilRegQueryValue for multistrings. Using Unicode strings. 
 *        Same as EmcutilRegQueryValueMultiString but takes absolute/full Registry key.\n
 *        Call EmcutilRegQueryReturnAllocatedValue() to free returned value.
 * @param subKeyName Full key pathname (Unicode)
 * @param valueName Value name (Unicode)
 * @param valueDefault Optional default value (Unicode)
 * @param valuePtrRv [OUT] Pointer to returned multistring value (in Unicode)
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryValueMultiStringDirectW(
    EMCUTIL_REG_SUBKEY_NAME_W subKeyName,
    EMCUTIL_REG_VALUE_NAME_W valueName,
    csx_cwstring_t valueDefault,
    csx_wstring_t * valuePtrRv);

/*!
 * @brief Convenience wrapper for EmcutilRegQueryValue for 32-bit values. 
 *        Same as EmcutilRegQueryValue32 but takes absolute/full Registry key.\n
 *        Call EmcutilRegQueryReturnAllocatedValue() to free returned value.
 * @param subKeyName Full key pathname
 * @param valueName Value name
 * @param valueDefault Pointer to optional default value
 * @param valuePtrRv [OUT] Pointer to returned 32-bit value
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryValue32DirectA(
    EMCUTIL_REG_SUBKEY_NAME_A subKeyName,
    EMCUTIL_REG_VALUE_NAME_A valueName,
    csx_u32_t * valueDefault,
    csx_u32_t * valuePtrRv);

/*!
 * @brief Convenience wrapper for EmcutilRegQueryValue for 32-bit values. Using Unicode strings. 
 *        Same as EmcutilRegQueryValue32 but takes absolute/full Registry key.\n
 *        Call EmcutilRegQueryReturnAllocatedValue() to free returned value.
 * @param subKeyName Full key pathname (Unicode)
 * @param valueName Value name (Unicode)
 * @param valueDefault Pointer to optional default value
 * @param valuePtrRv [OUT] Pointer to returned 32-bit value
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryValue32DirectW(
    EMCUTIL_REG_SUBKEY_NAME_W subKeyName,
    EMCUTIL_REG_VALUE_NAME_W valueName,
    csx_u32_t * valueDefault,
    csx_u32_t * valuePtrRv);

/*!
 * @brief Convenience wrapper for EmcutilRegQueryValue for 64-bit values. 
 *        Same as EmcutilRegQueryValue64 but takes absolute/full Registry key.\n
 *        Call EmcutilRegQueryReturnAllocatedValue() to free returned value.
 * @param subKeyName Full key pathname
 * @param valueName Value name
 * @param valueDefault Pointer to optional default value
 * @param valuePtrRv [OUT] Pointer to returned 64-bit value
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryValue64DirectA(
    EMCUTIL_REG_SUBKEY_NAME_A subKeyName,
    EMCUTIL_REG_VALUE_NAME_A valueName,
    csx_u64_t * valueDefault, 
    csx_u64_t * valuePtrRv);

/*!
 * @brief Convenience wrapper for EmcutilRegQueryValue for 64-bit values. Using Unicode strings. 
 *        Same as EmcutilRegQueryValue64 but takes absolute/full Registry key.\n
 *        Call EmcutilRegQueryReturnAllocatedValue() to free returned value.
 * @param subKeyName Full key pathname (Unicode)
 * @param valueName Value name (Unicode)
 * @param valueDefault Pointer to optional default value
 * @param valuePtrRv [OUT] Pointer to returned 64-bit value
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryValue64DirectW(
    EMCUTIL_REG_SUBKEY_NAME_W subKeyName,
    EMCUTIL_REG_VALUE_NAME_W valueName,
    csx_u64_t * valueDefault,
    csx_u64_t * valuePtrRv);

/*!
 * @brief Convenience wrapper for EmcutilRegQueryValue for binary blobs.
 *        Same as EmcutilRegQueryValueBinary but takes absolute/full Registry key.\n
 *        Call EmcutilRegQueryReturnAllocatedValue() to free returned value.
 * @param subKeyName Full key pathname
 * @param valueName Value name
 * @param valueDefaultPtr Pointer to optional default value
 * @param valueDefaultSize Size of optional default value
 * @param valuePtrRv [OUT] Pointer to returned binary blob
 * @param valueSizeRv [OUT] Pointer to size of returned binary blob
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryValueBinaryDirectA(
    EMCUTIL_REG_SUBKEY_NAME_A subKeyName,
    EMCUTIL_REG_VALUE_NAME_A valueName,
    csx_cpvoid_t valueDefaultPtr,
    EMCUTIL_SIZE_T valueDefaultSize,
    csx_pvoid_t * valuePtrRv,
    EMCUTIL_SIZE_T * valueSizeRv);

/*!
 * @brief Convenience wrapper for EmcutilRegQueryValue for binary blobs. Using Unicode strings.
 *        Same as EmcutilRegQueryValueBinary but takes absolute/full Registry key.\n
 *        Call EmcutilRegQueryReturnAllocatedValue() to free returned value.
 * @param subKeyName Full key pathname (Unicode)
 * @param valueName Value name (Unicode)
 * @param valueDefaultPtr Pointer to optional default value
 * @param valueDefaultSize Size of optional default value
 * @param valuePtrRv [OUT] Pointer to returned binary blob
 * @param valueSizeRv [OUT] Pointer to size of returned binary blob
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegQueryValueBinaryDirectW(
    EMCUTIL_REG_SUBKEY_NAME_W subKeyName,
    EMCUTIL_REG_VALUE_NAME_W valueName,
    csx_cpvoid_t valueDefaultPtr,
    EMCUTIL_SIZE_T valueDefaultSize,
    csx_pvoid_t * valuePtrRv,
    EMCUTIL_SIZE_T * valueSizeRv);

/*!
 * @brief Query for string value, takes absolute/full Registry key. 
 *        Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegQueryValueStringDirect EmcutilRegQueryValueStringDirectA
#else
#define EmcutilRegQueryValueStringDirect EmcutilRegQueryValueStringDirectW
#endif

/*!
 * @brief Query for multistring value, takes absolute/full Registry key. 
 *        Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegQueryValueMultiStringDirect EmcutilRegQueryValueMultiStringDirectA
#else
#define EmcutilRegQueryValueMultiStringDirect EmcutilRegQueryValueMultiStringDirectW
#endif

/*!
 * @brief Query for 32-bit value, takes absolute/full Registry key. 
 *        Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegQueryValue32Direct EmcutilRegQueryValue32DirectA
#else
#define EmcutilRegQueryValue32Direct EmcutilRegQueryValue32DirectW
#endif

/*!
 * @brief Query for 64-bit value, takes absolute/full Registry key. 
 *        Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegQueryValue64Direct EmcutilRegQueryValue64DirectA
#else
#define EmcutilRegQueryValue64Direct EmcutilRegQueryValue64DirectW
#endif

/*!
 * @brief Query for binary-blob value, takes absolute/full Registry key. 
 *        Ascii or Unicode based on EmcUTIL configuration.
 */
#ifndef EMCUTIL_UNICODE_IN_USE
#define EmcutilRegQueryValueBinaryDirect EmcutilRegQueryValueBinaryDirectA
#else
#define EmcutilRegQueryValueBinaryDirect EmcutilRegQueryValueBinaryDirectW
#endif

////////////////////////////////////

/*! @} END emcutil_registry_abs */

CSX_CDECLS_END

////////////////////////////////////

#include "EmcUTIL_Registry_PLA.h"

////////////////////////////////////

//++
//.End file EmcUTIL_Registry.h
//--

#endif                                     /* EMCUTIL_REGISTRY_H_ */
