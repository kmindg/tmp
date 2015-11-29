/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 3.00.44 */
/* at Mon Dec 29 14:18:29 1997
 */
/* Compiler settings for K10AdminExport.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

const IID IID_IK10Admin = {0x1d0eb802,0x771b,0x11d1,{0x80,0x54,0x00,0x60,0xb0,0x1a,0x1c,0x6e}};


const IID LIBID_K10ADMINLib = {0x2fce15b4,0x771b,0x11d1,{0x80,0x54,0x00,0x60,0xb0,0x1a,0x1c,0x6e}};


const CLSID CLSID_K10Admin = {0x45112e52,0x771b,0x11d1,{0x80,0x54,0x00,0x60,0xb0,0x1a,0x1c,0x6e}};


#ifdef __cplusplus
}
#endif

