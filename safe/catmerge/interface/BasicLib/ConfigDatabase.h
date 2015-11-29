/************************************************************************
*
*  Copyright (c) 2002, 2005-2009 EMC Corporation.
*  All rights reserved.
*
*  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.  ACCESS TO THIS
*  SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT. THIS CODE IS TO
*  BE KEPT STRICTLY CONFIDENTIAL.
*
*  UNAUTHORIZED MODIFICATIONS OF THIS FILE WILL VOID YOUR SUPPORT
*  CONTRACT WITH EMC CORPORATION.  IF SUCH MODIFICATIONS ARE FOR
*  THE PURPOSE OF CIRCUMVENTING LICENSING LIMITATIONS, LEGAL ACTION
*  MAY RESULT.
*
************************************************************************/

#ifndef __ConfigDatabase__
#define __ConfigDatabase__

//++
// File Name:
//      ConfigDatabase.h
//
// Contents:
//      A "Configuration Database" is an interface for accessing configuration
//      information, where that information is in a tree structure.  The interface does
//      not define how the data is stored, but the interface is modeled after the Windows
//      registry.
//
//      A Configuration Databse constist of DatabaseValues and DatabaseKeys.  
//
//      A "database" accessed by DatabaseKey/DatabaseValue is a tree structure (no cycles)
//
//      A DatabaseKey holds 0-n other DatabaseKeys, and 0-m DatabaseValues.
//
//      DatabaseValues are typed (EMCPAL_UNICODE_STRING, ULONG, WWID, BINARY), and can only be
//      leaf nodes in the tree.
//
//            
// Revision History:
//--
# include "k10defs.h"
# include "K10AnsiString.h"

class DatabaseValue;

//  A DatabaseKey is the root of a ConfigDatabase. A ConfigDatabase is a tree of
//  DatabaseKeys with DatabaseValues at the leaves.
//
//  A DatabaseKey holds 0-n other DatabaseKeys, and 0-m DatabaseValues.
//
//  DatabaseValues are typed (EMCPAL_UNICODE_STRING, ULONG, WWID, BINARY).
class DatabaseKey {
public:

    // Release a reference to this DatabaseKey.
    virtual VOID Close() = 0;

    // Find a sub-key by name.
    //
    // @param   SubKey - the name of the sub-key to find.
    // 
    // Return Value:
    //  pointer to the sub-key.  The caller must Close() this sub-key when done. NULL
    //  Indicates the subkey does not exist
    virtual DatabaseKey * OpenSubKey(const PCHAR SubKey) = 0;

    // 
    //    Cause *Next* functions to start over from the beginning
    virtual void ResetIterators() = 0;

    // 
    //  Opens the next subkey under this value.  Allows enumeration (once) of all SubKeys
    //  for this Key.
    // 
    // Return Value:
    //   - NULL: All SubKeys have been enumerated. 
    //   - non-NULL: pointer to the sub-key.  The caller must delete this sub-key when
    //     done.
    virtual DatabaseKey * OpenNextSubKey() = 0;

    // Find a value by name.
    //
    // @param ValueName - the name of the value to find
    // 
    // Return Value:
    //   - NULL  value did not exist. 
    //   - non-NULL: pointer to the value.  The caller must Close() this value when done.
    virtual DatabaseValue * GetValue(const PCHAR ValueName) const = 0;

    // 
    //   Allows enumeration (once) of all values for this Key.
    //
    // @param ValueName - the name of the value to find
    //
    // Return Value:
    //   - NULL: All values have been enumerated.   
    //   - non-NULL: pointer to the values.  The caller must Close() this value when done.
    virtual DatabaseValue * GetNextValue(const PCHAR ValueName) = 0;

    // Find a value by name, and returns its contents as a string.
    //
    // @param ValueName - the name of the value to return
    // 
    // Return Value:
    //   true - the value was found, and a string returned.
    //   false - the value was not found or memory could not be allocated
    virtual bool GetValue(const PCHAR ValueName, K10AnsiString & string) const = 0;

    // Find a value by name, and returns its contents as a string.
    //
    // @param ValueName - the name of the value to return
    // @param string - returns the value of key in this buffer.
    // @param maxLen - Maximum characters that buffer can store.
    // 
    // Return Value:
    //   true - the value was found, and a string returned.
    //   false - the value was not found or memory could not be allocated
    virtual bool GetValue(const PCHAR ValueName, char* string, int maxLen) const = 0;

    // Find a value by name, and returns its contents as a ULONG.
    //
    // @param ValueName - the name of the value to return
    // @param defaultValue - the value to return if ValueName not found or a resource
    //                       allocation failed or the value was the wrong type.
    // 
    // Return Value:
    //    The value if it exists, otherwise defaultValue.
    //
    virtual ULONG GetULONGValue(const PCHAR ValueName, ULONG defaultValue = 0) const = 0;

    // Find a value by name, and returns its contents as a ULONGLONG.
    //
    // @param ValueName - the name of the value to return
    // @param defaultValue - the value to return if ValueName not found or a resource
    //                       allocation failed or the value was the wrong type.
    // 
    // Return Value:
    //    The value if it exists, otherwise defaultValue.
    //
    virtual ULONGLONG GetULONGLONGValue(const PCHAR ValueName, ULONGLONG defaultValue = 0) const = 0;

    // Find a value by name, and returns its contents as a 128bit WWN.
    //
    // @param ValueName - the name of the value to return
    // @param defaultValue - the value to return if ValueName not found or a resource
    //                       allocation failed or the value was the wrong type.
    // 
    // Return Value:
    //    The value if it exists, otherwise defaultValue.
    //
    virtual K10_WWID GetK10_WWIDValue(const PCHAR ValueName, K10_WWID defaultValue = K10_WWID_Zero()) const = 0;

    // Get a binary value from the database key. Returns TRUE if no errors, FALSE otherwise.
    // @param ValueName - the name of the value to find
    // @param ValuePtr - Pointer to buffer where binary data should be returned.
    // @param MaxLen - the max number of bytes to be stored in ValuePtr.  
    // @param ValueLen - Number of binary bytes returned.  Returned as 0 if the ValueName 
    //                  does not exist or if there is a conversion error.  Returns the length
    //                  required on buffer overflow
    virtual bool GetBinaryValue(const PCHAR ValueName, PVOID ValuePtr,  ULONG MaxLen, ULONG & ValueLen) const = 0;

    //  Return the name of this key. 
    virtual PCHAR GetKeyName() const= 0;

    // Factory which constructs the Dat
    static DatabaseKey *Factory(const PCHAR  RegistryPath);

    // size of DatabaseKey
    static ULONG KeySize();

protected:
    virtual ~DatabaseKey() {};

};


// Represents a leaf node (e.g., file) in the Database Tree.
class DatabaseValue {
public:
    // Returns the name of this DatabaseValue.  The return value's lifetime is limited to
    // the lifetime of this database value.
    virtual PCHAR GetName() const = 0;

    // Returns the value as a ULONG.  
    //
    // @param defValue - the value to return if the value cannot be converted to ULONG.
    virtual const ULONG     GetULONG(ULONG defValue = 0) const = 0;

    // Returns the value as a ULONGLONG.  
    //
    // @param defValue - the value to return if the value cannot be converted to
    //                   ULONGLONG.
    virtual const ULONGLONG GetULONGLONG(ULONGLONG defValue = 0) const = 0;


    // Returns the value as a 128 bit WWN.  
    // @param defValue - the value to return if the value cannot be converted to K10_WWID.
    virtual const K10_WWID  GetK10_WWID(const K10_WWID defValue = K10_WWID_Zero()) const = 0;

    // Get a pointer to the binary value from the database key. 
    // @param ValuePtr - Pointer to buffer where binary data should be returned.
    // @param MaxLen - Maximum number of bytes of binary data buffer can hold
    // @param ValueLen - Number of binary bytes returned.  Returned as 0 if the ValueName 
    //                  does not exist or if there is a conversion error.  Returns the length
    //                  required on buffer overflow
    virtual bool GetBinary(PVOID ValuePtr, ULONG MaxLen, ULONG & ValueLen) const = 0;

    // Returns a reference to the value as a string.  The reference only exists during the
    // lifetime of the value.
    // @param value - the value to return
    virtual VOID GetValue(PCHAR & value) const = 0;

    // Get a pointer to the value and its length from the database key. 
    // @param ValuePtr - Returns the pointer to the value.  This value exists only while
    //                   the value remains open.
    // @param ValueLen - the number of bytes stored in ValuePtr.
    virtual VOID GetValue(PVOID & ValuePtr,  ULONG & ValueLen) const = 0;

    // Release is DatabaseValue.
    virtual VOID Close() = 0;

protected:
    // Destroys a value.
    virtual ~DatabaseValue() {}

};





#endif  // __BasicVolume__

