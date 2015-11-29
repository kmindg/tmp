#ifndef __K10ANSIString__
#define __K10ANSIString__

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
//      K10AnsiString.h
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
};
#include "EmcPAL_Memory.h"
#include "EmcPAL_String.h"

// A K10AnsiString contains a set of methods for manipulating 
// a EMCPAL_ANSI_STRING. K10AnsiString adds nothing to the size of
// a EMCPAL_ANSI_STRING.
class K10AnsiString : public EMCPAL_ANSI_STRING {
public:
    K10AnsiString()  
    {
        MaximumLength =  0;
        Length = 0;
        Buffer = NULL;
    };

    K10AnsiString(const K10AnsiString & ref) 
    {
        MaximumLength =  0;
        Length = 0;
        Buffer = NULL;

        SetValue(ref);
    }

    K10AnsiString(const char * string)
    {
        MaximumLength =  0;
        Length = 0;
        Buffer = NULL;

        if (string == NULL) { return; };

        SetValue(string);
    }

    virtual ~K10AnsiString() 
    {
        if (Buffer && MaximumLength > 0) {
            delete [] Buffer;
        }
        Buffer = NULL;
    }

    void Display(void); // Defined in debug dlls.
    void Display(ULONG64 spObjAddress);

    // Sets a EMCPAL_ANSI_STRING to point to the constant
    // parameter.
    void SetConstantValue(csx_cpchar_t pStr) 
    {
        if (Buffer && MaximumLength > 0) {
            delete [] Buffer;
        }
        Buffer =  (csx_pchar_t)pStr; 
        MaximumLength = 0; // Nothing to free!
        Length = (USHORT)(csx_p_strlen(pStr) * sizeof(Buffer[0]));
    }

    // Sets a EMCPAL_ANSI_STRING to point to the parameter.
    bool SetValue(csx_cpchar_t pStr) 
    {
        EMCPAL_ANSI_STRING InputString;
        
        EmcpalInitAnsiString( &InputString,
                              pStr );

        return SetValue(InputString);
    }

    bool SetValue(const EMCPAL_ANSI_STRING & source)
    {
        if (MaximumLength == 0 || MaximumLength <= source.Length) {

            if (Buffer && MaximumLength > 0)  {
                delete [] Buffer;
            }
            Buffer = NULL;

            // One more for null
            MaximumLength = source.Length + sizeof(csx_char_t);

            if (MaximumLength < 14) MaximumLength = 16;

            Buffer = new CHAR[(MaximumLength / sizeof(csx_char_t)) + 1 ];
                        
            if (!Buffer) { 
                MaximumLength = 0;
                Length = 0;
                return false; 
            }
        }

        EmcpalCopyMemory(Buffer, source.Buffer, source.Length);
        Length = source.Length;
        Buffer[Length/sizeof(csx_char_t)] = 0; 

        return true;
    }

   // Returns buffer in pstring 
    bool GetCString(char *pString, ULONG Size) const
    {

        EMCPAL_ANSI_STRING LocalAnsiString;

        LocalAnsiString.Length = Length;
        LocalAnsiString.MaximumLength = MaximumLength;
        LocalAnsiString.Buffer = Buffer;

        // Copy ansi string to c string.
        // Only copy as much is necessary, Plus one for trailing null.
    
        memcpy(pString, LocalAnsiString.Buffer, __min((LONG)Size, LocalAnsiString.Length + 1 ) ); 

        return TRUE;
    }


    // @ Compare for Equal ; @return Type: true ; false     
    bool operator ==(const PCSTR other) const 
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

    // @ == Operator overload ; compare K10AnsiString
    bool operator ==(const K10AnsiString & input) 
    {
        if(input.Length != Length)
        {
            return false; 
        }
        return (memcmp(input.Buffer, Buffer, input.Length) ? false : true) ;
    }   

    // @ Compare for Not Equal ; @return Type: true ; false             
    bool operator !=(const PCSTR other) const { 
        return !(*this == other);
    }

    // @ Compare for not Equal
    bool operator !=(const K10AnsiString & input) const { 
        return !(*this == input);
    }

    // @ = Operator overload
    K10AnsiString & operator =(const K10AnsiString & source) 
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

    // @ Get Length of Characters in String + NULL
    ULONG NumChars() const 
    { 
        return (ULONG) ((Length+1)/sizeof(csx_char_t));  
    }

    // @ Get String Length
    ULONG NumBytes() const 
    { 
        return (ULONG) Length;
    }

    // @ Get Buffer
    operator PCSTR() const 
    { 
        return Buffer; 
    } 

    // @ Delete value
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

typedef K10AnsiString *  PK10AnsiString;
typedef const K10AnsiString *  PCK10AnsiString;


#endif  // __K10ANSIString__
