//
//
//      Copyright (C) EMC Corporation, 2004
//      All rights reserved.
//      Licensed material - Property of EMC Corporation.
//
//
//  Module:
//
//      K10CommonTracePrivate.h
//
//  Description:
//
//      This file contains the macros and infrastructure for supporting
//      the common trace macros.
//
//  Notes:
//
//
//  History:
//
//     11/04/04;    ALT     Created
//
//

#ifndef K10_COMMON_TRACE_PRIVATE_H
#define K10_COMMON_TRACE_PRIVATE_H

//
// Define the printf() style routine that is used to display the information. 
//

#define K10_COMMON_TRACE_DISPLAY_ROUTINE KvPrint

//
// Define the identification stamp that we place on all displayed strings.
//

#define K10_COMMON_TRACE_IDENTIFICATION_STAMP   "K10CT:"

//
// Define a few helper macros to help convert constant #defines to strings.
//

#define X_STRINGIZE(m_str) #m_str
#define STRINGIZE(m_str) X_STRINGIZE(m_str)

//
// Define our private version of the constructed IOCTLs.  Using these constants
// allows us to "stringize" the correct string instead of the CTL_CODE()
// macro guts.
//

#define K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_TRESPASS_QUERY                             0x72254
#define K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS                    0x72258
#define K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_TRESPASS_EXECUTE                           0x72248
#define K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_DCA_QUERY                                  0x72220
#define K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_READ_BUFFER                                0x72224
#define K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_WRITE_BUFFER                               0x723e8
#define K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_REGISTER_FOR_CHANGE_NOTIFICATION           0x7225c
#define K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_MODIFY_CAPACITY                            0x72260
#define K10_COMMON_TRACE_PRIVATE_IOCTL_DISK_GET_DRIVE_GEOMETRY                          0x70000
#define K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_GET_RAID_INFO                              0x7222c
#define K10_COMMON_TRACE_PRIVATE_IOCTL_DISK_VERIFY                                      0x70014
#define K10_COMMON_TRACE_PRIVATE_IOCTL_DISK_IS_WRITABLE                                 0x70024
#ifdef ALAMOSA_WINDOWS_ENV
#define K10_COMMON_TRACE_PRIVATE_IOCTL_DISK_GET_PARTITION_INFO                          0x74004
#else
#define K10_COMMON_TRACE_PRIVATE_IOCTL_DISK_GET_PARTITION_INFO                          0x70004
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - OK */

#define K10_COMMON_TRACE_PRIVATE_IOCTL_K10ATTACH_BIND                                   0x80da2814
#define K10_COMMON_TRACE_PRIVATE_IOCTL_K10ATTACH_UNBIND                                 0x80da2818

//
// Define the strings for each IOCTL that we will display.
//

#define K10_COMMON_TRACE_IRP_MJ_CREATE_STR \
    "IMCR[" STRINGIZE(IRP_MJ_CREATE) "]"
    
#define K10_COMMON_TRACE_IRP_MJ_CLOSE_STR \
    "IMCL[" STRINGIZE(IRP_MJ_CLOSE) "]"

#define K10_COMMON_TRACE_IRP_MJ_READ_STR \
    "IMCL[" STRINGIZE(IRP_MJ_READ) "]"

#define K10_COMMON_TRACE_IRP_MJ_WRITE_STR \
    "IMCL[" STRINGIZE(IRP_MJ_WRITE) "]"

#define K10_COMMON_TRACE_FLARE_DCA_READ_STR \
    "IMCL[" STRINGIZE(IRP_MJ_INTERNAL_DEVICE_CONTROL) ":" STRINGIZE(FLARE_DCA_READ) "]"

#define K10_COMMON_TRACE_FLARE_DCA_WRITE_STR \
    "IMCL[" STRINGIZE(IRP_MJ_INTERNAL_DEVICE_CONTROL) ":" STRINGIZE(FLARE_DCA_WRITE) "]"


#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_QUERY_STR \
    "IFTQ[" STRINGIZE(K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_TRESPASS_QUERY) "]"

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_STR \
    "IFTL[" STRINGIZE(K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS) "]"

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_STR \
    "IFTE[" STRINGIZE(K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_TRESPASS_EXECUTE) "]"

#define K10_COMMON_TRACE_IOCTL_FLARE_DCA_QUERY_STR \
    "IFDQ[" STRINGIZE(K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_DCA_QUERY) "]"

#define K10_COMMON_TRACE_IOCTL_FLARE_READ_BUFFER_STR \
    "IFRB[" STRINGIZE(K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_READ_BUFFER) "]"

#define K10_COMMON_TRACE_IOCTL_FLARE_WRITE_BUFFER_STR \
    "IFWB[" STRINGIZE(K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_WRITE_BUFFER) "]"

#define K10_COMMON_TRACE_IOCTL_FLARE_REGISTER_FOR_CHANGE_NOTIFICATION_STR \
    "IFRC[" STRINGIZE(K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_REGISTER_FOR_CHANGE_NOTIFICATION) "]"

#define K10_COMMON_TRACE_IOCTL_FLARE_MODIFY_CAPACITY_STR \
    "IFMC[" STRINGIZE(K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_MODIFY_CAPACITY) "]"

#define K10_COMMON_TRACE_IOCTL_DISK_GET_DRIVE_GEOMETRY_STR \
    "IDGD[" STRINGIZE(K10_COMMON_TRACE_PRIVATE_IOCTL_DISK_GET_DRIVE_GEOMETRY) "]"

#define K10_COMMON_TRACE_IOCTL_FLARE_GET_RAID_INFO_STR \
    "IFGR[" STRINGIZE(K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_GET_RAID_INFO) "]"

#define K10_COMMON_TRACE_IOCTL_DISK_VERIFY_STR \
    "IDVE[" STRINGIZE(K10_COMMON_TRACE_PRIVATE_IOCTL_DISK_VERIFY) "]"

#define K10_COMMON_TRACE_IOCTL_DISK_IS_WRITABLE_STR \
    "IDIW[" STRINGIZE(K10_COMMON_TRACE_PRIVATE_IOCTL_DISK_IS_WRITABLE) "]"

#define K10_COMMON_TRACE_IOCTL_DISK_GET_PARTITION_INFO_STR \
    "IDGP[" STRINGIZE(K10_COMMON_TRACE_PRIVATE_IOCTL_DISK_GET_PARTITION_INFO) "]"

#define K10_COMMON_TRACE_IOCTL_K10ATTACH_BIND_STR \
    "IKAB[" STRINGIZE(K10_COMMON_TRACE_PRIVATE_IOCTL_K10ATTACH_BIND) "]"

#define K10_COMMON_TRACE_IOCTL_K10ATTACH_UNBIND_STR \
    "IKAU[" STRINGIZE(K10_COMMON_TRACE_PRIVATE_IOCTL_K10ATTACH_UNBIND) "]"    


//
// Since we have to define our own version of the value for these IOCTLs do
// some comparisons to make sure that they are the values that we expect them
// to be.  If something is broken, then fail the compile.
//

#if K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_TRESPASS_QUERY != \
        IOCTL_FLARE_TRESPASS_QUERY
#error "IOCTL_FLARE_TRESPASS_QUERY value changed!"
#endif

#if K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS != \
        IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS
#error "IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS value changed!"
#endif

#if K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_TRESPASS_EXECUTE != \
        IOCTL_FLARE_TRESPASS_EXECUTE
#error "IOCTL_FLARE_TRESPASS_EXECUTE value changed!"
#endif

#if K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_DCA_QUERY != \
        IOCTL_FLARE_DCA_QUERY
#error "IOCTL_FLARE_DCA_QUERY value changed!"
#endif

#if K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_READ_BUFFER != \
        IOCTL_FLARE_READ_BUFFER
#error "IOCTL_FLARE_READ_BUFFER value changed!"
#endif

#if K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_WRITE_BUFFER != \
        IOCTL_FLARE_WRITE_BUFFER
#error "IOCTL_FLARE_WRITE_BUFFER value changed!"
#endif

#if K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_REGISTER_FOR_CHANGE_NOTIFICATION != \
        IOCTL_FLARE_REGISTER_FOR_CHANGE_NOTIFICATION
#error "IOCTL_FLARE_REGISTER_FOR_CHANGE_NOTIFICATION value changed!"
#endif

#if K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_MODIFY_CAPACITY != \
        IOCTL_FLARE_MODIFY_CAPACITY
#error "IOCTL_FLARE_MODIFY_CAPACITY value changed!"
#endif

#if K10_COMMON_TRACE_PRIVATE_IOCTL_DISK_GET_DRIVE_GEOMETRY != \
        IOCTL_DISK_GET_DRIVE_GEOMETRY
#error "IOCTL_DISK_GET_DRIVE_GEOMETRY value changed!"
#endif

#if K10_COMMON_TRACE_PRIVATE_IOCTL_FLARE_GET_RAID_INFO != \
        IOCTL_FLARE_GET_RAID_INFO
#error "IOCTL_FLARE_GET_RAID_INFO value changed!"
#endif

#if K10_COMMON_TRACE_PRIVATE_IOCTL_DISK_VERIFY != \
        IOCTL_DISK_VERIFY
#error "IOCTL_DISK_VERIFY value changed!"
#endif

#if K10_COMMON_TRACE_PRIVATE_IOCTL_DISK_IS_WRITABLE != \
        IOCTL_DISK_IS_WRITABLE
#error "IOCTL_DISK_IS_WRITABLE value changed!"
#endif

#if K10_COMMON_TRACE_PRIVATE_IOCTL_DISK_GET_PARTITION_INFO != \
        IOCTL_DISK_GET_PARTITION_INFO
#error "IOCTL_DISK_GET_PARTITION_INFO value changed!"
#endif


//
// The following defines a special value to indicate that the parameter
// is not used.
//

#define K10_COMMON_TRACE_UNUSED_PARAMETER       (0xffffffff)

//
// Private Macro: K10_COMMON_TRACE_RECEPTION_OPEN_COUNT
//
// Description: Used upon operation reception.
//
// Parameters:  m_operation
//                  The root operation (IRP_MJ_CREATE / IRP_MJ_CLOSE).
//
//              m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the device name.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_lun
//                  The LUN that corresponds to this device.  If it is unknown,
//                  callers should use K10_COMMON_TRACE_UNUSED_PARAMETER.
//
//              m_open_count
//                  The current open count for the device.
//
//              m_total_open_count
//                  The current total open count for the devices in the device
//                  graph managed by this component.  If unused, then callers
//                  should use K10_COMMON_TRACE_UNUSED_PARAMETER.
//
#define K10_COMMON_TRACE_RECEPTION_OPEN_COUNT( m_operation, \
                                               m_component, \
                                               m_device_name, \
                                               m_device_object, \
                                               m_lun, \
                                               m_open_count, \
                                               m_total_open_count ) \
    if ( m_total_open_count == K10_COMMON_TRACE_UNUSED_PARAMETER && \
         m_lun == K10_COMMON_TRACE_UNUSED_PARAMETER ) \
    { \
        K10_COMMON_TRACE_DISPLAY_ROUTINE( \
            K10_COMMON_TRACE_IDENTIFICATION_STAMP \
            m_component \
            m_operation \
            " %s 0x%x Open %d\n", \
            m_device_name, m_device_object, m_open_count ); \
    } \
    else if ( m_open_count == K10_COMMON_TRACE_UNUSED_PARAMETER && \
              m_lun == K10_COMMON_TRACE_UNUSED_PARAMETER ) \
    { \
        K10_COMMON_TRACE_DISPLAY_ROUTINE( \
            K10_COMMON_TRACE_IDENTIFICATION_STAMP \
            m_component \
            m_operation \
            " %s 0x%x TOpen %d\n", \
            m_device_name, m_device_object, m_total_open_count ); \
    } \
    else if ( m_total_open_count == K10_COMMON_TRACE_UNUSED_PARAMETER && \
              m_lun != K10_COMMON_TRACE_UNUSED_PARAMETER ) \
    { \
        K10_COMMON_TRACE_DISPLAY_ROUTINE( \
            K10_COMMON_TRACE_IDENTIFICATION_STAMP \
            m_component \
            m_operation \
            " %s 0x%x LUN %d Open %d\n", \
            m_device_name, m_device_object, m_lun, m_open_count ); \
    } \
    else if ( m_open_count == K10_COMMON_TRACE_UNUSED_PARAMETER && \
              m_lun != K10_COMMON_TRACE_UNUSED_PARAMETER ) \
    { \
        K10_COMMON_TRACE_DISPLAY_ROUTINE( \
            K10_COMMON_TRACE_IDENTIFICATION_STAMP \
            m_component \
            m_operation \
            " %s 0x%x LUN %d TOpen %d\n", \
            m_device_name, m_device_object, m_lun, m_total_open_count ); \
    } \
    else \
    { \
        K10_COMMON_TRACE_DISPLAY_ROUTINE( \
            K10_COMMON_TRACE_IDENTIFICATION_STAMP \
            m_component \
            m_operation \
            " %s 0x%x Error Invalid Parameters!!!!\n", \
            m_device_name, m_device_object ); \
    }


//
// Private Macro: K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM
//
// Description: Used to display detailed information with the device.
//
// Parameters:  m_operation
//                  The root operation (IRP_MJ_CREATE / IRP_MJ_CLOSE).
//
//              m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the device name.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_lun
//                  The LUN that corresponds to this device.  If it is unknown,
//                  callers should use K10_COMMON_TRACE_UNUSED_PARAMETER.
//
//              m_format_string
//                  A string used to specify the printf style formatting.
//
//              m_parameter
//                  The parameter used to display based on m_format_string.
//
#define K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( m_operation, \
                                                    m_component, \
                                                    m_device_name, \
                                                    m_device_object, \
                                                    m_lun, \
                                                    m_format_string, \
                                                    m_parameter ) \
    if ( m_lun == K10_COMMON_TRACE_UNUSED_PARAMETER ) \
    { \
        K10_COMMON_TRACE_DISPLAY_ROUTINE( \
            K10_COMMON_TRACE_IDENTIFICATION_STAMP \
            m_component \
            m_operation \
            " %s 0x%x " m_format_string "\n", \
            m_device_name, m_device_object, m_parameter ); \
    } \
    else if ( m_lun != K10_COMMON_TRACE_UNUSED_PARAMETER ) \
    { \
        K10_COMMON_TRACE_DISPLAY_ROUTINE( \
            K10_COMMON_TRACE_IDENTIFICATION_STAMP \
            m_component \
            m_operation \
            " %s 0x%x LUN %d " m_format_string "\n", \
            m_device_name, m_device_object, m_lun, m_parameter ); \
    }

#endif // K10_COMMON_TRACE_PRIVATE_H

