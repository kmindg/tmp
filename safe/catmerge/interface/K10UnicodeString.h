#ifndef __K10UniCodeString__
#define __K10UniCodeString__

/************************************************************************
 *
 *  Copyright (c) 2002, 2005-2006 EMC Corporation.
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

//++
// File Name:
//      K10UnicodeString.h
//
// Contents:
//      Defines various utility functions for manipulating IRPs in C++.
//
// Revision History:
//--
#include "cppcmm.h"



extern "C" {
#include "ktrace.h"
#include <stdlib.h>
#include <string.h>
#ifndef ALAMOSA_WINDOWS_ENV
#include <wchar.h>
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE */
};
#include "EmcPAL_Memory.h"
#include "EmcPAL_String.h"
// A K10UnicodeString contains a set of methods for manipulating 
// a EMCPAL_UNICODE_STRING. K10UnicodeString adds nothing to the size of
// a EMCPAL_UNICODE_STRING.
class K10UnicodeString : public EMCPAL_UNICODE_STRING {
public:
    K10UnicodeString()  
	{
        MaximumLength =  0;
        Length = 0;
        Buffer = NULL;
    };

	K10UnicodeString(const K10UnicodeString & ref) 
    {
        MaximumLength =  0;
        Length = 0;
        Buffer = NULL;

        SetValue(ref);
    }
    
    K10UnicodeString(const char * string)
    {
        MaximumLength =  0;
        Length = 0;
        Buffer = NULL;

        if (string == NULL) { return; };

        SetValue(string);
    }


    virtual ~K10UnicodeString() 
	{
        if (Buffer && MaximumLength > 0) {
		    delete [] Buffer;
        }
		Buffer = NULL;
    }

	void Display(void); // Defined in debug dlls.
	void Display(ULONG64 spObjAddress);

	// Sets a EMCPAL_UNICODE_STRING to point to the constant
	// parameter.
	void SetConstantValue(const PCWSTR pWstr) 
	{
        if (Buffer && MaximumLength > 0) {
		    delete [] Buffer;
        }
		Buffer = (PWSTR) pWstr; 
		MaximumLength = 0; // Nothing to free!
		Length = (USHORT)(wcslen(pWstr) * sizeof(Buffer[0]));
    }
    
    bool SetValue(const PWCHAR pWstr) 
	{
        EMCPAL_UNICODE_STRING InputString;
        
        EmcpalInitUnicodeString( &InputString,
                              pWstr );

        return SetValue(InputString);        
    }

    bool SetValue(const EMCPAL_UNICODE_STRING & source)
    {
        if (MaximumLength == 0 || MaximumLength <= source.Length) {

            if (Buffer && MaximumLength > 0)  {
                delete [] Buffer;
			}
			Buffer = NULL;


            // One more for null
            MaximumLength = source.Length + sizeof(WCHAR);

            if (MaximumLength < 14) MaximumLength = 16;

            Buffer = new WCHAR[(MaximumLength / sizeof(WCHAR)) + 1 ];
            
            
            if (!Buffer) { 
                MaximumLength = 0;
                Length = 0;
                return false; 
            }
            
        }
        EmcpalCopyMemory(Buffer, source.Buffer, source.Length);
        Length = source.Length;
        Buffer[Length/sizeof(WCHAR)] = 0; 
        return true;
    }


    bool SetValue(const char * source)
    {
        USHORT   sourceBytes = (USHORT) strlen(source);
        USHORT   neededBytes = (USHORT)(sourceBytes*sizeof(WCHAR)+sizeof(WCHAR));
        if (MaximumLength == 0 || MaximumLength < neededBytes) {

            if (Buffer && MaximumLength > 0)  {
                delete [] Buffer;
			}
			Buffer = NULL;

            MaximumLength =  neededBytes;

            if (MaximumLength < 14) MaximumLength = 16;

            Buffer = new WCHAR[ (MaximumLength / 2) + 1 ];
            
            
            if (!Buffer) { 
                MaximumLength = 0;
                Length = 0;
                return false; 
            }
            
        }
        for (ULONG i=0; i < sourceBytes; i ++) {
             Buffer[i] = source[i];
        }
        Length = sourceBytes*sizeof(WCHAR);
        Buffer[sourceBytes] = 0; 
        return true;
    }


     
    bool operator ==(const PCWSTR other) const 
	{
        ULONG i; 
        for (i = 0; i < NumChars(); i++) {
            if (other[i] != Buffer[i]) {
                return false;
            }
        }

        // Used all the characters in *this, make sure that 
        // all characters in "other" are used.
        return other[i] == 0; 
    } 

    bool operator !=(const PCWSTR other) const { 
        return !(*this == other);
    }

    K10UnicodeString & operator =(const K10UnicodeString & source) 
    {

        Length = 0;
		if (Buffer && MaximumLength > 0)  {
			delete [] Buffer;
		}
		Buffer = NULL;
		MaximumLength =  0;

        SetValue(source);
        return *this;
    }



	bool cStringToUnicode(EMCPAL_PCSZ pString)
	{
        EMCPAL_UNICODE_STRING LocalUnicodeString;
        EMCPAL_STATUS Status;
        EMCPAL_ANSI_STRING AnsiString;

		if (Buffer && MaximumLength > 0)  {
			delete [] Buffer;
		}
		/*
		 * The below code (this)->setValue( LocalUnicodeString) will only 
		 * allocate a buffer when MaximumLength ==0 or the final length
		 * is less than the current max.
		 *
		 * A properly initialized empty unicodestring 
		 * will have Buffer = NULL and MaximumLength = 0
		 */
		MaximumLength = 0;
		Buffer = NULL;

        // First copy the cstring over to the ansi string.

        EmcpalInitAnsiString(&AnsiString, pString);
    
        // Make the ansi string into a UNICODE String

        Status = EmcpalAnsiStringToUnicodeString(&LocalUnicodeString,
                                              &AnsiString,
                                              TRUE);

        if (!EMCPAL_IS_SUCCESS(Status))
        {        
            return FALSE;
        }
    
        // Next set the value in the K10UnicodeString from the UNICODE string
        bool mRet;
        
        mRet = (this)->SetValue( LocalUnicodeString );

        // Free the LocalUnicodeString
    
#ifndef EMCPAL_USE_CSX_STRINGS
        EmcpalFreeUnicodeString(&LocalUnicodeString);
#else
        csx_rt_mx_nt_RtlFreeUnicodeString(&LocalUnicodeString);
#endif /* EMCPAL_USE_CSX_STRINGS */
        return mRet;
    }


    bool UnicodeToCString(char *pString, ULONG Size) const
	{
            
        EMCPAL_STATUS Status;
        EMCPAL_ANSI_STRING LocalAnsiString;

        EMCPAL_UNICODE_STRING LocalUnicode;

        LocalUnicode.Length = Length;
        LocalUnicode.MaximumLength = MaximumLength;
        LocalUnicode.Buffer = Buffer;        

        // Convert unicode string to ansi string.
    
#ifndef EMCPAL_USE_CSX_STRINGS
        Status = EmcpalUnicodeStringToAnsiString(&LocalAnsiString,
                                              &LocalUnicode,  
                                              TRUE); 
#else
        Status = csx_rt_mx_nt_RtlUnicodeStringToAnsiString(&LocalAnsiString,
                                              &LocalUnicode,  
                                              TRUE); 
#endif /* EMCPAL_USE_CSX_STRINGS */
        if (!EMCPAL_IS_SUCCESS(Status))
        {        
            return FALSE;
        }
    
        // Copy ansi string to c string.
        // Only copy as much is necessary, Plus one for trailing null.
    
        memcpy(pString, LocalAnsiString.Buffer, __min((LONG)Size, LocalAnsiString.Length + 1 ) ); 

        // Free the LocalAnsiString
    
#ifndef EMCPAL_USE_CSX_STRINGS
        EmcpalFreeAnsiString(&LocalAnsiString);
#else
        csx_rt_mx_nt_RtlFreeAnsiString(&LocalAnsiString);
#endif /* EMCPAL_USE_CSX_STRINGS */

        return TRUE;
    }

    ULONG UnicodeToAnsiString(EMCPAL_PANSI_STRING LocalAnsiString) 
	{
            
        EMCPAL_STATUS Status;

        EMCPAL_UNICODE_STRING LocalUnicode;
        
        LocalUnicode.Length = Length;
        LocalUnicode.MaximumLength = MaximumLength;
        LocalUnicode.Buffer = Buffer;        

        // Convert unicode string to ansi string.
    
#ifndef EMCPAL_USE_CSX_STRINGS
        Status = EmcpalUnicodeStringToAnsiString(LocalAnsiString,
                                              &LocalUnicode,  
                                              TRUE); 
#else
        Status = csx_rt_mx_nt_RtlUnicodeStringToAnsiString(LocalAnsiString,
                                              &LocalUnicode,  
                                              TRUE); 
#endif /* EMCPAL_USE_CSX_STRINGS */
        if (!EMCPAL_IS_SUCCESS(Status))
        {        
            return FALSE;
        }
    
        return 0;
    }

    ULONG NumChars() const 
	{ 
		return (ULONG) ((Length+1)/sizeof(WCHAR));  
	}

    ULONG NumBytes() const 
	{ 
		return (ULONG) Length;
	}

    operator PCWSTR() const 
	{ 
		return Buffer; 
	} 
    bool operator ==(const K10UnicodeString & input) 
    {
        if(input.Length != Length)
        {
            return FALSE; 
        }
        return (memcmp(input.Buffer, Buffer, input.Length) ? FALSE : TRUE) ;
	}   

    bool operator !=(const K10UnicodeString & input) const { 
        return !(*this == input);
    }



    VOID DeleteValue() 
	{
		if (Buffer && MaximumLength > 0)  {
			delete [] Buffer;
		}
		Buffer = NULL;
        MaximumLength = Length = 0;
    }


	private:


};

typedef K10UnicodeString *  PK10UnicodeString;
typedef const K10UnicodeString *  PCK10UnicodeString;


#endif  // __K10UniCodeString__

