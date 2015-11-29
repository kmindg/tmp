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

#ifndef __MemoryBasedConfigDatabase__
#define __MemoryBasedConfigDatabase__

//++
// File Name:
//      MemoryBasedConfigDatabase.h
//
// Contents:
//      Defines an implementation of a ConfigDatabase that only uses memory, and has no
//      persistence.
//
//      This file defines the types MemoryBasedDatabaseValue and MemoryBasedDatabaseKey.
//
// Revision History:
//--
#include "Local_ConfigDatabase.h"
#include "SinglyLinkedList.h"

class MemoryBasedDatabaseValue;
class MemoryBasedDatabaseKey;

SLListDefineListType(MemoryBasedDatabaseValue, ListOfMemoryBasedDatabaseValue);

SLListDefineListType(MemoryBasedDatabaseKey, ListOfMemoryBasedDatabaseKey);

// MemoryBasedDatabaseValue works in conjunction with the class MemoryBasedDatabaseKey,
// where the latter class allows values to be found and located.
//
// MemoryBasedDatabaseValue implements the DatabaseValue interface.
class MemoryBasedDatabaseValue : public DatabaseValue {
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
    const K10_WWID  GetK10_WWID(const K10_WWID defValue = K10_WWID_Zero()) const;


    // Returns true if a valid value is held, false if there were allocation problems
    // during construction.
    bool IsValueValid() const { return mType != VT_UNKNOWN; } 


    // Returns a reference to the value as a string.  The reference only exists during the
    // lifetime of the value.
    // @param value - the value to return
    VOID GetValue(PCHAR & value) const;

    // Get a pointer to the binary value from the database key. 
    // @param ValuePtr - Returns the pointer to the value.  This value exists only while
    //                   the value remains open.
    // @param ValueLen - the number of bytes stored in ValuePtr.
    VOID GetValue(PVOID & ValuePtr,  ULONG & ValueLen) const;


    // Release this DatabaseValue.
    void Close();


    // Construct the value, given the value should hold this type of data.
    // @param valueName - the name to use as the key for the value, used to locate the
    //                    value under a database key.  This name must not change for the
    //                    lifetime of this object.
    // 
    // @param defValue - the data to be copied into this object.
    MemoryBasedDatabaseValue(const PCHAR valueName, ULONG defValue);

    // Construct the value, given the value should hold this type of data.
    // @param valueName - the name to use as the key for the value, used to locate the
    //                    value under a database key.  This name must not change for the
    //                    lifetime of this object.
    // 
    // @param defValue - the data to be copied into this object.
    MemoryBasedDatabaseValue(const PCHAR valueName, ULONGLONG defValue);

    // Construct the value, given the value should hold this type of data.
    // @param valueName - the name to use as the key for the value, used to locate the
    //                    value under a database key.  This name must not change for the
    //                    lifetime of this object.
    // 
    // @param defValue - the data to be copied into this object.
    MemoryBasedDatabaseValue(const PCHAR valueName, K10_WWID defValue);

    // Construct the value, given the value should hold this type of data.
    // @param valueName - the name to use as the key for the value, used to locate the
    //                    value under a database key.  This name must not change for the
    //                    lifetime of this object.
    // 
    // @param defValue - the data to be copied into this object.
    //
    // Note: this constructor may fail.  IsValueValid() must be called to determine
    // whether construction was successful.
    MemoryBasedDatabaseValue(const PCHAR valueName, const PCHAR defValue);

    // Construct the value, given the value should hold this type of data.
    // @param valueName - the name to use as the key for the value, used to locate the
    //                    value under a database key.  This name must not change for the
    //                    lifetime of this object.
    // 
    // @param defValue - the data to be copied into this object.
    //
    // Note: this constructor may fail.  IsValueValid() must be called to determine
    // whether construction was successful.
    MemoryBasedDatabaseValue(PCWSTR valueName, EMCPAL_UNICODE_STRING * defValue);


    // Construct a binary value, given the value should hold this type of data.
    // @param valueName - the name to use as the key for the value, used to locate the
    //                    value under a database key. This name must not change for the
    //                    lifetime of this object.
    //
    // @param binaryValue - the data to be copied into this object.
    // @param len - the number of bytes to data to copy.
    //
    // Note: this constructor may fail.  IsValueValid() must be called to determine
    // whether construction was successful.
    MemoryBasedDatabaseValue(const PCHAR valueName, PVOID binaryValue, ULONG len);

private:
    friend struct ListOfMemoryBasedDatabaseValue;   
    friend struct ListOfMemoryBasedDatabaseValueNoConstructor;   

    // Allows lists of database values to be created.  
    MemoryBasedDatabaseValue *   mLink;

    friend class MemoryBasedDatabaseKey;

    // Every DatabaseValue has both a name and a value. This is the name.
    PCHAR mValueName;

    // What kind of value is this?
    enum ValueType {
        // Value not set
        VT_UNKNOWN, 
        // for type ULONG
        VT_ULONG, 

        // for type ULONGLONG
        VT_ULONGLONG, 

        // for type K10_WWID
        VT_K10_WWID, 

        // for type UNICODE_STRING
        VT_STRING, 

        // for arbitrary size byte array.
        VT_BINARY 
    };


    // Type of value, selects the member of the union.
    ValueType mType;

    // Holds the actual data, depending upon mType, except for VT_STRING
    union {

        // Used for VT_ULONG
        ULONG       ULong;

        // Used for VT_ULONGLONG
        ULONGLONG   ULongLong;

        // Used for VT_K10_WWID
        K10_WWID    K10wwid;

        // Used for VT_BINARY
        struct {
            ULONG   len;

            // Memory is allocated separately for this.
            PVOID   data;

        } Binary;
    };


    // Used by VT_STRING
    PCHAR StringValue;

    // Total number of reference across all objects, solely for debug, tracking down
    // leaks.
    static LONG gTotalRefCount;

    // Increment the reference count for this value
    VOID RefCountIncrement();

    // Decrement the reference count for this value, and return the new value.  If 0 is
    // returned, the caller is expected to delete this object.
    ULONG RefCountDecrement();

    // The current number of references to this object.  This object should be deleted
    // when this transitions to 0.
    ULONG mRefCount;

    ~MemoryBasedDatabaseValue();
   
};


// MemoryBasedDatabaseKey works in conjunction with the class MemoryBasedDatabaseValue.
//
// A MemoryBasedDatabaseKey is a node in a tree structure, which has 0 or 1 parents, and
// 0-n children, where the children can be either of type MemoryBasedDatabaseKey or
// MemoryBasedDatabaseValue.
//
// MemoryBasedDatabaseKey implements the DatabaseKey interface.
class MemoryBasedDatabaseKey : public DatabaseKey {
public:
    // Construct a key.
    // @param KeyName - the name for the key, which is copied into the key.
    //
    // Warning: copying the key name requires a memory allocation which may fail.
    // GetKeyName() should be verified as being non-NULL after construction.
    MemoryBasedDatabaseKey(const char * KeyName);


    // Release a reference to this DatabaseKey.  Implementation of pure virtual function
    // from base class
    VOID Close();

    // Find a sub-key by name. Implementation of pure virtual function from base class
    //
    // @param   SubKey - the name of the sub-key to find.
    // 
    // Return Value:
    //  Pointer to the sub-key.  The caller must Close() this sub-key when done. NULL if
    //  the subkey does not exist.
    DatabaseKey * OpenSubKey(const PCHAR SubKey);

    // 
    //  Opens the next subkey under this value.  Allows enumeration (once) of all SubKeys
    //  for this Key.
    // Implementation of pure virtual function from base class
    // 
    // Return Value:
    //   - NULL All SubKeys have been enumerated. 
    //   - non-NULL: pointer to the sub-key.  The caller must delete this sub-key when
    //     done.
    DatabaseKey * OpenNextSubKey();

    // 
    //    Cause *Next* functions to start over from the beginning
    // Implementation of pure virtual function from base class
    void ResetIterators() { mKeyIndex = 0; mValueIndex = 0; }

    // Find a value by name. Implementation of pure virtual function from base class
    //
    // @param ValueName - the name of the value to find
    // 
    // Return Value:
    //   - NULL:  value did not exist.  
    //   - non-NULL: pointer to the value.  The caller must Close() this value when done.
    DatabaseValue * GetValue(const PCHAR ValueName) const;

    // 
    //   Allows enumeration (once) of all values for this Key.
    // Implementation of pure virtual function from base class
    //
    // @param ValueName - the name of the value to find
    //
    // Return Value:
    //   Pointer to the next value.  The caller must Close() this value when done. NULL
    //   indicates that all values have been enumerated.
    DatabaseValue * GetNextValue(const PCHAR ValueName) { /*STUB*/ return NULL; }


    // Find a value by name, and returns its contents as a string. Implementation of pure
    // virtual function from base class
    //
    // @param ValueName - the name of the value to return
    // 
    // Return Value:
    //   true - the value was found, and a string returned.
    //   false - the value was not found or memory could not be allocated
    bool GetValue(const PCHAR ValueName, K10AnsiString & string) const;

    // Find a value by name, and returns its contents as a string. Implementation of pure
    // virtual function from base class
    //
    // @param ValueName - the name of the value to return
    // @param string - function will copy the string in this buffer
    // @param maxLen - Maximum characters that buffer can store.
    // 
    // Return Value:
    //   true - the value was found, and a string returned.
    //   false - the value was not found or memory could not be allocated
    bool GetValue(const PCHAR ValueName, char* string, int maxLen) const;

    // Find a value by name, and returns its contents as a ULONG. Implementation of pure
    // virtual function from base class
    //
    // @param ValueName - the name of the value to return
    // @param defaultValue - the value to return if ValueName not found or a resource
    //                       allocation failed or the value was the wrong type.
    // 
    // Return Value:
    //    The value if it exists, otherwise defaultValue.
    //
    ULONG GetULONGValue(const PCHAR ValueName, ULONG defaultValue = 0) const;

    // Find a value by name, and returns its contents as a 128bit WWN. Implementation of
    // pure virtual function from base class
    //
    // @param ValueName - the name of the value to return
    // @param defaultValue - the value to return if ValueName not found or a resource
    //                       allocation failed or the value was the wrong type.
    // 
    // Return Value:
    //    The value if it exists, otherwise defaultValue.
    //
    K10_WWID GetK10_WWIDValue(const PCHAR ValueName, K10_WWID defaultValue = K10_WWID_Zero()) const;

    // Return the name of this key.  Implementation of pure virtual function from base
    // class
    PCHAR GetKeyName() const { return mKeyName; }

    // Find a value by name, and returns its contents as a binary value. Implementation of
    // pure virtual function from base class
    //
    // @param ValueName - the name of the value to return
    // @param defaultValue - the value to return if ValueName not found or a resource
    //                       allocation failed or the value was the wrong type.
    // 
    // Return Value:
    //    The value, if it exists and is of type ULONG or ULONGLONG, otherwise
    //    defaultValue.
    //
    ULONGLONG GetULONGLONGValue(const PCHAR ValueName, ULONGLONG defaultValue = 0) const;
   
    // Find a value by name, and returns its contents as a binary value. Implementation of
    // pure virtual function from base class
    //
    // @param ValueName - the name of the value to return
    // @param defaultValue - the value to return if ValueName not found or a resource
    //                       allocation failed or the value was the wrong type.
    // 
    // Return Value:
    //    The value if it exists, otherwise defaultValue.
    //
    VOID GetBinaryValue(const PCHAR ValueName, PVOID & ValuePtr,  ULONG & ValueLen) const;

    // Add a key below this node
    // @param key - the key to add
    VOID AddSubKey(  MemoryBasedDatabaseKey * key);

    // Add a value below this node
    // @param value - the value to add
    VOID AddValue(  MemoryBasedDatabaseValue * value);


    // Allows this item to be linked onto lists.
    MemoryBasedDatabaseKey *   mLink;

private:

    // The name used to find this key within the parent key
    PCHAR          mKeyName;

    // The list of sub-keys
    ListOfMemoryBasedDatabaseKey    mKeys;

    // The list of values attached to this key
    ListOfMemoryBasedDatabaseValue mValues;

    // Used for iterating though all subkeys of this key
    ULONG mKeyIndex;

    // Used for iterating through all values directly under this key
    ULONG mValueIndex;

    // Debugging code to look for leaks
    static LONG gTotalRefCount;

    // Increment the reference count for this object.
    VOID RefCountIncrement();

    // Decrement the reference count for this object. Returns the value for this reference
    // count.
    ULONG RefCountDecrement();

    // The current number of references.
    ULONG mRefCount;

    ~MemoryBasedDatabaseKey();

};

SLListDefineInlineCppListTypeFunctions(MemoryBasedDatabaseKey, 
                                       ListOfMemoryBasedDatabaseKey, mLink);
SLListDefineInlineCppListTypeFunctions(MemoryBasedDatabaseValue, 
                                       ListOfMemoryBasedDatabaseValue, mLink);


#endif  // __BasicVolume__

