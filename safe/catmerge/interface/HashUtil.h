//**************************************************************************
// Copyright (C) EMC Corporation 2009-2012
// All rights reserved.
// Licensed material -- property of EMC Corporation
//*************************************************************************

//	The HashUtil class provides an implementation of
//  a hash table to do a one-to-one mapping of a arbitrary key type 
//  to an object it is associated with.
//
//	Revision History
//	----------------
//   04 02 09    R. Hicks    Initial version.
//   04 16 09    R. Hicks    DIMS 224902: Check for empty hash table in FindHashEntry()
//   10 30 09    D. Hesi     DIMS 239950: Throw MMAN_ERROR_DUPLICATE_HASH_KEY for duplicate key entry
//   02 12 10    R. Hicks    Complete rewrite.
//   06 20 11    D. Hesi     AR 421682: Fixed memory leak in AddEntry(). Patch candidate
//   06 20 11    D. Hesi     AR 421680: Fixed memory leak in AddEntry()
//   04 25 12    D. Hesi     Added Source and Key details to exception string

#ifndef HASH_UTIL_H
#define HASH_UTIL_H

#include "MMan.h"
#include "k10defs.h"
#include "CaptivePointer.h"
//#include "CRCUtil.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

#define HASH_TABLE_NAME		64
#define ERROR_STRING_BUFFER	1024

// hash table entry mapping a key to some object
template <class keyType, class objType> class HashEntry
{
public:
	keyType key;
	objType obj;
	HashEntry<keyType, objType> *pNext;
};

//------------------------------------------------------------HashUtil

template <class keyType, class objType> class CSX_CLASS_EXPORT HashUtil
{
public:

	// constructor
	HashUtil(DWORD baseSize, char* tableName,
		bool (*compareFunc)(const keyType &, const keyType &) = NULL);

	//destructor
	~HashUtil();

	// return the address associated with the given key
	objType* GetAssoc(const keyType &key);

	// add a new assocation in the hash table
	void AddEntry(const keyType &key, const objType &obj);

	// remove the hash table entry having the given key
	void RemoveEntry(const keyType &key);

	// clear the hash table
	void Clear();

	// remove all hash table entries associated to the given object
	void PurgeAssocEntries(const objType &obj);

	// get binary key data in printable string
	void GetHashKeyString(const BYTE *key, int key_len, char* strKey);

private:

	// Hashtable object name
	char mHashTableName[HASH_TABLE_NAME];

	// get the hash table location for the given key
	DWORD GetHashKey(const keyType &key);

	// initialization of hash table
	void Init(DWORD baseSize);

	void init_xor_table();
	DWORD crc32(csx_uchar_t *buf, DWORD len);

	// Attributes

	NAPtr <HashEntry<keyType, objType> *>	mnapHashTable;
	DWORD mHashTableSize;		// number of hash table entries

	bool (*mCompareFunc)(const keyType &, const keyType &);

	long mHashCollisions;

	DWORD crc32_xor_table[256];
};

 //constructor
template <class keyType, class objType>
HashUtil<keyType, objType>::HashUtil(DWORD baseSize, char* hashtableName,
						 bool (*compareFunc)(const keyType &, const keyType &))
:
	mCompareFunc(compareFunc),
	mHashCollisions(0)
{
	init_xor_table();

	// use prime numbers for hash table sizes
	DWORD hashSizes[] = 
	{67, 131, 257, 521, 1031, 2053, 4099, 8209, 16411, 32771,
	 65537, 131101, 262147, 524309, 1048583};

	memset(mHashTableName, 0, HASH_TABLE_NAME);
	if(hashtableName != NULL)
	{
		SAFE_SPRINTF_1(mHashTableName, (HASH_TABLE_NAME - 1), "%s", hashtableName);
	}
	else
	{
		THROW_K10_EXCEPTION("Empty Hashtable name",
					MMAN_ERROR_PROG_ERROR);
	}

	mHashTableSize = 0;
	for (int i = 0; i < (sizeof(hashSizes) / sizeof(DWORD)); i++)
	{
		// find a hash table size
		if (hashSizes[i] > (baseSize))
		{			
			mHashTableSize = hashSizes[i];
			break;
		}
	}

	if (mHashTableSize == 0)
	{
		char buffer[ERROR_STRING_BUFFER] = "";
		SAFE_SPRINTF_1(buffer, ERROR_STRING_BUFFER, "Invalid hash table size. Table Name : %s ", mHashTableName);
		THROW_K10_EXCEPTION(buffer, MMAN_ERROR_PROG_ERROR);
	}

	mnapHashTable = new HashEntry<keyType, objType> * [mHashTableSize];
	if (mnapHashTable.IsNull()) { 
		THROW_MEMORY_ERROR();
	}

	memset(*mnapHashTable, 0, sizeof(HashEntry<keyType, objType> *) * mHashTableSize);
}

template <class keyType, class objType>
void HashUtil<keyType, objType>::init_xor_table()
{
	for (int i = 0; i < 256; ++i) 
	{
		DWORD c32 = (DWORD)i;
		for (int j = 0, v = i; j < 8; ++j, v <<= 1)
		{
			c32 <<= 1;
			if (v & 0x80)
			{
				c32 ^= 0x04c11db7;
			}
		}

		crc32_xor_table[i] = c32;
	}
}

template <class keyType, class objType>
DWORD HashUtil<keyType, objType>::crc32(csx_uchar_t *buf, DWORD len)
{
	DWORD crc = ~(DWORD)0;
	for (csx_uchar_t *p = buf; len > 0; ++p, --len)
	{
		crc = crc32_xor_table[(crc >> 24) ^ *p] ^ (crc << 8);
	}
	return crc;
}

//destructor
template <class keyType, class objType>
HashUtil<keyType, objType>::~HashUtil()
{	
	Clear();
}

template <class keyType, class objType>
void HashUtil<keyType, objType>::Clear()
{	
	for (int ix = 0; ix < mHashTableSize; ix++)
	{
		if ((*mnapHashTable)[ix] != NULL)
		{
			HashEntry<keyType, objType> *pCurHashEntry = (*mnapHashTable)[ix];
			while (pCurHashEntry != NULL)
			{
				HashEntry<keyType, objType> *pTmp = pCurHashEntry->pNext;
				delete pCurHashEntry;
				pCurHashEntry = pTmp;
			}

			(*mnapHashTable)[ix] = NULL;
		}
	}

	mHashCollisions = 0;
}

// get the hash table location for the given key
template <class keyType, class objType>
DWORD HashUtil<keyType, objType>::GetHashKey(const keyType &key)
{
	// to come up with a hash table location for arbitrary types, just calculate
	// a 32-bit CRC on the data, then mod this by the hash table size
	//return ((DWORD)(CRCUtil::crc32((csx_uchar_t *)(&key), sizeof(keyType)) % mHashTableSize));
	return ((DWORD)(crc32((csx_uchar_t *)(&key), sizeof(keyType)) % mHashTableSize));
}

// return the object associated with the given key
template <class keyType, class objType>
objType* HashUtil<keyType, objType>::GetAssoc(const keyType &key)
{
	DWORD hashKey = GetHashKey(key);

	HashEntry<keyType, objType> **ppHashEntry = &(*mnapHashTable)[hashKey];

	for (HashEntry<keyType, objType> *pCurHashEntry = *ppHashEntry;
		pCurHashEntry != NULL;
		pCurHashEntry = pCurHashEntry->pNext)
	{
		if (mCompareFunc == NULL)
		{
			if (memcmp(&pCurHashEntry->key, &key, sizeof(key)) == 0)
			{
				return &pCurHashEntry->obj;
			}
		}
		else if (mCompareFunc(pCurHashEntry->key, key))
		{
			return &pCurHashEntry->obj;
		}
	}

	return NULL;
}

// add a new assocation in the hash table for the given key
template <class keyType, class objType>
void HashUtil<keyType, objType>::AddEntry(const keyType &key, const objType &obj)
{
	HashEntry<keyType, objType> *pCurHashEntry;
	HashEntry<keyType, objType> *pNewHashEntry = new HashEntry<keyType, objType>;
	if (pNewHashEntry == NULL)
	{
		THROW_MEMORY_ERROR();
	}
	pNewHashEntry->key = key;
	pNewHashEntry->obj = obj;
	pNewHashEntry->pNext = NULL;

	DWORD hashKey = GetHashKey(key);

	HashEntry<keyType, objType> **ppHashEntry = &(*mnapHashTable)[hashKey];

	if (*ppHashEntry == NULL)
	{
		*ppHashEntry = pNewHashEntry;
	}
	else
	{
		for (pCurHashEntry = *ppHashEntry; 
			;
			pCurHashEntry = pCurHashEntry->pNext)
		{
			bool bFound = false;
			if (mCompareFunc == NULL)
			{
				if (memcmp(&pCurHashEntry->key, &key, sizeof(key)) == 0)
				{
					bFound = true;
				}
			}
			else if (mCompareFunc(pCurHashEntry->key, key))
			{
				bFound = true;
			}
			if (bFound)
			{
				delete pNewHashEntry;
				char buffer[ERROR_STRING_BUFFER] = "";
				int key_len = sizeof(key);
				NAPtr<char> key_buf = new char[(key_len*2) + key_len + 1];
				GetHashKeyString((BYTE *)(&key), key_len, *key_buf);
				SAFE_SPRINTF_2(buffer, ERROR_STRING_BUFFER, 
					"Duplicate hash key. Key : 0x%s, Table Name : %s ", *key_buf, mHashTableName);

				THROW_K10_EXCEPTION(buffer, MMAN_ERROR_DUPLICATE_HASH_KEY);
			}

			if (pCurHashEntry->pNext == NULL)
			{
				break;
			}
		}

		pCurHashEntry->pNext = pNewHashEntry;

		mHashCollisions++;
	}
}

// remove the hash table entry having the given key
template <class keyType, class objType>
void HashUtil<keyType, objType>::RemoveEntry(const keyType &key)
{
	DWORD hashKey = GetHashKey(key);

	HashEntry<keyType, objType> **ppHashEntry = &(*mnapHashTable)[hashKey];

	if (*ppHashEntry == NULL)
	{
		char buffer[ERROR_STRING_BUFFER] = "";
		int key_len = sizeof(key);
		NAPtr<char> key_buf = new char[(key_len*2) + key_len + 1];
		GetHashKeyString((BYTE *)&key, key_len, *key_buf);
		SAFE_SPRINTF_2(buffer, ERROR_STRING_BUFFER, 
			"Hash key doesn't exist. Key : 0x%s, Table Name : %s ", *key_buf, mHashTableName);

		THROW_K10_EXCEPTION(buffer,	MMAN_ERROR_EXECUTION);
	}

	// match at head of list
	bool bFound = false;
	if (mCompareFunc == NULL)
	{
		if (memcmp(&(*ppHashEntry)->key, &key, sizeof(key)) == 0)
		{
			bFound = true;
		}
	}
	else if (mCompareFunc((*ppHashEntry)->key, key))
	{
		bFound = true;
	}
	if (bFound)
	{
		HashEntry<keyType, objType> *pTmp = *ppHashEntry;
		*ppHashEntry = (*ppHashEntry)->pNext;
		delete pTmp;

		return;
	}

	HashEntry<keyType, objType> *pPrevHashEntry = *ppHashEntry;
	for (HashEntry<keyType, objType> *pCurHashEntry = (*ppHashEntry)->pNext;
		pCurHashEntry != NULL;
		pPrevHashEntry = pCurHashEntry, pCurHashEntry = pCurHashEntry->pNext)
	{
		bFound = false;
		if (mCompareFunc == NULL)
		{
			if (memcmp(&pCurHashEntry->key, &key, sizeof(key)) == 0)
			{
				bFound = true;
			}
		}
		else if (mCompareFunc(pCurHashEntry->key, key))
		{
			bFound = true;
		}
		if (bFound)
		{
			pPrevHashEntry->pNext = pCurHashEntry->pNext;
			delete pCurHashEntry;

			return;
		}
	}

	char buffer[ERROR_STRING_BUFFER] = "";
	int key_len = sizeof(key);
	NAPtr<char> key_buf = new char[(key_len*2) + key_len + 1];
	GetHashKeyString((BYTE *)(&key), key_len, *key_buf);
	SAFE_SPRINTF_2(buffer, ERROR_STRING_BUFFER, 
		"Hash key doesn't exist. Key : 0x%s, Table Name : %s ", *key_buf, mHashTableName);
	
	THROW_K10_EXCEPTION(buffer,	MMAN_ERROR_EXECUTION);
}

// remove all hash table entries associated to the given object
template <class keyType, class objType>
void HashUtil<keyType, objType>::PurgeAssocEntries(const objType &obj)
{
	for (int ix = 0; ix < mHashTableSize; ix++)
	{
		if ((*mnapHashTable)[ix] != NULL)
		{
			HashEntry<keyType, objType> *pCurHashEntry = (*mnapHashTable)[ix];
			while (pCurHashEntry != NULL)
			{
				if (pCurHashEntry->obj == obj)
				{
					HashEntry<keyType, objType> *pTmp = pCurHashEntry->pNext;
					RemoveEntry(pCurHashEntry->key);
					pCurHashEntry = pTmp;
				}
				else
				{
					pCurHashEntry = pCurHashEntry->pNext;
				}
			}
		}
	}
}

// get binary key data in printable string ( Binary to string logic from AdminObjectImpl::PrintKey() function )
template <class keyType, class objType>
void HashUtil<keyType, objType>::GetHashKeyString(const BYTE *key, int key_len, char* key_buf)
{
	const BYTE *p = key;
	int cur = 0;
	int spot = 0;
	int len = key_len*2;

	// convert the key to a string
	while (cur < len)
	{
		for (int j = 4; j >= 0; j -= 4)
		{
			unsigned char  n = (*p >> j) & 0x0f;

			if (n < 0xa)
			{
				key_buf[spot++] = '0' + n; 
				cur++;
			}
			else
			{
				key_buf[spot++] = ('A' - 0xa) + n;
				cur++;
			}
		}

		if (cur != len)
		{	// skip the final ':' if cur == len
			key_buf[spot++] = ':';
		}

		p++;
	}
	key_buf[spot] = '\0';
}

#endif
