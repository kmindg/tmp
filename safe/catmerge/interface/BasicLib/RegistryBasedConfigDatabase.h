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

#ifndef __RegistryBasedConfigDatabase__
#define __RegistryBasedConfigDatabase__

//++
// File Name:
//      RegistryBasedConfigDatabase.h
//
// Contents:
//      An implementation of a ConfigDatabase that utilizes the registry.
//
// Revision History:
//--
#include "EmcPAL.h"
#include "ConfigDatabase.h"

class RegistryBasedDatabaseValue;

// A RegistryBasedConfigDatabase is an implementation of the ConfigDatabase that sits on
// top of the windows registry.
class RegistryBasedDatabaseKey : public DatabaseKey {
public:
    // Constructor for RegistryBasedDatabaseKey.
    // @param RegistryPathName - the registry path this key should reference.
    RegistryBasedDatabaseKey(const PCHAR  RegistryPathName);

    // Find a sub-key by name.
    //
    // @param   SubKey - the name of the sub-key to find.
    // 
    // Return Value:
    //  pointer to the sub-key.  The caller must Close() this sub-key when done. NULL
    //  Indicates the subkey does not exist
    DatabaseKey * OpenSubKey(const PCHAR SubKey);

    // Opens the next subkey under this value.  Allows enumeration (once) of all SubKeys
    // for this Key.
    // 
    // Return Value:
    //   - NULL: All SubKeys have been enumerated. 
    //   - non-NULL: pointer to the sub-key.  The caller must delete this sub-key when
    //     done.
    DatabaseKey * OpenNextSubKey();

    // Find a value by name.
    //
    // @param ValueName - the name of the value to find
    // 
    // Return Value:
    //   - NULL  value did not exist. 
    //   - non-NULL: pointer to the value.  The caller must Close() this value when done.
    DatabaseValue * GetValue(const PCHAR ValueName) const;


    //   Allows enumeration (once) of all values for this Key.
    //
    // @param ValueName - the name of the value to find
    //
    // Return Value:
    //   - NULL: All values have been enumerated.   
    //   - non-NULL: pointer to the values.  The caller must Close() this value when done.
    DatabaseValue * GetNextValue(const PCHAR ValueName) { /*STUB*/ return NULL; }

    // Find a value by name, and returns its contents as a string.
    //
    // @param ValueName - the name of the value to return
    // 
    // Return Value:
    //   true - the value was found, and a string returned.
    //   false - the value was not found or memory could not be allocated
    bool GetValue(const PCHAR ValueName, K10AnsiString & string) const;

    // Find a value by name, and returns its contents as a char string.
    //
    // @param ValueName - the name of the value to return
    // @param string - function will copy the string in this buffer
    // @param maxLen - Maximum characters that buffer can store.
    // 
    // Return Value:
    //   true - the value was found, and a string returned.
    //   false - the value was not found or memory could not be allocated
    bool GetValue(const PCHAR ValueName, char* string, int maxLen) const;

    // Returns the value as a ULONG.  
    //
    // @param defValue - the value to return if the value cannot be converted to ULONG.
    ULONG GetULONGValue(const PCHAR ValueName, ULONG defaultValue = 0) const;

    // Find a value by name, and returns its contents as a 128bit WWN.
    //
    // @param ValueName - the name of the value to return
    // @param defaultValue - the value to return if ValueName not found or a resource
    //                       allocation failed or the value was the wrong type.
    // 
    // Return Value:
    //    The value if it exists, otherwise defaultValue.
    K10_WWID GetK10_WWIDValue(const PCHAR ValueName, K10_WWID defaultValue = K10_WWID_Zero()) const;

    // Get a pointer to the value and its length from the database key. 
    // @param ValuePtr - Returns the pointer to the value.  This value exists only while
    //                   the value remains open.
    // @param ValueLen - the number of bytes stored in ValuePtr.
    VOID GetValue(PVOID & ValuePtr,  ULONG & ValueLen) const;

    // Returns the name of this key.  If NULL, indicates that there was a memory
    // allocation problem on construction.
    PCHAR GetKeyName() const { return mRegKeyName; }

    // 
    //    Cause *Next* functions to start over from the beginning    
    void ResetIterators() { mKeyIndex = 0; mValueIndex = 0; }

    // Returns the value as a ULONGLONG.  
    //
    // @param defValue - the value to return if the value cannot be converted to
    //                   ULONGLONG.
    ULONGLONG GetULONGLONGValue(const PCHAR ValueName, ULONGLONG defaultValue = 0) const;

    // Get a binary value from the database key. 
    // @param ValueName - the name of the value to find
    // @param ValuePtr - Pointer to buffer where binary data should be returned.
    // @param MaxLen - the max number of bytes to be stored in ValuePtr.  
    // @param ValueLen - Number of binary bytes returned.  Returned as 0 if the ValueName 
    //                  does not exist or if there is a conversion error.  Returns the length
    //                  required on buffer overflow
    bool GetBinaryValue(const PCHAR ValueName, PVOID ValuePtr,  ULONG MaxLen, ULONG & ValueLen) const;

    // Close if previously open.
    VOID Close();


private:
    ~RegistryBasedDatabaseKey();

    // @param hKey - the already opened handle to the subkey.
    // @param keyName - a string containing the name of this key, newly allocated, where
    //                  this class is taking over responsibility for deleting this string.
    RegistryBasedDatabaseKey(EMCPAL_REGISTRY_HANDLE hKey, PCHAR keyName);

    // Used for iterating though all subkeys of this key
    ULONG mKeyIndex;

    // Used for iterating through all values directly under this key
    ULONG mValueIndex;

    // Debugging code to look for leaks
    static LONG gTotalRefCount;

    // Handle recieved from the EmcpalRegistryOpenKey call.
    EMCPAL_REGISTRY_HANDLE mKeyHandle;

    // The name of this key
    PCHAR mRegKeyName;

    // Increment the reference count for this object.
    VOID RefCountIncrement();

    // Decrement the reference count for this object. Returns the value for this reference
    // count.
    ULONG RefCountDecrement();

    // The current number of references.
    ULONG mRefCount;
};


//RegistryBasedDatabaseValue is an implementation of a ConfigDatabase that utilizes the
//registry.
class RegistryBasedDatabaseValue : DatabaseValue {
public:
    // Returns the name of this DatabaseValue.  The return value's lifetime is limited to
    // the lifetime of this database value.
    PCHAR GetName() const;

    // Returns the value as a ULONG.  
    //
    // @param defValue - the value to return if the value cannot be converted to ULONG.
    const ULONG     GetULONG(ULONG defValue = 0) const;

    // Returns the value as a ULONGLONG.  
    //
    // @param defValue - the value to return if the value cannot be converted to
    //                   ULONGLONG.
    const ULONGLONG GetULONGLONG(ULONGLONG defValue = 0) const;

    // Returns the value as a 128 bit WWN.  
    // @param defValue - the value to return if the value cannot be converted to K10_WWID.
    const K10_WWID  GetK10_WWID(const K10_WWID defValue = K10_WWID()) const;

    // Get the binary value and length from the database key.
    // @param ValuePtr - Pointer to buffer where binary data should be returned.
    // @param MaxLen - Maximum number of bytes of binary data buffer can hold
    // @param ValueLen - Number of binary bytes returned.  Returned as 0 if the ValueName 
    //                  does not exist or if there is a conversion error.  Returns the length
    //                  required on buffer overflow
    bool GetBinary(PVOID ValuePtr, ULONG MaxLen, ULONG & ValueLen) const;

    // Returns a reference to the value as a string.  The reference only exists during the
    // lifetime of the value.
    // @param value - the value to return
    VOID GetValue(PCHAR & value) const;

    // Get a pointer to the value and its length from the database key. 
    // @param ValuePtr - Returns the pointer to the value.  This value exists only while
    //                   the value remains open.
    // @param ValueLen - the number of bytes stored in ValuePtr.
    VOID GetValue(PVOID & ValuePtr,  ULONG & ValueLen) const;

    // Construct empty value 
    RegistryBasedDatabaseValue();

    // Release this DatabaseValue.
    VOID Close();


private:
    ~RegistryBasedDatabaseValue();

    // data read from registry.
	ULONG 	mDataLength;
	PVOID	mData;
	ULONG 	mRegPathLength;
	PCHAR   mRegPath;
	ULONG 	mParmNameLength;
	PCHAR	mParmName;

    friend class RegistryBasedDatabaseKey;

    // prevent assignments by making private.
    DatabaseValue & operator = (const DatabaseValue & val);

    // For debug.
    static LONG gTotalRefCount;

    // Increment the reference count for this value
    VOID RefCountIncrement();

    // Decrement the reference count for this value, and return the new value.  If 0 is
    // returned, the caller is expected to delete this object.
    ULONG RefCountDecrement();

    // The current number of references to this object.  This object should be deleted
    // when this transitions to 0.
    ULONG mRefCount;
};




#endif  // __BasicVolume__

