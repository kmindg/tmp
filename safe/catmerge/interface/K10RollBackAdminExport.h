//++
//
//  Module:
//
//      K10RollBackAdminExport.h
//
//  Description:
//
//      This file contains the exported definitions needed by software that wishes to interface
//      with the RollBack driver. This file is used by the RollBackAdminLib as well as Navisphere
//      to provide management capabilities of the Driver. This file is also used by the Driver
//      and all internal testing tools.
//
//  Notes:
//
//      None.
//
//  History:
//
//      9-Dec-98    PTM;    Created file.
//     26-Sep-00    BY      Task 1774 - Cleanup the header files
//
//--


#ifndef K10_ROLLBACK__ADMIN_EXPORT_H
#define K10_ROLLBACK__ADMIN_EXPORT_H


//
//  Define the atomic name of the K10RollBackAdminLib according to the K10 Admin documentation.
//

#define K10_ROLLBACK__LIBRARY_NAME                               "K10RollBackAdmin"


//
//  Define the IK10Admin database Id's that the RollBackAdminLib supports. Currently this
//  feature is not widely used so all commands sent to the library use the default id.
//

typedef enum _K10_ROLLBACK__DB_ID
{
    K10_ROLLBACK__DB_ID_MIN = 0,
    
    K10_ROLLBACK__DB_ID_DEFAULT = K10_ROLLBACK__DB_ID_MIN,
    
    K10_ROLLBACK__DB_ID_MAX = K10_ROLLBACK__DB_ID_DEFAULT

} K10_ROLLBACK__DB_ID;


//
//  Define the base value for errors returned to the Admin Interface.
//  Value was assigned by Dave Zeryck, Czar of Admin.
//

#define K10_ERROR_BASE_ROLLBACK_                                 0x71280000


//
//  Define the device type value.  Note that values used by Microsoft
//  Corporation are in the range 0-32767, and 32768-65535 are reserved for use
//  by customers.
//
#define FILE_DEVICE_ROLLBACK                        61679


//
//  NT defines legal custom function codes to be in the range of 2048-4095
//
#define ROLLBACK_STARTING_FUNCTION_CODE             2048


//++
//
//  IOCTLs used by the RollBackAdminLib to communicate with the driver
//
//__

#define IOCTL_ROLLBACK_GET_LU_STATUS (EMCPAL_IOCTL_CTL_CODE( FILE_DEVICE_ROLLBACK, \
                                                (ROLLBACK_STARTING_FUNCTION_CODE + 1), \
                                                EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS ))
 
 
//++
//
//  Input/Output buffers used by the RollBackAdminLib to communicate with the driver
//
//--

//
//  Input Buffer used by IOCTL_ROLLBACK_GET_LU_STATUS.
//
typedef struct _ROLLBACK_GET_LU_STATUS_IN_BUF
{
    K10_WWID WWN;

} ROLLBACK_GET_LU_STATUS_IN_BUF, *PROLLBACK_GET_LU_STATUS_IN_BUF;

//
//  Output Buffer used by IOCTL_ROLLBACK_GET_LU_STATUS
//
typedef struct _ROLLBACK_GET_LU_STATUS_OUT_BUF
{
    ULONGLONG ExportedLuSizeInBlocks;

} ROLLBACK_GET_LU_STATUS_OUT_BUF, *PROLLBACK_GET_LU_STATUS_OUT_BUF;


//++
//
//	Tag definitions have moved to interface\_Tags.h.
//
//	Do not define any new TAG_ literals in this file!
//
//--

#endif // K10_ROLLBACK__ADMIN_EXPORT_H
