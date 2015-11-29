#ifndef _ZONE_UTILS_INTERFACE_H_
#define _ZONE_UTILS_INTERFACE_H_


//**********************************************************************
//.++
// CONSTANT:
//      The following constants are used to implement the TEA (Tiny
//      Encryption Algorithm).
//
// DESCRIPTION:
//      ZONE_UTILS_TEA_BLOCK_SIZE   Number of Bytes per pass of the 
//                                  Encryption Algorithm
//      ZONE_UTILS_TEA_ITERATIONS   Number of passes thru encryption loop
//      ZONE_UTILS_TEA_CRYPT_LEN    Number of words in the encryption key
//      ZONE_UTILS_TEA_DELTA        Random starting point for TEA
//      ZONE_UTILS_TEA_VALUE_1      Encryption seed values 
//      ZONE_UTILS_TEA_VALUE_2      Encryption seed values          
//      ZONE_UTILS_TEA_VALUE_3      Encryption seed values          
//      ZONE_UTILS_TEA_VALUE_4      Encryption seed values  
//
// REMARKS:
//      The Tiny Encryption Algorithm was developed by David Wheeler
//      and Roger Needham of the Computer laboratory at Cambridge 
//      University, England. The algorithm uses a large number of 
//      iterations and a 128-bit key value to provide reasonably secure
//      reversable data encryption/decryption. Clariion uses a version 
//      of the code modified from the source used by EMC Symetrix.
//
// REVISION HISTORY:
// 		02-Oct-06  kdewitt		Initial Port to 64 bit OS.
//      27-Jan-06  kdewitt      Modifying TEA to accept any size
//      26-Jan-06  kdewitt      Created.
//.--
//**********************************************************************
 
#define ZONE_UTILS_TEA_BLOCK_SIZE           8    
#define ZONE_UTILS_TEA_ITERATIONS           32   
#define ZONE_UTILS_TEA_CRYPT_LEN            4    
#define ZONE_UTILS_TEA_DELTA                0x9e3779b9  
#define ZONE_UTILS_TEA_VALUE_1              0x4e4f5454 
#define ZONE_UTILS_TEA_VALUE_2              0x68455661  
#define ZONE_UTILS_TEA_VALUE_3              0x6c756555  
#define ZONE_UTILS_TEA_VALUE_4              0x5365656b  

//**********************************************************************
//++
// DESCRIPTION:
//      This structure is used to hold the 128-bit Encryption/Decryption
//      key for the Tiny Encryption Algorithm (TEA).
//
// MEMBERS:
//
//  EncryptKey:       128-bit Encryption/Decryption key
//
//
// REMARKS:
//
//
// REVISION HISTORY:
//      26-Jan-06  kdewitt      Created.
// --
//**********************************************************************

typedef struct _ZONE_UTILS_TEA_CRYPT_KEY 
{
  	UINT_32 EncryptKey[ZONE_UTILS_TEA_CRYPT_LEN];
} ZONE_UTILS_TEA_CRYPT_KEY, *PZONE_UTILS_TEA_CRYPT_KEY;


//
//Function prototypes.
//

csx_bool_t __cdecl ZoneUtilsEncryptData(  
                        UCHAR                       *pPlainData, 
                        ZONE_UTILS_TEA_CRYPT_KEY    *pTEAKey, 
                        UCHAR                       *pCryptData, 
                        UINT32                     	size
                        );


csx_bool_t __cdecl ZoneUtilsDecryptData(   UCHAR *     pCryptData, 
                                        UCHAR *     pPlainData, 
                                        UINT32     	size, 
                                        UINT32 *    pKey
                                        );
    
//
// Given a zone in the database this function returns the next zone. To get the 
// first zone, specify prev zone as NULL. 
// Will return NULL if prev zone is set to the last zone.
//
PISCSI_ZONE_ENTRY_HEADER __cdecl ZoneUtilsGetNextZone(
    PISCSI_ZONE_DB_HEADER    pZoneDBHeader,
    PISCSI_ZONE_ENTRY_HEADER pPreviousZoneHeader);
        
//
// Given a path in a zone, this function returns the next path. To get the 
// first path, specify prev path as NULL. 
// Will return NULL if prev path is set to the last path.
//
PISCSI_ZONE_PATH __cdecl ZoneUtilsGetNextZonePath(PISCSI_ZONE_ENTRY_HEADER pZoneHeader, 
                                                  PISCSI_ZONE_PATH pPreviousZonePath );
        
//
// Given a auth in a zone, this function returns the next auth. To get the 
// first auth, specify prev auth as NULL. 
// Will return NULL if prev auth is set to the last auth.
//
PISCSI_ZONE_AUTH_INFO __cdecl ZoneUtilsGetNextAuthInfo(PISCSI_ZONE_ENTRY_HEADER pZoneHeader, 
                                                       PISCSI_ZONE_AUTH_INFO pPreviousAuthInfo);
    
//
// Given a zone and a path number, get the appropriate Auth information for that
// path.
//
HRESULT __cdecl ZoneUtilsGetAuthToUseForPath(PISCSI_ZONE_DB_HEADER pZoneDBHeader,
                                             PISCSI_ZONE_ENTRY_HEADER pZoneEntryHeader,
                                             USHORT PathNumber,
                                             PISCSI_ZONE_AUTH_INFO * ppZoneAuthInfo);


#endif //_ZONE_UTILS_INTERFACE_H_
        
